// TelnetServer.cpp : Defines the entry point for the application.
//


#include "stdafx.h"
#include "bpqkanthost.h"
#include "bpq32.h"
#include "GetVersion.h"


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text

char ClassName[]="TNC2MAINWINDOW";					// the main window class name

HWND MainWnd;

char szBuff[80];

BOOL UsingBPQSerial = FALSE;
BOOL FirstBPQPort = TRUE;
int CurrentConnections = 5;

HANDLE hControl;

struct ConnectionInfo Connections[6];

BOOL PartPacket = FALSE;
int	PartLen;
UCHAR *	PartPtr;


//struct ConnectionInfo ConnectionInfo[MaxSockets+1];



// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL Initialise();
int DoStateChange(Stream);
int DoReceivedData(Stream);
int DoMonitorData(Stream);
int Connected(Stream);
int Disconnected(Stream);
int SaveConfig(hWnd);
int Refresh();

HANDLE BPQOpenSerialControl(ULONG * lasterror);
int BPQSerialAddDevice(HANDLE hDevice,ULONG * port,ULONG * result);
int BPQSerialDeleteDevice(HANDLE hDevice,ULONG * port,ULONG * result);
HANDLE BPQOpenSerialPort( int port,DWORD * lasterror);
int BPQSerialSetCTS(HANDLE hDevice);
int BPQSerialSetDSR(HANDLE hDevice);
int BPQSerialSetDCD(HANDLE hDevice);
int BPQSerialClrCTS(HANDLE hDevice);
int BPQSerialClrDSR(HANDLE hDevice);
int BPQSerialClrDCD(HANDLE hDevice);
int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen);
int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen);
int BPQSerialGetQCounts(HANDLE hDevice,ULONG * RXCount, ULONG * TXCount);
int BPQSerialGetDeviceList(HANDLE hDevice,ULONG * Slot,ULONG * Port);
int BPQSerialIsCOMOpen(HANDLE hDevice,ULONG * Count);
int BPQSerialGetDTRRTS(HANDLE hDevice,ULONG * Flags);

BOOL InitTNC2Port(int Port,int * Stream);

BOOL OpenRealPort(conn);

VOID ProcessPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKHOSTPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID SendKISSData(struct ConnectionInfo * conn, UCHAR * txbuffer, int Len);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);

BOOL ReleaseTNC2Port(int Stream);

VOID CALLBACK TimerProc();
TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;
UINT_PTR TimerHandle = 0;


BOOL cfgMinToTray;

#define DllImport	__declspec( dllimport )


DllImport INT  BPQHOSTAPIPTR();

int BPQHOSTAPI;

DllImport INT  MONDECODEPTR();

int MONDECODE;



int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	int i;
	ULONG Errorval;
	struct ConnectionInfo * conn;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(NULL,TimerHandle);

	if (cfgMinToTray) DeleteTrayMenuItem(MainWnd);
	
	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->TypeFlag[0] == 'V')
		{
			BPQSerialClrCTS(conn->hDevice);
			BPQSerialClrDSR(conn->hDevice);
			BPQSerialClrDCD(conn->hDevice);
		}
		
		CloseHandle(conn->hDevice);
		ReleaseTNC2Port(conn->BPQPort);

		if (conn->Created) BPQSerialDeleteDevice(hControl, &conn->ComPort, &Errorval);

	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  connENTS:
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
    WNDCLASS  wc;
	HBRUSH bgBrush;

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( BPQICON ) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 
	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	return RegisterClass(&wc);


}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   connENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND hWnd;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

	int err;
	char Title[80];

	hInst = hInstance; // Store instance handle in our global variable

	hWnd=CreateDialog(hInst,ClassName,0,NULL);


   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    //  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
	   err=GetLastError();
      return FALSE;
   }

   MainWnd=hWnd;

	GetVersionInfo(NULL);

	wsprintf(Title,"BPQ Kantronics Hostmode Emulator Version %s", VersionString);

	SetWindowText(hWnd,Title);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   	BPQMsg = RegisterWindowMessage(BPQWinMsg);

   	return Initialise();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_connAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	int state,change;

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


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:


		switch (wmId)

	case TN_SAVE:

		SaveConfig(hWnd);

		break;

    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		return (0);


	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


BOOL Initialise()
{
	int i,  resp, OpenCount;
	ULONG Errorval;
	struct ConnectionInfo * conn;
	int retCode,Type,Vallen;
	char Key[20];

	HKEY hKey=0;
	
	_asm{

	mov eax,BPQHOSTAPIPTR
	mov eax,[eax]
	mov	BPQHOSTAPI,eax;

	mov eax,MONDECODEPTR
	mov eax,[eax]
	mov	MONDECODE,eax;

	}

	cfgMinToTray = GetMinimizetoTrayFlag();

	// Get config from Registry 


	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
		"SOFTWARE\\G8BPQ\\BPQ32\\BPQKHOST",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

//'   Get Params from Registry

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		wsprintf(Key,"Port%d",i);
		Vallen=4;
		retCode = RegQueryValueEx(hKey,Key,0,			
			(ULONG *)&Type,(UCHAR *)&conn->ComPort,(ULONG *)&Vallen);

		wsprintf(Key,"Port%dType",i);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,Key,0,			
			(ULONG *)&Type,(UCHAR *)&conn->TypeFlag,(ULONG *)&Vallen);

		strcpy(conn->Params,"9600,N,8,1");

		wsprintf(Key,"Port%dParam",i);
		Vallen=20;
		retCode = RegQueryValueEx(hKey,Key,0,			
			(ULONG *)&Type,(UCHAR *)&conn->Params,(ULONG *)&Vallen);
    
		if (conn->ComPort == 0) conn->TypeFlag[0]=0;    
		if (conn->TypeFlag[0] == ' ') conn->ComPort = 0;
	}

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		conn->KHOST = TRUE;

		if (conn->TypeFlag[0] == 'V') // And ComPort(i) <> "" Then
		{
			//' BPQ Virtual Port
        
			if (FirstBPQPort)
			{
				hControl = BPQOpenSerialControl(&Errorval);

				if (hControl == (HANDLE) -1)
				{
					MessageBox (MainWnd, "BPQ Virtual COM Driver Control Channel Open Failed","",0);
				}

				FirstBPQPort = FALSE;
			}

			conn->hDevice = BPQOpenSerialPort(conn->ComPort, &Errorval);
        
			if (conn->hDevice == (HANDLE) -1 && Errorval == 2)
			{
				//' Not found, so create
            
				resp = BPQSerialAddDevice(hControl, &conn->ComPort, &Errorval);
				//Debug.Print "Create "; ComPort(i), resp, Hex(Errorval)
				conn->Created = resp;
         		
				conn->hDevice = BPQOpenSerialPort(conn->ComPort, &Errorval);
			}			

			if (conn->hDevice != (HANDLE) -1)
			{
				wsprintf(conn->PortLabel,"Virtual COM%d", conn->ComPort);
 				OpenCount = -1;
            
				resp = BPQSerialIsCOMOpen(conn->hDevice, &OpenCount);
				conn->PortEnabled = OpenCount;
            
				resp = BPQSerialSetCTS(conn->hDevice);
				resp = BPQSerialSetDSR(conn->hDevice);
            
				conn->CTS = 1;
				conn->DSR = 1;
      
				conn->BPQPort = conn->HostPort;
				resp = InitTNC2Port(i, &conn->BPQPort);
				conn->HostPort = conn->BPQPort;
            
				UsingBPQSerial = TRUE;
			}
			else
				wsprintf(conn->PortLabel,"Open Failed", conn->ComPort);

		}

	}

	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);

	Refresh();

	if (cfgMinToTray)
	
		AddTrayMenuItem(MainWnd, "KHOST Emulator");

	return TRUE;
}

int Refresh()
{
	struct ConnectionInfo * conn;
	int i;
	char Buff[10];


	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		SendDlgItemMessage(MainWnd,TN_Check+i,BM_SETCHECK,(WPARAM)conn->PortEnabled,0);
		SendDlgItemMessage(MainWnd,TN_Label+i,WM_SETTEXT,0,(LPARAM)&conn->PortLabel);
		SendDlgItemMessage(MainWnd,TN_Param+i,WM_SETTEXT,0,(LPARAM)&conn->Params);
		SendDlgItemMessage(MainWnd,TN_COM+i,WM_SETTEXT,0,(LPARAM) _ltoa(conn->ComPort,Buff,10));
		SendDlgItemMessage(MainWnd,TN_TYPE+i,WM_SETTEXT,0,(LPARAM)&conn->TypeFlag);
		SendDlgItemMessage(MainWnd,TN_BPQ+i,WM_SETTEXT,0,(LPARAM) _ltoa(conn->BPQPort,Buff,10));
		SendDlgItemMessage(MainWnd,TN_CTS+i,BM_SETCHECK,(WPARAM)conn->CTS,0);
		SendDlgItemMessage(MainWnd,TN_RTS+i,BM_SETCHECK,(WPARAM)conn->RTS,0);
		SendDlgItemMessage(MainWnd,TN_DCD+i,BM_SETCHECK,(WPARAM)conn->DCD,0);
		SendDlgItemMessage(MainWnd,TN_DSR+i,BM_SETCHECK,(WPARAM)conn->DSR,0);
		SendDlgItemMessage(MainWnd,TN_DTR+i,BM_SETCHECK,(WPARAM)conn->DTR,0);
	}

	return 0;
}

VOID CALLBACK TimerProc()
{
	struct ConnectionInfo * conn;
	int i, ConCount, ModemStat;
	char rxbuffer[1000];
	int RXCount, TXCount, Read, resp;

	CheckTimer();

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->TypeFlag[0] == 'V')
		{
			ConCount = 0;
    
			BPQSerialIsCOMOpen(conn->hDevice, &ConCount);
    
			if (conn->PortEnabled == 1 && ConCount == 0)

				//' Connection has just closed - disconnect stream
        
				SessionControl(conn->BPQPort, 2, 0);

    
	        if (conn->PortEnabled != ConCount)
			{
				conn->PortEnabled = ConCount;
        		Refresh();
			}

			resp = BPQSerialGetQCounts(conn->hDevice, &RXCount, &TXCount);
                
			if (RXCount > 0)
			{
				resp = BPQSerialGetData(conn->hDevice, rxbuffer, 1000, &Read);

					ProcessPacket(conn, (UCHAR *)&rxbuffer, Read);
	
			}
       
			BPQSerialGetDTRRTS(conn->hDevice,&ModemStat);
			
			if ((ModemStat & 1) != conn->DTR)
			{
				conn->DTR=!conn->DTR;
				Refresh();
			}

			if ((ModemStat & 2) >> 1 != conn->RTS)
			{
				conn->RTS=!conn->RTS;
				Refresh();
			}

		}
	}

	return;
}




int SaveConfig(hWnd)
{
	int i;
	int retCode,disp;
	char Key[20];
	BOOL OK1, OK2, OK3;
	int Port;
	char TypeFlag[3];
	char Params[20];
	BOOL DUFF=FALSE;
	HKEY hKey=0;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQKHOST",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

	if (retCode != ERROR_SUCCESS) return 0;


	for (i = 1; i <= CurrentConnections; i++)
	{
		Port=GetDlgItemInt(MainWnd,TN_COM+i,&OK1,FALSE);
		OK2=GetDlgItemText(MainWnd,TN_TYPE+i,(LPSTR)TypeFlag,3);
		OK3=GetDlgItemText(MainWnd,TN_Param+i,(LPSTR)Params,20);

		if (!OK1 || Port < 0 || Port > 255)
		{
			SetDlgItemText(MainWnd,TN_COM+i,"?");
			DUFF=TRUE;
		}

		if (OK2 != 1 || (toupper(TypeFlag[0]) != 'R' && toupper(TypeFlag[0]) != 'V' && TypeFlag[0] != ' '))
		{
			SetDlgItemText(MainWnd,TN_TYPE+i,"?");
			DUFF=TRUE;
		}
		
		TypeFlag[0]=toupper(TypeFlag[0]);

		if (!DUFF)
		{
			wsprintf(Key,"Port%d",i);

			RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&Port,4);

			wsprintf(Key,"Port%dType",i);

			RegSetValueEx(hKey,Key,0,REG_SZ,(BYTE *)&TypeFlag,2);

			wsprintf(Key,"Port%dParam",i);

			RegSetValueEx(hKey,Key,0,REG_SZ,(BYTE *)&Params,strlen(Params)+1);
 
		}
	}

	MessageBox(MainWnd,"You must restart BPQKHOST for changes to be actioned","BPQKHOST",0);

	return 0;
}

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02



// Kantronics Host Mode Stuff

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD


VOID ProcessPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;
	
	//	Split into KISS Packets. By far the most likely is a single KISS frame
	//	or a single cr delimited frma, to treat as special case

	FendPtr = memchr(&rxbuffer[1], FEND, Len-1);

	if (rxbuffer[0] == FEND)
	{
		if (FendPtr == &rxbuffer[Len-1])
		{
			ProcessKHOSTPacket(conn, &rxbuffer[1], Len - 2);
			return;
		}

		if (FendPtr == NULL)
		{
			// We have a partial Packet - Save it

			PartPacket = TRUE;
			PartLen = Len;
			PartPtr = rxbuffer;
			return;
		}
		
		// Process the first Packet in the buffer

		NewLen =  FendPtr - rxbuffer -1;
		ProcessKHOSTPacket(conn, &rxbuffer[1], NewLen );
	
		// Loop Back

		ProcessPacket(conn, FendPtr+1, Len - NewLen -2);
		return;
	}

	// First Packet is not Host Mode. 

	if (FendPtr != NULL)
	{
		// Non Host Packet, followed by host packet

		NewLen =  FendPtr - rxbuffer;
		ProcessKPacket(conn, rxbuffer, NewLen);

		// Loop Back

		ProcessPacket(conn, FendPtr, Len - NewLen);
		return;
	}

	ProcessKPacket(conn, rxbuffer, Len);
	return;
}

VOID ProcessKPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len)
{
	UCHAR Command[80];
	UCHAR Reply[400]="cmd:\r";
	UCHAR CmdReply[]="C00";

	if (Len > 80) Len = 80;
		
	memcpy(Command, rxbuffer, Len);
	Command[Len] = 0x0d;
	Command[Len+1] = 0;

	OutputDebugString(Command);

//cmd:

//INTFACE HOST
//INTFACE was TERMINAL
//cmd:RESET
//ÀÀS00À
//ÀC20XFLOW OFFÀ


	SendKISSData(conn, CmdReply, 3);
	
	//BPQSerialSendData(conn->hDevice, Reply, 5);


	//	Process Non-Hostmode Packet

	return;
}

VOID ProcessKHOSTPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len)
{
	UCHAR Command[80];
	UCHAR Reply[400];
	UCHAR CmdReply[]="C00";
	UCHAR StatusReply1[]="C00FREE BYTES 936";
	UCHAR StatusReply2[]="C00A/V stream - DISCONNECTED";

	UCHAR Chan, Stream;

	switch (rxbuffer[0])
	{
	
	case 'C':

		// Command Packet. Extract Command

		if (Len > 80) Len = 80;
	
		Chan = rxbuffer[1];
		Stream = rxbuffer[2];

		memcpy(Command, &rxbuffer[3], Len-3);
		Command[Len-3] = 0;

		if (stricmp(Command, "S") == 0)
		{
			// Status

			SendKISSData(conn, StatusReply1, strlen(StatusReply1));
			SendKISSData(conn, StatusReply2, strlen(StatusReply2));
			return;
		}

		if (memcmp(Command, "C ", 2) == 0)
		{
			// Connect

			Reply[0] = 'C';
			Reply[1] = Chan;
			Reply[2] = Stream;
			SendKISSData(conn, Reply, 3);

			SessionControl(conn->BPQPort, 1, 0);

			Command[Len-3] = 0xd;
			SendMsg(conn->BPQPort, Command, Len-2);

			return;
		}

		if (stricmp(Command, "D") == 0)
		{
			// Disconnect

			Reply[0] = 'D';
			Reply[1] = Chan;
			Reply[2] = Stream;
//			SendKISSData(conn, Reply, 3);

			SessionControl(conn->BPQPort, 2, 0);

			return;
		}


		memcpy(Reply,CmdReply,3);
		SendKISSData(conn, Reply, 3);
		return;

	case 'D':

		// Data to send

		Chan = rxbuffer[1];
		Stream = rxbuffer[2];

		SendMsg(conn->BPQPort, &rxbuffer[3], Len-3);


		memcpy(Reply,CmdReply,3);
		SendKISSData(conn, Reply, 3);
		return;

	default:

		memcpy(Reply,CmdReply,3);
		SendKISSData(conn, Reply, 3);
		return;
	}
}

/*
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
	
 	return;
}

*/
VOID SendKISSData(struct ConnectionInfo * conn, UCHAR * txbuffer, int Len)
{
	// Send A Packet With KISS Encoding

	UCHAR EncodedReply[800];
	int TXLen;

	TXLen = KissEncode(txbuffer, EncodedReply, Len);

	BPQSerialSendData(conn->hDevice, EncodedReply, TXLen);

}

int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	outbuff[0]=FEND;
	txptr=1;

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
  


#define IOCTL_SERIAL_SET_BAUD_RATE      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 1,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_QUEUE_SIZE     CTL_CODE(FILE_DEVICE_SERIAL_PORT, 2,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_LINE_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT, 3,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_BREAK_ON       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 4,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_BREAK_OFF      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 5,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_IMMEDIATE_CHAR     CTL_CODE(FILE_DEVICE_SERIAL_PORT, 6,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 7,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 8,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 9,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT,10,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_RESET_DEVICE       CTL_CODE(FILE_DEVICE_SERIAL_PORT,11,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT,12,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT,13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XOFF           CTL_CODE(FILE_DEVICE_SERIAL_PORT,14,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XON            CTL_CODE(FILE_DEVICE_SERIAL_PORT,15,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT,16,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT,17,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_WAIT_ON_MASK       CTL_CODE(FILE_DEVICE_SERIAL_PORT,18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_PURGE              CTL_CODE(FILE_DEVICE_SERIAL_PORT,19,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_BAUD_RATE      CTL_CODE(FILE_DEVICE_SERIAL_PORT,20,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_LINE_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT,21,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_CHARS          CTL_CODE(FILE_DEVICE_SERIAL_PORT,22,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_CHARS          CTL_CODE(FILE_DEVICE_SERIAL_PORT,23,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_HANDFLOW       CTL_CODE(FILE_DEVICE_SERIAL_PORT,24,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_HANDFLOW       CTL_CODE(FILE_DEVICE_SERIAL_PORT,25,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_MODEMSTATUS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,26,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_COMMSTATUS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,27,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_XOFF_COUNTER       CTL_CODE(FILE_DEVICE_SERIAL_PORT,28,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_PROPERTIES     CTL_CODE(FILE_DEVICE_SERIAL_PORT,29,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_DTRRTS         CTL_CODE(FILE_DEVICE_SERIAL_PORT,30,METHOD_BUFFERED,FILE_ANY_ACCESS)



#define IOCTL_SERIAL_IS_COM_OPEN CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_CLR_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x808,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_BPQ_ADD_DEVICE     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x809,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQ_DELETE_DEVICE  CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80a,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQ_LIST_DEVICES   CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80b,METHOD_BUFFERED,FILE_ANY_ACCESS)


HANDLE BPQOpenSerialControl(ULONG * lasterror)
{            
	HANDLE hDevice;

	*lasterror=0;
	
	hDevice = CreateFile( "//./BPQControl", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (hDevice == (HANDLE) -1 )
	{
		*lasterror=GetLastError();
	}

	return hDevice;

}
int BPQSerialAddDevice(HANDLE hDevice,ULONG * port,ULONG * result)
{
	ULONG bytesReturned;

	return DeviceIoControl (hDevice,IOCTL_BPQ_ADD_DEVICE,port,4,result,4,&bytesReturned,NULL);
}

int BPQSerialDeleteDevice(HANDLE hDevice,ULONG * port,ULONG * result)
{
	ULONG bytesReturned;

	return DeviceIoControl (hDevice,IOCTL_BPQ_DELETE_DEVICE,port,4,result,4,&bytesReturned,NULL);
}


HANDLE BPQOpenSerialPort( int port,DWORD * lasterror)
{            
	char szPort[ 15 ];
	HANDLE hDevice;

  // load the COM prefix string and append port number
   
	*lasterror=0;
	
	wsprintf( szPort, "\\\\.\\BPQ%d",port) ;

	hDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (hDevice == (HANDLE) -1 )
	{
		*lasterror=GetLastError();
	}

   return hDevice;
}
 

int BPQSerialSetCTS(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(	hDevice,IOCTL_SERIAL_SET_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialClrCTS(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}
int BPQSerialClrDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialClrDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen)
{
	ULONG bytesReturned;

	if (MsgLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	return DeviceIoControl(hDevice,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen)
{
	if (BufLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	return DeviceIoControl(hDevice,IOCTL_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL);
                  
}

int BPQSerialGetQCounts(HANDLE hDevice,ULONG * RXCount, ULONG * TXCount)
{

	SERIAL_STATUS Resp;
	int MsgLen;
	int ret;

	ret = DeviceIoControl(hDevice,IOCTL_SERIAL_GET_COMMSTATUS,NULL,0,&Resp,sizeof(SERIAL_STATUS),&MsgLen,NULL);

    *RXCount=Resp.AmountInInQueue;
	*TXCount=Resp.AmountInOutQueue;

	return ret;

}

int BPQSerialGetDeviceList(HANDLE hDevice,ULONG * Slot,ULONG * Port)
{
	ULONG bytesReturned;

	return  DeviceIoControl (hDevice,IOCTL_BPQ_LIST_DEVICES,Slot,4,Port,4,&bytesReturned,NULL);

}




int BPQSerialIsCOMOpen(HANDLE hDevice,ULONG * Count)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
}
int BPQSerialGetDTRRTS(HANDLE hDevice,ULONG * Flags)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
}


BOOL InitTNC2Port(int Port,int * Stream)
{
	int ret;
	
	if (Port > 10 || Port == 0) return FALSE;
	
	if (*Stream == 0)
	{
		*Stream = FindFreeStream();

		if (*Stream == 255) return FALSE;

		BPQSetHandle(*Stream, hWnd);

	}
	else
	{
		ret = AllocateStream(*Stream);

		if (ret != 0) return FALSE;


	}


	return TRUE;

}

BOOL ReleaseTNC2Port(int Stream)
{	

	if (Stream == 0) return FALSE;
	
	SessionControl(Stream, 2, 0);
	DeallocateStream(Stream);

	return TRUE;

}


int Connected(Stream)
{
	byte ConnectingCall[10];
	byte * ApplCallPtr;
	byte * keyptr;
	byte ApplCall[10]="";
	byte ErrorMsg[100];
	UCHAR Msg[50];
	int i, Len;
	struct ConnectionInfo * conn;


	int ApplNum,con;
	struct SocketConnectionInfo * sockptr;

	GetCallsign(Stream, ConnectingCall);

	for (i=9;i>0;i--)
		if (ConnectingCall[i]==32)
			ConnectingCall[i]=0;


	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->BPQPort == Stream)
		{
			Len = wsprintf (Msg, "S1A*** CONNECTED to %s", ConnectingCall);
//			SendKISSData(conn, Msg, Len);

			return 0;
		}
	}

	return 0;
}

int Disconnected (Stream)
{
	int con;
	UCHAR Msg[50];
	int i, Len;
	struct ConnectionInfo * conn;


	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->BPQPort == Stream)
		{
			Len = wsprintf (Msg, "S1A*** DISCONNECTED");
			SendKISSData(conn, Msg, Len);

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
	int Len;
	struct ConnectionInfo * conn;


	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->BPQPort == Stream)
		{
			
			do { 

				GetMsg(Stream, &Buffer[3], &len, &count);

				if (len > 0)
				{
					Buffer[0] = 'D';
					Buffer[1] = '1';
					Buffer[2] = 'A';

					SendKISSData(conn, Buffer, len+3);
				}
	  
			}while (count > 0);

			return 0;
		}
	}

	
	return 0;
}

int DoMonitorData(int Stream)
{
	byte Buffer[500];
	int RawLen,Length,Count;
	byte Port;
	struct SocketConnectionInfo * sockptr;	
	byte AGWBuffer[500];
	int n;
	int Stamp, Frametype;
	BOOL RXFlag, NeedAGW;

	do
	{
		Stamp=GetRaw(Stream, Buffer, &RawLen, &Count );
 
	}

	while (Count > 0);

	return 0;

}








