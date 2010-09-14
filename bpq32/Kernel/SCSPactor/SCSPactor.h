
#include <winioctl.h>
#include "resource.h"
#include "resrc1.h"


#define MAXBLOCK 4096

#define MaxStreams 10			// First is used for Pactor, even though Pactor uses channel 31
#define PactorStream 31


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

struct WL2KInfo
{
	char * Freq;
	char Bandwidth;
	char * TimeList;		// eg 06-10,12-15
};


#define MAXFREQS 20

struct TNCINFO
{ 
	struct STREAMINFO Streams[MaxStreams+1];	// 0 is Pactor 1 - 10 are ax.25.
	int LastStream;				// Last one polled for status or send

	int BPQtoRadio_Q;			// Frames for Rig Interface

	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitScript;			// Initialisation Commands
	char * InitPtr;				// Next Command
	int	ReinitState;			// Reinit State Machine
	BOOL TNCOK;					// TNC is reponding
	int	FramesOutstanding;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands

	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port

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
	char NodeCall[10];				// Call we listen for (PORTCALL or NODECALL
	char MyCall[10]	;				// Call we are using
	char RemoteCall[10];			// Callsign
	int Buffers;					// Free buffers in TNC
	BOOL WantToChangeFreq;			// Request from Scanner to Change
	int OKToChangeFreq;				// 1 = SCS Says OK to change, -1 = Dont Change zero = still waiting
	BOOL DontWantToChangeFreq;		// Change done - ok to relaase SCS

	HWND hDlg;						// Status Window Handle

	int VCOMPort;					// COMM Port for Rig COntrol Channel
	HANDLE VCOMHandle;

	UCHAR NexttoPoll[20];			// Streams with data outstanding (from General Poll)
	BOOL PollSent;					// Toggle to ensure we issue a general poll regularly

	struct RIGINFO * RIG;			// Pointer to Rig Control RIG record 

	char Bandwidth;					// Currently set Mode W or N

	BOOL OverrideBusy;
	int BusyDelay;					// Timer for busy timeout
	char * ConnectCmd;				// Saved command if waiting for busy to clear
	int Mode;						// Mode Flag
	int Busy;						// Busy Flag

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

	int PacketChannels;

};


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
