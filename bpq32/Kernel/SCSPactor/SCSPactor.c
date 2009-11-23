//
//	DLL to inteface SCS TNC in Pactor Mode to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include <stdlib.h>

//#include <process.h>
//#include <time.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#include "SCSPactor.h"
#include "ASMStrucs.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"
 
#define DllImport	__declspec(dllimport)
#define DllExport	__declspec(dllexport)

DllImport UINT CRCTAB;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

BOOL Win98 = FALSE;

HANDLE STDOUT=0;

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
int NEAR ReadCommBlock(int port, LPSTR lpszBlock, int nMaxLength);
OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed);
VOID DEDPoll(int Port);
VOID CRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len);
unsigned short int compute_crc(unsigned char *buf,int len);
int Unstuff(UCHAR * Msg, int len);
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);

// Buffer Pool

#define NUMBEROFBUFFERS 20

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers

// Get buffer from Queue

UINT * Q_REM(UINT *Q)
{
	UINT  * first,next;
	
	(int)first=Q[0];
	if (first == 0) return (0);			// Empty
	
	next=first[0];						// Address of next buffer
	Q[0]=next;

	return (first);
}


// Return Buffer to Free Queue

UINT ReleaseBuffer(UINT *BUFF)
{
	UINT * pointer;
	
	(UINT)pointer=FREE_Q;
	*BUFF=(UINT)pointer;
	FREE_Q=(UINT)BUFF;

	return (0);
}


int Q_ADD(UINT *Q,UINT *BUFF)
{
	UINT * next;
	
	BUFF[0]=0;							// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=(UINT)BUFF;				// New one on front
		return(0);
	}

	(int)next=Q[0];

	while (next[0]!=0)
		next=(UINT *)next[0];			// Chain to end of queue

	next[0]=(UINT)BUFF;					// New one on end

	return(0);
}



BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	int i;
	
	switch(ul_reason_being_called)
	{
	case DLL_PROCESS_ATTACH:

		// Build buffer pool

		FREE_Q = 0;			// In case reloading;

		for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
		{	
			ReleaseBuffer(&BufferPool[100*i]);
		}

		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		return 1;
	}
 
	return 1;
}

DllExport int ExtProc(int fn, int port,unsigned char * buff)
{
	int len = 0, txlen = 0;
	UCHAR rxbuff[1000];
	unsigned short  crc;
	UINT * buffptr;
	struct TNCINFO * TNC = &TNCInfo[port];


	switch (fn)
	{
	case 1:				// poll

		DEDPoll(port);

		len = ReadCommBlock(port, rxbuff, 350);

		if (len > 0)
		{
			if (TNC->HostMode == 0)
			{
				// We think TNC is in Terminal Mode

				if (rxbuff[0] != 170)
				{
					// Probably in Term Mode

					return 0;
				}
				else
				{
					// It is in Host Mode

					TNC->HostMode = TRUE;

					// Drop Through
				}
			}
			if (rxbuff[0] == 170)
			{
				len = Unstuff(&rxbuff[2], len - 2);
				crc = compute_crc(&rxbuff[2], len);

				if (crc == 0xf0b8)		// Good CRC
					ProcessDEDFrame(TNC, rxbuff, len);
				else
				{
					// Bad CRC - should we check of complete frame (how)
				}
			}
			else
			{
					// We think it is in Host Mode, but got a frame not starting with 170
					// See if it had dropped back to Terminal Mode

			}

		}

		if (TNC->PACTORtoBPQ_Q !=0)
		{
			int datalen;
			
			buffptr=Q_REM(&TNC->PACTORtoBPQ_Q);

			datalen=buffptr[1];

			buff[7] = 0xf0;
			memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
			datalen+=8;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);
		}
			
		return 0;

	case 2:				// send

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == 0) return (0);			// No buffers, so ignore
		
		if (!TNC->TNCOK)
		{
			// Send Error Response

			buffptr[1] = 36;
			memcpy(buffptr+2, "No Connection to PACTOR TNC\r", 36);

			Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
			
			return 0;
		}

		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		Q_ADD(&TNC->BPQtoPACTOR_Q, buffptr);
		
		return (0);


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(TNCInfo[port].hDevice);
		return (0);
	}

	return 0;

}

DllExport int APIENTRY ExtInit(struct PORTCONTROL *  PortEntry)
{
	char msg[80];
	
	//
	//	Will be called once for each Pactor Port
	//	The COM port number is in IOBASE
	//

	GetAPI();

	wsprintf(msg,"Pactor COM%d", PortEntry->IOBASE);
	WritetoConsole(msg);
	
//	PORTVECTOR(npTTYInfo) = PortVector; //	BX on entry to char handlers
//	RXVECTOR(npTTYInfo) = RXVector; //	Routine to call for each char

	OpenCOMMPort(&TNCInfo[PortEntry->PORTNUMBER], PortEntry->IOBASE, PortEntry->BAUDRATE);

	return ((int)ExtProc);
}
 
VOID KISSCLOSE(int Port)
{ 
	DestroyTTYInfo(Port);
}

BOOL NEAR DestroyTTYInfo(int port)
{
   // force connection closed (if not already closed)

   CloseConnection(&TNCInfo[port]);

   return TRUE;

} // end of DestroyTTYInfo()


BOOL CloseConnection(struct TNCINFO * conn)
{
   // disable event notification and wait for thread
   // to halt

   SetCommMask(conn->hDevice, 0);

   // drop DTR

   EscapeCommFunction(conn->hDevice, CLRDTR);

   // purge any outstanding reads/writes and close device handle

   PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
   CloseHandle(conn->hDevice);
 
   return TRUE;

} // end of CloseConnection()

OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed)
{
	char szPort[15];
	char buf[80];
	BOOL fRetVal;
	COMMTIMEOUTS CommTimeOuts;

	DCB	dcb;

	// load the COM prefix string and append port number
   
	wsprintf( szPort, "//./COM%d", Port) ;

	// open COMM device

	conn->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (conn->hDevice == (HANDLE) -1)
	{
		wsprintf(buf,"COM%d Setup Failed %d ", Port, GetLastError());
		WritetoConsole(buf);
		OutputDebugString(buf);

		return (FALSE);
	}

	SetupComm(conn->hDevice, 4096, 4096); // setup device buffers

	// purge any information in the buffer

	PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O
	  
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(conn->hDevice, &CommTimeOuts);

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(conn->hDevice, &dcb);

	 // setup hardware flow control

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
//	dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.BaudRate = Speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fInX = dcb.fOutX = 0;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;

	// other various settings

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	fRetVal = SetCommState(conn->hDevice, &dcb);

//	conn->RTS = 1;
//	conn->DTR = 1;

	EscapeCommFunction(conn->hDevice,SETDTR);
	EscapeCommFunction(conn->hDevice,SETRTS);
	
	return TRUE;
}


int NEAR ReadCommBlock( int port, LPSTR lpszBlock, int nMaxLength)
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue 
	
	ClearCommError(TNCInfo[port].hDevice, &dwErrorFlags, &ComStat);

	dwLength = min( (DWORD) nMaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(TNCInfo[port].hDevice, lpszBlock,
		                    dwLength, &dwLength, NULL);
		if (!fReadStat)
		{
		    dwLength = 0;
			ClearCommError(TNCInfo[port].hDevice, &dwErrorFlags, &ComStat);
		}
	}

   return (dwLength);

} // end of ReadCommBlock()


BOOL NEAR WriteCommBlock(struct TNCINFO * TNC)
{
	BOOL        fWriteStat;
	DWORD       dwBytesWritten;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(TNC->hDevice, TNC->TXBuffer, TNC->TXLen, &dwBytesWritten, NULL);

	if ((!fWriteStat) || (TNC->TXLen != dwBytesWritten))
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);
	}

	TNC->Timeout = 20;

	return TRUE;

}


VOID DEDPoll(int Port)
{
	struct TNCINFO * TNC = &TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;

	// Send a Generic Poll (Assume CRC Extended DED, so we only poll channel 255)

	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		TNC->Retries--;

		if(TNC->Retries)
		{
			WriteCommBlock(TNC);	// Retransmit Block
			return;
		}

		TNC->TNCOK = FALSE;
	}

	// Send Data if avail, else send poll

	if (TNC->TNCOK && TNC->BPQtoPACTOR_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&TNC->BPQtoPACTOR_Q);

		datalen=buffptr[1];

		Poll[0] = 170;
		Poll[1] = 170;
		Poll[2] = 31;		// PACTOR Channel
		Poll[3] = 0;		// Data
		Poll[4] = datalen - 1;			// Len-1
		memcpy(&Poll[5], buffptr+2, datalen);		// Data goes to +7, but we have an extra byte
		
		ReleaseBuffer(buffptr);
		
		CRCStuffAndSend(TNC, Poll, datalen + 5);

		return;
	}


	Poll[0] = 170;
	Poll[1] = 170;
	Poll[2] = 255;			// Channel
	Poll[3] = 1;			// Command
	Poll[4] = 0;			// Len-1
	Poll[5] = 'G';			// Poll

	CRCStuffAndSend(TNC, Poll, 6);

	return;
}
unsigned short int compute_crc(unsigned char *buf,int len)
{
	int fcs;

	_asm{

	mov	esi,buf
	mov	ecx,len
	mov	edx,-1		; initial value
	mov	edi,CRCTAB	; Going via Import Table

crcloop:

	lodsb

	XOR	DL,AL		; OLD FCS .XOR. CHAR
	MOVZX EBX,DL		; CALC INDEX INTO TABLE FROM BOTTOM 8 BITS
	ADD	EBX,EBX
	ADD	EBX, EDI	; To Table entry
	MOV	DL,DH		; SHIFT DOWN 8 BITS
	XOR	DH,DH		; AND CLEAR TOP BITS
	XOR	DX,[EBX]	; XOR WITH TABLE ENTRY
	
	loop	crcloop

	mov	fcs,EDX

	}	
	return (fcs);
  }

VOID CRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	unsigned short int crc;

    Msg[3] |= TNC->Toggle;
	TNC->Toggle ^= 0x80;

	crc = compute_crc(&Msg[2], Len-2);
	crc ^= 0xffff;

	Msg[Len++] = (crc&0xff);
	Msg[Len++] = (crc>>8);

	TNC->TXLen = Len;

	WriteCommBlock(TNC);

	TNC->Retries = 5;

}

int Unstuff(UCHAR * Msg, int len)
{
	int i, j=0;

	for (i=0; i<len; i++, j++)
	{
		Msg[j] = Msg[i];
		if (Msg[i] == 170)			// Remove next byte
			i++;
	}

	return j;
}

VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * Msg, int len)
{
	// Any valid frame is an ACK

	TNC->Timeout = 0;
	TNC->TNCOK = TRUE;

	//	See if Poll Reply or Data
	
	if (Msg[3] == 1)
	{
		// Poll Response

		return;
	}

	if (Msg[3] == 0)
	{
		// Data

		UINT * buffptr;
		
		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore
			
		buffptr[1] = Msg[4] + 1;
		memcpy(buffptr[1], &Msg[5], buffptr[1]);
		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

		return;
	}

}



