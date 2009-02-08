
// TNC2 Emulator for BPQ32 Switch

// Version 1.1.0 January 2009

//		Support Win98 VirtualCOM
//		Add StartMinimized Option

#include "stdafx.h"
#include "bpqtnc2.h"
#define DYNLOADBPQ

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

BOOL Win98 = FALSE;		// Running on Win98

struct ConnectionInfo Connections[6];

//struct ConnectionInfo ConnectionInfo[MaxSockets+1];



// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
//INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL Initialise();
int DoStateChange(int Stream);
int DoReceivedData(int Stream);
int DoMonitorData(int Stream);
int Connected(int Stream);
int Disconnected(int Stream);
int SaveConfig(HWND hWnd);
int Refresh();
VOID RealPortThread(int i);

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
BOOL TNC2Timer();
BOOL TNC2GetChar(int port,int * returnedchar, int * moretocome);
BOOL TNC2PutChar(int port,int sentchar);
BOOL TNC2GetVMSR(int port,int * returnedchar);

BOOL OpenRealPort(struct ConnectionInfo * conn);

int	INITTNC2();
BOOL APIGETCHAR();
BOOL APIKEYSUPPORT();
BOOL APIGETVMSR();
BOOL TNCTIMERPROC();
BOOL ReleaseTNC2Port(int Stream);

VOID CALLBACK TimerProc();
TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;
unsigned int TimerHandle = 0;


BOOL cfgMinToTray;

#define DllImport	__declspec( dllimport )

unsigned long _beginthread( void( *start_address )( int ), unsigned stack_size, int arglist);


int BPQHOSTAPI;


UINT MONDECODE;

int Semaphore=0;
int SEMCLASHES=0;

void GetSemaphore()
{
	//
	//	Wait for it to be free
	//
	
	if (Semaphore !=0)
		SEMCLASHES++;

loop1:

	while (Semaphore != 0)
		Sleep(10);

	//
	//	try to get semaphore
	//

	_asm{

	mov	eax,1
	xchg Semaphore,eax	// this instruction is locked
	
	cmp	eax,0
	jne loop1			// someone else got it - try again
;
;	ok, we've got the semaphore
;
	}

	return;
}
void FreeSemaphore()
{
	Semaphore=0;

	return; 
}



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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
	int err;
	char Title[80];

	hInst = hInstance; // Store instance handle in our global variable

	 GetAPI();


	BPQHOSTAPI= GETBPQAPI();
	MONDECODE = GETMONDECODE();


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

	wsprintf(Title,"BPQ TNC2 Emulator Version %s", VersionString);

	SetWindowText(hWnd,Title);

	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

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
	ULONG Errorval, CtlErrorval;
	struct ConnectionInfo * conn;
	int retCode,Type,Vallen;
	char Key[20];

	HKEY hKey=0;

	OutputDebugString("BPQTNC2 Init\n");

#pragma warning(push)
#pragma warning(disable : 4996)

	if (HIBYTE(_winver) < 5)
		Win98 = TRUE;

#pragma warning(pop)

	// Get config from Registry 


	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
		"SOFTWARE\\G8BPQ\\BPQ32\\BPQTNC2",
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

	OutputDebugString("BPQTNC2 Got Config\n");


	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->TypeFlag[0] == 'V') // And ComPort(i) <> "" Then
		{
			//' BPQ Virtual Port
        
			conn->hDevice = BPQOpenSerialPort(conn->ComPort, &Errorval);


			if (FirstBPQPort)
			{
				hControl = BPQOpenSerialControl(&CtlErrorval);

				if (hControl == (HANDLE) -1)
				{
					MessageBox (MainWnd, "BPQ Virtual COM Driver Control Channel Open Failed","",0);
				}

				FirstBPQPort = FALSE;
			}

        
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

		if (conn->TypeFlag[0] == 'R')
		{        
        //' real Serial Port

			wsprintf(conn->PortLabel,"COM%d", conn->ComPort);

			if (OpenRealPort(conn))
			{
				wsprintf(conn->PortLabel,"COM%d", conn->ComPort);
				conn->PortEnabled = 1;   
				conn->BPQPort = conn->HostPort;
				resp = InitTNC2Port(i, &conn->BPQPort);
				conn->RTS = 1;
//				conn->DTR = 1;
//				EscapeCommFunction(conn->hDevice,SETDTR);
				EscapeCommFunction(conn->hDevice,SETRTS);

 				conn->HostPort = conn->BPQPort;

				_beginthread(RealPortThread,0,i);
			}
			else
				wsprintf(conn->PortLabel,"Open Failed", conn->ComPort);

		}
	}

	OutputDebugString("BPQTNC2 Starting Timer\n");

	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);

	Refresh();

	if (cfgMinToTray)
		AddTrayMenuItem(MainWnd, "TNC2 Emulator");

	OutputDebugString("BPQTNC2 Init Complete\n");

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
	int i, n, ConCount, ModemStat;
	char rxbuffer[1000];
	int retval, more;
	char TXMsg[1000];
	int RXCount, TXCount, Read, resp;
	char msg[100];


	CheckTimer();

	GetSemaphore();

	TNC2Timer();

	FreeSemaphore();


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

	        if (!conn->PortEnabled)
				break;

			resp = BPQSerialGetQCounts(conn->hDevice, &RXCount, &TXCount);


                
			if (RXCount > 0)
			{
				resp = BPQSerialGetData(conn->hDevice, rxbuffer, 80, &Read);

				wsprintf(msg, "BPQTNC2 %d %d\n", RXCount, TXCount);
				OutputDebugString(msg);
		
				GetSemaphore();
			
				for (n = 0; n < Read; n++)
					TNC2PutChar(i, rxbuffer[n]);

				FreeSemaphore();
			}
      
			if (TXCount > 4096) goto getstatus;
       
			n=0;

			GetSemaphore();

		getloop:

			TNC2GetChar(i, &retval, &more);

			if (retval != -1)
				TXMsg[n++] = retval;

			if (more > 0 && n < 1000) goto getloop;
    
			FreeSemaphore();
        
			if (n > 0 && conn->PortEnabled) 
				BPQSerialSendData(conn->hDevice, TXMsg, n);

		getstatus:


        
			TNC2GetVMSR(i, &retval);
        
			if ((retval & 8) == 8)	 //' DCD (Connected) Changed
			{	
				conn->DCD = (retval & 128) / 128;
				
				if (conn->DCD == 1)
					BPQSerialSetDCD(conn->hDevice);
				else
					BPQSerialClrDCD(conn->hDevice);

				Refresh();
			}

			if ((retval & 1) == 1)  //' CTS (Flow Control) Changed
			{			
				conn->CTS = (retval & 16) / 16;
        
				if (conn->CTS == 1)
					BPQSerialSetCTS(conn->hDevice);
				else
					BPQSerialClrCTS(conn->hDevice);

				Refresh();
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



VOID RealPortThread(int i)
{
	struct ConnectionInfo * conn;
	int n;
	char rxbuffer[1000];
	int retval, more, BytesWritten, ModemStat;
	char TXMsg[500];
	int RXCount, Read, resp;

	COMSTAT    ComStat;
	DWORD      dwErrorFlags;

	conn = &Connections[i];

	// only try to read number of bytes in queue 
	do {

		ClearCommError(conn->hDevice, &dwErrorFlags, &ComStat ) ;
        
		RXCount = min(1000, ComStat.cbInQue);
        
		if (RXCount > 0)
		{
			resp = ReadFile(conn->hDevice, rxbuffer, RXCount, &Read, NULL);

			if (!resp)
			{
				Read = 0;
				ClearCommError(conn->hDevice, &dwErrorFlags, &ComStat ) ;
			}
				
			GetSemaphore();

			for (n = 0; n < Read; n++)
				TNC2PutChar(i, rxbuffer[n]);

			FreeSemaphore();
		}

		if (!conn->CTS)
			goto getstatusR;
       
		n=0;

		GetSemaphore();

getloopR:

		TNC2GetChar(i, &retval, &more);

		if (retval != -1)
			TXMsg[n++] = retval;

		if (more > 0 && n < 500) goto getloopR;
    
		FreeSemaphore();

		if (n > 0 && conn->PortEnabled) 
			resp = WriteFile(conn->hDevice, TXMsg, n, &BytesWritten, NULL);

getstatusR:
        
		TNC2GetVMSR(i, &retval);
        
		if ((retval & 8) == 8)	 //' DCD (Connected) Changed
		{
			conn->DTR = (retval & 128) / 128;
        
			if (conn->DTR == 1)
				EscapeCommFunction(conn->hDevice,SETDTR);
			else
				EscapeCommFunction(conn->hDevice,CLRDTR);

			Refresh();
		}

		if ((retval & 1) == 1)  //' CTS (Flow Control) Changed
		{			
			conn->RTS = (retval & 16) / 16;
        
			if (conn->RTS == 1)
				EscapeCommFunction(conn->hDevice,SETRTS);
			else
				EscapeCommFunction(conn->hDevice,CLRRTS);

			Refresh();
		}
		
		GetCommModemStatus(conn->hDevice,&ModemStat);

		if ((ModemStat & MS_CTS_ON) >> 4 != conn->CTS)
		{
			conn->CTS=!conn->CTS;
			Refresh();
		}

		if ((ModemStat & MS_DSR_ON) >> 5 != conn->DSR)
		{
			conn->DSR=!conn->DSR;
			Refresh();
		}

		if ((ModemStat & MS_RLSD_ON) >> 7 != conn->DCD)
		{
			conn->DCD=!conn->DCD;
			Refresh();
		}

	Sleep(10);

	} while (TRUE);
}










int SaveConfig(HWND hWnd)
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
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQTNC2",
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

	MessageBox(MainWnd,"You must restart BPQTNC2 for changes to be actioned","BPQTNC2",0);

	return 0;
}

BOOL OpenRealPort(struct ConnectionInfo * conn)
{
   char szPort[ 15 ];
   BOOL fRetVal ;
   COMMTIMEOUTS CommTimeOuts ;
   int i;

   DCB	dcb;

  // load the COM prefix string and append port number
   
  wsprintf( szPort, "//./COM%d", conn->ComPort) ;

   // open COMM device

   conn->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (conn->hDevice == (HANDLE) -1 )
	{
		i=GetLastError();
		return ( FALSE ) ;
	}

      // setup device buffers

      SetupComm(conn->hDevice, 4096, 4096 ) ;

      // purge any information in the buffer

      PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

      // set up for overlapped I/O
	  
      CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
      CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
      SetCommTimeouts(conn->hDevice, &CommTimeOuts ) ;

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02


   
	  dcb.DCBlength = sizeof( DCB );

	  GetCommState(conn->hDevice, &dcb );

	  BuildCommDCB(conn->Params, &dcb);	

	  // setup hardware flow control

      dcb.fDtrControl = DTR_CONTROL_ENABLE;
    
//	  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE ;

      dcb.fRtsControl = RTS_CONTROL_ENABLE;


	  dcb.fInX = dcb.fOutX = 0;
	  dcb.XonChar = 0 ;
	  dcb.XoffChar = 0 ;
	  dcb.XonLim = 0 ;
	  dcb.XoffLim = 0 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = TRUE ;

   fRetVal = SetCommState(conn->hDevice, &dcb ) ;

   return ( fRetVal ) ;

} // end of SetupConnection()



// BPQ Serial Device Support

// On W2K and above, BPQVIrtualCOM.sys provides a pair of cross-connected devices, and a control channel
//	to enumerate, add and delete devices.

// On Win98 BPQVCOMM.VXD provides a single IOCTL interface, over which calls for each COM device are multiplexed




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

#define	IOCTL_BPQ_SET_POLLDELAY	 CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80c,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define	IOCTL_BPQ_SET_DEBUGMASK	 CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80d,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define W98_SERIAL_IS_COM_OPEN 0x800
#define W98_SERIAL_GETDATA     0x801
#define W98_SERIAL_SETDATA     0x802

#define W98_SERIAL_SET_CTS     0x803
#define W98_SERIAL_SET_DSR     0x804
#define W98_SERIAL_SET_DCD     0x805

#define W98_SERIAL_CLR_CTS     0x806
#define W98_SERIAL_CLR_DSR     0x807
#define W98_SERIAL_CLR_DCD     0x808

#define W98_BPQ_ADD_DEVICE     0x809
#define W98_BPQ_DELETE_DEVICE  0x80a
#define W98_BPQ_LIST_DEVICES   0x80b

#define	W98_BPQ_SET_POLLDELAY	 0x80c
#define	W98_BPQ_SET_DEBUGMASK	 0x80d

#define W98_SERIAL_GET_COMMSTATUS    27
#define W98_SERIAL_GET_DTRRTS        30

#define DebugModemStatus 1
#define DebugCOMStatus 2
#define DebugWaitCompletion 4
#define DebugReadCompletion 8


HANDLE BPQOpenSerialControl(ULONG * lasterror)
{            
	HANDLE hDevice;

	*lasterror=0;

	if (Win98)
	{
		hDevice = CreateFile( "\\\\.\\BPQVCOMM.VXD", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
	else
	{
		hDevice = CreateFile( "//./BPQControl", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
				  
	if (hDevice == (HANDLE) -1 )
	{
		*lasterror=GetLastError();
	}

	return hDevice;

}
int BPQSerialAddDevice(HANDLE hDevice, ULONG * port, ULONG * result)
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
	char szPort[15];
	HANDLE hDevice;

	*lasterror=0;

	if (Win98)
	{
		wsprintf( szPort, "\\\\.\\COM%d",port) ;

		hDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
		if (hDevice == (HANDLE) -1 )
		{
			// If In Use(5) ok, else fail

			if (GetLastError() == 5)
				return (HANDLE)(port<<16);			// Port Number is a pseudohandle to the device

			return (HANDLE) -1;
		}

		CloseHandle(hDevice);

		return (HANDLE)(port<<16);			// Port Number is a pseudohandle to the device
	}


  // load the COM prefix string and append port number
   
	
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

	if (Win98)
		return DeviceIoControl(hControl,(UINT)(UINT)hDevice | W98_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);

}

int BPQSerialSetDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_SET_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DSR, NULL,0,NULL,0, &bytesReturned,NULL);

                  
}

int BPQSerialSetDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);

                  
}

int BPQSerialClrCTS(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);

                  
}
int BPQSerialClrDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);

                  
}

int BPQSerialClrDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_CLR_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DCD, NULL,0,NULL,0, &bytesReturned,NULL);

                     
}

int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen)
{
	ULONG bytesReturned;

	if (MsgLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen)
{
	if (BufLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL);
                  
}

int BPQSerialGetQCounts(HANDLE hDevice,ULONG * RXCount, ULONG * TXCount)
{

	SERIAL_STATUS Resp;
	int MsgLen;
	int ret;

	if (Win98)
		ret = DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_GET_COMMSTATUS,NULL,0,&Resp,sizeof(SERIAL_STATUS),&MsgLen,NULL);
	else
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

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
}

int BPQSerialGetDTRRTS(HANDLE hDevice,ULONG * Flags)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
}

int BPQSerialSetPollDelay(HANDLE hDevice, int PollDelay)
{
	ULONG bytesReturned;
	
	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_BPQ_SET_POLLDELAY,&PollDelay,4,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_BPQ_SET_POLLDELAY,&PollDelay,4,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDebugMask(HANDLE hDevice, int DebugMask)
{
	ULONG bytesReturned;
	
	return DeviceIoControl(hDevice, IOCTL_BPQ_SET_DEBUGMASK, &DebugMask, 4, NULL, 0, &bytesReturned,NULL);
                  
}

BOOL InitTNC2Port(int Port,int * Stream)
{
	int ret;
	
	if (Port > 10 || Port == 0) return FALSE;
	
	if (*Stream == 0)
	{
		*Stream = FindFreeStream();

		if (*Stream == 255) return FALSE;
	}
	else
	{
		ret = AllocateStream(*Stream);

		if (ret != 0) return FALSE;
	}


	_asm {

		mov ecx,Port
		dec	ecx
		mov eax,Stream
		mov	eax,[eax]

		call INITTNC2
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


BOOL TNC2GetChar(int port,int * returnedchar, int * moretocome)
{            
	_asm {

		mov	ecx,port
		dec	ecx
		call APIGETCHAR

		mov	edi,returnedchar
		mov	[edi],eax
	
		mov	edi,moretocome
		mov	[edi],ecx
	}

	return TRUE;

}

BOOL TNC2PutChar(int port,int sentchar)
{            
	_asm {

		mov	eax,sentchar
		mov	ecx,port
		dec	ecx
		call APIKEYSUPPORT

	}

	return TRUE;

}

BOOL TNC2GetVMSR(int port,int * returnedchar)
{            
	_asm {

		mov	ecx,port
		dec	ecx
		call APIGETVMSR

		mov	edi,returnedchar
		mov	[edi],eax
	
	}

	return TRUE;

}


BOOL TNC2Timer()
{            
	_asm {

		call TNCTIMERPROC

	}
	return TRUE;

}

