/*
Copyright 2001-2022 John Wiseman G8BPQ

This file is part of LinBPQ/BPQ32.

LinBPQ/BPQ32 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LinBPQ/BPQ32 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LinBPQ/BPQ32.  If not, see http://www.gnu.org/licenses
*/	


//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE

#define DllImport

#include "cheaders.h"
#include <stdlib.h>
#ifndef WIN32
#include <limits.h>
#endif

#include "tncinfo.h"
#include "time.h"
#include "bpq32.h"
#include "telnetserver.h"
#include "common_web_components.h"

// This is needed to link with a lib built from source

#ifdef WIN32
#define ZEXPORT __stdcall
#endif

#include <zlib.h>

#define CKernel
#include "httpconnectioninfo.h"

extern int MAXBUFFS, QCOUNT, MINBUFFCOUNT, NOBUFFCOUNT, BUFFERWAITS, L3FRAMES;
extern int NUMBEROFNODES, MAXDESTS, L4CONNECTSOUT, L4CONNECTSIN, L4FRAMESTX, L4FRAMESRX, L4FRAMESRETRIED, OLDFRAMES;
extern int STATSTIME;
extern  TRANSPORTENTRY * L4TABLE;
extern BPQVECSTRUC BPQHOSTVECTOR[];
extern BOOL APRSApplConnected;  
extern char VersionString[];
VOID FormatTime3(char * Time, time_t cTime);
DllExport int APIENTRY Get_APPLMASK(int Stream);
VOID SaveUIConfig();
int ProcessNodeSignon(SOCKET sock, struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, int LOCAL);
VOID SetupUI(int Port);
VOID SendUIBeacon(int Port);
VOID GetParam(char * input, char * key, char * value);
VOID ARDOPAbort(struct TNCINFO * TNC);
VOID WriteMiniDump();
BOOL KillTNC(struct TNCINFO * TNC);
BOOL RestartTNC(struct TNCINFO * TNC);
int GetAISPageInfo(char * Buffer, int ais, int adsb);
int GetAPRSPageInfo(char * Buffer, double N, double S, double W, double E, int aprs, int ais, int adsb);
unsigned char * Compressit(unsigned char * In, int Len, int * OutLen);
char * stristr (char *ch1, char *ch2);
int GetAPRSIcon(unsigned char * _REPLYBUFFER, char * NodeURL);
char * GetStandardPage(char * FN, int * Len);
BOOL SHA1PasswordHash(char * String, char * Hash);
char * byte_base64_encode(char *str, int len);
int APIProcessHTTPMessage(char * response, char * Method, char * URL, char * request,	BOOL LOCAL, BOOL COOKIE);
int RHPProcessHTTPMessage(struct ConnectionInfo * conn, char * response, char * Method, char * URL, char * request, BOOL LOCAL, BOOL COOKIE);
int doinflate(unsigned char * source, unsigned char * dest, int Len, int destlen, int * outLen);

extern struct ROUTE * NEIGHBOURS;
extern int  ROUTE_LEN;
extern int  MAXNEIGHBOURS;

extern struct DEST_LIST * DESTS;				// NODE LIST
extern int  DEST_LIST_LEN;
extern int  MAXDESTS;			// MAX NODES IN SYSTEM

extern struct _LINKTABLE * LINKS;
extern int	LINK_TABLE_LEN; 
extern int	MAXLINKS;
extern char * RigWebPage;
extern COLORREF Colours[256];

extern BOOL IncludesMail;
extern BOOL IncludesChat;

extern BOOL APRSWeb;  
extern BOOL RigActive;

extern HKEY REGTREE;

extern BOOL APRSActive;

extern UCHAR LogDirectory[];

extern struct RIGPORTINFO * PORTInfo[34];
extern int NumberofPorts;

extern UCHAR ConfigDirectory[260];

extern struct AXIPPORTINFO * Portlist[];

VOID sendandcheck(SOCKET sock, const char * Buffer, int Len);
int CompareNode(const void *a, const void *b);
int CompareAlias(const void *a, const void *b);
int CompareRoutes(const void * a, const void * b);

void ProcessMailHTTPMessage(struct HTTPConnectionInfo * Session, char * Method, char * URL, char * input, char * Reply, int * RLen, int InputLen, char * Token);
void ProcessChatHTTPMessage(struct HTTPConnectionInfo * Session, char * Method, char * URL, char * input, char * Reply, int * RLen);
struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
int SetupNodeMenu(char * Buff, size_t BuffSize, int SYSOP);
int StatusProc(char * Buff);
int ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, BOOL WebMail, int LOCAL);
int ProcessMailAPISignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, BOOL WebMail, int LOCAL);
int ProcessChatSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, int LOCAL);
VOID APRSProcessHTTPMessage(SOCKET sock, char * MsgPtr, BOOL LOCAL, BOOL COOKIE);


static struct HTTPConnectionInfo * SessionList;	// active term mode sessions

char Mycall[10];

char MAILPipeFileName[] = "\\\\.\\pipe\\BPQMAILWebPipe";
char CHATPipeFileName[] = "\\\\.\\pipe\\BPQCHATWebPipe";

//char APRSBit[] = "<td><a href=../aprs>APRS Pages</a></td>";

//char MailBit[] = "<td><a href=../Mail/Header>Mail Mgmt</a></td>"
//				 "<td><a href=/Webmail>WebMail</a></td>";
//char ChatBit[] = "<td><a href=../Chat/Header>Chat Mgmt</a></td>";

char Tail[] = "</body></html>";

#define HTTP_NODE_TABLE_HEADER_ROW "<tr>"
#define HTTP_NODE_SORT_CONTROLS "<div class='text-center my-20'><form method='get' action='/Node/Nodes.html' class='flex-center-wrap-gap-10'>" \
	"<input type='submit' class='btn' name='a' value='Nodes Sorted by Alias'>" \
	"<input type='submit' class='btn' name='c' value='Nodes Sorted by Call'>" \
	"<input type='submit' class='btn' name='t' value='Nodes With Traffic'></form></div>"

#define HTTP_NODE_TABLE_OPEN_STACK_CLASS(CLASSLIST) "<div class='table-wrap'><table class='node-table node-table-stack " CLASSLIST "'><thead>"
#define HTTP_NODE_TABLE_OPEN_STACK HTTP_NODE_TABLE_OPEN_STACK_CLASS("")
#define HTTP_NODE_TABLE_OPEN_STACK_COMPACT HTTP_NODE_TABLE_OPEN_STACK_CLASS("compact-table")
#define HTTP_NODE_TABLE_OPEN_STACK_ROUTES HTTP_NODE_TABLE_OPEN_STACK_CLASS("routes-table")
#define HTTP_NODE_TABLE_OPEN_STACK_STATS HTTP_NODE_TABLE_OPEN_STACK_CLASS("compact-table stats-table")
#define HTTP_NODE_TABLE_MID_STACK "</thead><tbody>"
#define HTTP_NODE_TABLE_HEADER_WITH_COLS(TABLE_CLASS, COLS) HTTP_NODE_TABLE_OPEN_STACK_CLASS(TABLE_CLASS) HTTP_NODE_TABLE_HEADER_ROW COLS "</tr>" HTTP_NODE_TABLE_MID_STACK
#define HTTP_NODE_SECTION_TABLE_HEADER(TITLE, TABLE_CLASS, COLS) HTTP_NODE_H2(TITLE) HTTP_NODE_TABLE_HEADER_WITH_COLS(TABLE_CLASS, COLS)
#define HTTP_NODE_SECTION_TABLE_OPEN(TITLE, TABLE_CLASS) HTTP_NODE_H2(TITLE) HTTP_NODE_TABLE_OPEN_STACK_CLASS(TABLE_CLASS) HTTP_NODE_TABLE_MID_STACK
#define HTTP_NODE_H2(TEXT) "<h2 class='node-h2'>" TEXT "</h2>"
#define HTTP_NODE_H3(TEXT) "<h3 class='node-h3'>" TEXT "</h3>"

#define HTTP_NODE_COLS_ROUTES "<th scope=\"col\">Port</th><th scope=\"col\">Call</th><th scope=\"col\">Quality</th><th scope=\"col\">Node Count</th><th scope=\"col\">Frame Count</th><th scope=\"col\">Retries</th><th scope=\"col\">Percent</th><th scope=\"col\">Maxframe</th><th scope=\"col\">Frack</th><th scope=\"col\">Last Heard</th><th scope=\"col\">Queued</th><th scope=\"col\">Rem Qual</th><th scope=\"col\">SRTT</th><th scope=\"col\">Rem SRTT</th>"
#define HTTP_NODE_COLS_TRAFFIC "<th scope=\"col\">Call</th><th scope=\"col\">Frames</th><th scope=\"col\">RTT</th><th scope=\"col\">BPQ?</th><th scope=\"col\">Hops</th>"
#define HTTP_NODE_COLS_LINKS "<th scope=\"col\">Far Call</th><th scope=\"col\">Our Call</th><th scope=\"col\">Port</th><th scope=\"col\">ax.25 state</th><th scope=\"col\">Link Type</th><th scope=\"col\">ax.25 Version</th>"
#define HTTP_NODE_COLS_USERS "<th scope=\"col\">Circuit</th><th scope=\"col\">Link</th><th scope=\"col\">Circuit</th>"
#define HTTP_NODE_COLS_PORTS "<th scope=\"col\">Port</th><th scope=\"col\">Driver</th><th scope=\"col\">ID</th><th scope=\"col\">Beacons</th><th scope=\"col\">Driver Window</th><th scope=\"col\">Stats Graph</th>"

#define HTTP_NODE_MENU_CSS \
	":root{--menu-max-width:1100px;}" \
	"body { font-family: " COMMON_FONT_MONO "; font-size: clamp(1rem,0.96rem + 0.22vw,1.125rem); line-height: 1.5; margin: 0; padding: 12px; background: var(--bg); color: var(--text); }" \
	"h1 { text-align: center; font-family: " COMMON_FONT_TITLE "; margin: 10px 0 18px; font-size: clamp(1.25rem,3vw,1.75rem); line-height: 1.25; }" \
	".menu { margin: 20px auto; }" \
	".menu > a, .menu > .dropdown { min-width: 10ch; }" \
	".dropdown { position: relative; display: inline-flex; }" \
	".menu > .dropdown > .btn { width: 100%%; }" \
	".dropdown-content { display: none; position: absolute; left: 50%%; transform: translateX(-50%%); background-color: var(--surface); min-width: 220px; border: 1px solid var(--border); border-radius: 6px; padding: 8px; z-index: 10; box-shadow: var(--shadow-overlay); font-family: " COMMON_FONT_TITLE "; }" \
	".dropdown-content a, .dropdown-content .btn { display: inline-flex; align-items: center; justify-content: center; min-height: 44px; width: 100%%; margin-top: 6px; padding: 10px 16px; background: var(--surface); text-decoration: none; border-radius: 6px; border: 1px solid var(--border); color: var(--link); box-sizing: border-box; font-size: clamp(1rem,0.94rem + 0.25vw,1.125rem); font-family: " COMMON_FONT_TITLE "; line-height: 1.2; }" \
	".dropdown-content a:hover, .dropdown-content .btn:hover { background: var(--surface-hover); }" \
	".dropdown-content a:focus-visible, .dropdown-content .btn:focus-visible { outline: 3px solid var(--focus-ring); outline-offset: 2px; }" \
	".dropdown-content form { margin: 0; }" \
	".dropdown-content label { display: block; margin-bottom: 8px; font-size: clamp(1rem,0.95rem + 0.2vw,1.0625rem); font-family: " COMMON_FONT_TITLE "; }" \
	".dropdown-content input[type='date'] { width: 100%%; box-sizing: border-box; margin-top: 4px; font-size: clamp(1rem,0.95rem + 0.2vw,1.0625rem); min-height: 44px; font-family: " COMMON_FONT_TITLE "; }" \
	".dropdown-content input[type='submit'] { width: 100%%; margin-top: 6px; font-family: " COMMON_FONT_TITLE "; }" \
	".mgmt-section { display: none; margin-top: 6px; border-top: 1px solid var(--border-light); padding-top: 6px; }" \
	".mgmt-section.show { display: block; }" \
	".show { display: block; }" \
	"input.btn:active { background: var(--primary-dark); color: var(--on-primary); }" \
	COMMON_NODE_MENU_MOBILE_CSS

char RouteHddr[] = HTTP_NODE_SECTION_TABLE_HEADER("Routes", "routes-table", HTTP_NODE_COLS_ROUTES);

char RouteLine[] = "<tr><td data-label='Port' class='text'>%s%d</td><td data-label='Call' class='text'>%s%s</td><td data-label='Quality' class='num'>%d</td><td data-label='Node Count' class='num'>%d</td><td data-label='Frame Count' class='num'>%d</td><td data-label='Retries' class='num'>%d</td><td data-label='Percent' class='num'>%d%%</td><td data-label='Maxframe' class='num'>%d</td><td data-label='Frack' class='num'>%d</td>"
"<td data-label='Last Heard' class='text'>%02d:%02d</td><td data-label='Queued' class='num'>%d</td><td data-label='Rem Qual' class='num'>%d</td><td data-label='SRTT' class='text'>-</td><td data-label='Rem SRTT' class='text'>-</td></tr>";

char RouteLineINP3[] = "<tr><td data-label='Port' class='text'>%s%d</td><td data-label='Call' class='text'>%s%s</td><td data-label='Quality' class='num'>%d</td><td data-label='Node Count' class='num'>%d</td><td data-label='Frame Count' class='num'>%d</td><td data-label='Retries' class='num'>%d</td><td data-label='Percent' class='num'>%d%%</td><td data-label='Maxframe' class='num'>%d</td><td data-label='Frack' class='num'>%d</td>"
"<td data-label='Last Heard' class='text'>%02d:%02d</td><td data-label='Queued' class='num'>%d</td><td data-label='Rem Qual' class='num'>%d</td><td data-label='SRTT' class='num'>%4.2fs</td><td data-label='Rem SRTT' class='num'>%4.2fs</td></tr>";

char NodeHddr[] = HTTP_NODE_SORT_CONTROLS
HTTP_NODE_H2("Nodes %s");

char NodeHeaderTraffic[] = HTTP_NODE_TABLE_HEADER_WITH_COLS("compact-table", HTTP_NODE_COLS_TRAFFIC);

char NodeLine[] = "<a href='NodeDetail?%s'>%s:%s</a>";
char NodeTrafficLine[] = "<tr><td data-label='Call' class='text'><a href=NodeDetail?%s>%s:%s</a></td><td data-label='Frames' class='num'>%d</td><td data-label='RTT' class='num'>%d</td><td data-label='BPQ?' class='center'>%c</td><td data-label='Hops' class='num'>%.0d</td></tr>";


char StatsHddr[] = HTTP_NODE_SECTION_TABLE_OPEN("Node Stats", "compact-table stats-table");

char StatsLine[] = "<tr><td data-label='Statistic' class='text'>%s</td><td data-label='Value' class='text'>%s</td></tr>";

char PortStatsHddr[] = HTTP_NODE_SECTION_TABLE_OPEN("Stats for Port %d", "compact-table stats-table");

char PortStatsLine[] = "<tr><td data-label='Statistic' class='text'>%s</td><td data-label='Value' class='num'>%d</td></tr>";


char Beacons[] =
	"<div class='form-section'>"
	HTTP_NODE_H2("Beacon Configuration for Port %d")
	HTTP_NODE_H3("You need to be signed in to save changes")
	"<form method=post action=BeaconAction>"
	"<div class='form-row'><label for=Every>Send Interval (Minutes)</label><input type=text id=Every name=Every tabindex=1 value=%d></div>"
	"<div class='form-row'><label for=Dest>To</label><input type=text id=Dest name=Dest class='text-uppercase' tabindex=2 value=%s></div>"
	"<div class='form-row'><label for=Path>Path</label><input type=text id=Path name=Path class='text-uppercase' maxlength=50 value=%s></div>"
	"<div class='form-row'><label for=File>Send From File</label><input type=text id=File name=File maxlength=50 value=%s></div>"
	"<div class='form-row'><label for=Text>Text</label><textarea id=Text name=Text>%s</textarea></div>"
	"<input type=hidden name=Port value=%d>"
	"<div class='sticky-buttons'><input type=submit value=Save><input type=submit value=Test name=Test></div>"
	"</form>"
	"</div>";


char LinkHddr[] = HTTP_NODE_SECTION_TABLE_HEADER("Links", "compact-table", HTTP_NODE_COLS_LINKS);

char LinkLine[] = "<tr><td data-label='Far Call' class='text'>%s</td><td data-label='Our Call' class='text'>%s</td><td data-label='Port' class='num'>%d</td><td data-label='ax.25 state' class='text'>%s</td><td data-label='Link Type' class='text'>%s</td><td data-label='ax.25 Version' class='num'>%d</td></tr>";

char UserHddr[] = HTTP_NODE_SECTION_TABLE_HEADER("Sessions", "compact-table", HTTP_NODE_COLS_USERS);

char UserLine[] = "<tr><td data-label='Circuit' class='text'>%s</td><td data-label='Link' class='center'>%s</td><td data-label='Circuit' class='text'>%s</td></tr>";

#define HTTP_SIGNON_HEAD(TITLE) COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING "<title>" TITLE "</title><style>" COMMON_SIGNON_CSS "</style><script>" COMMON_THEME_COOKIE_INIT_JAVASCRIPT "</script></head><body>"
#define HTTP_SIGNON_FORM_OPEN(ACTION) "<div class=\"form-container\"><form method=post action=" ACTION ">"
#define HTTP_SIGNON_USER_ROW "<div class=\"form-row\"><label>User</label><input type=text name=user tabindex=1 size=20 maxlength=50 /></div>"
#define HTTP_SIGNON_PASS_ROW "<div class=\"form-row\"><label>Password</label><input type=password name=password tabindex=2 size=20 maxlength=50 /></div>"
#define HTTP_SIGNON_SUBMIT_CANCEL_ROW "<div class=\"form-row\"><input type=submit class='btn' value=Submit><input type=submit class='btn' value=Cancel name=Cancel formnovalidate formmethod=get formaction=/Node/NodeIndex.html /></div></form></div>"

char TermSignon[] = HTTP_SIGNON_HEAD("BPQ32 Node %s Terminal Access")
"<h2>BPQ32 Node %s Terminal Access</h2>"
"<h3>Please enter username and password to access the node</h3>"
HTTP_SIGNON_FORM_OPEN("TermSignon")
HTTP_SIGNON_USER_ROW
HTTP_SIGNON_PASS_ROW
"<div class=\"form-row\"><input type=submit class='btn' value=Submit><input type=submit class='btn' value=Cancel name=Cancel formnovalidate formmethod=get formaction=/Node/NodeIndex.html />"
"<input type=hidden name=Appl value=\"%s\"  id=Pass></form></div></div>";


char PassError[] = "<div class='alert-error'>Sorry, User or Password is invalid - please try again</div>";

char BusyError[] = "<div class='alert-warn'>Sorry, No sessions available - please try later</div>";

char LostSession[] = COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING_STYLE_OPEN COMMON_SIGNON_CSS "</style><script>" COMMON_THEME_COOKIE_INIT_JAVASCRIPT "</script></head><body class='msg-page'><h1>BPQ32 Node Terminal</h1><h2>Sorry, Session had been lost - refresh page to sign in again</h2></body></html>";
char NoSessions[] = COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING_STYLE_OPEN COMMON_SIGNON_CSS "</style><script>" COMMON_THEME_COOKIE_INIT_JAVASCRIPT "</script></head><body class='msg-page'><h1>BPQ32 Node Terminal</h1><h2>Sorry, No Sessions available - refresh page to try again</h2></body></html>";

char TermPage[] = COMMON_HTML_HEAD_COMMON COMMON_HTML_META_VIEWPORT "<meta http-equiv=Content-Type content='text/html; charset=UTF-8' />"
"<title>BPQ32 Node %s</title><style>" COMMON_CSS_VARIABLES COMMON_NODE_TERM_IO_COLORS_CSS COMMON_THEME_SELECTOR_CSS "body { margin: 0; padding: 10px; font-family: " COMMON_FONT_MONO "; font-size: clamp(1rem,0.96rem + 0.22vw,1.125rem); background: var(--bg); color: var(--text); } h3 { text-align: center; font-family: " COMMON_FONT_TITLE "; margin: 10px 0; font-size: clamp(1.25rem,3vw,1.75rem); } .term-container { display: flex; flex-direction: column; height: calc(100vh - 180px); gap: 10px; } .term-actions { text-align: center; margin: 10px 0; }" COMMON_BTN_PANEL_BASE_CSS COMMON_BTN_HOVER_NEUTRAL_CSS COMMON_BTN_ACTIVE_DARK_CSS "#output-frame { flex: 1; border: 2px solid var(--border-card); background: var(--term-io-bg); min-height: 200px; } #input-frame { height: 50px; border: 2px solid var(--border-card); background: var(--term-io-bg); flex-shrink: 0; }" COMMON_NODE_TERM_MOBILE_CSS "</style>"
"<script>" COMMON_THEME_COOKIE_INIT_JAVASCRIPT COMMON_THEME_SELECTOR_INIT_JAVASCRIPT "</script>"
"</head><body>"
COMMON_THEME_SELECTOR_HTML
"<h3>BPQ32 Node %s</h3>"
"<form method=post action=/Node/TermClose?%s class='term-actions'>"
"<input type=submit class='btn' value='Close and return to Node Page' /></form>"
"<div class=\"term-container\">"
"<iframe id=output-frame frameborder=0 marginwidth=0 marginheight=0 src=OutputScreen.html?%s></iframe>"
"<iframe id=input-frame frameborder=0 marginwidth=0 marginheight=0 src=InputLine.html?%s></iframe>"
"</div></body>";

char TermOutput[] = COMMON_HTML_HEAD_OPEN_DOCTYPE
COMMON_HTML_META_UTF8
COMMON_FONT_INTER_LINK
"<meta http-equiv=cache-control content=no-cache>"
"<meta http-equiv=pragma content=no-cache>"
"<meta http-equiv=expires content=0>" 
"<meta http-equiv=refresh content=2>"
"<script>" COMMON_THEME_COOKIE_INIT_JAVASCRIPT "</script>"
"<style>" COMMON_CSS_VARIABLES COMMON_NODE_TERM_IO_COLORS_CSS "body { margin: 0; padding: 8px; background: var(--term-io-bg); font-family: " COMMON_FONT_MONO "; font-size: clamp(0.75rem,0.65rem + 1vw,0.9375rem); color: var(--term-io-text); } #Text div { white-space: nowrap; %s }</style>"
"<script type=\"text/javascript\">\r\n"
"function ScrollOutput()\r\n"
"{window.scrollBy(0,document.body.scrollHeight)}</script>"
"</head><body id=Text>"
"<div>";


// font-family: ui-monospace, 'Cascadia Code', 'Segoe UI Mono', 'SF Mono', 'Roboto Mono', 'Courier New', monospace;background-color:black;color:lawngreen;font-size:12px

char TermOutputTail[] = "</div><script type=\"text/javascript\">\r\nsetTimeout(ScrollOutput, 1)</script></body></html>";

/*
char InputLine[] = "<html><head></head><body onload='resize()' onresize='resize()'>"
"<form name=inputform method=post action=/TermInput?%s>"
"<script>document.inputform.input.focus();"
"function resize(){"
"var w=window,d=document,e=d.documentElement,g=d.getElementsByTagName('body')[0];"
"x=w.innerWidth;y=w.innerHeight;"
"var inp=document.getElementById('inp');"
"inp.style.width =  x + 'px';}</script>"
"<input id=inp type=text width=100%% name=input /></form>";
*/
char InputLine[] = COMMON_HTML_HEAD_COMMON COMMON_HTML_META_UTF8 "<style>" COMMON_CSS_VARIABLES
"* { margin: 0; padding: 0; box-sizing: border-box; } "
COMMON_NODE_TERM_IO_COLORS_CSS
"body { background: var(--term-io-bg); color: var(--term-io-text); font-family: " COMMON_FONT_MONO "; padding: 5px; height: 100%%; display: flex; align-items: center; } "
"form { width: 100%%; margin: 0; } "
"#inp { width: 100%%; height: 40px; padding: 8px; border: 1px solid var(--border-card); border-radius: 6px; box-sizing: border-box; font-family: " COMMON_FONT_MONO "; font-size: clamp(0.75rem,0.65rem + 1vw,0.9375rem); background: var(--term-io-bg); color: var(--term-io-text); overflow: hidden; white-space: nowrap; }"
"</style></head><body>"
"<form name=inputform method=post action=/TermInput?%s>"
"<input id=inp type=text name=input autocomplete=off style=\"%s\" />"
"<script>document.inputform.input.focus();</script></form></body></html>";

static char NodeSignon[] = HTTP_SIGNON_HEAD("BPQ32 Node SYSOP Access")
"<h2>BPQ32 Node %s SYSOP Access</h2>"
"<h3>This page sets Cookies. Don't continue if you object to this</h3>"
"<h3>Please enter Callsign and Password to access the Node</h3>"
HTTP_SIGNON_FORM_OPEN("/Node/Signon?Node")
HTTP_SIGNON_USER_ROW
HTTP_SIGNON_PASS_ROW
HTTP_SIGNON_SUBMIT_CANCEL_ROW;


static char MailSignon[] = HTTP_SIGNON_HEAD("BPQ32 Mail Server Access")
"<h2>BPQ32 Mail Server %s Access</h2>"
"<h3>Please enter Callsign and Password to access the BBS</h3>"
HTTP_SIGNON_FORM_OPEN("/Mail/Signon?Mail")
HTTP_SIGNON_USER_ROW
HTTP_SIGNON_PASS_ROW
HTTP_SIGNON_SUBMIT_CANCEL_ROW;

static char ChatSignon[] = HTTP_SIGNON_HEAD("BPQ32 Chat Server Access")
"<h2>BPQ32 Chat Server %s Access</h2>"
"<h3>Please enter Callsign and Password to access the Chat Server</h3>"
HTTP_SIGNON_FORM_OPEN("/Chat/Signon?Chat")
HTTP_SIGNON_USER_ROW
HTTP_SIGNON_PASS_ROW
HTTP_SIGNON_SUBMIT_CANCEL_ROW;


static char MailLostSession[] = COMMON_HTML_HEAD_VIEWPORT_SELF_CLOSING_STYLE_OPEN COMMON_SIGNON_CSS "</style><script>" COMMON_THEME_COOKIE_INIT_JAVASCRIPT "</script></head><body>"
"<div class=\"form-container\" style=\"margin-top:50px;text-align:center\"><h1>BPQ32 Mail Server</h1><h2>Sorry, Session had been lost</h2>"
"<form method=post action=/Mail/Lost?%s>"
"<input name=Submit value=Restart type=submit class='btn'> <input type=submit class='btn' value=Exit name=Cancel formnovalidate formmethod=get formaction=/Node/NodeIndex.html></form></div>";


static char ConfigEditPage[] = COMMON_HTML_HEAD_UTF8_VIEWPORT
"<title>Edit Config</title>" COMMON_BPQ_CSS_LINK "<style>" COMMON_MONO_PAGE_CLAMP_MARGIN0_PAD10_CSS " form { text-align: center; } textarea { width: min(100%%, 1100px); min-height: 60vh; box-sizing: border-box; " COMMON_MONO_COMPACT_TEXT_CSS " white-space: pre; }" COMMON_BTN_PANEL_BASE_CSS ".btn{margin:10px 6px 0;}</style></head><body>"
"<form method=post action=CFGSave?%s>"
"<textarea cols=100 rows=25 name=Msg>%s</textarea><br><br>"
"<input name=Save value=Save type=submit class='btn'><input name=Cancel value=Cancel type=submit class='btn'><br></form>";

static char EXCEPTMSG[80] = "";


void UndoTransparency(char * input)
{
	char * ptr1, * ptr2;
	char c;
	int hex;

	if (input == NULL)
		return;

	ptr1 = ptr2 = input;

	// Convert any %xx constructs

	while (1)
	{
		c = *(ptr1++);

		if (c == 0)
			break;

		if (c == '%')
		{
			c = *(ptr1++);
			if(isdigit(c))
				hex = (c - '0') << 4;
			else
				hex = (tolower(c) - 'a' + 10) << 4;

			c = *(ptr1++);

			if(isdigit(c))
				hex += (c - '0');
			else
				hex += (tolower(c) - 'a' + 10);

			*(ptr2++) = hex;
		}
		else if (c == '+')
			*(ptr2++) = 32;
		else
			*(ptr2++) = c;
	}
	*ptr2 = 0;
}
VOID PollSession(struct HTTPConnectionInfo * Session)
{
	int state, change;
	int count, len;
	char Formatted[8192];
	char Msg[400] = "";
	char * ptr1, * ptr2;
	char c;
	int Line;

	// Poll Node

	SessionState(Session->Stream, &state, &change);

	if (change == 1)
	{
		int Line = Session->LastLine++;
		free(Session->ScreenLines[Line]);

		if (state == 1)// Connected
			Session->ScreenLines[Line] = _strdup("*** Connected<br>\r\n");
		else
			Session->ScreenLines[Line] = _strdup("*** Disconnected<br>\r\n");

		if (Line == 99)
			Session->LastLine = 0;

		Session->Changed = TRUE;
	}

	if (RXCount(Session->Stream) > 0)
	{
		int realLen = 0;

		do
		{
			GetMsg(Session->Stream, &Msg[0], &len, &count);

			// replace cr with <br> and space with &nbsp;


			ptr1 = Msg;
			ptr2 = &Formatted[0];

			if (Session->PartLine)
			{
				// Last line was incomplete - append to it

				realLen = Session->PartLine;

				Line = Session->LastLine - 1;

				if (Line < 0)
					Line = 99;

				strcpy(Formatted, Session->ScreenLines[Line]);
				ptr2 += strlen(Formatted);

				Session->LastLine = Line;
				Session->PartLine = FALSE;
			}

			while (len--)
			{
				c = *(ptr1++);
				realLen++;

				if (c == 13)
				{
					int LineLen;

					strcpy(ptr2, "<br>\r\n");

					// Write to screen

					Line = Session->LastLine++;
					free(Session->ScreenLines[Line]);

					LineLen = (int)strlen(Formatted);

					// if line starts with a colour code, process it

					if (Formatted[0] ==  0x1b && LineLen > 1)
					{
						int ColourCode = Formatted[1] - 10;
						COLORREF Colour = Colours[ColourCode];
						char ColString[30];

						memmove(&Formatted[20], &Formatted[2], LineLen);
						sprintf(ColString, "<font color=#%02X%02X%02X>",  GetRValue(Colour), GetGValue(Colour), GetBValue(Colour));
						memcpy(Formatted, ColString, 20);
						strcat(Formatted, "</font>");
						LineLen =+ 28;
					}

					Session->ScreenLineLen[Line] = LineLen;
					Session->ScreenLines[Line] = _strdup(Formatted);

					if (Line == 99)
						Session->LastLine = 0;

					ptr2 = &Formatted[0];
					realLen = 0;

				}
				else if (c == 32)
				{
					memcpy(ptr2, "&nbsp;", 6);
					ptr2 += 6;

					// Make sure line isn't too long
					// but beware of spaces expanded to &nbsp; - count chars in line

					if ((realLen) > 100)
					{
						strcpy(ptr2, "<br>\r\n");

						Line = Session->LastLine++;
						free(Session->ScreenLines[Line]);

						Session->ScreenLines[Line] = _strdup(Formatted);

						if (Line == 99)
							Session->LastLine = 0;

						ptr2 = &Formatted[0];
						realLen = 0;
					}
				}
				else if (c == '>')
				{
					memcpy(ptr2, "&gt;", 4);
					ptr2 += 4;
				}
				else if (c == '<')
				{
					memcpy(ptr2, "&lt;", 4);
					ptr2 += 4;
				}
				else
					*(ptr2++) = c;

			}

			*ptr2 = 0;

			if (ptr2 != &Formatted[0])
			{
				// Incomplete line

				// Save to screen

				Line = Session->LastLine++;
				free(Session->ScreenLines[Line]);

				Session->ScreenLines[Line] = _strdup(Formatted);

				if (Line == 99)
					Session->LastLine = 0;

				Session->PartLine = realLen;
			}

			//	strcat(Session->ScreenBuffer, Formatted);
			Session->Changed = TRUE;

		} while (count > 0);
	}
}


VOID HTTPTimer()
{
	// Run every tick. Check for status change and data available

	struct HTTPConnectionInfo * Session = SessionList;	// active term mode sessions
	struct HTTPConnectionInfo * PreviousSession = NULL;

//	inf();

	while (Session)
	{
		Session->KillTimer++;

		if (Session->Key[0] != 'T')
		{
			PreviousSession = Session;
			Session = Session->Next;
			continue;
		}

		if (Session->KillTimer > 3000)		// Around 5 mins
		{
			int i;
			int Stream = Session->Stream;

			for (i = 0; i < 100; i++)
			{
				free(Session->ScreenLines[i]);
			}

			SessionControl(Stream, 2, 0);
			SessionState(Stream, &i, &i);
			DeallocateStream(Stream);

			if (PreviousSession)
				PreviousSession->Next = Session->Next;		// Remove from chain
			else
				SessionList = Session->Next;

			free(Session);

			break;
		}

		PollSession(Session);

		//		if (Session->ResponseTimer == 0 && Session->Changed)
		//			Debugprintf("Data to send but no outstanding GET");

		if (Session->ResponseTimer)
		{
			Session->ResponseTimer--;

			if (Session->ResponseTimer == 0 || Session->Changed)
			{
				SOCKET sock = Session->sock;
				char _REPLYBUFFER[100000];
				int ReplyLen;
				char Header[1024];
				int HeaderLen;
				int Last = Session->LastLine;
				int n;
				struct TNCINFO * TNC = Session->TNC;
				struct TCPINFO * TCP = 0;
				
				if (TNC)
					TCP = TNC->TCPInfo;

				if (TCP && TCP->WebTermCSS)	
					sprintf(_REPLYBUFFER, TermOutput, TCP->WebTermCSS);
				else
					sprintf(_REPLYBUFFER, TermOutput, "");

				for (n = Last;;)
				{
					if ((strlen(Session->ScreenLines[n]) + strlen(_REPLYBUFFER)) < 99999)
						strcat(_REPLYBUFFER, Session->ScreenLines[n]);

					if (n == 99)
						n = -1;

					if (++n == Last)
						break;
				}

				ReplyLen = (int)strlen(_REPLYBUFFER);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", TermOutputTail);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen);
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, _REPLYBUFFER, ReplyLen);

				Session->ResponseTimer = Session->Changed = 0;
			}
		}
		PreviousSession = Session;
		Session = Session->Next;
	}
}

struct HTTPConnectionInfo * AllocateSession(SOCKET sock, char Mode)
{
	time_t KeyVal;
	struct HTTPConnectionInfo * Session = zalloc(sizeof(struct HTTPConnectionInfo));
	int i;

	if (Session == NULL)
		return NULL;

	if (Mode == 'T')
	{
		// Terminal

		for (i = 0; i < 20; i++)
			Session->ScreenLines[i] = _strdup("Scroll to end<br>");

		for (i = 20; i < 100; i++)
			Session->ScreenLines[i] = _strdup("<br>\r\n");

		Session->Stream = FindFreeStream();

		if (Session->Stream == 0)
			return NULL;

		SessionControl(Session->Stream, 1, 0);
	}

	KeyVal = (int)sock * time(NULL);

	sprintf(Session->Key, "%c%012X", Mode, (int)KeyVal);

	if (SessionList)
		Session->Next = SessionList;

	SessionList = Session;

	return Session;
}

struct HTTPConnectionInfo * FindSession(char * Key)
{
	struct HTTPConnectionInfo * Session = SessionList;

	while (Session)
	{
		if (strcmp(Session->Key, Key) == 0)
			return Session;

		Session = Session->Next;
	}

	return NULL;
}

void ProcessTermInput(SOCKET sock, char * MsgPtr, int MsgLen, char * Key)
{
	char _REPLYBUFFER[9000];
	int ReplyLen;
	char Header[1024];
	int HeaderLen;
	int State;
	struct HTTPConnectionInfo * Session = FindSession(Key);
	int Stream;
	int maxlen = 1000;


	if (Session == NULL)
	{
		ReplyLen = sprintf(_REPLYBUFFER, "%s", LostSession);
	}
	else
	{
		char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
		char * end = &MsgPtr[MsgLen];
		int Line = Session->LastLine++;
		char * ptr1, * ptr2;
		char c;
		UCHAR hex;

		int msglen = end - input;

		struct TNCINFO * TNC = Session->TNC;
		struct TCPINFO * TCP = 0;
				
		if (TNC)
			TCP = TNC->TCPInfo;

		if (TCP && TCP->WebTermCSS)
			maxlen -= strlen(TCP->WebTermCSS);

		if (MsgLen > maxlen)
		{
			Session->KillTimer = 99999; // close session
			return;
		}


		if (TCP && TCP->WebTermCSS)	
			ReplyLen = snprintf(_REPLYBUFFER, sizeof(_REPLYBUFFER), InputLine, Key, TCP->WebTermCSS);
		else
			ReplyLen = snprintf(_REPLYBUFFER, sizeof(_REPLYBUFFER), InputLine, Key, "");


		Stream = Session->Stream;

		input += 10;
		ptr1 = ptr2 = input;

		// Convert any %xx constructs

		while (ptr1 != end)
		{
			c = *(ptr1++);
			if (c == '%')
			{
				c = *(ptr1++);
				if(isdigit(c))
					hex = (c - '0') << 4;
				else
					hex = (tolower(c) - 'a' + 10) << 4;

				c = *(ptr1++);
				if(isdigit(c))
					hex += (c - '0');
				else
					hex += (tolower(c) - 'a' + 10);

				*(ptr2++) = hex;
			}
			else if (c == '+')
				*(ptr2++) = 32;
			else
				*(ptr2++) = c;
		}

		end = ptr2;

		*ptr2 = 0;

		strcat(input, "<br>\r\n");

		free(Session->ScreenLines[Line]);

		Session->ScreenLines[Line] = _strdup(input);

		if (Line == 99)
			Session->LastLine = 0;

		*end++ = 13;
		*end = 0;

		SessionStateNoAck(Stream, &State);

		if (State == 0)
		{
			char AXCall[10];
			SessionControl(Stream, 1, 0);
			if (BPQHOSTVECTOR[Session->Stream -1].HOSTSESSION == NULL)
			{
				//No L4 sessions free

				ReplyLen = sprintf(_REPLYBUFFER, "%s", NoSessions);
				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);                                                                                                                                                                                                                                        				
				send(sock, Tail, (int)strlen(Tail), 0);
				return;
			}

			ConvToAX25(Session->HTTPCall, AXCall);
			ChangeSessionCallsign(Stream, AXCall);
			if (Session->USER)
				BPQHOSTVECTOR[Session->Stream -1].HOSTSESSION->Secure_Session = Session->USER->Secure;
			else
				Debugprintf("HTTP Term Session->USER is NULL");
		}

		SendMsg(Stream, input, (int)(end - input));
		Session->Changed = TRUE;
		Session->KillTimer = 0;
	}

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, _REPLYBUFFER, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);
}


void ProcessTermClose(SOCKET sock, char * MsgPtr, int MsgLen, char * Key, int LOCAL)
{
	char _REPLYBUFFER[250000];
	int ReplyLen = sprintf(_REPLYBUFFER, InputLine, Key, "");
	char Header[1024];
	int HeaderLen;
	struct HTTPConnectionInfo * Session = FindSession(Key);

	if (Session)
	{
		Session->KillTimer = 99999;
	}

	ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, _REPLYBUFFER, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);
}

int ProcessTermSignon(struct TNCINFO * TNC, SOCKET sock, char * MsgPtr, int MsgLen,  int LOCAL)
{
	char _REPLYBUFFER[250000];
	int ReplyLen;
	char Header[1024];
	int HeaderLen;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Context, * Appl;
	char NoApp[] = "";
	struct TCPINFO * TCP = TNC->TCPInfo;

	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = sprintf(_REPLYBUFFER, "<!DOCTYPE html><html><head><title>Node Redirect</title></head><body><script>window.location.href='/Node/NodeIndex.html';</script>");
			goto Sendit;
		}
		user = strtok_s(&input[9], "&", &Context);
		password = strtok_s(NULL, "=", &Context);
		password = strtok_s(NULL, "&", &Context);

		Appl = strtok_s(NULL, "=", &Context);
		Appl = strtok_s(NULL, "&", &Context);

		if (Appl == 0)
			Appl = NoApp;

		if (password == NULL)
		{
			ReplyLen = sprintf(_REPLYBUFFER, TermSignon, Mycall, Mycall, Appl);
			ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", PassError);
			goto Sendit;
		}

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if ((strcmp(password, USER->Password) == 0) &&
				((_stricmp(user, USER->UserName) == 0 ) || (_stricmp(USER->UserName, "ANON") == 0)))
			{
				// ok

				struct HTTPConnectionInfo * Session = AllocateSession(sock, 'T');

				if (Session)
				{
					char AXCall[10];
					ReplyLen = sprintf(_REPLYBUFFER, TermPage, Mycall, Mycall, Session->Key, Session->Key, Session->Key);
					if (_stricmp(USER->UserName, "ANON") == 0)
						strcpy(Session->HTTPCall, _strupr(user));
					else
						strcpy(Session->HTTPCall, USER->Callsign);
					ConvToAX25(Session->HTTPCall, AXCall);
					ChangeSessionCallsign(Session->Stream, AXCall);
					BPQHOSTVECTOR[Session->Stream -1].HOSTSESSION->Secure_Session = USER->Secure;
					Session->USER = USER;

					if (USER->Appl[0])
						SendMsg(Session->Stream, USER->Appl, (int)strlen(USER->Appl));
				}
				else
				{
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);

					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", BusyError);
				}
				break;
			}
		}

		if (i == TCP->NumberofUsers)
		{
			//   Not found

			ReplyLen = sprintf(_REPLYBUFFER, TermSignon, Mycall, Mycall, Appl);
			ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", PassError);
		}

	}

Sendit:

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, _REPLYBUFFER, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);

	return 1;
}

char * LookupKey(char * Key)
{
	if (strcmp(Key, "##MY_CALLSIGN##") == 0)
	{
		char Mycall[10];
		memcpy(Mycall, &MYNODECALL, 10);
		strlop(Mycall, ' ');

		return _strdup(Mycall);
	}
	return NULL;
}


int ProcessSpecialPage(char * Buffer, int FileSize)
{
	// replaces ##xxx### constructs with the requested data

	char * NewMessage = malloc(100000);
	char * ptr1 = Buffer, * ptr2, * ptr3, * ptr4, * NewPtr = NewMessage;
	int PrevLen;
	int BytesLeft = FileSize;
	int NewFileSize = FileSize;
	char * StripPtr = ptr1;

	// strip comments blocks

	while (ptr4 = strstr(ptr1, "<!--"))
	{
		ptr2 = strstr(ptr4, "-->");
		if (ptr2)
		{
			PrevLen = (int)(ptr4 - ptr1);
			memcpy(StripPtr, ptr1, PrevLen);
			StripPtr += PrevLen;
			ptr1 = ptr2 + 3;
			BytesLeft = (int)(FileSize - (ptr1 - Buffer));
		}
	}

	memcpy(StripPtr, ptr1, BytesLeft);
	StripPtr += BytesLeft;

	BytesLeft = (int)(StripPtr - Buffer);

	FileSize = BytesLeft;
	NewFileSize = FileSize;
	ptr1 = Buffer;
	ptr1[FileSize] = 0;

loop:
	ptr2 = strstr(ptr1, "##");

	if (ptr2)
	{
		PrevLen = (int)(ptr2 - ptr1);			// Bytes before special text

		ptr3 = strstr(ptr2+2, "##");

		if (ptr3)
		{
			char Key[80] = "";
			int KeyLen;
			char * NewText;
			int NewTextLen;

			ptr3 += 2;
			KeyLen = (int)(ptr3 - ptr2);

			if (KeyLen < 80)
				memcpy(Key, ptr2, KeyLen);

			NewText = LookupKey(Key);

			if (NewText)
			{
				NewTextLen = (int)strlen(NewText);
				NewFileSize = NewFileSize + NewTextLen - KeyLen;					
				//	NewMessage = realloc(NewMessage, NewFileSize);

				memcpy(NewPtr, ptr1, PrevLen);
				NewPtr += PrevLen;
				memcpy(NewPtr, NewText, NewTextLen);
				NewPtr += NewTextLen;

				free(NewText);
				NewText = NULL;
			}
			else
			{
				// Key not found, so just leave

				memcpy(NewPtr, ptr1, PrevLen + KeyLen);
				NewPtr += (PrevLen + KeyLen);
			}

			ptr1 = ptr3;			// Continue scan from here
			BytesLeft = (int)(Buffer + FileSize - ptr3);
		}
		else		// Unmatched ##
		{
			memcpy(NewPtr, ptr1, PrevLen + 2);
			NewPtr += (PrevLen + 2);
			ptr1 = ptr2 + 2;
		}
		goto loop;
	}

	// Copy Rest

	memcpy(NewPtr, ptr1, BytesLeft);
	NewMessage[NewFileSize] = 0;

	strcpy(Buffer, NewMessage);
	free(NewMessage);

	return NewFileSize;
}

int SendMessageFile(SOCKET sock, char * FN, BOOL OnlyifExists, int allowDeflate)
{
	int FileSize = 0, Sent, Loops = 0;
	char * MsgBytes;
	char MsgFile[512];
	FILE * hFile;
	int ReadLen;
	BOOL Special = FALSE;
	int Len;
	int HeaderLen;
	char Header[1024];
	char TimeString[64];
	char FileTimeString[64];
	struct stat STAT;
	char * ptr;
	char * Compressed = 0;
	char Encoding[] = "Content-Encoding: deflate\r\n";
	char Type[64] = "Content-Type: text/html\r\n";
 
#ifdef WIN32

	struct _EXCEPTION_POINTERS exinfo;
	strcpy(EXCEPTMSG, "SendMessageFile");

	__try {
#endif

		UndoTransparency(FN);

		if (strstr(FN, ".."))
		{
			FN[0] = '/';
			FN[1] = 0;
		}

		if (strlen(FN) > 256)
		{
			FN[256] = 0;
			Debugprintf("HTTP File Name too long %s", FN);
		}

		if (strcmp(FN, "/") == 0)
			if (APRSActive)
				sprintf(MsgFile, "%s/HTML/index.html", BPQDirectory);
			else
				sprintf(MsgFile, "%s/HTML/indexnoaprs.html", BPQDirectory);
		else
			sprintf(MsgFile, "%s/HTML%s", BPQDirectory, FN);

#ifndef WIN32
		{
			char Resolved[PATH_MAX];
			char ExpectedRoot[512];
			snprintf(ExpectedRoot, sizeof(ExpectedRoot), "%s/HTML", BPQDirectory);

			if (realpath(MsgFile, Resolved) != NULL)
			{
				size_t rootLen = strlen(ExpectedRoot);
				if (strncmp(Resolved, ExpectedRoot, rootLen) != 0 ||
					(Resolved[rootLen] != '/' && Resolved[rootLen] != '\0'))
				{
					Debugprintf("HTTP Path traversal blocked: %s -> %s", MsgFile, Resolved);
					if (OnlyifExists)
						return -1;
					Len = sprintf(Header, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
					send(sock, Header, Len, 0);
					return 0;
				}
			}
		}
#endif

		// First see if file exists so we can override standard ones in code

		if (stat(MsgFile, &STAT) == 0 && (hFile = fopen(MsgFile, "rb")))
		{
			FileSize = STAT.st_size;

			MsgBytes = zalloc(FileSize + 1);

			ReadLen = (int)fread(MsgBytes, 1, FileSize, hFile);

			fclose(hFile);

			//	ft.QuadPart -=  116444736000000000;
			//	ft.QuadPart /= 10000000;

			//	ctime = ft.LowPart;

			FormatTime3(FileTimeString, STAT.st_ctime);
		}
		else
		{
			// See if it is a hard coded file

			MsgBytes = GetStandardPage(&FN[1], &FileSize);

			if (MsgBytes)
			{
				if (FileSize == 0)
					FileSize = strlen(MsgBytes);

				FormatTime3(FileTimeString, 0);
			}
			else
			{
				if (OnlyifExists)					// Set if we dont want an error response if missing
					return -1;

				Len = sprintf(Header, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
				send(sock, Header, Len, 0);
				return 0;
			}
		}

		// if HTML file, look for ##...## substitutions

		if ((strcmp(FN, "/") == 0 || strstr(FN, "htm" ) || strstr(FN, "HTM")) &&  strstr(MsgBytes, "##" ))
		{
			FileSize = ProcessSpecialPage(MsgBytes, FileSize); 
			FormatTime3(FileTimeString, time(NULL));

		}

		FormatTime3(TimeString, time(NULL));

		ptr = FN;

		while (strchr(ptr, '.'))
		{
			ptr = strchr(ptr, '.');
			++ptr;
		}

		if (_stricmp(ptr, "js") == 0)
			strcpy(Type, "Content-Type: text/javascript\r\n");
	
		if (_stricmp(ptr, "css") == 0)
			strcpy(Type, "Content-Type: text/css\r\n");

		if (_stricmp(ptr, "pdf") == 0)
			strcpy(Type, "Content-Type: application/pdf\r\n");

		// Static assets (.js, .css) get a 1-day cache lifetime to reduce repeat requests
		char CacheControl[48] = "";
		if (_stricmp(ptr, "js") == 0 || _stricmp(ptr, "css") == 0)
			strcpy(CacheControl, "Cache-Control: max-age=86400\r\n");

		if (allowDeflate)
		{
			Compressed = Compressit(MsgBytes, FileSize, &FileSize);
		} 
		else
		{
			Encoding[0] = 0;
			Compressed = MsgBytes;
		}

		if (_stricmp(ptr, "jpg") == 0 || _stricmp(ptr, "jpeg") == 0 || _stricmp(ptr, "png") == 0 ||
			_stricmp(ptr, "gif") == 0 || _stricmp(ptr, "bmp") == 0 || _stricmp(ptr, "ico") == 0)
			strcpy(Type, "Content-Type: image\r\n");

		HeaderLen = snprintf(Header, sizeof(Header), "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
				"Date: %s\r\n"
				"Last-Modified: %s\r\n"
				"%s%s%s"
				COMMON_HTTP_SECURITY_HEADERS "\r\n", FileSize, TimeString, FileTimeString, Type, Encoding, CacheControl);
	
		send(sock, Header, HeaderLen, 0);

		Sent = send(sock, Compressed, FileSize, 0);

		while (Sent != FileSize && Loops++ < 3000)				// 100 secs max
		{	
			if (Sent > 0)					// something sent
			{
//				Debugprintf("%d out of %d sent", Sent, FileSize);
				FileSize -= Sent;
				memmove(Compressed, &Compressed[Sent], FileSize);
			}

			Sleep(30);
			Sent = send(sock, Compressed, FileSize, 0);
		}

//		Debugprintf("%d out of %d sent %d loops", Sent, FileSize, Loops);


		free (MsgBytes);
		if (allowDeflate)
			free (Compressed);

#ifdef WIN32
	}
#include "StdExcept.c"
	Debugprintf("Sending FIle %s", FN);
}
#endif

return 0;
}

VOID sendandcheck(SOCKET sock, const char * Buffer, int Len)
{
	int Loops = 0;
	int Sent = send(sock, Buffer, Len, 0);
	char * Copy = NULL;

	while (Sent != Len && Loops++ < 300)					// 10 secs max
	{	
		//		Debugprintf("%d out of %d sent %d Loops", Sent, Len, Loops);

		if (Copy == NULL)
		{
			Copy = malloc(Len);
			memcpy(Copy, Buffer, Len);
		}

		if (Sent > 0)					// something sent
		{
			Len -= Sent;
			memmove(Copy, &Copy[Sent], Len);
		}		

		Sleep(30);
		Sent = send(sock, Copy, Len, 0);
	}

	if (Copy)
		free(Copy);

	return;
}

int RefreshTermWindow(struct TCPINFO * TCP, struct HTTPConnectionInfo * Session, char * _REPLYBUFFER)
{
	int HeaderLen, ReplyLen;
	char Header[1024];

	PollSession(Session);			// See if anything received 

	if (Session->Changed)
	{
		int Last = Session->LastLine;
		int n;
	
		if (TCP && TCP->WebTermCSS)	
			sprintf(_REPLYBUFFER, TermOutput, TCP->WebTermCSS);
		else
			sprintf(_REPLYBUFFER, TermOutput, "");

		for (n = Last;;)
		{
			strcat(_REPLYBUFFER, Session->ScreenLines[n]);

			if (n == 99)
				n = -1;

			if (++n == Last)
				break;
		}

		Session->Changed = 0;

		ReplyLen = (int)strlen(_REPLYBUFFER);
		ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", TermOutputTail);

		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen);
		sendandcheck(Session->sock, Header, HeaderLen);
		sendandcheck(Session->sock, _REPLYBUFFER, ReplyLen);

		return 1;
	}
	else
		return 0;
}	

int SetupNodeMenu(char * Buff, size_t BuffSize, int LOCAL)
{
	int Len = 0, i;
	struct TNCINFO * TNC;
	int top = 0, left = 0;
	char MgmtMenu[8192];

	char NodeMenuHeader[] = "<!DOCTYPE html><html lang=\"en\"><head>" COMMON_HTML_META_UTF8_QUOTED COMMON_FONT_INTER_LINK COMMON_HTML_CANONICAL_LINK_ONLY COMMON_BPQ_CSS_LINK "<title>%s's BPQ32 Web Server</title>"
	"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>"
	"<style type=\"text/css\">" HTTP_NODE_MENU_CSS "</style>"
	"<script>" COMMON_THEME_FOUC_JAVASCRIPT "</script>"
	COMMON_BPQ_JS_SCRIPT
	"<script>\r\n"
	COMMON_NODE_MGMT_JAVASCRIPT

	"function dev_win(URL,w,h,top,left){"
		"var ww = \"width=\" + w;"
		"var xx = \",height=\" + h;"
		"var yy = \",top=\" + top;"
		"var zz = \",left=\" + left;"
		"var param = \"toolbar=no, location=no, directories=no, status=no, "
		"menubar=no, scrollbars=no, resizable=no, titlebar=no, toobar=no, \" + ww + xx + yy + zz;"
		"window.open(URL,\"_blank\",param);"
		"}\r\n"
	
	"function open_win(){";

	char NodeMenuLine[] = "dev_win(\"/Node/Port?%d\",%d,%d,%d,%d);";

	char NodeMenuRest[] = "}</script></head>"
		"<body>"
		"<div class='theme-sel'>"
		"<button class='tbtn' data-tbtn='system' onclick='bpqSetThemeMode(\"system\")' title='System theme' aria-label='System theme'>&#9881;</button>"
		"<button class='tbtn' data-tbtn='light' onclick='bpqSetThemeMode(\"light\")' title='Light theme' aria-label='Light theme'>&#9728;</button>"
		"<button class='tbtn' data-tbtn='dark' onclick='bpqSetThemeMode(\"dark\")' title='Dark theme' aria-label='Dark theme'>&#9790;</button>"
		"</div>"
		"<h1>BPQ32 Node %s</h1>"
		"<div class='menu-header'><button id='menuToggle' class='menu-toggle' type='button' aria-expanded='false' aria-controls='mainMenu' onclick='toggleMenu(event)'>Menu</button></div>"
		"<div id='mainMenu' class=\"menu\">"
		"<a href='/'>Home</a>"
		"<a href='/Node/Routes.html'>Routes</a>"
		"<a href='/Node/Nodes.html'>Nodes</a>"
		"<a href='/Node/Ports.html'>Ports</a>"
		"<a href='/Node/Links.html'>Links</a>"
		"<a href='/Node/Users.html'>Users</a>"
		"<a href='/Node/Stats.html'>Stats</a>"
		"<a href='/Node/Terminal.html'>Terminal</a>%s%s%s%s";

	char DriverBit[] = "<a href=\"javascript:open_win();\">Driver Windows</a>"
		"<a href=\"javascript:dev_win('/Node/Streams',820,700,200,200);\">Stream Status</a>";

	char APRSBit[] = "<a href='../aprs'>APRS</a>";

	char WebMailBit[] = "<a href='/Webmail'>WebMail</a>";

	char MailBit[] = "<a href='../Mail/Header'>Mail Mgmt</a>";

	char ChatBit[] = "<a href='../Chat/Header'>Chat Mgmt</a>";
	char SigninBit[] = "<a href='/Node/Signon.html'>Sysop Signin</a>";

	char MgmtBit[] =
		"<div class='dropdown'>"
		"<button id='mgmtButton' class='btn' type='button' onclick='toggleMgmt(event)'>Mgmt</button>"
		"<div id='mgmtDropdown' class='dropdown-content'>"
		"%s%s%s"
		"<a href=\"javascript:open_win();\">Driver Windows</a>"
		"<a href=\"javascript:dev_win('/Node/Streams',820,700,200,200);\">Stream Status</a>"
		"<a href='/Node/EditCfg.html'>Edit Config</a>"
		"<button id='mgmtLogsToggle' class='btn mgmt-toggle' data-label='Logs' type='button' onclick='toggleMgmtSection(event,\"mgmtLogs\",\"mgmtLogsToggle\")'>Logs +</button>"
		"<div id='mgmtLogs' class='mgmt-section'>"
		"<form id='doDate' action='/node/ShowLog.html'><label>"
		"Select Date: <input type='date' name='date' id='e'>"
		"<script>"
		"document.getElementById('e').value = new Date().toISOString().substring(0, 10);"
		"</script></label>"
		"<input type='submit' class='btn' name='BBS' value='BBS Log'>"
		"<input type='submit' class='btn' name='Debug' value='BBS Debug Log'>"
		"<input type='submit' class='btn' name='Telnet' value='Telnet Log'>"
		"<input type='submit' class='btn' name='CMS' value='CMS Log'>"
		"<input type='submit' class='btn' name='Chat' value='Chat Log'>"
		"</form></div>"
		"</div>"
		"</div>";

	char NodeTail[] =
		"</div>";


	int n;

	if (BuffSize == 0)
		return 0;

	n = snprintf(Buff, BuffSize, NodeMenuHeader, Mycall);
	if (n < 0)
		return 0;

	if ((size_t)n >= BuffSize)
		return (int)(BuffSize - 1);

	Len = n;

	for (i=1; i <= MAXBPQPORTS; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC->WebWindowProc)
		{
			size_t Remaining;

			if ((size_t)Len >= BuffSize)
				return (int)(BuffSize - 1);

			Remaining = BuffSize - (size_t)Len;
			n = snprintf(&Buff[Len], Remaining, NodeMenuLine, i, TNC->WebWinX, TNC->WebWinY, top, left);

			if (n < 0)
				return Len;

			if ((size_t)n >= Remaining)
				return (int)(BuffSize - 1);

			Len += n;
			top += 22;
			left += 22;
		}
	}

	n = snprintf(MgmtMenu, sizeof(MgmtMenu), MgmtBit,
		(IncludesMail)?MailBit:"",
		(IncludesChat)?ChatBit:"",
		(LOCAL)?"":SigninBit);

	if (n < 0)
		MgmtMenu[0] = 0;
	else if ((size_t)n >= sizeof(MgmtMenu))
		MgmtMenu[sizeof(MgmtMenu) - 1] = 0;

	if ((size_t)Len >= BuffSize)
		return (int)(BuffSize - 1);

	n = snprintf(&Buff[Len], BuffSize - (size_t)Len, NodeMenuRest, Mycall,
		(APRSWeb)?APRSBit:"",
		(IncludesMail)?WebMailBit:"",
		MgmtMenu,
		NodeTail);

	if (n < 0)
		return Len;

	if ((size_t)n >= BuffSize - (size_t)Len)
		return (int)(BuffSize - 1);

	Len += n;

	return Len;
}

int BuildNodeIndexPage(char * Buff, size_t BuffSize, int LOCAL)
{
	char InsertFile[512];
	char * InsertBytes = NULL;
	char * InsertPoint;
	FILE * hInsert;
	struct stat InsertSTAT;
	int InsertLen;
	int ReplyLen = SetupNodeMenu(Buff, BuffSize, LOCAL);
	int TailLen = (int)strlen(Tail);

	if (ReplyLen <= 0)
		return ReplyLen;

	// SetupNodeMenu leaves the document open; append custom content immediately
	// after the menu markup so it stays outside the menu container.
	InsertPoint = Buff + ReplyLen;

	snprintf(InsertFile, sizeof(InsertFile), "%s/HTML/index_insert.html", BPQDirectory);

	if (stat(InsertFile, &InsertSTAT) != 0)
	{
		if ((size_t)(ReplyLen + TailLen) >= BuffSize)
			return ReplyLen;

		memcpy(Buff + ReplyLen, Tail, TailLen + 1);
		return ReplyLen + TailLen;
	}

	hInsert = fopen(InsertFile, "rb");

	if (hInsert == NULL)
	{
		if ((size_t)(ReplyLen + TailLen) >= BuffSize)
			return ReplyLen;

		memcpy(Buff + ReplyLen, Tail, TailLen + 1);
		return ReplyLen + TailLen;
	}

	InsertBytes = zalloc(InsertSTAT.st_size + 4096 + 1);
	InsertLen = (int)fread(InsertBytes, 1, InsertSTAT.st_size, hInsert);
	fclose(hInsert);

	if (InsertLen <= 0)
	{
		free(InsertBytes);

		if ((size_t)(ReplyLen + TailLen) >= BuffSize)
			return ReplyLen;

		memcpy(Buff + ReplyLen, Tail, TailLen + 1);
		return ReplyLen + TailLen;
	}

	if (strstr(InsertBytes, "##"))
		InsertLen = ProcessSpecialPage(InsertBytes, InsertLen);

	if ((size_t)(ReplyLen + InsertLen + TailLen) >= BuffSize)
	{
		free(InsertBytes);
		return ReplyLen;
	}

	memmove(InsertPoint + InsertLen, InsertPoint, 1);
	memcpy(InsertPoint, InsertBytes, InsertLen);
	free(InsertBytes);

	ReplyLen += InsertLen;
	memcpy(Buff + ReplyLen, Tail, TailLen + 1);

	return ReplyLen + TailLen;
}

VOID SaveConfigFile(SOCKET sock , char * MsgPtr, char * Rest, int LOCAL)
{
	int ReplyLen = 0;
	char * ptr, * ptr1, * ptr2, *input;
	char c;
	int MsgLen, WriteLen = 0;
	char inputname[250]="bpq32.cfg";
	FILE *fp1;
	char Header[1024];
	int HeaderLen;
	char Reply[250000];
	char Mess[256];
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];
	struct stat STAT;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = SetupNodeMenu(Reply, sizeof(Reply), LOCAL);
			//			ReplyLen = sprintf(Reply, "%s", "<html><script>window.close();</script></html>");
			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
			send(sock, Header, HeaderLen, 0);
			send(sock, Reply, ReplyLen, 0);
			send(sock, Tail, (int)strlen(Tail), 0);
			return;
		}

		ptr = strstr(input, "&Save=");

		if (ptr)
		{
			*ptr = 0;

			// Undo any % transparency

			ptr1 = ptr2 = input + 8;

			c = *(ptr1++);

			while (c)
			{
				if (c == '%')
				{
					int n;
					int m = *(ptr1++) - '0';
					if (m > 9) m = m - 7;
					n = *(ptr1++) - '0';
					if (n > 9) n = n - 7;

					c  = m * 16 + n;
				}
				else if (c == '+')
					c = ' ';

#ifndef WIN32
				if (c != 13)				// Strip CR if Linux
#endif
					*(ptr2++) = c;

				c = *(ptr1++);

			}

			*(ptr2++) = 0;

			MsgLen = (int)strlen(input + 8);

			if (ConfigDirectory[0] == 0)
			{
				strcpy(inputname, "bpq32.cfg");
			}
			else
			{
				strcpy(inputname,ConfigDirectory);
				strcat(inputname,"/");
				strcat(inputname, "bpq32.cfg");
			}

			// Make a backup of the config

			// Keep 4 Generations

			strcpy(Backup2, inputname);
			strcat(Backup2, ".bak.3");

			strcpy(Backup1, inputname);
			strcat(Backup1, ".bak.2");

			DeleteFile(Backup2);			// Remove old .bak.3
			MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

			strcpy(Backup2, inputname);
			strcat(Backup2, ".bak.1");

			MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

			strcpy(Backup1, inputname);
			strcat(Backup1, ".bak");

			MoveFile(Backup1, Backup2);		// Move .bak to .bak.1

			CopyFile(inputname, Backup1, FALSE);	// Copy to .bak

			// Get length to compare with new length

			stat(inputname, &STAT);

			fp1 = fopen(inputname, "wb");

			if (fp1)
			{
				WriteLen = (int)fwrite(input + 8, 1, MsgLen, fp1);
				fclose(fp1);
			}

			if (WriteLen != MsgLen)
				sprintf_s(Mess, sizeof(Mess), "Failed to write Config File");
			else
				sprintf_s(Mess, sizeof(Mess), "Configuration Saved, Orig Length %d New Length %d", 
				STAT.st_size, MsgLen);
		}

		ReplyLen = sprintf(Reply, "<!DOCTYPE html><html><head><title>Config Save Status</title></head><body><script>alert(\"%s\");window.close();</script></body></html>", Mess);
		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen);
		send(sock, Header, HeaderLen, 0);
		send(sock, Reply, ReplyLen, 0);
	}
	return;
}

// Compress using deflate. Caller must free output buffer after use

unsigned char * Compressit(unsigned char * In, int Len, int * OutLen)
{
	z_stream defstream;
	int maxSize;
	unsigned char * Out;

	defstream.zalloc = Z_NULL;
	defstream.zfree = Z_NULL;
	defstream.opaque = Z_NULL;

	defstream.avail_in = Len; // size of input
	defstream.next_in = (Bytef *)In; // input char array

	deflateInit(&defstream, Z_BEST_COMPRESSION);
	maxSize = deflateBound(&defstream, Len);

	Out = malloc(maxSize);

	defstream.avail_out = maxSize; // size of output
	defstream.next_out = (Bytef *)Out; // output char array

	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);

	*OutLen = defstream.total_out;

	return Out;
}


int InnerProcessHTTPMessage(struct ConnectionInfo * conn)
{
	struct TCPINFO * TCP = conn->TNC->TCPInfo;
	SOCKET sock = conn->socket;
	char * MsgPtr = conn->InputBuffer;
	int MsgLen = conn->InputLen;
	int InputLen = 0;
	int OutputLen = 0;
	int Bufferlen;
	struct HTTPConnectionInfo CI;
	struct HTTPConnectionInfo * sockptr = &CI;
	struct HTTPConnectionInfo * Session = NULL;

	char URL[100000];
	char * ptr;
	char * encPtr = 0;
	int allowDeflate = 0;
	char * Compressed = 0;
	char * HostPtr = 0;

	char * Context, * Method, * NodeURL = 0, * Key;
	char _REPLYBUFFER[300000];
	char Reply[250000];

	int ReplyLen = 0;
	char Header[1024];
	int HeaderLen;
	char TimeString[64];
	BOOL LOCAL = FALSE;
	BOOL COOKIE = FALSE;
	int Len;
	char * WebSock = 0;

	char PortsHddr[] = HTTP_NODE_SECTION_TABLE_HEADER("Ports", "compact-table", HTTP_NODE_COLS_PORTS);

//	char PortLine[] = "<tr><td>%d</td><td><a href=PortStats?%d&%s>&nbsp;%s</a></td><td>%s</td></tr>";

	char PortLineWithBeacon[] = "<tr><td data-label='Port' class='num'>%d</td><td data-label='Driver' class='text'><a href=PortStats?%d&%s>%s</a></td><td data-label='ID' class='text'>%s</td>"
		"<td data-label='Beacons' class='text'><a href=PortBeacons?%d>Beacons</a></td><td data-label='Driver Window' class='text'>-</td><td data-label='Stats Graph' class='text'>%s</td></tr>\r\n";

	char SessionPortLine[] = "<tr><td data-label='Port' class='num'>%d</td><td data-label='Driver' class='text'>%s</td><td data-label='ID' class='text'>%s</td><td data-label='Beacons' class='text'>-</td>"
		"<td data-label='Driver Window' class='text'>-</td><td data-label='Stats Graph' class='text'>%s</td></tr>\r\n";

	char PortLineWithDriver[] = "<tr><td data-label='Port' class='num'>%d</td><td data-label='Driver' class='text'>%s</td><td data-label='ID' class='text'>%s</td><td data-label='Beacons' class='text'>-</td>"
		"<td data-label='Driver Window' class='text'><a href=\"javascript:dev_win('/Node/Port?%d',%d,%d,%d,%d);\">Driver Window</a></td><td data-label='Stats Graph' class='text'>%s</td></tr>\r\n";


	char PortLineWithBeaconAndDriver[] = "<tr><td data-label='Port' class='num'>%d</td><td data-label='Driver' class='text'>%s</td><td data-label='ID' class='text'>%s</td>"
		"<td data-label='Beacons' class='text'><a href=PortBeacons?%d>Beacons</a></td>"
		"<td data-label='Driver Window' class='text'><a href=\"javascript:dev_win('/Node/Port?%d',%d,%d,%d,%d);\">Driver Window</a></td><td data-label='Stats Graph' class='text'>%s</td></tr>\r\n";

	char RigControlLine[] = "<tr><td data-label='Port' class='num'>%d</td><td data-label='Driver' class='text'>%s</td><td data-label='ID' class='text'>%s</td><td data-label='Beacons' class='text'>-</td>"
		"<td data-label='Driver Window' class='text'><a href=\"javascript:dev_win('/Node/RigControl.html',%d,%d,%d,%d);\">Rig Control</a></td><td data-label='Stats Graph' class='text'>-</td></tr>\r\n";


	char Encoding[] = "Content-Encoding: deflate\r\n";

#ifdef WIN32xx

	struct _EXCEPTION_POINTERS exinfo;
	strcpy(EXCEPTMSG, "ProcessHTTPMessage");

	__try {
#endif

		Len = (int)strlen(MsgPtr);
		if (Len > 100000)
			return 0; 

		strcpy(URL, MsgPtr);

		HostPtr = strstr(MsgPtr, "Host: ");

		WebSock = strstr(MsgPtr, "Upgrade: websocket");
		
		if (HostPtr)
		{
			uint32_t Host;
			char Hostname[32]= "";
			struct LOCALNET * LocalNet = conn->TNC->TCPInfo->LocalNets;
			
			HostPtr += 6;
			memcpy(Hostname, HostPtr, 31);
			strlop(Hostname, ':');
			Host = inet_addr(Hostname);

			if (strcmp(Hostname, "127.0.0.1") == 0)
				LOCAL = TRUE;
			else
			{
				if (conn->sin.sin_family != AF_INET6)
				{
					while(LocalNet)
					{
						uint32_t MaskedHost = conn->sin.sin_addr.s_addr & LocalNet->Mask;
						if (MaskedHost == LocalNet->Network)
						{				
							LOCAL = 1;
							break;
						}
						LocalNet = LocalNet->Next;
					}
				}
			}
		}

		encPtr = stristr(MsgPtr, "Accept-Encoding:");

		if (encPtr && stristr(encPtr, "deflate"))
			allowDeflate = 1;
		else
			Encoding[0] = 0;

		ptr = strstr(MsgPtr, "BPQSessionCookie=N");

		if (ptr)
		{
			COOKIE = TRUE;
			Key = ptr + 17;
			ptr = strchr(Key, ',');
			if (ptr)
			{
				*ptr = 0;
				Session = FindSession(Key);
				*ptr = ',';
			}
			else
			{
				ptr = strchr(Key, 13);
				if (ptr)
				{
					*ptr = 0;
					Session = FindSession(Key);
					*ptr = 13;
				}
			}
		}

		if (WebSock)
		{
			// Websock connection request - Reply and remember state.

			char KeyMsg[128];
			char Webx[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";	// Fixed UID
			char Hash[64] = "";
			char * Hash64;		// base 64 version
			char * ptr;

			//Sec-WebSocket-Key: l622yZS3n+zI+hR6SVWkPw==

			char ReplyMsg[] = 
				"HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: %s\r\n"
//				"Sec-WebSocket-Protocol: chat\r\n"
				"\r\n";

			ptr = strstr(MsgPtr, "Sec-WebSocket-Key:");

			if (ptr)
			{
				ptr += 18;
				while (*ptr == ' ')
					ptr++;

				memcpy(KeyMsg, ptr, 40);
				strlop(KeyMsg, 13);
				strlop(KeyMsg, ' ');
				strcat(KeyMsg, Webx);

				SHA1PasswordHash(KeyMsg, Hash);
				Hash64 = byte_base64_encode(Hash, 20);

				conn->WebSocks = 1;
				strlop(&URL[4], ' ');
				strcpy(conn->WebURL, &URL[4]);

				ReplyLen = sprintf(Reply, ReplyMsg, Hash64);
			
				free(Hash64);
				goto Returnit;

			}
		}


		ptr = strstr(URL, " HTTP");

		if (ptr)
			*ptr = 0;

		Method = strtok_s(URL, " ", &Context);

		memcpy(Mycall, &MYNODECALL, 10);
		strlop(Mycall, ' ');


		// Look for API messages

		if (_memicmp(Context, "/api/", 5) == 0 || _stricmp(Context, "/api") == 0)
		{
			char * Compressed;

			// if for mail api process signon here and rearrange url from
			// api/v1/mail to mail/api/v1 so it goes to mail handler later

			if (_memicmp(Context, "/api/v1/mail/", 13) == 0)
			{
				memcpy(MsgPtr, "GET /mail/api/v1/", 17);
		
				if (memcmp(&Context[13], "login", 5) == 0)
				{
					ReplyLen = ProcessMailAPISignon(TCP, MsgPtr, "M", Reply, sizeof(Reply), &Session, FALSE, LOCAL);
					memcpy(MsgPtr, "GET /mail/api/v1/", 17);

					if (ReplyLen)			// Error message
						goto Returnit;
				}

				memcpy(Context, "/mail/api/v1/", 13);
				goto doHeader;
			}
			else
			{
				ReplyLen = APIProcessHTTPMessage(_REPLYBUFFER, Method, Context, MsgPtr, LOCAL, COOKIE);
				
				if (memcmp(_REPLYBUFFER, "HTTP", 4) == 0)
				{
					// Full Message - just send it

					sendandcheck(sock, _REPLYBUFFER, ReplyLen);

					return 0;
				}

				if (allowDeflate)
					Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
				else
					Compressed = _REPLYBUFFER;

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\n"
					"Content-Length: %d\r\n"
					"Content-Type: application/json\r\n"
					"Connection: close\r\n"
					"Access-Control-Allow-Origin: *\r\n"
					"%s\r\n", ReplyLen, Encoding);

				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, Compressed, ReplyLen);

				if (allowDeflate)
					free (Compressed);

				return 0;
			}
		}

		if (_memicmp(Context, "/rhp/", 5) == 0 || _stricmp(Context, "/rhp") == 0)
		{
			{
				ReplyLen = RHPProcessHTTPMessage(conn, _REPLYBUFFER, Method, Context, MsgPtr, LOCAL, COOKIE);
				
				if (memcmp(_REPLYBUFFER, "HTTP", 4) == 0)
				{
					// Full Message - just send it

					sendandcheck(sock, _REPLYBUFFER, ReplyLen);

					return 0;
				}

				if (allowDeflate)
					Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
				else
					Compressed = _REPLYBUFFER;

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\n"
					"Content-Length: %d\r\n"
					"Content-Type: application/json\r\n"
					"Connection: close\r\n"
					"Access-Control-Allow-Origin: *\r\n"
					"%s\r\n", ReplyLen, Encoding);

				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, Compressed, ReplyLen);

				if (allowDeflate)
					free (Compressed);

				return 0;
			}
		}


		// APRS process internally, including shared /bpq static assets

		if (_memicmp(Context, "/APRS/", 6) == 0 || _stricmp(Context, "/APRS") == 0 ||
			_memicmp(Context, "/bpq/", 5) == 0 || _stricmp(Context, "/bpq") == 0)
		{
			APRSProcessHTTPMessage(sock, MsgPtr, LOCAL, COOKIE);
			return 0;
		}


		if (_stricmp(Context, "/Node/Signon?Node") == 0)
		{
			char * IContext;

			NodeURL = strtok_s(Context, "?", &IContext);
			Key = strtok_s(NULL, "?", &IContext);

			ProcessNodeSignon(sock, TCP, MsgPtr, Key, Reply, sizeof(Reply), &Session, LOCAL);
			return 0;

		}

		// If for Mail or Chat, check for a session, and send login screen if necessary

		// Moved here to simplify operation with both internal and external clients

		if (_memicmp(Context, "/Mail/", 6) == 0)
		{
			int RLen = 0;
			char Appl;
			char * input;
			char * IContext;

			NodeURL = strtok_s(Context, "?", &IContext);
			Key = strtok_s(NULL, "?", &IContext);

			if (_stricmp(NodeURL, "/Mail/Signon") == 0)
			{
				ReplyLen = ProcessMailSignon(TCP, MsgPtr, Key, Reply, sizeof(Reply), &Session, FALSE, LOCAL);

				if (ReplyLen)
				{
					goto Returnit;
				}

#ifdef LINBPQ
				strcpy(Context, "/Mail/Header");
#else
				strcpy(MsgPtr, "POST /Mail/Header");
#endif
				goto doHeader;
			}

			if (_stricmp(NodeURL, "/Mail/Lost") == 0)
			{
				input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

				if (input && strstr(input, "Cancel=Exit"))
				{
					ReplyLen = sprintf(Reply, "<!DOCTYPE html><html><head><title>Mail Redirect</title></head><body><script>window.location.href='/Node/NodeIndex.html';</script>");
					RLen = ReplyLen;
					goto Returnit;
				}
				if (Key)
					Appl = Key[0];

				Key = 0;
			}

			if (Key == 0 || Key[0] == 0)
			{
				// No Session 

				// if not local send a signon screen, else create a user session

				if (LOCAL || COOKIE)
				{
					Session = AllocateSession(sock, 'M');

					if (Session)
					{
						strcpy(Context, "/Mail/Header");
						goto doHeader;
					}
					else
					{
						ReplyLen = SetupNodeMenu(Reply, sizeof(Reply), LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					RLen = ReplyLen;

					goto Returnit;

				}

				ReplyLen = sprintf(Reply, MailSignon, Mycall, Mycall);

				RLen = ReplyLen;
				goto Returnit;
			}

			Session = FindSession(Key);

	
			if (Session == NULL && _memicmp(Context, "/Mail/API/", 10) != 0)
			{
				ReplyLen = sprintf(Reply, MailLostSession, Key);
				RLen = ReplyLen;
				goto Returnit;
			}
		}

		if (_memicmp(Context, "/Chat/", 6) == 0)
		{
			int RLen = 0;
			char Appl;
			char * input;
			char * IContext;


			HostPtr = strstr(MsgPtr, "Host: ");

			if (HostPtr)
			{
				uint32_t Host;
				char Hostname[32]= "";
				struct LOCALNET * LocalNet = conn->TNC->TCPInfo->LocalNets;

				HostPtr += 6;
				memcpy(Hostname, HostPtr, 31);
				strlop(Hostname, ':');
				Host = inet_addr(Hostname);

				if (strcmp(Hostname, "127.0.0.1") == 0)
					LOCAL = TRUE;
				else while(LocalNet)
				{
					uint32_t MaskedHost = Host & LocalNet->Mask;
					if (MaskedHost == LocalNet->Network)
					{				
						char * rest;
						LOCAL = 1;
						rest = strchr(HostPtr, 13);
						if (rest)
						{
							memmove(HostPtr + 9, rest, strlen(rest) + 1);
							memcpy(HostPtr, "127.0.0.1", 9);
							break;
						}
					}
					LocalNet = LocalNet->Next;
				}
			}

			NodeURL = strtok_s(Context, "?", &IContext);
			Key = strtok_s(NULL, "?", &IContext);

			if (_stricmp(NodeURL, "/Chat/Signon") == 0)
			{
				ReplyLen = ProcessChatSignon(TCP, MsgPtr, Key, Reply, sizeof(Reply), &Session, LOCAL);

				if (ReplyLen)
				{
					goto Returnit;
				}

#ifdef LINBPQ
				strcpy(Context, "/Chat/Header");
#else
				strcpy(MsgPtr, "POST /Chat/Header");
#endif
				goto doHeader;
			}

			if (_stricmp(NodeURL, "/Chat/Lost") == 0)
			{
				input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

				if (input && strstr(input, "Cancel=Exit"))
				{
					ReplyLen = sprintf(Reply, "<!DOCTYPE html><html><head><title>Chat Redirect</title></head><body><script>window.location.href='/Node/NodeIndex.html';</script>");
					RLen = ReplyLen;
					goto Returnit;
				}
				if (Key)
					Appl = Key[0];

				Key = 0;
			}

			if (Key == 0 || Key[0] == 0)
			{
				// No Session 

				// if not local send a signon screen, else create a user session

				if (LOCAL || COOKIE)
				{
					Session = AllocateSession(sock, 'C');

					if (Session)
					{
						strcpy(Context, "/Chat/Header");
						goto doHeader;
					}
					else
					{
						ReplyLen = SetupNodeMenu(Reply, sizeof(Reply), LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					RLen = ReplyLen;

					goto Returnit;

				}

				ReplyLen = sprintf(Reply, ChatSignon, Mycall, Mycall);

				RLen = ReplyLen;
				goto Returnit;
			}

			Session = FindSession(Key);

			if (Session == NULL)
			{
				int Sent, Loops = 0;
				ReplyLen = sprintf(Reply, MailLostSession, Key);
				RLen = ReplyLen;
Returnit:
				if (memcmp(Reply, "HTTP", 4) == 0)
				{
					// Full Header provided by appl - just send it

					// Send may block

					Sent = send(sock, Reply, ReplyLen, 0);

					while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
					{	
						//			Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

						if (Sent > 0)					// something sent
						{
							InputLen -= Sent;
							memmove(Reply, &Reply[Sent], ReplyLen);
						}

						Sleep(30);
						Sent = send(sock, Reply, ReplyLen, 0);
					}

					return 0;
				}

				if (NodeURL && _memicmp(NodeURL, "/mail/api/", 10) != 0)
				{
					// Add tail

					strcpy(&Reply[ReplyLen], Tail);
					ReplyLen += strlen(Tail);
				}

				// compress if allowed
				
				if (allowDeflate)
					Compressed = Compressit(Reply, ReplyLen, &ReplyLen);
				else
					Compressed = Reply;

				if (NodeURL && _memicmp(NodeURL, "/mail/api/", 10) == 0)
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/json\r\nConnection: close\r\n%s\r\n", ReplyLen, Encoding);
				else
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "%s\r\n", ReplyLen, Encoding);
				
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, Compressed, ReplyLen);

				if (allowDeflate)
					free (Compressed);

				return 0;
			}
		}

doHeader:

#ifdef LINBPQ

		if ((_memicmp(Context, "/MAIL/", 6) == 0) || (_memicmp(Context, "/WebMail", 8) == 0))
		{
			char _REPLYBUFFER[300000];
			struct HTTPConnectionInfo Dummy = {0};
			int Sent, Loops = 0;
			char token[16] = "";

			// look for auth header	

			const char * auth_header = "Authorization: Bearer ";
			char * token_begin = strstr(MsgPtr, auth_header);
			int Flags = 0, n;

			char * Tok;
			char * param;

			if (token_begin)
			{
				// Using Auth Header

				// Extract the token from the request (assuming it's present in the request headers)

				token_begin += strlen(auth_header); // Move to the beginning of the token
				strncpy(token, token_begin, 13);
				token[13] = '\0'; // Null-terminate the token
			}

			ReplyLen = 0;

			if (Session == 0)
				Session = &Dummy;

			if (LOCAL)
				Session->TNC = (void *)1;		// TNC only used for Web Terminal Sessions
			else
				Session->TNC = (void *)0;

			ProcessMailHTTPMessage(Session, Method, Context, MsgPtr, _REPLYBUFFER, &ReplyLen, MsgLen, token);

			if (Context && _memicmp(Context, "/mail/api/", 10) == 0)
			{
				if (memcmp(_REPLYBUFFER, "HTTP", 4) == 0)
				{
					// Full Header provided by appl - just send it

					// Send may block

					Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);

					while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
					{	
						//				Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

						if (Sent > 0)					// something sent
						{
							InputLen -= Sent;
							memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], ReplyLen);
						}	

						Sleep(30);
						Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);
					}
					return 0;
				}

				// compress if allowed

				if (allowDeflate)
					Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
				else
					Compressed = _REPLYBUFFER;

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/json\r\nConnection: close\r\n%s\r\n", ReplyLen, Encoding);
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, Compressed, ReplyLen);

				if (allowDeflate)
					free (Compressed);

				return 0;
			}

			if (memcmp(_REPLYBUFFER, "HTTP", 4) == 0)
			{
				// Full Header provided by appl - just send it

				// Send may block

				Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);

				while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
				{	
					//				Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

					if (Sent > 0)					// something sent
					{
						InputLen -= Sent;
						memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], ReplyLen);
					}	

					Sleep(30);
					Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);
				}
				return 0;
			}

			if (Context && _memicmp(Context, "/mail/api/", 10) != 0)
			{

			// Add tail

			strcpy(&_REPLYBUFFER[ReplyLen], Tail);
			ReplyLen += strlen(Tail);

			}

			// compress if allowed
				
			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = Reply;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;


/*

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
			send(sock, Header, HeaderLen, 0);

			// Send may block

			Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);

			if (Sent == -1)
				return 0;

			while (Sent != ReplyLen && Loops++ < 3000)					// 100 secs max
			{	
				//			Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

				if (Sent > 0)					// something sent
				{
					InputLen -= Sent;
					memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], ReplyLen);
				}

				Sleep(30);
				Sent = send(sock, _REPLYBUFFER, ReplyLen, 0);
			}

			send(sock, Tail, (int)strlen(Tail), 0);
			return 0;
*/
		}

		if (_memicmp(Context, "/CHAT/", 6) == 0)
		{
			char _REPLYBUFFER[100000];

			ReplyLen = 0;

			ProcessChatHTTPMessage(Session, Method, Context, MsgPtr, _REPLYBUFFER, &ReplyLen);

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
			send(sock, Header, HeaderLen, 0);
			send(sock, _REPLYBUFFER, ReplyLen, 0);
			send(sock, Tail, (int)strlen(Tail), 0);

			return 0;

		}


		/*
		Sent = send(sock, _REPLYBUFFER, InputLen, 0);

		while (Sent != InputLen && Loops++ < 3000)					// 100 secs max
		{	
		//					Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

		if (Sent > 0)					// something sent
		{
		InputLen -= Sent;
		memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], InputLen);
		}

		Sleep(30);
		Sent = send(sock, _REPLYBUFFER, InputLen, 0);
		}
		return 0;
		}
		*/
#else

		// Pass to MailChat if active

		NodeURL = Context;

		if ((_memicmp(Context, "/MAIL/", 6) == 0) || (_memicmp(Context, "/WebMail", 8) == 0))
		{
			// If for Mail, Pass to Mail Server via Named Pipe

			HANDLE hPipe;

			hPipe = CreateFile(MAILPipeFileName, GENERIC_READ | GENERIC_WRITE,
				0,                    // exclusive access
				NULL,                 // no security attrs
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 
				NULL );

			if (hPipe == (HANDLE)-1)
			{
				InputLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 28\r\n\r\nMail Data is not available\r\n");
				send(sock, Reply, InputLen, 0);
			}
			else
			{
				//			int Sent;
				int Loops = 0;
				struct HTTPConnectionInfo Dummy = {0};

				if (Session == 0)
					Session = &Dummy;

				if (LOCAL)
					Session->TNC = (struct TNCINFO *)(uintptr_t)1;		// TNC is only used on Web Terminal Sessions so can reuse as LOCAL flag
				else
					Session->TNC = 0;

				WriteFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				WriteFile(hPipe, MsgPtr, MsgLen, &InputLen, NULL);


				ReadFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				ReadFile(hPipe, Reply, 250000, &ReplyLen, NULL);
				if (ReplyLen <= 0)
				{
					InputLen = GetLastError();
				}

				CloseHandle(hPipe);
				goto Returnit;
			}
			return 0;
		}

		if (_memicmp(Context, "/CHAT/", 6) == 0)
		{
			HANDLE hPipe;

			hPipe = CreateFile(CHATPipeFileName, GENERIC_READ | GENERIC_WRITE,
				0,                    // exclusive access
				NULL,                 // no security attrs
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 
				NULL );

			if (hPipe == (HANDLE)-1)
			{
				InputLen = sprintf(Reply, "HTTP/1.1 404 Not Found\r\nContent-Length: 28\r\n\r\nChat Data is not available\r\n");
				send(sock, Reply, InputLen, 0);
			}
			else
			{
				//			int Sent;
				int Loops = 0;

				WriteFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				WriteFile(hPipe, MsgPtr, MsgLen, &InputLen, NULL);


				ReadFile(hPipe, Session, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
				ReadFile(hPipe, Reply, 100000, &ReplyLen, NULL);
				if (ReplyLen <= 0)
				{
					InputLen = GetLastError();
				}

				CloseHandle(hPipe);
				goto Returnit;
			}
			return 0;
		}

#endif

		NodeURL = strtok_s(NULL, "?", &Context);

		if (NodeURL == NULL)
			return 0;

		if (strcmp(Method, "POST") == 0)
		{
			if (_stricmp(NodeURL, "/Node/freqOffset") == 0)
			{
				char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
				int port = atoi(Context);

				if (input == 0)
					return 1;

				input += 4;
	
				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];
					char value[6];

					if (TNC == 0)
						return 1;

					TNC->TXOffset = atoi(input);
#ifdef WIN32
					sprintf(value, "%d", TNC->TXOffset);
					MySetWindowText(TNC->xIDC_TXTUNEVAL, value);
					SendMessage(TNC->xIDC_TXTUNE, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) TNC->TXOffset);  // min. & max. positions

#endif
				}
				return 1;
			}

			if (_stricmp(NodeURL, "/Node/PortAction") == 0)
			{
				char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
				int port = atoi(Context);

				if (input == 0)
					return 1;

				input += 4;

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					if (TNC == 0)
						return 1;

					if (LOCAL == FALSE && COOKIE == FALSE)
						return 1;

					if (strcmp(input, "Abort") == 0)
					{
						if (TNC->ForcedCloseProc)
							TNC->ForcedCloseProc(TNC, 0);
					}
					else if (strcmp(input, "Kill") == 0)
					{
						TNC->DontRestart = TRUE;
						KillTNC(TNC);
					}
					else if (strcmp(input, "KillRestart") == 0)
					{
						TNC->DontRestart = FALSE;
						KillTNC(TNC);
						RestartTNC(TNC);

					}
				}
				return 1;
			}

			if (_stricmp(NodeURL, "/TermInput") == 0)
			{
				ProcessTermInput(sock, MsgPtr, MsgLen, Context);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/TermSignon") == 0)
			{
				ProcessTermSignon(conn->TNC, sock, MsgPtr, MsgLen, LOCAL);
			}

			if (_stricmp(NodeURL, "/Node/Signon") == 0)
			{
				ProcessNodeSignon(sock, TCP, MsgPtr, Key, Reply, sizeof(Reply), &Session, LOCAL);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/TermClose") == 0)
			{
				ProcessTermClose(sock, MsgPtr, MsgLen, Context, LOCAL);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/BeaconAction") == 0)
			{
				char Header[1024];
				int HeaderLen;
				char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
				int Port;
				char Param[100];
#ifndef LINBPQ
				int retCode, disp;
				char Key[80];
				HKEY hKey;
#endif
				struct PORTCONTROL * PORT;
				int Slot = 0;


				if (LOCAL == FALSE && COOKIE == FALSE)
				{
					//	Send Not Authorized

					ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);	
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<br><B>Not authorized - please sign in</B>");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					return 1;
				}

				if (strstr(input, "Cancel=Cancel"))
				{
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);	
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					return 1;
				}

				GetParam(input, "Port=", &Param[0]);
				Port = atoi(&Param[0]);
				PORT = GetPortTableEntryFromPortNum(Port); // Need slot not number
				if (PORT)
					Slot = PORT->PortSlot;

				GetParam(input, "Every=", &Param[0]);
				Interval[Slot] = atoi(&Param[0]);

				GetParam(input, "Dest=", &Param[0]);
				_strupr(Param);
				strcpy(UIUIDEST[Slot], &Param[0]);

				GetParam(input, "Path=", &Param[0]);
				_strupr(Param);
				if (UIUIDigi[Slot])
					free(UIUIDigi[Slot]);
				UIUIDigi[Slot] = _strdup(&Param[0]);

				GetParam(input, "File=", &Param[0]);
				strcpy(FN[Slot], &Param[0]);
				GetParam(input, "Text=", &Param[0]);
				strcpy(Message[Slot], &Param[0]);

				MinCounter[Slot] = Interval[Slot];

				SendFromFile[Slot] = 0;

				if (FN[Slot][0])
					SendFromFile[Slot] = 1;

				SetupUI(Slot);

#ifdef LINBPQ
				SaveUIConfig();
#else
				SaveUIConfig();

				wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", Port);

				retCode = RegCreateKeyEx(REGTREE,
					Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

				if (retCode == ERROR_SUCCESS)
				{
					retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIUIDEST[Port][0], strlen(&UIUIDEST[Port][0]));
					retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN[Port][0], strlen(&FN[Port][0]));
					retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message[Port][0], strlen(&Message[Port][0]));
					retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval[Port], 4);
					retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile[Port], 4);
					retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ, UIUIDigi[Port], strlen(UIUIDigi[Port]));

					RegCloseKey(hKey);
				}
#endif
				if (strstr(input, "Test=Test"))
					SendUIBeacon(Slot);


				ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);	
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], Beacons, Port,		
					Interval[Slot], &UIUIDEST[Slot][0], &UIUIDigi[Slot][0], &FN[Slot][0], &Message[Slot][0], Port);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);
				send(sock, Tail, (int)strlen(Tail), 0);

				return 1;
			}

			if (_stricmp(NodeURL, "/Node/CfgSave") == 0)
			{
				//	Save Config File

				SaveConfigFile(sock, MsgPtr, Key, LOCAL);
				return 0;
			}

			if (_stricmp(NodeURL, "/Node/LogAction") == 0)
			{
				ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);	
				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);
				send(sock, Tail, (int)strlen(Tail), 0);

				return 1;
			}


			if (_stricmp(NodeURL, "/Node/ARDOPAbort") == 0)
			{
				int port = atoi(Context);

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					if (TNC && TNC->ForcedCloseProc)
						TNC->ForcedCloseProc(TNC, 0);


					if (TNC && TNC->WebWindowProc)
						ReplyLen = TNC->WebWindowProc(TNC, _REPLYBUFFER, LOCAL);


					ReplyLen = sprintf(Reply, "<!DOCTYPE html><html><head><title>Operation Complete</title></head><body><script>alert(\"%s\");window.close();</script>", "Ok");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, Reply, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					//				goto SendResp;

					//				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + strlen(Tail));
					//				send(sock, Header, HeaderLen, 0);
					//				send(sock, _REPLYBUFFER, ReplyLen, 0);
					//				send(sock, Tail, strlen(Tail), 0);

					return 1;
				}

			}

			send(sock, _REPLYBUFFER, InputLen, 0);
			return 0;
		}

		if (_stricmp(NodeURL, "/") == 0 || _stricmp(NodeURL, "/Index.html") == 0 || _stricmp(NodeURL, "/Node/NodeIndex.html") == 0)
		{		
			char * IndexURL = NodeURL;
			char RootURL[] = "/";

			if (_stricmp(NodeURL, "/Node/NodeIndex.html") == 0)
				IndexURL = RootURL;

			// Send if present, else use default custom NodeIndex page

			Bufferlen = SendMessageFile(sock, IndexURL, TRUE, allowDeflate);		// return -1 if not found

			if (Bufferlen != -1)
				return 0;						// We've sent it
			else
			{
				ReplyLen = BuildNodeIndexPage(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen);
				send(sock, Header, HeaderLen, 0);
				send(sock, _REPLYBUFFER, ReplyLen, 0);

				return 0;
			}
		}

		else if (_stricmp(NodeURL, "/NodeMenu.html") == 0 || _stricmp(NodeURL, "/Node/NodeMenu.html") == 0)
		{		
			// Send if present, else use default

			char Menu[] = "/NodeMenu.html";

			Bufferlen = SendMessageFile(sock, Menu, TRUE, allowDeflate);		// return -1 if not found

			if (Bufferlen != -1)
				return 0;											// We've sent it
		}

		else if (_memicmp(NodeURL, "/aisdata.txt", 12) == 0)
		{
			char * Compressed;
			ReplyLen = GetAISPageInfo(_REPLYBUFFER, 1, 1);

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;
		}

		else if (_memicmp(NodeURL, "/aprsdata.txt", 13) == 0)
		{
			char * Compressed;
			char * ptr;
			double N, S, W, E;
			int aprs = 1, ais = 1, adsb = 1;

			ptr = &NodeURL[14];
			
			N = atof(ptr);
			ptr = strlop(ptr, '|');
			S = atof(ptr);
			ptr = strlop(ptr, '|');
			W = atof(ptr);
			ptr = strlop(ptr, '|');
			E = atof(ptr);
			ptr = strlop(ptr, '|');	
			if (ptr)
			{
			aprs = atoi(ptr);
			ptr = strlop(ptr, '|');		
			ais = atoi(ptr);
			ptr = strlop(ptr, '|');		
			adsb = atoi(ptr);
			}
			ReplyLen = GetAPRSPageInfo(_REPLYBUFFER, N, S, W, E, aprs, ais, adsb);

			if (ReplyLen < 240000)
				ReplyLen += GetAISPageInfo(&_REPLYBUFFER[ReplyLen], ais, adsb);

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;
		}

		else if (_memicmp(NodeURL, "/portstats.txt", 15) == 0)
		{
			char * Compressed;
			char * ptr;
			int port;
			struct PORTCONTROL * PORT;

			ptr = &NodeURL[15];
			
			port = atoi(ptr);

			PORT = GetPortTableEntryFromPortNum(port);

			ReplyLen = 0;

			if (PORT && PORT->TX)
			{
				// We send the last 24 hours worth of data. Buffer is cyclic so oldest byte is at StatsPointer

				int first = PORT->StatsPointer;
				int firstlen = 1440 - first;

				memcpy(&_REPLYBUFFER[0], &PORT->TX[first], firstlen);
				memcpy(&_REPLYBUFFER[firstlen], PORT->TX, first);

				memcpy(&_REPLYBUFFER[1440], &PORT->BUSY[first], firstlen);
				memcpy(&_REPLYBUFFER[1440 + firstlen], PORT->BUSY, first);

				ReplyLen = 2880;
			}

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;
		}


		else if (_memicmp(NodeURL, "/Icon", 5) == 0 && _memicmp(&NodeURL[10], ".png", 4) == 0)
		{
			// APRS internal Icon

			char * Compressed;
				
			ReplyLen = GetAPRSIcon(_REPLYBUFFER, NodeURL);

			if (allowDeflate)
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			else
				Compressed = _REPLYBUFFER;

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: Text\r\n%s\r\n", ReplyLen, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);

			return 0;

		}

		else if (_memicmp(NodeURL, "/NODE/", 6))
		{
			// Not Node, See if a local file

			Bufferlen = SendMessageFile(sock, NodeURL, FALSE, allowDeflate);		// Send error if not found
			return 0;
		}

		// Node URL

		{

			ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);

			if (_stricmp(NodeURL, "/Node/webproc.css") == 0)
			{
				char WebprocCSS[] = COMMON_NODE_WEBPROC_DROPDOWN_CSS;
				ReplyLen = sprintf(_REPLYBUFFER, "%s", WebprocCSS);
			}

			else if (_stricmp(NodeURL, "/Node/Killandrestart") == 0)
			{
				int port = atoi(Context);

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					KillTNC(TNC);
					TNC->DontRestart = FALSE;
					RestartTNC(TNC);

					if (TNC && TNC->WebWindowProc)
						ReplyLen = TNC->WebWindowProc(TNC, _REPLYBUFFER, LOCAL);

				}
			}

			else if (_stricmp(NodeURL, "/Node/Port") == 0 || _stricmp(NodeURL, "/Node/ARDOPAbort") == 0)
			{
				int port = atoi(Context);

				if (port > 0 && port <= MaxBPQPortNo)
				{
					struct TNCINFO * TNC = TNCInfo[port];

					if (TNC && TNC->WebWindowProc)
						ReplyLen = TNC->WebWindowProc(TNC, _REPLYBUFFER, LOCAL);
				}

			}

			else if (_stricmp(NodeURL, "/Node/Streams") == 0)
			{
				ReplyLen = StatusProc(_REPLYBUFFER);
			}

			else if (_stricmp(NodeURL, "/Node/Stats.html") == 0)
			{
				struct tm * TM;
				char UPTime[50];
				char Value[128];
				time_t szClock = STATSTIME * 60;

				TM = gmtime(&szClock);

				sprintf(UPTime, "%.2d:%.2d:%.2d", TM->tm_yday, TM->tm_hour, TM->tm_min);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", StatsHddr);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "Version", VersionString);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "Uptime (Days Hours Mins)", UPTime);

				snprintf(Value, sizeof(Value), "%d / %d", Semaphore.Gets - Semaphore.Rels, Semaphore.Clashes);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "Semaphore: Get-Rel/Clashes", Value);

				snprintf(Value, sizeof(Value), "%d / %d / %d / %d / %d", MAXBUFFS, QCOUNT, MINBUFFCOUNT, NOBUFFCOUNT, BUFFERWAITS);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "Buffers: Max/Cur/Min/Out/Wait", Value);

				snprintf(Value, sizeof(Value), "%d / %d", NUMBEROFNODES, MAXDESTS);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "Known Nodes/Max Nodes", Value);

				snprintf(Value, sizeof(Value), "%d / %d", L4CONNECTSOUT, L4CONNECTSIN);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "L4 Connects Sent/Rxed", Value);

				snprintf(Value, sizeof(Value), "%d / %d / %d / %d", L4FRAMESTX, L4FRAMESRX, L4FRAMESRETRIED, OLDFRAMES);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "L4 Frames TX/RX/Resent/Reseq", Value);

				snprintf(Value, sizeof(Value), "%d", L3FRAMES);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], StatsLine, "L3 Frames Relayed", Value);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");

			}

			else if (_stricmp(NodeURL, "/Node/RigControl.html") == 0)
			{
				char Test[] =
					COMMON_HTML_HEAD_COMMON
					COMMON_HTML_META_UTF8
					COMMON_HTML_META_VIEWPORT_COMPACT
					"<meta http-equiv=expires content=0>"
					"<title>Rigcontrol</title>"
					"<script type=\"text/javascript\">\r\n"
					"var ws;"
					"function WebSocketTest()"
					"{"
					" if (\"WebSocket\" in window)"
					" {"
					"   // Build ws/wss URL from current page location\r\n"
					"\tvar wsProto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';"
					"   ws = new WebSocket(wsProto + '//' + window.location.host + '/RIGCTL');\r\n"
					
					"   ws.onopen = function() {\r\n"
			
					"   // Web Socket is connected\r\n"
					"	const div = document.getElementById('div');\r\n"
					"	div.innerHTML = 'Websock Connected'\r\n"
					"    };\r\n"
					
					"   ws.onmessage = function (evt)"
					"   {"
					"     var received_msg = evt.data;\r\n"
					"	  const div = document.getElementById('div');\r\n"
					"	  div.innerHTML = received_msg\r\n"
					"     };"

					"   ws.onclose = function()"
					"   {"

					"    // websocket is closed.\r\n"
					"	 const div = document.getElementById('div');\r\n"
					"	 div.innerHTML = 'Websock Connection Lost'\r\n"
					"    };"
					"   ws.onerror = function()"
					"   {"
					"\t const div = document.getElementById('div');\r\n"
					"\t div.innerHTML = 'Websock Connection Failed'\r\n"
					"    };"
					" }"
					" else"
					" {"
					"  // The browser doesn't support WebSocket\r\n"
					"	const div = document.getElementById('div');\r\n"
					"	div.innerHTML = 'WebSocket not supported by your Browser - RigControl Page not availible'\r\n"
					" }"
					"}"
					"function PTT(p)"
					"{"
					"  ws.send(p);"
					"}" 
					"</script>\r\n"
					"</head>\r\n"
					"<body onload=WebSocketTest()>\r\n"
					"<div id='div'>Waiting for data...</div>\r\n"
					"</body></html>\r\n";

		
				char NoRigCtl[] =
					COMMON_HTML_HEAD_COMMON
					COMMON_HTML_META_UTF8
					COMMON_HTML_META_VIEWPORT_COMPACT
					"<meta http-equiv=expires content=0>"
					"<title>Rigcontrol</title>"
					"</head>\r\n"
					"<body>"
					"<div id='div'>RigControl Not Configured...</div>\r\n"
					"</body></html>\r\n";

				if (RigWebPage)
					ReplyLen = sprintf(_REPLYBUFFER, "%s", Test);
				else
					ReplyLen = sprintf(_REPLYBUFFER, "%s", NoRigCtl);
			}

			else if (_stricmp(NodeURL, "/Node/ShowLog.html") == 0)
			{
				char ShowLogPage[] =
					COMMON_HTML_HEAD_UTF8_VIEWPORT COMMON_BPQ_CSS_LINK "<style>" COMMON_MONO_PAGE_CLAMP_MARGIN4_CSS COMMON_BTN_PANEL_BASE_CSS "#log { " COMMON_MONO_COMPACT_TEXT_CSS " }</style>"
					"<title>Log Display</title>"
					"<script>"
					"function myResize() {"
					" var h = document.getElementById('outer').clientHeight;"
					" var offsets = document.getElementById('log').getBoundingClientRect();"
					" document.getElementById('log').style.height = h - offsets.top;}"
					"</script>"
					"</head>"
					"<body onload='myResize()' onresize='myResize()'>"
					"<div id=outer style=\"width: 100%%; height: 100%%;\">"
					"<form id = form><input name=input value=Back type=submit class='btn'>"
//					"<form id = doDate><input type=date value=Date name='date'><input type='submit'>"
					"</form>"
					"<textarea id=log style=\"box-sizing: border-box; overflow: auto; white-space: pre; width: 100%%; height: auto\" name=Msg>%s</textarea>"
					"</div>";

				char * _REPLYBUFFER;
				int ReplyLen;
				char Header[1024];
				int HeaderLen;
				char * CfgBytes;
				int CfgLen;
				char inputname[250];
				FILE *fp1;
				struct stat STAT;
				char DummyKey[] = "DummyKey";
				time_t T;
				struct tm * tm;
				char Name[64] = "";

				T = time(NULL);
				tm = gmtime(&T);

				if (LOCAL == FALSE && COOKIE == FALSE)
				{
					//	Send Not Authorized

					char _REPLYBUFFER[250000];	
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);	
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<br><B>Not authorized - please sign in</B>");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);
					return 1;
				}

				if (COOKIE == FALSE)
					Key = DummyKey;

				if (memcmp(Context, "date=", 5) == 0)
				{
					memset(tm, 0, sizeof(struct tm));
					tm->tm_year = atoi(&Context[5]) - 1900;
					tm->tm_mon = atoi(&Context[10]) - 1;
					tm->tm_mday = atoi(&Context[13]);
				}



				if (strcmp(Context, "input=Back") == 0)
				{
					ReplyLen = SetupNodeMenu(Reply, sizeof(Reply), LOCAL);
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, Reply, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);
					return 1;
				}

				if (LogDirectory[0] == 0)
				{
					strcpy(inputname, "logs/");
				}
				else
				{
					strcpy(inputname,LogDirectory);
					strcat(inputname,"/");
					strcat(inputname, "/logs/");
				}

				if (strstr(Context, "CMS"))
				{
					sprintf(Name, "CMSAccess_%04d%02d%02d.log",
						tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "Debug"))
				{
					sprintf(Name, "log_%02d%02d%02d_DEBUG.txt",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "BBS"))
				{
					sprintf(Name, "log_%02d%02d%02d_BBS.txt",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "Chat"))
				{
					sprintf(Name, "log_%02d%02d%02d_CHAT.txt",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}
				else if (strstr(Context, "Telnet"))
				{
					sprintf(Name, "Telnet_%02d%02d%02d.log",
						tm->tm_year - 100, tm->tm_mon+1, tm->tm_mday);
				}

				strcat(inputname, Name);

				if (stat(inputname, &STAT) == -1)
				{
					CfgBytes = malloc(256);
					sprintf(CfgBytes, "Log %s not found", inputname);
					CfgLen = strlen(CfgBytes);
				}
				else
				{
					fp1 = fopen(inputname, "rb");

					if (fp1 == 0)
					{
						CfgBytes = malloc(256);
						sprintf(CfgBytes, "Log %s not found", inputname);
						CfgLen = strlen(CfgBytes);
					}
					else
					{	
						CfgLen = STAT.st_size;

						CfgBytes = malloc(CfgLen + 1);

						CfgLen = (int)fread(CfgBytes, 1, CfgLen, fp1);
						CfgBytes[CfgLen] = 0;
					}		
				}

				_REPLYBUFFER = malloc(CfgLen + 1000);

				ReplyLen = sprintf(_REPLYBUFFER, ShowLogPage, CfgBytes);
				free (CfgBytes);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, _REPLYBUFFER, ReplyLen);
				sendandcheck(sock, Tail, (int)strlen(Tail));
				free (_REPLYBUFFER);

				return 1;
			}

			else if (_stricmp(NodeURL, "/Node/EditCfg.html") == 0)
			{
				char * _REPLYBUFFER;
				int ReplyLen;
				char Header[1024];
				int HeaderLen;
				char * CfgBytes;
				int CfgLen;
				char inputname[250]="bpq32.cfg";
				FILE *fp1;
				struct stat STAT;
				char DummyKey[] = "DummyKey";

				if (LOCAL == FALSE && COOKIE == FALSE)
				{
					//	Send Not Authorized

					char _REPLYBUFFER[250000];	
					ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);	
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<br><B>Not authorized - please sign in</B>");
					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
					send(sock, Header, HeaderLen, 0);
					send(sock, _REPLYBUFFER, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);
					return 1;
				}

				if (COOKIE ==FALSE)
					Key = DummyKey;

				if (ConfigDirectory[0] == 0)
				{
					strcpy(inputname, "bpq32.cfg");
				}
				else
				{
					strcpy(inputname,ConfigDirectory);
					strcat(inputname,"/");
					strcat(inputname, "bpq32.cfg");
				}


				if (stat(inputname, &STAT) == -1)
				{
					CfgBytes = _strdup("Config File not found");
				}
				else
				{
					fp1 = fopen(inputname, "rb");

					if (fp1 == 0)
					{
						CfgBytes = _strdup("Config File not found");
					}
					else
					{	
						CfgLen = STAT.st_size;

						CfgBytes = malloc(CfgLen + 1);

						CfgLen = (int)fread(CfgBytes, 1, CfgLen, fp1);
						CfgBytes[CfgLen] = 0;
					}		
				}

				_REPLYBUFFER = malloc(CfgLen + 1000);

				ReplyLen = sprintf(_REPLYBUFFER, ConfigEditPage, Key, CfgBytes);
				free (CfgBytes);

				HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", ReplyLen + (int)strlen(Tail));
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, _REPLYBUFFER, ReplyLen);
				sendandcheck(sock, Tail, (int)strlen(Tail));
				free (_REPLYBUFFER);

				return 1;
			}



			if (_stricmp(NodeURL, "/Node/PortBeacons") == 0)
			{
				char * PortChar = strtok_s(NULL, "&", &Context);
				int PortNo = atoi(PortChar);
				struct PORTCONTROL * PORT;
				int PortSlot = 0;

				PORT = GetPortTableEntryFromPortNum(PortNo); // Need slot not number
				if (PORT)
					PortSlot = PORT->PortSlot;


				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], Beacons, PortNo,
					Interval[PortSlot], &UIUIDEST[PortSlot][0], &UIUIDigi[PortSlot][0], &FN[PortSlot][0], &Message[PortSlot][0], PortNo);
			}



			if (_stricmp(NodeURL, "/Node/PortStats") == 0)
			{
				struct _EXTPORTDATA * Port;

				char * PortChar = strtok_s(NULL, "&", &Context);
				int PortNo = atoi(PortChar);
				int Protocol;
				int PortType;

				//		char PORTTYPE;	// H/W TYPE
				// 0 = ASYNC, 2 = PC120, 4 = DRSI
				// 6 = TOSH, 8 = QUAD, 10 = RLC100
				// 12 = RLC400 14 = INTERNAL 16 = EXTERNAL

#define KISS 0
#define NETROM 2
#define HDLC 6
#define L2 8
#define WINMOR 10


				// char PROTOCOL;	// PORT PROTOCOL
				// 0 = KISS, 2 = NETROM, 4 = BPQKISS
				//; 6 = HDLC, 8 = L2


				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsHddr, PortNo);

				Port = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(PortNo);

				if (Port == NULL)
				{
					ReplyLen = sprintf(_REPLYBUFFER, "Invalid Port");
					goto SendResp;
				}

				Protocol = Port->PORTCONTROL.PROTOCOL;
				PortType = Port->PORTCONTROL.PROTOCOL;

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Digied", Port->PORTCONTROL.L2DIGIED);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Heard", Port->PORTCONTROL.L2FRAMES);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Received", Port->PORTCONTROL.L2FRAMESFORUS);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Frames Sent", Port->PORTCONTROL.L2FRAMESSENT);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Timeouts", Port->PORTCONTROL.L2TIMEOUTS);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "REJ Frames Received", Port->PORTCONTROL.L2REJCOUNT);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX out of Seq", Port->PORTCONTROL.L2OUTOFSEQ);
				//		ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "L2 Resequenced", Port->PORTCONTROL.L2RESEQ);
				if (Protocol == HDLC)
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "Underrun", Port->PORTCONTROL.L2URUNC);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX Overruns", Port->PORTCONTROL.L2ORUNC);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX CRC Errors", Port->PORTCONTROL.RXERRORS);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "Frames abandoned", Port->PORTCONTROL.L1DISCARD);
				}
				else if ((Protocol == KISS && Port->PORTCONTROL.KISSFLAGS) || Protocol == NETROM)
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "Poll Timeout", Port->PORTCONTROL.L2URUNC);
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "RX CRC Errors", Port->PORTCONTROL.RXERRORS);
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "FRMRs Sent", Port->PORTCONTROL.L2FRMRTX);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortStatsLine, "FRMRs Received", Port->PORTCONTROL.L2FRMRRX);
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");

				//		DB	'Link Active %   '
				//		DD	AVSENDING

			}

			if (_stricmp(NodeURL, "/Node/Ports.html") == 0)
			{
				struct _EXTPORTDATA * ExtPort;
				struct PORTCONTROL * Port;

				int count;
				char DLL[20];
				char StatsURL[64];

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", PortsHddr);

				for (count = 1; count <= NUMBEROFPORTS; count++)
				{
					Port = GetPortTableEntryFromSlot(count);
					ExtPort = (struct _EXTPORTDATA *)Port;

					// see if has a stats page

					if (Port->AVACTIVE)
						sprintf(StatsURL, "<a href=/PortStats.html?%d>&nbsp;Stats Graph</a>", Port->PORTNUMBER);
					else
						StatsURL[0] = 0;

					if (Port->PORTTYPE == 0x10)
					{	
						strcpy(DLL, ExtPort->PORT_DLL_NAME);
						strlop(DLL, '.');
					}
					else if (Port->PORTTYPE == 0)
						strcpy(DLL, "ASYNC");

					else if (Port->PORTTYPE == 22)
						strcpy(DLL, "I2C");

					else if (Port->PORTTYPE == 14)
						strcpy(DLL, "INTERNAL");

					else if (Port->PORTTYPE > 0 && Port->PORTTYPE < 14)
						strcpy(DLL, "HDLC");


					if (Port->TNC && Port->TNC->WebWindowProc)		// Has a Window
					{
						if (Port->UICAPABLE)
							ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortLineWithBeaconAndDriver, Port->PORTNUMBER, DLL,
							Port->PORTDESCRIPTION, Port->PORTNUMBER, Port->PORTNUMBER, Port->TNC->WebWinX, Port->TNC->WebWinY, 200, 200, StatsURL);
						else
							ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortLineWithDriver, Port->PORTNUMBER, DLL,
							Port->PORTDESCRIPTION, Port->PORTNUMBER, Port->TNC->WebWinX, Port->TNC->WebWinY, 200, 200, StatsURL);

						continue;
					}

					if (Port->PORTTYPE == 16 && Port->PROTOCOL == 10 && Port->UICAPABLE == 0)		// EXTERNAL, Pactor/WINMO
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], SessionPortLine, Port->PORTNUMBER, DLL,
						Port->PORTDESCRIPTION, Port->PORTNUMBER, StatsURL);
					else
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], PortLineWithBeacon, Port->PORTNUMBER, Port->PORTNUMBER,
						DLL, DLL, Port->PORTDESCRIPTION, Port->PORTNUMBER, StatsURL);
				}

				if (RigActive)
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], RigControlLine, 64, "Rig Control", "Rig Control", 600, 350, 200, 200);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");

			}

			if (_stricmp(NodeURL, "/Node/Nodes.html") == 0)
			{
				struct DEST_LIST * Dests = DESTS;
				int count, i;
				char Normcall[10];
				char Alias[10];
				int n = 0;
				struct DEST_LIST * List[1000];
				char Param = 0;

				if (Context)
				{
					_strupr(Context);
					Param = Context[0];
				}

				for (count = 0; count < MAXDESTS; count++)
				{
					if (Dests->DEST_CALL[0] != 0)
					{
						if (Param != 'T' || Dests->DEST_COUNT)
							List[n++] = Dests;

						if (n > 999)
							break;
					}

					Dests++;
				}

				if (n > 1)
				{
					if (Param == 'C') 
						qsort(List, n, sizeof(void *), CompareNode);
					else
						qsort(List, n, sizeof(void *), CompareAlias);
				}

				Alias[6] = 0; 

				if (Param == 'T')
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeHddr, "with traffic");
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", NodeHeaderTraffic);
				}
				else if (Param == 'C') 
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeHddr, "sorted by Call");
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", "<div class='node-grid'>");
				}
				else
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeHddr, "sorted by Alias");
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", "<div class='node-grid'>");
				}

				for (i = 0; i < n; i++)
				{
					int len = ConvFromAX25(List[i]->DEST_CALL, Normcall);
					Normcall[len]=0;

					memcpy(Alias, List[i]->DEST_ALIAS, 6);
					strlop(Alias, ' ');

					if (Param == 'T')
					{
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeTrafficLine,
							Normcall, Normcall, Alias, List[i]->DEST_COUNT, List[i]->DEST_RTT /16,
							(List[i]->DEST_STATE & 0x40)? 'B':' ', (List[i]->DEST_STATE & 63));

					}
					else
					{
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], NodeLine, Normcall, Alias, Normcall);
					}
				}

				if (Param == 'T')
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");
				else
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</div>");
			}

			if (_stricmp(NodeURL, "/Node/NodeDetail") == 0)
			{
				UCHAR AXCall[8];
				struct DEST_LIST * Dest = DESTS;
				struct NR_DEST_ROUTE_ENTRY * NRRoute;
				struct ROUTE * Neighbour;
				char Normcall[10];
				int i, len, count, Active;
				char Alias[7];

				Alias[6] = 0;

				_strupr(Context);

				ConvToAX25(Context, AXCall);

				for (count = 0; count < MAXDESTS; count++)
				{
					if (CompareCalls(Dest->DEST_CALL, AXCall))
					{
						break;
					}
					Dest++;
				}

				if (count == MAXDESTS)
				{
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<h3 style=\"text-align:center;font-size:clamp(1.0625rem,0.95rem + 0.6vw,1.25rem);\">Call %s not found</h3>", Context);
					goto SendResp;
				}

				memcpy(Alias, Dest->DEST_ALIAS, 6);
				strlop(Alias, ' ');

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen],
					HTTP_NODE_H3("Info for Node %s:%s"), Alias, Context);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", HTTP_NODE_TABLE_OPEN_STACK_CLASS("node-detail-table") HTTP_NODE_TABLE_HEADER_ROW "<th scope=col>Frames</th><th scope=col>RTT</th><th scope=col>BPQ?</th><th scope=col>Hops</th></tr>" HTTP_NODE_TABLE_MID_STACK);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td data-label='Frames' class='num'>%d</td><td data-label='RTT' class='num'>%d</td><td data-label='BPQ?' class='center'>%c</td><td data-label='Hops' class='num'>%.0d</td></tr></tbody></table></div>",
					Dest->DEST_COUNT, Dest->DEST_RTT /16,
					(Dest->DEST_STATE & 0x40)? 'B':' ', (Dest->DEST_STATE & 63));

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", HTTP_NODE_H3("Neighbours"));

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen],
					"%s", HTTP_NODE_TABLE_OPEN_STACK_CLASS("node-detail-table") HTTP_NODE_TABLE_HEADER_ROW "<th scope=col>Active</th><th scope=col>Qual</th><th scope=col>Obs</th><th scope=col>Port</th><th scope=col>Call</th></tr>" HTTP_NODE_TABLE_MID_STACK);

				NRRoute = &Dest->NRROUTE[0];

				Active = Dest->DEST_ROUTE;

				for (i = 1; i < 4; i++)
				{
					Neighbour = NRRoute->ROUT_NEIGHBOUR;

					if (Neighbour)
					{
						len = ConvFromAX25(Neighbour->NEIGHBOUR_CALL, Normcall);
						Normcall[len] = 0;

						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "<tr><td data-label='Active' class='center'>%c</td><td data-label='Qual' class='num'>%d</td><td data-label='Obs' class='num'>%d</td><td data-label='Port' class='num'>%d</td><td data-label='Call' class='text'>%s</td></tr>",
							(Active == i)?'>':' ',NRRoute->ROUT_QUALITY, NRRoute->ROUT_OBSCOUNT, Neighbour->NEIGHBOUR_PORT, Normcall);
					}
					NRRoute++;
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");

				goto SendResp;

			}
			/*

			MOV	ESI,OFFSET32 NODEROUTEHDDR
			MOV	ECX,11
			REP MOVSB

			LEA	ESI,DEST_CALL[EBX]
			CALL	DECODENODENAME		; CONVERT TO ALIAS:CALL
			REP MOVSB

			CMP	DEST_RTT[EBX],0
			JE SHORT @f			; TIMER NOT SET - DEST PROBABLY NOT USED

			MOVSB				; ADD SPACE
			CALL	DORTT

			@@:
			MOV	AL,CR
			STOSB

			MOV	ECX,3
			MOV	DH,DEST_ROUTE[EBX]	; CURRENT ACTIVE ROUTE
			MOV	DL,1

			push ebx

			PUBLIC	CMDN110
			CMDN110:

			MOV	ESI,ROUT1_NEIGHBOUR[EBX]
			CMP	ESI,0
			JE CMDN199


			MOV	AX,'  '
			CMP	DH,DL
			JNE SHORT CMDN112			; NOT CURRENT DEST
			MOV	AX,' >'

			CMDN112:

			STOSW

			PUSH	ECX

			MOV	AL,ROUT1_QUALITY[EBX]
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			MOV	AL,ROUT1_OBSCOUNT[EBX]
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			MOV	AL,NEIGHBOUR_PORT[ESI]	; GET PORT
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb


			PUSH	EDI
			CALL	CONVFROMAX25		; CONVERT TO CALL
			POP	EDI

			MOV	ESI,OFFSET32 NORMCALL
			REP MOVSB

			MOV	AL,CR
			STOSB

			ADD	EBX,ROUTEVECLEN
			INC	DL			; ROUTE NUMBER

			POP	ECX
			DEC	ECX
			JNZ	CMDN110

			PUBLIC	CMDN199
			CMDN199:

			POP	EBX

			; DISPLAY  INP3 ROUTES

			MOV	ECX,3
			MOV	DL,4

			PUBLIC	CMDNINP3
			CMDNINP3:

			MOV	ESI,INPROUT1_NEIGHBOUR[EBX]
			CMP	ESI,0
			JE CMDNINPEND

			MOV	AX,'  '
			CMP	DH,DL
			JNE SHORT @F			; NOT CURRENT DEST
			MOV	AX,' >'

			@@:

			STOSW

			PUSH	ECX

			MOV	AL, Hops1[EBX]
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			MOVZX	EAX, SRTT1[EBX]

			MOV	EDX,0
			MOV ECX, 100
			DIV ECX	
			CALL	CONV_5DIGITS
			MOV	AL,'.'
			STOSB
			MOV EAX, EDX
			CALL	PRINTNUM
			MOV	AL,'s'
			STOSB
			MOV	AL,' '
			STOSB

			MOV	AL,NEIGHBOUR_PORT[ESI]	; GET PORT
			CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

			mov	AL,' '
			stosb

			PUSH	EDI
			CALL	CONVFROMAX25		; CONVERT TO CALL
			POP	EDI

			MOV	ESI,OFFSET32 NORMCALL
			REP MOVSB


			MOV	AL,CR
			STOSB

			ADD	EBX,INPROUTEVECLEN
			INC	DL			; ROUTE NUMBER

			POP	ECX
			LOOP	CMDNINP3

			CMDNINPEND:

			ret

			*/

			// AXIP Partners

			if (_stricmp(NodeURL, "/Node/AXIP.html") == 0)
			{
				int i;
				char Normcall[10];
				int n = 0, nd = 0;
				struct arp_table_entry * List[1000];
				struct arp_table_entry * ListD[1000];

				struct AXIPPORTINFO * AXPORT = Portlist[0];
				struct PORTCONTROL * PORT = PORTTABLE;
				struct arp_table_entry * arp;
				time_t NOW = time(NULL);
				
				char AXIPHeader[] =
					HTTP_NODE_H2("AXIP Partners")
					HTTP_NODE_TABLE_OPEN_STACK
					HTTP_NODE_TABLE_HEADER_ROW "<th scope=col>Status</th><th scope=col>Call</th><th scope=col>Last Heard (s)</th></tr>"
					HTTP_NODE_TABLE_MID_STACK;

				char AXIPRow[] = "<tr><td data-label='Status' class='text'>%s</td><td data-label='Call' class='text'>%s</td><td data-label='Last Heard (s)' class='num'>%d</td></tr>";
				

				while (PORT)
				{
					AXPORT = Portlist[PORT->PORTNUMBER];

					if (AXPORT)
					{
						// Get ARP entries

						for (i = 0; i < AXPORT->arp_table_len; i++)
						{
							arp = &AXPORT->arp_table[i];

							if (arp->LastHeard == 0 || (NOW - arp->LastHeard) > 3600)			// Considered down
								ListD[nd++] = arp;
							else
								List[n++] = arp;
						}
					}
					PORT = PORT->PORTPOINTER;
				}

				if (n > 1)
					qsort(List, n, sizeof(void *), CompareNode);
				if (nd > 1)
					qsort(ListD, nd, sizeof(void *), CompareNode);

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", AXIPHeader);

				for (i = 0; i < n; i++)
				{
					int len = ConvFromAX25(List[i]->callsign, Normcall);
					Normcall[len]=0;
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], AXIPRow, "Up", Normcall, (List[i]->LastHeard)?(int)(NOW - List[i]->LastHeard):0);
				}

				for (i = 0; i < nd; i++)
				{
					int len = ConvFromAX25(ListD[i]->callsign, Normcall);
					Normcall[len]=0;
					ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], AXIPRow, "Down", Normcall, (ListD[i]->LastHeard)?(int)(NOW - ListD[i]->LastHeard):0);
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");
			}

			if (_stricmp(NodeURL, "/Node/Routes.html") == 0)
			{
				struct ROUTE * Routes = NEIGHBOURS;
				int MaxRoutes = MAXNEIGHBOURS;
				int count, i;
				char Normcall[10];
				char locked[4] = " ";
				int NodeCount;
				int Percent = 0;
				int Iframes, Retries;
				char Active[10];
				int Queued;
				
				int x = 0, n = 0;
				struct ROUTE * List[1000];

			
				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", RouteHddr);

				// Build and sort list of routes
				
				for (count = 0; count < MaxRoutes; count++)
				{
					if (Routes->NEIGHBOUR_CALL[0] != 0)
					{
						List[n++] = Routes;

						if (n > 999)
							break;
					}

					Routes++;
				}

				if (n > 1)
					qsort(List, n, sizeof(void *), CompareRoutes);

				for (i = 0; i < n; i++)
				{
					Routes = List[i];
					{
						int len = ConvFromAX25(Routes->NEIGHBOUR_CALL, Normcall);
						Normcall[len]=0;

						if (Routes->NEIGHBOUR_FLAG == LOCKEDBYCONFIG)
							strcpy(locked, "!");
						else if (Routes->NEIGHBOUR_FLAG == LOCKEDBYSYSOP)
							strcpy(locked, "!!");
						else if (Routes->NEIGHBOUR_FLAG == LOCKEDBYSYSOP + LOCKEDBYCONFIG)
							strcpy(locked, "!!!");
						else
							strcpy(locked, " ");

						NodeCount = COUNTNODES(Routes);

						if (Routes->NEIGHBOUR_LINK)
							Queued = COUNT_AT_L2(Routes->NEIGHBOUR_LINK);
						else
							Queued = 0;

						Iframes = Routes->NBOUR_IFRAMES;
						Retries = Routes->NBOUR_RETRIES;

						if (Routes->NEIGHBOUR_LINK && Routes->NEIGHBOUR_LINK->L2STATE >= 5)
							strcpy(Active, ">");
						else
							strcpy(Active, "&nbsp;");

						if (Iframes)
							Percent = (Retries * 100) / Iframes;
						else
							Percent = 0;

						if (Routes->INP3Node)		// INP3 Enabled?
						{
							double srtt = Routes->SRTT/100.0;
							double nsrtt = Routes->NeighbourSRTT/100.0;
	
							ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], RouteLineINP3, Active, Routes->NEIGHBOUR_PORT, Normcall, locked, 
								Routes->NEIGHBOUR_QUAL,	NodeCount, Iframes, Retries, Percent, Routes->NBOUR_MAXFRAME, Routes->NBOUR_FRACK,
								Routes->NEIGHBOUR_TIME >> 8, Routes->NEIGHBOUR_TIME & 0xff, Queued, Routes->OtherendsRouteQual, srtt, nsrtt);
						}
						else
						{
							ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], RouteLine, Active, Routes->NEIGHBOUR_PORT, Normcall, locked, 
								Routes->NEIGHBOUR_QUAL,	NodeCount, Iframes, Retries, Percent, Routes->NBOUR_MAXFRAME, Routes->NBOUR_FRACK,
								Routes->NEIGHBOUR_TIME >> 8, Routes->NEIGHBOUR_TIME & 0xff, Queued, Routes->OtherendsRouteQual);
						}
					}
					Routes+=1;
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");
			}

			if (_stricmp(NodeURL, "/Node/Links.html") == 0)
			{
				struct _LINKTABLE * Links = LINKS;
				int MaxLinks = MAXLINKS;
				int count;
				char Normcall1[10];
				char Normcall2[10];
				char State[12] = "", Type[12] = "Uplink";
				int axState;
				int cctType;

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", LinkHddr);

				for (count=0; count<MaxLinks; count++)
				{
					if (Links->LINKCALL[0] != 0)
					{
						int len = ConvFromAX25(Links->LINKCALL, Normcall1);
						Normcall1[len] = 0;

						len = ConvFromAX25(Links->OURCALL, Normcall2);
						Normcall2[len] = 0;

						axState = Links->L2STATE;

						if (axState == 2)
							strcpy(State, "Connecting");
						else if (axState == 3)
							strcpy(State, "FRMR");
						else if (axState == 4)
							strcpy(State, "Closing");
						else if (axState == 5)
							strcpy(State, "Active");
						else if (axState == 6)
							strcpy(State, "REJ Sent");

						cctType = Links->LINKTYPE;

						if (cctType == 1)
							strcpy(Type, "Uplink");
						else if (cctType == 2)
							strcpy(Type, "Downlink");
						else if (cctType == 3)
							strcpy(Type, "Node-Node");

						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], LinkLine, Normcall1, Normcall2, Links->LINKPORT->PORTNUMBER, 
							State, Type, 2 - Links->VER1FLAG );

						Links+=1;
					}
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");
			}

			if (_stricmp(NodeURL, "/Node/Users.html") == 0)
			{
				TRANSPORTENTRY * L4 = L4TABLE;
				TRANSPORTENTRY * Partner;
				int MaxLinks = MAXLINKS;
				int count;
				char State[12] = "", Type[12] = "Uplink";
				char LHS[50] = "", MID[10] = "", RHS[50] = "";

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", UserHddr);

				for (count=0; count < MAXCIRCUITS; count++)
				{
					if (L4->L4USER[0])
					{
						RHS[0] = MID[0] = 0;

						if ((L4->L4CIRCUITTYPE & UPLINK) == 0)   //SHORT CMDS10A		; YES
						{
							// IF DOWNLINK, ONLY DISPLAY IF NO CROSSLINK

							if (L4->L4CROSSLINK == 0)	//jne 	CMDS60			; WILL PROCESS FROM OTHER END
							{
								// ITS A DOWNLINK WITH NO PARTNER - MUST BE A CLOSING SESSION
								// DISPLAY TO THE RIGHT FOR NOW

								strcpy(LHS, "(Closing) ");
								DISPLAYCIRCUIT(L4, RHS);
								goto CMDS50;
							}
							else
								goto CMDS60;		// WILL PROCESS FROM OTHER END
						}

						if (L4->L4CROSSLINK == 0)
						{
							// Single Entry

							DISPLAYCIRCUIT(L4, LHS);
						}
						else
						{
							DISPLAYCIRCUIT(L4, LHS);

							Partner = L4->L4CROSSLINK;

							if (Partner->L4STATE == 5)
								strcpy(MID, "<-->");
							else
								strcpy(MID, "<~~>");

							DISPLAYCIRCUIT(Partner, RHS);
						}
CMDS50:
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], UserLine, LHS, MID, RHS);
					}
CMDS60:			
					L4++;	
				}

				ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "</tbody></table></div>");
			}
			/*
			PUBLIC	CMDUXX_1
			CMDUXX_1:
			push	EBX
			push	ESI
			PUSH	ECX
			push	EDI

			call	_FINDDESTINATION
			pop	EDI

			jz SHORT NODE_FOUND

			push	EDI			; NET/ROM not found
			call	CONVFROMAX25		; CONVERT TO CALL
			pop	EDI
			mov	ESI,OFFSET32 NORMCALL
			rep movsb

			jmp	SHORT END_CMDUXX

			PUBLIC	NODE_FOUND
			NODE_FOUND:

			lea	ESI,DEST_CALL[EBX]
			call	DECODENODENAME

			REP 	MOVSB

			PUBLIC	END_CMDUXX
			END_CMDUXX:

			POP	ECX
			pop	ESI
			pop	EBX
			ret

			}}}
			*/

			else if (_stricmp(NodeURL, "/Node/Terminal.html") == 0)
			{
				if (COOKIE && Session)
				{
					// Already signed in as sysop

					struct UserRec * USER = Session->USER;

					struct HTTPConnectionInfo * NewSession = AllocateSession(sock, 'T');

					if (NewSession)
					{
						char AXCall[10];
						ReplyLen = sprintf(_REPLYBUFFER, TermPage, Mycall, Mycall, NewSession->Key, NewSession->Key, NewSession->Key);
						strcpy(NewSession->HTTPCall, USER->Callsign);
						ConvToAX25(NewSession->HTTPCall, AXCall);
						ChangeSessionCallsign(NewSession->Stream, AXCall);
						BPQHOSTVECTOR[NewSession->Stream -1].HOSTSESSION->Secure_Session = USER->Secure;
						Session->USER = USER;
						NewSession->TNC = conn->TNC;


						//			if (Appl[0])
						//			{
						//				strcat(Appl, "\r");
						//				SendMsg(Session->Stream, Appl, strlen(Appl));
						//			}

					}
					else
					{
						ReplyLen = SetupNodeMenu(_REPLYBUFFER, sizeof(_REPLYBUFFER), LOCAL);
						ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", BusyError);
					}
				}
				else if (LOCAL)
				{
					// connected to 127.0.0.1 so sign in using node call

					struct HTTPConnectionInfo * NewSession = AllocateSession(sock, 'T');

					if (NewSession)
					{
						ReplyLen = sprintf(_REPLYBUFFER, TermPage, Mycall, Mycall, NewSession->Key, NewSession->Key, NewSession->Key);
						strcpy(NewSession->HTTPCall, MYNODECALL);
						ChangeSessionCallsign(NewSession->Stream, MYCALL);
						BPQHOSTVECTOR[NewSession->Stream -1].HOSTSESSION->Secure_Session = TRUE;
						NewSession->TNC = conn->TNC;
					}
				}
				else
					ReplyLen = sprintf(_REPLYBUFFER, TermSignon, Mycall, Mycall, Context);
			}

			else if (_stricmp(NodeURL, "/Node/Signon.html") == 0)
			{
				ReplyLen = sprintf(_REPLYBUFFER, NodeSignon, Mycall, Mycall, Context);
			}

			else if (_stricmp(NodeURL, "/Node/Drivers") == 0)
			{
				int Bufferlen = SendMessageFile(sock, "/Drivers.htm", TRUE, allowDeflate);		// return -1 if not found

				if (Bufferlen != -1)
					return 0;											// We've sent it
			}

			else if (_stricmp(NodeURL, "/Node/OutputScreen.html") == 0)
			{
				struct HTTPConnectionInfo * Session = FindSession(Context);

				if (Session == NULL)
				{
					ReplyLen = sprintf(_REPLYBUFFER, "%s", LostSession);
				}
				else
				{
					Session->sock = sock;				// socket to reply on
					ReplyLen = RefreshTermWindow(TCP, Session, _REPLYBUFFER);

					if (ReplyLen == 0)		// Nothing new
					{
						//				Debugprintf("GET with no data avail - response held");
						Session->ResponseTimer = 1200;		// Delay response for up to a minute
					}
					else
					{
						//				Debugprintf("GET - outpur sent, timer was %d, set to zero", Session->ResponseTimer);
						Session->ResponseTimer = 0;
					}

					Session->KillTimer = 0;
					return 0;				// Refresh has sent any available output
				}
			}

			else if (_stricmp(NodeURL, "/Node/InputLine.html") == 0)
			{
				struct TNCINFO * TNC = conn->TNC;
				struct TCPINFO * TCP = 0;

				if (TNC)
					TCP = TNC->TCPInfo;

				if (TCP && TCP->WebTermCSS)	
					ReplyLen = sprintf(_REPLYBUFFER, InputLine, Context, TCP->WebTermCSS);
				else
					ReplyLen = sprintf(_REPLYBUFFER, InputLine, Context, "");

			}

			else if (_stricmp(NodeURL, "/Node/PTT") == 0)
			{
				struct TNCINFO * TNC = conn->TNC;
				int x = atoi(Context);
			}


SendResp:

			FormatTime3(TimeString, time(NULL));

			strcpy(&_REPLYBUFFER[ReplyLen], Tail);
			ReplyLen += (int)strlen(Tail);


			if (allowDeflate)
			{
				Compressed = Compressit(_REPLYBUFFER, ReplyLen, &ReplyLen);
			} 
			else
			{
				Encoding[0] = 0;
				Compressed = _REPLYBUFFER;
			}

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n"
				COMMON_HTTP_SECURITY_HEADERS "Date: %s\r\n%s\r\n", ReplyLen, TimeString, Encoding);
			sendandcheck(sock, Header, HeaderLen);
			sendandcheck(sock, Compressed, ReplyLen);

			if (allowDeflate)
				free (Compressed);
		}
		return 0;

#ifdef WIN32xx
	}
#include "StdExcept.c"
}
return 0;
#endif
}

void ProcessHTTPMessage(void * conn)
{
	// conn is a malloc'ed copy to handle reused connections, so need to free it

	InnerProcessHTTPMessage((struct ConnectionInfo *)conn);
	free(conn);
	return;
}

static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


VOID FormatTime3(char * Time, time_t cTime)
{
	struct tm * TM;
	TM = gmtime(&cTime);

	sprintf(Time, "%s, %02d %s %3d %02d:%02d:%02d GMT", dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon],
		TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);

}

// Sun, 06 Nov 1994 08:49:37 GMT

int StatusProc(char * Buff)
{
	int i;
	char callsign[12] = "";
	char flag[3];
	UINT Mask, MaskCopy;
	int Flags;
	int AppNumber;
	int OneBits;
	int Len = sprintf(Buff, COMMON_HTML_HEAD_COMMON
		COMMON_HTML_META_UTF8
		COMMON_HTML_META_VIEWPORT_COMPACT
		"<meta http-equiv=expires content=0>"
		"<meta http-equiv=refresh content=15>"
		"<title>Stream Status</title>"
		COMMON_BPQ_CSS_LINK
		COMMON_NODE_CSS_LINK
		"</head><body>"
		"<h3>Stream Status</h3>");

	Len += sprintf(&Buff[Len], "<table>");
	Len += sprintf(&Buff[Len], "<tr><th>#</th><th>RX</th><th>TX</th>");
	Len += sprintf(&Buff[Len], "<th>MON</th><th>App</th><th>Flg</th>");
	Len += sprintf(&Buff[Len], "<th>Callsign</th><th>Program</th>");
	Len += sprintf(&Buff[Len], "<th>#</th><th>RX</th><th>TX</th>");
	Len += sprintf(&Buff[Len], "<th>MON</th><th>App</th><th>Flg</th>");
	Len += sprintf(&Buff[Len], "<th>Callsign</th><th>Program</th></tr><tr>");

	for (i=1; i <=BPQHOSTSTREAMS; i++)
	{		
		callsign[0]=0;

		if (GetAllocationState(i))

			strcpy(flag,"*");
		else
			strcpy(flag," ");

		GetCallsign(i,callsign);

		Mask = MaskCopy = Get_APPLMASK(i);

		// if only one bit set, convert to number

		AppNumber = 0;
		OneBits = 0;

		while (MaskCopy)
		{
			if (MaskCopy & 1)
				OneBits++;

			AppNumber++;
			MaskCopy = MaskCopy >> 1;
		}

		Flags=GetApplFlags(i);

		if (OneBits > 1)
			Len += sprintf(&Buff[Len], "<td>%d%s</td><td>%d</td><td>%d</td><td>%d</td><td>%x</td>"
			"<td>%x</td><td>%s</td><td>%s</td>", 
			i, flag, RXCount(i), TXCount(i), MONCount(i), Mask, Flags, callsign, BPQHOSTVECTOR[i-1].PgmName);

		else
			Len += sprintf(&Buff[Len], "<td>%d%s</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td>"
			"<td>%x</td><td>%s</td><td>%s</td>", 
			i, flag, RXCount(i), TXCount(i), MONCount(i), AppNumber, Flags, callsign, BPQHOSTVECTOR[i-1].PgmName);

		if ((i & 1) == 0)
			Len += sprintf(&Buff[Len], "</tr><tr>");

	}

	Len += sprintf(&Buff[Len], "</tr></table></body></html>");
	return Len;
}

int ProcessNodeSignon(SOCKET sock, struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, int LOCAL)
{
	int ReplyLen = 0;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Key;
	char Header[1024];
	int HeaderLen;
	struct HTTPConnectionInfo *Sess;


	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = sprintf(Reply, "<!DOCTYPE html><html><head><title>Node Redirect</title></head><body><script>window.location.href='/Node/NodeIndex.html';</script>");

			HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", (int)(ReplyLen + strlen(Tail)));	
			send(sock, Header, HeaderLen, 0);
			send(sock, Reply, ReplyLen, 0);
			send(sock, Tail, (int)strlen(Tail), 0);
			return ReplyLen;
		}
		user = strtok_s(&input[9], "&", &Key);
		password = strtok_s(NULL, "=", &Key);
		password = Key;

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (user && _stricmp(user, USER->UserName) == 0)
			{
				if (strcmp(password, USER->Password) == 0 && USER->Secure)
				{
					// ok

					Sess = *Session = AllocateSession(sock, 'N');

					if (Sess == NULL)
					{
						ReplyLen = SetupNodeMenu(Reply, ReplyBufferSize, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);

						HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", (int)(ReplyLen + strlen(Tail)));
						send(sock, Header, HeaderLen, 0);
						send(sock, Reply, ReplyLen, 0);
						send(sock, Tail, (int)strlen(Tail), 0);
						return ReplyLen;
					}

					Sess->USER = USER;

					ReplyLen =  SetupNodeMenu(Reply, ReplyBufferSize, LOCAL);

					HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "Set-Cookie: BPQSessionCookie=%s; Path = /\r\n\r\n", (int)(ReplyLen + strlen(Tail)), Sess->Key);	
					send(sock, Header, HeaderLen, 0);
					send(sock, Reply, ReplyLen, 0);
					send(sock, Tail, (int)strlen(Tail), 0);

					return ReplyLen;
				}
			}
		}
	}	

	ReplyLen = sprintf(Reply, NodeSignon, Mycall, Mycall);
	ReplyLen += sprintf(&Reply[ReplyLen], "%s", PassError);

	HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n" COMMON_HTTP_SECURITY_HEADERS "\r\n", (int)(ReplyLen + strlen(Tail)));	
	send(sock, Header, HeaderLen, 0);
	send(sock, Reply, ReplyLen, 0);
	send(sock, Tail, (int)strlen(Tail), 0);

	return 0;
}

int ProcessMailAPISignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, BOOL WebMail, int LOCAL)
{
	int ReplyLen = 0;
	char * user, * password;
	struct HTTPConnectionInfo * NewSession;
	int i;
	struct UserRec * USER;

	if (strchr(MsgPtr, '?'))
	{
		// Check Password

		user = strlop(MsgPtr, '?');
		password = strlop(user, '&');
		strlop(password, ' ');

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (user && _stricmp(user, USER->UserName) == 0)
			{
				if ((strcmp(password, USER->Password) == 0) && (USER->Secure || WebMail))
				{
					// ok

					NewSession = AllocateSession(Appl[0], 'M');

					*Session = NewSession;

					if (NewSession)
					{
						ReplyLen = 0;
						strcpy(NewSession->Callsign, USER->Callsign);
					}
					else
					{
						ReplyLen =  SetupNodeMenu(Reply, ReplyBufferSize, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					return ReplyLen;

				}
			}
		}

		// Pass failed attempt to BBS code so it can try a bbs user login

		// Need to put url back together

		if (user && user[0] && password && password[0])
		{
			sprintf(&MsgPtr[strlen(MsgPtr)], "?%s&%s", user, password); 
		}
	}

	NewSession = AllocateSession(Appl[0], 'M');

	*Session = NewSession;

	if (NewSession)
		ReplyLen = 0;
	else
	{
		ReplyLen =  SetupNodeMenu(Reply, ReplyBufferSize, LOCAL);
		ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
	}
	
	return ReplyLen;
}




int ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, BOOL WebMail, int LOCAL)
{
	int ReplyLen = 0;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Key;
	struct HTTPConnectionInfo * NewSession;

	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = sprintf(Reply, "<!DOCTYPE html><html><head><title>Mail Redirect</title></head><body><script>window.location.href='/Node/NodeIndex.html';</script>");
			return ReplyLen;
		}
		user = strtok_s(&input[9], "&", &Key);
		password = strtok_s(NULL, "=", &Key);
		password = Key;

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (user && _stricmp(user, USER->UserName) == 0)
			{
				if (strcmp(password, USER->Password) == 0 && (USER->Secure || WebMail))
				{
					// ok

					NewSession = AllocateSession(Appl[0], 'M');

					*Session = NewSession;

					if (NewSession)
					{

						ReplyLen = 0;
						strcpy(NewSession->Callsign, USER->Callsign);
					}
					else
					{
						ReplyLen =  SetupNodeMenu(Reply, ReplyBufferSize, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					return ReplyLen;
				}
			}
		}
	}	

	ReplyLen = sprintf(Reply, MailSignon, Mycall, Mycall);
	ReplyLen += sprintf(&Reply[ReplyLen], "%s", PassError);

	return ReplyLen;
}


int ProcessChatSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int ReplyBufferSize, struct HTTPConnectionInfo ** Session, int LOCAL)
{
	int ReplyLen = 0;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Key;

	if (input)
	{
		int i;
		struct UserRec * USER;

		UndoTransparency(input);

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = sprintf(Reply, "<!DOCTYPE html><html><head><title>Chat Redirect</title></head><body><script>window.location.href='/Node/NodeIndex.html';</script>");
			return ReplyLen;
		}

		user = strtok_s(&input[9], "&", &Key);
		password = strtok_s(NULL, "=", &Key);
		password = Key;


		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (user && _stricmp(user, USER->UserName) == 0)
			{
				if (strcmp(password, USER->Password) == 0 && USER->Secure)
				{
					// ok

					*Session = AllocateSession(Appl[0], 'C');

					if (*Session)
					{
						ReplyLen = 0;
					}
					else
					{
						ReplyLen = SetupNodeMenu(Reply, ReplyBufferSize, LOCAL);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					return ReplyLen;
				}
			}
		}
	}	

	ReplyLen = sprintf(Reply, ChatSignon, Mycall, Mycall);
	ReplyLen += sprintf(&Reply[ReplyLen], "%s", PassError);

	return ReplyLen;

}

#define SHA1_HASH_LEN 20

/*

Copyright (C) 1998, 2009
Paul E. Jones <paulej@packetizer.com>

Freeware Public License (FPL)

This software is licensed as "freeware."  Permission to distribute
this software in source and binary forms, including incorporation 
into other products, is hereby granted without a fee.  THIS SOFTWARE 
IS PROVIDED 'AS IS' AND WITHOUT ANY EXPRESSED OR IMPLIED WARRANTIES, 
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE.  THE AUTHOR SHALL NOT BE HELD 
LIABLE FOR ANY DAMAGES RESULTING FROM THE USE OF THIS SOFTWARE, EITHER 
DIRECTLY OR INDIRECTLY, INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA 
OR DATA BEING RENDERED INACCURATE.
*/

/*  sha1.h
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved
 *
 *****************************************************************************
 *  $Id: sha1.h 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This class implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      Many of the variable names in the SHA1Context, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *
 *      Please read the file sha1.c for more information.
 *
 */

#ifndef _SHA1_H_
#define _SHA1_H_

/* 
 *  This structure will hold context information for the hashing
 *  operation
 */
typedef struct SHA1Context
{
    unsigned Message_Digest[5]; /* Message Digest (output)          */

    unsigned Length_Low;        /* Message length in bits           */
    unsigned Length_High;       /* Message length in bits           */

    unsigned char Message_Block[64]; /* 512-bit message blocks      */
    int Message_Block_Index;    /* Index into message block array   */

    int Computed;               /* Is the digest computed?          */
    int Corrupted;              /* Is the message digest corruped?  */
} SHA1Context;

/*
 *  Function Prototypes
 */
void SHA1Reset(SHA1Context *);
int SHA1Result(SHA1Context *);
void SHA1Input( SHA1Context *, const unsigned char *, unsigned);

#endif
 
BOOL SHA1PasswordHash(char * lpszPassword, char * Hash)
{
	SHA1Context sha; 
	int i;

	SHA1Reset(&sha);
	SHA1Input(&sha, lpszPassword, strlen(lpszPassword));
	SHA1Result(&sha);

	// swap byte order if little endian
	
	for (i = 0; i < 5; i++)
		sha.Message_Digest[i] = htonl(sha.Message_Digest[i]);

	memcpy(Hash, &sha.Message_Digest[0], 20);

    return TRUE;
}

int BuildRigCtlPage(char * _REPLYBUFFER)
{
	int ReplyLen;

	struct RIGPORTINFO * PORT;
	struct RIGINFO * RIG;
	int p, i;

	char Page[] =
		COMMON_HTML_HEAD_COMMON
		COMMON_HTML_META_UTF8
		COMMON_HTML_META_VIEWPORT_COMPACT
		"<meta http-equiv=expires content=10>\r\n"
		"<title>Rig Control</title>\r\n"
		COMMON_BPQ_CSS_LINK
		COMMON_NODE_CSS_LINK
		"<style>form{margin:0;padding:0;display:inline;}td{white-space:nowrap;padding:4px 8px;}</style>"
		"</head><body>"
		"<h3>Rig Control</h3>\r\n"
		"<table><tr>\r\n"
		"<th>Radio</th>\r\n"
		"<th>Freq</th>\r\n"
		"<th>Mode</th>\r\n"
		"<th>ST</th>\r\n"
		"<th>Ports</th>\r\n"
		"<th style=\"display:none\">Action</th>\r\n"
		"</tr>";
	char RigLine[] =
		"<tr><td data-label='Radio' class='text'>%s</td><td data-label='Freq' class='text'>%s</td><td data-label='Mode' class='text'>%s/1</td>"
		"<td data-label='ST' class='text'>%c%c</td><td data-label='Ports' class='text'>%s</td>"
		"<td style=\"display:none\"><input onclick=PTT('R%d') type=submit class='btn' value='PTT'></td></tr>";
	char Tail[] =		
		"</table>\r\n"
		"</body></html>\r\n";

	ReplyLen = sprintf(_REPLYBUFFER, "%s", Page);

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];
			ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], RigLine, RIG->WEB_Label, RIG->WEB_FREQ, RIG->WEB_MODE, RIG->WEB_SCAN, RIG->WEB_PTT, RIG->WEB_PORTS, RIG->Interlock);
		}
	}

	ReplyLen += sprintf(&_REPLYBUFFER[ReplyLen], "%s", Tail);
	return ReplyLen;
}


void SendRigWebPage()
{
	int i, n;
	struct ConnectionInfo * sockptr;
	struct TNCINFO * TNC;
	struct TCPINFO * TCP;

	for (i = 0; i < 33; i++)
	{
		TNC = TNCInfo[i];

		if (TNC && TNC->Hardware == H_TELNET)
		{
			TCP = TNC->TCPInfo;

			if (TCP)
			{
				for (n = 0; n <= TCP->MaxSessions; n++)
				{
					sockptr = TNC->Streams[n].ConnectionInfo;

					if (sockptr->SocketActive)
					{
						if (sockptr->HTTPMode && sockptr->WebSocks  && strcmp(sockptr->WebURL, "RIGCTL") == 0)
						{
							char RigMsg[8192];
							int RigMsgLen = strlen(RigWebPage);
							char* ptr;

							RigMsg[0] = 0x81;		// Fin, Data
							RigMsg[1] = 126;		// Unmasked, Extended Len
							RigMsg[2] = RigMsgLen >> 8;
							RigMsg[3] = RigMsgLen & 0xff;
							strcpy(&RigMsg[4], RigWebPage);

							// If secure session enable PTT button

							if (sockptr->WebSecure)
							{
								while (ptr = strstr(RigMsg, "hidden"))
									memcpy(ptr, "      ", 6);
							}

							send(sockptr->socket, RigMsg, RigMsgLen + 4, 0);
						}
					}
				}
			}
		}
	}
}

// Webmail web socket code

int ProcessWebmailWebSock(char * MsgPtr, char * OutBuffer);

void ProcessWebmailWebSockThread(void * conn)
{
	// conn is a malloc'ed copy to handle reused connections, so need to free it

	struct ConnectionInfo * sockptr = (struct ConnectionInfo *)conn;
	char * URL = sockptr->WebURL;
	int Loops = 0;
	int Sent;
	struct HTTPConnectionInfo Dummy = {0};
	int ReplyLen = 0;
	int InputLen = 0;

#ifdef LINBPQ

	char _REPLYBUFFER[250000];

	ReplyLen = ProcessWebmailWebSock(URL, _REPLYBUFFER);

	// Send may block

	Sent = send(sockptr->socket, _REPLYBUFFER, ReplyLen, 0);

	if (Sent == -1)		// Connecton lost
	{
		closesocket(sockptr->socket);
		free(conn);
		return;
	}

	while (Sent != ReplyLen && Loops++ < 3000)					// 90 secs max
	{	
		if (Sent > 0)					// something sent
		{
			ReplyLen -= Sent;
			memmove(_REPLYBUFFER, &_REPLYBUFFER[Sent], ReplyLen);
		}	

		Sleep(30);
		Sent = send(sockptr->socket, _REPLYBUFFER, ReplyLen, 0);
	}
	
#else
	// Send URL to BPQMail via Pipe. Just need a dummy session, as URL contains session key

	HANDLE hPipe;
	char Reply[250000];



	hPipe = CreateFile(MAILPipeFileName, GENERIC_READ | GENERIC_WRITE,
		0,                    // exclusive access
		NULL,                 // no security attrs
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 
		NULL );

	if (hPipe == (HANDLE)-1)
	{
		free(conn);
		return;
	}

	WriteFile(hPipe, &Dummy, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
	WriteFile(hPipe, URL, strlen(URL), &InputLen, NULL);

	ReadFile(hPipe, &Dummy, sizeof (struct HTTPConnectionInfo), &InputLen, NULL);
	ReadFile(hPipe, Reply, 250000, &ReplyLen, NULL);

	if (ReplyLen <= 0)
	{
		InputLen = GetLastError();
	}

	CloseHandle(hPipe);

	Sent = send(sockptr->socket, Reply, ReplyLen, 0);

	if (Sent == -1)		// Connecton lost
	{
		free(conn);
		closesocket(sockptr->socket);
		return;
	}

	while (Sent != ReplyLen && Loops++ < 3000)					// 90 secs max
	{	
		//			Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);

		if (Sent > 0)					// something sent
		{
			InputLen -= Sent;
			memmove(Reply, &Reply[Sent], ReplyLen);
		}

		Sleep(30);
		Sent = send(sockptr->socket, Reply, ReplyLen, 0);
	}
#endif
	free(conn);
	return;
}

/*
 *  sha1.c
 *
 *  Copyright (C) 1998, 2009
 *  Paul E. Jones <paulej@packetizer.com>
 *  All Rights Reserved
 *
 *****************************************************************************
 *  $Id: sha1.c 12 2009-06-22 19:34:25Z paulej $
 *****************************************************************************
 *
 *  Description:
 *      This file implements the Secure Hashing Standard as defined
 *      in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The Secure Hashing Standard, which uses the Secure Hashing
 *      Algorithm (SHA), produces a 160-bit message digest for a
 *      given data stream.  In theory, it is highly improbable that
 *      two messages will produce the same message digest.  Therefore,
 *      this algorithm can serve as a means of providing a "fingerprint"
 *      for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code was
 *      written with the expectation that the processor has at least
 *      a 32-bit machine word size.  If the machine word size is larger,
 *      the code should still function properly.  One caveat to that
 *      is that the input functions taking characters and character
 *      arrays assume that only 8 bits of information are stored in each
 *      character.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits
 *      long. Although SHA-1 allows a message digest to be generated for
 *      messages of any number of bits less than 2^64, this
 *      implementation only works with messages with a length that is a
 *      multiple of the size of an 8-bit character.
 *
 */

/*
 *  Define the circular shift macro
 */
#define SHA1CircularShift(bits,word) \
                ((((word) << (bits)) & 0xFFFFFFFF) | \
                ((word) >> (32-(bits))))

/* Function prototypes */
void SHA1ProcessMessageBlock(SHA1Context *);
void SHA1PadMessage(SHA1Context *);

/*  
 *  SHA1Reset
 *
 *  Description:
 *      This function will initialize the SHA1Context in preparation
 *      for computing a new message digest.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to reset.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1Reset(SHA1Context *context)
{
    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Message_Digest[0]      = 0x67452301;
    context->Message_Digest[1]      = 0xEFCDAB89;
    context->Message_Digest[2]      = 0x98BADCFE;
    context->Message_Digest[3]      = 0x10325476;
    context->Message_Digest[4]      = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;
}

/*  
 *  SHA1Result
 *
 *  Description:
 *      This function will return the 160-bit message digest into the
 *      Message_Digest array within the SHA1Context provided
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to use to calculate the SHA-1 hash.
 *
 *  Returns:
 *      1 if successful, 0 if it failed.
 *
 *  Comments:
 *
 */
int SHA1Result(SHA1Context *context)
{

    if (context->Corrupted)
    {
        return 0;
    }

    if (!context->Computed)
    {
        SHA1PadMessage(context);
        context->Computed = 1;
    }

    return 1;
}

/*  
 *  SHA1Input
 *
 *  Description:
 *      This function accepts an array of octets as the next portion of
 *      the message.
 *
 *  Parameters:
 *      context: [in/out]
 *          The SHA-1 context to update
 *      message_array: [in]
 *          An array of characters representing the next portion of the
 *          message.
 *      length: [in]
 *          The length of the message in message_array
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1Input(     SHA1Context         *context,
                    const unsigned char *message_array,
                    unsigned            length)
{
    if (!length)
    {
        return;
    }

    if (context->Computed || context->Corrupted)
    {
        context->Corrupted = 1;
        return;
    }

    while(length-- && !context->Corrupted)
    {
        context->Message_Block[context->Message_Block_Index++] =
                                                (*message_array & 0xFF);

        context->Length_Low += 8;
        /* Force it to 32 bits */
        context->Length_Low &= 0xFFFFFFFF;
        if (context->Length_Low == 0)
        {
            context->Length_High++;
            /* Force it to 32 bits */
            context->Length_High &= 0xFFFFFFFF;
            if (context->Length_High == 0)
            {
                /* Message is too long */
                context->Corrupted = 1;
            }
        }

        if (context->Message_Block_Index == 64)
        {
            SHA1ProcessMessageBlock(context);
        }

        message_array++;
    }
}

/*  
 *  SHA1ProcessMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *      Many of the variable names in the SHAContext, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *         
 *
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const unsigned K[] =            /* Constants defined in SHA-1   */      
    {
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    int         t;                  /* Loop counter                 */
    unsigned    temp;               /* Temporary word value         */
    unsigned    W[80];              /* Word sequence                */
    unsigned    A, B, C, D, E;      /* Word buffers                 */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = ((unsigned) context->Message_Block[t * 4]) << 24;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Message_Digest[0];
    B = context->Message_Digest[1];
    C = context->Message_Digest[2];
    D = context->Message_Digest[3];
    E = context->Message_Digest[4];

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->Message_Digest[0] =
                        (context->Message_Digest[0] + A) & 0xFFFFFFFF;
    context->Message_Digest[1] =
                        (context->Message_Digest[1] + B) & 0xFFFFFFFF;
    context->Message_Digest[2] =
                        (context->Message_Digest[2] + C) & 0xFFFFFFFF;
    context->Message_Digest[3] =
                        (context->Message_Digest[3] + D) & 0xFFFFFFFF;
    context->Message_Digest[4] =
                        (context->Message_Digest[4] + E) & 0xFFFFFFFF;

    context->Message_Block_Index = 0;
}

/*  
 *  SHA1PadMessage
 *
 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64
 *      bits represent the length of the original message.  All bits in
 *      between should be 0.  This function will pad the message
 *      according to those rules by filling the Message_Block array
 *      accordingly.  It will also call SHA1ProcessMessageBlock()
 *      appropriately.  When it returns, it can be assumed that the
 *      message digest has been computed.
 *
 *  Parameters:
 *      context: [in/out]
 *          The context to pad
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1PadMessage(SHA1Context *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = (context->Length_High >> 24) & 0xFF;
    context->Message_Block[57] = (context->Length_High >> 16) & 0xFF;
    context->Message_Block[58] = (context->Length_High >> 8) & 0xFF;
    context->Message_Block[59] = (context->Length_High) & 0xFF;
    context->Message_Block[60] = (context->Length_Low >> 24) & 0xFF;
    context->Message_Block[61] = (context->Length_Low >> 16) & 0xFF;
    context->Message_Block[62] = (context->Length_Low >> 8) & 0xFF;
    context->Message_Block[63] = (context->Length_Low) & 0xFF;

    SHA1ProcessMessageBlock(context);
}


