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
  - Items: flex: 1 1 200px (minimum 200px width boxes)
  - Cards: white background, padding: 15px, box-shadow: 0 0 5px rgba(0,0,0,0.1), border-radius: 8px
  - Mobile: Single column layout (flex-direction: column)
- **`.buttons`**: Centered button container (text-align: center)

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
- Mobile-First & Fluid: Set table { width: 100%; max-width: 100%; } so tables fill the container but do not overflow.
- Horizontal Scrolling: For complex data, wrap the <table> in a <div> with overflow-x: auto. Ensure the table is still legible and consider adding a scroll prompt.
- Stacked/Card Layout (Mobile): At smaller breakpoints, convert rows to block-level elements (td { display: block; }) and use data-attributes to add labels, turning each row into a distinct "card".
- Fixed Table Layout: Use table-layout: fixed; to allow the browser to render the table faster and allow columns to size based on width attributes rather than content, improving Readability & Styling Best Practices
- Zebra Striping: Use tbody tr:nth-child(even) { background-color: #f2f2f2; } to improve readability for large data sets.
- Alignment: Align text to the left and numerical data to the right (left-to-right languages) to facilitate easier scanning.
- Spacing/Padding: Ensure generous padding on td and th elements to reduce visual clutter.
- Sticky Headers: Implement position: sticky on the <thead> or the first column to keep context while scrolling, improving the experience of long, narrow tables.
- Accessibility: Use <th> elements for headers, apply scope attributes, and use <caption to describe the table.

## Accessibility & Best Practices
- All form inputs have associated labels or clear labels within form-row divs
- Checkbox styling: margin 0 5px for spacing
- Full-width form elements at mobile breakpoint
- Center-aligned main headings and page titles
- Readonly text inputs styled consistently with regular inputs
