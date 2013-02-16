
#include "stdafx.h"

typedef struct topic_t
{
	struct topic_t *next;
	char  *name;
	int  refcnt;
} TOPIC;

// Nodes.

typedef struct node_t
{
	struct node_t *next;
	char *alias;
	char *call;
	char * Version;
	int refcnt;
} CHATNODE;



typedef struct ct_t
{
	struct ct_t *next;
	TOPIC *topic;
	int  refcnt;
} CT;

typedef struct link_t
{
	struct link_t *next;
	char *alias;
	char *call;
	int  flags; // See circuit flags.
	int delay;	// Limit connects when failing

} LINK;

typedef struct ChatConnectionInfo_S
{
	struct ChatConnectionInfo_S *next;
	PROC *proc;
	UCHAR rtcflags;             // p_linked or p_user.
	int s;                 // Socket.
//	char buf[ln_ibuf];      // Line of incoming text.
	union
	{
		struct user_t *user;  // Associated user if local.
		struct link_t *link;  // Associated link if link.
	} u;
	int refcnt;               // If link, # of users on that link.
	struct cn_t *hnode;       // Nodes heard from this link.
	struct ct_t *topic;       // Out this circuit if from these topics.

	int Number;					// Number of record - for Connections display
	BOOL Active;
    int BPQStream;
	int paclen;
	UCHAR Callsign[11];			// Station call including SSID
    BOOL GotHeader;

	char FBBReplyChars[80];		// Version from other end

    UCHAR InputBuffer[10000];
    int InputLen;				// Data we have already = Offset of end of an incomplete packet;

	struct UserInfo * UserPointer;
    int Retries;
	int	LoginState;				// 1 = user ok, 2 = password ok
	int Flags;

	// Data to the user is kept in a static buffer. This can be appended to,
	// and data sucked out under both terminal and system flow control. PACLEN is
	// enfored when sending to node.

	UCHAR OutputQueue[10000];	// Messages to user
	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	int OutputGetPointer;		// Next byte to send. When Getpointer = Queue Length all is sent - free the buffer and start again.

	int CloseAfterFlush;		// Close session when all sent. Set to 100ms intervals to wait.
	
	BOOL Paging;				// Set if user wants paging
	int LinesSent;				// Count when paging
	int PageLen;				// Lines per page

	BOOL sysop;					// Set if user is authenticated as a sysop
	BOOL Secure_Session;		// Set if Local Terminal, or Telnet connect with SYSOP status

	BOOL NewUser;						// Set if first time user has accessed BBS
	int Watchdog;						// Hung Circuit Detect.
	int SessType;						// BPQ32 sesstype bits

#define Sess_L2LINK 1
#define Sess_SESSION	2
#define Sess_UPLINK	4
#define Sess_DOWNLINK 8
#define Sess_BPQHOST 0x20
#define Sess_PACTOR	0x40

	HANDLE DebugHandle;					// File Handle for session-based debugging

} ChatConnectionInfo, ChatCIRCUIT;

typedef struct user_t
{
	struct  user_t *next;
	char    *call;
	char    *name;
	char    *qth;
	CHATNODE    *node;          // Node user logged into.
	ChatCIRCUIT *circuit;       // Circuit user is on, local or link.
	TOPIC   *topic;         // Topic user is in.
	int     rtflags;
	time_t	lastmsgtime;	// Time of last input from user
	time_t	lastsendtime;	// Time of last output to user
	int Colour;				// For Console Display
} USER;

// Bits for circuit flags and link flags.

#define p_nil     0x00    // Circuit is being shut down.
#define p_user    0x01    // User connected.
#define p_linked  0x02    // Active link with another RT.
#define p_linkini 0x04    // Outgoing link setup with another RT.
#define p_linkwait 0x08   // Incoming link setup - waiting for *RTL


extern char PassError[];
extern char BusyError[];

extern char NodeTail[];
extern BOOL APRSApplConnected;
extern char Mycall[10];

extern char ConfigName[250];
extern char ChatConfigName[250];

extern char OtherNodesList[1000];

extern USER *user_hd;
extern LINK *link_hd;	

extern UCHAR BPQDirectory[260];

extern ConnectionInfo Connections[];
extern int	NumberofStreams;

extern ChatCIRCUIT ChatConnections[];
extern int	NumberofChatStreams;

extern int SMTPMsgs;

extern int ChatApplNum;
extern int MaxChatStreams;

extern char Position[81];
extern char PopupText[251];
extern int PopupMode;



#define MaxCMS	10				// Numbr of addresses we can keep - currently 4 are used.

struct UserInfo * BBSLIST[NBBBS + 1];

int MaxBBS = 0;

struct TCPUserRec
{
	char * Callsign;
	char * UserName;
	char * Password;
	char * Appl;				// Autoconnect APPL
	BOOL Secure;				// Authorised User
};


struct TCPINFO
{
	int NumberofUsers;
	struct TCPUserRec ** UserRecPtr;
	int CurrentConnections;

	struct TCPUserRec RelayUser;

	int CurrentSockets;

	int TCPPort;
	int FBBPort[100];
	int RelayPort;
	int HTTPPort;
	int CMDPort;

	BOOL IPV4;					// Allow Connect using IPV4
	BOOL IPV6;					// Allow Connect using IPV6
	BOOL CMS;					// Allow Connect to CMS
	BOOL CMSOK;					// Internet link is ok.
	BOOL UseCachedCMSAddrs;
	struct in_addr CMSAddr[MaxCMS];
	BOOL CMSFailed[MaxCMS];		// Set if connect to CMS failed.
	char * CMSName[MaxCMS];		// Reverse DNS Name of Server
	int NumberofCMSAddrs;
	int NextCMSAddr;			// Round Robin Pointer
	int CheckCMSTimer;			// CMS Poll Timer

	BOOL DisconnectOnClose;

	char PasswordMsg[100];

	char cfgHOSTPROMPT[100];

	char cfgCTEXT[300];

	char cfgLOCALECHO[100];

	int MaxSessions;

	char LoginMsg[100];

	char RelayAPPL[20];

	SOCKET TCPSock;
	SOCKET FBBsock[100];
	SOCKET Relaysock;
	SOCKET HTTPsock;
	SOCKET sock6;
	SOCKET FBBsock6[100];
	SOCKET Relaysock6;
	SOCKET HTTPsock6;

	fd_set ListenSet;
	int maxsock;

	HMENU hActionMenu;
	HMENU hLogMenu;
	HMENU hDisMenu;					// Disconnect Menu Handle
	HWND hCMSWnd;

};



struct MailConnectionInfo		// Used for Web Server for thread-specific stuff
{
	struct MailConnectionInfo * Next;
	struct UserInfo * User;		// Selected User
	char Key[20];				// Session Key
	struct MsgInfo * Msg;		// Selected Message
	WPRec * WP;					// Selected WP record
	BOOL Connected;
};

static struct MailConnectionInfo * SessionList;	// active bbs config sessions

static struct MailConnectionInfo * AllocateSession(char Appl);
VOID ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int * RLen);
static struct MailConnectionInfo * FindSession(char * Key);
VOID ProcessUserUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessMsgFwdUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendConfigPage(char * Reply, int * ReplyLen, char * Key);
VOID ProcessConfUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessUIUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendUserSelectPage(char * Reply, int * ReplyLen, char * Key);
VOID SendFWDSelectPage(char * Reply, int * ReplyLen, char * Key);
int EncryptPass(char * Pass, char * Encrypt);
VOID ProcessFWDUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SendStatusPage(char * Reply, int * ReplyLen, char * Key);
VOID SendChatStatusPage(char * Reply, int * ReplyLen, char * Key);
VOID SendUIPage(char * Reply, int * ReplyLen, char * Key);
VOID GetParam(char * input, char * key, char * value);
BOOL GetConfig(char * ConfigName);
VOID ProcessDisUser(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessChatDisUser(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
int APIENTRY SessionControl(int stream, int command, int param);
int SendMessageDetails(struct MsgInfo * Msg, char * Reply, char * Key);
VOID ProcessMsgUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID ProcessMsgAction(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
int APIENTRY GetNumberofPorts();
int APIENTRY GetPortNumber(int portslot);
UCHAR * APIENTRY GetPortDescription(int portslot, char * Desc);
struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot);
VOID SendHouseKeeping(char * Reply, int * ReplyLen, char * Key);
VOID SendWelcomePage(char * Reply, int * ReplyLen, char * Key);
VOID SaveWelcome(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
VOID GetMallocedParam(char * input, char * key, char ** value);
VOID SaveMessageText(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SaveHousekeeping(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
VOID SaveWP(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
int SendWPDetails(WPRec * WP, char * Reply, char * Key);
int SendUserDetails(struct MailConnectionInfo * Session, char * Reply, char * Key);
int SetupNodeMenu(char * Buff);
VOID SendFwdSelectPage(char * Reply, int * ReplyLen, char * Key);
VOID SendFwdDetails(struct MailConnectionInfo * Session, char * Reply, int * ReplyLen, char * Key);
VOID SetMultiStringValue(char ** values, char * Multi);
VOID SendFwdMainPage(char * Reply, int * ReplyLen, char * Key);
VOID SaveFwdCommon(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
VOID SaveFwdDetails(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest);
char **	SeparateMultiString(char * MultiString);
VOID SendChatConfigPage(char * Reply, int * ReplyLen, char * Key);
VOID SaveChatInfo(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key);
int rtlink (char * Call);


char UNC[] = "";
char CHKD[] = "checked=checked ";
char sel[] = "selected";

char Sent[] = "#98FFA0";
char ToSend[] = "#FFFF00";
char NotThisOne[] = "#FFFFFF";

char MailSignon[] = "<html><head><title>BPQ32 Mail Server Access</title></head><body background=\"/background.jpg\">"
	"<h3 align=center>BPQ32 Mail Server %s Access</h3>"
	"<h3 align=center>Please enter Callsign and Password to access the BBS</h3>"
	"<form method=post action=/Mail/Signon?Mail>"
	"<table align=center  bgcolor=white>"
	"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
	"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
	"<p align=center><input type=submit value=Submit /><input type=submit value=Cancel name=Cancel /></form>";

char ChatSignon[] = "<html><head><title>BPQ32 Chat Server Access</title></head><body background=\"/background.jpg\">"
	"<h3 align=center>BPQ32 Chat Server %s Access</h3>"
	"<h3 align=center>Please enter Callsign and Password to access the Chat Server</h3>"
	"<form method=post action=/Mail/Signon?Chat>"
	"<table align=center  bgcolor=white>"
	"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
	"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
	"<p align=center><input type=submit value=Submit /><input type=submit value=Cancel name=Cancel /></form>";

char MailPage[] = "<html><head><title>%s's BBS Web Server</title></head>"
	"<body background=\"/background.jpg\"><h3 align=center>BPQ32 BBS %s</h3><P>"
	"<P align=center><table border=1 cellpadding=2 bgcolor=white><tr>"
	"<td><a href=/Mail/Status?%s>Status</a></td>"
	"<td><a href=/Mail/Conf?%s>Configuration</a></td>"
	"<td><a href=/Mail/Users?%s>Users</a></td>"
	"<td><a href=/Mail/Msgs?%s>Messages</a></td>"
	"<td><a href=/Mail/FWD?%s>Forwarding</a></td>"
	"<td><a href=/Mail/Wel?%s>Welcome Msgs & Prompts</a></td>"
	"<td><a href=/Mail/HK?%s>Housekeeping</a></td>"
	"<td><a href=/Mail/WP?%s>WP Update</a></td>"
	"<td><a href=/>Node Menu</a></td>"
	"</tr></table>";

char ChatPage[] = "<html><head><title>%s's Chat Server</title></head>"
	"<body background=\"/background.jpg\"><h3 align=center>BPQ32 Chat Node %s</h3><P>"
	"<P align=center><table border=1 cellpadding=2 bgcolor=white><tr>"
	"<td><a href=/Mail/ChatStatus?%s>Status</a></td>"
	"<td><a href=/Mail/ChatConf?%s>Configuration</a></td>"
	"<td><a href=/>Node Menu</a></td>"
	"</tr></table>";


char RefreshMainPage[] = "<html><head>"
	"<meta http-equiv=refresh content=10>"
	"<title>%s's BBS Web Server</title></head>"
	"<body background=\"/background.jpg\"><h3 align=center>BPQ32 BBS %s</h3><P>"
	"<P align=center><table border=1 cellpadding=2 bgcolor=white><tr>"
	"<td><a href=/Mail/Status?%s>Status</a></td>"
	"<td><a href=/Mail/Conf?%s>Configuration</a></td>"
	"<td><a href=/Mail/Users?%s>Users</a></td>"
	"<td><a href=/Mail/Msgs?%s>Messages</a></td>"
	"<td><a href=/Mail/FWD?%s>Forwarding</a></td>"
	"<td><a href=/Mail/Wel?%s>Welcome Msgs & Prompts</a></td>"
	"<td><a href=/Mail/HK?%s>Housekeeping</a></td>"
	"<td><a href=/Mail/WP?%s>WP Update</a></td>"
	"<td><a href=/>Node Menu</a></td>"
	"</tr></table>";

char StatusPage [] = 

"<form style=\"font-family: monospace; text-align: center\"  method=post action=/Mail/DisSession?%s>"
"<br>User&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Callsign&nbsp;&nbsp; Stream Queue<br>"
"<select style=\"font-family: monospace;\" tabindex=1 size=10 name=call>";

char StatusTail [] = 
"</select><br><input name=Disconnect value=Disconnect type=submit><br><br>"
"Msgs&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; <input readonly=readonly value=%d size=3><br>"
"Sysop Msgs <input readonly=readonly value=%d size=3><br>"
"Held Msgs&nbsp; <input readonly=readonly value=%d size=3><br>"
"SMTP Msgs&nbsp; <input readonly=readonly value=%d size=3><br></form>";


char UIHddr [] = "<form style=\"font-family: monospace;\" align=center method=post"
" action=/Mail/UI?%s> Mailfor Header <input size=40 value=\"%s\" name=MailFor><br>"
"&nbsp;&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; &nbsp;&nbsp;&nbsp;"
"&nbsp;&nbsp;&nbsp; (use \\r to insert newline in message)<br><br>"
"Enable Port&nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp;"
"&nbsp;&nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; Path&nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp;"
"&nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; &nbsp;&nbsp; Send Empty Mailfor<br><br>";

char UILine[] = "<input %sname=En%d type=checkbox> %s <input size=40 value=\"%s\" name=Path%d>"
" <input %sname=SndNull%d type=checkbox><br>";

char UITail[] = "<br><br><input name=Update value=Update type=submit> "
"<input name=Cancel value=Cancel type=submit><br></form>";

char FWDSelectHddr[] = 
	"<form style=\"font-family: monospace; text-align: center;\" method=post action=/Mail/FWDSel?%s>"
	"Max Size to Send &nbsp;&nbsp; <input value=%d size=3 name=MaxTX><br>"
	"Max Size to Receive <input value=%d size=3 name=MaxRX><br>"
	"Warn if no route for P or T <input %sname=WarnNoRoute type=checkbox><br>"
	"Use Local Time&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
	"<input %sname=LocalTime type=checkbox><br><br>"
	"Aliases &nbsp; &nbsp; &nbsp;&nbsp;&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp; Select BBS<br>"
	"<textarea rows=10 cols=20 name=Aliases>%s</textarea> &nbsp<select tabindex=1 size=10 name=call>";

char FWDSelectTail[] =
	"</select><br>&nbsp;&nbsp; <input name=Save value=Save type=submit>&nbsp;<input "
	"name=Cancel value=Cancel type=submit>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
	"&nbsp; <input name=Select value=Select type=submit></form>";

char UserSelectHddr[] = 
	"<form style=\"font-family: monospace; text-align: center\" method=post action=/Mail/Users?%s>"
	"Please Select User<br><br><select tabindex=1 size=10 name=call>";

char UserSelectLine[] = "<option value=%s>%s</option>";

char StatusLine[] = "<option value=%d>%s</option>";

char UserSelectTail[] = "</select><br><br>"
	"<input size=6 value=\"\" name=NewCall>"
	"<input type=submit value=\"Add User\" name=Adduser><br>"
	"<input type=submit value=Select> "
	"<input type=submit value=Cancel name=Cancel><br></form>";

char UserUpdateHddr[] =
	"<h3 align=center>Update User %s</h3>"
	"<form style=\"font-family: monospace; text-align: center\" method=post action=/Mail/Users?%s>";

char UserUpdateLine[] = "<option value=%s>%s</option>";

//<option value="G8BPQ">G8BPQ</option>
//<input checked="checked" name=%s type="checkbox"><br>


char FWDUpdate[] = 
"<h3 align=center>Update Forwarding for BBS %s</h3>"
"<form style=\"font-family: monospace; text-align: center\" method=post action=/Mail/FWD?%s"
" name=Test>&nbsp;&nbsp;&nbsp;&nbsp;"
"TO &nbsp; &nbsp; &nbsp; &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"AT&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
"TIMES&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &nbsp;&nbsp; Connect Script<br>"
"<textarea wrap=hard rows=10 cols=10 name=TO>%s</textarea>"
" <textarea wrap=hard rows=10 cols=10 name=AT>%s</textarea>"
" <textarea wrap=hard rows=10 cols=10 name=Times>%s</textarea>"
" <textarea wrap=hard rows=10 cols=20 name=FWD>%s</textarea><br>"
"<textarea wrap=hard rows=10 cols=30 name=HRB>%s</textarea>"
" <textarea wrap=hard rows=10 cols=30 name=HRP>%s</textarea><br><br>"
"Enable Forwarding&nbsp;<input %sname=EnF type=checkbox> Interval"
"<input value=%d size=3 name=Interval>(Secs) Request Reverse"
"<input %sname=EnR type=checkbox> Interval <input value=%d size=3 "
"name=RInterval>(Secs)<br>"
"Send new messages without waiting for poll timer<input %sname=NoWait type=checkbox><br>"
"BBS HA <input value=%s size=60 name=BBSHA> FBB Max Block <input "
"value=%d size=3 name=FBBBlock><br>"
"Send Personal Mail Only <input %sname=Personal type=checkbox>&nbsp;"
"Allow Binary&nbsp; <input %sname=Bin type=checkbox>&nbsp;&nbsp; Use B1 "
"Protocol <input %sname=B1 type=checkbox>&nbsp; Use B2 Protocol<input "
"%sname=B2 type=checkbox><br><br>"
"<input name=Submit value=Update type=submit> <input name=Fwd value=\"Start Forwarding\" type=submit> "
"<input name=Cancel value=Cancel type=submit></form><br></body></html>";

static char MailDetailPage[] = 
"<html><head><meta content=\"text/html; charset=ISO-8859-1\" http-equiv=\"content-type\">"
"<title>MsgEdit</title></head><body><h4>Message %d</h4>"
"<form style=\"font-family: monospace;\" method=post action=/Mail/Msg?%s name=Msgs>"
"From&nbsp; <input size=10 name=From value=%s> Sent&nbsp;&nbsp;&nbsp;"
"&nbsp; &nbsp; &nbsp; <input readonly=readonly size=12 name=Sent value=\"%s\">&nbsp;"
"Type &nbsp;&nbsp;&nbsp;&nbsp;<select tabindex=1 size=1 name=Type>"
"<option %s value=B>B</option>"
"<option %s value=P>P</option>"
"<option %s value=T>T</option>"
"</select><br><br>"
"To&nbsp; &nbsp; <input size=10 name=To value=%s>"
" Received&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input readonly=readonly size=12 name=RX value=\"%s\">&nbsp;"
"Status &nbsp;&nbsp;<select tabindex=1 size=1 name=Status>"
"<option %s value=N>N</option>"
"<option %s value=Y>Y</option>"
"<option %s value=F>F</option>"
"<option %s value=K>K</option>"
"<option %s value=H>H</option>"
"<option %s value=D>D</option>"
"<option %s value=$>$</option>"
"</select><br><br>"
"BID&nbsp;&nbsp; <input size=10 name=BID value=\"%s\"> Last Changed <input readonly=readonly size=12 name=LastChange value=\"%s\">&nbsp;"
"Size&nbsp; <input readonly=readonly size=5 name=Size value=%d><br><br>"
"VIA&nbsp;&nbsp; <input style=\"width:360px;\" name=VIA value=%s><br><br>"
"Title <input style=\"width:360px;\" name=Title value=\"%s\"> <br><br>"
"<span align = center><input onclick=editmsg(\"EditM?%s?%d\") value=\"Edit Text\" type=button> "
"<input onclick=save(this.form) value=Save type=button> "
"<input onclick=doit(\"SavetoFile\") value=\"Save to File\" type=button> "
"<input onclick=doit(\"Print\") value=Print type=button> "
"<input onclick=doit(\"Export\") value=Export type=button></span><br><br>"
"Green = Sent, Yellow = Queued"
"<table style=\"text-align: left; width: 490px; font-family: monospace; align=center \" border=1 cellpadding=2 cellspacing=2>";

char MailDetailTail[] = "</table></form>";

char Welcome[] = "<form style=\"font-family: monospace; text-align: center;\"" 
"method=post action=/Mail/Welcome?%s>"
"Normal User Welcome<br>"
"<textarea cols=80 rows=3 name=NUWelcome>%s</textarea><br>"
"New User Welcome<br>"
"<textarea cols=80 rows=3 name=NewWelcome>%s</textarea><br>"
"Expert User Welcome<br>"
"<textarea cols=80 rows=3 name=ExWelcome>%s</textarea><br>"
"Normal User Prompt<br>"
"<textarea cols=80 rows=3 name=NUPrompt>%s</textarea><br>"
"New User Prompt<br>"
"<textarea cols=80 rows=3 name=NewPrompt>%s</textarea><br>"
"Expert User Prompt<br>"
"<textarea cols=80 rows=3 name=ExPrompt>%s</textarea><br><br>"
"$U:Callsign of the user&nbsp; $I:First name of the user $X:Messages for user $x:Unread messages<br>"
"$L:Number of the latest message $N:Number of active messages. $Z:Last message read by user<br><br>"
"<input name=Save value=Save type=submit> <inputcname=Cancel value=Cancel type=submit></form>";

static char MsgEditPage[] = "<html><head><meta content=\"text/html; charset=ISO-8859-1\" http-equiv=\"content-type\">"
"<title></title></head><body>"
"<form style=\"font-family: monospace;  text-align: center;\"method=post action=EMSave?%s>"
"<textarea cols=90 rows=33 name=Msg>%s</textarea><br><br>"
"<input name=Save value=Save type=submit><input name=Cancel value=Cancel type=submit><br></form>";

 
static char WPDetail[] = "<form style=\"font-family: monospace;\" method=post action=/Mail/WP?%s>"
"<br><table style=\"text-align: left; width: 431px;\" border=0 cellpadding=2 cellspacing=2><tbody>"
 
"<tr><td>Call</td><td><input readonly=readonly size=10 value=\"%s\"></td></tr>"
"<tr><td>Name</td><td><input size=30 name=Name value=\"%s\"></td></tr>"
"<tr><td>Home BBS 1</td><td><input size=40 name=Home1 value=%s></td></tr>"
"<tr><td>Home BBS 2</td><td><input size=40 name=Home2 value %s></td></tr>"
"<tr><td>QTH 1</td><td><input size=40 name=QTH1 value=\"%s\"></td></tr>"
"<tr><td>QTH 2</td><td><input size=40 name=QTH2 value=\"%s\"></td></tr>"
"<tr><td>ZIP 1<br></td><td><input size=10 name=ZIP1 value=%s></td></tr>"
"<tr><td>ZIP 2<br></td><td><input size=10 name=ZIP2 value=%s></td></tr>"
"<tr><td>Last Seen<br></td><td><input size=15 name=Seen value=\"%s\"></td></tr>"
"<tr><td>Last Modified<br></td><td><input size=15 name=Modif value=\"%s\"></td></tr>"
"<tr><td>Type<br></td><td><input size=4 name=Type value=%c></td></tr>"
"<tr><td>Changed<br></td><td><input size=4 name=Changed value=%d></td></tr>"
"<tr><td>Seen<br></td><td><input size=4 name=Seen value=%d></td></tr></tbody></table>"
"<br><input onclick=save(this.form) value=Save type=button> "
"<input onclick=del(this.form) value=Delete type=button> "
"<input name=Cancel value=Cancel type=submit></form>";


static char LostSession[] = "<html><body>"
"<form style=\"font-family: monospace; text-align: center;\" method=post action=/Mail/Lost?%s>"
"Sorry, Session had been lost<br><br>&nbsp;&nbsp;&nbsp;&nbsp;"
"<input name=Submit value=Restart type=submit> <input type=submit value=Exit name=Cancel><br></form>";

char * MsgEditTemplate = NULL;
char * HousekeepingTemplate = NULL;
char * ConfigTemplate = NULL;
char * ChatConfigTemplate = NULL;
char * WPTemplate = NULL;
char * UserListTemplate = NULL;
char * UserDetailTemplate = NULL;
char * FwdTemplate = NULL;
char * FwdDetailTemplate = NULL;
char * ChatStatusTemplate = NULL;

char * GetTemplateFromFile(char * FN)
{
	int FileSize;
	char * MsgBytes;
	char MsgFile[265];
	FILE * hFile;
	int ReadLen;
	BOOL Special = FALSE;
	struct stat STAT;

	sprintf(MsgFile, "%s/HTML/%s", BPQDirectory, FN);

	if (stat(MsgFile, &STAT) == -1)
	{
		MsgBytes = _strdup("File is missing");
		return MsgBytes;
	}

	hFile = fopen(MsgFile, "rb");
	
	if (hFile == 0)
	{
		MsgBytes = _strdup("File is missing");
		return MsgBytes;
	}

	
	FileSize = STAT.st_size;
	MsgBytes = malloc(FileSize + 1);
	ReadLen = fread(MsgBytes, 1, FileSize, hFile); 
	MsgBytes[FileSize] = 0;
	fclose(hFile);
	return MsgBytes;
}


static int compare(const void *arg1, const void *arg2)
{
   // Compare Calls. Fortunately call is at start of stuct

   return _stricmp(*(char**)arg1 , *(char**)arg2);
}

int SendHeader(char * Reply, char * Key)
{
	return sprintf(Reply, MailPage, Mycall, Mycall, Key, Key, Key, Key, Key, Key, Key, Key);
}

int SendChatHeader(char * Reply, char * Key)
{
	return sprintf(Reply, ChatPage, Mycall, Mycall, Key, Key);
}


void ProcessMailHTTPMessage(struct TCPINFO * TCP, char * Method, char * URL, char * input, char * Reply, int * RLen, SOCKET sock)
{
	char * Conxtext = 0, * NodeURL;
	int ReplyLen;
	struct MailConnectionInfo * Session;
	BOOL LOCAL = FALSE;
	char * Key;
	char Appl = 'M';

	if (strstr(input, "Host: 127.0.0.1"))
		LOCAL = TRUE;

	NodeURL = strtok_s(URL, "?", &Conxtext);
	Key = strtok_s(NULL, "?", &Conxtext);

	if (_stricmp(NodeURL, "/Mail/Signon") == 0)
	{
		ProcessMailSignon(TCP, input, Key, Reply, RLen);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/Lost") == 0)
	{
		input = strstr(input, "\r\n\r\n");	// End of headers

		if (input && strstr(input, "Cancel=Exit"))
		{
			ReplyLen = SetupNodeMenu(Reply);
			*RLen = ReplyLen;
			return;
		}
		if (Key)
			Appl = Key[0];

		Key = 0;
	}

	if (Key == 0 || Key[0] == 0)
	{
		// No Session 

		// if not local send a signon screen, else create a user session

		if (LOCAL)
		{
			if (strstr(NodeURL, "Chat"))
				Appl = 'C';

			Session = AllocateSession(Appl);

			if (Session)
			{
				if (Appl == 'C')
					ReplyLen = SendChatHeader(Reply, Session->Key);
				else
					ReplyLen = SendHeader(Reply, Session->Key);
			}
			else
			{
				ReplyLen = SetupNodeMenu(Reply);
				ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
			}
			*RLen = ReplyLen;
			return;
		}
		if (_stricmp(NodeURL, "/Mail/Chat.html") == 0)
			ReplyLen = sprintf(Reply, ChatSignon, Mycall, Mycall);
		else
			ReplyLen = sprintf(Reply, MailSignon, Mycall, Mycall);

		*RLen = ReplyLen;
		return;
	}

	Session = FindSession(Key);

	if (Session == NULL)
	{
		ReplyLen = sprintf(Reply, LostSession, Key);
		*RLen = ReplyLen;
		return;
	}

	if (strcmp(Method, "POST") == 0)
	{
		if (_stricmp(NodeURL, "/Mail/Config") == 0)
		{
			NodeURL[strlen(NodeURL)] = ' ';				// Undo strtok
			ProcessConfUpdate(Session, input, Reply, RLen, Key);
			return ;
		}

		if (_stricmp(NodeURL, "/Mail/ChatConfig") == 0)
		{
			NodeURL[strlen(NodeURL)] = ' ';				// Undo strtok
			SaveChatInfo(Session, input, Reply, RLen, Key);
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

		if (_stricmp(NodeURL, "/Mail/ChatDisSession") == 0)
		{
			ProcessChatDisUser(Session, input, Reply, RLen, Key);
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
			char  * FF = "", *FT = "", *FV = "";
			char * param, * ptr1, *ptr2;
			struct MsgInfo * Msg;

			// Get filter string

			param = strstr(input, "\r\n\r\n");	// End of headers
			

			if (param)
			{
				ptr1 = param + 4;
				ptr2 = strchr(ptr1, '|');
				if (ptr2){*(ptr2++) = 0; FF = ptr1; ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FT = ptr1;ptr1 = ptr2;}
				ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0; FV = ptr1;ptr1 = ptr2;}
			}

			for (n = NumberofMessages; n >= 1; n--)
			{
				Msg = MsgHddrPtr[n];
				
				if ((!FT[0] || strstr(Msg->to, FT)) &&
					(!FF[0] || strstr(Msg->from, FF)) &&
					(!FV[0] || strstr(Msg->via, FV)))
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
					SendFwdDetails(Session, Reply, RLen, Key); 
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

	if (_stricmp(NodeURL, "/Mail/Status") == 0)
	{
		SendStatusPage(Reply, RLen, Key);
		return;
	}

	if ((_stricmp(NodeURL, "/Mail/ChatStatus") == 0) || (_stricmp(NodeURL, "/Mail/ChatDisSession") == 0))
	{
		if (ChatStatusTemplate)
			free(ChatStatusTemplate);
	
		ChatStatusTemplate = GetTemplateFromFile("ChatStatus.txt");
		SendChatStatusPage(Reply, RLen, Key);

		return;
	}

	if (_stricmp(NodeURL, "/Mail/Conf") == 0)
	{
		if (ConfigTemplate)
			free(ConfigTemplate);

		ConfigTemplate = GetTemplateFromFile("MainConfig.txt");

		SendConfigPage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/ChatConf") == 0)
	{
		if (ChatConfigTemplate)
			free(ChatConfigTemplate);

		ChatConfigTemplate = GetTemplateFromFile("ChatConfig.txt");

		SendChatConfigPage(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/FWD") == 0)
	{
		if (FwdTemplate)
			free(FwdTemplate);

		FwdTemplate = GetTemplateFromFile("FwdPage.txt");

		if (FwdDetailTemplate)
			free(FwdDetailTemplate);

		FwdDetailTemplate = GetTemplateFromFile("FwdDetail.txt");

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

		UserListTemplate = GetTemplateFromFile("UserPage.txt");

		if (UserDetailTemplate)
			free(UserDetailTemplate);

		UserDetailTemplate = GetTemplateFromFile("UserDetail.txt");

		*RLen = sprintf(Reply, UserListTemplate, Key, Key, Mycall,
			Key, Key, Key, Key, Key, Key, Key, Key);
	
		return;
	}

	if (_stricmp(NodeURL, "/Mail/Msgs") == 0)
	{
		struct UserInfo * USER = NULL;

		if (MsgEditTemplate)
			free(MsgEditTemplate);

		MsgEditTemplate = GetTemplateFromFile("MsgPage.txt");

		// Refresh BBS No to BBS list

		MaxBBS = 0;

		for (USER = BBSChain; USER; USER = USER->BBSNext)
		{
			int n = USER->BBSNumber;
			BBSLIST[n] = USER;
			if (n > MaxBBS)
				MaxBBS = n;
		}

		if (MsgEditTemplate)
		{
			int len =sprintf(Reply, MsgEditTemplate, Key, Key, Key, Key, Key, 
				Mycall, Key, Key, Key, Key, Key, Key, Key, Key);
			*RLen = len;
			return;
		}




	}

	if (_stricmp(NodeURL, "/Mail/EditM") == 0)
	{
		// Edit Message

		char * MsgBytes;
	
		MsgBytes = ReadMessageFile(Session->Msg->number);

		// See if Multipart

//		if (Msg->B2Flags & Attachments)
//			EnableWindow(GetDlgItem(hDlg, IDC_SAVEATTACHMENTS), TRUE);

		if (MsgBytes)
		{
			*RLen = sprintf(Reply, MsgEditPage, Key, MsgBytes);
			free (MsgBytes);
		}
		else
			*RLen = sprintf(Reply, MsgEditPage, Key, "Message Not Found");

		return;
	}

	if (_stricmp(NodeURL, "/Mail/HK") == 0)
	{
		if (HousekeepingTemplate)
			free(HousekeepingTemplate);

		HousekeepingTemplate = GetTemplateFromFile("Housekeeping.txt");

		SendHouseKeeping(Reply, RLen, Key);
		return;
	}

	if (_stricmp(NodeURL, "/Mail/WP") == 0)
	{
		if (WPTemplate)
			free(WPTemplate);

		WPTemplate = GetTemplateFromFile("WP.txt");

		if (WPTemplate)
		{
			int len =sprintf(Reply, WPTemplate, Key, Key, Key, Key,
				Mycall, Key, Key, Key, Key, Key, Key, Key, Key);
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

		qsort((void *)WP, i, 4, compare);

		for (i=0; i < NumberofWPrecs; i++)
		{
			len += sprintf(&Reply[len], "%s|", WP[i]->callsign);
		}

		*RLen = len;
		return;
	}


	ReplyLen = sprintf(Reply, MailSignon, Mycall, Mycall);
	*RLen = ReplyLen;

}

int SendWPDetails(WPRec * WP, char * Reply, char * Key)
{
	int len = 0;
	char D1[80], D2[80];
	
	if (WP)
	{
		strcpy(D1, FormatDateAndTime(WP->last_modif, FALSE));
		strcpy(D2, FormatDateAndTime(WP->last_seen, FALSE));

		len = sprintf(Reply, WPDetail, Key, WP->callsign, WP->name,
			WP->first_homebbs, WP->secnd_homebbs,
			WP->first_qth, WP->secnd_qth,
			WP->first_zip, WP->secnd_zip, D1, D2,
			WP->Type,
			WP->changed, 
			WP->seen);
	}
	return(len);	
}
VOID SaveWP(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
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
			*RLen = SendWPDetails(Session->WP, Reply, Session->Key);
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

		*RLen = SendWPDetails(WP, Reply, Key);
	}
}


int SendMessageDetails(struct MsgInfo * Msg, char * Reply, char * Key)
{	
	int BBSNo = 1, x, y, len = 0;
	char D1[80], D2[80], D3[80];
	
	if (Msg)
	{
		strcpy(D1, FormatDateAndTime(Msg->datecreated, FALSE));
		strcpy(D2, FormatDateAndTime(Msg->datereceived, FALSE));
		strcpy(D3, FormatDateAndTime(Msg->datechanged, FALSE));

		len = sprintf(Reply, MailDetailPage, Msg->number, Key,
			Msg->from, D1, 
			(Msg->type == 'B')?sel:"",
			(Msg->type == 'P')?sel:"",
			(Msg->type == 'T')?sel:"",
			Msg->to, D2,
			(Msg->status == 'N')?sel:"",
			(Msg->status == 'Y')?sel:"",
			(Msg->status == 'F')?sel:"",
			(Msg->status == 'K')?sel:"",
			(Msg->status == 'H')?sel:"",
			(Msg->status == 'D')?sel:"",
			(Msg->status == '$')?sel:"",
			Msg->bid, D3, Msg->length, Msg->via, Msg->title,
					Key, Msg->number);

		for (y = 0; y < 10; y++)
		{
			len += sprintf(&Reply[len],"<tr>");
			for (x= 0; x < 8; x++)
			{
				char * Colour  = NotThisOne;	

				if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)	
					if (check_fwd_bit(Msg->fbbs, BBSNo))
							Colour = ToSend;
				if (memcmp(Msg->forw, zeros, NBMASK) != 0)	
					if (check_fwd_bit(Msg->forw, BBSNo))
							Colour = Sent;
				
				if (BBSLIST[BBSNo])
					len += sprintf(&Reply[len],"<td style=\"background-color: %s;\"onclick=ck(\"%d\")>%s</td>",
						Colour, BBSNo, BBSLIST[BBSNo]->Call);
				else
					len += sprintf(&Reply[len], "<td>&nbsp;</td>");

				BBSNo++;
			}
			len += sprintf(&Reply[len],"</tr>");
			if (BBSNo++ > MaxBBS)
				break;
		}
		len += sprintf(&Reply[len], "%s", MailDetailTail);
	}
	return(len);	
}

char ** GetMultiStringInput(char * input, char * key)
{
	char MultiString[2048] = "";

	GetParam(input, key, MultiString);

	if (MultiString[0] == 0)
		return NULL;

	return SeparateMultiString(MultiString);
}

char **	SeparateMultiString(char * MultiString)
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

	Value = zalloc(4);				// always NULL entry on end even if no values
	Value[0] = NULL;

	ptr = DecodedString;

	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, '|');
			
		if (ptr1)
			*(ptr1++) = 0;

		Value = realloc(Value, (Count+2)*4);
		Value[Count++] = _strupr(_strdup(ptr));
		ptr = ptr1;
	}

	Value[Count] = NULL;
	return Value;
}

VOID GetMallocedParam(char * input, char * key, char ** value)
{
	char Param[2048] = "";

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
	char Param[2048];
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

	Value = zalloc(4);				// always NULL entry on end even if no values
	Value[0] = NULL;
	
	while (ptr && strlen(ptr))
	{
		ptr1 = strchr(ptr, 13);
			
		if (ptr1)
		{
			*(ptr1) = 0;
			ptr1 += 2;
		}
		Value = realloc(Value, (Count+2)*4);
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




VOID SaveHousekeeping(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;
	char Temp[80];
	char Multi[2048];

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
		GetParam(input, "MAXMSG=", Temp);
		MaxMsgno = atoi(Temp);
		GetParam(input, "BIDLife=", Temp);
		BidLifetime= atoi(Temp);
		GetParam(input, "LogLife=", Temp);
		LogAge = atoi(Temp);

		GetCheckBox(input, "Deltobin=", &DeletetoRecycleBin);
		GetCheckBox(input, "SendND=", &SendNonDeliveryMsgs);
		GetCheckBox(input, "NoMail=", &SuppressMaintEmail);
		GetCheckBox(input, "OvUnsent=", &OverrideUnsent);

		GetParam(input, "PR=", Temp);
		PR = atoi(Temp);
		GetParam(input, "PUR=", Temp);
		PUR = atoi(Temp);
		GetParam(input, "PF=", Temp);
		PF = atoi(Temp);
		GetParam(input, "PUF=", Temp);
		PNF = atoi(Temp);
		GetParam(input, "BF=", Temp);
		BF = atoi(Temp);
		GetParam(input, "BUF=", Temp);
		BNF = atoi(Temp);

		GetParam(input, "From=", Multi);
		LTFROM = GetOverrideFromString(Multi);

		GetParam(input, "To=", Multi);
		LTTO = GetOverrideFromString(Multi);

		GetParam(input, "At=", Multi);
		LTAT = GetOverrideFromString(Multi);
 	}
	
	SendHouseKeeping(Reply, RLen, Key);
	return;
}







VOID SaveWelcome(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
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
		GetMallocedParam(input, "NUPrompt=", &Prompt);
		GetMallocedParam(input, "NewPrompt=", &NewPrompt);
		GetMallocedParam(input, "ExPrompt=", &ExpertPrompt);
	}
	
	SendWelcomePage(Reply, RLen, Key);
	return;
}

VOID SaveChatInfo(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = NULL;
	char Temp[80];
	char Nodes[1000] = "";
	char * ptr1, * ptr2;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		if (strstr(input, "Cancel=Cancel"))
		{
			*RLen = SendHeader(Reply, Session->Key);
 			return;
		}

	
		GetParam(input, "ApplNum=", Temp);
		ChatApplNum = atoi(Temp);
		GetParam(input, "Streams=", Temp);
		MaxChatStreams = atoi(Temp);

		GetParam(input, "nodes=", Nodes);

		ptr1 = Nodes;
		ptr2 = OtherNodesList;

		while (*ptr1)
		{
			if ((*ptr1) == 13)
			{
				*(ptr2++) = ' ';
				ptr1 += 2;
			}
			else
				*(ptr2++) = *(ptr1++);
		}

		*ptr2 = 0;

		GetParam(input, "Posn=", Position);
		GetParam(input, "MapText=", PopupText);

		GetCheckBox(input, "PopType=Click", &PopupMode);

		if (strstr(input, "Restart=Restart+Links"))
		{
			char * ptr1, * ptr2, * Context;

			node_close();

			Sleep(2);
			
			// Dont call removelinks - they may still be attached to a circuit. Just clear header

			link_hd = NULL;
	 
			// Set up other nodes list. rtlink messes with the string so pass copy
	
			ptr2 = ptr1 = strtok_s(_strdup(OtherNodesList), " ,\r", &Context);

			while (ptr1)
			{
				rtlink(ptr1);			
				ptr1 = strtok_s(NULL, " ,\r", &Context);
			}

			free(ptr2);

			if (user_hd)			// Any Users?
				makelinks();		// Bring up links
		}

		if (strstr(input, "UpdateMap=Update+Map"))
		{
			char Msg[500];
			int len;

			len = sprintf(Msg, "INFO %s|%s|%d|\r", Position, PopupText, PopupMode);

			if (len < 256)
					Send_MON_Datagram(Msg, len);

		}

				
		SaveChatConfig(ChatConfigName);
		GetChatConfig(ChatConfigName);
	}
	
	SendChatConfigPage(Reply, RLen, Key);
	return;
}




VOID ProcessConfUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
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
		GetCheckBox(input, "RefuseBulls=", &RefuseBulls);
		GetCheckBox(input, "EnUI=", &EnableUI);

		GetParam(input, "UIInterval=", Temp);
		MailForInterval = atoi(Temp);

		GetCheckBox(input, "DontHold=", &DontHoldNewUsers);
		GetCheckBox(input, "FWDtoMe=", &ForwardToMe);

		GetParam(input, "POP3Port=", Temp);
		POP3InPort = atoi(Temp);

		GetParam(input, "SMTPPort=", Temp);
		SMTPInPort = atoi(Temp);

		GetParam(input, "NNTPPort=", Temp);
		NNTPInPort = atoi(Temp);

		GetCheckBox(input, "EnRemote=", &RemoteEmail);

		GetCheckBox(input, "EnISP=", &ISP_Gateway_Enabled);

		GetParam(input, "Domain=", MyDomain);
		GetParam(input, "SMTPServer=", ISPSMTPName);
			
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

		if (strstr(input, "Type=TypeB"))
			SendWPType = 'B';

		if (strstr(input, "Type=TypeP"))
			SendWPType = 'P';

		GetParam(input, "WPTO=", SendWPTO);
		GetParam(input, "WPVIA=", SendWPVIA);

		RejFrom = GetMultiStringInput(input, "Rfrom=");
		RejTo = GetMultiStringInput(input, "Rto=");
		RejAt = GetMultiStringInput(input, "Rat=");
		HoldFrom = GetMultiStringInput(input, "Hfrom=");
		HoldTo = GetMultiStringInput(input, "Hto=");
		HoldAt = GetMultiStringInput(input, "Hat=");

		SaveConfig(ConfigName);
		GetConfig(ConfigName);
	}
	
	SendConfigPage(Reply, RLen, Key);
	return;
}



VOID ProcessUIUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Key)
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
			char NullKey[12];
			char Temp[100];

			sprintf(EnKey, "En%d=", i);
			sprintf(DigiKey, "Path%d=", i);
			sprintf(NullKey, "SndNull%d=", i);

			GetCheckBox(input, EnKey, &UIEnabled[i]);
			GetParam(input, DigiKey, Temp);
			if (UIDigi[i])
				free (UIDigi[i]);
			UIDigi[i] = _strdup(Temp);
			GetCheckBox(input, NullKey, &UINull[i]);
		}
	}

	SendUIPage(Reply, RLen, Key);
	return;
}

VOID ProcessDisUser(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
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

VOID ProcessChatDisUser(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	char * input;
	char * ptr;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{
		ptr = strstr(input, "Stream=");
		if (ptr)
		{
			int Stream = atoi(ptr + 7);
			Disconnect(Stream);
		}
	}	
	SendChatStatusPage(Reply, RLen, Rest);
}





VOID SaveFwdCommon(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = NULL;

	char Temp[80];
	int Mask = 0;

	input = strstr(MsgPtr, "\r\n\r\n");	// End of headers

	if (input)
	{

		GetParam(input, "MaxTX=", Temp);
		MaxTXSize = atoi(Temp);
		GetParam(input, "MaxRX=", Temp);
		MaxRXSize = atoi(Temp);
		GetCheckBox(input, "WarnNoRoute=", &WarnNoRoute);
		GetCheckBox(input, "LocalTime=", &Localtime);
		AliasText = GetMultiStringInput(input, "Aliases=");
	}
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

VOID SaveFwdDetails(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
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
			StartForwarding(USER->BBSNumber);
			SendFwdDetails(Session, Reply, RLen, Session->Key);
			return;
		}

		// Fwd update

		ptr2 = input + 4;
		ptr1 = GetNextParam(&ptr2);		// TO
		FWDInfo->TOCalls = SeparateMultiString(ptr1);

		ptr1 = GetNextParam(&ptr2);		// AT
		FWDInfo->ATCalls = SeparateMultiString(ptr1);

		ptr1 = GetNextParam(&ptr2);		// TIMES
		FWDInfo->FWDTimes = SeparateMultiString(ptr1);

		ptr1 = GetNextParam(&ptr2);		// FWD SCRIPT
		FWDInfo->ConnectScript = SeparateMultiString(ptr1);

		ptr1 = GetNextParam(&ptr2);		// HRB
		FWDInfo->Haddresses = SeparateMultiString(ptr1);

		ptr1 = GetNextParam(&ptr2);		// HRP
		FWDInfo->HaddressesP = SeparateMultiString(ptr1);

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

		ptr1 = GetNextParam(&ptr2);		// RInterval
		FWDInfo->MaxFBBBlockSize = atoi(ptr1);

		ptr1 = GetNextParam(&ptr2);		// No Wait
		if (strcmp(ptr1, "true") == 0) FWDInfo->PersonalOnly = TRUE; else FWDInfo->PersonalOnly = FALSE;
		ptr1 = GetNextParam(&ptr2);		// No Wait
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowCompressed = TRUE; else FWDInfo->AllowCompressed = FALSE;
		ptr1 = GetNextParam(&ptr2);		// No Wait
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowB1 = TRUE; else FWDInfo->AllowB1 = FALSE;
		ptr1 = GetNextParam(&ptr2);		// No Wait
		if (strcmp(ptr1, "true") == 0) FWDInfo->AllowB2 = TRUE; else FWDInfo->AllowB2 = FALSE;

		SaveConfig(ConfigName);
		GetConfig(ConfigName);

		ReinitializeFWDStruct(Session->User);
	
		SendFwdDetails(Session, Reply, RLen, Session->Key);
	}
}



VOID ProcessUserUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
{
	int ReplyLen = 0;
	char * input;
	struct UserInfo * USER = Session->User;
	int SSID, Mask = 0;
	char * ptr1, *ptr2;

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
					USER->flags |= F_BBS;
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
		if (strcmp(ptr1, "true") == 0) USER->flags |= F_Temp_B2_BBS; else USER->flags &= ~F_Temp_B2_BBS;
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
		ptr1 = GetNextParam(&ptr2);		// Last Listed
		USER->lastmsg = atoi(ptr1);
		ptr1 = GetNextParam(&ptr2);		// Name
		strcpy(USER->Name, ptr1);
		ptr1 = GetNextParam(&ptr2);		// Pass
		strcpy(USER->pass, ptr1);		
		ptr1 = GetNextParam(&ptr2);		// QTH
		strcpy(USER->Address, ptr1);
		ptr1 = GetNextParam(&ptr2);		// HomeBBS
		strcpy(USER->HomeBBS, ptr1);
		_strupr(USER->HomeBBS);

		SaveUserDatabase();
		UpdateWPWithUserInfo(USER);

		*RLen = SendUserDetails(Session, Reply, Session->Key);
	}
}


VOID ProcessMailSignon(struct TCPINFO * TCP, char * MsgPtr, char * Appl, char * Reply, int * RLen)
{
	int ReplyLen = 0;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Key;

	if (input)
	{
		int i;
		struct TCPUserRec * USER;

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = SetupNodeMenu(Reply);
			*RLen = ReplyLen;
			return;
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

					struct MailConnectionInfo * Session = AllocateSession(Appl[0]);

					if (Session)
					{
						if (Appl[0] == 'C')
							ReplyLen = SendChatHeader(Reply, Session->Key);
						else
							ReplyLen = SendHeader(Reply, Session->Key);
					}
					else
					{
						ReplyLen = SetupNodeMenu(Reply);
						ReplyLen += sprintf(&Reply[ReplyLen], "%s", BusyError);
					}
					*RLen = ReplyLen;
					return;
				}
			}
		}
	}	
	
	ReplyLen = sprintf(Reply, MailSignon, Mycall, Mycall);
	ReplyLen += sprintf(&Reply[ReplyLen], "%s", PassError);
		
	*RLen = ReplyLen;
  
	return;
}

VOID ProcessMsgAction(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
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

VOID SaveMessageText(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
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

		MsgLen = strlen(input + 8);

		Msg->datechanged = time(NULL);
		Msg->length = MsgLen;

			sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
			hFile = fopen(MsgFile, "wb");
	
			if (hFile)
			{
				WriteLen = fwrite(input + 8, 1, Msg->length, hFile); 
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


VOID ProcessMsgUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
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
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->via, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;strcpy(Msg->title, ptr1);ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;Msg->type = *ptr1;ptr1 = ptr2;}
		ptr2 = strchr(ptr1, '|');if (ptr2){*(ptr2++) = 0;Msg->status = *ptr1;ptr1 = ptr2;}

		if (Msg->status != OldStatus)
		{
			// Need to take action if killing message

			if (Msg->status == 'K')
				FlagAsKilled(Msg);					// Clear forwarding bits
		}

		Msg->datechanged = time(NULL);

	}

	*RLen = SendMessageDetails(Msg, Reply, Session->Key);
}




VOID ProcessMsgFwdUpdate(struct MailConnectionInfo * Session, char * MsgPtr, char * Reply, int * RLen, char * Rest)
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
			if (FirstMessageIndextoForward > Msg->number)
				FirstMessageIndextoForward = Msg->number;

		}
		*RLen = SendMessageDetails(Msg, Reply, Session->Key);
	}
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



VOID SendFwdDetails(struct MailConnectionInfo * Session, char * Reply, int * ReplyLen, char * Key)
{
	int Len;
	struct UserInfo * User = Session->User;
	struct BBSForwardingInfo * FWDInfo = User->ForwardingInfo;
	char TO[2048] = "";
	char AT[2048] = "";
	char TIMES[2048] = "";
	char FWD[2048] = "";
	char HRB[2048] = "";
	char HRP[2048] = "";

	SetMultiStringValue(FWDInfo->TOCalls, TO);
	SetMultiStringValue(FWDInfo->ATCalls, AT);
	SetMultiStringValue(FWDInfo->FWDTimes, TIMES);
	SetMultiStringValue(FWDInfo->ConnectScript, FWD);
	SetMultiStringValue(FWDInfo->Haddresses, HRB);
	SetMultiStringValue(FWDInfo->HaddressesP, HRP);
		
	Len = sprintf(Reply, FwdDetailTemplate, User->Call, Key,
		TO, AT, TIMES , FWD, HRB, HRP, 
		(FWDInfo->BBSHA) ? FWDInfo->BBSHA : "", 
		(FWDInfo->Enabled) ? CHKD  : UNC,
		FWDInfo->FwdInterval,
		(FWDInfo->ReverseFlag) ? CHKD  : UNC,
		FWDInfo->RevFwdInterval, 
		(FWDInfo->SendNew) ? CHKD  : UNC,
		FWDInfo->MaxFBBBlockSize,
		(FWDInfo->PersonalOnly) ? CHKD  : UNC,
		(FWDInfo->AllowCompressed) ? CHKD  : UNC,
		(FWDInfo->AllowB1) ? CHKD  : UNC,
		(FWDInfo->AllowB2) ? CHKD  : UNC);

	*ReplyLen = Len;

}

VOID SendChatConfigPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;
	char Nodes[1000];
	char Text[1000];
	char * ptr1, * ptr2;

	//	Replace spaces in Node List with CR/LF

	ptr1 = OtherNodesList;
	ptr2 = Nodes;

	while (*ptr1)
	{
		if ((*ptr1) == ' ')
		{
			*(ptr2++) = 13;
			*(ptr2++) = 10;
			ptr1++ ;
		}
		else
			*(ptr2++) = *(ptr1++);
	}

	*ptr2 = 0;

	// Replace " in Text with &quot; 
		
	ptr1 = PopupText;
	ptr2 = Text;

	while (*ptr1)
	{
		if ((*ptr1) == '"')
		{
			*(ptr2++) = '&';
			*(ptr2++) = 'q';
			*(ptr2++) = 'u';
			*(ptr2++) = 'o';
			*(ptr2++) = 't';
			*(ptr2++) = ';';
			ptr1++ ;
		}
		else
			*(ptr2++) = *(ptr1++);
	}

	*ptr2 = 0;


	Len = sprintf(Reply, ChatConfigTemplate,
		Mycall, Key, Key, Key,
		ChatApplNum, MaxChatStreams, Nodes, Position,
		(PopupMode) ? UNC  : CHKD, 
		(PopupMode) ? CHKD  : UNC,  Text);

	*ReplyLen = Len;
}

VOID SendConfigPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;

	char HF[2048] = "";
	char HT[2048] = "";
	char HA[2048] = "";
	char RF[2048] = "";
	char RT[2048] = "";
	char RA[2048] = "";

	SetMultiStringValue(RejFrom, RF);
	SetMultiStringValue(RejTo, RT);
	SetMultiStringValue(RejAt, RA);
	SetMultiStringValue(HoldFrom, HF);
	SetMultiStringValue(HoldTo, HT);
	SetMultiStringValue(HoldAt, HA);

		
	Len = sprintf(Reply, ConfigTemplate,
		Mycall, Key, Key, Key, Key, Key, Key, Key, Key, Key,
		BBSName, SYSOPCall, HRoute, BBSApplNum, MaxStreams,
		(SendSYStoSYSOPCall) ? CHKD  : UNC,
		(RefuseBulls) ? CHKD  : UNC,
		(EnableUI) ? CHKD  : UNC,
		MailForInterval,
		(DontHoldNewUsers) ? CHKD  : UNC, 
		(ForwardToMe) ? CHKD  : UNC,
		POP3InPort, SMTPInPort, NNTPInPort,
		(RemoteEmail) ? CHKD  : UNC,
		(ISP_Gateway_Enabled) ? CHKD  : UNC,
		MyDomain, ISPSMTPName, ISPSMTPPort, ISPPOP3Name, ISPPOP3Port,
		ISPAccountName, ISPAccountPass, ISPPOP3Interval,
		(SMTPAuthNeeded) ? CHKD  : UNC,
		(SendWP) ? CHKD  : UNC,
		(SendWPType == 'B') ? CHKD  : UNC,
		(SendWPType == 'P') ? CHKD  : UNC, SendWPTO, SendWPVIA,
		RF, RT, RA, HF, HT, HA);

	*ReplyLen = Len;
}
VOID SendHouseKeeping(char * Reply, int * ReplyLen, char * Key)
{
	char FromList[1000]= "", ToList[1000]= "", AtList[1000] = "";
	char Line[80];
	struct Override ** Call;

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

		*ReplyLen = sprintf(Reply, HousekeepingTemplate, 
			 Mycall, Key, Key, Key, Key, Key, Key, Key, Key, Key,
			MaintTime, MaxMsgno, BidLifetime, LogAge,
			(DeletetoRecycleBin) ? CHKD  : UNC,
			(SendNonDeliveryMsgs) ? CHKD  : UNC,
			(SuppressMaintEmail) ? CHKD  : UNC,
			PR, PUR, PF, PNF, BF, BNF,
			FromList, ToList, AtList,
			(OverrideUnsent) ? CHKD  : UNC);

		return;

}


VOID SendWelcomePage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;

	Len = SendHeader(Reply, Key);
		
	Len += sprintf(&Reply[Len], Welcome, Key, WelcomeMsg, NewWelcomeMsg, ExpertWelcomeMsg,
			Prompt, NewPrompt, ExpertPrompt);
	*ReplyLen = Len;
}

VOID SendFwdMainPage(char * Reply, int * RLen, char * Key)
{
	char ALIASES[2048];

	SetMultiStringValue(AliasText, ALIASES);

	*RLen = sprintf(Reply, FwdTemplate, Key, Key, Mycall,
		Key, Key, Key, Key, Key, Key, Key, Key,
		Key, MaxTXSize, MaxRXSize,
		(WarnNoRoute) ? CHKD  : UNC, 
		(Localtime) ? CHKD  : UNC, ALIASES);
}


char TenSpaces[] = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";

VOID SendUIPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len, i;

	Len = SendHeader(Reply, Key);
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
				(UINull[i])?CHKD:UNC, i);
	}

	Len += sprintf(&Reply[Len], UITail, Key);

	*ReplyLen = Len;
}

VOID SendChatStatusPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len = 0;
	USER *user;
	char * Alias;
	char * Topic;
	LINK *link;

	char Streams[8192];
	char Users[8192];
	char Links[8192];

	ChatCIRCUIT * conn;
	int i = 0, n; 

	Users[0] = 0;

	for (user = user_hd; user; user = user->next)
	{
		if ((user->node == 0) || (user->node->alias == 0))
			Alias = "(Corrupt Alias)";
		else
			Alias = user->node->alias;

		if ((user->topic == 0) || (user->topic->name == 0))
			Topic = "(Corrupt Topic)";
		else
			Topic = user->topic->name;

		Len += sprintf(&Users[Len], "<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td></tr>",
			user->call, Alias, user->name, Topic, time(NULL) - user->lastmsgtime, user->qth);
	
	}

	Links[0] = 0;

	Len = 0;

	for (link = link_hd; link; link = link->next)
	{
		if (link->flags & p_linked )
			Len += sprintf(&Links[Len], "<tr><td>%s</td><td>Open</td></tr>", link->call);
		else if (link->flags & (p_linked | p_linkini))
			Len += sprintf(&Links[Len], "<tr><td>%s</td><td>Connecting</td></tr>", link->call);
		else
			Len += sprintf(&Links[Len], "<tr><td>%s</td><td>Idle</td></tr>", link->call);
	}

	Len = 0;
	Streams[0] = 0;

	for (n = 0; n < NumberofChatStreams; n++)
	{
		conn=&ChatConnections[n];
		i = conn->BPQStream;
		if (!conn->Active)
		{
			Len += sprintf(&Streams[Len], "<tr><td onclick= SelectRow(%d) id=cell_%d>Idle</td><td>&nbsp;&nbsp;</td><td>&nbsp;&nbsp;</td><td>&nbsp;&nbsp;</td><td>&nbsp;&nbsp;</td></tr>", i, i);

		}
		else
		{
			if (conn->Flags & CHATLINK)
			{
				Len += sprintf(&Streams[Len], "<tr><td onclick='SelectRow(%d)' id='cell_%d'>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%d</td></tr>",
					i, i, "Chat Link", conn->u.link->alias, conn->BPQStream,
					"", conn->OutputQueueLength - conn->OutputGetPointer);
			}
			else
			if ((conn->Flags & CHATMODE) && conn->topic)
			{
				Len += sprintf(&Streams[Len],  "<tr><td onclick='SelectRow(%d)' id='cell_%d'>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%d</td></tr>",
					i, i, conn->u.user->name, conn->u.user->call, conn->BPQStream,
					conn->topic->topic->name, conn->OutputQueueLength - conn->OutputGetPointer);
			}
			else
			{
				if (conn->UserPointer == 0)
					Len += sprintf(&Streams[Len], "Logging in");
				else
				{
					Len += sprintf(&Streams[Len], "<tr><td onclick='SelectRow(%d)' id='cell_%d'>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%d</td></tr>",
						i, i, conn->UserPointer->Name, conn->UserPointer->Call, conn->BPQStream,
						"CHAT", conn->OutputQueueLength - conn->OutputGetPointer);
				}
			}
		}
	}

	Len = sprintf(Reply, ChatStatusTemplate, Mycall, Mycall, Key, Key, Key, Streams, Users, Links);
	*ReplyLen = Len;
}



VOID SendStatusPage(char * Reply, int * ReplyLen, char * Key)
{
	int Len;
	char msg[1024];
	CIRCUIT * conn;
	int i,n, SYSOPMsgs = 0, HeldMsgs = 0; 
	char Name[80];

	SMTPMsgs = 0;

	Len = sprintf(Reply, RefreshMainPage, Mycall, Mycall, Key, Key, Key, Key, Key, Key, Key, Key);

	Len += sprintf(&Reply[Len], StatusPage, Key);

	for (n = 0; n < NumberofStreams; n++)
	{
		conn=&Connections[n];

		if (!conn->Active)
		{
			strcpy(msg,"Idle&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
								 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
								 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\r\n");
		}
		else
		{
			{
				if (conn->UserPointer == 0)
					strcpy(msg,"Logging in\r\n");
				else
				{
					strcpy(Name, conn->UserPointer->Name);
					Name[9] = 0;

					i=sprintf_s(msg, sizeof(msg), "%s%s%s%s%2d&nbsp;%5d\r\n",
						Name,
						&TenSpaces[strlen(Name) * 6],
						conn->UserPointer->Call,
						&TenSpaces[strlen(conn->UserPointer->Call) * 6],
						conn->BPQStream,
						conn->OutputQueueLength - conn->OutputGetPointer);
				}
			}
		}
		Len += sprintf(&Reply[Len], StatusLine, conn->BPQStream, msg);
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

	qsort((void *)users, i, 4, compare );
		
	for (n = 0; n < NumberofUsers; n++)
	{
		USER = users[n];
		Len += sprintf(&Reply[Len], "%s|", USER->Call);
	}
	*ReplyLen = Len;
}

int SendUserDetails(struct MailConnectionInfo * Session, char * Reply, char * Key)
{
	char SSID[16][16] = {""};
	int i, s, Len;
	struct UserInfo * User = Session->User;
	int flags = User->flags;
	int RMSSSIDBits = Session->User->RMSSSIDBits;
	i = 0;
	
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
	
	Len = sprintf(Reply, UserDetailTemplate, Key, User->Call,
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



		User->nbcon, User->MsgsReceived, User->MsgsRejectedIn,
		User->ConnectsOut, User->MsgsSent, User->MsgsRejectedOut,
		User->BytesForwardedIn, FormatDateAndTime(User->TimeLastConnected, FALSE), 
		User->BytesForwardedOut, User->lastmsg,
		User->Name,
		User->pass,
		User->Address,
		User->HomeBBS);

	return Len;
}

static struct MailConnectionInfo * AllocateSession(char Appl)
{
	int KeyVal;
	struct MailConnectionInfo * Session = zalloc(sizeof(struct MailConnectionInfo));

	if (Session == NULL)
		return NULL;

	KeyVal = time(NULL);

	sprintf(Session->Key, "%c%012X", Appl, KeyVal);

	if (SessionList)
		Session->Next = SessionList;

	SessionList = Session;

	return Session;
}

static struct MailConnectionInfo * FindSession(char * Key)
{
	struct MailConnectionInfo * Session = SessionList;

	while (Session)
	{
		if (strcmp(Session->Key, Key) == 0)
			return Session;

		Session = Session->Next;
	}

	return NULL;
}






