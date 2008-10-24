
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1995 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

//	Version 409p March 2005 Allow Multidigit COM Ports

#include "kiss.h"

int WritetoConsoleLocal(char * buff);

   
int ASYSEND(int port, char * buffer,int count)
{
   WriteCommBlock(port,buffer, count);
   return (0);
}



int	ASYINIT(int port, int speed, int PortVector,int RXVector)
{
	NPTTYINFO npTTYInfo ;
	
	WritetoConsoleLocal("ASYNC ");

	CreateTTYInfo(port,speed);

	if (NULL == (npTTYInfo = KISSInfo[port]))
		return ( FALSE ) ;

	PORTVECTOR(npTTYInfo) = PortVector; //	BX on entry to char handlers
	RXVECTOR(npTTYInfo) = RXVector; //	Routine to call for each char

	OpenConnection(port);
	return (0);
}

VOID KISSCLOSE(int Port)
{ 
	DestroyTTYInfo(Port);
}

NPTTYINFO CreateTTYInfo( int port,int speed )
{
   NPTTYINFO   npTTYInfo ;

   if (NULL == (npTTYInfo =
                   (NPTTYINFO) LocalAlloc( LPTR, sizeof( TTYINFO ) )))
      return ( (NPTTYINFO) -1 ) ;

   // initialize TTY info structure

   COMDEV( npTTYInfo )        = 0 ;
   CONNECTED( npTTYInfo )     = FALSE ;
   PORT( npTTYInfo )          = port;
   BAUDRATE( npTTYInfo )      = speed; //CBR_9600 ;
   BYTESIZE( npTTYInfo )      = 8 ;
   FLOWCTRL( npTTYInfo )      = 0; FC_XONXOFF ;
   PARITY( npTTYInfo )        = NOPARITY ;
   STOPBITS( npTTYInfo )      = ONESTOPBIT ;

	KISSInfo[port]=npTTYInfo;
	
	return (npTTYInfo);
}


//---------------------------------------------------------------------------
//  BOOL NEAR DestroyTTYInfo( HWND hWnd )
//
//  Description:
//     Destroys block associated with TTY window handle.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - Needed to clean up event objects created during initialization.
//
//---------------------------------------------------------------------------

BOOL NEAR DestroyTTYInfo( int port )
{
   NPTTYINFO npTTYInfo ;

   if (NULL == (npTTYInfo = KISSInfo[port]))
      return ( FALSE ) ;

   // force connection closed (if not already closed)

   if (CONNECTED( npTTYInfo ))
      CloseConnection( port ) ;

   LocalFree( npTTYInfo ) ;

   return ( TRUE ) ;

} // end of DestroyTTYInfo()

//  BOOL NEAR OpenConnection( HWND hWnd )
//
//  Description:
//     Opens communication port specified in the TTYINFO struct.
//     It also sets the CommState and notifies the window via
//     the fConnected flag in the TTYINFO struct.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - OpenComm() is not supported under Win-32.  Use CreateFile()
//       and setup for OVERLAPPED_IO.
//     - Win-32 has specific communication timeout parameters.
//     - Created the secondary thread for event notification.
//
//---------------------------------------------------------------------------

BOOL NEAR OpenConnection( int port)
{            
   char       szPort[ 15 ];
   BOOL       fRetVal ;
   NPTTYINFO  npTTYInfo ;
   COMMTIMEOUTS  CommTimeOuts ;
   
	char buf[100];

   if (NULL == (npTTYInfo = KISSInfo[port]))
      return ( FALSE ) ;

  // load the COM prefix string and append port number
   
  wsprintf( szPort, "//./COM%d", PORT( npTTYInfo ) ) ;

   // open COMM device

   COMDEV( npTTYInfo ) =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (COMDEV( npTTYInfo ) == (HANDLE) -1 )
	{
		wsprintf(buf,"COM%d could not be opened ", PORT( npTTYInfo ));
		WritetoConsoleLocal(buf);
		return ( FALSE ) ;
	}

   else
   {
      // setup device buffers

      SetupComm( COMDEV( npTTYInfo ), 4096, 4096 ) ;

      // purge any information in the buffer

      PurgeComm( COMDEV( npTTYInfo ), PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

      // set up for overlapped I/O
	  
      CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
      CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
      SetCommTimeouts( COMDEV( npTTYInfo ), &CommTimeOuts ) ;
   }

   fRetVal = SetupConnection( port ) ;

   if (fRetVal)
   {
      CONNECTED( npTTYInfo ) = TRUE ;

      // assert DTR

      EscapeCommFunction( COMDEV( npTTYInfo ), SETDTR ) ;

   }
   else
   {
      CONNECTED( npTTYInfo ) = FALSE ;
      CloseHandle( COMDEV( npTTYInfo ) ) ;
   }

   return ( fRetVal ) ;

} // end of OpenConnection()

//---------------------------------------------------------------------------
//  BOOL NEAR SetupConnection( HWND hWnd )
//
//  Description:
//     This routines sets up the DCB based on settings in the
//     TTY info structure and performs a SetCommState().
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - Win-32 requires a slightly different processing of the DCB.
//       Changes were made for configuration of the hardware handshaking
//       lines.
//
//---------------------------------------------------------------------------

BOOL NEAR SetupConnection( int port )
{
   BOOL       fRetVal ;
   BYTE       bSet ;
   DCB        dcb ;
   NPTTYINFO  npTTYInfo ;


   
   if (NULL == (npTTYInfo = KISSInfo[port]))
      return ( FALSE ) ;

   dcb.DCBlength = sizeof( DCB ) ;

   GetCommState( COMDEV( npTTYInfo ), &dcb ) ;

   dcb.BaudRate = BAUDRATE( npTTYInfo ) ;
   dcb.ByteSize = BYTESIZE( npTTYInfo ) ;
   dcb.Parity = PARITY( npTTYInfo ) ;
   dcb.StopBits = STOPBITS( npTTYInfo ) ;

   // setup hardware flow control

   bSet = (BYTE) ((FLOWCTRL( npTTYInfo ) & FC_DTRDSR) != 0) ;
   dcb.fOutxDsrFlow = bSet ;
   if (bSet)
      dcb.fDtrControl = DTR_CONTROL_HANDSHAKE ;
   else
      dcb.fDtrControl = DTR_CONTROL_ENABLE ;

   bSet = (BYTE) ((FLOWCTRL( npTTYInfo ) & FC_RTSCTS) != 0) ;
	dcb.fOutxCtsFlow = bSet ;
   if (bSet)
      dcb.fRtsControl = RTS_CONTROL_HANDSHAKE ;
   else
      dcb.fRtsControl = RTS_CONTROL_ENABLE ;

   // setup software flow control

   bSet = (BYTE) ((FLOWCTRL( npTTYInfo ) & FC_XONXOFF) != 0) ;

   dcb.fInX = dcb.fOutX = bSet ;
   dcb.XonChar = ASCII_XON ;
   dcb.XoffChar = ASCII_XOFF ;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = TRUE ;

   fRetVal = SetCommState( COMDEV( npTTYInfo ), &dcb ) ;

   return ( fRetVal ) ;

} // end of SetupConnection()

//---------------------------------------------------------------------------
//  BOOL NEAR CloseConnection( HWND hWnd )
//
//  Description:
//     Closes the connection to the port.  Resets the connect flag
//     in the TTYINFO struct.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//  Win-32 Porting Issues:
//     - Needed to stop secondary thread.  SetCommMask() will signal the
//       WaitCommEvent() event and the thread will halt when the
//       CONNECTED() flag is clear.
//     - Use new PurgeComm() API to clear communications driver before
//       closing device.
//
//---------------------------------------------------------------------------

BOOL NEAR CloseConnection( int port )
{
   NPTTYINFO  npTTYInfo ;

   if (NULL == (npTTYInfo = KISSInfo[port]))
      return ( FALSE ) ;

   // set connected flag to FALSE

   CONNECTED( npTTYInfo ) = FALSE ;

   // disable event notification and wait for thread
   // to halt

   SetCommMask( COMDEV( npTTYInfo ), 0 ) ;

   // drop DTR

   EscapeCommFunction( COMDEV( npTTYInfo ), CLRDTR ) ;

   // purge any outstanding reads/writes and close device handle

   PurgeComm( COMDEV( npTTYInfo ), PURGE_TXABORT | PURGE_RXABORT |
                                   PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
   CloseHandle( COMDEV( npTTYInfo ) ) ;

 
   return ( TRUE ) ;

} // end of CloseConnection()

//---------------------------------------------------------------------------
//  int NEAR ReadCommBlock( HWND hWnd, LPSTR lpszBlock, int nMaxLength )
//
//  Description:
//     Reads a block from the COM port and stuffs it into
//     the provided buffer.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//     LPSTR lpszBlock
//        block used for storage
//
//     int nMaxLength
//        max length of block to read
//
//  Win-32 Porting Issues:
//     - ReadComm() has been replaced by ReadFile() in Win-32.
//     - Overlapped I/O has been implemented.
//
//---------------------------------------------------------------------------

int NEAR ReadCommBlock( int port, LPSTR lpszBlock, int nMaxLength )
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;
	NPTTYINFO  npTTYInfo ;

	if (NULL == (npTTYInfo = KISSInfo[port]))
		return ( FALSE ) ;

	// only try to read number of bytes in queue 
	
	ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;

	dwLength = min( (DWORD) nMaxLength, ComStat.cbInQue ) ;

	if (dwLength > 0)
	{
		fReadStat = ReadFile( COMDEV( npTTYInfo ), lpszBlock,
		                    dwLength, &dwLength, NULL) ;
		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
		}
	}

   return ( dwLength ) ;

} // end of ReadCommBlock()

//---------------------------------------------------------------------------
//  BOOL NEAR WriteCommBlock( HWND hWnd, BYTE *pByte )
//
//  Description:
//     Writes a block of data to the COM port specified in the associated
//     TTY info structure.
//
//  Parameters:
//     HWND hWnd
//        handle to TTY window
//
//     BYTE *pByte
//        pointer to data to write to port
//
//  Win-32 Porting Issues:
//     - WriteComm() has been replaced by WriteFile() in Win-32.
//     - Overlapped I/O has been implemented.
//
//---------------------------------------------------------------------------

BOOL NEAR WriteCommBlock( int port, LPSTR lpByte , DWORD dwBytesToWrite)
{

	BOOL        fWriteStat ;
	DWORD       dwBytesWritten ;
	NPTTYINFO   npTTYInfo ;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;


	if (NULL == (npTTYInfo = KISSInfo[port]))
		return ( FALSE ) ;

	fWriteStat = WriteFile( COMDEV( npTTYInfo ), lpByte, dwBytesToWrite,
	                       &dwBytesWritten, NULL ) ;


	if (!fWriteStat) 
	{
		ClearCommError( COMDEV( npTTYInfo ), &dwErrorFlags, &ComStat ) ;
	}
	return ( TRUE ) ;

} // end of WriteCommBlock()


int POLLKISS(int port)
{
	int i,j,nLength;
    BYTE  abIn[ MAXBLOCK + 1];
	NPTTYINFO npTTYInfo ;

	if (NULL == (npTTYInfo = KISSInfo[port]))
		return ( FALSE ) ;

	
	if (nLength = ReadCommBlock(PORT( npTTYInfo ), (LPSTR) abIn, MAXBLOCK ))
	{
   		i=RXVECTOR(npTTYInfo);
		j=PORTVECTOR(npTTYInfo);

       	_asm {
						
			pushfd
			cld
			pushad
						
			lea esi,abIn
			mov	ecx,nLength
			jcxz	nochars							
charloop:
						
			lodsb
			mov edx,i
			mov	ebx,j
			push	esi
			push	ecx
			push	edx
			call	[edx]
			pop		edx
			pop		ecx
			pop		esi
			loop	charloop
nochars:
			popad
			popfd

			}
	}
	return (0);
}
