
//
//	Program to Configure BPQ Virtual Serial Ports

// Version 1.0.1.1 September 2010

// Increace numbe rof Virtual Ports to 20

#include "stdafx.h"
#include "resource.h"

//#include "bpq32.h"
#include "GetVersion.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text

char ClassName[]="MAINWINDOW";					// the main window class name
char Title[80];

UCHAR driverLocation[MAX_PATH];

HWND MainWnd;

char szBuff[80];

char Msg[100];

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

char DRIVERNAME[] = "BPQVirtualCOM"	;
char DRIVERFILENAME[]= "BPQVirtualCOM.sys";
 

char Driver[100]="";		// Win98 Driver Key


UCHAR Ports[20];

int PortKey[20];			// Win 98 Subkey for this port

FILETIME OldLastWriteTime;
FILETIME NewLastWriteTime;
SYSTEMTIME Time;

char SysDriverName[MAX_PATH];


BOOL Get98DriverTimeStamp();

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL Initialise();
void format_error (char *msg, int msglen, int errnum);
BOOLEAN ManageDriver(
    IN LPCTSTR  DriverName,
    IN LPCTSTR  ServiceName,
    IN USHORT   Function
    );
int LoadDriver();
VOID OutputLine(char * Msg);
VOID GetPorts();
VOID GetPorts98();
VOID GetPortsXP();

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

BOOL cfgMinToTray;

BOOL Win98 = FALSE;

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)


#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0




int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	

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
	int err, i;
	char port[20];

	hInst = hInstance; // Store instance handle in our global variable

   	if (HIBYTE(_winver) < 5)
	{
		Win98 = TRUE;
		strcpy(DRIVERFILENAME, "BPQVComm.vxd");
	}


	hWnd=CreateDialog(hInst,ClassName,0,NULL);


   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    //  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
	   err=GetLastError();
      return FALSE;
   }

   MainWnd=hWnd;

	for (i=1; i<256; i++)
	{
		wsprintf(port,"COM%d",i);
		SendDlgItemMessage(MainWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) port);
	}

	SendDlgItemMessage(MainWnd, IDC_COMBO1, CB_SETCURSEL, 0, 0);

	GetVersionInfo(NULL);

	if (Win98)
		wsprintf(Title,"VCOM Config Version %s on Win98", VersionString);
	else
		wsprintf(Title,"VCOM Config Version %s", VersionString);

	SetWindowText(hWnd,Title);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	GetPorts();

   	return Initialise();
}

VOID Refresh()
{
	int i;
	char com[4];

	for (i=0; i<20; i++)
	{
		wsprintf(com,"%d",Ports[i]);
		SendDlgItemMessage(MainWnd, IDC_EDIT1+i, WM_SETTEXT, 0, (LPARAM) (LPCTSTR) com);
	}
}

VOID GetPorts()
{
	if (Win98)
		GetPorts98(); else GetPortsXP();
}

VOID GetPortsXP()
{
	HANDLE hControl;
	int i, PortNo, ErrorVal;

	hControl = BPQOpenSerialControl(&ErrorVal);

	if (hControl == (HANDLE) -1)
	{
		MessageBox (MainWnd, "BPQ Virtual COM Driver Control Channel Open Failed\nIs the Driver loaded?","",0);
		return;
	}

	for (i=0; i<20; i++)
	{
		BPQSerialGetDeviceList(hControl, &i, &PortNo);
		Ports[i] = PortNo;
	}

	CloseHandle(hControl);

	Refresh();
}

VOID GetPorts98()
{
	HKEY hKey, hSubKey;
	int retCode, i=0, PortNo;
	FILETIME  ftLastWriteTime;
	int Index, cbName;
	char Name[100];
	char Key[100];
	char DeviceDesc[100];
		
	retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              "Enum\\Root\\PORTS",
 							  0,	// Options
                              KEY_ALL_ACCESS,
                              &hKey);

	if (retCode != ERROR_SUCCESS)
	{

		MessageBox (MainWnd, "Open PORTS Key Failed","",0);
		return;
	}
			;

	Index = 0;
	
	do {
		
		cbName = 100;

		retCode = RegEnumKeyEx(hKey, Index, Name, &cbName,	NULL, NULL, 0, &ftLastWriteTime );

		if (retCode != ERROR_SUCCESS) break;


		wsprintf(Key, "Enum\\Root\\PORTS\\%s", Name);

		retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              Key,
 							  0,	// Options
                              KEY_ALL_ACCESS,
                              &hSubKey);

		if (retCode != ERROR_SUCCESS)
		{

			MessageBox (MainWnd, "Open Key Failed","",0);
			return;
		}
		
		cbName = 100;
		retCode = RegQueryValueEx(hSubKey, "DeviceDesc", 0, NULL, DeviceDesc, &cbName);

		if (retCode == ERROR_SUCCESS)
		{
			if (_memicmp(DeviceDesc, "G8BPQ Virtual Serial Port", 25) == 0)
			{
				// BPQ Port
			
				PortNo = atoi(&DeviceDesc[30]);
				PortKey[i] = atoi(Name);
				Ports[i++] = PortNo;
			}

			CloseHandle(hSubKey);
		}


		Index++;

	} while (TRUE);


	CloseHandle(hKey);

	Refresh();
}


VOID AddDeviceXP(int id)
{
	HANDLE hControl;
	int Errorval, i;

	hControl = BPQOpenSerialControl(&Errorval);

	if (hControl == (HANDLE) -1)
	{
		MessageBox (MainWnd, "BPQ Virtual COM Driver Control Channel Open Failed\nIs the Driver loaded?","",0);
		return;
	}
           
	BPQSerialAddDevice(hControl, &id, &Errorval);

	CloseHandle(hControl);

	if (Errorval == 0)
	{
		for (i=0; i<20; i++)
		{
			if (Ports[i] == 0)
			{
				Ports[i]=id;
				Refresh();
				break;
			}
		
		}
	}
	else
	{
		MessageBox (MainWnd, "Add Port Failed","",0);
		return;
	}

	wsprintf(Msg,"Port COM%d Added",id);
	OutputLine(Msg);

}

VOID GetDriverKey98()
{
	HKEY hKey, hSubKey;
	int retCode, i=0;
	FILETIME  ftLastWriteTime;
	int Index, cbName;
	char Name[100];
	char Value[100];
		
	retCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                  "System\\CurrentControlSet\\Services\\Class\\Ports",
 							  0,	// Options
                              KEY_ALL_ACCESS,
                              &hKey);

	if (retCode != ERROR_SUCCESS)
	{

		MessageBox (MainWnd, "Open Services Key Failed","",0);
		return;
	}
			;

	Index = 0;
	
	do {
		
		cbName = 100;

		retCode = RegEnumKeyEx(hKey, Index, Name, &cbName,	NULL, NULL, 0, &ftLastWriteTime );

		if (retCode != ERROR_SUCCESS) break;


		retCode = RegOpenKeyEx(hKey,
                              Name,
 							  0,	// Options
                              KEY_ALL_ACCESS,
                              &hSubKey);

		if (retCode != ERROR_SUCCESS)
		{

			MessageBox (MainWnd, "Open Enumerated Subkey Key Failed","",0);
			return;
		}
		



		cbName = 100;
		retCode = RegQueryValueEx(hSubKey, "PortDriver", 0, NULL, Value, &cbName);

		if (retCode == ERROR_SUCCESS)
		{
			if (_stricmp(Value, "BPQVCOMM.VXD") == 0)
			{
				// Our Driver
			
				strcpy(Driver, "Ports\\");
				strcat(Driver, Name);
				CloseHandle(hSubKey);
				return;
			}

			CloseHandle(hSubKey);
		}


		Index++;

	} while (TRUE);


	CloseHandle(hKey);

	Refresh();
}


BYTE DCBVal[] = {0x1c,00,00,00,0x80,0x25,00,00,0x09,00,00,00,00,00,0x0a,00,0x0a,00,0x08,00,00,
	0x11,0x13,00,00,00,00,00};

BYTE SubClass[] = {1};


VOID CreateDriverKey98()
{

	// Need to Create a Driver entry in:
	// [HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\Class\Ports]

	HKEY hKey;
	int retCode, i=0;
	int disp;
	char Key[100];

	i = 1000;

	do {
		
		wsprintf(Key, "System\\CurrentControlSet\\Services\\Class\\PORTS\\%04d", i);

		retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			Key,
            0,	// Reserved
			0,	// Class
			0,	// Options
            KEY_ALL_ACCESS,
			NULL,	// Security Attrs
            &hKey,
			&disp);

		if (retCode == ERROR_SUCCESS)
		{
			if (disp == REG_CREATED_NEW_KEY)
			{				
				// Created, so OK


				RegSetValueEx(hKey, "ConfigDialog", 0, REG_SZ, "serialui.dll", 13);
				RegSetValueEx(hKey, "DCB", 0, REG_BINARY, (unsigned char *)&DCBVal, 28);
				RegSetValueEx(hKey, "DevLoader", 0, REG_SZ, "*vcomm", 7);
 				RegSetValueEx(hKey, "DriverDate", 0, REG_SZ, "1-01-2009", 10);
				RegSetValueEx(hKey, "DriverDesc", 0, REG_SZ, "G8BPQ Virtual Serial Port", 28); 
				RegSetValueEx(hKey, "EnumPropPages", 0, REG_SZ, "serialui.dll,EnumPropPages", 27); 
				RegSetValueEx(hKey, "PortDriver", 0, REG_SZ, "bpqvcomm.vxd", 13	);
				RegSetValueEx(hKey, "PortSubClass", 0, REG_BINARY, (unsigned char *)&SubClass, 1);

				wsprintf(Driver, "Ports\\%04d", i);

				return;

			}

			retCode = RegCloseKey (hKey);
		}

		i++;

	} while (i < 1100);		// Protect
	

	MessageBox (MainWnd, "Add Software Key Failed","",0);
	return;

}


BYTE ForcedConfig[]={00,04,00,00,00,00,00,00,0x20,00,00,00,02,00,00,00,01,00,02,00,
  05,01,05,01,00,00,00,00,0xf0,0xff,02,00,00,01,00,00,00,00,00,00,00,00,00,00};

BYTE ConfigFlags[] = {04,00,00,00};

BYTE Capabilities[] = {0x14,00,00,00};


VOID AddDevice98(int id)
{

	HKEY hKey ;
	int retCode, i=0, len, Slot;
	int  disp;
	char Key[100];
	char Val[40];


	for (Slot=0; Slot<20; Slot++)
	{
		if (Ports[Slot] == id)
		{

			MessageBox (MainWnd, "Port already present","",0);
			return;
		}
		if (Ports[Slot] == 0)
			break;
		
	}

	if (Slot == 20)
	{

		MessageBox (MainWnd, "You already have the maximum of 20 ports","",0);
		return;
	}


	GetDriverKey98();

	if (!Get98DriverTimeStamp())
	{
		MessageBox (MainWnd, "Can't open bpqvcomm.vxd\nTry loading the Driver","",0);
		return;
	}


	if (strlen(Driver) == 0)
		 CreateDriverKey98();

	i = 1;

	do {
		
		wsprintf(Key, "Enum\\Root\\PORTS\\%04d", i);

		retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
			Key,
            0,	// Reserved
			0,	// Class
			0,	// Options
            KEY_ALL_ACCESS,
			NULL,	// Security Attrs
            &hKey,
			&disp);

		if (retCode == ERROR_SUCCESS)
		{
			if (disp == REG_CREATED_NEW_KEY)
			{				
				// Created, so OK

	

				RegSetValueEx(hKey, "Capabilities", 0, REG_BINARY, Capabilities,4);

				RegSetValueEx(hKey, "Class", 0, REG_SZ, "Ports", 6);

				RegSetValueEx(hKey, "ClassGUID", 0, REG_SZ, "{4d36e978-e325-11ce-bfc1-08002be10318}", 39);

				RegSetValueEx(hKey, "ConfigFlags", 0, REG_BINARY, (unsigned char *)&ConfigFlags, 4);
				
				len = wsprintf(Val, "G8BPQ Virtual Serial Port (COM%d)", id);
				RegSetValueEx(hKey, "DeviceDesc", 0, REG_SZ, Val, len + 1);
  				RegSetValueEx(hKey, "FRIENDLYNAME", 0, REG_SZ, Val, len + 1);

				RegSetValueEx(hKey, "Driver", 0, REG_SZ, Driver, strlen(Driver) + 1);

				ForcedConfig[20] = id;
				ForcedConfig[22] = id;

				RegSetValueEx(hKey, "ForcedConfig", 0, REG_BINARY, (unsigned char  *)&ForcedConfig, 44);

				len = wsprintf(Val, "COM%d", id);
				RegSetValueEx(hKey, "PORTNAME", 0, REG_SZ, Val, len+1);

				RegSetValueEx(hKey, "Mfg", 0, REG_SZ, "G8BPQ", 6);
				
				Ports[Slot]=id;
				Refresh();

							
  				wsprintf(Msg,"Port COM%d Added",id);
				OutputLine(Msg);

				return;

			}

			retCode = RegCloseKey (hKey);
		}

		i++;

	} while (i < 100);		// Protect
	

	MessageBox (MainWnd, "Add Port Failed","",0);
	return;

}


DeleteDeviceXP(int id)
{
	HANDLE hControl;
	int Errorval, resp, i;
	HKEY hKey=0;

	char Msg[256];


	hControl = BPQOpenSerialControl(&Errorval);

	if (hControl == (HANDLE) -1)
	{
		MessageBox (MainWnd, "BPQ Virtual COM Driver Control Channel Open Failed\nIs the Driver loaded?","",0);
		return (0);
	}
           
	resp = BPQSerialDeleteDevice(hControl, &id, &Errorval);

	CloseHandle(hControl);

	if (Errorval == 0)
	{
		for (i=0; i<20; i++)
		{
			if (Ports[i] == id)
			{
				Ports[i] = 0;
				Refresh();
				break;
			}
		}
	}
	else
	{
		MessageBox (MainWnd, "Delete Port Failed","",0);
		return 0;
	}
	wsprintf(Msg,"Port COM%d Deleted",id);
	OutputLine(Msg);

	return 0;

}



VOID DeleteDevice98(int id)
{

	int retCode, i=0, Slot;
	char Key[100];


	for (Slot=0; Slot<20; Slot++)
	{
		if (Ports[Slot] == id)
		{
			break;
		}
	}

	if (Slot == 20)
	{

		MessageBox (MainWnd, "Port not found","",0);
		return;
	}

	wsprintf(Key, "Enum\\Root\\PORTS\\%04d", PortKey[Slot]);

	retCode = RegDeleteKey(HKEY_LOCAL_MACHINE, Key);

	if (retCode == ERROR_SUCCESS)
	{
		wsprintf(Msg,"Port COM%d Deleted",id);
		OutputLine(Msg);
		Ports[Slot] = 0;
		Refresh();
	}
	else
		MessageBox (MainWnd, "Delete Port Failed","",0);
		
	return;
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
	int wmId, wmEvent, id, disp, retCode, i;
	HANDLE hFile;
	HKEY hKey=0;
	char szPort[15];
	char Errmsg[500];
	char Msg[256];

	switch (message)
	{

	case WM_COMMAND:
		wmId    = LOWORD(wParam);

		wmEvent = HIWORD(wParam);


		// Parse the menu selections:


		switch (wmId)
		{
		case IDLOAD:

			return LoadDriver();

		case IDUNLOAD:

			return ManageDriver(DRIVERNAME, driverLocation,DRIVER_FUNC_REMOVE);

		case IDC_REFRESH:

			GetPorts();
			return 0;

		case IDADD:

			id=SendDlgItemMessage(MainWnd, IDC_COMBO1, CB_GETCURSEL, 0, 0);
			id++;

			// see if device exists
			
			wsprintf( szPort, "//./COM%d", id) ;
			
			hFile = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
			if (hFile == (HANDLE) -1 )
			{
				i=GetLastError();

				if (i == ERROR_FILE_NOT_FOUND)
					goto OKtoADD;
			}
			else
				CloseHandle(hFile);

			MessageBox (MainWnd, "Port Already Exists","",0);
			return (0);

		OKtoADD:

			if (Win98)	AddDevice98(id); else AddDeviceXP(id);

			return 0;

		case IDDELETE:


			id=SendDlgItemMessage(MainWnd, IDC_COMBO1, CB_GETCURSEL, 0, 0);
			id++;

			if (Win98)	DeleteDevice98(id); else DeleteDeviceXP(id);



		case IDSAVE:
			
			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SYSTEM\\CurrentControlSet\\Services\\BPQVirtualCOM",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

			if (retCode != ERROR_SUCCESS)
			{
				format_error(Msg, 256, retCode);
				wsprintf(Errmsg,"Open Key %s failed Error %d %s",
					"SYSTEM\\CurrentControlSet\\Services\\BPQVirtualCOM", retCode, Msg);
				MessageBox (hWnd,Errmsg,"VCOM Config",0);
				return 0;
			}

			retCode = RegSetValueEx(hKey,"Ports",0,REG_BINARY,(BYTE *)&Ports,20);
 
			retCode = RegCloseKey (hKey);

			return 0;
      
	
		default:

			return 0;

		}


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
	return TRUE;
}


void format_error (char *msg, int msglen, int errnum)
{
  if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM
                     | FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL,
                     errnum,
                     MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (LPTSTR) msg,
                     msglen,
                     NULL))
    {
      msg [strlen (msg) - 2] = '\0';
    }
  else
    {
      sprintf (msg, "Win32 error %d", errnum);
    }
}


BOOL Get98DriverTimeStamp()
{
	HANDLE handle;
	int DirLen;

	DirLen = GetSystemDirectory(SysDriverName, MAX_PATH);
  

	strcat(SysDriverName, "\\");
	strcat(SysDriverName, DRIVERFILENAME);


	handle = CreateFile(SysDriverName,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (handle != INVALID_HANDLE_VALUE)
	{

		GetFileTime(handle, NULL, NULL, &OldLastWriteTime);
		FileTimeToSystemTime(&OldLastWriteTime, &Time);
		CloseHandle(handle);
		return TRUE;
	}

	return FALSE;

}


VOID CopyDriver98()
{

	Get98DriverTimeStamp();


	if (CompareFileTime(&OldLastWriteTime, &NewLastWriteTime) > 0)

		if(MessageBox (MainWnd, "New Driver is older - do you want to continue?",
				"",MB_YESNO) == IDNO)
				return;


	if (CopyFile(driverLocation, SysDriverName, FALSE))

		wsprintf(Msg,"Driver Installed");

	else
		wsprintf(Msg,"Unable to install driver");


	OutputLine(Msg);

}




VOID OutputLine(char * Msg)
{
	int i;
	i=SendDlgItemMessage(MainWnd,IDC_LIST,LB_ADDSTRING,0,(LPARAM) Msg);
	SendDlgItemMessage(MainWnd,IDC_LIST,LB_SETCARETINDEX,(WPARAM) i, MAKELPARAM(TRUE, 0));

	return;
}

VOID ReportError(char * Err, int i)
{
	char Msg[256];
	char Line[500];
	int index;

	format_error (Msg, 256, i);
	wsprintf(Line,"%s Error %d=%s",Err,i,Msg);

	index=SendDlgItemMessage(MainWnd,IDC_LIST,LB_ADDSTRING,0,(LPARAM) Line);
	SendDlgItemMessage(MainWnd,IDC_LIST,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(TRUE, 0));

	return;

}

BOOLEAN
ManageDriver(
    IN LPCTSTR  DriverName,
    IN LPCTSTR  ServiceName,
    IN USHORT   Function
    );

BOOLEAN
SetupDriverName(
    PUCHAR DriverLocation
    );


BOOLEAN
InstallDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName,
    IN LPCTSTR    ServiceExe
    );


BOOLEAN
RemoveDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );

BOOLEAN
StartDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );

BOOLEAN
StopDriver(
    IN SC_HANDLE  SchSCManager,
    IN LPCTSTR    DriverName
    );



HKEY hKey=0;
HKEY hSubKey=0;
int retCode,Type,Vallen=99;
UCHAR Value[100];

//char Key[]="SYSTEM\\CurrentControlSet\\Services";
//char Key[]="SOFTWARE\\G8BPQ";

int LoadDriver()
{

	// Loads BPQ Virtual COMM Driver


    // Make sure driver exists
	
	if (!SetupDriverName(driverLocation))
	{

        return 0;
    }
       
	if(Win98)
	{
		CopyDriver98();
	}
	else
	{
		if (!ManageDriver(DRIVERNAME,
                          driverLocation,
                          DRIVER_FUNC_INSTALL
                          ))
		{
            wsprintf(Msg,"Unable to install driver");
			OutputLine(Msg);

            //
            // Error - remove driver.
            //

            ManageDriver(DRIVERNAME,
                         driverLocation,
                         DRIVER_FUNC_REMOVE
                         );
            
            return 0;
		}
	}
            
	return 0;
 
}

BOOLEAN InstallDriver(SC_HANDLE  SchSCManager, LPCTSTR    DriverName, LPCTSTR    ServiceExe)

{
    SC_HANDLE   schService;
    DWORD       err;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    //
    // Create a new a service object.
    //

    schService = CreateService(SchSCManager,           // handle of service control manager database
                               DriverName,             // address of name of service to start
                               DriverName,             // address of display name
                               SERVICE_ALL_ACCESS,     // type of access to service
                               SERVICE_KERNEL_DRIVER,  // type of service
                               SERVICE_AUTO_START,	   // when to start service
                               SERVICE_ERROR_NORMAL,   // severity if service fails to start
                               ServiceExe,             // address of name of binary file
                               "Base",                 // service group
                               NULL,                   // no tag requested
                               NULL,                   // no dependency names
                               NULL,                   // use LocalSystem account
                               NULL                    // no password for service account
                               );

    if (schService == NULL) {

        err = GetLastError();

        if (err == ERROR_SERVICE_EXISTS) {

            //
            wsprintf(Msg,"Driver Service Exists");
			OutputLine(Msg);

            return TRUE;
            
        } else {

           ReportError("CreateService failed", err );	

            //
            // Indicate an error.
            //

            return  FALSE;
        }
    }

    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle(schService);
    }

    wsprintf(Msg,"Driver Service Created");
	OutputLine(Msg);

    //
    // Indicate success.
    //

    return TRUE;

}   // InstallDriver

BOOLEAN ManageDriver(IN LPCTSTR DriverName, IN LPCTSTR ServiceName, IN USHORT Function)
{
    SC_HANDLE   schSCManager;
    BOOLEAN rCode = TRUE;

    //
    // Connect to the Service Control Manager and open the Services database.
    //

    schSCManager = OpenSCManager(NULL,                   // local machine
                                 NULL,                   // local database
                                 SC_MANAGER_ALL_ACCESS   // access required
                                 );

    if (!schSCManager) {

         ReportError("Open SC Manager failed", GetLastError());

        return FALSE;
    }

    //
    // Do the requested function.
    //

    switch( Function ) {

        case DRIVER_FUNC_INSTALL:

            //
            // Install the driver service.
            //

            if (InstallDriver(schSCManager,
                              DriverName,
                              ServiceName
                              )) {

                //
                // Start the driver service (i.e. start the driver).
                //

                rCode = StartDriver(schSCManager,
                                    DriverName
                                    );


            } else {

                //
                // Indicate an error.
                //

                rCode = FALSE;
            }

            break;

        case DRIVER_FUNC_REMOVE:

            //
            // Stop the driver.
            //

            if (!StopDriver(schSCManager,DriverName))
				return FALSE;

            //
            // Remove the driver service.
            //

            if (!RemoveDriver(schSCManager,DriverName))
				return FALSE;

			wsprintf(Msg,"Driver Removed");
			OutputLine(Msg);


            rCode = TRUE;

            break;

        default:

            rCode = FALSE;

            break;
    }

    //
    // Close handle to service control manager.
    //

    if (schSCManager) CloseServiceHandle(schSCManager);

    return rCode;

}   // ManageDriver


BOOLEAN
RemoveDriver(
    IN SC_HANDLE    SchSCManager,
    IN LPCTSTR      DriverName
    )
{
    SC_HANDLE   schService;
    BOOLEAN     rCode;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

         ReportError("OpenService failed", GetLastError());
		 OutputLine(Msg);

        //
        // Indicate error.
        //

        return FALSE;
    }

    //
    // Mark the service for deletion from the service control manager database.
    //

    if (DeleteService(schService)) {

        //
        // Indicate success.
        //

        rCode = TRUE;

    } else {

        ReportError("DeleteService failed", GetLastError());

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //

        rCode = FALSE;
    }

    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle(schService);
    }

    return rCode;

}   // RemoveDriver



BOOLEAN
StartDriver(
    IN SC_HANDLE    SchSCManager,
    IN LPCTSTR      DriverName
    )
{
    SC_HANDLE   schService;
    DWORD       err;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

        ReportError("OpenService failed", GetLastError());
		OutputLine(Msg);

        //
        // Indicate failure.
        //

        return FALSE;
    }

    //
    // Start the execution of the service (i.e. start the driver).
    //

    if (!StartService(schService,     // service identifier
                      0,              // number of arguments
                      NULL            // pointer to arguments
                      )) {

        err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING) {

             wsprintf(Msg,"Service Already Running");
			 OutputLine(Msg);

            return TRUE;

        } else {

            ReportError("StartService failure", err );

            //
            // Indicate failure.  Fall through to properly close the service handle.
            //

            return FALSE;
        }    
	}
	else
	{
		wsprintf(Msg,"Service Started");
		OutputLine(Msg);
	}


    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle(schService);
    }

    return TRUE;

}   // StartDriver



BOOLEAN StopDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName)
{
    BOOLEAN         rCode = TRUE;
    SC_HANDLE       schService;
    SERVICE_STATUS  serviceStatus;

    //
    // Open the handle to the existing service.
    //

    schService = OpenService(SchSCManager,
                             DriverName,
                             SERVICE_ALL_ACCESS
                             );

    if (schService == NULL) {

		ReportError("OpenService failed", GetLastError());

        return FALSE;
    }

    //
    // Request that the service stop.
    //

    if (ControlService(schService,
                       SERVICE_CONTROL_STOP,
                       &serviceStatus
                       )) {

        //
        // Indicate success.
        //

        rCode = TRUE;

    } else {

        ReportError("ControlService failed", GetLastError() );

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //

        rCode = FALSE;
    }

    //
    // Close the service object.
    //

    if (schService) {

        CloseServiceHandle (schService);
    }

    return rCode;

}   //  StopDriver

BOOLEAN SetupDriverName(PUCHAR DriverLocation)
{
    HANDLE fileHandle;

    DWORD driverLocLen = 0;

    //
    // Get the current directory.
    //

    driverLocLen = GetCurrentDirectory(MAX_PATH,DriverLocation);

    if (!driverLocLen) {

        wsprintf(Msg,"GetCurrentDirectory failed" );
		OutputLine(Msg);

        return FALSE;
    }

    //
    // Setup path name to driver file.
    //

	strcat(DriverLocation, "\\");
	strcat(DriverLocation, DRIVERFILENAME);


    //
    // Insure driver file is in the specified directory.
    //

    if ((fileHandle = CreateFile(DriverLocation,
                                 GENERIC_READ,
                                 0,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL
                                 )) == INVALID_HANDLE_VALUE) {


	    driverLocLen = GetCurrentDirectory(MAX_PATH,DriverLocation);
        wsprintf(Msg, "%s is not in %s", DRIVERFILENAME, DriverLocation );
		OutputLine(Msg);
        //
        // Indicate failure.
        //

        return FALSE;
    }

    //
    // Close open file handle.
    //


	GetFileTime(fileHandle, NULL, NULL, &NewLastWriteTime);
	FileTimeToSystemTime(&OldLastWriteTime, &Time);
	

    if (fileHandle) {

        CloseHandle(fileHandle);
    }

    //
    // Indicate success.
    //

    return TRUE;


}   // SetupDriverName




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


HANDLE BPQOpenSerialControl(ULONG * lasterror)
{            
	HANDLE hDevice;

	*lasterror=0;
	
	hDevice = CreateFile( "\\\\.\\BPQControl", GENERIC_READ | GENERIC_WRITE,
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
int BPQSerialAddDevice(HANDLE hDevice,ULONG * port,ULONG * result)
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
	char szPort[ 15 ];
	HANDLE hDevice;

  // load the COM prefix string and append port number
   
	*lasterror=0;
	
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

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(	hDevice,IOCTL_SERIAL_SET_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialClrCTS(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}
int BPQSerialClrDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialClrDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen)
{
	ULONG bytesReturned;

	if (MsgLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	return DeviceIoControl(hDevice,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen)
{
	if (BufLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	return DeviceIoControl(hDevice,IOCTL_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL);
                  
}


int BPQSerialGetDeviceList(HANDLE hDevice,ULONG * Slot,ULONG * Port)
{
	ULONG bytesReturned;

	return  DeviceIoControl (hDevice,IOCTL_BPQ_LIST_DEVICES,Slot,4,Port,4,&bytesReturned,NULL);

}


int BPQSerialIsCOMOpen(HANDLE hDevice,ULONG * Count)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
}
int BPQSerialGetDTRRTS(HANDLE hDevice,ULONG * Flags)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
}

