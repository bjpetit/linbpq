//
//	Telnet Driver for BPQ Switch 
//
//	Uses BPQ EXTERNAL interface
//

//#define WIN32_LEAN_AND_MEAN

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "winsock2.h"
#include "WS2tcpip.h"
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
static BOOL OpenSockets6(struct TNCINFO * TNC);
ProcessHTTPMessage(struct ConnectionInfo * conn);
extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;
extern char * PortConfig[33];
extern UCHAR BPQDirectory[];

extern int COUNT_QUEUED_FRAMES();

extern byte	MCOM;
extern byte	MUIONLY;
extern char	MTX;
extern ULONG MMASK;

extern HMENU hMainFrameMenu;
extern HMENU hBaseMenu;
extern HMENU hWndMenu;
extern HFONT hFont;
extern HBRUSH bgBrush;

extern HWND ClientWnd, FrameWnd;

extern HANDLE hInstance;

extern struct BPQVECSTRUC TELNETMONVEC;
extern int MONDECODE();

extern HKEY REGTREE;

static RECT Rect;

extern short L4LIMIT;

extern struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

#define MaxSockets 26

struct UserRec RelayUser;
struct UserRec CMSUser;


char AttemptsMsg[] = "Too many attempts - Disconnected\r\n";
char disMsg[] = "Disconnected by SYSOP\r\n";

char BlankCall[]="         ";

BOOL LogEnabled = FALSE;
BOOL CMSLogEnabled = TRUE;

static	HMENU hMenu, hPopMenu, hPopMenu2, hPopMenu3;		// handle of menu 

VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);
int C_Q_COUNT(UINT *Q);

static int ProcessLine(char * buf, int Port);
VOID __cdecl Debugprintf(const char * format, ...);
char * strlop(char * buf, char delim);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, struct TNCINFO * arglist);

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
static int DataSocket_Read(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, struct STREAMINFO * STREAM);
int DataSocket_ReadFBB(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, int Stream);
int DataSocket_ReadRelay(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, struct STREAMINFO * STREAM);
int DataSocket_ReadHTTP(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, int Stream);
int DataSocket_Write(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_Disconnect(struct TNCINFO * TNC, struct ConnectionInfo * sockptr);
BOOL ProcessTelnetCommand(struct ConnectionInfo * sockptr, byte * Msg, int Len);
int ShowConnections(struct TNCINFO * TNC);
int Terminate();
int SendtoSocket(SOCKET sock,char * Msg);
int WriteLog(char * msg);
VOID WriteCMSLog(char * msg);
byte * EncodeCall(byte * Call);
VOID SendtoNode(struct TNCINFO * TNC, int Stream, char * Msg, int MsgLen);

BOOL CheckCMS(struct TNCINFO * TNC);
TCPConnect(struct TNCINFO * TNC, struct TCPINFO * TCP, struct STREAMINFO * STREAM, char * Host, int Port);
CMSConnect(struct TNCINFO * TNC, struct TCPINFO * TCP, struct STREAMINFO * STREAM,  int Stream);
int Telnet_Connected(struct TNCINFO * TNC, SOCKET sock, int Error);
BOOL ProcessConfig();
VOID FreeConfig();
VOID SaveCMSHostInfo(int port, struct TCPINFO * TCP, int CMSNo);
VOID GetCMSCachedInfo(struct TNCINFO * TNC);
BOOL CMSCheck(struct TNCINFO * TNC, struct TCPINFO * TCP);
VOID Format_Addr(struct ConnectionInfo * sockptr, char * dst);

ProcessLine(char * buf, int Port)
{
	UCHAR * ptr;
	UCHAR * ptr1;

	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int len=510;
	char errbuf[256];
	char * param;
	char * value;
	char * Context, *User, *Pwd, *UserCall, *Secure, * Appl;
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
		ptr=strchr(buf,' ');

	if (!ptr)
		return 0;

	if (TNCInfo[Port] == NULL)
	{
		TNC = TNCInfo[Port] = zalloc(sizeof(struct TNCINFO));
		TCP = TNC->TCPInfo = zalloc(sizeof (struct TCPINFO)); // Telnet Server Specific Data

		TCP->MaxSessions = 10;				// Default Values
		TNC->Hardware = H_TELNET;
		TCP->IPV4 = TRUE;
	}

	TNC = TNCInfo[Port];
	TCP = TNC->TCPInfo;

	param=buf;
	*(ptr)=0;
	value=ptr+1;

	if (_stricmp(param, "IPV4") == 0)
		TCP->IPV4 = atoi(value);
	else
	if (_stricmp(param, "IPV6") == 0)
		TCP->IPV6 = atoi(value);
	else
	if (_stricmp(param, "CMS") == 0)
		TCP->CMS = atoi(value);
	else

	if (_stricmp(param,"LOGGING") == 0)
		LogEnabled = atoi(value);
	else
	if (_stricmp(param,"CMSLOGGING") == 0)
		CMSLogEnabled = atoi(value);
	else
	if (_stricmp(param,"DisconnectOnClose") == 0)
		TCP->DisconnectOnClose = atoi(value);
	else
		if (_stricmp(param,"TCPPORT") == 0)
			TCP->TCPPort = atoi(value);
		else
		if (_stricmp(param,"HTTPPORT") == 0)
			TCP->HTTPPort = atoi(value);
		else
		if (_stricmp(param,"FBBPORT") == 0)
		{
			int n = 0;
			char * context;
			char * ptr = strtok_s(value, ", ", &context);

			while (ptr && n < 99)
			{
				TCP->FBBPort[n++] = atoi(ptr);
				ptr = strtok_s(NULL, ", ", &context);
			}
		}
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
		{
			ptr1 = strchr(value, 13);
			if (ptr1) *ptr1 = 0;
			strcpy(TCP->LoginMsg,value);
		}
		else
	    if (_stricmp(param,"PASSWORDPROMPT") == 0)
		{
			ptr1 = strchr(value, 13);
			if (ptr1) *ptr1 = 0;
			strcpy(TCP->PasswordMsg, value);
		}
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
			Appl = strtok_s(NULL, ", ", &Context);
			Secure = strtok_s(NULL, ", ", &Context);

			if (User == 0 || Pwd == 0 || UserCall == 0) // invalid record
				return 0;

			_strupr(UserCall);

			if (TCP->NumberofUsers == 0)
				TCP->UserRecPtr = malloc(4);
			else
				TCP->UserRecPtr = realloc(TCP->UserRecPtr, (TCP->NumberofUsers+1)*4);

			USER = zalloc(sizeof(struct UserRec));

			TCP->UserRecPtr[TCP->NumberofUsers] = USER;
 
			USER->Callsign=malloc(strlen(UserCall)+1);
			USER->Password=malloc(strlen(Pwd)+1);
			USER->UserName=malloc(strlen(User)+1);
			USER->Appl = zalloc(32);
			USER->Secure = FALSE;

			if (Secure)
			{
				strlop(Secure, 13);
				if (_stricmp(Secure, "SYSOP") == 0)
					USER->Secure = TRUE;
			}
			strcpy(USER->UserName, User);
			strcpy(USER->Password, Pwd);
			strcpy(USER->Callsign, UserCall);
			if (Appl && strcmp(Appl, "\"\"") != 0)
			{
				strcpy(USER->Appl, _strupr(Appl));
				strcat(USER->Appl, "\r\n");
			}
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


static VOID WritetoTrace(int Stream, char * Msg, int Len)
{
	int index = 0;
	UCHAR * ptr1 = Msg, * ptr2;
	UCHAR Line[1000];
	int LineLen, i;
	char logmsg[200];

lineloop:

	if (Len > 0)
	{
		//	copy text to file a line at a time	
					
		ptr2 = memchr(ptr1, 13 , Len);

		if (ptr2)
		{
			ptr2++;
			LineLen = ptr2 - ptr1;
			Len -= LineLen;
			memcpy(Line, ptr1, LineLen);
			memcpy(&Line[LineLen - 1], "<cr>", 4);
			LineLen += 3;

			if ((*ptr2) == 10)
			{
				memcpy(&Line[LineLen], "<lf>", 4);
				LineLen += 4;
				ptr2++;
				Len --;
			}
			
			Line[LineLen] = 0;

			// If line contains any data above 7f, assume binary and dont display

			for (i = 0; i < LineLen; i++)
			{
				if (Line[i] > 127)
					goto Skip;
			}

			if (strlen(Line) < 100)
			{
				wsprintf(logmsg,"%d %s\r\n", Stream, Line);
				WriteCMSLog (logmsg);
			}

		Skip:
			ptr1 = ptr2;
			goto lineloop;
		}

		// No CR

		for (i = 0; i < Len; i++)
		{
			if (ptr1[i] > 127)
				break;
		}

		if (i == Len)			// No binary data
		{
			if (strlen(ptr1) < 100)
			{
				wsprintf(logmsg,"%d %s\r\n", Stream, ptr1);
				WriteCMSLog (logmsg);
			}
		}
	}
}

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int txlen = 0, n;
	UINT * buffptr;
	struct TNCINFO * TNC = TNCInfo[port];
	struct TCPINFO * TCP;

	int Stream;
	struct ConnectionInfo * sockptr;
	struct STREAMINFO * STREAM;

	if (TNC == NULL)
		return 0;					// Not configured

	switch (fn)
	{
	case 1:				// poll

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{	
			struct TRANSPORTENTRY * SESS;
			struct ConnectionInfo * sockptr = TNC->Streams[Stream].ConnectionInfo;

			if (sockptr && sockptr->UserPointer == &CMSUser)	// Connected to CMS
			{
				SESS = TNC->PortRecord->ATTACHEDSESSIONS[sockptr->Number];

				if (SESS)
				{
					n = SESS->L4KILLTIMER;
					if (n < (L4LIMIT - 120))
						SESS->L4KILLTIMER = L4LIMIT - 120;
				}
			}

			STREAM = &TNC->Streams[Stream];

			if (STREAM->NeedDisc)
			{
				STREAM->NeedDisc--;

				if (STREAM->NeedDisc == 0)
				{
					// Send the DISCONNECT

					STREAM->ReportDISC = TRUE;
				}
			}

			if (STREAM->ReportDISC)
			{
				STREAM->ReportDISC = FALSE;
				buff[4] = Stream;

				return -1;
			}
		}

		TelnetPoll(port);

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			STREAM = &TNC->Streams[Stream];

			if (STREAM->PACTORtoBPQ_Q !=0)
			{
				int datalen;
			
				buffptr=Q_REM(&STREAM->PACTORtoBPQ_Q);

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
		STREAM = &TNC->Streams[Stream];

		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		C_Q_ADD(&STREAM->BPQtoPACTOR_Q, buffptr);
		STREAM->FramesQueued++;

		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding
		
		_asm 
		{
			MOV	EAX,buff
			mov Stream,eax
		}

		STREAM = &TNC->Streams[Stream];

		if (STREAM->FramesQueued  > 4)
			return (257);						// Busy

		return 256;		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

#define SD_BOTH         0x02

		TCP = TNC->TCPInfo;

		for (n = 1; n <= TCP->CurrentSockets; n++)
		{
			sockptr = TNC->Streams[n].ConnectionInfo;
			closesocket(sockptr->socket);
		}
	
		shutdown(TCP->sock, SD_BOTH);

		n = 0;
		while (TCP->FBBsock[n])
			shutdown(TCP->FBBsock[n++], SD_BOTH);

		shutdown(TCP->Relaysock, SD_BOTH);
		shutdown(TCP->HTTPsock, SD_BOTH);
		shutdown(TCP->HTTPsock6, SD_BOTH);

		shutdown(TCP->sock6, SD_BOTH);

		n = 0;
		while (TCP->FBBsock6[n])
			shutdown(TCP->FBBsock6[n++], SD_BOTH);

		shutdown(TCP->Relaysock6, SD_BOTH);

		Sleep(500);
		closesocket(TCP->sock);

		n = 0;
		while (TCP->FBBsock[n])
			closesocket(TCP->FBBsock[n++]);

		closesocket(TCP->Relaysock);

		closesocket(TCP->sock6);

		n = 0;
		while (TCP->FBBsock6[n])
			closesocket(TCP->FBBsock6[n++]);

		closesocket(TCP->Relaysock6);
		closesocket(TCP->HTTPsock);
		closesocket(TCP->HTTPsock6);

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
	HWND x;

	wsprintf(msg,"Telnet Server ");
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile(port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		WritetoConsole("\n");
		return (int)ExtProc;
	}

	TCP = TNC->TCPInfo;

	TNC->Port = port;

	TNC->Hardware = H_TELNET;

	PortEntry->MAXHOSTMODESESSIONS = TNC->TCPInfo->MaxSessions + 1;		// Default

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] != 0)
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	PortEntry->PORTCONTROL.PROTOCOL = 10;	// WINMOR/Pactor
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->SCANCAPABILITIES = NONE;		// No Scan Control 
	PortEntry->PERMITGATEWAY = TRUE;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	TELNETMONVEC.HOSTAPPLFLAGS = 0x80;		// Requext Monitoring

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, TelWndProc, 300, 300);

	TCP->hCMSWnd = CreateWindowEx(0, "STATIC", "CMS OK ", WS_CHILD | WS_VISIBLE,
			240,0,50,16, TNC->hDlg, NULL, hInstance, NULL);

	SendMessage(TCP->hCMSWnd, WM_SETFONT, (WPARAM)hFont, 0);

	x = CreateWindowEx(0, "STATIC", " User      Callsign  Stream", WS_CHILD | WS_VISIBLE,
			0,0,240,16, TNC->hDlg, NULL, hInstance, NULL);

	SendMessage(x, WM_SETFONT, (WPARAM)hFont, 0);


	TNC->hMonitor= CreateWindowEx(0, "LISTBOX", "", WS_CHILD | WS_VISIBLE  | WS_VSCROLL,
			0,20,290,2800, TNC->hDlg, NULL, hInstance, NULL);

	SendMessage(TNC->hMonitor, WM_SETFONT, (WPARAM)hFont, 0);

	hPopMenu = CreatePopupMenu();
	hPopMenu2 = CreatePopupMenu();
	hPopMenu3 = CreatePopupMenu();

	AppendMenu(hPopMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu2,"Logging Options");
	AppendMenu(hPopMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu3,"Disconnect User");
	AppendMenu(hPopMenu, MF_STRING, TELNET_RECONFIG, "ReRead Config");
	AppendMenu(hPopMenu, MF_STRING, CMSENABLED, "CMS Access Enabled");
	AppendMenu(hPopMenu, MF_STRING, USECACHEDCMS, "Using Cached CMS Addresses");

	AppendMenu(hPopMenu2, MF_STRING, IDM_LOGGING, "Log incoming connections");
	AppendMenu(hPopMenu2, MF_STRING, IDM_CMS_LOGGING, "Log CMS Connections");

	AppendMenu(hPopMenu3, MF_STRING, IDM_DISCONNECT, "1");

	TCP->hActionMenu = hPopMenu;

	CheckMenuItem(TCP->hActionMenu, 3, MF_BYPOSITION | TCP->CMS<<3);

	TCP->hLogMenu = hPopMenu2;
	TCP->hDisMenu = hPopMenu3;

	CheckMenuItem(TCP->hLogMenu, 0, MF_BYPOSITION | LogEnabled<<3);
	CheckMenuItem(TCP->hLogMenu, 1, MF_BYPOSITION | CMSLogEnabled<<3);

//	ModifyMenu(hMenu, 1, MF_BYPOSITION | MF_OWNERDRAW | MF_STRING, 10000,  0); 
 

	// Malloc TCP Session Stucts

	for (i = 0; i <= TNC->TCPInfo->MaxSessions; i++)
	{
		TNC->Streams[i].ConnectionInfo = zalloc(sizeof(struct ConnectionInfo));
		TCP->CurrentSockets = i;  //Record max used to save searching all entries

		wsprintf(msg,"%d", i);

		if (i > 1)
			AppendMenu(TCP->hDisMenu, MF_STRING, IDM_DISCONNECT ,msg);
	}

	OpenSockets(TNC);
	OpenSockets6(TNC);

	TNC->RIG = &TNC->DummyRig;			// Not using Rig control, so use Dummy

	if (TCP->CMS)
		CheckCMS(TNC);

	WritetoConsole("\n");

	ShowConnections(TNC);

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
	SOCKET HTTPsock;

	char szBuff[80];
	struct TCPINFO * TCP = TNC->TCPInfo;
	int n;

	if (!TCP->IPV4)
		return TRUE;

	psin=&local_sin;
	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;

	if (TCP->TCPPort)
	{
		TCP->sock = sock = socket(AF_INET, SOCK_STREAM, 0);

	    if (sock == INVALID_SOCKET)
		{
	        wsprintf(szBuff, "socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;  
		}
 
		psin->sin_port = htons(TCP->TCPPort);        // Convert to network ordering 

 
		if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, "bind(sock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
		    closesocket( sock );

			 return FALSE;
		}

		if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(sock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect(sock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d\n", WSAGetLastError());

			WritetoConsole(szBuff);
			closesocket(sock);
		
			return FALSE;
		}
	}

	n = 0;

	while (TCP->FBBPort[n])
	{
		TCP->FBBsock[n] = FBBsock = socket( AF_INET, SOCK_STREAM, 0);

		if (FBBsock == INVALID_SOCKET)
		{
			wsprintf(szBuff, " socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
		}
 
		psin=&local_sin;
		psin->sin_port = htons(TCP->FBBPort[n++]);        // Convert to network ordering 

		if (bind(FBBsock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, " bind(FBBsock %d) failed Error %d\n", TCP->FBBPort[n-1], WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(FBBsock);

			return FALSE;
		}

		if (listen(FBBsock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, " listen(FBBsock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect(FBBsock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, " WSAAsyncSelect failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket( FBBsock );
		
			return FALSE;
		}
	}

	if (TCP->RelayPort)
	{
		RelayUser.UserName = _strdup("RMSRELAY");
		TCP->Relaysock = Relaysock = socket( AF_INET, SOCK_STREAM, 0);

		if (Relaysock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
		}
 
		psin=&local_sin;
		psin->sin_port = htons(TCP->RelayPort);        // Convert to network ordering 

	    if (bind( Relaysock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, "bind(Relaysock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket( Relaysock );

			return FALSE;
		}

		if (listen( Relaysock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(Relaysock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect( Relaysock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(Relaysock);
		
			return FALSE;
		}
	}

	if (TCP->HTTPPort)
	{
		TCP->HTTPsock = HTTPsock = socket( AF_INET, SOCK_STREAM, 0);

		if (HTTPsock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
		}
 
		psin=&local_sin;
		psin->sin_port = htons(TCP->HTTPPort);        // Convert to network ordering 

	    if (bind(HTTPsock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, "bind(HTTP) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(HTTPsock);

			return FALSE;
		}

		if (listen(HTTPsock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(HTTP) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect(HTTPsock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(HTTPsock);
		
			return FALSE;
		}
	}

	CMSUser.UserName = _strdup("CMS");

	return TRUE;
}


BOOL OpenSockets6(struct TNCINFO * TNC)
{
	SOCKADDR_IN6 local_sin;  /* Local socket - internet style */

	PSOCKADDR_IN6 psin;
	int status;
	SOCKET sock;
	SOCKET FBBsock;
	SOCKET Relaysock;
	SOCKET HTTPsock;
	char szBuff[80];
	struct TCPINFO * TCP = TNC->TCPInfo;
	int n;

	if (!TCP->IPV6)
		return TRUE;

	memset(&local_sin, 0, sizeof(local_sin));

	psin=&local_sin;
	psin->sin6_family = AF_INET6;
	psin->sin6_addr = in6addr_any;
	psin->sin6_flowinfo = 0;
	psin->sin6_scope_id = 0;

	if (TCP->TCPPort)
	{
		TCP->sock6 = sock = socket(AF_INET6, SOCK_STREAM, 0);

	    if (sock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "IPV6 socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;  
		}

		psin->sin6_port = htons(TCP->TCPPort);        // Convert to network ordering 

		if (bind(sock, (struct sockaddr FAR *)psin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, "IPV6 bind(sock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
		    closesocket( sock );

			return FALSE;
		}

		if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "IPV6 listen(sock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect(sock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			closesocket(sock);
		
			return FALSE;
		}
	}
	n = 0;

	while (TCP->FBBPort[n])
	{
		TCP->FBBsock6[n] = FBBsock = socket( AF_INET6, SOCK_STREAM, 0);

		if (FBBsock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "IPV6 socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
		}
 
		psin=&local_sin;
		psin->sin6_port = htons(TCP->FBBPort[n++]);        // Convert to network ordering 

		if (bind(FBBsock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, " bind(IPV6 FBBsock %d) failed Error %d\n", TCP->FBBPort[n-1], WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(FBBsock);

			return FALSE;
		}

		if (listen(FBBsock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, " listen(V6 FBBsock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect(FBBsock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, " WSAAsyncSelect failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket( FBBsock );
		
			return FALSE;
		}
	}

	if (TCP->RelayPort)
	{
		RelayUser.UserName = _strdup("RMSRELAY");
		TCP->Relaysock6 = Relaysock = socket( AF_INET6, SOCK_STREAM, 0);

		if (Relaysock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
		}
 
		psin=&local_sin;
		psin->sin6_port = htons(TCP->RelayPort);        // Convert to network ordering 

	    if (bind(Relaysock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, "bind(Relaysock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket( Relaysock );

			return FALSE;
		}

		if (listen(Relaysock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(Relaysock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect( Relaysock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(Relaysock);
		
			return FALSE;
		}
	}

	if (TCP->HTTPPort)
	{
		TCP->HTTPsock6 = HTTPsock = socket(AF_INET6, SOCK_STREAM, 0);

		if (HTTPsock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "socket() failed error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
		}
 
		psin=&local_sin;
		psin->sin6_port = htons(TCP->HTTPPort);        // Convert to network ordering 

	    if (bind(HTTPsock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, "bind(Relaysock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(HTTPsock);

			return FALSE;
		}

		if (listen(HTTPsock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(Relaysock) failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect(HTTPsock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d\n", WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(HTTPsock);
		
			return FALSE;
		}
	}

	CMSUser.UserName = _strdup("CMS");

	return TRUE;
}

int TelDecodeFrame(char * msg, char * buffer, int Stamp)
{
	UINT returnit;

	_asm {

	pushfd
	cld
	pushad

	mov	esi,msg
	mov	eax,Stamp
	mov	edi,buffer

	call	MONDECODE

	mov	returnit,ecx

	popad
	popfd

	}				// End of ASM

	return (returnit);
}


VOID TelnetPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	struct TCPINFO * TCP = TNC->TCPInfo;
	struct STREAMINFO * STREAM;
	int Msglen;

	int Stream;

	if (TCP->CMS)
	{
		TCP->CheckCMSTimer++;

		if (TCP->CMSOK)
		{
			if (TCP->CheckCMSTimer > 600 * 15)
				CheckCMS(TNC);
		}
		else
		{
			if (TCP->CheckCMSTimer > 600 * 2)
				CheckCMS(TNC);
		}
	}


	if (TELNETMONVEC.HOSTTRACEQ)
	{
		int stamp, len;
		BOOL MonitorNODES = FALSE;
		UINT * monbuff;
		UCHAR * monchars;

		unsigned char buffer[1024] = "\xff\x1b\xb";

		monbuff = Q_REM((UINT *)&TELNETMONVEC.HOSTTRACEQ);
		monchars = (UCHAR *)monbuff;

		stamp = monbuff[88];

		if ((UCHAR)monbuff[4] & 0x80)		// TX
			buffer[2] = 91;
		else
			buffer[2] = 17;
	
		for (Stream = 0; Stream <= TCP->MaxSessions; Stream++)
		{
			STREAM = &TNC->Streams[Stream];

			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])
			{
				struct ConnectionInfo * sockptr = STREAM->ConnectionInfo;

				if (sockptr->BPQTermMode)
				{
					if (!sockptr->MonitorNODES && monchars[21] == 3 && monchars[22] == 0xcf && monchars[23] == 0xff)
					{
						len = 0;
					}
					else
					{
						ULONG SaveMMASK = MMASK;
						BOOL SaveMTX = MTX;
						BOOL SaveMCOM = MCOM;
						BOOL SaveMUI = MUIONLY;

						SetTraceOptionsEx(sockptr->MMASK, sockptr->MTX, sockptr->MCOM, sockptr->MUIOnly);
						len = TelDecodeFrame((char *)monbuff,&buffer[3],stamp);
						SetTraceOptionsEx(SaveMMASK, SaveMTX, SaveMCOM, SaveMUI);

						if (len)
						{
							len += 3;
							buffer[len++] = 0xfe;
							send(STREAM->ConnectionInfo->socket, buffer, len, 0);
						}
					}
				}
			}
		}

		ReleaseBuffer(monbuff);
	}
	
	for (Stream = 0; Stream <= TCP->MaxSessions; Stream++)
	{
		STREAM = &TNC->Streams[Stream];

		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && STREAM->Attached == 0)
		{
			// New Attach

			int calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
			TNC->Streams[Stream].MyCall[calllen] = 0;

			STREAM->Attached = TRUE;
			STREAM->FramesQueued= 0;

			continue;
		}

		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0 && STREAM->Attached)
		{
			// Node has disconnected - clear any connection

			struct ConnectionInfo * sockptr = TNC->Streams[Stream].ConnectionInfo;
			SOCKET sock = sockptr->socket;
			char Msg[80];	
			UINT * buffptr;

			STREAM->Attached = FALSE;
			STREAM->Connected = FALSE;

            if (!sockptr->FBBMode)
			{
				wsprintf(Msg,"*** Disconnected from Stream %d\r\n",Stream);
				send(sock, Msg, strlen(Msg),0);
			}

			if (sockptr->UserPointer == &CMSUser)
			{
				if (CMSLogEnabled)
				{
					char logmsg[120];
					wsprintf(logmsg,"%d Disconnected. Bytes Sent = %d Bytes Received %d Time %d Seconds\r\n",
						sockptr->Number, STREAM->BytesTXed, STREAM->BytesRXed, time(NULL) - sockptr->ConnectTime);

					WriteCMSLog (logmsg);
				}
				// Always Disconnect CMS Socket

				DataSocket_Disconnect(TNC, sockptr);	
			}
			else
			{
				if (LogEnabled)
				{
					char logmsg[120];
					wsprintf(logmsg,"%d Disconnected. Bytes Sent = %d Bytes Received %d\n",
						sockptr->Number, STREAM->BytesTXed, STREAM->BytesRXed);

					WriteLog (logmsg);
				}

				if (TCP->DisconnectOnClose)
				{
					Sleep(1000);
					DataSocket_Disconnect(TNC, sockptr);
				}
			}

			sockptr->FromHostBuffPutptr = sockptr->FromHostBuffGetptr = 0;	// clear any queued data

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
		struct ConnectionInfo * sockptr = TNC->Streams[Stream].ConnectionInfo;
		STREAM = &TNC->Streams[Stream];
				
		if (STREAM->BPQtoPACTOR_Q)
		{
			int datalen;
			UINT * buffptr;
			UCHAR * MsgPtr;
			SOCKET sock;

			buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
			STREAM->FramesQueued--;
			datalen=buffptr[1];
			MsgPtr = (UCHAR *)&buffptr[2];

			if (TNC->Streams[Stream].Connected)
			{
				STREAM->BytesTXed += datalen;

				sock = sockptr->socket;

				if (sockptr->UserPointer  == &CMSUser)
					WritetoTrace(Stream, MsgPtr, datalen);

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
				
				ReleaseBuffer(buffptr);
				continue;
			}
			else // Not Connected
			{

				// Command. Do some sanity checking and look for things to process locally

				datalen--;				// Exclude CR
				MsgPtr[datalen] = 0;	// Null Terminate
				_strupr(MsgPtr);

				if (_memicmp(MsgPtr, "D", 1) == 0)
				{
					TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
					ReleaseBuffer(buffptr);
					return;
				}
				
				if (MsgPtr[0] == 'C' && MsgPtr[1] == ' ' && datalen > 2 && TCP->CMS)	// Connect
				{
					char Host[100];
					int Port;

					if (sscanf(&MsgPtr[2], "%s %d", (char *)&Host, &Port) != 1)
					{
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Error - Invalid Connect Command\r");
						C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
						STREAM->NeedDisc = 10;
						return;
					}

					if (strcmp(Host, "CMS") != 0)
					{
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Error - Only connects to CMS are allowed\r");
						C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
						STREAM->NeedDisc = 10;
						return;
					}

					if (!TCP->CMSOK)
					{
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Error - CMS Not Available\r");
						C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
						STREAM->NeedDisc = 10;
						return;
					}

					TNC->Streams[Stream].Connecting = TRUE;

					CMSConnect(TNC, TCP, STREAM, Stream);
					ReleaseBuffer(buffptr);

					return;
				}

				buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Error - Invalid Command\r");
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
				STREAM->NeedDisc = 10;
				return;
			}
		}

		Msglen = sockptr->FromHostBuffPutptr - sockptr->FromHostBuffGetptr;

		if (Msglen)
		{
			int Paclen = 0;
			int Queued = 0;
			struct TRANSPORTENTRY * Sess1 = TNC->PortRecord->ATTACHEDSESSIONS[Stream];
			struct TRANSPORTENTRY * Sess2 = NULL;
			
			if (Sess1)
				Sess2 = Sess1->L4CROSSLINK;
			
			// Can't use TXCount - it is Semaphored=

			Queued = C_Q_COUNT(&TNC->Streams[Stream].PACTORtoBPQ_Q);
			Queued += C_Q_COUNT((UINT *)&TNC->PortRecord->PORTCONTROL.PORTRX_Q);

			if (Sess2)
			{
				_asm
				{
	
				pushad

				mov esi, Sess2
				call COUNT_QUEUED_FRAMES

				movzx ebx,ah
				movzx eax,al
				add eax,ebx
				add	Queued, eax;

				mov esi, Sess1
				call COUNT_QUEUED_FRAMES

				movzx ebx,ah
				movzx eax,al
				add eax,ebx
				add	Queued, eax;

				popad
				}
			}

			if (Queued > 7)
				continue;

			if (Sess1)
				Paclen = Sess1->SESSPACLEN;

			if (Paclen == 0)
				Paclen = 256;

//			Debugprintf("%d %d %d %d", Msglen, Paclen, Queued, GetFreeBuffs());

			if (Msglen > Paclen)
				Msglen = Paclen;

			if (Sess1) Sess1->L4KILLTIMER = 0;
			if (Sess2) Sess2->L4KILLTIMER = 0;

			SendtoNode(TNC, Stream, &sockptr->FromHostBuffer[sockptr->FromHostBuffGetptr], Msglen);
			sockptr->FromHostBuffGetptr += Msglen;
		}
	}
}


LRESULT CALLBACK TelWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	int i, n;
	struct TNCINFO * TNC;
	struct TCPINFO * TCP;
	struct _EXTPORTDATA * PortRecord;
	HWND SavehDlg, SaveCMS, SaveMonitor;
	HMENU SaveMenu1, SaveMenu2, SaveMenu3; 
	MINMAXINFO * mmi;

    LPMEASUREITEMSTRUCT lpmis;  // pointer to item of data             
	LPDRAWITEMSTRUCT lpdis;     // pointer to item drawing data

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
			return DefMDIChildProc(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WSA_DATA: // Notification on data socket

		Socket_Data(TNC, wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(TNC, wParam);
		return 0;

	case WSA_CONNECT: /* Notification if a socket connection has completed. */

		Telnet_Connected(TNC, wParam, WSAGETSELECTERROR(lParam));
		return 0;

//	case WM_SIZING:
	case WM_SIZE:
	{
		RECT rcClient;
		int ClientHeight, ClientWidth;

		GetClientRect(TNC->hDlg, &rcClient); 

		ClientHeight = rcClient.bottom;
		ClientWidth = rcClient.right;

		MoveWindow(TNC->hMonitor, 0,20 ,ClientWidth-4, ClientHeight-25, TRUE);
		break;
	}

	case WM_GETMINMAXINFO:

 		mmi = (MINMAXINFO *)lParam;
		mmi->ptMaxSize.x = 300;
		mmi->ptMaxSize.y = 400;
		mmi->ptMaxTrackSize.x = 300;
		mmi->ptMaxTrackSize.y = 400;
		break;


	case WM_MDIACTIVATE:
	{			 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu, "Actions");
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);

		}
		else
		{
			 // Deactivate
	
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		 }
			 
		// call DrawMenuBar after the menu items are set
		DrawMenuBar(FrameWnd);

		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);

	}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{ 
		case SC_RESTORE:

			TNC->Minimized = FALSE;
			SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);
			return DefMDIChildProc(hWnd, message, wParam, lParam);

		case  SC_MINIMIZE: 

			TNC->Minimized = TRUE;
			return DefMDIChildProc(hWnd, message, wParam, lParam);
		}
		
		return DefMDIChildProc(hWnd, message, wParam, lParam);

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
		case CMSENABLED:

			// Toggle CMS Enabled Flag

			TCP = TNC->TCPInfo;
			
			TCP->CMS = !TCP->CMS;
			CheckMenuItem(TCP->hActionMenu, 3, MF_BYPOSITION | TCP->CMS<<3);

			if (TCP->CMS)
				CheckCMS(TNC);
			else
			{
				TCP->CMSOK = FALSE;
				SetWindowText(TCP->hCMSWnd, ""); 
			}
			break;

		case IDM_LOGGING:

			// Toggle Logging Flag

			LogEnabled = !LogEnabled;
			CheckMenuItem(TNC->TCPInfo->hLogMenu, 0, MF_BYPOSITION | LogEnabled<<3);

			break;

		case IDM_CMS_LOGGING:

			// Toggle Logging Flag

			LogEnabled = !LogEnabled;
			CheckMenuItem(TNC->TCPInfo->hLogMenu, 1, MF_BYPOSITION | CMSLogEnabled<<3);

			break;

		case TELNET_RECONFIG:

			ProcessConfig();
			FreeConfig();

			for (n = 1; n <= TNC->TCPInfo->CurrentSockets; n++)
			{
				sockptr = TNC->Streams[n].ConnectionInfo;
				sockptr->SocketActive = FALSE;
				closesocket(sockptr->socket);
			}

			TCP = TNC->TCPInfo;

			shutdown(TCP->sock, SD_BOTH);
			shutdown(TCP->sock6, SD_BOTH);

			n = 0;
			while (TCP->FBBsock[n])
				shutdown(TCP->FBBsock[n++], SD_BOTH);

			shutdown(TCP->Relaysock, SD_BOTH);
			shutdown(TCP->HTTPsock, SD_BOTH);
			shutdown(TCP->HTTPsock6, SD_BOTH);


			n = 0;
			while (TCP->FBBsock6[n])
				shutdown(TCP->FBBsock[n++], SD_BOTH);

			shutdown(TCP->Relaysock6, SD_BOTH);

			Sleep(500);
	
			closesocket(TCP->sock);
			closesocket(TCP->sock6);

			n = 0;
			while (TCP->FBBsock[n])
				closesocket(TCP->FBBsock[n++]);

			n = 0;
			while (TCP->FBBsock6[n])
				closesocket(TCP->FBBsock6[n++]);

			closesocket(TCP->Relaysock);
			closesocket(TCP->Relaysock6);
			closesocket(TCP->HTTPsock);
			closesocket(TCP->HTTPsock6);

			// Save info from old TNC record
			
			n = TNC->Port;
			PortRecord = TNC->PortRecord;
			SavehDlg = TNC->hDlg;
			SaveCMS = TCP->hCMSWnd;
			SaveMonitor = TNC->hMonitor;
			SaveMenu1 = TCP->hActionMenu;
			SaveMenu2 = TCP->hDisMenu;
			SaveMenu3 = TCP->hLogMenu;

			// Free old TCP Session Stucts

			for (i = 0; i <= TNC->TCPInfo->MaxSessions; i++)
			{
				free(TNC->Streams[i].ConnectionInfo);
			}

			ReadConfigFile(TNC->Port, ProcessLine);

			TNC = TNCInfo[n];
			TNC->Port = n;
			TNC->Hardware = H_TELNET;
			TNC->hDlg = SavehDlg;
			TNC->RIG = &TNC->DummyRig;			// Not using Rig control, so use Dummy

			// Get Menu Handles

			TCP = TNC->TCPInfo;

			TCP->hCMSWnd = SaveCMS;
			TNC->hMonitor = SaveMonitor;
			TCP->hActionMenu = SaveMenu1;
			TCP->hDisMenu = SaveMenu2;
			TCP->hLogMenu = SaveMenu3;

			CheckMenuItem(TCP->hActionMenu, 3, MF_BYPOSITION | TNC->TCPInfo->CMS<<3);
			CheckMenuItem(TCP->hLogMenu, 0, MF_BYPOSITION | LogEnabled<<3);
			CheckMenuItem(TCP->hLogMenu, 1, MF_BYPOSITION | CMSLogEnabled<<3);

			// Malloc TCP Session Stucts

			for (i = 0; i <= TNC->TCPInfo->MaxSessions; i++)
			{			
				TNC->Streams[i].ConnectionInfo = zalloc(sizeof(struct ConnectionInfo));
				TCP->CurrentSockets = i;  //Record max used to save searching all entries

				if (i > 0)
					ModifyMenu(TCP->hDisMenu,i - 1 ,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + 1, ".");
			}

			TNC->PortRecord = PortRecord;

			Sleep(500);
			OpenSockets(TNC);
			OpenSockets6(TNC);
			CheckCMS(TNC);
			ShowConnections(TNC);

			break;
		default:
			return DefMDIChildProc(hWnd, message, wParam, lParam);
		}
		break;

 //   case WM_SIZE:

//		if (wParam == SIZE_MINIMIZED)
//		return (0);

	case WM_CTLCOLORDLG:
	
		return (LONG)bgBrush;


	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;

		if (TNC->TCPInfo->hCMSWnd == (HWND)lParam)
		{
			if (TNC->TCPInfo->CMSOK)
				SetTextColor(hdcStatic, RGB(0, 128, 0));
			else
				SetTextColor(hdcStatic, RGB(255, 0, 0));
		}
	
		SetBkMode(hdcStatic, TRANSPARENT);
		return (LONG)bgBrush;
	}
	case WM_MEASUREITEM: 
 
		// Retrieve pointers to the menu item's 
		// MEASUREITEMSTRUCT structure and MYITEM structure. 
 
		lpmis = (LPMEASUREITEMSTRUCT) lParam;  
		lpmis->itemWidth = 300; 

		return TRUE; 

	case WM_DRAWITEM: 
 
            // Get pointers to the menu item's DRAWITEMSTRUCT 
            // structure and MYITEM structure. 
 
            lpdis = (LPDRAWITEMSTRUCT) lParam; 
 
            // If the user has selected the item, use the selected 
            // text and background colors to display the item.

			SetTextColor(lpdis->hDC, RGB(0, 128, 0));

			if (TNC->TCPInfo->CMS)
			{
				if (TNC->TCPInfo->CMSOK)
				  TextOut(lpdis->hDC, 340, lpdis->rcItem.top + 2, "CMS OK", 6);
				else
				{
					SetTextColor(lpdis->hDC, RGB(255, 0, 0));
					TextOut(lpdis->hDC, 340, lpdis->rcItem.top + 2, "NO CMS", 6);
				}
			}
			else
				TextOut(lpdis->hDC, 340, lpdis->rcItem.top + 2, "             ", 13);

            return TRUE; 

	case WM_DESTROY:

		break;
	}
			
	return DefMDIChildProc(hWnd, message, wParam, lParam);

}

int Socket_Accept(struct TNCINFO * TNC, int SocketId)
{
	int n,addrlen;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	char Negotiate[6]={IAC,WILL,suppressgoahead,IAC,WILL,echo};
//	char Negotiate[3]={IAC,WILL,echo};
	struct TCPINFO * TCP = TNC->TCPInfo;
	HMENU hDisMenu = TCP->hDisMenu;

//   Find a free Socket

	for (n = 1; n <= TCP->MaxSessions; n++)
	{
		sockptr = TNC->Streams[n].ConnectionInfo;
		
		if (sockptr->SocketActive == FALSE)
		{
			addrlen=sizeof(SOCKADDR_IN6);

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
			sockptr->BPQTermMode = FALSE;
			sockptr->ConnectTime = time(NULL);

			TNC->Streams[n].BytesRXed = TNC->Streams[n].BytesTXed = 0;
			TNC->Streams[n].FramesQueued = 0;

			sockptr->HTTPMode = FALSE;	
			sockptr->FBBMode = FALSE;	
			sockptr->RelayMode = FALSE;

			if (SocketId == TCP->HTTPsock || SocketId == TCP->HTTPsock6)
				sockptr->HTTPMode = TRUE;

			else if (SocketId == TCP->Relaysock || SocketId == TCP->Relaysock6)
				sockptr->RelayMode = TRUE;

			else if (SocketId != TCP->sock && SocketId != TCP->sock6)				// We can have several listening FBB mode sockets
				sockptr->FBBMode = TRUE;

			ModifyMenu(hDisMenu, n - 1, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT + n, ".");

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

			if (sockptr->HTTPMode)
				return 0;

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

			if (sockptr->FromHostBuffer == 0)
			{
				sockptr->FromHostBuffer = malloc(10000);
				sockptr->FromHostBufferSize = 10000;
			}

			sockptr->FromHostBuffPutptr = sockptr->FromHostBuffGetptr = 0;

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

	for (n = 0; n <= TCP->CurrentSockets; n++)
	{
		sockptr = TNC->Streams[n].ConnectionInfo;
	
		if (sockptr->socket == sock && sockptr->SocketActive)
		{
			switch (eventcode)
			{
				case FD_READ:

					if (sockptr->RelayMode)
						return DataSocket_ReadRelay(TNC, sockptr, sock, &TNC->Streams[n]);

					if (sockptr->HTTPMode)
						return DataSocket_ReadHTTP(TNC, sockptr, sock, n);
					
					if (sockptr->FBBMode)
						return DataSocket_ReadFBB(TNC, sockptr, sock, n);
					else
						return DataSocket_Read(TNC, sockptr, sock, &TNC->Streams[n]);

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

		ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number, FALSE);
		TNC->PortRecord->ATTACHEDSESSIONS[sockptr->Number]->Secure_Session = sockptr->UserPointer->Secure;
	}
			
	buffptr[1] = MsgLen;				// Length
	
	memcpy(&buffptr[2], Msg, MsgLen);
	
	C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
}


int DataSocket_Read(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, struct STREAMINFO * STREAM)
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
	int SendIndex = 0;

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

			while (InputLen > 255)
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
			
		STREAM->BytesRXed += MsgLen;
		SendIndex = 0;

		// Line could be up to 500 chars if coming from a program rather than an interative user
		// Limit send to node to 255. Should really use PACLEN instead of 255....

		while (MsgLen > 255)
		{		
			SendtoNode(TNC, sockptr->Number, MsgPtr + SendIndex, 255);
			SendIndex += 255;
			MsgLen -= 255;
		}

		SendtoNode(TNC, sockptr->Number, MsgPtr + SendIndex, MsgLen);
		
		MsgLen += SendIndex;

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
			char Addr[100];
			
			Format_Addr(sockptr, Addr);
			wsprintf(logmsg,"%d %s User=%s\n", sockptr->Number, Addr, MsgPtr);
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

				ModifyMenu(TCP->hDisMenu, n - 1, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT + n, MsgPtr);

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
			char Addr[100];
			
			Format_Addr(sockptr, Addr);
			wsprintf(logmsg,"%d %s Password=%s\n", sockptr->Number, Addr, MsgPtr);
			WriteLog (logmsg);
		}

		if (strcmp(MsgPtr, sockptr->UserPointer->Password) == 0)
		{
			char * ct = TCP->cfgCTEXT;
			char * Appl;
			int ctlen = strlen(ct);

			ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number, FALSE);

			TNC->PortRecord->ATTACHEDSESSIONS[sockptr->Number]->Secure_Session = sockptr->UserPointer->Secure;

            sockptr->LoginState = 2;
            sockptr->InputLen = 0;
            
            if (ctlen > 0)  send(sock, ct, ctlen, 0);

			STREAM->BytesTXed = ctlen;

            if (LogEnabled)
			{
				char Addr[100];
			
				Format_Addr(sockptr, Addr);
				wsprintf(logmsg,"%d %s Call Accepted Callsign=%s\n", sockptr->Number, Addr, sockptr->Callsign);
				WriteLog (logmsg);
			}

			Appl = sockptr->UserPointer->Appl;
			
			if (Appl[0])
				SendtoNode(TNC, sockptr->Number, Appl, strlen(Appl));

			ShowConnections(TNC);

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

int DataSocket_ReadRelay(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, struct STREAMINFO * STREAM)
{
	int len=0, maxlen, InputLen, MsgLen, n;
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

		STREAM->BytesRXed += InputLen;

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

		ModifyMenu(TCP->hDisMenu, n - 1, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT + n, MsgPtr);

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

		ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number, FALSE);

		send(sock, RelayMsg, strlen(RelayMsg), 0);

		sockptr->LoginState = 2;

		sockptr->InputLen = 0;

		if (LogEnabled)
		{
			wsprintf(logmsg,"%d %d.%d.%d.%d Call Accepted Callsign %s\n",
					sockptr->Number,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
					sockptr->Callsign);

			WriteLog (logmsg);
		}

		ShowConnections(TNC);

		sockptr->InputLen = 0;

		// Connect to the BBS

		SendtoNode(TNC, sockptr->Number, TCP->RelayAPPL, strlen(TCP->RelayAPPL));

		ShowConnections(TNC);;
	
		return 0;
	
	default:

		return 0;

	}

	return 0;
}


int DataSocket_ReadFBB(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, int Stream)
{
	int len=0, maxlen, InputLen, MsgLen, i, n;
	char NLMsg[3]={13,10,0};
	byte * CRPtr;
	byte * MsgPtr;
	char logmsg[1000];
	struct UserRec * USER;
	struct TCPINFO * TCP = TNC->TCPInfo;
	struct STREAMINFO * STREAM = &TNC->Streams[Stream];
	
	ioctlsocket(sock,FIONREAD,&len);

	maxlen = InputBufferLen - sockptr->InputLen;
	
	if (len > maxlen) len = maxlen;

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
	MsgPtr[InputLen] = 0;

MsgLoop:

	if (sockptr->LoginState == 2)
	{
		// Data. FBB is binary

		int Paclen = 0;

		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])
			Paclen = TNC->PortRecord->ATTACHEDSESSIONS[Stream]->SESSPACLEN;

//		if (Paclen == 0)
			Paclen = 256;

		if (sockptr->BPQTermMode)
		{
			if (memcmp(MsgPtr, "\\\\\\\\", 4) == 0)
			{
				// Monitor Control

				int n = sscanf(&MsgPtr[4], "%x %x %x %x %x %x",
					&sockptr->MMASK, &sockptr->MTX, &sockptr->MCOM, &sockptr->MonitorNODES,
					&sockptr->MonitorColour, &sockptr->MUIOnly);

				if (n == 5)
					sockptr->MUIOnly = 0;

				sockptr->InputLen = 0;
				return 0;
			}
		}

		if (sockptr->UserPointer == &CMSUser)
			WritetoTrace(Stream, MsgPtr, InputLen);

		if (InputLen == 8 && memcmp(MsgPtr, ";;;;;;\r\n", 8) == 0)
		{
			//	CMS Keepalive

			sockptr->InputLen = 0;
			return 0;
		}

		// Queue to Node. Data may arrive it large quatities, possibly exceeding node buffer capacity

		STREAM->BytesRXed += InputLen;

		if (sockptr->FromHostBuffPutptr + InputLen > sockptr->FromHostBufferSize)
		{
			sockptr->FromHostBufferSize += 10000;
			sockptr->FromHostBuffer = realloc(sockptr->FromHostBuffer, sockptr->FromHostBufferSize);
		}

		memcpy(&sockptr->FromHostBuffer[sockptr->FromHostBuffPutptr], MsgPtr, InputLen); 

		sockptr->FromHostBuffPutptr += InputLen;
		sockptr->InputLen = 0;

		return 0;
	}
	
	if (InputLen > 256)
	{
		// Long message received when waiting for user or password - just ignore

		sockptr->InputLen=0;

		return 0;
	}
	
	if (MsgPtr[0] == 10)			// LF
	{
		// Remove the LF

		InputLen--;

		memmove(MsgPtr, MsgPtr+1, InputLen);
	}

	CRPtr = memchr(MsgPtr, 13, InputLen);
	
	if (CRPtr == 0)
		return 0;							// Waitr for more
	
	// Got a CR

	// Process data up to the cr

	MsgLen = CRPtr - MsgPtr;

	if (MsgLen == 0)						// Just CR
	{
		MsgPtr++;							// Skip it
		InputLen--;
		goto MsgLoop;
	}


	switch (sockptr->LoginState)
	{
	case 3:
		
		if (strstr(MsgPtr, "Callsign :")) 
		{
			char Msg[80];
			int Len;

			Len = wsprintf(Msg, "%s\r\n", TNC->Streams[sockptr->Number].MyCall);			
			send(sock, Msg, Len,0);
			sockptr->InputLen=0;

			return TRUE;
		}

		if (strstr(MsgPtr, "Password :")) 
		{
			// Send “CMSTelnet” + gateway callsign + frequency + emission type if info is available

			struct TRANSPORTENTRY * Sess1 = TNC->PortRecord->ATTACHEDSESSIONS[Stream];
			struct TRANSPORTENTRY * Sess2 = NULL;
			char Passline[80] = "CMSTELNET\r";
			int len = 10;

			if (Sess1)
			{
				Sess2 = Sess1->L4CROSSLINK;

				if (Sess2)
				{
					// See if L2 session - if so, get info from WL2K report line

					if (Sess2->L4CIRCUITTYPE & L2LINK)
					{
						LINKTABLE * LINK = Sess2->L4TARGET;
						PORTCONTROLX * PORT = GetPortTableEntryFromPortNum(LINK->LINKPORT);
						
						if (PORT->WL2KInfo)
							len = sprintf(Passline, "CMSTELNET %s %d %d\r", PORT->WL2KInfo->RMSCall, PORT->WL2KInfo->Freq, PORT->WL2KInfo->mode);

					}
					else
					{
						if (Sess2->RMSCall[0])
						{
							len = sprintf(Passline, "CMSTELNET %s %d %d\r", Sess2->RMSCall, Sess2->Frequency, Sess2->Mode);
						}
					}
				}
			}
			send(sock, Passline, len, 0);
			sockptr->LoginState = 2;		// Data
			sockptr->InputLen=0;

			if (CMSLogEnabled)
			{
				char logmsg[120];
				wsprintf(logmsg,"%d %s\r\n", sockptr->Number, Passline);
				WriteCMSLog (logmsg);
			}

			return TRUE;
		}

		return TRUE;

	case 0:
		
        //   Check Username
        //

		*(CRPtr)=0;				 // remove cr
        
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

				ModifyMenu(TCP->hDisMenu, n - 1, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT + n, MsgPtr);

				ShowConnections(TNC);;

               	InputLen=InputLen-(MsgLen+1);

				sockptr->InputLen=InputLen;
				
				if (InputLen > 0)
				{
					memmove(MsgPtr, CRPtr+1, InputLen);
					goto MsgLoop;
				}
			}
		}

        //   User Not found
        
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
		   
		*(CRPtr)=0;				 // remove cr
            
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
			char * Appl;

			ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number, FALSE);
			TNC->PortRecord->ATTACHEDSESSIONS[sockptr->Number]->Secure_Session = sockptr->UserPointer->Secure;

            sockptr->LoginState = 2;
            
            sockptr->InputLen = 0;
            
            if (LogEnabled)
			{
				wsprintf(logmsg,"%d %d.%d.%d.%d Call Accepted  Callsign %s\n",
					sockptr->Number,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
					sockptr->sin.sin_addr.S_un.S_un_b.s_b4,
					sockptr->Callsign);

				WriteLog (logmsg);
			}

			ShowConnections(TNC);;
			InputLen=InputLen-(MsgLen+1);

			sockptr->InputLen=InputLen;

			// What is left is the Command to connect to the BBS

			if (InputLen > 1)
			{
				if (*(CRPtr+1) == 10)
				{
					CRPtr++;
					InputLen--;
				}

				memmove(MsgPtr, CRPtr+1, InputLen);

				if (_memicmp(MsgPtr, "BPQTermTCP", 10) == 0)
				{
					send(sock, "Connected to TelnetServer\r", 26, 0);
					sockptr->BPQTermMode = TRUE;
				}
				else
				{
					MsgPtr[InputLen] = 13;
					SendtoNode(TNC, sockptr->Number, MsgPtr, InputLen+1);
				}
				sockptr->InputLen = 0;
				return 0;
			}

			Appl = sockptr->UserPointer->Appl;
			
			if (Appl[0])
				SendtoNode(TNC, sockptr->Number, Appl, strlen(Appl));

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

extern char MYNODECALL[];	// 10 chars,not null terminated


int DataSocket_ReadHTTP(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, int Stream)
{
	int len=0, maxlen, InputLen;
	char NLMsg[3]={13,10,0};
	UCHAR * MsgPtr;
	UCHAR * CRLFCRLF;
	UCHAR * LenPtr;
	int BodyLen, ContentLen;

	ioctlsocket(sock,FIONREAD,&len);

	maxlen = InputBufferLen - sockptr->InputLen;
	
	if (len > maxlen) len = maxlen;

	len = recv(sock, &sockptr->InputBuffer[sockptr->InputLen], len, 0);

	if (len == SOCKET_ERROR || len ==0)
	{
		// Failed or closed - clear connection

		return 0;
	}

	// Make sure request is complete - should end crlfcrlf, and if a post have the required input message

	MsgPtr = &sockptr->InputBuffer[0];
	sockptr->InputLen += len;
	InputLen = sockptr->InputLen;

	MsgPtr[InputLen] = 0;

	CRLFCRLF = strstr(MsgPtr, "\r\n\r\n");

	if (CRLFCRLF == 0)
		return 0;

	LenPtr = strstr(MsgPtr, "Content-Length:");

	if (LenPtr)
	{
		ContentLen = atoi(LenPtr + 15);
		BodyLen = InputLen - (CRLFCRLF + 4 - MsgPtr);

		if (BodyLen < ContentLen)
			return 0;
	}

	sockptr->TCP = TNC->TCPInfo;

	_beginthread(ProcessHTTPMessage, 0, (VOID *) sockptr);

	return 0;
}

int DataSocket_Disconnect(struct TNCINFO * TNC,  struct ConnectionInfo * sockptr)
{
	int n;

	if (sockptr->SocketActive)
	{
		closesocket(sockptr->socket);

		n = sockptr->Number;

		ModifyMenu(TNC->TCPInfo->hDisMenu, n - 1, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT + n, ".");

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

	SendMessage(TNC->hMonitor,LB_RESETCONTENT,0,0);

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
			{
				if (sockptr->HTTPMode)
					strcpy(msg,"HTTP Session");
				else
					strcpy(msg,"Logging in");
			}
			else
			{
				i=wsprintf(msg,"%-10s %-10s %2d",
					sockptr->UserPointer->UserName,sockptr->Callsign,sockptr->BPQStream);
			}
		}
		SendMessage(TNC->hMonitor, LB_ADDSTRING ,0, (LPARAM)msg);
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
		
	if ((file = fopen(Value, "a")) == NULL)
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

VOID WriteCMSLog(char * msg)
{
	UCHAR Value[MAX_PATH];
	time_t T;
	struct tm * tm;
	HANDLE Handle;
	char LogMsg[256];
	int MsgLen, cnt;

	if (CMSLogEnabled == FALSE)
		 return;

	T = time(NULL);
	tm = gmtime(&T);

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value, "CMSAccess");
	}
	else
	{
		strcpy(Value, BPQDirectory);
		strcat(Value, "\\");
		strcat(Value, "CMSAccess");
	}

	wsprintf(Value, "%s_%04d%02d%02d.log", Value,
				tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday);

	Handle = CreateFile(Value, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE)
		return;

	SetFilePointer(Handle, 0, 0, FILE_END);

	MsgLen = wsprintf(LogMsg, "%02d:%02d:%02d %s", tm->tm_hour, tm->tm_min, tm->tm_sec, msg);

	WriteFile(Handle ,LogMsg , MsgLen, &cnt, NULL);

	CloseHandle(Handle);

	return;
}
int Telnet_Connected(struct TNCINFO * TNC, SOCKET sock, int Error)
{
	struct ConnectionInfo * sockptr;
	struct TCPINFO * TCP = TNC->TCPInfo;
	UINT * buffptr;
	int Stream;

	// Connect Complete
	//  Find our Socket

	for (Stream = 0; Stream <= TCP->MaxSessions; Stream++)
	{
		sockptr = TNC->Streams[Stream].ConnectionInfo;
		
		if (sockptr->SocketActive)
		{
			if (sockptr->socket == sock)
			{
//				Debugprintf("TCP Connect Complete %d result %d", sock, Error);

				buffptr = GetBuff();
				if (buffptr == 0) return 0;			// No buffers, so ignore
				
				if (Error)
				{
					// Try Next

					TCP->CMSFailed[sockptr->CMSIndex] = TRUE;

					CMSConnect(TNC, TNC->TCPInfo, &TNC->Streams[Stream], Stream);
					return 0;

/*					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Failed to Connect - Error %d\r", Error);
					
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
					
					closesocket(sock);
					TNC->Streams[Stream].Connecting = FALSE;
					sockptr->SocketActive = FALSE;
					ShowConnections(TNC);
					CheckCMS(TNC);
					return 0;
*/
				}

				SaveCMSHostInfo(TNC->Port, TNC->TCPInfo, sockptr->CMSIndex);

				buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** %s Connected to CMS\r", TNC->Streams[Stream].MyCall);;
				C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
		
				WSAAsyncSelect(sock, TNC->hDlg, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
				sockptr->SocketActive = TRUE;
				sockptr->InputLen = 0;
				sockptr->Number = Stream;
				sockptr->LoginState = 3;			// Password State
				sockptr->UserPointer  = &CMSUser;
				sockptr->DoEcho = FALSE;
				sockptr->FBBMode = TRUE;
				sockptr->RelayMode = FALSE;
				sockptr->ConnectTime = time(NULL);
				TNC->Streams[Stream].Connecting = FALSE;
				TNC->Streams[Stream].Connected = TRUE;
				ShowConnections(TNC);

				if (sockptr->FromHostBuffer == 0)
				{
					sockptr->FromHostBuffer = malloc(10000);
					sockptr->FromHostBufferSize = 10000;
				}

				sockptr->FromHostBuffPutptr = sockptr->FromHostBuffGetptr = 0;

				TNC->Streams[Stream].BytesRXed = TNC->Streams[Stream].BytesTXed = 0;

				if (CMSLogEnabled)
				{
					char logmsg[120];
					wsprintf(logmsg,"%d %s Connected to CMS\r\n",
						sockptr->Number, TNC->Streams[Stream].MyCall);

					WriteCMSLog (logmsg);
				}


				return 0;
			}
		}
	}
	return 0;
}

VOID ReportError(struct STREAMINFO * STREAM, char * Msg)
{
	UINT * buffptr;

	buffptr = GetBuff();
	if (buffptr == 0) return;			// No buffers, so ignore
				
	buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "Error %s\r", Msg);
					
	C_Q_ADD(&STREAM->PACTORtoBPQ_Q, buffptr);
}

BOOL CheckCMSThread(struct TNCINFO * TNC);

BOOL CheckCMS(struct TNCINFO * TNC)
{
	TNC->TCPInfo->CheckCMSTimer = 0;
	_beginthread(CheckCMSThread, 0, TNC);
	return 0;
}

BOOL CheckCMSThread(struct TNCINFO * TNC)
{
	// Resolve Name and check connectivity to each address

	struct TCPINFO * TCP = TNC->TCPInfo;
	struct hostent * HostEnt;
	struct in_addr addr;
	struct hostent *remoteHost;
    char **pAlias;
	int i = 0;
	BOOL INETOK = FALSE;

	// First make sure we have a functioning DNS

	TCP->UseCachedCMSAddrs = FALSE;
	
	HostEnt = gethostbyname("a.root-servers.net");
		 
	if (HostEnt)
		INETOK = TRUE;			// We have connectivity
	else
	{
		Debugprintf("Resolve root nameserver failed");

		// Most likely is a local Internet Outage, but we could have Internet, but no name servers

		// Either way, switch to using cached CMS addresses. CMS Validation will check connectivity

		TCP->UseCachedCMSAddrs = TRUE;
		goto CheckServers;
	}

	HostEnt = gethostbyname("server.winlink.org");
		 
	if (!HostEnt || HostEnt->h_addr_list[1] == 0)	// Resolve Failed, or Returned only one Host
	{
		Debugprintf("Resolve CMS Failed");

		// Switch to Cached Servers
		
		TCP->UseCachedCMSAddrs = TRUE;
		goto CheckServers;
	}

	while (HostEnt->h_addr_list[i] != 0 && i < MaxCMS)
	{
		addr.s_addr = *(u_long *) HostEnt->h_addr_list[i];
		TCP->CMSAddr[i] = addr;
		TCP->CMSFailed[i++] = FALSE;
		Debugprintf("CMS Address #%d: %s", i, inet_ntoa(addr));
	}

	TCP->NumberofCMSAddrs = i;

	i = 0;

	while (i < TCP->NumberofCMSAddrs)
	{
		if (TCP->CMSName[i])
			free(TCP->CMSName[i]);
		   		
		remoteHost = gethostbyaddr((char *) &TCP->CMSAddr[i], 4, AF_INET);

		if (remoteHost == NULL)
		{
			int dwError = WSAGetLastError();
	
			TCP->CMSName[i] = NULL;

			if (dwError != 0)
			{
				if (dwError == WSAHOST_NOT_FOUND)
					printf("Host not found\n");
				else if (dwError == WSANO_DATA)
					printf("No data record found\n");
			}
	   }
	   else
	   { 
		   Debugprintf("Official name #%d: %s",i,  remoteHost->h_name);
		   
		   TCP->CMSName[i] = _strdup(remoteHost->h_name);			// Save Host Name
	
		   for (pAlias = remoteHost->h_aliases; *pAlias != 0; pAlias++)
		   {
			   Debugprintf("\tAlternate name #%d: %s", ++i, *pAlias);
		   }
	   }
		i++;
	}

CheckServers:

	CheckMenuItem(TNC->TCPInfo->hActionMenu, 4, MF_BYPOSITION | TCP->UseCachedCMSAddrs<<3);

	if (TCP->UseCachedCMSAddrs)
	{
		// Get Cached Servers from Registry - Names are in RegTree\G8BPQ\BPQ32\PACTOR\PORTn\CMSInfo

		GetCMSCachedInfo(TNC);
	}

	if (TCP->NumberofCMSAddrs == 0)
	{
		TCP->CMSOK = FALSE;
		SetWindowText(TCP->hCMSWnd, "NO CMS"); 

		return TRUE;
	}

	// if we don't know we have Internet connectivity, make sure we can connect to at least one of them

	TCP->CMSOK = INETOK | CMSCheck(TNC, TCP);		// If we know we have Inet, dont check connectivity
	
	if (TCP->CMSOK)
		SetWindowText(TCP->hCMSWnd, "CMS OK"); 
	else
		SetWindowText(TCP->hCMSWnd, "NO CMS"); 

	return TRUE;
}

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 255
#define MAX_VALUE_DATA 255


VOID GetCMSCachedInfo(struct TNCINFO * TNC)
{
	struct TCPINFO * TCP = TNC->TCPInfo;
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 
 
    DWORD i, retCode; 
 
    TCHAR  achValue[MAX_VALUE_NAME]; 
    DWORD cchValue = MAX_VALUE_NAME; 

	HKEY hKey=0;
	char Key[80];

	Debugprintf("Getting cached CMS Server info"); 

	TCP->NumberofCMSAddrs = 0;

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d\\CMSInfo", TNC->Port);
	
	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode != ERROR_SUCCESS)
	{
		Debugprintf("Cached CMS Server info key not found"); 
		return;
	}

    // Get the class name and the value count. 
    retCode = RegQueryInfoKey(
        hKey,                    // key handle 
        achClass,                // buffer for class name 
        &cchClassName,           // size of class string 
        NULL,                    // reserved 
        &cSubKeys,               // number of subkeys 
        &cbMaxSubKey,            // longest subkey size 
        &cchMaxClass,            // longest class string 
        &cValues,                // number of values for this key 
        &cchMaxValue,            // longest value name 
        &cbMaxValueData,         // longest value data 
        &cbSecurityDescriptor,   // security descriptor 
        &ftLastWriteTime);       // last write time 
 
    // Enumerate the key values. 

    if (cValues == 0)
	{
		Debugprintf("No Cached CMS Servers found"); 
		return;
	}

	for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++) 
	{ 
		int Type;
		int ValLen = MAX_VALUE_DATA;
		UCHAR Value[MAX_VALUE_DATA] = "";

		cchValue = MAX_VALUE_NAME; 
		achValue[0] = '\0'; 
	
		retCode = RegEnumValue(hKey, i, achValue, &cchValue, NULL, &Type, Value, &ValLen);
 
		if (retCode == ERROR_SUCCESS) 
		{
			char Time[80], IPADDR[80];
			ULONG IPAD;
			
			Debugprintf("%s = %s", achValue, Value); 
			sscanf(Value, "%s %s", &Time, &IPADDR);

			IPAD = inet_addr(IPADDR);
			memcpy(&TCP->CMSAddr[i], &IPAD, 4);

			TCP->CMSFailed[i] = FALSE;
		
			if (TCP->CMSName[i])
				free(TCP->CMSName[i]);
		   		
			TCP->CMSName[i] = _strdup(achValue);			// Save Host Name
		}
	}

	RegCloseKey(hKey);

	TCP->NumberofCMSAddrs = i;
	return;
}

BOOL CMSCheck(struct TNCINFO * TNC, struct TCPINFO * TCP)
{
	// Make sure at least one CMS can be connected to

	u_long param=1;
	BOOL bcopt=TRUE;
	SOCKET sock;
	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	int n = 0;

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(8772);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	for (n = 0; n < TCP->NumberofCMSAddrs;  n++)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);

		if (sock == INVALID_SOCKET)
			return FALSE;
	
		memcpy(&destaddr.sin_addr.s_addr, &TCP->CMSAddr[n], 4);

		setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

		if (bind(sock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	  	 	return FALSE;

		if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
		{
			closesocket(sock);
			return TRUE;
		}

		// Failed - try next

		closesocket(sock);
	}
	return FALSE;
}


					
CMSConnect(struct TNCINFO * TNC, struct TCPINFO * TCP, struct STREAMINFO * STREAM, int Stream)
{
	int err, status;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	int n;

	sockptr = STREAM->ConnectionInfo;
		
	sock = sockptr->socket = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET)
	{
		ReportError(STREAM, "Create Socket Failed");
		return FALSE;
	}
	
	WSAAsyncSelect(sock, TNC->hDlg, WSA_DATA,
		FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

	sockptr->SocketActive = TRUE;
	sockptr->InputLen = 0;
	sockptr->LoginState = 2;
	sockptr->UserPointer = 0;
	sockptr->DoEcho = FALSE;
	sockptr->BPQTermMode = FALSE;

	sockptr->FBBMode = TRUE;		// Raw Data

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(8772);

	// See if current CMS is down

	n = 0;

	while (TCP->CMSFailed[TCP->NextCMSAddr])
	{
		TCP->NextCMSAddr++;
		if (TCP->NextCMSAddr >= TCP->NumberofCMSAddrs) TCP->NextCMSAddr = 0;
		n++;

		if (n == TCP->NumberofCMSAddrs)
		{
			TCP->CMSOK = FALSE;
			DrawMenuBar(TNC->hDlg);	
			ReportError(STREAM, "All CMS Servers are inaccessible");
			closesocket(sock);
			STREAM->NeedDisc = 10;
			TNC->Streams[Stream].Connecting = FALSE;
			sockptr->SocketActive = FALSE;
			ShowConnections(TNC);
			return FALSE;
		}
	}

	sockptr->CMSIndex = TCP->NextCMSAddr;
	memcpy(&destaddr.sin_addr.s_addr, &TCP->CMSAddr[TCP->NextCMSAddr++], 4);

	if (TCP->NextCMSAddr >= TCP->NumberofCMSAddrs)
		TCP->NextCMSAddr = 0;

	ioctlsocket (sockptr->socket, FIONBIO, &param);
 
	setsockopt (sockptr->socket, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sockptr->socket, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		ReportError(STREAM, "Bind Failed");	
  	 	return FALSE; 
	}

	if ((status = WSAAsyncSelect(sockptr->socket, TNC->hDlg, WSA_CONNECT, FD_CONNECT)) > 0)
	{
		closesocket(sockptr->socket);
		ReportError(STREAM, "WSAAsyncSelect");	
		return FALSE;
	}

	ModifyMenu(TCP->hDisMenu, Stream - 1, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT + Stream, "CMS");

	if (connect(sockptr->socket,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		ReportError(STREAM, "*** Connected");	
		return TRUE;
	}
	else
	{
		err=WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
		{
			//	Connect in Progress

			sockptr->UserPointer  = &CMSUser;
			return TRUE;
		}
		else
		{
			//	Connect failed

			closesocket(sockptr->socket);
			ReportError(STREAM, "Connect Failed");
			CheckCMS(TNC);
			return FALSE;
		}
	}
	return FALSE;

}

VOID SaveCMSHostInfo(int port, struct TCPINFO * TCP, int CMSNo)
{
	HKEY hKey=0;
	char Info[80];
	char Key[80];
	int retCode, disp;

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d\\CMSInfo", port);
	
	retCode = RegCreateKeyEx(REGTREE, Key, 0, 0, 0,
            KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		wsprintf(Info,"%d %d.%d.%d.%d", time(NULL),
			TCP->CMSAddr[CMSNo].S_un.S_un_b.s_b1,
			TCP->CMSAddr[CMSNo].S_un.S_un_b.s_b2,
			TCP->CMSAddr[CMSNo].S_un.S_un_b.s_b3,
			TCP->CMSAddr[CMSNo].S_un.S_un_b.s_b4);

		retCode = RegSetValueEx(hKey, TCP->CMSName[CMSNo], 0, REG_SZ, (BYTE *)&Info, strlen(Info));
		RegCloseKey(hKey);
	}

	return;
}

/*

Keep this in cse we ever do general outgoing TCP

TCPConnect(struct TNCINFO * TNC, struct TCPINFO * TCP, struct STREAMINFO * STREAM, char * Host, int Port)
{
	int err, status;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	int i;

	sockptr = STREAM->ConnectionInfo;
		
	sock = sockptr->socket = socket(AF_INET, SOCK_STREAM, 0);

	if (sock == INVALID_SOCKET)
	{
		ReportError(STREAM, "Create Socket Failed");
		return FALSE;
	}
	
	WSAAsyncSelect(sock, TNC->hDlg, WSA_DATA,
		FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

	sockptr->SocketActive = TRUE;
	sockptr->InputLen = 0;
	sockptr->LoginState = 2;
	sockptr->UserPointer = 0;
	sockptr->DoEcho = FALSE;

	sockptr->FBBMode = TRUE;		// Raw Data
	
	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(Port);

	destaddr.sin_addr.s_addr = inet_addr(Host);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		struct hostent * HostEnt;

		//	Resolve name to address

		HostEnt = gethostbyname(Host);
		 
		 if (!HostEnt)
		 {
			ReportError(STREAM, "Resolve HostName Failed");
			return FALSE;			// Resolve failed
		 }
		 i = 0;
		 while (HostEnt->h_addr_list[i] != 0)
		 {
			    struct in_addr addr;
				addr.s_addr = *(u_long *) HostEnt->h_addr_list[i++];
                Debugprintf("CMS Address #%d: %s", i, inet_ntoa(addr));
		 }
		 memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

	ioctlsocket (sockptr->socket, FIONBIO, &param);
 
	setsockopt (sockptr->socket, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sockptr->socket, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		ReportError(STREAM, "Bind Failed");	
  	 	return FALSE; 
	}

	if ((status = WSAAsyncSelect(sockptr->socket, TNC->hDlg, WSA_CONNECT, FD_CONNECT)) > 0)
	{
		closesocket(sockptr->socket);
		ReportError(STREAM, "WSAAsyncSelect");	
		return FALSE;
	}

	if (connect(sockptr->socket,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		ReportError(STREAM, "*** Connected");	
		return TRUE;
	}
	else
	{
		err=WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
		{
			//	Connect in Progress

			sockptr->UserPointer  = &CMSUser;
			return TRUE;
		}
		else
		{
			//	Connect failed

			closesocket(sockptr->socket);
			ReportError(STREAM, "Connect Failed");	
			return FALSE;
		}
	}
	return FALSE;

}
*/

static VOID Format_Addr(struct ConnectionInfo * sockptr, char * dst)
{
	unsigned char * src;
	char zeros[12] = "";
	char * ptr;
	struct
	{
		int base, len;
	} best, cur;
	unsigned int words[8];
	int i;

	if (sockptr->sin.sin_family != AF_INET6)
	{
		wsprintf(dst,"%d.%d.%d.%d",
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4);

		return;
	}

	src = (char *)&sockptr->sin6.sin6_addr;

	// See if Encapsulated IPV4 addr

	if (src[12] != 0)
	{
		if (memcmp(src, zeros, 12) == 0)	// 12 zeros, followed by non-zero
		{
			wsprintf(dst,"::%d.%d.%d.%d", src[12], src[13], src[14], src[15]);
			return;
		}
	}

	// COnvert 16 bytes to 8 words
	
	for (i = 0; i < 16; i += 2)
	    words[i / 2] = (src[i] << 8) | src[i + 1];

	// Look for longest run of zeros
	
	best.base = -1;
	cur.base = -1;
	
	for (i = 0; i < 8; i++)
	{
		if (words[i] == 0)
		{
	        if (cur.base == -1)
				cur.base = i, cur.len = 1;		// New run, save start
	          else
	            cur.len++;						// Continuation - increment length
		}
		else
		{
			// End of a run of zeros

			if (cur.base != -1)
			{
				// See if this run is longer
				
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				
				cur.base = -1;	// Start again
			}
		}
	}
	
	if (cur.base != -1)
	{
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	
	if (best.base != -1 && best.len < 2)
	    best.base = -1;
	
	ptr = dst;
	  
	for (i = 0; i < 8; i++)
	{
		/* Are we inside the best run of 0x00's? */

		if (best.base != -1 && i >= best.base && i < (best.base + best.len))
		{
			// Just output one : for whole string of zeros
			
			*ptr++ = ':';
			i = best.base + best.len - 1;	// skip rest of zeros
			continue;
		}
	    
		/* Are we following an initial run of 0x00s or any real hex? */
		
		if (i != 0)
			*ptr++ = ':';
		
		ptr += sprintf (ptr, "%x", words[i]);
	        
		//	Was it a trailing run of 0x00's?
	}

	if (best.base != -1 && (best.base + best.len) == 8)
		*ptr++ = ':';
	
	*ptr++ = '\0';	
}

