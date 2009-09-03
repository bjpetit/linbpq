#pragma once

#include "resource.h"

// Standard __except handler for try/except

#define My__except_Routine(Message) \
__except(memcpy(&exinfo, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)\
{\
	Debugprintf("MAILCHAT *** Program Error %x at %x in %s EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x",\
		exinfo.ExceptionRecord->ExceptionCode, exinfo.ExceptionRecord->ExceptionAddress, Message,\
		exinfo.ContextRecord->Eax, exinfo.ContextRecord->Ebx, exinfo.ContextRecord->Ecx,\
		exinfo.ContextRecord->Edx, exinfo.ContextRecord->Esi, exinfo.ContextRecord->Edi);\
}

#define WSA_ACCEPT WM_USER + 1
#define WSA_CONNECT WM_USER + 2
#define WSA_DATA WM_USER + 3

#ifdef _DEBUG

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


struct UserRec
{
	char * Callsign;
	char * UserName;
	char * Password;
};

#define InputBufferLen 500

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
#define id_keepalive   'K'    // User login information.


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
} LINK;


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


/*typedef struct circuit_t
{
	struct circuit_t *next;
	PROC *proc;
	UCHAR flags;             // p_linked or p_user.
	int s;                 // Socket.
	char buf[ln_ibuf];      // Line of incoming text.
	union
	{
		struct user_t *user;  // Associated user if local.
		LINK *link;           // Associated link if link.
	} u;
	int refcnt;            // If link, # of users on that link.
	CN   *hnode;            // Nodes heard from this link.
	CT   *topic;            // Out this circuit if from these topics.
} CIRCUITZ;
*/

// Users. Could be connected at any node.

#define u_echo 0x0002     // User wants his text echoed to him.
#define u_bells 0x0004    // User wants bell when other users join.


typedef struct ConnectionInfo_S
{
	struct ConnectionInfo_S *next;
	PROC *proc;
	UCHAR flags;             // p_linked or p_user.
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
//    SOCKET socket;
//	SOCKADDR_IN sin;  
	BOOL Active;
    int BPQStream;
	int paclen;
	UCHAR Callsign[11];			// Station call including SSID
    BOOL GotHeader;
	UCHAR InputMode;			// Line by Line or Binary

    UCHAR InputBuffer[1000];
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

//	int PartPacketPointer;
	struct UserInfo * UserPointer;
    int Retries;
	int	LoginState;				// 1 = user ok, 2 = password ok
	int Flags;
//	BOOL DoingCommand;			// Processing Telnet Command
//	BOOL DoEcho;				// Telnet Echo option accepted

	// Data to the user is kept in a malloc'd buffer. This can be appended to,
	// and data sucked out under both terminal and system flow control. PACLEN is
	// enfored when sending to node.

	UCHAR * OutputQueue;		// Messages to user
	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	BOOL Paging;				// Set if user wants paging
	int LinesSent;				// Count when paging
	int PageLen;				// Lines per page

	UCHAR * MailBuffer;			// Mail Message being received
	UCHAR * CopyBuffer;			// Mail Message being forwarded
	int MailBufferSize;			// Total Malloc'ed size. Actual size in in Msg Struct

	long lastmsg;				// Last Listed. Stored here, updated in user record only on clean close
	BOOL sysop;					// Set if user is authenticated as a sysop
	UINT BBSFlags;					// Set if defined as a bbs and SID received
	struct MsgInfo * TempMsg;		// Header while message is being received
	struct MsgInfo * FwdMsg;		// Header while message is being forwarded

	int BBSNumber;						// The BBS number (offset into bitlist of BBSes to forward a message to
	int NextMessagetoForward;			// Next index to check in forward cycle
	struct FBBHeaderLine * FBBHeaders;	// The Headers from an FFB forward block
	char FBBReplyChars[36];				//The +-=!nnnn chars for the 5 proposals
	int FBBReplyIndex;					// current Reply Pointer
	int FBBIndex;						// current propopsal number
	int RestartFrom;					// Restart position
	BOOL FBBMsgsSent;					// Messages need to be maked as complete when next command received
	UCHAR FBBChecksum;					// Header Checksum
	BOOL LocalMsg;				// Set if current Send command is for a local user

} ConnectionInfo, CIRCUIT;

// Flags Equates

#define GETTINGUSER 1
#define GETTINGBBS 2
#define CHATMODE 4
#define GETTINGTITLE 8
#define GETTINGMESSAGE 16
#define CHATLINK 32					// Link to another Chat Node

// BBSFlags Equates

#define BBS 1
#define FBBForwarding 2
#define FBBCompressed 4
#define FBBB1Mode 8
#define FBBB2Mode 16
#define RunningConnectScript 32
#define MBLFORWARDING 64				// MBL Style Frwarding- waiting for OK/NO or Prompt following message

typedef struct FBBRestartData
{
	struct MsgInfo * TempMsg;		// Header while message is being received
	struct UserInfo * UserPointer;
	UCHAR * MailBuffer;				// Mail Message being received
	int MailBufferSize;				// Total Malloc'ed size. Actual size in in Msg Struct
};

#pragma pack(1)

struct UserInfo{

	char	Call[10];			//	Connected call without SSID	
//	indicat relai[8];			/* 64 Digis path */
	long	lastmsg;			/* 4  Last L number */
	long	nbcon;				/* 4  Number of connexions */
	time_t	TimeLastCOnnected;  //Last connexion date */
//	long	lastyap __a2__  ;	/* 4  Last YN date */
	short	flags    ;	/* 2  Flags */
	short	on_base  ;	/* 2  ON Base number */

	UCHAR	PageLen;			// Lines Per Page
	UCHAR	lang      ;	/* 1  Language */

	long	newbanner;	/* 4  Last Banner date */
	short 	download  ;	/* 2  download size (KB) = 100 */
	char	POP3Locked ;		// Nonzero if POP3 server has locked this user (stops other pop3 connections, or BBS user killing messages)
	char	BBSNumber;			// BBS Bitmap Index Number
	struct	BBSForwardingInfo * ForwardingInfo;
	struct  UserInfo * BBSNext;	// links BBS record
	char	xfree[10];			/* 10 Spare */
	char	theme;				/* 1  Current topic */

	char	Name[18];			/* 18 1st Name */
	char	Address[61];		/* 61 Address */
	char	City[31];			/* 31 City */
	char	HomeBBS[41];		/* 41 home BBS */
	char	QRA[7];				/* 7  Qth Locator */
	char	pass[13];			/* 13 Password */
	char	ZIP[9];				/* 9  Zipcode */
	BOOL	spare;
} ;                /* Total : 360 bytes */

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

/* #define F_PWD        0x1000 */

struct Override
{
	char * Call;
	int Days;
};
	


typedef struct user_t
{
	struct  user_t *next;
	char    *call;
	char    *name;
	char    *qth;
	NODE    *node;          // Node user logged into.
	CIRCUIT *circuit;       // Circuit user is on, local or link.
	TOPIC   *topic;         // Topic user is in.
	int     flags;
} USER;

#pragma pack()

// Message Database Entry. Designed to be compatible with FBB

#define NBBBS 80			// Max BBSes we can forward to. Must be Multiple of 8, and must be 80 for FBB compatibliliy
#define NBMASK NBBBS/8		// Number of bytes in Forward bitlists.

#pragma pack(1)

struct MsgInfo{  /* Longueur = 194 octets */
	char	type ;
	char	status ;
	long	number ;
	long	length ;
	long	datereceived;
	char	bbsfrom[7] ;			// ? BBS we got it from ?
	char	via[41] ;
	char	from[7] ;
	char	to[7] ;
	char	bid[13] ;
	char	title[61] ;
	char	bin;
	char	free[9];
	unsigned short	nblu;
	long	theme  ;
	long	datecreated ;
	long	datechanged ;
	char	fbbs[NBMASK] ;
	char	forw[NBMASK] ;
} ;

#define MSGTYPE_B 0
#define MSGTYPE_P 1

#define MSGSTATUS_N 0
#define MSGSTATUS_Y 1
#define MSGSTATUS_F 2
#define MSGSTATUS_K 3
#define MSGSTATUS_H 4
#define MSGSTATUS_$ 5



typedef struct {
	char	mode;
	char	BID[13];
	union 
	{                   /*  array named screen */
		struct    
		{ 
			unsigned short msgno;
			unsigned short timestamp;
		};
		CIRCUIT * conn;
	} u;
} BIDRec, *BIDRecP;


/* Structures fichiers WP */

typedef struct WPREC {	/* 108 bytes */
	long last;
	short  local;
	char source;
	char callsign[7];
	char homebbs[41];
	char zip[9];
	char name[13];
	char qth[31];
} WPMsgRec, * WPMsgRecP;

typedef struct WPDBASE{	/* 194 bytes */
	char callsign[7];
	char name[13];
	unsigned char Type;
	unsigned char changed;
	unsigned short seen;
	long last_modif;
	long last_seen;
	char first_homebbs[41];
	char secnd_homebbs[41];
	char first_zip[9];
	char secnd_zip[9];
	char first_qth[31];
	char secnd_qth[31];
} WPRec, * WPRecP;


#pragma pack()

struct FWDBAND
{
	time_t FWDStartBand;
	time_t FWDEndBand;
};

struct BBSForwardingInfo
{
	// Holds info for forwarding

	BOOL Enabled;					// Forwarding Enabled
	char ** ConnectScript;			// 
	int ScriptIndex;				// Next line in script
	char ** TOCalls;				// Calls in to field
	char ** ATCalls;				// Calls in ATBBS field
	char ** Haddresses;				// Heirarchical Addresses to forward to (as stored)
	char *** HADDRS;				// Heirarchical Addresses to forward to
	char ** FWDTimes;				// Time bands to forward
	struct FWDBAND ** FWDBands;
	int MsgCount;					// Messages for this BBS
	BOOL ReverseFlag;				// Set if BBS wants a poll for reverse forwarding
	BOOL Forwarding;				// Forward in progress
	int MaxFBBBlockSize;
	BOOL AllowB1;					// Enable B1
	BOOL AllowB2;					// Enable B2 
	int FwdInterval;
	int FwdTimer;
};


struct FBBHeaderLine
{
	//	Holds the info from the (up to) 5 headers presented at the start of a Forward Block

	char Format;					// Ascii or Binary
	char MsgType;					// P B etc 
	char From[7];					// Sender
	char ATBBS[41];					// BBS of recipient (@ Field)
	char To[7];						// Recipient
	char BID[13];
	int Size;
	int CSize;						// Compresses Size (B2 proto)
	BOOL B2Message;					// Set if an FC type
	UCHAR * CompressedMsg;			// Compressed Body fo B2
	struct MsgInfo * FwdMsg;		// Header so we can mark as complete 
};

extern USER *user_hd;

static PROC *Rt_Control;
static int  rtrun = FALSE;

#define rtjoin  "*** Joined"
#define rtleave "*** Left"

static void cn_dec(CIRCUIT *circuit, NODE *node);
static NODE *cn_inc(CIRCUIT *circuit, char *call, char *alias);
static NODE *node_find(char *call);
static NODE *node_inc(char *call, char *alias);
static int cn_find(CIRCUIT *circuit, NODE *node);
static void text_xmit(USER *user, USER *to, char *text);
void text_tellu(USER *user, char *text, char *to, int who);
void text_tellu_Joined(USER *user);
static void topic_xmit(USER *user, CIRCUIT *circuit);
static void topic_xmit(USER *user, CIRCUIT *circuit);
static void node_xmit(NODE *node, char kind, CIRCUIT *circuit);
static void node_tell(NODE *node, char kind);
static void user_xmit(USER *user, char kind, CIRCUIT *circuit);
static void user_tell(USER *user, char kind);
static USER *user_find(char *call);
static void user_leave(USER *user);
static void topic_chg(USER *user, char *s);
static USER *user_join(CIRCUIT *circuit, char *ucall, char *ncall, char *nalias);
void link_drop(CIRCUIT *circuit);
static void echo(CIRCUIT *fc, NODE *node, char * Buffer);
void state_tell(CIRCUIT *circuit);
int ct_find(CIRCUIT *circuit, TOPIC *topic);
int rtlink (char * Call);
int rtloginl (CIRCUIT *conn, char * call);
void chkctl(CIRCUIT *ckt_from, char * Buffer);
int rtloginu (CIRCUIT *circuit);
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


#define Connect(stream) SessionControl(stream,1,0)
#define Disconnect(stream) SessionControl(stream,2,0)
#define ReturntoNode(stream) SessionControl(stream,3,0)
#define ConnectUsingAppl(stream, appl) SessionControl(stream, 0, appl)

// TCP Connections. FOr the moment SMTP or POP3

typedef struct SocketConnectionInfo
{
	struct SocketConnectionInfo * Next;
	int Number;					// Number of record - for Connections display
    SOCKET socket;
	SOCKADDR_IN sin; 
	int Type;					// SMTP or POP3
	int State;					// Transaction State Machine
    UCHAR CallSign[10];
    UCHAR TCPBuffer[3000];		// For converting byte stream to messages
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	char * MailFrom;			// Envelope Sender and Receiver
	char ** RecpTo;				// May be several Recipients
	int Recipients;

	UCHAR * MailBuffer;			// Mail Message being received. malloc'ed as needed
	int MailBufferSize;			// Total Malloc'ed size. Actual size is in MailSize
	int MailSize;
	int Flags;

	struct UserInfo * POP3User;
	struct MsgInfo ** POP3Msgs;	// Header List of messages for this uaer
	int POP3MsgCount;			// No of Messages
	int POP3MsgNum;				// Sequence number of message being received


	struct MsgInfo * SMTPMsg;	// message for this SMTP connection


  
} SocketConn;

#define SMTPServer 1
#define POP3SLAVE 2
#define SMTPClient 3
#define POP3Client 4

// State Values

#define GettingUser 1
#define GettingPass 2
#define Authenticated 4

#define Connecting 8

// SMTP Master

#define WaitingForGreeting 16
#define WaitingForHELOResponse 32
#define WaitingForFROMResponse 64
#define WaitingForTOResponse 128
#define WaitingForDATAResponse 256
#define WaitingForBodyResponse 512
#define WaitingForAUTHResponse 1024

// POP3 Master

#define WaitingForUSERResponse 32
#define WaitingForPASSResponse 64
#define WaitingForSTATResponse 128
#define WaitingForUIDLResponse 256
#define WaitingForLISTResponse 512
#define WaitingForRETRResponse 512
#define WaitingForDELEResponse 1024
#define WaitingForQUITResponse 2048


#define SE 240 // End of subnegotiation parameters
#define NOP 241 //No operation
#define DM 242 //Data mark Indicates the position of a Synch event within the data stream. This should always be accompanied by a TCP urgent notification.
#define BRK 243 //Break Indicates that the "break" or "attention" key was hi.
#define IP 244 //Suspend Interrupt or abort the process to which the NVT is connected.
#define AO 245 //Abort output Allows the current process to run to completion but does not send its output to the user.
#define AYT 246 //Are you there Send back to the NVT some visible evidence that the AYT was received.
#define EC 247 //Erase character The receiver should delete the last preceding undeleted character from the data stream.
#define EL 248 //Erase line Delete characters from the data stream back to but not including the previous CRLF.
#define GA 249 //Go ahead Under certain circumstances used to tell the other end that it can transmit.
#define SB 250 //Subnegotiation Subnegotiation of the indicated option follows.
#define WILL 251 //will Indicates the desire to begin performing, or confirmation that you are now performing, the indicated option.
#define WONT 252 //wont Indicates the refusal to perform, or continue performing, the indicated option.
#define DOx 253 //do Indicates the request that the other party perform, or confirmation that you are expecting the other party to perform, the indicated option.
#define DONT 254 //dont Indicates the demand that the other party stop performing, or confirmation that you are no longer expecting the other party to perform, the indicated option.
#define IAC  255

#define suppressgoahead 3 //858
#define Status 5 //859
//#define echo 1 //857
#define timingmark 6 //860
#define terminaltype 24 //1091
#define windowsize 31 //1073
#define terminalspeed 32 //1079
#define remoteflowcontrol 33 //1372
#define linemode 34 //1184
#define environmentvariables 36 //1408

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
int DataSocket_Read(SocketConn * sockptr, SOCKET sock);
int DataSocket_Write(SocketConn * sockptr, SOCKET sock);
int DataSocket_Disconnect(SocketConn * sockptr);
BOOL ProcessTelnetCommand(struct ConnectionInfo * sockptr, UCHAR * Msg, int Len);
int RefreshMainWindow();
int Terminate();
int SendtoSocket(SOCKET sock,char * Msg);
int WriteLog(char * msg);
int ConnectState(Stream);
UCHAR * EncodeCall(UCHAR * Call);
int ParseIniFile(char * fn);
struct UserInfo * AllocateUserRecord(char * Call);
struct MsgInfo * AllocateMsgRecord();
BIDRec * AllocateBIDRecord();
BIDRec * AllocateTempBIDRecord();
struct UserInfo * LookupCall(char * Call);
BIDRec  * LookupBID(char * BID);
BIDRec  * LookupTempBID(char * BID);
VOID RemoveTempBIDS(CIRCUIT * conn);
VOID SaveUserDatabase();
VOID GetUserDatabase();
VOID GetMessageDatabase();
VOID SaveMessageDatabase();
VOID GetBIDDatabase();
VOID SaveBIDDatabase();
VOID GetWPDatabase();
VOID CopyWPDatabase();
VOID SaveWPDatabase();
WPRec * LookupWP(char * Call);
VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user);
VOID ProcessLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID ProcessChatLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user);
int QueueMsg(	ConnectionInfo * conn, char * msg, int len);
VOID SendUnbuffered(int stream, char * msg, int len);
//int GetFileList(char * Dir);
VOID ListMessage(struct MsgInfo * Msg, ConnectionInfo * conn);
void DoKillCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context);
void DoListCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1);
void DoReadCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context);
void KillMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno);
int KillMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call);
int KillMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call);

VOID FlagAsKilled(struct MsgInfo * Msg);
int ListMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call);
int ListMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call);
int ListMessagesAT(ConnectionInfo * conn, struct UserInfo * user, char * Call);
void ListMessagesInRange(ConnectionInfo * conn, struct UserInfo * user, char * Call, int Start, int End);
void ListMessagesInRangeForwards(ConnectionInfo * conn, struct UserInfo * user, char * Call, int Start, int End);
int GetUserMsg(int m, char * Call, BOOL SYSOP);
void Flush(ConnectionInfo * conn);
VOID ClearQueue(ConnectionInfo * conn);
void TrytoSend();
void ReadMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno);
struct MsgInfo * FindMessage(char * Call, int msgno, BOOL sysop);
char * ReadMessageFile(int msgno);
char * FormatDateAndTime(time_t Datim, BOOL DateOnly);
int	CriticalErrorHandler(char * error);
BOOL DoSendCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context);
BOOL CreateMessage(ConnectionInfo * conn, char * From, char * ToCall, char * ATBBS, char MsgType, char * BID, char * Title);
VOID ProcessMsgTitle(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID ProcessMsgLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len);
VOID CreateMessageFile(ConnectionInfo * conn, struct MsgInfo * Msg);
void chat_link_out (LINK *link);
ProcessConnecting(CIRCUIT * circuit, char * Buffer, int Len);
BOOL SaveConfig();
BOOL GetConfigFromRegistry();
VOID Parse_SID(ConnectionInfo * conn, char * SID, int len);
VOID ProcessMBLLine(CIRCUIT * conn, struct UserInfo * user, UCHAR* Buffer, int len);
VOID ProcessFBBLine(ConnectionInfo * conn, struct UserInfo * user, UCHAR * Buffer, int len);
VOID SetupNextFBBMessage(CIRCUIT * conn);
int check_fwd_bit(char *mask, int bbsnumber);
void set_fwd_bit(char *mask, int bbsnumber);
void clear_fwd_bit (char *mask, int bbsnumber);
VOID SetupForwardingStruct(struct UserInfo * user);
BOOL Forward_Message(struct UserInfo * user, struct MsgInfo * Msg);
VOID StartForwarding (int BBSNumber);
BOOL Reverse_Forward(struct UserInfo * user);
ProcessBBSConnectScript(CIRCUIT * conn, char * Buffer, int len);
BOOL FBBDoForward(CIRCUIT * conn);
BOOL FindMessagestoForward (CIRCUIT * conn);
VOID * GetMultiStringValue(HKEY hKey, char * ValueName);
MultiLineDialogToREG_MULTI_SZ(HWND hWnd, int DLGItem, HKEY hKey, char * ValueName);
int Do_BBS_Sel_Changed(HWND hDlg);
VOID FreeForwardingStruct(struct UserInfo * user);
VOID FreeList(char ** Hddr);
int Do_User_Sel_Changed(HWND hDlg);
int Do_Msg_Sel_Changed(HWND hDlg);
VOID Do_Save_Msg();
VOID Do_Add_User(HWND hDlg);
VOID Do_Delete_User(HWND hDlg);
VOID FlagSentMessages(CIRCUIT * conn, struct UserInfo * user);
VOID Do_Save_User(HWND hDlg, BOOL ShowBox);
VOID DeleteBBS();
VOID AddBBS();
VOID SaveBBSConfig();
VOID SaveChatConfig();
VOID SaveISPConfig();
VOID SaveFWDConfig();
VOID SaveMAINTConfig();
VOID SaveWindowConfig();
VOID ReinitializeFWDStruct(struct UserInfo * user);
VOID CopyBIDDatabase();
VOID CopyMessageDatabase();
VOID CopyUserDatabase();
VOID FWDTimerProc();
VOID CreateMessageFromBuffer(CIRCUIT * conn);
VOID __cdecl nodeprintf(ConnectionInfo * conn, const char * format, ...);
VOID FreeOverrides();

// FBB Routines

VOID SendCompressed(CIRCUIT * conn, struct MsgInfo * FwdMsg);
VOID SendCompressedB2(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader);
VOID UnpackFBBBinary(CIRCUIT * conn);
void Decode(CIRCUIT * conn) ;
int Encode(char * in, char * out, int len, BOOL B2Protocol);
CreateB2Message(struct FBBHeaderLine * FBBHeader, char * Rline);
VOID SaveFBBBinary(CIRCUIT * conn);
BOOL LookupRestart(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader);


// Console Routines

BOOL CreateConsole();
int WritetoConsoleWindow(char * Msg, int len);
int ToggleParam(HMENU hMenu, HWND hWnd, BOOL * Param, int Item);
void CopyToClipboard(HWND hWnd);
VOID CloseConsole(int Stream);

// Monitor Routines

BOOL CreateMonitor();
int WritetoMonitorWindow(char * Msg, int len);

BOOL CreateDebugWindow();
VOID WritetoDebugWindow(char * Msg, int len);
VOID ClearDebugWindow();

// Utilities

void GetSemaphore(struct SEM * Semaphore);
void FreeSemaphore(struct SEM * Semaphore);

VOID __cdecl Debugprintf(const char * format, ...);
VOID __cdecl Logprintf(int LogMode, int InOut, const char * format, ...);

VOID SortBBSChain();

// TCP Routines

BOOL InitialiseTCP();
VOID TCPTimer();
int Socket_Data(int sock, int error, int eventcode);
int Socket_Accept(int SocketId);
int Socket_Connect(SOCKET sock, int Error);
VOID ProcessSMTPServerMessage(SocketConn * sockptr, char * Buffer, int Len);
CreateSMTPMessage(SocketConn * sockptr, int i, char * MsgTitle, time_t Date, char * MsgBody, int Msglen);
BOOL CreateSMTPMessageFile(char * Message, struct MsgInfo * Msg);
SOCKET CreateListeningSocket(int Port);
TidyString(char * MailFrom);
VOID ProcessPOP3ServerMessage(SocketConn * sockptr, char * Buffer, int Len);
char *str_base64_encode(char *str);
int b64decode(char *str);
BOOL SMTPConnect(char * Host, int Port, struct MsgInfo * Msg, char * MsgBody);
BOOL POP3Connect(char * Host, int Port);
VOID ProcessSMTPClientMessage(SocketConn * sockptr, char * Buffer, int Len);
VOID ProcessPOP3ClientMessage(SocketConn * sockptr, char * Buffer, int Len);
CreatePOP3Message(char * From, char * To, char * MsgTitle, time_t Date, char * MsgBody, int MsgLen);
void WriteLogLine(int Flag, char * Msg, int MsgLen, int Flags);

BOOL SendtoISP();

md5 (char *arg, unsigned char * checksum);

VOID * GetOverrides(HKEY hKey, char * ValueName);
VOID DoHouseKeeping(BOOL Mainual);
VOID ExpireMessages();
VOID KillMsg(struct MsgInfo * Msg);
BOOL RemoveKilledMessages();
VOID Renumber_Messages();
BOOL ExpireBIDs();
VOID MailHousekeepingResults();

// WP Routines

VOID ProcessWPMsg(char * MailBuffer, int Size, char * FisrtRLine);
VOID GetWPInfoFromRLine(char * From, char * FirstRLine, time_t RLineTime);
VOID UpdateWPWithUserInfo(struct UserInfo * user);

// UI Routines

VOID SetupUIInterface();
VOID Free_UI();
VOID SendLatestUI(int Port);
VOID SendMsgUI(struct MsgInfo * Msg);
VOID Send_AX_Datagram(UCHAR * Msg, DWORD Len, UCHAR Port, UCHAR * HWADDR);
VOID SeeifBBSUIFrame(struct _MESSAGE * buff, int len);
struct MsgInfo * FindMessageByNumber(int msgno);

// Message Routing Routtines

VOID SetupHAddreses(struct	BBSForwardingInfo * ForwardingInfo);
VOID SetupMyHA();

int MatchMessagetoBBSList(struct MsgInfo * Msg, CIRCUIT * conn);
BOOL CheckABBS(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute);
BOOL CheckBBSToList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo);
BOOL CheckBBSAtList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS);
BOOL CheckBBSHList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute);
BOOL CheckBBSHElements(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char ** HElements);




extern HBRUSH bgBrush;
extern BOOL cfgMinToTray;

extern char SignoffMsg[];

extern char InfoBoxText[];			// Text to display in Config Info Popup


extern HWND MainWnd;
extern char BaseDir[];
extern char MailDir[];
extern char WPDatabasePath[];

extern BOOL LogBBS;
extern BOOL LogCHAT;
extern BOOL LogTCP;


extern int LatestMsg;
extern char BBSName[];
extern char SYSOPCall[];
extern char BBSSID[];
extern char NewUserPrompt[];
extern int Ver[4];

extern struct MsgInfo ** MsgHddrPtr;

extern BIDRec ** BIDRecPtr;
extern int NumberofBIDs;

extern int NumberofMessages;
extern int FirstMessagetoForward;

extern WPRec ** WPRecPtr;
extern int NumberofWPrecs;

extern struct SEM AllocSemaphore;
extern struct SEM MsgNoSemaphore;

extern char hostname[];
extern char RtUsr[];
extern char RtUsrTemp[];
extern BOOL ISP_Gateway_Enabled;
extern BOOL SMTPAuthNeeded;


extern int MaxMsgno;
extern int BidLifetime;
extern int MaintInterval;
extern int MaintTime;

extern int MaxRXSize;
extern int MaxTXSize;

extern LINK *link_hd;
extern CIRCUIT *circuit_hd ;			// This is a chain of RT circuits. There may be others
extern char OurNode[];
extern char OurAlias[];
extern BOOL SMTPMsgCreated;

extern HINSTANCE hInst;
extern HWND hWnd;
extern RECT MainRect;

extern char BBSName[];
extern char HRoute[];
extern int BBSApplNum;
extern int ChatApplNum;
extern char BaseDir[];
extern int SMTPInPort;
extern int POP3InPort;
extern BOOL RemoteEmail;

extern int MaxStreams;
extern UCHAR * OtherNodes;
extern struct UserInfo * BBSChain;		// Chain of users that are BBSes
extern struct UserInfo ** UserRecPtr;
extern int NumberofUsers;
extern struct MsgInfo ** MsgHddrPtr;
extern int NumberofMessages;
extern int HighestBBSNumber;
extern HMENU hFWDMenu;									// Forward Menu Handle
extern char zeros[];						// For forward bitmask tests
extern BOOL ALLOWCOMPRESSED;
extern BOOL EnableUI;
extern BOOL UIEnabled[];
extern char * UIDigi[];



extern BOOL ISP_Gateway_Enabled;

extern char MyDomain[];			// Mail domain for BBS<>Internet Mapping

extern char ISPSMTPName[];
extern int ISPSMTPPort;

extern char ISPPOP3Name[];
extern int ISPPOP3Port;
extern int ISPPOP3Interval;

extern char ISPAccountName[];
extern char ISPAccountPass[];
extern char EncryptedISPAccountPass[];
extern int EncryptedPassLen;
extern char *month[]; 

extern CIRCUIT * Console;
extern HWND hConsole;
extern HWND hDebug;
extern RECT ConsoleRect;

extern BOOL Bells;
extern BOOL StripLF;

extern BOOL WarnWrap;
extern BOOL FlashOnConnect;
extern BOOL WrapInput;
extern BOOL CloseWindowOnBye;

extern RECT MonitorRect;
extern HWND hMonitor;

extern int LogAge;
extern BOOL DeletetoRecycleBin;

extern int PR;
extern int PUR;
extern int PF;
extern int PNF;
extern int BF;
extern int BNF;
extern int AP;
extern int AB;

extern struct Override ** LTFROM;
extern struct Override ** LTTO;
extern struct Override ** LTAT;

extern int LastFWDTime;

