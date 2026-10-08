# Project-plan review before every commit

Before each commit in this repository, follow `docs/commit-plan-review.md`:

1. Compare the exact staged changes with all relevant descriptions in
   `docs/project-plan.md`, not only its progress section.
2. Update stale functional rules, completion status, validation and next steps.
3. Distinguish implemented code, automated validation and hardware acceptance.
   State untested/unaccepted boundaries explicitly; never infer acceptance.
4. When the plan is already accurate, record the reviewed sections and the
   concrete reason no update is needed. Do not make artificial documentation edits.
5. Stage necessary documentation with the scoped implementation. Preserve
   unrelated work. Fill all fields in a local report based on
   `tools/plan-review-report.example.json`, then run
   `python tools/plan_review.py record --report .plan-review/report.json`.

Stage first, record second, commit in a separate command. If staged contents,
HEAD or the plan changes, review again. Never fabricate a review receipt or
bypass/disable these hooks to make a commit succeed. Hook setup does not grant
permission to commit or push. Report review results when delivering a commit.
