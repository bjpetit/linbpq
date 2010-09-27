
#include <winioctl.h>
#include "resource.h"

#define MAXBLOCK 4096

struct STREAMINFO
{
//	struct TRANSPORTENTRY * AttachedSession;

	int PACTORtoBPQ_Q;			// Frames for BPQ
	int BPQtoPACTOR_Q;			// Frames for PACTOR
	int	FramesQueued;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands

	BOOL Attached;					// Set what attached to a BPQ32 stream
	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;			// Set when Outward Connect in progress
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	int Timeout;				// Timeout response counter

	char MyCall[10]	;				// Call we are using
	char RemoteCall[10];			// Callsign

	int BytesTXed;
	int BytesAcked;
	int BytesRXed;
	int BytesOutstanding;		// For Packet Channels

	int DefaultMode;
	int CurrentMode;

	// Mode Equates

	#define Clover 1
	#define Pactor 2
	#define AMTOR 3

};



struct TNCINFO
{
	struct STREAMINFO Streams[1];	// Only Supports one stream
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitScript;			// Initialisation Commands
	int InitScriptLen;			// Length
	BOOL TNCOK;					// TNC is reponding
	BOOL InternalCmd;			// Last Command was generated internally

	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port

	char NodeCall[10];				// Call we listen for (PORTCALL or NODECALL

	HANDLE hDevice;
	BOOL HostMode;					// Set if TNC initialised
	BOOL NeedPACTOR;				// Set if need to send PACTOR to put into Standby Mode
	int Timeout;					// Timeout response counter
//	int Retries;
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR DataBuffer[500];			// Data Chars split from  received stream
	UCHAR CmdBuffer[500];			// Cmd/Response chars split from received stream
	int DataLen;					// Data in DataBuffer
	int CmdLen;						// Data in CmdBuffer
	BOOL CmdEsc;						// Set if last char rxed was 0x80
	BOOL DataEsc;					// Set if last char rxed was 0x81

	char TXRXState;					// Current State
	int Mem2;

	HWND hDlg;						// Status Window Handle

	int PollDelay;					// Don't poll too often;

	// HAL only supports one session, so don't need separate stream info

	int PACTORtoBPQ_Q;			// Frames for BPQ
	int BPQtoPACTOR_Q;			// Frames for PACTOR
	int	FramesQueued;		// Frames Queued - used for flow control
	int	IntCmdDelay;			// To limit internal commands

	BOOL Attached;					// Set what attached to a BPQ32 stream
	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;			// Set when Outward Connect in progress
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	char MyCall[10]	;				// Call we are using
	char RemoteCall[10];			// Callsign

	int BytesTXed;
	int BytesAcked;
	int BytesRXed;
	int BytesOutstanding;		// For Packet Channels

	int DefaultMode;
	int CurrentMode;

	// Mode Equates

	#define Clover 1
	#define Pactor 2
	#define AMTOR 3

	struct RIGINFO * RIG;			// Pointer to Rig Control RIG record 
	char * RigConfigMsg;			// Message to pass to rig control

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
