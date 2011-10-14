//
//	DLL to inteface the BPQ Virtual COM emulator to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

//	Version 1.0 November 2005
//

//  Version 1.1	October 2006

//		Write diagmnostics to BPQ console window instead of STDOUT

// Version 1.2 February 2008

//		Changes for dynamic unload of bpq32.dll

// Version 1.2.1 May 2008

//		Correct RX length (was 1 byte too long)

// Version 1.3.1 Jan 2009

//		Support Win98 VirtualCOM Driver

#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include <stdlib.h>

//#include <process.h>
//#include <time.h>

#define VERSION_MAJOR         1
#define VERSION_MINOR         0


#include "bpqvkiss.h"
#include "ASMStrucs.h"

//#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
//#define EXTDLL			// Use GetMuduleHandle instead of LoadLibrary 
#include "bpq32.h"
 

static int	ASYINIT(int comport, int speed, int bpqport);
int	kissencode(UCHAR * inbuff, UCHAR * outbuff, int len);
int GetRXMessage(int port,UCHAR * buff);
void CheckReceivedData(PVCOMINFO  pVCOMInfo);
static int ReadCommBlock(PVCOMINFO  pVCOMInfo, LPSTR lpszBlock, DWORD nMaxLength );
static BOOL WriteCommBlock(int port, LPSTR lpByte , DWORD dwBytesToWrite);

PVCOMINFO CreateInfo( int port,int speed, int bpqport )	;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

static BOOL Win98 = FALSE;
static BOOL NewVCOM = FALSE;		// Use new User Mode VCOM Driver


static int ExtProc(int fn, int port,unsigned char * buff)
{
	int len,txlen=0;
	char txbuff[1000];

	switch (fn)
	{
	case 1:				// poll

		len = GetRXMessage(port,buff);
	
		if (len>0)
			return (len);
		
		return len;

	case 2:				// send

		txlen=(buff[6]<<8) + buff[5];

		txlen=kissencode(&buff[7],(char *)&txbuff,txlen-7);

		WriteCommBlock(port,txbuff,txlen);
		
		return (0);


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK
			
		break;

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(VCOMInfo[port]->ComDev);

		return (0);

	}

	return 0;

}

UINT WINAPI VCOMExtInit(struct PORTCONTROL *  PortEntry)
{
	char msg[80];
	
	//
	//	Will be called once for each port to be mapped to a BPQ Virtual COM Port
	//	The VCOM port number is in IOBASE
	//

	wsprintf(msg,"VKISS COM%d", PortEntry->IOBASE);
	WritetoConsole(msg);
	
	CreateInfo(PortEntry->IOBASE,9600, PortEntry->PORTNUMBER);

	// Open File
	
	ASYINIT(PortEntry->IOBASE,9600, PortEntry->PORTNUMBER);

	return ((UINT) ExtProc);
}

static int	kissencode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	outbuff[0]=FEND;
	outbuff[1]=0;
	txptr=2;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];
		
		switch (c)
		{
		case FEND:
			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFEND;
			break;

		case FESC:

			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFESC;
			break;

		default:

			outbuff[txptr++]=c;
		}
	}

	outbuff[txptr++]=FEND;

	return txptr;

}

int	ASYINIT(int comport, int speed, int bpqport)
{
   char       szPort[ 30 ];
   char buf[256];
   int n;

#pragma warning( push )
#pragma warning( disable : 4996 )

   if (HIBYTE(_winver) < 5)
		Win98 = TRUE;

#pragma warning( pop ) 

   if (Win98)
	{
		VCOMInfo[bpqport]->ComDev = CreateFile( "\\\\.\\BPQVCOMM.VXD", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
	else if (NewVCOM)
	{
		wsprintf( szPort, "\\\\.\\pipe\\BPQCOM%d", comport ) ;

		VCOMInfo[bpqport]->ComDev = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}		  
	else
	{
		wsprintf( szPort, "\\\\.\\BPQ%d", comport ) ;

		VCOMInfo[bpqport]->ComDev = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}		  
	if (VCOMInfo[bpqport]->ComDev == (HANDLE) -1 )
	{
		n=wsprintf(buf,"Virtual COM Port %d could not be opened ",comport);
		WritetoConsole(buf);

		return (FALSE) ;
	}

	return (TRUE) ;
}

static int GetRXMessage(int port,UCHAR * buff)
{
	int len;
	PVCOMINFO pVCOMInfo ;

	if (NULL == (pVCOMInfo = VCOMInfo[port]))
		return 0;

	if (!pVCOMInfo->MSGREADY)
		CheckReceivedData(pVCOMInfo);		// Look for data in RXBUFFER and COM port

	if (pVCOMInfo->MSGREADY)
	{
		len=pVCOMInfo->RXMPTR-&pVCOMInfo->RXMSG[1];		// Don't need KISS Control Byte
		
 		if (pVCOMInfo->RXMSG[0] != 0)
		{
			pVCOMInfo->MSGREADY=FALSE;
			pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;
			return 0;						// Not KISS Data
		}

		//
		//	Remove KISS control byte
		//

		memcpy(&buff[7],&pVCOMInfo->RXMSG[1],len);

		len+=7;
		buff[5]=(len & 0xff);
		buff[6]=(len >> 8);
		
		//
		//	reset pointers
		//

		pVCOMInfo->MSGREADY=FALSE;
		pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;

		return len;
	}
	else

	return 0;					// nothing doing
}

static void CheckReceivedData(PVCOMINFO pVCOMInfo)
{
 	UCHAR c;

	if (pVCOMInfo->RXBCOUNT == 0)
	{	
		//
		//	Check com buffer
		//
	
		pVCOMInfo->RXBCOUNT = ReadCommBlock(pVCOMInfo, (LPSTR) &pVCOMInfo->RXBUFFER, MAXBLOCK-1 );
		pVCOMInfo->RXBPTR=(UCHAR *)&pVCOMInfo->RXBUFFER; 
	}

	if (pVCOMInfo->RXBCOUNT == 0)
		return;

	while (pVCOMInfo->RXBCOUNT != 0)
	{
		pVCOMInfo->RXBCOUNT--;

		c = *(pVCOMInfo->RXBPTR++);

		if (pVCOMInfo->ESCFLAG)
		{
			//
			//	FESC received - next should be TFESC or TFEND

			pVCOMInfo->ESCFLAG = FALSE;

			if (c == TFESC)
				c=FESC;
	
			if (c == TFEND)
				c=FEND;

		}
		else
		{
			switch (c)
			{
			case FEND:		
	
				//
				//	Either start of message or message complete
				//
				
				if (pVCOMInfo->RXMPTR == (UCHAR *)&pVCOMInfo->RXMSG)
					continue;

				pVCOMInfo->MSGREADY=TRUE;
				return;

			case FESC:
		
				pVCOMInfo->ESCFLAG = TRUE;
				continue;

			}
		}
		
		//
		//	Ok, a normal char
		//

		*(pVCOMInfo->RXMPTR++) = c;

	}

	if (pVCOMInfo->RXMPTR - (UCHAR *)&pVCOMInfo->RXMSG > 500)
		pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;
	
 	return;
}

static PVCOMINFO CreateInfo( int port,int speed, int bpqport )
{
   PVCOMINFO pVCOMInfo ;

   if (NULL == (pVCOMInfo =
                   (PVCOMINFO) LocalAlloc( LPTR, sizeof( VCOMINFO ) )))
      return ( (PVCOMINFO) -1 ) ;

 	pVCOMInfo->RXBCOUNT=0;
	pVCOMInfo->MSGREADY=FALSE;
	pVCOMInfo->RXBPTR=(UCHAR *)&pVCOMInfo->RXBUFFER; 
	pVCOMInfo->RXMPTR=(UCHAR *)&pVCOMInfo->RXMSG;
   
	pVCOMInfo->ComDev = 0 ;
	pVCOMInfo->Connected = FALSE ;
	pVCOMInfo->Port = port;

	VCOMInfo[bpqport]=pVCOMInfo;
	
	return (pVCOMInfo);
}

static BOOL NEAR DestroyTTYInfo( int port )
{
   PVCOMINFO pVCOMInfo ;

   if (NULL == (pVCOMInfo = VCOMInfo[port]))
      return ( FALSE ) ;

   LocalFree( pVCOMInfo ) ;

   VCOMInfo[port] = 0;

   return ( TRUE ) ;

} 

static int ReadCommBlock(PVCOMINFO  pVCOMInfo, LPSTR lpszBlock, DWORD nMaxLength)
{
	DWORD dwLength = 0;
	DWORD Available = 0;

	if (Win98)
		DeviceIoControl(pVCOMInfo->ComDev, (pVCOMInfo->Port << 16) | W98_SERIAL_GETDATA,
					NULL,0,lpszBlock,nMaxLength, &dwLength,NULL);

	else if (NewVCOM)
	{
		PeekNamedPipe(pVCOMInfo->ComDev, NULL, 0, NULL, &Available, NULL);

		if (Available > nMaxLength)
			Available = nMaxLength;
		
		if (Available)
			ReadFile(pVCOMInfo->ComDev, lpszBlock, Available, &dwLength, NULL);
	}

	else
		DeviceIoControl(
			pVCOMInfo->ComDev,IOCTL_SERIAL_GETDATA,NULL,0,lpszBlock,nMaxLength, &dwLength,NULL);

   return (dwLength);

}

static BOOL WriteCommBlock(int port, LPSTR Message , DWORD MsgLen)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(
			VCOMInfo[port]->ComDev,(VCOMInfo[port]->Port << 16) | W98_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);

	else if (NewVCOM)
		return WriteFile(VCOMInfo[port]->ComDev, Message, MsgLen, &bytesReturned, NULL);

	else
		return DeviceIoControl(
			VCOMInfo[port]->ComDev,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);

}



