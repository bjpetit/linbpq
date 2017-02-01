// MapWebServer
//

// This code is hacked together from the BPQ32 webserver,
//	so looks rather odd and has lots of redundant
//	data fields

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:

#include <time.h>
#include <sys/stat.h>
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

#include <winioctl.h>
#include "winsock2.h"
#include "WS2tcpip.h"

#define ioctl ioctlsocket

#define InputBufferLen 100000
#define MAX_PENDING_CONNECTS 5

#include "MapWebServer.h"

void * zalloc(int len)
{
	// malloc and clear

	void * ptr;

	ptr=malloc(len);

	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}



#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

VOID TIMERINTERRUPT();
void TelnetInit();
int ProcessHTTPMessage(struct ConnectionInfo * conn);
int DataSocket_Disconnect(struct TNCINFO * TNC,  struct ConnectionInfo * sockptr);
BOOL OpenSockets(struct TNCINFO * TNC);
BOOL OpenSockets6(struct TNCINFO * TNC);
int Socket_Accept(struct TNCINFO * TNC, int SocketId);
int InnerProcessHTTPMessage(struct ConnectionInfo * conn);
int SendMessageFile(SOCKET sock, char * FN, BOOL OnlyifExists);

HINSTANCE hInst;


int REALTIMETICKS = 0;

int Connects = 0;
int Connections = 0;
int MaxCons = 0;

VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );


TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;
int TimerHandle = 0;

VOID CALLBACK TimerProc(
	HWND  hwnd,	// handle of window for timer messages 
    UINT  uMsg,	// WM_TIMER message
    UINT  idEvent,	// timer identifier
    DWORD  dwTime)	// current system time	
{
 	KillTimer(NULL,TimerHandle);
	TIMERINTERRUPT();
	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MAPWEBSERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAPWEBSERVER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAPWEBSERVER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MAPWEBSERVER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND hWnd;
   
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

   WSADATA       WsaData;            // receives data from WSAStartup

   hInst = hInstance; // Store instance handle in our global variable


   	hInst = hInstance;

	hWnd=CreateDialog(hInst,szWindowClass,0,WndProc);

	UpdateWindow(hWnd);
	ShowWindow(hWnd, nCmdShow);
	  			
	WSAStartup(MAKEWORD(2, 0), &WsaData);

	TelnetInit();

	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDCANCEL:
		case CLOSEBUTTON:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}


struct TNCINFO * TNCInfo;

static VOID SetupListenSet(struct TNCINFO * TNC);

int DataSocket_ReadHTTP(struct TNCINFO * TNC, struct ConnectionInfo * sockptr, SOCKET sock, int Stream)
{
	int len=0, maxlen, InputLen;
	char NLMsg[3]={13,10,0};
	UCHAR * MsgPtr;
	UCHAR * CRLFCRLF;
	UCHAR * LenPtr;
	int BodyLen, ContentLen;
	struct ConnectionInfo * sockcopy;
	
	ioctl(sock,FIONREAD,&len);

//	Debugprintf("Read Msg Len %d Sock %d", len, sock);

	maxlen = InputBufferLen - sockptr->InputLen;
	
	if (len > maxlen) len = maxlen;

	len = recv(sock, &sockptr->InputBuffer[sockptr->InputLen], len, 0);

	if (len == SOCKET_ERROR || len == 0)
	{
		// Failed or closed - clear connection

		TNC->Streams[sockptr->Number].ReportDISC = TRUE;		//Tell Node
		DataSocket_Disconnect(TNC, sockptr);
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

	sockcopy = malloc(sizeof(struct ConnectionInfo));
	sockptr->TNC = TNC;
	sockptr->LastSendTime = REALTIMETICKS;

	memcpy(sockcopy, sockptr, sizeof(struct ConnectionInfo));

	_beginthread((void (*)())ProcessHTTPMessage, 0, (VOID *)sockcopy);

	sockptr->InputLen = 0;
	return 0;
}

int DataSocket_Disconnect(struct TNCINFO * TNC,  struct ConnectionInfo * sockptr)
{
	int n;

//	Debugprintf("Closing Socket %d", sockptr->socket);

	if (sockptr->SocketActive)
	{
		closesocket(sockptr->socket);

		n = sockptr->Number;

		sockptr->SocketActive = FALSE;

	}
	return 0;
}


void TelnetInit()
{
	struct TNCINFO * TNC;
	struct TCPINFO * TCP;
	int i;
	HWND x=0;

	TNC = TNCInfo = zalloc(sizeof(struct TNCINFO));
	TCP = TNC->TCPInfo = zalloc(sizeof (struct TCPINFO)); // Telnet Server Specific Data

	TCP->MaxSessions = 60;	
	TCP->IPV4 = TRUE;
	TCP->HTTPPort= 81;

	// Malloc TCP Session Stucts

	for (i = 0; i <= TNC->TCPInfo->MaxSessions; i++)
	{
		TNC->Streams[i].ConnectionInfo = zalloc(sizeof(struct ConnectionInfo));
		TNC->Streams[i].ConnectionInfo->Number = i;
		TCP->CurrentSockets = i;  //Record max used to save searching all entries
	}

	OpenSockets(TNC);
	OpenSockets6(TNC);
	SetupListenSet(TNC);

	return;
}

SOCKET OpenSocket4(struct TNCINFO * xTNC, int port)
{
	struct sockaddr_in  local_sin;  /* Local socket - internet style */
	struct sockaddr_in * psin;
	SOCKET sock = 0;
	u_long param=1;

	char szBuff[80];

	psin=&local_sin;
	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;

	if (port)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);

	    if (sock == INVALID_SOCKET)
		{
	        sprintf(szBuff, "socket() failed error %d\n", WSAGetLastError());
			return 0;
		}

		setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&param,4);

 
		psin->sin_port = htons(port);        // Convert to network ordering 

		if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			sprintf(szBuff, "bind(sock) failed port %d Error %d\n", port, WSAGetLastError());
		    closesocket( sock );
			return FALSE;
		}

		if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
		{
			sprintf(szBuff, "listen(sock) failed port %d Error %d\n", port, WSAGetLastError());
			return FALSE;
		}
		ioctl(sock, FIONBIO, &param);
	}

	return sock;
}

BOOL OpenSockets(struct TNCINFO * TNC)
{
	struct sockaddr_in  local_sin;  /* Local socket - internet style */
	struct sockaddr_in * psin;
	u_long param=1;
	struct TCPINFO * TCP = TNC->TCPInfo;

	if (!TCP->IPV4)
		return TRUE;

	psin=&local_sin;
	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;

	if (TCP->HTTPPort)
		TCP->HTTPsock = OpenSocket4(TNC, TCP->HTTPPort);

	return TRUE;
}

BOOL OpenSocket6(struct TNCINFO * TNC, int port)
{
	struct sockaddr_in6 local_sin;  /* Local socket - internet style */
	struct sockaddr_in6 * psin;
	SOCKET sock;
	char szBuff[80];
	u_long param=1;

	memset(&local_sin, 0, sizeof(local_sin));

	psin=&local_sin;
	psin->sin6_family = AF_INET6;
	psin->sin6_addr = in6addr_any;
	psin->sin6_flowinfo = 0;
	psin->sin6_scope_id = 0;

	sock = socket(AF_INET6, SOCK_STREAM, 0);
	
	if (sock == INVALID_SOCKET)
	{
		sprintf(szBuff, "IPV6 socket() failed error %d\n", WSAGetLastError());
		return FALSE;  
	}

	psin->sin6_port = htons(port);        // Convert to network ordering 

	if (bind(sock, (struct sockaddr FAR *)psin, sizeof(local_sin)) == SOCKET_ERROR)
	{
		sprintf(szBuff, "IPV6 bind(sock) failed Error %d\n", WSAGetLastError());
	    closesocket( sock );

		return FALSE;
	}

	if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
	{
		sprintf(szBuff, "IPV6 listen(sock) failed Error %d\n", WSAGetLastError());

		return FALSE;
	}

	ioctl(sock, FIONBIO, &param);
	return sock;
}


BOOL OpenSockets6(struct TNCINFO * TNC)
{
	struct sockaddr_in6 local_sin;  /* Local socket - internet style */

	struct sockaddr_in6 * psin;
	struct TCPINFO * TCP = TNC->TCPInfo;
	u_long param=1;

	if (!TCP->IPV6)
		return TRUE;

	memset(&local_sin, 0, sizeof(local_sin));

	psin=&local_sin;
	psin->sin6_family = AF_INET6;
	psin->sin6_addr = in6addr_any;
	psin->sin6_flowinfo = 0;
	psin->sin6_scope_id = 0;

	if (TCP->HTTPPort)
		TCP->HTTPsock6 = OpenSocket6(TNC, TCP->HTTPPort);

	return TRUE;
}

static VOID SetupListenSet(struct TNCINFO * TNC)
{
	// Set up master set of fd's for checking for incoming calls

	struct TCPINFO * TCP = TNC->TCPInfo;
	SOCKET maxsock = 0;
	fd_set * readfd = &TCP->ListenSet;
	SOCKET sock;

	FD_ZERO(readfd);

	sock = TCP->HTTPsock;
	if (sock)
	{
		FD_SET(sock, readfd);
		if (sock > maxsock)
			maxsock = sock;
	}
		
	sock = TCP->HTTPsock6;
	if (sock)
	{
		FD_SET(sock, readfd);
		if (sock > maxsock)
			maxsock = sock;
	}

	TCP->maxsock = maxsock;
}



VOID TelnetPoll()
{
	struct TNCINFO * TNC = TNCInfo;
	struct TCPINFO * TCP = TNC->TCPInfo;

	//	we now poll for incoming connections and data

	fd_set readfd, writefd, exceptfd;
	struct timeval timeout;
	int retval;
	int n;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	int Active = 0;
	SOCKET maxsock;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;				// poll

	if (TCP->maxsock == 0)
		return;
		
	memcpy(&readfd, &TCP->ListenSet, sizeof(fd_set));

	retval = select(TCP->maxsock + 1, &readfd, NULL, NULL, &timeout);

	if (retval == -1)
	{
		retval = WSAGetLastError();
		perror("listen select");
	}

	if (retval)
	{
		n = 0;
		
		sock = TCP->HTTPsock;

		if (sock)
		{
			if (FD_ISSET(sock, &readfd))
				Socket_Accept(TNC, sock);
		}
		
		sock = TCP->HTTPsock6;
		if (sock)
		{
			if (FD_ISSET(sock, &readfd))
			Socket_Accept(TNC, sock);
		}	
	}
		
	// look for data on any active sockets

	maxsock = 0;

	FD_ZERO(&readfd);
	FD_ZERO(&writefd);
	FD_ZERO(&exceptfd);

	for (n = 0; n <= TCP->MaxSessions; n++)
	{
		sockptr = TNC->Streams[n].ConnectionInfo;
		
		//	Should we use write event after a blocked write ????

		if (sockptr->SocketActive)
		{
			FD_SET(sockptr->socket, &readfd);
			FD_SET(sockptr->socket, &exceptfd);
			
			Active++;
			if (sockptr->socket > maxsock)
				maxsock = sockptr->socket;

		}
	}

	if (Active)
	{
		retval = select(maxsock + 1, &readfd, &writefd, &exceptfd, &timeout);

		if (retval == -1)
		{				
			perror("data select");
			Debugprintf("Select Error %d", WSAGetLastError());

			// 
		}
		else
		{
			if (retval)
			{
				// see who has data

				for (n = 0; n <= TCP->MaxSessions; n++)
				{
					sockptr = TNC->Streams[n].ConnectionInfo;
		
					if (sockptr->SocketActive)
					{
						sock = sockptr->socket;

						if (FD_ISSET(sock, &readfd))
						{
							DataSocket_ReadHTTP(TNC, sockptr, sock, n);
						}
					}
				}
			}
		}
	}

}

VOID TIMERINTERRUPT()
{
	int txlen = 0, n, active = 0;
	struct TNCINFO * TNC = TNCInfo;
	struct TCPINFO * TCP = TNC->TCPInfo;

	struct ConnectionInfo * sockptr;

	// Main Processing loop - CALLED EVERY 100 MS

	REALTIMETICKS++;

	// We now use persistent HTTP sessions, so need to close after a reasonable time
		
		for (n = 0; n <= TCP->MaxSessions; n++)
		{
			sockptr = TNC->Streams[n].ConnectionInfo;

			if (sockptr->SocketActive && sockptr->HTTPMode && sockptr->LastSendTime)
			{
				active++;

				if ((REALTIMETICKS -sockptr->LastSendTime) > 1500)	// ~ 2.5 mins
				{
//					Debugprintf("Timer Closing Sess %d Sock %d", n, sockptr->socket);
					closesocket(sockptr->socket);
					sockptr->SocketActive = FALSE;
				}
			}
		}

		if (active != Connections)
		{
			Connections = active;
			SetDlgItemInt(hWnd, IDC_ACTIVE, Connections, FALSE);
		}

		TelnetPoll();


}
int ProcessHTTPMessage(struct ConnectionInfo * conn)
{
	// conn is a malloc'ed copy to handle reused connections, so need to free it


	int ret = InnerProcessHTTPMessage(conn);
	free(conn);
	return ret;
}

int InnerProcessHTTPMessage(struct ConnectionInfo * conn)
{
	struct TCPINFO * TCP = conn->TNC->TCPInfo;
	SOCKET sock = conn->socket;
	char * MsgPtr = conn->InputBuffer;
	int MsgLen = conn->InputLen;
	int InputLen = 0;
	int OutputLen = 0;
	int Bufferlen;
	struct HTTPConnectionInfo CI;
	struct HTTPConnectionInfo * sockptr = &CI;
	struct HTTPConnectionInfo * Session = NULL;

	char URL[100000];
	char * ptr;
	char * Context, * Method, * NodeURL, * Key;
	int ReplyLen = 0;
	BOOL LOCAL = FALSE;
	BOOL COOKIE = FALSE;
	int Len;

//	Debugprintf("Process Msg Sess Sock %d", sock);

	__try {

	Len = strlen(MsgPtr);
	if (Len > 100000)
		return 0; 

	strcpy(URL, MsgPtr);

	if (strstr(MsgPtr, "Host: 127.0.0.1"))
		LOCAL = TRUE;


	ptr = strstr(URL, " HTTP");

	if (ptr)
		*ptr = 0;

	Method = strtok_s(URL, " ", &Context);
	NodeURL = strtok_s(Context, "?", &Key);


	
		// Not Node, See if a local file

//	Debugprintf("NodeURL %s", NodeURL);
		
		Bufferlen = SendMessageFile(sock, NodeURL, FALSE);		// Send error if not found
		return 0;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		Debugprintf("InnerProcessHMLMessage Exception");
	}
	return 0;
}

static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


VOID FormatTime2(char * Time, time_t cTime)
{
	struct tm * TM;
	TM = gmtime(&cTime);

	sprintf(Time, "%s, %02d %s %3d %02d:%02d:%02d GMT", dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon],
		TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);

}

// Sun, 06 Nov 1994 08:49:37 GMT


int SendMessageFile(SOCKET sock, char * FN, BOOL OnlyifExists)
{
	int FileSize, Sent, Loops = 0;
	char * MsgBytes;
	FILE * hFile;
	int ReadLen;
	BOOL Special = FALSE;
	int Len;
	int HeaderLen;
	char Header[256];
	struct stat STAT;

	__try
	{
		FN++;		// Skip leading /

		if (strchr(FN, '/') || strchr(FN, '\\'))
			FN[0] = 0;

		if (strlen(FN) > 256)
		{
			FN[256] = 0;
			Debugprintf("HTTP File Name too long %s", FN);
		}

		if (FN[0] == 0)
			strcpy(FN, "NodeMap.html");

		if (stat(FN, &STAT) == -1)
		{
			Len = sprintf(Header, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
			send(sock, Header, Len, 0);

			return 0;
		}

		hFile = fopen(FN, "rb");
	
		if (hFile == 0)
		{
			Len = sprintf(Header, "HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
			send(sock, Header, Len, 0);
			return 0;
		}
	
		FileSize = STAT.st_size;

		MsgBytes = malloc(FileSize + 1);

		ReadLen = fread(MsgBytes, 1, FileSize, hFile); 

		fclose(hFile);

		HeaderLen = sprintf(Header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
			"Content-Type: text/html\r\n\r\n", FileSize); 

		send(sock, Header, HeaderLen, 0);

		Sent = send(sock, MsgBytes, FileSize, 0);

		if (Sent < 0)
		{
			Debugprintf("Send Failed %d", WSAGetLastError());
			free (MsgBytes);
			return 0;
		}
		
		while (Sent != FileSize && Loops++ < 3000)				// 100 secs max
		{	
			if (Sent > 0)					// something sent
			{
				FileSize -= Sent;
				memmove(MsgBytes, &MsgBytes[Sent], FileSize);
			}
					
			Sleep(30);
			Sent = send(sock, MsgBytes, FileSize, 0);
//		Debugprintf("%d out of %d sent", Sent, FileSize);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		Debugprintf("SendMessageFileException");
	}
	free (MsgBytes);
	return 0;
}


int Socket_Accept(struct TNCINFO * TNC, int SocketId)
{
	int n, addrlen = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 sin6;  

	struct ConnectionInfo * sockptr;
	SOCKET sock;
	struct TCPINFO * TCP = TNC->TCPInfo;
	u_long param=1;



//   Find a free Session

	for (n = 1; n <= TCP->MaxSessions; n++)
	{
		sockptr = TNC->Streams[n].ConnectionInfo;
		
		if (sockptr->SocketActive == FALSE)
		{
			sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

			if (sock == INVALID_SOCKET)
			{
				Debugprintf("MapWebServer accept() failed Error %d", WSAGetLastError());
				return FALSE;
			}

			if (n > MaxCons)
				MaxCons = n;

			Connects++;

			SetDlgItemInt(hWnd, IDC_CONNECTIONS, Connects, FALSE);
			SetDlgItemInt(hWnd, IDC_MAX, MaxCons, FALSE);

			Debugprintf("Socket Accept Sess %d Sock %d", n, sock);

			ioctl(sock, FIONBIO, &param);

			sockptr->socket = sock;
			sockptr->SocketActive = TRUE;
			sockptr->InputLen = 0;
			sockptr->Number = n;
			sockptr->LoginState = 0;
			sockptr->UserPointer = 0;
			sockptr->DoEcho = FALSE;
			sockptr->BPQTermMode = FALSE;
			sockptr->ConnectTime = time(NULL);
			sockptr->Keepalive = FALSE;
			sockptr->UTF8 = 0;

			TNC->Streams[n].BytesRXed = TNC->Streams[n].BytesTXed = 0;
			TNC->Streams[n].FramesQueued = 0;

			sockptr->HTTPMode = FALSE;	
			sockptr->FBBMode = FALSE;	
			sockptr->RelayMode = FALSE;
			sockptr->ClientSession = FALSE;
			sockptr->NeedLF = FALSE;

			if (SocketId == TCP->HTTPsock || SocketId == TCP->HTTPsock6)
				sockptr->HTTPMode = TRUE;

			return 0;
		}	
	}

	//	No free sessions. Must accept() then close

	sock = accept(SocketId, (struct sockaddr *)&sin6, &addrlen);

	send(sock,"No Free Sessions\r\n", 18,0);
	Sleep (1000);
	closesocket(sock);

	return 0;
}


