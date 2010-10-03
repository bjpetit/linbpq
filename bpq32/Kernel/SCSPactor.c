//
//	DLL to inteface SCS TNC in Pactor Mode to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

// Dec 29 2009

//	Add Scan Control using %W Hostmode Command
//	Map Rig control port to a Virtual Serial Port.
//	Add Support for packet port(s).

// July 2010

// Support up to 32 BPQ Ports

// Version 1.1.1.14 August 2010 

// Drop RTS as well as DTR on close

// Version 1.2.1.1 August 2010 

// Save Minimized State

// Version 1.2.1.2 August 2010 

// Implement scan bandwidth change

// Version 1.2.1.3 September 2010 

// Don't connect if channel is busy
// Add WL2K reporting
// Add PACKETCHANNELS config command
// And Port Selector (P1 or P2) for Packet Ports

// Version 1.2.1.4 September 2010

// Fix Freq Display after Node reconfig
// Only use AutoConnect APPL for Pactor Connects

// Version 1.2.2.1 September 2010

// Add option to get config from bpq32.cfg


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"

//#include <process.h>
//#include <time.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#include "SCSPactor.h"
#include "ASMStrucs.h"
#include "RigControl.h"

#include "bpq32.h"

static char ClassName[]="PACTORSTATUS";
static char WindowTitle[] = "SCS Pactor";
static int RigControlRow = 210;

HANDLE hInstance;

#define SCS
#define WL2K
#define NARROWMODE 12
#define WIDEMODE 16			// PIII only

//
//	Code Common to Pactor Modules

#include <commctrl.h>

#define BGCOLOUR RGB(236,233,216)

extern BOOL MinimizetoTray;

static HBRUSH bgBrush;

BOOL WINAPI Rig_Command();
struct RIGINFO * WINAPI RigConfig();
BOOL WINAPI Rig_Poll();
VOID WINAPI Rig_PTT();
struct RIGINFO * WINAPI Rig_GETPTTREC();
struct ScanEntry ** WINAPI CheckTimeBands();

extern UCHAR BPQDirectory[];

extern BOOL Minimized;				// Start Minimized Flag

extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;
extern char * PortConfig[33];

RECT Rect;

struct RIGINFO DummyRig;		// Used if not using Rigcontrol

struct TNCINFO * TNCInfo[34];		// Records are Malloc'd

GetLine(char * buf);
int ProcessLine(char * buf, int Port);
VOID __cdecl Debugprintf(const char * format, ...);
BOOL CheckAppl(struct TNCINFO * TNC, char * Appl);
BOOL SendReporttoWL2K(struct TNCINFO * TNC);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

/*

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

#ifdef WINMOR
	int i;
	struct TNCINFO * TNC;
#endif
#ifdef SCS
	int i;
	struct TNCINFO * TNC;
#endif

	switch (message) { 

//		case WM_ACTIVATE:

//			SetFocus(hwndInput);
//			break;


#ifdef WINMOR

	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WM_INITMENUPOPUP:

		for (i=1; i<17; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
			
			if (TNC->hDlg == hWnd)
				break;
		}


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

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		for (i=1; i<17; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
		
			if (TNC->hDlg == hWnd)
				break;
		}


		switch (wmId) {

//		case WINMOR_CONFIG:
//
//			DialogBoxParam(hInstance, MAKEINTRESOURCE(WINMORCONFIG), hWnd, ConfigDialogProc, (LPARAM)TNC);
//			break;

		case WINMOR_KILL:

			KillTNC(TNC);
			break;

		case WINMOR_RESTART:

			KillTNC(TNC);
			RestartTNC(TNC);
 
			break;

		case WINMOR_RESTARTAFTERFAILURE:

			RestartAfterFailure = !RestartAfterFailure;
			CheckMenuItem(TNC->hPopMenu, WINMOR_RESTARTAFTERFAILURE, (RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

			break;

		default:

			return 0;

		}

	case WM_SIZING:

		for (i=1; i<17; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
		
			if (TNC->hDlg == hWnd)
				break;
		}

		MoveWindows(TNC);
			
		return TRUE;

#endif

#ifdef SCS

	case WM_COMMAND:

		for (i=1; i<17; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
			
			if (TNC->hDlg == hWnd)
				break;
		}

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

	case IDC_TEST:

//		SendExitEnter(TNC);

		break;

		default:

			return 0;

		}
#endif

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case SC_RESTORE:

			Minimized = FALSE;
			return (DefWindowProc(hWnd, message, wParam, lParam));

		case  SC_MINIMIZE: 

			Minimized = TRUE;
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

*/
static VOID UpdateMH(struct TNCINFO * TNC, UCHAR * Call, char Mode)
{
	struct MHSTRUC * MH = TNC->PortRecord->PORTCONTROL.PORTMHEARD;
	struct MHSTRUC * MHBASE = MH;
	UCHAR AXCall[8];
	int i;

	if (MH == 0) return;

	ConvToAX25(Call, AXCall);

	for (i = 0; i < 20; i++)
	{
		if ((MH->MHCALL[0] == 0) || ((memcmp(AXCall, MH->MHCALL, 7) == 0) &&
			MH->MHDIGI == Mode && strcmp(MH->MHFreq, TNC->RIG->Valchar) == 0)) // Spare our our entry
		{
			// Move others down and add at front

			if (i != 0)				// First
			{
				memmove(MHBASE + 1, MHBASE, i * sizeof(struct MHSTRUC));
			}

			memcpy (MHBASE->MHCALL, AXCall, 7);
			MHBASE->MHDIGI = Mode;
			MHBASE->MHTIME = _time32(NULL);
			strcpy(MHBASE->MHFreq, TNC->RIG->Valchar);

			return;
		}
		MH++;
	}

	return;
}

static ProcessLine(char * buf, int Port)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	char errbuf[256];

	strcpy(errbuf, buf);

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	ptr = strtok(NULL, " \t\n\r");

	if (_stricmp(buf, "ADDR") == 0)			// Winmor Using BPQ32 COnfig
	{
		BPQport = Port;
		p_ipad = ptr;
	}
	else
	if (_stricmp(buf, "APPL") == 0)			// Using BPQ32 COnfig
	{
		BPQport = Port;
		p_cmd = ptr;
	}
	else
	if (_stricmp(buf, "PORT") != 0)			// Using Old Config
	{
		// New config without a PORT or APPL  - this is a Config Command

		strcpy(buf, errbuf);

		BPQport = Port;

		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));

		TNC->InitScript = malloc(1000);
		TNC->InitScript[0] = 0;
#ifdef AEA
		strcpy(TNC->InitScript, "RESTART\r");
#endif
		goto ConfigLine;
	}
	else

	{

		// Old Config from file

	BPQport=0;
	BPQport = atoi(ptr);
	
	p_cmd = strtok(NULL, " \t\n\r");

	#ifdef WINMOR

		p_ipad = strtok(NULL, " \t\n\r");

	#endif

	if (Port && Port != BPQport)
	{
		// Want a particular port, and this isn't it

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

		}
	}
	}
	if(BPQport > 0 && BPQport < 33)
	{
		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));

		TNC->InitScript = malloc(1000);
		TNC->InitScript[0] = 0;
#ifdef AEA
		strcpy(TNC->InitScript, "RESTART\r");
#endif

#ifdef WINMOR
	
		if (p_ipad == NULL) return (FALSE);
	
		p_port = strtok(NULL, " \t\n\r");
			
		if (p_port == NULL) return (FALSE);

		WINMORport = atoi(p_port);

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(WINMORport);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(WINMORport+1);

		TNC->WINMORHostName = malloc(strlen(p_ipad)+1);

		if (TNC->WINMORHostName == NULL) return TRUE;

		strcpy(TNC->WINMORHostName,p_ipad);

		ptr = strtok(NULL, " \t\n\r");

		if (ptr)
		{
			if (_stricmp(ptr, "PTT") == 0)
			{
				ptr = strtok(NULL, " \t\n\r");

				if (ptr)
				{
					if (_stricmp(ptr, "CI-V") == 0)
						TNC->PTTMode = PTTCI_V;
					else if (_stricmp(ptr, "RTS") == 0)
						TNC->PTTMode = PTTRTS;
					else if (_stricmp(ptr, "DTR") == 0)
						TNC->PTTMode = PTTDTR;
					else if (_stricmp(ptr, "DTRRTS") == 0)
						TNC->PTTMode = PTTDTR | PTTRTS;

					p_cmd = strtok(NULL, " \t\n\r");
				}
			}
			else
				p_cmd = ptr;
		}
		else
			p_cmd = ptr;

		p_cmd = strtok(NULL, " \t\n\r");
#endif			
		if (p_cmd != NULL)
		{
			if (p_cmd[0] != ';' && p_cmd[0] != '#')
				TNC->ApplCmd=_strdup(p_cmd);
		}

		// Read Initialisation lines

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;
ConfigLine:
			strcpy(errbuf, buf);

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

			ptr = strchr(buf, ';');
			if (ptr)
			{
				*ptr++ = 13;
				*ptr = 0;
			}

			if (_memicmp(buf, "RIGCONTROL", 10) == 0)
			{
#ifdef SCS
				char * vcom = strstr(buf, "VCOM");
				if (vcom)
	
					// SCS Virtual COM Channel

					TNC->VCOMPort = atoi(&vcom[4]);		
#endif
				// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

				TNC->RigConfigMsg = _strdup(buf);
			}
			else

#ifdef SCS				
			if (_memicmp(buf, "PACKETCHANNELS", 14) == 0)
	
				// Packet Channels

				TNC->PacketChannels = atoi(&buf[14]);
			else
#endif
#ifdef KAM

			if (_memicmp(buf, "OLDMODE", 7) == 0)
				TNC->OldMode = TRUE;
			else
#endif
#ifdef WINMOR

			if ((_memicmp(buf, "CAPTURE", 7) == 0) || (_memicmp(buf, "PLAYBACK", 8) == 0))
			{}
			else
#endif
#ifdef WL2K
			if (_memicmp(buf, "WL2KREPORT", 10) == 0)
			{
				// WL2KREPORT Host, Port, G8BPQ,IO68VL,Testing BPQ,RIGCONTROL
				char * Context;
						
				p_cmd = strtok_s(&buf[10], ", \t\n\r", &Context);
				if (p_cmd)
				{
					TNC->Host = _strdup(p_cmd);
					p_cmd = strtok_s(NULL, " ,\t\n\r", &Context);		
					if (p_cmd)
					{
					TNC->Port = atoi(p_cmd);
					if (TNC->Port == 0) goto BadLine;
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
								if (strcmp(p_cmd, "RIGCONTROL") == 0)
									TNC->UseRigCtrlFreqs = TRUE;
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
				TNC->UpdateWL2KTimer = 3000;	// Send first after 5 Mins
				goto Okline;
			BadLine:
				WritetoConsole(" Bad config record ");
				WritetoConsole(errbuf);
				WritetoConsole("\r\n");
			Okline:;
			}
			else

#endif
#ifdef HAL

			if (_memicmp(buf, "TONES", 5) == 0)
			{
				int tone1 = 0, tone2 = 0;

				ptr = strtok(&buf[6], " ,/\t\n\r");
				if (ptr)
				{
					tone1 = atoi(ptr);
					ptr = strtok(NULL, " ,/\t\n\r");
					if (ptr)
					{
						tone2 = atoi(ptr);
						ptr = &TNC->InitScript[TNC->InitScriptLen];

						*(ptr++) = SetTones;		// Set Tones (Mark, Space HI byte first)
						*(ptr++) = tone1 >> 8;
						*(ptr++) = tone1 & 0xff;
						*(ptr++) = tone2 >> 8;
						*(ptr++) = tone2 & 0xff;

						TNC->InitScriptLen += 5;

						goto OkLine;
					}
				}
				goto BadLine;
			}
			if (_memicmp(buf, "DEFAULTMODE ", 12) == 0)
			{
					
				ptr = strtok(&buf[12], " ,\t\n\r");
				if (ptr)
				{
					if (_stricmp(ptr, "CLOVER") == 0)
						TNC->DefaultMode = Clover;
					else if (_stricmp(ptr, "PACTOR") == 0)
						TNC->DefaultMode = Pactor;
					else if (_stricmp(ptr, "AMTOR") == 0)
						TNC->DefaultMode = AMTOR;
					else goto BadLine;
				
				goto OkLine;
				}
				else goto BadLine;
			}

		BadLine:
			WritetoConsole(" Bad config record ");
			WritetoConsole(errbuf);
			WritetoConsole("\r\n");
		OkLine:;

#else
				strcat (TNC->InitScript, buf);
#endif
		}
	}

	return (TRUE);
	
}

// WL2K Reporting Code.

#ifdef XXXX

static SOCKADDR_IN sinx; 

static BOOL CheckAppl(struct TNCINFO * TNC, char * Appl)
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

		if (memcmp(APPL->APPLCMD, Appl, 12) == 0)
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
	char errmsg[80];
	int Error;              // catches return value of WSAStartup
    WORD VersionRequested;   // passed to WSAStartup
    WSADATA WsaData;            // receives data from WSAStartup

	struct ScanEntry ** Freqptr;
	char * Valchar;
	int dec, sign;
	char FreqString[80]="";
	int Mode;
	struct TimeScan ** TimeBands;	// List of TimeBands/Frequencies


	VersionRequested = MAKEWORD(1, 0);
    Error = WSAStartup(VersionRequested, &WsaData);

    if (Error)
	{
       MessageBox(NULL,
            TEXT("Could not initialise WinSock"),
            TEXT("WINMOR"), MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        return;
	}

	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_addr.s_addr = inet_addr(TNC->Host);
	destaddr.sin_port = htons(TNC->Port);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
	//	Resolve name to address

		Debugprintf("Resolving %s", TNC->Host);
		HostEnt = gethostbyname (TNC->Host);
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();

			wsprintf(errmsg, TEXT("Resolve Failed for %s %d %x"), TNC->Host, err, err);
			MessageBox(NULL, errmsg, "WINMOR Reporting", MB_OK);

			return;			// Resolve failed
		}
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	}

	//   Allocate a Socket entry


	if (sock)
		closesocket(sock);

	sock=socket(AF_INET,SOCK_DGRAM,0);

	if (sock == INVALID_SOCKET)
	{
  	 	return; 
	}

	ioctlsocket (sock, FIONBIO, &param);
 
	setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&bcopt, 4);

	destaddr.sin_family = AF_INET;

	if (TNC->UseRigCtrlFreqs)
	{
		int HHStart;
		int HHEnd;

		if (TNC->RIG == 0)
			TNC->RIG = Rig_GETPTTREC(TNC->PortRecord->PORTCONTROL.PORTNUMBER);

		if (TNC->RIG)
		{
			struct WL2KInfo * WL2KInfoPtr;
			int n = 0;

			TimeBands = TNC->RIG->TimeBands;

			if (TimeBands == NULL)
				return;

			// Build Frequency list if needed

			if (TNC->WL2KInfoList[0].Bandwidth == 0)
			{
				// Not set up yet

			Debugprintf("Building Freq List");

			Mode = NARROWMODE;

			__try {

			while(TimeBands[1])
			{
				Freqptr = TimeBands[1]->Scanlist;
	
				if (Freqptr == NULL)
					return;			
		
				while (Freqptr[0])
				{
					__try 
					{

					Valchar = _fcvt(Freqptr[0]->Freq + 1500, 0, &dec, &sign);

					if (Freqptr[0]->Bandwidth == 'W')
						Mode = WIDEMODE;
					else if (Freqptr[0]->Bandwidth == 'N')
						Mode = NARROWMODE;

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
					;
					}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{
						Debugprintf("Program Error processing freq entry");
					}

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

			__try
			{

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
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				Debugprintf("Program Error sending freq list");
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

#endif



extern UINT CRCTAB;
extern char * CTEXTMSG;
extern USHORT CTEXTLEN;
extern UINT FULL_CTEXT;
extern BPQTRACE();

BOOL EnterExit = FALSE;

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct TNCINFO * TNC);
static OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed);
VOID DEDPoll(int Port);
VOID CRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len);
unsigned short int compute_crc(unsigned char *buf,int len);
int Unstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len);
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
VOID ExitHost(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);
BOOL OpenVirtualSerialPort(struct TNCINFO * TNC);
int ReadVCommBlock(struct TNCINFO * TNC, char * Block, int MaxLength);
VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len);
Switchmode(struct TNCINFO * TNC, int Mode);


extern UCHAR NEXTID;
extern  struct TRANSPORTENTRY * L4TABLE;
extern  WORD MAXCIRCUITS;
extern  UCHAR L4DEFAULTWINDOW;
extern  WORD L4T1;

VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

char status[8][8] = {"ERROR",  "REQUEST", "TRAFFIC", "IDLE", "OVER", "PHASE", "SYNCH", ""};

char ModeText[8][14] = {"STANDBY", "AMTOR-ARQ",  "PACTOR-ARQ", "AMTOR-FEC", "PACTOR-FEC", "RTTY / CW", "LISTEN", "Channel-Busy"};

char PactorLevelText[4][14] = {"Not Connected", "PACTOR-I", "PACTOR-II", "PACTOR-III"};

VOID SCSClose()
{
	int i;
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	struct TNCINFO * TNC;

	
		for (i=1; i<33; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
			if (TNC->hDlg == NULL)
				continue;

			ShowWindow(TNC->hDlg, SW_RESTORE);
			GetWindowRect(TNC->hDlg, &Rect);

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", i);
	
			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, 0, 0,
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

			if (retCode == ERROR_SUCCESS)
			{
				wsprintf(Size,"%d,%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom, Minimized);
				retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

				RegCloseKey(hKey);
			}
			if (MinimizetoTray)	
				DeleteTrayMenuItem(TNC->hDlg);

		}
 
}

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int txlen = 0;
	UINT * buffptr;
	struct TNCINFO * TNC = TNCInfo[port];
	int Param;
	char Block[100];
	int Stream = 0;

	if (TNC == NULL || TNC->hDevice == (HANDLE) -1)
		return 0;			// Port not open

	if (!TNC->RIG)
	{
		TNC->RIG = Rig_GETPTTREC(port);

		if (TNC->RIG == 0)
			TNC->RIG = &DummyRig;			// Not using Rig control, so use Dummy
	}	

	switch (fn)
	{
	case 1:				// poll

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].ReportDISC)
			{
				TNC->Streams[Stream].ReportDISC = FALSE;
				buff[4] = Stream;

				return -1;
			}
		}

		if (TNC->VCOMHandle)
			ReadVCommBlock(TNC, Block, 100);

	
		if (EnterExit)
			return 0;						// Switching to Term mode to change bandwidth
		
		CheckRX(TNC);
		DEDPoll(port);

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].PACTORtoBPQ_Q !=0)
			{
				int datalen;
			
				buffptr=Q_REM(&TNC->Streams[Stream].PACTORtoBPQ_Q);

				datalen=buffptr[1];

				buff[4] = Stream;
				buff[7] = 0xf0;
				memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
				datalen+=8;
				buff[5]=(datalen & 0xff);
				buff[6]=(datalen >> 8);
		
				ReleaseBuffer(buffptr);
	
				return (1);
			}
		}
	
			
		return 0;

	case 2:				// send

		buffptr = GetBuff();

		if (buffptr == 0) return (0);			// No buffers, so ignore

		Stream = buff[4];

		if (!TNC->TNCOK)
		{
			// Send Error Response

			buffptr[1] = 36;
			memcpy(buffptr+2, "No Connection to PACTOR TNC\r", 36);

			C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			
			return 0;
		}

		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);

		TNC->Streams[Stream].FramesOutstanding++;
		
		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding

		_asm 
		{
			MOV	EAX,buff
			mov Stream,eax
		}

		if (Stream == 0)
		{
			if (TNC->Streams[0].FramesOutstanding  > 4)
				return (1 | TNC->HostMode << 8);
		}
		else
		{
			if (TNC->Streams[Stream].FramesOutstanding > 3 || TNC->Buffers < 200)	
				return (1 | TNC->HostMode << 8);		}

		return TNC->HostMode << 8;		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(TNCInfo[port]->hDevice);
				
		PostMessage(TNC->hDlg, WM_DESTROY,0,0);
		DestroyWindow(TNC->hDlg);

		if (MinimizetoTray)	
			DeleteTrayMenuItem(TNC->hDlg);

		TNC->hDlg = 0;

		return (0);

	case 6:				// Scan Stop Interface

		_asm 
		{
			MOV	EAX,buff
			mov Param,eax
		}

		if (Param == 1)		// Request Permission
		{
			if (TNC->TNCOK)
			{
				TNC->WantToChangeFreq = TRUE;
				TNC->OKToChangeFreq = FALSE;
				return TRUE;
			}
			return 0;		// Don't lock scan if TNC isn't reponding
		}

		if (Param == 2)		// Check  Permission
			return TNC->OKToChangeFreq;

		if (Param == 3)		// Release  Permission
		{
			TNC->WantToChangeFreq = FALSE;
			TNC->DontWantToChangeFreq = TRUE;
			return 0;
		}

		if (Param == 4)		// Set Wide Mode
		{
			if (TNC->Bandwidth != 'W')
			{
				TNC->Bandwidth = 'W';
				Switchmode(TNC, 3);
			}
			return 0;
		}

		if (Param == 5)		// Set Narrow Mode
		{
			if (TNC->Bandwidth != 'N')
			{
				TNC->Bandwidth = 'N';
				Switchmode(TNC, 2);
			}
			return 0;
		}
	}
	return 0;
}

UINT WINAPI SCSExtInit(EXTPORTDATA *  PortEntry)
{
	char msg[500];
	struct TNCINFO * TNC;
	int port;
	char * ptr;
	int Stream = 0;
	char * TempScript;

	//
	//	Will be called once for each Pactor Port
	//	The COM port number is in IOBASE
	//

	wsprintf(msg,"Pactor COM%d", PortEntry->PORTCONTROL.IOBASE);
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile("SCSPACTOR.CFG", port, ProcessLine);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(msg," ** Error - no info in SCSPACTOR.cfg for this port");
		WritetoConsole(msg);

		return (int) ExtProc;
	}

	if (TNC->RigConfigMsg)
	{
		char * SaveRigConfig = _strdup(TNC->RigConfigMsg);

		TNC->RIG = RigConfig(TNC->RigConfigMsg, port);
			
		if (TNC->RIG == NULL)
		{
			// Report Error

			wsprintf(msg,"Invalid Rig Config %s", SaveRigConfig);
			WritetoConsole(msg);

		}
		free(SaveRigConfig);
	}
	else
		TNC->RIG = NULL;		// In case restart

	// Set up DED addresses for streams (first stream (Pactor) = DED 31
	
	TNC->Streams[0].DEDStream = 31;

	for (Stream = 1; Stream <= MaxStreams; Stream++)
	{
		TNC->Streams[Stream].DEDStream = Stream;
	}

	if (TNC->PacketChannels > MaxStreams)
		TNC->PacketChannels = MaxStreams;

	PortEntry->MAXHOSTMODESESSIONS = TNC->PacketChannels + 1;
	PortEntry->PERMITGATEWAY = TRUE;					// Can change ax.25 call on each stream
	PortEntry->SCANCAPABILITIES = CONLOCK;				// Scan Control 3 stage/conlock 

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);
		
	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set TONES to 4

	TempScript = malloc(1000);

	strcpy(TempScript, "TONES 4\r");			// Tones may be changed but I want this as standard

	strcat(TempScript, "MAXERR 30\r");			// Max retries 
	strcat(TempScript, "MODE 0\r");				// ASCII mode, no PTC II compression (Forwarding will use FBB Compression)
	strcat(TempScript, "MAXSUM 20\r");			// Max count for memory ARQ
	strcat(TempScript, "CWID 0 2\r");			// CW ID disabled

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Others go on end so they can't be overriden

	strcat(TNC->InitScript, "ADDLF 0\r");      //      Auto Line Feed disabled
	strcat(TNC->InitScript, "ARX 0\r");      //        Amtor Phasing disabled
	strcat(TNC->InitScript, "BELL 0\r");      //       Disable Bell
	strcat(TNC->InitScript, "BC 0\r");      //         FEC reception is disabled
	strcat(TNC->InitScript, "BKCHR 2\r");      //      Breakin Char = 2
	strcat(TNC->InitScript, "CHOBELL 0\r");      //    Changeover Bell off
	strcat(TNC->InitScript, "CMSG 0\r");      //       Connect Message Off
	strcat(TNC->InitScript, "LFIGNORE 0\r");      //   No insertion of Line feed
	strcat(TNC->InitScript, "LISTEN 0\r");      //     Pactor Listen disabled
	strcat(TNC->InitScript, "MAIL 0\r");      //       Disable internal mailbox reporting
	strcat(TNC->InitScript, "REMOTE 0\r");      //     Disable remote control
	strcat(TNC->InitScript, "PAC CBELL 0\r");      //  
	strcat(TNC->InitScript, "PAC CMSG 0\r");      //  
	strcat(TNC->InitScript, "PAC PRBOX 0\r");      //  	Turn off Packet Radio Mailbox
	
	//  Automatic Status must be enabled for BPQ32
	//  Pactor must use Host Mode Chanel 31
	//  PDuplex must be set. The Node code relies on automatic IRS/ISS changeover
	//	5 second duplex timer

	strcat(TNC->InitScript, "STATUS 2\rPTCHN 31\rPDUPLEX 1\rPDTIMER 5\r");

	wsprintf(msg, "MYCALL %s\rPAC MYCALL %s\r", TNC->NodeCall, TNC->NodeCall);
	strcat(TNC->InitScript, msg);

	MinimizetoTray = GetMinimizetoTrayFlag();

	CreatePactorWindow(TNC, ClassName, WindowTitle, RigControlRow);

	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.IOBASE, PortEntry->PORTCONTROL.BAUDRATE);

	if (TNC->VCOMPort)
		OpenVirtualSerialPort(TNC);

	return ((int)ExtProc);
}

static void CheckRX(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;
	unsigned short crc;
	char UnstuffBuffer[500];

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)TNC->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return;
	}
	
	TNC->RXLen += Length;

	Length = TNC->RXLen;

	// DED mode doesn't have an end of frame delimiter. We need to know if we have a full frame

	// Fortunately this is a polled protocol, so we only get one frame at a time

	// If first char != 170, then probably a Terminal Mode Frame. Wait for CR on end

	// If first char is 170, we could check rhe length field, but that could be corrupt, as
	// we haen't checked CRC. All I can think of is to check the CRC and if it is ok, assume frame is
	// complete. If CRC is duff, we will eventually time out and get a retry. The retry code
	// can clear the RC buffer
			
	if (TNC->RXBuffer[0] != 170)
	{
		// Char Mode Frame I think we need to see cmd: on end

		// If we think we are in host mode, then to could be noise - just discard.

		if (TNC->HostMode)
		{
			TNC->RXLen = 0;		// Ready for next frame
			return;
		}

		TNC->RXBuffer[TNC->RXLen] = 0;

//		if (TNC->Streams[Stream].RXBuffer[TNC->Streams[Stream].RXLen-2] != ':')

		if (strlen(TNC->RXBuffer) < TNC->RXLen)
			TNC->RXLen = 0;

		if (strstr(TNC->RXBuffer, "cmd: ") == 0)

			return;				// Wait for rest of frame

		// Complete Char Mode Frame

		TNC->RXLen = 0;		// Ready for next frame
					
		if (TNC->HostMode == 0)
		{
			// We think TNC is in Terminal Mode
			ProcessTermModeResponse(TNC);
			return;
		}
		// We thought it was in Host Mode, but are wrong.

		TNC->HostMode = FALSE;
		return;
	}

	// Receiving a Host Mode frame

	if (Length < 6)				// Minimum Frame Sise
		return;

	if (TNC->RXBuffer[2] == 170)
	{
		// Retransmit Request
	
		TNC->RXLen = 0;
		return;				// Ignore for now
	}

	// Can't unstuff into same buffer - fails if partial msg received, and we unstuff twice


	Length = Unstuff(&TNC->RXBuffer[2], &UnstuffBuffer[2], Length - 2);

	if (Length == -1)
	{
		// Unstuff returned an errors (170 not followed by 0)

		TNC->RXLen = 0;
		return;				// Ignore for now
	}
	crc = compute_crc(&UnstuffBuffer[2], Length);

	if (crc == 0xf0b8)		// Good CRC
	{
		TNC->RXLen = 0;		// Ready for next frame
		ProcessDEDFrame(TNC, UnstuffBuffer, Length);

		// If there are more channels to poll (more than 1 entry in general poll response,
		// and link is not active, poll the next one

		if (TNC->Timeout == 0)
		{
			UCHAR * Poll = TNC->TXBuffer;

			if (TNC->NexttoPoll[0])
			{
				UCHAR Chan = TNC->NexttoPoll[0] - 1;

				memmove(&TNC->NexttoPoll[0], &TNC->NexttoPoll[1], 19);

				Poll[2] = Chan;			// Channel
				Poll[3] = 0x1;			// Command

				if (Chan == 254)		// Status - Send Extended Status (G3)
				{
					Poll[4] = 1;			// Len-1
					Poll[5] = 'G';			// Extended Status Poll
					Poll[6] = '3';
				}
				else
				{
					Poll[4] = 0;			// Len-1
					Poll[5] = 'G';			// Poll
				}

				CRCStuffAndSend(TNC, Poll, Poll[4] + 6);
				TNC->InternalCmd = FALSE;

				return;
			}
			else
			{
				// if last message wasn't a general poll, send one now

				if (TNC->PollSent)
					return;

				TNC->PollSent = TRUE;

				// Use General Poll (255)

				Poll[2] = 255 ;			// Channel
				Poll[3] = 0x1;			// Command

				Poll[4] = 0;			// Len-1
				Poll[5] = 'G';			// Poll

				CRCStuffAndSend(TNC, Poll, 6);
				TNC->InternalCmd = FALSE;
			}
		}
		return;
	}

	// Bad CRC - assume incomplete frame, and wait for rest. If it was a full bad frame, timeout and retry will recover link.

	return;
}

static BOOL NEAR WriteCommBlock(struct TNCINFO * TNC)
{
	BOOL        fWriteStat;
	DWORD       dwBytesWritten;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(TNC->hDevice, TNC->TXBuffer, TNC->TXLen, &dwBytesWritten, NULL);

	if ((!fWriteStat) || (TNC->TXLen != dwBytesWritten))
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);
	}

	TNC->Timeout = 20;				// 2 secs

	return TRUE;
}

VOID DEDPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	char Status[80];
	int Stream = 0;
	int nn;

	if (TNC->UpdateWL2K)
	{
		TNC->UpdateWL2KTimer--;

		if (TNC->UpdateWL2KTimer == 0)
		{
			TNC->UpdateWL2KTimer = 36000;		// Every Hour
			if (strcmp(TNC->ApplCmd, "RMS") == 0)
				if (CheckAppl(TNC, "RMS         ")) // Is RMS Available?
					SendReporttoWL2K(TNC);
		}
	}


	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && TNC->Streams[Stream].Attached == 0)
		{
			// New Attach

			// If Pactor, stop scanning and take out of listen mode.

			// Set call to connecting user's call

			int calllen=0;

			TNC->Streams[Stream].Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
			TNC->Streams[Stream].MyCall[calllen] = 0;

			TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = malloc(100);
			wsprintf(TNC->Streams[Stream].CmdSet, "I%s\r", TNC->Streams[Stream].MyCall);

			if (Stream == 0)
			{
				wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

				// Stop Scanner
		
				wsprintf(Status, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
				Rig_Command(-1, Status);
			}
		}
	}

	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		TNC->Retries--;

		if(TNC->Retries)
		{
			WriteCommBlock(TNC);	// Retransmit Block
			return;
		}

		// Retried out.

		if (TNC->HostMode == 0)
		{
			DoTermModeTimeout(TNC);
			return;
		}

		// Retried out in host mode - Clear any connection and reinit the TNC

		Debugprintf("PACTOR - Link to TNC Lost");
		TNC->TNCOK = FALSE;

		wsprintf(Status,"COM%d Open but TNC not responding", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		TNC->HostMode = 0;
		TNC->ReinitState = 0;
		
		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])		// Connected
			{
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
			}
		}
	}

	// We delay clearing busy for 3 secs

	if (TNC->Busy)
		if (TNC->Mode != 7)
			TNC->Busy--;

	if (TNC->BusyDelay)		// Waiting to send connect
	{
		// Still Busy?

		if (TNC->Busy == 0)
		{
			// No, so send

			int datalen = strlen(TNC->ConnectCmd);
			
			Poll[2] = TNC->Streams[0].DEDStream;		// Channel
			Poll[3] = 1;			// Command
			Poll[4] = datalen - 1;

			memcpy(&Poll[5], TNC->ConnectCmd, datalen);
		
		
			CRCStuffAndSend(TNC, Poll, datalen + 5);

			TNC->Streams[0].InternalCmd = TNC->Streams[0].Connected;
			TNC->Streams[0].Connecting = TRUE;

			wsprintf(Status, "%s Connecting to %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			free(TNC->ConnectCmd);
			TNC->BusyDelay = 0;
			return;
		}
		else
		{
			// Wait Longer

			TNC->BusyDelay--;

			if (TNC->BusyDelay == 0)
			{
				// Timed out - Send Error Response

				UINT * buffptr = GetBuff();

				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr[1]=39;
				memcpy(buffptr+2,"Sorry, Can't Connect - Channel is busy\r", 39);

				C_Q_ADD(&TNC->Streams[0].PACTORtoBPQ_Q, buffptr);
				free(TNC->ConnectCmd);

			}
		}
	}





	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0 && TNC->Streams[Stream].Attached)
		{
			UINT * buffptr;

			// Node has disconnected us. If connected to remote, disconnect.
			// Then set mycall back to Node or Port Call
		
			TNC->Streams[Stream].Attached = FALSE;

			if (Stream == 0)
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");

			// Queue it - it won't go till after the response to the D command

			TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = malloc(100);
			wsprintf(TNC->Streams[Stream].CmdSet, "I%s\r", TNC->NodeCall);

			//	if Pactor Channel, Start Scanner
				
			if (Stream == 0)
			{
				wsprintf(Status, "%d SCANSTART 15", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
				Rig_Command(-1, Status);
			}

			while(TNC->Streams[Stream].BPQtoPACTOR_Q)
			{
				buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
				ReleaseBuffer(buffptr);
			}

			while(TNC->Streams[Stream].PACTORtoBPQ_Q)
			{
				buffptr=Q_REM(&TNC->Streams[Stream].PACTORtoBPQ_Q);
				ReleaseBuffer(buffptr);
			}

			if (TNC->Streams[Stream].Connected)
			{
				// Node has disconnected
			
				UCHAR * Poll = TNC->TXBuffer;

				TNC->Streams[Stream].Connected = FALSE;
				TNC->TXBuffer[2] = TNC->Streams[Stream].DEDStream;
				TNC->TXBuffer[3] = 0x1;
				TNC->TXBuffer[4] = 0x0;
				TNC->TXBuffer[5] = 'D';

				CRCStuffAndSend(TNC, TNC->TXBuffer, 6);

				return;
			}
		}
	}

	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (!TNC->HostMode)
	{
		DoTNCReinit(TNC);
		return;
	}

	TNC->PollSent = FALSE;

	//If sending internal command list, send next element

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->Streams[Stream].CmdSet)
		{
			char * start, * end;
			int len;

			start = TNC->Streams[Stream].CmdSet;
		
			if (*(start) == 0)			// End of Script
			{
				free(TNC->Streams[Stream].CmdSave);
				TNC->Streams[Stream].CmdSet = NULL;
			}
			else
			{
				end = strchr(start, 13);
				len = ++end - start -1;	// exclude cr
				TNC->Streams[Stream].CmdSet = end;

				Poll[2] = TNC->Streams[Stream].DEDStream;		// Channel
				Poll[3] = 1;			// Command
				Poll[4] = len - 1;
				memcpy(&Poll[5], start, len);
		
				CRCStuffAndSend(TNC, Poll, len + 5);

				return;
			}
		}
	}
	// if Freq Change needed, check if ok to do it.
	
	if (TNC->TNCOK)
	{
		if (TNC->WantToChangeFreq)
		{
			Poll[2] = 31;			// Command
			Poll[3] = 1;			// Command
			Poll[4] = 2;			// Len -1
			Poll[5] = '%';
			Poll[6] = 'W';
			Poll[7] = '0';
		
			CRCStuffAndSend(TNC, Poll, 8);

			TNC->InternalCmd = TRUE;
			TNC->WantToChangeFreq = FALSE;

			return;
		}

		if (TNC->DontWantToChangeFreq)
		{
			Poll[2] = 31;			// Command
			Poll[3] = 1;			// Command
			Poll[4] = 2;			// Len -1
			Poll[5] = '%';
			Poll[6] = 'W';
			Poll[7] = '1';
		
			CRCStuffAndSend(TNC, Poll, 8);

			TNC->InternalCmd = TRUE;
			TNC->DontWantToChangeFreq = FALSE;
			TNC->OKToChangeFreq = FALSE;

			return;
		}

	}


	// Send Radio Command if avail

	if (TNC->TNCOK && TNC->BPQtoRadio_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&TNC->BPQtoRadio_Q);

		datalen=buffptr[1];

		Poll[2] = 253;		// Radio Channel
		Poll[3] = 0;		// Data?
		Poll[4] = datalen - 1;
	
		memcpy(&Poll[5], buffptr+2, datalen);
		
		ReleaseBuffer(buffptr);
		
		CRCStuffAndSend(TNC, Poll, datalen + 5);

//		Debugprintf("SCS Sending Rig Command");

		return;
	}

		// Check status Periodically
		
	if (TNC->TNCOK)
	{
		if (TNC->IntCmdDelay == 6)
		{
			Poll[2] = 254;			// Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = 'G';			// Extended Status Poll
			Poll[6] = '3';

			CRCStuffAndSend(TNC, Poll, 7);
						
			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay == 4)
		{
			Poll[2] = 31;			 // Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = '%';			// Bytes acked Status
			Poll[6] = 'T';

			CRCStuffAndSend(TNC, Poll, 7);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay <=0)
		{
			Poll[2] = 31;			// Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = '@';			// Buffer Status
			Poll[6] = 'B';

			CRCStuffAndSend(TNC, Poll, 7);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay = 20;	// Every 2 secs

			return;
		}
		else
			TNC->IntCmdDelay--;
	}




	// If busy, send status poll, send Data if avail

	// We need to start where we last left off, or a busy stream will lock out the others

	for (nn = 0; nn <= MaxStreams; nn++)
	{
		Stream = TNC->LastStream++;

		if (TNC->LastStream > MaxStreams) TNC->LastStream = 0;


		if (TNC->TNCOK && TNC->Streams[Stream].BPQtoPACTOR_Q)
		{
			int datalen;
			UINT * buffptr;
			
			buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);

			datalen=buffptr[1];

			Poll[2] = TNC->Streams[Stream].DEDStream;		// Channel

			if (TNC->Streams[Stream].Connected)
			{
				Poll[3] = 0;			// Data?
				TNC->Streams[Stream].BytesTXed += datalen;
			}
			else
			{
				// Command. Do some sanity checking and look for things to process locally

				char * Buffer = (char *)&buffptr[2];	// Data portion of frame

				Poll[3] = 1;			// Command
				datalen--;				// Exclude CR
				Buffer[datalen] = 0;	// Null Terminate
				_strupr(Buffer);

				if (_memicmp(Buffer, "D", 1) == 0)
				{
					TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
					ReleaseBuffer(buffptr);
					return;
				}

				if (strcmp(Buffer, "XXX") == 0)
				{
					CheckAppl(TNC, "RMS         "); // Is RMS Available?
					SendReporttoWL2K(TNC);

					return;
				}


				if (memcmp(Buffer, "RADIO ", 6) == 0)
				{
					wsprintf(&Buffer[40], "%d %s", TNC->PortRecord->PORTCONTROL.PORTNUMBER, &Buffer[6]);

					if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &Buffer[40]))
					{
						ReleaseBuffer(buffptr);
					}
					else
					{
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], &Buffer[40]);
						C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
					}
					return;
				}

				if (memcmp(Buffer, "MYLEVEL ", 8) == 0)
				{
					Switchmode(TNC, Buffer[8] - '0');

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Ok\r");		
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

					return;
				}

				if (_memicmp(Buffer, "OVERRIDEBUSY", 12) == 0)
				{
					TNC->OverrideBusy = TRUE;

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "SCS} OK\r");
					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

					return;
				}

				if (Buffer[0] == 'C' && datalen > 2)	// Connect
				{
					if (*(++Buffer) == ' ') Buffer++;		// Space isn't needed

					if ((memcmp(Buffer, "P1 ", 3) == 0) ||(memcmp(Buffer, "P2 ", 3) == 0))
					{
						// Port Selector for Packet Connect convert to 2:CALL

						Buffer[0] = Buffer[1];		
						Buffer[1] = ':';
						memmove(&Buffer[2], &Buffer[3], datalen--);
						Buffer += 2;
					}
					memcpy(TNC->Streams[Stream].RemoteCall, Buffer, 9);

					if (Stream == 0)
					{
						// See if Busy
				
						if (TNC->Busy)
						{
							// Channel Busy. Unless override set, wait

							if (TNC->OverrideBusy == 0)
							{
								// Save Command, and wait up to 10 secs

								Buffer -=2;
								TNC->ConnectCmd = _strdup(Buffer);
								TNC->BusyDelay = 100;		// 10 secs
								ReleaseBuffer(buffptr);
								return;
							}
						}

						TNC->OverrideBusy = FALSE;

						wsprintf(Status, "%s Connecting to %s", TNC->Streams[Stream].MyCall, TNC->Streams[Stream].RemoteCall);
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}

					TNC->Streams[Stream].Connecting = TRUE;
				}
			}

			Poll[4] = datalen - 1;
			memcpy(&Poll[5], buffptr+2, datalen);
		
			ReleaseBuffer(buffptr);
		
			CRCStuffAndSend(TNC, Poll, datalen + 5);

			TNC->Streams[Stream].InternalCmd = TNC->Streams[Stream].Connected;

			return;
		}

		// if frames outstanding, issue a poll

		if (TNC->Streams[Stream].FramesOutstanding)
		{
			Poll[2] = TNC->Streams[Stream].DEDStream;
			Poll[3] = 0x1;			// Command
			Poll[4] = 0;			// Len-1
			Poll[5] = 'L';			// Status

			CRCStuffAndSend(TNC, Poll, 6);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;
			return;
		}

	}

	TNC->PollSent = TRUE;

	// Use General Poll (255)

	Poll[2] = 255 ;			// Channel
	Poll[3] = 0x1;			// Command

	if (TNC->ReinitState == 3)
	{
		TNC->ReinitState = 0;
		Poll[3] = 0x41;
	}

	Poll[4] = 0;			// Len-1
	Poll[5] = 'G';			// Poll

	CRCStuffAndSend(TNC, Poll, 6);
	TNC->InternalCmd = FALSE;

	return;

}

VOID DoTNCReinit(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Just Starting - Send a TNC Mode Command to see if in Terminal or Host Mode

		char Status[80];
		
		TNC->TNCOK = FALSE;
		wsprintf(Status,"COM%d Initialising TNC", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		Poll[0] = 13;
		TNC->TXLen = 1;

		WriteCommBlock(TNC);
		TNC->Retries = 1;
	}

	if (TNC->ReinitState == 1)		// Forcing back to Term
		TNC->ReinitState = 0;

	if (TNC->ReinitState == 2)		// In Term State, Sending Initialisation Commands
	{
		char * start, * end;
		int len;

		start = TNC->InitPtr;
		
		if (*(start) == 0)			// End of Script
		{
			// Put into Host Mode

			TNC->TXBuffer[2] = 0;
			TNC->Toggle = 0;

			memcpy(Poll, "JHOST4\r", 7);

			TNC->TXLen = 7;
			WriteCommBlock(TNC);

			// Timeout will enter host mode

			TNC->Timeout = 1;
			TNC->Retries = 1;
			TNC->Toggle = 0;
			TNC->ReinitState = 3;	// Set toggle force bit

			return;
		}
		
		end = strchr(start, 13);
		len = ++end - start;
		TNC->InitPtr = end;
		memcpy(Poll, start, len);

		TNC->TXLen = len;
		WriteCommBlock(TNC);

		TNC->Retries = 2;
	}
}

VOID DoTermModeTimeout(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		//Checking if in Terminal Mode - Try to set back to Term Mode

		TNC->ReinitState = 1;
		ExitHost(TNC);
		TNC->Retries = 1;

		return;
	}

	if (TNC->ReinitState == 1)
	{
		// Forcing back to Term Mode

		TNC->ReinitState = 0;
		DoTNCReinit(TNC);				// See if worked
		return;
	}

	if (TNC->ReinitState == 3)
	{
		// Entering Host Mode
	
		// Assume ok

		TNC->HostMode = TRUE;
		TNC->IntCmdDelay = 10;

		return;
	}
}

BOOL CheckRXText(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)TNC->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return FALSE;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return FALSE;
	}
	
	TNC->RXLen += Length;

	Length = TNC->RXLen;

	TNC->RXBuffer[TNC->RXLen] = 0;

	if (strlen(TNC->RXBuffer) < TNC->RXLen)
		TNC->RXLen = 0;

	if (strstr(TNC->RXBuffer, "cmd: ") == 0)
		return 0;				// Wait for rest of frame

	// Complete Char Mode Frame

	TNC->RXLen = 0;		// Ready for next frame

	return TRUE;
					
}

BOOL CheckRXHost(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;
	unsigned short crc;
	char UnstuffBuffer[500];

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)TNC->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return FALSE;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return FALSE;
	}
	
	TNC->RXLen += Length;

	Length = TNC->RXLen;

	if (Length < 6)				// Minimum Frame Sise
		return FALSE;

	if (TNC->RXBuffer[2] == 170)
	{
		// Retransmit Request
	
		TNC->RXLen = 0;
		return FALSE;				// Ignore for now
	}

	// Can't unstuff into same buffer - fails if partial msg received, and we unstuff twice

	Length = Unstuff(&TNC->RXBuffer[2], &UnstuffBuffer[2], Length - 2);

	if (Length == -1)
	{
		// Unstuff returned an errors (170 not followed by 0)

		TNC->RXLen = 0;
		return FALSE;				// Ignore for now
	}

	crc = compute_crc(&UnstuffBuffer[2], Length);

	if (crc == 0xf0b8)		// Good CRC
	{
		TNC->RXLen = 0;		// Ready for next frame
		return TRUE;
	}

	// Bad CRC - assume incomplete frame, and wait for rest. If it was a full bad frame, timeout and retry will recover link.

	return FALSE;
}


//#include "Mmsystem.h"

Switchmode(struct TNCINFO * TNC, int Mode)
{
	int n;
	
	// Send Exit/Enter Host Sequence

		UCHAR * Poll = TNC->TXBuffer;

		EnterExit = TRUE;

		Poll[2] = 31;
		Poll[3] = 0x41;
		Poll[4] = 0x5;
		memcpy(&Poll[5], "JHOST0", 6);

		CRCStuffAndSend(TNC, Poll, 11);

	n = 0;

	while (CheckRXHost(TNC) == FALSE)
	{
		Sleep(5);
		n++;
		if (n > 100) break;
	}

	wsprintf(Poll, "MYL %d\r", Mode);

	TNC->TXLen = 6;
	WriteCommBlock(TNC);

	n = 0;

	while (CheckRXText(TNC) == FALSE)
	{
		Sleep(5);
		n++;
		if (n > 100) break;
	}

	memcpy(Poll, "JHOST4\r", 7);

	TNC->TXLen = 7;
	WriteCommBlock(TNC);

	// No response expected

	Sleep(10);

	Poll[2] = 0;
	TNC->Toggle = 0;
	Poll[3] = 0x41;
	Poll[4] = 0;			// Len-1
	Poll[5] = 'G';			// Poll

	CRCStuffAndSend(TNC, Poll, 6);
	TNC->InternalCmd = FALSE;
	TNC->Timeout = 5;		// 1/2 sec - In case missed

	EnterExit = FALSE;

	return 0;
}


VOID ExitHost(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	// Try to exit Host Mode

	TNC->TXBuffer[2] = 31;
	TNC->TXBuffer[3] = 0x41;
	TNC->TXBuffer[4] = 0x5;
	memcpy(&TNC->TXBuffer[5], "JHOST0", 6);

	CRCStuffAndSend(TNC, Poll, 11);

	return;
}

VOID CRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	unsigned short int crc;
	UCHAR StuffedMsg[500];
	int i, j;

    Msg[3] |= TNC->Toggle;
	TNC->Toggle ^= 0x80;

	crc = compute_crc(&Msg[2], Len-2);
	crc ^= 0xffff;

	Msg[Len++] = (crc&0xff);
	Msg[Len++] = (crc>>8);

	for (i = j = 2; i < Len; i++)
	{
		StuffedMsg[j++] = Msg[i];
		if (Msg[i] == 170)
		{
			StuffedMsg[j++] = 0;
		}
	}

	if (j != i)
	{
		Len = j;
		memcpy(Msg, StuffedMsg, j);
	}

	TNC->TXLen = Len;

	Msg[0] = 170;
	Msg[1] = 170;

	WriteCommBlock(TNC);

	TNC->Retries = 5;
}

int Unstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len)
{
	int i, j=0;

	for (i=0; i<len; i++, j++)
	{
		MsgOut[j] = MsgIn[i];
		if (MsgIn[i] == 170)			// Remove next byte
		{
			i++;
			if (MsgIn[i] != 0)
				if (i != len) return -1;
		}
	}

	return j;
}

VOID ProcessTermModeResponse(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Testing if in Term Mode. It is, so can now send Init Commands

		TNC->InitPtr = TNC->InitScript;
		TNC->ReinitState = 2;

		// Send Restart to make sure PTC is in a known state

		strcpy(Poll, "RESTART\r");

		TNC->TXLen = 8;
		WriteCommBlock(TNC);

		TNC->Timeout = 60;				// 6 secs

		return;
	}
	if (TNC->ReinitState == 2)
	{
		// Sending Init Commands

		DoTNCReinit(TNC);		// Send Next Command
		return;
	}
}

VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * Msg, int framelen)
{
	UINT * buffptr;
	char * Buffer;				// Data portion of frame
	char Status[80];
	int Stream = 0;

	// Any valid frame is an ACK

	TNC->Timeout = 0;

	if (TNC->TNCOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		TNC->TNCOK = TRUE;
		wsprintf(Status,"COM%d TNC link OK", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);
	}

	Stream = Msg[2];

	if (Stream == 31) Stream = 0;

	//	See if Poll Reply or Data
	
	if (Msg[3] == 0)
	{
		// Success - Nothing Follows

		if (Stream < 32)
			if (TNC->Streams[Stream].CmdSet)
				return;						// Response to Command Set

		if ((TNC->TXBuffer[3] & 1) == 0)	// Data
			return;

		// If the response to a Command, then we should convert to a text "Ok" for forward scripts, etc

		if (TNC->TXBuffer[5] == 'G')	// Poll
			return;

		if (TNC->TXBuffer[5] == 'C')	// Connect - reply we need is async
			return;

		if (TNC->TXBuffer[5] == 'L')	// Shouldnt happen!
			return;

		if (TNC->TXBuffer[5] == '%' && TNC->TXBuffer[6] == 'W')	// Scan Control - Response to W1
			if (TNC->InternalCmd)
				return;					// Just Ignore

		if (TNC->TXBuffer[5] == 'J')	// JHOST
		{
			if (TNC->TXBuffer[10] == '0')	// JHOST0
			{
				TNC->Timeout = 1;			// 
				return;
			}
		}	



		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} Ok\r");

		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[3] > 0 && Msg[3] < 6)
	{
		// Success with message - null terminated

		UCHAR * ptr;
		int len;

		if (Msg[2] == 0xff)			// General Poll Response
		{
			UCHAR * Poll = TNC->TXBuffer;
			UCHAR Chan = Msg[4] - 1;

			if (Chan == 255)
				return;				// Nothing doing

			if (Msg[5] != 0)
			{
				// More than one to poll - save the list of channels to poll

				strcpy(TNC->NexttoPoll, &Msg[5]);
			}

			// Poll the channel that had data

			Poll[2] = Chan;			// Channel
			Poll[3] = 0x1;			// Command

			if (Chan == 254)		// Status - Send Extended Status (G3)
			{
				Poll[4] = 1;			// Len-1
				Poll[5] = 'G';			// Extended Status Poll
				Poll[6] = '3';
			}
			else
			{
				Poll[4] = 0;			// Len-1
				Poll[5] = 'G';			// Poll
			}

			CRCStuffAndSend(TNC, Poll, Poll[4] + 6);
			TNC->InternalCmd = FALSE;

			return;
		}

		Buffer = &Msg[4];
		
		ptr = strchr(Buffer, 0);

		if (ptr == 0)
			return;

		*(ptr++) = 13;
		*(ptr) = 0;

		len = ptr - Buffer;

		if (len > 256)
			return;

		// See if we need to process locally (Response to our command, Incoming Call, Disconencted, etc

		if (Msg[3] < 3)						// 1 or 2 - Success or Fail
		{
			// See if a response to internal command

			if (TNC->InternalCmd)
			{
				// Process it

				char LastCmd = TNC->TXBuffer[5];

				if (LastCmd == 'L')		// Status
				{
					int s1, s2, s3, s4, s5, s6, num;

					num = sscanf(Buffer, "%d %d %d %d %d %d", &s1, &s2, &s3, &s4, &s5, &s6);
			
					TNC->Streams[Stream].FramesOutstanding = s3;
					return;
				}

				if (LastCmd == '@')		// @ Commands
				{
					if (TNC->TXBuffer[6]== 'B')	// Buffer Status
					{
						TNC->Buffers = atoi(Buffer);
						SetDlgItemText(TNC->hDlg, IDC_4, Buffer);
						return;
					}
				}

				if (LastCmd == '%')		// % Commands
				{
					char Status[80];
					
					if (TNC->TXBuffer[6]== 'T')	// TX count Status
					{
						wsprintf(Status, "RX %d TX %d ACKED %s", TNC->Streams[Stream].BytesRXed, TNC->Streams[Stream].BytesTXed, Buffer);
						SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
						return;
					}

					if (TNC->TXBuffer[6] == 'W')	// Scan Control
					{
						if (Msg[4] == '1')			// Ok to Change
							TNC->OKToChangeFreq = 1;
						else
							TNC->OKToChangeFreq = -1;
					}
				}
				return;
			}
		}

		if (Msg[3] == 3)					// Status
		{			
			if (strstr(Buffer, "DISCONNECTED"))
			{
				if ((TNC->Streams[Stream].Connecting | TNC->Streams[Stream].Connected) == 0)
					return;

				if (TNC->Streams[Stream].Connecting)
				{
					// Connect Failed
			
					buffptr = GetBuff();
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Failure with %s\r", TNC->Streams[Stream].RemoteCall);

					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
	
					TNC->Streams[Stream].Connecting = FALSE;
					TNC->Streams[Stream].Connected = FALSE;				// In case!
					TNC->Streams[Stream].FramesOutstanding = 0;

					if (Stream == 0)
					{
						wsprintf(Status, "In Use by %s", TNC->Streams[Stream].MyCall);
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}

					return;
				}
					
				// Must Have been connected - Release Session

				TNC->Streams[Stream].Connecting = FALSE;
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
				TNC->Streams[Stream].FramesOutstanding = 0;

				return;
			}

			if (strstr(Buffer, "CONNECTED"))
			{
				char * Call = strstr(Buffer, " to ");
				char * ptr;

				Call += 4;

				if (Call[1] == ':')
					Call +=2;

				ptr = strchr(Call, ' ');	
				if (ptr) *ptr = 0;

				ptr = strchr(Call, 13);	
				if (ptr) *ptr = 0;


				TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel
				TNC->Streams[Stream].Connecting = FALSE;

				TNC->Streams[Stream].BytesRXed = TNC->Streams[Stream].BytesTXed = 0;

				//	Stop Scanner

				if (Stream == 0)
				{
					wsprintf(Status, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
					Rig_Command(-1, Status);

					UpdateMH(TNC, Call, '+');
				}

				if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
				{
					// Incomming Connect

					struct TRANSPORTENTRY * Session;
					int Index = 0;

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
						return;					// Tables Full

					Buffer[len-1] = 0;

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

					if (Stream == 0)
					{
						if (TNC->RIG)
							wsprintf(Status, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->NodeCall, TNC->RIG->Valchar);
						else
							wsprintf(Status, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->NodeCall);
					
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					
						// If an autoconnect APPL is defined, send it

						if (TNC->ApplCmd)
						{
							buffptr = GetBuff();
							if (buffptr == 0) return;			// No buffers, so ignore

							buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "%s\r", TNC->ApplCmd);
							C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
							return;
						}
					}

					if (FULL_CTEXT)
					{
						int Len = CTEXTLEN, CTPaclen = 100;
						int Next = 0;

						while (Len > CTPaclen)		// CTEXT Paclen
						{
							buffptr = GetBuff();
							if (buffptr == 0) return;			// No buffers, so ignore

							buffptr[1] = CTPaclen;
							memcpy(&buffptr[2], &CTEXTMSG[Next], CTPaclen);
							C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);

							Next += CTPaclen;
							Len -= CTPaclen;
						}

						buffptr = GetBuff();
						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr[1] = Len;
						memcpy(&buffptr[2], &CTEXTMSG[Next], Len);
						C_Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
					}

					return;
				}
				else
				{
					// Connect Complete
			
					buffptr = GetBuff();
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Connected to %s\r", Call);;

					C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

					if (Stream == 0)
					{
						if (TNC->RIG)
							wsprintf(Status, "%s Connected to %s Outbound Freq %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall, TNC->RIG->Valchar);
						else
							wsprintf(Status, "%s Connected to %s Outbound", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);

					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}
	
					return;
				}
			}
			return;

		}

		if (Msg[3] == 4 || Msg[3] == 5)
		{
			// Monitor

			DoMonitor(TNC, &Msg[3], framelen - 3);
			return;

		}

		// 1, 2, 4, 5 - pass to Appl

		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} %s", &Msg[4]);

		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[3] == 6)
	{
		// Monitor Data With length)

		DoMonitor(TNC, &Msg[3], framelen - 3);
		return;
	}

	if (Msg[3] == 7)
	{
		char StatusMsg[60];
		int Status, ISS, Offset;
		
		if (Msg[2] == 0xfe)						// Status Poll Response
		{
			Status = Msg[5];
			
			TNC->Streams[0].PTCStatus0 = Status;
			TNC->Streams[0].PTCStatus1 = Msg[6] & 3;		// Pactor Level 1-3
			TNC->Streams[0].PTCStatus2 = Msg[7];			// Speed Level
			Offset = Msg[8];

			if (Offset > 128)
				Offset -= 128;

			TNC->Streams[0].PTCStatus3 = Offset; 

			TNC->Mode = (Status >> 4) & 7;
			ISS = Status & 8;
			Status &= 7;

			wsprintf(StatusMsg, "%x %x %x %x", TNC->Streams[0].PTCStatus0,
				TNC->Streams[0].PTCStatus1, TNC->Streams[0].PTCStatus2, TNC->Streams[0].PTCStatus3);
			
			if (ISS)
				SetDlgItemText(TNC->hDlg, IDC_1, "Sender");
			else
				SetDlgItemText(TNC->hDlg, IDC_1, "Receiver");

			SetDlgItemText(TNC->hDlg, IDC_2, status[Status]);
			SetDlgItemText(TNC->hDlg, IDC_3, ModeText[TNC->Mode]);

			if (TNC->Mode == 7)
				TNC->Busy = 30;				// 3 sec delay

			if (Offset == 128)		// Undefined
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset Unknown",
					PactorLevelText[TNC->Streams[0].PTCStatus1], Msg[7]);
			else
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset %d",
					PactorLevelText[TNC->Streams[0].PTCStatus1], Msg[7], Offset);

			SetDlgItemText(TNC->hDlg, IDC_PACTORLEVEL, StatusMsg);

			return;
		}
		
		if (Msg[2] == 253)						// Rig Port Response
		{
			int ret;

			// (Win98)
			//	return DeviceIoControl(
			//			VCOMInfo[port]->ComDev,(VCOMInfo[port]->Port << 16) | W98_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
			//else

			DeviceIoControl(
					TNC->VCOMHandle, IOCTL_SERIAL_SETDATA, &Msg[5], Msg[4] + 1, NULL, 0, &ret, NULL);

//			Debugprintf("SCS - Rig Control Response");


			return;
		}
		// Connected Data
		
		buffptr = GetBuff();

		if (buffptr == NULL) return;			// No buffers, so ignore
			
		buffptr[1] = Msg[4] + 1;				// Length
		TNC->Streams[Stream].BytesRXed += buffptr[1];
		memcpy(&buffptr[2], &Msg[5], buffptr[1]);
		C_Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}
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


BOOL OpenVirtualSerialPort(struct TNCINFO * TNC)
{
	char szPort[16];
	char buf[80];

/*#pragma warning( push )
#pragma warning( disable : 4996 )

   if (HIBYTE(_winver) < 5)
		Win98 = TRUE;

#pragma warning( pop ) 

   if (Win98)
	{
		VCOMInfo[bpqport]->ComDev = CreateFile( "\\\\.\\BPQVCOMM.VXD", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
	else
*/
	{
		wsprintf( szPort, "\\\\.\\BPQ%d", TNC->VCOMPort) ;

		TNC->VCOMHandle = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}		  
	if (TNC->VCOMHandle == (HANDLE) -1 )
	{
		wsprintf(buf,"Virtual COM Port %d could not be opened ", TNC->VCOMPort);
		WritetoConsole(buf);

		return (FALSE) ;
	}

	return (TRUE) ;
}

int ReadVCommBlock(struct TNCINFO * TNC, char * Block, int MaxLength)
{
	DWORD Length;
	UINT * buffptr;

	
	Length = 0;

//	if (Win98)
//		DeviceIoControl(
//			pVCOMInfo->ComDev, (pVCOMInfo->Port << 16) |W98_SERIAL_GETDATA,NULL,0,lpszBlock,nMaxLength, &dwLength,NULL);
//	else
	DeviceIoControl(
			TNC->VCOMHandle, IOCTL_SERIAL_GETDATA, NULL, 0, Block, MaxLength, &Length,NULL);

	if (Length == 0)
		return 0;

	if (Length == MaxLength)
		return 0;							// Probably garbage in buffer

	if (!TNC->TNCOK)
		return 0;

	// Queue for TNC

	buffptr = GetBuff();

	if (buffptr == 0) return (0);			// No buffers, so ignore

	buffptr[1] = Length;
		
	memcpy(buffptr+2, Block, Length);
		
	C_Q_ADD(&TNC->BPQtoRadio_Q, buffptr);

//	Debugprintf("SCS Rig Command Queued");

   return 0;

}
MESSAGE Monframe;		// I frames come in two parts.

#define TIMESTAMP 352

VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	// Convert to ax.25 form and pass to monitor

	UCHAR * ptr;

	if (Msg[0] == 6)		// Second part of I or UI
	{
		int len = Msg[1] +1;

		memcpy(Monframe.L2DATA, &Msg[2], len);
		Monframe.LENGTH += len;

		_asm {

		pushad

		mov edi, offset Monframe
		mov eax, BPQTRACE
		call eax

		popad
		}

		return;
	}

	Monframe.LENGTH = 23;				// Control Frame
	Monframe.PORT = TNC->PortRecord->PORTCONTROL.PORTNUMBER;


	ptr = strstr(Msg, "fm ");

	ConvToAX25(&ptr[3], Monframe.ORIGIN);

	UpdateMH(TNC, &ptr[3], ' ');

	ptr = strstr(ptr, "to ");

	ConvToAX25(&ptr[3], Monframe.DEST);

	Monframe.ORIGIN[6] |= 1;				// Set end of address

	ptr = strstr(ptr, "ctl ");

	if (memcmp(&ptr[4], "SABM", 4) == 0)
		Monframe.CTL = 0x2f;
	else  
	if (memcmp(&ptr[4], "DISC", 4) == 0)
		Monframe.CTL = 0x43;
	else 
	if (memcmp(&ptr[4], "UA", 2) == 0)
		Monframe.CTL = 0x63;
	else  
	if (memcmp(&ptr[4], "DM", 2) == 0)
		Monframe.CTL = 0x0f;
	else 
	if (memcmp(&ptr[4], "UI", 2) == 0)
		Monframe.CTL = 0x03;
	else 
	if (memcmp(&ptr[4], "RR", 2) == 0)
		Monframe.CTL = 0x1 | (ptr[6] << 5);
	else 
	if (memcmp(&ptr[4], "RNR", 3) == 0)
		Monframe.CTL = 0x5 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "REJ", 3) == 0)
		Monframe.CTL = 0x9 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "FRMR", 4) == 0)
		Monframe.CTL = 0x87;
	else  
	if (ptr[4] == 'I')
	{
		Monframe.CTL = (ptr[5] << 5) | (ptr[6] & 7) << 1 ;
	}

	if (strchr(&ptr[4], '+'))
	{
		Monframe.CTL |= 0x10;
		Monframe.DEST[6] |= 0x80;				// SET COMMAND
	}

	if (strchr(&ptr[4], '-'))	
	{
		Monframe.CTL |= 0x10;
		Monframe.ORIGIN[6] |= 0x80;				// SET COMMAND
	}

	if (Msg[0] == 5)							// More to come
	{
		ptr = strstr(ptr, "pid ");	
		sscanf(&ptr[3], "%x", &Monframe.PID);
		return;	
	}

	_asm {

	pushad

	mov edi, offset Monframe

	push	ecx
	push	edx
	
	push	0
	call	time

	add	esp,4
	
	pop		edx
	pop		ecx

	MOV	TIMESTAMP[EDI],EAX

	mov eax, BPQTRACE
	
	call eax

	popad

	}


	







// Message in EDI







}
//1:fm G8BPQ to KD6PGI-1 ctl I11^ pid F0
//fm KD6PGI-1 to G8BPQ ctl DISC+