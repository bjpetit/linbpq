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

UCHAR * BPQDirectory;


RECT Rect;

struct TNCINFO * TNCInfo[18] = {NULL};		// Records are Malloc'd

BOOL ReadConfigFile(char * filename);
int ProcessLine(char * buf);

#ifndef WINMOR

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
	
	wsprintf(Title,"Pactor Status - COM%d", TNC->PortRecord->PORTCONTROL.IOBASE);

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
			sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);
	}

	Top = Rect.top;
	Left = Rect.left;

	GetWindowRect(TNC->hDlg, &Rect);	// Get the real size

	MoveWindow(TNC->hDlg, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	ShowWindow(TNC->hDlg, SW_SHOWNORMAL);

	return TRUE;
}

#endif



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
		if ((MH->MHCALL[0] == 0) || ((memcmp(AXCall, MH->MHCALL, 7) == 0) && MH->MHDIGI == Mode)) // Spare our our entry
		{
			// Move others down and add at front

			if (i != 0)				// First
			{
				memmove(MHBASE + 1, MHBASE, i * sizeof(struct MHSTRUC));
			}

			memcpy (MHBASE->MHCALL, AXCall, 7);
			MHBASE->MHDIGI = Mode;
			MHBASE->MHTIME = _time32(NULL);

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

	return 1;
}

ProcessLine(char * buf)
{
	char * ptr,* p_cmd;
	char * p_ipad = 0;
	char * p_port = 0;
	unsigned short WINMORport = 0;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	ptr = strtok(NULL, " \t\n\r");

	BPQport=0;

	BPQport = atoi(ptr);


	if(BPQport > 0 && BPQport < 17)
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

#endif

		p_cmd = strtok(NULL, " \t\n\r");
			
		if (p_cmd != NULL)
		{
			if (p_cmd[0] != ';' && p_cmd[0] != '#')
				TNC->ApplCmd=_strdup(p_cmd);
		}

		// Read Initialisation lines

		TNC->InitScript = malloc(1000);

		TNC->InitScript[0] = 0;

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;

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
#endif
				strcat (TNC->InitScript, buf);
		}
	}

	return (FALSE);
	
}

BOOL LoadRigDriver()
{
	char msg[128];
	int err=0;
	UCHAR Value[MAX_PATH];
	char DLL[]="RigControl.dll";
	
	// If no directory, use current

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value, DLL);
	}
		else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value, DLL);
	}
		
	hRigModule = LoadLibrary(Value);

	if (hRigModule == NULL)
	{
		err=GetLastError();

		wsprintf(msg,"Error loading Driver %s - Error code %d",	DLL,err);
		
		WritetoConsole(msg);
		return FALSE;

	}
	else
	{
		Rig_Init = (int (__stdcall *)())GetProcAddress(hRigModule,"_Rig_Init@0");
		Rig_Poll = (int (__stdcall *)())GetProcAddress(hRigModule,"_Rig_Poll@0");
		Rig_Command = (int (__stdcall *)())GetProcAddress(hRigModule,"_Rig_Command@8");
	}
	return TRUE;
}



