# Copilot Instructions

- When modifying code, do not alter existing string literals unless the change explicitly requires it.
- When modifying code, do not alter existing comments unless the change explicitly requires it.
- Watch for unintended encoding changes (for example UTF-8/ANSI/newline conversions) and preserve the original file encoding and line endings.
- When updating HTML/CSS, prefer reusing common code patterns to reduce duplication.
- When updating printf-style statements, double-check that format specifiers and arguments line up correctly.
- When making changes do no stage file, commit, or push any code. Instead, just make the changes in the local file and leave it unstaged.
