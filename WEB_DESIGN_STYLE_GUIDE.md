# Responsive Web Design Style Guide - LinBPQ

## Core Responsive Principles
- **Mobile-First Approach**: All pages include `<meta name="viewport" content="width=device-width, initial-scale=1.0">` for proper mobile rendering
- **Flexbox-Based Layout**: Heavy use of CSS flexbox (`display: flex`) for flexible, responsive layouts
- **Media Query Breakpoints**: 
  - 768px: Main breakpoint for tablet/desktop (flex-direction: column resets for mobile)
  - 480px: Mobile-specific adjustments for signup forms

## Typography & Color Scheme
- **Font**: Arial, sans-serif (fallback to system fonts)
- **Monospace Fonts**: 'JetBrains Mono', Consolas, Monaco, 'Courier New', monospace
- **Background**: Light neutral gray (#f4f4f4) for page backgrounds, white for cards/forms
- **Primary Color**: Blue (#007bff) for buttons and interactive elements
- **Button Hover State**: Darker blue (#0056b3) on hover
- **Borders**: Light gray (#ccc) for subtle form element borders
- **Text**: Default black on white/light backgrounds, centered text for main headings

## Form & Input Styling
- **Text Inputs**: 100% width, padding: 5-10px, border: 1px solid #ccc, border-radius: 4px
- **Textareas**: Full width, padding: 5-8px, consistent border styling, border-radius: 4px
- **Selects/Dropdowns**: padding: 5px, border: 1px solid #ccc, border-radius: 4px
- **Buttons**: 
  - Background: #007bff
  - Color: white
  - Padding: 10px 20px (or 8px 16px for smaller variants)
  - Border: none, border-radius: 4px
  - Cursor: pointer
  - Hover: background #0056b3

## Layout Components
- **`.form-row`**: Flex container with `flex-wrap: wrap` for responsive form fields
  - Labels: flex: 1 1 150px-200px (flexible width minimum)
  - Inputs: flex: 2 1 100px-200px (proportionally larger)
  - Mobile: Stacks vertically (flex-direction: column) below 768px
- **`.menu`**: Flex container for navigation/menu items
  - Items: content-width by default (inline-flex), full-width only on mobile
  - Items: min-height: 44px, padding: 10px 16px, border-radius: 6px, border: 1px solid #ccc
  - Mobile: Single column layout (flex-direction: column)
- **`.buttons`**: Centered button container (text-align: center)

## Node Pages (`/Nodes/`) Implementation Notes
- **Menu max width**: 1100px (see `HTTPcode.c` `HTTP_NODE_MENU_CSS`)
- **Routes page heading**: `Routes.html` uses shared heading macro with responsive clamp sizing (`HTTP_NODE_H2`)
- **Routes table classing**:
  - Uses `node-table node-table-stack routes-table`
  - Wrapped in `.table-wrap` for horizontal scrolling on narrow screens
  - Uses `data-label` attributes on all cells for mobile stacked cards
- **Compact route density**:
  - `.routes-table` uses compact monospace sizing (`COMMON_TABLE_FONT_SIZE_COMPACT`)
  - Header/body cell padding is reduced (`5px 4px`) for dense route columns
- **Node stats mobile behavior**:
  - `stats-table` suppresses `data-label` pseudo-labels on mobile for cleaner two-column stat cards

## Node Modernization Status
- **Modernized (`/Nodes/`) pages**:
  - `Routes.html`, `Nodes.html`, `Links.html`, `Users.html`, `NodeStats.html`, and port stats views use shared menu/table CSS patterns (`COMMON_MENU_CSS`, `COMMON_TABLE_CSS`) and responsive stacked table behavior.
  - `Index[]` (root landing page) modernized: responsive flex card layout, DOCTYPE, viewport, no legacy table markup.
  - `Beacons[]` modernized: `.form-section` + `.form-row` + `.buttons` pattern, no layout tables.
- **Legacy islands still present in Node code paths**:
  - Terminal templates (`TermSignon[]`, `TermPage[]`, `TermOutput[]`) are responsive but remain standalone inline-style pages, not yet unified with `common_web_components.h`.
  - A few deep status/diagnostic paths still emit inline table/style HTML directly (`HTTPcode.c` ~lines 4477, 4913).

## Node Migration Priority (Guide)
- **Priority 1 (done)**: Keep `Routes`/`Nodes`/`Links`/`Users`/`Stats` on shared table/menu macros and avoid introducing new presentational attributes.
- **Priority 2 (done)**: `Beacons[]` refactored to `.form-section` + `.form-row` patterns. `Index[]` modernized to flex card layout.
- **Priority 3**: Migrate terminal templates to shared tokens/components where practical (fonts, spacing, buttons, form controls).
- **Priority 4 (done)**: `StatusProc` (Stream Status page) and `BuildRigCtlPage` (Rig Control page) modernized: DOCTYPE, viewport, charset, `<style>` block replacing inline table attributes (`border`, `cellpadding`, `cellspacing`, inline `style=` on `<table>`), fixed invalid `<body height:580px>` syntax, removed `&nbsp;` padding from header cells, added missing `</body></html>` to `StatusProc`.

## Preferred Menu Design (Standard)
- **Use this menu pattern on all pages**: Mail header pages, node menu pages, and main configuration pages
- **Desktop behavior (>= 769px)**:
  - `.menu-header` is hidden
  - `.menu` is visible as a wrapped flex row (`display: flex; flex-wrap: wrap; justify-content: center; gap: 10px`)
  - Menu items are touch-friendly with minimum height `44px`
  - Preferred menu item sizing: `min-height: 44px; padding: 10px 16px; font-size: 15px; box-sizing: border-box` (keeps rendered height at 44px)
  - Preferred desktop menu item width: content-based (`auto`) unless a page specifically needs fixed-width controls
  - **Max-width**: Use `980px` for `/Mail/` pages, `1100px` for node menu
- **Mobile behavior (<= 768px)**:
  - `.menu-header` is visible with a full-width `.menu-toggle` button labeled `Menu`/`Close`
  - Preferred `.menu-toggle` sizing: `width: 100%; min-height: 44px; font-size: 16px; box-sizing: border-box`
  - `.menu` is hidden by default and shown only when `.menu-open` is applied
  - Menu items stack in a single column (`flex-direction: column`), full width, centered text
- **Menu item styling**:
  - Background: white
  - Border: 1px solid #ccc (or nearest neutral border token)
  - Radius: 6px to 8px
  - Hover: light gray (`#e9ecef`)
  - Text color: dark neutral (`#1f2937`)
- **Interaction requirements**:
  - `toggleMenu(event)` toggles `.menu-open` and updates button label
  - On mobile, clicking outside the menu closes it and resets the toggle label to `Menu`
  - Use `id`-based hooks (`menuToggle`, `mailMenu` or `mainMenu`) for consistent behavior
- **Reference implementations**:
  - `HTTPcode.c` (`SetupNodeMenu`) - 1100px for node menu
  - `BBSHTMLConfig.c` (`MailPage`, `RefreshMainPage`) - 980px for mail pages
  - `templatedefs.c` (`MainConfigtxt`) - 980px for configuration

## Spacing & Shadows
- **Page Margin/Padding**: 20px standard (10px for mobile)
- **Form Gaps**: gap: 10px between flex items
- **Box Shadows**: 
  - Cards: 0 0 10px rgba(0,0,0,0.1) (larger shadows)
  - Menu items: 0 0 5px rgba(0,0,0,0.1) (subtle shadows)
  - Login containers: 0 2px 8px rgba(0,0,0,0.1) (refined shadow)
- **Border Radius**: 4px for inputs/buttons, 8px for larger containers

## Table Layout
- Node tables use `width: max-content` and `table-layout: auto` within `.table-wrap` wrappers.
- Horizontal Scrolling: Always wrap Node data tables in `.table-wrap` (`overflow-x: auto`) so wide tables (notably `Routes.html`) remain usable.
- Stacked/Card Layout (Mobile): Under 768px, `.node-table-stack` hides `<thead>` and renders rows/cells as block cards using `data-label` pseudo-labels.
- Compact variants:
  - `.routes-table` and `.compact-table` use reduced font size and tighter cell padding.
  - Use these classes for dense operational pages (`Routes`, `Links`, `Sessions`, `Stats`).
- Zebra Striping: `tbody tr:nth-child(even)` uses `#f2f2f2` on desktop table view.
- Alignment: Use `.text`, `.num`, and `.center` cell classes for semantic alignment.
- Accessibility: Use `<th>` with `scope=col` for table headers.

## Accessibility & Best Practices
- All form inputs have associated labels or clear labels within form-row divs
- Checkbox styling: margin 0 5px for spacing
- Full-width form elements at mobile breakpoint
- Center-aligned main headings and page titles
- Readonly text inputs styled consistently with regular inputs

---

## Shared CSS Macro Reference (`common_web_components.h`)

All web-generating C files that produce HTML should `#include "common_web_components.h"` and use these macros instead of writing inline CSS blocks.

### Base / Foundation
| Macro | Purpose | Use in |
|---|---|---|
| `COMMON_CSS_ROOT` | `:root` color variables + `*{box-sizing:border-box}` | Foundation of every page `<style>` |
| `COMMON_BODY_BASE_CSS` | `body` with safe-area padding, font, background, `@supports` | Every full-page template |
| `COMMON_HEADING_CSS` | Centered `h3` with clamp sizing | Every full-page template |
| `COMMON_REDUCED_MOTION_CSS` | `@media(prefers-reduced-motion)` zero-duration animations | Every full-page template |
| `COMMON_CSS_VARIABLES` | Combines all four above — shorthand for full page base | **Preferred** for any new full-page template |

### Navigation Menu
| Macro | Purpose | Note |
|---|---|---|
| `COMMON_MENU_CSS` | Responsive collapsible menu CSS | For `sprintf`/`snprintf` contexts (`%%` safe) |
| `COMMON_MENU_CSS_TEMPLATE(PCT)` | Parameterized version | Use `"%"` for raw string contexts, `"%%"` for printf contexts |
| `COMMON_MENU_JAVASCRIPT` | Toggle/close/escape JS for the menu | Inject into `<script>` block |

### Tables
| Macro | Purpose |
|---|---|
| `COMMON_TABLE_CSS` | Responsive stacked-card table with `.node-table`, `.node-table-stack`, `.compact-table`, `.routes-table`, `.stats-table`, `.table-wrap` |
| `COMMON_TABLE_FONT_SIZE` | `clamp()` string for normal table font |
| `COMMON_TABLE_FONT_SIZE_COMPACT` | `clamp()` string for dense table font |

### Forms & Inputs
| Macro | Purpose |
|---|---|
| `COMMON_FORM_CSS` | `.form-section`, `.form-row`, label, input, textarea, checkbox, responsive stacking |
| `COMMON_BUTTON_CSS` | `.buttons` sticky container with primary-color `input`/`button` |
| `COMMON_UTILITY_CSS` | Lightweight helpers: `.text-center`, `.muted-note`, `.inline-label`, `.flex-2-200`, `.input-w-80/100` |

### Signon / Login Pages
| Macro | Purpose |
|---|---|
| `COMMON_SIGNON_CSS` | Full signon page CSS: body, h2/h3, `.form-container`, `.form-row`, label, inputs, `.btn`, `.btn:hover/active`, `.alert-error`, `.alert-warn`, `.msg-page` |

Used by: `TermSignon`, `NodeSignon`, `MailSignon`, `ChatSignon`, `LostSession`, `NoSessions`, `MailLostSession` in `HTTPcode.c`.

**For `PassError`/`BusyError` snippets** (injected into existing signon pages that already carry `COMMON_SIGNON_CSS`):
```
"<div class='alert-error'>..message..</div>"
"<div class='alert-warn'>..message..</div>"
```

### APRS Pages
All APRS CSS is defined by a family of parameterized templates, with convenience aliases:

| Alias | Template | Use in |
|---|---|---|
| `COMMON_APRS_CONTENT_CSS` | `COMMON_APRS_CONTENT_CSS_TEMPLATE("%")` | Raw string contexts |
| `COMMON_APRS_MAP_CSS` | `COMMON_APRS_MAP_CSS_TEMPLATE("%")` | Raw string contexts |
| `COMMON_APRS_MESSAGE_PAGE_CSS` | `COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE("%")` | Raw string contexts |
| `COMMON_APRS_CONTENT_CSS_FMT` | `COMMON_APRS_CONTENT_CSS_TEMPLATE("%%")` | `sprintf`/`snprintf` contexts |
| `COMMON_APRS_MAP_CSS_FMT` | `COMMON_APRS_MAP_CSS_TEMPLATE("%%")` | `sprintf`/`snprintf` contexts |
| `COMMON_APRS_MESSAGE_PAGE_CSS_FMT` | `COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE("%%")` | `sprintf`/`snprintf` contexts |

**CSS hierarchy**: `BASE_TEMPLATE` → `CONTENT_TEMPLATE` (extends base) → `MAP_TEMPLATE` (extends content). `MESSAGE_PAGE_TEMPLATE` extends base independently.

**Used in**: `APRSStdPages.c` (`APRS_CSS`, `APRS_MAP_CSS`), `APRSCode.c` (`APRS_MSG_PAGE_STYLE`).

### Menus (App-Specific)
| Macro | Usage |
|---|---|
| `COMMON_MAIL_MENU` | `sprintf(buf, COMMON_MAIL_MENU, key×9)` — inject after page `<body>` open in mail pages |
| `COMMON_CHAT_MENU` | `sprintf(buf, COMMON_CHAT_MENU, key×2)` — inject after page `<body>` open in chat pages |

### `%` vs `%%` Pattern
This codebase has two distinct string contexts:
- **Raw string templates** (e.g., static `char Foo[]` arrays used directly, not through `sprintf`): use single `%` in CSS (e.g., `100%`)
- **printf-format templates** (passed to `sprintf`/`snprintf`): must use `%%` to produce a literal `%` in output

When adding a new shared CSS macro that contains percentage values, use the `TEMPLATE(PCT)` pattern:
```c
#define MY_CSS_TEMPLATE(PCT) ".foo{width:100" PCT ";}"
#define MY_CSS     MY_CSS_TEMPLATE("%")   // raw string alias
#define MY_CSS_FMT MY_CSS_TEMPLATE("%%")  // printf-safe alias
```
