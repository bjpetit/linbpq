
#define MAXFREQS 20

struct WL2KInfo
{
	char * Freq;
	char Bandwidth;
	char * TimeList;		// eg 06-10,12-15
};


struct STREAMINFO
{
//	struct TRANSPORTENTRY * AttachedSession;

	int PACTORtoBPQ_Q;			// Frames for BPQ
	int BPQtoPACTOR_Q;			// Frames for PACTOR
	int	FramesOutstanding;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands

	BOOL Attached;				// Set what attached to a BPQ32 stream
	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;			// Set when Outward Connect in progress
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

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

	char * CmdSet;			// A series of commands to send to the TNC
	char * CmdSave;				// Base address for free

};



typedef struct TNCINFO
{ 
	HWND hDlg;						// Status Window Handle
	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port
	struct RIGINFO * RIG;			// Pointer to Rig Control RIG record 
	char * InitScript;			// Initialisation Commands

	int Hardware;				// Hardware Type

#define H_WINMOR 1
#define H_SCS 2
#define H_KAM 3
#define H_AEA 4
#define H_HAL 5

	int WINMORtoBPQ_Q;			// Frames for BPQ, indexed by BPQ Port
	int BPQtoWINMOR_Q;			// Frames for WINMOR. indexed by WINMOR port. Only used it TCP session is blocked

	SOCKET WINMORSock;			// Control Socket
	SOCKET WINMORDataSock;		// Data Socket

	char * WINMORSignon;		// Pointer to message for secure signin
	char * WINMORHostName;		// WINMOR Host - may be dotted decimal or DNS Name
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)

    UCHAR TCPBuffer[1000];		// For converting byte stream to messages
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	BOOL Connected;				// When set, all data is passed to Data Socket
	BOOL Connecting;
	BOOL Disconnecting;			// Disconnect Sent - waiting for DISCONNECTED or Timout
	int	DiscTimeout;			// Disconnect Timeout counter.
	BOOL Attached;				// Set what attached to a BPQ32 stream
	BOOL StartSent;				// Codec Start send (so will get a disconnect)
	BOOL ConnectPending;		// Set if Connect Pending Received. If so, mustn't allow freq change.
	BOOL DiscPending;			// Set if Disconnect Pending Received. So we can time out stuck in Disconnecting
	int NeedDisc;				// Timer to send DISC if appl not available
	BOOL HadConnect;				// Flag to say have been in session
	BOOL FECMode;				// In FEC Mode
	BOOL FEC1600;				// Use 1600 Hz FEC Mode
	int FECIDTimer;				// Time in FEC Mode. Used to trigger ID broadcasts

	int Busy;					// Channel Busy Flags

#define CDBusy 1
#define PTTBusy 2

	BOOL FECPending;			// Need an FEC Send when channel is next idle

	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	time_t lasttime;

	BOOL CONNECTING;			// TCP Session Flags
	BOOL CONNECTED;
	BOOL Alerted;				// COnnect Failed Prompt sent
	BOOL DATACONNECTING;
	BOOL DATACONNECTED;

	char MyCall[10]	;				// Call we are using
	char RemoteCall[10];			// Callsign
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

	BOOL OverrideBusy;
	int BusyDelay;					// Timer for busy timeout
	char * ConnectCmd;				// Saved command if waiting for busy to clear

	// Fields for reporting to WL2K Map

	char * Host;
	short Port;

	int UpdateWL2KTimer;
	BOOL UpdateWL2K;
	char RMSCall[10];
	char BaseCall[10];
	char GridSquare[7];
	char Comment[80];
	BOOL UseRigCtrlFreqs;
	char WL2KFreq[12];
	char WL2KMode;

	struct WL2KInfo WL2KInfoList[MAXFREQS];		// Freqs for sending to WL2K

	struct STREAMINFO Streams[27];	// 0 is Pactor 1 - 10 are ax.25.
	int LastStream;				// Last one polled for status or send

	int BPQtoRadio_Q;			// Frames for Rig Interface

	char * InitPtr;				// Next Command
	int	ReinitState;			// Reinit State Machine
	BOOL TNCOK;					// TNC is reponding
	int	FramesOutstanding;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands


	HANDLE hDevice;
	BOOL HostMode;					// Set if in DED Host Mode
//	BOOL CRCMode;					// Set if using SCS Extended DED Mode (JHOST4)
	int Timeout;					// Timeout response counter
	int Retries;
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR RXBuffer[500];			// Message being received - may not arrive all at once
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

	char Bandwidth;					// Currently set Mode W or N

	int Mode;						// Mode Flag

	int PacketChannels;

	BOOL OldMode;					// Use PACTOR instead of TOR (for old software)

	int Mem1;						// Free Bytes (VHF /HF)
	int Mem2;

	BOOL HFPacket;					// Set if HF port is in Packet mode instead of Pactor Mode

	int TimeInRX;					// Time waiting for ISS before sending
	char TXRXState;					// Current ISS/IRS State

	BOOL NeedPACTOR;				// Set if need to send PACTOR to put into Standby Mode
	int CmdStream;					// Stream last command was issued on

};

BOOL ReadConfigFile(char * filename, int Port, int ProcLine());
GetLine(char * buf);
BOOL CreatePactorWindow(struct TNCINFO * TNC, char * ClassName, char * WindowTitle, int RigControlRow);