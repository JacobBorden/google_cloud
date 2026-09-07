# AGENT GUIDELINES

When working on this project, please adhere to the following guidelines:

## Code Style & Standards
- Use modern C++ (C++20 as specified in CMakeLists.txt).
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) for resource management if raw RAII wrappers are insufficient.
- Maintain the Rule of Five for any classes managing resources (like the `Socket` class).

## Pre-commit Checks
- Ensure the code compiles with `cmake` and `make` without warnings. Use `-Wall -Wextra` to catch potential issues (e.g., missing field initializers).
- Run `valgrind` to verify that there are no memory leaks or uses of uninitialized values. Specifically check the `Socket` lifecycle.

## OpenSSL
- Be careful with OpenSSL pointer management (`SSL*`, `SSL_CTX*`). Ensure proper freeing of these resources.
- Avoid hardcoding port numbers (like `"443"`) to deduce protocol type; use explicit configuration or protocol parsing.

## Testing
- Add unit tests for utility functions (like HTTP parsing if added) and integration tests for socket connections.
- Mock network connections where possible to ensure tests can run without internet access.

## Modifying Files
- If you modify `socket.h` or `main.cpp`, ensure you update the `TODO.MD` and `DESIGN.MD` if architectural changes are made or bugs are fixed.

## Project operations

Read `PROJECT_MANAGEMENT.md` for the repository baseline, prioritized management work, and verification requirements. Preserve existing TODO.MD and DESIGN.MD as the technical backlog and architecture sources.

Jacob authorized private Slack operations alerts on 2026-09-07. Send meaningful project changes, new blockers, recoveries, and required actions to #codex-ops (C0C01JWQ77Y). Suppress unchanged alerts, record confirmed delivery in task or automation memory, and retain the primary result if Slack is unavailable. Do not post credentials or raw private source material.
