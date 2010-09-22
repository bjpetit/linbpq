//
//	Code Common to Pactor Modules

#define BGCOLOUR RGB(236,233,216)

BOOL MinimizetoTray = FALSE;
HANDLE hInstance;
HBRUSH bgBrush;

HINSTANCE hRigModule = 0;

BOOL (FAR WINAPI * Rig_Command) ();
BOOL (FAR WINAPI * Rig_Init) ();
BOOL (FAR WINAPI * Rig_Poll) ();
VOID (FAR WINAPI * Rig_PTT) ();
struct RIGINFO * (FAR WINAPI * Rig_GETPTTREC) ();
struct ScanEntry ** (FAR WINAPI * CheckTimeBands) ();

UCHAR * BPQDirectory;

static BOOL Minimized;				// Start Minimized Flag

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

DllImport struct APPLCALLS APPLCALLTABLE[];
DllImport char APPLS;
DllImport struct BPQVECSTRUC * BPQHOSTVECPTR;

RECT Rect;

struct RIGINFO DummyRig;		// Used if not using Rigcontrol

struct TNCINFO * TNCInfo[34] = {NULL};		// Records are Malloc'd

BOOL ReadConfigFile(char * filename);
int ProcessLine(char * buf);
VOID __cdecl Debugprintf(const char * format, ...);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

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
	
#ifdef WINMOR
	wsprintf(Title,"WINMOR Status - Port %d", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
#else
#ifdef HAL
	wsprintf(Title,"HAL Status - COM%d", TNC->PortRecord->PORTCONTROL.IOBASE);
#else
	wsprintf(Title,"Pactor Status - COM%d", TNC->PortRecord->PORTCONTROL.IOBASE);
#endif
#endif
	SetWindowText(TNC->hDlg, Title);

	if (MinimizetoTray)
		AddTrayMenuItem(TNC->hDlg, Title);


	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
	
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &Minimized);

#ifdef WINMOR

		retCode = RegQueryValueEx(hKey,"RestartAfterFailure",0,			
			(ULONG *)&Type,(UCHAR *)&RestartAfterFailure,(ULONG *)&Vallen);
#endif

	}

	Top = Rect.top;
	Left = Rect.left;

	GetWindowRect(TNC->hDlg, &Rect);	// Get the real size

	MoveWindow(TNC->hDlg, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	if (Minimized)
		ShowWindow(TNC->hDlg, SW_HIDE);
	else
		ShowWindow(TNC->hDlg, SW_SHOWNORMAL);

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

BOOL ReadConfigFile(char * fn)
{
	char buf[256],errbuf[256];

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

	return 1;
}

ProcessLine(char * buf)
{
	UCHAR * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;
	char errbuf[256];

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	ptr = strtok(NULL, " \t\n\r");

	BPQport=0;

	BPQport = atoi(ptr);


	if(BPQport > 0 && BPQport < 33)
	{
		TNC = TNCInfo[BPQport] = malloc(sizeof(struct TNCINFO));
		memset(TNC, 0, sizeof(struct TNCINFO));

#ifdef WINMOR

		p_ipad = strtok(NULL, " \t\n\r");
		
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


#else
		p_cmd = strtok(NULL, " \t\n\r");
#endif			
		if (p_cmd != NULL)
		{
			if (p_cmd[0] != ';' && p_cmd[0] != '#')
				TNC->ApplCmd=_strdup(p_cmd);
		}

		// Read Initialisation lines


		TNC->InitScript = malloc(1000);

		TNC->InitScript[0] = 0;

#ifdef AEA

		strcpy(TNC->InitScript, "RESTART\r");

#endif

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;

			strcpy(errbuf, buf);

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

			ptr = strchr(buf, ';');
			if (ptr)
			{
				*ptr++ = 13;
				*ptr = 0;
			}

#ifdef SCS

			if (_memicmp(buf, "RIGCONTROL COM", 14) == 0)
	
				// SCS Virtual COM Channel

				TNC->VCOMPort = atoi(&buf[14]);
			else
				
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

BOOL LoadRigDriver()
{
	char msg[128];
	int err=0;

	if (hRigModule)
		return TRUE;					// Already done it.

	hRigModule = GetModuleHandle("bpq32.dll");
		
	if (hRigModule == NULL)
	{
		err=GetLastError();

		wsprintf(msg,"Error loading bpq32.dll - Error code %d", err);
		
		WritetoConsole(msg);
		return FALSE;

	}
	else
	{
		Rig_Init = (int (__stdcall *)())GetProcAddress(hRigModule,"_Rig_Init@0");
		Rig_Poll = (int (__stdcall *)())GetProcAddress(hRigModule,"_Rig_Poll@0");
		Rig_Command = (int (__stdcall *)())GetProcAddress(hRigModule,"_Rig_Command@8");
		Rig_PTT = (VOID (__stdcall *)())GetProcAddress(hRigModule,"_Rig_PTT@8");
		Rig_GETPTTREC = (struct RIGINFO * (__stdcall *)())GetProcAddress(hRigModule,"_Rig_GETPTTREC@4");
		CheckTimeBands = (struct ScanEntry ** (__stdcall *)())GetProcAddress(hRigModule,"_CheckTimeBands@4");
	}
	return TRUE;
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
					else
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
