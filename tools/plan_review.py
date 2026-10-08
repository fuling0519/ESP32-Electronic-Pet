#!/usr/bin/env python3
"""Bind a human/agent project-plan review to the exact staged Git contents."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

PLAN = "docs/project-plan.md"
FIELDS = ("scope", "descriptions", "implementation", "automated_validation",
          "hardware_acceptance", "next_steps", "plan_decision")
GUIDANCE = (
    "Project-plan review required. Read docs/commit-plan-review.md. "
    "Compare the staged diff with the relevant descriptions throughout "
    "docs/project-plan.md; update stale rules, progress, validation and next steps. "
    "Separate implemented code, automated validation and hardware acceptance. "
    "If no update is needed, explain why. Stage required documentation, then "
    "record a review using tools/plan_review.py record --report <report.json>. "
    "Do not create a receipt without actually reviewing the plan."
)


def git(root, *args):
    result = subprocess.run(["git", "-C", str(root), *args], capture_output=True)
    if result.returncode:
        raise ValueError(result.stderr.decode("utf-8", errors="replace").strip())
    return result.stdout


def repository(cwd):
    return Path(git(cwd, "rev-parse", "--show-toplevel").decode().strip())


def receipt_path(root):
    # Local to this worktree, ignored and never added to a commit.
    return root / ".plan-review" / "review.json"


def snapshot(root):
    index = git(root, "ls-files", "--stage", "-z")
    if not git(root, "diff", "--cached", "--name-only", "--no-ext-diff").strip():
        raise ValueError("No staged changes. Stage the intended commit first.")
    plan = git(root, "show", ":" + PLAN)
    if git(root, "diff", "--name-only", "--no-ext-diff", "--", PLAN).strip():
        raise ValueError("Project plan has unstaged changes; stage its reviewed version first.")
    head = subprocess.run(["git", "-C", str(root), "rev-parse", "--verify", "HEAD"],
                          capture_output=True)
    if head.returncode and git(root, "rev-parse", "--is-inside-work-tree").strip() != b"true":
        raise ValueError("Cannot determine Git state.")
    return {"index_sha256": hashlib.sha256(index).hexdigest(),
            "head": head.stdout.decode().strip() if head.returncode == 0 else "unborn",
            "plan_sha256": hashlib.sha256(plan).hexdigest()}


def validate_report(report, root):
    if not isinstance(report, dict):
        raise ValueError("Review report must be an object.")
    for key in FIELDS:
        if not isinstance(report.get(key), str) or not report[key].strip():
            raise ValueError("Review report needs a non-empty explanation: " + key)
    example = json.loads((Path(__file__).parent / "plan-review-report.example.json").read_text(encoding="utf-8"))
    if any(report[key] == example[key] for key in FIELDS):
        raise ValueError("Replace all example explanations with actual review results.")
    if report.get("plan_status") not in ("updated", "unchanged"):
        raise ValueError("plan_status must be updated or unchanged.")
    changed = bool(git(root, "diff", "--cached", "--name-only", "--no-ext-diff", "--", PLAN).strip())
    if changed != (report["plan_status"] == "updated"):
        raise ValueError("plan_status does not match the staged project-plan diff.")


def check(root):
    current = snapshot(root)
    path = receipt_path(root)
    if not path.is_file():
        raise ValueError("Missing review receipt. " + GUIDANCE)
    saved = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(saved, dict):
        raise ValueError("Review receipt must be an object. " + GUIDANCE)
    if saved.get("snapshot") != current:
        raise ValueError("Staged contents, HEAD or project plan changed after review. " + GUIDANCE)
    validate_report(saved.get("report"), root)


def hook():
    payload = json.load(sys.stdin)
    tool_input = payload.get("tool_input", {})
    command = tool_input.get("command", tool_input.get("cmd", "")) if isinstance(tool_input, dict) else ""
    # Deliberately conservative: wrappers and compound commands may also match.
    # The Git hook performs the authoritative check after Git builds the index.
    if not (re.search(r"\bgit(?:\.exe)?\b", command, re.I) and
            re.search(r"\bcommit\b", command, re.I)):
        return
    try:
        root = repository(payload.get("cwd", "."))
        check(root)
    except (ValueError, OSError, json.JSONDecodeError) as exc:
        print(json.dumps({"hookSpecificOutput": {
            "hookEventName": "PreToolUse", "permissionDecision": "deny",
            "permissionDecisionReason": str(exc)}}, ensure_ascii=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("record", "check", "hook"))
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.action == "hook":
        hook()
        return
    root = repository(Path.cwd())
    if args.action == "record":
        if args.report is None:
            parser.error("record requires --report")
        report = json.loads(args.report.read_text(encoding="utf-8-sig"))
        validate_report(report, root)
        current = snapshot(root)
        path = receipt_path(root)
        path.parent.mkdir(exist_ok=True)
        path.write_text(json.dumps({"snapshot": current, "report": report},
                                   ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print("Review recorded for the current staged contents.")
    else:
        check(root)
        print("Project-plan review passed.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, json.JSONDecodeError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(2)
