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

#define VERSION_MAJOR         1
#define VERSION_MINOR         0


#include "SCSPactor.h"
#include "ASMStrucs.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetMuduleHandle instead of LoadLibrary 
#include "bpq32.h"
 
#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )


#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

BOOL Win98 = FALSE;

HANDLE STDOUT=0;

struct KISSINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct KISSINFO * conn);
BOOL NEAR WriteCommBlock(int, LPSTR, DWORD);
BOOL NEAR DestroyTTYInfo(int port);


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:

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
	char txbuff[1000];

	switch (fn)
	{
	case 1:				// poll

		len = ReadCommBlock(port,&buff[8], 350);

		if (len > 0)
		{
			buff[7] = 0xf0;
			len+=8;
			buff[5]=(len & 0xff);
			buff[6]=(len >> 8);
		}
			
		return len;

	case 2:				// send

		txlen=(buff[6]<<8) + buff[5];

//		txlen=kissencode(&buff[7],(char *)&txbuff,txlen-7);

		WriteCommBlock(port,&buff[8],txlen-8);
		
		return (0);


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(KISSInfo[port].hDevice);
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

	OpenCOMMPort(&KISSInfo[PortEntry->PORTNUMBER], PortEntry->IOBASE, PortEntry->BAUDRATE);

	return ((int) ExtProc);
}
 
int ASYSEND(int port, char * buffer,int count)
{
   WriteCommBlock(port,buffer, count);
   return (0);
}

VOID KISSCLOSE(int Port)
{ 
	DestroyTTYInfo(Port);
}

BOOL NEAR DestroyTTYInfo( int port )
{

   // force connection closed (if not already closed)

   CloseConnection(&KISSInfo[port]);

   return TRUE;

} // end of DestroyTTYInfo()


BOOL CloseConnection(struct KISSINFO * conn)
{
   // disable event notification and wait for thread
   // to halt

   SetCommMask(conn->hDevice, 0);

   // drop DTR

   EscapeCommFunction(conn->hDevice, CLRDTR);

   // purge any outstanding reads/writes and close device handle

   PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT |
                                   PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
   CloseHandle(conn->hDevice);
 
   return TRUE;

} // end of CloseConnection()

OpenCOMMPort(struct KISSINFO * conn, int Port, int Speed)
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
				  
	if (conn->hDevice == (HANDLE) -1 )
	{
		wsprintf(buf,"COM%d Setup Failed %d ", Port, GetLastError());
		WritetoConsole(buf);
		OutputDebugString(buf);

		return (FALSE) ;
	}

      // setup device buffers

      SetupComm(conn->hDevice, 4096, 4096 ) ;

      // purge any information in the buffer

      PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

      // set up for overlapped I/O
	  
      CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
      CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
      SetCommTimeouts(conn->hDevice, &CommTimeOuts ) ;

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02

   
	  dcb.DCBlength = sizeof( DCB );

	  GetCommState(conn->hDevice, &dcb );

	  BuildCommDCB(conn->Params, &dcb);	

	  // setup hardware flow control

      dcb.fDtrControl = DTR_CONTROL_ENABLE;
    
//	  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE ;

      dcb.fRtsControl = RTS_CONTROL_ENABLE;

	  dcb.BaudRate = Speed;
	  dcb.ByteSize = 8;
	  dcb.Parity = NOPARITY;
	  dcb.StopBits = ONESTOPBIT;

	  dcb.fInX = dcb.fOutX = 0;
	  dcb.XonChar = 0 ;
	  dcb.XoffChar = 0 ;
	  dcb.XonLim = 0 ;
	  dcb.XoffLim = 0 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = TRUE ;

   fRetVal = SetCommState(conn->hDevice, &dcb ) ;

   conn->RTS = 1;
//				conn->DTR = 1;
//				EscapeCommFunction(conn->hDevice,SETDTR);

   EscapeCommFunction(conn->hDevice,SETRTS);

   return TRUE;


}


int NEAR ReadCommBlock( int port, LPSTR lpszBlock, int nMaxLength )
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue 
	
	ClearCommError(KISSInfo[port].hDevice, &dwErrorFlags, &ComStat ) ;

	dwLength = min( (DWORD) nMaxLength, ComStat.cbInQue ) ;

	if (dwLength > 0)
	{
		fReadStat = ReadFile(KISSInfo[port].hDevice, lpszBlock,
		                    dwLength, &dwLength, NULL) ;
		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError(KISSInfo[port].hDevice, &dwErrorFlags, &ComStat ) ;
		}
	}

   return ( dwLength ) ;

} // end of ReadCommBlock()


BOOL NEAR WriteCommBlock( int port, LPSTR lpByte , DWORD dwBytesToWrite)
{

	BOOL        fWriteStat ;
	DWORD       dwBytesWritten ;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;


	fWriteStat = WriteFile(KISSInfo[port].hDevice, lpByte, dwBytesToWrite,
	                       &dwBytesWritten, NULL ) ;


	if ((!fWriteStat) || (dwBytesToWrite != dwBytesWritten))
	{
		ClearCommError(KISSInfo[port].hDevice, &dwErrorFlags, &ComStat ) ;
	}
	return ( TRUE ) ;

} // end of WriteCommBlock()

