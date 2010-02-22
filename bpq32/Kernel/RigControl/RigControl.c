//
//	DLL to inteface SCS TNC in Pactor Mode to BPQ32 switch 
//

// Dec 29 2009

//	Add Scan Control for SCS 


#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"
#include <commctrl.h>


//#include <process.h>
//#include <time.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#include "RigControl.h"
#include "ASMStrucs.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#define BPQWinMsg
#include "bpq32.h"

static char ClassName[] = "RIGCONTROL";
 
#define DllImport	__declspec(dllimport)
#define DllExport	__declspec(dllexport)
#define Dll	__declspec( dllexport )


DllImport UINT CRCTAB;
DllImport char * CTEXTMSG;
DllImport USHORT CTEXTLEN;
DllImport UINT FULL_CTEXT;

#undef BPQMsg

DllImport UINT BPQMsg;

RECT Rect;

int Row = -20;

UCHAR * BPQDirectory;

char CFGFN[MAX_PATH];

struct PORTINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct PORTINFO * PORT);
BOOL NEAR WriteCommBlock(struct PORTINFO * PORT);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct PORTINFO * PORT);
OpenCOMMPort(struct PORTINFO * PORT, int Port, int Speed);
VOID ICOMPoll(struct PORTINFO * PORT);
VOID ProcessFrame(struct PORTINFO * PORT, UCHAR * rxbuff, int len);
BOOL ReadConfigFile();
int ProcessLine(char * buf);
VOID ProcessHostFrame(struct PORTINFO * PORT, UCHAR * rxbuffer, int Len);
SendResponse(int Stream, char * Msg);
VOID CreateDisplay(struct PORTINFO * PORT);
VOID CreatePortLine(struct PORTINFO * PORT);
int CreateICOMLine(struct RIGINFO * RIG);
int CreateYaesuLine(struct RIGINFO * RIG);
VOID ProcessYaesuFrame(PORT);
VOID YaesuPoll(struct PORTINFO * PORT);
VOID ProcessYaesuCmdAck(struct PORTINFO * PORT);

VOID ProcessKenwoodFrame(PORT);
VOID KenwoodPoll(struct PORTINFO * PORT);

DllImport UCHAR NEXTID;
DllImport struct TRANSPORTENTRY * L4TABLE;
DllImport WORD MAXCIRCUITS;
DllImport UCHAR L4DEFAULTWINDOW;
DllImport WORD L4T1;

VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset); 
BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase);

INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Buffer Pool

#define NUMBEROFBUFFERS 20

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers

char Modes[8][6] = {"LSB",  "USB", "AM", "CW", "RTTY", "FM", "WFM", "????"};

//							0		1	  2		3	   4	5	6	7	8	9		0A  0B    0C    88

char YaesuModes[16][6] = {"LSB",  "USB", "CW", "CWR", "AM", "", "", "", "FM", "", "DIG", "", "PKT", "FMN", "????"};


char KenwoodModes[8][6] = {"????", "LSB",  "USB", "CW", "FM", "AM", "FSK", "????"};


char AuthPassword[100] = "";

char LastPassword[17];


//
//	Code Common to Pactor Modules

#define BGCOLOUR RGB(236,233,216)

BOOL MinimizetoTray = FALSE;
BOOL StartMinimized = FALSE;

HANDLE hInstance;
HBRUSH bgBrush;

RECT Rect;
HWND hDlg;						// Status Window Handle

HFONT hFont;
LOGFONT LFTTYFONT ;

int NumberofPorts = 0;

struct PORTINFO * PORTInfo[18] = {NULL};		// Records are Malloc'd

int ProcessLine(char * buf);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	
	switch (message) { 

//		case WM_ACTIVATE:

//			SetFocus(hwndInput);
//			break;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

		case RIG_CONFIG:

			DialogBox(hInstance, MAKEINTRESOURCE(RIGCONFIG), hDlg, ConfigDialogProc);
			break;

		default:

			return 0;

		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case  SC_MINIMIZE: 

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

		case WM_SIZING:
			
			return TRUE;


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

HMENU hPopMenu;


BOOL CreateRigWindow()
{
    WNDCLASS  wc;
	char Title[80];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Key[80];
	char Size[80];
	int Top, Left;
	HMENU hMenu;

	if (hDlg)
	{
		ShowWindow(hDlg, SW_SHOWNORMAL);
		SetForegroundWindow(hDlg);
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

	hDlg = CreateDialog(hInstance,ClassName,0,NULL);

/*
	hMenu=CreateMenu();
	TNC->hPopMenu=CreatePopupMenu();
	SetMenu(hDlg,hMenu);

	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)TNC->hPopMenu,"Actions");

	AppendMenu(TNC->hPopMenu,MF_STRING,RIG_CONFIG,"Configure");
	
	DrawMenuBar(hDlg);	
*/
	
	wsprintf(Title,"Rig Control");

	SetWindowText(hDlg, Title);

	if (MinimizetoTray)
		AddTrayMenuItem(hDlg, Title);


	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\RIGCONTROL");
	
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);
	}

	Top = Rect.top;
	Left = Rect.left;

	GetWindowRect(hDlg, &Rect);	// Get the real size

	MoveWindow(hDlg, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	GetWindowRect(hDlg, &Rect);	// Get Actual Position

	ShowWindow(hDlg, SW_SHOWNORMAL);

	return TRUE;
}


struct TimeScan * AllocateTimeRec(struct RIGINFO * RIG)
{
	struct TimeScan * Band = malloc(sizeof (struct TimeScan));
	
	RIG->TimeBands = realloc(RIG->TimeBands, (++RIG->NumberofBands+1)*4);
	RIG->TimeBands[RIG->NumberofBands] = Band;

	return Band;
}

char * CheckTimeBands(struct RIGINFO * RIG)
{
	int i = 0;
	time_t NOW = time(NULL) % 86400;
				
	// Find TimeBand

	while (i < RIG->NumberofBands)
	{
		if (RIG->TimeBands[i + 1]->Start > NOW)
		{
			break;
		}
		i++;
	}

	RIG->FreqPtr = RIG->TimeBands[i]->Scanlist;

	return RIG->FreqPtr;
}


FILE *file;

char errbuf[256];

BOOL ReadConfigFile()
{
	char buf[256];

	if ((file = fopen(CFGFN,"r")) == NULL)
	{
		return (TRUE);
	}

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
		if (!ProcessLine(buf))
		{
			WritetoConsole(" Bad config record ");
			WritetoConsole(errbuf);
		}			
	}

	fclose(file);
	return (TRUE);
}

GetLine(char * buf)
{
loop:
	if (fgets(buf, 255, file) == NULL)
		return 0;

	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	strcpy(errbuf,buf);			// save in case of error

	_strupr(buf);

	return 1;
}

ProcessLine(char * buf)
{
	char * ptr;
	int Radio;
	int len = 510;
	struct PORTINFO * PORT;
	struct RIGINFO * RIG;
	char * FreqPtr;
	char * Context;
	int Portno;
	struct PORTCONTROL * PortRecord;

NextPort:

	ptr = strtok_s(buf, " \t\n\r", &Context);

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment


	if (memcmp(ptr, "AUTH", 4) == 0)
	{
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		if (ptr == NULL) return FALSE;
		if (strlen(ptr) > 100) return FALSE;

		strcpy(AuthPassword, ptr);
		return TRUE;
	}

//#COM58 19200 ICOM

	if (memcmp(ptr, "COM", 3) == 0)
	{
		// New Port definition

		PORT = PORTInfo[NumberofPorts++] = malloc(sizeof(struct PORTINFO));
		memset(PORT, 0, sizeof(struct PORTINFO));

		PORT->IOBASE = atoi(&ptr[3]);
		ptr = strtok_s(NULL, " \t\n\r", &Context);
		if(ptr == NULL) return (FALSE);
		PORT->SPEED = atoi(ptr);

		ptr = strtok_s(NULL, " \t\n\r", &Context);
		if(ptr == NULL) return (FALSE);

		if (strcmp(ptr, "ICOM") == 0 || strcmp(ptr, "YAESU") == 0 
			|| strcmp(ptr, "KENWOOD") == 0 || strcmp(ptr, "PTTONLY") == 0)
		{
			// RADIO IC706 4E 5 14.103/U1 14.112/u1 18.1/U1 10.12/l1
			// Read RADIO Lines

			if (strcmp(ptr, "ICOM") == 0)
				PORT->PortType = ICOM;
			else
			if (strcmp(ptr, "YAESU") == 0)
				PORT->PortType = YAESU;
			else
			if (strcmp(ptr, "KENWOOD") == 0)
				PORT->PortType = KENWOOD;
			else
			if (strcmp(ptr, "PTTONLY") == 0)
				PORT->PortType = PTT;

			Radio = 0;

			while(TRUE)
			{
				if (GetLine(buf) == 0)
					return TRUE;

				if (memcmp(buf, "COM", 3) == 0)
					goto NextPort;

				RIG = &PORT->Rigs[Radio];

				ptr = strtok_s(buf, " \t\n\r", &Context);
				if (ptr == NULL) return (FALSE);

				ptr = strtok_s(NULL, " \t\n\r", &Context);
				if (ptr == NULL) return (FALSE);

				if (strlen(ptr) > 9) return FALSE;
				strcpy(RIG->RigName, ptr);

				ptr = strtok_s(NULL, " \t\n\r", &Context);
				if (ptr == NULL) return (FALSE);

				if 	(PORT->PortType == ICOM)
				{
					sscanf(ptr, "%x", &RIG->RigAddr);
					if (RIG->RigAddr == 0) return FALSE;

					ptr = strtok_s(NULL, " \t\n\r", &Context);
					if (ptr == NULL) return (FALSE);
				}

				Portno = atoi(ptr);
					
				RIG->BPQPort |=  (1 << Portno);

				// See if other ports in same scan/interlock group

				PortRecord = GetPortTableEntry(Portno);

				if (PortRecord->PORTINTERLOCK)
				{
					// Interlocked Ports

					int LockGroup = PortRecord->PORTINTERLOCK;
					
					PortRecord = PortRecord->PORTPOINTER;
					
					while (PortRecord)
					{
						if (LockGroup == PortRecord->PORTINTERLOCK)
							RIG->BPQPort |=  (1 << PortRecord->PORTNUMBER);

						PortRecord = PortRecord->PORTPOINTER;
					}
				}

				if (PORT->PortType == PTT)
				{
					Radio++;
					PORT->ConfiguredRigs++;
					return TRUE;
				}

				ptr = strtok_s(NULL, " \t\n\r", &Context);
				if (ptr == NULL) return (FALSE);

				RIG->ScanFreq = atoi(ptr);

				RIG->FreqText = _strdup(Context);

				ptr = strtok_s(NULL, " \t\n\r", &Context);

				// Frequency List

				if (ptr)
					if (ptr[0] == ';' || ptr[0] == '#')
						ptr = NULL;

				if (ptr != NULL)
				{
					struct TimeScan * Band = AllocateTimeRec(RIG);

					Band->Start = 0;
					FreqPtr = Band->Scanlist = RIG->FreqPtr = malloc(1000);
				}

				while(ptr)
				{
					int ModeNo;
					double Freq = 0.0;
					char FreqString[80]="";
					char * Valchar;
					char * Modeptr;

					int dec, sign;

					if (ptr[0] == ';' || ptr[0] == '#')
						break;

					Modeptr = strchr(ptr, '/');
					
					if (Modeptr)
						*Modeptr++ = 0;

					if (strchr(ptr, ':'))
					{
						// New TimeBand

						struct TimeScan * Band = AllocateTimeRec(RIG);

						*FreqPtr = 0;		// Terminate Last Band
						
						Band->Start = (atoi(ptr) * 3600) + (atoi(&ptr[3]) * 60);

						FreqPtr = Band->Scanlist = RIG->FreqPtr = malloc(1000);

						ptr = strtok_s(NULL, " \t\n\r", &Context);
											
						Modeptr = strchr(ptr, '/');
					
						if (Modeptr)
							*Modeptr++ = 0;

					}


					Freq = atof(ptr);

					if (Modeptr)
					{
						switch(PORT->PortType)
						{
						case ICOM:						
						
							for (ModeNo = 0; ModeNo < 8; ModeNo++)
							{
								if (Modes[ModeNo][0] == Modeptr[0])
									break;
							}
							break;

						case YAESU:						
						
							for (ModeNo = 0; ModeNo < 16; ModeNo++)
							{
								if (YaesuModes[ModeNo][0] == Modeptr[0])
									break;
							}
							break;

						case KENWOOD:						
						
							for (ModeNo = 0; ModeNo < 8; ModeNo++)
							{
								if (KenwoodModes[ModeNo][0] == Modeptr[0])
									break;
							}
							break;
						}

					}

					Freq = Freq * 1000000.0;

					Valchar = _fcvt(Freq, 0, &dec, &sign);

					if (dec == 9)	// 10-100
						wsprintf(FreqString, "%s", Valchar);
					else
					if (dec == 8)	// 10-100
						wsprintf(FreqString, "0%s", Valchar);		
					else
					if (dec == 7)	// 1-10
						wsprintf(FreqString, "00%s", Valchar);
					else
					if (dec == 6)	// 0.1 - 1
						wsprintf(FreqString, "000%s", Valchar);
					else
					if (dec == 5)	// 0.01 - .1
						wsprintf(FreqString, "000%s", Valchar);

					if 	(PORT->PortType == ICOM)
					{
					*(FreqPtr++) = 0xFE;
					*(FreqPtr++) = 0xFE;
					*(FreqPtr++) = RIG->RigAddr;
					*(FreqPtr++) = 0xE0;
					*(FreqPtr++) = 0x5;		// Set frequency command

					// Need to convert two chars to bcd digit
	
					*(FreqPtr++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
					*(FreqPtr++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
					*(FreqPtr++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
					*(FreqPtr++) = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
					*(FreqPtr++) = (FreqString[0] - 48);

					*(FreqPtr++) = 0xFD;

					if (Modeptr)
					{
						*(FreqPtr++) = 0xFE;
						*(FreqPtr++) = 0xFE;
						*(FreqPtr++) = RIG->RigAddr;
						*(FreqPtr++) = 0xE0;
						*(FreqPtr++) = 0x6;		// Set Mode
						*(FreqPtr++) = ModeNo;
						*(FreqPtr++) = Modeptr[1] - '0';
						*(FreqPtr++) = 0xFD;
					}
					else
					{
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
					}

					}
					else if	(PORT->PortType == YAESU)
					{	
						//Send Mode first - changing mode can change freq

						*(FreqPtr++) = ModeNo;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 0;
						*(FreqPtr++) = 7;

						*(FreqPtr++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
						*(FreqPtr++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
						*(FreqPtr++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
						*(FreqPtr++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
						*(FreqPtr++) = 1;
					
					}
					else if	(PORT->PortType == KENWOOD)
					{	
						FreqPtr += wsprintf(FreqPtr, "FA00%s;MD%d;", FreqString, ModeNo);
					}

					*(FreqPtr) = 0;

					RIG->ScanCounter = 20;

					ptr = strtok_s(NULL, " \t\n\r", &Context);
				}

				Radio++;
				PORT->ConfiguredRigs++;
				
			}
		}
		return FALSE; //Unknown type
	}

	return (FALSE);
	
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

char VersionString[100];

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];
	char Key[80];

	switch(ul_reason_being_called)
	{
	case DLL_PROCESS_ATTACH:

		hInstance = hInst;
		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		if (hDlg == NULL)
			return 1;

		ShowWindow(hDlg, SW_RESTORE);
		GetWindowRect(hDlg, &Rect);

		wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\RIGCONTROL");
	
		retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, 0, 0,
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

		if (retCode == ERROR_SUCCESS)
		{
			wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
			retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

			RegCloseKey(hKey);
		}
		return 1;
	}
}
DllExport VOID APIENTRY Rig_PTT(struct RIGINFO * RIG, BOOL PTTState)
{
	struct PORTINFO * PORT = RIG->PORT;

	if (PTTState)
		SetWindowText(RIG->hPTT, "T");
	else
		SetWindowText(RIG->hPTT, "");


	if (RIG->PTTMode & PTTCI_V)
	{
		UCHAR * Poll = PORT->TXBuffer;

		*(Poll++) = 0xFE;
		*(Poll++) = 0xFE;
		*(Poll++) = RIG->RigAddr;
		*(Poll++) = 0xE0;
		*(Poll++) = 0x1C;		// RIG STATE
		*(Poll++) = 0x00;		// PTT
		*(Poll++) = PTTState;	// OFF/ON

		*(Poll++) = 0xFD;
	
		PORT->TXLen = 8;		// First send the set Freq
		WriteCommBlock(PORT);

		PORT->Retries = 1;

		return;
	}

	if (RIG->PTTMode & PTTRTS)
		if (PTTState)
			EscapeCommFunction(PORT->hDevice,SETRTS);
		else
			EscapeCommFunction(PORT->hDevice,CLRRTS);

	if (RIG->PTTMode & PTTDTR)
		if (PTTState)
			EscapeCommFunction(PORT->hDevice,SETDTR);
		else
			EscapeCommFunction(PORT->hDevice,CLRDTR);
		
}

DllExport struct RIGINFO * APIENTRY Rig_GETPTTREC(int Port)
{
	struct RIGINFO * RIG;
	struct PORTINFO * PORT;
	int i, p;

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->BPQPort & (1 << Port))
				return RIG;
		}
	}

	return NULL;
}


DllExport int APIENTRY Rig_Command(int Session, char * Command)
{
	int n, Port, ModeNo, Filter;
	double Freq = 0.0;
	char FreqString[80]="", Mode[80];
	UINT * buffptr;
	UCHAR * Poll;
	char * 	Valchar ;
	int dec, sign;
	struct PORTINFO * PORT;
	int i, p;
	struct RIGINFO * RIG;
	struct TRANSPORTENTRY * L4 = L4TABLE;

	//	Only Allow RADIO from Secure Applications

	if (memcmp(Command, " AUTH ", 6) == 0)
	{
		if (AuthPassword[0] && (memcmp(LastPassword, &Command[6], 16) != 0))
		{
			if (CheckOneTimePassword(&Command[6], AuthPassword))
			{
				L4 += Session;
				L4->Secure_Session = 1;

				wsprintf(Command, "Ok\r");

				memcpy(LastPassword, &Command[6], 16);	// Save

				return FALSE;
			}
		}
		
		wsprintf(Command, "Sorry AUTH failed\r");
		return FALSE;
	}

	if (Session != -1)				// Used for internal Stop/Start
	{		
		L4 += Session;

		if (L4->Secure_Session == 0)
		{
			wsprintf(Command, "Sorry - you are not allowed to use this command\r");
			return FALSE;
		}
	}
	if (NumberofPorts == 0)
	{
		wsprintf(Command, "Sorry - Rig Control not configured\r");
		return FALSE;
	}

	n = sscanf(Command,"%d %s %s %d ", &Port, &FreqString, &Mode  , &Filter);

	// Look for the port 

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i< PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];

			if (RIG->BPQPort & (1 << Port))
				goto portok;
		}
	}



	wsprintf(Command, "Sorry - Port not found\r");
	return FALSE;

portok:

	if (RIG->RIGOK == 0)
	{
		wsprintf(Command, "Sorry - Radio not responding\r");
		return FALSE;
	}

	if (n > 1)
	{
		if (strcmp(FreqString, "SCANSTART") == 0)
		{
			if (RIG->NumberofBands)
			{
				RIG->ScanStopped &= (0xffffffff ^ (1 << Port));
				if (n > 2)
					RIG->ScanCounter = atoi(Mode) * 10;  //Start Delay
				else
					RIG->ScanCounter = 10;

				RIG->WaitingForPermission = FALSE;		// In case stuck	

				if (RIG->ScanStopped == 0)
					SetWindowText(RIG->hSCAN, "S");

				wsprintf(Command, "Ok\r");
			}
			else
				wsprintf(Command, "Sorry no Scan List defined for this port\r");

			return FALSE;
		}

		if (strcmp(FreqString, "SCANSTOP") == 0)
		{
			RIG->ScanStopped |= (1 << Port);
			SetWindowText(RIG->hSCAN, "");

			wsprintf(Command, "Ok\r");
			return FALSE;
		}
	}

	RIG->Session = Session;		// BPQ Stream

	Freq = atof(FreqString);

	if (Freq < 0.1)
	{
		strcpy(Command, "Sorry - Invalid Frequency\r");
		return FALSE;
	}

	Freq = Freq * 1000000.0;

	Valchar = _fcvt(Freq, 0, &dec, &sign);

	if (dec == 9)	// 10-100
		wsprintf(FreqString, "%s", Valchar);
	else
	if (dec == 8)	// 10-100
		wsprintf(FreqString, "0%s", Valchar);
	else
	if (dec == 7)	// 1-10
		wsprintf(FreqString, "00%s", Valchar);
	else
	if (dec == 6)	// 0 - 1
		wsprintf(FreqString, "000%s", Valchar);

	switch (PORT->PortType)
	{ 
	case ICOM:
	
		if (n != 4)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode Filter Width\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 8; ModeNo++)
		{
			if (_stricmp(Modes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo == 8)
		{
			wsprintf(Command, "Sorry - Invalid Mode\r");
			return FALSE;
		}

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		Poll = (UCHAR *)&buffptr[2];

		*(Poll++) = 0xFE;
		*(Poll++) = 0xFE;
		*(Poll++) = RIG->RigAddr;
		*(Poll++) = 0xE0;
		*(Poll++) = 0x5;		// Set frequency command

		// Need to convert two chars to bcd digit
	
		*(Poll++) = (FreqString[8] - 48) | ((FreqString[7] - 48) << 4);
		*(Poll++) = (FreqString[6] - 48) | ((FreqString[5] - 48) << 4);
		*(Poll++) = (FreqString[4] - 48) | ((FreqString[3] - 48) << 4);
		*(Poll++)  = (FreqString[2] - 48) | ((FreqString[1] - 48) << 4);
		*(Poll++) = (FreqString[0] - 48);
		*(Poll++) = 0xFD;

		*(Poll++) = 0xFE;
		*(Poll++) = 0xFE;
		*(Poll++) = RIG->RigAddr;
		*(Poll++) = 0xE0;
		*(Poll++) = 0x6;		// Set Mode
		*(Poll++) = ModeNo;
		*(Poll++) = Filter;
		*(Poll++) = 0xFD;

		buffptr[1] = 19;
		
		Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case YAESU:
			
		if (n != 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 15; ModeNo++)
		{
			if (_stricmp(YaesuModes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo == 15)
		{
			wsprintf(Command, "Sorry -Invalid Mode\r");
			return FALSE;
		}

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Send Mode then Freq - setting Mode seems to change frequency

		Poll = (UCHAR *)&buffptr[2];

		*(Poll++) = ModeNo;
		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 0;
		*(Poll++) = 7;		// Set Mode

		*(Poll++) = (FreqString[1] - 48) | ((FreqString[0] - 48) << 4);
		*(Poll++) = (FreqString[3] - 48) | ((FreqString[2] - 48) << 4);
		*(Poll++) = (FreqString[5] - 48) | ((FreqString[4] - 48) << 4);
		*(Poll++) = (FreqString[7] - 48) | ((FreqString[6] - 48) << 4);
		*(Poll++) = 1;		// Set Freq
					
		buffptr[1] = 10;
		
		Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	case KENWOOD:
			
		if (n != 3)
		{
			strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode\r");
			return FALSE;
		}

		for (ModeNo = 0; ModeNo < 8; ModeNo++)
		{
			if (_stricmp(KenwoodModes[ModeNo], Mode) == 0)
				break;
		}

		if (ModeNo == 8)
		{
			wsprintf(Command, "Sorry -Invalid Mode\r");
			return FALSE;
		}

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == 0)
		{
			wsprintf(Command, "Sorry - No Buffers available\r");
			return FALSE;
		}

		// Send Mode then Freq - setting Mode seems to change frequency

		Poll = (UCHAR *)&buffptr[2];

		buffptr[1] = wsprintf(Poll, "FA00%s;MD%d;", FreqString, ModeNo);
		
		Q_ADD(&RIG->BPQtoRADIO_Q, buffptr);

		return TRUE;

	}
	return TRUE;
}

int BittoInt(UINT BitMask)
{
	int i = 0;
	while ((BitMask & 1) == 0)
	{	
		BitMask >>= 1;
		i ++;
	}
	return i;
}


DllExport BOOL APIENTRY Rig_Init()
{
	HRSRC RH;
  	struct tagVS_FIXEDFILEINFO * HG;
	struct PORTINFO * PORT;
	int i, p;
	HMODULE HM;
	struct RIGINFO * RIG;

#ifdef _DEBUG 
	char isDebug[]="Debug Build";
#else
	char isDebug[]="";
#endif

	HM=GetModuleHandle("RigControl.dll");

	RH=FindResource(HM,MAKEINTRESOURCE(VS_VERSION_INFO),RT_VERSION);

	HG=LoadResource(HM,RH);

	(int)HG+=40;

	sprintf(VersionString,"%d.%d.%d.%d %s",
					HIWORD(HG->dwFileVersionMS),
					LOWORD(HG->dwFileVersionMS),
					HIWORD(HG->dwFileVersionLS),
					LOWORD(HG->dwFileVersionLS),
					isDebug);

	GetAPI();

	BPQDirectory=GetBPQDirectory();

	if (BPQDirectory[0] == 0)
	{
		strcpy(CFGFN,"RigControl.cfg");
	}
	else
	{
		strcpy(CFGFN,BPQDirectory);
		strcat(CFGFN,"\\");
		strcat(CFGFN,"RigControl.cfg");

	}
	
	MinimizetoTray=GetMinimizetoTrayFlag();
	if (GetStartMinimizedFlag) StartMinimized=GetStartMinimizedFlag();


	ReadConfigFile();

	if (NumberofPorts == 0)
	{
		return TRUE;
	}

	// Build buffer pool

	FREE_Q = 0;			// In case reloading;

	for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
	{	
		ReleaseBuffer(&BufferPool[100*i]);
	}

	CreateRigWindow();

	// setup default font information

   LFTTYFONT.lfHeight =			15;
   LFTTYFONT.lfWidth =          6 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        DEFAULT_CHARSET  ;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = VARIABLE_PITCH ;
   lstrcpy( LFTTYFONT.lfFaceName, "MS Sans Serif" ) ;

   hFont = CreateFontIndirect(&LFTTYFONT) ;


	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];
		
		CreateDisplay(PORT);

		OpenCOMMPort(PORT, PORT->IOBASE, PORT->SPEED);
	}

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];

		for (i=0; i < PORT->ConfiguredRigs; i++)
		{
			RIG = &PORT->Rigs[i];
			RIG->PortRecord = (struct _EXTPORTDATA *)GetPortTableEntry(BittoInt(RIG->BPQPort)); // BPQ32 port record for this port
			RIG->PORT = PORT;		// For PTT
			
			if (RIG->NumberofBands)
				CheckTimeBands(RIG);		// Set initial timeband
		}
	}
	MoveWindow(hDlg, Rect.left, Rect.top, Rect.right - Rect.left, Row + 100, TRUE);

	WritetoConsole("\nRig Control Enabled\n");

	return TRUE;
}


VOID CreateDisplay(struct PORTINFO * PORT)
{
	int i;
	char msg[80];
	struct RIGINFO * RIG;

	CreatePortLine(PORT);

	Row +=25;

		if (PORT->PortType == ICOM)
		{
			CreateWindow(WC_STATIC, "Radio      CI-V Addr",
				WS_CHILD | WS_VISIBLE, 5, Row, 150, 20, hDlg, NULL, hInstance, NULL);
			CreateWindow(WC_STATIC, "Freq                   Mode",
				WS_CHILD | WS_VISIBLE, 135, Row, 200, 20, hDlg, NULL, hInstance, NULL);

		}
		else
		if (PORT->PortType == PTT)
		{
			CreateWindow(WC_STATIC, "Radio",
				WS_CHILD | WS_VISIBLE, 5, Row, 60,20, hDlg, NULL, hInstance, NULL);
			CreateWindow(WC_STATIC, "PTT",
				WS_CHILD | WS_VISIBLE, 300, Row, 60,20, hDlg, NULL, hInstance, NULL);
		}
		else
		{
			CreateWindow(WC_STATIC, "Radio",
				WS_CHILD | WS_VISIBLE, 5, Row, 60, 20, hDlg, NULL, hInstance, NULL);
			CreateWindow(WC_STATIC, "Freq                   Mode",
				WS_CHILD | WS_VISIBLE, 135, Row, 200, 20, hDlg, NULL, hInstance, NULL);
		}
	Row += 5;

	for (i=0; i < PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];
		CreateICOMLine(RIG);
		if (PORT->PortType == ICOM)
		{
			wsprintf(msg,"%02X", PORT->Rigs[i].RigAddr);
			SetWindowText(RIG->hCAT, msg);
		}
		SetWindowText(RIG->hLabel, PORT->Rigs[i].RigName);
	}
}


DllExport BOOL APIENTRY Rig_Poll()
{
	int p;
	
	struct PORTINFO * PORT;

	for (p = 0; p < NumberofPorts; p++)
	{
		PORT = PORTInfo[p];
		if (PORT == NULL || PORT->hDevice == (HANDLE) -1)
			continue;

		CheckRX(PORT);

		switch (PORT->PortType)
		{ 
		case ICOM:
			
			ICOMPoll(PORT);
			break;

		case YAESU:
			
			YaesuPoll(PORT);
			break;

		case KENWOOD:
			
			KenwoodPoll(PORT);
			break;
		}
	}

	return TRUE;
}
 

VOID KISSCLOSE(int Port)
{ 
	DestroyTTYInfo(Port);
}

BOOL NEAR DestroyTTYInfo(int port)
{
   // force connection closed (if not already closed)

   CloseConnection(PORTInfo[port]);

   return TRUE;

} // end of DestroyTTYInfo()


BOOL CloseConnection(struct PORTINFO * PORT)
{
   // disable event notification and wait for thread
   // to halt

   SetCommMask(PORT->hDevice, 0);

   // drop DTR

   EscapeCommFunction(PORT->hDevice, CLRDTR);

   // purge any outstanding reads/writes and close device handle

   PurgeComm(PORT->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
   CloseHandle(PORT->hDevice);
 
   return TRUE;

} // end of CloseConnection()

OpenCOMMPort(struct PORTINFO * PORT, int Port, int Speed)
{
	char szPort[15];
	char buf[80];
	BOOL fRetVal;
	COMMTIMEOUTS CommTimeOuts;

	DCB	dcb;

	// load the COM prefix string and append port number
   
	wsprintf( szPort, "//./COM%d", Port) ;

	// open COMM device

	PORT->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (PORT->hDevice == (HANDLE) -1)
	{
		wsprintf(buf," COM%d Setup Failed - Error %d ", Port, GetLastError());
		WritetoConsole(buf);
		OutputDebugString(buf);
		SetWindowText(PORT->hStatus, &buf[1]);

		return (FALSE);
	}

	SetupComm(PORT->hDevice, 4096, 4096); // setup device buffers

	// purge any information in the buffer

	PurgeComm(PORT->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O
	  
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(PORT->hDevice, &CommTimeOuts);

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(PORT->hDevice, &dcb);

	 // setup hardware flow control

	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;

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

	fRetVal = SetCommState(PORT->hDevice, &dcb);

	if (PORT->PortType == !PTT)
	{
		EscapeCommFunction(PORT->hDevice,SETDTR);
		EscapeCommFunction(PORT->hDevice,SETRTS);
	}

	wsprintf(buf,"COM%d Open", Port);
	SetWindowText(PORT->hStatus, buf);

	return TRUE;
}

void CheckRX(struct PORTINFO * PORT)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;

	if (PORT->hDevice == (HANDLE) -1) 
		return;


	// only try to read number of bytes in queue 

	if (PORT->RXLen == 500)
		PORT->RXLen = 0;

	ClearCommError(PORT->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)PORT->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return;					// Nothing doing

	fReadStat = ReadFile(PORT->hDevice, &PORT->RXBuffer[PORT->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(PORT->hDevice, &dwErrorFlags, &ComStat);		
		return;
	}
	
	PORT->RXLen += Length;

	Length = PORT->RXLen;

	switch (PORT->PortType)
	{ 
	case ICOM:
	
		if (Length < 6)				// Minimum Frame Sise
			return;

		if (PORT->RXBuffer[Length-1] != 0xfd)
			return;	

		ProcessHostFrame(PORT, PORT->RXBuffer, Length);	// Could have multiple packets in buffer

		PORT->RXLen = 0;		// Ready for next frame	
		return;
	
	case YAESU:

		// Possible responses are a single byte ACK/NAK or a 5 byte info frame

		if (Length == 1 && PORT->CmdSent)
		{
			ProcessYaesuCmdAck(PORT);
			return;
		}
	
		if (Length < 5)			// Frame Sise
			return;

		if (Length > 5)			// Frame Sise
		{
			PORT->RXLen = 0;	// Corruption - reset and wait for retry	
			return;
		}

		ProcessYaesuFrame(PORT);

		PORT->RXLen = 0;		// Ready for next frame	
		return;

	case KENWOOD:
	
		if (Length < 3)				// Minimum Frame Sise
			return;

		if (Length > 50)			// Garbage
		{
			PORT->RXLen = 0;		// Ready for next frame	
			return;
		}

		if (PORT->RXBuffer[Length-1] != ';')
			return;	

		ProcessKenwoodFrame(PORT, PORT->RXBuffer, Length);	

		PORT->RXLen = 0;		// Ready for next frame	
		return;
	}
}

VOID ProcessHostFrame(struct PORTINFO * PORT, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;

	//	Split into Packets. By far the most likely is a single KISS frame
	//	so treat as special case
	
	FendPtr = memchr(rxbuffer, 0xfd, Len);
	
	if (FendPtr == &rxbuffer[Len-1])
	{
		ProcessFrame(PORT, rxbuffer, Len);
		return;
	}
		
	// Process the first Packet in the buffer

	NewLen =  FendPtr - rxbuffer +1;

	ProcessFrame(PORT, rxbuffer, NewLen);
	
	// Loop Back

	ProcessHostFrame(PORT, FendPtr+1, Len - NewLen);
	return;
}



BOOL NEAR WriteCommBlock(struct PORTINFO * PORT)
{
	BOOL        fWriteStat;
	DWORD       dwBytesWritten;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(PORT->hDevice, PORT->TXBuffer, PORT->TXLen, &dwBytesWritten, NULL);

	if ((!fWriteStat) || (PORT->TXLen != dwBytesWritten))
	{
		ClearCommError(PORT->hDevice, &dwErrorFlags, &ComStat);
	}

	PORT->Timeout = 100;		// 2 secs

	return TRUE;  
}

VOID ICOMPoll(struct PORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	int i;

	struct RIGINFO * RIG;

	for (i=0; i< PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];

		if (RIG->ScanStopped == 0)
			if (RIG->ScanCounter)
				RIG->ScanCounter--;
	}

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			WriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		RIG = &PORT->Rigs[PORT->CurrentRig];


		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
//		SetWindowText(RIG->hFREQ, "145.810000");
//		SetWindowText(RIG->hMODE, "RTTY/1");

		PORT->Rigs[PORT->CurrentRig].RIGOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	PORT->CurrentRig++;

	if (PORT->CurrentRig >= PORT->ConfiguredRigs)
		PORT->CurrentRig = 0;

	RIG = &PORT->Rigs[PORT->CurrentRig];

	if (RIG->NumberofBands && RIG->RIGOK && (RIG->ScanStopped == 0))
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			UCHAR * ptr;

			// Get Permission to change

			if (RIG->WaitingForPermission)
			{
				RIG->OKtoChange = RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 2);	// Get Ok Flag
	
				if (RIG->OKtoChange == 1)
					goto DoChange;

				if (RIG->OKtoChange == -1)
				{
					// Permission Refused. Wait Scan Interval and try again

					RIG->WaitingForPermission = FALSE;
					SetWindowText(RIG->hSCAN, "-");

					RIG->ScanCounter = 10 * RIG->ScanFreq; 
					goto ScanExit;
				}
				
				goto ScanExit;			// Haven't got reply yet.
			}
			else
			{
				if (RIG->PortRecord->PORT_EXT_ADDR)
					RIG->WaitingForPermission = RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 1);	// Request Perrmission
				
				// If it returns zero there is no need to wait.
				
				if (RIG->WaitingForPermission)
					goto ScanExit;
			}

		DoChange:	

			SetWindowText(RIG->hSCAN, "S");
			RIG->WaitingForPermission = FALSE;

			RIG->ScanCounter = 10 * RIG->ScanFreq; 
	
			ptr = RIG->FreqPtr;

			if (ptr == NULL)
				return;					// No Freqs
		
			if (*(ptr) == 0)			// End of list - reset to start
				ptr = CheckTimeBands(RIG);

			memcpy(PORT->TXBuffer, ptr, 19);

			RIG->FreqPtr += 19;
	
			PORT->TXLen = 11;
			WriteCommBlock(PORT);
			PORT->Retries = 2;
			PORT->AutoPoll = TRUE;

			return;
		}
	}

ScanExit:

	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&RIG->BPQtoRADIO_Q);

		datalen=buffptr[1];

		memcpy(Poll, buffptr+2, datalen);

		PORT->TXLen = 11;					// First send the set Freq
		WriteCommBlock(PORT);
		PORT->Retries = 2;

		ReleaseBuffer(buffptr);

		PORT->AutoPoll = FALSE;

		return;
	}

	if (RIG->RIGOK && (RIG->ScanStopped == 0))
		return;						// no point in reading freq if we are about to change it
		
	if (RIG->PollCounter)
	{
		RIG->PollCounter--;
		if (RIG->PollCounter)
			return;
	}

	if (RIG->RIGOK)
	{
		PORT->Retries = 2;
		RIG->PollCounter = 10 / PORT->ConfiguredRigs;			// Once Per Sec
	}
	else
	{
		PORT->Retries = 1;
		RIG->PollCounter = 100 / PORT->ConfiguredRigs;			// Slow Poll if down
	}

	PORT->AutoPoll = TRUE;

	// Read Frequency 

	Poll[0] = 0xFE;
	Poll[1] = 0xFE;
	Poll[2] = RIG->RigAddr;
	Poll[3] = 0xE0;
	Poll[4] = 0x3;		// Get frequency command
	Poll[5] = 0xFD;

	PORT->TXLen = 6;
	WriteCommBlock(PORT);
	return;
}


VOID ProcessFrame(struct PORTINFO * PORT, UCHAR * Msg, int framelen)
{
	char Status[80];
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG;
	int i;

	if (Msg[0] != 0xfe || Msg[1] != 0xfe)

		// Duff Packer - return

		return;	

	if (Msg[2] != 0xe0)
	{
		// Echo - Proves a CI-V interface is attached

		if (PORT->PORTOK == FALSE)
		{
			// Just come up
			char Status[80];
		
			PORT->PORTOK = TRUE;
			wsprintf(Status,"COM%d CI-V link OK", PORT->IOBASE);
			SetWindowText(PORT->hStatus, Status);
		}
		return;
	}

	for (i=0; i< PORT->ConfiguredRigs; i++)
	{
		RIG = &PORT->Rigs[i];
		if (Msg[3] == RIG->RigAddr)
			goto ok;
	}

	return;

ok:

	if (Msg[4] == 0xFB)
	{
		// Accept

		// if it was the set freq, send the set mode

		if (PORT->TXBuffer[4] == 5)
		{
			if (PORT->TXBuffer[11])
			{
				memcpy(PORT->TXBuffer, &PORT->TXBuffer[11], 8);
				PORT->TXLen = 8;
				WriteCommBlock(PORT);
				PORT->Retries = 2;
			}
			else
			{
				if (!PORT->AutoPoll)
					SendResponse(RIG->Session, "Frequency Set OK");
			}

			return;
		}

		if (PORT->TXBuffer[4] == 6)
		{
			// Set Mode Response - if scanning read freq, else return OK to user

			if (RIG->ScanStopped == 0)
			{
				RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 3);	// Release Perrmission

				Poll[0] = 0xFE;
				Poll[1] = 0xFE;
				Poll[2] = RIG->RigAddr;
				Poll[3] = 0xE0;
				Poll[4] = 0x3;		// Get frequency command
				Poll[5] = 0xFD;

				PORT->TXLen = 6;
				WriteCommBlock(PORT);
				PORT->Retries = 2;
				return;
			}

			else
				if (!PORT->AutoPoll)
					SendResponse(RIG->Session, "Frequency and Mode Set OK");
		}

		PORT->Timeout = 0;
		return;
	}

	if (Msg[4] == 0xFA)
	{
		// Reject
		PORT->Timeout = 0;
		if (PORT->TXBuffer[4] == 5)
			SendResponse(RIG->Session, "Sorry - Set Frequency Command Rejected");
		else
		if (PORT->TXBuffer[4] == 6)
			SendResponse(RIG->Session, "Sorry - Set Mode Command Rejected");

		return;
	}

	if (Msg[4] == PORT->TXBuffer[4])
	{
		// Response to our command

		// Any valid frame is an ACK

		RIG->RIGOK = TRUE;
		PORT->Timeout = 0;
	}
	else 
		return;		// What does this mean??


	if (PORT->PORTOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		PORT->PORTOK = TRUE;
		wsprintf(Status,"COM%d PORT link OK", PORT->IOBASE);
		SetWindowText(PORT->hStatus, Status);
	}

	if (Msg[4] == 3)
	{
		// Rig Frequency
		int n, j, Freq = 0, decdigit;
		double FreqF;
		char Valchar[_CVTBUFSIZE];

		for (j = 9; j > 4; j--)
		{
			n = Msg[j];
			decdigit = (n >> 4);
			decdigit *= 10;
			decdigit += n & 0xf;
			Freq = (Freq *100 ) + decdigit;
		}

		FreqF = Freq / 1000000.0;

//		Valchar = _fcvt(FreqF, 6, &dec, &sign);
		_gcvt(FreqF, 9, Valchar);
 


		wsprintf(Status,"%s", Valchar);
		SetWindowText(RIG->hFREQ, Status);

		// Now get Mode

			Poll[0] = 0xFE;
			Poll[1] = 0xFE;
			Poll[2] = RIG->RigAddr;
			Poll[3] = 0xE0;
			Poll[4] = 0x4;		// Get Mode
			Poll[5] = 0xFD;

		PORT->TXLen = 6;
		WriteCommBlock(PORT);
		PORT->Retries = 2;
		return;
	}
	if (Msg[4] == 4)
	{
		// Mode

		unsigned int Mode = Msg[5];

		if (Mode > 7) Mode = 7;

		wsprintf(Status,"%s/%d", Modes[Mode], Msg[6]);
		SetWindowText(RIG->hMODE, Status);
	}
}

VOID * APIENTRY GetBuff();

SendResponse(int Session, char * Msg)
{
	PMESSAGE Buffer = GetBuff();
	struct BPQVECSTRUC * VEC;
	struct TRANSPORTENTRY * L4 = L4TABLE;

	if (Session == -1)
		return 0;

	L4 += Session;

	Buffer->LENGTH = wsprintf((LPSTR)Buffer, "       \xf0%s\r", Msg);

	VEC = L4->L4TARGET;

	Q_ADD((UINT *)&L4->L4TX_Q, (UINT *)Buffer);

	PostMessage(VEC->HOSTHANDLE, BPQMsg, VEC->HOSTSTREAM, 2);  

	return 0;
}

VOID CreatePortLine(struct PORTINFO * PORT)
{
	Row +=25;
	
	PORT->hStatus = CreateWindow(WC_STATIC , "", WS_CHILD | WS_VISIBLE,
                5, Row, 290 ,20, hDlg, NULL, hInstance, NULL);
}


int CreateICOMLine(struct RIGINFO * RIG)
{
	Row +=20;
	
	RIG->hLabel = CreateWindow(WC_STATIC , "", WS_CHILD | WS_VISIBLE,
                 5, Row, 80,20, hDlg, NULL, hInstance, NULL);
	
	RIG->hCAT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 90, Row, 40,20, hDlg, NULL, hInstance, NULL);
	
	RIG->hFREQ = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 135, Row, 100,20, hDlg, NULL, hInstance, NULL);
	
	RIG->hMODE = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 240, Row, 60,20, hDlg, NULL, hInstance, NULL);
	
	RIG->hSCAN = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 300, Row, 20,20, hDlg, NULL, hInstance, NULL);

	RIG->hPTT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 320, Row, 20,20, hDlg, NULL, hInstance, NULL);

//SendMessage(RIG->hLabel, WM_SETFONT,(WPARAM) hFont, 0);
//	SendMessage(RIG->hCAT, WM_SETFONT,(WPARAM) hFont, 0);
//	SendMessage(RIG->hFREQ, WM_SETFONT,(WPARAM) hFont, 0);
//	SendMessage(RIG->hMODE, WM_SETFONT,(WPARAM) hFont, 0);



	return 0;
}
VOID ProcessYaesuCmdAck(struct PORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	PORT->Timeout = 0;
	PORT->RXLen = 0;					// Ready for next frame	

	if (PORT->CmdSent == 1)						// Set Freq
	{
		RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 3);	// Release Perrmission

		if (Msg[0])
		{
			// I think nonzero is a Reject

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Sorry - Set Frequency Rejected");

			return;
		}
		else
		{
			if (RIG->ScanStopped == 0)
			{
				// Send a Get Freq - We Don't Poll when scanning

				Poll[0] = Poll[1] = Poll[2] = Poll[3] = 0;
				Poll[4] = 0x3;		// Get frequency amd mode command

				WriteCommBlock(PORT);
				PORT->Retries = 2;
				PORT->CmdSent = 0;
			}
			else

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Mode and Frequency Set OK");

			return;
		}
	}

	if (PORT->CmdSent == 7)						// Set Mode
	{
		if (Msg[0])
		{
			// I think nonzero is a Reject

			if (!PORT->AutoPoll)
				SendResponse(RIG->Session, "Sorry - Set Mode Rejected");

			return;
		}
		else
		{
			// Send the Frequency
			
			memcpy(Poll, &Poll[5], 5);
			WriteCommBlock(PORT);
			PORT->CmdSent = Poll[4];
			PORT->Retries = 2;

			return;
		}
	}

}
VOID ProcessYaesuFrame(struct PORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
	int n, j, Freq = 0, decdigit;
	double FreqF;
	char Valchar[_CVTBUFSIZE];
	char Status[80];
	unsigned int Mode;

	// I'm not sure we get anything but a Command Response,
	// and the only command we send is Get Rig Frequency and Mode

	
	RIG->RIGOK = TRUE;
	PORT->Timeout = 0;

	for (j = 0; j < 4; j++)
		{
			n = Msg[j];
			decdigit = (n >> 4);
			decdigit *= 10;
			decdigit += n & 0xf;
			Freq = (Freq *100 ) + decdigit;
		}

		FreqF = Freq / 100000.0;

//		Valchar = _fcvt(FreqF, 6, &dec, &sign);
		_gcvt(FreqF, 9, Valchar);

		wsprintf(Status,"%s", Valchar);
		SetWindowText(RIG->hFREQ, Status);

		Mode = Msg[4];

		if (Mode > 15) Mode = 15;

		wsprintf(Status,"%s", YaesuModes[Mode]);
		SetWindowText(RIG->hMODE, Status);
}

VOID YaesuPoll(struct PORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			WriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
//		SetWindowText(RIG->hFREQ, "145.810000");
//		SetWindowText(RIG->hMODE, "RTTY/1");

		PORT->Rigs[PORT->CurrentRig].RIGOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	if (RIG->ScanStopped == 0)
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			UCHAR * ptr;

			// Get Permission to change

			if (RIG->WaitingForPermission)
			{
				RIG->OKtoChange = RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 2);	// Get Ok Flag
	
				if (RIG->OKtoChange == 1)
					goto DoChange;
				
				if (RIG->OKtoChange == -1)
				{
					// Premission Refused. Wait Scan Interval and try again

					RIG->WaitingForPermission = FALSE;
					RIG->ScanCounter = 10 * RIG->ScanFreq; 
					SetWindowText(RIG->hSCAN, "-");

					goto ScanExit;
				}
				
				goto ScanExit;			// Haven't got reply yet.
			}
			else
			{
				if (RIG->PortRecord->PORT_EXT_ADDR)
					RIG->WaitingForPermission = RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 1);	// Request Perrmission
				
				// If it returns zero there is no need to wait.
				
				if (RIG->WaitingForPermission)
					goto ScanExit;
			}

		DoChange:

			SetWindowText(RIG->hSCAN, "S");

			RIG->WaitingForPermission = FALSE;

			RIG->ScanCounter = 10 * RIG->ScanFreq; 

			ptr = RIG->FreqPtr;

			if (ptr == NULL)
				return;					// No Freqs
		
			if (*(ptr) == 0)			// End of list - reset to start
				ptr = CheckTimeBands(RIG);

			memcpy(PORT->TXBuffer, ptr, 10);

			RIG->FreqPtr += 10;
	
			PORT->TXLen = 5;
			WriteCommBlock(PORT);
			PORT->CmdSent = Poll[4];
			PORT->Retries = 2;
			PORT->AutoPoll = TRUE;

			return;
		}
	}

ScanExit:
	
	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&RIG->BPQtoRADIO_Q);

		datalen=buffptr[1];

		memcpy(Poll, buffptr+2, datalen);

		PORT->TXLen = 5;					// First send the set Freq
		WriteCommBlock(PORT);
		PORT->CmdSent = Poll[4];
		PORT->Retries = 2;

		ReleaseBuffer(buffptr);
		PORT->AutoPoll = FALSE;
	
		return;
	}

	if (RIG->ScanStopped == 0)
		return;						// no point in reading freq if we are about to change it
		
	// Read Frequency 

	Poll[0] = 0;
	Poll[1] = 0;
	Poll[2] = 0;
	Poll[3] = 0;
	Poll[4] = 0x3;		// Get frequency amd mode command

	PORT->TXLen = 5;
	WriteCommBlock(PORT);
	PORT->Retries = 2;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}


VOID ProcessKenwoodFrame(struct PORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	UCHAR * Msg = PORT->RXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu
//	int n, j, Freq = 0, decdigit;
//	double FreqF;
//	char Valchar[_CVTBUFSIZE];
	char Status[80];
//	unsigned int Mode;

	PORT->Timeout = 0;

	if (PORT->PORTOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		PORT->PORTOK = TRUE;
		wsprintf(Status,"COM%d PORT link OK", PORT->IOBASE);
		SetWindowText(PORT->hStatus, Status);
	}

	RIG->RIGOK = TRUE;

	wsprintf(Status,"%s", &Msg[2]);
	SetWindowText(RIG->hFREQ, Status);


}


VOID KenwoodPoll(struct PORTINFO * PORT)
{
	UCHAR * Poll = PORT->TXBuffer;
	struct RIGINFO * RIG = &PORT->Rigs[0];		// Only one on Yaseu

	if (RIG->ScanStopped == 0)
		if (RIG->ScanCounter)
			RIG->ScanCounter--;

	if (PORT->Timeout)
	{
		PORT->Timeout--;
		
		if (PORT->Timeout)			// Still waiting
			return;

		PORT->Retries--;

		if(PORT->Retries)
		{
			WriteCommBlock(PORT);	// Retransmit Block
			return;
		}

		SetWindowText(RIG->hFREQ, "------------------");
		SetWindowText(RIG->hMODE, "----------");
//		SetWindowText(RIG->hFREQ, "145.810000");
//		SetWindowText(RIG->hMODE, "RTTY/1");

		PORT->Rigs[PORT->CurrentRig].RIGOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	if (RIG->ScanStopped == 0)
	{
		if (RIG->ScanCounter <= 0)
		{
			//	Send Next Freq

			UCHAR * ptr;

			// Get Permission to change

			if (RIG->WaitingForPermission)
			{
				RIG->OKtoChange = RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 2);	// Get Ok Flag
	
				if (RIG->OKtoChange == 1)
					goto DoChange;
				
				if (RIG->OKtoChange == -1)
				{
					// Premission Refused. Wait Scan Interval and try again

					RIG->WaitingForPermission = FALSE;
					RIG->ScanCounter = 10 * RIG->ScanFreq; 
					SetWindowText(RIG->hSCAN, "-");

					goto ScanExit;
				}
				
				goto ScanExit;			// Haven't got reply yet.
			}
			else
			{
				if (RIG->PortRecord->PORT_EXT_ADDR)
					RIG->WaitingForPermission = RIG->PortRecord->PORT_EXT_ADDR(6, RIG->PortRecord->PORTCONTROL.PORTNUMBER, 1);	// Request Perrmission
				
				// If it returns zero there is no need to wait.
				
				if (RIG->WaitingForPermission)
					goto ScanExit;
			}

		DoChange:

			SetWindowText(RIG->hSCAN, "S");

			RIG->WaitingForPermission = FALSE;

			RIG->ScanCounter = 10 * RIG->ScanFreq; 

			ptr = RIG->FreqPtr;

			if (ptr == NULL)
				return;					// No Freqs
		
			if (*(ptr) == 0)			// End of list - reset to start
				ptr = CheckTimeBands(RIG);

			memcpy(PORT->TXBuffer, ptr, 18);

			RIG->FreqPtr += 18;
	
			PORT->TXLen = 18;
			WriteCommBlock(PORT);
			PORT->CmdSent = 1;
			PORT->Retries = 2;
			PORT->AutoPoll = TRUE;

			return;
		}
	}

ScanExit:
	
	if (RIG->RIGOK && RIG->BPQtoRADIO_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&RIG->BPQtoRADIO_Q);

		datalen=buffptr[1];

		memcpy(Poll, buffptr+2, datalen);

		PORT->TXLen = datalen;					// First send the set Freq
		WriteCommBlock(PORT);
		PORT->CmdSent = Poll[4];
		PORT->Retries = 2;

		ReleaseBuffer(buffptr);
		PORT->AutoPoll = FALSE;
	
		return;
	}

	if (RIG->ScanStopped == 0)
		return;						// no point in reading freq if we are about to change it
		
	// Read Frequency 

	Poll[0] = 'F';
	Poll[1] = 'A';
	Poll[2] = ';';

	PORT->TXLen = 3;
	WriteCommBlock(PORT);
	PORT->Retries = 2;
	PORT->CmdSent = 0;

	PORT->AutoPoll = TRUE;

	return;
}

int CRow;
HANDLE hComPort, hSpeed, hRigType, hButton, hAddr, hLabel, hTimes, hFreqs, hBPQPort;

VOID CreateRigConfigLine(HWND hDlg, struct PORTINFO * PORT, struct RIGINFO * RIG)
{
	char Port[10];

	hButton =  CreateWindow(WC_BUTTON , "", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,
					10, CRow+5, 10,10, hDlg, NULL, hInstance, NULL);

	if (PORT->PortType == ICOM)
	{
		char Addr[10];

		wsprintf(Addr, "%X", RIG->RigAddr);

		hAddr =  CreateWindow(WC_EDIT , Addr, WS_CHILD | WS_VISIBLE  | WS_TABSTOP | WS_BORDER,
                 305, CRow, 30,20, hDlg, NULL, hInstance, NULL);

	}
	hLabel =  CreateWindow(WC_EDIT , RIG->RigName, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER,
                 340, CRow, 60,20, hDlg, NULL, hInstance, NULL);

	wsprintf(Port, "%d", RIG->PortRecord->PORTCONTROL.PORTNUMBER);
	hBPQPort =  CreateWindow(WC_EDIT , Port, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                 405, CRow, 20, 20, hDlg, NULL, hInstance, NULL);

	hTimes =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 430, CRow, 100,80, hDlg, NULL, hInstance, NULL);

	hFreqs =  CreateWindow(WC_EDIT , RIG->FreqText, WS_CHILD | WS_VISIBLE| WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                 535, CRow, 300, 20, hDlg, NULL, hInstance, NULL);

	SendMessage(hTimes, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "0000:1159");
	SendMessage(hTimes, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "1200:2359");
	SendMessage(hTimes, CB_SETCURSEL, 0, 0);

	CRow += 30;	

}

VOID CreatePortConfigLine(HWND hDlg, struct PORTINFO * PORT)
{	
	char Port[20]; 
	int i;

	hComPort =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 30, CRow, 90, 160, hDlg, NULL, hInstance, NULL);

	for (i = 1; i < 256; i++)
	{
		wsprintf(Port, "COM%d", i);
		SendMessage(hComPort, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Port);
	}

	wsprintf(Port, "COM%d", PORT->IOBASE);

	i = SendMessage(hComPort, CB_FINDSTRINGEXACT, 0,(LPARAM) Port);

	SendMessage(hComPort, CB_SETCURSEL, i, 0);
	
	
	hSpeed =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | 
                    WS_VSCROLL | WS_TABSTOP,
                 120, CRow, 75, 80, hDlg, NULL, hInstance, NULL);

	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "1200");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "2400");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "4800");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "9600");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "19200");
	SendMessage(hSpeed, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "38400");

	wsprintf(Port, "%d", PORT->SPEED);

	i = SendMessage(hSpeed, CB_FINDSTRINGEXACT, 0, (LPARAM)Port);

	SendMessage(hSpeed, CB_SETCURSEL, i, 0);

	hRigType =  CreateWindow(WC_COMBOBOX , "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL | WS_TABSTOP,
                 200, CRow, 100,80, hDlg, NULL, hInstance, NULL);

	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "ICOM");
	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "YAESU");
	SendMessage(hRigType, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "KENWOOD");

	SendMessage(hRigType, CB_SETCURSEL, PORT->PortType -1, 0);

}

INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Cmd = LOWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		struct PORTINFO * PORT;
		struct RIGINFO * RIG;
		int i, p;

		CRow = 40;

		for (p = 0; p < NumberofPorts; p++)
		{
			PORT = PORTInfo[p];

			CreatePortConfigLine(hDlg, PORT);
		
			for (i=0; i < PORT->ConfiguredRigs; i++)
			{
				RIG = &PORT->Rigs[i];
				CreateRigConfigLine(hDlg, PORT, RIG);
			}
		}



/*	 CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 90, Row, 40,20, hDlg, NULL, hInstance, NULL);
	
	 CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 135, Row, 100,20, hDlg, NULL, hInstance, NULL);
*/
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

