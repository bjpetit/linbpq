
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
extern BOOL MinimizetoTray;
HANDLE hInstance;
HBRUSH bgBrush;
#define BGCOLOUR RGB(236,233,216)

extern UCHAR NEXTID;
extern struct TRANSPORTENTRY * L4TABLE;
extern WORD MAXCIRCUITS;
extern UCHAR L4DEFAULTWINDOW;
extern WORD L4T1;

BOOL WINAPI Rig_Command();

KillTNC(struct TNCINFO * TNC);
RestartTNC(struct TNCINFO * TNC);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

VOID __cdecl Debugprintf(const char * format, ...);

extern UCHAR BPQDirectory[];

static RECT Rect;

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;

struct TNCINFO * TNCInfo[34] = {NULL};		// Records are Malloc'd

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

int Winmor_Socket_Data(int sock, int error, int eventcode);

VOID * zalloc(int len)
{
	// malloc and clear

	void * ptr;

	ptr=malloc(len);
	memset(ptr, 0, len);

	return ptr;
}


VOID MoveWindows(struct TNCINFO * TNC)
{
	RECT rcClient;
	int ClientHeight, ClientWidth;

	GetClientRect(TNC->hDlg, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	MoveWindow(TNC->hMonitor,4 , 200, ClientWidth-8, ClientHeight-205, TRUE);
}

FILE *file;
char * Config;
char * ptr1, * ptr2;

BOOL ReadConfigFile(char * fn, int Port, int ProcLine())
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
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
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

	UCHAR Value[100];

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
		wsprintf(buf," Config file %s not found ",Value);
		WritetoConsole(buf);
		return (TRUE);			// Will give Port Not Defined error later
	}

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcLine(buf, Port))
		{
			WritetoConsole(" Bad config record ");
			WritetoConsole(errbuf);
		}			
	}

	fclose(file);
	file = NULL;
	}
	return (TRUE);
}
GetLine(char * buf)
{
loop:

	if (file)
	{
		if (fgets(buf, 255, file) == NULL)
			return 0;
	}
	else
	{
		if (ptr2 == NULL) 
			return 0;

		memcpy(buf, ptr1, ptr2 - ptr1 + 2);
		buf[ptr2 - ptr1 + 2] = 0;
		ptr1 = ptr2 + 2;
		ptr2 = strchr(ptr1, 13);
	}

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

	int i;
	struct TNCINFO * TNC;

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

	switch (message) { 

	case WSA_DATA:				// Notification on data socket

		Winmor_Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)TNC->hPopMenu)
		{
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, "WINMOR TNC"))
				{
					EnableMenuItem(TNC->hPopMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(TNC->hPopMenu, WINMOR_KILL, MF_BYCOMMAND | MF_ENABLED);
		
					return TRUE;
				}
			}
			EnableMenuItem(TNC->hPopMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(TNC->hPopMenu, WINMOR_KILL, MF_BYCOMMAND | MF_GRAYED);
		}
			
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId) {

		case WINMOR_KILL:

			KillTNC(TNC);
			break;

		case WINMOR_RESTART:

			KillTNC(TNC);
			RestartTNC(TNC);
			break;

		case WINMOR_RESTARTAFTERFAILURE:

			TNC->RestartAfterFailure = !TNC->RestartAfterFailure;
			CheckMenuItem(TNC->hPopMenu, WINMOR_RESTARTAFTERFAILURE, (TNC->RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

			break;

		default:

			return 0;

		}

	case WM_SIZING:

		MoveWindows(TNC);
			
		return TRUE;

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
		
			// Remove the subclass from the edit control. 

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

BOOL CreatePactorWindow(struct TNCINFO * TNC, char * ClassName, char * WindowTitle, int RigControlRow, WNDPROC WndProc, LPCSTR MENU)
{
    WNDCLASS  wc;
	char Title[80];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Key[80];
	char Size[80];
	int Top, Left;
	HANDLE hDlg;

	if (TNC->hDlg)
	{
		ShowWindow(TNC->hDlg, SW_SHOWNORMAL);
		SetForegroundWindow(TNC->hDlg);
		return FALSE;							// Already open
	}

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON2) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = MENU;;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	TNC->hDlg = hDlg = CreateDialog(hInstance,ClassName,0,NULL);
	
	if (TNC->Hardware == H_WINMOR || TNC->Hardware == H_TELNET)
		wsprintf(Title, "%s Status - Port %d", WindowTitle, TNC->Port);
	else
		wsprintf(Title,"%s Status - COM%d", WindowTitle, TNC->PortRecord->PORTCONTROL.IOBASE);

	SetWindowText(hDlg, Title);

	if (MinimizetoTray)
		AddTrayMenuItem(hDlg, Title);


	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->Port);
	
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &TNC->Minimized);

		if (TNC->Hardware == H_WINMOR)	
			retCode = RegQueryValueEx(hKey,"TNC->RestartAfterFailure",0,			
				(ULONG *)&Type,(UCHAR *)&TNC->RestartAfterFailure,(ULONG *)&Vallen);
	}
	Top = Rect.top;
	Left = Rect.left;

	GetWindowRect(hDlg, &Rect);	// Get the real size

	MoveWindow(hDlg, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	if (TNC->Minimized)
		ShowWindow(hDlg, SW_HIDE);
	else
		ShowWindow(hDlg, SW_SHOWNORMAL);

	if (TNC->RIG)
	{
		struct RIGINFO * RIG = TNC->RIG;
		int RigRow = RigControlRow;

		RIG->hLabel = CreateWindow(WC_STATIC , "", WS_CHILD | WS_VISIBLE,
                 10, RigRow, 80,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hCAT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 90, RigRow, 40,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hFREQ = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 135, RigRow, 100,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hMODE = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 240, RigRow, 60,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hSCAN = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 300, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

		RIG->hPTT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 320, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

		//if (PORT->PortType == ICOM)
		//{
		//	wsprintf(msg,"%02X", PORT->Rigs[i].RigAddr);
		//	SetWindowText(RIG->hCAT, msg);
		//}
		SetWindowText(RIG->hLabel, RIG->RigName);

	}

	return TRUE;
}


// WL2K Reporting Code.

static SOCKADDR_IN sinx; 

BOOL CheckAppl(struct TNCINFO * TNC, char * Appl)
{
	struct APPLCALLS * APPL;
	struct BPQVECSTRUC * PORTVEC;
	int Allocated = 0, Available = 0;
	int App, Stream;
	// See if there is an RMS Application

	Debugprintf("Checking if RMS is running");

	for (App = 0; App < 32; App++)
	{
		APPL=&APPLCALLTABLE[App];

		if (_memicmp(APPL->APPLCMD, Appl, 12) == 0)
		{
			int ApplMask = 1 << App;

			memcpy(TNC->RMSCall, APPL->APPLCALL_TEXT, 9);		// Need Null on end

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
						
						return TRUE;		// Running
					}
				}
				PORTVEC++;
			}
		}
	}

	return FALSE;			// Not Running
}

VOID SendReporttoWL2KThread(struct TNCINFO * TNC);

BOOL SendReporttoWL2K(struct TNCINFO * TNC)
{
	Debugprintf("Starting WL2K Update Thread");

	_beginthread(SendReporttoWL2KThread,0,(int)TNC);

	return 0;
}

SOCKET sock;

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
	char BandWidth;

	struct TimeScan ** TimeBands;	// List of TimeBands/Frequencies

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

			Mode = TNC->NARROWMODE;
			BandWidth = 'N';

			__try {

			while(TimeBands[1])
			{
				Freqptr = TimeBands[1]->Scanlist;
	
				if (Freqptr == NULL)
					return;			
		
				while (Freqptr[0])
				{

					Valchar = _fcvt(Freqptr[0]->Freq + 1500, 0, &dec, &sign);

					if (Freqptr[0]->Bandwidth == 'W')
					{
						BandWidth = 'W';
						Mode = TNC->WIDEMODE;
					}
					else if (Freqptr[0]->Bandwidth == 'N')
					{
						BandWidth = 'N';
						Mode = TNC->NARROWMODE;
					}

					if (BandWidth == 'W')
					{
						if (TNC->DontReportNarrowOnWideFreqs && TNC->WIDEMODE == TNC->NARROWMODE)
						{
							Freqptr++;
							continue;
						}
					}

					HHStart = TimeBands[1]->Start /3600;
					HHEnd = TimeBands[1]->End /3600;

					// See if freq already defined

					n = 0;
					
					WL2KInfoPtr = &TNC->WL2KInfoList[0];

					while (WL2KInfoPtr->Bandwidth)
					{
						if (strcmp(WL2KInfoPtr->Freq, Valchar) == 0)
						{
							// Add timeband to freq

							wsprintf(WL2KInfoPtr->TimeList, "%s,%02d-%02d",
								WL2KInfoPtr->TimeList, HHStart, HHEnd);

							goto gotfreq;
						}

						WL2KInfoPtr = &TNC->WL2KInfoList[++n];
					}

					// Not found - add it

					WL2KInfoPtr->Freq = _strdup(Valchar);
					WL2KInfoPtr->TimeList = malloc(100);

					wsprintf(WL2KInfoPtr->TimeList, "%02d-%02d", HHStart, HHEnd);
					WL2KInfoPtr->Bandwidth = Mode;
			
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

			while (WL2KInfoPtr->Bandwidth)
			{
				wsprintf(Message, "02'%s', '%s', '%s', %s, %d, 0, 0, 0, 0, 000, '%s', 1",
					TNC->RMSCall, TNC->BaseCall, TNC->GridSquare, WL2KInfoPtr->Freq,
					WL2KInfoPtr->Bandwidth, WL2KInfoPtr->TimeList);

				Debugprintf("Sending %s", Message);

				sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));

				WL2KInfoPtr = &TNC->WL2KInfoList[++n];
			}
		}
	}
	else
	{
		wsprintf(Message, "02'%s', '%s', '%s', %s, %d, 0, 0, 0, 0, 000, '%s', 1",
			TNC->RMSCall, TNC->BaseCall, TNC->GridSquare, TNC->WL2KFreq, TNC->WL2KMode, TNC->Comment);

		Debugprintf("Sending %s", Message);
		sendto(sock, Message, strlen(Message),0,(LPSOCKADDR)&destaddr,sizeof(destaddr));
	}

	Sleep(100);

	closesocket(sock);
	sock = 0;

	return;
}

DecodeWL2KReportLine(struct TNCINFO * TNC,char *  buf, char NARROWMODE, char WIDEMODE)
{
	// WL2KREPORT Host, Port, G8BPQ, IO68VL,Testing BPQ,RIGCONTROL
	
	char * Context;
	char * p_cmd;
	char errbuf[256];

	p_cmd = strtok_s(&buf[10], ", \t\n\r", &Context);
	if (p_cmd)
	{
		TNC->Host = _strdup(p_cmd);
		p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);		
		if (p_cmd)
		{
			TNC->WL2KPort = atoi(p_cmd);
			if (TNC->WL2KPort == 0) goto BadLine;
			p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);		
			if (p_cmd)
			{
				if (strlen(p_cmd) > 9) goto BadLine;
				strcpy(TNC->BaseCall, p_cmd);
				p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);		
				if (p_cmd)
				{
					if (strlen(p_cmd) != 6) goto BadLine;
						strcpy(TNC->GridSquare, p_cmd);
						p_cmd = strtok_s(NULL, ",\t\n\r", &Context);
						if (p_cmd)
						{
							if (strlen(p_cmd) > 79) goto BadLine;
							strcpy(TNC->Comment, p_cmd);
							p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);
							if (p_cmd)
							{
								if (_stricmp(p_cmd, "RIGCONTROL") == 0)
								{
									TNC->UseRigCtrlFreqs = TRUE;
									p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);
									if (p_cmd)
									{
										if (_stricmp(p_cmd, "DontReportNarrowOnWideFreqs") == 0)
										{
											TNC->DontReportNarrowOnWideFreqs = TRUE;
										}
									}
								}
								else
								{
									if (strlen(p_cmd) > 11) goto BadLine;
									strcpy(TNC->WL2KFreq, p_cmd);
									TNC->WL2KMode = NARROWMODE;
									p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);
									if (p_cmd)
									{
										if (p_cmd[0] == 'W')
										TNC->WL2KMode = WIDEMODE;
									}
								}
							}
						}
					}
				}
			}
		}
		TNC->UpdateWL2K = TRUE;
		TNC->UpdateWL2KTimer = 3000 + (rand() /100); // Send first after 5 Mins
		TNC->NARROWMODE = NARROWMODE;
		TNC->WIDEMODE = WIDEMODE;			// PIII only

		goto Okline;
	BadLine:
		WritetoConsole(" Bad config record ");
		WritetoConsole(errbuf);
		WritetoConsole("\r\n");
	Okline:;
			
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

	if (Mode != ' ' && TNC->RIG->Valchar[0])
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
		}		
	}
	else
		LOC = NoLOC;

NOLOC:

	for (i = 0; i < MHENTRIES; i++)
	{
		if (Mode == ' ')			// Packet
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

	if (Mode == ' ')					// Packet Data
	{
		MHBASE->MHLocator[0] = 0;
		MHBASE->MHFreq[0] = 0;
		return;
	}

	memcpy(MHBASE->MHLocator, LOC, 6);
	strcpy(MHBASE->MHFreq, ReportFreq);
			
	ReportMode[0] = TNC->Hardware + '@';
	ReportMode[1] = Mode;
	ReportMode[2] = (TNC->RIG->CurrentBandWidth) ? TNC->RIG->CurrentBandWidth : '?';
	ReportMode[3] = Direction;
	ReportMode[4] = 0;

	SendMH(TNC->Hardware, Call, ReportFreq, LOC, ReportMode);

	return;
}

VOID SaveWindowPos(int port)
{
	struct TNCINFO * TNC;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;

	TNC = TNCInfo[port];
	if (TNC == NULL)
		return;

	if (TNC->hDlg == NULL)
		return;

	ShowWindow(TNC->hDlg, SW_RESTORE);
	GetWindowRect(TNC->hDlg, &Rect);

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", port);
	
	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, 0, 0,
            KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		wsprintf(Size,"%d,%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom, TNC->Minimized);
		retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

		retCode = RegSetValueEx(hKey,"TNC->RestartAfterFailure",0,REG_DWORD,(BYTE *)&TNC->RestartAfterFailure, 4);

		RegCloseKey(hKey);
	}
	if (MinimizetoTray)	
		DeleteTrayMenuItem(TNC->hDlg);

//	PostMessage(TNC->hDlg, WM_QUIT,0,0);
	PostMessage(TNC->hDlg, WM_DESTROY,0,0);
	DestroyWindow(TNC->hDlg);

	TNC->hDlg = NULL;

	return;
}

BOOL ProcessIncommingConnect(struct TNCINFO * TNC, char * Call, int Stream)
{
	struct TRANSPORTENTRY * Session;
	int Index = 0;

	// Stop Scanner

	if (Stream == 0)
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
			Session->SESSPACLEN = 100;
			Session->KAMSESSION = Stream;

			TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel

			return TRUE;
}

VOID ShowTraffic(struct TNCINFO * TNC)
{
	char Status[80];

	wsprintf(Status, "RX %d TX %d ACKED %d ", 
		TNC->Streams[0].BytesRXed, TNC->Streams[0].BytesTXed, TNC->Streams[0].BytesAcked);

	SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
}

OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed)
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
		WritetoConsole(buf);
		OutputDebugString(buf);
		SetDlgItemText(conn->hDlg, IDC_COMMSSTATE, buf);

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
	SetDlgItemText(conn->hDlg, IDC_COMMSSTATE, buf);

	
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
			// Need to do a tidy close

			STREAM->Disconnecting = TRUE;
			STREAM->DisconnectingTimeout = 300;			// 30 Secs

			if (Stream == 0)
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Disconnecting");

			if (STREAM->BPQtoPACTOR_Q)					// Still data to send?
				return;									// Will close when all acked
			
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
			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");

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
	
		if (TNC->RIG == NULL)
			TNC->RIG = &TNC->DummyRig;		// Not using Rig control, so use Dummy
	
		if (TNC->WL2KFreq[0])
		{
			// put in ValChar for MH reporting

			double Freq;

			Freq = atof(TNC->WL2KFreq) - 1500;
			Freq = Freq/1000000.;

			_gcvt(Freq, 9, TNC->RIG->Valchar);
		}
	}
}


