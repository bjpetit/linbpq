#pragma once

#include "resource.h"

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2

#define _USE_32BIT_TIME_T
#include "time.h"

struct UserRec
{
	char * Callsign;
	char * UserName;
	char * Password;
};

#define InputBufferLen 500

#define ln_ibuf 128
#define deftopic "General"


// Protocol version.

#define FORMAT       'A'
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
	UCHAR Callsign[10];			// Station call including SSID
    BOOL GotHeader;
	BOOL InputMode;				// Line by Line or Binary

    UCHAR InputBuffer[1000];
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

//	int PartPacketPointer;
	struct UserInfo * UserPointer;
    int Retries;
	int	LoginState;				// 1 = user ok, 2 = password ok
	int Flags;
//	BOOL DoingCommand;			// Processing Telnet Command
//	BOOL DoEcho;				// Telnet Echo option accepted
	int Conference;				// Conf number in chat mode

	// Data to the user is kept in a malloc'd buffer. This can be appended to,
	// and data sucked out under both terminal and system flow control. PACLEN is
	// enfored when sending to node.

	UCHAR * OutputQueue;		// Messages to user
	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	UCHAR * MailBuffer;			// Mail Message being received
	int MailBufferSize;			// Total Malloc'ed size. Actual size in in Msg Struct

	long lastmsg;				// Last Listed. Stored here, updated in user record only on clean close
	BOOL sysop;					// Set if user is authenticated as a sysop
	struct MsgInfo * TempMsg;		// Header while message is being received
} ConnectionInfo, CIRCUIT;

// Flags Equates

#define GETTINGUSER 1
#define GETTINGBBS 2
#define CHATMODE 4
#define GETTINGTITLE 8
#define GETTINGMESSAGE 16
#define CHATLINK 32					// Link to another Chat Node

#pragma pack(1)

struct UserInfo{

	char	Call[10];			//	Connected call without SSID	
//	indicat relai[8]  ;	/* 64 Digis path */
	long	lastmsg  ;	/* 4  Last L number */
	long	nbcon;	/* 4  Number of connexions */
	time_t	TimeLastCOnnected;  //Last connexion date */
//	long	lastyap __a2__  ;	/* 4  Last YN date */
	short	flags    ;	/* 2  Flags */
	short	on_base  ;	/* 2  ON Base number */

	UCHAR	nbl       ;	/* 1  Lines paging */
	UCHAR	lang      ;	/* 1  Language */

	long	newbanner;	/* 4  Last Banner date */
	short 	download  ;	/* 2  download size (KB) = 100 */
	char	xfree[20]  ;	/* 20 Reserved */
	char	theme     ;	/* 1  Current topic */

	char	Name[18]   ;	/* 18 1st Name */
	char	Address[61] ;	/* 61 Address */
	char	City[31] ;	/* 31 City */
	char	HomeBBS[41]  ;	/* 41 home BBS */
	char	QRA[7]    ;	/* 7  Qth Locator */
	char	pass[13]  ;	/* 13 Password */
	char	ZIP[9]    ;	/* 9  Zipcode */
	BOOL	sysop;
} ;                /* Total : 360 bytes */

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

#define NBBBS 80
#define NBMASK NBBBS/8

#pragma pack(1)

struct MsgInfo{  /* Longueur = 194 octets */
	char	type ;
	char	status ;
	long	number ;
	long	length ;
	long	date   ;
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
	long	datesd ;
	long	datechanged ;
	char	fbbs[NBMASK] ;
	char	forw[NBMASK] ;
} ;



#pragma pack()

static USER *user_hd = NULL;

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
VOID nputs(CIRCUIT * conn, char * buf);
void makelinks(void);

#define Connect(stream) SessionControl(stream,1,0)
#define Disconnect(stream) SessionControl(stream,2,0)
#define ReturntoNode(stream) SessionControl(stream,3,0)
#define ConnectUsingAppl(stream, appl) SessionControl(stream, 0, appl)


typedef struct SocketConnectionInfoX
{
	struct SocketConnectionInfoX * Next;
	int Number;					// Number of record - for Connections display
    SOCKET socket;
	SOCKADDR_IN sin;  
    UCHAR CallSign[10];
    UCHAR TCPBuffer[3000];
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	UCHAR * MailBuffer;			// Mail Message being received
	int MailBufferSize;			// Total Malloc'ed size. Actual size is in MailSize
	int MailSize;
	int Flags;
  
} SocketConn;


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
int ShowConnections();
int Terminate();
int SendtoSocket(SOCKET sock,char * Msg);
int WriteLog(char * msg);
int ConnectState(Stream);
UCHAR * EncodeCall(UCHAR * Call);
int ParseIniFile(char * fn);
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int Q_ADD(UINT *Q,UINT *BUFF);
struct UserInfo * AllocateUserRecord(char * Call);
struct MsgInfo * AllocateMsgRecord();
struct UserInfo * LookupCall(char * Call);
VOID SaveUserDatabase();
VOID GetUserDatabase();
VOID GetMessageDatabase();
VOID SaveMessageDatabase();
VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user);
VOID ProcessLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID ProcessChatLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user);
int QueueMsg(	ConnectionInfo * conn, char * msg, int len);
char * GetConfStations(int Conference);
VOID SendtoOtherUsers(ConnectionInfo * conn, char* Msg, int msglen);
//int GetFileList(char * Dir);
VOID ListMessage(struct MsgInfo * Msg, ConnectionInfo * conn);
void DoKillCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context);
void DoListCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1);
void DoReadCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context);
void KillMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno);
void ListMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call);
void ListMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call);
void ListMessagesInRange(ConnectionInfo * conn, struct UserInfo * user, char * Call, int Start, int End);
int GetUserMsg(int m, char * Call, BOOL SYSOP);
void Flush(ConnectionInfo * conn);
VOID ClearQueue(ConnectionInfo * conn);
void TrytoSend();
void ReadMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno);
struct MsgInfo * FindMessage(char * Call, int msgno, BOOL sysop);
char * ReadMessageFile(int msgno);
char * FormatDateAndTime(time_t Datim, BOOL DateOnly);
int	CriticalErrorHandler(char * error);
void DoSendCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context);
void CreateMessage(ConnectionInfo * conn, struct UserInfo * user, char * ToCall, char MsgType);
VOID ProcessMsgTitle(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID ProcessMsgLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID CreateMessageFile(ConnectionInfo * conn, struct MsgInfo * Msg);
void link_out (LINK *link);
ProcessConnecting(CIRCUIT * circuit, char * Buffer);

// TCP Routines

BOOL InitialiseTCP();
int Socket_Data(int sock, int error, int eventcode);
int Socket_Accept(int SocketId);
VOID ProcessSMTPMessage(SocketConn * sockptr, char * Buffer, int Len);


