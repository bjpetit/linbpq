
#include <winioctl.h>
#include "resrc1.h"

#define MAXBLOCK 4096

struct TNCINFO
{ 
	int PACTORtoBPQ_Q;			// Frames for BPQ
	int BPQtoPACTOR_Q;			// Frames for PACTOR
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitScript;			// Initialisation Commands
	char * InitPtr;				// Next Command
	int	ReinitState;			// Reinit State Machine
	BOOL TNCOK;					// TNC is reponding
	int	FramesOutstanding;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands

	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port

	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;				// Set when Outward Connect in progress
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	HANDLE hDevice;
	BOOL HostMode;					// Set if in DED Host Mode
	BOOL NeedPACTOR;				// Set if need to send PACTOR to put into Standby Mode
	int Timeout;					// Timeout response counter
	int Retries;
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR RXBuffer[500];			// Message being received - may not arrive all at once
	int RXLen;						// Data in RXBUffer
	char RemoteCall[10];			// Callsign
	int BytesTXed;
	int BytesAcked;
	int BytesRXed;

	HWND hDlg;						// Status Window Handle
};

struct TNCINFO  TNCInfo[16]={0};

struct STREAMINFO
{
	struct TRANSPORTENTRY * ATTACHEDSESSION;

	int PACTORtoBPQ_Q;			// Frames for BPQ
	int BPQtoPACTOR_Q;			// Frames for PACTOR
	int	FramesOutstanding;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands

	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;			// Set when Outward Connect in progress
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	int Timeout;				// Timeout response counter

	char RemoteCall[10];		// Callsign
	int BytesTXed;
	int BytesAcked;
	int BytesRXed;

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
