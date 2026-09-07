# Development and maintenance workflow

Start from current origin/master in an isolated branch or worktree. Preserve unrelated local changes. Search existing PRs and issues before opening a new one; compare actual diffs and test evidence, not titles. Extend an existing suitable PR when possible. Keep each change bounded and reviewable.

## Validation

```sh
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --no-tests=error
```

Repeat in a separate Release build directory. CMake fetches external dependencies when needed; no manual dependency checkout is required. GitHub CMake checks run for master pushes, PRs, and manual dispatch. A zero-test run is a failure. Do not run demo executables against public services as a build check.

CTest covers request serialization, socket move/destruction, and plaintext loopback I/O with file-descriptor leak checks. PR #96 is the canonical foundation. CI also checks lifecycle and loopback under Valgrind. TLS verification, encrypted Send/Receive, context ownership and shared-context concurrency remain follow-ups; offline tests do not certify production TLS.

## Scheduled maintenance

A Codex weekday maintenance automation checks current master and open PRs, prioritizes a failing check or a tested canonical PR, and handles at most one bounded repair per run. Existing unreleased work and clean user checkouts must be preserved. New work is committed and pushed on an isolated branch with a draft PR; existing PR repairs are revalidated on their new head. Review and merge remain separate decisions. Never bulk-close equivalent-looking PRs or deploy/release automatically.

Record the checked SHA, command results and nonzero test counts in automation memory. Track the selected canonical PR and already-reported blockers to avoid repeated work. Alerts go only to private #codex-ops (C0C01JWQ77Y) for a meaningful change, failure, recovery, or required user action. Record delivery only after Slack confirms it; keep unchanged runs quiet.
