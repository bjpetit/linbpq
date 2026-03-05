/*
Copyright 2026 LinBPQ/BPQ32 Contributors

This file is part of LinBPQ/BPQ32.

Common reusable CSS and JavaScript components for web interface templates.
This consolidates responsive menu systems and base styles to reduce duplication.
*/

#ifndef COMMON_WEB_COMPONENTS_H
#define COMMON_WEB_COMPONENTS_H

/*
 * TYPOGRAPHY
 *
 * COMMON_FONT_MONO   - Monospace font stack used for terminal output, code blocks,
 *                     config editors, and log views. IBM Plex Mono is loaded via
 *                     Google Fonts; the remaining entries are system fallbacks.
 *
 * COMMON_FONT_TITLE  - Proportional font stack used for headings and navigation.
 *                     Inter is loaded via Google Fonts; the remaining entries are
 *                     system UI fallbacks.
 *
 * COMMON_FONT_INTER_LINK - The <link> tag that loads both Inter and IBM Plex Mono
 *                     from Google Fonts in a single request. Update the URL here to
 *                     switch to a different hosted font — also update the font stack
 *                     macros above so they match.
 *
 * To switch fonts: change the family name in COMMON_FONT_MONO / COMMON_FONT_TITLE,
 * then update the Google Fonts URL in COMMON_FONT_INTER_LINK accordingly.
 */
#define COMMON_FONT_MONO "\"IBM Plex Mono\",ui-monospace,\"Cascadia Code\",\"Segoe UI Mono\",\"SF Mono\",\"Roboto Mono\",\"Courier New\",monospace"
#define COMMON_FONT_TITLE "\"Inter\",-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,Helvetica,Arial,sans-serif"
#define COMMON_FONT_INTER_LINK "<link href=\"https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500&family=Inter:wght@400;500;600;700&display=swap\" rel=\"stylesheet\">"
#define COMMON_HTML_DOCTYPE "<!DOCTYPE html>"
#define COMMON_HTML_HEAD_OPEN "<html><head>"
#define COMMON_HTML_HEAD_OPEN_DOCTYPE COMMON_HTML_DOCTYPE COMMON_HTML_HEAD_OPEN
#define COMMON_HTML_HEAD_OPEN_DOCTYPE_LANG_EN "<!DOCTYPE html><html lang=\"en\"><head>"
#define COMMON_HTML_META_UTF8 "<meta charset=utf-8>"
#define COMMON_HTML_META_UTF8_QUOTED "<meta charset=\"UTF-8\">"
#define COMMON_HTML_META_VIEWPORT "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
#define COMMON_HTML_META_VIEWPORT_COMPACT "<meta name=viewport content=\"width=device-width,initial-scale=1.0\">"
#define COMMON_HTML_META_VIEWPORT_SELF_CLOSING "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>"
#define COMMON_HTML_HEAD_COMMON COMMON_HTML_HEAD_OPEN_DOCTYPE COMMON_FONT_INTER_LINK
#define COMMON_HTML_HEAD_COMMON_LANG_EN COMMON_HTML_HEAD_OPEN_DOCTYPE_LANG_EN COMMON_FONT_INTER_LINK
#define COMMON_HTML_HEAD_UTF8_VIEWPORT COMMON_HTML_HEAD_COMMON COMMON_HTML_META_UTF8 COMMON_HTML_META_VIEWPORT
#define COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING COMMON_HTML_HEAD_COMMON COMMON_HTML_META_VIEWPORT_SELF_CLOSING
#define COMMON_HTML_HEAD_LANG_EN_UTF8Q_VIEWPORT COMMON_HTML_HEAD_COMMON_LANG_EN COMMON_HTML_META_UTF8_QUOTED COMMON_HTML_META_VIEWPORT
#define COMMON_HTML_HEAD_COMMON_STYLE_OPEN COMMON_HTML_HEAD_COMMON "<style>"
#define COMMON_HTML_HEAD_UTF8_VIEWPORT_STYLE_OPEN COMMON_HTML_HEAD_UTF8_VIEWPORT "<style>"
#define COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING_STYLE_OPEN COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING "<style>"

/*
 * COLOUR SCHEME
 *
 * All colours are defined as CSS custom properties (variables) inside
 * COMMON_CSS_ROOT below. Every page that uses COMMON_CSS_VARIABLES picks them
 * up automatically, so changing a value here affects the entire web interface.
 *
 * HOW TO CHANGE COLOURS:
 *   1. Edit the hex values in the :root { } block for the light-mode palette.
 *   2. Edit the matching values in the @media(prefers-color-scheme:dark) block
 *      for the dark-mode palette. Keep both in sync so the dark theme looks
 *      intentional rather than accidental.
 *   3. Rebuild and restart BPQ — no other files need touching.
 *
 * COLOUR TOKEN REFERENCE:
 *   --bg                Page background
 *   --surface           Card / panel background
 *   --primary           Brand / primary action colour (buttons, links)
 *   --primary-dark      Darker shade of primary used for :active states
 *   --on-primary        Text colour drawn on top of --primary backgrounds
 *   --text              Main body text
 *   --link              Hyperlink and navigation text
 *   --border            Standard UI border
 *   --border-light      Subtle divider lines
 *   --border-card       Card outline border
 *   --surface-hover     Background colour on hover
 *   --surface-soft      Lightly tinted surface (e.g. alternating rows)
 *   --focus-ring        Keyboard focus outline colour (accessibility)
 *   --table-header      Table header row background
 *   --table-stripe      Alternating table row background
 *   --table-selected    Highlighted / selected table row
 *   --fwd-none          Mail forwarding status: not forwarded
 *   --fwd-queued        Mail forwarding status: queued
 *   --fwd-sent          Mail forwarding status: sent
 *   --alert-error-*     Error banner background, border, and text
 *   --alert-warn-*      Warning banner background, border, and text
 *   --shadow-card       Box shadow for cards
 *   --shadow-overlay    Box shadow for dropdowns / overlays
 *   --overlay-hover     Semi-transparent overlay tint on hover
 */
#define COMMON_CSS_ROOT \
	":root{" \
	"--bg:#f4f4f4;" \
	"--surface:#fff;" \
	"--primary:#0056b3;" \
	"--primary-dark:#003d82;" \
	"--on-primary:#ffffff;" \
	"--border:#999999;" \
	"--border-light:#e2e8f0;" \
	"--text:#1f2937;" \
	"--link:#1f2937;" \
	"--surface-hover:#e9ecef;" \
	"--surface-soft:#eef2f7;" \
	"--focus-ring:#0056b3;" \
	"--table-header:#f0f0f0;" \
	"--table-stripe:#f2f2f2;" \
	"--table-selected:#dbeafe;" \
	"--fwd-none:#ffffff;" \
	"--fwd-queued:#fff59d;" \
	"--fwd-sent:#b7f7bf;" \
	"--alert-error-bg:#ffeeee;" \
	"--alert-error-border:#ffcccc;" \
	"--alert-error-text:#cc3333;" \
	"--alert-warn-bg:#ffffcc;" \
	"--alert-warn-border:#ffaa33;" \
	"--alert-warn-text:#aa6600;" \
	"--border-card:#d1d5db;" \
	"--shadow-card:0 1px 3px rgba(15,23,42,0.12);" \
	"--shadow-overlay:0 8px 20px rgba(15,23,42,0.22);" \
	"--overlay-hover:rgba(15,23,42,0.08);" \
	"}" \
	"@media(prefers-color-scheme:dark){:root{" \
	"--bg:#111827;" \
	"--surface:#1e2535;" \
	"--primary:#3b82f6;" \
	"--primary-dark:#2563eb;" \
	"--on-primary:#ffffff;" \
	"--border:#4b5563;" \
	"--border-light:#374151;" \
	"--text:#f9fafb;" \
	"--link:#93c5fd;" \
	"--surface-hover:#2d3748;" \
	"--surface-soft:#263044;" \
	"--focus-ring:#60a5fa;" \
	"--table-header:#374151;" \
	"--table-stripe:#253047;" \
	"--table-selected:#1e3a5f;" \
	"--fwd-none:#1e2535;" \
	"--fwd-queued:#665c14;" \
	"--fwd-sent:#1f4a2f;" \
	"--alert-error-bg:#4a1f27;" \
	"--alert-error-border:#7f1d2d;" \
	"--alert-error-text:#fecaca;" \
	"--alert-warn-bg:#4a3b13;" \
	"--alert-warn-border:#7c5e10;" \
	"--alert-warn-text:#fde68a;" \
	"--border-card:#374151;" \
	"--shadow-card:0 1px 3px rgba(0,0,0,0.45);" \
	"--shadow-overlay:0 10px 24px rgba(0,0,0,0.55);" \
	"--overlay-hover:rgba(255,255,255,0.06);" \
	"}}" \
	"*{box-sizing:border-box;}"

#define COMMON_BODY_BASE_CSS \
	"body{" \
	"font-family:" COMMON_FONT_MONO ";" \
	"background:var(--bg);" \
	"color-scheme:light dark;" \
	"margin:0;" \
	"padding:max(20px,env(safe-area-inset-left));" \
	"color:var(--text);" \
	"-webkit-font-smoothing:antialiased;" \
	"}" \
	"@supports(padding:max(0px)){" \
	"body{" \
	"padding:clamp(15px,4vw,20px);" \
	"padding-left:max(clamp(15px,4vw,20px),env(safe-area-inset-left));" \
	"padding-right:max(clamp(15px,4vw,20px),env(safe-area-inset-right));" \
	"}" \
	"}"

#define COMMON_HEADING_CSS \
	"h1{text-align:center;margin:0 0 15px;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.75rem,5vw,3rem);line-height:1.25;}" \
	"h2{text-align:center;margin:0 0 15px;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.5rem,4vw,2.25rem);line-height:1.25;}" \
	"h3{text-align:center;margin:0 0 15px;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.25rem,3vw,1.75rem);line-height:1.25;}"

#define COMMON_REDUCED_MOTION_CSS \
	"@media(prefers-reduced-motion:reduce){" \
	"*{animation-duration:0!important;transition-duration:0!important;}" \
	"}"

#define COMMON_CSS_VARIABLES \
	COMMON_CSS_ROOT \
	COMMON_BODY_BASE_CSS \
	COMMON_HEADING_CSS \
	COMMON_REDUCED_MOTION_CSS

// Responsive Menu System CSS
// Provides collapsible mobile menu with hamburger toggle.
// Use COMMON_MENU_CSS for sprintf-based templates and COMMON_MENU_CSS_TEMPLATE("%")
// for raw string templates that are not passed through printf/snprintf.
#define COMMON_MENU_CSS_TEMPLATE(PCT) \
	".menu-header{display:none;max-width:980px;margin:0 auto 10px;}" \
	".menu-toggle{" \
	"width:100" PCT ";" \
	"min-height:48px;" \
	"box-sizing:border-box;" \
	"border:1px solid var(--border);" \
	"border-radius:6px;" \
	"background:var(--surface);" \
	"font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);" \
	"font-family:" COMMON_FONT_TITLE ";" \
	"color:var(--link);" \
	"cursor:pointer;" \
	"touch-action:manipulation;" \
	"font-weight:500;" \
	"}" \
	".menu-toggle:active{background:var(--surface-hover);}" \
	".menu-toggle:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".menu{" \
	"display:flex;" \
	"flex-wrap:wrap;" \
	"justify-content:center;" \
	"gap:10px;" \
	"max-width:980px;" \
	"margin:0 auto;" \
	"}" \
	".menu a{" \
	"display:inline-flex;" \
	"align-items:center;" \
	"justify-content:center;" \
	"min-height:44px;" \
	"padding:10px 16px;" \
	"box-sizing:border-box;" \
	"background:var(--surface);" \
	"text-decoration:none;" \
	"border-radius:6px;" \
	"border:1px solid var(--border);" \
	"color:var(--link);" \
	"font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);" \
	"font-family:" COMMON_FONT_TITLE ";" \
	"cursor:pointer;" \
	"touch-action:manipulation;" \
	"}" \
	".menu a:hover{background:var(--surface-hover);}" \
	".menu a:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".menu a:active{background:var(--primary-dark);color:var(--on-primary);}" \
	"@media(max-width:768px){" \
	".menu-header{display:block;}" \
	".menu{display:none;flex-direction:column;align-items:stretch;gap:8px;}" \
	".menu.menu-open{display:flex;}" \
	".menu a{width:100" PCT ";text-align:center;min-height:48px;}" \
	"}"

// Max-width should be set per-application (980px for mail, 1100px for node)
#define COMMON_MENU_CSS COMMON_MENU_CSS_TEMPLATE("%%")

// WebMail menu base CSS variants.
// These keep existing page-specific behavior while reducing duplicated rule blocks.
#define COMMON_WEBMAIL_MENU_BTN_BASE_CSS_TEMPLATE(PCT) \
	".menu-header{display:none;margin-bottom:10px;}" \
	".menu-toggle{width:100" PCT ";min-height:48px;box-sizing:border-box;border:1px solid var(--border);border-radius:6px;background:var(--surface);font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";color:var(--link);}" \
	".menu-toggle:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".wm-menu{display:flex;flex-wrap:wrap;gap:10px;justify-content:center;margin-bottom:10px;}" \
	".wm-btn{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:10px 16px;box-sizing:border-box;background:var(--surface);border:1px solid var(--border);border-radius:6px;color:var(--link);text-decoration:none;font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";cursor:pointer;}" \
	".wm-btn:hover{background:var(--surface-hover);}" \
	".wm-btn:active{background:var(--primary-dark);color:var(--on-primary);}" \
	".wm-btn:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"

#define COMMON_WEBMAIL_MENU_LINK_TOUCH_BASE_CSS_TEMPLATE(PCT) \
	".menu-header{display:none;margin-bottom:10px;}" \
	".menu-toggle{width:100" PCT ";min-height:48px;box-sizing:border-box;border:1px solid var(--border);border-radius:6px;background:var(--surface);font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";color:var(--link);cursor:pointer;touch-action:manipulation;font-weight:500;}" \
	".menu-toggle:active{background:var(--surface-hover);}" \
	".menu-toggle:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".wm-menu{display:flex;flex-wrap:wrap;gap:10px;justify-content:center;margin-bottom:10px;}" \
	".wm-menu a{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:10px 16px;box-sizing:border-box;background:var(--surface);border:1px solid var(--border);border-radius:6px;color:var(--link);text-decoration:none;font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";cursor:pointer;touch-action:manipulation;}" \
	".wm-menu a:hover{background:var(--surface-hover);}" \
	".wm-menu a:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".wm-menu a:active{background:var(--primary-dark);color:var(--on-primary);}"

#define COMMON_WEBMAIL_MENU_LINK_TRANSITION_BASE_CSS_TEMPLATE(PCT) \
	".menu-header{display:none;margin-bottom:10px;}" \
	".menu-toggle{width:100" PCT ";min-height:48px;box-sizing:border-box;border:1px solid var(--border);border-radius:6px;background:var(--surface);font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";color:var(--link);}" \
	".menu-toggle:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".wm-menu{display:flex;flex-wrap:wrap;gap:10px;justify-content:center;margin-bottom:16px;}" \
	".wm-menu a{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:10px 16px;box-sizing:border-box;background:var(--surface);border:1px solid var(--border);border-radius:6px;color:var(--link);text-decoration:none;font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";cursor:pointer;transition:background 0.15s ease;}" \
	".wm-menu a:hover{background:var(--surface-hover);}" \
	".wm-menu a:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".wm-menu a:active{background:var(--primary-dark);color:var(--on-primary);}"

#define COMMON_WEBMAIL_MENU_BTN_BASE_CSS COMMON_WEBMAIL_MENU_BTN_BASE_CSS_TEMPLATE("%%")
#define COMMON_WEBMAIL_MENU_LINK_TOUCH_BASE_CSS COMMON_WEBMAIL_MENU_LINK_TOUCH_BASE_CSS_TEMPLATE("%%")
#define COMMON_WEBMAIL_MENU_LINK_TRANSITION_BASE_CSS COMMON_WEBMAIL_MENU_LINK_TRANSITION_BASE_CSS_TEMPLATE("%%")

// WebMail compose/check form shared CSS fragments.
// Keep these small and behavior-preserving so page-specific overrides remain local.
#define COMMON_WEBMAIL_FORM_LAYOUT_CORE_CSS \
	".form-row{display:flex;align-items:center;gap:10px;margin:8px 0;}" \
	".form-row.message-row{align-items:flex-start;}" \
	".form-row.message-row label{padding-top:10px;}" \
	".form-row.meta-row select{flex:0 0 120px;max-width:120px;}" \
	".form-row.meta-row input{flex:0 1 260px;max-width:260px;}"

#define COMMON_WEBMAIL_FORM_BUTTON_INTERACTION_CSS \
	".btn:hover,.buttons input:hover{background:var(--surface-soft);}" \
	".btn:focus-visible,.buttons input:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".buttons input[name=Send]:hover{background:var(--primary-dark);}"

#define COMMON_WEBMAIL_FORM_BUTTON_ROW_BASE_CSS \
	".buttons{display:flex;justify-content:flex-end;gap:10px;margin-top:14px;}" \
	".buttons input[name=Send]{background:var(--primary);border-color:var(--primary);color:var(--on-primary);}"

#define COMMON_WEBMAIL_FORM_MOBILE_STACK_BASE_CSS \
	".form-row{flex-direction:column;align-items:stretch;gap:6px;margin:6px 0;}" \
	".form-row label{flex:none;width:100%%;}" \
	".form-row.meta-row label{flex:none;width:100%%;}" \
	".form-row.meta-row select,.form-row.meta-row input{flex:1 1 auto;max-width:none;}"

// WebMail utility popup shared CSS fragments.
#define COMMON_WEBMAIL_POPUP_BODY_CENTER_CSS \
	"body{font-family:" COMMON_FONT_MONO ";margin:20px;text-align:center;background:var(--bg);color:var(--text);}"

#define COMMON_WEBMAIL_POPUP_SELECT_CSS \
	"select{padding:8px;border:1px solid var(--border);border-radius:4px;font-size:14px;}"

#define COMMON_WEBMAIL_POPUP_TABLE_CSS \
	"table{border-collapse:collapse;margin:20px auto;}"

#define COMMON_WEBMAIL_POPUP_TD_CSS \
	"td{padding:8px;border:1px solid var(--border);}"

#define COMMON_WEBMAIL_POPUP_TH_TD_CSS \
	"th,td{padding:8px;border:1px solid var(--border);}"

#define COMMON_WEBMAIL_POPUP_TH_CSS \
	"th{background:var(--table-header);}"

#define COMMON_WEBMAIL_POPUP_P_CSS \
	"p{margin:15px 0;}"

// Modem/TNC status page CSS for sprintf-style HTML templates.
#define COMMON_MODEM_STATUS_PAGE_CSS_FMT \
	COMMON_CSS_VARIABLES \
	"body{font-family:" COMMON_FONT_MONO ";margin:10px;background:var(--bg);color:var(--text);}" \
	"h2,h3{text-align:center;}" \
	"table{border-collapse:collapse;margin:20px auto;}" \
	"td{padding:8px;border:1px solid var(--border);}" \
	"textarea{width:100%%;max-width:600px;display:block;margin:20px auto;background:var(--surface);color:var(--text);border:1px solid var(--border);}" 

#define COMMON_MODEM_STATUS_TABLE_OPEN_HTML \
	"<table style=\"border-collapse: collapse; margin: 20px auto;\" border=1 cellpadding=2 cellspacing=0>"

#define COMMON_BTN_HOVER_NEUTRAL_CSS \
	".btn:hover{background:var(--surface-hover);}"

#define COMMON_BTN_HOVER_SOFT_CSS \
	".btn:hover{background:var(--surface-soft);}"

#define COMMON_BTN_FOCUS_RING_CSS \
	".btn:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"

#define COMMON_BTN_ACTIVE_DARK_CSS \
	"input.btn:active{background:var(--primary-dark);color:var(--on-primary);}" \
	"submit.btn:active{background:var(--primary-dark);color:var(--on-primary);}"

#define COMMON_BTN_PANEL_BASE_CSS \
	".btn{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:10px 16px;background:var(--surface);text-decoration:none;border-radius:6px;border:1px solid var(--border-card);color:var(--text);box-sizing:border-box;font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);cursor:pointer;}"

// Mail admin shared form/list snippets.
// Keep these generic so page templates can layer their own layout specifics.
#define COMMON_ADMIN_FIELD_THEME_CSS \
	"input[type=text],input[type=number],input[type=password],input[type=input],select,textarea{background:var(--surface-soft);color:var(--text);}" \
	"input[type=text]::placeholder,input[type=number]::placeholder,input[type=password]::placeholder,input[type=input]::placeholder,textarea::placeholder{color:var(--muted);}" \
	"input[type=text]:focus-visible,input[type=number]:focus-visible,input[type=password]:focus-visible,input[type=input]:focus-visible,select:focus-visible,textarea:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"

#define COMMON_ADMIN_LIST_TOGGLE_CSS_TEMPLATE(SELECTOR) \
	SELECTOR "{display:none;width:100%%;min-height:44px;padding:10px 12px;box-sizing:border-box;border:1px solid var(--border);border-radius:6px;background:var(--surface);color:var(--text);font-weight:600;cursor:pointer;text-align:left;margin-bottom:8px;flex-shrink:0;}" \
	SELECTOR ":focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"

#define COMMON_ADMIN_LIST_BODY_CSS_TEMPLATE(SELECTOR) \
	SELECTOR "{display:block;width:100%%;flex:1 1 auto;overflow-y:auto;box-sizing:border-box;min-height:0;}"

#define COMMON_ADMIN_COLLAPSIBLE_LIST_MOBILE_CSS_TEMPLATE(LIST_SELECTOR, BODY_SELECTOR, TOGGLE_SELECTOR, COLLAPSED_BODY_SELECTOR) \
	LIST_SELECTOR "{order:1;}" \
	BODY_SELECTOR "{max-height:240px;}" \
	TOGGLE_SELECTOR "{display:block;}" \
	COLLAPSED_BODY_SELECTOR "{display:none;}"

// Node web UI responsive snippets.
// Keep these as shared fragments to avoid repeating identical media queries
// inside individual Node page templates.
#define COMMON_NODE_MENU_MOBILE_CSS \
	"@media (max-width: 768px) { .menu a, .menu .btn, .dropdown { width: 100%%; text-align: center; min-height: 48px; } .dropdown-content { position: static; transform: none; width: 100%%; margin-top: 8px; box-shadow: none; } }"

#define COMMON_NODE_TERM_MOBILE_CSS \
	"@media (max-width: 768px) { .term-actions .btn { width: 100%%; } .term-container { height: calc(100vh - 200px); } }"

// Node web terminal I/O shared colors.
// Keep input and output panes in sync while leaving global theme variables untouched.
// Example green-on-black terminal override:
// :root{--term-io-bg:#000;--term-io-text:#00ff00;}
#define COMMON_NODE_TERM_IO_COLORS_CSS \
	":root{--term-io-bg:var(--surface);--term-io-text:var(--text);}"

// APRS shared web styles.
// RAW variants are for direct string templates; FMT variants are for sprintf/snprintf templates.
#define COMMON_APRS_BASE_CSS_TEMPLATE(PCT) \
	COMMON_CSS_ROOT \
	"body{font-family:" COMMON_FONT_MONO ";background:var(--bg);margin:0;padding:clamp(15px,4vw,20px);color:var(--text);-webkit-font-smoothing:antialiased;}" \
	".aprs-page-shell{max-width:1100px;margin:0 auto;padding:0 10px;}" \
	".aprs-info-content{max-width:80" PCT ";margin:0 auto;}" \
	"h1{text-align:center;margin:0.5em 0;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.75rem,5vw,3rem);line-height:1.25;}" \
	"h2{text-align:center;margin:0.5em 0;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.5rem,4vw,2.25rem);line-height:1.25;}" \
	"h3{text-align:center;margin:0.5em 0;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.25rem,3vw,1.75rem);line-height:1.25;}" \
	COMMON_REDUCED_MOTION_CSS \
	COMMON_MENU_CSS_TEMPLATE(PCT)

#define COMMON_APRS_CONTENT_CSS_TEMPLATE(PCT) \
	COMMON_APRS_BASE_CSS_TEMPLATE(PCT) \
	".table-wrap{width:100" PCT ";max-width:1100px;margin:0 auto 12px;overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".aprs-station-table{border-collapse:collapse;font-family:" COMMON_FONT_MONO ";}" \
	".aprs-station-table th,.aprs-station-table td{border:1px solid var(--border);padding:6px 8px;}" \
	".aprs-station-table th{background:var(--table-header);text-align:left;}" \
	".aprs-station-table tbody tr{background:var(--surface);}" \
	".aprs-station-table tbody tr:nth-child(even){background:var(--table-stripe);}" \
	".aprs-weather-table{border-collapse:collapse;margin:0 auto 12px;}" \
	".aprs-weather-table th,.aprs-weather-table td{border:1px solid var(--border);padding:6px 10px;}" \
	".aprs-weather-table th{background:var(--table-header);}" \
	".aprs-map{width:100" PCT ";max-width:600px;height:500px;border:0;display:block;margin:10px auto;}" \
	".aprs-note{text-align:center;font-style:italic;margin:8px 0;}" \
	".aprs-station-title{text-align:center;font-size:1.2rem;margin:12px 0;}" \
	".aprs-detail{text-align:center;margin:8px 0;}" \
	".aprs-lastposit{background:var(--surface-soft);border:1px solid var(--border);border-radius:4px;padding:8px 12px;max-width:800px;margin:10px auto;word-break:break-word;}"

#define COMMON_APRS_MAP_CSS_TEMPLATE(PCT) \
	COMMON_APRS_CONTENT_CSS_TEMPLATE(PCT) \
	"html,body{height:100" PCT ";width:100" PCT ";}" \
	"body{display:flex;flex-direction:column;padding:0;min-height:100vh;}" \
	".menu-wrapper{background:var(--bg);padding:10px 10px 0;}" \
	".menu{max-width:1100px;margin:0 auto 10px;}" \
	".menu a{font-size:16px;}" \
	".aprs-controls{display:flex;flex-wrap:wrap;justify-content:center;gap:12px;background:var(--surface);border:1px solid var(--border);border-radius:6px;max-width:1100px;margin:0 auto 10px;padding:10px 12px;}" \
	".aprs-controls label{display:inline-flex;align-items:center;gap:6px;min-height:32px;font-size:15px;}" \
	".aprs-controls input[type=checkbox]{width:18px;height:18px;margin:0;}" \
	"#map{flex:1 1 auto;min-height:320px;width:100" PCT ";margin:0;}" \
	"@media(max-width:768px){.aprs-controls{justify-content:flex-start;}}" \
	".popup{border:1px solid var(--border);margin:0;padding:0;font-size:12px;min-height:16px;box-shadow:none;}" \
	".leaflet-tooltip-left.popup::before{border-left-color:transparent;}" \
	".leaflet-tooltip-right.popup::before{border-right-color:transparent;}"

#define COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE(PCT) \
	COMMON_APRS_BASE_CSS_TEMPLATE(PCT) \
	"h2{text-align:center;margin:10px 0 14px;font-size:clamp(1.5rem,4vw,2.25rem);}" \
	".menu{max-width:1100px;margin:0 auto 14px;}" \
	".menu-header{max-width:1100px;}" \
	".aprs-msg-wrap{max-width:1100px;margin:0 auto;}" \
	".aprs-msg-table-wrap{width:100" PCT ";overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".aprs-msg-table{width:max-content;min-width:100" PCT ";border-collapse:collapse;background:var(--surface);}" \
	".aprs-msg-table th,.aprs-msg-table td{border:1px solid var(--border-card);padding:6px 8px;white-space:nowrap;}" \
	".aprs-msg-table th{background:var(--table-header);text-align:left;}" \
	".aprs-msg-table tbody tr:nth-child(even){background:var(--table-stripe);}" \
	".aprs-msg-form{max-width:900px;margin:0 auto;background:var(--surface);border:1px solid var(--border-card);border-radius:8px;padding:14px;box-shadow:var(--shadow-card);}" \
	".aprs-msg-form table{width:100" PCT ";border-collapse:collapse;}" \
	".aprs-msg-form td{padding:6px;vertical-align:top;}" \
	".aprs-msg-form input[type=text]{width:100" PCT ";padding:8px;border:1px solid var(--border-card);border-radius:4px;box-sizing:border-box;}" \
	".aprs-msg-actions{text-align:center;margin-top:10px;}" \
	".aprs-msg-actions input[type=submit]{min-height:44px;padding:10px 16px;background:var(--surface);border:1px solid var(--border-card);border-radius:6px;cursor:pointer;margin:0 6px;}" \
	".aprs-msg-actions input[type=submit]:hover{background:var(--surface-hover);}"

#define COMMON_APRS_CONTENT_CSS COMMON_APRS_CONTENT_CSS_TEMPLATE("%")
#define COMMON_APRS_MAP_CSS COMMON_APRS_MAP_CSS_TEMPLATE("%")
#define COMMON_APRS_MESSAGE_PAGE_CSS COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE("%")
#define COMMON_APRS_CONTENT_CSS_FMT COMMON_APRS_CONTENT_CSS_TEMPLATE("%%")
#define COMMON_APRS_MAP_CSS_FMT COMMON_APRS_MAP_CSS_TEMPLATE("%%")
#define COMMON_APRS_MESSAGE_PAGE_CSS_FMT COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE("%%")

// Common responsive table styles
// Use classes: table-wrap, node-table, node-table-stack, num/text/center
#define COMMON_TABLE_FONT_SIZE "clamp(0.75rem,0.65rem + 1vw,0.9375rem)"
#define COMMON_TABLE_FONT_SIZE_COMPACT "clamp(0.6875rem,0.62rem + 0.45vw,0.8125rem)"

#define COMMON_TABLE_CSS \
	".table-container{width:100%%;max-width:1100px;margin:0 auto 12px;overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".table-wrap{width:100%%;max-width:1100px;margin:0 auto 12px;overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".node-table{width:max-content;max-width:none;border-collapse:collapse;table-layout:auto;margin:0 auto;background:var(--surface);font-family:" COMMON_FONT_MONO ";font-size:" COMMON_TABLE_FONT_SIZE ";}" \
	".node-table caption{caption-side:top;text-align:left;font-family:" COMMON_FONT_TITLE ";font-size:clamp(0.9375rem,0.9rem + 0.2vw,1rem);font-weight:600;color:var(--text);padding:0 0 8px;}" \
	".node-table th,.node-table td{border:1px solid var(--border);padding:8px;vertical-align:top;}" \
	".node-table th,.node-table td{white-space:nowrap;}" \
	".node-table th{background:var(--table-header);text-align:left;}" \
	".node-table tbody tr:nth-child(even){background-color:var(--table-stripe);}" \
	".node-table td.num{text-align:right;}" \
	".node-table td.text{text-align:left;}" \
	".node-table td.center{text-align:center;}" \
	".node-table.routes-table,.node-table.compact-table{font-size:" COMMON_TABLE_FONT_SIZE_COMPACT ";}" \
	".node-table.routes-table th,.node-table.routes-table td,.node-table.compact-table th,.node-table.compact-table td{padding:5px 4px;}" \
	".node-table.node-detail-table{margin-left:auto;margin-right:auto;}" \
	".node-table a{color:var(--link);}" \
	"@media(max-width:768px){" \
	".node-table-stack thead{display:none;}" \
	".node-table-stack thead th{display:none;}" \
	".node-table-stack th{display:none;}" \
	".node-table-stack,.node-table-stack tbody,.node-table-stack tr,.node-table-stack td{display:block;width:100%%;}" \
	".node-table-stack tr{margin:0 0 10px;border:1px solid var(--border);border-radius:8px;background:var(--surface);padding:6px;}" \
	".node-table-stack tbody tr:nth-child(even){background:var(--surface);}" \
	".node-table-stack td{border:none;border-bottom:1px solid var(--border-light);text-align:left;padding:6px 4px;}" \
	".node-table-stack td.num,.node-table-stack td.center,.node-table-stack td.text{text-align:left;}" \
	".node-table-stack td:last-child{border-bottom:none;}" \
	".node-table-stack td::before{content:attr(data-label);display:inline-block;min-width:130px;font-weight:700;color:var(--text);margin-right:8px;}" \
	".node-table-stack.stats-table td::before{content:none;display:none;}" \
	"}" \
	".node-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(17ch,max-content));justify-content:center;gap:0;width:100%%;padding:8px;box-sizing:border-box;font-family:" COMMON_FONT_MONO ";font-size:" COMMON_TABLE_FONT_SIZE_COMPACT ";}" \
	".node-grid a{color:var(--link);white-space:nowrap;padding:3px 10px 3px 6px;display:block;border:1px solid var(--border);}" \
	".node-grid a:hover{background:var(--overlay-hover);}"

// Responsive Menu JavaScript
// Handles toggle, click-outside-to-close, and escape key
// Expects menu element ID to be 'mainMenu' or 'mailMenu'
// Expects toggle button ID to be 'menuToggle'
#define COMMON_MENU_JAVASCRIPT \
	"function getMenu(){return document.getElementById('mainMenu')||document.getElementById('mailMenu')||document.getElementById('chatMenu');}"\
	"function toggleMenu(event){" \
	"if(event)event.preventDefault();" \
	"var menu=getMenu();" \
	"var toggle=document.getElementById('menuToggle');" \
	"if(!menu||!toggle)return;" \
	"if(menu.classList.contains('menu-open')){" \
	"menu.classList.remove('menu-open');" \
	"toggle.textContent='Menu';" \
	"toggle.setAttribute('aria-expanded','false');" \
	"}else{" \
	"menu.classList.add('menu-open');" \
	"toggle.textContent='Close';" \
	"toggle.setAttribute('aria-expanded','true');" \
	"}" \
	"}" \
	"window.addEventListener('click',function(event){" \
	"var menu=getMenu();" \
	"var toggle=document.getElementById('menuToggle');" \
	"var header=document.querySelector('.menu-header');" \
	"if(menu&&toggle&&window.matchMedia('(max-width:768px)').matches){" \
	"var inMenu=menu.contains(event.target);" \
	"var inHeader=header&&header.contains(event.target);" \
	"if(!inMenu&&!inHeader){" \
	"menu.classList.remove('menu-open');" \
	"toggle.textContent='Menu';" \
	"}" \
	"}" \
	"});" \
	"window.addEventListener('keydown',function(event){" \
	"if(event.key==='Escape'){" \
	"var menu=getMenu();" \
	"if(menu&&menu.classList.contains('menu-open')){" \
	"var t=document.getElementById('menuToggle');" \
	"menu.classList.remove('menu-open');" \
	"if(t){t.textContent='Menu';t.setAttribute('aria-expanded','false');t.focus();}" \
	"}" \
	"}" \
	"});"

// Common Form Styles
// Provides responsive form-row pattern used throughout the application
#define COMMON_FORM_CSS \
	".form-section{" \
	"background:var(--surface);" \
	"padding:clamp(12px,4vw,20px);" \
	"border-radius:4px;" \
	"box-shadow:var(--shadow-card);" \
	"margin:15px 0;" \
	"}" \
	".form-row{" \
	"display:flex;" \
	"flex-wrap:wrap;" \
	"margin:clamp(10px,2vw,15px) 0;" \
	"gap:10px;" \
	"align-items:flex-start;" \
	"}" \
	".form-row label{" \
	"flex:1 1 clamp(100px,25%%,150px);" \
	"font-weight:bold;" \
	"font-size:clamp(1rem,0.95rem + 0.2vw,1.0625rem);" \
	"padding-top:2px;" \
	"}" \
	".form-row input[type=text],.form-row input[type=number],.form-row input[type=password],.form-row select{" \
	"flex:2 1 200px;" \
	"padding:8px;" \
	"border:1px solid var(--border);" \
	"border-radius:4px;" \
	"font-size:clamp(1rem,0.95rem + 0.2vw,1.0625rem);" \
	"touch-action:manipulation;" \
	"min-height:44px;" \
	"}" \
	".form-row input[type=text]:focus-visible,.form-row input[type=number]:focus-visible,.form-row input[type=password]:focus-visible,.form-row select:focus-visible{" \
	"outline:3px solid var(--focus-ring);outline-offset:2px;" \
	"}" \
	".form-row input[type=checkbox]{" \
	"margin:5px 5px 5px 0;" \
	"width:18px;" \
	"height:18px;" \
	"cursor:pointer;" \
	"}" \
	".form-row textarea{" \
	"width:100%%;" \
	"padding:8px;" \
	"border:1px solid var(--border);" \
	"border-radius:4px;" \
	"min-height:120px;" \
	"font-family:" COMMON_FONT_MONO ";" \
	"font-size:clamp(0.9375rem,0.9rem + 0.2vw,1rem);" \
	"touch-action:manipulation;" \
	"}" \
	".checkbox-group{" \
	"display:flex;" \
	"flex-wrap:wrap;" \
	"gap:15px;" \
	"align-items:center;" \
	"}" \
	"@media(max-width:768px){" \
	".form-row{" \
	"flex-direction:column;" \
	"gap:6px;" \
	"margin:8px 0;" \
	"}" \
	".form-row label{" \
	"flex:none;" \
	"width:100%%;" \
	"padding-top:0;" \
	"margin-bottom:4px;" \
	"}" \
	".form-row input[type=text],.form-row input[type=number],.form-row input[type=password],.form-row select{" \
	"width:100%%;" \
	"min-height:48px;" \
	"}" \
	".checkbox-group{" \
	"flex-direction:column;" \
	"align-items:flex-start;" \
	"}" \
	"}"

// Common Utility Styles
// Lightweight helper classes to replace one-off inline style attributes
#define COMMON_UTILITY_CSS \
	".text-center{text-align:center;}" \
	".my-20{margin:20px 0;}" \
	".flex-center-wrap-gap-10{display:flex;justify-content:center;gap:10px;flex-wrap:wrap;}" \
	".text-uppercase{text-transform:uppercase;}" \
	".muted-note{margin:10px 0 0 0;color:var(--text);font-size:clamp(0.875rem,0.84rem + 0.15vw,1rem);line-height:1.45;}" \
	".font-normal{font-weight:normal;}" \
	".inline-label{flex:1 1 100px;font-weight:bold;margin:0;padding-left:10px;}" \
	".flex-2-200{flex:2 1 200px;}" \
	".form-row input.input-w-80{flex:none;width:80px;}" \
	".form-row input.input-w-100{flex:none;width:100px;}"

// Common Button Styles
// Provides consistent button appearance with proper touch targets
#define COMMON_BUTTON_CSS \
	".buttons{" \
	"text-align:center;" \
	"margin:20px 0;" \
	"position:sticky;" \
	"bottom:0;" \
	"background:var(--surface);" \
	"padding:12px;" \
	"border-top:1px solid var(--border-light);" \
	"z-index:10;" \
	"}" \
	".buttons input,.buttons button{" \
	"background:var(--primary);" \
	"color:var(--on-primary);" \
	"padding:10px 20px;" \
	"border:none;" \
	"border-radius:4px;" \
	"cursor:pointer;" \
	"margin:5px;" \
	"touch-action:manipulation;" \
	"min-height:44px;" \
	"}" \
	".buttons input:hover,.buttons button:hover{" \
	"background:var(--primary-dark);" \
	"}" \
	".buttons input:focus-visible,.buttons button:focus-visible{" \
	"outline:3px solid var(--focus-ring);outline-offset:2px;" \
	"}" \
	"@media(max-width:768px){" \
	".buttons{" \
	"padding:10px;" \
	"margin:0 -20px -20px;" \
	"border-radius:0;" \
	"}" \
	".buttons input,.buttons button{" \
	"width:calc(50%%-6px);" \
	"}" \
	"}" \
	"@media(max-width:480px){" \
	".buttons input,.buttons button{" \
	"width:100%%;" \
	"min-height:48px;" \
	"margin:4px 0;" \
	"}" \
	"}"

// Common Signon/Login Page Styles
// Used by TermSignon, NodeSignon, MailSignon, ChatSignon in HTTPcode.c
#define COMMON_SIGNON_CSS \
	COMMON_CSS_VARIABLES \
	"body{font-family:" COMMON_FONT_TITLE ";font-size:clamp(1rem,0.96rem + 0.22vw,1.125rem);margin:20px;background:var(--bg);color:var(--text);}" \
	"h2,h3{text-align:center;font-family:" COMMON_FONT_TITLE ";}" \
	"h2{font-size:clamp(1.5rem,4vw,2.25rem);}" \
	"h3{font-size:clamp(1.25rem,3vw,1.75rem);}" \
	".form-container{max-width:400px;margin:20px auto;padding:20px;border:1px solid var(--border);border-radius:6px;background:var(--surface);}" \
	".form-row{margin:15px 0;}" \
	"label{display:block;margin-bottom:5px;}" \
	"input[type=text],input[type=password]{width:100%%;padding:8px;border:1px solid var(--border);border-radius:6px;box-sizing:border-box;min-height:44px;font-size:clamp(1rem,2vw,1rem);font-family:" COMMON_FONT_TITLE ";background:var(--surface);color:var(--text);}" \
	"input[type=text]:focus-visible,input[type=password]:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".btn{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:10px 16px;background:var(--surface);text-decoration:none;border-radius:6px;border:1px solid var(--border);color:var(--text);box-sizing:border-box;font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);cursor:pointer;margin-right:10px;margin-top:10px;}" \
	".btn:hover{background:var(--surface-hover);}" \
	".btn:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".btn:active{background:var(--primary-dark);color:var(--on-primary);}" \
	".alert-error{max-width:400px;margin:20px auto;padding:15px;background:var(--alert-error-bg);border:1px solid var(--alert-error-border);border-radius:6px;color:var(--alert-error-text);text-align:center;font-weight:bold;}" \
	".alert-warn{max-width:400px;margin:20px auto;padding:15px;background:var(--alert-warn-bg);border:1px solid var(--alert-warn-border);border-radius:6px;color:var(--alert-warn-text);text-align:center;font-weight:bold;}" \
	".msg-page{text-align:center;padding-top:50px;}"

// Helper: Build mail menu HTML
// Usage: sprintf(buffer, COMMON_MAIL_MENU, key, key, key, ..., key);
#define COMMON_MAIL_MENU \
	"<div class=\"menu-header\"><button id=\"menuToggle\" class=\"menu-toggle\" type=\"button\" aria-expanded=\"false\" aria-controls=\"mailMenu\" onclick=\"toggleMenu(event)\">Menu</button></div>" \
	"<div id=\"mailMenu\" class=\"menu\">" \
	"<a href=\"/Mail/Status?%s\">Status</a>" \
	"<a href=\"/Mail/Conf?%s\">Configuration</a>" \
	"<a href=\"/Mail/Users?%s\">Users</a>" \
	"<a href=\"/Mail/Msgs?%s\">Messages</a>" \
	"<a href=\"/Mail/FWD?%s\">Forwarding</a>" \
	"<a href=\"/Mail/Wel?%s\">Welcome Msgs &amp; Prompts</a>" \
	"<a href=\"/Mail/HK?%s\">Housekeeping</a>" \
	"<a href=\"/Mail/WP?%s\">WP Update</a>" \
	"<a href=\"/Webmail\">WebMail</a>" \
	"<a href=\"/\">Node Menu</a>" \
	"</div>"

// Helper: Build chat menu HTML
// Usage: sprintf(buffer, COMMON_CHAT_MENU, key, key);
#define COMMON_CHAT_MENU \
	"<div class=\"menu-header\"><button id=\"menuToggle\" class=\"menu-toggle\" type=\"button\" aria-expanded=\"false\" aria-controls=\"chatMenu\" onclick=\"toggleMenu(event)\">Menu</button></div>" \
	"<div id=\"chatMenu\" class=\"menu\">" \
	"<a href=\"/Chat/ChatStatus?%s\">Status</a>" \
	"<a href=\"/Chat/ChatConf?%s\">Configuration</a>" \
	"<a href=\"/\">Node Menu</a>" \
	"</div>"

#endif // COMMON_WEB_COMPONENTS_H
