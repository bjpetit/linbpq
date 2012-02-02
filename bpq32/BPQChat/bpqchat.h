#pragma once

#include "resource.h"

// Standard __except handler for try/except

VOID CheckProgramErrors();

extern int ProgramErrors;

extern struct _EXCEPTION_POINTERS exinfox;


Dump_Process_State(struct _EXCEPTION_POINTERS * exinfo, char * Msg);

#define My__except_Routine(Message) \
__except(memcpy(&exinfo, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)\
{\
	Debugprintf("CHAT *** Program Error %x at %x in %s EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x",\
		exinfo.ExceptionRecord->ExceptionCode, exinfo.ExceptionRecord->ExceptionAddress, Message,\
		exinfo.ContextRecord->Eax, exinfo.ContextRecord->Ebx, exinfo.ContextRecord->Ecx,\
		exinfo.ContextRecord->Edx, exinfo.ContextRecord->Esi, exinfo.ContextRecord->Edi);\
		CheckProgramErrors();\
}

/*
#define My__except_Routine(Message) \
__except(memcpy(&exinfox, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)\
{\
	Dump_Process_State(&exinfox, Message);\
	CheckProgramErrors();\
}

#define My__except_RoutineWithDisconnect(Message) \
__except(memcpy(&exinfo, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)\
{\
	Debugprintf("MAILCHAT *** Program Error %x at %x in %s EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x",\
		exinfo.ExceptionRecord->ExceptionCode, exinfo.ExceptionRecord->ExceptionAddress, Message,\
		exinfo.ContextRecord->Eax, exinfo.ContextRecord->Ebx, exinfo.ContextRecord->Ecx,\
		exinfo.ContextRecord->Edx, exinfo.ContextRecord->Esi, exinfo.ContextRecord->Edi);\
	FreeSemaphore(&ChatSemaphore);\
	if (conn->BPQStream <  0)\
		CloseConsole(conn->BPQStream);\
	else\
		Disconnect(conn->BPQStream);\
}
*/
#define My_except_RoutineWithDiscBBS(Message) \
__except(memcpy(&exinfo, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)\
{\
	Debugprintf("CHAT *** Program Error %x at %x in %s EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x",\
		exinfo.ExceptionRecord->ExceptionCode, exinfo.ExceptionRecord->ExceptionAddress, Message,\
		exinfo.ContextRecord->Eax, exinfo.ContextRecord->Ebx, exinfo.ContextRecord->Ecx,\
		exinfo.ContextRecord->Edx, exinfo.ContextRecord->Esi, exinfo.ContextRecord->Edi);\
	if (conn->BPQStream <  0)\
		CloseConsole(conn->BPQStream);\
	else\
		Disconnect(conn->BPQStream);\
	CheckProgramErrors();\
}

#define MAXUSERNAMELEN 6

#define WSA_ACCEPT WM_USER + 1
#define WSA_CONNECT WM_USER + 2
#define WSA_DATA WM_USER + 3
#define NNTP_ACCEPT WM_USER + 4
#define NNTP_DATA WM_USER + 5

#ifdef _DEBUG

VOID * _malloc_dbg_trace(int len, int type, char * file, int line);

#define   malloc(s)             _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   calloc(c, s)          _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   realloc(p, s)         _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _recalloc(p, c, s)    _recalloc_dbg(p, c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   _expand(p, s)         _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define   free(p)               _free_dbg(p, _NORMAL_BLOCK)
#define   _strdup(s)			_strdup_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)


#define   zalloc(s)             _zalloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define   zalloc(s)             _zalloc(s)
#endif

VOID * _zalloc_dbg(int len, int type, char * file, int line);

#define LOG_BBS 0
#define LOG_CHAT 1
#define LOG_TCP 2
#define LOG_DEBUG 3

typedef struct SEM
{
	int Flag;
	int Clashes;
};

//Chat Duplicate suppression Code

#define MAXDUPS 10			// Number to keep
#define DUPSECONDS 5		// TIme to Keep

typedef struct DUPINFO
{
	time_t DupTime;
	char  DupUser[10];
	char  DupText[100];
};


struct UserRec
{
	char * Callsign;
	char * UserName;
	char * Password;
};

//#define ln_ibuf 128
#define deftopic "General"


// Protocol version.

#define FORMAT       1	 // Ctrl/A
#define FORMAT_O     0   // Offset in frame to format byte.
#define TYPE_O       1   // Offset in frame to kind byte.
#define DATA_O       2   // Offset in frame to data.

// Protocol Frame Types.

#define id_join   'J'    // User joins RT.
#define id_leave  'L'    // User leaves RT.
#define id_link   'N'    // Node joins RT.
#define id_unlink 'Q'    // Node leaves RT.
#define id_data   'D'    // Data for all users.
#define id_send   'S'    // Data for one user.
#define id_topic  'T'    // User changes topic.
#define id_user   'I'    // User login information.
#define id_keepalive   'K'    // Node-Node Keepalive.

#define o_all    1  // To all users.
#define o_one    2  // To a specific user.
#define o_topic  3  // To all users in a specific topic.


// RT protocol version 1.
// First two bytes are FORMAT and Frame Type.
// These are followed by text fields delimited by blanks.
// Note that "node", "to", "from", "user" are callsigns.

// ^AD<node> <user> <text>        - Data for all users.
// ^AI<node> <user> <name> <qth>  - User information.
// ^AJ<node> <user> <name> <qth>  - User joins.
// ^AL<node> <user> <name> <qth>  - User leaves.
// ^AN<node> <node> <alias>       - Node joins.
// ^AQ<node> <node>               - Node leaves.
// ^AS<node> <from> <to>   <text> - Data for one user.
// ^AT<node> <user> <topic>       - User changes topic.

// Connect protocol:

// 1. Connect to node.
// 2. Send *RTL
// 3. Receive OK. Will get disconnect if link is not allowed.
// 4. Go to it.

// Disconnect protocol:

// 1. If there are users on this node, send an id_leave for each user,
//    to each node you are disconnecting from.
// 2. Disconnect.

// Other RT systems to link with. Flags can be p_linked, p_linkini.

typedef struct link_t
{
	struct link_t *next;
	char *alias;
	char *call;
	int  flags; // See circuit flags.
	int delay;	// Limit connects when failing

} LINK;


typedef struct knownnode_t
{
	struct knownnode_t *next;
	char *call;
	time_t LastHeard;

} KNOWNNODE;


// Topics.

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
} NODE;


// Topics in use at each circuit.

typedef struct ct_t
{
	struct ct_t *next;
	TOPIC *topic;
	int  refcnt;
} CT;

// Nodes reached on each circuit.

typedef struct cn_t
{
	struct cn_t *next;
	NODE *node;
	int refcnt;
} CN;

// Circuits.
// A circuit may be used by one local user, or one link.
// If it is used by a link, there may be many users on that link.

// Bits for circuit flags and link flags.

#define p_nil     0x00    // Circuit is being shut down.
#define p_user    0x01    // User connected.
#define p_linked  0x02    // Active link with another RT.
#define p_linkini 0x04    // Outgoing link setup with another RT.
#define p_linkwait 0x08   // Incoming link setup - waiting for *RTL


// Users. Could be connected at any node.

#define u_echo 0x0002		// User wants his text echoed to him.
#define u_bells 0x0004		// User wants bell when other users join.
#define u_colour 0x0008		// User wants BPQTerminal colour codes.
#define u_keepalive 0x0010	// User wants Keepalive Messages.
#define u_shownames 0x0020	// User wants name as well as call on each message.

typedef struct ConnectionInfo_S
{
	struct ConnectionInfo_S *next;
	PROC *proc;
	UCHAR rtcflags;             // p_linked or p_user.
	int s;                 // Socket.
//	char buf[ln_ibuf];      // Line of incoming text.
	union
	{
		struct user_t *user;  // Associated user if local.
		LINK *link;           // Associated link if link.
	} u;
	int refcnt;            // If link, # of users on that link.
	CN   *hnode;            // Nodes heard from this link.
	CT   *topic;            // Out this circuit if from these topics.

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

} ConnectionInfo, CIRCUIT;

// Flags Equates

#define GETTINGUSER 1
#define CHATMODE 4
#define CHATLINK 32					// Link to another Chat Node

// BBSFlags Equates


#pragma pack(1)

struct TempUserInfo
{
	int LastAuthCode;				// Protect against playback attack
};

struct UserInfo{

	char	Call[10];			//	Connected call without SSID	
	char	Name[18];			/* 18 1st Name */

}; 

// flags equates

#define F_Excluded   0x0001
#define F_LOC        0x0002
#define F_Expert     0x0004
#define F_SYSOP      0x0008
#define F_BBS        0x0010
#define F_PAG        0x0020
#define F_GST        0x0040
#define F_MOD        0x0080
#define F_PRV        0x0100
#define F_UNP        0x0200
#define F_NEW        0x0400
#define F_PMS        0x0800
#define F_EMAIL      0x1000
#define F_HOLDMAIL   0x2000
#define F_POLLRMS	 0x4000
#define F_SYSOP_IN_LM 0x8000
#define F_Temp_B2_BBS 0x10000

/* #define F_PWD        0x1000 */


typedef struct user_t
{
	struct  user_t *next;
	char    *call;
	char    *name;
	char    *qth;
	NODE    *node;          // Node user logged into.
	CIRCUIT *circuit;       // Circuit user is on, local or link.
	TOPIC   *topic;         // Topic user is in.
	int     rtflags;
	time_t	lastmsgtime;	// Time of last input from user
	time_t	lastsendtime;	// Time of last output to user
	int Colour;				// For Console Display
} USER;

#pragma pack()


#pragma pack(1)


#pragma pack()


#define MAXSTACK 20
//#define MAXLINE 10000
#define INPUTLEN 512

#define MAXLINES 1000
#define LINELEN 200

char RTFHeader[4000];

int RTFHddrLen;

struct ConsoleInfo 
{
	struct ConsoleInfo * next;
	CIRCUIT * Console;
	int BPQStream;
	WNDPROC wpOrigInputProc; 
	HWND hConsole;
	HWND hwndInput;
	HWND hwndOutput;
	HMENU hMenu;		// handle of menu 
	RECT ConsoleRect;
	RECT OutputRect;

	int Height, Width, LastY;

	int ClientHeight, ClientWidth;
	char kbbuf[INPUTLEN];
	int kbptr;

	char * readbuff;		// Malloc'ed
	int readbufflen;		// Current Length
	char * KbdStack[MAXSTACK];

	int StackIndex;

	BOOL Bells;
	BOOL FlashOnBell;		// Flash instead of Beep
	BOOL StripLF;

	BOOL WarnWrap;
	BOOL FlashOnConnect;
	BOOL WrapInput;
	BOOL CloseWindowOnBye;

	unsigned int WrapLen;
	int WarnLen;
	int maxlinelen;

	int PartLinePtr;
	int PartLineIndex;		// Listbox index of (last) incomplete line

	DWORD dwCharX;      // average width of characters 
	DWORD dwCharY;      // height of characters 
	DWORD dwClientX;    // width of client area 
	DWORD dwClientY;    // height of client area 
	DWORD dwLineLen;    // line length 
	int nCaretPosX; // horizontal position of caret 
	int nCaretPosY; // vertical position of caret 

	COLORREF FGColour;		// Text Colour
	COLORREF BGColour;		// Background Colour
	COLORREF DefaultColour;	// Default Text Colour

	int CurrentLine;				// Line we are writing to in circular buffer.

	int Index;
	BOOL SendHeader;
	BOOL Finished;

	char OutputScreen[MAXLINES][LINELEN];

	int Colourvalue[MAXLINES];
	int LineLen[MAXLINES];

	int CurrentColour;
	int Thumb;
	int FirstTime;
	BOOL Scrolled;				// Set if scrolled back
	int RTFHeight;				// Height of RTF control in pixels 

};


extern USER *user_hd;

static PROC *Rt_Control;
static int  rtrun = FALSE;

#define rtjoin  "*** Joined"
#define rtleave "*** Left"

KNOWNNODE *knownnode_find(char *call);
static void cn_dec(CIRCUIT *circuit, NODE *node);
static NODE *cn_inc(CIRCUIT *circuit, char *call, char *alias, char * Version);
NODE *node_find(char *call);
static NODE *node_inc(char *call, char *alias, char * Version);
static int cn_find(CIRCUIT *circuit, NODE *node);
static void text_xmit(USER *user, USER *to, char *text);
void text_tellu(USER *user, char *text, char *to, int who);
void text_tellu_Joined(USER *user);
static void topic_xmit(USER *user, CIRCUIT *circuit);
static void node_xmit(NODE *node, char kind, CIRCUIT *circuit);
static void node_tell(NODE *node, char kind);
static void user_xmit(USER *user, char kind, CIRCUIT *circuit);
static void user_tell(USER *user, char kind);
USER *user_find(char *call, char * node);
static void user_leave(USER *user);
static BOOL topic_chg(USER *user, char *s);
static USER *user_join(CIRCUIT *circuit, char *ucall, char *ncall, char *nalias, BOOL Local);
void link_drop(CIRCUIT *circuit);
static void echo(CIRCUIT *fc, NODE *node, char * Buffer);
void state_tell(CIRCUIT *circuit, char * Version);
int ct_find(CIRCUIT *circuit, TOPIC *topic);
int rtlink (char * Call);
int rtloginl (CIRCUIT *conn, char * call);
void chkctl(CIRCUIT *ckt_from, char * Buffer, int Len);
int rtloginu (CIRCUIT *circuit, BOOL Local);
void logout(CIRCUIT *circuit);
void show_users(CIRCUIT *circuit);
VOID __cdecl nprintf(CIRCUIT * conn, const char * format, ...);
BOOL matchi(char * p1, char * p2);
char * strlop(char * buf, char delim);
int rt_cmd(CIRCUIT *circuit, char * Buffer);
CIRCUIT *circuit_new(CIRCUIT *circuit, int flags);
VOID BBSputs(CIRCUIT * conn, char * buf);
void makelinks(void);
VOID * _zalloc(int len);
VOID FreeChatMemory();
VOID ChatTimer();
VOID nputs(CIRCUIT * conn, char * buf);
VOID node_close();
VOID removelinks();
VOID SetupChat();
VOID SendChatLinkStatus();
VOID ClearChatLinkStatus();
void rduser(USER *user);
void upduser(USER *user);
VOID Send_MON_Datagram(UCHAR * Msg, DWORD Len);

#define Connect(stream) SessionControl(stream,1,0)
#define Disconnect(stream) SessionControl(stream,2,0)
#define ReturntoNode(stream) SessionControl(stream,3,0)
#define ConnectUsingAppl(stream, appl) SessionControl(stream, 0, appl)

BOOL Initialise();
INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int DisplaySessions();
int DoStateChange(Stream);
int DoReceivedData(Stream);
int DoMonitorData(Stream);
int Connected(Stream);
int Disconnected(Stream);
int DeleteConnection(con);
//int Socket_Accept(int SocketId);
//int Socket_Data(int SocketId,int error, int eventcode);
int RefreshMainWindow();
int Terminate();
int WriteLog(char * msg);
int ConnectState(Stream);
UCHAR * EncodeCall(UCHAR * Call);
int ParseIniFile(char * fn);
VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user);
VOID ProcessLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID ProcessChatLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user);
int QueueMsg(	ConnectionInfo * conn, char * msg, int len);
VOID SendUnbuffered(int stream, char * msg, int len);
void WriteLogLine(CIRCUIT * conn, int Flag, char * Msg, int MsgLen, int Flags);

void Flush(ConnectionInfo * conn);
VOID ClearQueue(ConnectionInfo * conn);
void TrytoSend();
int	CriticalErrorHandler(char * error);
void chat_link_out (LINK *link);
ProcessConnecting(CIRCUIT * circuit, char * Buffer, int Len);
BOOL SaveConfig();
VOID SaveWindowConfig();
VOID __cdecl nodeprintf(ConnectionInfo * conn, const char * format, ...);

// Console Routines

BOOL CreateConsole(int Stream);
int WritetoConsoleWindow(int Stream, char * Msg, int len);
int ToggleParam(HMENU hMenu, HWND hWnd, BOOL * Param, int Item);
void CopyRichTextToClipboard(HWND hWnd);
void CopyToClipboard(HWND hWnd);
VOID CloseConsole(int Stream);

// Monitor Routines

BOOL CreateMonitor();
int WritetoMonitorWindow(char * Msg, int len);

BOOL CreateDebugWindow();
VOID WritetoDebugWindow(char * Msg, int len);
VOID ClearDebugWindow();
int RemoveLF(char * Message, int len);

// Utilities

BOOL isdigits(char * string);
void GetSemaphore(struct SEM * Semaphore);
void FreeSemaphore(struct SEM * Semaphore);

VOID __cdecl Debugprintf(const char * format, ...);
VOID __cdecl Logprintf(int LogMode, CIRCUIT * conn, int InOut, const char * format, ...);

VOID SortBBSChain();
VOID ExpandAndSendMessage(CIRCUIT * conn, char * Msg, int LOG);

extern HBRUSH bgBrush;
extern BOOL cfgMinToTray;

extern CIRCUIT * Console;

extern ULONG ChatApplMask;
extern char Verstring[];

extern char SignoffMsg[];
extern char AbortedMsg[];
extern char InfoBoxText[];			// Text to display in Config Info Popup

extern int LastVer[4];				// In case we need to do somthing the first time a version is run

extern HWND MainWnd;
extern char BaseDir[];
extern char BaseDirRaw[];
extern char MailDir[];
extern char WPDatabasePath[];
extern char RlineVer[50];

extern BOOL LogBBS;
extern BOOL LogCHAT;
extern BOOL LogTCP;


extern int LatestMsg;
extern char SYSOPCall[];
extern char ChatSID[];
extern char NewUserPrompt[];


extern int Ver[4];

extern struct SEM AllocSemaphore;
extern struct SEM ConSemaphore;
extern struct SEM MsgNoSemaphore;


extern char hostname[];
extern char RtUsr[];
extern char RtUsrTemp[];
extern char RtKnown[];
extern int AXIPPort;
extern BOOL NeedStatus;

extern LINK *link_hd;
extern CIRCUIT *circuit_hd ;			// This is a chain of RT circuits. There may be others
extern char OurNode[];
extern char OurAlias[];
extern BOOL SMTPMsgCreated;

extern HINSTANCE hInst;
extern HWND hWnd;
extern RECT MainRect;

extern int ChatApplNum;

extern int MaxStreams;
extern UCHAR * OtherNodes;
								// Forward Menu Handle
extern char zeros[];						// For forward bitmask tests
extern char *month[]; 

extern HWND hDebug;
extern RECT MonitorRect;
extern RECT DebugRect;
extern HWND hMonitor;
//extern HWND hConsole;
//extern RECT ConsoleRect;
extern int LogAge;
extern BOOL DeletetoRecycleBin;
extern BOOL SuppressMaintEmail;
extern BOOL SaveRegDuringMaint;
extern BOOL SendWP;
extern BOOL OverrideUnsent;
extern BOOL SendNonDeliveryMsgs;

struct ConsoleInfo * ConsHeader[];
