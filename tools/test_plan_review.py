#!/usr/bin/env python3
"""Integration tests using disposable Git repositories, including real commits."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[1]


class ReviewTests(unittest.TestCase):
    def setUp(self):
        scratch = SOURCE / ".plan-review"
        scratch.mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix="test-", dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.env = os.environ.copy()
        # Do not inherit a caller's temporary Git index or repository.
        for key in list(self.env):
            if key.startswith("GIT_"):
                self.env.pop(key)
        self.run_cmd("git", "init", "-q")
        self.git("config", "user.name", "Hook Test")
        self.git("config", "user.email", "hook-test@example.invalid")
        self.git("config", "commit.gpgsign", "false")
        self.git("config", "core.autocrlf", "false")
        for directory in ("tools", "docs", ".githooks"):
            (self.root / directory).mkdir()
        for name in ("plan_review.py", "plan-review-report.example.json"):
            shutil.copyfile(SOURCE / "tools" / name, self.root / "tools" / name)
        shutil.copyfile(SOURCE / ".githooks/pre-commit", self.root / ".githooks/pre-commit")
        (self.root / ".githooks/pre-commit").chmod(0o755)
        self.write("docs/project-plan.md", "# Plan\nImplemented; automated checks pending; board acceptance pending.\n")
        self.write("feature.txt", "initial\n")
        self.write(".gitignore", ".plan-review/\n")
        self.git("add", ".")
        self.git("-c", "core.hooksPath=", "commit", "-qm", "baseline")
        self.git("config", "core.hooksPath", ".githooks")
        self.write("feature.txt", "changed\n")
        self.git("add", "feature.txt")

    def run_cmd(self, *args, input=None, ok=True):
        result = subprocess.run(args, cwd=self.root, env=self.env, input=input,
                                capture_output=True, text=True, encoding="utf-8")
        if ok:
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
        return result

    def git(self, *args, **kwargs):
        return self.run_cmd("git", *args, **kwargs)

    def write(self, name, text):
        (self.root / name).write_text(text, encoding="utf-8")

    def report(self, status="unchanged"):
        value = {key: "Reviewed actual staged feature and plan: " + key for key in
                 ("scope", "descriptions", "implementation", "automated_validation",
                  "hardware_acceptance", "next_steps", "plan_decision")}
        value["plan_status"] = status
        self.write("report.json", json.dumps(value))

    def review(self, status="unchanged"):
        self.report(status)
        self.run_cmd(sys.executable, "tools/plan_review.py", "record", "--report", "report.json")

    def check(self, expected=0):
        result = self.run_cmd(sys.executable, "tools/plan_review.py", "check", ok=False)
        self.assertEqual(result.returncode, expected, result.stderr)

    def test_real_commit_blocks_without_review_then_allows_and_expires(self):
        before = self.git("rev-parse", "HEAD").stdout
        result = self.git("commit", "-qm", "missing review", ok=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.git("rev-parse", "HEAD").stdout, before)
        self.review()
        self.git("commit", "-qm", "reviewed feature")
        self.write("feature.txt", "next change\n")
        self.git("add", "feature.txt")
        self.check(2)

    def test_staging_change_invalidates_receipt(self):
        self.review()
        self.write("feature.txt", "different staged contents\n")
        self.git("add", "feature.txt")
        self.check(2)

    def test_unstaged_feature_does_not_change_commit(self):
        self.review()
        self.write("feature.txt", "unrelated later work\n")
        self.check()

    def test_unstaged_plan_blocks(self):
        self.review()
        self.write("docs/project-plan.md", "# Changed plan\n")
        self.check(2)

    def test_updated_plan_must_be_staged_with_feature(self):
        self.write("docs/project-plan.md", "# Updated plan\nNew feature; tests passed; board pending.\n")
        self.git("add", "docs/project-plan.md")
        self.report("unchanged")
        result = self.run_cmd(sys.executable, "tools/plan_review.py", "record",
                              "--report", "report.json", ok=False)
        self.assertEqual(result.returncode, 2)
        self.review("updated")
        self.git("commit", "-qm", "feature and plan")

    def test_commit_all_cannot_reuse_stale_review(self):
        self.review()
        self.write("feature.txt", "commit -a would include this\n")
        result = self.git("commit", "-aqm", "changed after review", ok=False)
        self.assertNotEqual(result.returncode, 0)

    def test_hook_denies_commit_but_allows_reads(self):
        def invoke(command):
            payload = {"cwd": str(self.root), "tool_input": {"command": command}}
            return self.run_cmd(sys.executable, "tools/plan_review.py", "hook",
                                input=json.dumps(payload)).stdout
        self.assertEqual(invoke("git status --short"), "")
        decision = json.loads(invoke('git commit -m "feature"'))
        self.assertEqual(decision["hookSpecificOutput"]["permissionDecision"], "deny")
        self.review()
        self.assertEqual(invoke('git commit -m "feature"'), "")

    def test_corrupt_receipt_and_empty_report_block(self):
        self.review()
        self.write(".plan-review/review.json", "{broken")
        self.check(2)
        self.write("report.json", "{}")
        result = self.run_cmd(sys.executable, "tools/plan_review.py", "record",
                              "--report", "report.json", ok=False)
        self.assertEqual(result.returncode, 2)

    def test_example_report_is_not_evidence(self):
        result = self.run_cmd(sys.executable, "tools/plan_review.py", "record", "--report",
                              "tools/plan-review-report.example.json", ok=False)
        self.assertEqual(result.returncode, 2)

    def test_windows_line_endings(self):
        self.git("config", "core.autocrlf", "true")
        path = self.root / "docs/project-plan.md"
        path.write_bytes(path.read_bytes().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n"))
        self.git("add", "docs/project-plan.md")
        self.review()
        self.check()

    def test_configured_launcher_from_subdirectory(self):
        config = json.loads((SOURCE / ".codex/hooks.json").read_text(encoding="utf-8"))
        command = config["hooks"]["PreToolUse"][0]["hooks"][0]["command"]
        payload = json.dumps({"cwd": str(self.root / "docs"),
                              "tool_input": {"command": "git commit -m feature"}})
        result = subprocess.run(command, shell=True, cwd=self.root / "docs", env=self.env,
                                input=payload, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        decision = json.loads(result.stdout)
        self.assertEqual(decision["hookSpecificOutput"]["permissionDecision"], "deny")
        if os.name == "nt":
            result = subprocess.run(["powershell.exe", "-NoProfile", "-Command", command],
                                    cwd=self.root / "docs", env=self.env, input=payload,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)["hookSpecificOutput"]["permissionDecision"], "deny")


if __name__ == "__main__":
    unittest.main(verbosity=2)
