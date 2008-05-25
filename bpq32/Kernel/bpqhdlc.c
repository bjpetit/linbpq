//
//	Module to provide HDLC Card (DRSI, Baycom etc) support for
//	G8BPQ switch in a 32bit environment

//
//	Win95 - Uses BPQHDLC.VXD to drive card
//  NT - Not yet supported
//

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "bpq32.h"
#pragma pack(1) 

struct PORTCONTROL
{
	char PORTCALL[7];
	char PORTALIAS[7];		//USED FOR UPLINKS ONLY
	char PORTNUMBER;
	struct PORTCONTROL * PORTPOINTER; // NEXT IN CHAIN

	UCHAR PORTQUALITY;		// 'STANDARD' QUALITY FOR THIS PORT
	char * PORTRX_Q;		// FRAMES RECEIVED ON THIS PORT
	char * PORTTX_Q;		// FRAMES TO BE SENT ON THIS PORT

	UINT * PORTTXROUTINE;	// POINTER TO TRANSMIT ROUTINE FOR THIS PORT
	UINT * PORTRXROUTINE;	// POINTER TO RECEIVE ROUTINE FOR THIS PORT
	UINT * PORTINITCODE;		// INITIALISATION ROUTINE
	UINT * PORTTIMERCODE;	//	
	char PORTDESCRIPTION[30];// TEXT DESCRIPTION OF FREQ/SPEED ETC

	char PORTBBSFLAG;		// NZ MEANS PORT CALL/ALIAS ARE FOR BBS
	char PORTL3FLAG;			// NZ RESTRICTS OUTGOING L2 CONNECTS
//
//	CWID FIELDS
//

	USHORT CWID[9];			// 8 ELEMENTS + FLAG
	USHORT ELEMENT;			// REMAINING BITS OF CURRENT CHAR
	char * CWPOINTER;		// POINTER TO NEXT CHAR
	USHORT CWIDTIMER;		// TIME TO NEXT ID
	char CWSTATE;			// STATE MACHINE FOR CWID
	char CWTYPE;				// SET TO USE ON/OFF KEYING INSTEAD OF
							// FSK (FOR RUH MODEMS)
	UCHAR PORTMINQUAL;		// MIN QUAL TO BRAOCAST ON THIS PORT

//	STATS COUNTERS

	int L2DIGIED;
	int L2FRAMES;
	int L2FRAMESFORUS;
	int L2FRAMESSENT;
	int L2TIMEOUTS;
	int L2ORUNC;			// OVERRUNS
	int L2URUNC;			// UNDERRUNS
	int L1DISCARD;			// FRAMES DISCARDED (UNABLE TO TX DUE TO DCD)
	int L2FRMRRX;
	int L2FRMRTX;
	int RXERRORS;			// RECEIVE ERRORS
	int L2REJCOUNT;			// REJ FRAMES RECEIVED
	int L2OUTOFSEQ;			// FRAMES RECEIVED OUT OF SEQUENCE
	int L2RESEQ;			// FRAMES RESEQUENCED

	USHORT SENDING;			// LINK STATUS BITS
	USHORT ACTIVE;

	UCHAR AVSENDING;			// LAST MINUTE
	UCHAR AVACTIVE;

	char PORTTYPE;	// H/W TYPE
					// 0 = ASYNC, 2 = PC120, 4 = DRSI
					// 6 = TOSH, 8 = QUAD, 10 = RLC100
					// 12 = RLC400 14 = INTERNAL 16 = EXTERNAL

	char PROTOCOL;	// PORT PROTOCOL
					// 0 = KISS, 2 = NETROM, 4 = BPQKISS
					//; 6 = HDLC, 8 = L2

	USHORT IOBASE;		// CONFIG PARAMS FOR HARDWARE DRIVERS 

	char INTLEVEL;		// NEXT 4 SAME FOR ALL H/W TYPES
	USHORT BAUDRATE;	// SPEED
	char CHANNELNUM;	// ON MULTICHANNEL H/W
	struct PORTCONTROL * INTCHAIN; // POINTER TO NEXT PORT USING THIS LEVEL
	UCHAR PORTWINDOW;	// L2 WINDOW FOR THIS PORT
	USHORT PORTTXDELAY;	// TX DELAY FOR THIS PORT
	UCHAR PORTPERSISTANCE;	// PERSISTANCE VALUE FOR THIS PORT
	UCHAR FULLDUPLEX;	// FULL DUPLEX IF SET
	UCHAR SOFTDCDFLAG;	// IF SET USE 'SOFT DCD' - IF MODEM CANT GIVE A REAL ONE
	UCHAR PORTSLOTTIME;	// SLOT TIME
	UCHAR PORTTAILTIME;	// TAIL TIME
	UCHAR BBSBANNED;	// SET IF PORT CAN'T ACCEPT L2 CALLS TO BBS CALLSIGN 
	UCHAR PORTT1;		// L2 TIMEOUT
	UCHAR PORTT2;		// L2 DELAYED ACK TIMER
	UCHAR PORTN2;		// RETRIES
	UCHAR PORTPACLEN;	// DEFAULT PACLEN FOR INCOMING SESSIONS

	UINT * PORTINTERRUPT; // ADDRESS OF INTERRUPT HANDLER

	UCHAR QUAL_ADJUST;	// % REDUCTION IN QUALITY IF ON SAME PORT

	char * PERMITTEDCALLS;	// POINTER TO PERMITED CALLS LIST
	char * PORTUNPROTO;		// POINTER TO UI DEST AND DIGI LIST
	UCHAR PORTDISABLED;		// PORT TX DISABLE FLAG
	UCHAR DIGIFLAG;			// ENABLE/DISABLE/UI ONLY
	UCHAR DIGIPORT;			// CROSSBAND DIGI PORT
	UCHAR USERS;			// MAX USERS ON PORT
	UCHAR KISSFLAGS;		// KISS SPECIAL MODE BITS
	UCHAR PORTINTERLOCK;	// TO DEFINE PORTS WHICH CANT TX AT SAME TIME
	UCHAR NODESPACLEN;		// MAX LENGTH OF 'NODES' MSG 
	UCHAR TXPORT;			// PORT FOR SHARED TX OPERATION
	char * PORTMHEARD;		// POINTER TO MH DATA

	USHORT PARAMTIMER;		// MOVED FROM HW DATA FOR SYSOPH
	UCHAR PORTMAXDIGIS;		// DIGIS ALLOWED ON THIS PORT
	char PORTALIAS2[7];		// 2ND ALIAS FOR DIGIPEATING FOR APRS
//
//	Mapping of VXD fields (mainly to simplify debugging
//


	USHORT ASIOC;			// A CHAN ADDRESSES
	USHORT SIO;				// OUR ADDRESSES (COULD BE A OR B) 
	USHORT SIOC;
	USHORT BSIOC;			// B CHAN CONTROL

	struct PORTCONTROL * A_PTR; // PORT ENTRY FOR A CHAN
	struct PORTCONTROL * B_PTR; // PORT ENTRY FOR B CHAN

	UINT * IOTXCA;				// INTERRUPT VECTORS
	UINT * IOTXEA;
	UINT * IORXCA;
	UINT * IORXEA;	

	UCHAR LINKSTS;

	UINT * SDRNEXT;
	UINT * SDRXCNT;
	UINT * CURALP;
	UCHAR OLOADS;				// LOCAL COUNT OF BUFFERS SHORTAGES
	USHORT FRAMELEN;
	UINT * SDTNEXT;				// POINTER to NEXT BYTE to TRANSMIT
	USHORT SDTXCNT;				// CHARS LEFT TO SEND
	UCHAR RR0;					// CURRENT RR0
	UINT * TXFRAME;				// ADDRESS OF FRAME BEING SENT

	UCHAR SDFLAGS;				// GENERAL FLAGS

	UINT * PCTX_Q;				// HDLC HOLDING QUEUE
	UINT * RXMSG_Q;				// RX INTERRUPT TO SDLC BG


//;SOFTDCD		DB	0		; RX ACTIVE FLAG FOR 'SOFT DC
	UCHAR TXDELAY;				// TX KEYUP DELAY TIMER
	UCHAR SLOTTIME;				// TIME TO WAIT IF WE DONT SEND
	UCHAR FIRSTCHAR;			// CHAR TO SEND FOLLOWING TXDELAY
	USHORT L1TIMEOUT;			// UNABLE TO TX TIMEOUT
	UCHAR PORTSLOTIMER;

	USHORT TXBRG;				// FOR CARDS WITHOUT /32 DIVIDER
	USHORT RXBRG;	

	UCHAR WR10	;				// NRZ/NRZI FLAG

	int	IRQHand;


	struct PORTCONTROL * DRIVERPORTTABLE;	// ADDR OF PORT TABLE ENTRY IN VXD

}	portcontrol;


#pragma pack()

DWORD n;

HANDLE hDevice=0;
BYTE bOutput[4]=" ";
DWORD cb=0;
int fResult=0;

extern int QCOUNT;


int HDLCRX(struct PORTCONTROL * PORTVEC, UCHAR * buff)
{
	DWORD len=0;

	if (hDevice == 0)
		return (0);

	fResult = DeviceIoControl(
			hDevice,   // device handle
			'G',		   // control code
			PORTVEC->DRIVERPORTTABLE,rand() & 0xff, //Input Params
	        buff,360,&len, // output parameters
			0);

	return (len);
}

int HDLCTIMER(struct PORTCONTROL * PORTVEC)
{
	DWORD len=0;

	if (hDevice == 0)
		return (0);

	fResult = DeviceIoControl(
			hDevice,   // device handle
			'T',		   // control code
			PORTVEC->DRIVERPORTTABLE,4, //Input Params
	        0,0,&len, // output parameters
			0);

	return (0);
}

int HDLCTX(struct PORTCONTROL * PORTVEC,UCHAR * buff)
{
	DWORD txlen=0;

	if (hDevice == 0)
		return (0);

	txlen=(buff[6]<<8) + buff[5];
	
	memcpy(buff,&PORTVEC->DRIVERPORTTABLE,4);
	
	fResult = DeviceIoControl(
			hDevice,   // device handle
			'S',		   // control code
			buff,txlen,// input parameters
	        NULL,0,&cb, // output parameters
			0);

	return (0);
}


int HDLCINIT(struct PORTCONTROL * PORTVEC)
{
	char msg[255];
	int err;

	//
	//	Open HDLC Driver, send send config params
	//

	if (hDevice == 0)		// Not already loaded
	{
		//
		//	Load VXD
		//

		hDevice = CreateFile("\\\\.\\BPQHDLC.VXD",
			0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);

		if (hDevice == INVALID_HANDLE_VALUE)
		{
			hDevice=0;

			err=GetLastError();
	
			wsprintf(msg,"Error loading Driver \\\\.\\BPQHDLC.VXD - Error code %d",err);
			MessageBox(NULL,msg,"BPQ32",MB_ICONSTOP);

			WritetoConsole("Initialisation Failed");


			return (FALSE);
		}

		fResult = DeviceIoControl(
			hDevice,          // device handle
			10,//DIOC_GETVERSION,  // control code
			NULL,0,// input parameters
			bOutput, 4, &cb,  // output parameters
			0);

		srand( (unsigned)time( NULL ) );  //Prime random no generator

 		
	}

	//
	//	Initailize Driver for this card and channel
	//

	fResult = DeviceIoControl(
		hDevice,   // device handle
		'I',		   // control code
		PORTVEC,sizeof portcontrol,// input parameters
		bOutput, 4, &cb,  // output parameters
        0);

	memcpy(&PORTVEC->DRIVERPORTTABLE,bOutput,4);

	return (TRUE);
		
}

int PC120INIT(struct PORTCONTROL * PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int DRSIINIT(struct PORTCONTROL * PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int TOSHINIT(struct PORTCONTROL * PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int RLC100INIT(struct PORTCONTROL * PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int BAYCOMINIT(struct PORTCONTROL * PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int PA0INIT(struct PORTCONTROL * PORTVEC)		// 14 PA0HZP OPTO-SCC
{
	return (HDLCINIT(PORTVEC));
}
