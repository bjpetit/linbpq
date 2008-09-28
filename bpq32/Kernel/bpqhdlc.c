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

#include "AsmStrucs.h"

typedef struct _HDLCDATA
{
	struct PORTCONTROL PORTCONTROL	;	// REMAP HARDWARE INFO
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

}HDLCDATA, * PHDLCDATA;


#pragma pack()

int x=sizeof(HDLCDATA);
int y=sizeof(struct PORTCONTROL);

DWORD n;

HANDLE hDevice=0;
BYTE bOutput[4]=" ";
DWORD cb=0;
int fResult=0;

extern int QCOUNT;


int HDLCRX(PHDLCDATA PORTVEC, UCHAR * buff)
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

int HDLCTIMER(PHDLCDATA  PORTVEC)
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

int HDLCTX(PHDLCDATA  PORTVEC,UCHAR * buff)
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


int HDLCINIT(HDLCDATA * PORTVEC)
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

	memcpy(PORTVEC->DRIVERPORTTABLE,bOutput,4);

	return (TRUE);
		
}

int PC120INIT(PHDLCDATA PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int DRSIINIT(PHDLCDATA PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int TOSHINIT(PHDLCDATA PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int RLC100INIT(PHDLCDATA PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int BAYCOMINIT(PHDLCDATA PORTVEC)
{
	return (HDLCINIT(PORTVEC));
}

int PA0INIT(PHDLCDATA PORTVEC)		// 14 PA0HZP OPTO-SCC
{
	return (HDLCINIT(PORTVEC));
}
