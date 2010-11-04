//
//	Telnet Driver for BPQ Switch 
//
//	Uses BPQ EXTERNAL interface
//


//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"

#include <winioctl.h>
#include "kernelresource.h"

#define IDM_DISCONNECT			2000
#define IDM_LOGGING				2100


#define MAXBLOCK 4096

#include "TNCINFO.h"

#include "ASMStrucs.h"

#include "TelnetServer.h"
#include "bpq32.h"

#define MAX_PENDING_CONNECTS 4

static char ClassName[]="TELNETSERVER";
static char WindowTitle[] = "Telnet Server";
static int RigControlRow = 190;

static BOOL OpenSockets(struct TNCINFO * TNC);

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;
extern char * PortConfig[33];
extern UCHAR BPQDirectory[];

static RECT Rect;

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd


#define MaxSockets 26

struct UserRec RelayUser;

BOOL cfgMinToTray;

char AttemptsMsg[] = "Too many attempts - Disconnected\r\n";
char disMsg[] = "Disconnected by SYSOP\r\n";


char BlankCall[]="         ";

BOOL LogEnabled=FALSE;


VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;

extern struct BPQVECSTRUC * BPQHOSTVECPTR;

static int ProcessLine(char * buf, int Port);
VOID __cdecl Debugprintf(const char * format, ...);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

LRESULT CALLBACK TelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int DisplaySessions(struct TNCINFO * TNC);
int DoStateChange(Stream);
int DoReceivedData(Stream);
int DoMonitorData(Stream);
int Connected(Stream);
int Disconnected(Stream);
int DeleteConnection(con);
static int Socket_Accept(struct TNCINFO * TNC, int SocketId);
static int Socket_Data(struct TNCINFO * TNC, int SocketId,int error, int eventcode);
static int DataSocket_Read(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_ReadFBB(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_ReadRelay(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_Write(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_Disconnect(struct TNCINFO * TNC, struct ConnectionInfo * sockptr);
BOOL ProcessTelnetCommand(struct ConnectionInfo * sockptr, byte * Msg, int Len);
int ShowConnections(struct TNCINFO * TNC);
int Terminate();
int SendtoSocket(SOCKET sock,char * Msg);
int WriteLog(char * msg);
byte * EncodeCall(byte * Call);

BOOL ProcessConfig();
VOID FreeConfig();

ProcessLine(char * buf, int Port)
{
	UCHAR * ptr;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int len=510;
	char errbuf[256];
	char * param;
	char * value;
	char * Context, *User, *Pwd, *UserCall;
	UINT i;
	int End = strlen(buf) -1;
	struct TNCINFO * TNC;
	struct TCPINFO * TCP;

	strcpy(errbuf,buf);			// save in case of error

	if (buf[End] == 10)
		buf[End]=0;			// remove newline

	if(buf[0] =='#') return (TRUE);			// comment

	if(buf[0] ==';') return (TRUE);			// comment

	ptr=strchr(buf,'=');

	if (!ptr)
		return 0;

	if (TNCInfo[Port] == NULL)
	{

		TNC = TNCInfo[Port] = zalloc(sizeof(struct TNCINFO));
		TCP = TNC->TCPInfo = zalloc(sizeof (struct TCPINFO)); // Telnet Server Specific Data

		TCP->MaxSessions = 10;				// Default Values

	}

	TNC = TNCInfo[Port];
	TCP = TNC->TCPInfo;

	param=buf;
	*(ptr)=0;
	value=ptr+1;

	if (_stricmp(param,"LOGGING") == 0)
		LogEnabled = atoi(value);
	else
	if (_stricmp(param,"DisconnectOnClose") == 0)
		TCP->DisconnectOnClose = atoi(value);
	else
		if (_stricmp(param,"TCPPORT") == 0)
			TCP->TCPPort = atoi(value);
		else
		if (_stricmp(param,"FBBPORT") == 0)
			TCP->FBBPort = atoi(value);
		else
		if (_stricmp(param,"RELAYAPPL") == 0)
		{
			TCP->RelayPort = 8772;
			strcpy(TCP->RelayAPPL, value);
			strcat(TCP->RelayAPPL, "\r");
		}
		else
		//		if (strcmp(param,"LOGINRESPONSE") == 0) cfgLOGINRESPONSE = value;
//	    if (strcmp(param,"PASSWORDRESPONSE") == 0) cfgPASSWORDRESPONSE = value;

	    if (_stricmp(param,"LOGINPROMPT") == 0)
			strcpy(TCP->LoginMsg,value);
		else
	    if (_stricmp(param,"PASSWORDPROMPT") == 0)
			strcpy(TCP->PasswordMsg, value);
		else
	    if (_stricmp(param,"HOSTPROMPT") == 0)
			strcpy(TCP->cfgHOSTPROMPT, value);
		else
		if (_stricmp(param,"LOCALECHO") == 0)
			strcpy(TCP->cfgLOCALECHO, value);
		else
		if (_stricmp(param,"MAXSESSIONS") == 0)
		{
			TCP->MaxSessions = atoi(value);
			if (TCP->MaxSessions > MaxSockets ) TCP->MaxSessions = MaxSockets;
		}
		else
		if (_stricmp(param,"CTEXT") == 0 )
		{
			int len = strlen (value);

			if (value[len -1] == ' ')
				value[len -1] = 0;

			strcpy(TCP->cfgCTEXT, value);

			// Replace \n Signon string with cr lf

			ptr = &TCP->cfgCTEXT[0];

		scanCTEXT:

			ptr = strstr(ptr, "\\n");
    
			if (ptr)
			{    
				*(ptr++)=13;			// put in cr
				*(ptr++)=10;			// put in lf

				goto scanCTEXT;
			}  
		}
		else
		if (_stricmp(param,"USER") == 0)
		{
			struct UserRec * USER;

			User = strtok_s(value, ", ", &Context);
			Pwd = strtok_s(NULL, ", ", &Context);
			UserCall = strtok_s(NULL, ", ", &Context);

			if (User == 0 || Pwd == 0)
				// invalid record
				return 0;

			// Callsign may be missing
			
			if (UserCall) 
			{							
				if (UserCall[0] == 0)
					UserCall=&BlankCall[0];
				else
					for (i=0; i<strlen(UserCall);i++)
						UserCall[i]=toupper(UserCall[i]);
			}
			else
				UserCall=&BlankCall[0];

			if (TCP->NumberofUsers == 0)
				TCP->UserRecPtr = malloc(4);
			else
				TCP->UserRecPtr = realloc(TCP->UserRecPtr, (TCP->NumberofUsers+1)*4);

			USER = malloc(12);

			TCP->UserRecPtr[TCP->NumberofUsers] = USER;
 
			USER->Callsign=malloc(strlen(UserCall)+1);
			USER->Password=malloc(strlen(Pwd)+1);
			USER->UserName=malloc(strlen(User)+1);

			strcpy(USER->UserName,User);
			strcpy(USER->Password,Pwd);
			strcpy(USER->Callsign,UserCall);

			TCP->NumberofUsers += 1;
		}

	return TRUE;
}

extern BPQTRACE();

static int MaxStreams = 26;

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct TNCINFO * TNC);
OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed);
VOID TelnetPoll(int Port);
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);

VOID ProcessPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKHOSTPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKNormCommand(struct TNCINFO * TNC, UCHAR * rxbuffer);
VOID ProcessHostFrame(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len);

//	Note that Kantronics host Mode uses KISS format Packets (without a KISS COntrol Byte)

VOID EncodeAndSend(struct TNCINFO * TNC, UCHAR * txbuffer, int Len);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, UCHAR * outbuff, int len);

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int txlen = 0, n;
	UINT * buffptr;
	struct TNCINFO * TNC = TNCInfo[port];
	int Stream;
	struct ConnectionInfo * sockptr;

	switch (fn)
	{
	case 1:				// poll

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].ReportDISC)
			{
				TNC->Streams[Stream].ReportDISC = FALSE;
				buff[4] = Stream;

				return -1;
			}
		}

		TelnetPoll(port);

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].PACTORtoBPQ_Q !=0)
			{
				int datalen;
			
				buffptr=Q_REM(&TNC->Streams[Stream].PACTORtoBPQ_Q);

				datalen=buffptr[1];

				buff[4] = Stream;
				buff[7] = 0xf0;
				memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
				datalen+=8;
				buff[5]=(datalen & 0xff);
				buff[6]=(datalen >> 8);
		
				ReleaseBuffer(buffptr);
	
				return (1);
			}
		}
			
		return 0;

	case 2:				// send

		buffptr = GetBuff();

		if (buffptr == 0) return (0);			// No buffers, so ignore

		// Find TNC Record

		Stream = buff[4];
		
		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);

		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding
		
		_asm 
		{
			MOV	EAX,buff
			mov Stream,eax
		}

		return 256;		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

#define SD_BOTH         0x02

		for (n = 1; n <= TNC->TCPInfo->CurrentSockets; n++)
		{
			sockptr = TNC->Streams[n].ConnectionInfo;
			closesocket(sockptr->socket);
		}
	
		shutdown(TNC->TCPInfo->sock, SD_BOTH);
		shutdown(TNC->TCPInfo->FBBsock, SD_BOTH);
		shutdown(TNC->TCPInfo->Relaysock, SD_BOTH);
		Sleep(500);
		closesocket(TNC->TCPInfo->sock);
		closesocket(TNC->TCPInfo->FBBsock);
		closesocket(TNC->TCPInfo->Relaysock);

		SaveWindowPos(port);
		
		return (0);

	case 6:				// Scan Control

		return 0;		// None Yet

	}
	return 0;

}

UINT WINAPI TelnetExtInit(EXTPORTDATA * PortEntry)
{
	char msg[500];
	struct TNCINFO * TNC;
	struct TCPINFO * TCP;
	int port;
	char * ptr;
	int i;
	HMENU hMenu;		// handle of menu 


	//
	//	Will be called once for each Pactor Port
	//	The COM port number is in IOBASE
	//

	wsprintf(msg,"Telnet Server");
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile("BPQTelnetServer.cfg", port, ProcessLine);

	TNC = TNCInfo[port];


	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(msg," ** Error - no info in BPQTelnetServer.cfg for this port");
		WritetoConsole(msg);

		return (int)ExtProc;
	}

	TCP = TNC->TCPInfo;

	TNC->Port = port;

	TNC->Hardware = H_TELNET;

	PortEntry->MAXHOSTMODESESSIONS = TNC->TCPInfo->MaxSessions + 1;		// Default

	// Malloc TCP Session Stucts

	for (i = 0; i <= TNC->TCPInfo->MaxSessions; i++)
	{
		TNC->Streams[i].ConnectionInfo = zalloc(sizeof(struct ConnectionInfo));
	}

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	PortEntry->PORTCONTROL.PROTOCOL = 10;	// WINMOR/Pactor
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->SCANCAPABILITIES = NONE;		// No Scan Control 

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, TelWndProc, MAKEINTRESOURCE(IDC_TELNETSERVER));

	hMenu=GetMenu(TNC->hDlg);
	TCP->hActionMenu=GetSubMenu(hMenu,0);

	TCP->hLogMenu=GetSubMenu(TCP->hActionMenu,0);

	TCP->hDisMenu=GetSubMenu(TCP->hActionMenu,1);

	CheckMenuItem(TCP->hLogMenu,0,MF_BYPOSITION | LogEnabled<<3);
	
	OpenSockets(TNC);

	TNC->RIG = &TNC->DummyRig;			// Not using Rig control, so use Dummy

	return ((int)ExtProc);
}

BOOL OpenSockets(struct TNCINFO * TNC)
{
	SOCKADDR_IN local_sin;  /* Local socket - internet style */
	PSOCKADDR_IN psin;
	int status;
	SOCKET sock;
	SOCKET FBBsock;
	SOCKET Relaysock;
	char szBuff[80];
	struct TCPINFO * TCP = TNC->TCPInfo;

	TCP->sock = sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
	{
        wsprintf(szBuff, "socket() failed error %d", WSAGetLastError());
		WritetoConsole(szBuff);
		return FALSE;  
	}
 
	psin=&local_sin;

	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;
    psin->sin_port = htons(TCP->TCPPort);        // Convert to network ordering 

 
    if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
	{
		wsprintf(szBuff, "bind(sock) failed Error %d", WSAGetLastError());
		WritetoConsole(szBuff);
        closesocket( sock );

		 return FALSE;
	}

    if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
	{
		wsprintf(szBuff, "listen(sock) failed Error %d", WSAGetLastError());

		WritetoConsole(szBuff);

		return FALSE;
	}
   
	if ((status = WSAAsyncSelect(sock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
	{
		wsprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());

		WritetoConsole(szBuff);

		closesocket(sock);
		
		return FALSE;

	}

	if (TCP->FBBPort)
	{
		if (TCP->FBBPort == TCP->TCPPort)
		{
			TCP->FBBsock = FBBsock = sock;
		}
		else
		{
			TCP->FBBsock = FBBsock = socket( AF_INET, SOCK_STREAM, 0);

			if (FBBsock == INVALID_SOCKET)
			{
				wsprintf(szBuff, "socket() failed error %d", WSAGetLastError());
				WritetoConsole(szBuff);
				return FALSE;
			}
 
			psin=&local_sin;
			psin->sin_port = htons(TCP->FBBPort);        // Convert to network ordering 

 
			if (bind(FBBsock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
			{
				 wsprintf(szBuff, "bind(FBBsock) failed Error %d", WSAGetLastError());
				 WritetoConsole(szBuff);
				closesocket( FBBsock );

				return FALSE;
			}

			if (listen(FBBsock, MAX_PENDING_CONNECTS ) < 0)
			{
				wsprintf(szBuff, "listen(FBBsock) failed Error %d", WSAGetLastError());
				WritetoConsole(szBuff);

				return FALSE;
			}
   
			if ((status = WSAAsyncSelect(FBBsock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
			{
				wsprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());
				WritetoConsole(szBuff);
				closesocket( FBBsock );
		
				return FALSE;
			}
		}
	}
	if (TCP->RelayPort)
	{
		RelayUser.UserName = _strdup("RMSRELAY");
		TCP->Relaysock = Relaysock = socket( AF_INET, SOCK_STREAM, 0);

		if (Relaysock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "socket() failed error %d", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
	}
 
		psin=&local_sin;
		psin->sin_port = htons(TCP->RelayPort);        // Convert to network ordering 

	    if (bind( Relaysock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, "bind(Relaysock) failed Error %d", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket( Relaysock );

			return FALSE;
		}

		if (listen( Relaysock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(Relaysock) failed Error %d", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect( Relaysock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(Relaysock);
		
			return FALSE;
		}
	}
	return TRUE;
}


VOID TelnetPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	struct TCPINFO * TCP = TNC->TCPInfo;

	int Stream;
	
	for (Stream = 0; Stream <= TCP->MaxSessions; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0 && TNC->Streams[Stream].Attached)
		{
			// Node has disconnected - clear any connection

			struct ConnectionInfo * sockptr = TNC->Streams[Stream].ConnectionInfo;
			SOCKET sock = sockptr->socket;
			char Msg[80];	
			UINT * buffptr;

			TNC->Streams[Stream].Attached = FALSE;
			TNC->Streams[Stream].Connected = FALSE;

            wsprintf(Msg,"*** Disconnected from Stream %d\r\n",Stream);

			send(sock, Msg, strlen(Msg),0);

			if (TCP->DisconnectOnClose)
			{
				Sleep(1000);
				DataSocket_Disconnect(TNC, sockptr);
			}
	
			while(TNC->Streams[Stream].BPQtoPACTOR_Q)
			{
				buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
				ReleaseBuffer(buffptr);
			}

			while(TNC->Streams[Stream].PACTORtoBPQ_Q)
			{
				buffptr=Q_REM(&TNC->Streams[Stream].PACTORtoBPQ_Q);
				ReleaseBuffer(buffptr);
			}
		}
	}

	for (Stream = 0; Stream <= TCP->MaxSessions; Stream++)
	{
		if (TNC->Streams[Stream].BPQtoPACTOR_Q)
		{
			int datalen;
			UCHAR TXMsg[1000] = "D20";
			UINT * buffptr;
			UCHAR * MsgPtr;
			SOCKET sock;
			struct ConnectionInfo * sockptr = TNC->Streams[Stream].ConnectionInfo;

			if (TNC->Streams[Stream].Connected)
			{
				buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
				datalen=buffptr[1];
				MsgPtr = (UCHAR *)&buffptr[2];

				sock = sockptr->socket;
	
				if (sockptr->FBBMode)
				{
					send(sock, MsgPtr, datalen, 0);
				}
				else
				{
					// Replace cr with crlf

					char * ptr2, * ptr = &MsgPtr[0];
					int i;
					do 
					{
						ptr2 = memchr(ptr, 13, datalen);

						if (ptr2 == 0)
						{
							//	no cr, so just send as is 

							send(sock, ptr, datalen, 0);
							i=0;
							break;
						}

						i=ptr2+1-ptr;

						send(sock,ptr,i,0);
						send(sock,"\n",1,0);

						datalen-=i;
						ptr=ptr2+1;
					}
					while (datalen>0);
				}
	
				return;
			}
			else // Not Connected
			{
				buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
				datalen=buffptr[1];
				MsgPtr = (UCHAR *)&buffptr[2];

				// Command. Do some sanity checking and look for things to process locally

				datalen--;				// Exclude CR
				MsgPtr[datalen] = 0;	// Null Terminate
				_strupr(MsgPtr);


				if (_memicmp(MsgPtr, "D", 1) == 0)
				{
					TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
					return;
				}
				
				buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Error - Invalid Command\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				return;
			}


/*
				if (MsgPtr[0] == 'C' && MsgPtr[1] == ' ' && datalen > 2)	// Connect
				{
					memcpy(TNC->Streams[Stream].RemoteCall, &MsgPtr[2], 9);
					TNC->Streams[Stream].Connecting = TRUE;

					// If Stream 0, Convert C CALL to PACTOR CALL

					if (Stream == 0)
					{
	//					TNC->HFPacket = TRUE;

						if (TNC->HFPacket)
							datalen = wsprintf(TXMsg, "C2AC %s", TNC->Streams[0].RemoteCall);
						else
							datalen = wsprintf(TXMsg, "C20PACTOR %s", TNC->Streams[0].RemoteCall);

						wsprintf(Status, "%s Connecting to %s",
							TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}
					else
						datalen = wsprintf(TXMsg, "C1%cC %s", Stream + '@', TNC->Streams[Stream].RemoteCall);

					EncodeAndSend(TNC, TXMsg, datalen);
					TNC->Timeout = 50;
					TNC->InternalCmd = 'C';			// So we dont send the reply to the user.
					ReleaseBuffer(buffptr);
					TNC->Streams[Stream].Connecting = TRUE;

					return;
				}
*/	

			}

	
	}
return;
}


LRESULT CALLBACK TelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	int i, n;
	struct TNCINFO * TNC;
	struct _EXTPORTDATA * PortRecord;
	HWND SavehDlg;

//	struct ConnectionInfo * ConnectionInfo;

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;
		
		if (TNC->hDlg == hWnd)
			break;
	}

	if (TNC == NULL)
		return (DefWindowProc(hWnd, message, wParam, lParam));

	switch (message)
	{
	case WSA_DATA: // Notification on data socket

		Socket_Data(TNC, wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(TNC, wParam);
		return 0;

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId) { 

		case SC_RESTORE:

			TNC->Minimized = FALSE;
			return (DefWindowProc(hWnd, message, wParam, lParam));

		case  SC_MINIMIZE: 

			TNC->Minimized = TRUE;
			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmId > IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			sockptr = TNC->Streams[wmId-IDM_DISCONNECT].ConnectionInfo;
		
			if (sockptr->SocketActive)
			{
				sock=sockptr->socket;

				send(sock,disMsg, strlen(disMsg),0);

				Sleep (1000);
    
				shutdown(sock,2);

				DataSocket_Disconnect(TNC, sockptr);

				TNC->Streams[wmId-IDM_DISCONNECT].ReportDISC = TRUE;		//Tell Node

				return 0;
			}
		}

		switch (wmId)
		{
		case IDM_LOGGING:

			// Toggle Logging Flag

			LogEnabled = !LogEnabled;
			CheckMenuItem(TNC->TCPInfo->hLogMenu, 0, MF_BYPOSITION | LogEnabled<<3);

			break;

		case TELNET_RECONFIG:

			ProcessConfig();
			FreeConfig();

			for (n = 1; n <= TNC->TCPInfo->CurrentSockets; n++)
			{
				sockptr = TNC->Streams[n].ConnectionInfo;
				closesocket(sockptr->socket);
			}
	
			shutdown(TNC->TCPInfo->sock, SD_BOTH);
			shutdown(TNC->TCPInfo->FBBsock, SD_BOTH);
			shutdown(TNC->TCPInfo->Relaysock, SD_BOTH);
			Sleep(500);
			closesocket(TNC->TCPInfo->sock);
			closesocket(TNC->TCPInfo->FBBsock);
			closesocket(TNC->TCPInfo->Relaysock);

			// Save info from old TNC record
			
			n = TNC->Port;
			PortRecord = TNC->PortRecord;
			SavehDlg = TNC->hDlg;

			// Free old TCP Session Stucts

			for (i = 0; i <= TNC->TCPInfo->MaxSessions; i++)
			{
				free(TNC->Streams[i].ConnectionInfo);
			}

			ReadConfigFile("", TNC->Port, ProcessLine);

			TNC = TNCInfo[n];
			TNC->Port = n;
			TNC->Hardware = H_TELNET;
			TNC->hDlg = SavehDlg;
			TNC->RIG = &TNC->DummyRig;			// Not using Rig control, so use Dummy


			// Malloc TCP Session Stucts

			for (i = 0; i <= TNC->TCPInfo->MaxSessions; i++)
			{
				TNC->Streams[i].ConnectionInfo = zalloc(sizeof(struct ConnectionInfo));
			}

			TNC->PortRecord = PortRecord;

			Sleep(500);
			OpenSockets(TNC);

			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		return (0);

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:

		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


int Socket_Accept(struct TNCINFO * TNC, int SocketId)
{
	int n,addrlen;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	char msg[10];
	char Negotiate[6]={IAC,WILL,suppressgoahead,IAC,WILL,echo};
//	char Negotiate[3]={IAC,WILL,echo};
	struct TCPINFO * TCP = TNC->TCPInfo;
	HMENU hDisMenu = TCP->hDisMenu;

//   Find a free Socket

	for (n = 1; n <= MaxSockets; n++)
	{
		sockptr = TNC->Streams[n].ConnectionInfo;
		
		if (sockptr->SocketActive == FALSE)
		{
			addrlen=sizeof(struct sockaddr);

			sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

			if (sock == INVALID_SOCKET)
			{
				Debugprintf("BPQ32 Telnet accept() failed Error %d", WSAGetLastError());
				return FALSE;
			}

			WSAAsyncSelect(sock, TNC->hDlg, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
			sockptr->socket = sock;
			sockptr->SocketActive = TRUE;
			sockptr->InputLen = 0;
			sockptr->Number = n;
			sockptr->LoginState = 0;
			sockptr->UserPointer = 0;
			sockptr->DoEcho = FALSE;

			if (SocketId == TCP->FBBsock)
				sockptr->FBBMode = TRUE;
			else
				sockptr->FBBMode = FALSE;

			if (SocketId == TCP->Relaysock)
				sockptr->RelayMode = TRUE;
			else
				sockptr->RelayMode = FALSE;
	
			if (TCP->CurrentSockets < n)
			{

				// Just Created a new one - add an item to disconnect menu
				
				TCP->CurrentSockets=n;  //Record max used to save searching all entries

				wsprintf(msg,"Port %d",n);

				if (n != 1)
					AppendMenu(hDisMenu, MF_STRING | MF_CHECKED,IDM_DISCONNECT + n ,msg);
				else
					ModifyMenu(hDisMenu,0,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + 1,msg);
			}
			else
				//	reusing an entry
				ModifyMenu(hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n," ");

			DrawMenuBar(TNC->hDlg);	
			ShowConnections(TNC);

			if (n > TCP->MaxSessions)
			{
				//	More than we want

				send(sock,"No Free Sessions\r\n", 18,0);
				Sleep (1000);
				closesocket(sock);

				return 0;
			}
			if (sockptr->RelayMode)
			{
				send(sock,"\r\rCallsign :\r", 13,0);
			}
			else
			{
				if (sockptr->FBBMode == FALSE)
				{
					send(sock, Negotiate, 6, 0);
					send(sock, TCP->LoginMsg, strlen(TCP->LoginMsg), 0);
				}
			}
			return 0;
		}
		
	//	Can only happen if MaxSessions > maxSockets, which is a config error

		
	}

	return 0;
	return 0;
}


int Socket_Data(struct TNCINFO * TNC, int sock, int error, int eventcode)
{
	int n;
	struct ConnectionInfo * sockptr;
	struct TCPINFO * TCP = TNC->TCPInfo;
	HMENU hDisMenu = TCP->hDisMenu;

	//	Find Connection Record

	for (n = 1; n <= TCP->CurrentSockets; n++)
	{
		sockptr = TNC->Streams[n].ConnectionInfo;
	
		if (sockptr->socket == sock && sockptr->SocketActive)
		{
			switch (eventcode)
			{
				case FD_READ:

					if (sockptr->FBBMode)
						return DataSocket_ReadFBB(TNC, sockptr,sock);
					else
						if (sockptr->RelayMode)
							return DataSocket_ReadRelay(TNC, sockptr,sock);
						else
							return DataSocket_Read(TNC, sockptr,sock);

				case FD_WRITE:

					return 0;

				case FD_OOB:

					return 0;

				case FD_ACCEPT:

					return 0;

				case FD_CONNECT:

					return 0;

				case FD_CLOSE:

					TNC->Streams[n].ReportDISC = TRUE;		//Tell Node
					DataSocket_Disconnect(TNC, sockptr);
					return 0;
				}
			return 0;
		}
	}

	return 0;
}

#define PACLEN 100

VOID SendtoNode(struct TNCINFO * TNC, int Stream, char * Msg, int MsgLen)
{
	UINT * buffptr = GetBuff();

	if (buffptr == NULL) return;			// No buffers, so ignore

	if (TNC->Streams[Stream].Connected == 0)
	{
		// Connection Closed - Get Another

		struct ConnectionInfo * sockptr = TNC->Streams[Stream].ConnectionInfo;

		ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);
	}
			
	buffptr[1] = MsgLen;				// Length
	
	memcpy(&buffptr[2], Msg, MsgLen);
	
	C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
}


int DataSocket_Read(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock)
{
	int len=0, maxlen, InputLen, MsgLen, i, n,charsAfter;
	char NLMsg[3]={13,10,0};
	byte * IACptr;
	byte * LFPtr;
	byte * BSptr;
	byte * MsgPtr;
	BOOL wait;
	char logmsg[120];
	struct UserRec * USER;
	struct TCPINFO * TCP = TNC->TCPInfo;

	ioctlsocket(sock,FIONREAD,&len);

	maxlen = InputBufferLen - sockptr->InputLen;
	
	if (len > maxlen) len=maxlen;

	len = recv(sock, &sockptr->InputBuffer[sockptr->InputLen], len, 0);

	if (len == SOCKET_ERROR || len ==0)
	{
		// Failed or closed - clear connection

		return 0;
	}

	// echo data just read

	if (sockptr->DoEcho && sockptr->LoginState != 1)  // Password
		send(sockptr->socket,&sockptr->InputBuffer[sockptr->InputLen],len,0);

	// look for backspaces in data just read
	
	BSptr = memchr(&sockptr->InputBuffer[sockptr->InputLen], 8, len);

    if (BSptr != 0)
	{
		// single char or BS as last is most likely, and worth treating as a special case

		charsAfter = len-(BSptr-&sockptr->InputBuffer[sockptr->InputLen])-1;

		if (charsAfter == 0)
		{
			sockptr->InputLen+=(len-1);
			if (sockptr->InputLen > 0) sockptr->InputLen--; //Remove last char
			return 0;
		}

		// more than single char. Copy stuff after bs over char before

		memmove(BSptr-1, BSptr+1, charsAfter);
	
		sockptr->InputLen+=(len-2);		// drop bs and char before

		// see if more bs chars
BSCheck:
		BSptr = memchr(&sockptr->InputBuffer[0], 8, sockptr->InputLen);

		if (BSptr == NULL)
			goto IACLoop;

		charsAfter = sockptr->InputLen-(BSptr-&sockptr->InputBuffer[0])-1;

		if (charsAfter == 0)
		{
			sockptr->InputLen--;		// Remove BS
			if (sockptr->InputLen > 0) sockptr->InputLen--; //Remove last char if not at start
			return 0;
		}

		memmove(BSptr-1, BSptr+1, charsAfter);
		sockptr->InputLen--;		// Remove BS
		if (sockptr->InputLen > 0) sockptr->InputLen--; //Remove last char if not at start

		goto BSCheck;					// may be more bs chars

	}
	else
	{
		sockptr->InputLen+=len;
	}

IACLoop:

	IACptr=memchr(sockptr->InputBuffer, IAC, sockptr->InputLen);

	if (IACptr)
	{
		wait = ProcessTelnetCommand(sockptr, IACptr, sockptr->InputLen+IACptr-&sockptr->InputBuffer[0]);

		if (wait) return 0;				// need more chars

		// If ProcessTelnet Command returns FALSE, then it has removed the IAC and its
		// params from the buffer. There may still be more to process.
		
		if (sockptr->InputLen == 0) return 0;	// Nothing Left
	
		goto IACLoop;					// There may be more
	}

	// Extract lines from input stream

	MsgPtr = &sockptr->InputBuffer[0];
	InputLen = sockptr->InputLen;

MsgLoop:

	LFPtr=memchr(MsgPtr, 10, InputLen);
	
	if (LFPtr == 0)
	{
		// Check Paclen

		if (InputLen > PACLEN)
		{
			if (sockptr->LoginState != 2)		// Normal Data
			{
				// Long message received when waiting for user or password - just ignore

				sockptr->InputLen=0;

				return 0;
			}

			// Send to Node
    
			// Line could be up to 500 chars if coming from a program rather than an interative user
			// Limit send to node to 255

			if (InputLen > 255)
			{		
				SendtoNode(TNC, sockptr->Number, MsgPtr, 255);
				sockptr->InputLen -= 255;
				InputLen -= 255;

				memmove(MsgPtr,MsgPtr+255,InputLen);
			}
	           
			SendtoNode(TNC, sockptr->Number, MsgPtr, InputLen);

			sockptr->InputLen = 0;
		
		} // PACLEN

		return 0;	// No CR
	}
	
	// Got a LF

	// Process data up to the cr

	MsgLen=LFPtr-MsgPtr;	// Include the CR but not LF

	switch (sockptr->LoginState)
	{
	case 2:

		// Normal Data State
			
//		send(sock,"\n",1,0);

                 
	//	*(LFPtr-1)=0;

		// Line could be up to 500 chars if coming from a program rather than an interative user
		// Limit send to node to 255. Should really use PACLEN instead of 255....

		if (MsgLen > 255)
		{		
			SendtoNode(TNC, sockptr->Number, MsgPtr, 255);
			SendtoNode(TNC, sockptr->Number, MsgPtr + 255, MsgLen - 255);
		}
		else
			SendtoNode(TNC, sockptr->Number, MsgPtr, MsgLen);

		// If anything left, copy down buffer, and go back

		InputLen=InputLen-MsgLen-1;

		sockptr->InputLen=InputLen;

		if (InputLen > 0)
		{
			memmove(MsgPtr,LFPtr+1,InputLen);

			goto MsgLoop;
		}

		return 0;

	case 0:
		
        //   Check Username
        //

		*(LFPtr-1)=0;				 // remove cr
        
  //      send(sock, NLMsg, 2, 0);

        if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d User=%s\n",
				sockptr->Number,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
				MsgPtr);

			WriteLog (logmsg);
		}

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (strcmp(MsgPtr,USER->UserName) == 0)
			{
                sockptr->UserPointer = USER;      //' Save pointer for checking password
                strcpy(sockptr->Callsign, USER->Callsign); //' for *** linked
                
                send(sock, TCP->PasswordMsg, strlen(TCP->PasswordMsg),0);
                
                sockptr->Retries = 0;
                
                sockptr->LoginState = 1;
                sockptr->InputLen = 0;

				n=sockptr->Number;

				ModifyMenu(TCP->hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n,MsgPtr);

				ShowConnections(TNC);;

                return 0;
			}
		}
        
        //   Not found
        
        
        if (sockptr->Retries++ == 4)
		{
            send(sock,AttemptsMsg,sizeof(AttemptsMsg),0);
            Sleep (1000);
            closesocket(sock);
            DataSocket_Disconnect(TNC, sockptr);       //' Tidy up
		}
		else
		{        
            send(sock, TCP->LoginMsg, strlen(TCP->LoginMsg), 0);
            sockptr->InputLen=0;

		}

		return 0;
       
	case 1:
		   
		*(LFPtr-1)=0;				 // remove cr
        
        send(sock, NLMsg, 2, 0);	// Need to echo NL, as password is not echoed
    
        if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d Password=%s\n",
				sockptr->Number,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
				MsgPtr);

			WriteLog (logmsg);
		}

		if (strcmp(MsgPtr, sockptr->UserPointer->Password) == 0)
		{
			char * ct = TCP->cfgCTEXT;

			ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);

            sockptr->LoginState = 2;
            
            sockptr->InputLen = 0;
            
            if (strlen(ct) > 0)  send(sock, ct, strlen(ct), 0);

            if (LogEnabled)
			{
				wsprintf(logmsg,"%d %d.%d.%d.%d Call Accepted BPQ Stream=%d Callsign %s\n",
					sockptr->Number,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
					sockptr->Number,
					sockptr->Callsign);

				WriteLog (logmsg);
			}

			ShowConnections(TNC);;

            return 0;
		}

		// Bad Password
        
        if (sockptr->Retries++ == 4)
		{
			send(sock,AttemptsMsg, strlen(AttemptsMsg),0);
            Sleep (1000);
            closesocket(sock);
            DataSocket_Disconnect (TNC, sockptr);      //' Tidy up
		}
		else
		{
			send(sock, TCP->PasswordMsg, strlen(TCP->PasswordMsg), 0);
			sockptr->InputLen=0;
		}

		return 0;

	default:

		return 0;

	}

	return 0;
}

int DataSocket_ReadRelay(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock)
{
	int len=0, maxlen, InputLen, MsgLen, Stream, n;
	char NLMsg[3]={13,10,0};
	byte * LFPtr;
	byte * MsgPtr;
	char logmsg[120];
	char RelayMsg[] = "No CMS connection available - using local BPQMailChat\r";
	struct TCPINFO * TCP = TNC->TCPInfo;

	ioctlsocket(sock,FIONREAD,&len);

	maxlen = InputBufferLen - sockptr->InputLen;
	
	if (len > maxlen) len=maxlen;

	len = recv(sock, &sockptr->InputBuffer[sockptr->InputLen], len, 0);

	if (len == SOCKET_ERROR || len ==0)
	{
		// Failed or closed - clear connection

		return 0;
	}

	sockptr->InputLen+=len;

	// Extract lines from input stream

	MsgPtr = &sockptr->InputBuffer[0];
	InputLen = sockptr->InputLen;

MsgLoop:

	if (sockptr->LoginState == 2)
	{
		// Data. FBB is binary

		// Send to Node

		Stream = sockptr->BPQStream;

		if (InputLen > 256)
		{		
			SendtoNode(TNC, sockptr->Number, MsgPtr, 256);
			sockptr->InputLen -= 256;

			InputLen -= 256;

			memmove(MsgPtr,MsgPtr+256,InputLen);

			goto MsgLoop;
		}
			
		SendtoNode(TNC, sockptr->Number, MsgPtr, InputLen);
		sockptr->InputLen = 0;

		return 0;
	}
	
	if (InputLen > 256)
	{
		// Long message received when waiting for user or password - just ignore

		sockptr->InputLen=0;

		return 0;
	}

	LFPtr=memchr(MsgPtr, 13, InputLen);
	
	if (LFPtr == 0)
		return 0;							// Waitr for more
	
	// Got a CR

	// Process data up to the cr

	MsgLen=LFPtr-MsgPtr;

	switch (sockptr->LoginState)
	{

	case 0:
		
        //   Check Username
        //

		*(LFPtr)=0;				 // remove cr
        
        if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d User=%s\n",
				sockptr->Number,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
				MsgPtr);

			WriteLog (logmsg);
		}

		MsgPtr++;

		// Save callsign  for *** linked
               
		strcpy(sockptr->Callsign, MsgPtr);
                                
		send(sock, "Password :\r", 11,0);
                
		sockptr->Retries = 0;
		sockptr->LoginState = 1;
        sockptr->InputLen = 0;

		n=sockptr->Number;

		ModifyMenu(TCP->hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n,MsgPtr);

		ShowConnections(TNC);;

        return 0;
	
       
	case 1:
		   
		*(LFPtr)=0;				 // remove cr
            
        if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d Password=%s\n",
				sockptr->Number,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
				MsgPtr);

			WriteLog (logmsg);
		}

		sockptr->UserPointer  = &RelayUser;

		ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);

		send(sock, RelayMsg, strlen(RelayMsg), 0);

		sockptr->LoginState = 2;

		sockptr->InputLen = 0;

		if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d Call Accepted BPQ Stream=%d Callsign %s\n",
					sockptr->Number,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
					Stream,
					sockptr->Callsign);

			WriteLog (logmsg);
		}

		ShowConnections(TNC);;
		InputLen=InputLen-(MsgLen+1);

		sockptr->InputLen=InputLen;

		// Connect to the BBS

		SendtoNode(TNC, sockptr->Number, TCP->RelayAPPL, strlen(TCP->RelayAPPL));

		ShowConnections(TNC);;
	
		return 0;
	
	default:

		return 0;

	}

	return 0;
}


int DataSocket_ReadFBB(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock)
{
	int len=0, maxlen, InputLen, MsgLen, i, n;
	char NLMsg[3]={13,10,0};
	byte * LFPtr;
	byte * MsgPtr;
	char logmsg[120];
	struct UserRec * USER;
	struct TCPINFO * TCP = TNC->TCPInfo;

	ioctlsocket(sock,FIONREAD,&len);

	maxlen = InputBufferLen - sockptr->InputLen;
	
	if (len > maxlen) len=maxlen;

	len = recv(sock, &sockptr->InputBuffer[sockptr->InputLen], len, 0);

	if (len == SOCKET_ERROR || len ==0)
	{
		// Failed or closed - clear connection

		return 0;
	}

	sockptr->InputLen+=len;

	// Extract lines from input stream

	MsgPtr = &sockptr->InputBuffer[0];
	InputLen = sockptr->InputLen;

MsgLoop:

	if (sockptr->LoginState == 2)
	{
		// Data. FBB is binary

		// Send to Node

		if (InputLen > 256)
		{		
			SendtoNode(TNC, sockptr->Number, MsgPtr, 256);
			sockptr->InputLen -= 256;

			InputLen -= 256;

			memmove(MsgPtr,MsgPtr+256,InputLen);

			goto MsgLoop;
		}
			
		SendtoNode(TNC, sockptr->Number, MsgPtr, InputLen);
		sockptr->InputLen = 0;

		return 0;
	}
	
	if (InputLen > 256)
	{
		// Long message received when waiting for user or password - just ignore

		sockptr->InputLen=0;

		return 0;
	}

	LFPtr=memchr(MsgPtr, 13, InputLen);
	
	if (LFPtr == 0)
		return 0;							// Waitr for more
	
	// Got a CR

	// Process data up to the cr

	MsgLen=LFPtr-MsgPtr;

	switch (sockptr->LoginState)
	{

	case 0:
		
        //   Check Username
        //

		*(LFPtr)=0;				 // remove cr
        
        if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d User=%s\n",
				sockptr->Number,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
				MsgPtr);

			WriteLog (logmsg);
		}
		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (strcmp(MsgPtr,USER->UserName) == 0)
			{
                sockptr->UserPointer = USER;      //' Save pointer for checking password
                strcpy(sockptr->Callsign, USER->Callsign); //' for *** linked
                                
                sockptr->Retries = 0;
                
                sockptr->LoginState = 1;
                sockptr->InputLen = 0;

				n=sockptr->Number;

				ModifyMenu(TCP->hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n,MsgPtr);

				ShowConnections(TNC);;

               	InputLen=InputLen-(MsgLen+1);

				sockptr->InputLen=InputLen;
				
				if (InputLen > 0)
				{
					memmove(MsgPtr,LFPtr+1,InputLen);

					goto MsgLoop;
				}
			}
		}
        
        //   Not found
        
        
        if (sockptr->Retries++ == 4)
		{
            send(sock,AttemptsMsg,sizeof(AttemptsMsg),0);
            Sleep (1000);
            closesocket(sock);
            DataSocket_Disconnect(TNC, sockptr);       //' Tidy up
		}
		else
		{        
            send(sock, TCP->LoginMsg, strlen(TCP->LoginMsg), 0);
            sockptr->InputLen=0;

		}

		return 0;
       
	case 1:
		   
		*(LFPtr)=0;				 // remove cr
            
        if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d Password=%s\n",
				sockptr->Number,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
				MsgPtr);

			WriteLog (logmsg);
		}
		if (strcmp(MsgPtr, sockptr->UserPointer->Password) == 0)
		{
			ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);

            sockptr->LoginState = 2;
            
            sockptr->InputLen = 0;
            
            if (LogEnabled)
			{
				wsprintf(logmsg,"%d %d.%d.%d.%d Call Accepted BPQ Stream=%d Callsign %s\n",
					sockptr->Number,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
					sockptr->Number,
					sockptr->Callsign);

				WriteLog (logmsg);
			}

			ShowConnections(TNC);;
			InputLen=InputLen-(MsgLen+1);

			sockptr->InputLen=InputLen;

			// What is left is the Command to connect to the BBS

			if (InputLen > 0)
			{
				memmove(MsgPtr,LFPtr+1,InputLen);
				MsgPtr[InputLen] = 13;
				SendtoNode(TNC, sockptr->Number, MsgPtr, InputLen+1);
				sockptr->InputLen=0;
			}
			
			return 0;
	
		}
		// Bad Password
        
        if (sockptr->Retries++ == 4)
		{
			send(sock,AttemptsMsg, strlen(AttemptsMsg),0);
            Sleep (1000);
            closesocket(sock);
            DataSocket_Disconnect(TNC, sockptr);      //' Tidy up
		}
		else
		{
			send(sock, TCP->PasswordMsg, strlen(TCP->PasswordMsg), 0);
			sockptr->InputLen=0;
		}

		return 0;

	default:

		return 0;

	}

	return 0;
}



int DataSocket_Disconnect(struct TNCINFO * TNC,  struct ConnectionInfo * sockptr)
{
	int n;

	if (sockptr->SocketActive)
	{
		closesocket(sockptr->socket);

		n=sockptr->Number;

		ModifyMenu(TNC->TCPInfo->hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n, ".");

		sockptr->SocketActive = FALSE;
	
		ShowConnections(TNC);;
	}
	return 0;
}

int ShowConnections(struct TNCINFO * TNC)
{
	char msg[80];
	struct ConnectionInfo * sockptr;
	int i,n;

	SendDlgItemMessage(TNC->hDlg,100,LB_RESETCONTENT,0,0);

	for (n = 1; n <= TNC->TCPInfo->CurrentSockets; n++)
	{
		sockptr=TNC->Streams[n].ConnectionInfo;

		if (!sockptr->SocketActive)
		{
			strcpy(msg,"Idle");
		}
		else
		{
			if (sockptr->UserPointer == 0)
				strcpy(msg,"Logging in");
			else
			{
				i=wsprintf(msg,"%-15s %-10s %2d",
					sockptr->UserPointer->UserName,sockptr->Callsign,sockptr->BPQStream);
			}
		}
		SendDlgItemMessage(TNC->hDlg, 100, LB_ADDSTRING ,0, (LPARAM)msg);
	}

	return 0;
}
byte * EncodeCall(byte * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];
}
BOOL ProcessTelnetCommand(struct ConnectionInfo * sockptr, byte * Msg, int Len)
{
	int cmd, TelOption;
	int used;
//	char WillSupGA[3]={IAC,WILL,suppressgoahead};
//	char WillEcho[3]={IAC,WILL,echo};


	//	Note Msg points to the IAC, which may not be at the start of the receive buffer
	//	Len is number of bytes left in buffer including the IAC

	if (Len < 2) return TRUE;		//' Wait for more

	cmd = Msg[1];
	
	if (cmd == DOx || cmd == DONT || cmd == WILL || cmd == WONT)
		if (Len < 3) return TRUE;		//' wait for option
    
	TelOption = Msg[2];

	switch (cmd)
	{
	case DOx:
            
        switch (TelOption)
		{
		case echo:
			sockptr->DoEcho = TRUE;
			break;

		case suppressgoahead:

// 			send(sockptr->socket,WillSupGA,3,0);
			break;

		}
		used=3;

		break;

	case DONT:
    
 //       Debug.Print "DONT"; TelOption

		switch (TelOption)
		{
		case echo:
			sockptr->DoEcho = FALSE;
			break;
		}

		used=3;
   
		break;

	case WILL:

 //       Debug.Print "WILL"; TelOption
        
        if (TelOption == echo) sockptr->DoEcho = TRUE;
        
		used=3;

		break;

	case WONT:
            
//        Debug.Print "WONT"; TelOption

		used=3;
       
		break;

	default:
    
		used=2;

	}
   
	// remove the processed command from the buffer

	memmove(Msg,&Msg[used],Len-used);

	sockptr->InputLen-=used;

	return FALSE;
}


int WriteLog(char * msg)
{
	FILE *file;
	char timebuf[128];

	time_t ltime;
    struct tm today;

	UCHAR Value[100];

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value, "BPQTelnetServer.log");
	}
	else
	{
		strcpy(Value, BPQDirectory);
		strcat(Value, "\\");
		strcat(Value, "BPQTelnetServer.log");
	}
		
	if ((file = fopen("Value", "a")) == NULL)
		return FALSE;

	time( &ltime );

	_localtime32_s( &today, &ltime );

	strftime( timebuf, 128,
		"%d/%m/%Y %H:%M:%S ", &today );
    
	fputs(timebuf, file);

	fputs(msg, file);

	fclose(file);

	return 0;
}

