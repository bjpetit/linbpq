//
//	409l	Oct 2001 Fix l3timeout for KISS
//
//	409m	Oct 2001 Fix Crossband Digi
//
//	409n	May 2002 Change error handling on load ext DLL

//	409p	March 2005 Allow Multidigit COM Ports (kiss.c)

//	409r	August 2005 Treat NULL string in Registry as use current directory
//						Allow shutdown to close BPQ Applications

//	409s	October 2005 Add DLLExport entries to API for BPQTNC2

//	409t	January 2006
//
//			Add API for Perl "GetPerlMsg"
//			Add	API for BPQ1632	"GETBPQAPI"	- returns address of Assembler API routine
//			Add Registry Entry "BPQ Directory". If present, overrides "Config File Location"
//			Add New API "GetBPQDirectory" - Returns location of config file
//			Add New API "ChangeSessionCallsign" - equivalent to "*** linked to" command
//			Rename BPQNODES to BPQNODES.dat
//			New API "GetAttachedProcesses" - returns number of processes connected.
//			Warn if user trys to close Console Window.
//			Add Debug entries to record Process Attach/Detach
//			Fix recovery following closure of first process

//	409t Beta 2	February 2006
//
//			Add API Entry "GetPortNumber"
//
//	409u	February 2006
//
//			Fix crash if allocate/deallocate called with stream=0
//			Add API to ch
//			Display config file path
//			Fix saving of Locked Node flag
//			Added SAVENODES SYSOP command
//
//	409u 2	March 2006
//
//			Fix SetupBPQDirectory
//			Add CopyBPQDirectory (for Basic Programs)
//
//	409u 3	March 2006
//
//			Release streams on DLL unload

//	409v	October 2006
//
//			Support Minimize to Tray for all BPQ progams
//			Implement L4 application callsigns

//	410		November 2006
//
//			Modified to compile with C++ 2005 Express Edition
//			Make MCOM MTX MMASK local variables
//
// 410a		January 2007
//
//			Add program name to Attach-Detach messages
//			Attempt to detect processes which have died 
//			Fix bug in NETROM and IFrame decode which would cause crash if frame was corrupt
//			Add BCALL - origin call for Beacons
//			Fix KISS ACKMODE ACK processing
//

//	410b	November 2007
//
//			Allow CTEXT of up to 510, and enforce PACLEN, fragmenting if necessary

// 410c		December 2007

//				Fix problem with NT introduced in V410a
//				Display location of DLL on Console

// 410d		January 2008

//				Fix crash in DLL Init caused by long path to program
//				Invoke Appl2 alias on C command (if enabled)
//				Allow C command to be disabled
//				Remove debug trap in GETRAWFRAME
//				Validate Alias of directly connected node, mainly for KPC3 DISABL Problem
//				Move Port statup code out of DLLInit (mainly for perl)
//				Changes to allow Load/Unload of bpq32.dll by appl
//				CloseBPQ32 API added
//				Ext Driver Close routes called
//				Changes to release Mutex

// 410e		May 2008

//				Fix missing SSID on last call of UNPROTO string (CONVTOAX25 in main.asm)
//				Fix VCOM Driver (RX Len was 1 byte too long)
//				Fix possible crash on L4CODE if L4DACK received out of sequence
//				Add basic IP decoding

// 410f		October 2008

//				Add IP Gateway
//				Add Multiport DIGI capability
//				Add GetPortDescription API
//				Fix potential hangs if RNR lost
//				Fix problem if External driver failes to load
//				Put pushad/popad round _INITIALISEPORTS (main.asm)
//				Add APIs GetApplCallVB and GetPortDescription (mainly for RMS)
//				Ensure Route Qual is updated if Port Qual changed
//				Add Reload Option, plus menu items for DUMP and SAVENODES

// 410g		December 2008

//				Restore API Exports BPQHOSTAPIPTR and MONDECODEPTR (accidentally deleted)
//				Fix changed init of BPQDirectory (accidentally changed)
//				Fix Checks for lost processes (accidentally deleted)
//				Support HDLC Cards on W2K and above
//				Delete Tray List entries for crashed processes
//				Add Option to NODES command to sort by Callsign
//				Add options to save or clear BPQNODES before Reconfig.
//				Fix Reconfig in Win98
//				Monitor buffering tweaks
//				Fix Init for large (>64k) tables
//				Fix Nodes count in Stats

// 410h		January 2009

//				Add Start Minimized Option
//				Changes to KISS for WIn98 Virtual COM
//					Open \\.\com instead of //./COM
//					Extra Dignostics

// 410i		Febuary 2009

//				Revert KISS Changes
//				Save Window positions

// 410j		June 2009

//				Fix tidying of window List when program crashed
//				Add Max Nodes to Stats
//				Don't update APPLnALIAS with received NODES info
//				Fix MH display in other timezones
//				Fix Possible crash when processing NETROM type Zero frames (eg NRR)
//				Basic INP3 Stuff
//				Add extra diagnostics to Lost Process detection
//				Process Netrom Record Route frames.

// 410k		June 2009

//				Fix calculation of %retries in extended ROUTES display
//				Fix corruption of ROUTES table

// 410l		October 2009

//				Add GetVersionString API call.
//				Add GetPortTableEntry API call
//				Keep links to neighbouring nodes open

// Build 2

//				Fix PE in NOROUTETODEST (missing POP EBX)

// 410m		November 2009

//				Changes for PACTOR and WINMOR to support the ATTACH command
//				Enable INP3 if configured on a route.
//				Fox count of nodes in Stats Display
//				Overwrite the worst quality unused route if a call is received from a node not in your
//				table when the table is full

//	Build 5

//	Rig Control Interface
//	Limit KAM VHF attach and RADIO commands to authorised programs (MailChat and BPQTerminal)

// Build 6

// Fix reading INP3 Flag from BPQNODES

// Build 7

//	Add MAXHOPS and MAXRTT config options

// Build 8

// Fix INP3 deletion of Application Nodes.
// Fix GETCALLSIGN for Pactor Sessions
// Add N Call* to display all SSID's of a call
// Fix flow control on Pactor sessions.

// Build 9

// HDLC Support for XP
// Add AUTH routines

// Build 10

// Fix handling commands split over more that one packet.

// Build 11

// Attach cmd changes for winmor disconnecting state
// Option Interlock Winmor/Pactor ports

// Build 12

// Add APPLS export for winmor
// Handle commands ending CR LF

// Build 13

// Incorporate Rig Control in Kernel

// Build 14

// Fix config reload for Rig COntrol 

// 410n		March 2010

// Implement C P via PACTOR/WINMOR (for Airmail)

// Build 2

// Don't flip SSID bits on Downlink Connect if uplink is Pactor/WINMOR
// Fix resetting IDLE Timer on Pactor/WINMOR sessions
// Send L4 KEEPLI messages based on IDLETIME

// 410o		July 2010

// Read bpqcfg.txt instead of .bin
// Support 32 bit MMASK (Allowing 32 Ports)
// Support 32 bit APPLMASK (Allowing 32 Applications)
// Allow more commands
// Allow longer command aliases
// Fix logic error in RIGControl Port Initialisation (wasn't always raising RTS and DTR
// Clear RIGControl RTS and DTR on close

// 410o		Build 2 August 2010

// Fix couple of errors in config (needed APPLICATIONS and BBSCALL/ALIAS/QUAL)
// Fix Kenwood Rig Control when more than one message received at once.
// Save minimzed state of Rigcontrol Window

// 410o		Build 3 August 2010

// Fix reporting of set errors in scan to a random session

// 410o		Build 4 August 2010

// Change All xxx Ports are in use to no xxxx Ports are available if there are no sessions with applmask
// Fix validation of TRANSDELAY

// 410o		Build 5 August 2010

//	Add Repeater Shift and Set Data Mode options to Rigcontrol (for ICOM only)
//	Add WINMOR and SCS Pactor mode control option to RigControl
//  Extend INFOMSG to 2000 bytes
//  Improve Scan freq change lock (check both SCS and WINMOR Ports)

// 410o		Build 6 September 2010

// Incorporate IPGateway in main code.
// Fix GetSessionInfo for Pactor/Winmor Ports
// Add Antenna Selection to RigControl
// Allow Bandwidth options on RADIO command line (as well as in Scan definitions)

// 410o		Build 7 September 2010

// Move rigcontol display to driver windows
// Move rigontrol config to driver config.
// Allow driver and IPGateway config info in bpq32.cfg
// Move IPGateway, AXIP, VKISS, AGW and WINMOR drivers into bpq32.dll
// Add option to reread IP Gateway config.
// Fix Reinit after process with timer closes (error in TellSessions).

// 410p		Build 2 October 2010

// Move KAM and SCS drivers to bpq32.dll

// 410p		Build 3 October 2010

// Support more than one axip port.

// 410p		Build 4 October 2010

// Dynamically load psapi.dll (for 98/ME)

// 410p		Build 5 October 2010

// Incorporate TelnetServer
// Fix AXIP ReRead Config
// Report AXIP accept() fails to syslog, not a popup.

// 410p		Build 6 October 2010

// Includes HAL support
// Changes to Pactor Drivers disconnect code
// AXIP now sends with source port = dest port, unless overridden by SOURCEPORT param
// Config now checks for duplicate port definitions
// Add Node Map reporting
// Fix WINMOR deferred disconnect.
// Report Pactor PORTCALL to WL2K instead of RMS Applcall

// 410p		Build 7 October 2010

// Add In/Out flag to Map reporting, and report centre, not dial


// Add NOKEEPALIVES Port Param

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#pragma data_seg("_BPQDATA")

#include "windows.h"

#include "time.h"
#include "stdio.h"
#include "io.h"
#include <fcntl.h>					 
//#include "vmm.h"
#include "SHELLAPI.H"

#include "AsmStrucs.h"

#define SPECIALVERSION "Test 1"

#include "GetVersion.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

#define	CHECKLOADED		  0
#define	SETAPPLFLAGS	  1
#define	SENDBPQFRAME	  2
#define	GETBPQFRAME	      3
#define	GETSTREAMSTATUS	  4
#define	CLEARSTREAMSTATUS 5
#define	BPQCONDIS		  6
#define	GETBUFFERSTATUS	  7
#define	GETCONNECTIONINFO 8
#define	BPQRETURN		  9  // GETCALLS
//#define	RAWTX			  10  //IE KISS TYPE DATA
#define	GETRAWFRAME		  11
#define	UPDATESWITCH	  12
#define	BPQALLOC		  13
//#define	SENDNETFRAME	  14
#define	GETTIME			  15

extern short NUMBEROFPORTS;
extern long PORTENTRYLEN;
extern long LINKTABLELEN;
extern struct PORTCONTROL * PORTTABLE;
extern UINT FREE_Q;

DllExport struct TRANSPORTENTRY * L4TABLE = 0;
DllExport UCHAR NEXTID = 55;
DllExport WORD MAXCIRCUITS = 50;
DllExport UCHAR L4DEFAULTWINDOW = 4;
DllExport WORD L4T1 = 60;
DllExport struct APPLCALLS APPLCALLTABLE[NumberofAppls];
DllExport char * APPLS;

UINT WINAPI VCOMExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI AXIPExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI SCSExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI AEAExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI KAMExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI HALExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI ETHERExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI AGWExtInit(struct PORTCONTROL *  PortEntry);
UINT WINAPI WinmorExtInit(EXTPORTDATA * PortEntry);
UINT WINAPI TelnetExtInit(EXTPORTDATA * PortEntry);

extern char AUTOSAVE;

extern char MYNODECALL;	// 10 chars,not null terminated
extern char SIGNONMSG;

extern QCOUNT; 
extern struct BPQVECSTRUC BPQHOSTVECTOR[];
extern struct BPQVECSTRUC IPHOSTVECTOR;
extern char * CONFIGFILENAME;

DllExport struct BPQVECSTRUC * BPQHOSTVECPTR;

extern int BPQHOSTAPI();
extern int START();
extern int INITIALISEPORTS();
extern int TIMERINTERRUPT();
extern int MONDECODE();
extern int BPQMONOPTIONS();
extern char PWTEXT[];
extern char PWLEN;
extern int SEMGETS;
extern int SEMRELEASES;
extern int SEMCLASHES;
extern int FINDFREEDESTINATION();
extern int RAWTX();
extern int RELBUFF();
extern int SENDNETFRAME();
extern char MYCALL[];			// 7 chars, ax.25 format


char LOCATOR[10] = "";			// Maidenhead Locator for Reporting
int AXIPPort = 0;				// Port to report to
char ReportDest[7];

VOID __cdecl Debugprintf(const char * format, ...);

DllExport int APIENTRY CloseBPQ32();

//BOOL (FAR WINAPI * Init_IP) ();
//BOOL (FAR WINAPI * Poll_IP) ();

BOOL APIENTRY Init_IP();
BOOL APIENTRY Poll_IP();

BOOL APIENTRY Rig_Init();
BOOL APIENTRY Rig_Close();
BOOL APIENTRY Rig_Poll();
BOOL APIENTRY Rig_Command();

VOID IPClose();

int Flag = (int) &Flag;			//	 for Dump Analysis
int MAJORVERSION=4;
int MINORVERSION=9;

int Semaphore = 0;
int SemProcessID = 0;
int SemHeldByAPI = 0;
UINT Sem_eax = 0;
UINT Sem_ebx = 0;
UINT Sem_ecx = 0;
UINT Sem_edx = 0;
UINT Sem_esi = 0;
UINT Sem_edi = 0;

struct _DATABASE * DataBase  = (struct _DATABASE *)&DATABASE;

DllExport long  BPQHOSTAPIPTR = (long)&BPQHOSTAPI;
DllExport long  MONDECODEPTR = (long)&MONDECODE;

UCHAR BPQDirectory[MAX_PATH]="";

static char BPQWinMsg[] = "BPQWindowMessage";

UINT BPQMsg=0;

#define MAXLINELEN 120
#define MAXSCREENLEN 50


int LINELEN=120;
int SCREENLEN=50;

char Screen[MAXLINELEN*MAXSCREENLEN]={0};

int lineno=0;
int col=0;

#define REPORTINTERVAL 15 * 549;	// Magic Ticks Per Minute for PC's nominal 100 ms timer 
int ReportTimer = 0;

HANDLE OpenConfigFile(char * file);

VOID SetupBPQDirectory();
VOID SendLocation();

unsigned long _beginthread(void(*start_address)(), unsigned stack_size, int arglist);

#define TRAY_ICON_ID	1		//				ID number for the Notify Icon
#define MY_TRAY_ICON_MESSAGE	WM_APP	//		the message ID sent to our window

NOTIFYICONDATA niData; 

int SetupConsoleWindow();

BOOL StartMinimized=FALSE;
BOOL MinimizetoTray=TRUE;

HMENU trayMenu=0;

HWND hWnd;

static RECT Rect;			// Window Position

DllExport int APIENTRY DumpSystem();
DllExport int APIENTRY SaveNodes ();
DllExport int APIENTRY ClearNodes ();
DllExport int APIENTRY SetupTrayIcon();

VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

DllExport int APIENTRY DeallocateStream(int stream);

int VECTORLENGTH = sizeof bpqvecstruc;

int InitDone = 0;
int FirstInitDone = 0;
int PerlReinit = 0;
int TimerHandle = 0;
unsigned int TimerInst = 0xffffffff;
;
int AttachedProcesses = 0;
int AttachedPerlProcesses = 0;
int AttachingProcess = 0;
HINSTANCE hIPModule = 0;
HINSTANCE hRigModule = 0;

BOOL ReconfigFlag=FALSE;

int AttachedPIDList[100] = {0};
int AttachedPIDType[100] = {0};

HWND hWndArray[100] = {0};
int PIDArray[100] = {0};
char PopupText[30][100] = {""};

// Next 3 should be uninitialised so they are local to each process

byte	MCOM;
char	MTX;
ULONG	MMASK;

UCHAR AuthorisedProgram;			// Local Variable. Set if Program is on secure list

BOOL	Perl;
HANDLE Mutex;

TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;

BOOL ProcessConfig();
VOID FreeConfig();

DllExport int APIENTRY WritetoConsole(char * buff);

BOOLEAN CheckifBPQ32isLoaded();
BOOLEAN StartBPQ32();
DllExport VOID APIENTRY  Send_AX(VOID * Block, DWORD len, UCHAR Port);
BOOL LoadIPDriver();
BOOL Send_IP(VOID * Block, DWORD len);
VOID CheckforLostProcesses();
BOOL LoadRigDriver();

BOOL IPActive = FALSE;
BOOL IPRequired = FALSE;
BOOL RigRequired = TRUE;
BOOL RigActive = FALSE;

Tell_Sessions();

void GetSemaphore()
{
	//
	//	Wait for it to be free
	//
	
	if (Semaphore !=0)
		SEMCLASHES++;

loop1:

	while (Semaphore != 0)
	{
		Sleep(10);
	}

	//
	//	try to get semaphore
	//

	_asm{

	mov	eax,1
	xchg Semaphore,eax	// this instruction is locked
	
	cmp	eax,0
	jne loop1			// someone else got it - try again
;
;	ok, we've got the semaphore
;
	}

	SEMGETS++;
	SemProcessID=GetCurrentProcessId();

	return;
}
void FreeSemaphore()
{
	SEMRELEASES++;

	SemHeldByAPI = 0;

	Semaphore=0;

	return; 
}

#include <tlhelp32.h>

typedef  int (WINAPI FAR *FARPROCX)();

FARPROCX CreateToolHelp32SnapShotPtr;
FARPROCX Process32Firstptr;
FARPROCX Process32Nextptr;

void LoadToolHelperRoutines()
{
	HINSTANCE ExtDriver=0;
	int err;
	char msg[100];

	ExtDriver=LoadLibrary("kernel32.dll");

	if (ExtDriver == NULL)
	{
		err=GetLastError();

		wsprintf(msg,"BPQ32 Error loading kernel32.dll - Error code %d\n", err);
		
		OutputDebugString(msg);

		return;
	}

	CreateToolHelp32SnapShotPtr = (FARPROCX)GetProcAddress(ExtDriver,"CreateToolhelp32Snapshot");
	Process32Firstptr = (FARPROCX)GetProcAddress(ExtDriver,"Process32First");
	Process32Nextptr = (FARPROCX)GetProcAddress(ExtDriver,"Process32Next");
	
	if (CreateToolHelp32SnapShotPtr == 0)
	{
		err=GetLastError();

		wsprintf(msg,"BPQ32 Error getting CreateToolhelp32Snapshot entry point - Error code %d\n", err);
		
		OutputDebugString(msg);

		return;
	}

}

BOOL GetProcess(int ProcessID, char * Program)
{
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;
  int p;

   if (CreateToolHelp32SnapShotPtr==0)
   {
	   return (TRUE);	// Routine not available
   }
  // Take a snapshot of all processes in the system.
  hProcessSnap = (HANDLE)CreateToolHelp32SnapShotPtr(TH32CS_SNAPPROCESS, 0);
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    OutputDebugString( "CreateToolhelp32Snapshot (of processes) Failed\n" );
    return( FALSE );
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof( PROCESSENTRY32 );

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if( !Process32Firstptr( hProcessSnap, &pe32 ) )
  {
    OutputDebugString( "Process32First Failed\n" );  // Show cause of failure
    CloseHandle( hProcessSnap );     // Must clean up the snapshot object!
    return( FALSE );
  }

  // Now walk the snapshot of processes, and
  // display information about each process in turn
  do
  {
	if (ProcessID==pe32.th32ProcessID)
	{
		  // if running on 98, program contains the full path - remove it

		for (p=strlen(pe32.szExeFile); p>=0; p--)
		{
			if (pe32.szExeFile[p]=='\\') 
			{
				break;
			}
		}
		p++;		
		  
		wsprintf(Program,"%s", &pe32.szExeFile[p]);
		CloseHandle( hProcessSnap );
		return( TRUE );
	}

  } while( Process32Nextptr( hProcessSnap, &pe32 ) );


  wsprintf(Program,"PID %d Not Found", ProcessID);
  CloseHandle( hProcessSnap );
  return(FALSE);
}

BOOL IsProcess(int ProcessID)
{
	// Check that Process exists

  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;

  if (CreateToolHelp32SnapShotPtr==0) return (TRUE);  // Routine not available

  hProcessSnap = (HANDLE)CreateToolHelp32SnapShotPtr(TH32CS_SNAPPROCESS, 0);

  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    OutputDebugString( "CreateToolhelp32Snapshot (of processes) Failed\n" );
    return(TRUE);		// Don't know, so assume ok
  }

  pe32.dwSize = sizeof( PROCESSENTRY32 );

   if( !Process32Firstptr( hProcessSnap, &pe32 ) )
  {
    OutputDebugString( "Process32First Failed\n" );  // Show cause of failure
    CloseHandle( hProcessSnap );     // Must clean up the snapshot object!
    return(TRUE);		// Don't know, so assume ok
   }

   do
   {
	  if (ProcessID==pe32.th32ProcessID)
	  {
		  CloseHandle( hProcessSnap );
		  return( TRUE );
	  }

  } while( Process32Nextptr( hProcessSnap, &pe32 ) );

  CloseHandle( hProcessSnap );
  return(FALSE);
}

VOID MonitorThread(int x)
{
	
	// Thread to detect killed processes. Runs in process owning timer.

	// Obviously can't detect loss of timer owning thread!

	do {

		Sleep(60000);

		CheckforLostProcesses();

	} while (TRUE);
}

VOID CheckforLostProcesses()
{
	UCHAR buff[100];
	char Log[80];
	int i, n, ProcessID;

	for (n=0; n < AttachedProcesses; n++)
	{
		ProcessID=AttachedPIDList[n];
			
		if (!IsProcess(ProcessID))
		{
			// Process has died - Treat as a detach

			wsprintf(Log,"BPQ32 Process %d Died\n", ProcessID);
			OutputDebugString(Log);

			// Remove Tray Icom Entry

			for( i = 0; i < 100; ++i )
			{
				if (PIDArray[i] == ProcessID)
				{
					hWndArray[i] = 0;
					wsprintf(Log,"BPQ32 Removing Tray Item %s\n", PopupText[i]);
					OutputDebugString(Log);
					DeleteMenu(trayMenu,40000+i,MF_BYCOMMAND);
				}
			}

			// If process ) the semaphore, release it

			if (Semaphore == 1 && ProcessID == SemProcessID)
			{
				OutputDebugString("BPQ32 Process was holding Semaphore - attempting recovery\r\n");
				Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
					Sem_eax, Sem_ebx = 0, Sem_ecx = 0, Sem_edx = 0, Sem_esi = 0, Sem_edi); 

				Semaphore=0;
			}

			for (i=1;i<65;i++)
			{
				if (BPQHOSTVECTOR[i-1].STREAMOWNER == AttachedPIDList[n])
					DeallocateStream(i);
			}
				
			if (TimerInst == ProcessID)
			{
				KillTimer(NULL,TimerHandle);
				TimerHandle=0;
				TimerInst=0xffffffff;
				Tell_Sessions();
				OutputDebugString("BPQ32 Process was running timer \n");
			
				if (MinimizetoTray)
					Shell_NotifyIcon(NIM_DELETE,&niData);
			}
				
			//	Remove this entry from PID List

			AttachedPerlProcesses-=AttachedPIDType[n];

			for (i=n; i< AttachedProcesses; i++)
			{
				AttachedPIDType[i]=AttachedPIDType[i+1];
				AttachedPIDList[i]=AttachedPIDList[i+1];
			}
			AttachedProcesses--;

			wsprintf(buff,"BPQ32 Lost Process - %d Process(es) Attached %d Perl Proceses\n", AttachedProcesses, AttachedPerlProcesses);
			OutputDebugString(buff);
		}
	}
}
VOID MonitorTimerThread(int x)
{	
	// Thread to detect killed timer process. Runs in all other BPQ32 processes.

	do {

		Sleep(60000);

		if (!IsProcess(TimerInst))
		{
			// Timer owning Process has died - Force a new timer to be created
			//	New timer thread will detect lost process and tidy up
		
			OutputDebugString("BPQ32 Process with Timer died\n");

			// If process was holding the semaphore, release it

			if (Semaphore == 1 && TimerInst == SemProcessID)
			{
				OutputDebugString("BPQ32 Process was holding Semaphore - attempting recovery\r\n");
				Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
					Sem_eax, Sem_ebx = 0, Sem_ecx = 0, Sem_edx = 0, Sem_esi = 0, Sem_edi); 
				Semaphore=0;
			}

//			KillTimer(NULL,TimerHandle);
//			TimerHandle=0;
//			TimerInst=0xffffffff;
//			Tell_Sessions();

			CheckforLostProcesses();		// Normally only done in timer thread, which is now dead

//			if (MinimizetoTray)
//				Shell_NotifyIcon(NIM_DELETE,&niData);
		}
	
	} while (TRUE);
}


VOID CALLBACK TimerProc(

    HWND  hwnd,	// handle of window for timer messages 
    UINT  uMsg,	// WM_TIMER message
    UINT  idEvent,	// timer identifier
    DWORD  dwTime) 	// current system time	
{
	struct _EXCEPTION_POINTERS exinfo;

	//
	//	Get semaphore before proceeeding
	//

	GetSemaphore();

	SemHeldByAPI = 2;

	// See if reconfigure requested

	if (ReconfigFlag)
	{
		// Only do it it timer owning process, or we could get in a real mess!

		if(TimerInst == GetCurrentProcessId())
		{
			int i;
			struct BPQVECSTRUC * HOSTVEC;
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
			WSADATA       WsaData;            // receives data from WSAStartup

			ReconfigFlag = FALSE;

			lineno=0;
			memset(Screen, ' ', LINELEN*SCREENLEN);

			SetupBPQDirectory();

			WritetoConsole("Reconfiguring ...\n\n");
			OutputDebugString("BPQ32 Reconfiguring ...\n");	

			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
					if (PORTVEC->PORT_EXT_ADDR)
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
				
				PORTVEC->PORTCONTROL.PORTCLOSECODE(PORTVEC->PORTCONTROL.IOBASE);

				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
			}

			IPClose();
			Rig_Close();
			WSACleanup();

			Sleep(2000);

			WSAStartup(MAKEWORD(2, 0), &WsaData);

			_asm{

			pushad
			call START

			call INITIALISEPORTS			// Restart Ports

			popad
			}

			for (i=1;i<66;i++)			// Include IP Vec
			{
				HOSTVEC=&BPQHOSTVECTOR[i-1];

				HOSTVEC->HOSTTRACEQ=0;

				if (HOSTVEC->HOSTSESSION !=0)
				{
					// Had a connection
					
					HOSTVEC->HOSTSESSION=0;
					HOSTVEC->HOSTFLAGS |=3;	// Disconnected
					
					PostMessage(HOSTVEC->HOSTHANDLE, BPQMsg, i, 4);
				}
			}
			
			WritetoConsole("\n\nReconfiguration Complete\n");

			if (IPRequired)	IPActive = Init_IP();
			
			RigActive = Rig_Init();
			
			OutputDebugString("BPQ32 Reconfiguration Complete\n");	
		}
	}

	__try 
	{
		if (IPActive) Poll_IP();
		if (RigActive) Rig_Poll();
		
		TIMERINTERRUPT();
	}
	__except(memcpy(&exinfo, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)
	{
		unsigned int SPPtr;
		unsigned int SPVal;

		DWORD Stack[16];
		
		SPPtr = exinfo.ContextRecord->Esp;	

		__asm{

		mov eax, SPPtr
		mov SPVal,eax
		lea edi,Stack
		mov esi,eax
		mov ecx,64
		rep movsb

	}


		Debugprintf("BPQ32 *** Program Error %x at %x in Timer Processing",
			exinfo.ExceptionRecord->ExceptionCode, exinfo.ExceptionRecord->ExceptionAddress);

		
		Debugprintf("EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x ESP %x",
			exinfo.ContextRecord->Eax, exinfo.ContextRecord->Ebx, exinfo.ContextRecord->Ecx,
			exinfo.ContextRecord->Edx, exinfo.ContextRecord->Esi, exinfo.ContextRecord->Edi, SPVal);
		
		Debugprintf("Stack:");

		Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
			SPVal, Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7]);

		Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
			SPVal+32, Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);


	}

	FreeSemaphore();			// SendLocation needs to get the semaphore

	if (ReportTimer)
	{
		ReportTimer--;
	
		if (ReportTimer == 0)
		{
			ReportTimer = REPORTINTERVAL;
			SendLocation();
		}
	}

	return;
}

HANDLE NPHandle;

int (WINAPI FAR *GetModuleFileNameExPtr)();

FirstInit()
{
    WSADATA       WsaData;            // receives data from WSAStartup
	HINSTANCE ExtDriver=0;


	// First Time Ports and Timer init

	// Moved from DLLINIT to sort out perl problem, and meet MS Guidelines on minimising DLLMain 

	// Call wsastartup - most systems need winsock, and duplicate statups could be a problem

    WSAStartup(MAKEWORD(2, 0), &WsaData);

	// Load Psapi.dll if possible

	ExtDriver=LoadLibrary("Psapi.dll");

	if (ExtDriver)
		GetModuleFileNameExPtr = (FARPROCX)GetProcAddress(ExtDriver,"GetModuleFileNameExA");
	
	INITIALISEPORTS();

	SetupConsoleWindow();

	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
	TimerInst=GetCurrentProcessId();

	if (AXIPPort && LOCATOR[0])
	{
		// Enable Node Map Reports

		ReportTimer = 600;
	}

 	WritetoConsole("\n\nPort Initialisation Complete\n");

	if (IPRequired)	IPActive = Init_IP();

	RigActive = Rig_Init();

	_beginthread(MonitorThread,0,0);
	
	OutputDebugString("BPQ32 Port Initialisation Complete\n");

	return 0;
}
/*
BOOL LoadIPDriver()
{
	char msg[128];
	int err=0;
	UCHAR Value[MAX_PATH];
	char DLL[]="BPQIPModule.dll";
	
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
		
	hIPModule=LoadLibrary(Value);

	if (hIPModule == NULL)
	{
		err=GetLastError();

		wsprintf(msg,"Error loading Driver %s - Error code %d",	DLL,err);
		
		WritetoConsole(msg);
		return FALSE;

	}
	else
	{
		Init_IP = (int (__stdcall *)())GetProcAddress(hIPModule,"_Init_IP@0");
		Poll_IP = (int (__stdcall *)())GetProcAddress(hIPModule,"_Poll_IP@0");
	}
	return TRUE;
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

*/
Check_Timer()
{
	// Don't attach timer to Perl or ntvdm Process

	if (Perl == 1)
	{
		// If only perl or ntvdm program is running, start bpq32

		if (AttachedProcesses > AttachedPerlProcesses)
			return 0;
		else
		{
			if (AttachingProcess > 0 ) return 0;

			OutputDebugString("BPQ32 Only perl or ntvdm running - Start bpq32.exe\n");
			StartBPQ32();
			Sleep(2000);
			return 0;
		}
	}

	GetSemaphore();

	SemHeldByAPI = 3;

	if (FirstInitDone == 0)
	{
		FirstInitDone=1;

		FirstInit();
	}

	if (TimerHandle == 0)
	{
	    WSADATA       WsaData;            // receives data from WSAStartup
		HINSTANCE ExtDriver=0;

		OutputDebugString("BPQ32 Reinitialising External Ports and Attaching Timer\n");

		if (!ProcessConfig())
		{
			SetupConsoleWindow();
			ShowWindow(hWnd, SW_RESTORE);
			SendMessage(hWnd, WM_PAINT, 0, 0);

			MessageBox(NULL,"Configuration File Error","BPQ32",MB_ICONSTOP);

			FreeSemaphore();
			return (0);
		}


		GetVersionInfo("bpq32.dll");

		SetupConsoleWindow();

		lineno=0;
		memset(Screen, ' ', LINELEN*SCREENLEN);

		WritetoConsole(&SIGNONMSG);
		WritetoConsole("Reinitialising...\n");
 
		SetupBPQDirectory();

		Sleep(1000);			// Allow time for sockets to close	

		WSAStartup(MAKEWORD(2, 0), &WsaData);

		// Load Psapi.dll if possible

		ExtDriver  =LoadLibrary("Psapi.dll");

		if (ExtDriver)
			GetModuleFileNameExPtr = (FARPROCX)GetProcAddress(ExtDriver,"GetModuleFileNameExA");

		INITIALISEPORTS();

		WritetoConsole("\n\nPort Reinitialisation Complete\n");

		TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
		TimerInst=GetCurrentProcessId();

		BPQMsg = RegisterWindowMessage(BPQWinMsg);

		CreateMutex(NULL,TRUE,"BPQLOCKMUTEX");

//		NPHandle=CreateNamedPipe("\\\\.\\pipe\\BPQ32pipe",
//					PIPE_ACCESS_DUPLEX,0,64,4096,4096,1000,NULL);

		if (IPRequired)	IPActive = Init_IP();

		RigActive = Rig_Init();

		FreeConfig();

		_beginthread(MonitorThread,0,0);

		ReportTimer = 0;

		if (AXIPPort && LOCATOR[0])
		{
			// Enable Node Map Reports

			ReportTimer = 600;
		}

		FreeSemaphore();

		return (1);

	}

	FreeSemaphore();

	return (0);

}

DllExport INT APIENTRY CheckTimer()
{
	return Check_Timer();
}


Tell_Sessions()
{
	//
	//	Post a message to all listening sessions, so they call the 
	//	API, and cause a new timer to be allocated
	//
	HWND hWnd;
	int i;

	for (i=1;i<65;i++)
	{
		if (BPQHOSTVECTOR[i-1].HOSTFLAGS & 0x80)
		{
			hWnd = BPQHOSTVECTOR[i-1].HOSTHANDLE;
			PostMessage(hWnd, BPQMsg,i, 1);
			PostMessage(hWnd, BPQMsg,i, 2);
		}
	}
	return (0);
}

HANDLE hInstance=0;

char pgm[256];		// Uninitialised

int xx = sizeof(struct ROUTE);


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	HANDLE handle;
	DWORD n;
	char buf[350];
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];

	int i;
	unsigned int ProcessID;

	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:

		if (sizeof(HDLCDATA) > PORTENTRYLEN)
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"BPQ32 Too much HDLC data - Recompile","BPQ32", MB_OK);
			return 0;
		}
			  
		if (sizeof(LINKTABLE) != LINKTABLELEN)
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"L2 LINK Table .c and .asm mismatch - fix and rebuild","BPQ32", MB_OK);
			return 0;
		}
		if (sizeof(struct ROUTE) != DataBase->ROUTE_LEN)
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"ROUTE Table .c and .asm mismatch - fix and rebuild", "BPQ32", MB_OK);
			return 0;
		}
	
		if (sizeof(struct DEST_LIST) != DataBase->DEST_LIST_LEN)
		{
			// Catastrophic - Refuse to load
			
			MessageBox(NULL,"NODES Table .c and .asm mismatch - fix and rebuild", "BPQ32", MB_OK);
			return 0;
		}
	
		
		GetSemaphore();

		BPQHOSTVECPTR = &BPQHOSTVECTOR[0];

		SemHeldByAPI = 4;

		LoadToolHelperRoutines();

		GetProcess(GetCurrentProcessId(),pgm);

		if (_stricmp(pgm,"BPQTelnetServer.exe") == 0)
			AuthorisedProgram = FALSE;
		else
			AuthorisedProgram = TRUE;

		if (_stricmp(pgm,"perl.exe") == 0 || _stricmp(pgm,"ntvdm.exe") == 0)

			Perl=1;
		else
			Perl=0;

		if (InitDone == 0)
		{
			hInstance=hInst;

			Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

			if (Mutex != NULL)
			{
				OutputDebugString("Another BPQ32.dll is loaded\n");
				i=MessageBox(NULL,"BPQ32 DLL already loaded from another directory\nIf you REALLY want this, hit OK, else hit Cancel","BPQ32",MB_OKCANCEL);
				FreeSemaphore();

				if (i != IDOK) return (0);

				CloseHandle(Mutex);
			}

			if (Perl == 1)
				CheckifBPQ32isLoaded();					// Start BPQ32.exe if needed

			memset(Screen, ' ', LINELEN*SCREENLEN);

			GetVersionInfo("bpq32.dll");

			SetupBPQDirectory();

			if (!ProcessConfig())
			{
				SetupConsoleWindow();
				ShowWindow(hWnd, SW_RESTORE);
				SendMessage(hWnd, WM_PAINT, 0, 0);

				MessageBox(NULL,"Configuration File Error","BPQ32",MB_ICONSTOP);

				FreeSemaphore();
				return (0);
			}

			if (START() !=0)
			{
				Sleep(3000);
				FreeSemaphore();
				return (0);
			}
			else
			{
				InitDone=(int) &InitDone;
				BPQMsg = RegisterWindowMessage(BPQWinMsg);
//				TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
//				TimerInst=GetCurrentProcessId();

/*				Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

				if (Mutex != NULL)
				{
					OutputDebugString("Another BPQ32.dll is loaded\n");
					MessageBox(NULL,"BPQ32 DLL already loaded from another directory","BPQ32",MB_ICONSTOP);
					FreeSemaphore();
					return (0);
				}

*/
				Mutex=CreateMutex(NULL,TRUE,"BPQLOCKMUTEX");

//				CreatePipe(&H1,&H2,NULL,1000);
  
//				GetLastError();

//				NPHandle=CreateNamedPipe("\\\\.\\pipe\\BPQ32pipe",
//					PIPE_ACCESS_DUPLEX,0,64,4096,4096,1000,NULL);
				
//				GetLastError();

				//
				//	Read SYSOP password
				//

				if (PWTEXT[0] == 0)
				{
					handle = OpenConfigFile("PASSWORD.BPQ");

					if (handle == INVALID_HANDLE_VALUE)
					{
						WritetoConsole("Can't open PASSWORD.BPQ\n");
						PWLEN=0;
						PWTEXT[0]=0;
					}
					else
					{
						ReadFile(handle,PWTEXT,78,&n,NULL); 
						CloseHandle(handle);
					}
				}
			
				for (i=0;PWTEXT[i] > 0x20;i++); //Scan for cr or null 
				PWLEN=i;
				
			}
		}
		else
		{
			if (InitDone != (int) &InitDone)
			{
				MessageBox(NULL,"BPQ32 DLL already loaded at another address","BPQ32",MB_ICONSTOP);
				FreeSemaphore();
				return (0);
			}
		}
			
		// Run timer monitor thread in all processes - it is possible for the TImer thread not to be the first thread
	
		_beginthread(MonitorTimerThread,0,0);

		FreeSemaphore();

		AttachedPIDType[AttachedProcesses]=Perl;
		AttachedPIDList[AttachedProcesses++]=GetCurrentProcessId();
		AttachedPerlProcesses+=Perl;

		if (_stricmp(pgm,"bpq32.exe") == 0 &&  AttachingProcess == 1) AttachingProcess=0;

		GetProcess(GetCurrentProcessId(),pgm);
		n=wsprintf(buf,"BPQ32 DLL Attach complete - Program %s - %d Process(es) Attached %d\n",pgm,AttachedProcesses,AttachedPerlProcesses);
		OutputDebugString(buf);

		// Set up local variables
		
		MCOM=1;
		MTX=1;
		MMASK=0xffffffff;

		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
		
		return 1;
    
	case DLL_PROCESS_DETACH:
		
		ProcessID=GetCurrentProcessId();
		
		// Release any streams that the app has failed to release

		for (i=1;i<65;i++)
		{
			if (BPQHOSTVECTOR[i-1].STREAMOWNER == ProcessID)
				DeallocateStream(i);
		}

		if (Mutex) CloseHandle(Mutex);

		//	Remove our entry from PID List

		for (i=0;  i< AttachedProcesses; i++)
			if (AttachedPIDList[i] == ProcessID)
				break;

		for (; i< AttachedProcesses; i++)
		{
			AttachedPIDType[i]=AttachedPIDType[i+1];
			AttachedPIDList[i]=AttachedPIDList[i+1];
		}

		ShowWindow(hWnd, SW_RESTORE);
		GetWindowRect(hWnd, &Rect);

		if (hWnd) DestroyWindow(hWnd);

		if (TimerInst == ProcessID)
		{
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
	
			OutputDebugString("BPQ32 Process with Timer closing\n");

			// Call Port Close Routines
			
			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
					if (PORTVEC->PORT_EXT_ADDR && PORTVEC->DLLhandle == NULL) // Don't call if real .dll - it's not there!
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
				
				PORTVEC->PORTCONTROL.PORTCLOSECODE(PORTVEC->PORTCONTROL.IOBASE);

				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
			}

			IPClose();
			Rig_Close();
			WSACleanup();

			if (MinimizetoTray)
				Shell_NotifyIcon(NIM_DELETE,&niData);

			KillTimer(NULL,TimerHandle);
			TimerHandle=0;
			TimerInst=0xffffffff;

			Tell_Sessions();			// So anther process can get timet

		}

		AttachedProcesses--;
		AttachedPerlProcesses-=Perl;

		if (AttachedProcesses == 0)
		{
			KillTimer(NULL,TimerHandle);
			
			if (AUTOSAVE == 1) SaveNodes();	
			
			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

			if (retCode == ERROR_SUCCESS)
			{
				wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
				retCode = RegSetValueEx(hKey,"WindowSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));

				RegCloseKey(hKey);
			}

			
			if (MinimizetoTray)
				Shell_NotifyIcon(NIM_DELETE,&niData);

			// Unload External Drivers

			{
				PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
				
				for (i=0;i<NUMBEROFPORTS;i++)
				{
					if (PORTVEC->DLLhandle)
						FreeLibrary(PORTVEC->DLLhandle);

					PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;
				}
			}
		}

		GetProcess(GetCurrentProcessId(),pgm);
		n=wsprintf(buf,"BPQ32 DLL Detach complete - Program %s - %d Process(es) Attached %d\n",pgm,AttachedProcesses,AttachedPerlProcesses);
		OutputDebugString(buf);

		return 1;
	}
	return 1;
}

DllExport int APIENTRY CloseBPQ32()	
{
	// Unload External Drivers

	PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
	int i;

	if (TimerInst == GetCurrentProcessId())
	{	
		OutputDebugString("BPQ32 Process with Timer called CloseBPQ32\n");

		for (i=0;i<NUMBEROFPORTS;i++)
		{
			if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
				if (PORTVEC->PORT_EXT_ADDR)
					PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);

			PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
		}

		KillTimer(NULL,TimerHandle);
		TimerHandle=0;
		TimerInst=0xffffffff;
		Tell_Sessions();

		WSACleanup();
	}
	
	return 0;
}


VOID SetupBPQDirectory()
{
	HKEY hKey=0;
	int retCode,Type,Vallen=MAX_PATH,i;
	char msg[512];

	char DLLName[256]="Not Known";

	GetModuleFileName(hInstance,DLLName,256);

	BPQDirectory[0]=0;

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

    if (retCode == ERROR_SUCCESS)
	{
		// Try "BPQ Directory"
		
		retCode = RegQueryValueEx(hKey,"BPQ Directory",0,			
			&Type,(UCHAR *)&BPQDirectory,&Vallen);

		if (retCode == ERROR_SUCCESS)
		{
			if (strlen(BPQDirectory) == 2 && BPQDirectory[0] == '"' && BPQDirectory[1] == '"')
				BPQDirectory[0]=0;
		}

		if (BPQDirectory[0] == 0)
		{
			// BPQ Directory absent or = "" - try "Config File Location"

			Vallen=MAX_PATH;
			
			retCode = RegQueryValueEx(hKey,"Config File Location",0,			
				&Type,(UCHAR *)&BPQDirectory,&Vallen);

			if (retCode == ERROR_SUCCESS)
			{
				if (strlen(BPQDirectory) == 2 && BPQDirectory[0] == '"' && BPQDirectory[1] == '"')
					BPQDirectory[0]=0;
			}
		}

		if (BPQDirectory[0] == 0) GetCurrentDirectory(MAX_PATH, BPQDirectory);

		// Get StartMinimized and MinimizetoTray flags

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Start Minimized", 0, &Type, (UCHAR *)&StartMinimized, &Vallen);

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Minimize to Tray", 0, &Type, (UCHAR *)&MinimizetoTray, &Vallen);

		RegCloseKey(hKey);
	}

	i=sprintf(msg,"BPQ32 Ver %s Loaded from: %s by %s\n",VersionStringWithBuild, DLLName, pgm);
 	WritetoConsole(msg);
	OutputDebugString(msg);
	
	i=sprintf(msg,"BPQ32 Using config from: %s\n\n",BPQDirectory);
 	WritetoConsole(&msg[6]);
	msg[i-1]=0;
	OutputDebugString(msg);

	return;	
}

HANDLE OpenConfigFile(char *fn)
{
	HANDLE handle;
	UCHAR Value[MAX_PATH];
	FILETIME LastWriteTime;
	SYSTEMTIME Time;
	char Msg[256];


	// If no directory, use current
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
		
	handle = CreateFile(Value,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	GetFileTime(handle, NULL, NULL, &LastWriteTime);
	FileTimeToSystemTime(&LastWriteTime, &Time);

	wsprintf(Msg,"BPQ32 Config File %s Created %.2d:%.2d %d/%.2d/%.2d\n", Value,
				Time.wHour, Time.wMinute, Time.wYear, Time.wMonth, Time.wDay);

	OutputDebugString(Msg);


	return(handle);

}

 char * FormatMH(struct MHSTRUC * MH)
 {
	struct tm * TM;
	static char MHTime[50];
	time_t szClock = MH->MHTIME;
	char LOC[7];
	
	memcpy(LOC, MH->MHLocator, 6);
	LOC[6] = 0;

	szClock = (_time32(NULL) - szClock);
	TM = gmtime(&szClock);

	wsprintf(MHTime, "%.2d:%.2d:%.2d:%.2d  %s %s\r",
		TM->tm_yday, TM->tm_hour, TM->tm_min, TM->tm_sec, MH->MHFreq, LOC);

	return MHTime;

 }
DllExport int APIENTRY GETBPQAPI()
{
 	return (int)BPQHOSTAPI;
}
    
DllExport UINT APIENTRY GETMONDECODE()
{
 	return (UINT)MONDECODE;
}
    

DllExport INT APIENTRY BPQAPI(int Fn, char * params)
{

/*
;
;	BPQ HOST MODE SUPPORT CODE 
;
;	22/11/95
;
;	MOVED FROM TNCODE.ASM COS CONITIONALS WERE GETTING TOO COMPLICATED
;	(OS2 VERSION HAD UPSET KANT VERISON
;
;
*/


/*

  BPQHOSTPORT:
;
;	SPECIAL INTERFACE, MAINLY FOR EXTERNAL HOST MODE SUPPORT PROGS
;
;	COMMANDS SUPPORTED ARE
;
;	AH = 0	Get node/switch version number and description.  On return
;		AH='B',AL='P',BH='Q',BL=' '
;		DH = major version number and DL = minor version number.
;
;
;	AH = 1	Set application mask to value in DL (or even DX if 16
;		applications are ever to be supported).
;
;		Set application flag(s) to value in CL (or CX).
;		whether user gets connected/disconnected messages issued
;		by the node etc.
;
;
;	AH = 2	Send frame in ES:SI (length CX)
;
;
;	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
;		in CX.  BX returns the number of outstanding frames still to
;		be received (ie. after this one) or zero if no more frames
;		(ie. this is last one).
;
;
;
;	AH = 4	Get stream status.  Returns:
;
;		CX = 0 if stream disconnected or CX = 1 if stream connected
;		DX = 0 if no change of state since last read, or DX = 1 if
;		       the connected/disconnected state has changed since
;		       last read (ie. delta-stream status).
;
;
;
;	AH = 6	Session control.
;
;		CX = 0 Conneect - APPLMASK in DL
;		CX = 1 connect
;		CX = 2 disconnect
;		CX = 3 return user to node
;
;
;	AH = 7	Get buffer counts for stream.  Returns:
;
;		AX = number of status change messages to be received
;		BX = number of frames queued for receive
;		CX = number of un-acked frames to be sent
;		DX = number of buffers left in node
;		SI = number of trace frames queued for receive
;
;AH = 8		Port control/information.  Called with a stream number
;		in AL returns:
;
;		AL = Radio port on which channel is connected (or zero)
;		AH = SESSION TYPE BITS
;		BX = L2 paclen for the radio port
;		CX = L2 maxframe for the radio port
;		DX = L4 window size (if L4 circuit, or zero)
;		ES:DI = CALLSIGN

;AH = 9		Fetch node/application callsign & alias.  AL = application
;		number:
;
;		0 = node
;		1 = BBS
;		2 = HOST
;		3 = SYSOP etc. etc.
;
;		Returns string with alias & callsign or application name in
;		user's buffer pointed to by ES:SI length CX.  For example:
;
;		"WORCS:G8TIC"  or "TICPMS:G8TIC-10".
;
;
;	AH = 10	Unproto transmit frame.  Data pointed to by ES:SI, of
;		length CX, is transmitted as a HDLC frame on the radio
;		port (not stream) in AL.
;
;
;	AH = 11 Get Trace (RAW Data) Frame into ES:DI,
;			 Length to CX, Timestamp to AX
;
;
;	AH = 12 Update Switch. At the moment only Beacon Text may be updated
;		DX = Function
;		     1=update BT. ES:SI, Len CX = Text
;		     2=kick off nodes broadcast
;
;	AH = 13 Allocate/deallocate stream
;		If AL=0, return first free stream
;		If AL>0, CL=1, Allocate stream. If aleady allocated,
;		   return CX nonzero, else allocate, and return CX=0
;		If AL>0, CL=2, Release stream
;
;
;	AH = 14 Internal Interface for IP Router
;
;		Send frame - to NETROM L3 if DL=0
;			     to L2 Session if DL<>0
;
;
; 	AH = 15 Get interval timer


*/


    switch(Fn)
	{

	case CHECKLOADED:

		params[0]=MAJORVERSION;
		params[1]=MINORVERSION;
		params[2]=QCOUNT;
		
		return (1);
	}
	return 0;
}

DllExport int APIENTRY InitSwitch()
{
	return (0);
}

DllExport int APIENTRY GetNumberofPorts()
{
	return (NUMBEROFPORTS);
}

DllExport int APIENTRY GetPortNumber(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC->PORTNUMBER;

}

DllExport UCHAR * APIENTRY GetPortDescription(int portslot, char * Desc)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	memcpy(Desc, PORTVEC->PORTDESCRIPTION, 30);
	Desc[30]=0;

	return 0;
}

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntry(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}


/*DllExport int APIENTRY SwitchTimer()
{
	GetSemaphore();

	TIMERINTERRUPT();
	
	FreeSemaphore();

	return (0);
}
*/
DllExport int APIENTRY GetFreeBuffs()
{
//	Returns number of free buffers
//	(BPQHOST function 7 (part)).
	return (QCOUNT);
}

DllExport UCHAR * APIENTRY GetNodeCall()
{
	return (&MYNODECALL);
}


DllExport UCHAR * APIENTRY GetNodeAlias()
{
	return (&MYALIAS[0]);
}

DllExport UCHAR * APIENTRY GetBBSCall()
{
	return (UCHAR *)(&APPLCALLTABLE[0].APPLCALL_TEXT);
}


DllExport UCHAR * APIENTRY GetBBSAlias()
{
	return (UCHAR *)(&APPLCALLTABLE[0].APPLALIAS_TEXT);
}

DllExport VOID APIENTRY GetApplCallVB(int Appl, char * ApplCall)
{
	if (Appl < 1 || Appl > NumberofAppls ) return;

	strncpy(ApplCall,(char *)&APPLCALLTABLE[Appl-1].APPLCALL_TEXT, 10);
}
DllExport char * APIENTRY GetApplCall(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLCALL_TEXT);
}
DllExport char * APIENTRY GetApplAlias(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLALIAS_TEXT);
}

DllExport long APIENTRY GetApplQual(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return 0;

	return (APPLCALLTABLE[Appl-1].APPLQUAL);
}

DllExport char * APIENTRY GetApplName(int Appl)
{
	if (Appl < 1 || Appl > NumberofAppls ) return NULL;

	return (UCHAR *)(&APPLCALLTABLE[Appl-1].APPLCMD);
}
BOOL UpdateNodesForApp(int Appl);
DllExport BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call);

DllExport BOOL APIENTRY SetApplCall(int Appl, char * NewCall)
{
	char Call[10]="          ";
	int i;

	if (Appl < 1 || Appl > NumberofAppls ) return FALSE;
	
	i=strlen(NewCall);

	if (i > 10) i=10;

	strncpy(Call,NewCall,i);

	strncpy((char *)&APPLCALLTABLE[Appl-1].APPLCALL_TEXT,Call,10);

	if (!ConvToAX25(Call,APPLCALLTABLE[Appl-1].APPLCALL)) return FALSE;

	UpdateNodesForApp(Appl);

	return TRUE;

}

DllExport BOOL APIENTRY SetApplAlias(int Appl, char * NewCall)
{
	char Call[10]="          ";
	int i;

	if (Appl < 1 || Appl > NumberofAppls ) return FALSE;
	
	i=strlen(NewCall);

	if (i > 10) i=10;

	strncpy(Call,NewCall,i);

	strncpy((char *)&APPLCALLTABLE[Appl-1].APPLALIAS_TEXT,Call,10);

	if (!ConvToAX25(Call,APPLCALLTABLE[Appl-1].APPLALIAS)) return FALSE;

	UpdateNodesForApp(Appl);

	return TRUE;

}



DllExport BOOL APIENTRY SetApplQual(int Appl, int NewQual)
{
	if (Appl < 1 || Appl > NumberofAppls ) return FALSE;
	
	APPLCALLTABLE[Appl-1].APPLQUAL=NewQual;

	UpdateNodesForApp(Appl);

	return TRUE;

}


BOOL UpdateNodesForApp(int Appl)
{
	int App=Appl-1;
	int DestLen=sizeof dest_list;

	struct DEST_LIST * DEST=APPLCALLTABLE[App].NODEPOINTER;
	struct APPLCALLS * APPL=&APPLCALLTABLE[App];

	if (DEST==NULL)
	{
		// No dest at the moment. If we have valid call and Qual, create an entry
		
		if (APPLCALLTABLE[App].APPLQUAL == 0) return FALSE;

		if (APPLCALLTABLE[App].APPLCALL[0] < 41) return FALSE;


		GetSemaphore();

		SemHeldByAPI = 5;
	
		_asm{

		call FINDFREEDESTINATION

		jnz	nodests

		mov DEST,EBX

		}

		NUMBEROFNODES++;
		APPL->NODEPOINTER=DEST;
		
		memmove (DEST->DEST_CALL,APPL->APPLCALL,13);

		DEST->DEST_STATE=0x80;	// SPECIAL ENTRY
	
		DEST->NRROUTE1.ROUT_QUALITY = (BYTE)APPL->APPLQUAL;
		DEST->NRROUTE1.ROUT_OBSCOUNT = 255;

		FreeSemaphore();

		return TRUE;

		_asm{


nodests:
		}

		FreeSemaphore();
		return FALSE;
	}

	//	We have a destination. If Quality is zero, remove it, else update it

	if (APPLCALLTABLE[App].APPLQUAL == 0)
	{
		GetSemaphore();

		SemHeldByAPI = 6;

		_asm {

		mov	EBX,DEST
		CALL REMOVENODE			// Clear buffers, Remove from Sorted Nodes chain, and zap entry
		
		}

		APPL->NODEPOINTER=NULL;

		FreeSemaphore();
		return FALSE;

	}

	if (APPLCALLTABLE[App].APPLCALL[0] < 41)	return FALSE;

	GetSemaphore();

	SemHeldByAPI = 7;

	memmove (DEST->DEST_CALL,APPL->APPLCALL,13);

	DEST->DEST_STATE=0x80;	// SPECIAL ENTRY
	
	DEST->NRROUTE1.ROUT_QUALITY = (BYTE)APPL->APPLQUAL;
	DEST->NRROUTE1.ROUT_OBSCOUNT = 255;

	FreeSemaphore();
	return TRUE;

}

DllExport BOOL ConvToAX25(unsigned char * callsign, unsigned char * ax25call)
{
	int i;

	memset(ax25call,0x40,6);		// in case short
	ax25call[6]=0x60;				// default SSID	

	for (i=0;i<7;i++)
	{
		if (callsign[i] == '-')
		{
			//
			//	process ssid and return
			//
			i = atoi(&callsign[i+1]);

			if (i < 16)
			{
				ax25call[6] |= i<<1;
				return (TRUE);
			}
			return (FALSE);
		}

		if (callsign[i] == 0 || callsign[i] == 13 || callsign[i] == ' ')
		{
			//
			//	End of call - no ssid
			//
			return (TRUE);
		}
		
		ax25call[i] = callsign[i] << 1;
	}
	
	//
	//	Too many chars
	//

	return (FALSE);
}

DllExport UCHAR * APIENTRY GetSignOnMsg()
{
	return (&SIGNONMSG);
}

DllExport char * APIENTRY GetVersionString()
{
	return ((char *)&VersionStringWithBuild);
}


DllExport UCHAR * APIENTRY GetBPQDirectory()
{
	return (&BPQDirectory[0]);
}

// Version for Visual Basic

DllExport char * APIENTRY CopyBPQDirectory(char * dir)
{
	return (strcpy(dir,BPQDirectory));
}

DllExport int APIENTRY RXCount(int Stream)
{
//	Returns count of packets waiting on stream
//	 (BPQHOST function 7 (part)).

	int	cnt;
	
	Check_Timer();
	
	_asm{

	pushfd
	cld
	pushad


;	AH = 7	Get buffer counts for stream.  Returns:
;
;		AX = number of status change messages to be received
;		BX = number of frames queued for receive
;		CX = number of un-acked frames to be sent
;		DX = number of buffers left in node
;		SI = number of trace frames queued for receive
;
		
	mov	al,byte ptr Stream
	mov	ah,7

	mov		esi,0
	call	BPQHOSTAPI

	mov	cnt,ebx	
	
	popad
	popfd

	}

	return (cnt);
}

DllExport int APIENTRY TXCount(int Stream)
{
//	Returns number of packets on TX queue for stream
//	 (BPQHOST function 7 (part)).
	int	cnt;
		
	_asm{

	pushfd
	cld
	pushad


;	AH = 7	Get buffer counts for stream.  Returns:
;
;		AX = number of status change messages to be received
;		BX = number of frames queued for receive
;		CX = number of un-acked frames to be sent
;		DX = number of buffers left in node
;		SI = number of trace frames queued for receive
;
		
	mov	al,byte ptr Stream
	mov	ah,7

	call	BPQHOSTAPI

	mov	cnt,ecx	
	
	popad
	popfd

	}

	return (cnt);
}

DllExport int APIENTRY MONCount(int Stream)
{
//	Returns number of monitor frames available
//	 (BPQHOST function 7 (part)).
	int	cnt;
		
	Check_Timer();
		
	_asm{

	pushfd
	cld
	pushad

;	AH = 7	Get buffer counts for stream.  Returns:
;
;		AX = number of status change messages to be received
;		BX = number of frames queued for receive
;		CX = number of un-acked frames to be sent
;		DX = number of buffers left in node
;		SI = number of trace frames queued for receive
;
		
	mov	al,byte ptr Stream
	mov	ah,7

	call	BPQHOSTAPI

	mov	cnt,esi	
	
	popad
	popfd

	}

	return (cnt);
}

DllExport int APIENTRY GetCallsign(int stream, char * callsign)
{
//	Returns call connected on stream (BPQHOST function 8 (part)).
	
	int retcode=0;

	_asm {

	pushfd
	cld
	pushad

;AH = 8		Port control/information.  Called with a stream number
;		in AL returns:
;
;		AL = Radio port on which channel is connected (or zero)
;		AH = SESSION TYPE BITS
;		EBX = L2 paclen for the radio port
;		ECX = L2 maxframe for the radio port
;		EDX = L4 window size (if L4 circuit, or zero) or -1 if not connected
;		ES:DI = CALLSIGN


	mov	al,byte ptr stream // Get Stream Number
	mov	ah,8
	mov edi,callsign
	mov	ecx,10			// String Length

	call	BPQHOSTAPI

	cmp	edx,-1
	jne	retok

	mov	retcode,1			// Not Connected

retok:


	popad
	popfd

	}				// End of ASM

	return (retcode);
}

DllExport int APIENTRY GetConnectionInfo(int stream, char * callsign,
										 int * port, int * sesstype, int * paclen,
										 int * maxframe, int * l4window)
{

	int retcode=0;

	_asm {
		
	pushfd
	cld
	pushad

;AH = 8		Port control/information.  Called with a stream number
;		in AL returns:
;
;		AL = Radio port on which channel is connected (or zero)
;		AH = SESSION TYPE BITS
;		EBX = L2 paclen for the radio port
;		ECX = L2 maxframe for the radio port
;		EDX = L4 window size (if L4 circuit, or zero) or -1 if not connected
;		ES:DI = CALLSIGN


	mov	al,byte ptr stream // Get Stream Number
	mov	ah,8
	mov edi,callsign
	mov	ecx,10			// String Length

	call	BPQHOSTAPI

	mov	retcode,0			// Connected

	cmp	edx,-1
	jne	conn

	mov	retcode,1			// Not Connected
	mov	edx,0

conn:

	mov	esi,paclen
	mov	[esi],ebx

	movzx	ebx,al
	mov	esi,port
	mov	[esi],ebx

	movzx	ebx,ah
	mov	esi,sesstype
	mov	[esi],ebx

	mov	esi,maxframe
	mov	[esi],ecx
	mov	esi,l4window
	mov	[esi],edx


	popad
	popfd

	}				// End of ASM

	return (retcode);
}

DllExport int APIENTRY SessionControl(int stream, int command, int param)
{
//	Send Session Control command (BPQHOST function 6)
//;	CL=0 CONNECT USING APPL MASK IN DL
//;	CL=1, CONNECT. CL=2 - DISCONNECT. CL=3 RETURN TO NODE
	_asm {

	pushfd
	cld
	pushad

	mov	ah,6
	mov	al,byte ptr stream
	mov	cl,byte ptr command
	mov edx,param

	call	BPQHOSTAPI

	popad
	popfd

	}

	return (0);

}



		


DllExport int APIENTRY SetAppl(int stream, int flags, int mask)
{
//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in EDX (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.

	_asm{
		
	pushfd
	cld
	pushad

	mov	al,byte ptr stream
	mov	ah,1
	mov	edx,mask
	mov	ecx,flags
	
	call	BPQHOSTAPI

	popad
	popfd

	}
	
	return (0);
} 

DllExport int APIENTRY SessionState(int stream, int * state, int * change)
{

//	Get current Session State. Any state changed is ACK'ed
//	automatically. See BPQHOST functions 4 and 5.
	
	Check_Timer();

	_asm {

	pushfd
	cld
	pushad

	mov	al,byte ptr stream	// Get Stream Number
	mov	ah,4

	call	BPQHOSTAPI

	mov	esi,state
	mov	[esi],ecx

	mov	esi,change
	mov	[esi],edx

	cmp	edx,0
	je	dontack
//
//	ack the change
//
	mov	al,byte ptr stream
	mov	ah,5

	call	BPQHOSTAPI

dontack:

	popad
	popfd

	}


return (0);
}

DllExport int APIENTRY SessionStateNoAck(int stream, int * state)
{
//	Get current Session State. Dont ACK any change
//	See BPQHOST function 4

	Check_Timer();

	_asm {

	pushfd
	cld
	pushad

	mov	al,byte ptr stream	// Get Stream Number
	mov	ah,4

	call	BPQHOSTAPI

	mov	esi,state
	mov	[esi],ecx

	popad
	popfd

	}
	
return (0);
}

DllExport int APIENTRY SendMsg(int stream, char * msg, int len)
{

//	Send message to stream (BPQHOST Function 2)
//	AH = 2	Send frame in ES:SI (length CX)

	int retcode;

//	WriteFile(NPHandle,msg,len,&retcode,NULL);


	Check_Timer();

	_asm{
		
	pushfd
	cld
	pushad

	mov	al,byte ptr stream
	mov	ah,2
	mov	esi,msg
	mov	ecx,len
	
	call	BPQHOSTAPI

	mov	retcode,eax
	
	popad
	popfd

	}
	
	return (retcode);
}

DllExport int APIENTRY SendRaw(int port, char * msg, int len)
{
	int retcode;

	Check_Timer();

	//	Send RaW (KISS mode) frame to port (BPQHOST function 10)

	_asm {

	pushfd
	cld
	pushad

	mov	al,byte ptr port
	mov	ah,10
	mov	ecx,len
	mov	esi,msg

	call	BPQHOSTAPI

	mov	retcode,eax

	popad
	popfd

	}				// End of ASM

	return (retcode);

}
DllExport int APIENTRY GetMsg(int stream, char * msg, int * len, int * count )
{
//	Get message from stream. Returns length, and count of frames
//	still waiting to be collected. (BPQHOST function 3)
//	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
//		in CX.  BX returns the number of outstanding frames still to
//		be received (ie. after this one) or zero if no more frames
//		(ie. this is last one).
//

	Check_Timer();

	_asm{
		
	pushfd
	cld
	pushad

	mov	al,byte ptr stream
	mov	ah,3
	mov	edi,msg

	call	BPQHOSTAPI
	mov	edi,len
	mov	[edi],ecx

	mov	edi,count
	mov	[edi],ebx

	popad
	popfd

	}
	return (0);
}

DllExport int APIENTRY GetMsgPerl(int stream, char * msg)
{
	int len,count;

	GetMsg(stream, msg, &len, &count );

	return len;
}


DllExport int APIENTRY GetRaw(int stream, char * msg, int * len, int * count )
{
	int	stamp;

//	Get Raw (Trace) data (BPQHOST function 11)

	Check_Timer();

	_asm {
		
	pushfd
	cld
	pushad

	mov	al,byte ptr stream
	mov	ah,11
	mov	edi,msg

	call	BPQHOSTAPI

	mov	edi,len
	mov	[edi],ecx

	mov	edi,count
	mov	[edi],ebx

	mov	stamp,eax


	popad
	popfd

	}
	
	return (stamp);
}


DllExport int APIENTRY DecodeFrame(char * msg, char * buffer, int Stamp)
{

//	This is not an API function. It is a utility to decode a received
//	monitor frame into ascii text.
	int	returnit;


	GetSemaphore();

	SemHeldByAPI = 8;

	_asm {

	pushfd
	cld
	pushad

	mov	esi,msg
	mov	eax,Stamp
	mov	edi,buffer

	call	MONDECODE

	mov	returnit,ecx

	popad
	popfd

	}				// End of ASM

  	FreeSemaphore();
 
	return (returnit);

}

DllExport int APIENTRY SetTraceOptions(int mask, int mtxparam, int mcomparam)
{

//	Sets the tracing options for DecodeFrame. Mask is a bit
//	mask of ports to monitor (ie 101 binary will monitor ports
//	1 and 3). MTX enables monitoring on transmitted frames. MCOM
//	enables monitoring of protocol control frames (eg SABM, UA, RR),
//	as well as info frames.

	_asm {

	pushfd
	cld
	pushad

	mov	eax,mask	
	mov	ebx,mtxparam
	mov	ecx,mcomparam

	call	BPQMONOPTIONS

	popad
	popfd

	}				// End of ASM
	return (0);
}

DllExport int APIENTRY FindFreeStream()
{
	int retcode;

//	Returns number of first unused BPQHOST stream. If none available,
//	returns 255. See API function 13.


	GetSemaphore();

	SemHeldByAPI = 9;

	_asm{
	
;
;	LOOK FOR FIRST FREE SESSION
;
	MOV	EBX,OFFSET BPQHOSTVECTOR
	MOV	EAX,1
	
ALLOC_LOOP:

	TEST	BYTE PTR 4[EBX],80H
	JZ SHORT OK_TO_ALLOC

	ADD	EBX,VECTORLENGTH
	INC	EAX

	CMP	EAX,65
	JNE SHORT ALLOC_LOOP

	MOV	EAX,255

	JMP BPQRET

OK_TO_ALLOC:

	OR	BYTE PTR 4[EBX],128	; SET ALLOCATED BIT

BPQRET:

	mov	retcode,eax
}
	if (retcode != 255)
	{
		BPQHOSTVECTOR[retcode-1].STREAMOWNER=GetCurrentProcessId();
		BPQHOSTVECTOR[retcode-1].HOSTFLAGS |= 128; // SET ALLOCATED BIT
	}
	FreeSemaphore();

	return(retcode);


}

DllExport int APIENTRY AllocateStream(int stream)
{
//	Allocate stream. If stream is already allocated, return nonzero.
//	Otherwise allocate stream, and return zero.	
	
	int retcode;
	
	_asm {
	
	pushfd
	cld
	pushad


;		AH = 13 Allocate/deallocate stream
;		If AL=0, return first free stream
;		If AL>0, CL=1, Allocate stream. If aleady allocated,
;		   return CX nonzero, else allocate, and return CX=0
;		If AL>0, CL=2, Release stream

	mov	al,byte ptr stream
	mov	ah,13
	mov	cl,1

	call	BPQHOSTAPI
	
	mov	retcode,ecx
	
	popad
	popfd

	}
	return(retcode);
}
int APIENTRY Rig_Command(int Session, char * Command);

BOOL Rig_CommandInt(int Session, char * Command)
{
	return Rig_Command(Session, Command);
}

DllExport int APIENTRY DeallocateStream(int stream)
{
	struct BPQVECSTRUC * PORTVEC;

//	Release stream.

	stream--;

	if (stream < 0 || stream > 63)
		return (0);
	
	PORTVEC=&BPQHOSTVECTOR[stream];
	
	PORTVEC->STREAMOWNER=0;
	PORTVEC->HOSTAPPLFLAGS=0;
	PORTVEC->HOSTAPPLMASK=0;
	PORTVEC->HOSTFLAGS=0;
	PORTVEC->HOSTHANDLE=0;

	return(0);
}

DllExport int APIENTRY BPQSetHandle(int Stream, HWND hWnd)
{ 
	
	BPQHOSTVECTOR[Stream-1].HOSTHANDLE=hWnd;

	return (0);

}

#define L4USER 0

DllExport int APIENTRY ChangeSessionCallsign(int Stream, unsigned char * AXCall)
{ 

	// Equivalent to "*** linked to" command
	_asm {

	pushfd
	cld
	pushad

	MOV	EBX,OFFSET BPQHOSTVECTOR
	mov	eax,Stream
	dec	eax
	mul	VECTORLENGTH
	add	ebx,eax

	mov	esi,AXCall
	mov	EDI,L4USER[EBX]
	MOV	ECX,7
	REP MOVSB


	popad
	popfd

	}

	return (0);

}


struct BPQVECSTRUC * PORTVEC ;

DllExport int APIENTRY GetApplMask(int Stream)
{ 

	_asm {

	MOV	EBX,OFFSET BPQHOSTVECTOR
	mov	eax,Stream
	dec	eax
	mul	VECTORLENGTH
	add	ebx,eax

	mov	PORTVEC,ebx

	}

	return (PORTVEC->HOSTAPPLMASK);

}
DllExport int APIENTRY GetStreamPID(int Stream)
{ 
	_asm {

	MOV	EBX,OFFSET BPQHOSTVECTOR
	mov	eax,Stream
	dec	eax
	mul	VECTORLENGTH
	add	ebx,eax

	mov	PORTVEC,ebx

	}
	return (PORTVEC->STREAMOWNER);

}

DllExport int APIENTRY GetApplFlags(int Stream)
{ 

	_asm {

	MOV	EBX,OFFSET BPQHOSTVECTOR
	mov	eax,Stream
	dec	eax
	mul	VECTORLENGTH
	add	ebx,eax

	mov	PORTVEC,ebx

	}

	return (PORTVEC->HOSTAPPLFLAGS);

}

DllExport int APIENTRY GetApplNum(int Stream)
{ 

	_asm {

	MOV	EBX,OFFSET BPQHOSTVECTOR
	mov	eax,Stream
	dec	eax
	mul	VECTORLENGTH
	add	ebx,eax

	mov	PORTVEC,ebx

	}

	return (PORTVEC->HOSTAPPLNUM);

}

DllExport BOOL APIENTRY GetAllocationState(int Stream)
{ 

	_asm {

	MOV	EBX,OFFSET BPQHOSTVECTOR
	mov	eax,Stream
	dec	eax
	mul	VECTORLENGTH
	add	ebx,eax

	mov	PORTVEC,ebx

	}

	return ((PORTVEC->HOSTFLAGS & 0x80) && 0x80);

}





DllExport BOOL APIENTRY CheckIfOwner()
{
	//
	//	Returns TRUE if current process is root process
	//	that loaded the DLL
	//
	
	if (TimerInst == GetCurrentProcessId())

		return (TRUE);
	else
		return (FALSE);
		
}

UINT InitializeExtDriver(PEXTPORTDATA PORTVEC)
{
	HINSTANCE ExtDriver=0;
	char msg[128];
	int err=0;
	HKEY hKey=0;
	UCHAR Value[MAX_PATH];
	
	// If no directory, use current

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,PORTVEC->PORT_DLL_NAME);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,PORTVEC->PORT_DLL_NAME);
	}

	// Several Drivers are now built into bpq32.dll

	_strupr(Value);

	if (strstr(Value, "BPQVKISS"))
		return (UINT) VCOMExtInit;

	if (strstr(Value, "BPQAXIP"))
		return (UINT) AXIPExtInit;

	if (strstr(Value, "BPQETHER"))
		return (UINT) ETHERExtInit;

	if (strstr(Value, "BPQTOAGW"))
		return (UINT) AGWExtInit;

	if (strstr(Value, "AEAPACTOR"))
		return (UINT) AEAExtInit;

	if (strstr(Value, "HALDRIVER"))
		return (UINT) HALExtInit;

	if (strstr(Value, "KAMPACTOR"))
		return (UINT) KAMExtInit;

	if (strstr(Value, "SCSPACTOR"))
		return (UINT) SCSExtInit;

	if (strstr(Value, "WINMOR"))
		return (UINT) WinmorExtInit;
	
	if (strstr(Value, "TELNET"))
		return (UINT) TelnetExtInit;

	ExtDriver=LoadLibrary(Value);

	if (ExtDriver == NULL)
	{
		err=GetLastError();

		wsprintf(msg,"Error loading Driver %s - Error code %d",
				PORTVEC->PORT_DLL_NAME,err);
		
		MessageBox(NULL,msg,"BPQ32",MB_ICONSTOP);

		return(0);
	}

	PORTVEC->DLLhandle=ExtDriver;

	return (UINT)(GetProcAddress(ExtDriver,"_ExtInit@4"));

}

/*
_DATABASE	LABEL	BYTE

FILLER		DB	14 DUP (0)	; PROTECTION AGENST BUFFER PROBLEMS!
			DB	MAJORVERSION,MINORVERSION
NEIGHBOURS	DD	0
			DW	TYPE ROUTE
MAXNEIGHBOURS	DW	20		; MAX ADJACENT NODES

DESTS		DD	0		; NODE LIST
			DW	TYPE DEST_LIST
MAXDESTS	DW	100		; MAX NODES IN SYSTEM
*/


DllExport int APIENTRY GetAttachedProcesses()
{
	return (AttachedProcesses);
}

DllExport int * APIENTRY GetAttachedProcessList()
{
	return (&AttachedPIDList[0]);
}

DllExport int * APIENTRY SaveNodesSupport()
{
	return (&DATABASE);
}

//
//	Internal BPQNODES support
//

#define UCHAR unsigned char

/*
ROUTE ADD G1HTL-1 2 200  0 0 0
ROUTE ADD G4IRX-3 2 200  0 0 0
NODE ADD MAPPLY:G1HTL-1 G1HTL-1 2 200 G4IRX-3 2 98 
NODE ADD NOT:GB7NOT G1HTL-1 2 199 G4IRX-3 2 98 

*/

struct DEST_LIST * Dests;
struct ROUTE * Routes;

int MaxNodes;
int MaxRoutes;
int NodeLen;
int RouteLen;

int count;
int cursor;

int len,i;
	
ULONG cnt;
char Normcall[10];
char Portcall[10];
char Alias[7];

char line[100];

HANDLE handle;

DllExport int ConvFromAX25(unsigned char * incall,unsigned char * outcall)
{
	int in,out=0;
	unsigned char chr;

//
//	CONVERT AX25 FORMAT CALL IN [ESI] TO NORMAL FORMAT IN NORMCALL
//	   RETURNS LENGTH IN CX AND NZ IF LAST ADDRESS BIT IS SET
//

	memset(outcall,0x20,10);

	for (in=0;in<6;in++)
	{
		chr=incall[in];
		if (chr == 0x40)
			break;
		chr >>= 1;
		outcall[out++]=chr;
	}

	chr=incall[6];				// ssid
	chr >>= 1;
	chr	&= 15;

	if (chr > 0)
	{
		outcall[out++]='-';
		if (chr > 9)
		{
			chr-=10;
			outcall[out++]='1';
		}
		chr+=48;
		outcall[out++]=chr;
	}
	return (out);
}

int DoRoutes()
{
	char digis[30]="";
	char locked[3];

	for (count=0; count<MaxRoutes; count++)
	{
		if (Routes->NEIGHBOUR_CALL[0] != 0)
		{
			len=ConvFromAX25(Routes->NEIGHBOUR_CALL,Normcall);
			Normcall[len]=0;

			if ((Routes->NEIGHBOUR_FLAG & 1) == 1)
	
				strcpy(locked," !");
			else
				strcpy(locked," ");


			if (Routes->NEIGHBOUR_DIGI1[0] != 0)
			{
				memcpy(digis," VIA ",5);

				len=ConvFromAX25(Routes->NEIGHBOUR_DIGI1,Portcall);
				Portcall[len]=0;
				strcpy(&digis[5],Portcall);

				if (Routes->NEIGHBOUR_DIGI2[0] != 0)
				{
					len=ConvFromAX25(Routes->NEIGHBOUR_DIGI2,Portcall);
					Portcall[len]=0;
					strcat(digis," ");
					strcat(digis,Portcall);
				}
			}

			cursor=wsprintf(line,
					"ROUTE ADD %s %d %d%s%s %d %d %d %d \r\n",
					Normcall,
					Routes->NEIGHBOUR_PORT,
					Routes->NEIGHBOUR_QUAL,	locked, digis,
					Routes->NBOUR_MAXFRAME,
					Routes->NBOUR_FRACK,
					Routes->NBOUR_PACLEN,
					Routes->INP3Node | (Routes->NoKeepAlive << 1));	

			WriteFile(handle,line,cursor,&cnt,NULL);
		}
		
		Routes+=1;
	}

	return (0);
}

int DoNodes()
{
	Dests-=1;

	for (count=0; count<MaxNodes; count++)
	{
		Dests+=1;
		
		if (Dests->NRROUTE1.ROUT_NEIGHBOUR == 0)
			continue;

		__try 
		{
			len=ConvFromAX25(Dests->DEST_CALL,Normcall);
			Normcall[len]=0;

			memcpy(Alias,Dests->DEST_ALIAS,6);
		
			Alias[6]=0;

			for (i=0;i<6;i++)
			{
				if (Alias[i] == ' ')
					Alias[i] = 0;
			}

			cursor=wsprintf(line,"NODE ADD %s:%s ", Alias,Normcall);
				
			if (Dests->NRROUTE1.ROUT_NEIGHBOUR != 0 && Dests->NRROUTE1.ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE1.ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=wsprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE1.ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE1.ROUT_QUALITY);
	
				cursor+=len;

				if (Dests->NRROUTE1.ROUT_OBSCOUNT > 127)
				{
					len=wsprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

			if (Dests->NRROUTE2.ROUT_NEIGHBOUR != 0 && Dests->NRROUTE2.ROUT_NEIGHBOUR->INP3Node == 0)
			{
				len=ConvFromAX25(
					Dests->NRROUTE2.ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
				Portcall[len]=0;

				len=wsprintf(&line[cursor],"%s %d %d ",
					Portcall,
					Dests->NRROUTE2.ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
					Dests->NRROUTE2.ROUT_QUALITY);
	
				cursor+=len;

				if (Dests->NRROUTE2.ROUT_OBSCOUNT > 127)
				{
					len=wsprintf(&line[cursor],"! ");
					cursor+=len;
				}
			}

		if (Dests->NRROUTE3.ROUT_NEIGHBOUR != 0 && Dests->NRROUTE3.ROUT_NEIGHBOUR->INP3Node == 0)
		{
			len=ConvFromAX25(
				Dests->NRROUTE3.ROUT_NEIGHBOUR->NEIGHBOUR_CALL,Portcall);
			Portcall[len]=0;

			len=wsprintf(&line[cursor],"%s %d %d ",
				Portcall,
				Dests->NRROUTE3.ROUT_NEIGHBOUR->NEIGHBOUR_PORT,
				Dests->NRROUTE3.ROUT_QUALITY);
	
			cursor+=len;

			if (Dests->NRROUTE3.ROUT_OBSCOUNT > 127)
			{
				len=wsprintf(&line[cursor],"! ");
				cursor+=len;
			}
		}

		if (cursor > 30)
		{
			line[cursor++]='\r';
			line[cursor++]='\n';
			WriteFile(handle,line,cursor,&cnt,NULL);
		}
}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		Debugprintf("BPQ32 *** Program Error in DONODES");
	}
	}

	return (0);
}
int APIENTRY Restart()
{
	int i, Count = AttachedProcesses;
	HANDLE hProc;
	DWORD PID;

	for (i = 0; i < Count; i++)
	{
		PID = AttachedPIDList[i];
		
		// Kill Timer Owner last

		if (TimerInst != PID)
		{
			hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);

			if (hProc)
			{
				TerminateProcess(hProc, 0);
				CloseHandle(hProc);
			}
		}
	}

	hProc =  OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, TimerInst);

		if (hProc)
		{
			TerminateProcess(hProc, 0);
			CloseHandle(hProc);
		}

	
	return 0;
}

int APIENTRY Reboot()
{
	// Run shutdown -r -f

	STARTUPINFO  SInfo;
    PROCESS_INFORMATION PInfo;
	char Cmd[] = "shutdown -r -f";

	SInfo.cb=sizeof(SInfo);
	SInfo.lpReserved=NULL; 
	SInfo.lpDesktop=NULL; 
	SInfo.lpTitle=NULL; 
	SInfo.dwFlags=0; 
	SInfo.cbReserved2=0; 
  	SInfo.lpReserved2=NULL; 

	return CreateProcess(NULL, Cmd, NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);
}

int APIENTRY Reconfig()
{
	if (!ProcessConfig())
	{
		return (0);
	}
	SaveNodes();
	WritetoConsole("Nodes Saved\n");
	ReconfigFlag=TRUE;	
	WritetoConsole("Reconfig requested ... Waiting for Timer Poll\n");
	return 1;
}

DllExport int APIENTRY SaveNodes ()
{
	char FN[MAX_PATH];

	Routes = DataBase->NEIGHBOURS;
	RouteLen = DataBase->ROUTE_LEN;
	MaxRoutes = DataBase->MAXNEIGHBOURS;

	Dests = DataBase->DESTS;
	NodeLen = DataBase->DEST_LIST_LEN;
	MaxNodes = MAXDESTS;

	// Set up pointer to BPQNODES file

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"BPQNODES.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"\\");
		strcat(FN,"BPQNODES.dat");
	}

	handle = CreateFile(FN,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	DoRoutes();
	DoNodes();

 	CloseHandle(handle);
	
	return (0);
}

DllExport int APIENTRY ClearNodes ()
{
	char FN[MAX_PATH];

	// Set up pointer to BPQNODES file

	if (BPQDirectory[0] == 0)
	{
		strcpy(FN,"BPQNODES.dat");
	}
	else
	{
		strcpy(FN,BPQDirectory);
		strcat(FN,"\\");
		strcat(FN,"BPQNODES.dat");
	}

	handle = CreateFile(FN,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

 	CloseHandle(handle);
	
	return (0);
}

// Code to support minimizing all BPQ Apps to a single Tray ICON

// As we can't minimize the console window to the tray, I'll use an ordinary
// window instead. This also gives me somewhere to post the messages to


#include "SHELLAPI.H"
#include "kernelresource.h"

char AppName[] = "BPQ32";
char Title[80] = "BPQ32.dll Console";

int NewLine();


LOGFONT LFTTYFONT ;

HFONT hFont ;

HMENU hPopMenu;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int SetupConsoleWindow()
{
    WNDCLASS  wc;
	int i;
	HMENU hMenu;
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80];


	// Create Console Window
        
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON2));     
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);    
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	
	wc.lpszMenuName	 = 0;     
	wc.lpszClassName = AppName;

	i=RegisterClass(&wc);
		
//	GetVersionInfo("bpq32.dll");

	wsprintf (Title, "BPQ32.dll Console Version %s", VersionString);

	hWnd = CreateWindow(AppName, Title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 500,450,
		NULL, NULL, hInstance, NULL);

	i=GetLastError();

	if (!hWnd) {
		return (FALSE);
	}

	hMenu=CreateMenu();
	hPopMenu=CreatePopupMenu();
	SetMenu(hWnd,hMenu);

	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu,"Actions");

	AppendMenu(hPopMenu,MF_STRING,BPQSAVENODES,"Save Nodes to file BPQNODES.DAT");
	AppendMenu(hPopMenu,MF_STRING,BPQRECONFIG,"Save Nodes, Re-read bpq32.cfg and reconfigure node");
	AppendMenu(hPopMenu,MF_STRING,BPQCLEARRECONFIG,"Clear Nodes, Re-read bpq32.cfg and reconfigure node");
	AppendMenu(hPopMenu,MF_STRING,BPQDUMP,"Diagnostic Dump to file BPQDUMP");

	AppendMenu(hPopMenu,MF_STRING | (StartMinimized)? MF_CHECKED:MF_UNCHECKED, BPQSTARTMIN, "Start Minimized" );
	AppendMenu(hPopMenu,MF_STRING | (MinimizetoTray)? MF_CHECKED:MF_UNCHECKED, BPQMINTOTRAY, "Minimize to Notification Area (System Tray)" );
	
	DrawMenuBar(hWnd);	

	// setup default font information

   LFTTYFONT.lfHeight =			12;
   LFTTYFONT.lfWidth =          8 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        OEM_CHARSET ;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
   lstrcpy( LFTTYFONT.lfFaceName, "Fixedsys" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
	
	SetWindowText(hWnd,Title);

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                "SOFTWARE\\G8BPQ\\BPQ32",    
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"WindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);
	}

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hWnd, &Rect);
	}


	MoveWindow(hWnd,Rect.left,Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);


		
	if (StartMinimized)
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hWnd, SW_RESTORE);


	SetupTrayIcon();

	return 0;

}


DllExport int APIENTRY SetupTrayIcon()
{
	if (MinimizetoTray == 0) 
		return 0;

	trayMenu = CreatePopupMenu();

	for( i = 0; i < 100; ++i )
	{
		if (strcmp(PopupText[i],"BPQ32 Console") == 0)
		{
			hWndArray[i]=hWnd;
			goto doneit;
		}
	}

	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] == 0)
		{
			hWndArray[i]=hWnd;
			strcpy(PopupText[i],"BPQ32 Console");
			break;
		}
	}
doneit:

	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] != 0)
			AppendMenu(trayMenu,MF_STRING,40000+i,PopupText[i]);
	}

	//	Set up Tray ICON

	ZeroMemory(&niData,sizeof(NOTIFYICONDATA));

	niData.cbSize = sizeof(NOTIFYICONDATA);

	// the ID number can be any UINT you choose and will
	// be used to identify your icon in later calls to
	// Shell_NotifyIcon

	niData.uID = TRAY_ICON_ID;

	// state which structure members are valid
	// here you can also choose the style of tooltip
	// window if any - specifying a balloon window:
	// NIF_INFO is a little more complicated 

	strcpy(niData.szTip,"BPQ32 Windows"); 

	niData.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;

	// load the icon note: you should destroy the icon
	// after the call to Shell_NotifyIcon

	niData.hIcon = 
		
		//LoadIcon(NULL, IDI_APPLICATION);

		(HICON)LoadImage( hInstance,
			MAKEINTRESOURCE(IDI_ICON2),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_DEFAULTCOLOR);


	// set the window you want to receive event messages

	niData.hWnd = hWnd;

	// set the message to send
	// note: the message value should be in the
	// range of WM_APP through 0xBFFF

	niData.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

	//	Call Shell_NotifyIcon. NIM_ADD adds a new tray icon

	Shell_NotifyIcon(NIM_ADD,&niData);

	return 0;
}

VOID SaveConfig()
{
	HKEY hKey=0;
	int retCode, disp;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey, "Start Minimized", 0, REG_DWORD, (UCHAR *)&StartMinimized, 4);
		retCode = RegSetValueEx(hKey, "Minimize to Tray", 0, REG_DWORD, (UCHAR *)&MinimizetoTray, 4);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
    POINT pos;
	HWND handle;

	PAINTSTRUCT ps;
	HDC hdc;
    HFONT    hOldFont ;


	switch (message) { 

		case MY_TRAY_ICON_MESSAGE:
			
			switch(lParam)
			{
			case WM_RBUTTONUP:	
			case WM_LBUTTONUP:

				GetCursorPos(&pos);

				SetForegroundWindow(hWnd);

				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, hWnd, 0);
				return 0;
			}
	
		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!
			
			if (wmId == BPQSAVENODES)
			{
				SaveNodes();
				WritetoConsole("Nodes Saved\n");
				return 0;
			}		
			if (wmId == BPQCLEARRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}
		
				ClearNodes();
				WritetoConsole("Nodes file Cleared\n");
				ReconfigFlag=TRUE;	
				WritetoConsole("Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}
			if (wmId == BPQRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}
				SaveNodes();
				WritetoConsole("Nodes Saved\n");
				ReconfigFlag=TRUE;	
				WritetoConsole("Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}

			if (wmId == BPQDUMP)
			{
				DumpSystem();
				return 0;
			}

			if (wmId == BPQMINTOTRAY)
			{
				MinimizetoTray = !MinimizetoTray;
				
				if (MinimizetoTray)
					CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_CHECKED);
				else
					CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_UNCHECKED);

				SaveConfig();
				return 0;
			}

			if (wmId == BPQSTARTMIN)
			{
				StartMinimized = !StartMinimized;
				
				if (StartMinimized)
					CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_CHECKED);
				else
					CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_UNCHECKED);

				SaveConfig();
				return 0;
			}

			if (wmId >= 40000 && wmId < 40100)
			{ 
				handle=hWndArray[wmId-40000];
				PostMessage(handle, WM_SYSCOMMAND, SC_RESTORE, 0);
				//ShowWindow(handle, SW_RESTORE);
				SetForegroundWindow(handle);
				return 0;
			}		
	
		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

			case  SC_MINIMIZE: 

				GetWindowRect(hWnd, &Rect);

				if (MinimizetoTray)

					return ShowWindow(hWnd, SW_HIDE);
				else
					return (DefWindowProc(hWnd, message, wParam, lParam));
							
				break;
			
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);
			
			hOldFont = SelectObject( hdc, hFont) ;
			
			for (i=0; i<SCREENLEN; i++)
			{
				TextOut(hdc,0,i*14,&Screen[i*LINELEN],LINELEN);
			}
			
			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);
	
			break;        


		case WM_DESTROY:
		
//			SessionControl(Stream, 2, 0);
//			DeallocateStream(Stream);
//			PostQuitMessage(0);
			
			break;

		case WM_CHAR:
		
			if (wParam == 03)
			{
				DumpSystem();	
				return 0;
			}

 		case WM_CLOSE:
		
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

DllExport BOOL APIENTRY GetMinimizetoTrayFlag()
{
	return MinimizetoTray;
}

DllExport BOOL APIENTRY GetStartMinimizedFlag()
{
	return StartMinimized;
}

DllExport int APIENTRY AddTrayMenuItem(HWND hWnd, char * Label)
{
	int i;
	
	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] == 0)
		{
			hWndArray[i]=hWnd;
			PIDArray[i] = GetCurrentProcessId();
			strcpy(PopupText[i],Label);
			return AppendMenu(trayMenu,MF_STRING,40000+i,Label);
		}
	}

	return -1;

}
 
DllExport int APIENTRY DeleteTrayMenuItem(HWND hWnd)
{
	int i;
	
	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] == hWnd)
		{
			hWndArray[i] = 0;
			PIDArray[i] = 0;
			return DeleteMenu(trayMenu,40000+i,MF_BYCOMMAND);
		}
	}
	return -1;
}

int WritetoConsoleLocal(char * buff);

DllExport int APIENTRY WritetoConsole(char * buff)
{
	return WritetoConsoleLocal(buff);
}

int WritetoConsoleLocal(char * buff)
{
	int ptr;
	int len=strlen(buff);
	
	ptr=0;

	while (ptr < len) 
	{
		if (buff[ptr] == 13)
		{
			col=0;
			ptr++;
			continue;
		}

		if (buff[ptr] == 10)
		{
			NewLine();
			ptr++;
			continue;
		}

		Screen[lineno*LINELEN+col] = buff[ptr];
		col++;
		if (col == LINELEN)	NewLine();
		ptr++;
	
	}

	InvalidateRect(hWnd,NULL,FALSE);

	return (0);
}
int NewLine()
{
	col=0;
	lineno++;
	if (lineno > (SCREENLEN-1))
	{
		memmove(Screen,Screen+LINELEN,LINELEN*(SCREENLEN-1));
		memset(Screen+LINELEN*(SCREENLEN-1),' ',LINELEN);
		lineno=SCREENLEN-1;
	}

	return (0);
}

DllExport VOID APIENTRY  BPQOutputDebugString(char * String)
{
	OutputDebugString(String);
	return;
 }

HANDLE handle;
char fn[]="BPQDUMP";
ULONG cnt;
char * stack;
//char screen[1920];
//COORD ReadCoord;

DllExport int APIENTRY  DumpSystem()
{
	char fn[200];
	char Msg[250];

	wsprintf(fn,"%s\\BPQDUMP",BPQDirectory);

	handle = CreateFile(fn,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	_asm {

	mov	stack,esp
	}

	WriteFile(handle,stack,128,&cnt,NULL);

	WriteFile(handle,Screen,MAXLINELEN*MAXSCREENLEN,&cnt,NULL);

	WriteFile(handle,&Flag,(&ENDOFDATA - &Flag) * 4,&cnt,NULL);

 	CloseHandle(handle);

	wsprintf(Msg, "Dump to %s Completed\n", fn);
	WritetoConsole(Msg);

	return (0);
}

BOOLEAN CheckifBPQ32isLoaded()
{
	HANDLE Mutex;
	
	OutputDebugString("BPQ32 perl program or ntvdm attaching - Check if BPQ32 already loaded\n");

	// See if BPQ32 is running - if we create it in the NTVDM address space by
	// loading bpq32.dll it will not work.

	Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

	if (Mutex == NULL)
	{	
		OutputDebugString("BPQ32 No other bpq32 programs running - Loading BPQ32.exe\n");
		return StartBPQ32();
	}

	CloseHandle(Mutex);

	return TRUE;
}

BOOLEAN StartBPQ32()
{
	UCHAR Value[100];

	char bpq[]="BPQ32.exe";
	char *fn=(char *)&bpq;
	HKEY hKey=0;
	int ret,Type,Vallen=99;

	char Errbuff[100];
	char buff[20];		

	STARTUPINFO  StartupInfo;					// pointer to STARTUPINFO 
    PROCESS_INFORMATION  ProcessInformation; 	// pointer to PROCESS_INFORMATION 
	
// Get address of BPQ Directory

	Value[0]=0;

	ret = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (ret == ERROR_SUCCESS)
	{
		ret = RegQueryValueEx(hKey, "BPQ Directory", 0, &Type,(UCHAR *)&Value, &Vallen);
		
		if (ret == ERROR_SUCCESS)
		{
			if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
				Value[0]=0;
		}


		if (Value[0] == 0)
		{
		
			// BPQ Directory absent or = "" - "try Config File Location"
			
			ret = RegQueryValueEx(hKey,"Config File Location",0,			
							&Type,(UCHAR *)&Value,&Vallen);

			if (ret == ERROR_SUCCESS)
			{
				if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
					Value[0]=0;
			}

		}
		RegCloseKey(hKey);
	}
				
	if (Value[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcat(Value,"\\");
		strcat(Value,fn);				
	}

	StartupInfo.cb=sizeof(StartupInfo);
	StartupInfo.lpReserved=NULL; 
	StartupInfo.lpDesktop=NULL; 
	StartupInfo.lpTitle=NULL; 
	StartupInfo.dwFlags=0; 
	StartupInfo.cbReserved2=0; 
  	StartupInfo.lpReserved2=NULL; 

	if (!CreateProcess(Value,NULL,NULL,NULL,FALSE,
							CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
							NULL,NULL,&StartupInfo,&ProcessInformation))
	{				
		ret=GetLastError();

		_itoa(ret,buff,10);

		strcpy(Errbuff,	"BPQ32 Load ");
		strcat(Errbuff,Value);
		strcat(Errbuff," failed ");
		strcat(Errbuff,buff);
		OutputDebugString(Errbuff);

		return FALSE;		
	}

	AttachingProcess = 1;

	return TRUE;
}

VOID DigiToMultiplePorts(struct PORTCONTROL * PORTVEC, PMESSAGE Msg)
{
	USHORT Mask=PORTVEC->DIGIMASK;

	for (i=1; i<=NUMBEROFPORTS; i++)
	{
		if (Mask & 1)
		{
			// Block includes the Msg Header (7 bytes), Len Does not!

			Send_AX(Msg, Msg->LENGTH - 7, i);
			Mask>>=1;
		}
	}
}
DllExport VOID APIENTRY Send_AX(PMESSAGE Block, DWORD Len, UCHAR Port)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have

	// Block includes the Msg Header (7 bytes), Len Does not!

	_asm {

	pushfd
	cld
	pushad

	mov	al,Port
	mov	ah,10
	mov	ecx,Len
	mov	esi,Block
	add	esi,7

	call	RAWTX

	popad
	popfd
	}
	return;

}

DllExport struct BPQVECSTRUC * APIENTRY GetIPVectorAddr()
{
	return &IPHOSTVECTOR;
}

DllExport UINT APIENTRY GETSENDNETFRAMEADDR()
{
	return (UINT)&SENDNETFRAME;
}

DllExport VOID APIENTRY RelBuff(PMESSAGE Msg)
{
	_asm{

		mov	edi,Msg
		call RELBUFF
	}
}

DllExport PMESSAGE APIENTRY GetBuff()
{
	_asm{
 		call GETBUFF
		mov eax, edi
	}
}


VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

VOID APIENTRY md5 (char *arg, unsigned char * checksum);


DllExport VOID APIENTRY CreateOneTimePassword(char * Password, char * KeyPhrase, int TimeOffset)
{
	// Create a time dependent One Time Password from the KeyPhrase
	// TimeOffset is used when checking to allow for slight variation in clocks

	time_t NOW = time(NULL);
	UCHAR Hash[16];
	char Key[1000];
	int i, chr;

	NOW = NOW/30 + TimeOffset;				// Only Change every 30 secs

	wsprintf(Key, "%s%x", KeyPhrase, NOW);

	md5(Key, Hash);

	for (i=0; i<16; i++)
	{
		chr = (Hash[i] & 31);
		if (chr > 9) chr += 7;
		
		Password[i] = chr + 48; 
	}

	Password[16] = 0;

	Debugprintf("%s %d", Password, TimeOffset);

	return;
}
DllExport BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase)
{
	char CheckPassword[17];
	
	CreateOneTimePassword(CheckPassword, KeyPhrase, 0);

	if (memcmp(Password, CheckPassword, 16) == 0)
		return TRUE;

	CreateOneTimePassword(CheckPassword, KeyPhrase, -1);

	if (memcmp(Password, CheckPassword, 16) == 0)
		return TRUE;

	CreateOneTimePassword(CheckPassword, KeyPhrase, 1);

	if (memcmp(Password, CheckPassword, 16) == 0)
		return TRUE;
	CreateOneTimePassword(CheckPassword, KeyPhrase, -2);

	if (memcmp(Password, CheckPassword, 16) == 0)
		return TRUE;

	CreateOneTimePassword(CheckPassword, KeyPhrase, 2);

	if (memcmp(Password, CheckPassword, 16) == 0)
		return TRUE;

	CreateOneTimePassword(CheckPassword, KeyPhrase, -3);

	if (memcmp(Password, CheckPassword, 16) == 0)
		return TRUE;

	return FALSE;
}


// C Routines to access the main BPQ32 buffer pool

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

	QCOUNT++;

	return 0;
}

int C_Q_ADD(UINT *Q,UINT *BUFF)
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

VOID SendLocation()
{
	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[100];
	int Len;

	Len = wsprintf(Msg, "%s %s", LOCATOR, VersionString);

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, ReportDest, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, Msg, Len);

	SendRaw(AXIPPort, (char *)&AXMSG.DEST, Len + 16);

	return;

}

VOID SendMH(int Hardware, char * call, char * freq, char * LOC, char * Mode)
{
	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[100];
	int Len;

	if (AXIPPort == 0 || LOCATOR[0] == 0)
		return;

	Len = wsprintf(Msg, "MH %s,%s,%s,%s", call, freq, LOC, Mode);

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, ReportDest, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, Msg, Len);

	if (Hardware == 1)		// H_WINMOR
	{
		GetSemaphore();
		Send_AX(&AXMSG, Len + 16, AXIPPort) ;
		FreeSemaphore(0);
	}
	else
		Send_AX(&AXMSG, Len + 16, AXIPPort) ;

	return;

}

