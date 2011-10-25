/*

 Program to allow Applications written for AGWPE to be used with BPQ

 Converted from the VB version, November 2006

 Version 1.0.0 
	
*/

//	Version 1.0.1 February 2008
//	accepts calls in 'd' packet either way round (initially for spider)

//	Version 1.2.0 January 2009
//	Add StartMinimized Command Line Option

//	Version 1.2.1 February 2009
//	Fix buffer overflow in Mon Decode

//	Version 1.2.2 February 2009

//		Get Registry Tree from BPQ32.dll

//	Version 1.2.3 October 2011

//		Call CloseBPQ32 on exit

#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"

#include "bpq32.h"

HWND MainWnd;
HINSTANCE hInst;
HWND DiagWnd;
HWND ConfigWnd;

#define BGCOLOUR RGB(236,233,216)

HBRUSH bgBrush;

HKEY REGTREE = HKEY_CURRENT_USER;

BOOL cfgMinToTray;

int controlStream;

char AGWPorts[500];
char BPQPorts[500];

byte AGWMessage[1000];

struct AGWHeader AGWTXHeader;

char SessionList[100];

struct BPQConnectionInfo Connections[65];

#define MaxSockets 64

struct SocketConnectionInfo Sockets[MaxSockets+1];

int CurrentConnections;

int CurrentSockets=0;

int Port = 0;
int Sessions = 0;
int	ApplMask = 0;
BOOL LoopMonFlag=FALSE;
BOOL Loopflag=FALSE;

char ClassName[]="AGWMAINWINDOW";
char ConnClassName[]="AGWCONNWINDOW";
char ConfigClassName[]="CONFIG";

char szBuff[ 80 ];

BOOL Initialise();
int SetUpHostSessions();
int DisplaySessions();
int DoStateChange(Stream);
int DoReceivedData(Stream);
int DoMonitorData(Stream);
int Connected(Stream);
int Disconnected(Stream);
int DeleteConnection(con);
int SendConMsgtoAppl(BOOL Incomming, int con, char * CallSign);
int SendDisMsgtoAppl(char * Msg, struct SocketConnectionInfo * sockptr);
int Socket_Accept(int SocketId);
int Socket_Data(int SocketId,int error, int eventcode);
int DataSocket_Read(struct SocketConnectionInfo * sockptr, SOCKET sock);
int DataSocket_Write(struct SocketConnectionInfo * sockptr, SOCKET sock);
int GetSessionKey(char * key, struct SocketConnectionInfo * sockptr);
int ProcessAGWCommand(struct SocketConnectionInfo * sockptr);
int SendDataToAppl(int Stream, byte * Buffer, int Length);
int InternalAGWDecodeFrame(char * msg, char * buffer, int Stamp, int * FrameType);
int DataSocket_Disconnect( struct SocketConnectionInfo * sockptr);
int SendRawPacket(struct SocketConnectionInfo * sockptr, char *txmsg, int Length);
int ShowApps();
int Terminate();
int SendtoSocket(SOCKET sock,char * Msg);

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow )

{
    MSG msg;				     // message
	
	if (!InitApplication(hInstance))
		return (FALSE);	


    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);


    while (GetMessage(&msg,(void *) NULL,0,0))		 
	{
		if (!IsDialogMessage(MainWnd,&msg))
		{
			TranslateMessage(&msg);	  
			DispatchMessage(&msg);
		}
    }
	
	Terminate();
    
	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	return (msg.wParam);	  
}

BOOL InitApplication(HINSTANCE hInstance)

{
    WNDCLASS  wc;

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = MainWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

    wc.lpfnWndProc = ConnWndProc;       
  	wc.lpszClassName = ConnClassName;

	RegisterClass(&wc);

    wc.lpfnWndProc = ConfigWndProc;       
  	wc.lpszClassName = ConfigClassName;

	return (RegisterClass(&wc));

}



BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int i;
    HWND  hWnd;             

	hInst = hInstance;

	REGTREE = GetRegistryKey();

    // Create a dialog box as the main window

	hWnd=CreateDialog(hInst,ClassName,0,NULL);
	

    if (!hWnd)
	{
		i=GetLastError();
        return (FALSE);
	}

	MainWnd=hWnd;

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


HWND ConnWnd;


int FAR PASCAL MainWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{

	int cmd,id,state,change,i;
	HWND hwndChild, hwndList;
	RECT * lprc;
	char * ApplMsg[10];

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

	case WM_SIZING:

		lprc = (LPRECT) lParam;

		hwndList = GetDlgItem(MainWnd, IDC_TEXTBOX3); 
 
		MoveWindow(hwndList,2,2,lprc->right-lprc->left-12,
			lprc->bottom-lprc->top-60,TRUE);



		return TRUE;

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:

		SetBkColor((HDC)wParam,BGCOLOUR);
		return (long)bgBrush;


	case WM_COMMAND:	

		id = LOWORD(wParam);
        hwndChild = (HWND)(UINT)lParam;
        cmd = HIWORD(wParam);
  

		if (id == IDC_SHOWAPPS)
		{
			if (ConnWnd == 0)
			{		
				ConnWnd=CreateDialog(hInst,ConnClassName,0,NULL);
    
				if (!ConnWnd)
				{
					i=GetLastError();
					return (FALSE);
				}
				ShowWindow(ConnWnd, SW_SHOW);  
				UpdateWindow(ConnWnd); 

				if (cfgMinToTray) AddTrayMenuItem(ConnWnd, "AGWtoBPQ Connections");
			}

			ShowApps();

			SetForegroundWindow(ConnWnd);

			return(0);
		}
			
			
		if (id == IDC_SAVE)
		{
			if (ConfigWnd == 0)
			{		
				ConfigWnd=CreateDialog(hInst,ConfigClassName,0,NULL);
    
				if (!ConfigWnd)
				{
					i=GetLastError();
					return (FALSE);
				}
				ShowWindow(ConfigWnd, SW_SHOW);  
				UpdateWindow(ConfigWnd); 

				SetDlgItemInt(ConfigWnd,1001,Port,FALSE);
				SetDlgItemInt(ConfigWnd,1002,Sessions,FALSE);
				wsprintf((LPSTR)ApplMsg,"0x%x",ApplMask);
				SetDlgItemText(ConfigWnd,1003,(LPSTR)ApplMsg);
				CheckDlgButton(ConfigWnd,1004,Loopflag);
				CheckDlgButton(ConfigWnd,1005,LoopMonFlag);

  			}

			SetForegroundWindow(ConfigWnd);

			return(0);
		}
		return 0;


    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		break;

	case WM_CLOSE:
	
		if (cfgMinToTray)	
			DeleteTrayMenuItem(MainWnd);

		return(DestroyWindow(hWnd));
			  

	case WM_DESTROY:		  
	    PostQuitMessage(0);
		return(0);

	}		
	
	return (DefWindowProc(hWnd, message, wParam, lParam));

}

int FAR PASCAL ConnWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{

	int cmd,id;
	HWND hwndChild;


	switch (message)
	{

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:

		SetBkColor((HDC)wParam,BGCOLOUR);
		return (long)bgBrush;

	case WM_COMMAND:	

		id = LOWORD(wParam);
        hwndChild = (HWND)(UINT)lParam;
        cmd = HIWORD(wParam);
  

    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);
	
		return (0);

	case WM_CLOSE:
	
		if (cfgMinToTray)
			DeleteTrayMenuItem(ConnWnd);

		return(DestroyWindow(hWnd));
			  

	case WM_DESTROY:

		ConnWnd=0;
	
		return(0);

	}		
	
	return (DefWindowProc(hWnd, message, wParam, lParam));

}

int FAR PASCAL ConfigWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{

	int cmd,id;
	HWND hwndChild;
	BOOL OK1,OK2,OK3;

	HKEY hKey=0,hSubKey=0;
	int retCode,disp;
	char ApplTxt[10];

	switch (message)
	{

	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:

		SetBkColor((HDC)wParam,BGCOLOUR);
		return (long)bgBrush;

	case WM_COMMAND:	

		id = LOWORD(wParam);
        hwndChild = (HWND)(UINT)lParam;
        cmd = HIWORD(wParam);

		switch (id)
		{
		case IDC_SAVE:

			Port=GetDlgItemInt(ConfigWnd,1001,&OK1,FALSE);
			Sessions=GetDlgItemInt(ConfigWnd,1002,&OK2,FALSE);


			ApplMask=GetDlgItemText(ConfigWnd,1003,(LPSTR)ApplTxt,9);
			OK3=sscanf(ApplTxt,"%x",&ApplMask);
			Loopflag=IsDlgButtonChecked(ConfigWnd,1004);
			LoopMonFlag=IsDlgButtonChecked(ConfigWnd,1005);

			if (OK1 && OK2 && OK3==1)
			{
				retCode = RegCreateKeyEx(REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\AGWtoBPQ",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

				if (retCode == ERROR_SUCCESS)
				{
					retCode = RegSetValueEx(hKey,"Port",0,REG_DWORD,(BYTE *)&Port,4);
					retCode = RegSetValueEx(hKey,"Sessions",0,REG_DWORD,(BYTE *)&Sessions,4);
					retCode = RegSetValueEx(hKey,"ApplMask",0,REG_DWORD,(BYTE *)&ApplMask,4);
					retCode = RegSetValueEx(hKey,"LoopRawFrames",0,REG_DWORD,(BYTE *)&Loopflag,4);
					retCode = RegSetValueEx(hKey,"LoopMonFrames",0,REG_DWORD,(BYTE *)&LoopMonFlag,4);
  
					RegCloseKey(hKey);
				}
				return(DestroyWindow(hWnd));
			}

			// Validation failed

			if (!OK1) SetDlgItemText(ConfigWnd,1001,"????");
			if (!OK2) SetDlgItemText(ConfigWnd,1002,"????");
			if (!OK3) SetDlgItemText(ConfigWnd,1003,"????");

			break;

			case 1009:

				return(DestroyWindow(hWnd));

		}

		break;


	case WM_CLOSE:
	
		if (cfgMinToTray)
	
			DeleteTrayMenuItem(ConnWnd);

		return(DestroyWindow(hWnd));
			  

	case WM_DESTROY:

		ConfigWnd=0;
	
		return(0);

	}		
	
	return (DefWindowProc(hWnd, message, wParam, lParam));

}


BOOL APIENTRY About(HWND hDlg, UINT msg, UINT wParam, LONG lParam)

{
    switch (msg) {
	case WM_INITDIALOG:		  
	    return (TRUE);

	case WM_COMMAND:		    
	    if (wParam == IDOK                
                || wParam == IDCANCEL) {   
		EndDialog(hDlg, TRUE);	     
		return (TRUE);
	    }
	    break;
    }
    return (FALSE);			    
}

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

SOCKADDR_IN local_sin;  /* Local socket - internet style */

PSOCKADDR_IN psin;

SOCKET sock;

BOOL Initialise()
{
	int status;	
		
	int           Error;              // catches return value of WSAStartup
    WORD          VersionRequested;   // passed to WSAStartup
    WSADATA       WsaData;            // receives data from WSAStartup
    BOOL          ReturnValue = TRUE; // return value
	int retCode,Type,Vallen;

	HKEY hKey=0;
	//
	//	Register message for posting by BPQDLL
	//



	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	// Get config from Registry 

	retCode = RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\AGWtoBPQ",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Port",0,			
			(ULONG *)&Type,(UCHAR *)&Port,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Sessions",0,			
			(ULONG *)&Type,(UCHAR *)&Sessions,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ApplMask",0,			
			(ULONG *)&Type,(UCHAR *)&ApplMask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"LoopRawFrames",0,			
			(ULONG *)&Type,(UCHAR *)&Loopflag,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"LoopMonFrames",0,			
			(ULONG *)&Type,(UCHAR *)&LoopMonFlag,(ULONG *)&Vallen);
		
	}

	if (Port == 0)

		// Not configured

		PostMessage(MainWnd, WM_COMMAND, IDC_SAVE, 0);

	SetUpHostSessions();

	DisplaySessions();


    // Start WinSock 2.  If it fails, we don't need to call
    // WSACleanup().

    VersionRequested = MAKEWORD(VERSION_MAJOR, VERSION_MINOR);

	Error = WSAStartup(VersionRequested, &WsaData);
    
	if (Error)
	{
        MessageBox(NULL,
            "Could not find high enough version of WinSock",
            "AGWtoBPQ", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        return FALSE;

    } else {

        // Now confirm that the WinSock 2 DLL supports the exact version
        // we want. If not, make sure to call WSACleanup().
 
		if (LOBYTE(WsaData.wVersion) != VERSION_MAJOR ||
            HIBYTE(WsaData.wVersion) != VERSION_MINOR) {
            MessageBox(NULL,
                "Could not find the correct version of WinSock",
                "AGWtoBPQ",  MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
            WSACleanup();

            return FALSE;
        }
    }


//	Create listening socket

	sock = socket( AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
	{
        sprintf(szBuff, "socket() failed error %d", WSAGetLastError());
		MessageBox(MainWnd, szBuff, "AGWtoBPQ", MB_OK);
		return FALSE;
        
	}
 
	psin=&local_sin;

	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;
    psin->sin_port = htons(Port);        /* Convert to network ordering */

 
    if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
	{
         sprintf(szBuff, "bind(sock) failed Error %d", WSAGetLastError());

         MessageBox(MainWnd, szBuff, "AGWtoBPQ", MB_OK);
         closesocket( sock );

		 return FALSE;
	}

    if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
	{

		sprintf(szBuff, "listen(sock) failed Error %d", WSAGetLastError());

		MessageBox(MainWnd, szBuff, "AGWtoBPQ", MB_OK);

		return FALSE;
	}
   
	if ((status = WSAAsyncSelect( sock, MainWnd, WSA_ACCEPT, FD_ACCEPT)) > 0)
	{

		sprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());

		MessageBox(MainWnd, szBuff, "AGWtoBPQ", MB_OK);

		closesocket( sock );
		
		return FALSE;

	}

	controlStream = FindFreeStream();

	if (controlStream == 255)
	{
		MessageBox (MainWnd,"No free BPQ Streams found",NULL,MB_ICONSTOP);
		return(FALSE);
	}

	//
	//	Enable Async notification
	//
	
	BPQSetHandle(controlStream, MainWnd);

	Connect(controlStream);					// Connect

	SetAppl(controlStream,128,0);			// Request Monitoring

	SendMsg(controlStream, "P\r", 2);		// Request Ports listing

	if (cfgMinToTray)
	
		AddTrayMenuItem(MainWnd, "AGWtoBPQ");


//If Debugging Then Open "AGWtoBPQ.dbg" For Output As 10

	return TRUE;

}


int SetUpHostSessions()
{
	int Stream, i;

	if (ApplMask == 0) return 0;
	
	for (i = 1; i <= Sessions; i++)
	{           
		Stream = FindFreeStream();
  
		if (Stream == 255) break;
    
		BPQSetHandle(Stream, MainWnd);	
		
		SetAppl(Stream, 0, ApplMask);
    
		Connections[CurrentConnections].CallKey[0] = 0;
		Connections[CurrentConnections].BPQStream = Stream;
		Connections[CurrentConnections].SocketIndex = 0;
		Connections[CurrentConnections].Connecting = FALSE;
		Connections[CurrentConnections].Listening = TRUE;
		Connections[CurrentConnections].ApplMask = ApplMask;
          
		CurrentConnections++;

	}

	DisplaySessions();

return 0;

}

int DisplaySessions()
{
	char * ptr;
	int i, con;

	byte key[21]; // As String, char As String

	strcpy (SessionList,"   Port    Calls         Stream Socket Connecting Listening Mask");
  	
	SendDlgItemMessage(MainWnd,IDC_TEXTBOX3,LB_RESETCONTENT,0,0);
	SendDlgItemMessage(MainWnd,IDC_TEXTBOX3,LB_ADDSTRING,0,(LPARAM) SessionList);

	for (con = 0; con < CurrentConnections; con++)
	{
		memcpy(key,Connections[con].CallKey,21);

		if (key[0] == 0) key[0] = 32;


		key[10]=0;
		key[20]=0;

		ptr=&SessionList[0];

		i=wsprintf(ptr,"%2d %2c ",con,key[0]);

		ptr+=i;

		i=wsprintf(ptr,"%-10s%-10s ",&key[1],&key[11]);

		ptr+=i;

		i=wsprintf(ptr,"%2d     %2d      %s     %s    %x",
			Connections[con].BPQStream,
			(Connections[con].SocketIndex == 0) ? 0 : Connections[con].SocketIndex->Number,
			(Connections[con].Connecting == 0) ? "FALSE" : "TRUE ",
			(Connections[con].Listening == 0) ? "FALSE" : "TRUE ",
			Connections[con].ApplMask);

		SendDlgItemMessage(MainWnd,IDC_TEXTBOX3,LB_ADDSTRING,0,(LPARAM) SessionList);
	}

	return (0);

}

int Connected(Stream)
{
	byte ConnectingCall[10];
	byte * ApplCallPtr;
	byte * keyptr;
	byte ApplCall[10]="";
	byte ErrorMsg[100];
	int i;

	int ApplNum,con;
	struct SocketConnectionInfo * sockptr;

	if (Stream == controlStream) return 0; // Only used to get ports and monitor data

	GetCallsign(Stream, ConnectingCall);

	for (i=9;i>0;i--)
		if (ConnectingCall[i]==32)
			ConnectingCall[i]=0;

	ApplNum = GetApplNum(Stream);

	if (ApplNum == 0) 
	{
		return 0; // Cant be an incomming connect
	}
	ApplCallPtr = GetApplCall(ApplNum);

	if (ApplCallPtr != 0) memcpy(ApplCall,ApplCallPtr,10);

	// Convert trailing spaces to nulls
	
	for (i=9;i>0;i--)
		if (ApplCall[i]==32)
			ApplCall[i]=0;

//   See if incomming connection

	for (con = 0; con < CurrentConnections; con++)
	{
		if (Connections[con].BPQStream == Stream)
		{
			if (Connections[con].Listening)
			{    
				//	Allocate Session and send "c" Message
				//
				//	Find an AGW session
                          
				for (sockptr=&Sockets[1]; sockptr <= &Sockets[CurrentSockets]; sockptr++)
				{
					if (sockptr->SocketActive && (memcmp(sockptr->CallSign,ApplCall,10) ==0))
					{

						// Create Key
            
						keyptr=(byte *)&Connections[con].CallKey;

						*(keyptr++)='1';
						memcpy(keyptr,sockptr->CallSign, 10);
						keyptr+=10;
						memcpy(keyptr,ConnectingCall, 10);
                        		
						// Make sure key is not already in use

						for (i = 0; i < CurrentConnections; i++)
						{
							if (i == con) continue;		// Dont Check ourself!

							if (Connections[i].SocketIndex == sockptr &&
									memcmp(&Connections[con].CallKey,&Connections[i].CallKey,21) == 0)
							{
								SendMsg(Stream, "AGWtoBPQ - Callsign is already connected\r", 43);
								Sleep (500);
								Disconnect(Stream);
								
								Connections[con].CallKey[0]=0;

								return 0;
							}
						}

						Connections[con].Listening = FALSE;
						Connections[con].SocketIndex = sockptr; 

						DisplaySessions();
                      
						SendConMsgtoAppl(TRUE, con, ConnectingCall);

						return 0;
   					}
				}

				SendMsg(Stream, "No AGWPE Host Sessions available\r", 33);
				Sleep (500);
                Disconnect(Stream);   // disconnect
				return (0);
			}

		// Not listening ??

			OutputDebugString("Inbound Connection on Outgoing Stream");

			SendMsg(Stream, "AGWtoBPQ - Inbound Connection on Outgoing Stream\r", 49);
			Sleep (500);
            Disconnect(Stream);   // disconnect
			return (0);

		}
	} 

	OutputDebugString("Connection on undefined stream");

	i=wsprintf(ErrorMsg,"AGWtoBPQ - Connection on undefined stream %d\r",Stream);
	SendMsg(Stream, ErrorMsg,i);
	Sleep (500);
    Disconnect(Stream);   // disconnect
	return 0;
}

int Disconnected (Stream)
{
	int con;
	struct SocketConnectionInfo * sockptr;	
	char key[21];

	if (Stream == controlStream) return 0;

	for (con = 0; con < CurrentConnections;con++)
	{
		if (Connections[con].BPQStream == Stream)
		{
			memcpy(key,Connections[con].CallKey,21);

			sockptr = Connections[con].SocketIndex;
        
			if (sockptr != 0)
			{  
				AGWTXHeader.Port = key[0] - 49;
               
                memcpy(&AGWTXHeader.callfrom,&key[11],10);
				memcpy(&AGWTXHeader.callto,&key[1],10);

				//   Send a "d" Message
        
				// DisMsg = "*** DISCONNECTED From Station "

 				SendDisMsgtoAppl("*** DISCONNECTED From Station ", sockptr);
		
			}

			if (Connections[con].ApplMask != 0)
			{
				Connections[con].Listening = TRUE;
				Connections[con].SocketIndex = 0;
				memset(&Connections[con].CallKey ,0 ,21);
            
				DisplaySessions();
			}
			else
			{
				DeleteConnection(con);
			}
		
			return 0;

		}
	}


	return 0;

}
int DoReceivedData(int Stream)
{
	byte Buffer[400];
	int len,count,i,portcount;
	char * ptr;
	char * ptr2;

//Dim n As Integer, i As Integer, j As Integer, portcount As Integer
//Dim start As Integer



do { 

	GetMsg(Stream, Buffer,&len, &count);

    if (Stream == controlStream)
	{
        //
        //   used to get BPQ Port list - once complete, reformat to AGW form
        //

        if (AGWPorts[0] == 0)
		{
            strncat(BPQPorts, Buffer, len);
        
			len = strlen(BPQPorts);

			if (BPQPorts[len-1] == '\r')
			{       
				ptr = strchr(BPQPorts, '\r');
                
				ptr++;

				strcpy(BPQPorts, ptr);
        
                portcount = strlen(BPQPorts) / 35;
     
                ptr = _itoa(portcount,AGWPorts,10);
				
				*(++ptr)=';';
				*(++ptr)=0;
	
				ptr2=&BPQPorts[4];

                for (i=1; i <= portcount; i++)
				{
					memcpy(ptr,"Port",4);
					ptr = _itoa(i,ptr+4,10);
					ptr+=1;
					memcpy(ptr," with ",6);
					ptr+=6;
					memcpy(ptr,ptr2,29);		// ";"
					ptr+=29;
					ptr2+=35;
					
					while (*(--ptr) == ' ') {}

					ptr++;

					*(ptr++)=';';
				}
	    
				*(ptr)=0;
              
                Disconnect(controlStream);   // Finished with connection
            }
      

		}
       
	}
	else
	{
        
		if (len > 0) SendDataToAppl(Stream, Buffer,len);

	}
    
	}while (count > 0);
	
	return 0;
}

int DoMonitorData(int Stream)
{
	byte Buffer[500];
	int RawLen,Length,Count;
	byte Port;
	struct SocketConnectionInfo * sockptr;	
	byte AGWBuffer[1000];
	int n;
	int Stamp, Frametype;
	BOOL RXFlag, NeedAGW;

	do
	{
		Stamp=GetRaw(Stream, Buffer, &RawLen, &Count );
 
//'   4 byte chain
//'   1 byte port - top bit = transmit
//'   2 byte length (LO-HI)

		if (RawLen < 7) return 0;

		Port = Buffer[4];

		if (Port > 127)
		{
			RXFlag = FALSE;
			Port = Port - 128;
		}
		else
		{
		    RXFlag = TRUE;
		}

		NeedAGW = FALSE;

		for (n = 1; n<= CurrentSockets; n++)
		{
			sockptr=&Sockets[n];

			if (sockptr->SocketActive && sockptr->MonFlag) NeedAGW = TRUE;
		}

		if (NeedAGW)
		{
			if (RXFlag || LoopMonFlag)    // only send txed frames if requested
			{
				Length = InternalAGWDecodeFrame(Buffer, AGWBuffer,Stamp, &Frametype);
	 
				//
				//   Decode frame and send to applications which have requested monitoring
				//
				if (Length > 0)
				{
					AGWTXHeader.Port = Port - 1;       // AGW Ports start from 0
        
					if (Frametype == 3)
					{
						AGWTXHeader.DataKind = 'U';
					}
					else
					{
						if (Frametype && 1 == 0)
						{
							AGWTXHeader.DataKind = 'I';
						}
						else
						{
							AGWTXHeader.DataKind = 'S';
						}
					}

                      
           /* For i = 8 To 17
        
                cChar = Asc(Mid(AGWBuffer, i - 1))
            
                If cChar = 32 Then Exit For
            
                AGWTXHeader(i) = cChar
            
            Next i
        
            j = i + 3
        
            For i = 18 To 27
        
                cChar = Asc(Mid(AGWBuffer, j))
            
                If cChar = 32 Then Exit For
            
                AGWTXHeader(i) = cChar
            
                j = j + 1
            
            Next i

        */

				    AGWTXHeader.DataLength = Length;
       
	 				for (n = 1; n<= CurrentSockets; n++)
					{
						sockptr=&Sockets[n];
        
						if (sockptr->SocketActive && sockptr->MonFlag)
							SendRawPacket(sockptr, AGWBuffer, Length);
					}
				}
			}
		}

		RawLen = RawLen - 6;
 
		if (RXFlag || Loopflag) // Send transmitted frames if requested
		{

        //
        //  Send raw data to any sockets that have requested Raw frames
        //
        
			Buffer[6]=0;
       
			AGWTXHeader.Port = Port - 1;       // AGW Ports start from 0
			AGWTXHeader.DataKind = 'K';
        
			AGWTXHeader.DataLength = RawLen;
         
	 		for (n = 1; n<= CurrentSockets; n++)
			{
				sockptr=&Sockets[n];
       
				if (sockptr->SocketActive && sockptr->RawFlag)
					SendRawPacket(sockptr, &Buffer[6], RawLen);
        
			}
		}
		
   }

while (Count > 0);

	return 0;

}

int DeleteConnection(con)
{
	int i;

	//
	//	remove specified session
	//

    SetAppl(Connections[con].BPQStream, 0, 0);

	Disconnect(Connections[con].BPQStream);
 
    DeallocateStream(Connections[con].BPQStream);

//   move all down one

	for (i = con; i <= CurrentConnections - 2; i++)
	{
	    memcpy(&Connections[i],&Connections[i + 1],sizeof ConInfoRec);  
	}
    
	CurrentConnections--;

	DisplaySessions();

	return 0;
}

int SendConMsgtoAppl(BOOL Incomming, int con, char * CallSign)
{
	char key[21];
	char ConMsg[80]="*** CONNECTED ";
	struct SocketConnectionInfo * sockptr;


    memcpy(key,&Connections[con].CallKey,21);
        
    sockptr = Connections[con].SocketIndex;
        
    AGWTXHeader.Port = key[0] - 49;
            
    memcpy(AGWTXHeader.callfrom, &key[11],10);
        
    memcpy(AGWTXHeader.callto, &key[1],10);
            
/*    '
    '   Send a "C" Message
    '
'01 00 00 00 43 00 00 00 47 4D 38 42 50 51 2D 34    C   GM8BPQ-4
'00 EA 47 4D 38 42 50 51 2D 34 00 FF 25 00 00 00  êGM8BPQ-4 ÿ%
'00 00 00 00 2A 2A 2A 20 43 4F 4E 4E 45 43 54 45     *** CONNECTE
'44 20 57 69 74 68 20 53 74 61 74 69 6F 6E 20 47 D With Station G
'4D 38 42 50 51 2D 34 0D 00 M8BPQ-4
*/

    AGWTXHeader.DataKind = 'C';

    AGWTXHeader.PID = 0;
                    
    if (Incomming)	
		strcat(ConMsg,"To");
	else
		strcat(ConMsg,"With");
    
    strcat(ConMsg," Station ");
    strcat(ConMsg,CallSign);
                
    AGWTXHeader.DataLength = strlen(ConMsg)+1;
               
	SendtoSocket(sockptr->socket, ConMsg);
            
	return 0;

}




int SendDisMsgtoAppl(char * Msg, struct SocketConnectionInfo * sockptr)
{
    byte DisMsg[100];
            
    strcpy(DisMsg,Msg);
	strcat(DisMsg,(const char *)&AGWTXHeader.callfrom);
            
    AGWTXHeader.Port = sockptr->AGWRXHeader.Port;
    AGWTXHeader.DataKind = 'd';
 
    strcat(DisMsg,"\r\0");
                                
    AGWTXHeader.DataLength = strlen(DisMsg)+1;
            
	SendtoSocket(sockptr->socket, DisMsg);

	return 0;

}



int Socket_Accept(int SocketId)
{
	int n,addrlen;
	struct SocketConnectionInfo * sockptr;
	SOCKET sock;

//   Find a free Socket

	for (n = 1; n <= MaxSockets; n++)
	{
		sockptr=&Sockets[n];
		
		if (sockptr->SocketActive == FALSE)
		{
			addrlen=sizeof(struct sockaddr);

			sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

			if (sock == INVALID_SOCKET)
			{
				sprintf(szBuff, " accept() failed Error %d", WSAGetLastError());
				MessageBox(MainWnd, szBuff, "AGWtoBPQ", MB_OK);
				return FALSE;
			}

			WSAAsyncSelect(sock, MainWnd, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
			sockptr->socket = sock;
			sockptr->SocketActive = TRUE;
			sockptr->GotHeader = FALSE;
			sockptr->MsgDataLength = 0;
			sockptr->Number = n;

			if (CurrentSockets < n) CurrentSockets=n;  //Record max used to save searching all entries

			ShowApps();

			return 0;
		}
	}

	// Should accept, then immediately close

	return 0;
}

int SendtoSocket(SOCKET sock,char * Msg)
{
	int len;
	
	len=AGWTXHeader.DataLength;
	
	send(sock,(char *)&AGWTXHeader, 36,0);
	if (len > 0) send(sock, Msg, len,0);

	return 0;
}




int Socket_Data(int sock, int error, int eventcode)
{
	int n;
	struct SocketConnectionInfo * sockptr;

	//	Find Connection Record

	for (n = 1; n <= CurrentSockets; n++)
	{
		sockptr=&Sockets[n];
		
		if (sockptr->socket == sock && sockptr->SocketActive)
		{
			switch (eventcode)
			{
				case FD_READ:

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

int DataSocket_Read(struct SocketConnectionInfo * sockptr, SOCKET sock)
{
	int i;
	int DataLength;

	ioctlsocket(sock,FIONREAD,&DataLength);

	if (sockptr->GotHeader)
	{
		// Received a header, without sufficient data bytes
   
		if (DataLength < sockptr->MsgDataLength)
		{
			// Fiddle - seem to be problems somtimes with un-Neagled hosts
        
			Sleep(500);

			ioctlsocket(sock,FIONREAD,&DataLength);
		}
		
		if (DataLength >= sockptr->MsgDataLength)
		{
			//   Read Data and Process Command
    
			i=recv(sock, AGWMessage, sockptr->MsgDataLength, 0);

			ProcessAGWCommand (sockptr);
        
			sockptr->GotHeader = FALSE;
		}

		// Not Enough Data - wait

	}
	else	// Not got header
	{
		if (DataLength > 35)//         A header
		{
			i=recv(sock,(char *)&sockptr->AGWRXHeader, 36, 0);
            
			if (i == SOCKET_ERROR)
			{
				i=WSAGetLastError();

				if (i == WSAECONNABORTED)
				{
					DataSocket_Disconnect(sockptr);
				}

			}

			
			sockptr->MsgDataLength = sockptr->AGWRXHeader.DataLength;

			if (sockptr->MsgDataLength > 500)
				OutputDebugString("Corrupt AGW message");

            
		    if (sockptr->MsgDataLength == 0)
			{
				ProcessAGWCommand (sockptr);
			}
			else
			{
				sockptr->GotHeader = TRUE;            // Wait for data
			}

		} 
		
		// not got 36 bytes

	}
	
	return 0;
}


int ProcessAGWCommand(struct SocketConnectionInfo * sockptr)
{
	int AGWVersion[2]={2003,999};
	char AGWRegReply[1];
	struct BPQConnectionInfo * Connection;
	int Stream;
	char AXCall[10];
	char TXMessage[500];
	int Digis,MsgStart,j;
	byte * TXMessageptr;
	char key[21];
	char ToCall[10];
	char ConnectMsg[20];
	int con,conport;
	char AGWYReply[4]={0,0,0,0};

	switch (sockptr->AGWRXHeader.DataKind)
	{
	case 'C':

    
        //   Connect
        
        //   Create Session Key from port and callsign pair

		GetSessionKey(key, sockptr);
            
        memcpy(ToCall, &key[11],10);
               
        Stream = FindFreeStream();
  
        if (Stream == 255) return 0;

        BPQSetHandle(Stream, MainWnd);	

		Connection=&Connections[CurrentConnections];

        memcpy(&Connection->CallKey,key,21);
        Connection->BPQStream = Stream;
        Connection->SocketIndex = sockptr;
        Connection->Connecting = TRUE;
        
        Connect(Stream);				// Connect

        
        //LinkedMsg = "*** Linked to " & RTrim$(Sockets(Index).CallSign) & vbCr
        
        //Debug.Print LinkedMsg
        //Debug.Print BPQCtrl1.SendData(Stream, LinkedMsg, Len(LinkedMsg))
        
		ConvToAX25(sockptr->CallSign, AXCall);
		ChangeSessionCallsign(Stream, AXCall);

        DisplaySessions();
        
        if (memcmp(ToCall,"SWITCH",6) == 0)
		{
			//  Just connect to command level on switch
            
            SendConMsgtoAppl(FALSE, CurrentConnections, ToCall);
            Connection->Connecting = FALSE;
		} 
        else
		{
 
			// Need to convert port index (used by AGW) to port number

			conport=GetPortNumber(key[0]-48);

			wsprintf(ConnectMsg,"C %d %s\r",conport,ToCall);
            SendMsg(Stream, ConnectMsg, strlen(ConnectMsg));

		}

        CurrentConnections++;

        DisplaySessions();

		return 0;
        
	case 'D':
   
        //   Send Data
        //
        
        //   Create Session Key from port and callsign pair
        
		
        GetSessionKey(key, sockptr);

        for (con = 0; con < CurrentConnections; con++)
		{
			if (memcmp(Connections[con].CallKey,key,21) == 0)
			{
				SendMsg(Connections[con].BPQStream, AGWMessage, sockptr->MsgDataLength);
				return 0;
			}
		}

		return 0;


  
	case 'd':

    
    //   Disconnect
            
         
        memcpy(AGWTXHeader.callto,sockptr->AGWRXHeader.callfrom,10);

        memcpy(AGWTXHeader.callfrom,sockptr->AGWRXHeader.callto,10);
        
        SendDisMsgtoAppl("*** DISCONNECTED RETRYOUT With ", sockptr);
   
        GetSessionKey(key, sockptr);
 
        for (con = 0; con < CurrentConnections; con++)
		{
			if (memcmp(Connections[con].CallKey,key,21) == 0)
			{ 
                Disconnect(Connections[con].BPQStream);

				return 0;
			}
		}

		// There is confusion about the correct ordring of calls in the "d" packet. AGW appears to accept either,
		//	so I will too.

		memset(&key[1],0,20);
		strcpy(&key[1],sockptr->AGWRXHeader.callto);
		strcpy(&key[11],sockptr->AGWRXHeader.callfrom);

		for (con = 0; con < CurrentConnections; con++)
		{
			if (memcmp(Connections[con].CallKey,key,21) == 0)
			{ 
                Disconnect(Connections[con].BPQStream);
				return 0;
			}
		}

		return 0;


	case 'R':
    
    //   Version
    
        memset(&AGWTXHeader,0,36);
    
        AGWTXHeader.DataKind = 'R';

        AGWTXHeader.DataLength = 8;       // Length
    
        SendtoSocket(sockptr->socket, (char *)&AGWVersion[0]);

		return 0;
    

	case 'G':

        //   Port info. String is in AGWPorts
        
        
        memset(&AGWTXHeader,0,36);

        AGWTXHeader.DataKind = 'G';

        AGWTXHeader.DataLength = strlen(AGWPorts)+1;     // Length
    
        SendtoSocket(sockptr->socket, AGWPorts);

		return 0;

    
	case 'k':

       //   Toggle Raw receive

        sockptr->RawFlag = !sockptr->RawFlag;
        
		return 0;

	case 'K':

        // Send Raw Frame
 
		SendRaw(sockptr->AGWRXHeader.Port+1,&AGWMessage[1], sockptr->MsgDataLength - 1);
        return 0;


	case 'm':
     
       //   Toggle Monitor receive
    
        sockptr->MonFlag = !sockptr->MonFlag;
		return 0;
    
  
	case 'M':
	case 'V':         // Send UNProto Frame "V" includes Via string
  
   
        ConvToAX25(sockptr->AGWRXHeader.callto,TXMessage);
		ConvToAX25(sockptr->AGWRXHeader.callfrom,&TXMessage[7]);

		Digis=0;
        MsgStart = 0;

        if (sockptr->AGWRXHeader.DataKind == 'V')	// Unproto with VIA string
		{        
            Digis = AGWMessage[0];                 // Number of digis
                    
			for (j = 1; j<= Digis; j++)
			{
				ConvToAX25(&AGWMessage[(j - 1) * 10 + 1],&TXMessage[7+(j*7)]);      // No "last" bit
			}

			// set end of call 

           MsgStart = Digis * 10 + 1;                // UI Data follows digis in message
 		}
   
		TXMessageptr=&TXMessage[13+(Digis*7)];

		*(TXMessageptr++) |= 1;		// set last bit
        
		*(TXMessageptr++) = 3;     // UI

        if (sockptr->AGWRXHeader.PID == 0)

            *(TXMessageptr++) = 240;		 // PID
		else
            *(TXMessageptr++) = sockptr->AGWRXHeader.PID; 
   
        memcpy(TXMessageptr,&AGWMessage[MsgStart], sockptr->MsgDataLength - MsgStart);
        
		TXMessageptr += (sockptr->MsgDataLength - MsgStart);

        SendRaw(sockptr->AGWRXHeader.Port + 1, TXMessage, TXMessageptr-&TXMessage[0]);

		return 0;

	case 'X':
 
        //   Register Callsign
        
		memset(&AGWTXHeader,0,36);
		
		memset(&sockptr->CallSign,0,10);
	  
		memcpy(sockptr->CallSign, sockptr->AGWRXHeader.callfrom,strlen(sockptr->AGWRXHeader.callfrom));  
        AGWTXHeader.DataKind = 'X';
        
        AGWTXHeader.DataLength = 1;      // Length
             
        AGWRegReply[0] = 1;

        SendtoSocket(sockptr->socket, AGWRegReply);

		ShowApps();


		return 0;

   
	case 'Y':
    
        //   Session Status
        
        //   Create Session Key from port and callsign pair
        
        GetSessionKey(key, sockptr);

        for (con = 0; con < CurrentConnections; con++)
		{
			if (memcmp(Connections[con].CallKey,key,21) == 0)
			{
				memcpy(&AGWTXHeader,&sockptr->AGWRXHeader,36);

				AGWTXHeader.DataLength = 4;      // Length
				SendtoSocket(sockptr->socket, &AGWYReply[0]);

				return 0;
			}
		}

		return 0;


	default:
    
        //If Debugging Then Print #10, "Unknown Message "; Chr$(Sockets(Index).AGWRXHeader(4))
       // Debug.Print "Unknown Message "; Chr$(Sockets(Index).AGWRXHeader(4))
        
		return 0;

		}

	return 0;
   
}

int GetSessionKey(char * key, struct SocketConnectionInfo * sockptr)
{

//   Create Session Key from port and callsign pair
        
  

	key[0] = sockptr->AGWRXHeader.Port + '1'; 
        
	memset(&key[1],0,20);
	strcpy(&key[1],sockptr->AGWRXHeader.callfrom);
	strcpy(&key[11],sockptr->AGWRXHeader.callto);

	return 0;

}
int SendDataToAppl(int Stream, byte * Buffer, int Length)
{
	int con;
	char * i;
	char ConMsg[80];
	char DisMsg[80];
	char key[21];
	struct SocketConnectionInfo * sockptr;

//Dim i As Long, Length As Long, con As Long, key As String, hilen As Long, lolen As Long
//Dim Index As Integer, ConMsg As String, DisMsg As String
//Dim BytesSent As Long


	//'   Find Connection number and call pair

	for (con = 0; con < CurrentConnections; con++)
	{
		if (Connections[con].BPQStream == Stream)
		{
			memcpy(key,&Connections[con].CallKey,21);

			if (key[0] == 32)
			{
				//Debug.Print "Data on Unconnected Session"

				Disconnect(Stream);
				return (0);
			}
        
			sockptr = Connections[con].SocketIndex;
			
			if (sockptr == 0)
			{
				// No connection, but have a key - wot's going on!!
				// Probably best to clear out connection

				Disconnect(Stream);

				return (0);
			}

			AGWTXHeader.Port = key[0] - 49;

			memcpy(AGWTXHeader.callfrom,&key[11],10);
			memcpy(AGWTXHeader.callto,&key[1],10);

			if (Connections[con].Connecting)
			{

            //   See if *** Connected message

				i = strstr(Buffer, "Connected to");
            
				if (i != 0)
				{  
					Connections[con].Connecting = FALSE;
                
					DisplaySessions();

					AGWTXHeader.DataKind = 'C';
	                AGWTXHeader.PID = 0;
                
		            strcpy(ConMsg,"*** CONNECTED With Station ");
					strcat(ConMsg, AGWTXHeader.callfrom);
					strcat(ConMsg,"\0");
                
					AGWTXHeader.DataLength = strlen(ConMsg)+1;
           
					SendtoSocket(sockptr->socket, ConMsg);
            
					return (0);

				}
            
				i = strstr(Buffer, "Failure with");
            
				if (i != 0)
				{   
					Connections[con].Connecting = FALSE;
                               
					strcpy(DisMsg,"*** DISCONNECTED RETRYOUT With ");
        
					SendDisMsgtoAppl(DisMsg, sockptr);

					DeleteConnection(con);
                
		            return 0;

				}
				
				i = strstr(Buffer, "Busy from");
            
				if (i != 0)
				{   
					Connections[con].Connecting = FALSE;
                               
					strcpy(DisMsg,"*** DISCONNECTED RETRYOUT With ");
        
					SendDisMsgtoAppl(DisMsg, sockptr);
					
					DeleteConnection(con);
                
		            return 0;

				}
			}

	        AGWTXHeader.DataKind = 'D';
		    AGWTXHeader.PID = 0xF0;
			AGWTXHeader.DataLength = Length;              
        
			SendtoSocket(sockptr->socket, Buffer);
		}
	}

	return 0;
 }


int DataSocket_Disconnect( struct SocketConnectionInfo * sockptr)
{
	int con;

	closesocket(sockptr->socket);

	for (con = 0; con < CurrentConnections; con++)
	{
		if (Connections[con].SocketIndex == sockptr)
			Disconnect(Connections[con].BPQStream);
	}

	sockptr->SocketActive = FALSE;
	sockptr->RawFlag = FALSE;
	sockptr->MonFlag = FALSE;
	
	ShowApps();


	return 0;
}

int SendRawPacket(struct SocketConnectionInfo * sockptr, char *txmsg, int Length)
{
	SendtoSocket(sockptr->socket, txmsg);

	return 0;
}

extern int AGWMONDECODE();
extern int AGWMONOPTIONS();


int InternalAGWDecodeFrame(char * msg, char * buffer, int Stamp, int * FrameType)
{
	int	returnit;

	_asm {

	pushfd
	cld
	pushad


	mov	esi,msg
	mov	eax,Stamp
	mov	edi,buffer

	call	AGWMONDECODE

	mov	edi,FrameType
	mov	[edi],eax
	mov	returnit,ecx


	popad
	popfd

	}				// End of ASM

	return (returnit);
}

int ShowApps()
{
	struct SocketConnectionInfo * sockptr;
	int i;
	char Msg[80];
	char IPAddr[20];

	if (ConnWnd == 0) return 0; // Not on display
	
	SendDlgItemMessage(ConnWnd,IDC_CONNECTIONS_LIST,LB_RESETCONTENT,0,0);

	for (i = 1; i <= CurrentSockets; i++)
	{
		sockptr=&Sockets[i];

		if (sockptr->SocketActive)
		{
			wsprintf(IPAddr,"%d.%d.%d.%d",
				sockptr->sin.sin_addr.S_un.S_un_b.s_b1,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b2,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b3,
				sockptr->sin.sin_addr.S_un.S_un_b.s_b4);

			wsprintf(Msg,"%2d   %-16s %5d %-10s",i,IPAddr,htons(sockptr->sin.sin_port),&sockptr->CallSign);
		}
		else
		{
			wsprintf(Msg,"%2d   Idle",i);
		}

		SendDlgItemMessage(ConnWnd,IDC_CONNECTIONS_LIST,LB_ADDSTRING,0,(LPARAM) Msg);
	}

	return 0;
}

int Terminate()
{
	int con, State, Change;
	struct BPQConnectionInfo * Connection;

	SetAppl(controlStream, 0, 0);

	Disconnect(controlStream);

	DeallocateStream(controlStream);

// Ack to state change

	SessionState(controlStream, &State, &Change);

//
//   Release all streams
//
	for (con = 0; con < CurrentConnections; con++)
	{
		Connection=&Connections[con];

        SetAppl(Connection->BPQStream, 0, 0);

        Disconnect(Connection->BPQStream);

        DeallocateStream(Connection->BPQStream);
    
        SessionState(Connection->BPQStream, &State, &Change);
    
	}

	if (cfgMinToTray)
	{
		DeleteTrayMenuItem(MainWnd);
		DeleteTrayMenuItem(ConnWnd);
	}

	return 0;
}

