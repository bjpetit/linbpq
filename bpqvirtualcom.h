//
// ComEmulDrv serial port bridge driver
//
// Copyright (c) 2002 
// MixW team
// http://www.mixw.net
//

#define COMBUFLEN 50000

#define MAXDEVICES 10

typedef struct _SERIAL_DEVICE_EXTENSION 
{
	BOOLEAN ControlDevice;	// True on Control Device
	BOOLEAN COMDevice;		// TRUE on COM device

	KSPIN_LOCK IoctlSpinLock;
	KSPIN_LOCK WriteSpinLock;
	KSPIN_LOCK ReadSpinLock;

	ULONG EventMask;
	ULONG HistoryEvents;

	ULONG BaudRate;
	ULONG RTSstate;
	ULONG DTRstate;
	ULONG CTSstate;
	ULONG DCDstate;
	ULONG DSRstate;

	SERIAL_TIMEOUTS Timeouts;
	SERIAL_LINE_CONTROL Lc;

	CHAR Buffer[COMBUFLEN];

	ULONG BufHead, BufTail;

	CHAR OutBuffer[COMBUFLEN];

	ULONG OutBufHead, OutBufTail;

    PIRP pWaitIrp;
    PIRP pReadIrp;

	BOOLEAN APIOpen;
	BOOLEAN COMOpen;
	//
    // This is the kernal timer structure used to handle
    // total read request timing.
    //
    KTIMER ReadRequestTotalTimer;

    //
    // This dpc is fired off if the timer for the total timeout
    // for the read expires.  It will execute a dpc routine that
    // will cause the current read to complete.
    //
    //
    KDPC TotalReadTimeoutDpc;

    SERIAL_HANDFLOW SerialHandflow;




} SERIAL_DEVICE_EXTENSION,*PSERIAL_DEVICE_EXTENSION;

typedef struct _API_DEVICE_EXTENSION 
{

	BOOLEAN ControlDevice;	// True on Control Device
	BOOLEAN COMDevice;		// FALSE on API

	KSPIN_LOCK IoctlSpinLock;
	KSPIN_LOCK WriteSpinLock;
	KSPIN_LOCK ReadSpinLock;

	ULONG EventMask;
	ULONG HistoryEvents;

	struct _SERIAL_DEVICE_EXTENSION* pCOMDevice;		// Used to link API>COM.

    PIRP pWaitIrp;
    PIRP pReadIrp;


} API_DEVICE_EXTENSION,*PAPI_DEVICE_EXTENSION;




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
#define	IOCTL_BPQ_LIST_DEVICES	 CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80b,METHOD_BUFFERED,FILE_ANY_ACCESS)
