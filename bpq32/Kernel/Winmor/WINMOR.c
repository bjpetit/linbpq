//
//	DLL to provide interface to allow G8BPQ switch to use WINMOR as a Port Driver 
//
//	Uses BPQ EXTERNAL interface
//


//  Version 1.0 January 2009 - Initial Version
//


#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <time.h>

#include "AsmStrucs.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetMuduleHandle instead of LoadLibrary 
#include "bpq32.h"

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

unsigned long _beginthread( void( *start_address )( int ), unsigned stack_size, int arglist);

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

DllImport int ResetExtDriver(int num);
void ConnecttoWINMORThread(int port);
VOID ProcessDataSocketData(int port);
BOOL ReadConfigFile(char * filename);
int ConnecttoWINMOR();
int ProcessReceivedData(int bpqport);
int ProcessLine(char * buf);
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int Q_ADD(UINT *Q,UINT *BUFF);

DllImport UCHAR NEXTID;
DllImport struct TRANSPORTENTRY * L4TABLE;
DllImport WORD MAXCIRCUITS;
DllImport UCHAR L4DEFAULTWINDOW;
DllImport WORD L4T1;

#pragma pack(1)
#pragma pack()

typedef struct TNCINFO_S
{ 
	int WINMORtoBPQ_Q;			// Frames for BPQ, indexed by BPQ Port
	int BPQtoWINMOR_Q;			// Frames for WINMOR. indexed by WINMOR port. Only used it TCP session is blocked

	SOCKET WINMORSock;			// Control Socket, indexed by BPQ Port
	SOCKET WINMORDataSock;		// Data Socket, indexed by BPQ Port

	char * WINMORSignon;		// Pointer to message for secure signin
	char * WINMORHostName;		// WINMOR Host - may be dotted decimal or DNS Name
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitScript;			// Initialisation Commands

    UCHAR TCPBuffer[100];		// For converting byte stream to messages
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	BOOL Connected;				// When set, all data is passed to Data Socket

	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	time_t lasttime;

	BOOL CONNECTING;
	BOOL CONNECTED;
	BOOL DATACONNECTING;
	BOOL DATACONNECTED;

	SOCKADDR_IN destaddr;
	SOCKADDR_IN Datadestaddr;

	struct _EXTPORTDATA * PortRecord;

} TNCINFO;

#define MAXBPQPORTS 16

TNCINFO TNCInfo[MAXBPQPORTS+1]={0};

static time_t ltime;

//LOGFONT LFTTYFONT ;

//HFONT hFont ;

static char * WINMORSignon[MAXBPQPORTS+1];			// Pointer to message for secure signin

static char * WINMORHostName[MAXBPQPORTS+1];		// WINMOR Host - may be dotted decimal or DNS Name

#pragma pack()

static unsigned int WINMORInst = 0;
static int AttachedProcesses=0;

static HWND hResWnd,hMHWnd;
static BOOL GotMsg;

static HANDLE STDOUT=0;

//SOCKET sock;

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);

static BOOL Alerted = FALSE;

//static short WINMORPort=0;

BOOL InitWINMOR(void);
BOOL InitWS2(void);

//HANDLE hInstance;

// Buffer Pool

#define NUMBEROFBUFFERS 20

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
//	hInstance=hInst;

	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:
		
		AttachedProcesses++;
		
		if (WINMORInst == 0)				// First entry
		{
			GetAPI();					// Load BPQ32

			WINMORInst = GetCurrentProcessId();

//			STDOUT = GetStdHandle(STD_OUTPUT_HANDLE);

			if (!InitWS2())
				return FALSE;

			if (!InitWINMOR())
				return FALSE;

		}
		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		if (WINMORInst == GetCurrentProcessId())
		{			
			WINMORInst=0;
		}
		
		AttachedProcesses--;

		if (AttachedProcesses == 0)
		{
//			KillTimer(NULL,TimerHandle);
		}
		return 1;
	}
 
	return 1;
	
}


static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;


DllExport int ExtProc(int fn, int port,unsigned char * buff)
{
	int i,winerr;
	int datalen;
	UINT * buffptr;
	char txbuff[500];

	unsigned int bytes,txlen=0;
	char ErrMsg[255];

	TNCINFO * TNC = &TNCInfo[port];

	switch (fn)
	{
	case 1:				// poll

		if (TNC->PortRecord->ATTACHEDSESSION == 0 && TNC->Connected)
		{
			// Node has disconnected - clear any connection

			TNC->Connected = FALSE;
			send(TNC->WINMORSock,"DISCONNECT\r", 11, 0);
		}


		if (TNC->ReportDISC)
		{
			TNC->ReportDISC = FALSE;
			return -1;
		}

			if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
			{
				//	See if time to reconnect
		
				time(&ltime);
				if (ltime - TNC->lasttime >9 )
				{
					ConnecttoWINMOR(port);
					TNC->lasttime = ltime;
				}
			}
		
			FD_ZERO(&readfs);
			
			if (TNC->CONNECTED) FD_SET(TNC->WINMORSock,&readfs);
			
			FD_ZERO(&writefs);

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->WINMORSock,&writefs);	// Need notification of busy clearing

			FD_ZERO(&errorfs);
		
			if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->WINMORSock,&errorfs);

			if (select(3,&readfs,&writefs,&errorfs,&timeout) > 0)
			{
				//	See what happened

				if (readfs.fd_count == 1)
					ProcessReceivedData(port);			

				if (writefs.fd_count == 1)
				{
					// Write block has cleared. Send rest of packet

					buffptr=Q_REM(&TNC->BPQtoWINMOR_Q);
					txlen=buffptr[1];
					memcpy(txbuff,buffptr+2,txlen);
					bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
					ReleaseBuffer(buffptr);
				}
					
				if (errorfs.fd_count == 1)
				{
					i=wsprintf(ErrMsg, "WINMOR Connection lost for BPQ Port %d\r\n", port);
					WritetoConsole(ErrMsg);
				
					TNC->CONNECTING = FALSE;
					TNC->CONNECTED = FALSE;
				}
			}

			FD_ZERO(&readfs);
			
			if (TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&readfs);
			
			FD_ZERO(&writefs);

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->WINMORDataSock,&writefs);	// Need notification of busy clearing

			FD_ZERO(&errorfs);
		
			if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&errorfs);

			if (select(3,&readfs,&writefs,&errorfs,&timeout) > 0)
			{
				//	See what happened

				if (readfs.fd_count == 1)
					ProcessDataSocketData(port);			
				
				if (writefs.fd_count == 1)
				{
					// Write block has cleared. Send rest of packet

					buffptr=Q_REM(&TNC->BPQtoWINMOR_Q);
					txlen=buffptr[1];
					memcpy(txbuff,buffptr+2,txlen);
					bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
					ReleaseBuffer(buffptr);
				}
					
				if (errorfs.fd_count == 1)
				{
					i=wsprintf(ErrMsg, "WINMOR Data Connection lost for BPQ Port %d\r\n", port);
					WritetoConsole(ErrMsg);
					TNC->CONNECTING = FALSE;
					TNC->CONNECTED = FALSE;
				}
			}
		
		// See if any frames for this port

		if (TNC->WINMORtoBPQ_Q !=0)
		{
			buffptr=Q_REM(&TNC->WINMORtoBPQ_Q);

			datalen=buffptr[1];

			buff[7] = 0xf0;
			memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
			datalen+=8;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);

		}

		return (0);



	case 2:				// send

		if (!TNC->CONNECTED)
		{
			// Send Error Response

			UINT * buffptr = Q_REM(&FREE_Q);

			if (buffptr == 0) return (0);			// No buffers, so ignore

			buffptr[1]=36;
			memcpy(buffptr+2,"No Connection to WINMOR Virtual TNC\r", 36);

			Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		if (TNC->BPQtoWINMOR_Q) return 0;		// Socket is blocked - just drop packets
														// till it clears
		txlen=(buff[6]<<8) + buff[5]-8;	
		
		if (TNC->Connected)
			bytes=send(TNC->WINMORDataSock,(const char FAR *)&buff[8],txlen,0);
		else
			bytes=send(TNC->WINMORSock,(const char FAR *)&buff[8],txlen,0);
		
		if (bytes != txlen)
		{

			// WINMOR doesn't seem to recover from a blocked write. For now just reset
			
//			if (bytes == SOCKET_ERROR)
//			{
				winerr=WSAGetLastError();
				
				i=wsprintf(ErrMsg, "WINMOR Write Failed for port %d - error code = %d\r\n", port, winerr);
				WritetoConsole(ErrMsg);
					
	
//				if (winerr != WSAEWOULDBLOCK)
//				{
				closesocket(TNC->WINMORSock);
					
					TNC->CONNECTED = FALSE;

					return (0);
//				}
//				else
//				{
//					bytes=0;		// resent whole packet
//				}

//			}

			// Partial Send or WSAEWOULDBLOCK. Save data, and send once busy clears

			
			// Get a buffer
						
//			buffptr=Q_REM(&FREE_Q);

//			if (buffptr == 0)
//			{
				// No buffers, so can only break connection and try again

//				closesocket(WINMORSock[MasterPort[port]]);
					
//				CONNECTED[MasterPort[port]]=FALSE;

//				return (0);
//			}
	
//			buffptr[1]=txlen-bytes;			// Bytes still to send

//			memcpy(buffptr+2,&txbuff[bytes],txlen-bytes);

//			Q_ADD(&BPQtoWINMOR_Q[MasterPort[port]],buffptr);
	
//			return (0);
		}


		return (0);

	


	case 3:				// CHECK IF OK TO SEND

		return (0);		// OK
			
		break;

	case 4:				// reinit

//		return(ReadConfigFile("BPQAXIP.CFG"));

		return (0);
	}
	return 0;
}


DllExport int APIENTRY ExtInit(EXTPORTDATA *  PortEntry)
{
	int i, port;
	char Msg[255];

	TNCINFO * TNC;

	//
	//	Will be called once for each WINMOR port 
	//
	//	The Socket to connect to is in IOBASE
	//
	
	port=PortEntry->PORTCONTROL.PORTNUMBER;
	TNC = &TNCInfo[port];

	TNC->PortRecord = PortEntry;

	if (TNC->destaddr.sin_family == 0)
	{
		// not defined in config file, so use localhost and port from IOBASE

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE+1);

		TNC->WINMORHostName=malloc(10);

		if (TNC->WINMORHostName != NULL) 
			strcpy(TNC->WINMORHostName,"127.0.0.1");

	}

	i=wsprintf(Msg,"WINMOR Host %s %d", TNC->WINMORHostName, htons(TNC->destaddr.sin_port));
	WritetoConsole(Msg);

	ConnecttoWINMOR(port);

	time(&TNC->lasttime);			// Get initial time value

	return ((int) ExtProc);
}

BOOL InitWS2(void)
{
    int           Error;              // catches return value of WSAStartup
    WORD          VersionRequested;   // passed to WSAStartup
    WSADATA       WsaData;            // receives data from WSAStartup
    BOOL          ReturnValue = TRUE; // return value

    // Start WinSock 2.  If it fails, we don't need to call
    // WSACleanup().

    VersionRequested = MAKEWORD(VERSION_MAJOR, VERSION_MINOR);
    Error = WSAStartup(VersionRequested, &WsaData);
    if (Error) {
        MessageBox(NULL,
            "Could not find high enough version of WinSock",
            "BPQtoWINMOR", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        ReturnValue = FALSE;
    } else {

        // Now confirm that the WinSock 2 DLL supports the exact version
        // we want. If not, make sure to call WSACleanup().
 
		if (LOBYTE(WsaData.wVersion) != VERSION_MAJOR ||
            HIBYTE(WsaData.wVersion) != VERSION_MINOR) {
            MessageBox(NULL,
                "Could not find the correct version of WinSock",
                "BPQtoWINMOR",  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            WSACleanup();
            ReturnValue = FALSE;
        }
    }

	return(ReturnValue);

} // InitWS2()


InitWINMOR()
{
	u_long param=1;
	BOOL bcopt=TRUE;
	int i;

	ReadConfigFile("BPQtoWINMOR.CFG");

	// Build buffer pool
	
	for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
	{	
		ReleaseBuffer(&BufferPool[100*i]);
	}
	 
	//
	//	Open MH window if needed
	
	//if (MHEnabled)
	//	CreateMHWindow();
	
	return (TRUE);		
}

/*

#	Config file for BPQtoWINMOR
#
#	For each WINMOR port defined in BPQCFG.TXT, Add a line here
#	Format is BPQ Port, Host/IP Address, Port

#
#	Any unspecified Ports will use 127.0.0.1 and port for BPQCFG.TXT IOADDR field
#

1 127.0.0.1 8000
2 127.0.0.1 8001

*/

FILE *file;

BOOL ReadConfigFile(char * fn)
{
	char buf[256],errbuf[256];

	UCHAR Value[100];
	UCHAR * BPQDirectory;

	BPQDirectory=GetBPQDirectory();

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,fn);
	}
		
	if ((file = fopen(Value,"r")) == NULL)
	{
		wsprintf(buf,"BPQtoWINMOR - Config file %s not found ",Value);
		WritetoConsole(buf);
		return (TRUE);			// Dont need it at the moment
	}

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf))
		{
			WritetoConsole("BPQtoWINMOR - Bad config record ");
			WritetoConsole(errbuf);
		}			
	}

	fclose(file);
	return (TRUE);
}
GetLine(char * buf)
{
loop:
	if (fgets(buf, 255, file) == NULL)
		return 0;

	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}

ProcessLine(char * buf)
{
	char * ptr,* p_cmd;
	char * p_ipad;
	char * p_port;
//	unsigned long ipad;
	unsigned short WINMORport;
	int BPQport;
	int len=510;
	TNCINFO * TNC;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	BPQport=0;

	BPQport = atoi(ptr);

	if(BPQport > 0 && BPQport <17)
	{
		TNC = &TNCInfo[BPQport];

		p_ipad = strtok(NULL, " \t\n\r");
		
		if (p_ipad == NULL) return (FALSE);
	
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		WINMORport = atoi(p_port);

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(WINMORport);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(WINMORport+1);

		TNC->WINMORHostName = malloc(strlen(p_ipad)+1);

		if (TNC->WINMORHostName == NULL) return TRUE;

		strcpy(TNC->WINMORHostName,p_ipad);

		p_cmd = strtok(NULL, " \t\n\r");
			
		if (p_cmd != NULL)
		{
			TNC->ApplCmd=_strdup(p_cmd);
		}
/*
			
			
			return (TRUE);

		p_password = strtok(NULL, " \t\n\r");
			
		if (p_password == NULL) return (TRUE);

		// Allocate buffer for signon message

		TNC->WINMORSignon=malloc(546);

		if (TNC->WINMORSignon == NULL) return TRUE;

		memset(TNC->WINMORSignon,0,546);

		TNC->WINMORSignon[4]='P';

		memcpy(&WINMORSignon[BPQport][28],&len,4);

		strcpy(&WINMORSignon[BPQport][36],p_user);

		strcpy(&WINMORSignon[BPQport][291],p_password);
*/

		// Read Initialisation lines

		TNC->InitScript = malloc(1000);

		TNC->InitScript[0] = 0;

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;
				
			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

			strcat (TNC->InitScript, buf);
		}
	}

	return (FALSE);
	
}
	
int ConnecttoWINMOR(int port)
{
	_beginthread(ConnecttoWINMORThread,0,port);

	return 0;
}

VOID ConnecttoWINMORThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	TNCINFO * TNC = &TNCInfo[port];

	Sleep(1000);		// Allow init to complete 

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname (WINMORHostName[port]);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);

	}

	closesocket(TNC->WINMORSock);
	closesocket(TNC->WINMORDataSock);

	TNC->WINMORSock=socket(AF_INET,SOCK_STREAM,0);
	TNC->WINMORDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORSock == INVALID_SOCKET || TNC->WINMORDataSock == INVALID_SOCKET)
	{
		i=wsprintf(Msg, "Socket Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}
 
	setsockopt (TNC->WINMORDataSock,SOL_SOCKET,SO_REUSEADDR,(const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(TNC->WINMORSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}
	if (bind(TNC->WINMORDataSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for WINMOR Data socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	TNC->CONNECTING = TRUE;

	if (connect(TNC->WINMORSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		TNC->CONNECTED=TRUE;
	}
	else
	{
		if (Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=wsprintf(Msg, "Connect Failed for WINMOR socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		return;
	}

	if (connect(TNC->WINMORDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
	{
		TNC->DATACONNECTED=TRUE;
		ioctlsocket (TNC->WINMORSock,FIONBIO,&param);
		ioctlsocket (TNC->WINMORDataSock,FIONBIO,&param);
		TNC->CONNECTING = FALSE;

		// Send INIT script

		send(TNC->WINMORSock, TNC->InitScript , strlen(TNC->InitScript), 0);
		Alerted = TRUE;
		return;
	}

 	i=wsprintf(Msg, "Connect Failed for WINMOR Data socket - error code = %d\r\n", WSAGetLastError());
	WritetoConsole(Msg);
	closesocket(TNC->WINMORSock);
	closesocket(TNC->WINMORDataSock);

	return;
}

VOID ProcessResponse(TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
{
	// Response on WINMOR control channel. Could be a reply to a command, or
	// an Async  Response

	UINT * buffptr;

	Buffer[MsgLen] = 0;
	
	if (_memicmp(Buffer, "CONNECTED", 9) == 0)
	{
		if (TNC->PortRecord->ATTACHEDSESSION == 0)
		{
			// Incomming Connect

			struct TRANSPORTENTRY * Session;
			int Index = 0;

			Session=L4TABLE;

			// Find a free Circuit Entry

			while (Index < MAXCIRCUITS)
			{
				if (Session->L4USER[0] == 0)
					break;

				Session++;
				Index++;
			}

			if (Index == MAXCIRCUITS)
				return;					// Tables Full

			Buffer[MsgLen-1] = 0;

			ConvToAX25(&Buffer[10], Session->L4USER);
			ConvToAX25(GetNodeCall(), Session->L4MYCALL);

			Session->CIRCUITINDEX = Index;
			Session->CIRCUITID = NEXTID;
			NEXTID++;
			if (NEXTID == 0) NEXTID++;		// Keep non-zero

			TNC->PortRecord->ATTACHEDSESSION = Session;

			Session->L4TARGET = TNC->PortRecord;
	
			Session->L4CIRCUITTYPE = UPLINK+PACTOR;
			Session->L4WINDOW = L4DEFAULTWINDOW;
			Session->L4STATE = 5;
			Session->SESSIONT1 = L4T1;

			TNC->Connected = TRUE;			// Subsequent data to data channel

			// If an autoconnect APPL is defined, send it

			if (TNC->ApplCmd)
			{
				MsgLen = wsprintf(Buffer, "%s\r", TNC->ApplCmd);
				buffptr = Q_REM(&FREE_Q);

				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr[1] = MsgLen;
				memcpy(buffptr+2, Buffer, MsgLen);

				Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			}
			return;
		}
		else
		{
			// Connect Complete

			char Reply[80];
			int ReplyLen;
			
			buffptr = Q_REM(&FREE_Q);

			if (buffptr == 0) return;			// No buffers, so ignore

			ReplyLen = wsprintf(Reply, "*** Connected to %s", &Buffer[10]);

			buffptr[1] = ReplyLen;
			memcpy(buffptr+2, Reply, ReplyLen);

			Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			TNC->Connected = TRUE;			// Subsequent data to data channel

			return;
		}
	}

	if (_memicmp(Buffer, "DISCONNECTED", 12) == 0)
	{
		// Release Session

		TNC->Connected = FALSE;		// Back to Command Mode
		TNC->ReportDISC = TRUE;		// Tell Node

		return;
	}

	if (_memicmp(Buffer, "PENDING", 6) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "RRADIO", 16) == 0)
	{
		// Response to Radio Control Command
		return;
	}

	if (_memicmp(Buffer, "FAULT", 5) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		return;
	}

	// Others should be responses to commands

	buffptr = Q_REM(&FREE_Q);

	if (buffptr == 0) return;			// No buffers, so ignore

	buffptr[1] = wsprintf(buffptr+2,"Winmor} %s", Buffer);

	Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
}

int ProcessReceivedData(int port)
{
	char ErrMsg[255];
	struct TNCINFO_S * TNC = &TNCInfo[port];

	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[2000];

	// May have several messages per packet, or message split over packets

	if (TNC->InputLen > 100)	// Shouldnt have lines longer  than this on command connection
		TNC->InputLen=0;
				
	InputLen=recv(TNC->WINMORSock, &TNC->TCPBuffer[TNC->InputLen], 100 - TNC->InputLen, 0);

	if (InputLen == 0)
	{
		// Does this mean closed?
		
		if (!TNC->CONNECTING)
		{
			wsprintf(ErrMsg, "WINMOR Connection lost for BPQ Port %d\r\n", port);
			WritetoConsole(ErrMsg);
		}
		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;

		return 0;					
	}

	TNC->InputLen += InputLen;

loop:
	
	ptr = memchr(TNC->TCPBuffer, '\r', TNC->InputLen);

	if (ptr)	//  CR in buffer
	{
		ptr2 = &TNC->TCPBuffer[TNC->InputLen];
		ptr++;				// Assume LF Follows CR

		if (ptr == ptr2)
		{
			// Usual Case - single meg in buffer
	
			ProcessResponse(TNC, TNC->TCPBuffer, TNC->InputLen);
			TNC->InputLen=0;
		
		}
		else
		{
			// buffer contains more that 1 message

			MsgLen = TNC->InputLen - (ptr2-ptr);

			memcpy(Buffer, TNC->TCPBuffer, MsgLen);

			ProcessResponse(TNC, Buffer, MsgLen);

			memmove(TNC->TCPBuffer, ptr, TNC->InputLen-MsgLen);

			TNC->InputLen -= MsgLen;
			goto loop;
		}
	}
	return 0;
}


VOID ProcessDataSocketData(int port)
{
	// Info on Data Socket - just packetize and send on
	
	struct TNCINFO_S * TNC = &TNCInfo[port];

	int InputLen, PacLen = 236;
	char ErrMsg[80];

	UINT * buffptr;
loop:
	buffptr = Q_REM(&FREE_Q);

	if (buffptr == NULL) return;			// No buffers, so ignore
			
	InputLen=recv(TNC->WINMORDataSock, (char *)&buffptr[2], PacLen, 0);

	if (InputLen == -1)
	{
		ReleaseBuffer(buffptr);
		return;
	}

	if (InputLen == 0)
	{
		// Does this mean closed?
		
		wsprintf(ErrMsg, "WINMOR Connection lost for BPQ Port %d\r\n", port);
		WritetoConsole(ErrMsg);

		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;

		return;					
	}

	buffptr[1] = InputLen;
	Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

	goto loop;

}


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


