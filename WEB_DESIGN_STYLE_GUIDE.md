# Responsive Web Design Style Guide - LinBPQ

## Scope
- This guide documents the current non-Chat web layout and CSS conventions.
- Chat pages are intentionally excluded while that redesign is still in progress.
- Source of truth is the shared macros in `common_web_components.h`, with page-level overrides where needed.

## Baseline Tokens and Theme
- Shared CSS variables:
  - `--bg: #f4f4f4`
  - `--surface: #fff`
  - `--primary: #007bff`
  - `--primary-dark: #0056b3`
  - `--border: #ccc`
  - `--border-light: #e2e8f0`
  - `--text: #1f2937`
  - `--link: #1f2937`
- Preferred body font stack:
  - `-apple-system, BlinkMacSystemFont, "Segoe UI", Arial, sans-serif`
- Global body spacing:
  - Safe-area aware with clamp-based padding (`15px` to `20px` effective range on most displays)
- Motion and accessibility baseline:
  - `prefers-reduced-motion` disables animation/transition timing.

## Responsive Breakpoints
- `max-width: 768px`: primary mobile/tablet collapse point.
- `max-width: 480px`: additional compact button stacking in shared button container.

## Shared Menu Pattern (Standard)
- Shared classes and IDs:
  - `.menu-header`, `.menu-toggle`, `.menu`, `.menu-open`
  - `#menuToggle` button
  - `#mainMenu` or `#mailMenu` container
- Desktop behavior:
  - `.menu-header` hidden.
  - `.menu` is visible, wrapped row, centered, `gap: 10px`.
  - Menu links are touch sized with `min-height: 44px`, `padding: 10px 16px`.
- Mobile behavior (`<= 768px`):
  - `.menu-header` shown.
  - `.menu` hidden by default, shown with `.menu-open`.
  - Links stack vertically and expand to full width.
- JS behavior:
  - `toggleMenu(event)` toggles open/close and updates text `Menu`/`Close`.
  - Clicking outside the menu on mobile closes it.
  - Escape key closes open menu.

## Shared Form Pattern
- `.form-section`:
  - Card-style container with responsive padding and subtle shadow.
- `.form-row`:
  - Flex row with wrap, `gap: 10px`, label + input sizing split.
  - Mobile stacks to column below `768px`.
- Inputs:
  - Text/number/password/select use `min-height: 44px` for touch targets.
  - Border/radius defaults are `1px solid var(--border)` and `4px`.
- Textareas:
  - Full width, monospace, minimum `120px` height.

## Shared Buttons Pattern
- `.buttons` container:
  - Centered action area, sticky to bottom in long forms.
  - Surface background with top border for visual separation.
- Buttons inside `.buttons`:
  - Primary blue fill (`var(--primary)`), white text.
  - Hover darkens to `var(--primary-dark)`.
  - Minimum height `44px`.
- Mobile behavior:
  - Two-column button width at `<= 768px`.
  - Single-column full-width buttons at `<= 480px`.

## Shared Tables Pattern
- `.table-container` wraps tables for horizontal overflow safety.
- Default shared table style (`COMMON_TABLE_CSS`):
  - Header cells use primary blue background with white text.
  - Zebra striping and hover tint are enabled.
  - Checkbox controls are touch-sized.
- Mobile (`<= 768px`):
  - Reduced table font and cell padding.

## Current Non-Chat Page Layout Patterns
- Mail shell pages (`/Mail/Status`, `/Mail/Conf`, `/Mail/Users`, `/Mail/Msgs`, `/Mail/FWD`, etc.):
  - Use shared variables plus either shared menu CSS or page-local menu equivalent.
  - Typical max-width is `980px`.
- Message and user management pages:
  - Two-column desktop layouts (sidebar + detail area), collapsed for mobile.
  - Sidebar lists use monospace link rows and mobile toggles.
- Forwarding and housekeeping pages:
  - Card/grid patterns with form rows and responsive list/detail split.

## Known Page-Level Overrides (Expected)
- Some pages still include local CSS blocks for specialized UX while keeping shared tokens.
- Routes-style status views intentionally override shared table defaults:
  - Light gray header (`#f0f0f0`), black text, white row backgrounds.
  - Zebra/hover color shifts disabled to match Routes visual parity.
- A few legacy templates still specify `Arial, sans-serif` locally; this is accepted where unchanged.

## Implementation Rules for New Non-Chat Pages
- Always include:
  - `<meta name="viewport" content="width=device-width, initial-scale=1.0">`
  - `COMMON_CSS_VARIABLES`
- Prefer shared blocks first:
  - `COMMON_MENU_CSS` + `COMMON_MENU_JAVASCRIPT`
  - `COMMON_FORM_CSS`
  - `COMMON_BUTTON_CSS`
  - `COMMON_TABLE_CSS`
  - `COMMON_UTILITY_CSS` as needed
- Override locally only when a page has a specific UX requirement.
- Keep touch targets at least `44px` high for interactive controls.

## Accessibility Requirements
- Keyboard operation:
  - All interactive controls must be fully operable by keyboard alone.
  - Use logical tab order that follows visual flow.
  - Do not remove native focus outlines unless replaced with an equally visible style.
- Focus visibility:
  - Apply visible `:focus-visible` styles to links, buttons, toggles, inputs, selects, and textareas.
  - Focus indicators should be high-contrast and remain visible on light and dark control backgrounds.
- Semantic controls:
  - Use native elements (`button`, `a`, `input`, `select`, `textarea`) for interaction whenever possible.
  - If a custom control is necessary, include keyboard support and explicit state/relationship attributes.
- Form accessibility:
  - Every input requires an associated label.
  - Required fields must be clearly indicated in both label text and validation behavior.
  - Validation errors should appear near the field, include clear text, and be programmatically associated.
- Color and contrast:
  - Meet WCAG contrast guidance for text and UI controls.
  - Do not rely on color alone to communicate state (for example, add text/icon/state labels).
- Motion and reduced motion:
  - Respect `prefers-reduced-motion` and avoid essential information conveyed only through animation.

## Mobile Interaction Best Practices
- Touch targets:
  - Minimum control size remains `44px`.
  - Provide adequate spacing between adjacent controls to reduce accidental taps.
- Viewport and zoom:
  - Do not disable pinch zoom in viewport settings.
  - Ensure text remains readable without horizontal page scrolling at common phone widths.
- Forms on mobile:
  - Prefer single-column field flow on narrow screens.
  - Keep labels visible above or immediately beside fields at mobile breakpoints.
  - Preserve sufficient vertical spacing between form rows for touch use.
- Data tables on mobile:
  - Wrap wide tables in horizontal scroll containers with visible affordance.
  - For dense datasets, prefer card/stacked alternatives when readability drops.
  - Keep header-to-cell relationships clear when responsive transformations are applied.

## Accessibility and Mobile QA Checklist
- Keyboard-only pass:
  - Can open/close menus, navigate links, operate forms, and submit actions without mouse/touch.
- Focus pass:
  - Every interactive element shows a clear visible focus state.
- Contrast pass:
  - Text, controls, and focus indicators meet contrast requirements.
- Screen reader smoke test:
  - Page title, headings, labels, and actionable controls are announced clearly.
- Mobile pass:
  - No clipped controls, no unreadable text, no unintended horizontal page scrolling.
- Error handling pass:
  - Validation errors are clear, field-associated, and do not trap keyboard focus.

## Consistency Checklist
- Menu collapses correctly below `768px` and closes on outside click/escape.
- Forms stack cleanly on mobile and keep labels readable.
- Buttons remain tappable and do not overflow narrow widths.
- Tables remain readable on small screens and do not break layout.
- Color tokens (`var(--*)`) are used instead of hard-coded values unless explicitly required.
