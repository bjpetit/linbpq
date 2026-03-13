/*
Copyright 2026 LinBPQ/BPQ32 Contributors

This file is part of LinBPQ/BPQ32.

Common reusable CSS and JavaScript components for web interface templates.
This consolidates responsive menu systems and base styles to reduce duplication.
*/

#ifndef COMMON_WEB_COMPONENTS_H
#define COMMON_WEB_COMPONENTS_H

// CSS Variables and Universal Styles
// Used by all pages for consistent theming
#define COMMON_CSS_ROOT \
	":root{" \
	"--bg:#f4f4f4;" \
	"--surface:#fff;" \
	"--primary:#007bff;" \
	"--primary-dark:#0056b3;" \
	"--border:#ccc;" \
	"--border-light:#e2e8f0;" \
	"--text:#1f2937;" \
	"--link:#1f2937;" \
	"}" \
	"*{box-sizing:border-box;}"

#define COMMON_BODY_BASE_CSS \
	"body{" \
	"font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",Arial,sans-serif;" \
	"background:var(--bg);" \
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
	"h3{text-align:center;margin:0 0 15px;font-size:clamp(1.25rem,1.05rem + 0.9vw,1.75rem);line-height:1.25;}"

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
// Provides collapsible mobile menu with hamburger toggle
// Max-width should be set per-application (980px for mail, 1100px for node)
#define COMMON_MENU_CSS \
	".menu-header{display:none;max-width:980px;margin:0 auto 10px;}" \
	".menu-toggle{" \
	"width:100%%;" \
	"min-height:44px;" \
	"box-sizing:border-box;" \
	"border:1px solid var(--border);" \
	"border-radius:6px;" \
	"background:var(--surface);" \
	"font-size:clamp(1rem,0.94rem + 0.25vw,1.125rem);" \
	"color:var(--text);" \
	"cursor:pointer;" \
	"touch-action:manipulation;" \
	"font-weight:500;" \
	"}" \
	".menu-toggle:active{background:#e9ecef;}" \
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
	"cursor:pointer;" \
	"touch-action:manipulation;" \
	"}" \
	".menu a:hover{background:#e9ecef;}" \
	".menu a:focus-visible{outline:2px solid var(--primary);outline-offset:2px;}" \
	".menu a:active{background:black;color:white;}" \
	"@media(max-width:768px){" \
	".menu-header{display:block;}" \
	".menu{display:none;flex-direction:column;align-items:stretch;gap:8px;}" \
	".menu.menu-open{display:flex;}" \
	".menu a{width:100%%;text-align:center;min-height:48px;}" \
	"}"

// Common responsive table styles
// Use classes: table-wrap, node-table, node-table-stack, num/text/center
#define COMMON_TABLE_FONT_SIZE "clamp(0.75rem,0.65rem + 1vw,0.9375rem)"
#define COMMON_TABLE_FONT_SIZE_COMPACT "clamp(0.6875rem,0.62rem + 0.45vw,0.8125rem)"

#define COMMON_TABLE_CSS \
	".table-container{width:100%%;max-width:1100px;margin:0 auto 12px;overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".table-wrap{width:100%%;max-width:1100px;margin:0 auto 12px;overflow-x:auto;-webkit-overflow-scrolling:touch;}" \
	".node-table{width:max-content;max-width:none;border-collapse:collapse;table-layout:auto;background:var(--surface);font-family:'JetBrains Mono',Consolas,Monaco,'Courier New',monospace;font-size:" COMMON_TABLE_FONT_SIZE ";}" \
	".node-table caption{caption-side:top;text-align:left;font-family:Arial,sans-serif;font-size:clamp(0.9375rem,0.9rem + 0.2vw,1rem);font-weight:600;color:var(--text);padding:0 0 8px;}" \
	".node-table th,.node-table td{border:1px solid var(--border);padding:8px;vertical-align:top;}" \
	".node-table th,.node-table td{white-space:nowrap;}" \
	".node-table th{background:#f0f0f0;text-align:left;}" \
	".node-table tbody tr:nth-child(even){background-color:#f2f2f2;}" \
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
	".node-table-stack td{border:none;border-bottom:1px solid #eee;text-align:left;padding:6px 4px;}" \
	".node-table-stack td.num,.node-table-stack td.center,.node-table-stack td.text{text-align:left;}" \
	".node-table-stack td:last-child{border-bottom:none;}" \
	".node-table-stack td::before{content:attr(data-label);display:inline-block;min-width:130px;font-weight:700;color:var(--text);margin-right:8px;}" \
	".node-table-stack.stats-table td::before{content:none;display:none;}" \
	"}" \
	".node-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(17ch,max-content));justify-content:center;gap:0;width:100%%;padding:8px;box-sizing:border-box;font-family:'JetBrains Mono',Consolas,Monaco,'Courier New',monospace;font-size:" COMMON_TABLE_FONT_SIZE_COMPACT ";}" \
	".node-grid a{color:var(--link);white-space:nowrap;padding:3px 10px 3px 6px;display:block;border:1px solid var(--border);}" \
	".node-grid a:hover{background:rgba(0,0,0,0.05);}"

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
	"}else{" \
	"menu.classList.add('menu-open');" \
	"toggle.textContent='Close';" \
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
	"menu.classList.remove('menu-open');" \
	"document.getElementById('menuToggle').textContent='Menu';" \
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
	"box-shadow:0 0 5px rgba(0,0,0,0.1);" \
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
	"font-family: 'JetBrains Mono', 'Fira Code', 'Source Code Pro', Consolas, Monaco, 'Courier New', monospace;" \
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
	"min-height:44px;" \
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
	".muted-note{margin:10px 0 0 0;color:#666;font-size:clamp(0.875rem,0.84rem + 0.15vw,1rem);line-height:1.45;}" \
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
	"color:white;" \
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
	"margin:4px 0;" \
	"}" \
	"}"
// Helper: Build mail menu HTML
// Usage: sprintf(buffer, COMMON_MAIL_MENU, key, key, key, ..., key);
#define COMMON_MAIL_MENU \
	"<div class=\"menu-header\"><button id=\"menuToggle\" class=\"menu-toggle\" type=\"button\" onclick=\"toggleMenu(event)\">Menu</button></div>" \
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
	"<div class=\"menu-header\"><button id=\"menuToggle\" class=\"menu-toggle\" type=\"button\" onclick=\"toggleMenu(event)\">Menu</button></div>" \
	"<div id=\"chatMenu\" class=\"menu\">" \
	"<a href=\"/Chat/ChatStatus?%s\">Status</a>" \
	"<a href=\"/Chat/ChatConf?%s\">Configuration</a>" \
	"<a href=\"/\">Node Menu</a>" \
	"</div>"

#endif // COMMON_WEB_COMPONENTS_H
