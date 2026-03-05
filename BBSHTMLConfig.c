/*
Copyright 2001-2018 John Wiseman G8BPQ

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

#define _CRT_SECURE_NO_DEPRECATE

#include "cheaders.h"
#include "bpqmail.h"
#include "common_web_components.h"

#ifdef WIN32
//#include "C:\Program Files (x86)\GnuWin32\include\iconv.h"
#else
#include <iconv.h>
#endif

extern char NodeTail[];
extern char BBSName[10];

extern char LTFROMString[2048];
extern char LTTOString[2048];
extern char LTATString[2048];

//static UCHAR BPQDirectory[260];

extern ConnectionInfo Connections[];

extern int NumberofStreams;
extern time_t MaintClock;						// Time to run housekeeping

extern int SMTPMsgs;

extern int ChatApplNum;
extern int MaxChatStreams;

extern char Position[81];
extern char PopupText[251];
extern int PopupMode;
extern int reportMailEvents;

#define MaxCMS	10				// Numbr of addresses we can keep - currently 4 are used.

struct UserInfo * BBSLIST[NBBBS + 1];

int MaxBBS = 0;

#define MAIL
#include "httpconnectioninfo.h"

struct TCPINFO * TCP;

VOID ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int * RLen);
static struct HTTPConnectionInfo * FindSession(char * Key);
VOID ProcessUserUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessMsgFwdUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendConfigPage(char * Reply, int * ReplyLen, char * Key);
VOID ProcessConfUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessUIUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendUserSelectPage(char * Reply, int * ReplyLen, char * Key);
VOID SendFWDSelectPage(char * Reply, int * ReplyLen, char * Key);
int EncryptPass(char * Pass, char * Encrypt);
VOID ProcessFWDUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendStatusPage(char * Reply, int * ReplyLen, char * Key);
VOID SendUIPage(char * Reply, int * ReplyLen, char * Key);
VOID GetParam(char * input, char * key, char * value);
BOOL GetConfig(char * ConfigName);
VOID ProcessDisUser(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
int APIENTRY SessionControl(int stream, int command, int param);
int SendMessageDetails(struct MsgInfo * Msg, char * Reply, char * Key);
VOID ProcessMsgUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessMsgAction(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
int APIENTRY GetNumberofPorts();
int APIENTRY GetPortNumber(int portslot);
UCHAR * APIENTRY GetPortDescription(int portslot, char * Desc);
struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
VOID SendHouseKeeping(char * Reply, int * ReplyLen, char * Key);
VOID SendWelcomePage(char * Reply, int * ReplyLen, char * Key);
VOID SaveWelcome(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
VOID GetMallocedParam(char * input, char * key, char ** value);
VOID SaveMessageText(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SaveHousekeeping(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
VOID SaveWP(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
int SendWPDetails(WPRec * WP, char * Reply, char * Key);
VOID SendWPSelectPage(char * Reply, int * ReplyLen, char * Key);
int SendUserDetails(struct HTTPConnectionInfo * Session, char * Reply, char * Key);
int SetupNodeMenu(char * Buff);
VOID SendFwdSelectPage(char * Reply, int * ReplyLen, char * Key);
VOID SendFwdDetails(struct UserInfo * User, char * Reply, int * ReplyLen, char * Key);
VOID SetMultiStringValue(char ** values, char * Multi);
VOID SendFwdMainPage(char * Reply, int * ReplyLen, char * Key);
VOID SaveFwdCommon(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SaveFwdDetails(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
char **	SeparateMultiString(char * MultiString, BOOL NoToUpper);
VOID TidyPrompts();
char * GetTemplateFromFile(int Version, char * FN);
VOID FormatTime(char * Time, time_t cTime);
struct MsgInfo * GetMsgFromNumber(int msgno);
BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP);
BOOL OkToKillMessage(BOOL SYSOP, char * Call, struct MsgInfo * Msg);
int MulticastStatusHTML(char * Reply);
void ProcessWebMailMessage(struct HTTPConnectionInfo * Session, char * Key, BOOL LOCAL, char * Method, char * NodeURL, char * input, char * Reply, int * RLen, int InputLen);
int SendWebMailHeader(char * Reply, char * Key, struct HTTPConnectionInfo * Session);
struct UserInfo * FindBBS(char * Name);
void ReleaseWebMailStruct(WebMailInfo * WebMail);
VOID TidyWelcomeMsg(char ** pPrompt);
int MailAPIProcessHTTPMessage(struct HTTPConnectionInfo * Session, char * response, char * Method, char * URL, char * request, BOOL LOCAL, char * Param, char * Token);
void UndoTransparency(char * input);
int GetMessageSlotFromMessageNumber(int msgno);

char UNC[] = "";
char CHKD[] = "checked=checked ";
char sel[] = "selected";

char Sent[] = "var(--fwd-sent)";
char ToSend[] = "var(--fwd-queued)";
char NotThisOne[] = "var(--fwd-none)";

static char PassError[] = "<p style='text-align:center'>Sorry, User or Password is invalid - please try again</p>";

static char BusyError[] = "<p style='text-align:center'>Sorry, No sessions available - please try later</p>";

extern char WebMailSignon[];

char MailSignon[] = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
	"<title>BPQ32 Mail Server Access</title>"
	"<style>" COMMON_SIGNON_CSS "</style></head><body>"
	"<h2 class=text-center>BPQ32 Mail Server %s Access</h2>"
	"<h3 class=text-center>Please enter Callsign and Password</h3>"
	"<div class=\"form-container\"><form method=post action=/Mail/Signon?Mail>"
	"<div class=\"form-row\"><label for=user>User</label><input type=text id=user name=user tabindex=1 maxlength=50 required></div>"
	"<div class=\"form-row\"><label for=password>Password</label><input type=password id=password name=password tabindex=2 maxlength=50 required></div>"
	"<div class=\"form-row\"><input type=submit class='btn' value=Submit /><input type=submit class='btn' value=Cancel name=Cancel /></div></form></div></body></html>";


char MailPage[] = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
	"<title>%s's BBS Web Server</title>"
	"<style type=\"text/css\">"
	COMMON_CSS_VARIABLES
	COMMON_MENU_CSS
	"</style>"
	"<script>"
	COMMON_MENU_JAVASCRIPT
	"</script>"
	"</head>"
	"<body><h2>BPQ32 BBS %s</h2>"
	COMMON_MAIL_MENU;

char RefreshMainPage[] = "<html><head>"
	"<meta http-equiv=refresh content=10>"
	"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
	"<style type=\"text/css\">"
	COMMON_CSS_VARIABLES
	COMMON_MENU_CSS
	"</style>"
	"<script>"
	COMMON_MENU_JAVASCRIPT
	"</script>"
	"<title>%s's BBS Web Server</title></head>"
	"<body><h2>BPQ32 BBS %s</h2>"
	COMMON_MAIL_MENU;

char StatusPage [] = 
"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>"
COMMON_CSS_VARIABLES
COMMON_TABLE_CSS
COMMON_FORM_CSS
COMMON_UTILITY_CSS
COMMON_BUTTON_CSS
".status-grid{width:100%%;border-collapse:collapse;font-family: " COMMON_FONT_MONO ";white-space:nowrap;}"
".status-grid{font-size:clamp(0.75rem,0.65rem + 1vw,0.9375rem);}"
".status-grid{background:var(--surface);color:var(--text);}"
".status-grid thead tr{background:var(--table-header);}"
".status-grid th,.status-grid td{padding:10px;border:1px solid var(--border);color:var(--text);}"
".status-grid th{background:var(--table-header);font-weight:bold;}"
".status-grid td{background:var(--surface);}"
".status-grid tbody tr:nth-child(even){background:var(--table-stripe);}"
".status-grid tbody tr:hover{background:var(--surface-soft);}"
".status-grid th{text-align:left;}"
".status-grid td{text-align:left;}"
".status-grid th.num,.status-grid td.num{text-align:center;}"
".status-actions input{min-width:160px;}"
".stats-section{max-width:560px;margin:16px auto 0;padding:clamp(12px,2vw,18px);background:var(--surface);border:1px solid var(--border);border-radius:6px;box-shadow:var(--shadow-card);}"
".stat-row{display:flex;align-items:center;gap:12px;margin:8px 0;}"
".stat-row label{flex:1 1 220px;font-weight:600;font-size:clamp(0.8125rem,1.5vw,0.9375rem);line-height:1.3;}"
".stat-row input{flex:0 0 130px;max-width:130px;padding:clamp(10px,1vw,14px) clamp(12px,1.5vw,16px);line-height:1.5;box-sizing:border-box;border:1px solid var(--border);border-radius:4px;background:var(--surface-soft);color:var(--text);font-family:" COMMON_FONT_MONO ";font-size:clamp(0.875rem,2vw,1rem);font-variant-numeric:tabular-nums;text-align:right;min-height:44px;}"
".section-title{text-align:center;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.25rem,1.05rem + 0.9vw,1.75rem);font-weight:700;margin:12px 0 0;}"
"@media(max-width:768px){.status-grid th,.status-grid td{padding:8px 6px;}.stats-section{margin-top:12px;padding:12px;}.stat-row{flex-direction:column;align-items:stretch;gap:6px;}.stat-row label{flex:none;width:100%%;}.stat-row input{flex:1 1 auto;max-width:none;width:100%%;min-height:48px;text-align:left;}}"
"</style></head><body><div class=section-title>Active Sessions</div>"
"<form method=post action=/Mail/DisSession?%s>"
"<div class=table-container><table class=status-grid>"
"<thead><tr><th scope=col class=num>Select</th><th scope=col>User</th><th scope=col>Callsign</th><th scope=col class=num>Stream</th><th scope=col class=num>Queue</th><th scope=col class=num>Sent</th><th scope=col class=num>Rxed</th></tr></thead><tbody>";

char StreamEnd[] = 
"</tbody></table></div>";

char StatusTail[] = 
"<div class=stats-section>"
"<div class=stat-row><label>Total Messages</label><input readonly=readonly value=%d></div>"
"<div class=stat-row><label>Sysop Messages</label><input readonly=readonly value=%d></div>"
"<div class=stat-row><label>Held Messages</label><input readonly=readonly value=%d></div>"
"<div class=stat-row><label>SMTP Messages</label><input readonly=readonly value=%d></div>"
"</div>"
"<div class=\"buttons status-actions\"><input name=Disconnect value=Disconnect type=submit></div>"
"</form></body></html>";


char UIHddr [] = "<h3>User Interface Configuration</h3>"
	"<form method=post action=/Mail/UI?%s>"
	"<div class=form-section>"
	"<div class=form-row><label>Mailfor Header</label><input type=text value=\"%s\" name=MailFor></div>"
	"<p class=muted-note>(use \\r to insert newline in message)</p>"
	"</div>"
	"<div class=form-section>"
	"<div class=port-row-header>"
	"<span class=port-row-label>Enable</span><span class=flex-2-200>Port Path</span><span>Send MailFor</span><span>Send Headers</span><span>Send Empty</span>"
	"</div>";

char UILine[] = "<div class=port-row><input %s name=En%d type=checkbox><span class=port-row-label>%s</span><input type=text value=\"%s\" name=Path%d><input %s name=SndMF%d type=checkbox><input %s name=SndHDDR%d type=checkbox><input %s name=SndNull%d type=checkbox></div>";

char UITail[] = "</div><div class=buttons><input name=Update value=Update type=submit> <input name=Cancel value=Cancel type=submit></div></form></body></html>";

char FWDSelectHddr[] = 
	"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>"
	COMMON_CSS_VARIABLES
	COMMON_FORM_CSS
	COMMON_BUTTON_CSS
	"</style></head><body><h3>Forwarding Selection</h3>"
	"<form method=post action=/Mail/FWDSel?%s>"
	"<div class=form-section>"
	"<div class=form-row><label>Max Size to Send</label><input type=number value=%d name=MaxTX></div>"
	"<div class=form-row><label>Max Size to Receive</label><input type=number value=%d name=MaxRX></div>"
	"<div class=form-row><label>Warn if no route for P or T</label><div class=checkbox-group><input %s name=WarnNoRoute type=checkbox><span>Enable</span></div></div>"
	"<div class=form-row><label>Use Local Time</label><div class=checkbox-group><input %s name=LocalTime type=checkbox><span>Enable</span></div></div>"
	"</div>"
	"<div class=form-section>"
	"<div class=form-row><label>Aliases</label><textarea name=Aliases>%s</textarea></div>"
	"<div class=form-row><label>Select BBS</label><select name=call>";

char FWDSelectTail[] =
	"</select></div></div><div class=buttons><input name=Save value=Save type=submit> <input name=Cancel value=Cancel type=submit> <input name=Select value=Select type=submit></div></form></body></html>";

char UserSelectHddr[] = 
	"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>"
	COMMON_CSS_VARIABLES
	COMMON_FORM_CSS
	COMMON_BUTTON_CSS
	"</style></head><body><h3>Please Select User</h3><form method=post action=/Mail/Users?%s><select name=call>";

char UserSelectLine[] = "<option value=%s>%s</option>";

char StatusLine[] = "<option value=%d>%s</option>";

char UserSelectTail[] = "</select><div class=form-row><input size=6 value=\"\" name=NewCall><input type=submit value=\"Add User\" name=Adduser></div><div class=buttons><input type=submit value=Select> <input type=submit value=Cancel name=Cancel></div></form></body></html>";

char UserUpdateHddr[] =
	"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>"
	COMMON_CSS_VARIABLES
	COMMON_FORM_CSS
	COMMON_BUTTON_CSS
	"</style></head><body><h3>Update User %s</h3>"
	"<form method=post action=/Mail/Users?%s>";

char UserUpdateLine[] = "<option value=%s>%s</option>";

//<option value="G8BPQ">G8BPQ</option>
//<input checked="checked" name=%s type="checkbox"><br>


char FWDUpdate[] = 
"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>"
COMMON_CSS_VARIABLES
COMMON_FORM_CSS
COMMON_UTILITY_CSS
COMMON_BUTTON_CSS
"</style></head><body><h3>Update Forwarding for BBS %s</h3>"
"<form method=post action=/Mail/FWD?%s name=Test>"
"<div class=textarea-section><h4>Forwarding Addresses & Schedule</h4><div class=textarea-grid>"
"<div class=textarea-group><label>TO</label><textarea name=TO>%s</textarea></div>"
"<div class=textarea-group><label>AT</label><textarea name=AT>%s</textarea></div>"
"<div class=textarea-group><label>TIMES</label><textarea name=Times>%s</textarea></div>"
"<div class=textarea-group><label>Connect Script</label><textarea name=FWD>%s</textarea></div>"
"</div></div>"
"<div class=textarea-section><h4>Protocol Options</h4><div class=textarea-grid>"
"<div class=textarea-group><label>HRB</label><textarea name=HRB>%s</textarea></div>"
"<div class=textarea-group><label>HRP</label><textarea name=HRP>%s</textarea></div>"
"</div></div>"
"<div class=form-row><label>Enable Forwarding</label><div class=checkbox-group><input %s name=EnF type=checkbox><span class=font-normal>Enable</span><label class=inline-label>Interval (Secs)</label><input type=number class=input-w-80 value=%d size=3 name=Interval></div></div>"
"<div class=form-row><label>Request Reverse</label><div class=checkbox-group><input %s name=EnR type=checkbox><span class=font-normal>Enable</span><label class=inline-label>Interval (Secs)</label><input type=number class=input-w-80 value=%d size=3 name=RInterval></div></div>"
"<div class=form-row><label>Send new messages without waiting for poll timer</label><div class=checkbox-group><input %s name=NoWait type=checkbox><span class=font-normal>Enable</span></div></div>"
"<div class=form-row><label>BBS HA</label><input type=text value=\"%%s\" name=BBSHA></div>"
"<div class=form-row><label>FBB Max Block Size</label><input type=number class=input-w-100 value=%d size=3 name=FBBBlock></div>"
"<div class=form-row><label>Protocol Flags</label><div class=checkbox-group>"
"<div><input %s name=Personal type=checkbox><span class=font-normal>Personal Mail Only</span></div>"
"<div><input %s name=Bin type=checkbox><span class=font-normal>Allow Binary</span></div>"
"<div><input %s name=B1 type=checkbox><span class=font-normal>Use B1 Protocol</span></div>"
"<div><input %s name=B2 type=checkbox><span class=font-normal>Use B2 Protocol</span></div>"
"</div></div>"
"<div class=buttons><input name=Submit value=Update type=submit> <input name=Fwd value=\"Start Forwarding\" type=submit> <input name=Cancel value=Cancel type=submit></div></form></body></html>";

// Split MailDetailPage into smaller chunks to avoid ARM printf_positional limits with large format strings
static char MailDetailCSS[] = 
"<style>"
COMMON_CSS_VARIABLES
"h3{text-align:center;margin-bottom:20px;}"
".form-section{background:var(--surface);padding:clamp(12px,4vw,20px);border-radius:8px;box-shadow:var(--shadow-card);margin:15px 0;}"
".form-grid{display:grid;grid-template-columns:repeat(3,1fr);gap:16px 20px;margin:16px 0;}"
".form-field{display:flex;flex-direction:column;gap:6px;}"
".form-field label{font-weight:600;font-size:clamp(13px,1.5vw,15px);color:var(--text);text-transform:uppercase;letter-spacing:0.3px;}"
".form-field input,.form-field select{padding:10px 12px;border:1px solid var(--border);border-radius:6px;font-size:clamp(14px,2vw,16px);transition:border-color 0.15s ease,box-shadow 0.15s ease;min-height:44px;font-family: " COMMON_FONT_MONO ";}"
".form-field input:focus-visible,.form-field select:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"
".form-row-full{grid-column:1/-1;}"
"input[readonly]{background:var(--surface-soft);color:var(--text);cursor:not-allowed;}"
"input.uppercase{text-transform:uppercase;}"
".table-container{background:var(--surface);border-radius:8px;box-shadow:var(--shadow-card);overflow-x:auto;margin:15px 0;}"
"table{width:100%%;border-collapse:collapse;table-layout:fixed;}"
"th,td{padding:12px 14px;border:1px solid var(--border);text-align:center;font-size:14px;width:12.5%%;}"
"th{background:var(--table-header);font-weight:600;text-align:left;}"
"th[colspan]{text-align:center;width:100%%;}"
"td{cursor:pointer;transition:opacity 0.15s ease;}"
"td:hover:not(:empty){opacity:0.8;}"
".fwd-none{background-color:var(--fwd-none);}"
".fwd-queued{background-color:var(--fwd-queued);}"
".fwd-sent{background-color:var(--fwd-sent);}"
"tbody tr:nth-child(even){background:var(--table-stripe);}"
"tbody tr:hover{background:var(--surface-soft);transition:background 0.15s ease;}"
".status-legend{background:var(--surface);padding:16px;border-radius:8px;box-shadow:var(--shadow-card);margin:15px 0;font-size:14px;color:var(--text);line-height:1.6;}"
".status-legend strong{color:var(--text);display:block;margin-bottom:8px;}"
".buttons{display:flex;flex-wrap:wrap;gap:10px;margin:20px 0;}"
".buttons input,.buttons button,.buttons a button{flex:1 1 auto;min-width:140px;background:var(--primary);color:var(--on-primary);padding:12px 20px;border:none;border-radius:6px;cursor:pointer;font-size:clamp(14px,1.5vw,16px);font-weight:500;transition:background 0.15s ease;min-height:44px;}"
".buttons input:hover,.buttons button:hover,.buttons a button:hover{background:var(--primary-dark);}"
".buttons input:focus-visible,.buttons button:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"
".buttons a{text-decoration:none;}"
"@media(max-width:768px){.form-grid{grid-template-columns:1fr;gap:12px;}"
".form-row-full{grid-column:1;}"
".form-field label{font-size:13px;margin-bottom:4px;}"
".form-field input,.form-field select{min-height:44px;font-size:16px;}"
"table{font-size:13px;}"
"th,td{padding:10px 8px;}"
".buttons{flex-direction:column;gap:8px;}"
".buttons input,.buttons button,.buttons a button{width:100%;min-width:0;min-height:48px;}"
"}"
"@media(max-width:480px){body{padding:clamp(8px,2vw,12px);padding-left:max(clamp(8px,2vw,12px),env(safe-area-inset-left));}"
".form-section{padding:12px;margin:8px 0;}"
"table{font-size:12px;}"
"th,td{padding:8px 6px;}"
"}}</style>";

static char MailDetailHeader[] = "<h3>Message %d</h3><form method=post action=/Mail/Msg?%s name=Msgs>";

static char MailDetailFormFields[] = 
"<div class=form-section>"
"<div class=form-grid>"
"<div class=form-field><label>From</label><input class=uppercase name=From value=%s></div>"
"<div class=form-field><label>Sent</label><input readonly name=Sent value=\"%s\"></div>"
"<div class=form-field><label>Type</label><select name=Type><option %s value=B>B</option><option %s value=P>P</option><option %s value=T>T</option></select></div>"
"<div class=form-field><label>To</label><input class=uppercase name=To value=%s></div>"
"<div class=form-field><label>Received</label><input readonly name=RX value=\"%s\"></div>"
"<div class=form-field><label>Status</label><select name=Status><option %s value=N>N</option><option %s value=Y>Y</option><option %s value=F>F</option><option %s value=K>K</option><option %s value=H>H</option><option %s value=D>D</option><option %s value=$>$</option></select></div>"
"<div class=form-field><label>BID</label><input class=uppercase name=BID value=\"%s\"></div>"
"<div class=form-field><label>Last Changed</label><input readonly name=LastChange value=\"%s\"></div>"
"<div class=form-field><label>Size</label><input readonly name=Size value=%d></div>"
"%s"
"<div class=\"form-field form-row-full\"><label>VIA</label><input class=uppercase name=VIA value=%s></div>"
"<div class=\"form-field form-row-full\"><label>Title</label><input name=Title value=\"%s\"></div>"
"</div>"
"</div>";

static char MailDetailButtons[] = 
"<div class=form-section>"
"<div class=buttons>"
"<input onclick=editmsg(\"EditM?%s?%d\") value=\"Edit Text\" type=button>"
"<input onclick=save(this.form) value=Save type=button>"
"<a href=/Mail/SaveMessage?%s><button type=button>Download</button></a>"
"<a href=/Mail/SaveAttachment?%s><button type=button %s>Save Attachment</button></a>"
"</div>"
"<div class=status-legend><strong>Message Status Legend:</strong> Green = Sent to BBS, Yellow = Queued for sending</div>"
"</div>"
"<div class=table-container><table><thead><tr><th colspan=8>BBS Forwarding Status</th></tr></thead><tbody>";

char MailDetailTail[] = "</tbody></table></div></form></body></html>";

// Local CSS de-dup fragments used by multiple templates in this file.
#define SUBMIT_BUTTON_CSS "input[type=submit]{background:var(--primary);color:var(--on-primary);padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:5px;touch-action:manipulation;min-height:44px;}input[type=submit]:hover{background:var(--primary-dark);}input[type=submit]:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"

static char WPDetailCSS[] =
"<style>"
".wp-form{margin:0;}"
".wp-box{background:var(--surface);padding:15px;border-radius:4px;margin:15px 0;box-shadow:var(--shadow-card);}"
".wp-row{display:flex;flex-wrap:wrap;margin:10px 0;gap:10px;align-items:flex-start;}"
".wp-label{flex:1 1 100px;font-weight:bold;padding-top:2px;}"
".wp-input-text{flex:2 1 200px;padding:8px;border:1px solid var(--border);border-radius:4px;}"
".wp-input-small{flex:none;width:100px;padding:8px;border:1px solid var(--border);border-radius:4px;}"
".wp-input-readonly{background:var(--surface-soft);color:var(--text);}"
".wp-actions{text-align:center;margin:20px 0;position:sticky;bottom:0;background:var(--surface);padding:12px;border-top:1px solid var(--border-light);z-index:10;}"
".wp-btn{background:var(--primary);color:var(--on-primary);padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:5px;touch-action:manipulation;min-height:44px;}"
".wp-btn:hover{background:var(--primary-dark);}"
".wp-btn:focus-visible{outline:3px solid var(--focus-ring);outline-offset:2px;}"
"</style>";

char Welcome[] = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<style>"
COMMON_CSS_VARIABLES
"h3{text-align:center;color:var(--text);}"
".section-title{text-align:center;font-family:" COMMON_FONT_TITLE ";font-size:clamp(1.25rem,1.05rem + 0.9vw,1.75rem);font-weight:700;margin:12px 0 0;color:var(--text);}"
".form-row{display:block;margin:clamp(8px,2vw,15px) 0;}.form-row label{display:block;margin-bottom:6px;font-weight:bold;font-size:14px;color:var(--text);}.form-row textarea{width:100%%;padding:8px;border:1px solid var(--border);border-radius:4px;touch-action:manipulation;font-family:inherit;font-size:clamp(14px,2vw,16px);line-height:1.4;}p{font-size:13px;color:var(--text);line-height:1.5;}"
SUBMIT_BUTTON_CSS
"input[type=submit]{font-size:clamp(14px,1.5vw,16px);}@media(max-width:768px){body{padding:clamp(10px,2vw,15px);}.form-row textarea{min-height:100px;}input[type=submit]{width:calc(50%%-5px);min-height:48px;}}@media(max-width:480px){input[type=submit]{width:100%;margin:8px 0;}}"
"</style></head><body>"
"<div class=section-title>Welcome Messages and Prompts</div>"
"<form method=post action=/Mail/Welcome?%s>"
"<div class=form-row><label>Normal User Welcome</label><textarea rows=3 name=NUWelcome>%s</textarea></div>"
"<div class=form-row><label>New User Welcome</label><textarea rows=3 name=NewWelcome>%s</textarea></div>"
"<div class=form-row><label>Expert User Welcome</label><textarea rows=3 name=ExWelcome>%s</textarea></div>"
"<div class=form-row><label>Normal User Prompt</label><textarea rows=3 name=NUPrompt>%s</textarea></div>"
"<div class=form-row><label>New User Prompt</label><textarea rows=3 name=NewPrompt>%s</textarea></div>"
"<div class=form-row><label>Expert User Prompt</label><textarea rows=1 name=ExPrompt>%s</textarea></div>"
"<div class=form-row><label>Signoff</label><textarea rows=1 name=Bye>%s</textarea></div>"
"<p>$U:Callsign of the user&nbsp; $I:First name of the user $X:Messages for user $x:Unread messages<br>"
"$L:Number of the latest message $N:Number of active messages. $Z:Last message read by user</p>"
"<input name=Save value=Save type=submit> <input name=Cancel value=Cancel type=submit></form></body></html>";

static char MsgEditPage[] = "<style>"
COMMON_CSS_VARIABLES
"h3{text-align:center;margin-bottom:30px;}"
"textarea{width:100%%;padding:8px;border:1px solid var(--border);border-radius:4px;touch-action:manipulation;font-family: " COMMON_FONT_MONO ";font-size:12px;line-height:1.4;resize:vertical;}"
SUBMIT_BUTTON_CSS
"@media(max-width:768px){textarea{font-size:13px;min-height:200px;}input[type=submit]{width:calc(50%-5px);}}@media(max-width:480px){input[type=submit]{width:100%;margin:8px 0;}}"
"</style><h3>Edit Message</h3>"
"<form method=post action=EMSave?%s>"
"<textarea cols=90 rows=33 name=Msg>%s</textarea><br><br>"
"<input name=Save value=Save type=submit><input name=Cancel value=Cancel type=submit><br></form></body></html>";

// Split WPDetail into 5 smaller templates to avoid ARM printf_positional limits
// NOTE: This is AJAX-injected content; no <body> or </html> closing tags.
// A small fragment-local <style> is prepended so class-based markup is self-contained.
static char WPDetailOpen[] = "<h3>White Pages Record</h3><form method=post action=/Mail/WP?%s class=wp-form>";

static char WPDetailSection1[] = 
"<div class=wp-box>"
"<div class=wp-row>"
"<label class=wp-label>Call</label>"
"<input readonly size=10 class=\"wp-input-text wp-input-readonly\" value=\"%s\"></div>";

static char WPDetailSection2[] = 
"<div class=wp-row>"
"<label class=wp-label>Name</label>"
"<input class=wp-input-text name=Name value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>Home BBS 1</label>"
"<input class=wp-input-text name=Home1 value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>Home BBS 2</label>"
"<input class=wp-input-text name=Home2 value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>QTH 1</label>"
"<input class=wp-input-text name=QTH1 value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>QTH 2</label>"
"<input class=wp-input-text name=QTH2 value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>ZIP 1</label>"
"<input class=wp-input-small name=ZIP1 value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>ZIP 2</label>"
"<input class=wp-input-small name=ZIP2 value=\"%s\"></div>"
"</div></div>";

static char WPDetailSection3[] = 
"<div class=wp-box>"
"<div class=wp-row>"
"<label class=wp-label>Last Seen</label>"
"<input readonly class=\"wp-input-text wp-input-readonly\" value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>Last Modified</label>"
"<input readonly class=\"wp-input-text wp-input-readonly\" value=\"%s\"></div>"
"<div class=wp-row>"
"<label class=wp-label>Type</label>"
"<input class=wp-input-small name=Type value=\"%c\"></div>"
"<div class=wp-row>"
"<label class=wp-label>Changed</label>"
"<input readonly class=\"wp-input-small wp-input-readonly\" value=\"%d\"></div>"
"<div class=wp-row>"
"<label class=wp-label>Seen</label>"
"<input readonly class=\"wp-input-small wp-input-readonly\" value=\"%d\"></div>"
"</div></div>";

static char WPDetailButtons[] = 
"<div class=wp-actions>"
"<input onclick=save(this.form) value=Save class=wp-btn type=button> "
"<input onclick=del(this.form) value=Delete class=wp-btn type=button> "
"<input name=Cancel value=Cancel class=wp-btn type=submit></div></form>";


static char LostSession[] = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><style>"
COMMON_CSS_VARIABLES
"body{text-align:center;}"
SUBMIT_BUTTON_CSS
"input[type=submit]{font-size:14px;}@media(max-width:480px){input[type=submit]{width:calc(50%-5px);}input[type=submit]:only-of-type{width:100%;}}"
"</style></head><body>"
"<form method=post action=/Mail/Lost?%s>"
"Sorry, Session had been lost<br><br>&nbsp;&nbsp;&nbsp;&nbsp;"
"<input name=Submit value=Restart type=submit> <input type=submit value=Exit name=Cancel><br></form></body></html>";


char * MsgEditTemplate = NULL;
char * HousekeepingTemplate = NULL;
char * ConfigTemplate = NULL;
char * WPTemplate = NULL;
char * UserListTemplate = NULL;
char * UserDetailTemplate = NULL;
char * FwdTemplate = NULL;
char * FwdDetailTemplate = NULL;
char * WebMailTemplate = NULL;
char * WebMailMsgTemplate = NULL;
char * jsTemplate = NULL;


#ifdef LINBPQ
UCHAR * GetBPQDirectory();
#endif

static int compare(const void *arg1, const void *arg2)
{
   // Compare Calls. Fortunately call is at start of stuct

   return _stricmp(*(char**)arg1 , *(char**)arg2);
}

int SendHeader(char * Reply, char * Key)
{
	return sprintf(Reply, MailPage, BBSName, BBSName, Key, Key, Key, Key, Key, Key, Key, Key);
}


void ConvertTitletoUTF8(WebMailInfo * WebMail, char * Title, char * UTF8Title, int Len)
{
	Len = strlen(Title);

	if (WebIsUTF8(Title, Len) == FALSE)
	{
		int code = TrytoGuessCode(Title, Len);

		if (code == 437)
			Len = Convert437toUTF8(Title, Len, UTF8Title);
		else if (code == 1251)
			Len = Convert1251toUTF8(Title, Len, UTF8Title);
		else
			Len = Convert1252toUTF8(Title, Len, UTF8Title);

		UTF8Title[Len] = 0;
	}
	else
		strcpy(UTF8Title, Title);
}

BOOL GotFirstMessage = 0;

void ProcessMailHTTPMessage(struct HTTPConnectionInfo * Session, char * Method, char * URL, char * input, char * Reply, int * RLen, int InputLen, char * Token)
{
	char * Context = 0, * NodeURL;
	int ReplyLen;
	BOOL LOCAL = FALSE;
	char * Key;
	char Appl = 'M';

	if (URL[0] == 0 || Method == NULL)
		return;

	if (strstr(input, "Host: 127.0.0.1"))
		LOCAL = TRUE;

	if (Session->TNC == (void *)1)					// Re-using an address as a flag
		LOCAL = TRUE;

	NodeURL = strtok_s(URL, "?", &Context);

	Key = Session->Key;

	if (_memicmp(URL, "/WebMail", 8) == 0)
	{
		// Pass All Webmail messages to Webmail
			
		ProcessWebMailMessage(Session, Context, LOCAL, Method, NodeURL, input, Reply, RLen, InputLen);
		return;

	}


	if (_memicmp(URL, "/Mail/API/v1/", 13) == 0)
	{
		*RLen = MailAPIProcessHTTPMessage(Session, Reply, Method, URL, input, LOCAL, Context, Token);
		return;
	}

	// There is a problem if Mail is reloaded without reloading the node

	if (GotFirstMessage == 0)
	{
		if (_stricmp(NodeURL, "/Mail/Header") ==  0 || _stricmp(NodeURL, "/Mail/Lost") == 0)
		{
			*RLen = SendHeader(Reply, Session->Key);
		}
		else
		{
			*RLen = sprintf(Reply, "<html><script>window.location.href = '/Mail/Header?%s';</script>", Session->Key);
		}
		
		GotFirstMessage = 1;
		return;
	}

	
	if (strcmp(Method, "POST") == 0)
	{	
		if (_stricmp(NodeURL, "/Mail/Header") == 0)
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}


		if (_stricmp(NodeURL, "/Mail/Config") == 0)
		{
			NodeURL[strlen(NodeURL)] = ' ';				// Undo strtok
			ProcessConfUpdate(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/UI") == 0)
		{
			NodeURL[strlen(NodeURL)] = ' ';				// Undo strtok
			ProcessUIUpdate(Session, input, Reply, RLen, Key);
			return ;
		}
		if (_stricmp(NodeURL, "/Mail/FwdCommon") == 0)
		{
			SaveFwdCommon(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/DisSession") == 0)
		{
			ProcessDisUser(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/UserDetails") == 0)
		{
			char * param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				Session->User = LookupCall(param+4);
				if (Session->User)
				{
					* RLen = SendUserDetails(Session, Reply, Key); 
					return;
				}
			}
		}


		if (_stricmp(NodeURL, "/Mail/UserSave") == 0)
		{
			ProcessUserUpdate(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/MsgDetails") == 0)
		{
			char * param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				int Msgno = atoi(param + 4);
				struct MsgInfo * Msg = FindMessageByNumber(Msgno);

				Session->Msg = Msg;				// Save current Message
	
				* RLen = SendMessageDetails(Msg, Reply, Key); 
				return;
			}
		}

		if (_stricmp(NodeURL, "/Mail/MsgSave") == 0)
		{
			ProcessMsgUpdate(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/EMSave") == 0)
		{
			//	Save Message Text

			SaveMessageText(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/MsgAction") == 0)
		{
			ProcessMsgAction(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/MsgFwdUpdate") == 0)
		{
			ProcessMsgFwdUpdate(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/Welcome") == 0)
		{
			SaveWelcome(Session, input, Reply, RLen, Key);
			return;
		}
		if (_stricmp(NodeURL, "/Mail/HK") == 0)
		{
			SaveHousekeeping(Session, input, Reply, RLen, Key);
			return;
		}
		if (_stricmp(NodeURL, "/Mail/WPDetails") == 0)
		{
			char * param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				WPRec * WP = LookupWP(param+4);
				Session->WP = WP;				// Save current Message
	
				* RLen = SendWPDetails(WP, Reply, Key); 
				return;
			}
		}
		if (_stricmp(NodeURL, "/Mail/WPSave") == 0)
		{
			SaveWP(Session, input, Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/MsgInfo.txt") == 0)
		{
			int n, len = 0;
			char  * FF = "", *FT = "", *FB = "", *FV = "";
			char * param, * ptr1, *ptr2;
			struct MsgInfo * Msg;
			char UCto[80];
			char UCfrom[80];
			char UCvia[80];
			char UCbid[80];

			// Get filter string

			param = strstr(input, "\r\n\r\n");	// End of headers
			

			if (param)
 			{
				ptr1 = param + 4;
				ptr2 = strchr(ptr1, '|');
				if (ptr2){*(ptr2++) = 0; FF = ptr1; ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FT = ptr1;ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FV = ptr1;ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FB = ptr1;ptr1 = ptr2;}
			}

			if (FT[0])
				_strupr(FT);
			if (FF[0])
				_strupr(FF);
			if (FV[0])
				_strupr(FV);
			if (FB[0])
				_strupr(FB);

			for (n = NumberofMessages; n >= 1; n--)
			{
				Msg = MsgHddrPtr[n];

				strcpy(UCto, Msg->to);
				strcpy(UCfrom, Msg->from);
				strcpy(UCvia, Msg->via);
				strcpy(UCbid, Msg->bid);

				_strupr(UCto);
				_strupr(UCfrom);
				_strupr(UCvia);
				_strupr(UCbid);

				if ((!FT[0] || strstr(UCto, FT)) &&
					(!FF[0] || strstr(UCfrom, FF)) &&
					(!FB[0] || strstr(UCbid, FB)) &&
					(!FV[0] || strstr(UCvia, FV)))
				{
					len += sprintf(&Reply[len], "%d|", Msg->number);
				}
			} 
			*RLen = len;
			return;
		}
		
		if (_stricmp(NodeURL, "/Mail/UserList.txt") == 0)
		{
			SendUserSelectPage(Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/FwdList.txt") == 0)
		{
			SendFwdSelectPage(Reply, RLen, Key);
			return;
		}

		if (_stricmp(NodeURL, "/Mail/FwdDetails") == 0)
		{
			char * param;
			
			param = strstr(input, "\r\n\r\n");	// End of headers

			if (param)
			{		
				Session->User = LookupCall(param+4);
				if (Session->User)
				{
					SendFwdDetails(Session->User, Reply, RLen, Key); 
					return;
				}
			}
		}

		if (_stricmp(NodeURL, "/Mail/FWDSave") == 0)
		{
			SaveFwdDetails(Session, input, Reply, RLen, Key);
			return ;
		}

		// End of POST section
	}

	if (strstr(NodeURL, "webscript.js"))
	{
		if (jsTemplate)
			free(jsTemplate);
		
		jsTemplate = GetTemplateFromFile(1, "webscript.js");

		ReplyLen = sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
			"Cache-Control: max-age=900\r\nContent-Type: text/javascript\r\n\r\n%s", (int)strlen(jsTemplate), jsTemplate);
		*RLen = ReplyLen;
		return;
	}


	if (_stricmp(NodeURL, "/Mail/Header") == 0)
	{
		*RLen = SendHeader(Reply, Session->Key);
 		return;
	}
	
	if (_stricmp(NodeURL, "/Mail/all.html") == 0)
	{
		*RLen = SendHeader(Reply, Session->Key);
 		return;
	}

	if (_stricmp(NodeURL, "/Mail/Status") == 0 ||
		_stricmp(NodeURL, "/Mail/DisSession") == 0)		// Sent as POST by refresh timer for some reason
	{
		SendStatusPage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/Conf") == 0)
	{
		if (ConfigTemplate)
			free(ConfigTemplate);

		ConfigTemplate = GetTemplateFromFile(7, "MainConfig.txt");

		SendConfigPage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/FWD") == 0)
	{
		if (FwdTemplate)
			free(FwdTemplate);

		FwdTemplate = GetTemplateFromFile(4, "FwdPage.txt");

		if (FwdDetailTemplate)
			free(FwdDetailTemplate);

		FwdDetailTemplate = GetTemplateFromFile(3, "FwdDetail.txt");

		SendFwdMainPage(Reply, RLen, Key);
		return;
	}
	if (_stricmp(NodeURL, "/Mail/Wel") == 0)
	{
		SendWelcomePage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/Users") == 0)
	{
		if (UserListTemplate)
			free(UserListTemplate);

		UserListTemplate = GetTemplateFromFile(4, "UserPage.txt");

		if (UserDetailTemplate)
			free(UserDetailTemplate);

		UserDetailTemplate = GetTemplateFromFile(4, "UserDetail.txt");

		*RLen = sprintf(Reply, UserListTemplate, Key, Key,
			BBSName,
			Key, Key, Key, Key, Key, Key, Key, Key);
	
		return;
	}

	if (_stricmp(NodeURL, "/Mail/SaveMessage") == 0)
	{
		struct MsgInfo * Msg = Session->Msg;
		char * MailBuffer;

		int Files = 0;
		int BodyLen;
		char * ptr;
		int WriteLen=0;
		char Hddr[1000];
		char FullTo[100];

		MailBuffer = ReadMessageFile(Msg->number);
		BodyLen = Msg->length;

		ptr = MailBuffer;

		if (_stricmp(Msg->to, "RMS") == 0)
			 sprintf(FullTo, "RMS:%s", Msg->via);
		else
		if (Msg->to[0] == 0)
			sprintf(FullTo, "smtp:%s", Msg->via);
		else
			strcpy(FullTo, Msg->to);

		sprintf(Hddr, "From: %s%s\r\nTo: %s\r\nType/Status: %c%c\r\nDate/Time: %s\r\nBid: %s\r\nTitle: %s\r\n\r\n",
			Msg->from, Msg->emailfrom, FullTo, Msg->type, Msg->status, FormatDateAndTime((time_t)Msg->datecreated, FALSE), Msg->bid, Msg->title);

		if (Msg->B2Flags & B2Msg)
		{
			// Remove B2 Headers (up to the File: Line)
			
			char * bptr;
			bptr = strstr(ptr, "Body:");
			if (bptr)
			{
				BodyLen = atoi(bptr + 5);
				bptr = strstr(bptr, "\r\n\r\n");

				if (bptr)
					ptr = bptr+4;
			}
		}

		ptr[BodyLen] = 0;

		sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Disposition: attachment; filename=\"SavedMsg%05d.txt\" \r\n\r\n",
			(int)(strlen(Hddr) + strlen(ptr)), Msg->number);	
		strcat(Reply, Hddr);
		strcat(Reply, ptr);

		*RLen = (int)strlen(Reply);

		free(MailBuffer);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/SaveAttachment") == 0)
	{
		struct MsgInfo * Msg = Session->Msg;
		char * MailBuffer;

		int Files = 0, i;
		int BodyLen;
		char * ptr;
		int WriteLen=0;
		char FileName[100][MAX_PATH] = {""};
		int FileLen[100];
		char Noatt[] = "Message has no attachments";


		MailBuffer = ReadMessageFile(Msg->number);
		BodyLen = Msg->length;

		if ((Msg->B2Flags & Attachments) == 0)
		{
			sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s",
				(int)strlen(Noatt), Noatt);
			*RLen = (int)strlen(Reply);

			free(MailBuffer);
			return;
		}
		
		ptr = MailBuffer;

		while(ptr && *ptr != 13)
		{
			char * ptr2 = strchr(ptr, 10);	// Find CR

			if (memcmp(ptr, "Body: ", 6) == 0)
			{
				BodyLen = atoi(&ptr[6]);
			}

			if (memcmp(ptr, "File: ", 6) == 0)
			{
				char * ptr1 = strchr(&ptr[6], ' ');	// Find Space

				FileLen[Files] = atoi(&ptr[6]);

				memcpy(FileName[Files++], &ptr1[1], (ptr2-ptr1 - 2));
			}
				
			ptr = ptr2;
			ptr++;
		}

		ptr += 4;			// Over Blank Line and Separator
		ptr += BodyLen;		// to first file

		if (Files == 0)
		{
			sprintf(Reply, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s",
				(int)strlen(Noatt), Noatt);
			*RLen = (int)strlen(Reply);
			free(MailBuffer);
			return;
		}

		*RLen = 0;

		//	For now only handle first

		i = 0;

//		for (i = 0; i < Files; i++)
		{
			int Len = sprintf(&Reply[*RLen], "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Disposition: attachment; filename=\"%s\" \r\n\r\n",
				FileLen[i], FileName[i]);

			memcpy(&Reply[Len + *RLen], ptr, FileLen[i]);
	
			*RLen += (Len + FileLen[i]);

			ptr += FileLen[i];
			ptr +=2;				// Over separator - I don't think there should be one
		}

		free(MailBuffer);
		return;
	}


	if (_stricmp(NodeURL, "/Mail/Msgs") == 0)
	{
		struct UserInfo * USER = NULL;
		int PageLen;

		if (MsgEditTemplate)
			free(MsgEditTemplate);

		MsgEditTemplate = GetTemplateFromFile(2, "MsgPage.txt");

		// Refresh BBS No to BBS list

		MaxBBS = 0;

		for (USER = BBSChain; USER; USER = USER->BBSNext)
		{
			int n = USER->BBSNumber;
			BBSLIST[n] = USER;
			if (n > MaxBBS)
				MaxBBS = n;
		}

		PageLen = 334 + (MaxBBS / 8) * 24;

		if (MsgEditTemplate)
		{
			int len =sprintf(Reply, MsgEditTemplate, PageLen, PageLen, Key, Key, Key, Key, Key,
				BBSName,
				Key, Key, Key, Key, Key, Key, Key, Key);
			*RLen = len;
			return;
		}




	}

	if (_stricmp(NodeURL, "/Mail/EditM") == 0)
	{
		// Edit Message
		// URL format: /Mail/EditM?Key?MessageNumber

		char * MsgBytes;
		char * MsgnoStr;
		char * MyContext = NULL;
		int Msgno = 0;
		struct MsgInfo * Msg = NULL;

		// Parse URL parameters: Context contains "Key?MessageNumber"
		if (Context)
		{
			// Skip the Key and get the message number
			MsgnoStr = strtok_s(Context, "?", &MyContext);  // First token is Key
			if (MyContext && *MyContext)  // Second token is message number
			{
				Msgno = atoi(MyContext);
				if (Msgno > 0)
					Msg = FindMessageByNumber(Msgno);
			}
		}

		// Fall back to session if URL parsing didn't work
		if (!Msg && Session->Msg)
			Msg = Session->Msg;

		if (Msg)
		{
			Session->Msg = Msg;
			MsgBytes = ReadMessageFile(Msg->number);

			if (MsgBytes)
			{
				*RLen = sprintf(Reply, MsgEditPage, Key, MsgBytes);
				free (MsgBytes);
			}
			else
				*RLen = sprintf(Reply, MsgEditPage, Key, "Message Not Found");
		}
		else
			*RLen = sprintf(Reply, MsgEditPage, Key, "Message Not Found");

		return;
	}

	if (_stricmp(NodeURL, "/Mail/HK") == 0)
	{
		if (HousekeepingTemplate)
			free(HousekeepingTemplate);

		HousekeepingTemplate = GetTemplateFromFile(2, "Housekeeping.txt");

		SendHouseKeeping(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/WP") == 0)
	{
		if (WPTemplate)
			free(WPTemplate);

		WPTemplate = GetTemplateFromFile(1, "WP.txt");

		if (WPTemplate)
		{
			int len =sprintf(Reply, WPTemplate, Key, Key, BBSName, Key, Key,
				Key, Key, Key, Key, Key, Key);
			*RLen = len;
			return;
		}

		return;
	}

	if (_stricmp(NodeURL, "/Mail/WPInfo.txt") == 0)
	{
		int i = 0, n, len = 0;
		WPRec * WP[10000]; 

		// Get array of addresses

		for (n = 1; n <= NumberofWPrecs; n++)
		{
			WP[i++] = WPRecPtr[n];
			if (i > 9999) break;
		}

		qsort((void *)WP, i, sizeof(void *), compare);

		for (i=0; i < NumberofWPrecs; i++)
		{
			len += sprintf(&Reply[len], "%s|", WP[i]->callsign);
		}

		*RLen = len;
		return;
	}


	ReplyLen = sprintf(Reply, MailSignon, BBSName, BBSName);
	*RLen = ReplyLen;

}

int SendWPDetails(WPRec * WP, char * Reply, char * Key)
{
	int len = 0, nwritten;
	char D1[80], D2[80];
	
	// WP details are injected into the /Mail/WP page via AJAX.
	// Do not prepend the global header/menu here.
	// Return content only - CSS styling is already in the main page
	len = 0;
	
	if (!Key) Key = "";
	
	if (WP)
	{
		strcpy(D1, FormatDateAndTime(WP->last_modif, FALSE));
		strcpy(D2, FormatDateAndTime(WP->last_seen, FALSE));

		// Chunk 1: Opening with Key
		nwritten = snprintf(&Reply[len], 65536 - len, "%s", WPDetailCSS);
		if (nwritten < 0 || nwritten >= (65536 - len)) return len;
		len += nwritten;

		// Chunk 2: Opening with Key
		nwritten = snprintf(&Reply[len], 65536 - len, WPDetailOpen, Key);
		if (nwritten < 0 || nwritten >= (65536 - len)) return len;
		len += nwritten;

		// Chunk 3: Call field (1 specifier)
		nwritten = snprintf(&Reply[len], 65536 - len, WPDetailSection1, WP->callsign);
		if (nwritten < 0 || nwritten >= (65536 - len)) return len;
		len += nwritten;

		// Chunk 4: Contact info (7 specifiers: name, home1, home2, qth1, qth2, zip1, zip2)
		nwritten = snprintf(&Reply[len], 65536 - len, WPDetailSection2, 
			WP->name,
			WP->first_homebbs, WP->secnd_homebbs,
			WP->first_qth, WP->secnd_qth,
			WP->first_zip, WP->secnd_zip);
		if (nwritten < 0 || nwritten >= (65536 - len)) return len;
		len += nwritten;

		// Chunk 5: Metadata (5 specifiers: seen, modif, type, changed, seen)
		nwritten = snprintf(&Reply[len], 65536 - len, WPDetailSection3,
			D1, D2,
			WP->Type,
			WP->changed, 
			WP->seen);
		if (nwritten < 0 || nwritten >= (65536 - len)) return len;
		len += nwritten;

		// Chunk 6: Buttons and closing form
		nwritten = snprintf(&Reply[len], 65536 - len, "%s", WPDetailButtons);
		if (nwritten < 0 || nwritten >= (65536 - len)) return len;
		len += nwritten;
	}
	return(len);	
}
VOID SaveWP(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	WPRec * WP = Session->WP;
	char * input, * ptr1, * ptr2;
	int n;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}

		if (strcmp(input + 4, "Delete") == 0)
		{			
			for (n = 1; n <= NumberofWPrecs; n++)
			{
				if (Session->WP == WPRecPtr[n])
					break;
			}
		
			if (n <= NumberofWPrecs)
			{
				WP = Session->WP;

				for (n = n; n < NumberofWPrecs; n++)
				{
					WPRecPtr[n] = WPRecPtr[n+1];		// move down all following entries
				}
	
				NumberofWPrecs--;
	
				free(WP);
			
				SaveWPDatabase();

				Session->WP = WPRecPtr[1];
			}
			SendWPSelectPage(Reply, RLen, Session->Key);
 			return;
		}
	}
	if (input && WP)
	{
		ptr1 = input + 4;
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 12) ptr1[12] = 0;strcpy(WP->name, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 40) ptr1[40] = 0;strcpy(WP->first_homebbs, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 40) ptr1[40] = 0;strcpy(WP->secnd_homebbs, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 30) ptr1[30] = 0;strcpy(WP->first_qth, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 30) ptr1[30] = 0;strcpy(WP->secnd_qth, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 8) ptr1[8] = 0;strcpy(WP->first_zip, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;if (strlen(ptr1) > 8) ptr1[8] = 0;strcpy(WP->secnd_zip, ptr1);ptr1 = ptr2;}

	//	GetParam(input, "BBSCall=", BBSName);


/*
	GetDlgItemText(hDlg, IDC_WPNAME, WP->name, 13);
	GetDlgItemText(hDlg, IDC_HOMEBBS1, WP->first_homebbs, 41);
	GetDlgItemText(hDlg, IDC_HOMEBBS2, WP->first_homebbs, 41);
	GetDlgItemText(hDlg, IDC_QTH1, WP->first_qth, 31);
	GetDlgItemText(hDlg, IDC_QTH2, WP->secnd_qth, 31);
	GetDlgItemText(hDlg, IDC_ZIP1, WP->first_zip, 31);
	GetDlgItemText(hDlg, IDC_ZIP2, WP->secnd_zip, 31);
		WP->seen = GetDlgItemInt(hDlg, IDC_SEEN, &OK1, FALSE);
*/
	
		WP->last_modif = time(NULL);
		WP->Type = 'U';
		WP->changed = 1;

		SaveWPDatabase();

		*RLen = SendWPDetails(WP, Reply, Key);
	}
}

VOID SendWPSelectPage(char * Reply, int * ReplyLen, char * Key)
{
	WPRec * WP;
	int i = 0, n;
	int Len = 0;
	WPRec * WPs[10000];

	for (n = 1; n <= NumberofWPrecs; n++)
	{
		WPs[i++] = WPRecPtr[n];
		if (i > 9999) break;
	}

	qsort((void *)WPs, i, sizeof(void *), compare);

	for (n = 0; n < NumberofWPrecs; n++)
	{
		WP = WPs[n];
		Len += sprintf(&Reply[Len], "%s|", WP->callsign);
	}

	*ReplyLen = Len;
}


int SendMessageDetails(struct MsgInfo * Msg, char * Reply, char * Key)
{	
	const int ReplyCap = 250000;
	int BBSNo = 1, x, y, len = 0;
	char D1[80], D2[80], D3[80];
	struct UserInfo * USER;
	int i = 0, n;
	struct UserInfo * bbs[NBBBS+2] = {0}; 

	// Message details are injected into the /Mail/Msgs page via AJAX.
	// Do not prepend the global header/menu here.
	len = 0;

	if (Msg)
	{
		char EmailFromLine[256] = "";
		char MsgFrom[sizeof(Msg->from) + 1];
		char MsgTo[sizeof(Msg->to) + 1];
		char MsgTitle[sizeof(Msg->title) + 1];
		char MsgBid[sizeof(Msg->bid) + 1];
		char MsgVia[sizeof(Msg->via) + 1];
		char MsgEmailFrom[sizeof(Msg->emailfrom) + 1];
		int nwritten;

		memcpy(MsgFrom, Msg->from, sizeof(Msg->from));
		MsgFrom[sizeof(Msg->from)] = 0;
		memcpy(MsgTo, Msg->to, sizeof(Msg->to));
		MsgTo[sizeof(Msg->to)] = 0;
		memcpy(MsgTitle, Msg->title, sizeof(Msg->title));
		MsgTitle[sizeof(Msg->title)] = 0;
		memcpy(MsgBid, Msg->bid, sizeof(Msg->bid));
		MsgBid[sizeof(Msg->bid)] = 0;
		memcpy(MsgVia, Msg->via, sizeof(Msg->via));
		MsgVia[sizeof(Msg->via)] = 0;
		memcpy(MsgEmailFrom, Msg->emailfrom, sizeof(Msg->emailfrom));
		MsgEmailFrom[sizeof(Msg->emailfrom)] = 0;

		strcpy(D1, FormatDateAndTime((time_t)Msg->datecreated, FALSE));
		strcpy(D2, FormatDateAndTime((time_t)Msg->datereceived, FALSE));
		strcpy(D3, FormatDateAndTime((time_t)Msg->datechanged, FALSE));

		// Build Email From field in new grid format
		if (Msg->emailfrom[0])
			snprintf(EmailFromLine, sizeof(EmailFromLine), 
				"<div class=\"form-field form-row-full\"><label>Email From</label><input name=EFROM value=\"%s\"></div>", 
				MsgEmailFrom);

		// Ensure Key is never NULL to prevent SIGSEGV in snprintf
		if (!Key) Key = "";
		
		// Build response incrementally to avoid large format string limits on ARM
		// 1. CSS (no format specifiers)
		len += snprintf(&Reply[len], ReplyCap - len, "%s", MailDetailCSS);
		if (len >= ReplyCap - 1) return len;
		
		// 2. Header (message number and form action)
		nwritten = snprintf(&Reply[len], ReplyCap - len, MailDetailHeader, Msg->number, Key);
		if (nwritten < 0 || nwritten >= (ReplyCap - len)) return len;
		len += nwritten;
		
		// 3. Form fields in new grid layout order
		// Row 1: From, Sent, Type
		// Row 2: To, Received, Status
		// Row 3: BID, Last Changed, Size
		// Row 4: Email From (if present)
		// Row 5: VIA
		// Row 6: Title
		nwritten = snprintf(&Reply[len], ReplyCap - len, MailDetailFormFields,
			MsgFrom, D1,
			(Msg->type == 'B')?sel:"",
			(Msg->type == 'P')?sel:"",
			(Msg->type == 'T')?sel:"",
			MsgTo, D2,
			(Msg->status == 'N')?sel:"",
			(Msg->status == 'Y')?sel:"",
			(Msg->status == 'F')?sel:"",
			(Msg->status == 'K')?sel:"",
			(Msg->status == 'H')?sel:"",
			(Msg->status == 'D')?sel:"",
			(Msg->status == '$')?sel:"",
			MsgBid, D3, Msg->length,
			EmailFromLine, MsgVia, MsgTitle);
		if (nwritten < 0 || nwritten >= (ReplyCap - len)) return len;
		len += nwritten;
		
		// 4. Buttons and links
		nwritten = snprintf(&Reply[len], ReplyCap - len, MailDetailButtons,
			Key, Msg->number, Key, Key,
			(Msg->B2Flags & Attachments)?"":"disabled");
		if (nwritten < 0 || nwritten >= (ReplyCap - len)) return len;
		len += nwritten;

		// Get a sorted list of BBS records

		for (n = 1; n <= NumberofUsers; n++)
		{
			USER = UserRecPtr[n];

			if ((USER->flags & F_BBS))
				if (USER->BBSNumber)
					bbs[i++] = USER;
		}

		qsort((void *)bbs, i, sizeof(void *), compare );

		n = 0;
		
		for (y = 0; y < NBBBS/8; y++)
		{
			if (len >= ReplyCap - 1)
				break;

			nwritten = snprintf(&Reply[len], ReplyCap - len, "<tr>");
			if (nwritten < 0)
				break;
			if (nwritten >= (ReplyCap - len))
			{
				len = ReplyCap - 1;
				break;
			}
			len += nwritten;
			for (x= 0; x < 8; x++)
			{
				char * Colour  = NotThisOne;
				const char * CellClass = "fwd-none";

				if (bbs[n])
				{
					if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
						if (check_fwd_bit(Msg->fbbs, bbs[n]->BBSNumber))
							Colour = ToSend;
					if (memcmp(Msg->forw, zeros, NBMASK) != 0)
						if (check_fwd_bit(Msg->forw, bbs[n]->BBSNumber))
							Colour = Sent;

					if (Colour == ToSend)
						CellClass = "fwd-queued";
					else if (Colour == Sent)
						CellClass = "fwd-sent";

					nwritten = snprintf(&Reply[len], ReplyCap - len, "<td class=\"%s\" onclick=ck(\"%d\")>%.6s</td>",
						CellClass, bbs[n]->BBSNumber, bbs[n]->Call);
					if (nwritten < 0)
						break;
					if (nwritten >= (ReplyCap - len))
					{
						len = ReplyCap - 1;
						break;
					}
					len += nwritten;
				}
				else
				{
					nwritten = snprintf(&Reply[len], ReplyCap - len, "<td>&nbsp;</td>");
					if (nwritten < 0)
						break;
					if (nwritten >= (ReplyCap - len))
					{
						len = ReplyCap - 1;
						break;
					}
					len += nwritten;
				}

				n++;

				if (len >= ReplyCap - 1)
					break;
			}
			if (len >= ReplyCap - 1)
				break;

			nwritten = snprintf(&Reply[len], ReplyCap - len, "</tr>");
			if (nwritten < 0)
				break;
			if (nwritten >= (ReplyCap - len))
			{
				len = ReplyCap - 1;
				break;
			}
			len += nwritten;
			if (n > i)
				break;
		}

		if (len < ReplyCap - 1)
		{
			nwritten = snprintf(&Reply[len], ReplyCap - len, "%s", MailDetailTail);
			if (nwritten > 0)
			{
				if (nwritten >= (ReplyCap - len))
					len = ReplyCap - 1;
				else
					len += nwritten;
			}
		}
	}
	return(len);	
}

char ** GetMultiStringInput(char * input, char * key)
{
	char MultiString[16384] = "";

	GetParam(input, key, MultiString);

	if (MultiString[0] == 0)
		return NULL;

	return SeparateMultiString(MultiString, TRUE);
}

char **	SeparateMultiString(char * MultiString, BOOL NoToUpper)
{
	char * ptr1 = MultiString;
	char * ptr2 = NULL;
	char * DecodedString;
	char ** Value;
	int Count = 0;
	char c;
	char * ptr;

	ptr2 = zalloc(strlen(MultiString) + 1);
	DecodedString = ptr2;

	// Input has crlf or lf - replace with |

	while (*ptr1)
	{
		c = *(ptr1++);

		if (c == 13)
			continue;

		if (c == 10)
		{
			*ptr2++ = '|';
		}
		else
			*(ptr2++) = c;
	}

	// Convert to string array

	Value = zalloc(sizeof(void *));				// always NULL entry on end even if no values
	Value[0] = NULL;

	ptr = DecodedString;

	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, '|');
			
		if (ptr1)
			*(ptr1++) = 0;

		if (strlen(ptr))
		{
			Value = realloc(Value, (Count+2) * sizeof(void *));
			if (_memicmp(ptr, "file ", 5) == 0 || NoToUpper)
				Value[Count++] = _strdup(ptr);
			else
				Value[Count++] = _strupr(_strdup(ptr));
		}
		ptr = ptr1;
	}

	Value[Count] = NULL;
	return Value;
}

VOID GetMallocedParam(char * input, char * key, char ** value)
{
	char Param[32768] = "";

	GetParam(input, key, Param);

	if (Param[0])
	{
		free(*value);
		*value = _strdup(Param);
	}
}

VOID GetParam(char * input, char * key, char * value)
{
	char * ptr = strstr(input, key);
	char Param[32768];
	char * ptr1, * ptr2;
	char c;

	if (ptr)
	{
		ptr2 = strchr(ptr, '&');
		if (ptr2) *ptr2 = 0;
		strcpy(Param, ptr + strlen(key));
		if (ptr2) *ptr2 = '&';					// Restore string

		// Undo any % transparency

		ptr1 = Param;
		ptr2 = Param;

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

				*(ptr2++) = m * 16 + n;
			}
			else if (c == '+')
				*(ptr2++) = ' ';
			else
				*(ptr2++) = c;

			c = *(ptr1++);
		}

		*(ptr2++) = 0;

		strcpy(value, Param);
	}
}

VOID GetCheckBox(char * input, char * key, int * value)
{
	char * ptr = strstr(input, key);
	if (ptr)
		*value = 1;
	else
		*value = 0;
}

	
VOID * GetOverrideFromString(char * input)
{
	char * ptr1;
	char * MultiString = NULL;
	char * ptr = input;
	int Count = 0;
	struct Override ** Value;
	char * Val;

	Value = zalloc(sizeof(void *));				// always NULL entry on end even if no values
	Value[0] = NULL;
	
	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, 13);
			
		if (ptr1)
		{
			*(ptr1) = 0;
			ptr1 += 2;
		}
		Value = realloc(Value, (Count+2) * sizeof(void *));
		Value[Count] = zalloc(sizeof(struct Override));
		Val = strlop(ptr, ',');
		if (Val == NULL)
			break;

		Value[Count]->Call = _strupr(_strdup(ptr));
		Value[Count++]->Days = atoi(Val);
		ptr = ptr1;
	}

	Value[Count] = NULL;
	return Value;
}




VOID SaveHousekeeping(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;
	char Temp[80];
	struct tm *tm;
	time_t now;
	
	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "RunNow="))
		{
			DoHouseKeeping(FALSE);
			SendHouseKeeping(Reply, RLen, Key);
			return;
		}
		if (strstr(input, "Cancel=Cancel"))
		{
			SendHouseKeeping(Reply, RLen, Key);
 			return;
		}

		GetParam(input, "MTTime=", Temp);
		MaintTime = atoi(Temp);
		GetParam(input, "MTInt=", Temp);
		MaintInterval = atoi(Temp);
		GetParam(input, "MAXMSG=", Temp);
		MaxMsgno = atoi(Temp);
		GetParam(input, "BIDLife=", Temp);
		BidLifetime= atoi(Temp);
		GetParam(input, "MaxAge=", Temp);
		MaxAge = atoi(Temp);
		GetParam(input, "LogLife=", Temp);
		BBSLogAge = atoi(Temp);
		GetParam(input, "UserLife=", Temp);
		UserLifetime= atoi(Temp);

		GetCheckBox(input, "Deltobin=", &DeletetoRecycleBin);
		GetCheckBox(input, "SendND=", &SendNonDeliveryMsgs);
		GetCheckBox(input, "NoMail=", &SuppressMaintEmail);
		GetCheckBox(input, "GenTraffic=", &GenerateTrafficReport);
		GetCheckBox(input, "OvUnsent=", &OverrideUnsent);

		GetParam(input, "PR=", Temp);
		PR = atof(Temp);
		GetParam(input, "PUR=", Temp);
		PUR = atof(Temp);
		GetParam(input, "PF=", Temp);
		PF = atof(Temp);
		GetParam(input, "PUF=", Temp);
		PNF = atof(Temp);
		GetParam(input, "BF=", Temp);
		BF = atoi(Temp);
		GetParam(input, "BUF=", Temp);
		BNF = atoi(Temp);

		GetParam(input, "NTSD=", Temp);
		NTSD = atoi(Temp);

		GetParam(input, "NTSF=", Temp);
		NTSF = atoi(Temp);

		GetParam(input, "NTSU=", Temp);
		NTSU = atoi(Temp);

		GetParam(input, "From=", LTFROMString);
		LTFROM = GetOverrideFromString(LTFROMString);

		GetParam(input, "To=", LTTOString);
		LTTO = GetOverrideFromString(LTTOString);

		GetParam(input, "At=", LTATString);
		LTAT = GetOverrideFromString(LTATString);
 
		SaveConfig(ConfigName);
		GetConfig(ConfigName);

		// Calulate time to run Housekeeping
	
		now = time(NULL);

		tm = gmtime(&now);

		tm->tm_hour = MaintTime / 100;
		tm->tm_min = MaintTime % 100;
		tm->tm_sec = 0;

//		MaintClock = _mkgmtime(tm);
		MaintClock = mktime(tm) - (time_t)_MYTIMEZONE;

		while (MaintClock < now)
			MaintClock += MaintInterval * 3600;

		Debugprintf("Maint Clock %d NOW %d Time to HouseKeeping %d", MaintClock, now, MaintClock - now);
	}
	SendHouseKeeping(Reply, RLen, Key);
	return;
}







VOID SaveWelcome(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}
		
		GetMallocedParam(input, "NUWelcome=", &WelcomeMsg);
		GetMallocedParam(input, "NewWelcome=", &NewWelcomeMsg);
		GetMallocedParam(input, "ExWelcome=", &ExpertWelcomeMsg);

		TidyWelcomeMsg(&WelcomeMsg);
		TidyWelcomeMsg(&NewWelcomeMsg);
		TidyWelcomeMsg(&ExpertWelcomeMsg);

		GetMallocedParam(input, "NUPrompt=", &Prompt);
		GetMallocedParam(input, "NewPrompt=", &NewPrompt);
		GetMallocedParam(input, "ExPrompt=", &ExpertPrompt);
		TidyPrompts();

		GetParam(input, "Bye=", &SignoffMsg[0]);
		if (SignoffMsg[0])
		{
			if (SignoffMsg[strlen(SignoffMsg) - 1] == 10)
				SignoffMsg[strlen(SignoffMsg) - 1] = 0;

			if (SignoffMsg[strlen(SignoffMsg) - 1] != 13)
				strcat(SignoffMsg, "\r");
		}

		if (SignoffMsg[0] == 13)
			SignoffMsg[0] = 0;
	}
	
	SendWelcomePage(Reply, RLen, Key);
	return;
}

VOID ProcessConfUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = NULL;
	char Temp[80];

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}

		if (strstr(input, "ConfigUI=Config+UI"))
		{
			SendUIPage(Reply, RLen, Key);
			return;
		}
	
		GetParam(input, "BBSCall=", BBSName);
		_strupr(BBSName);
		strlop(BBSName, '-');
		GetParam(input, "SYSOPCall=", SYSOPCall);
		_strupr(SYSOPCall);
		strlop(SYSOPCall, '-');
		GetParam(input, "HRoute=", HRoute);
		_strupr(HRoute);
		GetParam(input, "ApplNum=", Temp);
		BBSApplNum = atoi(Temp);
		GetParam(input, "Streams=", Temp);
		MaxStreams = atoi(Temp);

		GetCheckBox(input, "SysToSYSOP=", &SendSYStoSYSOPCall);
		GetCheckBox(input, "BBSToSYSOP=", &SendBBStoSYSOPCall);
		GetCheckBox(input, "RefuseBulls=", &RefuseBulls);
		GetCheckBox(input, "EnUI=", &EnableUI);

		GetParam(input, "UIInterval=", Temp);
		MailForInterval = atoi(Temp);

		GetCheckBox(input, "DontHold=", &DontHoldNewUsers);
		GetCheckBox(input, "DefaultNoWinlink=", &DefaultNoWINLINK);
		GetCheckBox(input, "DontNeedName=", &AllowAnon);
		GetCheckBox(input, "DontNeedHomeBBS=", &DontNeedHomeBBS);
		GetCheckBox(input, "DontCheckFromCall=", &DontCheckFromCall);
		GetCheckBox(input, "UserCantKillT=", &UserCantKillT);
		UserCantKillT = !UserCantKillT;	// Reverse Logic
		GetCheckBox(input, "FWDtoMe=", &ForwardToMe);
		GetCheckBox(input, "OnlyKnown=", &OnlyKnown);
		GetCheckBox(input, "Events=", &reportMailEvents);

		GetParam(input, "POP3Port=", Temp);
		POP3InPort = atoi(Temp);

		GetParam(input, "SMTPPort=", Temp);
		SMTPInPort = atoi(Temp);

		GetParam(input, "NNTPPort=", Temp);
		NNTPInPort = atoi(Temp);

		GetCheckBox(input, "EnRemote=", &RemoteEmail);

		GetCheckBox(input, "EnISP=", &ISP_Gateway_Enabled);
		GetCheckBox(input, "SendAMPR=", &SendAMPRDirect);

		GetParam(input, "AMPRDomain=", AMPRDomain);

		GetParam(input, "ISPDomain=", MyDomain);
		GetParam(input, "SMTPServer=", ISPSMTPName);
		GetParam(input, "ISPEHLOName=", ISPEHLOName);
			
		GetParam(input, "ISPSMTPPort=", Temp);
		ISPSMTPPort = atoi(Temp);
	
		GetParam(input, "POP3Server=", ISPPOP3Name);

		GetParam(input, "ISPPOP3Port=", Temp);	
		ISPPOP3Port = atoi(Temp);
	
		GetParam(input, "ISPAccount=", ISPAccountName);
	
		GetParam(input, "ISPPassword=", ISPAccountPass);
		EncryptedPassLen = EncryptPass(ISPAccountPass, EncryptedISPAccountPass);
		
		GetParam(input, "PollInterval=", Temp);
		ISPPOP3Interval = atoi(Temp);

		GetCheckBox(input, "ISPAuth=", &SMTPAuthNeeded);

		GetCheckBox(input, "EnWP=", &SendWP);
		GetCheckBox(input, "RejWFBulls=", &FilterWPBulls);

		if (strstr(input, "Type=TypeB"))
			SendWPType = 0;

		if (strstr(input, "Type=TypeP"))
			SendWPType = 1;

		SendWPAddrs = GetMultiStringInput(input, "WPTO=");

		RejFrom = GetMultiStringInput(input, "Rfrom=");
		RejTo = GetMultiStringInput(input, "Rto=");
		RejAt = GetMultiStringInput(input, "Rat=");
		RejBID = GetMultiStringInput(input, "RBID=");
		HoldFrom = GetMultiStringInput(input, "Hfrom=");
		HoldTo = GetMultiStringInput(input, "Hto=");
		HoldAt = GetMultiStringInput(input, "Hat=");
		HoldBID = GetMultiStringInput(input, "HBID=");

		// Look for fbb style filters

		input = strstr(input, "&Action=");

		// delete old list

		while(Filters && Filters->Next)
		{
			FBBFilter * next = Filters->Next;
			free(Filters);
			Filters = next;
		}

		free(Filters);
		Filters = NULL;

		UndoTransparency(input);

		while (input)
		{
			// extract and validate before saving

			FBBFilter Filter;
			FBBFilter * PFilter;

			memset(&Filter, 0, sizeof(FBBFilter));

			Filter.Action = toupper(input[8]);

			input = strstr(input, "&Type=");
			
			if (Filter.Action == 'H' || Filter.Action == 'R' || Filter.Action == 'A')
			{
				Filter.Type = toupper(input[6]);
				input = strstr(input, "&From=");
				memcpy(Filter.From, &input[6], 10);
				input = strstr(input, "&TO=");
				strlop(Filter.From, '&');
				_strupr(Filter.From);
				memcpy(Filter.TO, &input[4], 10);
				input = strstr(input, "&AT=");
				strlop(Filter.TO, '&');
				_strupr(Filter.TO);
				memcpy(Filter.AT, &input[4], 10);
				input = strstr(input, "&BID=");
				strlop(Filter.AT, '&');
				_strupr(Filter.AT);
				memcpy(Filter.BID, &input[5], 10);
				input = strstr(input, "&MaxLen=");
				strlop(Filter.BID, '&');
				_strupr(Filter.BID);
				Filter.MaxLen = atoi(&input[8]);

				if (Filter.Type == '&') Filter.Type = '*';
				if (Filter.From[0] == 0) strcpy(Filter.From, "*");
				if (Filter.TO[0] == 0) strcpy(Filter.TO, "*");
				if (Filter.AT[0] == 0) strcpy(Filter.AT, "*");
				if (Filter.BID[0] == 0) strcpy(Filter.BID, "*");

				// add to list

				PFilter = zalloc(sizeof(FBBFilter));

				memcpy(PFilter, &Filter, sizeof(FBBFilter));

				if (Filters == 0)
					Filters = PFilter;
				else
				{
					FBBFilter * p = Filters;

					while (p->Next)
						p = p->Next;

					p->Next = PFilter;
				}
			}

			input = strstr(input, "&Action=");
		}

		SaveConfig(ConfigName);
		GetConfig(ConfigName);
	}
	
	SendConfigPage(Reply, RLen, Key);
	return;
}



VOID ProcessUIUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0, i;
	char * input;
	struct UserInfo * USER = NULL;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
			return;
		}

		GetParam(input, "MailFor=", &MailForText[0]);

		for (i = 1; i <= GetNumberofPorts(); i++)
		{
			char EnKey[10];
			char DigiKey[10];
			char MFKey[12];
			char HDDRKey[12];
			char NullKey[12];
			char Temp[100];

			sprintf(EnKey, "En%d=", i);
			sprintf(DigiKey, "Path%d=", i);
			sprintf(MFKey, "SndMF%d=", i);
			sprintf(HDDRKey, "SndHDDR%d=", i);
			sprintf(NullKey, "SndNull%d=", i);

			GetCheckBox(input, EnKey, &UIEnabled[i]);
			GetParam(input, DigiKey, Temp);
			if (UIDigi[i])
				free (UIDigi[i]);
			UIDigi[i] = _strdup(Temp);
			GetCheckBox(input, MFKey, &UIMF[i]);
			GetCheckBox(input, HDDRKey, &UIHDDR[i]);
			GetCheckBox(input, NullKey, &UINull[i]);
		}

		SaveConfig(ConfigName);
		GetConfig(ConfigName);
	}

	SendUIPage(Reply, RLen, Key);
	return;
}

VOID ProcessDisUser(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	char * input;
	char * ptr;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		ptr = strstr(input, "call=");
		if (ptr)
		{
			int Stream = atoi(ptr + 5);
			Disconnect(Stream);
		}
	}	
	SendStatusPage(Reply, RLen, Rest);
}



VOID SaveFwdCommon(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = NULL;

	char Temp[80];
	int Mask = 0;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		int n;
		GetParam(input, "MaxTX=", Temp);
		MaxTXSize = atoi(Temp);
		GetParam(input, "MaxRX=", Temp);
		MaxRXSize = atoi(Temp);
		GetParam(input, "MaxAge=", Temp);
		MaxAge = atoi(Temp);
		GetCheckBox(input, "WarnNoRoute=", &WarnNoRoute);
		GetCheckBox(input, "LocalTime=", &Localtime);
		GetCheckBox(input, "SendPtoMultiple=", &SendPtoMultiple);
		GetCheckBox(input, "FourCharCont=", &FOURCHARCONT);

		// Reinitialise Aliases

		n = 0;

		if (Aliases)
		{
			while(Aliases[n])
			{
				free(Aliases[n]->Dest);
				free(Aliases[n]);
				n++;
			}

			free(Aliases);
			Aliases = NULL;
			FreeList(AliasText);
		}
	
		AliasText = GetMultiStringInput(input, "Aliases=");

		if (AliasText)
		{
			n = 0;

			while (AliasText[n])
			{
				_strupr(AliasText[n]);
				n++;
			}
		}
		SetupFwdAliases();
	}
	
	SaveConfig(ConfigName);
	GetConfig(ConfigName);

	SendFwdMainPage(Reply, RLen, Session->Key);
}

char * GetNextParam(char ** next)
{
	char * ptr1 = *next;
	char * ptr2 = strchr(ptr1, '|');
	if (ptr2)
	{
		*(ptr2++) = 0;
		*next = ptr2;
	}
	return ptr1;
}

VOID SaveFwdDetails(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = Session->User;
	struct BBSForwardingInfo * FWDInfo = USER->ForwardingInfo;
	char * ptr1, *ptr2;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "StartForward"))
		{
			StartForwarding(USER->BBSNumber, NULL);
			SendFwdDetails(Session->User, Reply, RLen, Session->Key);
			return;
		}

		if (strstr(input, "CopyForward"))
		{
			struct UserInfo * OldBBS;

			// Get call to copy from 

			ptr2 = input + 4;
			ptr1 = GetNextParam(&ptr2);		// Call
			_strupr(ptr2);

			OldBBS = FindBBS(ptr2);

			if (OldBBS == NULL)
			{

				*RLen = sprintf(Reply, "<h3 align=center>Copy From BBS %s not found</h3>", ptr2);
				return;
			}

			// Set current info from OldBBS
//
//			SetForwardingPage(hDlg, OldBBS);			// moved to separate routine as also called from copy config

			SendFwdDetails(OldBBS, Reply, RLen, Session->Key);
			return;
		}
		// Fwd update

		ptr2 = input + 4;
		ptr1 = GetNextParam(&ptr2);		// TO
		FWDInfo->TOCalls = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// AT
		FWDInfo->ATCalls = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// TIMES
		FWDInfo->FWDTimes = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// FWD SCRIPT
		FWDInfo->ConnectScript = SeparateMultiString(ptr1, TRUE);

		ptr1 = GetNextParam(&ptr2);		// HRB
		FWDInfo->Haddresses = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// HRP
		FWDInfo->HaddressesP = SeparateMultiString(ptr1, FALSE);

		ptr1 = GetNextParam(&ptr2);		// BBSHA
		if (FWDInfo->BBSHA)
			free(FWDInfo->BBSHA);

		FWDInfo->BBSHA = _strdup(_strupr(ptr1));

		ptr1 = GetNextParam(&ptr2);		// EnF
		if (strcmp(ptr1, "true") == 0) FWDInfo->Enabled = TRUE; else FWDInfo->Enabled = FALSE;

		ptr1 = GetNextParam(&ptr2);		// Interval
		FWDInfo->FwdInterval = atoi(ptr1);

		ptr1 = GetNextParam(&ptr2);		// EnR
		if (strcmp(ptr1, "true") == 0) FWDInfo->ReverseFlag = TRUE; else FWDInfo->ReverseFlag = FALSE;

		ptr1 = GetNextParam(&ptr2);		// RInterval
		FWDInfo->RevFwdInterval = atoi(ptr1);

		ptr1 = GetNextParam(&ptr2);		// No Wait
		if (strcmp(ptr1, "true") == 0) FWDInfo->SendNew = TRUE; else FWDInfo->SendNew = FALSE;

		ptr1 = GetNextParam(&ptr2);		// Blocked
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowBlocked = TRUE; else FWDInfo->AllowBlocked = FALSE;

		ptr1 = GetNextParam(&ptr2);		// FBB Block
		FWDInfo->MaxFBBBlockSize = atoi(ptr1);

		ptr1 = GetNextParam(&ptr2);		// Personals
		if (strcmp(ptr1, "true") == 0) FWDInfo->PersonalOnly = TRUE; else FWDInfo->PersonalOnly = FALSE;
		ptr1 = GetNextParam(&ptr2);		// Binary
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowCompressed = TRUE; else FWDInfo->AllowCompressed = FALSE;
		ptr1 = GetNextParam(&ptr2);		// B1
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowB1 = TRUE; else FWDInfo->AllowB1 = FALSE;
		ptr1 = GetNextParam(&ptr2);		// B2
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowB2 = TRUE; else FWDInfo->AllowB2 = FALSE;
		ptr1 = GetNextParam(&ptr2);		// CTRLZ
		if (strcmp(ptr1, "true") == 0) FWDInfo->SendCTRLZ = TRUE; else FWDInfo->SendCTRLZ = FALSE;
		ptr1 = GetNextParam(&ptr2);		// Connect Timeout
		FWDInfo->ConTimeout = atoi(ptr1);

		SaveConfig(ConfigName);
		GetConfig(ConfigName);

		ReinitializeFWDStruct(Session->User);
	
		SendFwdDetails(Session->User, Reply, RLen, Session->Key);
	}
}



VOID ProcessUserUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = Session->User;
	int SSID, Mask = 0;
	char * ptr1, *ptr2;
	int skipRMSExUser = 0;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
			return;
		}

		if (strstr(input, "Delete"))
		{
			int n;

			for (n = 1; n <= NumberofUsers; n++)
			{
				if (Session->User == UserRecPtr[n])
					break;
			}
		
			if (n <= NumberofUsers)
			{
				USER = Session->User;

				for (n = n; n < NumberofUsers; n++)
				{
					UserRecPtr[n] = UserRecPtr[n+1];		// move down all following entries
				}
	
				NumberofUsers--;

				if (USER->flags & F_BBS)	// was a BBS?
					DeleteBBS(USER);
	
				free(USER);
			
				SaveUserDatabase();

				Session->User = UserRecPtr[1];

				SendUserSelectPage(Reply, RLen, Session->Key);
				return;
			}
		}

		if (strstr(input, "Add="))
		{
			char * Call;

			Call = input + 8;
			strlop(Call, '-');

			if (strlen(Call) > 6)
				Call[6] = 0;

			_strupr(Call);
		
			if (Call[0] == 0 || LookupCall(Call))
			{
				// Null or exists

				SendUserSelectPage(Reply, RLen, Session->Key);
				return;
			}

			USER = AllocateUserRecord(Call);
			USER->Temp = zalloc(sizeof (struct TempUserInfo));
		
			SendUserSelectPage(Reply, RLen, Session->Key);
			return;

		}

		// User update

		ptr2 = input + 4;
		ptr1 = GetNextParam(&ptr2);		// BBS

		// If BBS Flag has changed, must set up or delete forwarding info

		if (strcmp(ptr1, "true") == 0)
		{
			if ((USER->flags & F_BBS) == 0)
			{
				// New BBS

				if(SetupNewBBS(USER))
				{
					USER->flags |= F_BBS;
					USER->flags &= ~F_Temp_B2_BBS;		// Clear RMS Express User
					skipRMSExUser = 1;					// Dont read old value
				}
				else
				{
					// Failed - too many bbs's defined

					//sprintf(InfoBoxText, "Cannot set user to be a BBS - you already have 80 BBS's defined");
					//DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
					USER->flags &= ~F_BBS;
					//CheckDlgButton(hDlg, IDC_BBSFLAG, (user->flags & F_BBS));
				}
			}
		}
		else
		{
			if (USER->flags & F_BBS)
			{
				//was a BBS

				USER->flags &= ~F_BBS;
				DeleteBBS(USER);
			}
		}

		ptr1 = GetNextParam(&ptr2);		// Permit Email
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_EMAIL; else USER->flags &= ~F_EMAIL;

		ptr1 = GetNextParam(&ptr2);		// PMS
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_PMS; else USER->flags &= ~F_PMS;

		ptr1 = GetNextParam(&ptr2);		// RMS EX User
		if (strcmp(ptr1, "true") == 0 && !skipRMSExUser) USER->flags |= F_Temp_B2_BBS; else USER->flags &= ~F_Temp_B2_BBS;
		ptr1 = GetNextParam(&ptr2);		// SYSOP
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_SYSOP; else USER->flags &= ~F_SYSOP;
		ptr1 = GetNextParam(&ptr2);		// PollRMS
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_POLLRMS; else USER->flags &= ~F_POLLRMS;
		ptr1 = GetNextParam(&ptr2);		// Expert
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_Expert; else USER->flags &= ~F_Expert;

		ptr1 = GetNextParam(&ptr2);		// SSID1
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		ptr1 = GetNextParam(&ptr2);		// SSID2
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		ptr1 = GetNextParam(&ptr2);		// SSID3
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		ptr1 = GetNextParam(&ptr2);		// SSID4
		SSID = atoi(ptr1);
		Mask |= (1 << SSID);
		Session->User->RMSSSIDBits = Mask;

		ptr1 = GetNextParam(&ptr2);		// Excluded
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_Excluded; else USER->flags &= ~F_Excluded;
		ptr1 = GetNextParam(&ptr2);		// Hold
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_HOLDMAIL; else USER->flags &= ~F_HOLDMAIL;
		ptr1 = GetNextParam(&ptr2);		// SYSOP gets LM
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_SYSOP_IN_LM; else USER->flags &= ~F_SYSOP_IN_LM;
		ptr1 = GetNextParam(&ptr2);		// Dont add winlink.org
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_NOWINLINK; else USER->flags &= ~F_NOWINLINK;
		ptr1 = GetNextParam(&ptr2);		// Allow Bulls
		if (strcmp(ptr1, "true") == 0) USER->flags &= ~F_NOBULLS; else USER->flags |= F_NOBULLS;	// Inverted flag
		ptr1 = GetNextParam(&ptr2);		// NTS Message Pickup Station
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_NTSMPS; else USER->flags &= ~F_NTSMPS;
		ptr1 = GetNextParam(&ptr2);		// APRS Mail For
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_RMSREDIRECT; else USER->flags &= ~F_RMSREDIRECT;
		ptr1 = GetNextParam(&ptr2);		// Redirect to RMS

		if (strcmp(ptr1, "true") == 0) USER->flags |= F_APRSMFOR; else USER->flags &= ~F_APRSMFOR;
	
		ptr1 = GetNextParam(&ptr2);		// APRS SSID
		SSID = atoi(ptr1);
		SSID &= 15;
		USER->flags &= 0x0fffffff;
		USER->flags |= (SSID << 28);


		ptr1 = GetNextParam(&ptr2);		// Last Listed
		USER->lastmsg = atoi(ptr1);
		ptr1 = GetNextParam(&ptr2);		// Name
		memcpy(USER->Name, ptr1, 17);
		ptr1 = GetNextParam(&ptr2);		// Pass
		memcpy(USER->pass, ptr1, 12);		
		ptr1 = GetNextParam(&ptr2);		// CMS Pass
		if (memcmp("****************", ptr1, strlen(ptr1) != 0))
		{
			memcpy(USER->CMSPass, ptr1, 15);
		}
		
		ptr1 = GetNextParam(&ptr2);		// QTH
		memcpy(USER->Address, ptr1, 60);
		ptr1 = GetNextParam(&ptr2);		// ZIP
		memcpy(USER->ZIP, ptr1, 8);
		ptr1 = GetNextParam(&ptr2);		// HomeBBS
		memcpy(USER->HomeBBS, ptr1, 40);
		_strupr(USER->HomeBBS);

		SaveUserDatabase();
		UpdateWPWithUserInfo(USER);

		*RLen = SendUserDetails(Session, Reply, Session->Key);
	}
}

VOID ProcessMsgAction(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	int BBSNumber = 0;
	struct MsgInfo * Msg = Session->Msg;
	char * ptr1;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input && Msg)
	{
		ptr1 = input + 4;
		*RLen = SendMessageDetails(Msg, Reply, Session->Key);
	}
}

VOID SaveMessageText(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	struct MsgInfo * Msg = Session->Msg;
	char * ptr, * ptr1, * ptr2, *input;
	char c;
	int MsgLen, WriteLen;
	char MsgFile[256];
	FILE * hFile;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = sprintf(Reply, "%s", "<html><script>window.close();</script></html>");
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

				*(ptr2++) = m * 16 + n;
			}
			else if (c == '+')
				*(ptr2++) = ' ';
			else
				*(ptr2++) = c;

			c = *(ptr1++);
		}

		*(ptr2++) = 0;

		MsgLen = (int)strlen(input + 8);

		Msg->datechanged = time(NULL);
		Msg->length = MsgLen;

			sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
			hFile = fopen(MsgFile, "wb");
	
			if (hFile)
			{
				WriteLen = (int)fwrite(input + 8, 1, Msg->length, hFile); 
				fclose(hFile);
			}

			if (WriteLen != Msg->length)
			{
				char Mess[80];
				sprintf_s(Mess, sizeof(Mess), "Failed to create Message File\r");
				CriticalErrorHandler(Mess);

				return;
			}

			SaveMessageDatabase();

			*RLen = sprintf(Reply, "%s", "<html><script>alert(\"Message Saved\");window.close();</script></html>");

		}
	}
	return;

}


VOID ProcessMsgUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	int BBSNumber = 0;
	struct MsgInfo * Msg = Session->Msg;
	char * ptr1, * ptr2;
	char OldStatus = Msg->status;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input && Msg)
	{
		ptr1 = input + 4;
		ptr2 = strchr(ptr1, '|');
		if (ptr2)
		{
			*(ptr2++) = 0;
			strcpy(Msg->from, ptr1);
			ptr1 = ptr2;
		}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->to, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->bid, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->emailfrom, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->via, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->title, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;Msg->type = *ptr1;ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;Msg->status = *ptr1;ptr1 = ptr2;}

		if (Msg->status != OldStatus)
		{
			// Need to take action if killing message

			if (Msg->status == 'K')
				FlagAsKilled(Msg, FALSE);					// Clear forwarding bits
		}

		Msg->datechanged = time(NULL);
		SaveMessageDatabase();
	}

	*RLen = SendMessageDetails(Msg, Reply, Session->Key);
}




VOID ProcessMsgFwdUpdate(struct HTTPConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	int BBSNumber = 0;
	struct UserInfo * User;
	struct MsgInfo * Msg = Session->Msg;
	BOOL toforward, forwarded;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input && Msg)
	{
		BBSNumber = atoi(input + 4);
		User = BBSLIST[BBSNumber];

		if (User == NULL)
			return;
		
		toforward = check_fwd_bit(Msg->fbbs, BBSNumber);
		forwarded = check_fwd_bit(Msg->forw, BBSNumber);

		if (forwarded)
		{
			// Changing to not this BBS

			clear_fwd_bit(Msg->forw, BBSNumber);
		}
		else if (toforward)
		{
			// Change to Forwarded

			clear_fwd_bit(Msg->fbbs, BBSNumber);
			User->ForwardingInfo->MsgCount--;
			set_fwd_bit(Msg->forw, BBSNumber);
		}
		else
		{
			// Change to to forward
			
			set_fwd_bit(Msg->fbbs, BBSNumber);
			User->ForwardingInfo->MsgCount++;
			clear_fwd_bit(Msg->forw, BBSNumber);
			if (FirstMessageIndextoForward > GetMessageSlotFromMessageNumber(Msg->number))
				FirstMessageIndextoForward = GetMessageSlotFromMessageNumber(Msg->number);

		}
		*RLen = SendMessageDetails(Msg, Reply, Session->Key);
	}
	SaveMessageDatabase();
}




VOID SetMultiStringValue(char ** values, char * Multi)
{
	char ** Calls;
	char * ptr = &Multi[0];

	*ptr = 0;

	if (values)
	{
		Calls = values;

		while(Calls[0])
		{
			strcpy(ptr, Calls[0]);
			ptr += strlen(Calls[0]);
			*(ptr++) = '\r';
			*(ptr++) = '\n';
			Calls++;
		}
		*(ptr) = 0;
	}
}



VOID SendFwdDetails(struct UserInfo * User, char * Reply, int * ReplyLen, char * Key)
{
	int Len;
	struct BBSForwardingInfo * FWDInfo = User->ForwardingInfo;
	char TO[2048] = "";
	char AT[2048] = "";
	char TIMES[2048] = "";
	char FWD[100000] = "";
	char HRB[2048] = "";
	char HRP[2048] = "";

	SetMultiStringValue(FWDInfo->TOCalls, TO);
	SetMultiStringValue(FWDInfo->ATCalls, AT);
	SetMultiStringValue(FWDInfo->FWDTimes, TIMES);
	SetMultiStringValue(FWDInfo->ConnectScript, FWD);
	SetMultiStringValue(FWDInfo->Haddresses, HRB);
	SetMultiStringValue(FWDInfo->HaddressesP, HRP);

	// Forwarding details are injected into the /Mail/FWD page via AJAX.
	// Do not prepend the global header/menu here.
	Len = 0;

	if (FwdDetailTemplate == NULL)
		FwdDetailTemplate = GetTemplateFromFile(3, "FwdDetail.txt");
		
	Len += sprintf(&Reply[Len], FwdDetailTemplate, User->Call,
		CountMessagestoForward (User), Key,
		TO, AT, TIMES , FWD, HRB, HRP, 
		(FWDInfo->BBSHA) ? FWDInfo->BBSHA : "", 
		(FWDInfo->Enabled) ? CHKD  : UNC,
		FWDInfo->FwdInterval,
		(FWDInfo->ReverseFlag) ? CHKD  : UNC,
		FWDInfo->RevFwdInterval, 
		(FWDInfo->SendNew) ? CHKD  : UNC,
		(FWDInfo->AllowBlocked) ? CHKD  : UNC,
		FWDInfo->MaxFBBBlockSize,
		(FWDInfo->PersonalOnly) ? CHKD  : UNC,
		(FWDInfo->AllowCompressed) ? CHKD  : UNC,
		(FWDInfo->AllowB1) ? CHKD  : UNC,
		(FWDInfo->AllowB2) ? CHKD  : UNC,
		(FWDInfo->SendCTRLZ) ? CHKD  : UNC,
		FWDInfo->ConTimeout);

	*ReplyLen = Len;

}

VOID SendConfigPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len, i;

	char HF[2048] = "";
	char HT[2048] = "";
	char HA[2048] = "";
	char HB[2048] = "";
	char RF[2048] = "";
	char RT[2048] = "";
	char RA[2048] = "";
	char RB[2048] = "";
	char WPTO[10000] = "";

	char FBBFilters[100000] = "";

	
	char * ptr = FBBFilters;
	FBBFilter * Filter = Filters;

	// ConfigTemplate includes its own page header/navigation.
	// Avoid prepending MailPage here or the menu can be rendered twice.
	Len = 0;

	SetMultiStringValue(RejFrom, RF);
	SetMultiStringValue(RejTo, RT);
	SetMultiStringValue(RejAt, RA);
	SetMultiStringValue(RejBID, RB);
	SetMultiStringValue(HoldFrom, HF);
	SetMultiStringValue(HoldTo, HT);
	SetMultiStringValue(HoldAt, HA);
	SetMultiStringValue(HoldBID, HB);
	SetMultiStringValue(SendWPAddrs, WPTO);

	// set up FBB style filters
	
	ptr += sprintf(ptr, 
		"<div class=table-container><table><thead><tr><th>Action</th><th>Type</th><th>From</th><th>To</th><th>@BBS</th><th>Bid</th><th>Max Size</th></tr></thead><tbody>");

	while(Filter)
	{
		ptr += sprintf(ptr, "<tr>"	
		"<td><input type=text class=uppercase name=Action maxlength=2 value=\"%c\"></td>"
		"<td><input type=text class=uppercase name=Type maxlength=2 value=\"%c\"></td>"
		"<td><input type=text class=uppercase name=From maxlength=7 value=\"%s\"></td>"
		"<td><input type=text class=uppercase name=TO maxlength=7 value=\"%s\"></td>"
		"<td><input type=text class=uppercase name=AT maxlength=7 value=\"%s\"></td>"
		"<td><input type=text class=uppercase name=BID maxlength=13 value=\"%s\"></td>"
		"<td><input type=text name=MaxLen maxlength=6 value=\"%d\"></td></tr>",
			Filter->Action, Filter->Type, Filter->From, Filter->TO, Filter->AT, Filter->BID, Filter->MaxLen);

		Filter = Filter->Next;
	}

	//	Add a few blank entries for input

	for (i = 0; i < 5; i++)
	{
		ptr += sprintf(ptr, "<tr>"
		"<td><input type=text class=uppercase name=Action maxlength=2 value=\"%c\"></td>"
		"<td><input type=text class=uppercase name=Type maxlength=2 value=\"%c\"></td>"
		"<td><input type=text class=uppercase name=From maxlength=7 value=\"%s\"></td>"
		"<td><input type=text class=uppercase name=TO maxlength=7 value=\"%s\"></td>"
		"<td><input type=text class=uppercase name=AT maxlength=7 value=\"%s\"></td>"
		"<td><input type=text class=uppercase name=BID maxlength=13 value=\"%s\"></td>"
		"<td><input type=text name=MaxLen maxlength=6 value=\"%d\"></td></tr>", ' ', ' ', "", "", "", "", 0);
	}

	ptr += sprintf(ptr, "</tbody></table></div>");

	Debugprintf("%d", strlen(FBBFilters));

	Len += sprintf(&Reply[Len], ConfigTemplate,
		BBSName,
		Key, Key, Key, Key, Key, Key, Key, Key,
		Key,
		BBSName, SYSOPCall, HRoute,
		(SendBBStoSYSOPCall) ? CHKD  : UNC,
		BBSApplNum, MaxStreams,
		(SendSYStoSYSOPCall) ? CHKD  : UNC,
		(RefuseBulls) ? CHKD  : UNC,
		(EnableUI) ? CHKD  : UNC,
		MailForInterval,
		(DontHoldNewUsers) ? CHKD  : UNC,
		(DefaultNoWINLINK) ? CHKD  : UNC,
		(AllowAnon) ? CHKD  : UNC, 
		(DontNeedHomeBBS) ? CHKD  : UNC, 
		(DontCheckFromCall) ? CHKD  : UNC, 
		(UserCantKillT) ? UNC : CHKD,		// Reverse logic
		(ForwardToMe) ? CHKD  : UNC,
		(OnlyKnown) ? CHKD  : UNC,
		(reportMailEvents) ? CHKD  : UNC,
		POP3InPort, SMTPInPort, NNTPInPort,
		(RemoteEmail) ? CHKD  : UNC,
		AMPRDomain,
		(SendAMPRDirect) ? CHKD  : UNC,
		(ISP_Gateway_Enabled) ? CHKD  : UNC,
		MyDomain, ISPSMTPName, ISPSMTPPort, ISPEHLOName, ISPPOP3Name, ISPPOP3Port,
		ISPAccountName, ISPAccountPass, ISPPOP3Interval,
		(SMTPAuthNeeded) ? CHKD  : UNC,
		(SendWP) ? CHKD  : UNC,
		(FilterWPBulls) ? CHKD  : UNC,
		(SendWPType == 0) ? CHKD  : UNC,
		(SendWPType == 1) ? CHKD  : UNC,
		WPTO,
		RF, RT, RA, RB, HF, HT, HA, HB, FBBFilters);

	*ReplyLen = Len;
}
VOID SendHouseKeeping(char * Reply, int * ReplyLen, char * Key)
{
	char FromList[1000]= "", ToList[1000]= "", AtList[1000] = "";
	char Line[80];
	struct Override ** Call;
	int Len;

	Len = SendHeader(Reply, Key);

	if (LTFROM)
	{
		Call = LTFROM;
		while(Call[0])
		{
			sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
			strcat(FromList, Line);
			Call++;
		}
	}
		if (LTTO)
		{
			Call = LTTO;
			while(Call[0])
			{
				sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(ToList, Line);
				Call++;
			}
		}

		if (LTAT)
		{
			Call = LTAT;
			while(Call[0])
			{
				sprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(AtList, Line);
				Call++;
			}
		}

		*ReplyLen = Len + sprintf(&Reply[Len], HousekeepingTemplate, 
			 Key,
			MaintTime, MaintInterval, MaxMsgno, BidLifetime, BBSLogAge, UserLifetime,
			(DeletetoRecycleBin) ? CHKD  : UNC,
			(SendNonDeliveryMsgs) ? CHKD  : UNC,
			(SuppressMaintEmail) ? CHKD  : UNC,
			(GenerateTrafficReport) ? CHKD  : UNC,
			PR, PUR, PF, PNF, BF, BNF, NTSD, NTSF, NTSU,
			FromList, ToList, AtList,
			(OverrideUnsent) ? CHKD  : UNC);

		return;

}


VOID SendWelcomePage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;

	Len = SendHeader(Reply, Key);
		
	Len += sprintf(&Reply[Len], Welcome, Key, WelcomeMsg, NewWelcomeMsg, ExpertWelcomeMsg,
			Prompt, NewPrompt, ExpertPrompt, SignoffMsg);
	*ReplyLen = Len;
}

VOID SendFwdMainPage(char * Reply, int * RLen, char * Key)
{
	char ALIASES[16384];
	int Len;

	// FwdTemplate is a full page template that includes its own header/navigation.
	// Avoid prepending MailPage here or the menu can be rendered twice.
	Len = 0;

	SetMultiStringValue(AliasText, ALIASES);

	*RLen = Len + sprintf(&Reply[Len], FwdTemplate, Key, Key,
		BBSName,
		Key, Key, Key, Key, Key, Key, Key, Key,
		Key, MaxTXSize, MaxRXSize, MaxAge,
		(WarnNoRoute) ? CHKD  : UNC, 
		(Localtime) ? CHKD  : UNC,
		(SendPtoMultiple) ? CHKD  : UNC,
		(FOURCHARCONT) ? CHKD  : UNC,
		ALIASES);
}


char TenSpaces[] = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";

VOID SendUIPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len, i;

	Len = SendHeader(Reply, Key);
	Len += sprintf(&Reply[Len], "<style>"
		COMMON_CSS_VARIABLES
		COMMON_FORM_CSS
		COMMON_UTILITY_CSS
		COMMON_BUTTON_CSS
		".port-row{display:flex;flex-wrap:wrap;margin:15px 0;gap:10px;padding:15px;background:var(--surface-soft);border:1px solid var(--border);border-radius:4px;align-items:center;}"
		".port-row-header{display:flex;flex-wrap:wrap;margin:10px 0;gap:15px;font-weight:bold;font-size:0.9em;}"
		".port-row-label{flex:0 1 120px;}"
		".port-row input[type=checkbox]{margin:0;}"
		".port-row input[type=text]{flex:2 1 200px;padding:8px;border:1px solid var(--border);border-radius:4px;}"
		".port-row span{flex:1 1 120px;}"
		"@media(max-width:768px){.port-row{flex-direction:column;align-items:flex-start;}.port-row input[type=text]{width:100%%;flex:none;}.port-row-header{flex-direction:column;align-items:flex-start;}}"
		"</style>");
	Len += sprintf(&Reply[Len], UIHddr, Key, MailForText);

	for (i = 1; i <= GetNumberofPorts(); i++)
	{
		char PortNo[512];
		char PortDesc[31];
		int n;

		// Only allow UI on ax.25 ports

		struct _EXTPORTDATA * PORTVEC;

		PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntryFromSlot(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
				if (PORTVEC->PORTCONTROL.UICAPABLE == 0)
					continue;


		GetPortDescription(i, PortDesc);
		n = sprintf(PortNo, "Port&nbsp;%2d %s", GetPortNumber(i), PortDesc);

		while (PortNo[--n] == ' ');

		PortNo[n + 1] = 0;
	
		while (n++ < 38)
			strcat(PortNo, "&nbsp;");

		Len += sprintf(&Reply[Len], UILine,
				(UIEnabled[i])?CHKD:UNC, i,
				 PortNo,
				 (UIDigi[i])?UIDigi[i]:"", i, 
			 	 (UIMF[i])?CHKD:UNC, i,
				 (UIHDDR[i])?CHKD:UNC, i,
				 (UINull[i])?CHKD:UNC, i);
	}

	Len += sprintf(&Reply[Len], "%s", UITail);

	*ReplyLen = Len;
}

void ConvertSpaceTonbsp(char * msg)
{
	// Replace any space with &nbsp;
		
	char * ptr;

	while (ptr = strchr(msg, ' '))
	{
		memmove(ptr + 5, ptr, strlen(ptr) + 1);
		memcpy(ptr, "&nbsp;", 6);
	}
}

VOID SendStatusPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;
	char msg[1024];
	CIRCUIT * conn;
	int i,n, SYSOPMsgs = 0, HeldMsgs = 0; 
	char Name[80];

	SMTPMsgs = 0;

	Len = sprintf(Reply, RefreshMainPage, BBSName, BBSName, Key, Key, Key, Key, Key, Key, Key, Key);

	Len += sprintf(&Reply[Len], StatusPage, Key);

	for (n = 0; n < NumberofStreams; n++)
	{
		conn=&Connections[n];

		if (!conn->Active)
		{
			Len += sprintf(&Reply[Len], "<tr><td class=num></td><td>Idle</td><td></td><td class=num></td><td class=num></td><td class=num></td><td class=num></td></tr>");
		}
		else
		{
			{
				if (conn->UserPointer == 0)
				{
					Len += sprintf(&Reply[Len], "<tr><td class=num><input type=radio name=call value=%d></td><td>Logging in</td><td></td><td class=num></td><td class=num></td><td class=num></td><td class=num></td></tr>", conn->BPQStream);
				}
				else
				{
					strcpy(Name, conn->UserPointer->Name);
					Name[9] = 0;

					Len += sprintf(&Reply[Len], "<tr><td class=num><input type=radio name=call value=%d></td><td>%s</td><td>%s</td><td class=num>%d</td><td class=num>%d</td><td class=num>%d</td><td class=num>%d</td></tr>",
						conn->BPQStream, Name, conn->UserPointer->Call, conn->BPQStream, conn->OutputQueueLength - conn->OutputGetPointer, conn->bytesSent, conn->bytesRxed);
				}
			}
		}
	}

	n = 0;

	for (i=1; i <= NumberofMessages; i++)
	{
		if (MsgHddrPtr[i]->status == 'N')
		{
			if (_stricmp(MsgHddrPtr[i]->to, SYSOPCall) == 0  || _stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0)
				SYSOPMsgs++;
			else
			if (MsgHddrPtr[i]->to[0] == 0)
				SMTPMsgs++;
		}
		else
		{
			if (MsgHddrPtr[i]->status == 'H')
				HeldMsgs++;
		}
	}

	Len += sprintf(&Reply[Len], StreamEnd, 
		NumberofMessages, SYSOPMsgs, HeldMsgs, SMTPMsgs);

	// If there are any active multicast transfers, display them.

	Len += MulticastStatusHTML(&Reply[Len]);

	Len += sprintf(&Reply[Len], StatusTail, 
		NumberofMessages, SYSOPMsgs, HeldMsgs, SMTPMsgs);

	*ReplyLen = Len;
}

VOID SendFwdSelectPage(char * Reply, int * ReplyLen, char * Key)
{
	struct UserInfo * USER;
	int i = 0;
	int Len = 0;

	for (USER = BBSChain; USER; USER = USER->BBSNext)
	{
		Len += sprintf(&Reply[Len], "%s|", USER->Call);
	}

	*ReplyLen = Len;
}

VOID SendUserSelectPage(char * Reply, int * ReplyLen, char * Key)
{
	struct UserInfo * USER;
	int i = 0, n;
	int Len = 0;
	struct UserInfo * users[10000]; 

	// Get array of addresses

	for (n = 1; n <= NumberofUsers; n++)
	{
		users[i++] = UserRecPtr[n];
		if (i > 9999) break;
	}

	qsort((void *)users, i, sizeof(void *), compare );
		
	for (n = 0; n < NumberofUsers; n++)
	{
		USER = users[n];
		Len += sprintf(&Reply[Len], "%s|", USER->Call);
	}
	*ReplyLen = Len;
}

int SendUserDetails(struct HTTPConnectionInfo * Session, char * Reply, char * Key)
{
	char SSID[16][16] = {""};
	char ASSID[16];
	int i, n, s, Len;
	struct UserInfo * User = Session->User;
	unsigned int flags = User->flags;
	int RMSSSIDBits = Session->User->RMSSSIDBits;
	char HiddenPass[20] = "";

	int	ConnectsIn;
	int ConnectsOut;
	int MsgsReceived;
	int MsgsSent;
	int MsgsRejectedIn;
	int MsgsRejectedOut;
	int BytesForwardedIn;
	int BytesForwardedOut;
//	char MsgsIn[80];
//	char MsgsOut[80];
//	char BytesIn[80];
//	char BytesOut[80];
//	char RejIn[80];
//	char RejOut[80];

	i = 0;

	ConnectsIn = User->Total.ConnectsIn - User->Last.ConnectsIn;
	ConnectsOut = User->Total.ConnectsOut - User->Last.ConnectsOut;

	MsgsReceived = MsgsSent = MsgsRejectedIn = MsgsRejectedOut = BytesForwardedIn = BytesForwardedOut = 0;

	for (n = 0; n < 4; n++)
	{
		MsgsReceived +=	User->Total.MsgsReceived[n] - User->Last.MsgsReceived[n];	
		MsgsSent += User->Total.MsgsSent[n] - User->Last.MsgsSent[n];
		BytesForwardedIn += User->Total.BytesForwardedIn[n] - User->Last.BytesForwardedIn[n];
		BytesForwardedOut += User->Total.BytesForwardedOut[n] - User->Last.BytesForwardedOut[n];
		MsgsRejectedIn += User->Total.MsgsRejectedIn[n] - User->Last.MsgsRejectedIn[n];
		MsgsRejectedOut += User->Total.MsgsRejectedOut[n] - User->Last.MsgsRejectedOut[n];
	}

	
	for (s = 0; s < 16; s++)
	{
		if (RMSSSIDBits & (1 << s))
		{
			if (s)
				sprintf(&SSID[i++][0], "%d", s);
			else
				SSID[i++][0] = 0;
		}
	}

	memset(HiddenPass, '*', strlen(User->CMSPass));

	i = (flags >> 28);
	sprintf(ASSID, "%d", i);

	if (i == 0)
		ASSID[0] = 0;

	// User details are injected into the /Mail/Users page via AJAX.
	// Do not prepend the global header/menu here, or it renders a duplicate menu inside #main.
	Len = 0;

	if (!UserDetailTemplate)
		UserDetailTemplate = GetTemplateFromFile(4, "UserDetail.txt");

	Len += sprintf(&Reply[Len], UserDetailTemplate, Key, User->Call,
		(flags & F_BBS)?CHKD:UNC,
		(flags & F_EMAIL)?CHKD:UNC,
		(flags & F_PMS)?CHKD:UNC,
		(flags & F_Temp_B2_BBS)?CHKD:UNC,
		(flags & F_SYSOP)?CHKD:UNC,
		(flags & F_POLLRMS)?CHKD:UNC,
		(flags & F_Expert)?CHKD:UNC,
		SSID[0], SSID[1], SSID[2], SSID[3],
		(flags & F_Excluded)?CHKD:UNC,
		(flags & F_HOLDMAIL)?CHKD:UNC,
		(flags & F_SYSOP_IN_LM)?CHKD:UNC,
		(flags & F_NOWINLINK)?CHKD:UNC,
		(flags & F_NOBULLS)?UNC:CHKD,		// Inverted flag
		(flags & F_NTSMPS)?CHKD:UNC,
		(flags & F_RMSREDIRECT)?CHKD:UNC,
		(flags & F_APRSMFOR)?CHKD:UNC, ASSID,

		ConnectsIn, MsgsReceived, MsgsRejectedIn,
		ConnectsOut, MsgsSent, MsgsRejectedOut,
		BytesForwardedIn, FormatDateAndTime((time_t)User->TimeLastConnected, FALSE), 
		BytesForwardedOut, User->lastmsg,
		User->Name,
		User->pass,
		HiddenPass,
		User->Address,
		User->ZIP,
		User->HomeBBS);

	return Len;
}

#ifdef WIN32

int ProcessWebmailWebSock(char * MsgPtr, char * OutBuffer);

static char PipeFileName[] = "\\\\.\\pipe\\BPQMailWebPipe";

// Constants

static DWORD WINAPI InstanceThread(LPVOID lpvParam)

// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{ 
	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
	BOOL fSuccess = FALSE;
	HANDLE hPipe  = NULL;
	char Buffer[250000];
	char OutBuffer[250000];
	char * MsgPtr;
	int InputLen = 0;
	int OutputLen = 0;
	struct HTTPConnectionInfo Session;
	char URL[100001];
	char * Context, * Method;
	int n;
	char token[16]= "";

	char * ptr;

	// The thread's parameter is a handle to a pipe object instance. 

	hPipe = (HANDLE) lpvParam; 

	// First block is the HTTPConnectionInfo record, rest is request

	n = ReadFile(hPipe, &Session, sizeof (struct HTTPConnectionInfo), &n, NULL);

	// Get the data

	fSuccess = ReadFile(hPipe, Buffer, 250000, &InputLen, NULL);

	if (!fSuccess || InputLen == 0)
	{   
		if (GetLastError() == ERROR_BROKEN_PIPE)
			Debugprintf("InstanceThread: client disconnected.", GetLastError()); 
		else
			Debugprintf("InstanceThread ReadFile failed, GLE=%d.", GetLastError()); 

		return 1;
	}

	Buffer[InputLen] = 0;

	MsgPtr = &Buffer[0];

	if (memcmp(MsgPtr,  "WMRefresh", 9) == 0)
	{
		OutputLen = ProcessWebmailWebSock(MsgPtr, OutBuffer);
	}
	else
	{
		// look for auth header	
		
		const char * auth_header = "Authorization: Bearer ";
		char * token_begin = strstr(MsgPtr, auth_header);
		int Flags = 0;

		// Node Flags isn't currently used

		if (token_begin)
		{
			// Using Auth Header

			// Extract the token from the request (assuming it's present in the request headers)

			token_begin += strlen(auth_header); // Move to the beginning of the token
			strncpy(token, token_begin, 13);
			token[13] = '\0'; // Null-terminate the token
		}
	}

	strcpy(URL, MsgPtr);



	ptr = strstr(URL, " HTTP");

	if (ptr)
		*ptr = 0;

	Method = strtok_s(URL, " ", &Context);

	ProcessMailHTTPMessage(&Session, Method, Context, MsgPtr, OutBuffer, &OutputLen, InputLen, token);


	WriteFile(hPipe, &Session, sizeof (struct HTTPConnectionInfo), &n, NULL);
	WriteFile(hPipe, OutBuffer, OutputLen, &cbWritten, NULL); 

	FlushFileBuffers(hPipe); 
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe);

	return 1;
}

static DWORD WINAPI PipeThreadProc(LPVOID lpvParam)
{
	BOOL   fConnected = FALSE; 
	DWORD  dwThreadId = 0; 
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL; 
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and this loop is free to wait for the
// next client connect request. It is an infinite loop.
 
	for (;;) 
	{ 
      hPipe = CreateNamedPipe( 
          PipeFileName,             // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
          PIPE_TYPE_BYTE |       // message type pipe 
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          4096,                  // output buffer size 
          4096,                  // input buffer size 
          0,                        // client time-out 
          NULL);                    // default security attribute 

      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          Debugprintf("CreateNamedPipe failed, GLE=%d.\n", GetLastError()); 
          return -1;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      fConnected = ConnectNamedPipe(hPipe, NULL) ? 
         TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 
 
      if (fConnected) 
	  {
         // Create a thread for this client. 
   
		 hThread = CreateThread( 
            NULL,              // no security attribute 
            0,                 // default stack size 
            InstanceThread,    // thread proc
            (LPVOID) hPipe,    // thread parameter 
            0,                 // not suspended 
            &dwThreadId);      // returns thread ID 

         if (hThread == NULL) 
         {
            Debugprintf("CreateThread failed, GLE=%d.\n", GetLastError()); 
            return -1;
         }
         else CloseHandle(hThread); 
       } 
      else 
        // The client could not connect, so close the pipe. 
         CloseHandle(hPipe); 
   } 

   return 0; 
} 

BOOL CreatePipeThread()
{
	DWORD ThreadId;
	CreateThread(NULL, 0, PipeThreadProc, 0, 0, &ThreadId);
	return TRUE;
}

#endif

char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

VOID FormatTime(char * Time, time_t cTime)
{
	struct tm * TM;
	TM = gmtime(&cTime);

	sprintf(Time, "%s, %02d %s %3d %02d:%02d:%02d GMT", dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon],
		TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);
}







