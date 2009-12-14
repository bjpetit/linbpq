//
//	DLL to inteface SCS TNC in Pactor Mode to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

#define WIN32_LEAN_AND_MEAN
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

UCHAR * BPQDirectory;

char CFGFN[MAX_PATH];

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct TNCINFO * TNC);
OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed);
VOID ICOMPoll(int Port);
VOID ProcessFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
BOOL ReadConfigFile();
int ProcessLine(char * buf);
VOID ProcessHostFrame(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
SendResponse(int Stream, char * Msg);

DllImport UCHAR NEXTID;
DllImport struct TRANSPORTENTRY * L4TABLE;
DllImport WORD MAXCIRCUITS;
DllImport UCHAR L4DEFAULTWINDOW;
DllImport WORD L4T1;

// Buffer Pool

#define NUMBEROFBUFFERS 20

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers

char Modes[8][6] = {"LSB",  "USB", "AM", "CW", "RTTY", "FM", "WFM", "????"};

//
//	Code Common to Pactor Modules

#define BGCOLOUR RGB(236,233,216)

BOOL MinimizetoTray = FALSE;
BOOL StartMinimized = FALSE;

HANDLE hInstance;
HBRUSH bgBrush;

RECT Rect;

int NumberofPorts = 0;


struct TNCINFO * TNCInfo[18] = {NULL};		// Records are Malloc'd

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

BOOL CreatePactorWindow(struct TNCINFO * TNC)
{
    WNDCLASS  wc;
	char Title[80];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Key[80];
	char Size[80];
	int Top, Left;

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

	TNC->hDlg = CreateDialog(hInstance,ClassName,0,NULL);
	
	wsprintf(Title,"Rig Control - COM%d", TNC->IOBASE);

	SetWindowText(TNC->hDlg, Title);

	if (MinimizetoTray)
		AddTrayMenuItem(TNC->hDlg, Title);


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

	GetWindowRect(TNC->hDlg, &Rect);	// Get the real size

	MoveWindow(TNC->hDlg, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	ShowWindow(TNC->hDlg, SW_SHOWNORMAL);

	return TRUE;
}





FILE *file;

char errbuf[256];

BOOL ReadConfigFile()
{
	char buf[256];

	if ((file = fopen(CFGFN,"r")) == NULL)
	{
		wsprintf(buf," Config file %s not found ", CFGFN);
		WritetoConsole(buf);
		return (TRUE);			// Dont need it at the moment
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
	struct TNCINFO * TNC;
	struct STREAMINFO * Stream;
	char * FreqPtr;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

//#COM58 19200 ICOM

NextPort:

	if (memcmp(ptr, "COM", 3) == 0)
	{
		// New Port definition

		TNC = TNCInfo[NumberofPorts++] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));

		TNC->IOBASE = atoi(&ptr[3]);
		ptr = strtok(NULL, " \t\n\r");
		if(ptr == NULL) return (FALSE);
		TNC->SPEED = atoi(ptr);

		ptr = strtok(NULL, " \t\n\r");
		if(ptr == NULL) return (FALSE);

		if (strcmp(ptr, "ICOM") == 0)
		{
			// RADIO IC706 4E 5 14.103/U1 14.112/u1 18.1/U1 10.12/l1
			// Read RADIO Lines

			Radio = 0;

			while(TRUE)
			{
				if (GetLine(buf) == 0)
					return TRUE;

				if (memcmp(buf, "COM", 3) == 0)
					goto NextPort;

				Stream = &TNC->Streams[Radio];

				ptr = strtok(buf, " \t\n\r");
				if (ptr == NULL) return (FALSE);

				ptr = strtok(NULL, " \t\n\r");
				if (ptr == NULL) return (FALSE);

				if (strlen(ptr) > 9) return FALSE;
				strcpy(Stream->RigName, ptr);

				ptr = strtok(NULL, " \t\n\r");
				if (ptr == NULL) return (FALSE);
				
				sscanf(ptr, "%x", &Stream->RigAddr);

				if (Stream->RigAddr == 0) return FALSE;

				ptr = strtok(NULL, " \t\n\r");
				if (ptr == NULL) return (FALSE);

				Stream->BPQPort = atoi(ptr);

				ptr = strtok(NULL, " \t\n\r");
				if (ptr == NULL) return (FALSE);

				Stream->ScanFreq = atoi(ptr);

				ptr = strtok(NULL, " \t\n\r");

				// Frequency List

				if (ptr)
					if (ptr[0] == ';' || ptr[0] == '#')
						ptr = NULL;

				if (ptr != NULL)
				{
					Stream->FreqPtr = Stream->FreqList = malloc(1000);
					FreqPtr = Stream->FreqList;
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

					Freq = atof(ptr);

					if (Modeptr)
					{
						for (ModeNo = 0; ModeNo < 8; ModeNo++)
						{
							if (Modes[ModeNo][0] == Modeptr[0])
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


					*(FreqPtr++) = 0xFE;
					*(FreqPtr++) = 0xFE;
					*(FreqPtr++) = Stream->RigAddr;
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
						*(FreqPtr++) = Stream->RigAddr;
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

					*(FreqPtr) = 0;

					ptr = strtok(NULL, " \t\n\r");
				}

				Radio++;
				TNC->MaxStreams++;
				
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

HANDLE hInstance;

char VersionString[100];


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	int i;
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	struct TNCINFO * TNC;

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
	
		for (i=0; i<16; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
			if (TNC->hDlg == NULL)
				continue;

			ShowWindow(TNC->hDlg, SW_RESTORE);
			GetWindowRect(TNC->hDlg, &Rect);

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
		}
 		return 1;
	}
 
	return 1;
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
	struct TNCINFO * TNC;
	int i;
	struct STREAMINFO * Stream;

	n = sscanf(Command,"%d %s %s %d ", &Port, &FreqString, &Mode  , &Filter);

	// Look for the port 

	TNC = TNCInfo[0];			// Only 1 for now

	if (TNC ==NULL)
	{
		wsprintf(Command, "Sorry - Rig Control not configured\r");
		return FALSE;
	}

	Stream = &TNC->Streams[0];

	for (i=0; i< TNC->MaxStreams; i++)
	{
		Stream = &TNC->Streams[i];

		if (Stream->BPQPort == Port)
			goto portok;

	}

	wsprintf(Command, "Sorry - Port not found\r");
	return FALSE;

portok:

	if (Stream->TNCOK == 0)
	{
		wsprintf(Command, "Sorry - Radio not responding\r");
		return FALSE;
	}

	if (n == 2)
	{
		if (strcmp(FreqString, "SCANSTART") == 0)
		{
			if (Stream->FreqList)
			{
				Stream->Scanning = TRUE;
				Stream->ScanCounter = 150;	// 15 sec delay
				wsprintf(Command, "Ok\r");
			}
			else
				wsprintf(Command, "Sorry no Scan List defined for this port\r");

			return FALSE;
		}

		if (strcmp(FreqString, "SCANSTOP") == 0)
		{
			Stream->Scanning = FALSE;
			wsprintf(Command, "Ok\r");
			return FALSE;
		}
	}

	Freq = atof(FreqString);

	if (n != 4)
	{
		strcpy(Command, "Sorry - Invalid Format - should be Port Freq Mode Filter Width\r");
		return FALSE;
	}

	if (Freq < 0.1)
	{
		strcpy(Command, "Sorry - Invalid Frequency\r");
		return FALSE;
	}

	for (ModeNo = 0; ModeNo < 8; ModeNo++)
	{
		if (_stricmp(Modes[ModeNo], Mode) == 0)
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

	Stream->Session = Session;		// BPQ Stream

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


	Poll = (UCHAR *)&buffptr[2];

	*(Poll++) = 0xFE;
	*(Poll++) = 0xFE;
	*(Poll++) = Stream->RigAddr;
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
	*(Poll++) = Stream->RigAddr;
	*(Poll++) = 0xE0;
	*(Poll++) = 0x6;		// Set Mode
	*(Poll++) = ModeNo;
	*(Poll++) = Filter;
	*(Poll++) = 0xFD;

	buffptr[1] = 19;
		
	Q_ADD(&Stream->BPQtoPACTOR_Q, buffptr);

	return TRUE;
}
DllExport BOOL APIENTRY Rig_Init()
{
	HRSRC RH;
  	struct tagVS_FIXEDFILEINFO * HG;
	struct TNCINFO * TNC;
	int i;
	char msg[80];

#ifdef _DEBUG 
	char isDebug[]="Debug Build";
#else
	char isDebug[]="";
#endif

	HMODULE HM;

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


	// Build buffer pool

	FREE_Q = 0;			// In case reloading;

	for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
	{	
		ReleaseBuffer(&BufferPool[100*i]);
	}

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
	
	ReadConfigFile();

	TNC = TNCInfo[0]; 

	if (TNC == NULL)
	{
		WritetoConsole("\nRig Control Disabled\n");
		return TRUE;
	}
	
	MinimizetoTray=GetMinimizetoTrayFlag();
	if (GetStartMinimizedFlag) StartMinimized=GetStartMinimizedFlag();

	CreatePactorWindow(TNC);

	for (i=0; i < TNC->MaxStreams; i++)
	{
		wsprintf(msg,"%02X", TNC->Streams[i].RigAddr);
		SetDlgItemText(TNC->hDlg, IDC_CAT + i, msg);
		SetDlgItemText(TNC->hDlg, IDC_RIG + i, TNC->Streams[i].RigName);
	}
	
	OpenCOMMPort(TNC, TNC->IOBASE, TNC->SPEED);

	WritetoConsole("\nRig Control Enabled\n");

	return TRUE;
}

DllExport BOOL APIENTRY Rig_Poll()
{
	struct TNCINFO * TNC = TNCInfo[0];

	if (TNC == NULL)
		return TRUE;
	
	ICOMPoll(0);
	CheckRX(TNC);

	return TRUE;
}
 

VOID KISSCLOSE(int Port)
{ 
	DestroyTTYInfo(Port);
}

BOOL NEAR DestroyTTYInfo(int port)
{
   // force connection closed (if not already closed)

   CloseConnection(TNCInfo[port]);

   return TRUE;

} // end of DestroyTTYInfo()


BOOL CloseConnection(struct TNCINFO * conn)
{
   // disable event notification and wait for thread
   // to halt

   SetCommMask(conn->hDevice, 0);

   // drop DTR

   EscapeCommFunction(conn->hDevice, CLRDTR);

   // purge any outstanding reads/writes and close device handle

   PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
   CloseHandle(conn->hDevice);
 
   return TRUE;

} // end of CloseConnection()

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
		wsprintf(buf," COM%d Setup Failed - Error %d ", Port, GetLastError());
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
void CheckRX(struct TNCINFO * TNC)
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
		return;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return;
	}
	
	TNC->RXLen += Length;

	Length = TNC->RXLen;
			
	if (Length < 6)				// Minimum Frame Sise
		return;

	if (TNC->RXBuffer[Length-1] != 0xfd)
		return;	

	ProcessHostFrame(TNC, TNC->RXBuffer, Length);	// Could have multiple packets in buffer

	TNC->RXLen = 0;		// Ready for next frame

		
	return;
}

VOID ProcessHostFrame(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;

	//	Split into Packets. By far the most likely is a single KISS frame
	//	so treat as special case
	
	FendPtr = memchr(rxbuffer, 0xfd, Len);
	
	if (FendPtr == &rxbuffer[Len-1])
	{
		ProcessFrame(TNC, rxbuffer, Len);
		return;
	}
		
	// Process the first Packet in the buffer

	NewLen =  FendPtr - rxbuffer +1;

	ProcessFrame(TNC, rxbuffer, NewLen);
	
	// Loop Back

	ProcessHostFrame(TNC, FendPtr+1, Len - NewLen);
	return;

}



BOOL NEAR WriteCommBlock(struct TNCINFO * TNC)
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

	TNC->Timeout = 20;		// 2 secs

	return TRUE;
}

VOID ICOMPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	int i;

	struct STREAMINFO * Stream;

	Stream = &TNC->Streams[0];

	for (i=0; i< TNC->MaxStreams; i++)
	{
		Stream = &TNC->Streams[i];

		if (Stream->Scanning)
			if (Stream->ScanCounter)
				Stream->ScanCounter--;
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

		SetDlgItemText(TNC->hDlg, IDC_FREQ + TNC->CurrentStream, "----------");
		SetDlgItemText(TNC->hDlg, IDC_MODE + TNC->CurrentStream, "-----");

		TNC->Streams[TNC->CurrentStream].TNCOK = FALSE;

		return;

	}

	// Send Data if avail, else send poll

	TNC->CurrentStream++;

	if (TNC->CurrentStream >= TNC->MaxStreams)
		TNC->CurrentStream = 0;

	Stream = &TNC->Streams[TNC->CurrentStream];


	if (Stream->Scanning)
	{
		if (Stream->ScanCounter <= 0)
		{
			//	Send Next Freq

			UCHAR * ptr;

			Stream->ScanCounter = 10 * Stream->ScanFreq; 

			ptr = Stream->FreqPtr;

			if (ptr == NULL)
				return;					// No Freqs
		
			if (*(ptr) == 0)			// End of list - reset to start
				Stream->FreqPtr = ptr = Stream->FreqList;

			memcpy(TNC->TXBuffer, ptr, 19);

			Stream->FreqPtr += 19;
	
			TNC->TXLen = 11;
			WriteCommBlock(TNC);
			TNC->Retries = 2;

			return;
		}
	}


	if (Stream->TNCOK && Stream->BPQtoPACTOR_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&Stream->BPQtoPACTOR_Q);

		datalen=buffptr[1];

		memcpy(Poll, buffptr+2, datalen);

		TNC->TXLen = 11;					// First send the set Freq
		WriteCommBlock(TNC);
		TNC->Retries = 2;
	
		return;
	}

	if (Stream->Scanning)
		return;						// no point in reading freq if we are about to change it
		
	// Read Frequency 

	Poll[0] = 0xFE;
	Poll[1] = 0xFE;
	Poll[2] = Stream->RigAddr;
	Poll[3] = 0xE0;
	Poll[4] = 0x3;		// Get frequency command
	Poll[5] = 0xFD;

	TNC->TXLen = 6;
	WriteCommBlock(TNC);
	TNC->Retries = 2;
	return;
}


VOID ProcessFrame(struct TNCINFO * TNC, UCHAR * Msg, int framelen)
{
	char Status[80];
	UCHAR * Poll = TNC->TXBuffer;
	struct STREAMINFO * Stream;
	int i;

	if (Msg[0] != 0xfe || Msg[1] != 0xfe)

		// Duff Packer - return

		return;	

	if (Msg[2] != 0xe0)
	{
		// Echo - Proves a CI-V interface is attached

		if (TNC->TNCOK == FALSE)
		{
			// Just come up
			char Status[80];
		
			TNC->TNCOK = TRUE;
			wsprintf(Status,"COM%d CI-V link OK", TNC->IOBASE);
			SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);
		}
		return;
	}

	for (i=0; i< TNC->MaxStreams; i++)
	{
		Stream = &TNC->Streams[i];
		if (Msg[3] == Stream->RigAddr)
			goto ok;
	}

	return;

ok:

	if (Msg[4] == 0xFB)
	{
		// Accept

		// if it was the set freq, send the set mode

		if (TNC->TXBuffer[4] == 5)
		{
			if (TNC->TXBuffer[11])
			{
				memcpy(TNC->TXBuffer, &TNC->TXBuffer[11], 8);
				TNC->TXLen = 8;
				WriteCommBlock(TNC);
				TNC->Retries = 2;
			}
			else
			{
				if (!Stream->Scanning)
					SendResponse(Stream->Session, "FrequencySet OK");
			}

			return;
		}

		if (TNC->TXBuffer[4] == 6)
		{
			// Set Mode Response - id scanning read freq, else return OK to user

			if (Stream->Scanning)
			{
				Poll[0] = 0xFE;
				Poll[1] = 0xFE;
				Poll[2] = Stream->RigAddr;
				Poll[3] = 0xE0;
				Poll[4] = 0x3;		// Get frequency command
				Poll[5] = 0xFD;

				TNC->TXLen = 6;
				WriteCommBlock(TNC);
				TNC->Retries = 2;
			}

			else
				SendResponse(Stream->Session, "Frequency and Mode Set OK");
		}

		TNC->Timeout = 0;
		return;
	}

	if (Msg[4] == 0xFA)
	{
		// Reject
		TNC->Timeout = 0;
		if (TNC->TXBuffer[4] == 5)
			SendResponse(Stream->Session, "Sorry - Set Frequency Command Rejected");
		else
		if (TNC->TXBuffer[4] == 6)
			SendResponse(Stream->Session, "Sorry - Set Mode Command Rejected");

		return;
	}

	if (Msg[4] == TNC->TXBuffer[4])
	{
		// Response to our command

		// Any valid frame is an ACK

		Stream->TNCOK = TRUE;
		TNC->Timeout = 0;
	}
	else 
		return;		// What does this mean??


	if (TNC->TNCOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		TNC->TNCOK = TRUE;
		wsprintf(Status,"COM%d TNC link OK", TNC->IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);
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
		SetDlgItemText(TNC->hDlg, IDC_FREQ + i, Status);

		// Now get Mode

			Poll[0] = 0xFE;
			Poll[1] = 0xFE;
			Poll[2] = Stream->RigAddr;
			Poll[3] = 0xE0;
			Poll[4] = 0x4;		// Get Mode
			Poll[5] = 0xFD;

		TNC->TXLen = 6;
		WriteCommBlock(TNC);
		TNC->Retries = 2;
		return;
	}
	if (Msg[4] == 4)
	{
		// Mode

		unsigned int Mode = Msg[5];

		if (Mode > 7) Mode = 7;

		wsprintf(Status,"%s/%d", Modes[Mode], Msg[6]);
		SetDlgItemText(TNC->hDlg, IDC_MODE + i, Status);
	}
}

VOID * APIENTRY GetBuff();

SendResponse(int Stream, char * Msg)
{
	PMESSAGE Buffer = GetBuff();
	struct BPQVECSTRUC * VEC;
	struct TRANSPORTENTRY * L4 = L4TABLE;

	L4 += Stream;

	Buffer->LENGTH = wsprintf((LPSTR)Buffer, "       \xf0%s\r", Msg);

	VEC = L4->L4TARGET;

	Q_ADD((UINT *)&L4->L4TX_Q, (UINT *)Buffer);

	PostMessage(VEC->HOSTHANDLE, BPQMsg, VEC->HOSTSTREAM, 2);  

	return 0;
}




