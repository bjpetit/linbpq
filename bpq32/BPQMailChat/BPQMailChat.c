// Mail and Chat Server for BPQ32 Packet Switch
//
//

#include "stdafx.h"
#include "BPQMailChat.h"
#include "bpq32.h"


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

struct ConnectionInfo Connections[MaxSockets+1];

int NumberofUserRecords=0;

struct UserInfo ** UserRecPtr;
int NumberofUsers=0;

int Port=8010;
BOOL cfgMinToTray;

BOOL DisconnectOnClose=FALSE;


char PasswordMsg[100]="Password:";

char cfgHOSTPROMPT[100];

char cfgCTEXT[100];

char cfgLOCALECHO[100];

char AttemptsMsg[] = "Too many attempts - Disconnected\r\n";
char disMsg[] = "Disconnected by SYSOP\r\n";

char LoginMsg[]="user:";

char BlankCall[]="         ";


BOOL LogEnabled=FALSE;

UCHAR BBSApplMask=1;
UCHAR ChatApplMask=4;

int	StartStream=0;
int	NumberofStreams=20;

char BBSSID[100]="[BPQ-1.00-AB1FHMRX$]\r";

char ChatSID[100]="[BPQChatServer-1.00]\r";

char NewUserPrompt[100]="Please enter your Name: ";

char Prompt[100]="de GM8BPQ>\r";

char BBSName[100]="GM8BPQ";

char SignoffMsg[100]="73 de GM8BPQ\r";

char UserDatabaseName[MAX_PATH] = "BPQBBSUsers.dat";

char MailDir[MAX_PATH] = "C:\\Program Files\\Amateur_Radio\\BPQBBS\\MAIL";

#define NUMBEROFBUFFERS 100

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL Initialise();
int DisplaySessions();
int DoStateChange(Stream);
int DoReceivedData(Stream);
int DoMonitorData(Stream);
int Connected(Stream);
int Disconnected(Stream);
int DeleteConnection(con);
//int Socket_Accept(int SocketId);
//int Socket_Data(int SocketId,int error, int eventcode);
int DataSocket_Read(struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_Write(struct tConnectionInfo * sockptr, SOCKET sock);
int DataSocket_Disconnect(struct ConnectionInfo * sockptr);
BOOL ProcessTelnetCommand(struct ConnectionInfo * sockptr, byte * Msg, int Len);
int ShowConnections();
int Terminate();
int SendtoSocket(SOCKET sock,char * Msg);
int WriteLog(char * msg);
int ConnectState(Stream);
byte * EncodeCall(byte * Call);
int ParseIniFile(char * fn);
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int Q_ADD(UINT *Q,UINT *BUFF);
struct UserInfo * AllocateUserRecord(char * Call);
struct UserInfo * LookupCall(char * Call);
VOID SaveUserDatabase();
VOID GetUserDatabase();
VOID SendWelcomeMsg(int Stream, struct	ConnectionInfo * conn, struct UserInfo * user);
VOID ProcessLine(struct	ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID ProcessChatLine(struct	ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len);
VOID SendPrompt(int Stream, struct UserInfo * user);
int QueueMsg(int stream, char * msg, int len);
char * GetConfStations(int Conference);
VOID SendtoOtherUsers(struct ConnectionInfo * conn, char* Msg, int msglen);
int GetFileList(char * Dir);

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

	if (cfgMinToTray) DeleteTrayMenuItem(MainWnd);
	
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
   hInst = hInstance;

   hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   MainWnd=hWnd;

   CheckTimer();

   SetTimer(hWnd,1,10000,NULL);

	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

   UpdateWindow(hWnd);

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
	struct ConnectionInfo * sockptr;


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
			
	case WM_TIMER:

		CheckTimer();
		return (0);


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmId > IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			sockptr=&Connections[wmId-IDM_DISCONNECT];
		
			if (sockptr->Active)
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
		break;
	}
	return (INT_PTR)FALSE;
}

int ShowConnections()
{
	char msg[80];
	struct ConnectionInfo * sockptr;
	int i,n;

	SendDlgItemMessage(MainWnd,100,LB_RESETCONTENT,0,0);

	for (n = 0; n < NumberofStreams; n++)
	{
		sockptr=&Connections[n];

		if (!sockptr->Active)
		{
			strcpy(msg,"Idle");
		}
		else
		{
			if (sockptr->UserPointer == 0)
				strcpy(msg,"Logging in");
			else
			{
				i=wsprintf(msg,"%-15s %-10s %2d %2d",
					sockptr->UserPointer->Name, sockptr->UserPointer->Call, sockptr->BPQStream, sockptr->Conference);
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
	int i;

	struct	ConnectionInfo * conn;
	BOOL	ReturnValue = TRUE; // return value

	HMENU hMenu;		// handle of menu 
	HKEY hKey=0;

	//	Register message for posting by BPQDLL

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	// Get Config From Registry


	// Build buffer pool
	
	for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
	{	
		ReleaseBuffer(&BufferPool[100*i]);
	}

	GetUserDatabase();

	GetFileList(MailDir);
	 
	// Allocate Streams

	for (i=0; i < NumberofStreams; i++)
	{
		conn = &Connections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, 2, BBSApplMask | ChatApplMask);

	}
	if (cfgMinToTray)
	
		AddTrayMenuItem(MainWnd, "Mail/Chat Server");

	// Get handles fou updating "Disconnect User" menu items

	hMenu=GetMenu(MainWnd);
	hActionMenu=GetSubMenu(hMenu,0);

	hLogMenu=GetSubMenu(hActionMenu,0);

	hDisMenu=GetSubMenu(hActionMenu,1);

	CheckMenuItem(hLogMenu,0,MF_BYPOSITION | LogEnabled<<3);

	return TRUE;
}

int Connected(Stream)
{
	int i, n, Mask;
	struct	ConnectionInfo * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, sesstype, paclen, maxframe, l4window;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (Stream == conn->BPQStream)
		{
			conn->Active = TRUE;
			conn->Flags = 0;

			GetConnectionInfo(Stream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);

			for (i=9; i > 0; i--)
			{
				if (callsign[i] == 32)
					callsign[i] = 0;
				else
					break;
			}

			user = LookupCall(callsign);

			if (user == NULL)
			{
				user = AllocateUserRecord(callsign);

				if (user == NULL) return 0; //		Cant happen??
			}

			time(&user->TimeLastCOnnected);

			conn->UserPointer = user;

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
	int n;
		char Msg[255];
	int len;


	for (n = 0; n <= NumberofStreams-1; n++)
	{
		if (Stream == Connections[n].BPQStream)
		{
			Connections[n].Active = FALSE;
			ShowConnections();
	
			user = Connections[n].UserPointer;

			len = wsprintf(Msg, "%s - %s Logged off Channel %d\r", user->Call, user->Name, Connections[n].Conference);
			SendtoOtherUsers(&Connections[n], Msg, len);
	

			return 0;
		}
	}

	return 0;
}

int DoReceivedData(int Stream)
{
	byte Buffer[400];
	int len,count;
	int n;
	struct	ConnectionInfo * conn;
	struct UserInfo * user;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];

		if (Stream == conn->BPQStream)
		{
			do
			{ 
				GetMsg(Stream, Buffer,&len, &count);

				if (len == 0) return 0;

				user = conn->UserPointer;

				ProcessLine(conn, user, Buffer, len);

			} while (count > 0);

			return 0;
		}
	}

	// Socket not found

	return 0;

}
int DoMonitorData(int Stream)
{
	byte Buffer[1000];
	byte buff[500];

	int n,len,count;
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
byte * EncodeCall(byte * Call)
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

struct UserInfo * AllocateUserRecord(char * Call)
{
	if (NumberofUsers == 0)
		UserRecPtr=malloc(4);
	else
		UserRecPtr=realloc(UserRecPtr,(NumberofUsers+1)*4);

	UserRecPtr[NumberofUsers]= malloc(sizeof (struct UserInfo));

	memset(UserRecPtr[NumberofUsers], 0, sizeof (struct UserInfo));

	memcpy(UserRecPtr[NumberofUsers]->Call, Call, 10);

	return UserRecPtr[NumberofUsers++];
}

struct UserInfo * LookupCall(char * Call)
{
	struct UserInfo * ptr = NULL;
	int i;

	for (i=0; i < NumberofUsers; i++)
	{
		ptr = UserRecPtr[i];

		if (_memicmp(ptr->Call, Call, 10) == 0) return ptr;

	}

	return NULL;
}

VOID GetUserDatabase()
{
	UCHAR Value[MAX_PATH];
	UCHAR * BPQDirectory;
	struct UserInfo UserRec;
	HANDLE Handle;
	int ReadLen;
	struct UserInfo * user;

	BPQDirectory=GetBPQDirectory();

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value, UserDatabaseName);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value, "\\");
		strcat(Value, UserDatabaseName);
	}

	Handle = CreateFile(Value,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
		return;

Next:

	ReadFile(Handle, &UserRec, sizeof (UserRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		user = AllocateUserRecord(UserRec.Call);
		memcpy(user, &UserRec,  sizeof (UserRec));
		goto Next;
	}

	CloseHandle(Handle);

	
}
VOID SaveUserDatabase()
{
	UCHAR Value[MAX_PATH];
	UCHAR * BPQDirectory;
	HANDLE Handle;
	int WriteLen;
	int i;

	BPQDirectory=GetBPQDirectory();

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value, UserDatabaseName);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value, "\\");
		strcat(Value, UserDatabaseName);
	}

	Handle = CreateFile(Value,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	for (i=0; i < NumberofUsers; i++)
	{
		WriteFile(Handle, UserRecPtr[i], sizeof (struct UserInfo), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

VOID SendWelcomeMsg(int Stream, struct	ConnectionInfo * conn, struct UserInfo * user)
{
	char Msg[255];
	int len;

	if (conn->Flags & CHATMODE)
		len = wsprintf(Msg, "Hello %s\rWelcome to %s Chat Server (/H for help) >\r", user->Name, BBSName);
	else
		len = wsprintf(Msg, "Hello %s\r%s BBS (H for help) >\r", user->Name, BBSName);

	SendMsg(Stream, Msg, len);

	if (conn->Flags & CHATMODE)
	{
		len = wsprintf(Msg, "%s - %s Logged on to Channel %d\r", user->Call, user->Name, conn->Conference);
		SendtoOtherUsers(conn, Msg, len);
	}

}

VOID SendPrompt(int Stream, struct UserInfo * user)
{
	char Msg[255];
	int len;

	len = wsprintf(Msg, "%s", Prompt);

	SendMsg(Stream, Msg, len);
}


VOID ProcessLine(struct	ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len)
{
	char * Cmd, * Arg1, * Arg2, * Arg3;
	char * Context;
	char seps[] = " \t\r";
	int CmdLen;
	
	if (conn->Flags & GETTINGUSER)
	{
		memcpy(user->Name, Buffer, len-1);
		conn->Flags &=  ~GETTINGUSER;
		SendWelcomeMsg(conn->BPQStream, conn, user);
	}

	if (conn->Flags & CHATMODE)
	{
		ProcessChatLine(conn, user, Buffer, len);
		return;
	}

	// Process Command

	if (len == 1)
	{
		SendPrompt(conn->BPQStream, user);
		return;
	}

	Buffer[len] = 0;

	Cmd = strtok_s(Buffer, seps, &Context);
	Arg1 = strtok_s(NULL, seps, &Context);
	Arg2 = strtok_s(NULL, seps, &Context);
	Arg3 = strtok_s(NULL, seps, &Context);
	CmdLen = strlen(Cmd);

	if (_memicmp(Cmd, "Bye", CmdLen) == 0)
	{
		SendMsg(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
		Sleep(1000);
		Disconnect(conn->BPQStream);
		return;
	}

	if (_memicmp(Cmd, "Name", CmdLen) == 0)
	{
		if (Arg1)
			strcpy(user->Name, Arg1);
		SendWelcomeMsg(conn->BPQStream, conn, user);

		return;
	}

	if (_memicmp(Cmd, "Chat", CmdLen) == 0)
	{
		conn->Flags |= CHATMODE;
		return;
	}

	SendPrompt(conn->BPQStream, user);

}


VOID ProcessChatLine(struct	ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int len)
{
	char * Cmd, * Arg1, * Arg2, * Arg3;
	char * Context;
	char seps[] = " \t\r";
	int CmdLen;
	char Msg[250];
	int msglen, n, NewChan = 0;
	struct	ConnectionInfo * otherconn;
	struct UserInfo * usr;

	Buffer[len] = 0;


//	if ((len > 1) && Buffer[0] == '/')
	if (Buffer[0] == '/')
	{
		// Process Command

		Cmd = strtok_s(&Buffer[1], seps, &Context);

		if (Cmd == NULL) goto NullCmd;

		CmdLen = strlen(Cmd);

		if (_memicmp(Cmd, "Bye", CmdLen) == 0)
		{
			SendMsg(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
			conn->Active = FALSE;
			ReturntoNode(conn->BPQStream);
			return;
		}

		if (_memicmp(Cmd, "Quit", CmdLen) == 0)
		{
			SendMsg(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
			Sleep(1000);
			Disconnect(conn->BPQStream);
			return;
		}

		if ((_memicmp(Cmd, "Help", CmdLen) == 0) || Cmd[0] == '?')
		{
			msglen = wsprintf(Msg, "BPQChat version 1.00 available commands are :-\r\r/? or /H - To read this list)\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

			msglen = wsprintf(Msg, "/B ------- To leave CHAT and return to the BPQ Node\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

			msglen = wsprintf(Msg, "/C n ----- To switch to Conference stream n (0 - 32)\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

			msglen = wsprintf(Msg, "/L Loc --- To register your QTH/Locator (max 30 chars)\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

			msglen = wsprintf(Msg, "/Q ------- To leave CHAT and disconnect from BPQ Node\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

			msglen = wsprintf(Msg, "/W ------- To list connected users\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

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

			msglen = wsprintf(Msg, "Location is %s\r", conn->UserPointer->City);
			QueueMsg(conn->BPQStream, Msg, msglen);

			return;

		}

		if (_memicmp(Cmd, "w", CmdLen) == 0)
		{
			// Show Users

			msglen = wsprintf(Msg, "User       User            User                      Port Conf  On\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

			msglen = wsprintf(Msg, "Name       Call    QTH/Location/Locator               No.  No.  DD/MM\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

			for (n = 0; n < NumberofStreams; n++)
			{
				otherconn = &Connections[n];
		
				if ((otherconn->Active) && (otherconn->Flags == CHATMODE))
				{
					usr = otherconn->UserPointer;

					msglen = wsprintf(Msg, "%-10s %-10s %-31s %2d   %2d   %02d/%02d\r",
								usr->Name, usr->Call, usr->City,
								otherconn->BPQStream, otherconn->Conference, 0, 0);

				
					QueueMsg(conn->BPQStream, Msg, msglen);
				}
			}

			msglen = wsprintf(Msg, "End of User List..\r");
			QueueMsg(conn->BPQStream, Msg, msglen);

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
			msglen = wsprintf(Msg, "You are now on Conference channel %d\r", conn->Conference);
			QueueMsg(conn->BPQStream, Msg, msglen);

			msglen = wsprintf(Msg, "Stations using channel %d = %s\r", conn->Conference, GetConfStations(conn->Conference));
			QueueMsg(conn->BPQStream, Msg, msglen);

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

*/
NullCmd:

		msglen = wsprintf(Msg, "Unrecognised command  - type /? for help\r");
		QueueMsg(conn->BPQStream, Msg, msglen);

		return;

	}

	// Send message to all other connected users on same channel

	if (len > 200)
	{
		Buffer[200] = '\r';
		Buffer[201] = 0;
		len = 200;
	}

	msglen = wsprintf(Msg, "[%d:%s:%s] %s", conn->BPQStream, conn->UserPointer->Call, conn->UserPointer->Name, Buffer);


	for (n = 0; n < NumberofStreams; n++)
	{
		otherconn = &Connections[n];
		
		if ((otherconn->Active) && (otherconn->Flags == CHATMODE) && (otherconn->Conference == conn->Conference) && conn != otherconn)
		{
			QueueMsg(otherconn->BPQStream, Msg, msglen);
		}
	}

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

int QueueMsg(int stream, char * msg, int len)
{
	return SendMsg(stream, msg, len);
}

char * GetConfStations(int Conference)
{
	static char Stns[1000] = "";
	int n;
	struct	ConnectionInfo * otherconn;

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

VOID SendtoOtherUsers(struct ConnectionInfo * conn, char* Msg, int msglen)
{
	int n;
	struct ConnectionInfo * otherconn;

	for (n = 0; n < NumberofStreams; n++)
	{
		otherconn = &Connections[n];
		
		if ((otherconn->Active) && (otherconn->Flags == CHATMODE) && conn != otherconn)
		{
			QueueMsg(otherconn->BPQStream, Msg, msglen);
		}
	}

}



//#include <windows.h>
//#include <tchar.h> 
//#include <stdio.h>

#include <strsafe.h>



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


