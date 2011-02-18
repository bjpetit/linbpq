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

extern struct BPQVECSTRUC TELNETMONVEC;
extern int MONDECODE();

static RECT Rect;

extern struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

#define MaxSockets 26

struct UserRec RelayUser;
struct UserRec CMSUser;

BOOL cfgMinToTray;

char AttemptsMsg[] = "Too many attempts - Disconnected\r\n";
char disMsg[] = "Disconnected by SYSOP\r\n";

char BlankCall[]="         ";

BOOL LogEnabled = FALSE;
BOOL CMSLogEnabled = TRUE;

VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

static int ProcessLine(char * buf, int Port);
VOID __cdecl Debugprintf(const char * format, ...);

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
int DataSocket_Write(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_Disconnect(struct TNCINFO * TNC, struct ConnectionInfo * sockptr);
BOOL ProcessTelnetCommand(struct ConnectionInfo * sockptr, byte * Msg, int Len);
int ShowConnections(struct TNCINFO * TNC);
int Terminate();
int SendtoSocket(SOCKET sock,char * Msg);
int WriteLog(char * msg);
VOID WriteCMSLog(char * msg);
byte * EncodeCall(byte * Call);

BOOL CheckCMS(struct TNCINFO * TNC);
TCPConnect(struct TNCINFO * TNC, struct TCPINFO * TCP, struct STREAMINFO * STREAM, char * Host, int Port);
CMSConnect(struct TNCINFO * TNC, struct TCPINFO * TCP, struct STREAMINFO * STREAM,  int Stream);
int Telnet_Connected(struct TNCINFO * TNC, SOCKET sock, int Error);
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
	}

	TNC = TNCInfo[Port];
	TCP = TNC->TCPInfo;

	param=buf;
	*(ptr)=0;
	value=ptr+1;

	if (_stricmp(param, "CMS") == 0)
		TCP->CMS = atoi(value);
	else
	if (_stricmp(param, "WL2KREPORT") == 0)
	{
		*(ptr) = ' ';
		DecodeWL2KReportLine(TNC, param, 0, 0);
	}
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
				if (_stricmp(Secure, "SYSOP") == 0)
					USER->Secure = TRUE;

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
					
		ptr2=memchr(ptr1,13,Len);

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

		for (i = 0; i < Len; i++)
		{
			if (ptr1[i] > 127)
				break;
		}

		if (i == Len)
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

	switch (fn)
	{
	case 1:				// poll

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
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
		Sleep(500);
		closesocket(TCP->sock);

		n = 0;
		while (TCP->FBBsock[n])
			closesocket(TCP->FBBsock[n++]);

		closesocket(TCP->Relaysock);

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

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow, TelWndProc, MAKEINTRESOURCE(IDC_TELNETSERVER));

	hMenu=GetMenu(TNC->hDlg);
	TCP->hActionMenu=GetSubMenu(hMenu,0);

	CheckMenuItem(TNC->TCPInfo->hActionMenu, 3, MF_BYPOSITION | TNC->TCPInfo->CMS<<3);

	TCP->hLogMenu=GetSubMenu(TCP->hActionMenu,0);

	TCP->hDisMenu=GetSubMenu(TCP->hActionMenu,1);

	CheckMenuItem(TCP->hLogMenu,0, MF_BYPOSITION | LogEnabled<<3);
	CheckMenuItem(TCP->hLogMenu, 1, MF_BYPOSITION | CMSLogEnabled<<3);

	ModifyMenu(hMenu, 1, MF_BYPOSITION | MF_OWNERDRAW | MF_STRING, 10000,  0); 

	DrawMenuBar(TNC->hDlg);
 

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

	TNC->RIG = &TNC->DummyRig;			// Not using Rig control, so use Dummy

	if (TCP->CMS)
		CheckCMS(TNC);

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
	int n;

	if (TCP->TCPPort)
	{
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
	}
	n = 0;

	while (TCP->FBBPort[n])
	{
		TCP->FBBsock[n] = FBBsock = socket( AF_INET, SOCK_STREAM, 0);

		if (FBBsock == INVALID_SOCKET)
		{
			wsprintf(szBuff, " socket() failed error %d", WSAGetLastError());
			WritetoConsole(szBuff);
			return FALSE;
		}
 
		psin=&local_sin;
		psin->sin_port = htons(TCP->FBBPort[n++]);        // Convert to network ordering 

		if (bind(FBBsock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			wsprintf(szBuff, " bind(FBBsock %d) failed Error %d", TCP->FBBPort[n-1], WSAGetLastError());
			WritetoConsole(szBuff);
			closesocket(FBBsock);

			return FALSE;
		}

		if (listen(FBBsock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, " listen(FBBsock) failed Error %d", WSAGetLastError());
			WritetoConsole(szBuff);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect(FBBsock, TNC->hDlg, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, " WSAAsyncSelect failed Error %d", WSAGetLastError());
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

	int Stream;

	if (TNC->UpdateWL2K && TCP->CMS)
	{
		TNC->UpdateWL2KTimer--;

		if (TNC->UpdateWL2KTimer == 0)
		{
			TNC->UpdateWL2KTimer = 32910;		// Every Hour
			if (CheckAppl(TNC, "RMS         ")) // Is RMS Available?
			{
				if (TNC->NodeCall[0])
					memcpy(TNC->RMSCall, TNC->NodeCall, 9);	// Report Port Call if present
				SendReporttoWL2K(TNC);
			}
		}
	}

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

		if ((UCHAR)monbuff[2] & 0x80)		// TX
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
						SetTraceOptions(sockptr->MMASK, sockptr->MTX, sockptr->MCOM);
						len = TelDecodeFrame((char *)monbuff,&buffer[3],stamp);
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
		STREAM = &TNC->Streams[Stream];
				
		if (STREAM->BPQtoPACTOR_Q)
		{
			int datalen;
			UINT * buffptr;
			UCHAR * MsgPtr;
			SOCKET sock;
			struct ConnectionInfo * sockptr = TNC->Streams[Stream].ConnectionInfo;

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
	struct TCPINFO * TCP;
	struct _EXTPORTDATA * PortRecord;
	HWND SavehDlg;
	HMENU hMenu;

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
		return (DefWindowProc(hWnd, message, wParam, lParam));

	switch (message)
	{
	case WSA_DATA: // Notification on data socket

		Socket_Data(TNC, wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(TNC, wParam);
		return 0;

	case WSA_CONNECT: /* Notification if a socket connection is pending. */

		Telnet_Connected(TNC, wParam, WSAGETSELECTERROR(lParam));
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
		case CMSENABLED:

			// Toggle CMS Enabled Flag

			TCP = TNC->TCPInfo;
			
			TCP->CMS = !TCP->CMS;
			CheckMenuItem(TNC->TCPInfo->hActionMenu, 3, MF_BYPOSITION | TCP->CMS<<3);

			if (TCP->CMS)
				CheckCMS(TNC);
			else
			{
				TCP->CMSOK = FALSE;
				DrawMenuBar(TNC->hDlg);	
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

			n = 0;
			while (TCP->FBBsock[n])
				shutdown(TCP->FBBsock[n++], SD_BOTH);

			shutdown(TCP->Relaysock, SD_BOTH);
			Sleep(500);
			closesocket(TCP->sock);

			n = 0;
			while (TCP->FBBsock[n])
				closesocket(TCP->FBBsock[n++]);

			closesocket(TCP->Relaysock);

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

			// Get Menu Handles

			hMenu=GetMenu(TNC->hDlg);
			TCP = TNC->TCPInfo;
			TCP->hActionMenu=GetSubMenu(hMenu,0);
			CheckMenuItem(TNC->TCPInfo->hActionMenu, 3, MF_BYPOSITION | TNC->TCPInfo->CMS<<3);
			TCP->hLogMenu=GetSubMenu(TCP->hActionMenu,0);
			TCP->hDisMenu=GetSubMenu(TCP->hActionMenu,1);
			CheckMenuItem(TCP->hLogMenu,0,MF_BYPOSITION | LogEnabled<<3);

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
			CheckCMS(TNC);
			ShowConnections(TNC);

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
 
        // Process other messages.  
 
 

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
			sockptr->BPQTermMode = FALSE;
			sockptr->ConnectTime = time(NULL);

			TNC->Streams[n].BytesRXed = TNC->Streams[n].BytesTXed = 0;
			TNC->Streams[n].FramesQueued = 0;

			sockptr->FBBMode = FALSE;	
			sockptr->RelayMode = FALSE;

			if (SocketId == TCP->Relaysock)
				sockptr->RelayMode = TRUE;
			else
			if (SocketId != TCP->sock)				// We can have several listening FBB mode sockets
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

	for (n = 0; n <= TCP->CurrentSockets; n++)
	{
		sockptr = TNC->Streams[n].ConnectionInfo;
	
		if (sockptr->socket == sock && sockptr->SocketActive)
		{
			switch (eventcode)
			{
				case FD_READ:

					if (sockptr->FBBMode)
						return DataSocket_ReadFBB(TNC, sockptr, sock, n);
					else
						if (sockptr->RelayMode)
							return DataSocket_ReadRelay(TNC, sockptr, sock, &TNC->Streams[n]);
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

		ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);
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
			
		STREAM->BytesRXed += MsgLen;

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
			char * Appl;
			int ctlen = strlen(ct);

			ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);

			TNC->PortRecord->ATTACHEDSESSIONS[sockptr->Number]->Secure_Session = sockptr->UserPointer->Secure;

            sockptr->LoginState = 2;
            sockptr->InputLen = 0;
            
            if (ctlen > 0)  send(sock, ct, ctlen, 0);

			STREAM->BytesTXed = ctlen;

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

		ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);

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

MsgLoop:

	if (sockptr->LoginState == 2)
	{
		// Data. FBB is binary

		int Paclen = 0;

		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])
			Paclen = TNC->PortRecord->ATTACHEDSESSIONS[Stream]->SESSPACLEN;

		if (Paclen == 0)
			Paclen = 256;

		if (sockptr->BPQTermMode)
		{
			if (memcmp(MsgPtr, "\\\\\\\\", 4) == 0)
			{
				// Monitor Control

				sscanf(&MsgPtr[4], "%x %x %x %x %x",
					&sockptr->MMASK, &sockptr->MTX, &sockptr->MCOM, &sockptr->MonitorNODES, &sockptr->MonitorColour);
				sockptr->InputLen = 0;
				return 0;
			}
		}

		if (sockptr->UserPointer == &CMSUser)
			WritetoTrace(Stream, MsgPtr, InputLen);

		// Send to Node

		STREAM->BytesRXed += InputLen;

		if (InputLen > Paclen)
		{		
			SendtoNode(TNC, sockptr->Number, MsgPtr, Paclen);
			sockptr->InputLen -= Paclen;

			InputLen -= Paclen;

			memmove(MsgPtr,MsgPtr+Paclen,InputLen);

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
			send(sock, "CMSTELNET\r\n", 11,0);
			sockptr->LoginState = 2; // Data
			sockptr->InputLen=0;
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

			ProcessIncommingConnect(TNC, sockptr->Callsign, sockptr->Number);
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

	for (Stream = 0; Stream <= MaxSockets; Stream++)
	{
		sockptr = TNC->Streams[Stream].ConnectionInfo;
		
		if (sockptr->SocketActive)
		{
			if (sockptr->socket == sock)
			{
				Debugprintf("TCP Connect Complete %d result %d", sock, Error);

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

	HostEnt = gethostbyname("server.winlink.org");
		 
	if (!HostEnt)
	{
		Debugprintf("Resolve CMS Failed");
		TCP->CMSOK = FALSE;
		DrawMenuBar(TNC->hDlg);	

		return FALSE;			// Resolve failed
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
	while (i <  TCP->NumberofCMSAddrs)
	{
		remoteHost = gethostbyaddr((char *) &TCP->CMSAddr[i++], 4, AF_INET);

	   if (remoteHost == NULL)
	   {
		    int dwError = WSAGetLastError();
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
		   for (pAlias = remoteHost->h_aliases; *pAlias != 0; pAlias++)
		   {
			   Debugprintf("\tAlternate name #%d: %s", ++i, *pAlias);
		   }
	   }
	}
	TCP->CMSOK = TRUE;
	DrawMenuBar(TNC->hDlg);	

	return TRUE;
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