
#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <time.h>
//#include <Psapi.h>
#include <commctrl.h>

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#include "bpq32.h"
#include "kernelresource.h"
#include "TNCINFO.h"
#include "AsmStrucs.h"

extern char * PortConfig[33];

HANDLE hInstance;
extern HBRUSH bgBrush;
extern HWND ClientWnd, FrameWnd;
extern int OffsetH, OffsetW;

extern HMENU hMainFrameMenu;
extern HMENU hBaseMenu;
extern HANDLE hInstance;
extern HFONT hFont;

extern UCHAR NEXTID;
extern struct TRANSPORTENTRY * L4TABLE;
extern WORD MAXCIRCUITS;
extern UCHAR L4DEFAULTWINDOW;
extern WORD L4T1;

extern HKEY REGTREE;

extern int Ver[];

BOOL WINAPI Rig_Command();

KillTNC(struct TNCINFO * TNC);
RestartTNC(struct TNCINFO * TNC);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

char * GetChallengeResponse(char * Call, char *  ChallengeString);

VOID __cdecl Debugprintf(const char * format, ...);

extern UCHAR BPQDirectory[];

static RECT Rect;

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

int Winmor_Socket_Data(int sock, int error, int eventcode);

struct WL2KInfo * WL2KReports;

int WL2KTimer = 0;

int ModetoBaud[31] = {0,0,0,0,0,0,0,0,0,0,0,			// 0 = 10
					  200,600,3200,600,3200,3200,		// 11 - 16
					  0,0,0,0,0,0,0,0,0,0,0,0,0,600};	// 17 - 30

char HFCTEXT[81] = "";
int HFCTEXTLEN = 0;


extern char WL2KCall[10];
extern char WL2KLoc[7];

VOID * zalloc(int len)
{
	// malloc and clear

	void * ptr;

	ptr=malloc(len);

	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}



VOID MoveWindows(struct TNCINFO * TNC)
{
	RECT rcClient;
	int ClientHeight, ClientWidth;

	GetClientRect(TNC->hDlg, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	if (TNC->hMonitor)
		MoveWindow(TNC->hMonitor,2 , 185, ClientWidth-4, ClientHeight-187, TRUE);
}

char * Config;
static char * ptr1, * ptr2;

BOOL ReadConfigFile(int Port, int ProcLine())
{
	char buf[256],errbuf[256];

	if (TNCInfo[Port])					// If restarting, free old config
		free(TNCInfo[Port]);
	
	TNCInfo[Port] = NULL;

	Config = PortConfig[Port];

	if (Config)
	{
		// Using config from bpq32.cfg

		if (strlen(Config) == 0)
		{
			// Empty Config File - OK for most types

			struct TNCINFO * TNC = TNCInfo[Port] = zalloc(sizeof(struct TNCINFO));

			TNC->InitScript = malloc(2);
			TNC->InitScript[0] = 0;
		
			return TRUE;
		}

		ptr1 = Config;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1 + 1);
			buf[ptr2 - ptr1 + 1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcLine(buf, Port))
			{
				WritetoConsole("Bad config record ");
				WritetoConsole(errbuf);
			}
		}
	}
	else
	{
		wsprintf(buf," ** Error - No Configuration info in bpq32.cfg");
		WritetoConsole(buf);
	}

	return (TRUE);
}
GetLine(char * buf)
{
loop:

	if (ptr2 == NULL) 
		return 0;

	memcpy(buf, ptr1, ptr2 - ptr1 + 2);
	buf[ptr2 - ptr1 + 2] = 0;
	ptr1 = ptr2 + 2;
	ptr2 = strchr(ptr1, 13);
	
	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}

LRESULT CALLBACK PacWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	MINMAXINFO * mmi;

	int i;
	struct TNCINFO * TNC;

	HKEY hKey;
	char Key[80];
	int retCode, disp;

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

	switch (message) { 

	case WSA_DATA:				// Notification on data socket

		Winmor_Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WM_CREATE:

		break;

	case WM_PAINT:

//			hdc = BeginPaint (hWnd, &ps);
			
//			SelectObject( hdc, hFont) ;
			
//			EndPaint (hWnd, &ps);
//
//			wParam = hdc;
	
			break;        


	case WM_GETMINMAXINFO:

 		if (TNC->ClientHeight)
		{
			mmi = (MINMAXINFO *)lParam;
			mmi->ptMaxSize.x = TNC->ClientWidth;
			mmi->ptMaxSize.y = TNC->ClientHeight;
			mmi->ptMaxTrackSize.x = TNC->ClientWidth;
			mmi->ptMaxTrackSize.y = TNC->ClientHeight;
		}

		break;


	case WM_MDIACTIVATE:
	{
			 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);

			if (TNC->hMenu)
				AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)TNC->hMenu, "Actions");
			
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);

//			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) TNC->hMenu, (LPARAM) TNC->hWndMenu);
		}
		else
		{
			 // Deactivate
	
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		 }
			 
		// call DrawMenuBar after the menu items are set
		DrawMenuBar(FrameWnd);

		return DefMDIChildProc(hWnd, message, wParam, lParam);
	}



	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)TNC->hMenu)
		{
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, " TNC"))
				{
					EnableMenuItem(TNC->hMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(TNC->hMenu, WINMOR_KILL, MF_BYCOMMAND | MF_ENABLED);
		
					break;
				}
			}
			EnableMenuItem(TNC->hMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(TNC->hMenu, WINMOR_KILL, MF_BYCOMMAND | MF_GRAYED);
		}
			
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case WINMOR_KILL:

			KillTNC(TNC);
			break;

		case WINMOR_RESTART:

			KillTNC(TNC);
			RestartTNC(TNC);
			break;

		case WINMOR_RESTARTAFTERFAILURE:

			TNC->RestartAfterFailure = !TNC->RestartAfterFailure;
			CheckMenuItem(TNC->hMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->Port);
	
			retCode = RegCreateKeyEx(REGTREE, Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

			if (retCode == ERROR_SUCCESS)
			{
				RegSetValueEx(hKey,"TNC->RestartAfterFailure",0,REG_DWORD,(BYTE *)&TNC->RestartAfterFailure, 4);
				RegCloseKey(hKey);
			}
			break;
		}
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_SIZING:
	case WM_SIZE:

		MoveWindows(TNC);
			
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		
		switch (wmId)
		{ 

		case SC_RESTORE:

			TNC->Minimized = FALSE;
			break;

		case SC_MINIMIZE: 

			TNC->Minimized = TRUE;
			break;
		}
		
		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_CTLCOLORDLG:
		return (LONG)bgBrush;

		 
	case WM_CTLCOLORSTATIC:
	{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)bgBrush;
	}

	case WM_DESTROY:
		
		break;
	}
	return DefMDIChildProc(hWnd, message, wParam, lParam);
}

BOOL CreatePactorWindow(struct TNCINFO * TNC, char * ClassName, char * WindowTitle, int RigControlRow, WNDPROC WndProc, int Width, int Height)
{
    WNDCLASS  wc;
	char Title[80];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Key[80];
	char Size[80];
	int Top, Left;
	HANDLE hDlg = 0;
	static int LP = 1235;

	if (TNC->hDlg)
	{
		ShowWindow(TNC->hDlg, SW_SHOWNORMAL);
		SetForegroundWindow(TNC->hDlg);
		return FALSE;							// Already open
	}

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
    wc.lpfnWndProc = WndProc;                                      
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 
	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	if (TNC->Hardware == H_WINMOR || TNC->Hardware == H_TELNET || TNC->Hardware == H_V4)
		wsprintf(Title, "%s Status - Port %d", WindowTitle, TNC->Port);
	else if (TNC->Hardware == H_UZ7HO)
		wsprintf(Title, "Rigcontrol for UZ7HO Port %d", TNC->Port);
	else if (TNC->Hardware == H_MPSK)
		wsprintf(Title, "Rigcontrol for MultiPSK Port %d", TNC->Port);
	else
		wsprintf(Title,"%s Status - COM%d", WindowTitle, TNC->PortRecord->PORTCONTROL.IOBASE);


	TNC->hDlg = hDlg =  CreateMDIWindow(ClassName, Title, 0,
		  0, 0, Width, Height, ClientWnd, hInstance, ++LP);
	
	//	CreateDialog(hInstance,ClassName,0,NULL);
	

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->Port);
	
	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
		{
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &TNC->Minimized);

			if (Rect.top < - 500 || Rect.left < - 500)
			{
				Rect.left = 0;
				Rect.top = 0;
				Rect.right = 600;
				Rect.bottom = 400;
			}
		}

		if (TNC->Hardware == H_WINMOR)	
			retCode = RegQueryValueEx(hKey,"TNC->RestartAfterFailure",0,			
				(ULONG *)&Type,(UCHAR *)&TNC->RestartAfterFailure,(ULONG *)&Vallen);

		RegCloseKey(hKey);
	}

	Top = Rect.top;
	Left = Rect.left;

//	GetWindowRect(hDlg, &Rect);	// Get the real size

	MoveWindow(hDlg, Left - (OffsetW /2), Top - OffsetH, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);
	
	if (TNC->Minimized)
		ShowWindow(hDlg, SW_SHOWMINIMIZED);
	else
		ShowWindow(hDlg, SW_RESTORE);

	TNC->RigControlRow = RigControlRow;

	SetWindowText(TNC->xIDC_TNCSTATE, "Free");

	return TRUE;
}


// WL2K Reporting Code.

static SOCKADDR_IN sinx; 

char * CheckAppl(struct TNCINFO * TNC, char * Appl)
{
	struct APPLCALLS * APPL;
	struct BPQVECSTRUC * PORTVEC;
	int Allocated = 0, Available = 0;
	int App, Stream;
	struct TNCINFO * APPLTNC;

	Debugprintf("Checking if %s is running", Appl);

	for (App = 0; App < 32; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (_memicmp(APPL->APPLCMD, Appl, 12) == 0)
		{
			int ApplMask = 1 << App;

			// If App has an alias, assume it is running , unless a CMS alias - then check CMS

			if (APPL->APPLHASALIAS)
			{
				if (APPL->APPLPORT)
				{
					APPLTNC = TNCInfo[APPL->APPLPORT];
					{
						if (APPLTNC)
						{
							if (APPLTNC->TCPInfo && !APPLTNC->TCPInfo->CMSOK)
							return NULL;
						}
					}
				}
				return APPL->APPLCALL_TEXT;
			}

			// See if App is running

			PORTVEC=BPQHOSTVECPTR;

			for (Stream = 0; Stream < 64; Stream++)
			{	
				if (PORTVEC->HOSTAPPLMASK & ApplMask)
				{
					Allocated++;

					if (PORTVEC->HOSTSESSION == 0 && (PORTVEC->HOSTFLAGS &3) == 0)
					{
						// Free and no outstanding report
						
						return APPL->APPLCALL_TEXT;		// Running
					}
				}
				PORTVEC++;
			}
		}
	}

	return NULL;			// Not Running
}

VOID SendReporttoWL2KThread();

VOID CheckWL2KReportTimer()
{
	if (WL2KReports == NULL)
		return;					// Shouldn't happen!

	WL2KTimer--;

	if (WL2KTimer != 0)
		return;

	WL2KTimer = 32910/2;		// Every Half Hour
		
	if (CheckAppl(NULL, "RMS         ") == NULL)
		return;

	_beginthread(SendReporttoWL2KThread, 0, 0);

	return;
}


VOID SendReporttoWL2KThread()
{
	struct WL2KInfo * WL2KReport = WL2KReports;
	char * LastHost = NULL;
	char * LastRMSCall = NULL;
	char Message[256];
	int LastSocket = 0;
	SOCKET sock = 0;
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	int GroupRef;

	// Send all reports in list

	while (WL2KReport)
	{
		// Resolve Name if needed

		if (LastHost && strcmp(LastHost, WL2KReport->Host) == 0)		// Same host?
			goto SkipResolve;
	
		LastHost = WL2KReport->Host;
	
		destaddr.sin_family = AF_INET; 
		destaddr.sin_addr.s_addr = inet_addr(WL2KReport->Host);
		destaddr.sin_port = htons(WL2KReport->WL2KPort);

		if (destaddr.sin_addr.s_addr == INADDR_NONE)
		{
			//	Resolve name to address

			Debugprintf("Resolving %s", WL2KReport->Host);
			HostEnt = gethostbyname (WL2KReport->Host);
		 
			if (!HostEnt)
			{
				err = WSAGetLastError();

				Debugprintf("Resolve Failed for %s %d %x", WL2KReport->Host, err, err);
				return;			// Resolve failed
			}
	
			memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
		}

		//   Allocate a Socket entry

		if (sock)
			closesocket(sock);

		sock = socket(AF_INET,SOCK_DGRAM,0);

		if (sock == INVALID_SOCKET)
  	 		return; 

		ioctlsocket(sock, FIONBIO, &param);
 
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&bcopt, 4);

		destaddr.sin_family = AF_INET;

	SkipResolve:

		if (strstr(WL2KReport->ServiceCode, "MARS"))
			GroupRef = 3;
		else
			GroupRef = 1;

		wsprintf(Message, "06'%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %03d, '%s', %d, '%s'",
				WL2KReport->RMSCall, WL2KReport->BaseCall, WL2KReport->GridSquare, WL2KReport->Freq,
				WL2KReport->mode, WL2KReport->baud, WL2KReport->power, WL2KReport->height, WL2KReport->gain,
				WL2KReport->direction, WL2KReport->Times, GroupRef, WL2KReport->ServiceCode);

		Debugprintf("Sending %s", Message);

		sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

		if (LastRMSCall == NULL || strcmp(WL2KReport->RMSCall, LastRMSCall) != 0)
		{
			LastRMSCall = WL2KReport->RMSCall;
		
			wsprintf(Message, "00,%s,BPQ32,%d.%d.%d.%d",
				WL2KReport->RMSCall, Ver[0], Ver[1], Ver[2], Ver[3]);

			Debugprintf("Sending %s", Message);

			sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));
		}

		WL2KReport = WL2KReport->Next;
	}

	Sleep(100);
	closesocket(sock);
	sock = 0;

}
BOOL SendReporttoWL2K(struct TNCINFO * TNC)
{
	Debugprintf("Starting WL2K Update Thread");

//	_beginthread(SendReporttoWL2KThread,0,(int)TNC);

	return 0;
}

static SOCKET sock;
/*
VOID SendReporttoWL2KThread(struct TNCINFO * TNC)
{
	char Message[100];
	
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;

	struct ScanEntry ** Freqptr;
	char * Valchar;
	int dec, sign;
	char FreqString[80]="";
	int Mode;
	int Baud;
	char BandWidth;
	BOOL RPonPTC;

	struct TimeScan ** TimeBands;	// List of TimeBands/Frequencies
	char * ptr;

	ptr = strchr(TNC->RMSCall, ' ');
	if (ptr) *ptr = 0;				// Null Terminate

	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_addr.s_addr = inet_addr(TNC->Host);
	destaddr.sin_port = htons(TNC->WL2KPort);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
	//	Resolve name to address

		Debugprintf("Resolving %s", TNC->Host);
		HostEnt = gethostbyname (TNC->Host);
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();

			Debugprintf("Resolve Failed for %s %d %x", TNC->Host, err, err);
			return;			// Resolve failed
		}
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	}

	//   Allocate a Socket entry

	if (sock)
		closesocket(sock);

	sock = socket(AF_INET,SOCK_DGRAM,0);

	if (sock == INVALID_SOCKET)
  	 	return; 

	ioctlsocket(sock, FIONBIO, &param);
 
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&bcopt, 4);

	destaddr.sin_family = AF_INET;

	if (TNC->TCPInfo && TNC->TCPInfo->CMS)
	{
		// Telnet reporting Packet Freqs

		struct WL2KInfo * WL2KInfoPtr;
		int n = 0;
		struct PacketReportInfo * PktInfo;
				
		WL2KInfoPtr = &TNC->WL2KInfoList[0];

		while (WL2KInfoPtr->PacketData)
		{
			PktInfo = WL2KInfoPtr->PacketData;

			wsprintf(Message, "06'%s', '%s', '%s', %s, %d, %d, %d, %d, %d, %03d, '%s', 1, '%s'",
				TNC->RMSCall, TNC->BaseCall, TNC->GridSquare, WL2KInfoPtr->Freq,
				PktInfo->mode, PktInfo->baud, PktInfo->power, PktInfo->height, PktInfo->gain, PktInfo->direction,
				TNC->Comment, TNC->ServiceCode);

			Debugprintf("Sending %s", Message);

			sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

			WL2KInfoPtr = &TNC->WL2KInfoList[++n];
		}

		wsprintf(Message, "00,%s,BPQ32,%d.%d.%d.%d",
				TNC->RMSCall, Ver[0], Ver[1], Ver[2], Ver[3]);

		Debugprintf("Sending %s", Message);

		sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

		Sleep(100);

		closesocket(sock);
		sock = 0;

		return;
	}

	if (TNC->UseRigCtrlFreqs)
	{
		int HHStart;
		int HHEnd;

		if (TNC->RIG)
		{
			struct WL2KInfo * WL2KInfoPtr;
			int n = 0;

			TimeBands = TNC->RIG->TimeBands;

			if (TimeBands == NULL)
			{
				Debugprintf("No Freqs");
				return;
			}

			// Build Frequency list if needed

			if (TNC->WL2KInfoList[0].Bandwidth == 0)
			{
				// Not set up yet

			Debugprintf("Building Freq List");

			BandWidth = 'N';

			__try {

			while(TimeBands[1])
			{
				Freqptr = TimeBands[1]->Scanlist;
	
				if (Freqptr == NULL)
					return;			
		
				while (Freqptr[0])
				{
					if (Freqptr[0]->Supress)
					{
						Freqptr++;
						continue;							// Don't Report HF Packet
					}

					Valchar = _fcvt(Freqptr[0]->Freq + 1500, 0, &dec, &sign);

					Mode = 0;

					if (TNC->Hardware == H_TRK)
					{
						if (Freqptr[0]->RPacketMode)
						{
							BandWidth = 'W';
							Mode = Report_Robust;
						}
						else
						{
							Freqptr++;
							continue;							// Don't Report HF Packet
						}
					}
					else if (TNC->Hardware == H_SCS)
					{
						if (Freqptr[0]->PMaxLevel == '1')
						{
							BandWidth = 'N';
							Mode = Report_P1;
						}
						else if (Freqptr[0]->PMaxLevel == '2')
						{
							BandWidth = 'N';
							if (Freqptr[0]->PMinLevel == '1')
								Mode = Report_P12;
							else
								Mode = Report_P2;
						}
						else if (Freqptr[0]->PMaxLevel == '3')
						{
							BandWidth = 'W';
							if (Freqptr[0]->PMinLevel == '1')
								Mode = Report_P123;
							else if (Freqptr[0]->PMinLevel == '2')
								Mode = Report_P23;
							else
								Mode = Report_P3;
						}
						else if (Freqptr[0]->PMaxLevel == '4')
						{
							BandWidth = 'W';
							if (Freqptr[0]->PMinLevel == '1')
								Mode = Report_P1234;
							else if (Freqptr[0]->PMinLevel == '2')
								Mode = Report_P234;
							else if (Freqptr[0]->PMinLevel == '3')
								Mode = Report_P34;
							else
								Mode = Report_P4;
						}
						if (Freqptr[0]->RPacketMode)
						{
							if (Mode == 0)		// Just RP - no Pactor
								Mode = Report_Robust;
							
							if (TNC->RobustTime)
								RPonPTC = TRUE;	// Need to report both Pactor and RP
						}
						else
							RPonPTC = FALSE;

					}
					else if (TNC->Hardware == H_KAM)
					{
						if (Freqptr[0]->Bandwidth == 'W')	// WINMOR Wide
						{
							if (TNC->DontReportNarrowOnWideFreqs)
							{
								Freqptr++;
								continue;
							}
						}
						Mode = Report_P1;					// KAM only supports P1
					}
					else if (Freqptr[0]->Bandwidth == 'W')		// WINMOR
					{
						BandWidth = 'W';
						Mode = Report_WINMOR1600;
					}
					else if (Freqptr[0]->Bandwidth == 'N')
					{
						BandWidth = 'N';
						Mode = Report_WINMOR500;
					}

					if (Mode == 0)
					{
						Freqptr++;
						continue;
					}

					HHStart = TimeBands[1]->Start /3600;
					HHEnd = TimeBands[1]->End /3600;

					// See if freq already defined

					n = 0;
					
					WL2KInfoPtr = &TNC->WL2KInfoList[0];

					while (WL2KInfoPtr->Bandwidth)
					{
						if ((strcmp(WL2KInfoPtr->Freq, Valchar) == 0) && WL2KInfoPtr->Bandwidth == Mode && WL2KInfoPtr->RPonPTC == RPonPTC)
						{
							// Add timeband to freq. First see if contiguous hours

							int len = strlen(WL2KInfoPtr->TimeList);
							int LastHour = atoi(&WL2KInfoPtr->TimeList[len - 2]);

							if (HHStart - LastHour == 1)	// Next Hour
							{
								// Replace end time


								WL2KInfoPtr->TimeList[len - 2] = 0;
					
								wsprintf(WL2KInfoPtr->TimeList, "%s%02d",	WL2KInfoPtr->TimeList, HHEnd);
							}
							else
								wsprintf(WL2KInfoPtr->TimeList, "%s,%02d-%02d",	WL2KInfoPtr->TimeList, HHStart, HHEnd);

							goto gotfreq;
						}

						WL2KInfoPtr = &TNC->WL2KInfoList[++n];
					}

					// Not found - add it

					WL2KInfoPtr->Freq = _strdup(Valchar);
					WL2KInfoPtr->TimeList = malloc(100);

					wsprintf(WL2KInfoPtr->TimeList, "%02d-%02d", HHStart, HHEnd);
					WL2KInfoPtr->Bandwidth = Mode;
					WL2KInfoPtr->RPonPTC = RPonPTC;
			
				gotfreq:

					Freqptr++;
				}
				TimeBands++;
			}
			}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{
						Debugprintf("Program Error processing freq list");
					}

			}
		
			// Send each entry in the list

			n = 0;
					
			WL2KInfoPtr = &TNC->WL2KInfoList[0];
//02'KB1TCE-5', 'KB1TCE', 'FN54KB', 14105700, 21, 0, 100, 25, 0, 000, '00-23', 1
			while (WL2KInfoPtr->Bandwidth)
			{

				Baud = ModetoBaud[WL2KInfoPtr->Bandwidth];

				wsprintf(Message, "06'%s', '%s', '%s', %s, %d, %d, 0, 0, 0, 000, '%s', 1, '%s'",
					TNC->RMSCall, TNC->BaseCall, TNC->GridSquare, WL2KInfoPtr->Freq,
					WL2KInfoPtr->Bandwidth, Baud, WL2KInfoPtr->TimeList, TNC->ServiceCode);

				Debugprintf("Sending %s", Message);

				sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

				// Also report RP (Type 30) if scanning for rebust on PTC controllers
				
				if ((TNC->Hardware == H_SCS) && WL2KInfoPtr->RPonPTC && (WL2KInfoPtr->Bandwidth != 30))
				{
					wsprintf(Message, "06'%s', '%s', '%s', %s, %d, 600, 0, 0, 0, 000, '%s', 1, '%s'",
						TNC->RMSCall, TNC->BaseCall, TNC->GridSquare, WL2KInfoPtr->Freq,
						30, WL2KInfoPtr->TimeList, TNC->ServiceCode);

					Debugprintf("Sending %s", Message);

					sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));
				}
				WL2KInfoPtr = &TNC->WL2KInfoList[++n];
			}
			wsprintf(Message, "00,%s,BPQ32,%d.%d.%d.%d",
			TNC->RMSCall, Ver[0], Ver[1], Ver[2], Ver[3]);

			Debugprintf("Sending %s", Message);
			sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));
		}
	}
	else
	{
		Baud = ModetoBaud[TNC->WL2KMode];
		
		wsprintf(Message, "06'%s', '%s', '%s', %s, %d, %d, 50, 25, 0, 000, '%s', 1, '%s'",
			TNC->RMSCall, TNC->BaseCall, TNC->GridSquare, TNC->WL2KFreq, TNC->WL2KMode, Baud, TNC->Comment, TNC->ServiceCode);

		Debugprintf("Sending %s", Message);
		sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

		// Also report RP (Type 30) if scanning for rebust on PTC controllers

		if (TNC->Hardware == H_SCS && TNC->RobustTime)
		{
			wsprintf(Message, "06'%s', '%s', '%s', %s, %d, 600, 100, 25, 0, 000, '%s', 1, '%s'",
				TNC->RMSCall, TNC->BaseCall, TNC->GridSquare, TNC->WL2KFreq, 30, TNC->Comment, TNC->ServiceCode);

			Debugprintf("Sending %s", Message);
			sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));
		}

		wsprintf(Message, "00,%s,BPQ32,%d.%d.%d.%d",
		TNC->RMSCall, Ver[0], Ver[1], Ver[2], Ver[3]);

		Debugprintf("Sending %s", Message);
		sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

//		wsprintf(Message, "03%s", TNC->RMSCall);

//		Debugprintf("Sending %s", Message);

//		sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

	}

	Sleep(100);

	closesocket(sock);
	sock = 0;

	return;
}
*/
struct WL2KInfo * DecodeWL2KReportLine(char *  buf)
{
	//06'<callsign>', '<base callsign>', '<grid square>', <frequency>, <mode>, <baud>, <power>,
	// <antenna height>, <antenna gain>, <antenna direction>, '<hours>', <group reference>, '<service code>'

	 // WL2KREPORT  service, www.winlink.org, 8778, GM8BPQ, IO68VL, 00-23, 144800000, PKT1200, 10, 20, 5, 0, BPQTEST
	
	char * Context;
	char * p_cmd;
	char * param;
	char errbuf[256];
	struct WL2KInfo * WL2KReport = zalloc(sizeof(struct WL2KInfo));
	char * ptr;


	strcpy(errbuf, buf); 

	p_cmd = strtok_s(&buf[10], ", \t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;
	
	strcpy(WL2KReport->ServiceCode, p_cmd);

	p_cmd = strtok_s(NULL, ", \t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;

	WL2KReport->Host = _strdup(p_cmd);

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);			
	if (p_cmd == NULL) goto BadLine;

	WL2KReport->WL2KPort = atoi(p_cmd);
	if (WL2KReport->WL2KPort == 0) goto BadLine;

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);		
	if (p_cmd == NULL) goto BadLine;

	strcpy(WL2KReport->RMSCall, p_cmd);
	strcpy(WL2KReport->BaseCall, p_cmd);
	strlop(WL2KReport->BaseCall, '-');					// Remove any SSID
	
	strcpy(WL2KCall, WL2KReport->BaseCall);				// For SYSOP Update

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);		
	if (p_cmd == NULL) goto BadLine;
	if (strlen(p_cmd) != 6) goto BadLine;
	
	strcpy(WL2KReport->GridSquare, p_cmd);
	strcpy(WL2KLoc, p_cmd);

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;
	if (strlen(p_cmd) > 79) goto BadLine;
	
	// Convert any : in times to comma

	ptr = strchr(p_cmd, ':');

	while (ptr)
	{
		*ptr = ',';
		ptr = strchr(p_cmd, ':');
	}

	strcpy(WL2KReport->Times, p_cmd);

	p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);
	if (p_cmd == NULL) goto BadLine;

	WL2KReport->Freq = atoi(p_cmd);

	if (WL2KReport->Freq == 0)	// Invalid
		goto BadLine;					

	param = strtok_s(NULL, " ,\t\n\r", &Context);

	// Mode Designator - one of

	// PKTnnnnnn
	// WINMOR500
	// WINMOR1600
	// ROBUST
	// P1 P12 P123 P1234 etc

	if (memcmp(param, "PKT", 3) == 0)
	{
		int Speed, Mode;

		Speed = atoi(&param[3]);

		 WL2KReport->baud = Speed;
			
		 if (Speed < 1200)
			 Mode = 0;
		 else if (Speed < 2400)
			 Mode = 1;					// 1200
		 else if (Speed < 4800)
			 Mode = 2;					// 2400
		 else if (Speed < 9600)
			 Mode = 3;					// 4800
		 else if (Speed < 19200)
			 Mode = 4;					// 9600
		 else if (Speed < 38400)
			 Mode = 5;					// 19200
		 else
			 Mode = 6;					// 38400 +

		WL2KReport->mode = Mode;
	}
	else if (_stricmp(param, "WINMOR500") == 0)
		WL2KReport->mode = 21;
	else if (_stricmp(param, "WINMOR1600") == 0)
		WL2KReport->mode = 22;
	else if (_stricmp(param, "ROBUST") == 0)
	{
		WL2KReport->mode = 30;
		WL2KReport->baud = 600;
	}
	else if (_stricmp(param, "P1") == 0)
		WL2KReport->mode = 11;
	else if (_stricmp(param, "P12") == 0)
		WL2KReport->mode = 12;
	else if (_stricmp(param, "P123") == 0)
		WL2KReport->mode = 13;
	else if (_stricmp(param, "P2") == 0)
		WL2KReport->mode = 14;
	else if (_stricmp(param, "P23") == 0)
		WL2KReport->mode = 15;
	else if (_stricmp(param, "P3") == 0)
		WL2KReport->mode = 16;
	else if (_stricmp(param, "P1234") == 0)
		WL2KReport->mode = 17;
	else if (_stricmp(param, "P234") == 0)
		WL2KReport->mode = 18;
	else if (_stricmp(param, "P34") == 0)
		WL2KReport->mode = 19;
	else if (_stricmp(param, "P4") == 0)
		WL2KReport->mode = 20;
	else
		goto BadLine;
	
	param = strtok_s(NULL, " ,\t\n\r", &Context);

	// Optional Params

	WL2KReport->power = (param)? atoi(param) : 0;
	param = strtok_s(NULL, " ,\t\n\r", &Context);
	WL2KReport->height = (param)? atoi(param) : 0;
	param = strtok_s(NULL, " ,\t\n\r", &Context);
	WL2KReport->gain = (param)? atoi(param) : 0;
	param = strtok_s(NULL, " ,\t\n\r", &Context);
	WL2KReport->direction = (param)? atoi(param) : 0;

	WL2KTimer = 60;

	WL2KReport->Next = WL2KReports;
	WL2KReports = WL2KReport;

	return WL2KReport;

BadLine:
	WritetoConsole(" Bad config record ");
	WritetoConsole(errbuf);
	WritetoConsole("\r\n");

	return 0;
}

VOID UpdateMH(struct TNCINFO * TNC, UCHAR * Call, char Mode, char Direction)
{
	struct MHSTRUC * MH = TNC->PortRecord->PORTCONTROL.PORTMHEARD;
	struct MHSTRUC * MHBASE = MH;
	UCHAR AXCall[8];
	int i;
	char * LOC, *  LOCEND;
	char ReportMode[20];
	char NoLOC[7] = "";
	double Freq;
	char ReportFreq[_CVTBUFSIZE] = "";

	if (MH == 0) return;

	ConvToAX25(Call, AXCall);

	// Adjust freq to centre

//	if (Mode != ' ' && TNC->RIG->Valchar[0])
	if (TNC->RIG->Valchar[0])
	{
		Freq = atof(TNC->RIG->Valchar) + 0.0015;
		_gcvt(Freq, 9, ReportFreq);
	}

	if (TNC->Hardware != H_WINMOR)			// Only WINMOR has a locator
	{
		LOC = NoLOC;
		goto NOLOC;
	}

	LOC = memchr(Call, '(', 20);

	if (LOC)
	{
		LOCEND = memchr(Call, ')', 30);
		if (LOCEND)
		{
			LOC--;
			*(LOC++) = 0;
			*(LOCEND) = 0;
			LOC++;
			if (strlen(LOC) != 6 && strlen(LOC) != 0)
			{
				Debugprintf("Corrupt LOC %s %s", Call, LOC);
				LOC = NoLOC;
			}
		}		
	}
	else
		LOC = NoLOC;

NOLOC:

	for (i = 0; i < MHENTRIES; i++)
	{
		if (Mode == ' ' || Mode == '*')			// Packet
		{
			if ((MH->MHCALL[0] == 0) || ((memcmp(AXCall, MH->MHCALL, 7) == 0) && MH->MHDIGI == Mode)) // Spare or our entry
				goto DoMove;
		}
		else
		{
			if ((MH->MHCALL[0] == 0) || ((memcmp(AXCall, MH->MHCALL, 7) == 0) &&
				MH->MHDIGI == Mode && strcmp(MH->MHFreq, ReportFreq) == 0)) // Spare or our entry
				goto DoMove;
		}
		MH++;
	}

	//	TABLE FULL AND ENTRY NOT FOUND - MOVE DOWN ONE, AND ADD TO TOP

	i = MHENTRIES - 1;
		
	// Move others down and add at front
DoMove:
	if (i != 0)				// First
		memmove(MHBASE + 1, MHBASE, i * sizeof(struct MHSTRUC));

	memcpy (MHBASE->MHCALL, AXCall, 7);
	MHBASE->MHDIGI = Mode;
	MHBASE->MHTIME = _time32(NULL);

	memcpy(MHBASE->MHLocator, LOC, 6);
	strcpy(MHBASE->MHFreq, ReportFreq);

	// Report to NodeMap

	if (Mode == '*')
		return;							// Digi'ed Packet
	
	if (Mode == ' ') 					// Packet Data
	{
		if (TNC->PktUpdateMap == 1)
			Mode = '!';
		else	
			return;
	}
			
	ReportMode[0] = TNC->Hardware + '@';
	ReportMode[1] = Mode;
	if (TNC->Hardware == H_HAL)
		ReportMode[2] = TNC->CurrentMode; 
	else
		ReportMode[2] = (TNC->RIG->CurrentBandWidth) ? TNC->RIG->CurrentBandWidth : '?';
	ReportMode[3] = Direction;
	ReportMode[4] = 0;

	SendMH(TNC->Hardware, Call, ReportFreq, LOC, ReportMode);

	return;
}

VOID CloseDriverWindow(int port)
{
	struct TNCINFO * TNC;

	TNC = TNCInfo[port];
	if (TNC == NULL)
		return;

	if (TNC->hDlg == NULL)
		return;

	PostMessage(TNC->hDlg, WM_CLOSE,0,0);
//	DestroyWindow(TNC->hDlg);

	TNC->hDlg = NULL;

	return;
}

VOID SaveWindowPos(int port)
{
	struct TNCINFO * TNC;
	char Key[80];

	TNC = TNCInfo[port];

	if (TNC == NULL)
		return;

	if (TNC->hDlg == NULL)
		return;
	
	wsprintf(Key, "PACTOR\\PORT%d", port);

	SaveMDIWindowPos(TNC->hDlg, Key, "Size", TNC->Minimized);




	return;
}

BOOL ProcessIncommingConnect(struct TNCINFO * TNC, char * Call, int Stream, BOOL SENDCTEXT)
{
	struct TRANSPORTENTRY * Session;
	int Index = 0;
	UINT * buffptr;

	// Stop Scanner

	if (Stream == 0 || TNC->Hardware == H_UZ7HO)
	{
		char Msg[80];

		wsprintf(Msg, "%d SCANSTOP", TNC->Port);
		
		Rig_Command(-1, Msg);

		UpdateMH(TNC, Call, '+', 'I');
	}
	
	Session=L4TABLE;

	// Find a free Circuit Entry

	while (Index < MAXCIRCUITS)
	{
		if (Session->L4USER[0] == 0)
			break;

		Session++;
		Index++;
	}

	if (Index == MAXCIRCUITS)
		return FALSE;					// Tables Full

	memset(Session, 0, sizeof(struct TRANSPORTENTRY));

	memcpy(TNC->Streams[Stream].RemoteCall, Call, 9);	// Save Text Callsign 

	ConvToAX25(Call, Session->L4USER);
	ConvToAX25(GetNodeCall(), Session->L4MYCALL);

	Session->CIRCUITINDEX = Index;
	Session->CIRCUITID = NEXTID;
	NEXTID++;
	if (NEXTID == 0) NEXTID++;		// Keep non-zero

	TNC->PortRecord->ATTACHEDSESSIONS[Stream] = Session;
	TNC->Streams[Stream].Attached = TRUE;

	Session->L4TARGET = TNC->PortRecord;
	
	Session->L4CIRCUITTYPE = UPLINK+PACTOR;
	Session->L4WINDOW = L4DEFAULTWINDOW;
	Session->L4STATE = 5;
	Session->SESSIONT1 = L4T1;
	Session->SESSPACLEN = TNC->PortRecord->PORTCONTROL.PORTPACLEN;
	Session->KAMSESSION = Stream;

	Debugprintf("%d", Session->SESSPACLEN);


	TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel

	if (HFCTEXTLEN > 1 && SENDCTEXT)
	{
		buffptr = GetBuff();
		if (buffptr == 0) return TRUE;			// No buffers
					
		buffptr[1] = HFCTEXTLEN;
		memcpy(&buffptr[2], HFCTEXT, HFCTEXTLEN);
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
	}
	return TRUE;	
}

VOID ShowTraffic(struct TNCINFO * TNC)
{
	char Status[80];

	wsprintf(Status, "RX %d TX %d ACKED %d ", 
		TNC->Streams[0].BytesRXed, TNC->Streams[0].BytesTXed, TNC->Streams[0].BytesAcked);

	SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
}

OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed, BOOL Quiet)
{
	char szPort[15];
	char buf[80];
	BOOL fRetVal;
	COMMTIMEOUTS CommTimeOuts;

	DCB	dcb;

	// load the COM prefix string and append port number
   
	wsprintf( szPort, "//./COM%d", Port) ;

	// open COMM device

	conn->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (conn->hDevice == (HANDLE) -1)
	{
		wsprintf(buf," COM%d Setup Failed %d ", Port, GetLastError());
		
		if (!Quiet)
			WritetoConsole(buf);
		if (conn->hDlg)
			SetWindowText(conn->xIDC_COMMSSTATE, buf);
	
		return (FALSE);
	}

	SetupComm(conn->hDevice, 4096, 4096); // setup device buffers

	// purge any information in the buffer

	PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O
	  
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(conn->hDevice, &CommTimeOuts);

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(conn->hDevice, &dcb);

	 // setup hardware flow control

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
//	dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.BaudRate = Speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fInX = dcb.fOutX = 0;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;

	// other various settings

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	fRetVal = SetCommState(conn->hDevice, &dcb);

//	conn->RTS = 1;
//	conn->DTR = 1;

	EscapeCommFunction(conn->hDevice,SETDTR);
	EscapeCommFunction(conn->hDevice,SETRTS);

	wsprintf(buf,"COM%d Open", Port);
	SetWindowText(conn->xIDC_COMMSSTATE, buf);

	
	return TRUE;
}

VOID CheckForDetach(struct TNCINFO * TNC, int Stream, struct STREAMINFO * STREAM,
			VOID TidyCloseProc(), VOID ForcedCloseProc(), VOID CloseComplete())
{
	UINT * buffptr;

	if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
	{
		// Node has disconnected - clear any connection

		if (STREAM->Disconnecting)
		{
			// Already detected the detach, and have started to close

			STREAM->DisconnectingTimeout--;
			
			if (STREAM->DisconnectingTimeout)
				return;							// Give it a bit longer

			// Close has timed out - force a disc, and clear

			ForcedCloseProc(TNC, Stream);		// Send Tidy Disconnect

			goto NotConnected;
		}

		// New Disconnect

		Debugprintf("New Disconnect Port %d Q %x", TNC->Port, STREAM->BPQtoPACTOR_Q);
			
		if (STREAM->Connected || STREAM->Connecting)
		{
			char logmsg[120];	
			time_t Duration;

			// Need to do a tidy close

			STREAM->Disconnecting = TRUE;
			STREAM->DisconnectingTimeout = 300;			// 30 Secs

			if (Stream == 0)
				SetWindowText(TNC->xIDC_TNCSTATE, "Disconnecting");

			// Create a traffic record

			Duration = time(NULL) - STREAM->ConnectTime;
				
			wsprintf(logmsg,"Port %2d %9s Bytes Sent %d  BPS %d Bytes Received %d BPS %d Time %d Seconds",
				TNC->Port, STREAM->RemoteCall,
				STREAM->BytesTXed, STREAM->BytesTXed/Duration,
				STREAM->BytesRXed, STREAM->BytesRXed/Duration, Duration);

			Debugprintf(logmsg);

			if (STREAM->BPQtoPACTOR_Q)					// Still data to send?
				return;									// Will close when all acked

//			if (STREAM->FramesOutstanding && TNC->Hardware == H_UZ7HO)
//				return;									// Will close when all acked
			
			TidyCloseProc(TNC, Stream);					// Send Tidy Disconnect

			return;
		}

		// Not connected 
NotConnected:

		STREAM->Disconnecting = FALSE;
		STREAM->Attached = FALSE;
		STREAM->Connecting = FALSE;
		STREAM->Connected = FALSE;

		if (Stream == 0)
			SetWindowText(TNC->xIDC_TNCSTATE, "Free");

		STREAM->FramesQueued = 0;
		STREAM->FramesOutstanding = 0;

		CloseComplete(TNC, Stream);

		while(STREAM->BPQtoPACTOR_Q)
		{
			buffptr=Q_REM(&STREAM->BPQtoPACTOR_Q);
			ReleaseBuffer(buffptr);
		}

		while(STREAM->PACTORtoBPQ_Q)
		{
			buffptr=Q_REM(&STREAM->PACTORtoBPQ_Q);
			ReleaseBuffer(buffptr);
		}
	}
}
VOID SetupPortRIGPointers()
{
	struct TNCINFO * TNC;
	int port;

// For each Winmor/Pactor port set up the TNC to RIG pointers

	for (port = 1; port < 33; port++)
	{
		TNC = TNCInfo[port];

		if (TNC == NULL)
			continue;

		if (TNC->RIG == NULL)
			TNC->RIG = Rig_GETPTTREC(port);

		if (TNC->Hardware == H_WINMOR || TNC->Hardware == H_V4)
			if (TNC->RIG && TNC->PTTMode)
				TNC->RIG->PTTMode = TNC->PTTMode;
	
		if (TNC->RIG == NULL)
			TNC->RIG = &TNC->DummyRig;		// Not using Rig control, so use Dummy
	
/*		if (TNC->WL2KFreq[0])
		{
			// put in ValChar for MH reporting

			double Freq;

			Freq = atof(TNC->WL2KFreq) - 1500;
			Freq = Freq/1000000.;

			_gcvt(Freq, 9, TNC->RIG->Valchar);
			TNC->RIG->CurrentBandWidth = TNC->WL2KModeChar;
		}
*/
	}
}

BOOL InterlockedCheckBusy(struct TNCINFO * ThisTNC)
{
	// See if this port, or any interlocked ports are reporting channel busy

	struct TNCINFO * TNC;
	int i, Interlock = ThisTNC->Interlock;

	if (ThisTNC->Busy)
		return TRUE;				// Our port is busy
		
	if (Interlock == 0)
		return ThisTNC->Busy;		// No Interlock

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
	
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

		if (Interlock == TNC->Interlock)	// Same Group	
			if (TNC->Busy)
				return TRUE;				// Interlocked port is busy

	}
	return FALSE;					// None Busy
}

char ChallengeResponse[13];

char * GetChallengeResponse(char * Call, char *  ChallengeString)
{
	// Generates a response to the CMS challenge string...

	__int64 Challenge = _atoi64(ChallengeString);
	__int64 CallSum = 0;
	__int64 Mask;
	__int64 Response;
	__int64 XX = 1065484730;

	char CallCopy[10];
	UINT i;


	if (Challenge == 0)
		return "000000000000";

// Calculate Mask from Callsign

	memcpy(CallCopy, Call, 10);
	strlop(CallCopy, '-');
	strlop(CallCopy, ' ');

	for (i = 0; i < strlen(CallCopy); i++)
	{
		CallSum += CallCopy[i];
	}
	
	Mask = CallSum + CallSum * 4963 + CallSum * 782386;

	Response = (Challenge % 930249781);
	Response ^= Mask;

	sprintf(ChallengeResponse, "%012d", Response);

	return ChallengeResponse; // 001065484730
}

BOOL GetWL2KSYSOPInfo(char * Call, char * SQL, char * ReplyBuffer)
{
	SOCKET sock = 0;
	SOCKADDR_IN destaddr;
	SOCKADDR_IN sinx; 
	int len = 100;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Buffer[100];
	char SendBuffer[1000];
		
	destaddr.sin_family = AF_INET; 
	destaddr.sin_addr.s_addr = inet_addr("www.winlink.org");
	destaddr.sin_port = htons(8775);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address
		HostEnt = gethostbyname ("www.winlink.org");
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();

			Debugprintf("Resolve Failed for %s %d %x", "halifax.winlink.org", err, err);
			return 0 ;			// Resolve failed
		}
	
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	}

	//   Allocate a Socket entry

	sock = socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
  	 	return 0; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (LPSOCKADDR) &sinx, addrlen) != 0 )
  	 	return FALSE; 

	if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) != 0)
	{
		err=WSAGetLastError();
		closesocket(sock);
		return 0;
	}

	len = recv(sock, &Buffer[0], len, 0);

	len = wsprintf(SendBuffer, "04%07d%-12s%s%s", strlen(SQL), Call, GetChallengeResponse(Call, Buffer), SQL);

	send(sock, SendBuffer, len, 0);

	len = 1000;

	len = recv(sock, ReplyBuffer, len, 0);

	ReplyBuffer[len] = 0;
	Debugprintf(ReplyBuffer);

	closesocket(sock);

	return TRUE;

}

BOOL UpdateWL2KSYSOPInfo(char * Call, char * SQL)
{

	SOCKET sock = 0;
	SOCKADDR_IN destaddr;
	SOCKADDR_IN sinx; 
	int len = 100;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Buffer[1000];
//	char SendBuffer[] = "040000176GM8BPQ      ............SELECT SysopName, StreetAddress1, StreetAddress2, City, State, Country, PostalCode, GridSquare, EMail, WEBSite, Phones, AdditionalData FROM SysopRecords WHERE Callsign='GM8BPQ'";
//	char SendBuffer[] = "080000008G8BPQ       ............AAAAAAAA";
//	char SendBuffer[] = "040000053G8BPQ       001010818628SELECT Password FROM Passwords WHERE Callsign='G8BPQ'";
	char SendBuffer[1000] = "020000366GW8BPQ      000365713154";
		
//	char SQL[] = "REPLACE INTO SysopRecords SET Callsign='G8BPQ', GridSquare='IO92KX', EMail='', WEBSite='', SysopName='John', StreetAddress1='', StreetAddress2='', City='Nottingham', State='', Country='', PostalCode='', Phones='', AdditionalData='Developing BPQ32 interface for RMS Packet'";	
//	char SQL[] = "DELETE FROM SysopRecords WHERE Callsign='GW8BPQ'";

	destaddr.sin_family = AF_INET; 
	destaddr.sin_addr.s_addr = inet_addr("www.winlink.org");
	destaddr.sin_port = htons(8775);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address
		HostEnt = gethostbyname ("www.winlink.org");
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();

			Debugprintf("Resolve Failed for %s %d %x", "halifax.winlink.org", err, err);
			return 0 ;			// Resolve failed
		}
	
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	}

	//   Allocate a Socket entry

	sock = socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
  	 	return 0; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (LPSOCKADDR) &sinx, addrlen) != 0 )
  	 	return FALSE; 

	if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) != 0)
	{
		err=WSAGetLastError();
		closesocket(sock);
		return 0;
	}

//	STRCPY(SQL, "SELECT Password FROM Passwords WHERE Callsign='G8BPQ'");

	len = recv(sock, &Buffer[0], len, 0);

	len = wsprintf(SendBuffer, "02%07d%-12s%s%s", strlen(SQL), Call, GetChallengeResponse(Call, Buffer), SQL);

	send(sock, SendBuffer, len, 0);

	len = 1000;

	len = recv(sock, &Buffer[0], len, 0);

	Buffer[len] = 0;
	Debugprintf(Buffer);

	closesocket(sock);

	return TRUE;

}
