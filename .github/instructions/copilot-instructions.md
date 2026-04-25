# Copilot Instructions

- When modifying code, do not alter existing string literals unless the change explicitly requires it.
- When modifying code, do not alter existing comments unless the change explicitly requires it.
- Watch for unintended encoding changes (for example UTF-8/ANSI/newline conversions) and preserve the original file encoding and line endings.
- `APRSCode.c` contains legacy non-UTF8 bytes in comments (Windows-1252 chars: 0x85, 0x96, 0x97, 0x98, 0x99); preserve those comment bytes exactly and do not normalize or replace them. After EVERY edit to `APRSCode.c`, **and after any edit to a file that `APRSCode.c` includes** (e.g. `common_web_components.h`, `APRSStdPages.c`), run the following perl script to restore any corrupted bytes, then verify with `python3 -c "data=open('APRSCode.c','rb').read(); print(data.count(b'\xef\xbf\xbd'))"` (should print 0):
  ```bash
  perl -i -0777 -pe '
  s/\xEF\xBF\xBD0000\.00N\\00000\.00W\.\xEF\xBF\xBD/\x850000.00N\\00000.00W.\x85/g;
  s/180 \xEF\xBF\xBD d \xEF\xBF\xBD 189/180 \x98 d \x98 189/g;
  s/range 100\xEF\xBF\xBD109/range 100\x96109/g;
  s/190 \xEF\xBF\xBD d \xEF\xBF\xBD 199/190 \x98 d \x98 199/g;
  s/range 0\xEF\xBF\xBD9 degrees\)\./range 0\x969 degrees)./g;
  s/m \xEF\xBF\xBD 60\./m \x99 60./g;
  s/range 0\xEF\xBF\xBD9\)\./range 0\x969)./g;
  s/Format \xEF\xBF\xBD with/Format \x97 with/g
  ' APRSCode.c
  ```
- When updating HTML/CSS, prefer reusing common code patterns to reduce duplication.
- When updating printf-style statements, double-check that format specifiers and arguments line up correctly.
- When making changes do no stage file, commit, or push any code. Instead, just make the changes in the local file and leave it unstaged.
