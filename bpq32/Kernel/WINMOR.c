//
//	DLL to provide interface to allow G8BPQ switch to use WINMOR as a Port Driver 
//
//	Uses BPQ EXTERNAL interface
//


//  Version 1.0 January 2009 - Initial Version
//

// March 22 2010

// Send FAULTS to Monitor Window
// Force PROTOCOL = WINMOR/PACTOR (to simplifiy Config)

// July 2010
// Support up to 32 BPQ Ports
// Support up to 32 Applications

// Version 1.2.1.2 August 2010 

// Save Minimized State
// Handle new "BLOCKED by Busy channel" message from TNC

// Version 1.2.1.4 August 2010 

// Add Scan control of BW setting
// Reset TNC if stuck in Disconnecting
// Add option to send reports to WL2K
// Disconnect if appl not available

// Version 1.2.1.5 August 2010 

// Updates to WL2K Reporting
// Send Watchdog polls every minute and restart if no response.
// Don't connect if channel is busy (unless specifically overridden)

// Version 1.2.1.6 September 2010

// Add option to kill and restart TNC after each transfer
// Fix PTT operation after Node reconfig

// Version 1.2.2.1 September 2010

// Add option to get config from bpq32.cfg
// Merge with BPQ32.dll


#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <time.h>
#include <Psapi.h>

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#include "bpq32.h"
#include "winmor.h"

#include "RigControl.h"

#include "AsmStrucs.h"



#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

static int Socket_Data(int sock, int error, int eventcode);
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

KillTNC(struct TNCINFO * TNC);
RestartTNC(struct TNCINFO * TNC);
KillPopups(struct TNCINFO * TNC);
VOID MoveWindows(struct TNCINFO * TNC);
SendReporttoWL2K(struct TNCINFO * TNC);
BOOL CheckAppl(struct TNCINFO * TNC, char * Appl);

static char ClassName[]="WINMORSTATUS";
static char WindowTitle[] = "WINMOR";
static int RigControlRow = 180;


BOOL RestartAfterFailure = FALSE;

#define WINMOR
#define WL2K
#define NARROWMODE 21
#define WIDEMODE 22

//
//	Code Common to Pactor Modules

#include <commctrl.h>

#define BGCOLOUR RGB(236,233,216)

extern BOOL MinimizetoTray;
HANDLE hInstance;
HBRUSH bgBrush;


BOOL WINAPI Rig_Command();
struct RIGINFO * WINAPI RigConfig();
BOOL WINAPI Rig_Poll();
VOID WINAPI Rig_PTT();
struct RIGINFO * WINAPI Rig_GETPTTREC();
struct ScanEntry ** WINAPI CheckTimeBands();
VOID __cdecl Debugprintf(const char * format, ...);

extern UCHAR * BPQDirectory;

static BOOL Minimized;				// Start Minimized Flag


extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;
extern struct BPQVECSTRUC * BPQHOSTVECPTR;
extern char * PortConfig[33];

static RECT Rect;

struct RIGINFO DummyRig;		// Used if not using Rigcontrol

struct TNCINFO * TNCInfo[34] = {NULL};		// Records are Malloc'd

static BOOL ReadConfigFile(char * filename, int Port);
static int ProcessLine(char * buf, int Port);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1



static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	int i;
	struct TNCINFO * TNC;

	switch (message) { 

//		case WM_ACTIVATE:

//			SetFocus(hwndInput);
//			break;


	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WM_INITMENUPOPUP:

		for (i=1; i<33; i++)
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

		for (i=1; i<33; i++)
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

//		case IDC_TEST:

//			SendExitEnter(TNC);

//			break;

		default:

			return 0;

		}

	case WM_SIZING:

		for (i=1; i<33; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
		
			if (TNC->hDlg == hWnd)
				break;
		}

		MoveWindows(TNC);
			
		return TRUE;

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

BOOL CreatePactorWindow(struct TNCINFO * TNC)
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

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	TNC->hDlg = hDlg = CreateDialog(hInstance,ClassName,0,NULL);
	
	if (TNC->Hardware == H_WINMOR)
		wsprintf(Title, "%s Status - Port %d", WindowTitle, TNC->PortRecord->PORTCONTROL.PORTNUMBER);
	else
		wsprintf(Title,"%s Status - COM%d", WindowTitle, TNC->PortRecord->PORTCONTROL.IOBASE);

	SetWindowText(hDlg, Title);

	if (MinimizetoTray)
		AddTrayMenuItem(hDlg, Title);


	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
	
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &Minimized);

		if (TNC->Hardware == H_WINMOR)	
			retCode = RegQueryValueEx(hKey,"RestartAfterFailure",0,			
				(ULONG *)&Type,(UCHAR *)&RestartAfterFailure,(ULONG *)&Vallen);
	}
	Top = Rect.top;
	Left = Rect.left;

	GetWindowRect(hDlg, &Rect);	// Get the real size

	MoveWindow(hDlg, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	if (Minimized)
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


VOID UpdateMH(struct TNCINFO * TNC, UCHAR * Call, char Mode)
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

FILE *file;
char * Config;
char * ptr1, * ptr2;

BOOL ReadConfigFile(char * fn, int Port)
{
	char buf[256],errbuf[256];
	
	Config = PortConfig[Port];

	if (Config)
	{
		// Using config from bpq32.cfg

		ptr1 = Config;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcessLine(buf, Port))
			{
				WritetoConsole("Bad config record ");
				WritetoConsole(errbuf);
			}
		}
	}
	else
	{

	UCHAR Value[100];

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
		wsprintf(buf," Config file %s not found ",Value);
		WritetoConsole(buf);
		return (TRUE);			// Will give Port Not Defined error later
	}

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf, Port))
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

ProcessLine(char * buf, int Port)
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
				if (_memicmp(buf, "RIGCONTROL VCOM", 15) == 0)
	
					// SCS Virtual COM Channel

					TNC->VCOMPort = atoi(&buf[15]);		
#endif
				// RIGCONTROL COM60 19200 ICOM IC706 5e 4 14.103/U1w 14.112/u1 18.1/U1n 10.12/l1

				TNC->RigConfigMsg = _strdup(buf);
			}
			else
				
/*			if (_memicmp(buf, "PACKETCHANNELS", 14) == 0)
				TNC->PacketChannels = atoi(&buf[14]);
			else

			if (_memicmp(buf, "OLDMODE", 7) == 0)
				TNC->OldMode = TRUE;
			else
*/
			if ((_memicmp(buf, "CAPTURE", 7) == 0) || (_memicmp(buf, "PLAYBACK", 8) == 0))
			{}
			else

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

/*
			if (TNC->Hardware == H_HAL && _memicmp(buf, "TONES", 5) == 0)
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
			if (_memicmp(TNC->Hardware == H_HAL && buf, "DEFAULTMODE ", 12) == 0)
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
*/
			strcat (TNC->InitScript, buf);
		}
	}

	return (TRUE);	
}



// WL2K Reporting Code.

#ifdef WL2K

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

	struct ScanEntry ** Freqptr;
	char * Valchar;
	int dec, sign;
	char FreqString[80]="";
	int Mode;
	struct TimeScan ** TimeBands;	// List of TimeBands/Frequencies

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



#define VERSION_MAJOR         2
#define VERSION_MINOR         0



void ConnecttoWINMORThread(int port);
VOID ProcessDataSocketData(int port);
int ConnecttoWINMOR();
int ProcessReceivedData(struct TNCINFO * TNC);
VOID ReleaseTNC(struct TNCINFO * TNC);
VOID SuspendOtherPorts(struct TNCINFO * ThisTNC);
VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC);
VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len);

VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);

extern UCHAR NEXTID;
extern struct TRANSPORTENTRY * L4TABLE;
extern WORD MAXCIRCUITS;
extern UCHAR L4DEFAULTWINDOW;
extern WORD L4T1;
extern struct APPLCALLS APPLCALLTABLE[];
extern char APPLS;

extern struct BPQVECSTRUC * BPQHOSTVECPTR;

#define MAXBPQPORTS 16

static time_t ltime;

//LOGFONT LFTTYFONT ;

//HFONT hFont ;

static char * WINMORSignon[MAXBPQPORTS+1];			// Pointer to message for secure signin

static char * WINMORHostName[MAXBPQPORTS+1];		// WINMOR Host - may be dotted decimal or DNS Name

#pragma pack()

//SOCKET sock;

static SOCKADDR_IN sinx; 
static SOCKADDR_IN rxaddr;

static int addrlen=sizeof(sinx);

//static short WINMORPort=0;

//HANDLE hInstance;


VOID WINMORClose()
{
	struct TNCINFO * TNC;
	int i;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;

	Debugprintf("WINMORClose Called");

		for (i=1; i<33; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
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

				retCode = RegSetValueEx(hKey,"RestartAfterFailure",0,REG_DWORD,(BYTE *)&RestartAfterFailure, 4);

				RegCloseKey(hKey);
			}
	 	
			send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);
			Sleep(100);
			shutdown(TNC->WINMORDataSock, SD_BOTH);
			Sleep(100);
			shutdown(TNC->WINMORSock, SD_BOTH);
			Sleep(100);
			closesocket(TNC->WINMORDataSock);
			closesocket(TNC->WINMORSock);

			if (MinimizetoTray)	
				DeleteTrayMenuItem(TNC->hDlg);

		}
		return ;
}


static fd_set readfs;
static fd_set writefs;
static fd_set errorfs;
static struct timeval timeout;

VOID ChangeMYC(struct TNCINFO * TNC, char * Call)
{
	UCHAR TXMsg[100];
	int datalen;

	if (strcmp(Call, TNC->CurrentMYC) == 0)
		return;								// No Change

	strcpy(TNC->CurrentMYC, Call);

//	send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);

	datalen = wsprintf(TXMsg, "MYC %s\r\n", Call);
	send(TNC->WINMORSock,TXMsg, datalen, 0);

//	send(TNC->WINMORSock, "CODEC TRUE\r\n", 12, 0);
//	TNC->StartSent = TRUE;

	send(TNC->WINMORSock, "MYC\r\n", 5, 0);

}

static int ExtProc(int fn, int port,unsigned char * buff)
{
	int i,winerr;
	int datalen;
	UINT * buffptr;
	char txbuff[500];
	char Status[80];
	unsigned int bytes,txlen=0;
	char ErrMsg[255];
	int Param;

	struct TNCINFO * TNC = TNCInfo[port];

	if (TNC == NULL)
		return 0;							// Port not defined

	if (!TNC->RIG)
	{
		TNC->RIG = Rig_GETPTTREC(port);

		if (TNC->RIG == 0)
		{
			TNC->RIG = &DummyRig;		// Not using Rig control, so use Dummy
			TNC->PTTMode = 0;			// Can't use PTT if no rig
		}

		TNC->RIG->PTTMode = TNC->PTTMode;
	}	

	switch (fn)
	{
	case 1:				// poll


		if (TNC->BusyDelay)
		{
			// Still Busy?

			if ((TNC->Busy & CDBusy) == 0)
			{
				// No, so send

				send(TNC->WINMORSock, TNC->ConnectCmd, strlen(TNC->ConnectCmd), 0);
				TNC->Connecting = TRUE;

				memset(TNC->RemoteCall, 0, 10);
				memcpy(TNC->RemoteCall, &TNC->ConnectCmd[8], strlen(TNC->ConnectCmd)-10);

				wsprintf(Status, "%s Connecting to %s", TNC->MyCall, TNC->RemoteCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

				free(TNC->ConnectCmd);
				TNC->BusyDelay = 0;
			}
			else
			{
				// Wait Longer

				TNC->BusyDelay--;

				if (TNC->BusyDelay == 0)
				{
					// Timed out - Send Error Response

					UINT * buffptr = GetBuff();

					if (buffptr == 0) return (0);			// No buffers, so ignore

					buffptr[1]=39;
					memcpy(buffptr+2,"Sorry, Can't Connect - Channel is busy\r", 39);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
					free(TNC->ConnectCmd);

				}
			}
		}



		if (TNC->HeartBeat++ > 600)			// Every Minute
		{
			TNC->HeartBeat = 0;

			if (TNC->CONNECTED)

				// Probe link

				send(TNC->WINMORSock, "BUFFERS\r\n", 9, 0);
			
		}

		if (TNC->FECMode)
		{
			if (TNC->FECIDTimer++ > 6000)		// ID every 10 Mins
			{
				if (!TNC->Busy)
				{
					TNC->FECIDTimer = 0;
					send(TNC->WINMORSock, "SENDID 0\r\n", 10, 0);
				}
			}
			if (TNC->FECPending)	// Check if FEC Send needed
			{
				if (!TNC->Busy)
				{
					TNC->FECPending = 0;

					if (TNC->FEC1600)
						send(TNC->WINMORSock,"FECSEND 1600\r\n", 14, 0);
					else
						send(TNC->WINMORSock,"FECSEND 500\r\n", 13, 0);
				}
			}
		}

		if (TNC->NeedDisc)
		{
			TNC->NeedDisc--;

			if (TNC->NeedDisc == 0)
			{
				// Send the DISCONNECT

				send(TNC->WINMORSock, "DISCONNECT\r\n", 12, 0);
			}
		}

		if (TNC->DiscPending)
		{
			TNC->DiscPending--;

			if (TNC->DiscPending == 0)
			{
				// Too long in Disc Pending - Kill and Restart TNC

				if (TNC->WIMMORPID)
				{
					KillTNC(TNC);
					RestartTNC(TNC);
				}
			}
		}

		if (TNC->UpdateWL2K)
		{
			TNC->UpdateWL2KTimer--;

			if (TNC->UpdateWL2KTimer == 0)
			{
				TNC->UpdateWL2KTimer = 36000;		// Every Hour
				if (CheckAppl(TNC, "RMS         ")) // Is RMS Available?
					SendReporttoWL2K(TNC);
			}
		}


		if (TNC->TimeSinceLast++ > 700)			// Allow 10 secs for Keepalive
		{
			// Restart TNC
		
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, "WINMOR TNC"))
				{
					struct tm * tm;
					char Time[80];
				
					TNC->Restarts++;
					TNC->LastRestart = time(NULL);

					tm = gmtime(&TNC->LastRestart);	
				
					sprintf_s(Time, sizeof(Time),"%04d/%02d/%02d %02d:%02dZ",
						tm->tm_year +1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

					SetDlgItemText(TNC->hDlg, IDC_RESTARTTIME, Time);
					sprintf_s(Time, sizeof(Time),"%d", TNC->Restarts);
					SetDlgItemText(TNC->hDlg, IDC_RESTARTS, Time);

					KillTNC(TNC);
					RestartTNC(TNC);

					TNC->TimeSinceLast = 0;
				}
			}
		}

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Attached == 0)
		{
			// New Attach

			int calllen;
			char Msg[80];

			TNC->Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->MyCall);
			TNC->MyCall[calllen] = 0;

			// Stop Listening, and set MYCALL to user's call

			send(TNC->WINMORSock, "LISTEN FALSE\r\n", 14, 0);
			ChangeMYC(TNC, TNC->MyCall);

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

			wsprintf(Status, "In Use by %s", TNC->MyCall);
			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			// Stop Scanning

			wsprintf(Msg, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
	
			Rig_Command(-1, Msg);

		}

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0 && TNC->Attached)
		{
			// Node has disconnected - clear any connection

			send(TNC->WINMORSock,"DISCONNECT\r\n", 12, 0);

			TNC->Attached = FALSE;
			
			if (TNC->Connecting || TNC->Connected)
			{
				TNC->Connected = FALSE;
				TNC->Connecting = FALSE;
				TNC->Disconnecting = TRUE;

				TNC->DiscTimeout = 300;			// 30 Secs
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Disconnecting");
			}
			else
				ReleaseTNC(TNC);

			if (TNC->FECMode)
			{
				TNC->FECMode = FALSE;
				send(TNC->WINMORSock,"SENDID 0\r\n", 10, 0);
			}

		}

		if (TNC->ReportDISC)
		{
			TNC->ReportDISC = FALSE;
			buff[4] = 0;
			return -1;
		}

		if (TNC->DiscTimeout)
		{
			TNC->DiscTimeout--;
			if (TNC->DiscTimeout == 0)
				ReleaseTNC(TNC);
		}

			if (TNC->CONNECTED == FALSE && TNC->CONNECTING == FALSE)
			{
				//	See if time to reconnect
		
				time(&ltime);
				if (ltime - TNC->lasttime >9 )
				{
					ConnecttoWINMOR(port);
					TNC->lasttime = ltime;
				}
			}
		
			FD_ZERO(&readfs);
			
			if (TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&readfs);
			
			FD_ZERO(&writefs);

			if (TNC->BPQtoWINMOR_Q) FD_SET(TNC->WINMORDataSock,&writefs);	// Need notification of busy clearing

			FD_ZERO(&errorfs);
		
			if (TNC->CONNECTING || TNC->CONNECTED) FD_SET(TNC->WINMORDataSock,&errorfs);

			if (select(3,&readfs,&writefs,&errorfs,&timeout) > 0)
			{
				//	See what happened

				if (readfs.fd_count == 1)
					ProcessDataSocketData(port);			
				
				if (writefs.fd_count == 1)
				{
					// Write block has cleared. Send rest of packet

					buffptr=Q_REM(&TNC->BPQtoWINMOR_Q);
					txlen=buffptr[1];
					memcpy(txbuff,buffptr+2,txlen);
					bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
					ReleaseBuffer(buffptr);
				}
					
				if (errorfs.fd_count == 1)
				{
					i=wsprintf(ErrMsg, "WINMOR Data Connection lost for BPQ Port %d\r\n", port);
					WritetoConsole(ErrMsg);
					TNC->CONNECTING = FALSE;
					TNC->CONNECTED = FALSE;
					TNC->ReportDISC = TRUE;
				}
			}
		
		// See if any frames for this port

		if (TNC->WINMORtoBPQ_Q !=0)
		{
			buffptr=Q_REM(&TNC->WINMORtoBPQ_Q);

			datalen=buffptr[1];

			buff[4] = 0;						// Compatibility with Kam Driver
			buff[7] = 0xf0;
			memcpy(&buff[8],buffptr+2,datalen);	// Data goes to +7, but we have an extra byte
			datalen+=8;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);
		}

		return (0);

	case 2:				// send

		if (!TNC->CONNECTED)
		{
			// Send Error Response

			UINT * buffptr = GetBuff();

			if (buffptr == 0) return (0);			// No buffers, so ignore

			buffptr[1]=36;
			memcpy(buffptr+2,"No Connection to WINMOR Virtual TNC\r", 36);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
			return 0;		// Don't try if not connected
		}

		if (TNC->BPQtoWINMOR_Q)
			return 0;		// Socket is blocked - just drop packets
														// till it clears
		txlen=(buff[6]<<8) + buff[5]-8;	
		
		if (TNC->Connected)
			bytes=send(TNC->WINMORDataSock,(const char FAR *)&buff[8],txlen,0);
		else
		{
			if (_memicmp(&buff[8], "D\r", 2) == 0)
			{
				TNC->ReportDISC = TRUE;		// Tell Node
				return 0;
			}
	
			if (_memicmp(&buff[8], "XXX\r", 4) == 0)
			{
				CheckAppl(TNC, "RMS         "); // Is RMS Available?

				SendReporttoWL2K(TNC);

				return 0;
			}


			if (TNC->FECMode)
			{
				char Buffer[300];
				int len;

				// Send FEC Data

				buff[8 + txlen] = 0;
				len = wsprintf(Buffer, "%-9s: %s", TNC->MyCall, &buff[8]);

				send(TNC->WINMORDataSock, Buffer, len, 0);

				if (TNC->Busy)
				{
					TNC->FECPending = 1;
				}
				else
				{
					if (TNC->FEC1600)
						send(TNC->WINMORSock,"FECSEND 1600\r\n", 14, 0);
					else
						send(TNC->WINMORSock,"FECSEND 500\r\n", 13, 0);
				}
				return 0;
			}


			// See if Local command (eg RADIO)

			if (_memicmp(&buff[8], "RADIO ", 6) == 0)
			{
				wsprintf(&buff[8], "%d %s", TNC->PortRecord->PORTCONTROL.PORTNUMBER, &buff[14]);

				if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &buff[8]))
				{
				}
				else
				{
					UINT * buffptr = GetBuff();

					if (buffptr == 0) return 1;			// No buffers, so ignore

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], &buff[8]);
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
				return 1;
			}

			if (_memicmp(&buff[8], "OVERRIDEBUSY", 12) == 0)
			{
				UINT * buffptr = GetBuff();

				TNC->OverrideBusy = TRUE;

				if (buffptr)
				{
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} OK\r");
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}

				return 0;

			}


			if (_memicmp(&buff[8], "MAXCONREQ", 9) == 0)
			{
				if (buff[17] != 13)
				{
					// Limit connects

					int tries = atoi(&buff[18]);
					if (tries > 10) tries = 10;
					wsprintf(&buff[8], "MAXCONREQ %d\r\nMAXCONREQ\r\n", tries);

					send(TNC->WINMORSock,&buff[8],strlen(&buff[8]), 0);
					return 0;
				}
			}

			if ((_memicmp(&buff[8], "BW 500", 6) == 0) || (_memicmp(&buff[8], "BW 1600", 7) == 0))
			{
				// Generate a local response
				
				UINT * buffptr = GetBuff();

				if (buffptr)
				{
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} OK\r");
					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
			}

			if (_memicmp(&buff[8], "CODEC TRUE", 9) == 0)
				TNC->StartSent = TRUE;

			if (_memicmp(&buff[8], "D\r", 2) == 0)
			{
				TNC->ReportDISC = TRUE;		// Tell Node
				return 0;
			}

			if (_memicmp(&buff[8], "FEC\r", 4) == 0 || _memicmp(&buff[8], "FEC ", 4) == 0)
			{
				TNC->FECMode = TRUE;
				TNC->FECIDTimer = 0;
				send(TNC->WINMORSock,"FECRCV TRUE\r\nFECRCV\r\n", 21, 0);
		
				if (_memicmp(&buff[8], "FEC 1600", 8) == 0)
					TNC->FEC1600 = TRUE;
				else
					TNC->FEC1600 = FALSE;

				return 0;
			}

			// See if a Connect Command. If so, start codec and set Connecting

			if (toupper(buff[8]) == 'C' && buff[9] == ' ' && txlen > 2)	// Connect
			{
				char Connect[80] = "CONNECT ";

				memcpy(&Connect[8], &buff[10], txlen);
				txlen += 6;
				Connect[txlen++] = 0x0a;
				Connect[txlen] = 0;

				_strupr(Connect);

				// See if Busy
				
				if (TNC->Busy & CDBusy)
				{
					// Channel Busy. Unless override set, wait

					if (TNC->OverrideBusy == 0)
					{
						// Save Command, and wait up to 10 secs

						TNC->ConnectCmd = _strdup(Connect);
						TNC->BusyDelay = 100;		// 10 secs
						return 0;
					}
				}

				TNC->OverrideBusy = FALSE;

				bytes=send(TNC->WINMORSock, Connect, txlen, 0);
				TNC->Connecting = TRUE;

				memset(TNC->RemoteCall, 0, 10);
				memcpy(TNC->RemoteCall, &Connect[8], txlen-10);

				wsprintf(Status, "%s Connecting to %s", TNC->MyCall, TNC->RemoteCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
			}
			else
			{
				buff[8 + txlen++] = 0x0a;
				bytes=send(TNC->WINMORSock,(const char FAR *)&buff[8],txlen,0);
			}
		}
		if (bytes != txlen)
		{

			// WINMOR doesn't seem to recover from a blocked write. For now just reset
			
//			if (bytes == SOCKET_ERROR)
//			{
				winerr=WSAGetLastError();
				
				i=wsprintf(ErrMsg, "WINMOR Write Failed for port %d - error code = %d\r\n", port, winerr);
				WritetoConsole(ErrMsg);
					
	
//				if (winerr != WSAEWOULDBLOCK)
//				{
				closesocket(TNC->WINMORSock);
					
					TNC->CONNECTED = FALSE;

					return (0);
//				}
//				else
//				{
//					bytes=0;		// resent whole packet
//				}

//			}

			// Partial Send or WSAEWOULDBLOCK. Save data, and send once busy clears

			
			// Get a buffer
						
//			buffptr=GetBuff();

//			if (buffptr == 0)
//			{
				// No buffers, so can only break connection and try again

//				closesocket(WINMORSock[MasterPort[port]]);
					
//				CONNECTED[MasterPort[port]]=FALSE;

//				return (0);
//			}
	
//			buffptr[1]=txlen-bytes;			// Bytes still to send

//			memcpy(buffptr+2,&txbuff[bytes],txlen-bytes);

//			C_Q_ADD(&BPQtoWINMOR_Q[MasterPort[port]],buffptr);
	
//			return (0);
		}


		return (0);

	


	case 3:	
		
		// CHECK IF OK TO SEND (And check TNC Status)

		if (TNC->Attached == 0)
			return TNC->CONNECTED << 8 | 1;

		return (TNC->CONNECTED << 8 | TNC->Disconnecting << 15);		// OK
			
		break;

	case 4:				// reinit

		return (0);

	case 5:				// Close

		send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);
		Sleep(500);
		shutdown(TNC->WINMORDataSock, SD_BOTH);
		Sleep(500);
		shutdown(TNC->WINMORSock, SD_BOTH);
		Sleep(500);
		closesocket(TNC->WINMORDataSock);
		closesocket(TNC->WINMORSock);

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
			if (!TNC->ConnectPending)
				return 0;	// OK to Change

//			send(TNC->WINMORSock, "LISTEN FALSE\r\n", 14, 0);

			return TRUE;
		}

		if (Param == 2)		// Check  Permission
		{
			if (TNC->ConnectPending)
				return -1;	// Skip Interval

			return 1;		// OK to change
		}

		if (Param == 3)		// Release  Permission
		{
//			send(TNC->WINMORSock, "LISTEN TRUE\r\n", 13, 0);
			return 0;
		}

		if (Param == 4)		// Set Wide Mode
		{
			send(TNC->WINMORSock, "BW 1600\r\n", 9, 0);
			return 0;
		}

		if (Param == 5)		// Set Narrow Mode
		{
			send(TNC->WINMORSock, "BW 500\r\n", 8, 0);
			return 0;
		}

		return 0;
	}
	return 0;
}

VOID ReleaseTNC(struct TNCINFO * TNC)
{
	// Set mycall back to Node or Port Call, and Start Scanner

	UCHAR TXMsg[1000];

	TNC->Disconnecting = FALSE;
	TNC->DiscTimeout = 0;

	ChangeMYC(TNC, TNC->NodeCall);

	send(TNC->WINMORSock, "LISTEN TRUE\r\nMAXCONREQ 4\r\n", 26, 0);

	SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");

	//	Start Scanner
				
	wsprintf(TXMsg, "%d SCANSTART 15", TNC->PortRecord->PORTCONTROL.PORTNUMBER);

	Rig_Command(-1, TXMsg);

	ReleaseOtherPorts(TNC);

}

VOID SuspendOtherPorts(struct TNCINFO * ThisTNC)
{
	// Disable other TNCs in same Interlock Group
	
	struct TNCINFO * TNC;
	int i, Interlock = ThisTNC->Interlock;

	if (Interlock == 0)
		return;

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

//		if (Interlock == TNC->Interlock)	// Same Group	
//			send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);
	}
}

VOID ReleaseOtherPorts(struct TNCINFO * ThisTNC)
{
	// Enable other TNCs in same Interlock Group
	
	struct TNCINFO * TNC;
	int i, Interlock = ThisTNC->Interlock;

	if (Interlock == 0)
		return;

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;

		if (TNC == ThisTNC)
			continue;

//		if (Interlock == TNC->Interlock)	// Same Group	
//			send(TNC->WINMORSock, "CODEC TRUE\r\n", 12, 0);
	}
}


UINT WINAPI WinmorExtInit(EXTPORTDATA * PortEntry)
{
	int i, port;
	char Msg[255];
	char * ptr;
	HMENU hMenu;
	struct APPLCALLS * APPL;
	struct TNCINFO * TNC;
	char Aux[100] = "MYAUX ";
	char Appl[11];
	char * TempScript;

	//
	//	Will be called once for each WINMOR port 
	//
	//	The Socket to connect to is in IOBASE
	//
	
	port = PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile("BPQtoWINMOR.CFG", port);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(Msg," ** Error - no info in BPQtoWINMOR.cfg for this port");
		WritetoConsole(Msg);

		return (int) ExtProc;
	}

	TNC->Hardware = H_WINMOR;

	if (TNC->RigConfigMsg)
	{
		char * SaveRigConfig = _strdup(TNC->RigConfigMsg);

		TNC->RIG = RigConfig(TNC->RigConfigMsg, port);

		if (TNC->RIG)
			TNC->RIG->PTTMode = TNC->PTTMode;
		else
		{
			// Report Error

			i=wsprintf(Msg,"Invalid Rig Config %s", TNC->RigConfigMsg);
			WritetoConsole(Msg);

		}
		free(SaveRigConfig);
	}
	else
		TNC->RIG = NULL;		// In case restart


	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	TNC->Interlock = PortEntry->PORTCONTROL.PORTINTERLOCK;

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->MAXHOSTMODESESSIONS = 1;	
	PortEntry->SCANCAPABILITIES = SIMPLE;			// Scan Control - pending connect only


	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	// Put overridable ones on front, essential ones on end

	TempScript = malloc(1000);

	strcpy(TempScript, "LOG True\r\n");
	strcat(TempScript, "DebugLog True\r\n");
	strcat(TempScript, "CWID False\r\n");
	strcat(TempScript, "BW 1600\r\n");
	strcat(TempScript, "ROBUST False\r\n");
	strcat(TempScript, "MODE AUTO\r\n");

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Set MYCALL


	strcat(TNC->InitScript,"FECRCV True\r\n");
	strcat(TNC->InitScript,"AUTOBREAK True\r\n");

	wsprintf(Msg, "MYC %s\r\nCODEC TRUE\r\nLISTEN TRUE\r\nMYC\r\n", TNC->NodeCall);
	strcat(TNC->InitScript, Msg);
	strcat(TNC->InitScript,"PROCESSID\r\n");


	for (i = 0; i < 32; i++)
	{
		APPL=&APPLCALLTABLE[i];

		if (APPL->APPLCALL_TEXT[0] > ' ')
		{
			char * ptr;
			memcpy(Appl, APPL->APPLCALL_TEXT, 10);
			ptr=strchr(Appl, ' ');

			if (ptr)
			{
				*ptr++ = ',';
				*ptr = 0;
			}

			strcat(Aux, Appl);
		}
	}
	strcat(TNC->InitScript, Aux);
	strcat(TNC->InitScript,"\r\nMYAUX\r\n");


	strcpy(TNC->CurrentMYC, TNC->NodeCall);

	if (TNC->destaddr.sin_family == 0)
	{
		// not defined in config file, so use localhost and port from IOBASE

		TNC->destaddr.sin_family = AF_INET;
		TNC->destaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE);
		TNC->Datadestaddr.sin_family = AF_INET;
		TNC->Datadestaddr.sin_port = htons(PortEntry->PORTCONTROL.IOBASE+1);

		TNC->WINMORHostName=malloc(10);

		if (TNC->WINMORHostName != NULL) 
			strcpy(TNC->WINMORHostName,"127.0.0.1");

	}

	MinimizetoTray = GetMinimizetoTrayFlag();

	CreatePactorWindow(TNC);

	hMenu=CreateMenu();
	TNC->hPopMenu=CreatePopupMenu();
	SetMenu(TNC->hDlg,hMenu);

	AppendMenu(hMenu, MF_STRING + MF_POPUP, (UINT)TNC->hPopMenu,"Actions");

	AppendMenu(TNC->hPopMenu, MF_STRING, WINMOR_KILL, "Kill Winmor TNC");
	AppendMenu(TNC->hPopMenu, MF_STRING, WINMOR_RESTART, "Kill and Restart Winmor TNC");
	AppendMenu(TNC->hPopMenu, MF_STRING, WINMOR_RESTARTAFTERFAILURE, "Restart TNC after each Connection");
	
	CheckMenuItem(TNC->hPopMenu, WINMOR_RESTARTAFTERFAILURE, (RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(TNC->hDlg);	

	TNC->hMonitor = GetDlgItem(TNC->hDlg, IDC_WINMORTRACE); 
	
	MoveWindows(TNC);

	i=wsprintf(Msg,"WINMOR Host %s %d", TNC->WINMORHostName, htons(TNC->destaddr.sin_port));
	WritetoConsole(Msg);

	ConnecttoWINMOR(port);

	time(&TNC->lasttime);			// Get initial time value

	return ((int) ExtProc);
}

int ConnecttoWINMOR(int port)
{
	_beginthread(ConnecttoWINMORThread,0,port);

	return 0;
}

VOID ConnecttoWINMORThread(port)
{
	char Msg[255];
	int err,i;
	u_long param=1;
	BOOL bcopt=TRUE;
	struct hostent * HostEnt;
	struct TNCINFO * TNC = TNCInfo[port];

	Sleep(1000);		// Allow init to complete 

	TNC->destaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);
	TNC->Datadestaddr.sin_addr.s_addr = inet_addr(TNC->WINMORHostName);

	if (TNC->destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		//	Resolve name to address

		 HostEnt = gethostbyname (WINMORHostName[port]);
		 
		 if (!HostEnt) return;			// Resolve failed

		 memcpy(&TNC->destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
		 memcpy(&TNC->Datadestaddr.sin_addr.s_addr,HostEnt->h_addr,4);

	}

	closesocket(TNC->WINMORSock);
	closesocket(TNC->WINMORDataSock);

	TNC->WINMORSock=socket(AF_INET,SOCK_STREAM,0);
	TNC->WINMORDataSock=socket(AF_INET,SOCK_STREAM,0);

	if (TNC->WINMORSock == INVALID_SOCKET || TNC->WINMORDataSock == INVALID_SOCKET)
	{
		i=wsprintf(Msg, "Socket Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}
 
	setsockopt (TNC->WINMORDataSock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt, 4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(TNC->WINMORSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for WINMOR socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	if (bind(TNC->WINMORDataSock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
		i=wsprintf(Msg, "Bind Failed for WINMOR Data socket - error code = %d\r\n", WSAGetLastError());
		WritetoConsole(Msg);

  	 	return; 
	}

	TNC->CONNECTING = TRUE;

	if (connect(TNC->WINMORSock,(LPSOCKADDR) &TNC->destaddr,sizeof(TNC->destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		TNC->CONNECTED=TRUE;
	}
	else
	{
		if (TNC->Alerted == FALSE)
		{
			err=WSAGetLastError();
   			i=wsprintf(Msg, "Connect Failed for WINMOR socket - error code = %d\r\n", err);
			WritetoConsole(Msg);
			SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connection to TNC failed");

			TNC->Alerted = TRUE;
		}
		
		TNC->CONNECTING = FALSE;
		return;
	}

	if (connect(TNC->WINMORDataSock,(LPSOCKADDR) &TNC->Datadestaddr,sizeof(TNC->Datadestaddr)) == 0)
	{
		TNC->DATACONNECTED=TRUE;
		ioctlsocket (TNC->WINMORSock,FIONBIO,&param);
		ioctlsocket (TNC->WINMORDataSock,FIONBIO,&param);
		TNC->CONNECTING = FALSE;

		// Send INIT script

		send(TNC->WINMORSock, TNC->InitScript , strlen(TNC->InitScript), 0);
		TNC->Alerted = TRUE;

		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connected to WINMOR TNC");

		// Use Async Notification on the Control port to give a fast enough response for Serial PTT

		if (WSAAsyncSelect(TNC->WINMORSock, TNC->hDlg, WSA_DATA, 
				FD_READ | FD_WRITE | FD_OOB | FD_CLOSE) > 0)

		{
			sprintf(Msg, "WSAAsyncSelect failed Error %d\r\n", WSAGetLastError());
			WritetoConsole(Msg);
		}

		return;
	}

 	i=wsprintf(Msg, "Connect Failed for WINMOR Data socket Port %d - error code = %d\r\n", port, WSAGetLastError());
	WritetoConsole(Msg);
	closesocket(TNC->WINMORSock);
	closesocket(TNC->WINMORDataSock);

	return;
}

BOOL CALLBACK EnumTNCWindowsProc(HWND hwnd, LPARAM  lParam)
{
	char wtext[100];
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;

	GetWindowText(hwnd,wtext,99);

	if (memcmp(wtext,"WINMOR Sound Card TNC", 21) == 0)
	{
		GetWindowThreadProcessId(hwnd, &ProcessId);

		if (TNC->WIMMORPID == ProcessId)
		{
			 // Our Process

			wsprintf (wtext, "WINMOR Sound Card TNC - BPQ %s", TNC->PortRecord->PORTCONTROL.PORTDESCRIPTION);
			SetWindowText(hwnd, wtext);
			return FALSE;
		}
	}
	
	return (TRUE);
}

VOID ProcessResponse(struct TNCINFO * TNC, UCHAR * Buffer, int MsgLen)
{
	// Response on WINMOR control channel. Could be a reply to a command, or
	// an Async  Response

	UINT * buffptr;
	char Status[80];

	if (_memicmp(Buffer, "FAULT failure to Restart Sound card", 20) == 0)
	{
		// Force a restart

			send(TNC->WINMORSock, "CODEC FALSE\r\n", 13, 0);
			send(TNC->WINMORSock, "CODEC TRUE\r\n", 12, 0);
	}
	else
	{
		TNC->TimeSinceLast = 0;
		TNC->HeartBeat = 0;
	}

	Buffer[MsgLen - 2] = 0;			// Remove CRLF

	if (_memicmp(Buffer, "PTT T", 5) == 0)
	{
		TNC->Busy |= PTTBusy;
		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, TRUE);
		return;
	}
	if (_memicmp(Buffer, "PTT F", 5) == 0)
	{
		TNC->Busy &= ~PTTBusy;
		if (TNC->PTTMode)
			Rig_PTT(TNC->RIG, FALSE);
		return;
	}

	if (_memicmp(Buffer, "BUSY TRUE", 9) == 0)
	{	
		TNC->Busy |= CDBusy;
		SetDlgItemText(TNC->hDlg, IDC_CHANSTATE, "Busy");
		return;
	}

	if (_memicmp(Buffer, "BUSY FALSE", 10) == 0)
	{
		TNC->Busy &= ~CDBusy;
		SetDlgItemText(TNC->hDlg, IDC_CHANSTATE, "Clear");
		return;
	}

	if (_memicmp(Buffer, "TARGET", 6) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
	}

	if (_memicmp(Buffer, "OFFSET", 6) == 0)
	{
//		WritetoTrace(TNC, Buffer, MsgLen - 2);
//		memcpy(TNC->TargetCall, &Buffer[7], 10);
		return;
	}

	if (_memicmp(Buffer, "CONNECTED", 9) == 0)
	{
		char Call[11];
		char * ptr;
		struct APPLCALLS * APPL;
		char * ApplPtr = &APPLS;
		int App;
		char Appl[10];

		WritetoTrace(TNC, Buffer, MsgLen - 2);

		memcpy(Call, &Buffer[10], 10);

		ptr = strchr(Call, ' ');	
		if (ptr) *ptr = 0;

		UpdateMH(TNC, Call, '+');

		TNC->HadConnect = TRUE;

		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
		{
			// Incomming Connect

			struct TRANSPORTENTRY * Session;
			int Index = 0;

			// Stop Scanner

			char Msg[80];

			wsprintf(Msg, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
			Rig_Command(-1, Msg);

			// Stop other ports in same group

			SuspendOtherPorts(TNC);

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

			Buffer[MsgLen-1] = 0;

			memcpy(TNC->RemoteCall, Call, 9);	// Save Text Callsign 

			ConvToAX25(Call, Session->L4USER);

			ConvToAX25(&Buffer[10], Session->L4USER);
			ConvToAX25(GetNodeCall(), Session->L4MYCALL);

			Session->CIRCUITINDEX = Index;
			Session->CIRCUITID = NEXTID;
			NEXTID++;
			if (NEXTID == 0) NEXTID++;		// Keep non-zero

			TNC->PortRecord->ATTACHEDSESSIONS[0] = Session;
			TNC->Attached = TRUE;

			Session->L4TARGET = TNC->PortRecord;
	
			Session->L4CIRCUITTYPE = UPLINK+PACTOR;
			Session->L4WINDOW = L4DEFAULTWINDOW;
			Session->L4STATE = 5;
			Session->SESSIONT1 = L4T1;

			TNC->Connected = TRUE;			// Subsequent data to data channel

			if (TNC->RIG)
				wsprintf(Status, "%s Connected to %s Inbound Freq %s", TNC->RemoteCall, TNC->TargetCall, TNC->RIG->Valchar);
			else
				wsprintf(Status, "%s Connected to %s Inbound", TNC->RemoteCall, TNC->TargetCall);

			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			// See which application the connect is for

			for (App = 0; App < 32; App++)
			{
				APPL=&APPLCALLTABLE[App];
				memcpy(Appl, APPL->APPLCALL_TEXT, 10);
				ptr=strchr(Appl, ' ');

				if (ptr)
					*ptr = 0;
	
				if (_stricmp(TNC->TargetCall, Appl) == 0)
					break;
			}

			if (App < 32)
			{
				char AppName[13];

				memcpy(AppName, &ApplPtr[App * 21], 12);

				// Make sure app is available

				if (CheckAppl(TNC, AppName))
				{
					MsgLen = wsprintf(Buffer, "%s\r", AppName);
					buffptr = GetBuff();

					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1] = MsgLen;
					memcpy(buffptr+2, Buffer, MsgLen);

					C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
				}
				else
				{
					char Msg[] = "Application not available\r\n";
					
					// Send a Message, then a disconenct
					
					send(TNC->WINMORDataSock, Msg, strlen(Msg), 0);
					TNC->NeedDisc = 100;	// 10 secs
				}
			}
			return;
		}
		else
		{
			// Connect Complete

			char Reply[80];
			int ReplyLen;
			
			buffptr = GetBuff();

			if (buffptr == 0) return;			// No buffers, so ignore

			ReplyLen = wsprintf(Reply, "*** Connected to %s\r", &Buffer[10]);

			buffptr[1] = ReplyLen;
			memcpy(buffptr+2, Reply, ReplyLen);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			TNC->Connecting = FALSE;
			TNC->Connected = TRUE;			// Subsequent data to data channel

			if (TNC->RIG)
				wsprintf(Status, "%s Connected to %s Outbound Freq %s",  TNC->MyCall, TNC->RemoteCall, TNC->RIG->Valchar);
			else
				wsprintf(Status, "%s Connected to %s Outbound", TNC->MyCall, TNC->RemoteCall);

			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

			return;
		}
	}

	if (_memicmp(Buffer, "DISCONNECTED", 12) == 0)
	{
		if (TNC->FECMode)
			return;

		if (TNC->StartSent)
		{
			TNC->StartSent = FALSE;		// Disconnect reported following start codec
			return;
		}

		if (TNC->Connecting)
		{
			// Report Connect Failed, and drop back to command mode

			TNC->Connecting = FALSE;
			buffptr = GetBuff();

			if (buffptr == 0) return;			// No buffers, so ignore

			buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} Failure with %s\r", TNC->RemoteCall);

			C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

			return;
		}

		WritetoTrace(TNC, Buffer, MsgLen - 2);

		// Release Session

		TNC->Connecting = FALSE;
		TNC->Connected = FALSE;		// Back to Command Mode
		TNC->ReportDISC = TRUE;		// Tell Node

		if (TNC->Disconnecting)		// 
			ReleaseTNC(TNC);

		return;
	}

	if (_memicmp(Buffer, "MONCALL", 7) == 0)
	{
		// Add to MHEARD

		UpdateMH(TNC, &Buffer[8], '!');
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		
		if (!TNC->FECMode)
			return;							// If in FEC mode pass ID messages to user.
	}
		
	if (_memicmp(Buffer, "CMD", 3) == 0)
	{
		return;
	}

	if (_memicmp(Buffer, "MODE", 4) == 0)
	{
	//	Debugprintf("WINMOR RX: %s", Buffer);
		SetDlgItemText(TNC->hDlg, IDC_MODE, &Buffer[5]);
		return;
	}

	if (_memicmp(Buffer, "PENDING", 6) == 0)
		return;

	if (_memicmp(Buffer, "FAULT", 5) == 0)
		WritetoTrace(TNC, Buffer, MsgLen - 2);


	if (_memicmp(Buffer, "NEWSTATE", 8) == 0)
	{
		if (_memicmp(&Buffer[9], "CONNECTPENDING", 14) == 0)	// Save Pending state for scan control
			TNC->ConnectPending = TRUE;
		else
			TNC->ConnectPending = FALSE;

		if (_memicmp(&Buffer[9], "DISCONNECTING", 13) == 0)	// So we can timout stuck discpending
			TNC->DiscPending = 600;

		if (_memicmp(&Buffer[9], "DISCONNECTED", 12) == 0)	// Save Pending state for scan control
		{
			TNC->DiscPending = FALSE;
			if (RestartAfterFailure)
			{
				if (TNC->HadConnect)
				{
					TNC->HadConnect = FALSE;

					if (TNC->WIMMORPID)
					{
						KillTNC(TNC);
						RestartTNC(TNC);
					}
				}
			}
		}
		SetDlgItemText(TNC->hDlg, IDC_PROTOSTATE, &Buffer[9]);
		return;
	}

	if (_memicmp(Buffer, "BUFFERS", 7) == 0)
	{
	//	Debugprintf("WINMOR RX: %s", Buffer);
		SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, &Buffer[8]);
		return;
	}

	if (_memicmp(Buffer, "PROCESSID", 9) == 0)
	{
		HANDLE hProc;
		char ExeName[256] = "";

		TNC->WIMMORPID = atoi(&Buffer[10]);

		// Get the File Name in case we want to restart it.

		hProc =  OpenProcess(PROCESS_QUERY_INFORMATION |PROCESS_VM_READ, FALSE, TNC->WIMMORPID);

		if (hProc)
		{
			GetModuleFileNameEx(hProc, 0,  ExeName, 255);
			CloseHandle(hProc);
			TNC->ProgramPath = _strdup(ExeName);
		}

		// Set Window Title to reflect BPQ Port Description

		EnumWindows(EnumTNCWindowsProc, (LPARAM)TNC);
	}

	if ((_memicmp(Buffer, "FAULT Not from state FEC", 24) == 0) || (_memicmp(Buffer, "FAULT Blocked by Busy Lock", 24) == 0))
	{
		if (TNC->FECMode)
		{
			Sleep(1000);
			
			if (TNC->FEC1600)
				send(TNC->WINMORSock,"FECSEND 1600\r\n", 14, 0);
			else
				send(TNC->WINMORSock,"FECSEND 500\r\n", 13, 0);
			return;
		}
	}

	if (_memicmp(Buffer, "PLAYBACKDEVICES", 15) == 0)
	{
		TNC->PlaybackDevices = _strdup(&Buffer[16]);
	}
	// Others should be responses to commands

	if (_memicmp(Buffer, "BLOCKED", 6) == 0)
	{
		WritetoTrace(TNC, Buffer, MsgLen - 2);
		return;
	}

	buffptr = GetBuff();

	if (buffptr == 0) return;			// No buffers, so ignore

	buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "Winmor} %s\r", Buffer);

	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);
			
}

static int ProcessReceivedData(struct TNCINFO * TNC)
{
	char ErrMsg[255];

	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[2000];

	// May have several messages per packet, or message split over packets

	if (TNC->InputLen > 1000)	// Shouldnt have lines longer  than this on command connection
		TNC->InputLen=0;
				
	InputLen=recv(TNC->WINMORSock, &TNC->TCPBuffer[TNC->InputLen], 1000 - TNC->InputLen, 0);

	if (InputLen == 0 || InputLen == SOCKET_ERROR)
	{
		// Does this mean closed?
		
		if (!TNC->CONNECTING)
		{
			wsprintf(ErrMsg, "WINMOR Connection lost for BPQ Port %d\r\n", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
			WritetoConsole(ErrMsg);
		}
		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;
		TNC->ReportDISC = TRUE;

		return 0;					
	}

	TNC->InputLen += InputLen;

loop:
	
	ptr = memchr(TNC->TCPBuffer, '\n', TNC->InputLen);

	if (ptr)	//  CR in buffer
	{
		ptr2 = &TNC->TCPBuffer[TNC->InputLen];
		ptr++;				// Assume LF Follows CR

		if (ptr == ptr2)
		{
			// Usual Case - single meg in buffer
	
			ProcessResponse(TNC, TNC->TCPBuffer, TNC->InputLen);
			TNC->InputLen=0;
		}
		else
		{
			// buffer contains more that 1 message

			MsgLen = TNC->InputLen - (ptr2-ptr);

			memcpy(Buffer, TNC->TCPBuffer, MsgLen);

			ProcessResponse(TNC, Buffer, MsgLen);

			memmove(TNC->TCPBuffer, ptr, TNC->InputLen-MsgLen);

			TNC->InputLen -= MsgLen;
			goto loop;
		}
	}
	return 0;
}


VOID ProcessDataSocketData(int port)
{
	// Info on Data Socket - just packetize and send on
	
	struct TNCINFO * TNC = TNCInfo[port];
	int InputLen, PacLen = 236;
	UINT * buffptr;
	char * msg;
		
	TNC->TimeSinceLast = 0;

loop:
	buffptr = GetBuff();

	if (buffptr == NULL) return;			// No buffers, so ignore
			
	InputLen=recv(TNC->WINMORDataSock, (char *)&buffptr[2], PacLen, 0);

	if (InputLen == -1)
	{
		ReleaseBuffer(buffptr);
		return;
	}


	//Debugprintf("Winmor: RXD %d bytes", InputLen);

	if (InputLen == 0)
	{
		// Does this mean closed?
		
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connection to TNC lost");
	
		TNC->CONNECTING = FALSE;
		TNC->CONNECTED = FALSE;
		TNC->ReportDISC = TRUE;

		ReleaseBuffer(buffptr);
		return;					
	}

	msg = (char *)&buffptr[2];
	msg[InputLen] = 0;	
	
	WritetoTrace(TNC, msg, InputLen);
		
	if (TNC->FECMode)
	{	
		InputLen = strlen((char *)&buffptr[2]);

		if (msg[InputLen - 1] == 3)		// End of errored block
			msg[InputLen++] = 13;		// Add CR

	}
	buffptr[1] = InputLen;
	C_Q_ADD(&TNC->WINMORtoBPQ_Q, buffptr);

	goto loop;
}


int Socket_Data(int sock, int error, int eventcode)
{
	int i=0;
	struct TNCINFO * TNC;
	char ErrMsg[255];

	for (i=1; i<33; i++)
	{
		TNC = TNCInfo[i];
		if (TNC == NULL)
			continue;
			
		if (TNC->WINMORSock == sock)
			break;
	}
	if (!TNC)
		return 0;


	switch (eventcode)
	{
		case FD_READ:

			return 	ProcessReceivedData(TNC);			


		case FD_WRITE:

//			sockptr->TCPState = TCPConnected;
/*
// Write block has cleared. Send rest of packet

					buffptr=Q_REM(&TNC->BPQtoWINMOR_Q);
					txlen=buffptr[1];
					memcpy(txbuff,buffptr+2,txlen);
					bytes=send(TNC->WINMORSock,(const char FAR *)&txbuff,txlen,0);
					ReleaseBuffer(buffptr);
*/

			return 0;

		case FD_OOB:

			return 0;

		case FD_CLOSE:

			i=wsprintf(ErrMsg, "WINMOR Connection lost for BPQ Port %d\r\n", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
			WritetoConsole(ErrMsg);

			SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, "Connection to WINMOR TNC lost");
				
			TNC->CONNECTING = FALSE;
			TNC->CONNECTED = FALSE;
			TNC->Alerted = FALSE;

			if (TNC->Attached)
				TNC->ReportDISC = TRUE;

			return 0;

		}

	return 0;
}
/*
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Cmd = LOWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		struct TNCINFO * TNC = (struct TNCINFO * )lParam;
		char * ptr1, *ptr2;
		int ptr3 = 0;
		char Line[1000];
		int len;

		ptr1 = TNC->CaptureDevices;

		if (!ptr1)
			return 0;				// No Devices


		while (ptr2 = strchr(ptr1, ','))
		{
			len = ptr2 - ptr1;
			memcpy(&Line[ptr3], ptr1, len);
			ptr3 += len;
			Line[ptr3++] = '\r';
			Line[ptr3++] = '\n';

			ptr1 = ++ptr2;
		}
		Line[ptr3] = 0;
		strcat(Line, ptr1);
	
		SetDlgItemText(hDlg, IDC_CAPTURE, Line);

		ptr3 = 0;

		ptr1 = TNC->PlaybackDevices;
	
		if (!ptr1)
			return 0;				// No Devices


		while (ptr2 = strchr(ptr1, ','))
		{
			len = ptr2 - ptr1;
			memcpy(&Line[ptr3], ptr1, len);
			ptr3 += len;
			Line[ptr3++] = '\r';
			Line[ptr3++] = '\n';

			ptr1 = ++ptr2;
		}
		Line[ptr3] = 0;
		strcat(Line, ptr1);
	
		SetDlgItemText(hDlg, IDC_PLAYBACK, Line);

		SendDlgItemMessage(hDlg, IDC_PLAYBACK, EM_SETSEL, -1, 0);

//		KillTNC(TNC);

		return TRUE; 
	}

	case WM_SIZING:
	{
		return TRUE;
	}

	case WM_ACTIVATE:

//		SendDlgItemMessage(hDlg, IDC_MESSAGE, EM_SETSEL, -1, 0);

		break;


	case WM_COMMAND:


		if (Cmd == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}
*/
KillTNC(struct TNCINFO * TNC)
{
	HANDLE hProc;

	if (TNC->WIMMORPID == 0) return 0;

	hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TNC->WIMMORPID);

	if (hProc)
	{
		TerminateProcess(hProc, 0);
		CloseHandle(hProc);
	}

	TNC->WIMMORPID = 0;			// So we don't try again

	return 0;
}

RestartTNC(struct TNCINFO * TNC)
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	CreateProcess(TNC->ProgramPath, NULL, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);

	return 0;
}

VOID WritetoTrace(struct TNCINFO * TNC, char * Msg, int Len)
{
	int index = 0;
	UCHAR * ptr1 = Msg, * ptr2;
	UCHAR Line[1000];
	int LineLen, i;

lineloop:

	if (Len > 0)
	{
		//	copy text to control a line at a time	
					
		ptr2=memchr(ptr1,13,Len);

		if (ptr2)
		{
			ptr2++;
			LineLen = ptr2 - ptr1;
			Len -= LineLen;
			memcpy(Line, ptr1, LineLen);
			memcpy(&Line[LineLen - 1], "<cr>", 4);
			LineLen += 3;

			if ((*ptr2) == 10)
			{
				memcpy(&Line[LineLen], "<lf>", 4);
				LineLen += 4;
				ptr2++;
				Len --;
			}
			
			Line[LineLen] = 0;

			// If line contains any data above 7f, assume binary and dont display

			for (i = 0; i < LineLen; i++)
			{
				if (Line[i] > 127)
					goto Skip;
			}

			index=SendMessage(TNC->hMonitor, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Line);
		Skip:
			ptr1 = ptr2;

			goto lineloop;

		}

		for (i = 0; i < Len; i++)
		{
			if (ptr1[i] > 127)
				break;
		}

		if (i == Len)
			index=SendMessage(TNC->hMonitor, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) ptr1 );

	}

	if (index > 1200)
						
	do{

		index=index=SendMessage(TNC->hMonitor, LB_DELETESTRING, 0, 0);
			
	} while (index > 1000);

	index=SendMessage(TNC->hMonitor, LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

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

