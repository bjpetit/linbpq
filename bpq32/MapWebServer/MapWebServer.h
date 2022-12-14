
#include "resource.h"

struct ConnectionInfo
{
	int Number;					// Number of record - for Connections display
    SOCKET socket;
	union
	{
		struct sockaddr_in6 sin6;  
		struct sockaddr_in sin;
	};
	BOOL SocketActive;
    int BPQStream;
    char Callsign[10];
    BOOL GotHeader;
    unsigned char InputBuffer[InputBufferLen];
    int InputLen;
	struct UserRec * UserPointer;
    int Retries;
	int	LoginState;				// 1 = user ok, 2 = password ok
	BOOL DoingCommand;			// Processing Telnet Command
	BOOL DoEcho;				// Telnet Echo option accepted
	BOOL CMSSession;			// Set if connect to CMS
	BOOL FBBMode;				// Pure TCP for FBB forwarding
	BOOL NeedLF;				// FBB mode, but with cr > crlf outbound
	BOOL RelayMode;				// Pure TCP for RMS Relay Emulation forwarding
	BOOL HTTPMode;				// HTML Terminal Emulator
	BOOL TriMode;				// Trimode emulation
	BOOL TriModeConnected;		// Set when remote session is connected - now send data to DataSock
	SOCKET TriModeDataSock;		// Data Socket
	BOOL Auth;					// Set if User is flagged as a Secure User
	BOOL BPQTermMode;			// Set if connected to BPQTermTCP
	BOOL ClientSession;			// Set if acting as a client (ie Linux HOST Mode)
	BOOL MonitorNODES;			// Monitor Control Flags
	UINT MMASK;
	BOOL MCOM;
	BOOL MonitorColour;
	BOOL MTX;
	BOOL MUIOnly;
	int CMSIndex;				// Pointer to CMS used for this connect
	UCHAR * FromHostBuffer;		// Somewhere to store msg from CMS - it sends the whole message at once
	int FromHostBufferSize;
	int FromHostBuffPutptr;
	int FromHostBuffGetptr;

	struct TNCINFO * TNC;		// Used to pass TCP struct to http code (for passwood list)

	time_t ConnectTime;
	BOOL UTF8;					// Set if BPQTerminal in UTF8 Mode
	BOOL RelaySession;			// Set if connection to RMS Relay
	BOOL LogonSent;				// To ignore second callsign: prompt
	char Signon[100];			// User/Pass/Appl for Outgoing Connects
	BOOL Keepalive;				// For HOST (esp CCC) Keepalives 
	time_t LastSendTime;
	UCHAR * ResendBuffer;		// Used if send() returns EWOULDBLOCK
	int	ResendLen;				// Len to resend
};

struct STREAMINFO
{
//	TRANSPORTENTRY * AttachedSession;

	UINT PACTORtoBPQ_Q;			// Frames for BPQ
	UINT BPQtoPACTOR_Q;			// Frames for PACTOR
	int	FramesOutstanding;		// Frames Queued - used for flow control
	int	FramesQueued;			// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands
	BOOL CheckingCall;			// Set on PTC if waiting for I response after a Connect RXed

	BOOL Attached;				// Set what attached to a BPQ32 stream
	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;			// Set when Outward Connect in progress
	BOOL Disconnecting;			// Set when disconnect in progress
								// Used when appplication disconnects the bpq session, and
								// prevents new attaches while a dirty disconnect is in progress
	int DisconnectingTimeout;	// A hard disconnect occurs if this expires before the disconnect complete
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel
	BOOL DiscWhenAllSent;		// Close session when all msgs have been sent to node
	BOOL ARQENDSent;			// Set when V4 ARQEND Sent

	int DEDStream;				// Stream number for DED interface (same as index except for pactor)

	char MyCall[10]	;			// Call we are using
	char RemoteCall[10];		// Callsign

	char AGWKey[21];			// Session Key for AGW Session Based Drivers

	time_t ConnectTime;			// Time connection made
	int BytesTXed;
	int BytesAcked;
	int BytesRXed;
	int PacketsSent;
	int BytesResent;
	int BytesOutstanding;		// For Packet Channels

	UCHAR PTCStatus0;			// Status Bytes
	UCHAR PTCStatus1;			// Status Bytes
	UCHAR PTCStatus2;			// Status Bytes
	UCHAR PTCStatus3;			// Status Bytes

	char * CmdSet;				// A series of commands to send to the TNC
	char * CmdSave;				// Base address for free

	struct ConnectionInfo * ConnectionInfo;	// TCP Server Connection Info

	int TimeInRX;				// Too long in send mode timer
	int NeedDisc;				// Timer to send DISC if appl not available

	BOOL NoCMSFallback;			// Dont use relay if CMS not available
	struct ARQINFO * ARQInfo;	// FLDIGI/FLARQ Stream Mode Specific Data

	HWND xIDC_MYCALL; 
	HWND xIDC_DESTCALL;
	HWND xIDC_STATUS; 
	HWND xIDC_SEND;
	HWND xIDC_RXED; 
	HWND xIDC_RESENT;
	HWND xIDC_ACKED;
	HWND xIDC_DIRN;
};

struct TCPINFO
{
	int NumberofUsers;
	struct UserRec ** UserRecPtr;
	int CurrentConnections;

	int CurrentSockets;

	int TCPPort;
	int FBBPort[100];
	int RelayPort;
	int HTTPPort;
	int TriModePort;
	int CMDPort[33];
	char RELAYHOST[64];
	char CMSServer[64];
	BOOL FallbacktoRelay;		// Use Relsy if can't connect to CMS

	BOOL IPV4;					// Allow Connect using IPV4
	BOOL IPV6;					// Allow Connect using IPV6

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
	SOCKET TriModeSock;
	SOCKET TriModeDataSock;
	struct ConnectionInfo * TriModeControlSession;
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

typedef struct TNCINFO
{ 
	HWND hDlg;						// Status Window Handle
	int (FAR * WebWindowProc)(struct TNCINFO * TNC, char * Buff, BOOL LOCAL);	// Routine to build web status window
	int WebWinX;
	int WebWinY;					// Size of window
	char * WebBuffer;				// Buffer for logs
	int RigControlRow;				// Rig Control Line in Dialog
	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port
	struct RIGINFO * RIG;		// Pointer to Rig Control RIG record 
	char * InitScript;			// Initialisation Commands
	int InitScriptLen;			// Length

	int Hardware;				// Hardware Type

#define H_WINMOR 1
#define H_SCS 2
#define H_KAM 3
#define H_AEA 4
#define H_HAL 5
#define H_TELNET 6
#define H_TRK 7
#define H_TRKM 7
#define H_V4 8
#define H_UZ7HO 9
#define H_MPSK 10
#define H_FLDIGI 11
#define H_UIARQ 12
#define H_ARDOP 13
#define H_AXARDOP 14


	int Port;					// BPQ Port Number

	BOOL Minimized;				// Start Minimized flag

	int WINMORtoBPQ_Q;			// Frames for BPQ, indexed by BPQ Port
	int BPQtoWINMOR_Q;			// Frames for WINMOR. indexed by WINMOR port. Only used it TCP session is blocked

	SOCKET WINMORSock;			// Control Socket
	SOCKET WINMORDataSock;		// Data Socket

	char * WINMORSignon;		// Pointer to message for secure signin
	char * WINMORHostName;		// WINMOR Host - may be dotted decimal or DNS Name
	int WINMORPort;				//
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	BOOL SwallowSignon;			// Set to suppress *** connected to APPL

    union
	{
		UCHAR TCPBuffer[1000];		// For converting byte stream to messages
		UCHAR DEDBuffer[1000];		// For converting byte stream to messages
	};

	UCHAR * ARDOPBuffer;			// Needs to be pretty big, so Malloc
	UCHAR * ARDOPDataBuffer;		// Needs to be pretty big, so Malloc

	int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;
	int DataInputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	int	MSGCOUNT;				// DED WORKING FIELD
	int	MSGLENGTH;				// DED Msg Len
	int	MSGCHANNEL;				// DED Msg Channel Number
	int	MSGTYPE;				// DED Msg Type

	int HOSTSTATE;				// ded HOST state machine


	BOOL StartSent;				// Codec Start send (so will get a disconnect)
	int ConnectPending;			// Set if Connect Pending Received. If so, mustn't allow freq change.
	BOOL DiscPending;			// Set if Disconnect Pending Received. So we can time out stuck in Disconnecting
	BOOL HadConnect;				// Flag to say have been in session
	BOOL FECMode;				// In FEC Mode
	BOOL FEC1600;				// Use 1600 Hz FEC Mode
	int FECIDTimer;				// Time in FEC Mode. Used to trigger ID broadcasts
	BOOL RestartAfterFailure;
	BOOL StartInRobust;			// For WINMOR, set to Robust Mode for first few packets

	int Busy;					// Channel Busy Timer/Counter . Non-zero = Busy

	int BusyFlags;				// Channel Busy Flags

#define CDBusy 1				// For WINMOR - reported busy (set till reported clear)
#define PTTBusy 2				// PTT Active

	BOOL FECPending;			// Need an FEC Send when channel is next idle

	time_t lasttime;

	BOOL CONNECTING;			// TCP Session Flags
	BOOL CONNECTED;
	BOOL Alerted;				// COnnect Failed Prompt sent
	BOOL DATACONNECTING;
	BOOL DATACONNECTED;

	char NodeCall[10];				// Call we listen for (PORTCALL or NODECALL
	char CurrentMYC[10];			// Save current call so we don't change it unnecessarily

	char TargetCall[10];			// Call incoming connect is addressed to (for appl call support)

	struct sockaddr_in  destaddr;
	struct sockaddr_in  Datadestaddr;

	int PTTMode;					// PTT Mode Flags

	int WIMMORPID;
	char * CaptureDevices;
	char * PlaybackDevices;
	char * ProgramPath;
	BOOL WeStartedTNC;

	int Restarts;					// TNC Kill/Restarts done
	time_t LastRestart;
	
	int TimeSinceLast;				// Time since last message from TNC (10ths of a sec)
	int HeartBeat;

	int Interlock;					// Port Interlock Group

	HWND hMonitor;					// Handle to Monitor control
//	HMENU hPopMenu;					// Actions Menu Handle

	int MaxConReq;					// For ARDOP
	int BusyHold;					// Hold Time from SCS reporting channel free till we call 
	int BusyWait;					// Time to wait for clear channel before connect

	BOOL OverrideBusy;
	int BusyDelay;					// Timer for busy timeout
	char * ConnectCmd;				// Saved command if waiting for busy to clear
	BOOL UseAPPLCalls;				// Robust Packet to use Applcalls
	BOOL UseAPPLCallsforPactor;		// Pactor to use Applcalls

	// Fields for reporting to WL2K Map

	struct WL2KInfo * WL2K;

/*
	char * Host;
	short WL2KPort;

	int UpdateWL2KTimer;
	BOOL UpdateWL2K;
	char RMSCall[10];
	char BaseCall[10];
	char GridSquare[7];
	char Comment[80];
	char ServiceCode[17];

	BOOL UseRigCtrlFreqs;
	char WL2KFreq[12];
	char WL2KModeChar;			// W or N
	BOOL DontReportNarrowOnWideFreqs;

//	char NARROWMODE;
//	char WIDEMODE;				// Mode numbers to report to WL2K

	struct WL2KInfo WL2KInfoList[MAXFREQS];		// Freqs for sending to WL2K
*/
	char WL2KMode;				// WL2K reporting mode

	struct STREAMINFO Streams[100];	// 0 is Pactor 1 - 10 are ax.25.
	int LastStream;				// Last one polled for status or send

	void * BPQtoRadio_Q;			// Frames to Rig Interface
	void * RadiotoBPQ_Q;			// Frames from Rig Interface

	char * InitPtr;				// Next Command
	int	ReinitState;			// Reinit State Machine
	int	ReinitCount;			// Count for DED Recovery
	BOOL TNCOK;					// TNC is reponding
	int	FramesOutstanding;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands


	HANDLE hDevice;
	int ReopenTimer;			//	Used to reopen device if failed (eg USB port removed)
	BOOL HostMode;					// Set if in DED Host Mode
//	BOOL CRCMode;					// Set if using SCS Extended DED Mode (JHOST4)
	int Timeout;					// Timeout response counter
	int Retries;
	int Window;						// Window Size for ARQ
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR RXBuffer[520];			// Message being received - may not arrive all at once
	UINT RXLen;						// Data in RXBUffer
	UCHAR Toggle;					// Sequence bit
	int Buffers;					// Free buffers in TNC
	BOOL WantToChangeFreq;			// Request from Scanner to Change
	int OKToChangeFreq;				// 1 = SCS Says OK to change, -1 = Dont Change zero = still waiting
	BOOL DontWantToChangeFreq;		// Change done - ok to  SCS
	BOOL DontReleasePermission;		// Hold Permission to prevent calls on this frequency
	int TimeEnteredSYNCMode;		// To detect scan lock when using applcalls on PTC
	BOOL SyncSupported;				// TNC reports sync
	int TimeScanLocked;				// ditto for TNCs that don't report SYNC
	int PTCStatus;					// Sync, Idle, Traffic, etc
	UCHAR NexttoPoll[20];			// Streams with data outstanding (from General Poll)
	BOOL PollSent;					// Toggle to ensure we issue a general poll regularly
	int StreamtoPoll;

	char Bandwidth;					// Currently set Mode W or N

	int Mode;						// Mode Flag

	BOOL Dragon;					// Set if P4Dragon
	BOOL DragonSingle;				// Set if P4Dragon using Pactor and Packet on same port
	BOOL DragonKISS;				// Set if P4Dragon supports sending KISS frames in Hostmode
	BOOL EnterExit;					// Switching to Term mode to change bandwidth
	int PktStream;					// Stream in use for Packet when in single port mode
	BOOL MaxLevel;					// Pactor Level to set for Wide Mode (3 or 4)
	int MinLevel;					// Mimimum accepted Pactor Level
	int MinLevelTimer;				// Time left to achieve Min Level
	int PacketChannels;
	int RobustTime;					// For PTC, Spend this part of scan cycle (in 10th secs) in Robust Packet Mode 
	int SwitchToPactor;				// Countdown to switch

	BOOL OldMode;					// Use PACTOR instead of TOR (for old software)
	BOOL VeryOldMode;				// Use MYCALL  instead of MYPTCALL (for old software)

	int Mem1;						// Free Bytes (VHF /HF)
	int Mem2;

	BOOL HFPacket;					// Set if HF port is in Packet mode instead of Pactor Mode
	BOOL Robust;					// Set if SCS Tracker is in Robust Packet mode or WINMOR TNC is in Robust Mode
	BOOL RobustDefault;				// Set if SCS Tracker default is Robust Packet mode
	BOOL ForceRobust;				// Don't allow Normal Packet even if scan requests it.
	char NormSpeed[8];				// Speed Param for Normal Packet on Tracker
	BOOL RPBEACON;					// Send Beacon after each session 

	int TimeInRX;					// Time waiting for ISS before sending
	char TXRXState;					// Current ISS/IRS State

	BOOL NeedPACTOR;				// Set if need to send PACTOR to put into Standby Mode
	int CmdStream;					// Stream last command was issued on

	union
	{
		struct TCPINFO * TCPInfo;		// Telnet Server Specific Data
		struct AGWINFO * AGWInfo;		// AGW Stream Mode Specific Data
		struct MPSKINFO * MPSKInfo;		// MPSK Stream Mode Specific Data
		struct FLINFO * FLInfo;			// FLDIGI Stream Mode Specific Data
	};

	struct ARQINFO * ARQInfo;	// FLDIGI/FLARQ Stream Mode Specific Data

	BOOL DataBusy;					// Waiting for Data Ack - Don't send any more data
	BOOL CommandBusy;				// Waiting for Command ACK

	BOOL TEXTMODE;					// Set if AEA in text mode
	BOOL NeedTurnRound;				// Set if we have sent data, so need to send ctrl/z 
	BOOL NeedTRANS;					// Set if we have to send TRANS when ctrl/z is acked. 

	char * CmdSet;					// A series of commands to send to the TNC
	char * CmdSave;					// Base address for free

	BOOL PktUpdateMap;				// Set if Packet MH data to be sent to NodeMap

	int DefaultMode;
	int CurrentMode;				// Used on HAL

	// Mode Equates

	#define Clover 'C'
	#define Pactor 'P'
	#define AMTOR 'A'

	UCHAR DataBuffer[500];			// Data Chars split from  received stream
	UCHAR CmdBuffer[500];			// Cmd/Response chars split from received stream
	int DataLen;					// Data in DataBuffer
	int CmdLen;						// Data in CmdBuffer
	BOOL CmdEsc;						// Set if last char rxed was 0x80
	BOOL DataEsc;					// Set if last char rxed was 0x81
	int PollDelay;					// Don't poll too often;
	int InData;						// FLDigi - MCAST <....> received
	int InPacket;					// FLDigi - SOH or < received.
	int MCASTLen;					// Data still to get

	int DataMode;					// How to treat data 

#define RXDATA  0x30				// Switch to Receive Data characters
#define TXDATA  0x31				// Switch to Transmit Data characters
#define SECDATA 0x32				// Switch to RX data from secondary port

	int TXMode;					// Where to send data 

#define TXMODEM 0x33				// Send TX data to modem
#define TXSEC   0x34				// Send TX data to secondary port

	BOOL XONXOFF;					// Set if hardware is using XON/XOFF

	double LastFreq;				// Used by V4 to see if freq has changed
	int ModemCentre;				// Modem centre frequency
	int ClientHeight;
	int ClientWidth;
	HWND xIDC_TNCSTATE; 
	HWND xIDC_COMMSSTATE;
	HWND xIDC_MODE;
	HWND xIDC_LEDS;
	HWND xIDC_TRAFFIC;
	HWND xIDC_BUFFERS;
	HWND xIDC_CHANSTATE;
	HWND xIDC_STATE;
	HWND xIDC_TXRX;
	HWND xIDC_PROTOSTATE;
	HWND xIDC_RESTARTTIME;
	HWND xIDC_RESTARTS;
	HWND xIDC_PACTORLEVEL;

	char * WEB_TNCSTATE; 
	char * WEB_COMMSSTATE;
	char * WEB_MODE;
	char * WEB_LEDS;
	char * WEB_TRAFFIC;
	char * WEB_BUFFERS;
	char * WEB_CHANSTATE;
	char * WEB_STATE;
	char * WEB_TXRX;
	char * WEB_PROTOSTATE;
	char * WEB_RESTARTTIME;
	char * WEB_RESTARTS;
	char * WEB_PACTORLEVEL;
	int WEB_CHANGED;				// Used to speed up refresh when active

	HMENU hMenu;
	HMENU hWndMenu;

	VOID (FAR * PORTTXROUTINE)();

	VOID (* SuspendPortProc) ();
	VOID (* ReleasePortProc) ();

	time_t WinmorRestartCodecTimer;
	int WinmorCurrentMode;
	char ARDOPCurrentMode[6];
	char ARDOPCommsMode;
	char * ARDOPSerialPort;			// Have Bus/Device for I2C
	int ARDOPSerialSpeed;
	int SlowTimer;
	int ARQPorts[32];				// For ARQ over KISS
	FILE * LogHandle;				// Ardop Logging File
	char * LogPath;

} *PTNCINFO;


struct HTTPConnectionInfo		// Used for Web Server for thread-specific stuff
{
	struct HTTPConnectionInfo * Next;
	struct STATIONRECORD * SelCall;	// Station Record for individual statond display
	char Callsign[12];
	int WindDirn, WindSpeed, WindGust, Temp, RainLastHour, RainLastDay, RainToday, Humidity, Pressure; //WX Fields
	char * ScreenLines[100];	// Screen Image for Teminal access mode - max 100 lines (cyclic buffer)
	int ScreenLineLen[100];		// Length of each lime
	int LastLine;				// Pointer to last line of data
	BOOL PartLine;				// Last line does not have CR on end
	char HTTPCall[10];			// Call of HTTP user
	BOOL Changed;				// Changed since last poll. If set, reply immediately, else set timer and wait
	SOCKET sock;				// Socket for pending send
	int ResponseTimer;			// Timer for delayed response
	int KillTimer;				// Clean up timer (no activity timeout)
	int Stream;					// BPQ Stream Number
	char Key[20];				// Session Key
	BOOL Connected;
	// Use by Mail Module
#ifdef MAIL
	struct UserInfo * User;		// Selected User
	struct MsgInfo * Msg;		// Selected Message
	WPRec * WP;					// Selected WP record
#else
	VOID * User;				// Selected User
	VOID * Msg;					// Selected Message
	VOID * WP;					// Selected WP record
#endif
	struct UserRec * USER;		// Telnet Server USER record
	int WebMailSkip;			// Number to skip at start of list (for paging)
	char WebMailTypes[4];		// Types To List
	BOOL WebMailMine;			// List all meessage to or from me
	time_t WebMailLastUsed;
};


