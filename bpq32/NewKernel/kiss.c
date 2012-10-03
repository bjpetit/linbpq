
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

//  Version 410h Jan 2009 Changes for Win98 Virtual COM
//		Open \\.\com instead of //./COM
//		Extra Dignostics

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include "kiss.h"
#include "AsmStrucs.h"

int WritetoConsoleLocal(char * buff);

   
int ASYSEND(struct PORTCONTROL * PortVector, char * buffer, int count)
{
	NPKISSINFO Port = KISSInfo[PortVector->PORTNUMBER];
	
	if (PortVector->PORTIPADDR.S_un.S_addr)		// KISS over UDP
		sendto(Port->sock, buffer, count, 0, (struct sockaddr *)&Port->destaddr, sizeof(Port->destaddr));
	else
		WriteCommBlock(Port, buffer, count);
	
	return (0);
}

VOID ASYDISP(struct PORTCONTROL * PortVector)
{
	char Msg[80];

	if (PortVector->PORTIPADDR.S_un.S_addr)

		// KISS over UDP

		wsprintf(Msg,"UDPKISS IP %s Port %d Chan %c\n", inet_ntoa(PortVector->PORTIPADDR), PortVector->IOBASE, PortVector->CHANNELNUM);
	else
		wsprintf(Msg,"ASYNC COM%d Chan %c\n", PortVector->IOBASE, PortVector->CHANNELNUM);
		
	WritetoConsoleLocal(Msg);
	return;
}


int	ASYINIT(int comport, int speed, struct PORTCONTROL * PortVector, int RXVector, char Channel )
{
	char Msg[80];
	NPKISSINFO npKISSINFO;
	int BPQPort = PortVector->PORTNUMBER;

	if (PortVector->PORTIPADDR.S_un.S_addr)
	{
		SOCKET sock;
		u_long param=1;
		BOOL bcopt=TRUE;
		SOCKADDR_IN sinx;

		// KISS over UDP

		wsprintf(Msg,"UDPKISS IP %s Port %d Chan %c ", inet_ntoa(PortVector->PORTIPADDR), PortVector->IOBASE, Channel);
		WritetoConsoleLocal(Msg);
		
		npKISSINFO = (NPKISSINFO) LocalAlloc( LPTR, sizeof(KISSINFO));

		memset(npKISSINFO, 0, sizeof(KISSINFO));
		npKISSINFO->bPort = comport;
  
		KISSInfo[PortVector->PORTNUMBER] = npKISSINFO;

		npKISSINFO->sock = sock = socket(AF_INET,SOCK_DGRAM,0);

		ioctlsocket(sock, FIONBIO, &param);
 
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&bcopt,4);

		sinx.sin_family = AF_INET;
		sinx.sin_addr.s_addr = INADDR_ANY;		
		sinx.sin_port = htons(PortVector->IOBASE);

		if (bind(sock, (LPSOCKADDR) &sinx, sizeof(sinx)) != 0 )
		{
			char Title[20];

			//	Bind Failed

			int err = WSAGetLastError();
			wsprintf(Msg, "Bind Failed for UDP socket - error code = %d", err);
			wsprintf(Title, "UDPKISS Port %d", PortVector->PORTNUMBER);
			MessageBox(NULL, Msg, Title, MB_OK);
			return 0;
		}

		npKISSINFO->destaddr.sin_family = AF_INET;
		npKISSINFO->destaddr.sin_addr.s_addr = PortVector->PORTIPADDR.S_un.S_addr;		
		npKISSINFO->destaddr.sin_port = htons(PortVector->IOBASE);
	}

	else
	{
		wsprintf(Msg,"ASYNC COM%d Chan %c ", comport, Channel);
		WritetoConsoleLocal(Msg);

		npKISSINFO = KISSInfo[PortVector->PORTNUMBER] = CreateKISSINFO(comport, speed);

		if (NULL == npKISSINFO)
			return ( FALSE ) ;

		OpenConnection(npKISSINFO, comport);
	}

	npKISSINFO->Portvector = PortVector; //	BX on entry to char handlers
	npKISSINFO->RXvector = RXVector; //	Routine to call for each char

	WritetoConsoleLocal("\n");

	return (0);
}

VOID KISSCLOSE(struct PORTCONTROL * PortVector)
{
	NPKISSINFO Port = KISSInfo[PortVector->PORTNUMBER];

	if (Port == NULL)
		return;
	
	if (PortVector->PORTIPADDR.S_un.S_addr)
	{
		closesocket(Port->sock);
	}
	else
	{
		SetCommMask(Port->idComDev, 0 ) ;

		// drop DTR

		EscapeCommFunction(Port->idComDev, CLRDTR);

		// purge any outstanding reads/writes and close device handle

		EscapeCommFunction(Port->idComDev, CLRDTR);
		EscapeCommFunction(Port->idComDev, CLRDTR);
		PurgeComm(Port->idComDev, PURGE_TXABORT | PURGE_RXABORT |
                                   PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
		
		CloseHandle(Port->idComDev);
	}
   
	LocalFree(Port);
	KISSInfo[PortVector->PORTNUMBER] = NULL;

	return;
}

NPKISSINFO CreateKISSINFO( int port,int speed )
{
   NPKISSINFO   npKISSINFO ;

   if (NULL == (npKISSINFO =
                   (NPKISSINFO) LocalAlloc( LPTR, sizeof( KISSINFO ) )))
      return ( (NPKISSINFO) 0 ) ;

   // initialize TTY info structure

	npKISSINFO->idComDev = 0 ;
	npKISSINFO->bPort = port;
	npKISSINFO->dwBaudRate = speed;
	
	return (npKISSINFO);
}

BOOL NEAR OpenConnection(NPKISSINFO npKISSINFO, int port)
{            
	char szPort[15];
	BOOL fRetVal ;
	COMMTIMEOUTS  CommTimeOuts ;
	int	Err;
	char buf[100];

	if (NULL == npKISSINFO)
		return FALSE;

  // load the COM prefix string and append port number
   
  wsprintf( szPort, "\\\\.\\COM%d", port);

   // open COMM device

   npKISSINFO->idComDev =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (npKISSINFO->idComDev == (HANDLE) -1 )
	{
		wsprintf(buf,"COM%d could not be opened ", port);
		WritetoConsoleLocal(buf);
		strcat(buf, "\r\n");
		OutputDebugString(buf);
		return ( FALSE ) ;
	}


   else
   {
	Err = GetFileType(npKISSINFO->idComDev);

	// setup device buffers

      SetupComm(npKISSINFO->idComDev, 4096, 4096 ) ;

      // purge any information in the buffer

      PurgeComm(npKISSINFO->idComDev, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

      // set up for overlapped I/O
	  
      CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
      CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
//      CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
      CommTimeOuts.WriteTotalTimeoutConstant = 500 ;
      SetCommTimeouts(npKISSINFO->idComDev, &CommTimeOuts ) ;
   }
	
	fRetVal = SetupConnection(npKISSINFO);

	if (fRetVal)
	{
		// assert DTR

		EscapeCommFunction(npKISSINFO->idComDev, SETDTR);
	}
	else
	{
	   wsprintf(buf,"COM%d Setup Failed %d ", npKISSINFO->bPort, GetLastError());
	   WritetoConsoleLocal(buf);
	   OutputDebugString(buf);

		CloseHandle(npKISSINFO->idComDev);
	}

	return fRetVal;

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

BOOL NEAR SetupConnection(NPKISSINFO npKISSINFO)
{
	BOOL       fRetVal ;
	DCB        dcb ;
	
	if (NULL == npKISSINFO)
      return FALSE;

   dcb.DCBlength = sizeof( DCB ) ;

   GetCommState(npKISSINFO->idComDev, &dcb ) ;

   dcb.BaudRate = npKISSINFO->dwBaudRate;
   dcb.ByteSize = 8;
   dcb.Parity = 0;
   dcb.StopBits = 0;

   // setup hardware flow control

   dcb.fOutxDsrFlow = 0;
   dcb.fDtrControl = DTR_CONTROL_ENABLE ;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_ENABLE ;

   // setup software flow control

   dcb.fInX = dcb.fOutX = 0;
   dcb.XonChar = 0;
   dcb.XoffChar = 0;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = FALSE;

   fRetVal = SetCommState(npKISSINFO->idComDev, &dcb ) ;

   return ( fRetVal ) ;

} // end of SetupConnection()

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

int NEAR ReadCommBlock(NPKISSINFO npKISSINFO, LPSTR lpszBlock, int nMaxLength )
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	if (NULL == npKISSINFO)
		return ( FALSE ) ;

	// only try to read number of bytes in queue 
	
	ClearCommError(npKISSINFO->idComDev, &dwErrorFlags, &ComStat ) ;

	dwLength = min( (DWORD) nMaxLength, ComStat.cbInQue ) ;

	if (dwLength > 0)
	{
		fReadStat = ReadFile(npKISSINFO->idComDev, lpszBlock,
		                    dwLength, &dwLength, NULL) ;
		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError(npKISSINFO->idComDev, &dwErrorFlags, &ComStat ) ;
		}
	}

   return ( dwLength ) ;

} // end of ReadCommBlock()


BOOL NEAR WriteCommBlock(NPKISSINFO npKISSINFO, LPSTR lpByte, DWORD dwBytesToWrite)
{
	BOOL        fWriteStat;
	DWORD       dwBytesWritten;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;

	if (NULL == npKISSINFO)
		return FALSE;

	fWriteStat = WriteFile(npKISSINFO->idComDev, lpByte, dwBytesToWrite,
	                       &dwBytesWritten, NULL );

	if ((!fWriteStat) || (dwBytesToWrite != dwBytesWritten))
	{
		ClearCommError(npKISSINFO->idComDev, &dwErrorFlags, &ComStat ) ;
	}
	return ( TRUE ) ;

} // end of WriteCommBlock()


int POLLKISS(struct PORTCONTROL * PortVector)
{
	int i,j,nLength = 0;
    BYTE  abIn[ MAXBLOCK + 1];
	NPKISSINFO npKISSINFO ;

	if (NULL == (npKISSINFO = KISSInfo[PortVector->PORTNUMBER]))
		return ( FALSE ) ;

	if (PortVector->PORTIPADDR.S_un.S_addr)		// KISS over UDP
	{
		NPKISSINFO Port = KISSInfo[PortVector->PORTNUMBER];
		SOCKADDR_IN rxaddr;
		int addrlen = sizeof(SOCKADDR_IN);

		nLength = recvfrom(Port->sock, abIn, MAXBLOCK, 0, (LPSOCKADDR)&rxaddr, &addrlen);
		if (nLength < 0)
		{
			nLength = GetLastError();
			nLength = 0;
		}
	}
	else
		nLength = ReadCommBlock(npKISSINFO, (LPSTR) abIn, MAXBLOCK );

	if (nLength)
	{
		i = npKISSINFO->RXvector;
		j = (int)npKISSINFO->Portvector;

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
