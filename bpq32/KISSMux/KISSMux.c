// GPSMuxPC.cpp : Defines the entry point for the application.
//

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "stdafx.h"
#include "KISSMux.h"


#define MAX_LOADSTRING 100
#define IDI_GPSMUXPC                    107
#define IDI_SMALL                       108
#define WSA_READ WM_USER + 1
#define WSA_ACCEPT WM_USER + 2
#define WSA_DATA WM_USER + 3
#define WSA_CONNECT WM_USER + 4


#define IDT_TIMER1 1
#define IDT_TIMER2 2

#define BGCOLOUR RGB(236,233,216)
HBRUSH RedBrush;
HBRUSH GreenBrush;
HBRUSH BlueBrush;
HBRUSH bgBrush;

HWND hWnd;
int TimerHandle, simTimerHandle;

SOCKET udpsock;
SOCKADDR_IN sinx; 
SOCKADDR_IN rxaddr;
SOCKADDR_IN txaddr;

int addrlen=sizeof(sinx);

int ListenSock=8873;

HANDLE hControl;

#define OUTPORTS 6
struct PortInfo Ports[OUTPORTS + 1] = {0};			// Extra for UDP Out
struct PortInfo InPorts[2] = {0};

UINT GPSType = 0xffff;		// Source of Postion info - 1 = Phillips 2 = AIT1000. ffff = not posn message

BOOL RMCMsg;

struct SatInfo SATS[40];

int SatsInView = 0;
int SatsInFix = 0;
int Recnum = 0;
int RecCount = 0;
int FixType = 0;
int FixD = 0;			// 2/3D

int GSVTimeout;
int FixTimeout;

RECT SatRect1 = {5, 180, 300, 200};
RECT SatRect2 = {5, 200, 300, 300};

char SimFileName[256];
int SimRate=100;
BOOL FileOpen;
HANDLE hInputFile;
HANDLE hSpeedLog, HBattLog;
int LastTickCount; 

int RecoveryTimer;			// Serial Port recovery

double PI = 3.1415926535;
double P2 = 3.1415926535 / 180;

double Latitude, Longtitude, SOG, COG, LatIncrement, LongIncrement;
double LastSOG = -1.0;

BOOL SendUDP = TRUE;
BOOL FilterVDO = FALSE;


// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[]="GPSMuxPC";						// The title bar text
TCHAR szWindowClass[]="KISSMUXMAINWINDOW";			// the main window class name

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

struct tcp_table_entry tcp_table;

int WindSpeedAcc = 0;
int WindSpeedCount = 0;
double WaterSpeed = 0;

int CksumErrs;
int Msgs;


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int GetConfig();
void SaveConfig();
int InitWS();
int InitPorts();
void ClosePorts();

HANDLE BPQOpenSerialControl(ULONG * lasterror);
HANDLE BPQOpenSerialPort( int port,DWORD * lasterror);
int BPQSerialAddDevice(HANDLE hDevice,ULONG * port,ULONG * result);
int BPQSerialDeleteDevice(HANDLE hDevice,ULONG * port,ULONG * result);
int BPQSerialIsCOMOpen(HANDLE hDevice,ULONG * Count);
int BPQSerialSetCTS(HANDLE hDevice);
int BPQSerialSetDSR(HANDLE hDevice);
int BPQSerialSetDCD(HANDLE hDevice);
int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen);
int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen);

void VirSerTimer();
void SimTimerProc();
void PollKISSIn();
VOID RealPortThread(struct PortInfo * portptr);
void DistributeMessage(char * GPSMsg, int len, BOOL TOGPS);
void SelectSource(int Index, BOOL Recovering);
BOOL OpenRealPort(struct PortInfo * portptr);
int Refresh(struct PortInfo * portptr);
OpenListeningSocket(struct tcp_table_entry * arp);
int Socket_Accept(int SocketId);
int Socket_Data(int sock, int error, int eventcode);
int DataSocket_Read(struct tcp_table_entry * sockptr, SOCKET sock);
void SendToGPS(char * GPSMsg, int len);
void CheckRX(struct PortInfo * Port, UCHAR * RXBuffer, int Length);

void SendAPRSFromRMC(char * msg, int len);


uintptr_t _beginthread( 
   void( *start_address )( void * ),
   unsigned stack_size,
   void *arglist 
);


VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	OutputDebugString(Mess);

	return;
}

#define NUMBEROFBUFFERS 500

UINT FREE_Q = 0;
UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers

int QCount = 0;

UINT * Q_REM(UINT *Q);
VOID Q_ADD(UINT *Q,UINT *BUFF);
UINT * GetBuffer();
VOID ReleaseBuffer(UINT *BUFF);

// C Routines to access buffer pool



UINT * GetBuffer()
{
	// Get Buffer from Free Queue
	
	UINT * Buff = Q_REM(&FREE_Q);

	if (Buff)
		QCount--;

	return Buff;
}
// Get buffer from Queue

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

VOID ReleaseBuffer(UINT *BUFF)
{
	UINT * pointer;
	
	(UINT)pointer=FREE_Q;

	*BUFF=(UINT)pointer;

	FREE_Q=(UINT)BUFF;

	QCount++;

	return;
}

VOID Q_ADD(UINT *Q,UINT *BUFF)
{
	UINT * next;
	
	BUFF[0]=0;							// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=(UINT)BUFF;				// New one on front
		return;
	}

	(int)next=Q[0];

	while (next[0]!=0)
		next=(UINT *)next[0];			// Chain to end of queue

	next[0]=(UINT)BUFF;					// New one on end

	return;
}

char * strsplit(char ** context, char delim)
{
	// Return chars before delim, update context past string

	char * ptr = strchr(*context, delim);
	char  * string = *context;

	if (*context[0] == 0)
		return NULL;

	if (ptr == NULL)
	{
		*context = strchr(string, 0);
		return string;
	}

	*(ptr)++=0;
	*context = ptr;

	return string;
}




int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.

	// Initialize global strings

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	SelectSource(IDC_RADIO1, FALSE);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	ClosePorts();
	
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
    WNDCLASS  wc;

	bgBrush = CreateSolidBrush(BGCOLOUR);
	RedBrush = CreateSolidBrush(RGB(255,0,0));
	GreenBrush = CreateSolidBrush(RGB(0,255,0));
	BlueBrush = CreateSolidBrush(RGB(0,0,255));

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS; 
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_GPSMUXPC));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;//"MENU_1";	
	wc.lpszClassName = szWindowClass; 

//	RegisterClass(&wc);

 //   wc.lpfnWndProc = TraceWndProc;       
 // 	wc.lpszClassName = TraceClassName;

//	RegisterClass(&wc);

//    wc.lpfnWndProc = ConfigWndProc;       
//  	wc.lpszClassName = ConfigClassName;

	return (RegisterClass(&wc));

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
	int i, ret;
	int Day = _time32(NULL)/86400;

   hInst = hInstance; // Store instance handle in our global variable

   	// Build buffer pool
	
	for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
	{	
		ReleaseBuffer(&BufferPool[100*i]);
	}


   hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

  if (!hWnd)
   {
      ret=GetLastError();
	  return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   memset(&Ports[0], 0, (OUTPORTS + 1) * sizeof(struct PortInfo));
   memset(&InPorts[0], 0, 2 * sizeof(struct PortInfo));

   InPorts[1].Index = 1;

   GetConfig();

//   InitWS();
  
   InitPorts();

   TimerHandle=SetTimer(hWnd,IDT_TIMER1,100,NULL);
 		
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
	int wmId, wmEvent, len;
	char UDPMsg[256];
	char Work[50];
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_TIMER:

		switch (wParam) 
		{ 		
			case IDT_TIMER1: 

				VirSerTimer();
				PollKISSIn();

				return 0; 
 		}

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(wParam);
		return 0;

	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_READ: // Notification on data socket

		len = recvfrom(udpsock,UDPMsg,256,0,(LPSOCKADDR)&rxaddr,&addrlen);

		if (len < 1)
			return 0;

		if (UDPMsg[len-1] == 0)
			len--;

//		if (UDPMsg[len-2] == 0xd && UDPMsg[len-1] == 0x0a)
//			len-=2;

		UDPMsg[len]=0;

		SetDlgItemText(hWnd,IDC_Trace,UDPMsg);
		DistributeMessage(UDPMsg, len, TRUE);			// Send to GPS Port


		return 0;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		if (wmId > 5005 && wmId < 5012)
		{
			// Loop Flag

			Ports[wmId - 5006].LoopAppls = IsDlgButtonChecked(hWnd, wmId);

			return 0;
		}

		if (wmId > 5011 && wmId < 5018)
		{
			// Loop Flag

			Ports[wmId - 5012].CheckSum = IsDlgButtonChecked(hWnd, wmId);

			return 0;
		}


		switch (wmId)
		{
		case IDC_RADIO1:
		case IDC_WINSOCK:
		case IDC_FILE:
		case IDC_SIMULATE:
			SelectSource(wmId, FALSE);
			break;

		case IDC_START:
		case IDC_STOP:

			break;

		case IDC_RESTART:

		GetDlgItemText(hWnd,IDC_LAT,Work,50);	
		Latitude=atof(Work);

		GetDlgItemText(hWnd,IDC_LONG,Work,50);
		Longtitude=atof(Work);
		
		break;

		


		case IDC_SAVE:
			 SaveConfig();
			 break;

		case IDC_UDP:

			SendUDP = IsDlgButtonChecked(hWnd, IDC_UDP);
			break;

		case IDC_TCP:

			if (IsDlgButtonChecked(hWnd, IDC_TCP))
			{
				OpenListeningSocket(&tcp_table);
			}

			else
			{
				closesocket(tcp_table.TCPListenSock);
				closesocket(tcp_table.TCPSock);
			}
			break;

//		case IDM_EXIT:
//			DestroyWindow(hWnd);
//			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_PAINT:

		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
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

BOOL InitWS(void)
{
    int Error;              // catches return value of WSAStartup
    WORD VersionRequested;   // passed to WSAStartup
    WSADATA WsaData;            // receives data from WSAStartup
    BOOL ReturnValue = TRUE; // return value
	int err;
	TCHAR Msg[100];

    // Start WinSock.  If it fails, we don't need to call
    // WSACleanup().

    VersionRequested = MAKEWORD(1, 0);
    Error = WSAStartup(VersionRequested, &WsaData);

    if (Error)
	{
       MessageBox(NULL,
            TEXT("Could not initialise WinSock"),
            TEXT("GPSMux"), MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        return FALSE;
	}

	udpsock=socket(AF_INET,SOCK_DGRAM,0);

	if (udpsock == INVALID_SOCKET)
	{
		MessageBox(NULL, TEXT("Failed to create UDP socket"),NULL,MB_OK);
		err = WSAGetLastError();
  	 	return FALSE; 
	}

	sinx.sin_family = AF_INET;
	sinx.sin_port = htons(ListenSock);

	if (bind(udpsock, (LPSOCKADDR) &sinx, sizeof(sinx)) != 0 )
	{
		//
		//	Bind Failed
		//
		err = WSAGetLastError();
		wsprintf(Msg, TEXT("Bind Failed for UDP socket - error code = %d"), err);
		MessageBox(NULL, Msg ,NULL, MB_OK);
		return FALSE;

	}

	if (WSAAsyncSelect( udpsock, hWnd, WSA_READ, FD_READ) > 0)
	{
		wsprintf(Msg, TEXT("WSAAsyncSelect failed Error %d"), WSAGetLastError());

		MessageBox(hWnd, Msg, TEXT("GPSMux"), MB_OK);

		closesocket( udpsock );
		
		return FALSE;

	}

	return TRUE;

}
int GetConfig()
{
	struct PortInfo * portptr;
	int retCode,Type,Vallen, i;
	HKEY hKey=0;
	char Work[50]="";
	char Key[20]="";

	// Get config from Registry 

	retCode = RegOpenKeyEx (HKEY_CURRENT_USER,
                              "SOFTWARE\\G8BPQ\\KISSMux",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		portptr = &InPorts[0];
		Vallen=50;
		retCode = RegQueryValueEx(hKey,"KISSPort1",0,			
			(ULONG *)&Type,(UCHAR *)&Work,(ULONG *)&Vallen);
		
		SetDlgItemText(hWnd,IDC_COM1,Work);
		portptr->ComPort=atoi(Work);

		Vallen=50;
		retCode = RegQueryValueEx(hKey,"PortSpeed1",0,			
			(ULONG *)&Type,(UCHAR *)&Work,(ULONG *)&Vallen);
		
		SetDlgItemText(hWnd,IDC_SPEED1,Work);
		strcpy(portptr->Params,Work);

		Vallen=50;
		retCode = RegQueryValueEx(hKey, "Port1Enable", 0,	&Type, Work, &Vallen);

		CheckDlgButton(hWnd,IDC_EN1,Work[0]);
		portptr->PortEnabled=Work[0];


		Vallen=50;
		retCode = RegQueryValueEx(hKey, "Port1Polled", 0,	&Type, Work, &Vallen);

		CheckDlgButton(hWnd,IDC_POLLED1, Work[0]);
		portptr->Polled = Work[0];

		Vallen=50;
		retCode = RegQueryValueEx(hKey, "Port1AckMode", 0,	&Type, Work, &Vallen);

		CheckDlgButton(hWnd,IDC_ACKMODE1, Work[0]);
		portptr->AckMode = Work[0];

		Vallen=50;
		retCode = RegQueryValueEx(hKey, "Port1CkSum", 0,	&Type, Work, &Vallen);

		CheckDlgButton(hWnd,IDC_CKSUM1, Work[0]);
		portptr->CheckSum = Work[0];


		portptr = &InPorts[1];
		Vallen=50;
		retCode = RegQueryValueEx(hKey,"KISSPort2",0,			
			(ULONG *)&Type,(UCHAR *)&Work,(ULONG *)&Vallen);
		
		SetDlgItemText(hWnd,IDC_COM2,Work);
		portptr->ComPort=atoi(Work);


		Vallen=50;
		retCode = RegQueryValueEx(hKey,"PortSpeed2",0,			
			(ULONG *)&Type,(UCHAR *)&Work,(ULONG *)&Vallen);
		
		SetDlgItemText(hWnd,IDC_SPEED2,Work);
		strcpy(portptr->Params,Work);

		Vallen=50;
		retCode = RegQueryValueEx(hKey,"Port2Enable",0,			
			(ULONG *)&Type,(UCHAR *)&Work,(ULONG *)&Vallen);
		
		CheckDlgButton(hWnd,IDC_EN2,Work[0]);
		portptr->PortEnabled=Work[0];

		Vallen=50;
		retCode = RegQueryValueEx(hKey, "Port2Polled", 0,	&Type, Work, &Vallen);

		CheckDlgButton(hWnd,IDC_POLLED2, Work[0]);
		portptr->Polled = Work[0];

		Vallen=50;
		retCode = RegQueryValueEx(hKey, "Port2AckMode", 0,	&Type, Work, &Vallen);

		CheckDlgButton(hWnd,IDC_ACKMODE2, Work[0]);
		portptr->AckMode = Work[0];

		Vallen=50;
		retCode = RegQueryValueEx(hKey, "Port2CkSum", 0,	&Type, Work, &Vallen);

		CheckDlgButton(hWnd,IDC_CKSUM2, Work[0]);
		portptr->CheckSum = Work[0];


		for (i = 0; i < OUTPORTS; i++)
		{
			char Map[12] = "@@@@@@@@@@";
			char Char[2] = " ";
			int j;
			UINT Port;

			portptr = &Ports[i];

			wsprintf(Key,"Port%d",i);
			Vallen=50;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&Work,(ULONG *)&Vallen);
			
			SetDlgItemText(hWnd,3000+i,Work);

			portptr->ComPort=atoi(Work);

			wsprintf(Key,"PortType%d",i);
			Vallen=50;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&Work,(ULONG *)&Vallen);
	
			SetDlgItemText(hWnd,4000+i,Work);
			portptr->PortType[0] = Work[0];

			wsprintf(Key,"LoopAppls%d",i);
			Vallen=4;

			retCode = RegQueryValueEx(hKey,Key,0, &Type, (UCHAR *)&portptr->LoopAppls, &Vallen);

			CheckDlgButton(hWnd, 5006 + i, portptr->LoopAppls);

			wsprintf(Key,"APChecksum%d",i);
			Vallen=4;

			retCode = RegQueryValueEx(hKey,Key,0, &Type, (UCHAR *)&portptr->CheckSum, &Vallen);

			CheckDlgButton(hWnd, 5012 + i, portptr->CheckSum);

			wsprintf(Key,"MAP%d",i);
			Vallen=11;
			retCode = RegQueryValueEx(hKey ,Key ,0, &Type, Map,&Vallen);

			for (j = 0; j < 10; j++)
			{
				if (Map[j] > '@')
				{
					Char[0] = Map[j];

					// Add to input port poll list

					if (Char[0] > 'J')
					{
						Port = Char[0] - 'K';
						InPorts[1].PollMask |= (1 << Port);
						if (Port > InPorts[1].PollLimit)
							InPorts[1].PollLimit = Port;
					}
					else
					{
						Port = Char[0] - 'A';
						InPorts[0].PollMask |= (1 << Port);
						if (Port > InPorts[0].PollLimit)
							InPorts[0].PollLimit = Port;
					}

					// Set up In>Out and Out > In Map

					portptr->IntoOutMap[Char[0] - 'A'] = j + 1;
					portptr->OuttoInMap[j] = Char[0] - '@';

				}

				else
					Char[0] = 0;

				SetDlgItemText(hWnd, 7001 + i * 10 + j, Char);
			}
		}
		RegCloseKey(hKey);

	}

	return 0;
}

void SaveConfig()
{
	struct PortInfo * portptr;
	int retCode, i, j, val, disp;
	HKEY hKey=0;
	char Work[50];
	char Key[20];
	

	retCode = RegCreateKeyEx(HKEY_CURRENT_USER,
                              "SOFTWARE\\G8BPQ\\KISSMux",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);


	if (retCode == ERROR_SUCCESS)
	{
		GetDlgItemText(hWnd,IDC_COM1,Work,50);			
		RegSetValueEx(hKey,"KISSPort1",0,REG_SZ,(BYTE *)&Work,strlen(Work)+1);

		GetDlgItemText(hWnd,IDC_COM2,Work,50);			
		RegSetValueEx(hKey,"KISSPort2",0,REG_SZ,(BYTE *)&Work,strlen(Work)+1);

		GetDlgItemText(hWnd,IDC_SPEED1,Work,50);			
		RegSetValueEx(hKey,"PortSpeed1",0,REG_SZ,(BYTE *)&Work,strlen(Work)+1);

		GetDlgItemText(hWnd,IDC_SPEED2,Work,50);			
		RegSetValueEx(hKey,"PortSpeed2",0,REG_SZ,(BYTE *)&Work,strlen(Work)+1);

		val=IsDlgButtonChecked(hWnd, IDC_EN1);
		RegSetValueEx(hKey,"Port1Enable",0,REG_DWORD,(BYTE *)&val,4);

		val=IsDlgButtonChecked(hWnd, IDC_EN2);
		RegSetValueEx(hKey,"Port2Enable",0,REG_DWORD,(BYTE *)&val,4);

		val=IsDlgButtonChecked(hWnd, IDC_POLLED1);
		RegSetValueEx(hKey,"Port1Polled",0,REG_DWORD,(BYTE *)&val,4);

		val=IsDlgButtonChecked(hWnd, IDC_POLLED2);
		RegSetValueEx(hKey,"Port2Polled",0,REG_DWORD,(BYTE *)&val,4);

		val=IsDlgButtonChecked(hWnd, IDC_ACKMODE1);
		RegSetValueEx(hKey,"Port1AckMode",0,REG_DWORD,(BYTE *)&val,4);

		val=IsDlgButtonChecked(hWnd, IDC_ACKMODE2);
		RegSetValueEx(hKey,"Port2AckMode",0,REG_DWORD,(BYTE *)&val,4);

		val=IsDlgButtonChecked(hWnd, IDC_CKSUM1);
		RegSetValueEx(hKey,"Port1CkSum",0,REG_DWORD,(BYTE *)&val,4);

		val=IsDlgButtonChecked(hWnd, IDC_CKSUM2);
		RegSetValueEx(hKey,"Port2CkSum",0,REG_DWORD,(BYTE *)&val,4);

		for (i = 0; i < OUTPORTS + 1; i++)
		{
			char Map[12] = "@@@@@@@@@@";

			portptr = &Ports[i];

			wsprintf(Key,"Port%d",i);

			GetDlgItemText(hWnd,3000+i,Work,50);
			RegSetValueEx(hKey,Key,0,REG_SZ,(BYTE *)&Work,strlen(Work)+1);
	
			wsprintf(Key,"PortType%d",i);

			GetDlgItemText(hWnd,4000+i,Work,50);
			RegSetValueEx(hKey,Key,0,REG_SZ,(BYTE *)&Work,strlen(Work)+1);

			wsprintf(Key,"LoopAppls%d",i);
			RegSetValueEx(hKey, Key, 0, REG_DWORD, (BYTE *)&portptr->LoopAppls, 4);

			for (j = 0; j < 10; j++)
			{
				GetDlgItemText(hWnd, 7001 + i * 10 + j, Work ,50);
				if (Work[0])
					Map[j] = Work[0];
			}

			wsprintf(Key,"APChecksum%d",i);
			RegSetValueEx(hKey, Key, 0, REG_DWORD, (BYTE *)&portptr->CheckSum, 4);

			for (j = 0; j < 10; j++)
			{
				GetDlgItemText(hWnd, 7001 + i * 10 + j, Work ,50);
				if (Work[0])
					Map[j] = Work[0];
			}


			wsprintf(Key,"MAP%d",i);
			RegSetValueEx(hKey, Key, 0, REG_SZ, Map, 11);
		}
	}
	
	RegCloseKey(hKey);

	MessageBox(NULL, "You need to restart KISSMux to action config changes",
		"KISSMux", MB_OK | MB_SETFOREGROUND);

	return;
}

OpenPort(struct PortInfo * portptr)
{
	int resp, i, Errorval, count;
	char label[20];

	i = portptr->Index;

		if (portptr->PortType[0] == 'V' && portptr->ComPort != 0)
		{
			//' BPQ Virtual Port

			// First try new (UMDF) Driver

			char szPort[40];
			
			wsprintf( szPort, "\\\\.\\pipe\\BPQCOM%d", portptr->ComPort);

			portptr->hDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        	if (portptr->hDevice == (HANDLE) -1)
			{
				Errorval = GetLastError();
	
				if (Errorval == 231)
				{
					portptr->NewVCOM = TRUE;
					wsprintf(label,"Open Failed %d",portptr->ComPort);
					SetDlgItemText(hWnd,2000+i,label);
					return 0;
				}
			}

        	if (portptr->hDevice != (HANDLE) -1)
			{
				portptr->NewVCOM = TRUE;
			}
			else
			{
				if (portptr->NewVCOM)		// Already know it is new, so must be recovering
					return 0;

				portptr->hDevice = BPQOpenSerialPort(portptr->ComPort, &Errorval);
		
				if (portptr->hDevice == INVALID_HANDLE_VALUE && Errorval == 2)
				{
		
					//' Not found, so create
            
					resp = BPQSerialAddDevice(hControl, &portptr->ComPort, &Errorval);

					portptr->Created = resp;
            
					portptr->hDevice = BPQOpenSerialPort(portptr->ComPort, &Errorval);
				}
			
				count = -1;
        
				resp = BPQSerialIsCOMOpen(portptr->hDevice, &count);
			
				if (resp)
				{
					resp = BPQSerialSetCTS(portptr->hDevice);
					resp = BPQSerialSetDSR(portptr->hDevice);
					resp = BPQSerialSetDCD(portptr->hDevice);
				}
			}
        	if (portptr->hDevice != (HANDLE) -1)
			{
				wsprintf(label,"Virtual COM%d",portptr->ComPort);
				SetDlgItemText(hWnd,2000+i,label);
			}
			else
				SetDlgItemText(hWnd,4000+i,"?");

		}
    
		if (portptr->PortType[0] == 'R' && portptr->ComPort != 0)
		{
			//' real Serial Port

			SetDlgItemText(hWnd,2000+i,label);

			if (OpenRealPort(portptr))
			{
				wsprintf(portptr->PortLabel,"COM%d", portptr->ComPort);
				portptr->RTS = 1;
//				portptr->DTR = 1;
//				EscapeCommFunction(portptr->hDevice,SETDTR);
				EscapeCommFunction(portptr->hDevice,SETRTS);

				_beginthread(RealPortThread,0,portptr);
			}
			else
				wsprintf(portptr->PortLabel,"Open Failed", portptr->ComPort);

			SetDlgItemText(hWnd,2000+i,portptr->PortLabel);

		}
		return 0;
}


int InitPorts()
{
	int resp, i, Errorval;
	struct PortInfo * portptr;

	hControl = BPQOpenSerialControl(&Errorval);

//	if(hControl == INVALID_HANDLE_VALUE)
//		MessageBox(hWnd, "BPQ Virtual COM Driver Control Channel Open Failed", TEXT("KISSMux"), MB_OK);

	for (i = 0; i < OUTPORTS; i++)
	{
		portptr = &Ports[i];
		portptr->Index = i;
		portptr->ApplPort = TRUE;

		OpenPort(portptr);
	}

	resp = CloseHandle(hControl);

	return 0;
}

BOOL OpenRealPort(struct PortInfo * portptr)
{
   char szPort[15];
   BOOL fRetVal ;
   COMMTIMEOUTS CommTimeOuts ;

   DCB	dcb;

  // load the COM prefix string and append port number
   
  wsprintf( szPort, "//./COM%d", portptr->ComPort) ;

   // open COMM device

   portptr->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_OVERLAPPED, 
                  NULL );
				  
	if (portptr->hDevice == (HANDLE) -1 )
	{
		portptr->LastError=GetLastError();
		return FALSE;
	}

      // setup device buffers

      SetupComm(portptr->hDevice, 4096, 4096 ) ;

      // purge any information in the buffer

      PurgeComm(portptr->hDevice, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

      // set up for overlapped I/O
	  
      CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
      CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
      SetCommTimeouts(portptr->hDevice, &CommTimeOuts ) ;

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
   
	  dcb.DCBlength = sizeof( DCB );

	  GetCommState(portptr->hDevice, &dcb );

	  BuildCommDCB(portptr->Params, &dcb);

	  dcb.ByteSize = 8;
	  dcb.fParity = 0;
	  dcb.StopBits = 0;

	  // setup hardware flow control

      dcb.fDtrControl = DTR_CONTROL_ENABLE;
      dcb.fRtsControl = RTS_CONTROL_ENABLE;

	  dcb.fInX = dcb.fOutX = 0;
	  dcb.XonChar = 0 ;
	  dcb.XoffChar = 0 ;
	  dcb.XonLim = 0 ;
	  dcb.XoffLim = 0 ;
	  dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = 0;
	  dcb.fDsrSensitivity = 0;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = TRUE ;

   fRetVal = SetCommState(portptr->hDevice, &dcb ) ;

	EscapeCommFunction(portptr->hDevice,SETDTR);
	EscapeCommFunction(portptr->hDevice,SETRTS);
	
   return (fRetVal);
}


void VirSerTimer()
{
	int count, i, resp, len=0;
	struct PortInfo * portptr;

	for (i = 0; i < 6; i++)
	{
		portptr = &Ports[i];

		if (portptr->hDevice == (HANDLE) -1)
		{
			// Try to reopen every 30 secs

			portptr->ReopenTimer++;
	
			if (portptr->ReopenTimer < 300)
				return;

			portptr->ReopenTimer = 0;
		
			OpenPort(portptr);

			if (portptr->hDevice == (HANDLE) -1)
				continue;
		}
	
		if (portptr->PortType[0] == 'V')
		{
			if (portptr->NewVCOM)
			{
			}
			else
			{
				count = -1;
				resp = BPQSerialIsCOMOpen(portptr->hDevice, &count);

				if (count == 1)
				{
					if (portptr->PortEnabled == 0)
					{
							// Changed
						portptr->PortEnabled = 1;
						CheckDlgButton(hWnd,5000 + portptr->Index, 1);
					}
				}
			
				else
				{
					if (portptr->PortEnabled == 1)
					{
							// Changed
						portptr->PortEnabled = 0;
						CheckDlgButton(hWnd,5000 + portptr->Index, 0);
					}
				}
			}
		}

	getcomm:

		if (portptr->NewVCOM)
		{
			int Available = 0;
		
			int ret = PeekNamedPipe(portptr->hDevice, NULL, 0, NULL, &Available, NULL);

			if (ret == 0)
			{
				ret = GetLastError();

				if (ret == ERROR_BROKEN_PIPE)
				{
					CloseHandle(portptr->hDevice);
					portptr->hDevice = INVALID_HANDLE_VALUE;
					continue;
				}
			}


			if (Available > 9000 - portptr->gpsinptr)
				Available = 9000 - portptr->gpsinptr;
		
			if (Available)
			{
				UCHAR * ptr1 = &portptr->GPSinMsg[portptr->gpsinptr];
				UCHAR * ptr2 = &portptr->GPSinMsg[portptr->gpsinptr];
				UCHAR c;
				int Length;
				
				ReadFile(portptr->hDevice, ptr1, Available, &len, NULL);

				// Have to look for FF escape chars

				Length = Available;

				while (Length != 0)
				{
					c = *(ptr1++);
					Length--;

					if (c == 0xff)
					{
						c = c = *(ptr1++);
						Length--;
						
						if (c == 0xff)			// ff ff means ff
						{
							Available--;
						}
						else
						{
							// This is connection statua from other end

							Available -= 2;
							portptr->PortEnabled = c;
							CheckDlgButton(hWnd,5000+portptr->Index, c);
							continue;
						}
					}
					*(ptr2++) = c;
				}
			}
			len = Available;
		}
		else
			BPQSerialGetData(portptr->hDevice, &portptr->GPSinMsg[portptr->gpsinptr], 2000 - portptr->gpsinptr, &len);

		if (len > 0)
		{
			portptr->gpsinptr+=len;

			CheckRX(portptr, portptr->GPSinMsg, portptr->gpsinptr);

			goto getcomm;
		}
	}

	return;
}

void PollKISSIn()
{
	int i, len;
	struct PortInfo * portptr;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	if (RecoveryTimer)
	{
		RecoveryTimer--;
		if (RecoveryTimer == 0)
		{
			SelectSource(IDC_RADIO1, TRUE);		// Try to re-open ports
		}
	}

	for (i = 0; i < 2; i++)
	{
		UINT * buffptr;
		int Written;

		portptr = &InPorts[i];
	
		if (!portptr->PortEnabled || !portptr->hDevice)
			continue;

		// if Polled, and Timer is running, decrement
		
		if (portptr->PollTimer)
			portptr->PollTimer--;

		if (portptr->PollTimer == 0)
		{
			// if Anything to send, send it (will only be for Polled ports

			if (portptr->KISS_Q)
			{
				buffptr = Q_REM(&portptr->KISS_Q);
				memset(&portptr->Overlapped, 0, sizeof(portptr->Overlapped));
				WriteFile(portptr->hDevice, &buffptr[2], buffptr[1], &Written, NULL);
				ReleaseBuffer(buffptr);
			}

			if (portptr->Polled && portptr->PollMask)
			{
				// Send Poll

				UCHAR Poll[10];
				UINT Port;

				Poll[0] = FEND;

				Port = portptr->NextPoll + 1;

				while ((portptr->PollMask & (1 << Port)) == 0)
				{
					Port++;
					if (Port > portptr->PollLimit)
						Port = 0;
				}

				portptr->NextPoll = Port;

				Poll[1] = (Port << 4) | 0x0e;	// Set Type = Poll
				Poll[2] = FEND;

				WriteFile(portptr->hDevice, Poll, 3, &Written, NULL);
				portptr->PollTimer = 10;

			}
		}



	getgpsin:

		// only try to read number of bytes in queue 
	
		len=0;

		if (!ClearCommError(portptr->hDevice, &dwErrorFlags, &ComStat))
		{
			// Comm Error - probably lost USB Port. Try closing and reopening after a delay

			if (RecoveryTimer == 0)
			{
				Debugprintf("Recovering Port %d", i);
				RecoveryTimer = 100;			// 10 Secs
				FlashWindow(hWnd, TRUE);
				continue;
			}
		}

		dwLength = min(2000, ComStat.cbInQue ) ;

		if (portptr->gpsinptr + dwLength > 9000)	// Corrupt data 
			portptr->gpsinptr = 0;

		if (dwLength > 0)
		{
			memset(&portptr->OverlappedRead, 0, sizeof(portptr->OverlappedRead));
			ReadFile(portptr->hDevice, &portptr->GPSinMsg [portptr->gpsinptr] , dwLength, &len, NULL);
		}
		if (len > 0)
		{
			portptr->gpsinptr+=len;

			CheckRX(portptr, portptr->GPSinMsg, portptr->gpsinptr);

			goto getgpsin;
		}
	}
	
	return;
}

void SendToGPS(char * GPSMsg, int len)
{
	int Written, i;
	struct PortInfo * portptr;

	for (i = 0; i < 2; i++)
	{
		portptr = &InPorts[i];
		if (portptr->PortEnabled)
		{
			memset(&portptr->Overlapped, 0, sizeof(portptr->Overlapped));
			WriteFile(portptr->hDevice, GPSMsg, len, &Written, &portptr->Overlapped);
		}
	}
}


void DistributeMessage(char * GPSMsg, int len, BOOL TOGPS)
{
	int Written, resp, i;
	struct PortInfo * portptr;

	SetDlgItemText(hWnd,IDC_Trace,GPSMsg);

	if (SendUDP)
	{
		txaddr.sin_family = AF_INET;
		txaddr.sin_port = htons(8874);
		txaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		sendto(udpsock, GPSMsg, len, 0, (LPSOCKADDR)&txaddr, addrlen);
	}

	if (TOGPS)
	{
		for (i = 0; i < 2; i++)
		{
			portptr = &InPorts[i];
			
			if (!portptr->PortEnabled || !portptr->hDevice)
				continue;

			memset(&portptr->Overlapped, 0, sizeof(portptr->Overlapped));
			WriteFile(portptr->hDevice, GPSMsg, len, &Written, &portptr->Overlapped);
		}
	
		GPSType = 0xffff;
		RMCMsg = FALSE;

		return;
	}

	for (i = 0; i < OUTPORTS; i++)
	{
		portptr = &Ports[i];

		if (portptr->PortEnabled)
		{
			if (portptr->PortType[0] == 'V')
			{
				if (portptr->NewVCOM)
				{
					// Have to escape all oxff chars, as these are used to get status info 

					UCHAR NewMessage[1000];
					UCHAR * ptr1 = GPSMsg;
					UCHAR * ptr2 = NewMessage;
					UCHAR c;

					int Length = len;
					int Newlen = len;

					while (Length != 0)
					{
						c = *(ptr1++);
						*(ptr2++) = c;

						if (c == 0xff)
						{
							*(ptr2++) = c;
							Newlen++;
						}
						Length--;
					}					
					resp =  WriteFile(portptr->hDevice, NewMessage, Newlen, &Written, NULL);
				}
				else
					resp = BPQSerialSendData(portptr->hDevice, GPSMsg, len);
				GetLastError();
			}

			if (portptr->PortType[0] == 'R')
			{
				memset(&portptr->Overlapped, 0, sizeof(portptr->Overlapped));
				WriteFile(portptr->hDevice, GPSMsg, len, &Written, &portptr->Overlapped);
			}
		}
	}

	GPSType = 0xffff;
	RMCMsg = FALSE;

	return;
}


VOID RealPortThread(struct PortInfo * portptr)
{
	int ModemStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;

	int len=0;
	char GPSMsg[160];
	char * ptr;
	DWORD      dwLength;

	do {

	GetCommModemStatus(portptr->hDevice,&ModemStat);

	if ((ModemStat & MS_CTS_ON) >> 4 != portptr->CTS)
	{
		portptr->CTS=!portptr->CTS;
		Refresh(portptr);
	}

	if ((ModemStat & MS_DSR_ON) >> 5 != portptr->DSR)
	{
		portptr->DSR=!portptr->DSR;
		Refresh(portptr);
	}

	if ((ModemStat & MS_RLSD_ON) >> 7 != portptr->DCD)
	{
		portptr->DCD=!portptr->DCD;
		Refresh(portptr);
	}

getcomm:

	// only try to read number of bytes in queue 
	
	len=0;

	ClearCommError(portptr->hDevice, &dwErrorFlags, &ComStat);

	dwLength = min(80, ComStat.cbInQue) ;

	if (dwLength > 0)
	{
		ReadFile(portptr->hDevice, &portptr->GPSinMsg [portptr->gpsinptr] ,dwLength,&len,0);

		if (len > 0)
		{
			portptr->gpsinptr+=len;

			ptr = memchr(portptr->GPSinMsg, 0x0a, portptr->gpsinptr);

			while (ptr != NULL)
			{
				ptr++;									// include lf
				len=ptr-(char *)&portptr->GPSinMsg;					
				memcpy(&GPSMsg,portptr->GPSinMsg,len);	

			//		DistributeMessage(GPSMsg, len);

				portptr->gpsinptr-=len;							// bytes left

				if (portptr->gpsinptr > 0)
				{
					memmove(portptr->GPSinMsg,ptr, portptr->gpsinptr);
					ptr = memchr(portptr->GPSinMsg, 0x0a, portptr->gpsinptr);
				}
				else
					ptr=0;
			}
			goto getcomm;
		}
	}


	Sleep(2000);

	} while (TRUE);
}


int Refresh(struct PortInfo * portptr)
{
	CheckDlgButton(hWnd,5000+portptr->Index, portptr->DCD);
	portptr->PortEnabled = portptr->DCD;   

	return 0;
}
void ClosePorts()
{
	int resp, i, Errorval;
	struct PortInfo * portptr;

	hControl = BPQOpenSerialControl(&Errorval);

	if(hControl == INVALID_HANDLE_VALUE)
		MessageBox(hWnd, "BPQ Virtual COM Driver Control Channel Open Failed", TEXT("GPSMux"), MB_OK);

	for (i = 0; i < OUTPORTS; i++)
	{
		portptr = &Ports[i];

		if (portptr->PortType[0] == 'V' && portptr->ComPort != 0)
		{ 
			//' BPQ Virtual Port
        
			CloseHandle(portptr->hDevice);
		}

		if (portptr->PortType[0] == 'R' && portptr->ComPort != 0)
		{
			//' real Serial Port

			CloseHandle(portptr->hDevice);
		}
	}

	resp = CloseHandle(hControl);

	if (InPorts[0].hDevice)
	{
		CloseHandle(InPorts[0].hDevice);
		InPorts[0].hDevice=0;
	}

	if (InPorts[1].hDevice)
	{
		CloseHandle(InPorts[1].hDevice);
		InPorts[1].hDevice=0;
	}

	return;
}

void SelectSource(int Index, BOOL Recovering)
{
	struct PortInfo * portptr;
	int i;
	char Msg[100];
	
	if (InPorts[0].hDevice)
	{
		CloseHandle(InPorts[0].hDevice);
		InPorts[0].hDevice=0;
	}

	if (InPorts[1].hDevice)
	{
		CloseHandle(InPorts[1].hDevice);
		InPorts[1].hDevice=0;
	}

	InPorts[0].PortEnabled = InPorts[0].ComPort;
	InPorts[1].PortEnabled = InPorts[1].ComPort;

	for (i = 0; i < 2; i++)
	{
		portptr = &InPorts[i];
		
		if (portptr->PortEnabled)
		{
		if (OpenRealPort(portptr))
		{
			portptr->RTS = 0;
			portptr->DTR = 0;
			EscapeCommFunction(portptr->hDevice,SETDTR);
			EscapeCommFunction(portptr->hDevice,SETRTS);
//			EscapeCommFunction(portptr->hDevice,CLRDTR);
//			EscapeCommFunction(portptr->hDevice,CLRRTS);

//			_beginthread(RealPortThread,0,portptr);
		}
		else
		{
			if (Recovering)
			{
				RecoveryTimer = 100;			// 10 Secs
			}
			else
			{
				wsprintf(Msg,"COMM %d Open Error %d", portptr->ComPort, portptr->LastError);               
				MessageBox(hWnd, Msg, TEXT("KISSMux"), MB_OK);
				portptr->PortEnabled = 0;   
				CheckDlgButton(hWnd,IDC_EN1+i, 0);
			}
		}
		}
	}
}


void SimulateRMC()
{
	char Msg[100];
	char NS='N', EW='E';
	char LatString[20], LongString[20], DateString[7], TimeString[7], CRCSTRING[3];
	time_t ltime;
	struct tm *newtime;


//Dim Msg As String, LatString As String, LongString As String, SogString As String, CoGString As String
//Dim DateString As String
//
//Dim CRC As Integer, resp As Long
	int Degrees, CRC;
	UINT i;
	double Minutes, Long;
	char Work[50];

//Dim x As Long
//Dim CRCSTRING As String

	int TickCount, Interval;
/*
'        RMC - Recommended minimum specific GPS/Transit data
'        RMC , 225446, A, 4916.45, N, 12311.12, W, 0.5, 54.7, 191194, 20.3, E * 68
'           225446       Time of fix 22:54:46 UTC
'           A            Navigation receiver warning A = OK, V = warning
'           4916.45,N    Latitude 49 deg. 16.45 min North
'           12311.12,W   Longitude 123 deg. 11.12 min West
'           000.5        Speed over ground, Knots
'           054.7        Course Made Good, True
'           191194       Date of fix  19 November 1994
'           020.3,E      Magnetic variation 20.3 deg East
'           *68          mandatory checksum

'
' Convert lat and long to rather odd NMEA format
'
*/

	TickCount = GetTickCount();

	Interval = TickCount - LastTickCount;  // Milliseconds since last tick

	LastTickCount = TickCount;

	GetDlgItemText(hWnd,IDC_HEADING,Work,50);
	COG=atof(Work);

	GetDlgItemText(hWnd,IDC_SPEED,Work,50);			
	SOG=atof(Work);


	LatIncrement = SOG * cos(COG * P2) / 216000 * Interval / 1000;

	LongIncrement = SOG * sin(COG * P2) / cos(Latitude * P2) / 216000 * Interval / 1000;

	Latitude = Latitude + LatIncrement;
	Longtitude = Longtitude + LongIncrement;

	if (Latitude < 0)
	{
		NS = 'S';
		Latitude=-Latitude;
	}
	if (Longtitude < 0)
	{
		EW = 'W';
		Long=-Longtitude;
	}
	else
		Long=Longtitude;

#pragma warning(push)
#pragma warning(disable:4244)

	Degrees = Latitude;
	Minutes = Latitude* 60.0 - (60 * Degrees);

	sprintf(LatString,"%02d%2.3f,%c",Degrees, Minutes, NS);
		
//		= Format(Degrees, "00") + Format(Minutes, "00.0000") + "," + NS
//	Text1 = Format(Degrees) + Chr(176) + Format(Minutes, " 00.000") + "' " + NS

	Degrees = Long;

#pragma warning(pop)

	Minutes = Long * 60 - 60 * Degrees;

	sprintf(LongString,"%03d%3.3f,%c",Degrees, Minutes, EW);

	time(&ltime);				     /* Get time in seconds */
	newtime = gmtime(&ltime);		 /* Convert time to struct */
                                     /* tm form */
	wsprintf(DateString,"%02d%02d%02d", newtime->tm_mday,newtime->tm_mon+1,newtime->tm_year-100);
	wsprintf(TimeString,"%02d%02d%02d", newtime->tm_hour,newtime->tm_min,newtime->tm_sec );

	sprintf(Msg,"$GPRMC,%s,A,%s,%s,%.1f,%.1f,%s,000.0,W*",TimeString, LatString, LongString, SOG, COG, DateString);

	CRC = 0;

	for (i = 1; i < strlen(Msg) - 1; i++)
	{
		CRC = CRC ^ Msg[i];
	}

	wsprintf(CRCSTRING,"%02X", CRC);

	strcat(Msg,CRCSTRING);
	strcat(Msg,"\r\n");

//Msg = Msg + CRCSTRING + vbCrLf

	DistributeMessage(Msg, strlen(Msg), FALSE);

	return;

}

// TCP Support (for GPS via ActiveSync)

int OpenListeningSocket(struct tcp_table_entry * arp)
{
	char Msg[255];
	PSOCKADDR_IN psin;
	BOOL bOptVal = TRUE;
	SOCKADDR_IN local_sin;  /* Local socket - internet style */
	int status;


	arp->TCPBuffer=malloc(4000);
	arp->TCPState = 0;

	// create the socket. Set to listening mode if Slave

	arp->TCPListenSock = socket(AF_INET, SOCK_STREAM, 0);

	if (arp->TCPListenSock == INVALID_SOCKET)
	{
		sprintf(Msg, "socket() failed error %d", WSAGetLastError());
		MessageBox(hWnd, Msg, "TCPMUX", MB_OK);
		return FALSE;
	}

	Debugprintf("TCP Listening Socket Created - socket %d  port %d ", arp->TCPListenSock, arp->port);

	psin=&local_sin;

	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = htonl(INADDR_ANY);
	
	psin->sin_port = htons(8873);        /* Convert to network ordering */

	if (bind(arp->TCPListenSock , (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
	{
		sprintf(Msg, "bind(sock) failed Error %d", WSAGetLastError());
		Debugprintf(Msg);
		closesocket(arp->TCPListenSock);

		return FALSE;
	}

	if (listen(arp->TCPListenSock, 1) < 0)
	{
		sprintf(Msg, "listen(sock) failed Error %d", WSAGetLastError());
		Debugprintf(Msg);
		closesocket(arp->TCPListenSock);
		return FALSE;
	}

	if ((status = WSAAsyncSelect(arp->TCPListenSock, hWnd, WSA_ACCEPT, FD_ACCEPT)) > 0)
	{
		sprintf(Msg, "WSAAsyncSelect failed Error %d", WSAGetLastError());
		Debugprintf(Msg);
		closesocket(arp->TCPListenSock);
		return FALSE;
	}

	return TRUE;
}



int Socket_Accept(int SocketId)
{
	int addrlen;
	struct tcp_table_entry * sockptr;
	SOCKET sock;
	char Msg[100];
	int index=0;
	BOOL bOptVal = TRUE;

	//  Find Socket entry

	Debugprintf("Incoming Connect - Socket %d", SocketId);

	sockptr = &tcp_table;

	addrlen=sizeof(struct sockaddr);

	sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

	if (sock == INVALID_SOCKET)
	{
		sprintf(Msg, " accept() failed Error %d", WSAGetLastError());
		MessageBox(hWnd, Msg, "GPSMUX", MB_OK);
		return FALSE;
	}

	Debugprintf("Connect accepted - Socket %d", sock);


	WSAAsyncSelect(sock, hWnd, WSA_DATA,
		FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

	sockptr->TCPSock = sock;

	return 0;

}
int Socket_Data(int sock, int error, int eventcode)
{
	struct tcp_table_entry  * sockptr;
	int index=0;

	//	Find Connection Record

	sockptr = &tcp_table;

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

			Debugprintf("TCP Close received for socket %d", sock);

			sockptr->TCPState = 0;
			closesocket(sock);
			return 0;
	}

	return 0;
}

int DataSocket_Read(struct tcp_table_entry * sockptr, SOCKET sock)
{
	int InputLen;

	// May have several messages per packet, or message split over packets

	if (sockptr->InputLen > 3000)	// Shouldnt have lines longer  than this in text mode
	{
		sockptr->InputLen=0;
	}
				
	InputLen=recv(sock, &sockptr->TCPBuffer[0], 100, 0);

	if (InputLen == 0)
		return 0;					// Does this mean closed?

	 DistributeMessage(&sockptr->TCPBuffer[0], InputLen, FALSE);

	return 0;
}
/*
$GPGGA

Global Positioning System Fix Data

eg1. $GPGGA,170834,4124.8963,N,08151.6838,W,1,05,1.5,280.2,M,-34.0,M,,,*75

Name	Example Data	Description
Sentence Identifier	$GPGGA	Global Positioning System Fix Data
Time	170834	17:08:34 UTC
Latitude	4124.8963, N	41d 24.8963' N or 41d 24' 54" N
Longitude	08151.6838, W	81d 51.6838' W or 81d 51' 41" W
Fix Quality:
- 0 = Invalid
- 1 = GPS fix
- 2 = DGPS fix	1	Data is from a GPS fix
Number of Satellites	05	5 Satellites are in view
Horizontal Dilution of Precision (HDOP)	1.5	Relative accuracy of horizontal position
Altitude	280.2, M	280.2 meters above mean sea level
Height of geoid above WGS84 ellipsoid	-34.0, M	-34.0 meters
Time since last DGPS update	blank	No last update
DGPS reference station id	blank	No station id
Checksum	*75	Used by program to check for transmission errors
C

*/
VOID SendKISSToAppl(struct PortInfo * portptr, UCHAR * rxbuffer, int Len)
{
	int Written, resp;

	if (portptr->PortType[0] == 'V')
	{
		if (portptr->NewVCOM)
		{
			// Have to escape all oxff chars, as these are used to get status info 

			UCHAR NewMessage[1000];
			UCHAR * ptr1 = rxbuffer;
			UCHAR * ptr2 = NewMessage;
			UCHAR c;

			int Length = Len;
			int Newlen = Len;

			while (Length != 0)
			{
				c = *(ptr1++);
				*(ptr2++) = c;

				if (c == 0xff)
				{
					*(ptr2++) = c;
					Newlen++;
				}
				Length--;
			}					
			resp =  WriteFile(portptr->hDevice, NewMessage, Newlen, &Written, NULL);
			return;
		}

		resp = BPQSerialSendData(portptr->hDevice, rxbuffer, Len);
		return;
	}

	if (portptr->PortType[0] == 'R')
	{
		memset(&portptr->Overlapped, 0, sizeof(portptr->Overlapped));
		WriteFile(portptr->hDevice, rxbuffer, Len, &Written, &portptr->Overlapped);
	}
	return;
}

void Checksum_Frame(UCHAR * rxbuffer, int * Len)
{
	UCHAR Sum = 0;
	char * ptr = rxbuffer;	

	while (ptr < &rxbuffer[*Len])
	{
		Sum ^= *(ptr++);
	}

	ptr--;

	if (Sum == FEND)
	{
		*ptr++ = FESC;
		*ptr++ = TFEND;
		(*Len) += 2;
	}
	else
	{
		if (Sum == FESC)
		{
			*ptr++ = FESC;
			*ptr++ = TFESC;
			(*Len) += 2;
		}
		else
		{
			*ptr++ = Sum;
			(*Len)++;
		}
	}
	*ptr++ = FEND;
}


BOOL Check_Checksum(UCHAR * rxbuffer, int * Len)
{
	UCHAR Sum = 0, Char;
	UCHAR * ptr = rxbuffer;		// Dont include Fend

	while (ptr < &rxbuffer[*Len])
	{
		Char = *(ptr++);

		if (Char == FESC)
		{
			Char = *(ptr++);

			if (Char == TFESC)			// ESC'ED FESC?
				Char = FESC;
			else
				if (Char == TFEND)			// ESC'ED FESC?
					Char = FEND;
			
		}
		Sum ^= Char;
	}

	if (Sum)
		return FALSE;
	
	// Must replace sum by FEND, and decrement count

	ptr-= 3;

	if (*ptr == FESC)
	{
		*ptr = FEND;
		(*Len) -= 2;
	}
	else
	{
		*(++ptr) = FEND;
		(*Len)--;
	}

	return TRUE;
}

VOID ProcessKISSPacket(struct PortInfo * Port, UCHAR * rxbuffer, int Len)
{
	int Written, i;
	struct PortInfo * portptr;
	UINT KISSCHAN, KISSTYPE;
	char Channel;
	UCHAR Copy[500];
	int OrigLen;

	KISSTYPE = rxbuffer[1] & 15;

	if (KISSTYPE == 0x0e)				// Poll/Poll Response
	{
		if (Port->ApplPort == 0)
			Port->PollTimer = 0;		// Response
		else
			// Poll on appl side. I think I can just echo it back - we don't realy need polling

			SendKISSToAppl(Port, rxbuffer, Len);

		return;
	}

	Port->PollTimer = 0;					// Data response 

	KISSCHAN = rxbuffer[1] >> 4;

	if (Port->CheckSum && KISSTYPE == 0)	// Data
	{
		if (Check_Checksum(rxbuffer, &Len) == 0)
			return;							// Duff
	}

	memcpy(Copy, rxbuffer, Len);			// Save a copy in case we checksum it
	OrigLen = Len;

	// if from Appl, send to host, and if looped set, to other appls

	if (Port->ApplPort)
	{
		portptr = &InPorts[0];
		Channel = Port->OuttoInMap[KISSCHAN];		// Channel No on KISS TNC Side

		if (Channel == 0)
			return;

		if (Channel > 10)
		{
			Channel -= 10;
			portptr = &InPorts[1];
		}

		if (portptr->PortEnabled)
		{	
			rxbuffer[1] = KISSTYPE | (Channel - 1) << 4;

			if (portptr->CheckSum && KISSTYPE == 0)
				Checksum_Frame(rxbuffer, &Len);

			if (portptr->Polled)
			{
				// Need to queue frames

				UINT * Buffer = GetBuffer();

				if (Buffer)
				{
					Buffer[1] = Len;
					memcpy(&Buffer[2], rxbuffer, Len);
					Q_ADD(&portptr->KISS_Q, Buffer);
				}
			}
			else
			{
				memset(&portptr->Overlapped, 0, sizeof(portptr->Overlapped));
				WriteFile(portptr->hDevice, rxbuffer, Len, &Written, NULL);
			}
		}
		if (Port->LoopAppls)
		{
			KISSCHAN = Channel - 1;				// Map as if from TNC

			for (i = 0; i < OUTPORTS; i++)
			{
				portptr = &Ports[i];

				if (portptr == Port)
					continue;				// Not to ourself

				if (portptr->PortEnabled && portptr->LoopAppls)
				{
					Channel = portptr->IntoOutMap[KISSCHAN];

					if (Channel == 0)
						continue;
		
					if (OrigLen != Len)			// If changed (by cksum)copy back
					{
						memcpy(rxbuffer, Copy, OrigLen);		
						Len = OrigLen;
					}

					rxbuffer[1] = KISSTYPE | (Channel - 1) << 4;

					if (portptr->CheckSum && KISSTYPE == 0)
						Checksum_Frame(rxbuffer, &Len);
					
					SendKISSToAppl(portptr, rxbuffer, Len);
				}
			}
		}
		return;
	}

	// From TNC, send to all open application ports with mapping for this channel

	if (Port->Index == 1)
		KISSCHAN += 10;

	for (i = 0; i < OUTPORTS; i++)
	{
		portptr = &Ports[i];

		if (portptr->PortEnabled)
		{
			Channel = portptr->IntoOutMap[KISSCHAN];

			if (Channel == 0)
				continue;
			
			rxbuffer[1] = KISSTYPE | (Channel - 1) << 4;
			SendKISSToAppl(portptr, rxbuffer, Len);
		}
	}

	return;
}


VOID ProcessKISSFrame(struct PortInfo * Port, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;
	UCHAR FrameCopy[500];

	//	Split into KISS Packets. By far the most likely is a single KISS frame
	//	so treat as special case

	if (rxbuffer[1] == FEND)			// Two FENDS - probably got out of sync
	{
		rxbuffer++;
		Len--;
	}
	
	FendPtr = memchr(&rxbuffer[1], FEND, Len-1);
	
	if (FendPtr == &rxbuffer[Len-1])
	{
		memcpy(FrameCopy, rxbuffer, Len);
		ProcessKISSPacket(Port, FrameCopy, Len );
		return;
	}
		
	// Process the first Packet in the buffer

	NewLen =  FendPtr - rxbuffer + 1;

	memcpy(FrameCopy, rxbuffer, NewLen);
	ProcessKISSPacket(Port, FrameCopy, NewLen );
	
	// Loop Back

	ProcessKISSFrame(Port, FendPtr+1, Len - NewLen);
	return;

}

void CheckRX(struct PortInfo * Port, UCHAR * RXBuffer, int Length)
{
	// If first char != FEND, then probably a Terminal Mode Frame. Wait for CR on end
			
	if (Length < 3)				// Minimum Frame Sise
		return;

	if (RXBuffer[Length-1] != FEND)
		return;					// Wait till we have a full frame

	ProcessKISSFrame(Port, RXBuffer, Length);	// Could have multiple packets in buffer

	Port->gpsinptr = 0;
		
	return;

}

