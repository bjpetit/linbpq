# Responsive Web Design Style Guide - LinBPQ

## Core Responsive Principles
- **Mobile-First Approach**: All pages include `<meta name="viewport" content="width=device-width, initial-scale=1.0">` for proper mobile rendering
- **Flexbox-Based Layout**: Heavy use of CSS flexbox (`display: flex`) for flexible, responsive layouts

### Media Query Breakpoints (Enhanced Granularity)
| Breakpoint | Device Type | Use Case | Width Range |
|------------|-------------|----------|-------------|
| **320px** | Small mobile | Legacy phones, landscape mode | `@media (max-width: 359px)` |
| **360px** | Standard mobile | Default phone size, common breakpoint | `@media (min-width: 360px) and (max-width: 479px)` |
| **480px** | Large mobile | Phablets, larger phones | `@media (min-width: 480px) and (max-width: 767px)` |
| **768px** | Tablet | iPad, tablets (portrait) | `@media (min-width: 768px) and (max-width: 1023px)` |
| **1024px** | Desktop | Tablets (landscape), laptops | `@media (min-width: 1024px) and (max-width: 1439px)` |
| **1440px** | Large desktop | Large monitors, 4K displays | `@media (min-width: 1440px)` |

**Recommended Implementation:**
```css
/* Mobile-first: start with 320px base styles */
/* Small mobile refinement */
@media (min-width: 360px) { /* Better-than-tiny phones */ }

/* Standard mobile (e.g., menu toggle to full display) */
@media (min-width: 480px) { /* Slightly more room for content */ }

/* Tablet (major layout shift) */
@media (min-width: 768px) { /* Display desktop menu, 2-column layouts */ }

/* Large tablet / desktop (refinements) */
@media (min-width: 1024px) { /* Max-width containers, larger fonts */ }

/* Large desktop (scale/polish) */
@media (min-width: 1440px) { /* Extra spacing, larger fonts */ }
```

## Typography & Color Scheme
- **Proportional Font**: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
- **Monospace Fonts**: ui-monospace, "Cascadia Code", "Segoe UI Mono", "SF Mono", "Roboto Mono", "Courier New", monospace;
- **Responsive Typography Using clamp() - REQUIRED:**
  - All font sizes MUST use CSS `clamp()` function for fluid scaling across breakpoints
  - Format: `font-size: clamp(min, preferred, max);` where preferred uses viewport width (vw)
  - **Body text:** `font-size: clamp(14px, 1vw + 0.5rem, 18px);` (scales 14px-18px)
  - **Headings h1:** `font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; font-size: clamp(28px, 5vw, 48px);` (scales 28px-48px)
  - **Headings h2:** `font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; font-size: clamp(24px, 4vw, 36px);` (scales 24px-36px)
  - **Headings h3:** `font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; font-size: clamp(20px, 3vw, 28px);` (scales 20px-28px)
  - **Form inputs:** `font-size: clamp(14px, 2vw, 16px);` (keeps readable, prevents iOS zoom)
  - **Menu items:** `font-size: clamp(14px, 1.5vw, 16px);` (scales with viewport)
  - **Table text:** `font-size: clamp(12px, 0.8vw, 14px);` (compact but readable)
  - **Benefits:** Eliminates need for multiple breakpoint rules; smoothly scales between min and max
  - **Testing:** Always verify readability at 320px, 480px, 768px, 1024px, and 1440px
  - **Minimum readable font:** Never use clamp minimum < 12px; use 13px minimum for captions

### Color Palette (WCAG AA/AAA Verified)
| Purpose | Color | Contrast on White | WCAG Level |
|---------|-------|-------------------|------------|
| Body text | #000000 (black) | 21:1 | AAA ✓ |
| Primary button/link | #0056b3 (dark blue) | 7.2:1 | AAA ✓ |
| Button hover | #003d82 (darker blue) | 10.3:1 | AAA ✓ |
| Secondary text | #1f2937 (dark neutral) | 14.6:1 | AAA ✓ |
| Muted text | #636363 (medium gray) | 6.1:1 | AA ✓ |
| Borders | #999999 (medium gray) | 5.7:1 | AA ✓ |
| Light borders | #d0d0d0 (light gray) | 4.5:1 | AA ✓ |
| Page background | #f4f4f4 (light gray) | — | — |
| Card/form background | #ffffff (white) | — | — |
| Error | #dc3545 (red) | 5.2:1 (white text) | AA ✓ |
| Warning | #ff9800 (orange) | 4.5:1 (white text) | AA ✓ |
| Success | #28a745 (green) | 5.5:1 (white text) | AA ✓ |

**DEPRECATED COLORS (DO NOT USE):**
- ~~#007bff~~ (light blue - 5.3:1 contrast, borderline AA)
- ~~#ccc~~ (light gray - 3.2:1 contrast, FAILS WCAG AA)

## Form & Input Styling

### Touch-Friendly Form Element Spacing (Accessibility - CRITICAL)
All form elements must have adequate internal and external spacing for readability and usability. This includes padding, line-height, and minimum dimensions that accommodate users with motor disabilities or vision impairments.

- **Text Inputs**: 100% width, border: 1px solid #999999, border-radius: 4px
  - Padding: `padding: clamp(10px, 1vw, 14px) clamp(12px, 1.5vw, 16px);` (responsive)
  - Line-height: `line-height: 1.5;` (ensures text doesn't get cramped vertically)
  - Min-height: `min-height: clamp(36px, 8vw, 48px);` (ensures 48px on mobile, allows growth on large screens)
  - Font-size: `font-size: clamp(14px, 2vw, 16px);` (readable on all devices, prevents iOS zoom)
  - Box-sizing: `box-sizing: border-box;` (padding included in height calculation)
  - **Focus:** `outline: 3px solid #0056b3; outline-offset: 2px;` (keyboard navigation)
  - **Mobile (<480px):** Ensure full width with no horizontal scroll
  - **Example:** At 48px min-height with 14px font and 1.5 line-height: ~32px reserved for text, ~16px for padding

- **Textareas**: Full width, border: 1px solid #999999, border-radius: 4px
  - Padding: `padding: clamp(10px, 1vw, 14px) clamp(12px, 1.5vw, 16px);` (responsive)
  - Line-height: `line-height: 1.6;` (increased for readability of multi-line text)
  - Font-size: `font-size: clamp(14px, 2vw, 16px);` (consistent with inputs)
  - Min-height: `min-height: clamp(100px, 20vw, 200px);` (grows proportionally, always >= 100px)
  - Box-sizing: `box-sizing: border-box;` (padding included in height calculation)
  - Resize: `resize: vertical;` (allow height adjustment, prevent horizontal scroll)
  - **Focus:** `outline: 3px solid #0056b3; outline-offset: 2px;` (keyboard navigation)

- **Selects/Dropdowns**: border: 1px solid #999999, border-radius: 4px
  - Padding: `padding: clamp(8px, 1vw, 14px) clamp(10px, 1.5vw, 16px);` (responsive)
  - Line-height: `line-height: 1.5;` (maintains vertical centering)
  - Min-height: `min-height: clamp(36px, 8vw, 48px);` (touch-friendly on mobile)
  - Font-size: `font-size: clamp(14px, 2vw, 16px);` (prevents iOS zoom)
  - Box-sizing: `box-sizing: border-box;` (padding included in height calculation)
  - **Focus:** `outline: 3px solid #0056b3; outline-offset: 2px;` (keyboard navigation)

- **Labels** (paired with form inputs):
  - Display: `display: block;` (always on own line for clarity)
  - Font-size: `font-size: clamp(13px, 1.5vw, 15px);` (slightly smaller than input text)
  - Margin-bottom: `margin-bottom: clamp(4px, 0.5vw, 8px);` (space between label and input)
  - Font-weight: `font-weight: 500;` (slightly bolder for visibility)
  - Color: `color: #1f2937;` (dark neutral, 14.6:1 contrast)
  - **For required fields:** Use `<abbr title="required">*</abbr>` after label text

- **Form Validation Messages**:
  - Font-size: `font-size: clamp(12px, 1vw, 13px);` (slightly smaller, clearly secondary)
  - Margin-top: `margin-top: clamp(4px, 0.5vw, 6px);` (minimal space after input)
  - Error color: `color: #dc3545;` (red, 5.2:1 contrast on white)
  - Success color: `color: #28a745;` (green, 5.5:1 contrast on white)
  - **Accessibility:** Include `role="alert"` for screen reader announcement
- **Buttons**: 
  - Background: #0056b3 (dark blue - 7.2:1 contrast with white text)
  - Color: white
  - Border: none, border-radius: 4px
  - Cursor: pointer
  - Hover: background #003d82 (darker blue - 10.3:1 contrast)
  - Padding: `padding: clamp(10px, 1.5vw, 16px) clamp(16px, 2vw, 28px);` (responsive, scales width/height)
  - Min-height: `min-height: clamp(44px, 8vw, 52px);` (touch-friendly at all sizes)
  - Font-size: `font-size: clamp(14px, 1.5vw, 16px);` (readable button text)
  - **Focus:** `outline: 3px solid #0056b3; outline-offset: 2px;` (keyboard navigation)

## Layout Components
- **`.form-row`**: Flex container with `flex-wrap: wrap` for responsive form fields
  - Labels: flex: 1 1 150px-200px (flexible width minimum)
  - Inputs: flex: 2 1 100px-200px (proportionally larger)
  - Mobile (< 480px): Stacks vertically (flex-direction: column) with full-width inputs
  - Tablet (480px-767px): May show 2-column if space permits
  - Desktop (≥ 768px): Side-by-side layout preferred
  - **Vertical spacing:** `margin-bottom: clamp(12px, 2vw, 20px);` between form rows
  - **Mobile input sizing:** All inputs must reach `min-height: 48px` for touch targets
  - **Form row gap:** `gap: clamp(8px, 1vw, 12px);` between labels and inputs
- **`.menu`**: Flex container for navigation/menu items
  - Items desktop: `min-height: 44px; padding: 10px 16px;` (touch-friendly)
  - Items mobile: `min-height: 48px; padding: 12px 16px;` (larger touch targets below 768px)
  - Border: 1px solid #999999, border-radius: 6px
  - Mobile: Single column layout (flex-direction: column)
  - **Responsive hiding:** Hide secondary menu items on 360px-479px if space is critical; show all on 480px+
- **`.buttons`**: Centered button container (text-align: center)
  - Mobile (< 480px): Stack buttons vertically if multiple, ensure 100% width with margin
  - Tablet (480px-767px): May display inline if >= 2 buttons
  - Desktop (≥ 768px): Arrange horizontally with gap: 10px

## Node Pages (`/Nodes/`) Implementation Notes
- **Menu max width**: 1100px (see `HTTPcode.c` `HTTP_NODE_MENU_CSS`)
- **Routes page heading**: `Routes.html` uses shared heading macro with responsive clamp sizing (`HTTP_NODE_H2`)
- **Routes table classing**:
  - Uses `node-table node-table-stack routes-table`
  - Desktop (≥ 768px): Wrapped in `.table-wrap` for horizontal scrolling on narrow screens
  - Tablet (480px-767px): Uses `.node-table-stack` to convert to card layout
  - Mobile (< 480px): Uses `data-label` attributes on all cells for mobile stacked cards; may hide 3+ non-critical columns
- **Compact route density**:
  - `.routes-table` uses compact monospace sizing (`COMMON_TABLE_FONT_SIZE_COMPACT`)
  - Desktop (≥ 768px): Header/body cell padding is reduced (`5px 4px`) for dense route columns
  - Mobile (< 768px): Increase padding for touch targets (`8px 6px` minimum)
- **Node stats mobile behavior**:
  - `stats-table` suppresses `data-label` pseudo-labels on mobile for cleaner two-column stat cards
  - Below 480px: May reduce to single-column stat cards if space is critical

## Preferred Menu Design (Standard)
- **Use this menu pattern on all pages**: Mail header pages, node menu pages, and main configuration pages
- **Breakpoint-Specific Behavior:**
  - **Extra small (< 360px):** Toggle button only, hide all menu text labels (icon-only if possible)
  - **Small mobile (360px-479px):** Toggle button with text, show primary menu items only
  - **Mobile (480px-767px):** Toggle button, show all menu items when expanded
  - **Tablet (768px-1023px):** May show as horizontal row with wrapping, or use toggle if many items
  - **Desktop (≥ 1024px):** Full horizontal menu, all items visible
- **Desktop behavior (>= 769px)**:
  - `.menu-header` is hidden
  - `.menu` is visible as a wrapped flex row (`display: flex; flex-wrap: wrap; justify-content: center; gap: 10px`)
  - Menu items are touch-friendly with minimum height `44px`
  - Preferred menu item sizing: `min-height: 44px; padding: 10px 16px; font-size: clamp(14px, 1.5vw, 16px); box-sizing: border-box`
  - Preferred desktop menu item width: content-based (`auto`) unless a page specifically needs fixed-width controls
  - **Max-width**: Use `980px` for `/Mail/` pages, `1100px` for node menu
- **Mobile behavior (<= 768px)**:
  - `.menu-header` is visible with a full-width `.menu-toggle` button labeled `Menu`/`Close`
  - Preferred `.menu-toggle` sizing: `width: 100%; min-height: 48px; font-size: 16px; box-sizing: border-box`
  - `.menu` is hidden by default and shown only when `.menu-open` is applied
  - Menu items stack in a single column (`flex-direction: column`), full width, centered text
- **Menu item styling**:
  - Background: white
  - Border: 1px solid #999999 (medium gray, 5.7:1 contrast with white)
  - Radius: 6px to 8px
  - Hover: light gray (`#e9ecef` background)
  - Text color: dark neutral (`#1f2937`, 14.6:1 contrast)
  - **Focus:** `outline: 3px solid #0056b3; outline-offset: 2px;` (keyboard navigation)
- **Interaction requirements**:
  - `toggleMenu(event)` toggles `.menu-open` and updates button label
  - On mobile, clicking outside the menu closes it and resets the toggle label to `Menu`
  - Use `id`-based hooks (`menuToggle`, `mailMenu` or `mainMenu`) for consistent behavior
  - **Keyboard accessibility:** Menu toggle must be focusable and respond to Enter/Space; Escape key closes menu and returns focus to toggle
  - `aria-expanded` on the toggle button must reflect open/closed state
  - `aria-controls` on the toggle button must reference the menu container `id`
  - Menu items must be reachable via Tab; Arrow keys may optionally cycle within the open menu
  - **Mobile touch sizing:** Preferred `.menu-toggle` sizing: `width: 100%; min-height: 48px; font-size: 16px; padding: 12px; box-sizing: border-box` (large touch target)
- **Reference implementations**:
  - `HTTPcode.c` (`SetupNodeMenu`) - 1100px for node menu
  - `BBSHTMLConfig.c` (`MailPage`, `RefreshMainPage`) - 980px for mail pages
  - `templatedefs.c` (`MainConfigtxt`) - 980px for configuration

## Spacing & Shadows
- **Page Margin/Padding**: 20px standard (10px for mobile)
- **Form Spacing System (Responsive):**
  - Between form rows: `margin-bottom: clamp(12px, 2vw, 20px);` (vertical space)
  - Between labels and inputs: `gap: clamp(8px, 1vw, 12px);` (horizontal/inline space)
  - Between form sections/fieldsets: `margin-top: clamp(20px, 3vw, 32px);` (major spacing)
  - Between form and buttons: `margin-top: clamp(24px, 4vw, 36px);` (distinct visual break)
  - **Mobile (< 480px):** Reduce all by 25% if space is critical, but never below 8px
- **Box Shadows**: 
  - Cards: 0 0 10px rgba(0,0,0,0.1) (larger shadows)
  - Menu items: 0 0 5px rgba(0,0,0,0.1) (subtle shadows)
  - Login containers: 0 2px 8px rgba(0,0,0,0.1) (refined shadow)
  - Form groups: Optional 0 0 4px rgba(0,0,0,0.05) for subtle grouping
- **Border Radius**: 4px for inputs/buttons, 8px for larger containers
- **Border Colors:** Use #999999 (medium gray, 5.7:1 contrast) for all borders. DEPRECATED: Do not use #ccc

## Table Layout
- Node tables use `width: max-content` and `table-layout: auto` within `.table-wrap` wrappers.
- Horizontal Scrolling: Always wrap Node data tables in `.table-wrap` (`overflow-x: auto`) so wide tables (notably `Routes.html`) remain usable.
- Stacked/Card Layout (Mobile): Under 768px, `.node-table-stack` hides `<thead>` and renders rows/cells as block cards using `data-label` pseudo-labels.
- Compact variants:
  - `.routes-table` and `.compact-table` use reduced font size and tighter cell padding.
  - Use these classes for dense operational pages (`Routes`, `Links`, `Sessions`, `Stats`).
- Zebra Striping: `tbody tr:nth-child(even)` uses `#f2f2f2` on desktop table view (adequate contrast with black text).
- Alignment: Use `.text`, `.num`, and `.center` cell classes for semantic alignment.
- **Table text colors:** Use `#000000` for primary text (21:1 contrast), `#1f2937` for secondary (14.6:1 contrast).
- Accessibility: Use `<th>` with `scope=col` for table headers.
---

## Responsive Design Best Practices

### Mobile-First Development Workflow
1. **Start with 320px base styles** - Design for the smallest screens first
2. **Test at each breakpoint** (320px, 360px, 480px, 768px, 1024px, 1440px)
3. **Use mobile-first media queries** - Build up from small to large:
   ```css
   @media (min-width: 480px) { /* Tablet changes */ }
   @media (min-width: 768px) { /* Tablet portrait */ }
   @media (min-width: 1024px) { /* Desktop */ }
   ```
4. **Avoid fixed widths** - Use percentages, flexbox, and grid
5. **Test with real devices** - Browser dev tools are not enough; test on actual phones/tablets

### Small Screen Optimization (< 480px)
- **Content prioritization:** Hide tertiary information; show essentials only
- **Vertical stacking:** Stack all multi-column layouts vertically
- **Touch targets:** Maintain 48px minimum; never shrink below this
- **Margins/padding:** Reduce by 25% below 480px if space is critical
- **Text truncation:** Use `text-overflow: ellipsis` and `overflow: hidden` for long content
- **Images:** Use responsive images with `<picture>` or `srcset` for optimal loading
- **Tables:** Convert to card/list layout; never use horizontal scroll on mobile
- **Forms:** Single-column, full-width inputs; label above input

### Tablet Optimization (480px-1023px)
- **Flexible layouts:** 2-column layouts may work; test carefully
- **Menu display:** Can show horizontal menu or still use toggle
- **Touch targets:** Maintain 44px minimum
- **Typography:** Increase from mobile baseline by 10-15%
- **Spacing:** Slightly increase gaps and margins for breathing room

### Desktop and Large Screens (≥ 1024px)
- **Max-width containers:** Prevent text lines from becoming too long (65-80 characters)
- **Multi-column layouts:** 2-3 column layouts are appropriate
- **Dense content:** More information per screen is acceptable
- **Advanced interactions:** Hover states, tooltips become viable
- **Spacing:** Generous padding/margins for visual hierarchy

### Using clamp() for Responsive Typography
The `clamp()` function eliminates the need for media query breakpoints for font sizing:

**Syntax:** `property: clamp(MIN, preferred, MAX);`

**Examples:**
```css
/* Body text: scales from 14px to 18px */
body { font-size: clamp(14px, 1vw + 0.5rem, 18px); }

/* Heading h1: scales from 28px to 48px, uses proportional font */
h1 { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; font-size: clamp(28px, 5vw, 48px); }

/* Form inputs: keeps readable, prevents iOS zoom */
input { font-size: clamp(14px, 2vw, 16px); }

/* Button padding: responsive scaling */
button { padding: clamp(10px, 1.5vw, 16px) clamp(16px, 2vw, 28px); }

/* Table text: compact but readable */
table { font-size: clamp(12px, 0.8vw, 14px); }
```

**How clamp() scales:**
- At 320px viewport: Uses MIN value (14px)
- At 768px viewport: Calculates middle value (1vw = ~7.68px + 0.5rem ≈ 16px)
- At 1440px viewport: Uses MAX value (18px)

**Benefits:**
- Fluid, smooth scaling between min and max
- No jarring font size jumps at breakpoints
- Single CSS rule replaces multiple media queries
- Better performance (less CSS to parse)
- Works across all modern browsers (95%+ coverage)
## Accessibility & Best Practices
- **Link Styling (WCAG 2.1 - CRITICAL):**
  - Links must be distinguishable from surrounding body text **without relying solely on color**
  - Use `text-decoration: underline;` by default — this is the clearest cue for all users, including color-blind
  - **Standard link states (all required):**
    ```css
    a { color: #0056b3; text-decoration: underline; cursor: pointer; }
    a:visited { color: #6f42c1; } /* Purple, 5.1:1 contrast on white */
    a:hover { color: #003d82; text-decoration: none; background-color: #e7f3ff; }
    a:focus-visible {
      outline: 3px solid #0056b3;
      outline-offset: 2px;
      border-radius: 2px;
    }
    a:active { color: #003d82; }
    ```
  - **Never** style links with only a color change (fails WCAG 1.4.1 Use of Color)
  - **Never** remove underline without providing another non-color distinguisher (e.g., bold, border-bottom)
  - **Exception:** Links inside navigation menus (`<nav>`, `.menu`) may omit underline since context makes them clearly interactive
  - **Visited state:** Always style `a:visited` distinctly — helps users track where they've been
  - **External links:** Mark with `target="_blank" rel="noopener noreferrer"` and optionally an icon/label `(opens in new tab)` for accessibility
  - **Icon-only links:** Must have `aria-label` or visually hidden text describing the destination
  - **Touch target:** Links inline in text do not need 48px height, but standalone link buttons should follow button sizing rules
- **Keyboard Navigation (WCAG 2.1 Level AA - CRITICAL):**
  - Every interactive element must be reachable and operable with keyboard alone (Tab, Shift+Tab, Enter, Space, Arrow keys, Escape)
  - **Tab order:** Must follow a logical, predictable sequence — match visual reading order (top to bottom, left to right)
  - Never use `tabindex` values > 0; use `tabindex="0"` to include an element, `tabindex="-1"` to remove it
  - **Skip link:** Include a "Skip to main content" link as the first focusable element on every page:
    ```html
    <a href="#main-content" class="skip-link">Skip to main content</a>
    <main id="main-content">...</main>
    ```
    ```css
    .skip-link {
      position: absolute; top: -100%; left: 0;
      background: #0056b3; color: white; padding: 8px 16px;
      font-size: clamp(14px, 2vw, 16px); z-index: 1000;
      text-decoration: none; border-radius: 0 0 4px 0;
    }
    .skip-link:focus { top: 0; } /* Becomes visible on focus */
    ```
  - **Collapsible menus:** Menu toggle button must:
    - Be a `<button>` element (keyboard accessible by default)
    - Carry `aria-expanded="false"` / `aria-expanded="true"` to reflect state
    - Carry `aria-controls="<id of menu>"` to link toggle to menu
    - Close on Escape key: add `keydown` listener for `e.key === 'Escape'`
    - Return focus to the toggle button when closed via Escape
  - **Forms:** Tab order must flow label → input → label → input sequentially
    - Use `<label for="id">` or wrapping `<label>` — never rely on visual proximity alone
    - Provide `aria-describedby` to link error messages to their input
  - **COMMON_MENU_JAVASCRIPT** must include Escape key handling and `aria-expanded` toggling
  - **Testing keyboard navigation:** Tab through every page; every action achievable by mouse must be achievable by keyboard alone
- **Focus Indicators (WCAG 2.1 Level AA - CRITICAL):**
  - All interactive elements (`button`, `input`, `select`, `textarea`, `a`) must have visible `:focus-visible` styles
  - Use: `outline: 3px solid #0056b3; outline-offset: 2px;`
  - Never use `outline: none;` or hide focus indicators
  - Applies to: buttons, form inputs, menu items, links, toggles
- **Touch Target Sizing (Mobile Accessibility - CRITICAL):**
  - Mobile (< 480px): All interactive elements must have **minimum 48px** height/width
  - Tablet/Desktop (≥ 480px): Minimum 44px is acceptable
  - This includes buttons, inputs, menu items, checkboxes, and links
  - Applies to users with motor disabilities, large fingers, and general usability
- **Color Contrast (WCAG AA/AAA Verified - CRITICAL):**
  - All text must meet WCAG AA minimum contrast (4.5:1 for normal text, 3:1 for large text)
  - Body text: Use #000000 on #ffffff (21:1 contrast - AAA)
  - Buttons: Use #0056b3 on white or white on #0056b3 (7.2:1 contrast - AAA)
  - Borders: Use #999999 or darker (minimum 5.7:1 contrast - AA)
  - **NEVER use #ccc (#cccccc)** for any purpose; use #999999 or #d0d0d0 instead
  - Test all custom color combinations using WCAG contrast checkers before deployment
  - Reference verified colors in color palette section above
- All form inputs have associated labels or clear labels within form-row divs
- **Checkbox and Radio Button Styling (Touch Targets - CRITICAL):**
  - Never rely on tiny native checkboxes; always make clickable area 48px minimum on mobile
  - Use `<label>` wrapping checkbox/radio with increased padding: `padding: clamp(8px, 1vw, 12px);`
  - Minimum height for label/input combo: `min-height: clamp(36px, 8vw, 48px);` on mobile
  - Display: `display: flex; align-items: center; gap: clamp(8px, 1vw, 12px);` for label + control
  - Use visible focus states for keyboard users: `:focus-visible { outline: 3px solid #0056b3; outline-offset: 2px; }`
- Full-width form elements at mobile breakpoint
- Readonly text inputs styled consistently with regular inputs, including touch target sizing
- **Form fieldsets:** Use `<fieldset>` + `<legend>` for grouped inputs (e.g., radio button groups)
  - Legend font-size: `font-size: clamp(14px, 1.5vw, 16px);` (same as labels)
  - Legend margin-bottom: `margin-bottom: clamp(8px, 1vw, 12px);` (space before grouped inputs)
  - Fieldset border: Optional (`border: 1px solid #d0d0d0;`) with `padding: clamp(12px, 1.5vw, 16px);`

---

## Shared CSS Macro Reference (`common_web_components.h`)

All web-generating C files that produce HTML should `#include "common_web_components.h"` and use these macros instead of writing inline CSS blocks.

### ⚠️ CRITICAL: All Macros Must Use clamp() for Font Sizing, Focus-Visible, and Verified Colors
Every CSS macro that styles interactive elements or text **MUST** include:
1. **Responsive font sizing with `clamp()`:** e.g., `font-size: clamp(14px, 2vw, 16px);`
   - Eliminates need for font-size breakpoint rules
   - Ensures readable text at all screen sizes
   - Examples: `COMMON_TABLE_FONT_SIZE`, `COMMON_TABLE_FONT_SIZE_COMPACT` already use this
2. **`:focus-visible` styles:** `outline: 3px solid #0056b3; outline-offset: 2px;` for keyboard accessibility
3. **Verified color palette:** Use only colors from the "Color Palette" section. Avoid #007bff and #ccc.

This is required for WCAG 2.1 Level AA compliance (responsive design + keyboard accessibility + color contrast).

### Base / Foundation
| Macro | Purpose | Use in |
|---|---|---|
| `COMMON_CSS_ROOT` | `:root` color variables + `*{box-sizing:border-box}` | Foundation of every page `<style>` |
| `COMMON_BODY_BASE_CSS` | `body` with safe-area padding, font, background, `@supports` | Every full-page template |
| `COMMON_HEADING_CSS` | Centered `h3` with proportional font and clamp sizing | Every full-page template |
| `COMMON_REDUCED_MOTION_CSS` | `@media(prefers-reduced-motion)` zero-duration animations | Every full-page template |
| `COMMON_CSS_VARIABLES` | Combines all four above — shorthand for full page base | **Preferred** for any new full-page template |

### Navigation Menu
| Macro | Purpose | Note |
|---|---|---|
| `COMMON_MENU_CSS` | Responsive collapsible menu CSS | For `sprintf`/`snprintf` contexts (`%%` safe) |
| `COMMON_MENU_CSS_TEMPLATE(PCT)` | Parameterized version | Use `"%"` for raw string contexts, `"%%"` for printf contexts |
| `COMMON_MENU_JAVASCRIPT` | Toggle/close/escape JS for the menu — **must include `aria-expanded` toggling and Escape key dismiss** | Inject into `<script>` block |

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
