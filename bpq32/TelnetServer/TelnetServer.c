// Telnet Server for BPQ32 Packet Switch
//
//
//	Version 2.1.1 December 2007
//
//	Poll CheckTimer() to detect loss of program owning timer

//	Version 2.1.2  October 2008
//	Add logging param to config file
//	Include IP address in log

//	Version 2.1.3  January 2009
//	Add StartMinimized Command Line Option

//	Version 2.1.4  March 2009
//	Add DisconnectOnClose Config option

//	Version 2.1.5  April 2009
//  Remove limit on number of user records

//	Version 2.1.6  July 2009
//  Remove entry from Disconnect User Dialog when session closes
//	Fix reception of multiple backspaces in same packet

//	Version 2.1.7  August 2009
//  Support FBB forwarding

//	Version 2.1.8  january 2010
//  Change connected message to be fixed text. (for Outpost)

//	Version 2.1.9 June 2010
//  Prevent overlong packets being sent to node (for Outpost forwarding, etc)

//	Version 2.1.10 September 2010

//  Fix loss of las letter of callsign  if newline missing from USER line.

#include "stdafx.h"
#include "TelnetServer.h"
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

struct ConnectionInfo ConnectionInfo[MaxSockets+1];

int NumberofUsers=0;
struct UserRec ** UserRecPtr;
int CurrentConnections=0;

struct UserRec RelayUser;

int CurrentSockets=0;

int Port=8010;
int FBBPort=0;
int RelayPort=8772;

BOOL cfgMinToTray;

BOOL DisconnectOnClose=FALSE;


char PasswordMsg[100]="Password:";

char cfgHOSTPROMPT[100];

char cfgCTEXT[300];

char cfgLOCALECHO[100];

char AttemptsMsg[] = "Too many attempts - Disconnected\r\n";
char disMsg[] = "Disconnected by SYSOP\r\n";

int MaxSessions=10;

char LoginMsg[100]="user:";

char BlankCall[]="         ";

char RelayAPPL[20] = "";

BOOL LogEnabled=FALSE;

SOCKET sock;
SOCKET FBBsock;
SOCKET Relaysock;



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
int Socket_Accept(int SocketId);
int Socket_Data(int SocketId,int error, int eventcode);
int DataSocket_Read(struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_ReadFBB(struct ConnectionInfo * sockptr, SOCKET sock);
int DataSocket_ReadRelay(struct ConnectionInfo * sockptr, SOCKET sock);
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
	LoadString(hInstance, IDC_TELNETSERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TELNETSERVER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	for (n = 1; n <= CurrentSockets; n++)
	{
		BPQStream=ConnectionInfo[n].BPQStream;
		
		if (BPQStream)
		{
			SetAppl(BPQStream, 0, 0);
			Disconnect(BPQStream);
			DeallocateStream(BPQStream);
		}
	}
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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TELNETSERVER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= bgBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TELNETSERVER);
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    //  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

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
	SOCKET sock;


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


	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(wParam);
		return 0;



	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmId > IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			sockptr=&ConnectionInfo[wmId-IDM_DISCONNECT];
		
			if (sockptr->SocketActive)
			{
				sock=sockptr->socket;

				send(sock,disMsg, strlen(disMsg),0);

				Sleep (1000);
    
				shutdown(sock,2);

				DataSocket_Disconnect(sockptr);
	
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

int Socket_Accept(int SocketId)
{
	int n,addrlen;
	struct ConnectionInfo * sockptr;
	SOCKET sock;
	char msg[10];
	char Negotiate[6]={IAC,WILL,suppressgoahead,IAC,WILL,echo};
//	char Negotiate[3]={IAC,WILL,echo};

//   Find a free Socket

	for (n = 1; n <= MaxSockets; n++)
	{
		sockptr=&ConnectionInfo[n];
		
		if (sockptr->SocketActive == FALSE)
		{
			addrlen=sizeof(struct sockaddr);

			sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

			if (sock == INVALID_SOCKET)
			{
				wsprintf(szBuff, " accept() failed Error %d", WSAGetLastError());
				MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
				return FALSE;
			}

			WSAAsyncSelect(sock, MainWnd, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
			sockptr->socket = sock;
			sockptr->SocketActive = TRUE;
			sockptr->InputLen = 0;
			sockptr->Number = n;
			sockptr->LoginState = 0;
			sockptr->UserPointer = 0;
			sockptr->DoEcho = FALSE;

			if (SocketId == FBBsock)
				sockptr->FBBMode = TRUE;
			else
				sockptr->FBBMode = FALSE;

			if (SocketId == Relaysock)
				sockptr->RelayMode = TRUE;
			else
				sockptr->RelayMode = FALSE;
	
			if (CurrentSockets < n)
			{
				// Just Created a new one - add an item to disconnect menu
				
				CurrentSockets=n;  //Record max used to save searching all entries

				wsprintf(msg,"Port %d",n);

				if (n != 1)
					AppendMenu(hDisMenu, MF_STRING | MF_CHECKED,IDM_DISCONNECT + n ,msg);
				else
					ModifyMenu(hDisMenu,0,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + 1,msg);
			}
			else
				//	reusing an entry
				ModifyMenu(hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n," ");

			DrawMenuBar(MainWnd);	
			ShowConnections();

			if (n > MaxSessions)
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
					send(sock,LoginMsg,strlen(LoginMsg),0);
				}
			}
			return 0;
		}
		
	//	Can only happen if MaxSessions > maxSockets, which is a config error

		
	}

	return 0;
	return 0;
}







int Socket_Data(int sock, int error, int eventcode)
{
	int n;
	struct ConnectionInfo * sockptr;

	//	Find Connection Record

	for (n = 1; n <= CurrentSockets; n++)
	{
		sockptr=&ConnectionInfo[n];
		
		if (sockptr->socket == sock && sockptr->SocketActive)
		{
			switch (eventcode)
			{
				case FD_READ:

					if (sockptr->FBBMode)
						return DataSocket_ReadFBB(sockptr,sock);
					else
						if (sockptr->RelayMode)
							return DataSocket_ReadRelay(sockptr,sock);
						else
							return DataSocket_Read(sockptr,sock);

				case FD_WRITE:

					return 0;

				case FD_OOB:

					return 0;

				case FD_ACCEPT:

					return 0;

				case FD_CONNECT:

					return 0;

				case FD_CLOSE:

					DataSocket_Disconnect(sockptr);
					return 0;
				}
			return 0;
		}
	}

	return 0;
}

#define PACLEN 100

int DataSocket_Read(struct ConnectionInfo * sockptr, SOCKET sock)
{
	int len=0, maxlen, InputLen, MsgLen, Stream, i, n,charsAfter;
	char NLMsg[3]={13,10,0};
	byte * IACptr;
	byte * LFPtr;
	byte * BSptr;
	byte * MsgPtr;
	BOOL wait;
	char logmsg[120];

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

			Stream = sockptr->BPQStream;
    
			if (ConnectState(Stream) == 0)
			{
				Connect(Stream);
  
				if (sockptr->Callsign[0] != '@') 
					ChangeSessionCallsign(Stream, EncodeCall(sockptr->Callsign));
			}

			// Line could be up to 500 chars if coming from a program rather than an interative user
			// Limit send to node to 255

			if (InputLen > 255)
			{		
				SendMsg(Stream, MsgPtr, 255);
				sockptr->InputLen -= 255;
				InputLen -= 255;

				memmove(MsgPtr,MsgPtr+255,InputLen);
			}
	           
			SendMsg(Stream, MsgPtr, InputLen);
		
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
			
		Stream = sockptr->BPQStream;
    
		if (ConnectState(Stream) == 0)
		{
			Connect(Stream);
  
			if (sockptr->Callsign[0] != '@') 
				ChangeSessionCallsign(Stream, EncodeCall(sockptr->Callsign));

			ShowConnections();
		}
                 
		*(LFPtr-1)=0;

        if (_stricmp(MsgPtr,"\\\\\\mon on") == 0)
            SetAppl(Stream, 128, 0);
		else
			if (_stricmp(MsgPtr,"\\\\\\mon off") == 0)
	            SetAppl(Stream, 0, 0);
            else
			{
				*(LFPtr-1)=13;

				// Line could be up to 500 chars if coming from a program rather than an interative user
				// Limit send to node to 255. Should really use PACLEN instead of 255....

				if (MsgLen > 255)
				{		
					SendMsg(Stream, MsgPtr, 255);
					SendMsg(Stream, MsgPtr + 255, MsgLen - 255);
				}
				else
					SendMsg(Stream, MsgPtr, MsgLen);
			}

	
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
		for (i = 0; i < NumberofUsers; i++)
		{
			if (strcmp(MsgPtr,UserRecPtr[i]->UserName) == 0)
			{
                sockptr->UserPointer = UserRecPtr[i];      //' Save pointer for checking password
                strcpy(sockptr->Callsign, UserRecPtr[i]->Callsign); //' for *** linked
                
                send(sock, PasswordMsg, strlen(PasswordMsg),0);
                
                sockptr->Retries = 0;
                
                sockptr->LoginState = 1;
                sockptr->InputLen = 0;

				n=sockptr->Number;

				ModifyMenu(hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n,MsgPtr);

				ShowConnections();

                return 0;
			}
		}
        
        //   Not found
        
        
        if (sockptr->Retries++ == 4)
		{
            send(sock,AttemptsMsg,sizeof(AttemptsMsg),0);
            Sleep (1000);
            closesocket(sock);
            DataSocket_Disconnect(sockptr);       //' Tidy up
		}
		else
		{        
            send(sock, LoginMsg, strlen(LoginMsg), 0);
            sockptr->InputLen=0;

		}

		return 0;
       
	case 1:
		   
		*(LFPtr-1)=0;				 // remove cr
        
        send(sock, NLMsg, 2, 0); // Need to echo NL, as password is not echoed
    
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
			Stream = sockptr->BPQStream;
			
			if (Stream == 0)
			{
				Stream = FindFreeStream();

				if (Stream == 255)
				{
					// no free streams - send error and close

					return 0;
				}

				sockptr->BPQStream = Stream;

				BPQSetHandle(Stream, MainWnd);
			}

			Connect(Stream);
  
			if (sockptr->Callsign[0] != ' ') 
				ChangeSessionCallsign(Stream, EncodeCall(sockptr->Callsign));
            
            sockptr->LoginState = 2;
            
            sockptr->InputLen = 0;
            
            if (strlen(cfgCTEXT) > 0)  send(sock,cfgCTEXT, strlen(cfgCTEXT),0);

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

			ShowConnections();

            return 0;
		}

		// Bad Password
        
        if (sockptr->Retries++ == 4)
		{
			send(sock,AttemptsMsg, strlen(AttemptsMsg),0);
            Sleep (1000);
            closesocket(sock);
            DataSocket_Disconnect (sockptr);      //' Tidy up
		}
		else
		{
			send(sock, PasswordMsg, strlen(PasswordMsg), 0);
			sockptr->InputLen=0;
		}

		return 0;

	default:

		return 0;

	}

	return 0;
}

int DataSocket_ReadRelay(struct ConnectionInfo * sockptr, SOCKET sock)
{
	int len=0, maxlen, InputLen, MsgLen, Stream, n;
	char NLMsg[3]={13,10,0};
	byte * LFPtr;
	byte * MsgPtr;
	char logmsg[120];
	char RelayMsg[] = "No CMS connection available - using local BPQMailChat\r";


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
			SendMsg(Stream, MsgPtr, 256);
			sockptr->InputLen -= 256;

			InputLen -= 256;

			memmove(MsgPtr,MsgPtr+256,InputLen);

			goto MsgLoop;
		}
			
		SendMsg(Stream, MsgPtr, InputLen);
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

		ModifyMenu(hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n,MsgPtr);

		ShowConnections();

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

		Stream = sockptr->BPQStream;
			
		if (Stream == 0)
		{
			Stream = FindFreeStream();

			if (Stream == 255)
			{
				// no free streams - send error and close

				return 0;
			}

			sockptr->BPQStream = Stream;

			BPQSetHandle(Stream, MainWnd);

		}

		send(sock, RelayMsg, strlen(RelayMsg), 0);

		Connect(Stream);
  
		if (sockptr->Callsign[0] != ' ') 
			ChangeSessionCallsign(Stream, EncodeCall(sockptr->Callsign));

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

		ShowConnections();
		InputLen=InputLen-(MsgLen+1);

		sockptr->InputLen=InputLen;

		// Connect to the BBS

		SendMsg(Stream, RelayAPPL, strlen(RelayAPPL));

		ShowConnections();
	
		return 0;
	
	default:

		return 0;

	}

	return 0;
}


int DataSocket_ReadFBB(struct ConnectionInfo * sockptr, SOCKET sock)
{
	int len=0, maxlen, InputLen, MsgLen, Stream, i, n;
	char NLMsg[3]={13,10,0};
	byte * LFPtr;
	byte * MsgPtr;
	char logmsg[120];

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
			SendMsg(Stream, MsgPtr, 256);
			sockptr->InputLen -= 256;

			InputLen -= 256;

			memmove(MsgPtr,MsgPtr+256,InputLen);

			goto MsgLoop;
		}
			
		SendMsg(Stream, MsgPtr, InputLen);
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
		for (i = 0; i < NumberofUsers; i++)
		{
			if (strcmp(MsgPtr,UserRecPtr[i]->UserName) == 0)
			{
                sockptr->UserPointer = UserRecPtr[i];      //' Save pointer for checking password
                strcpy(sockptr->Callsign, UserRecPtr[i]->Callsign); //' for *** linked
                                
                sockptr->Retries = 0;
                
                sockptr->LoginState = 1;
                sockptr->InputLen = 0;

				n=sockptr->Number;

				ModifyMenu(hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n,MsgPtr);

				ShowConnections();

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
            DataSocket_Disconnect(sockptr);       //' Tidy up
		}
		else
		{        
            send(sock, LoginMsg, strlen(LoginMsg), 0);
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
			Stream = sockptr->BPQStream;
			
			if (Stream == 0)
			{
				Stream = FindFreeStream();

				if (Stream == 255)
				{
					// no free streams - send error and close

					return 0;
				}

				sockptr->BPQStream = Stream;

				BPQSetHandle(Stream, MainWnd);
			}

			Connect(Stream);
  
			if (sockptr->Callsign[0] != ' ') 
				ChangeSessionCallsign(Stream, EncodeCall(sockptr->Callsign));

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

			ShowConnections();
			InputLen=InputLen-(MsgLen+1);

			sockptr->InputLen=InputLen;

			// What is left is the Command to connect to the BBS

			if (InputLen > 0)
			{
				memmove(MsgPtr,LFPtr+1,InputLen);
				MsgPtr[InputLen] = 13;
				SendMsg(Stream, MsgPtr, InputLen+1);
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
            DataSocket_Disconnect (sockptr);      //' Tidy up
		}
		else
		{
			send(sock, PasswordMsg, strlen(PasswordMsg), 0);
			sockptr->InputLen=0;
		}

		return 0;

	default:

		return 0;

	}

	return 0;
}



int DataSocket_Disconnect( struct ConnectionInfo * sockptr)
{
	int n;

	if (sockptr->SocketActive)
	{
		closesocket(sockptr->socket);

		Disconnect(sockptr->BPQStream);

		n=sockptr->Number;

		ModifyMenu(hDisMenu,n-1,MF_BYPOSITION | MF_STRING,IDM_DISCONNECT + n, ".");

		sockptr->SocketActive = FALSE;
	
		ShowConnections();
	}
	return 0;
}

int ShowConnections()
{
	char msg[80];
	struct ConnectionInfo * sockptr;
	int i,n;

	SendDlgItemMessage(MainWnd,100,LB_RESETCONTENT,0,0);

	for (n = 1; n <= CurrentSockets; n++)
	{
		sockptr=&ConnectionInfo[n];

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
		SendDlgItemMessage(MainWnd,100,LB_ADDSTRING,0,(LPARAM)msg);
	}

	return 0;
}

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

SOCKADDR_IN local_sin;  /* Local socket - internet style */

PSOCKADDR_IN psin;


BOOL Initialise()
{
	int status;	
		
	int           Error;              // catches return value of WSAStartup
	WORD          VersionRequested;   // passed to WSAStartup
	WSADATA       WsaData;            // receives data from WSAStartup
	BOOL          ReturnValue = TRUE; // return value

	HMENU hMenu;		// handle of menu 
	HKEY hKey=0;

	//	Register message for posting by BPQDLL

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	if (!ParseIniFile("BPQTelnetServer.cfg")) return FALSE;

    // Start WinSock 2.  If it fails, we don't need to call
    // WSACleanup().

    VersionRequested = MAKEWORD(VERSION_MAJOR, VERSION_MINOR);

	Error = WSAStartup(VersionRequested, &WsaData);
    
	if (Error)
	{
        MessageBox(NULL,
            "Could not find high enough version of WinSock",
            "Telnet Server", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        return FALSE;

    } else {

        // Now confirm that the WinSock 2 DLL supports the exact version
        // we want. If not, make sure to call WSACleanup().
 
		if (LOBYTE(WsaData.wVersion) != VERSION_MAJOR ||
            HIBYTE(WsaData.wVersion) != VERSION_MINOR) {
            MessageBox(NULL,
                "Could not find the correct version of WinSock",
                "Telnet Server",  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            WSACleanup();

            return FALSE;
        }
    }

//	Create listening socket(s)

	sock = socket( AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
	{
        wsprintf(szBuff, "socket() failed error %d", WSAGetLastError());
		MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
		return FALSE;
        
	}
 
	psin=&local_sin;

	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;
    psin->sin_port = htons(Port);        // Convert to network ordering 

 
    if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
	{
         wsprintf(szBuff, "bind(sock) failed Error %d", WSAGetLastError());

         MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
         closesocket( sock );

		 return FALSE;
	}

    if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
	{
		wsprintf(szBuff, "listen(sock) failed Error %d", WSAGetLastError());

		MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);

		return FALSE;
	}
   
	if ((status = WSAAsyncSelect( sock, MainWnd, WSA_ACCEPT, FD_ACCEPT)) > 0)
	{
		wsprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());

		MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);

		closesocket( sock );
		
		return FALSE;

	}

	if (FBBPort && FBBPort == Port)
	{
		FBBsock = sock;
	}
	else
	{
		FBBsock = socket( AF_INET, SOCK_STREAM, 0);

		if (FBBsock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "socket() failed error %d", WSAGetLastError());
			MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
			return FALSE;
        
		}
 
		psin=&local_sin;

		psin->sin_family = AF_INET;
		psin->sin_addr.s_addr = INADDR_ANY;
		psin->sin_port = htons(FBBPort);        // Convert to network ordering 

 
		if (bind( FBBsock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			 wsprintf(szBuff, "bind(FBBsock) failed Error %d", WSAGetLastError());

			 MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
			closesocket( FBBsock );

			return FALSE;
		}

		if (listen( FBBsock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(FBBsock) failed Error %d", WSAGetLastError());

			MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect( FBBsock, MainWnd, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());

			MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);

			closesocket( FBBsock );
		
			return FALSE;

		}
	}

	if (RelayPort)
	{
		RelayUser.UserName = _strdup("RMSRELAY");
		Relaysock = socket( AF_INET, SOCK_STREAM, 0);

		if (Relaysock == INVALID_SOCKET)
		{
			wsprintf(szBuff, "socket() failed error %d", WSAGetLastError());
			MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
			return FALSE;
	}
 
		psin=&local_sin;
		psin->sin_family = AF_INET;
		psin->sin_addr.s_addr = INADDR_ANY;
		psin->sin_port = htons(RelayPort);        // Convert to network ordering 

	    if (bind( Relaysock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			 wsprintf(szBuff, "bind(Relaysock) failed Error %d", WSAGetLastError());
			MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
			closesocket( Relaysock );

			return FALSE;
		}

		if (listen( Relaysock, MAX_PENDING_CONNECTS ) < 0)
		{
			wsprintf(szBuff, "listen(Relaysock) failed Error %d", WSAGetLastError());
			MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);

			return FALSE;
		}
   
		if ((status = WSAAsyncSelect( Relaysock, MainWnd, WSA_ACCEPT, FD_ACCEPT)) > 0)
		{
			wsprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());

			MessageBox(MainWnd, szBuff, "Telnet Server", MB_OK);
			closesocket(Relaysock);
		
			return FALSE;
		}
	}

	if (cfgMinToTray)
	
		AddTrayMenuItem(MainWnd, "Telnet Server");

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
	byte Msg[80];
	SOCKET sock;
	int n;

	for (n = 1; n <= CurrentSockets; n++)
	{
		if (Stream == ConnectionInfo[n].BPQStream)
		{
			sock=ConnectionInfo[n].socket;
    
            if(ConnectionInfo[n].FBBMode == 0)
			{
				wsprintf(Msg,"*** Connected to SWITCH\r\n");
//				wsprintf(Msg,"*** Connected using Stream %d\r\n",Stream);
				send(sock, Msg, strlen(Msg),0);
			}
			return 0;
		}
	}

	return 0;
}

int Disconnected (Stream)
{
	byte Msg[80];

	SOCKET sock;
	int n;

	for (n = 1; n <= CurrentSockets; n++)
	{
		if (Stream == ConnectionInfo[n].BPQStream)
		{
			sock=ConnectionInfo[n].socket;
    
            wsprintf(Msg,"*** Disconnected from Stream %d\r\n",Stream);

			send(sock, Msg, strlen(Msg),0);

			if (DisconnectOnClose)
			{
				Sleep(1000);
				DataSocket_Disconnect(&ConnectionInfo[n]);
			}
			return 0;
		}
	}

	return 0;
}

int DoReceivedData(int Stream)
{
	byte Buffer[400];
	int len,count,i;
	char * ptr;
	char * ptr2;
	SOCKET sock;
	int n;

	for (n = 1; n <= CurrentSockets; n++)
	{
		if (Stream == ConnectionInfo[n].BPQStream)
		{
			sock=ConnectionInfo[n].socket;

			do { 

				GetMsg(Stream, Buffer,&len, &count);

				if (len == 0) return 0;
				
				if (ConnectionInfo[n].FBBMode)
				{
					send(sock,Buffer,len,0);
				}
				else
				{
					// Replace cr with crlf

				ptr=&Buffer[0];

				do 
				{
					ptr2 = memchr(ptr,13,len);

					if (ptr2 == 0)
					{
						//	no cr, so just send as is 

						send(sock,ptr,len,0);
						i=0;
						break;
					}

					i=ptr2+1-ptr;

					send(sock,ptr,i,0);
					send(sock,"\n",1,0);

					len-=i;
					ptr=ptr2+1;
				}
				while (len>0);
				}
				}

			while (count > 0);

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

	int i,n,len,count;
	int stamp;

	char * ptr;
	char * ptr2;
	SOCKET sock;

	for (n = 1; n <= CurrentSockets; n++)
	{
		if (Stream == ConnectionInfo[n].BPQStream)
		{
			sock=ConnectionInfo[n].socket;

			do { 

			stamp=GetRaw(Stream, buff,&len,&count);
			len=DecodeFrame(buff,Buffer,stamp);

			if (len == 0) return 0;
				
			// Replace cr with crlf

			ptr=&Buffer[0];

			do 
			{
				ptr2 = memchr(ptr,13,len);

				if (ptr2 == 0)
				{
					//	no cr, so just send as is 

					send(sock,ptr,len,0);
					i=0;
					break;
				}

				i=ptr2+1-ptr;

				send(sock,ptr,i,0);
				send(sock,"\n",1,0);

				len-=i;
				ptr=ptr2+1;
			}
			while (len>0);
		}


		while (count > 0);	
   }
	}

	return 0;

}
int ConnectState(Stream)
{
	int state;

	SessionStateNoAck(Stream, &state);
	return state;
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
byte * EncodeCall(byte * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}

int ParseIniFile(char * fn)
{
	size_t i;
	FILE *file;
	char buf[256],errbuf[256];
	char * param;
	char * value;
	char * Context, *ptr, *User, *Pwd, *UserCall, *pos;
	HKEY hKey=0;
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
		wsprintf(buf,"Config file %s could not be opened ",Value);
		MessageBox(MainWnd, buf, "Telnet Server", MB_OK);
		return FALSE;
	}

	while(fgets(buf, 255, file) != NULL)
	{
		int End = strlen(buf) -1;
		
		strcpy(errbuf,buf);			// save in case of error

		if (buf[End] == 10)
			buf[End]=0;			// remove newline

		ptr=strchr(buf,'=');

		if (!ptr)
	
			continue;

		param=buf;
		*(ptr)=0;
		value=ptr+1;

		if (_stricmp(param,"LOGGING") == 0)
			LogEnabled = atoi(value);

		if (_stricmp(param,"DisconnectOnClose") == 0)
			DisconnectOnClose = atoi(value);

		if (_stricmp(param,"TCPPORT") == 0)
			Port = atoi(value);

		if (_stricmp(param,"FBBPORT") == 0)
			FBBPort = atoi(value);

		if (_stricmp(param,"RELAYAPPL") == 0)
		{
			RelayPort = 8778;
			strcpy(RelayAPPL, value);
			strcat(RelayAPPL, "\r");
		}

		//		if (strcmp(param,"LOGINRESPONSE") == 0) cfgLOGINRESPONSE = value;
//	    if (strcmp(param,"PASSWORDRESPONSE") == 0) cfgPASSWORDRESPONSE = value;

	    if (_stricmp(param,"LOGINPROMPT") == 0)
			strcpy(LoginMsg,value);

	    if (_stricmp(param,"PASSWORDPROMPT") == 0)
			strcpy(PasswordMsg, value);

	    if (_stricmp(param,"HOSTPROMPT") == 0)
			strcpy(cfgHOSTPROMPT, value);

		if (_stricmp(param,"LOCALECHO") == 0)
			strcpy(cfgLOCALECHO, value);

		if (_stricmp(param,"MAXSESSIONS") == 0)
		{
			MaxSessions = atoi(value);
			if (MaxSessions > MaxSockets ) MaxSessions = MaxSockets;
		}
		if (_stricmp(param,"CTEXT") == 0 )
			strcpy(cfgCTEXT,value);

		if (_stricmp(param,"USER") == 0)
		{
			User = strtok_s(value, ", ", &Context);
			Pwd = strtok_s(NULL, ", ", &Context);
			UserCall = strtok_s(NULL, ", ", &Context);

			if (User == 0 || Pwd == 0)
				// invalid record
				continue;

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

			if (NumberofUsers == 0)
				UserRecPtr=malloc(4);
			else
				UserRecPtr=realloc(UserRecPtr,(NumberofUsers+1)*4);

			UserRecPtr[NumberofUsers]=malloc(12);
			UserRecPtr[NumberofUsers]->Callsign=malloc(strlen(UserCall)+1);
			UserRecPtr[NumberofUsers]->Password=malloc(strlen(Pwd)+1);
			UserRecPtr[NumberofUsers]->UserName=malloc(strlen(User)+1);

			strcpy(UserRecPtr[NumberofUsers]->UserName,User);
			strcpy(UserRecPtr[NumberofUsers]->Password,Pwd);
			strcpy(UserRecPtr[NumberofUsers]->Callsign,UserCall);

			NumberofUsers += 1;
		}
	}

	fclose(file);

    if (NumberofUsers == 0)
	{
		wsprintf(szBuff,"No users in config file",Value);
		MessageBox(MainWnd,szBuff, "Telnet Server", MB_OK);
		return FALSE;
			
	}


	// Replace \n string with cr lf

	pos=&cfgCTEXT[0];

scanCTEXT:

    pos = strstr(pos, "\\n");
    
    if (pos)
	{
        
		*(pos++)=13;			// put in cr
		*(pos++)=10;			// put in cr

        goto scanCTEXT;
	}  

	return TRUE;

}

int WriteLog(char * msg)
{
	FILE *file;
	char timebuf[128];

	time_t ltime;
    struct tm today;
 
	if ((file = fopen("BPQTelnetServer.log","a")) == NULL)
		return FALSE;

	time( &ltime );

	_localtime64_s( &today, &ltime );

	strftime( timebuf, 128,
		"%d/%m/%Y %H:%M:%S ", &today );
    
	fputs(timebuf, file);

	fputs(msg, file);

	fclose(file);

	return 0;
}

