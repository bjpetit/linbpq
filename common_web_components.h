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
#define COMMON_HTML_CANONICAL_LINK_ONLY "<link rel=\"canonical\" id=\"bpqCanonical\" href=\"/\"><script>(function(){var l=document.getElementById('bpqCanonical');if(l&&window.location){l.href=window.location.origin+window.location.pathname;}})();</script>"
#define COMMON_HTML_CANONICAL_DYNAMIC COMMON_HTML_CANONICAL_LINK_ONLY "<script>" COMMON_THEME_COOKIE_INIT_JAVASCRIPT "</script>"
#define COMMON_HTML_HEAD_COMMON COMMON_HTML_HEAD_OPEN_DOCTYPE COMMON_FONT_INTER_LINK COMMON_HTML_CANONICAL_DYNAMIC
#define COMMON_HTML_HEAD_COMMON_LANG_EN COMMON_HTML_HEAD_OPEN_DOCTYPE_LANG_EN COMMON_FONT_INTER_LINK COMMON_HTML_CANONICAL_DYNAMIC
#define COMMON_HTML_HEAD_UTF8_VIEWPORT COMMON_HTML_HEAD_COMMON COMMON_HTML_META_UTF8 COMMON_HTML_META_VIEWPORT
#define COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING COMMON_HTML_HEAD_COMMON COMMON_HTML_META_VIEWPORT_SELF_CLOSING
#define COMMON_HTML_HEAD_LANG_EN_UTF8Q_VIEWPORT COMMON_HTML_HEAD_COMMON_LANG_EN COMMON_HTML_META_UTF8_QUOTED COMMON_HTML_META_VIEWPORT
#define COMMON_HTML_HEAD_COMMON_STYLE_OPEN COMMON_HTML_HEAD_COMMON "<style>"
#define COMMON_HTML_HEAD_UTF8_VIEWPORT_STYLE_OPEN COMMON_HTML_HEAD_UTF8_VIEWPORT "<style>"
#define COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING_STYLE_OPEN COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING "<style>"

/*
 * HTTP SECURITY RESPONSE HEADERS
 *
 * COMMON_HTTP_SECURITY_HEADERS must be inserted into every HTTP/1.1 200 OK
 * response that returns text/html content, placed after the last fixed header
 * and before the terminating blank line (\r\n).
 *
 * Usage in a sprintf format string:
 *   "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n"
 *   COMMON_HTTP_SECURITY_HEADERS "\r\n"
 *
 * Header notes:
 *   X-Content-Type-Options  - prevents MIME-sniffing attacks
 *   X-Frame-Options         - prevents clickjacking via iframe embedding
 *   Referrer-Policy         - limits referrer leakage across origins
 *   Strict-Transport-Security - instructs browsers to use HTTPS only
 *   Content-Security-Policy - restricts resource origins; 'unsafe-inline' is
 *                             required because pages use inline <style>/<script>
 *                             blocks; Google Fonts sources are whitelisted;
 *                             img-src includes https: for APRS tile servers;
 *                             script-src/style-src allow unpkg.com (Leaflet,
 *                             maplibre-gl) and ajax.googleapis.com (jQuery);
 *                             frame-src allows Google Maps embeds;
 *                             worker-src blob: allows maplibre-gl web workers
 */
#define COMMON_HTTP_SECURITY_HEADERS \
	"X-Content-Type-Options: nosniff\r\n" \
	"X-Frame-Options: SAMEORIGIN\r\n" \
	"Referrer-Policy: strict-origin-when-cross-origin\r\n" \
	"Strict-Transport-Security: max-age=31536000\r\n" \
	"Content-Security-Policy: default-src 'self' 'unsafe-inline' https://fonts.googleapis.com https://fonts.gstatic.com data: blob:; script-src 'self' 'unsafe-inline' https://unpkg.com https://ajax.googleapis.com blob:; style-src 'self' 'unsafe-inline' https://unpkg.com https://fonts.googleapis.com; img-src 'self' data: blob: https:; frame-src 'self' https://maps.google.com https://www.google.com; worker-src blob:;\r\n"

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
	":root,html[data-theme=\"light\"]{color-scheme:light;" \
	"--bg:#edf0f4;" \
	"--surface:#fff;" \
	"--primary:#0056b3;" \
	"--primary-dark:#003d82;" \
	"--on-primary:#ffffff;" \
	"--border:#767676;" \
	"--border-light:#e2e8f0;" \
	"--text:#1f2937;" \
	"--link:#0056b3;" \
	"--muted:#6b7280;" \
	"--surface-hover:#e9ecef;" \
	"--surface-soft:#eef2f7;" \
	"--focus-ring:#0056b3;" \
	"--table-header:#e6e9ee;" \
	"--table-stripe:#e8ebef;" \
	"--table-selected:#dbeafe;" \
	"--fwd-none:#ffffff;" \
	"--fwd-queued:#fff59d;" \
	"--fwd-sent:#b7f7bf;" \
	"--alert-error-bg:#ffeeee;" \
	"--alert-error-border:#ffcccc;" \
	"--alert-error-text:#cc3333;" \
	"--alert-warn-bg:#ffffcc;" \
	"--alert-warn-border:#ffaa33;" \
	"--alert-warn-text:#7c5100;" \
	"--border-card:#bfc5d0;" \
	"--shadow-card:0 2px 8px rgba(15,23,42,0.10),0 1px 2px rgba(15,23,42,0.06);" \
	"--shadow-overlay:0 8px 24px rgba(15,23,42,0.20),0 2px 6px rgba(15,23,42,0.10);" \
	"--overlay-hover:rgba(15,23,42,0.08);" \
	"}" \
	"@media(prefers-color-scheme:dark){:root{" \
	"--bg:#111827;" \
	"--surface:#202d42;" \
	"--primary:#1d4ed8;" \
	"--primary-dark:#1e40af;" \
	"--on-primary:#ffffff;" \
	"--border:#4b5563;" \
	"--border-light:#374151;" \
	"--text:#f9fafb;" \
	"--link:#93c5fd;" \
	"--muted:#9ca3af;" \
	"--surface-hover:#2d3748;" \
	"--surface-soft:#2b3850;" \
	"--focus-ring:#60a5fa;" \
	"--table-header:#374151;" \
	"--table-stripe:#26334e;" \
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
	"--shadow-card:0 2px 8px rgba(0,0,0,0.50),0 1px 2px rgba(0,0,0,0.30);" \
	"--shadow-overlay:0 10px 28px rgba(0,0,0,0.60),0 2px 6px rgba(0,0,0,0.35);" \
	"--overlay-hover:rgba(255,255,255,0.06);" \
	"}}" \
	"html[data-theme=\"dark\"]{color-scheme:dark;" \
	"--bg:#111827;--surface:#202d42;--primary:#1d4ed8;--primary-dark:#1e40af;--on-primary:#ffffff;" \
	"--border:#4b5563;--border-light:#374151;--text:#f9fafb;--link:#93c5fd;--muted:#9ca3af;" \
	"--surface-hover:#2d3748;--surface-soft:#2b3850;--focus-ring:#60a5fa;" \
	"--table-header:#374151;--table-stripe:#26334e;--table-selected:#1e3a5f;" \
	"--fwd-none:#1e2535;--fwd-queued:#665c14;--fwd-sent:#1f4a2f;" \
	"--alert-error-bg:#4a1f27;--alert-error-border:#7f1d2d;--alert-error-text:#fecaca;" \
	"--alert-warn-bg:#4a3b13;--alert-warn-border:#7c5e10;--alert-warn-text:#fde68a;" \
	"--border-card:#374151;--shadow-card:0 2px 8px rgba(0,0,0,0.50),0 1px 2px rgba(0,0,0,0.30);" \
	"--shadow-overlay:0 10px 28px rgba(0,0,0,0.60),0 2px 6px rgba(0,0,0,0.35);--overlay-hover:rgba(255,255,255,0.06);}" \
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

/* FOCUS RING STYLING - Consolidated
 * COMMON_FOCUS_RING_STYLE - Core style properties for keyboard focus states
 * Use with selectors: "selector{" COMMON_FOCUS_RING_STYLE "}"
 * Or with pseudo-selector: "selector" COMMON_FOCUS_RING_STATE
 */
#define COMMON_FOCUS_RING_STYLE \
	"outline:3px solid var(--focus-ring);outline-offset:2px;"

#define COMMON_FOCUS_RING_STATE \
	":focus-visible{" COMMON_FOCUS_RING_STYLE "}"

#define COMMON_LINK_CSS \
	"a{color:var(--link);}" \
	"a" COMMON_FOCUS_RING_STATE

#define COMMON_CSS_VARIABLES \
	COMMON_CSS_ROOT \
	COMMON_BODY_BASE_CSS \
	COMMON_HEADING_CSS \
	COMMON_REDUCED_MOTION_CSS \
	COMMON_LINK_CSS \
	"::selection{background:var(--primary);color:var(--on-primary);}" \
	"::-moz-selection{background:var(--primary);color:var(--on-primary);}" 

// Slim page base CSS for pages that already link bpq.css.
// bpq.css includes COMMON_CSS_ROOT, COMMON_REDUCED_MOTION_CSS, and COMMON_LINK_CSS,
// so those do not need to be repeated inline. Use this instead of COMMON_CSS_VARIABLES
// on any page that has COMMON_BPQ_CSS_LINK in its <head>.
#define COMMON_PAGE_BASE_CSS \
	COMMON_BODY_BASE_CSS \
	COMMON_HEADING_CSS \
	"::selection{background:var(--primary);color:var(--on-primary);}" \
	"::-moz-selection{background:var(--primary);color:var(--on-primary);}"

// Responsive Menu System CSS
// Provides collapsible mobile menu with hamburger toggle.
// Use COMMON_MENU_CSS for sprintf-based templates and COMMON_MENU_CSS_TEMPLATE("%")
// for raw string templates that are not passed through printf/snprintf.
#define COMMON_MENU_CSS_TEMPLATE(PCT) \
	".menu-header{display:none;max-width:var(--menu-max-width,980px);margin:0 auto 10px;}" \
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
	"max-width:var(--menu-max-width,980px);" \
	"margin:0 auto;" \
	"}" \
	".menu a,.menu .btn{" \
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
	"transition:background 0.15s ease;" \
	"}" \
	".menu a:hover,.menu .btn:hover{background:var(--surface-hover);}" \
	".menu a" COMMON_FOCUS_RING_STATE \
	".menu .btn" COMMON_FOCUS_RING_STATE \
	".menu a:active,.menu .btn:active{background:var(--primary-dark);color:var(--on-primary);}" \
	"@media(max-width:768px){" \
	".menu-header{display:block;}" \
	".menu{display:none;flex-direction:column;align-items:stretch;gap:8px;}" \
	".menu.menu-open{display:flex;}" \
	".menu a,.menu .btn{width:100" PCT ";text-align:center;min-height:48px;}" \
	"}"

// Max-width should be set per-application (980px for mail, 1100px for node)
#define COMMON_MENU_CSS COMMON_MENU_CSS_TEMPLATE("%%")

/* CONSOLIDATED WEBMAIL MENU CSS
 * Unified base styling with three specialized variants.
 * All variants share common structure: toggle button + flex menu container with items.
 * Differences parameterized: link selector, toggle behavior, item effects, container spacing.
 */

#define COMMON_WEBMAIL_MENU_BASE_CSS_TEMPLATE(PCT, LINK_SEL, TOGGLE_EXTRA, ITEM_EXTRA, MARGIN) \
	".menu-header{display:none;margin-bottom:" MARGIN ";}" \
	".menu-toggle{width:100" PCT ";min-height:48px;box-sizing:border-box;border:1px solid var(--border);border-radius:6px;background:var(--surface);font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";color:var(--link);" TOGGLE_EXTRA "}" \
	".menu-toggle" COMMON_FOCUS_RING_STATE \
	".wm-menu{display:flex;flex-wrap:wrap;gap:10px;justify-content:center;margin-bottom:" MARGIN ";}" \
	 LINK_SEL "{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:10px 16px;box-sizing:border-box;background:var(--surface);border:1px solid var(--border);border-radius:6px;color:var(--link);text-decoration:none;font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";cursor:pointer;" ITEM_EXTRA "}" \
	 LINK_SEL ":hover{background:var(--surface-hover);}" \
	 LINK_SEL COMMON_FOCUS_RING_STATE \
	 LINK_SEL ":active{background:var(--primary-dark);color:var(--on-primary);}"

/* Variant 1: Button-based menu items (no special effects) */
#define COMMON_WEBMAIL_MENU_BTN_BASE_CSS_TEMPLATE(PCT) \
	COMMON_WEBMAIL_MENU_BASE_CSS_TEMPLATE(PCT, ".wm-btn", "", "", "10px")

/* Variant 2: Link-based menu with touch interaction on toggle and items */
#define COMMON_WEBMAIL_MENU_LINK_TOUCH_BASE_CSS_TEMPLATE(PCT) \
	COMMON_WEBMAIL_MENU_BASE_CSS_TEMPLATE(PCT, ".wm-menu a", "cursor:pointer;touch-action:manipulation;font-weight:500;", "touch-action:manipulation;", "10px") \
	".menu-toggle:active{background:var(--surface-hover);}"

/* Variant 3: Link-based menu with smooth transition effect */
#define COMMON_WEBMAIL_MENU_LINK_TRANSITION_BASE_CSS_TEMPLATE(PCT) \
	COMMON_WEBMAIL_MENU_BASE_CSS_TEMPLATE(PCT, ".wm-menu a", "", "transition:background 0.15s ease;", "16px")

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

// Reused WebMail/admin shell and title snippets.
#define COMMON_WEBMAIL_SHELL_CSS_TEMPLATE(MAX_WIDTH) \
	".wm-shell{max-width:" MAX_WIDTH ";margin:0 auto;}"

#define COMMON_WEBMAIL_SHELL_980_CSS COMMON_WEBMAIL_SHELL_CSS_TEMPLATE("980px")
#define COMMON_WEBMAIL_SHELL_1100_CSS COMMON_WEBMAIL_SHELL_CSS_TEMPLATE("1100px")

#define COMMON_WEBMAIL_TITLE_STRONG_CSS \
	".wm-title{text-align:center;margin:0 0 16px;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.25rem,3vw,1.75rem);font-weight:600;color:var(--text);}"

#define COMMON_SECTION_TITLE_TEXT_CSS \
	".section-title{text-align:center;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.25rem,1.05rem + 0.9vw,1.75rem);font-weight:700;margin:12px 0 0;color:var(--text);}"

#define COMMON_SECTION_TITLE_CSS COMMON_SECTION_TITLE_TEXT_CSS

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
	"th,td{border:1px solid var(--border);padding:8px;}" \
	"th{background:var(--table-header);font-weight:600;}" \
	"tbody tr:nth-child(even){background:var(--table-stripe);}" \
	"textarea{width:100%%;max-width:600px;display:block;margin:20px auto;background:var(--surface);color:var(--text);border:1px solid var(--border);}" 

#define COMMON_MODEM_STATUS_TABLE_OPEN_HTML \
	"<table style=\"border-collapse: collapse; margin: 20px auto;\" border=1 cellpadding=2 cellspacing=0>"

/* CONSOLIDATED BUTTON STYLES
 * All .btn elements share this base styling and interaction states.
 * This replaces the previous fragmented COMMON_BTN_* macros.
 */
#define COMMON_BTN_BASE_COMPLETE_CSS \
	".btn{" \
	"display:inline-flex;align-items:center;justify-content:center;" \
	"min-height:44px;padding:10px 16px;" \
	"background:var(--surface);border:1px solid var(--border-card);" \
	"text-decoration:none;border-radius:6px;" \
	"color:var(--text);box-sizing:border-box;" \
	"font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);" \
	"cursor:pointer;touch-action:manipulation;" \
	"}" \
	".btn:hover{background:var(--surface-hover);}" \
	".btn" COMMON_FOCUS_RING_STATE \
	"input.btn:active,button[type=submit].btn:active{background:var(--primary-dark);color:var(--on-primary);}"

/* Backward-compatibility aliases for existing code */
#define COMMON_BTN_PANEL_BASE_CSS COMMON_BTN_BASE_COMPLETE_CSS
#define COMMON_BTN_HOVER_NEUTRAL_CSS ""
#define COMMON_BTN_ACTIVE_DARK_CSS ""

/* CONSOLIDATED INPUT STYLING
 * All text inputs share this base styling and focus states.
 * COMMON_INPUT_BASE_COMPLETE_CSS: Core styling for all input types (text, email, password, number, date, search, tel, url, select, textarea)
 * COMMON_INPUT_FOCUS_CSS: Focus-visible state — alias for COMMON_FOCUS_RING_STATE
 */
#define COMMON_INPUT_FOCUS_CSS COMMON_FOCUS_RING_STATE

#define COMMON_INPUT_BASE_COMPLETE_CSS \
	"input[type=text],input[type=email],input[type=password],input[type=number],input[type=date]," \
	"input[type=search],input[type=tel],input[type=url],select,textarea{" \
	"padding:10px 12px;line-height:1.5;border:1px solid var(--border);border-radius:6px;" \
	"background:var(--surface-soft);color:var(--text);box-sizing:border-box;min-height:44px;" \
	"}" \
	"input[type=text]::placeholder,input[type=email]::placeholder,input[type=password]::placeholder,input[type=number]::placeholder," \
	"input[type=date]::placeholder,input[type=search]::placeholder,input[type=tel]::placeholder,input[type=url]::placeholder," \
	"textarea::placeholder{color:var(--muted);}" \
	"input[type=text]" COMMON_INPUT_FOCUS_CSS \
	"input[type=email]" COMMON_INPUT_FOCUS_CSS \
	"input[type=password]" COMMON_INPUT_FOCUS_CSS \
	"input[type=number]" COMMON_INPUT_FOCUS_CSS \
	"input[type=date]" COMMON_INPUT_FOCUS_CSS \
	"input[type=search]" COMMON_INPUT_FOCUS_CSS \
	"input[type=tel]" COMMON_INPUT_FOCUS_CSS \
	"input[type=url]" COMMON_INPUT_FOCUS_CSS \
	"select" COMMON_INPUT_FOCUS_CSS \
	"textarea" COMMON_INPUT_FOCUS_CSS

/* Backward-compatibility alias for base padding/sizing only */
#define COMMON_INPUT_BASE_CSS \
	"padding:10px 12px;line-height:1.5;border:1px solid var(--border);border-radius:6px;" \
	"background:var(--surface-soft);color:var(--text);box-sizing:border-box;min-height:44px;"

/* COMMON_FORM_ROW_INPUT_CSS - Standardized styling for .form-row containers with inputs.
 * Combines layout, width constraints, and responsive behavior for form fields.
 */
#define COMMON_FORM_ROW_INPUT_CSS \
	".form-row input[type=text],.form-row input[type=email],.form-row input[type=password],.form-row input[type=number],.form-row input[type=date]," \
	".form-row input[type=search],.form-row input[type=tel],.form-row input[type=url]," \
	".form-row select{" COMMON_INPUT_BASE_CSS "font-family:" COMMON_FONT_MONO ";flex:1 1 auto;min-width:0;}" \
	".form-row input[type=text]{flex:none;width:220px;}" \
	".form-row input[type=number]{flex:none;width:96px;max-width:100%;}" \
	".form-row input:focus-visible,.form-row select:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	"@media (max-width:768px){.form-row input[type=text],.form-row input[type=number]{width:100%;flex:1 1 auto;}}"

#define COMMON_CHAT_STATUS_PAGE_CSS_TEMPLATE(PCT) \
	".chat-shell{max-width:1100px;margin:0 auto;}" \
	".chat-layout{display:grid;grid-template-columns:1fr 1fr;gap:14px;align-items:start;}" \
	".chat-section{padding:10px;min-width:0;" COMMON_CARD_CHROME_CSS "}" \
	".chat-section h4{margin:4px 0 10px 0;font-size:clamp(14px,2vw,16px);text-align:center;}" \
	".chat-section-wide{grid-column:1 / -1;}" \
	".chat-grid{width:100" PCT ";border-collapse:collapse;}" \
	".chat-grid th,​.chat-grid td{border:1px solid var(--border);padding:8px;}" \
	".chat-grid th{background:var(--table-header);font-weight:600;}" \
	".chat-grid tbody tr:nth-child(even){background:var(--table-stripe);}" \
	".chat-grid tbody tr:hover{background:var(--surface-hover);}" \
	".chat-grid tr.selectable{cursor:pointer;}" \
	".chat-grid tr.selected td{background:var(--table-selected);}" \
	".chat-status-form{margin:0;}" \
	".chat-actions input{min-width:170px;}" \
	"@media (max-width:900px){.chat-layout{grid-template-columns:1fr;}.chat-section-wide{grid-column:auto;}}" \
	"@media (max-width:768px){body{padding:10px;}.chat-grid{font-size:13px;}.chat-grid th,.chat-grid td{padding:7px 6px;}}"

#define COMMON_CHAT_STATUS_PAGE_CSS COMMON_CHAT_STATUS_PAGE_CSS_TEMPLATE("%")
#define COMMON_CHAT_STATUS_PAGE_CSS_FMT COMMON_CHAT_STATUS_PAGE_CSS_TEMPLATE("%%")

#define COMMON_SCROLL_OUTPUT_JAVASCRIPT \
	"function ScrollOutput()" \
	"{var textarea = document.getElementById('textarea');" \
	"if(textarea)textarea.scrollTop = textarea.scrollHeight;}"

#define COMMON_CHAT_STATUS_JAVASCRIPT \
	"var Selected;" \
	"var Inpval;" \
	"var SelectedStream = 0;" \
	"window.__bpqChatStatusRefreshTimer = window.__bpqChatStatusRefreshTimer || 0;" \
	"function initialize(){Inpval=document.getElementById('inpval');if(!window.__bpqChatStatusRefreshTimer){window.__bpqChatStatusRefreshTimer=window.setInterval(condRefresh,10000);}}" \
	"function SelectRow(newRow){var cell=document.getElementById('cell_'+newRow);var Last=Selected;var row=cell?cell.parentNode:null;if(!row)return;Selected=row;SelectedStream=newRow;row.classList.add('selected');if(Last){Last.classList.remove('selected');}if(row==Last){SelectedStream=0;row.classList.remove('selected');}Inpval.value=SelectedStream;}" \
	"function condRefresh(){if(SelectedStream==0){location.reload(true);}}"

// Reusable panel/card chrome fragments for page-specific containers.
#define COMMON_PANEL_CHROME_CSS \
	"border:1px solid var(--border);border-radius:8px;background:var(--surface);"

#define COMMON_CARD_CHROME_CSS \
	COMMON_PANEL_CHROME_CSS \
	"box-shadow:var(--shadow-card);"

// Compact variants with 6px border-radius
#define COMMON_PANEL_CHROME_CSS_6PX \
	"border:1px solid var(--border);border-radius:6px;background:var(--surface);"

#define COMMON_CARD_CHROME_CSS_6PX \
	COMMON_PANEL_CHROME_CSS_6PX \
	"box-shadow:var(--shadow-card);"

// Mail admin shared form/list snippets.
// Now uses consolidated COMMON_INPUT_BASE_COMPLETE_CSS for standardized input styling.
#define COMMON_ADMIN_FIELD_THEME_CSS \
	COMMON_INPUT_BASE_COMPLETE_CSS

// Node page shared heading classes. Safe (no %%) — included in COMMON_BPQ_CSS_CONTENT
// so the Node Menu page (which links bpq.css) gets them without a second request.
#define COMMON_NODE_H2_H3_CSS \
	".node-h2{text-align:center;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.1rem,2.5vw,1.4rem);}" \
	".node-h3{text-align:center;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1rem,2vw,1.2rem);}"

// Mail BBS status page layout — served via bpq.css so StatusPage only needs content markup.
// Uses single % (not %%) — included in COMMON_BPQ_CSS_CONTENT, not used as sprintf format.
#define COMMON_MAIL_STATUS_CSS \
	".status-grid{width:100%;}" \
	".status-actions input{min-width:160px;}" \
	".stats-section{max-width:560px;margin:16px auto 0;padding:clamp(12px,2vw,18px);" COMMON_CARD_CHROME_CSS_6PX "}" \
	".stat-row{display:flex;align-items:center;gap:12px;margin:8px 0;}" \
	".stat-row label{flex:1 1 220px;font-weight:600;font-size:clamp(0.8125rem,1.5vw,0.9375rem);line-height:1.3;}" \
	".stat-row input{flex:0 0 130px;max-width:130px;" COMMON_INPUT_BASE_CSS "font-family:" COMMON_FONT_MONO ";font-size:clamp(0.875rem,2vw,1rem);font-variant-numeric:tabular-nums;text-align:right;}" \
	"@media(max-width:768px){.status-grid th,.status-grid td{padding:8px 6px;}.stats-section{margin-top:12px;padding:12px;}.stat-row{flex-direction:column;align-items:stretch;gap:6px;}.stat-row label{flex:none;width:100%;}.stat-row input{flex:1 1 auto;max-width:none;width:100%;min-height:48px;text-align:left;}}"

// White Pages detail form — AJAX-injected into mail pages that already load bpq.css.
// No %% patterns; safe to include in COMMON_BPQ_CSS_CONTENT.
#define COMMON_WP_DETAIL_CSS \
	".wp-form{margin:0;}" \
	".wp-box{background:var(--surface);padding:15px;border-radius:4px;margin:15px 0;box-shadow:var(--shadow-card);}" \
	".wp-row{display:flex;flex-wrap:wrap;margin:10px 0;gap:10px;align-items:flex-start;}" \
	".wp-label{flex:1 1 100px;font-weight:bold;padding-top:2px;}" \
	".wp-input-text{flex:2 1 200px;" COMMON_INPUT_BASE_CSS "}" \
	".wp-input-small{flex:none;width:100px;" COMMON_INPUT_BASE_CSS "}" \
	".wp-input-readonly{background:var(--surface-soft);color:var(--text);}" \
	".wp-actions{text-align:center;margin:20px 0;position:sticky;bottom:0;background:var(--surface);padding:12px;border-top:1px solid var(--border-light);z-index:10;}" \
	".wp-btn{background:var(--primary);color:var(--on-primary);padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:5px;touch-action:manipulation;min-height:44px;}" \
	".wp-btn:hover{background:var(--primary-dark);}" \
	".wp-btn:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"

// Mail message detail form — AJAX-injected into mail pages that already load bpq.css.
// Uses single % for CSS (corrects the latent %% bug from when this was a sprintf format).
#define COMMON_MAIL_DETAIL_CSS \
	"h3{text-align:center;margin-bottom:20px;}" \
	".form-section{padding:clamp(12px,4vw,20px);margin:15px 0;" COMMON_CARD_CHROME_CSS "}" \
	".form-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:16px 20px;margin:16px 0;}" \
	".form-field{display:flex;flex-direction:column;gap:6px;}" \
	".form-field label{font-weight:600;font-size:clamp(13px,1.5vw,15px);color:var(--text);text-transform:uppercase;letter-spacing:0.3px;}" \
	".form-field input,.form-field select{" COMMON_INPUT_BASE_CSS "font-family:" COMMON_FONT_MONO ";font-size:clamp(14px,2vw,16px);transition:border-color 0.15s ease,box-shadow 0.15s ease;}" \
	".form-field input" COMMON_INPUT_FOCUS_CSS \
	".form-field select" COMMON_INPUT_FOCUS_CSS \
	".form-row-full{grid-column:1/-1;}" \
	"input{background:var(--surface-soft);color:var(--text);}" \
	"input[readonly]{cursor:not-allowed;}" \
	"input.uppercase{text-transform:uppercase;}" \
	".table-container{background:var(--surface);border-radius:8px;box-shadow:var(--shadow-card);overflow-x:auto;margin:15px 0;}" \
	"table{width:100%;border-collapse:collapse;table-layout:fixed;}" \
	"th,td{padding:12px 14px;border:1px solid var(--border);text-align:center;font-size:14px;width:12.5%;}" \
	"th{background:var(--table-header);font-weight:600;text-align:left;}" \
	"th[colspan]{text-align:center;width:100%;}" \
	"td{cursor:pointer;transition:opacity 0.15s ease;}" \
	"td:hover:not(:empty){opacity:0.8;}" \
	".fwd-none{background-color:var(--fwd-none);}" \
	".fwd-queued{background-color:var(--fwd-queued);}" \
	".fwd-sent{background-color:var(--fwd-sent);}" \
	"tbody tr:nth-child(even){background:var(--table-stripe);}" \
	"tbody tr:hover{background:var(--surface-soft);transition:background 0.15s ease;}" \
	".status-legend{background:var(--surface);padding:16px;border-radius:8px;box-shadow:var(--shadow-card);margin:15px 0;font-size:14px;color:var(--text);line-height:1.6;}" \
	".status-legend strong{color:var(--text);display:block;margin-bottom:8px;}" \
	".buttons{display:flex;flex-wrap:wrap;gap:10px;margin:20px 0;}" \
	".buttons input,.buttons button,.buttons a button{flex:1 1 auto;min-width:140px;background:var(--primary);color:var(--on-primary);padding:12px 20px;border:none;border-radius:6px;cursor:pointer;font-size:clamp(14px,1.5vw,16px);font-weight:500;transition:background 0.15s ease;min-height:44px;}" \
	".buttons input:hover,.buttons button:hover,.buttons a button:hover{background:var(--primary-dark);}" \
	".buttons input:focus-visible,.buttons button:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".buttons a{text-decoration:none;}" \
	"@media(max-width:768px){.form-grid{grid-template-columns:1fr;gap:12px;}" \
	".form-row-full{grid-column:1;}" \
	".form-field label{font-size:13px;margin-bottom:4px;}" \
	".form-field input,.form-field select{min-height:44px;font-size:16px;}" \
	"table{font-size:13px;}" \
	"th,td{padding:10px 8px;}" \
	".buttons{flex-direction:column;gap:8px;}" \
	".buttons input,.buttons button,.buttons a button{width:100%;min-width:0;min-height:48px;}" \
	"}" \
	"@media(max-width:480px){body{padding:clamp(8px,2vw,12px);padding-left:max(clamp(8px,2vw,12px),env(safe-area-inset-left));}" \
	".form-section{padding:12px;margin:8px 0;}" \
	"table{font-size:12px;}" \
	"th,td{padding:8px 6px;}" \
	"}"

// Forwarding detail form — AJAX-injected into FwdPage which already loads bpq.css and
// provides COMMON_ADMIN_FIELD_THEME_CSS via its own inline <style>. No %% patterns needed.
#define COMMON_FWD_DETAIL_CSS \
	".fwd-detail-form{max-width:100%;}" \
	".fwd-detail-form h3{text-align:center;margin-bottom:20px;}" \
	".fwd-textarea-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px;margin:15px 0;}" \
	".fwd-textarea-grid label{display:block;font-weight:600;margin-bottom:5px;font-size:clamp(0.8125rem,1.5vw,0.9375rem);text-align:center;}" \
	".fwd-textarea-grid textarea{width:100%;min-height:120px;padding:clamp(10px,1vw,14px) clamp(12px,1.5vw,16px);line-height:1.6;box-sizing:border-box;border:1px solid var(--border);border-radius:4px;font-family:" COMMON_FONT_MONO ";font-size:clamp(0.875rem,2vw,1rem);resize:vertical;}" \
	".fwd-textarea-wide{grid-column:span 2;}" \
	".fwd-detail-row{display:flex;flex-wrap:wrap;margin:10px 0;gap:10px;align-items:center;}" \
	".fwd-detail-row label{font-weight:600;font-size:clamp(0.8125rem,1.5vw,0.9375rem);}" \
	".fwd-secondary-label{margin-left:15px;}" \
	".fwd-detail-row input[type=text],.fwd-detail-row input[type=number]{padding:clamp(10px,1vw,14px) clamp(12px,1.5vw,16px);line-height:1.5;box-sizing:border-box;border:1px solid var(--border);border-radius:4px;min-height:44px;font-size:clamp(0.875rem,2vw,1rem);}" \
	".fwd-detail-row input[type=checkbox]{margin:0 5px;width:18px;height:18px;}" \
	".fwd-bbsha{flex:1 1 320px;max-width:400px;}" \
	".fwd-buttons{display:flex;gap:10px;justify-content:center;margin-top:20px;flex-wrap:wrap;}" \
	".fwd-buttons input{background:var(--primary);color:var(--on-primary);padding:clamp(10px,1.5vw,16px) clamp(16px,2vw,28px);border:none;border-radius:4px;cursor:pointer;min-height:44px;font-size:clamp(0.875rem,1.5vw,1rem);}" \
	".fwd-buttons input:hover{background:var(--primary-dark);}" \
	".fwd-buttons input:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".fwd-buttons input.fwd-copy-call{background:var(--surface-soft);color:var(--text);border:1px solid var(--border);max-width:120px;}" \
	"@media(max-width:768px){" \
	".fwd-textarea-grid{grid-template-columns:1fr;}" \
	".fwd-textarea-wide{grid-column:span 1;}" \
	".fwd-detail-row{flex-direction:column;align-items:flex-start;}" \
	".fwd-detail-row input[type=text],.fwd-detail-row input[type=number],.fwd-textarea-grid textarea{width:100%;min-height:48px;}" \
	".fwd-bbsha{max-width:none;}" \
	".fwd-buttons{flex-direction:column;}" \
	".fwd-buttons input{width:100%;min-height:48px;}" \
	"}"

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

#define COMMON_NODE_WEBPROC_DROPDOWN_CSS \
	".dropbtn {position: relative; border: 1px solid var(--border);padding:1px;}\r\n" \
	".dropdown {position: relative; display: inline-block;}\r\n" \
	".dropdown-content {display: none; position: absolute;background-color: var(--surface); min-width: 160px; box-shadow: var(--shadow-overlay); z-index: 1;}\r\n" \
	".dropdown-content a {color: var(--text); padding: 1px 1px;text-decoration:none;display:block;}" \
	".dropdown-content a:hover {background-color: var(--surface-soft);}\r\n" \
	".dropdown:hover .dropdown-content {display: block;}\r\n" \
	".dropdown:hover .dropbtn {background-color: var(--surface-hover);}\r\n" \
	COMMON_BTN_ACTIVE_DARK_CSS

#define COMMON_NODE_TERM_MOBILE_CSS \
	"@media (max-width: 768px) { .term-actions .btn { width: 100%%; } .term-container { height: calc(100vh - 200px); } }"

// Node web terminal I/O shared colors.
// Keep input and output panes in sync while leaving global theme variables untouched.
// Example green-on-black terminal override:
// :root{--term-io-bg:#000;--term-io-text:#00ff00;}
#define COMMON_NODE_TERM_IO_COLORS_CSS \
	":root{--term-io-bg:var(--surface);--term-io-text:var(--text);}"

#define COMMON_THEME_SELECTOR_CSS \
	".theme-sel{position:fixed;top:10px;right:10px;display:flex;gap:4px;z-index:999;}" \
	".tbtn{background:var(--surface);border:1px solid var(--border);border-radius:6px;padding:4px 8px;cursor:pointer;font-size:1rem;line-height:1;color:var(--text);min-width:34px;min-height:34px;display:inline-flex;align-items:center;justify-content:center;}" \
	".tbtn.active{background:var(--primary);color:var(--on-primary);border-color:var(--primary);}" \
	".tbtn:hover:not(.active){background:var(--surface-hover);}"

#define COMMON_THEME_SELECTOR_HTML \
	"<div class='theme-sel'>" \
	"<button class='tbtn' data-tbtn='system' onclick='bpqSetThemeMode(\"system\")' title='System theme' aria-label='System theme'>&#9881;</button>" \
	"<button class='tbtn' data-tbtn='light' onclick='bpqSetThemeMode(\"light\")' title='Light theme' aria-label='Light theme'>&#9728;</button>" \
	"<button class='tbtn' data-tbtn='dark' onclick='bpqSetThemeMode(\"dark\")' title='Dark theme' aria-label='Dark theme'>&#9790;</button>" \
	"</div>"

#define COMMON_THEME_SELECTOR_INIT_JAVASCRIPT \
	"function updateThemeBtns(t){var b=document.querySelectorAll('[data-tbtn]');for(var i=0;i<b.length;i++)b[i].classList.toggle('active',b[i].getAttribute('data-tbtn')===t);}" \
	"window.addEventListener('DOMContentLoaded',function(){updateThemeBtns(bpqGetThemeMode());});"

// APRS shared web styles.
// RAW variants are for direct string templates; FMT variants are for sprintf/snprintf templates.
#define COMMON_APRS_BASE_CSS_TEMPLATE(PCT) \
	"body{font-family:" COMMON_FONT_MONO ";background:var(--bg);margin:0;padding:clamp(15px,4vw,20px);color:var(--text);color-scheme:light dark;-webkit-font-smoothing:antialiased;}" \
	".aprs-page-shell{max-width:1100px;margin:0 auto;padding:0 10px;}" \
	".aprs-info-content{max-width:80" PCT ";margin:0 auto;}" \
	"h1{text-align:center;margin:0.5em 0;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.25rem,3vw,1.75rem);line-height:1.25;}" \
	"h2{text-align:center;margin:0.5em 0;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.1rem,2.5vw,1.4rem);line-height:1.25;}" \
	"h3{text-align:center;margin:0.5em 0;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1rem,2vw,1.2rem);line-height:1.25;}"

#define COMMON_APRS_CONTENT_CSS_TEMPLATE(PCT) \
	COMMON_APRS_BASE_CSS_TEMPLATE(PCT) \
	".table-wrap{width:100" PCT ";max-width:1100px;margin:0 auto 12px;overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".aprs-station-table{border-collapse:collapse;font-family:" COMMON_FONT_MONO ";}" \
	".aprs-station-table th,.aprs-station-table td{border:1px solid var(--border);padding:8px;white-space:nowrap;}" \
	".aprs-station-table th{background:var(--table-header);text-align:left;font-weight:600;}" \
	".aprs-station-table tbody tr:nth-child(even){background:var(--table-stripe);}" \
	".aprs-station-table tbody tr:hover{background:var(--surface-hover);}" \
	"@media(max-width:768px){" \
	".aprs-station-table-stack,.aprs-station-table-stack tbody,.aprs-station-table-stack tr,.aprs-station-table-stack td{display:block;width:100" PCT ";}" \
	".aprs-station-table-stack tr{margin:0 0 10px;border:1px solid var(--border);border-radius:8px;background:var(--surface);padding:6px;}" \
	".aprs-station-table-stack tbody tr:nth-child(even){background:var(--surface);}" \
	".aprs-station-table-stack td{border:none;border-bottom:1px solid var(--border-light);padding:6px 4px;text-align:left;}" \
	".aprs-station-table-stack td:last-child{border-bottom:none;}" \
	"}" \
	".aprs-weather-table{border-collapse:collapse;margin:0 auto 12px;}" \
	".aprs-weather-table th,.aprs-weather-table td{border:1px solid var(--border);padding:8px;white-space:nowrap;}" \
	".aprs-weather-table th{background:var(--table-header);font-weight:600;}" \
	".aprs-weather-table tbody tr:nth-child(even){background:var(--table-stripe);}" \
	".aprs-weather-table tbody tr:hover{background:var(--surface-hover);}" \
	".aprs-map{width:100" PCT ";max-width:600px;height:500px;border:0;display:block;margin:10px auto;}" \
	".aprs-note{text-align:center;font-style:italic;margin:8px 0;}" \
	".aprs-station-title{text-align:center;font-size:1.2rem;margin:12px 0;}" \
	".aprs-detail{text-align:center;margin:8px 0;}" \
	".aprs-lastposit{background:var(--surface-soft);border:1px solid var(--border);border-radius:4px;padding:8px 12px;max-width:800px;margin:10px auto;word-break:break-word;}" \
	COMMON_ITEM_GRID_CSS("aprs-station-grid", "14ch", PCT)

#define COMMON_APRS_MAP_CSS_TEMPLATE(PCT) \
	COMMON_APRS_CONTENT_CSS_TEMPLATE(PCT) \
	"html,body{height:100" PCT ";width:100" PCT ";}" \
	"body{display:flex;flex-direction:column;padding:0;min-height:100vh;}" \
	".menu-wrapper{background:var(--bg);padding:10px 10px 0;}" \
	".menu{max-width:1100px;margin:0 auto 10px;}" \
	".menu a{font-size:16px;}" \
	".aprs-controls{display:flex;flex-wrap:wrap;justify-content:center;gap:12px;max-width:1100px;margin:0 auto 10px;padding:10px 12px;" COMMON_CARD_CHROME_CSS_6PX "}" \
	".aprs-controls label{display:inline-flex;align-items:center;gap:6px;min-height:32px;font-size:15px;}" \
	".aprs-controls input[type=checkbox]{width:18px;height:18px;margin:0;}" \
	"#map{flex:1 1 auto;min-height:320px;width:100" PCT ";margin:0;}" \
	"@media(max-width:768px){.aprs-controls{justify-content:flex-start;}}" \
	".popup{border:1px solid var(--border);margin:0;padding:0;font-size:12px;min-height:16px;box-shadow:none;}" \
	".leaflet-tooltip-left.popup::before{border-left-color:transparent;}" \
	".leaflet-tooltip-right.popup::before{border-right-color:transparent;}"

#define COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE(PCT) \
	COMMON_APRS_BASE_CSS_TEMPLATE(PCT) \
	"h2{text-align:center;margin:10px 0 14px;font-size:clamp(1.1rem,2.5vw,1.4rem);}" \
	".menu{max-width:1100px;margin:0 auto 14px;}" \
	".menu-header{max-width:1100px;}" \
	".aprs-msg-wrap{max-width:1100px;margin:0 auto;}" \
	".aprs-msg-table-wrap{width:100" PCT ";overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".aprs-msg-table{width:max-content;min-width:100" PCT ";border-collapse:collapse;background:var(--surface);}" \
	".aprs-msg-table th,.aprs-msg-table td{border:1px solid var(--border);padding:8px;white-space:nowrap;}" \
	".aprs-msg-table th{background:var(--table-header);text-align:left;font-weight:600;}" \
	".aprs-msg-table tbody tr:nth-child(even){background:var(--table-stripe);}" \
	".aprs-msg-table tbody tr:hover{background:var(--surface-hover);}" \
	".aprs-msg-form{max-width:900px;margin:0 auto;background:var(--surface);border:1px solid var(--border-card);border-radius:8px;padding:14px;box-shadow:var(--shadow-card);}" \
	".aprs-msg-form table{width:100" PCT ";border-collapse:collapse;}" \
	".aprs-msg-form td{padding:8px;vertical-align:top;}" \
	".aprs-msg-form input[type=text]{width:100" PCT ";" COMMON_INPUT_BASE_CSS "border:1px solid var(--border-card);font-family:inherit;}" \
	".aprs-msg-form input[type=text]:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".aprs-msg-actions{text-align:center;margin-top:10px;}" \
	".aprs-msg-actions input[type=submit]{min-height:44px;padding:10px 16px;background:var(--surface);border:1px solid var(--border-card);border-radius:6px;cursor:pointer;margin:0 6px;color:var(--text);font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);font-family:" COMMON_FONT_TITLE ";}" \
	".aprs-msg-actions input[type=submit]:hover{background:var(--surface-hover);}" \
	".aprs-msg-actions input[type=submit]:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}" \
	".aprs-msg-actions input[type=submit]:active{background:var(--primary-dark);color:var(--on-primary);}"

#define COMMON_APRS_CONTENT_CSS COMMON_APRS_CONTENT_CSS_TEMPLATE("%")
#define COMMON_APRS_MAP_CSS COMMON_APRS_MAP_CSS_TEMPLATE("%")
#define COMMON_APRS_MESSAGE_PAGE_CSS COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE("%")
#define COMMON_APRS_CONTENT_CSS_FMT COMMON_APRS_CONTENT_CSS_TEMPLATE("%%")
#define COMMON_APRS_MAP_CSS_FMT COMMON_APRS_MAP_CSS_TEMPLATE("%%")
#define COMMON_APRS_MESSAGE_PAGE_CSS_FMT COMMON_APRS_MESSAGE_PAGE_CSS_TEMPLATE("%%")

// Port statistics page chrome. Namespaced under .portstats-* selectors; safe to include globally.
#define COMMON_APRS_PORTSTATS_CHROME_CSS \
	".portstats-wrap{max-width:1100px;margin:0 auto;}" \
	".portstats-chart{display:block;width:100%;max-width:900px;height:auto;border:1px solid #d3d3d3;background:var(--surface);margin:10px auto;}" \
	".portstats-note{text-align:center;margin:8px 0 14px;}"

// Common Responsive table styles
// Use classes: table-wrap, node-table, node-table-stack, num/text/center
// Core table styling shared across all table variants
#define COMMON_TABLE_FONT_SIZE "clamp(0.75rem,0.65rem + 1vw,0.9375rem)"
#define COMMON_TABLE_FONT_SIZE_COMPACT "clamp(0.6875rem,0.62rem + 0.45vw,0.8125rem)"

// Shared item-grid CSS for auto-column call/node grids.
// CLASSNAME: CSS class name string, MINWIDTH: minmax min e.g. "17ch", PCT: "%" or "%%"
#define COMMON_ITEM_GRID_CSS(CLASSNAME, MINWIDTH, PCT) \
	"." CLASSNAME "{display:grid;grid-template-columns:repeat(auto-fill,minmax(" MINWIDTH ",max-content));justify-content:center;gap:0;width:100" PCT ";padding:8px;box-sizing:border-box;font-family:" COMMON_FONT_MONO ";font-size:" COMMON_TABLE_FONT_SIZE_COMPACT ";}" \
	"." CLASSNAME " a{color:var(--link);white-space:nowrap;padding:3px 10px 3px 6px;display:block;border:1px solid var(--border);}" \
	"." CLASSNAME " a:hover{background:var(--overlay-hover);}"

// PCT parameter should be "%%" when used in sprintf format strings, "%" for static bpq.css content.
#define COMMON_TABLE_CSS_TEMPLATE(PCT) \
	".table-wrap{width:100" PCT ";max-width:1100px;margin:0 auto 12px;overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".node-table{width:max-content;max-width:none;border-collapse:collapse;table-layout:auto;margin:0 auto;background:var(--surface);font-family:" COMMON_FONT_MONO ";font-size:" COMMON_TABLE_FONT_SIZE ";}" \
	".node-table caption{caption-side:top;text-align:left;font-family:" COMMON_FONT_TITLE ";font-size:clamp(0.9375rem,0.9rem + 0.2vw,1rem);font-weight:600;color:var(--text);padding:0 0 8px;}" \
	".node-table th,.node-table td{border:1px solid var(--border);padding:8px;vertical-align:top;white-space:nowrap;}" \
	".node-table th{background:var(--table-header);text-align:left;}" \
	".node-table tbody tr:nth-child(even){background-color:var(--table-stripe);}" \
	".node-table tbody tr:hover{background:var(--surface-hover);}" \
	".node-table td.num{text-align:center;}" \
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
	".node-table-stack,.node-table-stack tbody,.node-table-stack tr,.node-table-stack td{display:block;width:100" PCT ";}" \
	".node-table-stack tr{margin:0 0 10px;border:1px solid var(--border);border-radius:8px;background:var(--surface);padding:6px;}" \
	".node-table-stack tbody tr:nth-child(even){background:var(--surface);}" \
	".node-table-stack td{border:none;border-bottom:1px solid var(--border-light);text-align:left;padding:6px 4px;}" \
	".node-table-stack td.num,.node-table-stack td.center,.node-table-stack td.text{text-align:left;}" \
	".node-table-stack td:last-child{border-bottom:none;}" \
	".node-table-stack td::before{content:attr(data-label);display:inline-block;min-width:130px;font-weight:700;color:var(--text);margin-right:8px;}" \
	".node-table-stack.stats-table td::before{content:none;display:none;}" \
	"}" \
	COMMON_ITEM_GRID_CSS("node-grid", "17ch", PCT)
// Backward-compat alias for sprintf format string contexts (uses %% for literal %)
#define COMMON_TABLE_CSS COMMON_TABLE_CSS_TEMPLATE("%%")

#define COMMON_COMPACT_PAGE_CSS \
	"body{font-family:" COMMON_FONT_MONO ";font-size:13px;margin:10px;background:var(--bg);color:var(--text);}" \
	"h3{text-align:center;margin:8px 0 12px;}"

#define COMMON_COMPACT_TABLE_CSS_TEMPLATE(PADDING) \
	"table{border-collapse:collapse;margin:10px 0;}" \
	"th,td{border:1px solid var(--border);padding:" PADDING ";text-align:left;}" \
	"th{background:var(--table-header);font-weight:bold;}"

// Compact (tight) and padded (comfortable) table variants
#define COMMON_COMPACT_TABLE_CSS        COMMON_COMPACT_TABLE_CSS_TEMPLATE("3px 7px")
#define COMMON_COMPACT_TABLE_PADDED_CSS COMMON_COMPACT_TABLE_CSS_TEMPLATE("5px 10px")

#define COMMON_BTN_PRIMARY_COMPACT_CSS \
	".btn{display:inline-flex;align-items:center;justify-content:center;min-height:44px;padding:8px 12px;background:var(--primary);color:var(--on-primary);border:none;border-radius:4px;cursor:pointer;font-size:12px;box-sizing:border-box;}"

#define COMMON_BTN_PRIMARY_COMPACT_INTERACTION_CSS \
	".btn:hover,.btn:focus{background:var(--primary-dark);outline:2px solid var(--focus-ring);outline-offset:2px;}"

// MARGIN: e.g. "0" or "4px"; PADDING: e.g. "10px" or "0"
#define COMMON_MONO_PAGE_CLAMP_CSS_TEMPLATE(MARGIN, PADDING) \
	"body{font-family:" COMMON_FONT_MONO ";font-size:clamp(1rem,0.96rem + 0.22vw,1.125rem);margin:" MARGIN ";padding:" PADDING ";background:var(--bg);color:var(--text);}"

// Backward-compat aliases
#define COMMON_MONO_PAGE_CLAMP_MARGIN0_PAD10_CSS COMMON_MONO_PAGE_CLAMP_CSS_TEMPLATE("0", "10px")
#define COMMON_MONO_PAGE_CLAMP_MARGIN4_CSS       COMMON_MONO_PAGE_CLAMP_CSS_TEMPLATE("4px", "0")

#define COMMON_MONO_COMPACT_TEXT_CSS \
	"font-family: " COMMON_FONT_MONO "; font-size: " COMMON_TABLE_FONT_SIZE ";"

// Minimal FOUC-prevention script for pages that also load bpq.js.
// Contains only the three functions needed before first paint plus the IIFE.
// Does NOT include bpqSetThemeMode — that is provided by bpq.js.
// Use COMMON_THEME_COOKIE_INIT_JAVASCRIPT for pages that do NOT load bpq.js
// (signon, terminal, simple admin pages).
#define COMMON_THEME_FOUC_JAVASCRIPT \
	"function bpqGetCookie(n){var p=n+'=';var c=document.cookie?document.cookie.split(';'):[];for(var i=0;i<c.length;i++){var x=c[i].trim();if(x.indexOf(p)===0)return decodeURIComponent(x.substring(p.length));}return '';}" \
	"function bpqGetThemeMode(){var t=bpqGetCookie('bpq-theme');if(t==='dark'||t==='light'||t==='system')return t;return 'system';}" \
	"function bpqApplyThemeMode(t){var h=document.documentElement;if(t==='dark')h.setAttribute('data-theme','dark');else if(t==='light')h.setAttribute('data-theme','light');else h.removeAttribute('data-theme');}" \
	"(function(){bpqApplyThemeMode(bpqGetThemeMode());})()"

// Responsive Menu JavaScript
// Handles toggle, click-outside-to-close, and escape key
// Expects menu element ID to be 'mainMenu' or 'mailMenu'
// Expects toggle button ID to be 'menuToggle'
#define COMMON_THEME_COOKIE_INIT_JAVASCRIPT \
	"function bpqGetCookie(n){var p=n+'=';var c=document.cookie?document.cookie.split(';'):[];for(var i=0;i<c.length;i++){var x=c[i].trim();if(x.indexOf(p)===0)return decodeURIComponent(x.substring(p.length));}return '';}"\
	"function bpqGetThemeMode(){var t=bpqGetCookie('bpq-theme');if(t==='dark'||t==='light'||t==='system')return t;return 'system';}"\
	"function bpqApplyThemeMode(t){var h=document.documentElement;if(t==='dark')h.setAttribute('data-theme','dark');else if(t==='light')h.setAttribute('data-theme','light');else h.removeAttribute('data-theme');}"\
	"function bpqSetThemeMode(t){var v=(t==='dark'||t==='light')?t:'system';document.cookie='bpq-theme='+encodeURIComponent(v)+'; Path=/; Max-Age=31536000; SameSite=Lax';bpqApplyThemeMode(v);if(typeof updateThemeBtns==='function')updateThemeBtns(v);}"\
	"(function(){bpqApplyThemeMode(bpqGetThemeMode());})();"

#define COMMON_MENU_JAVASCRIPT \
	COMMON_THEME_COOKIE_INIT_JAVASCRIPT \
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
	"});" \
	"function closeMenuOnMobile(){" \
	"var menu=getMenu();" \
	"var t=document.getElementById('menuToggle');" \
	"if(!menu||!t)return;" \
	"if(window.matchMedia('(max-width:768px)').matches){" \
	"menu.classList.remove('menu-open');" \
	"t.textContent='Menu';" \
	"t.setAttribute('aria-expanded','false');" \
	"}" \
	"}" \
	"window.addEventListener('DOMContentLoaded',function(){" \
	"var menu=getMenu();" \
	"if(!menu)return;" \
	"menu.addEventListener('click',function(event){" \
	"var target=event.target;" \
	"if(target&&target.tagName==='A')closeMenuOnMobile();" \
	"if(target&&target.tagName==='INPUT'&&target.type==='submit')closeMenuOnMobile();" \
	"});" \
	"});"

// Common Form Styles
// Provides responsive form-row pattern used throughout the application
// PCT parameter should be "%%" when used in sprintf format strings, "%" for static bpq.css content.
#define COMMON_FORM_CSS_TEMPLATE(PCT) \
	".form-section{" \
	"background:var(--surface);" \
	"padding:clamp(12px,4vw,20px);" \
	"border-radius:8px;" \
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
	"flex:1 1 clamp(100px,25" PCT ",150px);" \
	"font-weight:bold;" \
	"font-size:clamp(1rem,0.95rem + 0.2vw,1.0625rem);" \
	"padding-top:2px;" \
	"}" \
	".form-row input[type=text],.form-row input[type=number],.form-row input[type=password],.form-row select{" \
	"flex:2 1 200px;" \
	"padding:10px 12px;" \
	"line-height:1.5;" \
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
	"width:100" PCT ";" \
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
	"width:100" PCT ";" \
	"padding-top:0;" \
	"margin-bottom:4px;" \
	"}" \
	".form-row input[type=text],.form-row input[type=number],.form-row input[type=password],.form-row select{" \
	"width:100" PCT ";" \
	"min-height:48px;" \
	"}" \
	".checkbox-group{" \
	"flex-direction:column;" \
	"align-items:flex-start;" \
	"}" \
	"}"
// Backward-compat alias for sprintf format string contexts (uses %% for literal %)
#define COMMON_FORM_CSS COMMON_FORM_CSS_TEMPLATE("%%")

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
// Provides sticky form-action button bar for Node config pages.
// Named .sticky-buttons to avoid conflict with .buttons in COMMON_MAIL_DETAIL_CSS
// (which is a flex row, not a sticky footer).
// PCT parameter should be "%%" when used in sprintf format strings, "%" for static bpq.css content.
#define COMMON_BUTTON_CSS_TEMPLATE(PCT) \
	".sticky-buttons{" \
	"text-align:center;" \
	"margin:20px 0;" \
	"position:sticky;" \
	"bottom:0;" \
	"background:var(--surface);" \
	"padding:12px;" \
	"border-top:1px solid var(--border-light);" \
	"z-index:10;" \
	"}" \
	".sticky-buttons input,.sticky-buttons button{" \
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
	".sticky-buttons input:hover,.sticky-buttons button:hover{" \
	"background:var(--primary-dark);" \
	"}" \
	".sticky-buttons input:focus-visible,.sticky-buttons button:focus-visible{" \
	"outline:3px solid var(--focus-ring);outline-offset:2px;" \
	"}" \
	"@media(max-width:768px){" \
	".sticky-buttons{" \
	"padding:10px;" \
	"margin:0 -20px -20px;" \
	"border-radius:0;" \
	"}" \
	".sticky-buttons input,.sticky-buttons button{" \
	"width:calc(50" PCT "-6px);" \
	"}" \
	"}" \
	"@media(max-width:480px){" \
	".sticky-buttons input,.sticky-buttons button{" \
	"width:100" PCT ";" \
	"min-height:48px;" \
	"margin:4px 0;" \
	"}" \
	"}"
// Backward-compat alias for sprintf format string contexts (uses %% for literal %)
#define COMMON_BUTTON_CSS COMMON_BUTTON_CSS_TEMPLATE("%%")

// Common Signon/Login Page Styles
// Used by TermSignon, NodeSignon, MailSignon, ChatSignon in HTTPcode.c
#define COMMON_SIGNON_CSS \
	COMMON_CSS_VARIABLES \
	COMMON_AUTOFILL_FORMAT_OVERRIDE \
	"body{font-family:" COMMON_FONT_TITLE ";font-size:clamp(1rem,0.96rem + 0.22vw,1.125rem);margin:20px;background:var(--bg);color:var(--text);}" \
	"h2,h3{text-align:center;font-family:" COMMON_FONT_TITLE ";}" \
	"h2{font-size:clamp(1.5rem,4vw,2.25rem);}" \
	"h3{font-size:clamp(1.25rem,3vw,1.75rem);}" \
	".form-container{max-width:400px;margin:20px auto;padding:20px;" COMMON_CARD_CHROME_CSS_6PX "}" \
	".form-row{margin:15px 0;}" \
	"label{display:block;margin-bottom:5px;}" \
	"input[type=text],input[type=password]{width:100%%;font-size:clamp(1rem,2vw,1rem);font-family:" COMMON_FONT_TITLE ";background:var(--surface);}" \
	COMMON_INPUT_BASE_COMPLETE_CSS \
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

/*
 * EXTERNAL STATIC ASSET ROUTES
 *
 * /bpq/bpq.css  — universal CSS (colour tokens, reduced-motion, box-sizing).
 *                 Replaces inline COMMON_CSS_ROOT + COMMON_REDUCED_MOTION_CSS
 *                 on full-page templates.  Served with Cache-Control: max-age=86400.
 *
 * /bpq/bpq.js   — universal JavaScript (theme cookie, menu toggle, theme selector,
 *                 and closeMenuOnMobile / DOMContentLoaded handlers).
 *                 Replaces the inline COMMON_MENU_JAVASCRIPT block.
 *                 Served with Cache-Control: max-age=86400.
 *
 * Use COMMON_BPQ_CSS_LINK and COMMON_BPQ_JS_SCRIPT in <head> sections.
 * Keep COMMON_THEME_COOKIE_INIT_JAVASCRIPT inline for FOUC prevention on pages
 * that need it (signon, terminal, simple admin pages without the full menu).
 */
#define COMMON_BPQ_CSS_CONTENT \
	COMMON_CSS_ROOT \
	COMMON_REDUCED_MOTION_CSS \
	COMMON_LINK_CSS \
	COMMON_THEME_SELECTOR_CSS \
	COMMON_MENU_CSS_TEMPLATE("%") \
	COMMON_CHAT_STATUS_PAGE_CSS \
	COMMON_ADMIN_FIELD_THEME_CSS \
	COMMON_BTN_BASE_COMPLETE_CSS \
	COMMON_APRS_PORTSTATS_CHROME_CSS \
	COMMON_MAIL_STATUS_CSS \
	COMMON_WP_DETAIL_CSS \
	COMMON_MAIL_DETAIL_CSS \
	COMMON_FWD_DETAIL_CSS \
	COMMON_NODE_H2_H3_CSS \
	COMMON_TABLE_CSS_TEMPLATE("%") \
	COMMON_FORM_CSS_TEMPLATE("%") \
	COMMON_BUTTON_CSS_TEMPLATE("%") \
	COMMON_UTILITY_CSS

// Static CSS served at /bpq/node.css — compact admin/status page styles shared
// across Node compact pages (StreamStatus, RigControl). Includes compact-page body
// and table styles, plus primary-colour compact buttons. Does not include
// .node-h2/.node-h3 (those are in bpq.css) or large table/form CSS that uses %%.
// Only the padded variant is included — both CSS variants share the same
// unscoped selectors (table/th/td), so including both would have the second
// override the first; PADDED (5px 10px) is the intentional final value.
#define COMMON_NODE_CSS_CONTENT \
	COMMON_COMPACT_PAGE_CSS \
	COMMON_COMPACT_TABLE_PADDED_CSS \
	COMMON_BTN_PRIMARY_COMPACT_CSS \
	COMMON_BTN_PRIMARY_COMPACT_INTERACTION_CSS

#define COMMON_NODE_CSS_LINK \
	"<link rel='stylesheet' href='/bpq/node.css'>"

#define COMMON_BPQ_JS_CONTENT \
	COMMON_MENU_JAVASCRIPT \
	COMMON_THEME_SELECTOR_INIT_JAVASCRIPT

#define COMMON_BPQ_CSS_LINK \
	"<link rel='stylesheet' href='/bpq/bpq.css'>"

#define COMMON_BPQ_JS_SCRIPT \
	"<script src='/bpq/bpq.js'></script>"

/*
 * NODE MANAGEMENT DROPDOWN JAVASCRIPT
 *
 * Extracted from NodeMenuHeader so the block has a single named home and can be
 * referenced in the external-asset plan.  Only needed on Node pages that render
 * the Mgmt dropdown.
 */
#define COMMON_NODE_MGMT_JAVASCRIPT \
	"function getMgmtDom(){" \
	"return{" \
	"dropdown:document.getElementById('mgmtDropdown')," \
	"button:document.getElementById('mgmtButton')," \
	"sections:document.querySelectorAll('#mgmtDropdown .mgmt-section')," \
	"toggles:document.querySelectorAll('#mgmtDropdown .mgmt-toggle')" \
	"};" \
	"}" \
	"function resetMgmtSections(parts){" \
	"var i;" \
	"for(i=0;i<parts.sections.length;i++)parts.sections[i].classList.remove('show');" \
	"for(i=0;i<parts.toggles.length;i++)parts.toggles[i].textContent=parts.toggles[i].getAttribute('data-label')+' +';" \
	"}" \
	"function setMgmtOpen(parts,open){" \
	"if(!parts.dropdown)return;" \
	"if(open)parts.dropdown.classList.add('show');" \
	"else parts.dropdown.classList.remove('show');" \
	"}" \
	"window.addEventListener('click',function(event){" \
	"var parts=getMgmtDom();" \
	"if(parts.dropdown&&parts.button&&!parts.dropdown.contains(event.target)&&!parts.button.contains(event.target)){" \
	"setMgmtOpen(parts,false);" \
	"resetMgmtSections(parts);" \
	"}" \
	"});" \
	"function closeMgmtSections(){" \
	"resetMgmtSections(getMgmtDom());" \
	"}" \
	"function toggleMgmt(event){" \
	"if(event){event.preventDefault();event.stopPropagation();}" \
	"var parts=getMgmtDom();" \
	"if(!parts.dropdown)return;" \
	"if(parts.dropdown.classList.contains('show')){" \
	"setMgmtOpen(parts,false);" \
	"resetMgmtSections(parts);" \
	"}else{" \
	"setMgmtOpen(parts,true);" \
	"}" \
	"}" \
	"function toggleMgmtSection(event,sectionId,toggleId){" \
	"if(event){event.preventDefault();event.stopPropagation();}" \
	"var parts=getMgmtDom();" \
	"var section=document.getElementById(sectionId);" \
	"var toggle=document.getElementById(toggleId);" \
	"if(!section||!toggle)return;" \
	"var openNow=section.classList.contains('show');" \
	"resetMgmtSections(parts);" \
	"if(!openNow){" \
	"section.classList.add('show');" \
	"toggle.textContent=toggle.getAttribute('data-label')+' -';" \
	"}" \
	"}" \
	"function closeMenuOnMobile(){" \
	"var menu=document.getElementById('mainMenu');" \
	"var toggle=document.getElementById('menuToggle');" \
	"var parts=getMgmtDom();" \
	"if(!menu||!toggle)return;" \
	"if(window.matchMedia('(max-width:768px)').matches){" \
	"menu.classList.remove('menu-open');" \
	"toggle.textContent='Menu';" \
	"setMgmtOpen(parts,false);" \
	"resetMgmtSections(parts);" \
	"}" \
	"}" \
	"window.addEventListener('DOMContentLoaded',function(){" \
	"var menu=document.getElementById('mainMenu');" \
	"if(!menu)return;" \
	"menu.addEventListener('click',function(event){" \
	"var target=event.target;" \
	"if(!target)return;" \
	"if(target.tagName==='A')closeMenuOnMobile();" \
	"if(target.tagName==='INPUT'&&target.type==='submit')closeMenuOnMobile();" \
	"});" \
	"});"

// Override Autocomplete styles in Chrome
#define COMMON_AUTOFILL_FORMAT_OVERRIDE \
	"input:-webkit-autofill," \
	"input:-webkit-autofill:hover," \
	"input:-webkit-autofill:focus," \
	"textarea:-webkit-autofill," \
	"textarea:-webkit-autofill:hover," \
	"textarea:-webkit-autofill:focus," \
	"select:-webkit-autofill," \
	"select:-webkit-autofill:hover," \
	"select:-webkit-autofill:focus {" \
	"  -webkit-text-fill-color: var(--text);" \
	"  -webkit-box-shadow: var(--bg);" \
	"  -webkit-font-size: inherit;" \
	"  transition: background-color 5000s ease-in-out 0s;" \
	"}"

#endif // COMMON_WEB_COMPONENTS_H
