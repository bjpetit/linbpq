// Mail and Chat Server for BPQ32 Packet Switch
//
//

#include "stdafx.h"


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

HWND MainWnd;
HMENU hActionMenu;
HMENU hLogMenu;
HMENU hDisMenu;									// Disconnect Menu Handle

char szBuff[80];

#define MaxSockets 64

ConnectionInfo Connections[MaxSockets+1];

int NumberofUserRecords=0;

struct UserInfo ** UserRecPtr=NULL;
int NumberofUsers=0;

struct UserInfo * BBSChain = NULL;						// Chain of users that are BBSes

struct MsgInfo ** MsgHddrPtr=NULL;
int NumberofMessages=0;

int FirstMessagetoForward=1;					// Lowest Message wirh a forward bit set - limits search

BIDRec ** BIDRecPtr=NULL;
int NumberofBIDs=0;


int LatestMsg = 0;

int Port=8010;
BOOL cfgMinToTray;

BOOL DisconnectOnClose=FALSE;


char PasswordMsg[100]="Password:";

char cfgHOSTPROMPT[100];

char cfgCTEXT[100];

char cfgLOCALECHO[100];

char AttemptsMsg[] = "Too many attempts - Disconnected\r\r";
char disMsg[] = "Disconnected by SYSOP\r\r";

char LoginMsg[]="user:";

char BlankCall[]="         ";


BOOL LogEnabled=FALSE;

UCHAR BBSApplMask;
UCHAR ChatApplMask;

int BBSApplNum=0;
int ChatApplNum=0;

int	StartStream=0;
int	NumberofStreams=20;
int MaxStreams=20;

char BBSSID[]="[BPQ-1.00-FH$]\r";
//char BBSSID[]="[BPQ-1.00-AB1FHMRX$]\r";

char ChatSID[]="[BPQChatServer-1.00]\r";

char NewUserPrompt[100]="Please enter your Name: ";

char Prompt[100]="de GM8BPQ>\r";

char BBSName[100];

char HRoute[100];

char SignoffMsg[100]="73 de GM8BPQ\r";

char AbortedMsg[100]="\rOutput aborted\r";

char UserDatabaseName[MAX_PATH] = "BPQBBSUsers.dat";
char UserDatabasePath[MAX_PATH];

char MsgDatabasePath[MAX_PATH];
char MsgDatabaseName[MAX_PATH] = "DIRMES.SYS";

char BIDDatabasePath[MAX_PATH];
char BIDDatabaseName[MAX_PATH] = "WFBID.SYS";

char BaseDir[MAX_PATH];

char MailDir[MAX_PATH];

extern char RtUsr[];
extern char RtUsrTemp[];

UCHAR * OtherNodes=NULL;

/*

extern int SMTPInPort;
extern int POP3InPort;

extern BOOL ISP_Gateway_Enabled;

extern int ISPPOP3Interval;

extern char MyDomain[];			// Mail domain for BBS<>Internet Mapping

extern char ISPSMTPName[];
extern int ISPSMTPPort;

extern char ISPPOP3Name[];
extern int ISPPOP3Port;

extern char ISPAccountName[];
extern char ISPAccountPass[];
extern char EncryptedISPAccountPass[];
extern int EncryptedPassLen;

*/

#define NUMBEROFBUFFERS 100

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers


extern LINK *link_hd;
extern CIRCUIT *circuit_hd ;			// This is a chain of RT circuits. There may be others
extern char OurNode[];
extern char OurAlias[];
extern BOOL SMTPMsgCreated;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	int BPQStream, n;
	
	UNREFERENCED_PARAMETER(hPrevInstance);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BPQMailChat, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BPQMailChat));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	for (n = 0; n < NumberofStreams; n++)
	{
		BPQStream=Connections[n].BPQStream;
		
		if (BPQStream)
		{
			SetAppl(BPQStream, 0, 0);
			Disconnect(BPQStream);
			DeallocateStream(BPQStream);
		}
	}

	SaveUserDatabase();
	SaveMessageDatabase();
	SaveBIDDatabase();


	if (cfgMinToTray) DeleteTrayMenuItem(MainWnd);

	// Free all allocated memory

	for (n = 0; n <= NumberofUsers; n++)
	{
		if (UserRecPtr[n]->ForwardingInfo) free(UserRecPtr[n]->ForwardingInfo); 
		free(UserRecPtr[n]);
	}
	
	free(UserRecPtr);

	for (n = 0; n < NumberofMessages; n++)
		free(MsgHddrPtr[n]);

	free(MsgHddrPtr);

	for (n = 0; n <= NumberofBIDs; n++)
		free(BIDRecPtr[n]);

	free(BIDRecPtr);

	free(OtherNodes);

	FreeChatMemory();


	_CrtDumpMemoryLeaks();

	
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

#define BGCOLOUR RGB(236,233,216)

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	HBRUSH bgBrush;


	bgBrush = CreateSolidBrush(BGCOLOUR);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= DLGWINDOWEXTRA;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BPQMailChat));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= bgBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BPQMailChat);
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
   WSADATA WsaData;


   hInst = hInstance;

   hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   MainWnd=hWnd;

   CheckTimer();

   SetTimer(hWnd,1,10000,NULL);		// BPQ TImer Check
   SetTimer(hWnd,2,100,NULL);		// Send to Node

	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

   UpdateWindow(hWnd);

   WSAStartup(MAKEWORD(2, 0), &WsaData);
    
   return Initialise();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int state,change;
	ConnectionInfo * conn;

	if (message == BPQMsg)
	{
		if (lParam & BPQMonitorAvail)
		{
			DoMonitorData(wParam);
			return 0;
		}
		if (lParam & BPQDataAvail)
		{
			DoReceivedData(wParam);
			return 0;
		}
		if (lParam & BPQStateChange)
		{
			//	Get current Session State. Any state changed is ACK'ed
			//	automatically. See BPQHOST functions 4 and 5.
	
			SessionState(wParam, &state, &change);
		
			if (change == 1)
			{
				if (state == 1)
	
				// Connected
			
					Connected(wParam);	
				else
					Disconnected(wParam);
			}
		}

		return 0;
	}


	switch (message)
	{

	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(wParam);
		return 0;

	case WSA_CONNECT: /* Notification if a socket connection completed. */

		Socket_Connect(wParam, WSAGETSELECTERROR(lParam));
		return 0;


			
	case WM_TIMER:

		if (wParam == 1)
		{
			ShowConnections();
			CheckTimer();
			TCPTimer();
		}
		else
			TrytoSend();

		return (0);

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmId > IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			conn=&Connections[wmId-IDM_DISCONNECT];
		
			if (conn->Active)
			{	
				return 0;
			}
		}

		switch (wmId)
		{
		case IDM_LOGGING:

			// Toggle Logging Flag

			LogEnabled = !LogEnabled;
			CheckMenuItem(hLogMenu,0,MF_BYPOSITION | LogEnabled<<3);

			break;


		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_CONFIG:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
			break;


		case IDM_EXIT:
			DestroyWindow(hWnd);
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

		KillTimer(hWnd,1);
		KillTimer(hWnd,2);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
			return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

int ShowConnections()
{
	char msg[80];
	ConnectionInfo * conn;
	int i,n;

	SendDlgItemMessage(MainWnd,100,LB_RESETCONTENT,0,0);

	for (n = 0; n < NumberofStreams; n++)
	{
		conn=&Connections[n];

		if (!conn->Active)
		{
			strcpy(msg,"Idle");
		}
		else
		{
			if (conn->UserPointer == 0)
				strcpy(msg,"Logging in");
			else
			{
				i=wsprintf(msg, "%-15s %-10s %2d %2d %6d",
					conn->UserPointer->Name, conn->UserPointer->Call, conn->BPQStream,
					conn->Conference, conn->OutputQueueLength - conn->OutputGetPointer);
			}
		}
		SendDlgItemMessage(MainWnd,100,LB_ADDSTRING,0,(LPARAM)msg);
	}

	return 0;
}

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

SOCKADDR_IN local_sin;  /* Local socket - internet style */

PSOCKADDR_IN psin;

SOCKET sock;

BOOL Initialise()
{
	int i, ptr, len;
	ConnectionInfo * conn;
//	BOOL	ReturnValue = TRUE; // return value

	HMENU hMenu;		// handle of menu 
	HKEY hKey=0;
	char * ptr1;

	//	Register message for posting by BPQDLL

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	if (!GetConfigFromRegistry())
		return FALSE;

	// Set up file and directory names
		
	strcpy(UserDatabasePath, BaseDir);
	strcat(UserDatabasePath, "\\");
	strcat(UserDatabasePath, UserDatabaseName);

	strcpy(MsgDatabasePath, BaseDir);
	strcat(MsgDatabasePath, "\\");
	strcat(MsgDatabasePath, MsgDatabaseName);

	strcpy(BIDDatabasePath, BaseDir);
	strcat(BIDDatabasePath, "\\");
	strcat(BIDDatabasePath, BIDDatabaseName);

	strcpy(MailDir, BaseDir);
	strcat(MailDir, "\\");
	strcat(MailDir, "MAIL");

	strcpy(RtUsr, BaseDir);
	strcat(RtUsr, "\\ChatUsers.txt");

	strcpy(RtUsrTemp, BaseDir);
	strcat(RtUsrTemp, "\\ChatUsers.tmp");

	BBSApplMask = 1<<(BBSApplNum-1);
	ChatApplMask = 1<<(ChatApplNum-1);

	// Build buffer pool
	
	for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
	{	
		ReleaseBuffer(&BufferPool[100*i]);
	}


	ptr1=GetApplCall(ChatApplNum);
	memcpy(OurNode, ptr1, 10);
	strlop(OurNode, ' ');

	ptr1=GetApplAlias(ChatApplNum);
	memcpy(OurAlias, ptr1,10);
	strlop(OurAlias, ' ');

	GetUserDatabase();

	GetMessageDatabase();

	GetBIDDatabase();

	// Set up other nodes list. rtlink messes with the string so pass copy
	
	ptr=0;

	while (OtherNodes[ptr])
	{
		len=strlen(&OtherNodes[ptr]);		
		rtlink(_strdup(&OtherNodes[ptr]));			
		ptr+= (len + 1);
	}

	//GetFileList(MailDir);
	 
	// Allocate Streams

	for (i=0; i < NumberofStreams; i++)
	{
		conn = &Connections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, 2, BBSApplMask | ChatApplMask);
		Disconnect(conn->BPQStream);

	}

	InitialiseTCP();


	if (cfgMinToTray)
	
		AddTrayMenuItem(MainWnd, "Mail/Chat Server");

	// Get handles fou updating "Disconnect User" menu items

	hMenu=GetMenu(MainWnd);
	hActionMenu=GetSubMenu(hMenu,0);

	hLogMenu=GetSubMenu(hActionMenu,0);

	hDisMenu=GetSubMenu(hActionMenu,1);

	CheckMenuItem(hLogMenu,0,MF_BYPOSITION | LogEnabled<<3);

	StartForwarding (UserRecPtr[4]);

	return TRUE;
}

int Connected(Stream)
{
	int n, Mask;
	CIRCUIT * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, sesstype, paclen, maxframe, l4window;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (Stream == conn->BPQStream)
		{
			if (conn->Active)
			{
				// Probably an outgoing connect

				if (conn->BBSFlags & Connecting)
				{
					// BBS Outgoing Connect

					conn->paclen = 128;
					nprintf(conn, "c %s\r", conn->Callsign);
					return 0;
				}
	
				
				if (conn->flags == p_linkini)
				{
					conn->paclen = 128;
					nprintf(conn, "c %s\r", conn->u.link->call);
					return 0;
				}
			}
	
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->Active = TRUE;
			conn->BPQStream = Stream;

			GetConnectionInfo(Stream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);

			strlop(callsign, ' ');		// Remove trailing spaces

			memcpy(conn->Callsign, callsign, 10);

			strlop(callsign, '-');		// Remove any SSID

			user = LookupCall(callsign);

			if (user == NULL)
			{
				user = AllocateUserRecord(callsign);

				if (user == NULL) return 0; //		Cant happen??
			}

			time(&user->TimeLastCOnnected);

			conn->UserPointer = user;

			conn->lastmsg = user->lastmsg;

			if (paclen == 0)
				paclen = 236;
			
			conn->paclen=paclen;

			//	Set SYSOP flag if user is defined as SYSOP and Host Session 
			
			if (((sesstype & 0x20) == 0x20) && user->sysop)
				conn->sysop = TRUE;

			Mask = 1 << (GetApplNum(Stream) - 1);

			// Send SID and Prompt

			if (Mask == ChatApplMask)
			{
				conn->Flags |= CHATMODE;

				SendMsg(Stream, ChatSID, strlen(ChatSID));
			}
			else
			{
				SendMsg(Stream, BBSSID, strlen(BBSSID));
			}

			if (user->Name[0] == 0)
			{
				conn->Flags |= GETTINGUSER;
				SendMsg(Stream, NewUserPrompt, strlen(NewUserPrompt));
			}
			else
				SendWelcomeMsg(Stream, conn, user);

			ShowConnections();
			
			return 0;
		}
	}

	return 0;
}

int Disconnected (Stream)
{
	struct UserInfo * user = NULL;
	ConnectionInfo * conn;
	int n;
//	char Msg[255];
//	int len;

	for (n = 0; n <= NumberofStreams-1; n++)
	{
		conn=&Connections[n];

		if (Stream == conn->BPQStream)
		{
			if (conn->Active == FALSE)
				return 0;

			ClearQueue(conn);
		
			conn->Active = FALSE;
			ShowConnections();

			if (conn->Flags & CHATMODE)
			{
				if (conn->Flags & CHATLINK)
					link_drop(conn);
				else
				{
					logout(conn);
				//	user = Connections[n].UserPointer;
				//	len = wsprintf(Msg, "%s - %s Logged off Channel %d\r", user->Call, user->Name, Connections[n].Conference);
				//	SendtoOtherUsers(&Connections[n], Msg, len);
				}
			}

			if (conn->FBBHeaders)
				free(conn->FBBHeaders);

			return 0;
		}
	}

	return 0;
}

int DoReceivedData(int Stream)
{
	int count, InputLen;
	UINT MsgLen;
	int n;
	ConnectionInfo * conn;
	struct UserInfo * user;
	char * ptr, * ptr2;
	char Buffer[512];

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];

		if (Stream == conn->BPQStream)
		{
			do
			{ 
				// May have several messages per packet, or message split over packets

				if (conn->InputLen > 256)	// Shouldnt have lines longer  than this in text mode
				{
					conn->InputLen=0;
				}
				
				GetMsg(Stream, &conn->InputBuffer[conn->InputLen], &InputLen, &count);

				if (InputLen == 0) return 0;

				conn->InputLen += InputLen;
			loop:
				ptr = memchr(conn->InputBuffer, '\r', conn->InputLen);

				if (ptr)	//  CR in buffer
				{
					user = conn->UserPointer;
				
					ptr2 = &conn->InputBuffer[conn->InputLen];
					
					if (++ptr == ptr2)
					{
						// Usual Case - single meg in buffer
	
	
						if (conn->flags == p_linkini)		// Chat Connect
							ProcessConnecting(conn, conn->InputBuffer);
						else if (conn->BBSFlags & Connecting)
							ProcessBBSConnecting(conn, conn->InputBuffer, conn->InputLen);
						else
							ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);

						conn->InputLen=0;
					}
					else
					{
						// buffer contains more that 1 message

						MsgLen = conn->InputLen - (ptr2-ptr);

						memcpy(Buffer, conn->InputBuffer, MsgLen);

						if (conn->flags == p_linkini)
							ProcessConnecting(conn, Buffer);
						else if (conn->BBSFlags & Connecting)
							ProcessBBSConnecting(conn, Buffer, MsgLen);
						else
							ProcessLine(conn, user, Buffer, MsgLen);

						memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

						conn->InputLen -= MsgLen;

						goto loop;

					}
				}

			} while (count > 0);

			return 0;
		}
	}

	// Socket not found

	return 0;

}
int DoMonitorData(int Stream)
{
	UCHAR Buffer[1000];
	UCHAR buff[500];

	int n,len,count=0;
	int stamp;

	for (n = 0; n < NumberofStreams; n++)
	{
		if (Stream == Connections[n].BPQStream)
		{
			do { 

			stamp=GetRaw(Stream, buff,&len,&count);
			len=DecodeFrame(buff,Buffer,stamp);

			if (len == 0) return 0;
				
			}
			while (len>0);
		}


		while (count > 0);	
 
	}

	return 0;

}
int ConnectState(Stream)
{
	int state;

	SessionStateNoAck(Stream, &state);
	return state;
}
UCHAR * EncodeCall(UCHAR * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}


int WriteLog(char * msg)
{
	FILE *file;
	char timebuf[128];

	time_t ltime;
    struct tm today;
 
	if ((file = fopen("BPQTelnetServer.log","a")) == NULL)
		return FALSE;

	time(&ltime);

	_localtime32_s( &today, &ltime );

	strftime( timebuf, 128,
		"%d/%m/%Y %H:%M:%S ", &today );
    
	fputs(timebuf, file);

	fputs(msg, file);

	fclose(file);

	return 0;
}

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

VOID __cdecl nodeprintf(ConnectionInfo * conn, const char * format, ...)
{
	char Mess[255];
	int len;
	va_list(arglist);

	
	va_start(arglist, format);
	len = vsprintf(Mess, format, arglist);

	QueueMsg(conn, Mess, len);

	return;
}

struct UserInfo * AllocateUserRecord(char * Call)
{
	UserRecPtr=realloc(UserRecPtr,(++NumberofUsers+1)*4);

	UserRecPtr[NumberofUsers]= malloc(sizeof (struct UserInfo));

	memset(UserRecPtr[NumberofUsers], 0, sizeof (struct UserInfo));

	strcpy(UserRecPtr[NumberofUsers]->Call, Call);

	return UserRecPtr[NumberofUsers];
}

struct MsgInfo * AllocateMsgRecord()
{
	MsgHddrPtr=realloc(MsgHddrPtr,(NumberofMessages+1)*4);

	MsgHddrPtr[NumberofMessages]= malloc(sizeof (struct MsgInfo));

	memset(MsgHddrPtr[NumberofMessages], 0, sizeof (struct MsgInfo));

	return MsgHddrPtr[NumberofMessages++];
}

BIDRec * AllocateBIDRecord()
{
	BIDRecPtr=realloc(BIDRecPtr,(++NumberofBIDs+1)*4);

	BIDRecPtr[NumberofBIDs]= malloc(sizeof (BIDRec));

	memset(BIDRecPtr[NumberofBIDs], 0, sizeof (BIDRec));

	return BIDRecPtr[NumberofBIDs];
}


struct UserInfo * LookupCall(char * Call)
{
	struct UserInfo * ptr = NULL;
	int i;

	for (i=1; i <= NumberofUsers; i++)
	{
		ptr = UserRecPtr[i];

		if (_stricmp(ptr->Call, Call) == 0) return ptr;

	}

	return NULL;
}

VOID GetUserDatabase()
{
	struct UserInfo UserRec;
	HANDLE Handle;
	int ReadLen;
	struct UserInfo * user;

	Handle = CreateFile(UserDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		UserRecPtr=malloc(4);
		UserRecPtr[0]= malloc(sizeof (struct UserInfo));
		memset(UserRecPtr[0], 0, sizeof (struct UserInfo));
		NumberofUsers = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &UserRec, sizeof (UserRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&UserRec, 0, sizeof (struct UserInfo));
	}

	// Set up control record

	UserRecPtr=malloc(4);
	UserRecPtr[0]= malloc(sizeof (struct UserInfo));
	memcpy(UserRecPtr[0], &UserRec,  sizeof (UserRec));

	NumberofUsers = 0;

Next:

	ReadFile(Handle, &UserRec, sizeof (UserRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		user = AllocateUserRecord(UserRec.Call);
		memcpy(user, &UserRec,  sizeof (UserRec));

		if (user->flags & F_BBS)
		{
			// Defined as BBS - allocate and initialise forwarding structure

			SetupForwardingStruct(user);

			// Add to BBS Chain;

			user->BBSNext = BBSChain;
			BBSChain = user;

		}
		goto Next;
	}

	CloseHandle(Handle);

	
}
VOID SaveUserDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;
	char Backup[MAX_PATH];

	strcpy(Backup, UserDatabasePath);
	strcat(Backup, ".BAK");

	CopyFile(UserDatabasePath, Backup, FALSE);

	Handle = CreateFile(UserDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	UserRecPtr[0]->nbcon = NumberofUsers;

	for (i=0; i <= NumberofUsers; i++)
	{
		WriteFile(Handle, UserRecPtr[i], sizeof (struct UserInfo), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

VOID GetMessageDatabase()
{
	struct MsgInfo MsgRec;
	HANDLE Handle;
	int ReadLen;
	struct MsgInfo * Msg;
	struct UserInfo * user;
	char zeros[NBMASK];

	memset(zeros, 0, NBMASK);

	Handle = CreateFile(MsgDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
		return;

Next:

	ReadFile(Handle, &MsgRec, sizeof (MsgRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		Msg = AllocateMsgRecord();
		memcpy(Msg, &MsgRec,  sizeof (MsgRec));

		// If any forward bits are set, increment count on corresponding BBS record.

		if (memcmp(MsgRec.fbbs, zeros, NBMASK) != 0)
		{
			for (user = BBSChain; user; user = user->BBSNext)
			{
				if (check_fwd_bit(MsgRec.fbbs, user->BBSNumber))
				{
					user->ForwardingInfo->MsgCount++;
				}
			}
		}

		goto Next;
	}

	if (NumberofMessages == 0)
		LatestMsg=0;
	else
		LatestMsg=MsgHddrPtr[NumberofMessages-1]->number;

	CloseHandle(Handle);

}

VOID SaveMessageDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;


	Handle = CreateFile(MsgDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	MsgHddrPtr[0]->number = NumberofMessages-1;

	for (i=0; i < NumberofMessages; i++)
	{
		WriteFile(Handle, MsgHddrPtr[i], sizeof (struct MsgInfo), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

VOID GetBIDDatabase()
{
	BIDRec BIDRec;
	HANDLE Handle;
	int ReadLen;
	BIDRecP BID;

	Handle = CreateFile(BIDDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		BIDRecPtr=malloc(4);
		BIDRecPtr[0]= malloc(sizeof (BIDRec));
		memset(BIDRecPtr[0], 0, sizeof (BIDRec));
		NumberofBIDs = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &BIDRec, sizeof (BIDRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&BIDRec, 0, sizeof (BIDRec));
	}

	// Set up control record

	BIDRecPtr=malloc(4);
	BIDRecPtr[0]= malloc(sizeof (BIDRec));
	memcpy(BIDRecPtr[0], &BIDRec,  sizeof (BIDRec));

	NumberofBIDs = 0;

Next:

	ReadFile(Handle, &BIDRec, sizeof (BIDRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		BID = AllocateBIDRecord();
		memcpy(BID, &BIDRec,  sizeof (BIDRec));
		goto Next;
	}

	CloseHandle(Handle);

	
}
VOID SaveBIDDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;

	Handle = CreateFile(BIDDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	BIDRecPtr[0]->msgno = NumberofBIDs-1;			// First Record has file size

	for (i=0; i < NumberofBIDs; i++)
	{
		WriteFile(Handle, BIDRecPtr[i], sizeof (BIDRec), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

BIDRec  * LookupBID(char * BID)
{
	BIDRec * ptr = NULL;
	int i;

	for (i=1; i < NumberofBIDs; i++)
	{
		ptr = BIDRecPtr[i];

		if (_stricmp(ptr->BID, BID) == 0) return ptr;
	}

	return NULL;
}


VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user)
{
	LINK    *link;
//	char Msg[255];
//	int len;

	if (conn->Flags & CHATMODE)
	{	
		// See if from a defined node
		
		for (link = link_hd; link; link = link->next)
		{
			if (matchi(conn->Callsign, link->call))
			{
				conn->flags = p_linkwait;
				return;						// Wait for *RTL
			}
		}

		// Not a defined node - assume it's a user

		if (!rtloginu (conn))
		{
			// Already connected - close
			
			Sleep(1000);
			Disconnect(conn->BPQStream);
		}
		return;
	}


		//nodeprintf(conn, "Hello %s\rWelcome to %s Chat Server (/H for help) >\r", user->Name, BBSName);
	
	nodeprintf(conn, "Hello %s. Latest Message is %d, Last listed is %d\r%s BBS (H for help) >\r",
				user->Name, LatestMsg, user->lastmsg, BBSName);

//	if (conn->Flags & CHATMODE)
//	{
//		len = wsprintf(Msg, "%s - %s Logged on to Channel %d\r", user->Call, user->Name, conn->Conference);
//		SendtoOtherUsers(conn, Msg, len);
//	}

}

VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user)
{
	nodeprintf(conn, "%s", Prompt);
}


VOID ProcessLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len)
{
	char * Cmd, * Arg1;
	char * Context;
	char seps[] = " \t\r";
	int CmdLen;

	if (conn->BBSFlags & FBBForwarding)
	{
		ProcessFBBLine(conn, user, Buffer, len);
		return;
	}

	if (conn->Flags & GETTINGMESSAGE)
	{
		ProcessMsgLine(conn, user, Buffer, len);
		return;	}

	if (conn->Flags & GETTINGTITLE)
	{
		ProcessMsgTitle(conn, user, Buffer, len);
		return;
	}

	if (conn->Flags & GETTINGUSER)
	{
		memcpy(user->Name, Buffer, len-1);
		conn->Flags &=  ~GETTINGUSER;
		SendWelcomeMsg(conn->BPQStream, conn, user);
		return;
	}


	if (conn->Flags & CHATMODE)
	{
		ProcessChatLine(conn, user, Buffer, len);
		return;
	}

	// Process Command

	if (len == 1)
	{
		SendPrompt(conn, user);
		return;
	}

	Buffer[len] = 0;

	if ((conn->BBSFlags & BBS) && (_stricmp(Buffer, "F>\r") == 0))
	{
		// Reverse forward request

		Disconnect(conn->BPQStream);
		return;
	}

	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		// If a BBS, set BBS Flag

		if (user->flags & F_BBS)
		{
			Parse_SID(conn, &Buffer[1], len-4);
			
			if (conn->BBSFlags & FBBForwarding)
			{
				conn->FBBIndex = 0;		// ready for first block;
				conn->FBBChecksum = 0;
			}
			else
				nputs(conn, ">\r");

			return;
		}
	}
	Cmd = strtok_s(Buffer, seps, &Context);
	Arg1 = strtok_s(NULL, seps, &Context);
	CmdLen = strlen(Cmd);

	if (_memicmp(Cmd, "Abort", 1) == 0)
	{
		ClearQueue(conn);
		QueueMsg(conn, AbortedMsg, strlen(AbortedMsg));
	}
	if (_memicmp(Cmd, "Bye", CmdLen) == 0)
	{
		SendMsg(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
		user->lastmsg = conn->lastmsg;
		Sleep(1000);
		Disconnect(conn->BPQStream);
		return;
	}

	if (_memicmp(Cmd, "K", 1) == 0)
	{
		DoKillCommand(conn, user, Cmd, Arg1, Context);
	}

	if (_memicmp(Cmd, "L", 1) == 0)
	{
		DoListCommand(conn, user, Cmd, Arg1);
	}

	if (_memicmp(Cmd, "Name", CmdLen) == 0)
	{
		if (Arg1)
			strcpy(user->Name, Arg1);
		SendWelcomeMsg(conn->BPQStream, conn, user);
	}

	if (_memicmp(Cmd, "R", 1) == 0)
	{
		DoReadCommand(conn, user, Cmd, Arg1, Context);
	}

	if (_memicmp(Cmd, "S", 1) == 0)
	{
		DoSendCommand(conn, user, Cmd, Arg1, Context);
	}

	if (_memicmp(Cmd, "Chat", CmdLen) == 0)
	{
		conn->Flags |= CHATMODE;
		
		if (!rtloginu (conn))
		{
			// Already connected - close
			
			Sleep(1000);
			Disconnect(conn->BPQStream);
		}

	}

	if (conn->Flags == 0)
		SendPrompt(conn, user);

	//	Send if possible

	Flush(conn);

}


VOID ProcessChatLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len)
{
	char * Cmd, * Arg1, * Arg2, * Arg3;
	char * Context;
	char seps[] = " \t\r";
	int CmdLen;
	char Msg[250];
	int msglen, n, NewChan = 0;
	ConnectionInfo * otherconn, *c;
	struct UserInfo * usr;

	Buffer[len] = 0;

	strlop(Buffer, '\r');

	if (conn->flags == p_linkwait)
	{
		//waiting for *RTL

		if ((len <6) && (memcmp(Buffer, "*RTL", 4) == 0))
		{
			// Node - Node Connect

			if (rtloginl (conn, conn->Callsign))
			{
				// Accepted
		
				conn->Flags |= CHATLINK;
				return;
			}
			else
			{
				// Connection refused
			
				Disconnect(conn->BPQStream);
				return;
			}
		}

		if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
			return;

		nprintf(conn, "Unexpected Message on Chat Node-Node Link - Disconnecting\r");
		Flush(conn);
		Sleep(500);
		Disconnect(conn->BPQStream);
		return;


	}


	if (conn->Flags & CHATLINK)
	{
		chkctl(conn, Buffer);
		return;
	}

	if(conn->u.user == NULL)
		return;									// A node link, but not activated yet

//	if ((len > 1) && Buffer[0] == '/')
	if (Buffer[0] == '/')
	{

		// Process Command

//		Cmd = strtok_s(&Buffer[1], seps, &Context);

//		if (Cmd == NULL) goto NullCmd;

//		CmdLen = strlen(Cmd);

		if (_memicmp(&Buffer[1], "Bye", 1) == 0)
		{
			SendMsg(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
			ReturntoNode(conn->BPQStream);
			return;
		}

		if (_memicmp(&Buffer[1], "Quit", 4) == 0)
		{
			SendMsg(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
			Sleep(1000);
			Disconnect(conn->BPQStream);
			return;
		}

		rt_cmd(conn, Buffer);

		return;
/*
		if ((_memicmp(Cmd, "Help", CmdLen) == 0) || Cmd[0] == '?')
		{
			nodeprintf(conn, "BPQChat version 1.00 available commands are :-\r\r/? or /H - To read this list)\r");
			nodeprintf(conn, "/B ------- To leave CHAT and return to the BPQ Node\r");
			nodeprintf(conn, "/C n ----- To switch to Conference stream n (0 - 32)\r");
			nodeprintf(conn, "/L Loc --- To register your QTH/Locator (max 30 chars)\r");
			nodeprintf(conn, "/Q ------- To leave CHAT and disconnect from BPQ Node\r");
			nodeprintf(conn, "/W ------- To list connected users\r");

			return;

		}

/*
BPQChat version 1.00 available commands are :-

/? or /H - To read this list
/n text -- Send a line of text only to a specific user on Channel n (0 > (10-1))
/A ------- Toggle linefeed after [callsign:name] option On/Off
/B ------- To leave FASTCHAT and return to the BPQ Node
/C ------- Show the current Time and Date (PC Clock)
/C n ----- To switch to Conference stream n (0 > 32)
/D ------- Show connection status (conference channel etc.. your on)
/E ------- Toggle expert user status On/Off (Expert=no ctext)
/F ------- Turn On/Off your bell filter
/I ------- Shows information about FASTCHAT 4.7b and the station plus more help.
/L Loc --- To register your QTH/Locator (max 35 chars)
/N Name -- To register onto John's FASTCHAT.
/Q ------- To disconnect from FASTCHAT 4.7b and BPQ Node completely
/S ------- Shows sysop information or wether John is available
/S Text -- Send a line of text to John the Sysop only
/T ------- Wake John the Sysop up (not at night please)
/U ------- List registered John's FASTCHAT users
/U Call -- Show details on a registered user
/W ------- To list logged in registered users
*/

		if (_memicmp(Cmd, "location", CmdLen) == 0)
		{
			// Change Address info

			if (strlen(Context) > 0)
			{
				Context[strlen(Context) - 1] = 0;
				if (strlen(Context) > 30) Context[30] = 0;
				strcpy(conn->UserPointer->City, Context);
			}

			nodeprintf(conn, "Location is %s\r", conn->UserPointer->City);

			return;

		}

		if (_memicmp(Cmd, "w", CmdLen) == 0)
		{
			// Show Users

			nodeprintf(conn, "User       User            User                      Port Conf  On\r");
			nodeprintf(conn, "Name       Call    QTH/Location/Locator               No.  No.  DD/MM\r");

			for (n = 0; n < NumberofStreams; n++)
			{
				otherconn = &Connections[n];
		
				if ((otherconn->Active) && (otherconn->Flags == CHATMODE))
				{
					usr = otherconn->UserPointer;

					nodeprintf(conn, "%-10s %-10s %-31s %2d   %2d   %02d/%02d\r",
								usr->Name, usr->Call, usr->City,
								otherconn->BPQStream, otherconn->Conference, 0, 0);

				}
			}

			nodeprintf(conn, "End of User List..\r");

			return;

		}

		Arg1 = strtok_s(NULL, seps, &Context);
		Arg2 = strtok_s(NULL, seps, &Context);
		Arg3 = strtok_s(NULL, seps, &Context);

		if (_memicmp(Cmd, "c", CmdLen) == 0)
		{
			// Change Conference Channel

			if (Arg1)
			{
				NewChan = atoi(Arg1);

				if (NewChan > 0 &&  NewChan < 33)
				{
					conn->Conference = NewChan;
					ShowConnections();
				}
			}
			nodeprintf(conn, "You are now on Conference channel %d\r", conn->Conference);
			nodeprintf(conn, "Stations using channel %d = %s\r", conn->Conference, GetConfStations(conn->Conference));

			msglen = wsprintf(Msg, "%s - %s has changed to conference channel %d\r", user->Call, user->Name, conn->Conference);
			SendtoOtherUsers(conn, Msg, msglen);

			return;

		}

//Stations using channel 3 = G8BPQ GM8BPQ 
/*
		/h
********* Bill G7LSQ's wonderful Conferencing Node - FSCHAT:G7LSQ-9 *********
****************** Located in Bryanston,Blandford,Dorset ********************
***************** A FASTPAK Research And Development node *******************

FASTCHAT version 4.7b available commands are :-

/? or /H - To read this list
/n text -- Send a line of text only to a specific user on Channel n (0 > (10-1))
/A ------- Toggle linefeed after [callsign:name] option On/Off
/B ------- To leave FASTCHAT and return to the BPQ Node
/C ------- Show the current Time and Date (PC Clock)
/C n ----- To switch to Conference stream n (0 > 32)
/D ------- Show connection status (conference channel etc.. your on)
/E ------- Toggle expert user status On/Off (Expert=no ctext)
/F ------- Turn On/Off your bell filter
/I ------- Shows information about FASTCHAT 4.7b and the station plus more help.
/L Loc --- To register your QTH/Locator (max 35 chars)
/N Name -- To register onto John's FASTCHAT.
/Q ------- To disconnect from FASTCHAT 4.7b and BPQ Node completely
/S ------- Shows sysop information or wether John is available
/S Text -- Send a line of text to John the Sysop only
/T ------- Wake John the Sysop up (not at night please)
/U ------- List registered John's FASTCHAT users
/U Call -- Show details on a registered user
/W ------- To list logged in registered users


NullCmd:
*/
		nodeprintf(conn, "Unrecognised command  - type /? for help\r");

		

		return;
	}

	// Send message to all other connected users on same channel

	if (len > 200)
	{
		Buffer[200] = '\r';
		Buffer[201] = 0;
		len = 200;
	}

/*	msglen = wsprintf(Msg, "[%d:%s:%s] %s", conn->BPQStream, conn->UserPointer->Call, conn->UserPointer->Name, Buffer);


	for (n = 0; n < NumberofStreams; n++)
	{
		otherconn = &Connections[n];
		
		if ((otherconn->Active) && (otherconn->Flags == CHATMODE) && (otherconn->Conference == conn->Conference) && conn != otherconn)
		{
			QueueMsg(otherconn, Msg, msglen);
		}
	}
*/
		
	text_tellu(conn->u.user, Buffer, NULL, o_topic); // To local users.
		
	// Send to Linked nodes

	for (c = circuit_hd; c; c = c->next)
		if ((c->flags & p_linked) && c->refcnt && ct_find(c, conn->u.user->topic))
			nprintf(c, "%c%c%s %s %s\r",
				FORMAT, id_data, OurNode, conn->u.user->call, Buffer);


}

/*
BPQ:GM8BPQ-2} Connected to CHAT
[FASTCHAT - 4.7b]
 Hi John,
John's Chat server, (FASTCHAT 4.7b), You are logged on channel 1 ,
Conference 0, total users allowed = 10 , Currently logged in users = 2
Current system time = 17:48:17.

hello
[0:G8BPQ:Harry] hello
G8BPQ - Harry has changed to conference channel 3
/c 3
You are now on Conference channel 3
Stations using channel 3 = G8BPQ GM8BPQ 
sadssdsdsda
G8BPQ - Harry logged off chan No. 0 - 30/04/2009  17:50:33
G8BPQ - Harry Logged on chan No. 0 - 30/04/2009  17:50:35 
*/

int QueueMsg(ConnectionInfo * conn, char * msg, int len)
{
	// Add Message to queue for this connection

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	// Create or extend buffer

	conn->OutputQueue=realloc(conn->OutputQueue, conn->OutputQueueLength + len);

	if (conn->OutputQueue == NULL)
	{
		// relloc failed - should never happen, but clean up

		CriticalErrorHandler("realloc failed to expand output queue");
		return 0;
	}

	memcpy(&conn->OutputQueue[conn->OutputQueueLength], msg, len);

	conn->OutputQueueLength+=len;

	return len;
}

void TrytoSend()
{
	// call Flush on any connected streams with queued data

	ConnectionInfo * conn;
	int n;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active == TRUE)
			if (conn->OutputQueue)
				Flush(conn);
	}
}


void Flush(ConnectionInfo * conn)
{
	int tosend, len, sent;
	
	// Try to send data to user. May be stopped by user paging or node flow control

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	if (conn->OutputQueue == NULL)
		return;						// Nothing to send

	tosend = conn->OutputQueueLength - conn->OutputGetPointer;

	sent=0;

	while (tosend > 0)
	{
		if (TXCount(conn->BPQStream) > 4)
			return;						// Busy
	
		if (tosend <= conn->paclen)
			len=tosend;
		else
			len=conn->paclen;

		SendMsg(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);

		conn->OutputGetPointer+=len;

		tosend-=len;
	
		sent++;

		if (sent > 4)
			return;

	}

	// All Sent. Free buffers and reset pointers

	ClearQueue(conn);
}

VOID ClearQueue(ConnectionInfo * conn)
{
	if (conn->OutputQueue == NULL)
		return;

	free(conn->OutputQueue);

	conn->OutputQueue=NULL;
	conn->OutputGetPointer=0;
	conn->OutputQueueLength=0;
}

char * GetConfStations(int Conference)
{
	static char Stns[1000] = "";
	int n;
	ConnectionInfo * otherconn;

	Stns[0] = 0;

	for (n = 0; n < NumberofStreams; n++)
	{
		otherconn = &Connections[n];
		
		if ((otherconn->Active) && (otherconn->Flags == CHATMODE) && (otherconn->Conference == Conference))
		{
			strcat(Stns, otherconn->UserPointer->Call);
			strcat(Stns, " ");
		}
	}

	return Stns;
}

VOID SendtoOtherUsers(ConnectionInfo * conn, char* Msg, int msglen)
{
	int n;
	ConnectionInfo * otherconn;

	for (n = 0; n < NumberofStreams; n++)
	{
		otherconn = &Connections[n];
		
		if ((otherconn->Active) && (otherconn->Flags == CHATMODE) && conn != otherconn)
		{
			QueueMsg(otherconn, Msg, msglen);
		}
	}
}


/*
int GetFileList(char * Dir)
{
   WIN32_FIND_DATA ffd;
   LARGE_INTEGER filesize;
   TCHAR szDir[MAX_PATH];
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
   
 
   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   StringCchCopy(szDir, MAX_PATH, Dir);
   StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
   {
      return dwError;
   } 
   
   // List all the files in the directory with some info about them.

   do
   {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         OutputDebugString(ffd.cFileName);
      }
      else
      {
         filesize.LowPart = ffd.nFileSizeLow;
         filesize.HighPart = ffd.nFileSizeHigh;
         OutputDebugString(ffd.cFileName);
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
 
   dwError = GetLastError();

   FindClose(hFind);
   return dwError;
}
*/


void DoKillCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;
	
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just K

		if (Arg1)
		{
			msgno=atoi(Arg1);
			KillMessage(conn, user, msgno);
		}

		return;

	case 'M':					// Kill Mine

		for (i=NumberofMessages-1; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if ((_stricmp(Msg->to, user->Call) == 0)&& (Msg->status == 'Y'))
			{
				Msg->status='K';
				Msg->datechanged=time(NULL);

				nodeprintf(conn, "Message #%d Killed\r", Msg->number);
			}
		}
	}
	return;

}

void KillMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;

	Msg = FindMessage(user->Call, msgno, conn->sysop);

	if (Msg == NULL)
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	Msg->status='K';
	Msg->datechanged=time(NULL);

	nodeprintf(conn, "Message #%d Killed\r", msgno);

}


VOID ListMessage(struct MsgInfo * Msg, ConnectionInfo * conn)
{
	if (Msg->to[0] == 0 && Msg->via[0] != 0)
		nodeprintf(conn, "%-6d %s %c%c   %5d %-6s %-6s %-61s\r",
				Msg->number, FormatDateAndTime(Msg->datesd, TRUE), Msg->type, Msg->status, Msg->length, Msg->via, Msg->from, Msg->title);

	else
		if (Msg->via[0] != 0)
			nodeprintf(conn, "%-6d %s %c%c   %5d %-6s@%-6s %-6s %-61s\r",
				Msg->number, FormatDateAndTime(Msg->datesd, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, Msg->via, Msg->from, Msg->title);
	else
		nodeprintf(conn, "%-6d %s %c%c   %5d %-6s        %-6s %-61s\r",
				Msg->number, FormatDateAndTime(Msg->datesd, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, Msg->from, Msg->title);

	if (Msg->number > conn->lastmsg) 
		conn->lastmsg = Msg->number;

}

void DoListCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1)
{
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just L

		if (Arg1)
		{
			// Range nnn-nnn or single value

			char * Arg2, * Arg3;
			char * Context;
			char seps[] = " -\t\r";
			int From=LatestMsg, To=0;

			Arg2 = strtok_s(Arg1, seps, &Context);
			Arg3 = strtok_s(NULL, seps, &Context);

			if (Arg2)
				To = atoi(Arg2);

			if (Arg3)
				From = atoi(Arg3);

			ListMessagesInRange(conn, user, user->Call, From, To);

		}
		else

			ListMessagesInRange(conn, user, user->Call, LatestMsg, conn->lastmsg);

		return;



	case 'L':				// List Last

		if (Arg1)
		{
			int i = atoi(Arg1);
			int m = NumberofMessages-1;
				
			for (; i>0 && m != 0; i--)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					ListMessage(MsgHddrPtr[m], conn);
					m--;
				}
			}
		}
		return;

	case 'M':			// LM - List Mine

		ListMessagesTo(conn, user, user->Call);
		return;

	case '>':			// L> - List to 

		if (Arg1)
			ListMessagesTo(conn, user, Arg1);
		
		return;

	case '<':

		if (Arg1)
			ListMessagesFrom(conn, user, Arg1);
		
		return;

	}
}
	
void ListMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i;

	for (i=NumberofMessages-1; i>0; i--)
	{
		if (_stricmp(MsgHddrPtr[i]->to, Call) == 0)
			ListMessage(MsgHddrPtr[i], conn);
	}


}

void ListMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i;

	for (i=NumberofMessages-1; i>0; i--)
	{
		if (_stricmp(MsgHddrPtr[i]->from, Call) == 0)
			ListMessage(MsgHddrPtr[i], conn);
	}


}
int GetUserMsg(int m, char * Call, BOOL SYSOP)
{
	struct MsgInfo * Msg;
	
	// Get Next (usually backwards) message which should be shown to this user
	//	ie Not Deleted, and not Private unless to or from Call

	do
	{
		Msg=MsgHddrPtr[m];

		if (SYSOP) 
			return m;					// Sysop can list or read anything

		if (Msg->status != 'K')
		{
			if (Msg->type == 'B') return m;

			if (Msg->type == 'P')
			{
				if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
					return m;
			}
		}

		m--;

	} while (m> 0);

	return 0;

}

void ListMessagesInRange(ConnectionInfo * conn, struct UserInfo * user, char * Call, int Start, int End)
{
	int m=NumberofMessages-1;

	struct MsgInfo * Msg;

	if (NumberofMessages == 0)
		return;


	do
	{
		m = GetUserMsg(m, user->Call, conn->sysop);

		if (m < 1)
			return;

		Msg=MsgHddrPtr[m];

		if (Msg->number < End)
			return;

		if (Msg->number <= Start)
			ListMessage(MsgHddrPtr[m], conn);

		m--;

	} while (m> 0);
}



void DoReadCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;

	
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just R

		if (Arg1)
		{
			msgno=atoi(Arg1);
			ReadMessage(conn, user, msgno);
		}

		return;




		case 'M':					// Kill Mine

		for (i=NumberofMessages-1; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if ((_stricmp(Msg->to, user->Call) == 0)&& (Msg->status == 'N'))
			{
				ReadMessage(conn, user, Msg->number);
			}
		}

		return;
	}
	return;


}


void ReadMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;
	char * MsgBytes;

	Msg = FindMessage(user->Call, msgno, conn->sysop);

	if (Msg == NULL)
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	nodeprintf(conn, "From: %s\rTo: %s\rType/Status %c%c\rDate/Time: %s\rBid: %s\rTitle: %s\r",
		Msg->from, Msg->to, Msg->type, Msg->status, FormatDateAndTime(Msg->datesd, FALSE), Msg->bid, Msg->title);

	MsgBytes = ReadMessageFile(msgno);

	if (MsgBytes)
	{
		QueueMsg(conn, MsgBytes, Msg->length);
		free(MsgBytes);

		nodeprintf(conn, "\r\r[End of Message #%d from %s]\r", msgno, Msg->from);

		if (strcmp(user->Call, Msg->to) == 0)
		{
			Msg->status = 'Y';
			Msg->datechanged=time(NULL);
		}


	}
	else
	{
		nodeprintf(conn, "File for Message %d not found\r", msgno);
	}

}
 struct MsgInfo * FindMessage(char * Call, int msgno, BOOL sysop)
 {
	int m=NumberofMessages-1;

	struct MsgInfo * Msg;

	do
	{
		m = GetUserMsg(m, Call, sysop);

		if (m == 0)
			return NULL;

		Msg=MsgHddrPtr[m];

		if (Msg->number == msgno)
			return Msg;

		m--;

	} while (m> 0);

	return NULL;

}

char * ReadMessageFile(int msgno)
{
	int FileSize;
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	char * MsgBytes;
	int ReadLen;
 
	wsprintf(MsgFile, "%s\\mail%d\\m_%06d.mes", MailDir, msgno%10, msgno);
	
	hFile = CreateFile(MsgFile,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	FileSize = GetFileSize(hFile, NULL);

	MsgBytes=malloc(FileSize+1);

	ReadFile(hFile, MsgBytes, FileSize, &ReadLen, NULL); 

	CloseHandle(hFile);

	MsgBytes[FileSize]=0;

	return MsgBytes;


}

/*
r 1378
From         : G8BPQ
To           : G8BPQ
Type/Status  : PN
Date/Time    : 09-May 13:01
Bid          : 1378_G8BPQ
Message #    : 1378
Title        : Error-report
> *** System boot on Sun 01/03/09 00:01 ***
> *** System boot on Tue 05/05/09 14:21 ***
> *** System boot on Sat 09/05/09 11:23 ***
> *** System boot on Sat 09/05/09 11:30 ***
> *** System boot on Sat 09/05/09 11:49 ***
> *** System boot on Sat 09/05/09 12:24 ***
***************



[End of Message #1378 from G8BPQ]
*/

char * FormatDateAndTime(time_t Datim, BOOL DateOnly)
{
	struct tm *newtime;
    char Time[80];
	static char Date[]="xx-xxx hh:mm";
  
	newtime = gmtime(&Datim);
	asctime_s(Time, sizeof(Time), newtime);
	Date[0]=Time[8];
	Date[1]=Time[9];
	Date[3]=Time[4];
	Date[4]=Time[5];
	Date[5]=Time[6];

	if (DateOnly)
	{
		Date[6]=0;
		return Date;
	}

	memcpy(&Date[6], &Time[10], 6);
	
	return Date;
}

void DoSendCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	// SB WANT @ ALLCAN < N6ZFJ $4567_N0ARY
	
	char * From = NULL;
	char * BID = NULL;
	char * ATBBS = NULL;
	char * ptr;
	char seps[] = " \t\r";


	switch (toupper(Cmd[1]))
	{

	case 0:					// Just S means SP
	case 'P':
	case 'B':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: The 'TO' callsign is missing\r");
			return;
		}
		// Look for Optional fields;


		ptr = strtok_s(NULL, seps, &Context);

		while (ptr)
		{
			if (strcmp(ptr, "@") == 0)
			{
				ATBBS = _strupr(strtok_s(NULL, seps, &Context));
			}
			else if(strcmp(ptr, "<") == 0)
			{
				From = strtok_s(NULL, seps, &Context);
			}
			else if (ptr[0] == '$')
				BID = &ptr[1];
			else
			{
				nodeprintf(conn, "*** Error: Invalid Format\r");
				return;
			}

			ptr = strtok_s(NULL, seps, &Context);
		}

		// Only allow < from a BBS

		if (From)
		{
			if (!(conn->BBSFlags & BBS))
			{
				nodeprintf(conn, "*** < can only be used by a BBS\r");
				return;
			}
		}

		if (!From)
			From = user->Call;


		CreateMessage(conn, From, Arg1, ATBBS, toupper(Cmd[1]), BID);
		return;
	}
}

VOID CreateMessage(ConnectionInfo * conn, char * From, char * ToCall, char * ATBBS, char MsgType, char * BID)
{
	struct MsgInfo * Msg;
	char * via;

	// Create a temp msg header entry

	Msg = malloc(sizeof (struct MsgInfo));

	if (Msg == 0)
	{
		CriticalErrorHandler("malloc failed for new message header");
		return;
	}
	
	memset(Msg, 0, sizeof (struct MsgInfo));

	conn->TempMsg = Msg;

	Msg->type = MsgType;
	Msg->status = 'N';
	Msg->date = Msg->datechanged = Msg->datesd = time(NULL);

	if (BID)
	{
		if (LookupBID(BID))
		{
			// Duplicate bid
	
			if (conn->BBSFlags & BBS)
				nodeprintf(conn, "NO - BID\r");
			else
				nodeprintf(conn, "*** Error- Duplicate BID\r");

			return;
		}

		if (strlen(BID) > 12) BID[12] = 0;
		strcpy(Msg->bid, BID);
	}

	if (_memicmp(ToCall, "smtp:", 5) == 0)
	{
		via=strlop(ToCall, ':');
		ToCall[0] = 0;
	}
	else
	{
		_strupr(ToCall);
		via=ATBBS;
		strlop(ToCall, '@');
	}

	if (strlen(ToCall) > 6) ToCall[6] = 0;
	
	strcpy(Msg->to, ToCall);

	if (via)
	{
		if (strlen(via) > 40) via[40] = 0;

		strcpy(Msg->via, via);
	}

	strcpy(Msg->from, From);

	conn->Flags |= GETTINGTITLE;

	if (!(conn->BBSFlags & BBS))
		nodeprintf(conn, "Enter Title (only):\r");
	else
		if (!(conn->BBSFlags & FBBForwarding))
			nodeprintf(conn, "OK\r");
}

VOID ProcessMsgTitle(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int msglen)
{
	if (msglen > 60) msglen = 60;

	Buffer[msglen-1] = 0;

	conn->Flags &= ~GETTINGTITLE;

	strcpy(conn->TempMsg->title, Buffer);

	// Create initial buffer of 10K. Expand if needed later

	conn->MailBuffer=malloc(10000);
	conn->MailBufferSize=10000;

	if (conn->MailBuffer == NULL)
	{
		nodeprintf(conn, "Failed to create Message Buffer\r");
		return;
	}

	conn->Flags |= GETTINGMESSAGE;

	if (!conn->BBSFlags & BBS)
		nodeprintf(conn, "Enter Message Text (end with /ex or ctrl/z)\r");

}

VOID ProcessMsgLine(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int msglen)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;

	if (((msglen < 3) && (Buffer[0] == 0x1a)) || ((msglen == 4) && (_memicmp(Buffer, "/ex", 3) == 0)))
	{
		conn->Flags &= ~GETTINGMESSAGE;

		// Allocate a message Record slot

		Msg = AllocateMsgRecord();
		memcpy(Msg, conn->TempMsg, sizeof(struct MsgInfo));
		
		// Set number here so they remain in sequence
		
		Msg->number = ++LatestMsg;

		// Create BIS if non supplied

		if (Msg->bid[0] == 0)
			wsprintf(Msg->bid, "%d_%s", LatestMsg, BBSName);

		CreateMessageFile(conn, Msg);

		BIDRec = AllocateBIDRecord();

		strcpy(BIDRec->BID, Msg->bid);
		BIDRec->mode = Msg->type;
		BIDRec->msgno = Msg->number;

		// Set up forwarding bitmap

		MatchMessagetoBBSList(Msg);

		if (!(conn->BBSFlags & BBS))
			nodeprintf(conn, "Message: %d Bid:  %s Size: %d\r", Msg->number, Msg->bid, Msg->length);
		else
			if (!(conn->BBSFlags & FBBForwarding))
				nputs(conn, ">\r");
			else
				SetupNextFBBMessage(conn);
		
		if(Msg->to[0] == 0)
			SMTPMsgCreated=TRUE;

		return;
	}

	Buffer[msglen++] = 0x0a;

	if ((conn->TempMsg->length + msglen) > conn->MailBufferSize)
	{
		conn->MailBufferSize += 10000;
		conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
		if (conn->MailBuffer == NULL)
		{
			nodeprintf(conn, "Failed to extend Message Buffer\r");

			conn->Flags &= ~GETTINGMESSAGE;
			return;
		}
	}

	memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, msglen);

	conn->TempMsg->length += msglen;


}

VOID CreateMessageFile(ConnectionInfo * conn, struct MsgInfo * Msg)
{
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int WriteLen=0;
	char Mess[255];
	int len;
   
	wsprintf(MsgFile, "%s\\mail%d\\m_%06d.mes", MailDir, Msg->number%10, Msg->number);
	
	hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, conn->MailBuffer, Msg->length, &WriteLen, NULL);
		CloseHandle(hFile);
	}

	free(conn->MailBuffer);
	conn->MailBufferSize=0;
	conn->MailBuffer=0;

	if (WriteLen != Msg->length)
	{
		len = wsprintf(Mess, "Failed to create Message File\r");
		QueueMsg(conn, Mess, len);
		CriticalErrorHandler(Mess);
	}
	return;
}


void chat_link_out (LINK *link)
{
	int n;
	CIRCUIT * conn;

	for (n = NumberofStreams-1; n > 0 ; n--)
	{
		conn = &Connections[n];
		
		if (conn->Active == FALSE)
		{
			conn->Active = TRUE;
			conn->Flags = CHATMODE | CHATLINK;
			circuit_new(conn,p_linkini);
			conn->u.link = link;

			ConnectUsingAppl(conn->BPQStream, ChatApplMask);

			//	Connected Event will trigger connect to remote system

			return;
		}
	}

	return;
	

}

ProcessConnecting(CIRCUIT * circuit, char * Buffer)
{
	char * Resp;

	if (memcmp(Buffer, "OK\r", 3) == 0)
	{
		circuit->u.link->flags = p_linked;
 	  	circuit->flags = p_linked;
		state_tell(circuit);
		return TRUE;
	}

	
	Resp=strlop(Buffer, '}');
	
	if (Resp)
	if (memcmp(Resp, " Connected", 10) == 0)
	{
		// Connected - Send *RTL 

		nputs(circuit, "*RTL\r");  // Log in to the remote RT system.
		return TRUE;

	}

//	if (memcmp(Resp, " Failure", 8) == 0)
//	{
//
	// Anything else - Failed - clear 

//	link_drop(circuit);
//	Disconnect(circuit->BPQStream);
	return FALSE;

}

VOID SetupForwardingStruct(struct UserInfo * user)
{
	struct	BBSForwardingInfo * ForwardingInfo;
	char ** Calls;

	ForwardingInfo = user->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));

	strcpy(ForwardingInfo->Callsign, "GM8BPQ-3");

	Calls = ForwardingInfo->Calls = zalloc(16);

	Calls[0] = _strdup("ALL");
	Calls[1] = _strdup("BPQ");

}

BOOL bbs_link_out (struct UserInfo * user)
{
	int n;
	CIRCUIT * conn;

	for (n = NumberofStreams-1; n > 0 ; n--)
	{
		conn = &Connections[n];
		
		if (conn->Active == FALSE)
		{
			conn->Active = TRUE;
			strcpy(conn->Callsign, user->ForwardingInfo->Callsign); 
			conn->BBSFlags |= Connecting;
			conn->UserPointer = user;

			ConnectUsingAppl(conn->BPQStream, BBSApplMask);

			//	Connected Event will trigger connect to remote system

			return TRUE;
		}
	}

	return FALSE;
	
}

ProcessBBSConnecting(CIRCUIT * conn, char * Buffer, int len)
{
	char * Resp;

	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		Parse_SID(conn, &Buffer[1], len-4);
			
		if (conn->BBSFlags & FBBForwarding)
		{
			conn->FBBIndex = 0;		// ready for first block;
			conn->FBBChecksum = 0;
		}

	}

	if (Buffer[len-2] == '>')
	{
		SendMsg(conn->BPQStream, BBSSID, strlen(BBSSID));
		nputs(conn, "FF\r");
		conn->BBSFlags &= !Connecting;
	}

	Resp=strlop(Buffer, '}');
	
	if (Resp)
	{
		if (memcmp(Resp, " Connected", 10) == 0)
		{
			// Connected - Wait for SID or Prompt 

//			nputs(circuit, "*RTL\r");  // Log in to the remote RT system.
			return TRUE;
		}
	

//		if (memcmp(Resp, " Failure", 8) == 0)
//		{
//
	}	
		// Anything else - Failed - clear 

//	link_drop(circuit);
//	Disconnect(circuit->BPQStream);
	return FALSE;

}

VOID Parse_SID(ConnectionInfo * conn, char * SID, int len)
{
	// scan backwards for first '-'

	while (len > 0)
	{
		switch (SID[len--])
		{
		case '-':
			len=0;
			break;

		case '$':
			conn->BBSFlags |= BBS;
			break;

		case 'F':			// FBB Blocked Forwarding

			conn->BBSFlags |= FBBForwarding;
		
			// Allocate a Header Block

			conn->FBBHeaders = zalloc(5 * sizeof(struct FBBHeaderLine));
			break;

		case 'B':
			conn->BBSFlags |= FBBCompressed;
			break;

		}
	}

	return;
}
int	CriticalErrorHandler(char * error)
{
	return 0;
}

BOOL StartForwarding (struct UserInfo * user)
{
	// See if any messages are queued for this BBS

	if (user->ForwardingInfo->MsgCount || user->ForwardingInfo->ReverseFlag)

		return	bbs_link_out(user);

	return FALSE;

}

BOOL FindMessagestoForward (CIRCUIT * conn)
{
	// See if any messages are queued for this BBS

	int m;
	struct MsgInfo * Msg;
	struct UserInfo * user = conn->UserPointer;
	struct FBBHeaderLine * FBBHeader;
	BOOL Found = FALSE;

	conn->FBBIndex = 0;

	if (user->ForwardingInfo->MsgCount == 0)
		return FALSE;

	for (m = FirstMessagetoForward; m < NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		if (check_fwd_bit(Msg->fbbs, user->BBSNumber))
		{
			// Message to be sent - do a consistancy check (State, etc)

			// if FBB forwarding add to list, eise save pointer

			if (conn->BBSFlags & FBBForwarding)
			{
				FBBHeader = &conn->FBBHeaders[conn->FBBIndex++];
				FBBHeader->FwdMsg = Msg;
				FBBHeader->MsgType = Msg->type;
				FBBHeader->Size = Msg->length;
				strcpy(FBBHeader->From, Msg->from);
				strcpy(FBBHeader->To, Msg->to);
				strcpy(FBBHeader->ATBBS, Msg->via);
				strcpy(FBBHeader->BID, Msg->bid);
				if (conn->FBBIndex == 5)
					return TRUE;							// Got max number

				Found = TRUE;								// Remember we have some
			}
			else
			{
				conn->FwdMsg = Msg;
				return TRUE;
			}
		}
	}

	return Found;


}

int Reverse_Forward(struct UserInfo * user)
{
	bbs_link_out(user);

	return FALSE;
}


int Forward_Message(struct UserInfo * user, struct MsgInfo * Msg)
{

/*
R:930108/1259 1530@KA6FUB.#NOCAL.CA.USA.NA
R:090209/0128Z 33040@N4JOA.#WPBFL.FL.USA.NOAM [164113] FBB7.01.35 alpha
R:090209/0128Z 23098@N9PMO.#SEWI.WI.USA.NOAM [Racine, WI] FBB7.00i
R:090209/0236Z @:VE2GPQ.#QBC.QC.CAN.NOAM #:14618 [Quebec] $:53824_VK2TV
R:090209/0226Z @:ON0BEL.#LG.BEL.EU #:7256 [Pactor-Belgium WFBB] $:53824_VK2TV
R:090209/0127Z @:7M3TJZ.13.JNET1.JPN.AS #:46823 [Tokyo] $:53824_VK2TV
R:090209/0026Z @:F6CDD.#82.FMLR.FRA.EU #:36863 [CASTELSARRASIN] $:53824_VK2TV
R:090209/0120Z @:F6BVP.FRPA.FRA.EU #:10859 [Paris] $:53824_VK2TV
R:090209/0122Z @:VK2TV.#MNC.NSW.AUS.OC #:53824 [Kempsey, QF68JX] $:53824_VK2TV
*/

	return TRUE;
}

VOID ProcessFBBLine(CIRCUIT * conn, struct UserInfo * user, UCHAR* Buffer, int len)
{
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FBB forward block
	int i;
	char * ptr;
	char * Context;
	char seps[] = " \r";
//	char FSLine[] = "FS +++++\r";


	if (conn->Flags & GETTINGMESSAGE)
	{
		ProcessMsgLine(conn, user, Buffer, len);
		return;
	}

	if (conn->Flags & GETTINGTITLE)
	{
		ProcessMsgTitle(conn, user, Buffer, len);
		return;
	}

	// Should be FA FB F> FS FF FQ

	if (Buffer[0] != 'F')
	{
		nputs(conn, "*** Protocol Error - Line should start with 'F'\r");
		Flush(conn);
		Sleep(500);
		Disconnect(conn->BPQStream);

		return;
	}

	switch (Buffer[1])
	{
	case 'F':

		// Request Reverse
		
		if (FindMessagestoForward(conn))
		{
			// Send Proposal Block

			struct FBBHeaderLine * FBBHeader;

			for (i=0; i < conn->FBBIndex; i++)
			{
				FBBHeader = &conn->FBBHeaders[i];
				
				nodeprintf(conn, "FB %c %s %s %s %s %d\r", 
						FBBHeader->MsgType,
						FBBHeader->From,
						FBBHeader->ATBBS, 
						FBBHeader->To, 
						FBBHeader->BID,
						FBBHeader->Size);

				//	FB P F6FBB FC1GHV.FFPC.FRA.EU FC1MVP 24657_F6FBB 1345
			}

			nodeprintf(conn, "F>\r");

		}
		else
			nputs(conn, "FQ\r");

		return;

	case 'S':

		//	Proposal response

		for (i=0; i < conn->FBBIndex; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];
				
			if (Buffer[i+3] == '-')				// Not wanted
			{
				// Zap the entry

				clear_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber);

				memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));

				conn->UserPointer->ForwardingInfo->MsgCount--;
			}

			if (Buffer[i+3] == '=')				// Defer
			{
				// Zap the entry

				clear_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber);

				memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));

				conn->UserPointer->ForwardingInfo->MsgCount--;
			}

			if (Buffer[i+3] == '+')				// Need it
			{
				char * MsgBytes;

				nodeprintf(conn, "%s\r", FBBHeader->FwdMsg->title);

				MsgBytes = ReadMessageFile(FBBHeader->FwdMsg->number);

				if (MsgBytes)
				{
					QueueMsg(conn, MsgBytes, FBBHeader->FwdMsg->length);
					free(MsgBytes);
				}
			
				nodeprintf(conn, "%c\r", 26);

			}

		}

		conn->FBBIndex = 0;		// ready for next block;
		conn->FBBChecksum = 0;


		return;

	case 'Q':

		Disconnect(conn->BPQStream);
		return;

	case 'A':			// Proposal
	case 'B':			// Proposal

		// Accumulate checksum

		for (i=0; i< len; i++)
		{
			conn->FBBChecksum+=Buffer[i];
		}

		// Parse Header

		// Find free line

		for (i = 0; i < 5; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (FBBHeader->Format == 0)
				break;
		}

		if (i == 5)
		{
			nputs(conn, "*** Protocol Error - Too Many Proposals\r");
			Flush(conn);
			Sleep(500);
			Disconnect(conn->BPQStream);
		}

		//FA P GM8BPQ G8BPQ G8BPQ 2209_GM8BPQ 8

		FBBHeader->Format = Buffer[1];

		ptr = strtok_s(&Buffer[3], seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) != 1) goto badparam;

		FBBHeader->MsgType = *ptr;

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 6 ) goto badparam;

		strcpy(FBBHeader->From, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 40 ) goto badparam;

		strcpy(FBBHeader->ATBBS, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 7 ) goto badparam;

		strcpy(FBBHeader->To, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 12 ) goto badparam;

		strcpy(FBBHeader->BID, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		FBBHeader->Size = atoi(ptr);

		goto ok;

badparam:

		nputs(conn, "*** Protocol Error - Proposal format error\r");
		Flush(conn);
		Sleep(500);
		Disconnect(conn->BPQStream);
		return;

ok:
		if (LookupBID(FBBHeader->BID))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// CLear header
			conn->FBBReplyChars[conn->FBBIndex++] = '-';
		}
		else
			conn->FBBReplyChars[conn->FBBIndex++] = '+';
		return;

	case '>':

		// Optional Checksum

		if (len > 3)
		{
			int sum;
			
			sscanf(&Buffer[3], "%x", &sum);

			conn->FBBChecksum+=sum;

			if (conn->FBBChecksum)
			{
				nputs(conn, "*** Proposal Checksum Error\r");
				Flush(conn);
				Sleep(500);
				Disconnect(conn->BPQStream);
				return;
			}
		}

		// Return "FS ", followed by +-= for each proposal

		conn->FBBReplyChars[conn->FBBIndex++] = 0;

		nodeprintf(conn, "FS %s\r", conn->FBBReplyChars);

		// if all rejected, send prompt, else set up for first message

		FBBHeader = &conn->FBBHeaders[0];

		if (FBBHeader->MsgType == 0)
			nputs(conn, "FF\r");
		else
			CreateMessage(conn, FBBHeader->From, FBBHeader->To, FBBHeader->ATBBS, FBBHeader->MsgType, FBBHeader->BID);


		return;

	}

/*
Connects FC1GHV	
Connected
[FBB-5.11-FHM$]Bienvenue a Poitiers, Jean-Paul.>
[FBB-5.11-FHM$]         (F6FBB has the F flag in the SID)
FB P F6FBB FC1GHV.FFPC.FRA.EU FC1MVP 24657_F6FBB 1345
FB P FC1CDC F6ABJ F6AXV 24643_F6FBB 5346
FB B F6FBB FRA FBB 22_456_F6FBB 8548
F> HH
  	FS +-+ (accepts the 1st and the 3rd)
Title 1st messageText 1st message
......
^Z
Title 3rd message
Text 3rd message
......
^Z	
FB P FC1GHV F6FBB F6FBB 2734_FC1GHV 234
FB B FC1GHV F6FBB FC1CDC 2745_FC1GHV 3524
F> HH
 FS -- (Don't need them, and send immediately the proposal).
 FB P FC1CDC F6ABJ F6AXV 24754_F6FBB 345F> HH
FS + (Accepts the message)
Title message
Text message......
^Z	
FF (no more message)
FB B F6FBB TEST FRA 24654_F6FBB 145
F> HH	
FS + (Accepts the message)
Title message
Text message
......
^Z	
FF (still no message)
FQ (No more message)	
Disconnection of the link	
*/
	return;
}


VOID SetupNextFBBMessage(CIRCUIT * conn)
{
	int i;
	
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FFB forward block

	memmove(&conn->FBBHeaders[0], &conn->FBBHeaders[1], 4 * sizeof(struct FBBHeaderLine));
	
	memset(&conn->FBBHeaders[4], 0, sizeof(struct FBBHeaderLine));

	FBBHeader = &conn->FBBHeaders[0];

	if (FBBHeader->MsgType == 0)
	{
		conn->FBBIndex = 0;		// ready for next block;
		conn->FBBChecksum = 0;

		if (FindMessagestoForward(conn))
		{
			// Send Proposal Block

			struct FBBHeaderLine * FBBHeader;

			for (i=0; i < conn->FBBIndex; i++)
			{
				FBBHeader = &conn->FBBHeaders[i];
				
				nodeprintf(conn, "FB %c %s %s %s %s %d\r", 
						FBBHeader->MsgType,
						FBBHeader->From,
						FBBHeader->ATBBS, 
						FBBHeader->To, 
						FBBHeader->BID,
						FBBHeader->Size);

				//	FB P F6FBB FC1GHV.FFPC.FRA.EU FC1MVP 24657_F6FBB 1345
			}

			nodeprintf(conn, "F>\r");

		}
		else


		nputs(conn, "FF\r");
	}
	else
		CreateMessage(conn, FBBHeader->From, FBBHeader->To, FBBHeader->ATBBS, FBBHeader->MsgType, FBBHeader->BID);
}

int check_fwd_bit(char *mask, int bbsnumber)
{
	return (mask[(bbsnumber - 1) / 8] & (1 << ((bbsnumber - 1) % 8)));
}


void set_fwd_bit(char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] |= (1 << ((bbsnumber - 1) % 8));
}


void clear_fwd_bit (char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] &= (~(1 << ((bbsnumber - 1) % 8)));
}

int MatchMessagetoBBSList(struct MsgInfo * Msg)
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo;
	char ATBBS[41];
	char * HRoute;
	int Count =0;

	strcpy(ATBBS, Msg->via);
	HRoute = strlop(ATBBS, '.');

	for (user = BBSChain; user; user = user->BBSNext)
	{		
		ForwardingInfo = user->ForwardingInfo;
		if (CheckABBS(Msg, user, ForwardingInfo, ATBBS, HRoute))		
		{
			set_fwd_bit(Msg->fbbs, user->BBSNumber);
			ForwardingInfo->MsgCount++;
			Count++;
		}
	}

	return Count;
}
BOOL CheckABBS(struct MsgInfo * Msg, struct UserInfo * user, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** Calls;

	if (strcmp(ATBBS, user->Call) == 0)					// @BBS = BBS
		return TRUE;

	// Check TO distributions

	if (ForwardingInfo->Calls)
	{
		Calls=ForwardingInfo->Calls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}

	if (HRoute)
	{
		// Match on Routes
	}

	return FALSE;

}