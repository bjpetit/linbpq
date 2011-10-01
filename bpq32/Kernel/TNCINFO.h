//
// Common definitons for Pactor-like Modules

#include <winioctl.h>
#include "kernelresource.h"
#include "rigcontrol.h"

#define MAXBLOCK 4096

#define MAXFREQS 20			// RigControl freqs to scan

struct PacketReportInfo
{
	int mode;              // see below (an integer)
	int baud;              // see below (an integer)
	int power;             // actual power if known, default to 100 for HF, 30 for VHF/UHF (an integer)
	int height;            // antenna height in feet if known, default to 25
	int gain;              // antenna gain if known, default to 0
	int direction;         // primary antenna direction in degrees if known, use 000 for omni (an integer)
};

struct WL2KInfo
{
	char * Freq;
	char Bandwidth;
	char * TimeList;		// eg 06-10,12-15
	struct PacketReportInfo * PacketData;
};




// Telnet Server User Record

struct UserRec
{
	char * Callsign;
	char * UserName;
	char * Password;
	char * Appl;				// Autoconnect APPL
	BOOL Secure;				// Authorised User
};

#define MaxCMS	10				// Numbr of addresses we can keep - currently 4 are used.

struct TCPINFO
{
	int NumberofUsers;
	struct UserRec ** UserRecPtr;
	int CurrentConnections;

	struct UserRec RelayUser;

	int CurrentSockets;

	int TCPPort;
	int FBBPort[100];
	int RelayPort;
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

	SOCKET sock;
	SOCKET FBBsock[100];
	SOCKET Relaysock;
	SOCKET sock6;
	SOCKET FBBsock6[100];
	SOCKET Relaysock6;
	HMENU hActionMenu;
	HMENU hLogMenu;
	HMENU hDisMenu;					// Disconnect Menu Handle

};


struct STREAMINFO
{
//	struct TRANSPORTENTRY * AttachedSession;

	UINT PACTORtoBPQ_Q;			// Frames for BPQ
	UINT BPQtoPACTOR_Q;			// Frames for PACTOR
	int	FramesOutstanding;		// Frames Queued - used for flow control
	int	FramesQueued;			// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands

	BOOL Attached;				// Set what attached to a BPQ32 stream
	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;			// Set when Outward Connect in progress
	BOOL Disconnecting;			// Set when disconnect in progress
								// Used when appplication disconnects the bpq session, and
								// prevents new attaches while a didy disconnect is in progress
	int DisconnectingTimeout;	// A hard disconnect occurs if this expires before the disconnect complete
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel
	BOOL DiscWhenAllSent;		// Close session when all msgs have been sent to node
	BOOL ARQENDSent;			// Set when V4 ARQEND Sent

	int DEDStream;				// Stream number for DED interface (same as index except for pactor)

	char MyCall[10]	;			// Call we are using
	char RemoteCall[10];		// Callsign

	int BytesTXed;
	int BytesAcked;
	int BytesRXed;
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
};



typedef struct TNCINFO
{ 
	HWND hDlg;						// Status Window Handle
	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port
	struct RIGINFO * RIG;			// Pointer to Rig Control RIG record 
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

	int Port;					// BPQ Port Number

	struct RIGINFO DummyRig;	// Used if not using Rigcontrol

	BOOL Minimized;				// Start Minimized flag

	int WINMORtoBPQ_Q;			// Frames for BPQ, indexed by BPQ Port
	int BPQtoWINMOR_Q;			// Frames for WINMOR. indexed by WINMOR port. Only used it TCP session is blocked

	SOCKET WINMORSock;			// Control Socket
	SOCKET WINMORDataSock;		// Data Socket

	char * WINMORSignon;		// Pointer to message for secure signin
	char * WINMORHostName;		// WINMOR Host - may be dotted decimal or DNS Name
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	BOOL SwallowSignon;			// Set to suppress *** connected to APPL

    union
	{
		UCHAR TCPBuffer[1000];		// For converting byte stream to messages
		UCHAR DEDBuffer[1000];		// For converting byte stream to messages
	};
	int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	int	MSGCOUNT;				// DED WORKING FIELD
	int	MSGLENGTH;				// DED Msg Len
	int	MSGCHANNEL;				// DED Msg Channel Number
	int	MSGTYPE;				// DED Msg Type

	int HOSTSTATE;				// ded HOST state machine


	BOOL StartSent;				// Codec Start send (so will get a disconnect)
	BOOL ConnectPending;		// Set if Connect Pending Received. If so, mustn't allow freq change.
	BOOL DiscPending;			// Set if Disconnect Pending Received. So we can time out stuck in Disconnecting
	BOOL HadConnect;				// Flag to say have been in session
	BOOL FECMode;				// In FEC Mode
	BOOL FEC1600;				// Use 1600 Hz FEC Mode
	int FECIDTimer;				// Time in FEC Mode. Used to trigger ID broadcasts
	BOOL RestartAfterFailure;

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

	SOCKADDR_IN destaddr;
	SOCKADDR_IN Datadestaddr;

	char * RigConfigMsg;			// Message to pass to rig control

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
	HMENU hPopMenu;					// Actions Menu Handle

	int BusyHold;					// Hold Time from SCS reporting channel free till we call 
	int BusyWait;					// Time to wait for clear channel before connect

	BOOL OverrideBusy;
	int BusyDelay;					// Timer for busy timeout
	char * ConnectCmd;				// Saved command if waiting for busy to clear
	BOOL UseAPPLCalls;				// Robust Packet to use Applcalls

	// Fields for reporting to WL2K Map

	char * Host;
	short WL2KPort;

	int UpdateWL2KTimer;
	BOOL UpdateWL2K;
	char RMSCall[10];
	char BaseCall[10];
	char GridSquare[7];
	char Comment[80];
	BOOL UseRigCtrlFreqs;
	char WL2KFreq[12];
	char WL2KMode;				// WL2K reporting mode
	char WL2KModeChar;			// W or N
	BOOL DontReportNarrowOnWideFreqs;

	char NARROWMODE;
	char WIDEMODE;				// Mode numbers to report to WL2K

	struct WL2KInfo WL2KInfoList[MAXFREQS];		// Freqs for sending to WL2K

	struct STREAMINFO Streams[27];	// 0 is Pactor 1 - 10 are ax.25.
	int LastStream;				// Last one polled for status or send

	int BPQtoRadio_Q;			// Frames for Rig Interface

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
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR RXBuffer[520];			// Message being received - may not arrive all at once
	UINT RXLen;						// Data in RXBUffer
	UCHAR Toggle;					// Sequence bit
	int Buffers;					// Free buffers in TNC
	BOOL WantToChangeFreq;			// Request from Scanner to Change
	int OKToChangeFreq;				// 1 = SCS Says OK to change, -1 = Dont Change zero = still waiting
	BOOL DontWantToChangeFreq;		// Change done - ok to relaase SCS


	int VCOMPort;					// COMM Port for Rig Control Channel
	HANDLE VCOMHandle;

	UCHAR NexttoPoll[20];			// Streams with data outstanding (from General Poll)
	BOOL PollSent;					// Toggle to ensure we issue a general poll regularly
	int StreamtoPoll;

	char Bandwidth;					// Currently set Mode W or N

	int Mode;						// Mode Flag

	int PacketChannels;
	int RobustTime;					// For PTC, Spend this part of scan cycle (in 10th secs) in Robust Packet Mode 
	int SwitchToPactor;				// Countdown to switch

	BOOL OldMode;					// Use PACTOR instead of TOR (for old software)

	int Mem1;						// Free Bytes (VHF /HF)
	int Mem2;

	BOOL HFPacket;					// Set if HF port is in Packet mode instead of Pactor Mode
	BOOL Robust;					// Set if SCS Tracker is in Robust Packet mode
	BOOL RobustDefault;				// Set if SCS Tracker default is Robust Packet mode

	int TimeInRX;					// Time waiting for ISS before sending
	char TXRXState;					// Current ISS/IRS State

	BOOL NeedPACTOR;				// Set if need to send PACTOR to put into Standby Mode
	int CmdStream;					// Stream last command was issued on

	struct TCPINFO * TCPInfo;		// Telnet Server Specific Data

	BOOL DataBusy;					// Waiting for Data Ack - Don't send any more data
	BOOL CommandBusy;				// Waiting for Command ACK

	BOOL TEXTMODE;					// Set if AEA in text mode
	BOOL NeedTurnRound;				// Set if we have sent data, so need to send ctrl/z 
	BOOL NeedTRANS;					// Set if we have to send TRANS when ctrl/z is acked. 

	char * CmdSet;					// A series of commands to send to the TNC
	char * CmdSave;					// Base address for free

	int DefaultMode;
	int CurrentMode;

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

	int DataMode;					// How to treat data 

#define RXDATA  0x30				// Switch to Receive Data characters
#define TXDATA  0x31				// Switch to Transmit Data characters
#define SECDATA 0x32				// Switch to RX data from secondary port

	int TXMode;					// Where to send data 

#define TXMODEM 0x33				// Send TX data to modem
#define TXSEC   0x34				// Send TX data to secondary port

	BOOL XONXOFF;					// Set if hardware is using XON/XOFF
};

VOID * zalloc(int len);

BOOL ReadConfigFile(char * filename, int Port, int ProcLine());
GetLine(char * buf);
BOOL CreatePactorWindow(struct TNCINFO * TNC, char * ClassName, char * WindowTitle, int RigControlRow, WNDPROC WndProc, LPCSTR MENU);
BOOL CheckAppl(struct TNCINFO * TNC, char * Appl);
BOOL SendReporttoWL2K(struct TNCINFO * TNC);
DecodeWL2KReportLine(struct TNCINFO * TNC,char *  buf, char NARROWMODE, char WIDEMODE);
VOID UpdateMH(struct TNCINFO * TNC, UCHAR * Call, char Mode, char Direction);
VOID SaveWindowPos(int port);
BOOL ProcessIncommingConnect(struct TNCINFO * TNC, char * Call, int Stream);
VOID ShowTraffic(struct TNCINFO * TNC);
OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed, BOOL Quiet);
VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);
VOID SendMH(int Hardware, char * call, char * freq, char * LOC, char * Mode);

static VOID TidyClose(struct TNCINFO * TNC, int Stream);
static VOID ForcedClose(struct TNCINFO * TNC, int Stream);
static VOID CloseComplete(struct TNCINFO * TNC, int Stream);

VOID CheckForDetach(struct TNCINFO * TNC, int Stream, struct STREAMINFO * STREAM,
				VOID TidyClose(), VOID ForcedClose(), VOID CloseComplete());

BOOL InterlockedCheckBusy(struct TNCINFO * ThisTNC);

extern UCHAR NEXTID;
extern struct TRANSPORTENTRY * L4TABLE;
extern WORD MAXCIRCUITS;
extern UCHAR L4DEFAULTWINDOW;
extern WORD L4T1;
extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;

extern UINT CRCTAB;
extern char * CTEXTMSG;
extern USHORT CTEXTLEN;
extern UINT FULL_CTEXT;
extern BPQTRACE();


extern struct BPQVECSTRUC * BPQHOSTVECPTR;

static int ProcessLine(char * buf, int Port);
VOID __cdecl Debugprintf(const char * format, ...);

extern BOOL MinimizetoTray;

BOOL WINAPI Rig_Command();
struct RIGINFO * WINAPI RigConfig();
BOOL WINAPI Rig_Poll();
VOID WINAPI Rig_PTT();
struct RIGINFO * WINAPI Rig_GETPTTREC();
struct ScanEntry ** WINAPI CheckTimeBands();

LRESULT CALLBACK PacWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#define Report_P1 11
#define Report_P12 12 
#define Report_P123 13
#define Report_P2 14
#define Report_P23 15
#define Report_P3 16

#define Report_WINMOR500 21
#define Report_WINMOR1600 22  

#define IOCTL_SERIAL_IS_COM_OPEN CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_CLR_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x808,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_BPQ_ADD_DEVICE     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x809,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQ_DELETE_DEVICE  CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80a,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define W98_SERIAL_GETDATA     0x801
#define W98_SERIAL_SETDATA     0x802
