/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
    
Module Name:

    genport.h

Abstract:  Include file for Generic Port I/O Example Driver


Author:    Robert R. Howell         January 6, 1993


Environment:

    Kernel mode

Revision History:

 Eliyas Yakub     Dec 29, 1998

     Converted to PNP driver
     
--*/
#if     !defined(__GENPORT_H__)
#define __GENPORT_H__

#define DBG 1

#include <ntddk.h>
#include <wdmsec.h>


// {BE3FF345-8849-4691-A87C-C10BBFDA6BF7}
//DEFINE_GUID(<<name>>, 
//0xbe3ff345, 0x8849, 0x4691, 0xa8, 0x7c, 0xc1, 0xb, 0xbf, 0xda, 0x6b, 0xf7);

// {BE3FF345-8849-4691-A87C-C10BBFDA6BF7}
//static const GUID <<name>> = 
//{ 0xbe3ff345, 0x8849, 0x4691, { 0xa8, 0x7c, 0xc1, 0xb, 0xbf, 0xda, 0x6b, 0xf7 } };



#define DEVICE_OBJECT_NAME_LENGTH       128
#define SYMBOLIC_NAME_LENGTH            128

#define SERIAL_INSUFFICIENT_RESOURCES 1

//
// Device type           -- in the "User Defined" range."
//
#define GPD_TYPE 40000


typedef struct  _BPQHDLC_ADDCHANNEL_INPUT {

	ULONG   IOBASE;			// IO Base Address
    ULONG	IOLEN;			// Number of Addresses
    UCHAR   Interrupt;		// Interrupt
	UCHAR	Channel;

	PUCHAR ASIOC;		// A CHAN ADDRESSES
	PUCHAR SIO;			//  OUR ADDRESSES (COULD BE A OR B) 
	PUCHAR SIOC;		// 
	PUCHAR BSIOC;		//  B CHAN CONTROL

	UCHAR SOFTDCD;			//RX ACTIVE FLAG FOR 'SOFT DC

	int TXBRG;				//FOR CARDS WITHOUT /32 DIVIDER
	int RXBRG;

	UCHAR WR10;				//NRZ/NRZI FLAG

	USHORT TXDELAY;			//TX KEYUP DELAY TIMER
	UCHAR PERSISTANCE;

}   BPQHDLC_ADDCHANNEL_INPUT, *PBPQHDLC_ADDCHANNEL_INPUT;


#define FILE_DEVICE_BPQHDLC			0x00008421

#define IOCTL_BPQHDLC_SEND			CTL_CODE(FILE_DEVICE_BPQHDLC,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQHDLC_POLL			CTL_CODE(FILE_DEVICE_BPQHDLC,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQHDLC_TIMER			CTL_CODE(FILE_DEVICE_BPQHDLC,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQHDLC_ADDCHANNEL	CTL_CODE(FILE_DEVICE_BPQHDLC,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQHDLC_CHECKTX		CTL_CODE(FILE_DEVICE_BPQHDLC,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQHDLC_IOREAD		CTL_CODE(FILE_DEVICE_BPQHDLC,0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQHDLC_IOWRITE		CTL_CODE(FILE_DEVICE_BPQHDLC,0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)


// NT device name
#define GPD_DEVICE_NAME L"\\Device\\BPQHDLC"

// File system device name.   When you execute a CreateFile call to open the
// device, use "\\.\GpdDev", or, given C's conversion of \\ to \, use
// "\\\\.\\GpdDev"

#define DOS_DEVICE_NAME L"\\DosDevices\\BPQHDLC"

#define HDLC_TAG 'CLDH'

//
// These are the states FDO transition to upon
// receiving a specific PnP Irp. Refer to the PnP Device States
// diagram in DDK documentation for better understanding.
//

typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted                 // Device has received the REMOVE_DEVICE IRP

} DEVICE_PNP_STATE;

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DevicePnPState =  NotStarted;\
        (_Data_)->PreviousPnPState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PreviousPnPState =  (_Data_)->DevicePnPState;\
        (_Data_)->DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DevicePnPState =   (_Data_)->PreviousPnPState;\


// One Entry Per Interrupt Supported. Maintains a list of Ports using that Interrupt

typedef struct _HDLC_INTERRUPTS {

    PKINTERRUPT Interrupt;
    KIRQL Irql;    // Translated Irql
    ULONG OriginalVector;  // Untranslated vector
    ULONG OriginalIrql;    // Untranslated irql
    ULONG AddressSpace;    // Address space
    ULONG BusNumber;		// Bus number
    INTERFACE_TYPE InterfaceType;    // Interface type
	PUCHAR Mapped_SIOC;		// 
	PUCHAR Mapped_BSIOC;	//  B CHAN CONTROL
	UCHAR Level;
    ULONG Vector;			// Translated vector
	ULONG InterruptMode;
    ULONG Affinity;
} HDLC_INTERRUPTS,*PHDLC_INTERRUPTS;

// driver local data structure specific to each device object

typedef struct _HDLC_CHANNEL;

typedef struct _LOCAL_DEVICE_INFO {
    PDEVICE_OBJECT      DeviceObject;			// The Gpd device object.
    PDEVICE_OBJECT      NextLowerDriver;		// The top of the stack
    DEVICE_PNP_STATE    DevicePnPState;			// Track the state of the device
    DEVICE_PNP_STATE    PreviousPnPState;		// Remembers the previous pnp state
    BOOLEAN             PortWasMapped;			// If TRUE, we have to unmap on unload  
	IO_REMOVE_LOCK      RemoveLock;
    KSPIN_LOCK			QueueSpinLock;			// Spinlock to Sync access to Buffer Queues
	LIST_ENTRY			FREE_Q;					// Free bufferes pool header
	HDLC_INTERRUPTS		Interrupt_Control[16];	// Interrupt Control Structs
	struct _HDLC_CHANNEL * ChannelPointers[16];	// Channel Records

} LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

// Create Local DRIVER_INFO for Intellisense

typedef struct _MYDEVICE_OBJECT {
    CSHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    struct _DRIVER_OBJECT *DriverObject;
    struct _DEVICE_OBJECT *NextDevice;
    struct _DEVICE_OBJECT *AttachedDevice;
    struct _IRP *CurrentIrp;
    PIO_TIMER Timer;
    ULONG Flags;                                // See above:  DO_...
    ULONG Characteristics;                      // See ntioapi:  FILE_...
    PVPB Vpb;
    PLOCAL_DEVICE_INFO DeviceExtension;
    DEVICE_TYPE DeviceType;
    CCHAR StackSize;
    union {
        LIST_ENTRY ListEntry;
        WAIT_CONTEXT_BLOCK Wcb;
    } Queue;
    ULONG AlignmentRequirement;
    KDEVICE_QUEUE DeviceQueue;
    KDPC Dpc;

    //
    //  The following field is for exclusive use by the filesystem to keep
    //  track of the number of Fsp threads currently using the device
    //

    ULONG ActiveThreadCount;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    KEVENT DeviceLock;

    USHORT SectorSize;
    USHORT Spare1;

    struct _DEVOBJ_Channel  *DeviceObjectChannel;
    PVOID  Reserved;
} MYDEVICE_OBJECT;


#if DBG
#define DebugPrint(_x_) \
               DbgPrint _x_;

#else
#define DebugPrint(_x_)
 #endif


/********************* function prototypes ***********************************/
//

NTSTATUS    
DriverEntry(       
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath 
    );



NTSTATUS GpdDispatch(IN DEVICE_OBJECT * pDO, IN    PIRP pIrp);

NTSTATUS    
GpdIoctlReadPort(  
    IN  PLOCAL_DEVICE_INFO pLDI,
    IN  PIRP pIrp,
    IN  PIO_STACK_LOCATION IrpStack,
    IN  ULONG IoctlCode              
    );

NTSTATUS    
GpdIoctlWritePort( 
    IN  PLOCAL_DEVICE_INFO pLDI,
    IN  PIRP pIrp,
    IN  PIO_STACK_LOCATION IrpStack,
    IN  ULONG IoctlCode              
    );

VOID        
GpdUnload(         
    IN  PDRIVER_OBJECT DriverObject 
    );


NTSTATUS
GpdAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


NTSTATUS
GpdDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
GpdStartDevice (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GpdDispatchPower(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );
NTSTATUS 
GpdDispatchSystemControl(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );
PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);

typedef struct _HDLC_CHANNEL {

	int	ChannelPointer;	// Our Slot in ChannelPointers

	// HDLC Variables

	ULONG IOBASE;		// CONFIG PARAMS FOR HARDWARE DRIVERS 
	UCHAR INTLEVEL;		// FIRST 4 SAME FOR ALL H/W TYPES
	USHORT BAUDRATE;	// SPEED
	UCHAR CHANNELNUM;	// ON MULTICHANNEL H/W

	// Mapped versions of Addresses

	PUCHAR Mapped_ASIOC;		// A CHAN ADDRESSES
	PUCHAR Mapped_SIO;			//  OUR ADDRESSES (COULD BE A OR B) 
	PUCHAR Mapped_SIOC;		// 
	PUCHAR Mapped_BSIOC;		//  B CHAN CONTROL

	struct _HDLC_CHANNEL * A_PTR;		//  PORT ENTRY FOR A CHAN
	struct _HDLC_CHANNEL * B_PTR;		//  PORT ENTRY FOR B CHAN

	VOID (FAR * VECTOR[4]) (); // INTERRUPT VECTORS

	UCHAR LINKSTS;

	UCHAR * SDRNEXT;
	int SDRXCNT;
	VOID * CURALP;
	UCHAR OLOADS;			//LOCAL COUNT OF BUFFERS SHORTAGES
	USHORT FRAMELEN	;
	UCHAR * SDTNEXT;			//POINTER to NEXT BYTE to TRANSMIT
	int SDTXCNT;			//CHARS LEFT TO SEND
	UCHAR RR0;				//CURRENT RR0
	VOID * TXFRAME;			//ADDRESS OF FRAME BEING SENT

	UCHAR SDFLAGS;			//GENERAL FLAGS

	LIST_ENTRY TXMSG_Q;			//HDLC HOLDING QUEUE
	LIST_ENTRY RXMSG_Q;			//RX INTERRUPT TO SDLC BG

	UCHAR SOFTDCD;			//RX ACTIVE FLAG FOR 'SOFT DC
	USHORT TXDELAY;		//TX KEYUP DELAY TIMER
	UCHAR SLOTTIME;			//TIME TO WAIT IF WE DONT SEND
	USHORT L1TIMEOUT;		// UNABLE TO TX TIMEOUT
	UCHAR PORTSLOTIMER;	

	int TXBRG;				//FOR CARDS WITHOUT /32 DIVIDER
	int RXBRG;

	UCHAR WR10;				//NRZ/NRZI FLAG

	int L2ORUNC;			// OVERRUNS
	int L2URUNC;			// UNDERRUNS
	int	RXERRORS;

    // Polling timer object
    KTIMER  TXDelayTimer;

	KDPC TXDelayDpc;
    KDPC TXCompleteDpc;
    KDPC RXCompleteDpc;

	int TXComplete;

	MYDEVICE_OBJECT * Device;		// Pointer to Device Object (Needed for Freeing Buffers in Interrupt Routines)

	struct HDLC_CHANNEL * INTCHAIN;	// Next SIO on this level

} HDLC_CHANNEL, *PHDLC_CHANNEL;

// Buffer Format using List Entry 

#define BUFFLEN	360		//	; ??


typedef struct {
	
	LIST_ENTRY ListEntry;
	ULONG MsgLen;
	UCHAR Message[BUFFLEN];

} BUF_ENTRY, *PBUF_ENTRY;


#define IOTXCA VECTOR[0]
#define IOTXEA VECTOR[1]
#define IORXCA VECTOR[2]
#define IORXEA VECTOR[3]

#define SDTINP	2		// 1 = TRANSMISSION in PROGRESS
#define SDRINP	0X40	// 1 = RX IN PROGRESS
#define ABSENT	1		// 1 = ABORT SENT


#define SDTXUND	0xC0	// RESET TX UNDERRUN/EOM LATCH

#define SDABTX	0x18
#define SDUNDER	0x40
#define SDTXCRC	0x80		// RESET TX CRC GEN
#define SDABORT	0x80		// ABORT DETECTED
#define SDEXTR	0x10		// RESET EXT/STATUS INTS
#define SDRPEND	0x28		// RESET TX INT PENDING



#define SIOR READ_PORT_UCHAR(Channel->Mapped_SIO)
#define SIOW(A) WRITE_PORT_UCHAR(Channel->Mapped_SIO,A)

#define SIOCR READ_PORT_UCHAR(Channel->Mapped_SIOC)
#define SIOCW(A) WRITE_PORT_UCHAR(Channel->Mapped_SIOC,A)

#define SETRVEC	Channel->IORXCA =
#define SETTVEC	Channel->IOTXCA =

VOID SDIDRX(PHDLC_CHANNEL Channel);
VOID SDOVRX(PHDLC_CHANNEL Channel);
VOID IGNORE(PHDLC_CHANNEL Channel);
VOID SDCMTX(PHDLC_CHANNEL Channel);
VOID SDADRX(PHDLC_CHANNEL Channel);
VOID EXTINT(PHDLC_CHANNEL Channel);
VOID SPCLINT(PHDLC_CHANNEL Channel);


VOID SENDDUMMY1(PHDLC_CHANNEL Channel);
VOID SENDDUMMY2(PHDLC_CHANNEL Channel);
VOID SENDDUMMY3(PHDLC_CHANNEL Channel);
VOID SENDDUMMY4(PHDLC_CHANNEL Channel);
VOID DROPRTS(PHDLC_CHANNEL Channel);

PHDLC_CHANNEL InitChannel(PBPQHDLC_ADDCHANNEL_INPUT Params, PLOCAL_DEVICE_INFO deviceInfo);
VOID SendPacket(PHDLC_CHANNEL Channel, UCHAR * Msg, PLOCAL_DEVICE_INFO deviceInfo);
VOID ReleaseResources(PLOCAL_DEVICE_INFO deviceInfo);
int ReceivePacket(PHDLC_CHANNEL Channel, UCHAR * Msg, PLOCAL_DEVICE_INFO deviceInfo);

VOID StartTX(PLOCAL_DEVICE_INFO, PHDLC_CHANNEL Channel, PBUF_ENTRY Buffer);
VOID TXDelay(IN PKDPC Dpc, IN PVOID Context, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
VOID TXComplete(IN PKDPC Dpc, IN PVOID Context, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
VOID RXComplete(IN PKDPC Dpc, IN PVOID Context, IN PVOID SystemArgument1, IN PVOID SystemArgument2);


#endif

