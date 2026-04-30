# Copilot Instructions

LinBPQ is a Linux/BSD packet-radio network node: multi-layer AX.25 routing engine, BBS/mail/chat
services, 20+ HF modem drivers, and an embedded HTTP web UI. The codebase is a C port of a legacy
DOS/Windows application; treat it as low-level systems code throughout.

---

## Build

```bash
make            # default build → ./linbpq
make nomqtt     # build without MQTT support
make noi2c      # build without I2C support
make clean      # remove .d / object files and rebuild
```

Build flags: `-DLINBPQ -MMD -g -fcommon -fasynchronous-unwind-tables`  
Libraries: libpaho-mqtt3a, libjansson, libminiupnpc, libconfig, libpcap, zlib, pthread  
`setcap` (capability assignment) is separate from normal compile/link — do not add it to Makefile targets.  
Pre-checked-in `.d` dependency files may reference missing generated headers on a fresh clone — `make clean` first if the build fails unexpectedly.  
Profile targets available: `debug`, `release`, `sanitize` (sanitizer flags applied to both compile and link).

---

## Key Source Files

| File | Role |
|---|---|
| `LinBPQ.c` | Main entry point — signal handling, config load, event loops |
| `CommonCode.c` | Buffer pool (`GetBuff`/`FreeBuf`) with guard zones and leak detection |
| `L2Code.c` / `L3Code.c` / `L4Code.c` | AX.25 layers 2–4 |
| `HTTPcode.c` | HTTP server, response generation, security headers |
| `common_web_components.h` | **Shared CSS/HTML macros** — always extend here, never duplicate |
| `templatedefs.c` | WebMail / BBS / Chat HTML templates |
| `APRSCode.c` | APRS parsing — contains legacy Windows-1252 bytes (see below) |
| `APRSStdPages.c` | APRS web pages + `/bpq/bpq.css` and `/bpq/bpq.js` route handlers |
| `WebMail.c` | Web mail client, including `GetReplyAddress` reply-routing helper |
| `config.c` | libconfig-based config file I/O |
| `cheaders.h` | Master include — included by virtually every .c file |

---

## Coding Conventions

- **Do not alter existing string literals** unless the change explicitly requires it.
- **Do not alter existing comments** unless the change explicitly requires it.
- **Preserve file encoding and line endings** — watch for UTF-8/ANSI/newline conversions.
- **Buffer allocation:** always use the `GetBuff(__FILE__, __LINE__)` / `FreeBuf` macros, not raw `malloc`/`free`; this feeds the guard-zone leak detector in `CommonCode.c`.
- **Semaphores:** every queue operation requires a held semaphore — `GetSemaphore()` → work → `FreeSemaphore()`; missing releases cause deadlocks.
- **`sprintf` vs `snprintf`:** the codebase uses `sprintf` heavily; when writing new or modified code prefer `snprintf` to avoid buffer overflows.
- **Format specifier alignment:** after any `printf`-style change, double-check every specifier matches its argument type and count.
- **Platform portability:** conditionals `-DLINBPQ`, `-DFREEBSD`, `-DMACBPQ`, `-DNOMQTT`, `-DNO_I2C` — do not break non-Linux paths.
- **No staging/committing:** make changes locally and leave them unstaged.

---

## Web UI Conventions

- **`common_web_components.h` is the single source of truth** for shared CSS, JS, and HTML head boilerplate. Add new shared fragments there; never duplicate them across `.c` files.
- **External assets:** shared CSS is served as `/bpq/bpq.css` and JS as `/bpq/bpq.js` (routes in `APRSStdPages.c`). Link them with `COMMON_BPQ_CSS_LINK` and `COMMON_BPQ_JS_SCRIPT` — do not inline the same CSS/JS again.
- **Security headers:** every HTML response must include `COMMON_HTTP_SECURITY_HEADERS` (defined in `common_web_components.h`). Never omit or hard-code its content.
- **Theme colors:** use CSS custom properties (`--bg`, `--primary`, `--text`, etc.) — no hard-coded color values.
- **Home page custom insert** (`HTML/index_insert.html`): inject before `</body></html>` (outside `#mainMenu`). Injecting before `</div></body></html>` places content inside the menu flex layout and breaks responsive menus.
- **WebMail reply routing:** bulletin messages (`Msg->type == 'B'`) must preserve original recipient and routing (`To@via`) as reply destination; use the `GetReplyAddress` helper in `WebMail.c`.
- **`COMMON_WEBMAIL_MENU_CSS_TEMPLATE(PCT)`** is the unified WebMail menu CSS; pass `"%%"` for sprintf templates, `"%"` for static strings. Do not use the old per-page CSS variants.

---

## APRSCode.c — Legacy Encoding Warning

`APRSCode.c` contains Windows-1252 bytes in comments (0x85, 0x96, 0x97, 0x98, 0x99). Many editors
and tools silently replace these with the UTF-8 replacement character (U+FFFD / `\xEF\xBF\xBD`).

After **every edit** to `APRSCode.c`, or to any file it includes (`common_web_components.h`,
`APRSStdPages.c`, etc.), run this repair script and verify the result:

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
python3 -c "data=open('APRSCode.c','rb').read(); print(data.count(b'\xef\xbf\xbd'))"
# must print 0
```
