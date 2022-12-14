//
//	409l	Oct 2001 Fix l3timeout for KISS
//
//	409m	Oct 2001 Fix Crossband Digi
//
//	409n	May 2002 Change error handling on load ext DLL

//	409p	March 2005 Allow Multidigit COM Ports (kiss.c)

//	409r	August 2005 Treat NULL string in Registry as use current directory
//						Allow shutdown to close BPQ Applications

//	409s	October 2005 Add DLL:Export entries to API for BPQTNC2

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
//				Fix count of nodes in Stats Display
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

// Move rigconrtol display to driver windows
// Move rigcontrol config to driver config.
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
// Write Telnet log to BPQ Directory
// Add Port to AXIP resolver display
// Send Reports to update.g8bpq.net:81
// Add support for FT100 to Rigcontrol
// Add timeout to Rigcontrol PTT
// Add Save Registry Command

// 410p		Build 8 November 2010

// Add NOKEEPALIVES Port Param
// Renumbered for release

// 410p		Build 9 November 2010

// Get Bandwith for map report from WL2K Report Command
// Fix freq display for FT100 (was KHz, not MHz)
// Don't try to change SCS mode whilst initialising
// Allow reporting of Lat/Lon as well as Locator
// Fix Telnet Log Name
// Fix starting with Minimized windows when Minimizetotray isn't set
// Extra Program Error trapping in SessionControl
// Fix reporting same freq with different bandwidths at different times.
// Code changes to support SCS Robust Packet Mode.
// Add FT2000 to Rigcontrol
// Only Send CTEXT to connects to Node (not to connects to an Application Call)

// Released as Build 10

// 410p		Build 11 January 2011

// Fix MH Update for SCS Outgoing Calls
// Add Direct CMS Access to TelnetServer
// Restructure DISCONNECT processing to run in Timer owning process

// 410p		Build 12 January 2011

// Add option for Hardware PTT to use a different com port from the scan port 
// Add CAT PTT for Yaesu 897 (and maybe others)
// Fix RMS Packet ports busy after restart
// Fix CMS Telnet with MAXSESSIONS > 10

// 410p		Build 13 January 2011

// Fix loss of buffers in TelnetServer
// Add CMS logging.
// Add non - Promiscuous mode option for BPQETHER 

// 410p		Build 14 January 2011

// Add support for BPQTermTCP
// Allow more that one FBBPORT
// Allow Telnet FBB mode sessions to send CRLF as well as CR on user and pass msgs
// Add session length to CMS Telnet logging.
// Return Secure Session Flag from GetConnectionInfo
// Show Uptime as dd/hh/mm

// 4.10.16.17	March 2011

// Add "Close all programs" command
// Add BPQ Program Directory registry key
// Use HKEY_CURRENT_USER on Vista and above (and move registry if necessary)
// Time out IP Gateway ARP entries, and only reload ax.25 ARP entries
// Add support for SCS Tracker HF Modes
// Fix WL2K Reporting
// Report Version to WL2K
// Add Driver to support Tracker with multiple sessions (but no scanning, wl2k report, etc)


// Above released as 5.0.0.1

// 5.2.0.1

// Add caching of CMS Server IP addresses
// Initialise TNC State on Pactor Dialogs
// Add Shortened (6 digit) AUTH mode.
// Update MH with all frames (not just I/UI)
// Add IPV6 Support for TelnetServer and AXIP
// Fix TNC OK Test for Tracker
// Fix crash in CMS mode if terminal disconnects while tcp commect in progress
// Add WL2K reporting for Robust Packet
// Add option to suppress WL2K reporting for specific frequencies
// Fix Timeband processing for Rig Control
// New Driver for SCS Tracker allowing multiple connects, so Tracker can be used for user access 
// New Driver for V4 TNC

// 5.2.1.3 October 2011

// Combine busy detector on Interlocked Ports (SCS PTC, WINMOR or KAM)
// Improved program error logging
// WL2K reporting changed to new format agreed with Lee Inman

// 5.2.3.1 January 2012

// Connects from the console to an APPLCALL or APPLALIAS now invoke any Command Alias that has been defined.
// Fix reporting of Tracker freqs to WL2K.
// Fix Tracker monitoring setup (sending M UISC)
// Fix possible call/application routing error on RP
// Changes for P4Dragon
// Include APRS Digi/IGate
// Tracker monitoring now includes DIGIS
// Support sending UI frames using SCSTRACKER, SCTRKMULTI and UZ7HO drivers
// Include driver for UZ7HO soundcard modem.
// Accept DRIVER as well as DLLNAME, and COMPORT as well as IOADDR in bpq32.cfg. COMPORT is decimal
// No longer supports separate config files, or BPQTELNETSERVER.exe
// Improved flow control for Telnet CMS Sessions
// Fix handling Config file without a newline after last line
// Add non - Promiscuous mode option for BPQETHER 
// Change Console Window to a Dialog Box.
// Fix possible corruption and loss of buffers in Tracker drivers
// Add Beacon After Session option to Tracker and UZ7HO Drivers
// Rewrite RigControl and add "Reread Config Command"
// Support User Mode VCOM Driver for VKISS ports

// 5.2.4.1 January 2012

// Remove CR from Telnet User and Password Prompts
// Add Rigcontrol to UZ7HO driver
// Fix corruption of Free Buffer Count by Rigcontol
// Fix WINMOR and V4 PTT
// Add MultiPSK Driver
// Add SendBeacon export for BPQAPRS
// Add SendChatReport function
// Fix check on length of Port Config ID String with trailing spaces
// Fix interlock when Port Number <> Port Slot
// Add NETROMCALL for L3 Activity
// Add support for APRS Application
// Fix Telnet with FBBPORT and no TCPPORT
// Add Reread APRS Config
// Fix switching to Pactor after scanning in normal packet mode (PTC)

// 5.2.5.1 February 2012

// Stop reading Password file.
// Add extra MPSK commands 
// Fix MPSK Transparency
// Make LOCATOR command compulsory
// Add MobileBeaconInterval APRS param
// Send Course and Speed when APRS is using GPS
// Fix Robust Packet reporting in PTC driver
// Fix corruption of some MIC-E APRS packets

// 5.2.6.1 February 2012

// Convert to MDI presentation of BPQ32.dll windows
// Send APRS Status packets
// Send QUIT not EXIT in PTC Init
// Implement new WL2K reporting format and include traffic reporting info in CMS signon
// New WL2KREPORT format
// Prevent loops when APPL alias refers to itself
// Add RigControl for Flex radios and ICOM IC-M710 Marine radio

// 5.2.7.1

//	Fix opening more thn one console window on Win98
//  Change method of configuring multiple timelots on WL2K reporting
//  Add option to update WK2K Sysop Database
//	Add Web server
//	Add UIONLY port option

// 5.2.7.2

//	Fix handling TelnetServer packets over 500 bytes in normal mode

// 5.2.7.3

//	Fix Igate handling packets from UIView

// 5.2.7.4

//	Prototype Baycom driver.

// 5.2.7.5

//  Set WK2K group ref to MARS (3) if using a MARS service code

// 5.2.7.7

//	Check for programs calling CloseBPQ32 when holding semaphore
//  Try/Except round Status Timer Processing

// 5.2.7.8

//  More Try/Except round Timer Processing

// 5.2.7.9

//	Enable RX in Baycom, and remove test loopback in tx

// 5.2.7.10

//	Try/Except round ProcessHTTPMessage

// 5.2.7.11

//	BAYCOM tweaks

// 5.2.7.13

//	Release semaphore after program error in Timer Processing
//  Check fro valid dest in REFRESHROUTE


//	Add TNC-X KISSOPTION (includes the ACKMODE bytes in the checksum(

// Version 5.2.9.1 Sept 2012

// Fix using KISS ports with COMn > 16
// Add "KISS over UDP" driver for PI as a TNC concentrator
// Fix SCSPACTOR Rig control via PTC Rig COntrol Port

#define Kernel

#include "Versions.h"

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#pragma data_seg("_BPQDATA")

#include "windows.h"
#include "winerror.h"


#include "time.h"
#include "stdio.h"
#include "io.h"
#include <fcntl.h>					 
//#include "vmm.h"
#include "SHELLAPI.H"

#include "AsmStrucs.h"

#include "SHELLAPI.H"
#include "kernelresource.h"
#include <tlhelp32.h>
#include "BPQTermMDI.h"

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
UINT WINAPI SoundModemExtInit(EXTPORTDATA * PortEntry);
UINT WINAPI TrackerExtInit(EXTPORTDATA * PortEntry);
UINT WINAPI TrackerMExtInit(EXTPORTDATA * PortEntry);
UINT WINAPI V4ExtInit(EXTPORTDATA * PortEntry);
UINT WINAPI UZ7HOExtInit(EXTPORTDATA * PortEntry);
UINT WINAPI MPSKExtInit(EXTPORTDATA * PortEntry);
UINT WINAPI BaycomExtInit(EXTPORTDATA * PortEntry);

extern char * Buffer;	// Config Area

extern char AUTOSAVE;

extern char MYNODECALL;	// 10 chars,not null terminated

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

extern HWND hIPResWnd;
extern BOOL IPMinimized;

VOID SaveWindowPos(int port);
VOID SaveAXIPWindowPos(int port);
VOID SetupRTFHddr();
DllExport VOID APIENTRY CreateNewTrayIcon();
int DoReceivedData(int Stream);
int	DoStateChange(int Stream);
int DoMonData(int Stream);
struct ConsoleInfo * CreateChildWindow(int Stream, BOOL DuringInit);
CloseHostSessions();
SaveHostSessions();
VOID SaveBPQ32Windows();
VOID CloseDriverWindow(int port);
VOID CheckWL2KReportTimer();

char EXCEPTMSG[80] = "";

char SIGNONMSG[128] = "";
char SESSIONHDDR[80] = "";
int SESSHDDRLEN = 0;

char WL2KCall[10];
char WL2KLoc[7];

char LOCATOR[80] = "";			// Locator for Reporting - may be Maidenhead or LAT:LON
char MAPCOMMENT[250] = "";		// Locator for Reporting - may be Maidenhead or LAT:LON
char LOC[7] = "";				// Maidenhead Locator for Reporting
char ReportDest[7];

VOID __cdecl Debugprintf(const char * format, ...);
VOID __cdecl Consoleprintf(const char * format, ...);

DllExport int APIENTRY CloseBPQ32();
DllExport int APIENTRY SessionControl(int stream, int command, int param);


BOOL APIENTRY Init_IP();
BOOL APIENTRY Poll_IP();

BOOL APIENTRY Init_APRS();
BOOL APIENTRY Poll_APRS();
VOID HTTPTimer();

BOOL APIENTRY Rig_Init();
BOOL APIENTRY Rig_Close();
BOOL APIENTRY Rig_Poll();
BOOL APIENTRY Rig_Command();

VOID IPClose();
VOID APRSClose();

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
UCHAR BPQProgramDirectory[MAX_PATH]="";

static char BPQWinMsg[] = "BPQWindowMessage";

static char ClassName[] = "BPQMAINWINDOW";

HKEY REGTREE = HKEY_CURRENT_USER;
char REGTREETEXT[100] = "HKEY_CURRENT_USER";

UINT BPQMsg=0;

#define MAXLINELEN 120
#define MAXSCREENLEN 50

#define BGCOLOUR RGB(236,233,216)

HBRUSH bgBrush = NULL;

//int LINELEN=120;
//int SCREENLEN=50;

//char Screen[MAXLINELEN*MAXSCREENLEN]={0};

//int lineno=0;
//int col=0;

#define REPORTINTERVAL 15 * 549;	// Magic Ticks Per Minute for PC's nominal 100 ms timer 
int ReportTimer = 0;

HANDLE OpenConfigFile(char * file);

VOID SetupBPQDirectory();
VOID SendLocation();

unsigned long _beginthread(void(*start_address)(), unsigned stack_size, int arglist);

#define TRAY_ICON_ID	      1		    //		ID number for the Notify Icon
#define MY_TRAY_ICON_MESSAGE  WM_APP	//		the message ID sent to our window

NOTIFYICONDATA niData; 

int SetupConsoleWindow();

BOOL StartMinimized=FALSE;
BOOL MinimizetoTray=TRUE;

BOOL StatusMinimized = FALSE;
BOOL ConsoleMinimized = FALSE;

HMENU trayMenu=0;

HWND hConsWnd = NULL, hWndCons = NULL, hWndBG = NULL, ClientWnd = NULL,  FrameWnd = NULL, StatusWnd = NULL;

BOOL FrameMaximized = FALSE;

BOOL IGateEnabled = TRUE;
extern int ISDelayTimer;			// Time before trying to reopen APRS-IS link
extern int ISPort;

static RECT Rect = {100,100,400,400};	// Console Window Position
static RECT FRect = {100,100,800,600};	// Frame 
static RECT StatusRect = {100,100,850,500};	// Status Window

DllExport int APIENTRY DumpSystem();
DllExport int APIENTRY SaveNodes ();
DllExport int APIENTRY ClearNodes ();
DllExport int APIENTRY SetupTrayIcon();
UINT * Q_REM(UINT *Q);
UINT ReleaseBuffer(UINT *BUFF);


VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

DllExport int APIENTRY DeallocateStream(int stream);

int VECTORLENGTH = sizeof bpqvecstruc;

int FirstEntry = 1;
BOOL CloseLast = TRUE;			// If the user started BPQ32.exe, don't close it when other programs close
BOOL Closing = FALSE;			// Set if Close All called - prevents respawning bpq32.exe

BOOL BPQ32_EXE;					// Set if Process is running BPQ32.exe. Not initialised.
								// Used to Kill surplus BPQ32.exe processes

DWORD Our_PID;					// Our Process ID - local variable

InitDone = 0;
int FirstInitDone = 0;
int PerlReinit = 0;
int TimerHandle = 0;
int SessHandle = 0;

unsigned int TimerInst = 0xffffffff;

HANDLE hInstance = 0;

int AttachedProcesses = 0;
int AttachingProcess = 0;
HINSTANCE hIPModule = 0;
HINSTANCE hRigModule = 0;

BOOL ReconfigFlag = FALSE;
BOOL RigReconfigFlag = FALSE;
BOOL APRSReconfigFlag = FALSE;
BOOL CloseAllNeeded = FALSE;

int AttachedPIDList[100] = {0};

HWND hWndArray[100] = {0};
int PIDArray[100] = {0};
char PopupText[30][100] = {""};

// Next 3 should be uninitialised so they are local to each process

byte	MCOM;
char	MTX;
ULONG	MMASK;
byte	MUIONLY;

UCHAR AuthorisedProgram;			// Local Variable. Set if Program is on secure list

char pgm[256];		// Uninitialised so per process

HANDLE Mutex;

BOOL PartLine = FALSE;
int pindex = 0;

VOID CALLBACK SetupTermSessions(HWND hwnd, UINT  uMsg, UINT  idEvent,  DWORD  dwTime);


TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;
TIMERPROC lpSetupTermSessions = (TIMERPROC) SetupTermSessions;


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
VOID SaveConfig();
VOID CreateRegBackup();
VOID ResolveUpdateThread();
VOID OpenReportingSockets();
DllExport VOID APIENTRY CloseAllPrograms();
DllExport BOOL APIENTRY SaveReg(char * KeyIn, HANDLE hFile);

BOOL IPActive = FALSE;
BOOL IPRequired = FALSE;
BOOL RigRequired = TRUE;
BOOL RigActive = FALSE;
BOOL APRSActive = FALSE;

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

	_asm
	{
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
}
void FreeSemaphore()
{
	SEMRELEASES++;
	SemHeldByAPI = 0;
	Semaphore=0;
}


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

			// Remove Tray Icon Entry

			for( i = 0; i < 100; ++i )
			{
				if (PIDArray[i] == ProcessID)
				{
					hWndArray[i] = 0;
					wsprintf(Log,"BPQ32 Removing Tray Item %s\n", PopupText[i]);
					OutputDebugString(Log);
					DeleteMenu(trayMenu,TRAYBASEID+i,MF_BYCOMMAND);
				}
			}

			// If process had the semaphore, release it

			if (Semaphore == 1 && ProcessID == SemProcessID)
			{
				OutputDebugString("BPQ32 Process was holding Semaphore - attempting recovery\r\n");
				Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
					Sem_eax, Sem_ebx, Sem_ecx, Sem_edx, Sem_esi, Sem_edi); 

				Semaphore = 0;
				SemHeldByAPI = 0;
			}

			for (i=1;i<65;i++)
			{
				if (BPQHOSTVECTOR[i-1].STREAMOWNER == AttachedPIDList[n])
				{
					DeallocateStream(i);
				}
			}
				
			if (TimerInst == ProcessID)
			{
				KillTimer(NULL,TimerHandle);
				TimerHandle=0;
				TimerInst=0xffffffff;
//				Tell_Sessions();
				OutputDebugString("BPQ32 Process was running timer \n");
			
				if (MinimizetoTray)
					Shell_NotifyIcon(NIM_DELETE,&niData);

				
			}
				
			//	Remove this entry from PID List

			for (i=n; i< AttachedProcesses; i++)
			{
				AttachedPIDList[i]=AttachedPIDList[i+1];
			}
			AttachedProcesses--;

			wsprintf(buff,"BPQ32 Lost Process - %d Process(es) Attached\n", AttachedProcesses);
			OutputDebugString(buff);
		}
	}
}
VOID MonitorTimerThread(int x)
{	
	// Thread to detect killed timer process. Runs in all other BPQ32 processes.

	do {

		Sleep(60000);

		if (TimerInst != 0xffffffff && !IsProcess(TimerInst))
		{
			// Timer owning Process has died - Force a new timer to be created
			//	New timer thread will detect lost process and tidy up
		
			Debugprintf("BPQ32 Process %d with Timer died", TimerInst);

			// If process was holding the semaphore, release it

			if (Semaphore == 1 && TimerInst == SemProcessID)
			{
				OutputDebugString("BPQ32 Process was holding Semaphore - attempting recovery\r\n");
				Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
					Sem_eax, Sem_ebx, Sem_ecx, Sem_edx, Sem_esi, Sem_edi); 
				Semaphore = 0;
				SemHeldByAPI = 0;
			}

//			KillTimer(NULL,TimerHandle);
//			TimerHandle=0;
//			TimerInst=0xffffffff;
//			Tell_Sessions();

			CheckforLostProcesses();		// Normally only done in timer thread, which is now dead

			// Timer can only run in BPQ32.exe
			
			if (Closing == FALSE && AttachingProcess == FALSE)
			{
				OutputDebugString("BPQ32 Reloading BPQ32.exe\n");
				StartBPQ32();
			}

//			if (MinimizetoTray)
//				Shell_NotifyIcon(NIM_DELETE,&niData);
		}
	
	} while (TRUE);
}

VOID SetApplPorts()
{
	// If any appl has an alias, get port number

	struct APPLCONFIG * App;
	struct APPLCALLS * APPL;

	char C[80];
	char Port[80];
	char Call[80];

	int i, n;

	App = (struct APPLCONFIG *)&Buffer[ApplOffset];

	for (i=0; i < NumberofAppls; i++)
	{
		APPL=&APPLCALLTABLE[i];

		if (APPL->APPLHASALIAS)
		{
			n = sscanf(App->CommandAlias, "%s %s %s", &C, &Port, &Call);
			if (n == 3)
				APPL->APPLPORT = atoi(Port);
		}
		App++;
	}
}

VOID CALLBACK TimerProc
(
    HWND  hwnd,	// handle of window for timer messages 
    UINT  uMsg,	// WM_TIMER message
    UINT  idEvent,	// timer identifier
    DWORD  dwTime)	// current system time	
{
	struct _EXCEPTION_POINTERS exinfo;

	//
	//	Get semaphore before proceeeding
	//

	GetSemaphore();

	SemHeldByAPI = 2;

	strcpy(EXCEPTMSG, "Timer ReconfigProcessing");
	
	__try 
	{

	if (trayMenu == NULL)
		SetupTrayIcon();

	// See if reconfigure requested

	if (CloseAllNeeded)
	{
		CloseAllNeeded = FALSE;
		CloseAllPrograms();
	}

	if (ReconfigFlag)
	{
		// Only do it it timer owning process, or we could get in a real mess!

		if(TimerInst == GetCurrentProcessId())
		{
			int i;
			struct BPQVECSTRUC * HOSTVEC;
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
			WSADATA       WsaData;            // receives data from WSAStartup
			RECT cRect;

			ReconfigFlag = FALSE;

			SetupBPQDirectory();

			WritetoConsole("Reconfiguring ...\n\n");
			OutputDebugString("BPQ32 Reconfiguring ...\n");	

			GetWindowRect(FrameWnd, &FRect);

			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
				{
					if (PORTVEC->PORT_EXT_ADDR)
					{
						SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						CloseDriverWindow(PORTVEC->PORTCONTROL.PORTNUMBER);
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
					}
				}
				PORTVEC->PORTCONTROL.PORTCLOSECODE(&PORTVEC->PORTCONTROL);
				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
			}

			IPClose();
			APRSClose();
			Rig_Close();
			WSACleanup();

			Sleep(2000);

			WSAStartup(MAKEWORD(2, 0), &WsaData);

			Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
			Consoleprintf(VerCopyright);
	
			_asm{

			pushad
			call START
			call INITIALISEPORTS			// Restart Ports

			popad
			}

			SetApplPorts();

			FreeConfig();

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

			OpenReportingSockets();
		
			WritetoConsole("\n\nReconfiguration Complete\n");

			if (IPRequired)	IPActive = Init_IP();

			APRSActive = Init_APRS();

			if (ISPort == 0)
				IGateEnabled = 0;

			CheckDlgButton(hConsWnd, IDC_ENIGATE, IGateEnabled);

			GetClientRect(hConsWnd, &cRect); 
			MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);
			if (APRSActive)
				MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
			else
			{
				ShowWindow(GetDlgItem(hConsWnd, IDC_GPS), SW_HIDE); 
				MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);
			}
			InvalidateRect(hConsWnd, NULL, TRUE);

			RigActive = Rig_Init();
			
			OutputDebugString("BPQ32 Reconfiguration Complete\n");	
		}
	}


	if (RigReconfigFlag)
	{
		// Only do it it timer owning process, or we could get in a real mess!

		if(TimerInst == GetCurrentProcessId())
		{
			RigReconfigFlag = FALSE;
			Rig_Close();				
			RigActive = Rig_Init();
			
			WritetoConsole("Rigcontrol Reconfiguration Complete\n");	
		}
	}

	if (APRSReconfigFlag)
	{
		// Only do it it timer owning process, or we could get in a real mess!

		if(TimerInst == GetCurrentProcessId())
		{
			APRSReconfigFlag = FALSE;
			APRSClose();				
			APRSActive = Init_APRS();
			
			WritetoConsole("APRS Reconfiguration Complete\n");	
		}
	}

	}
	#include "StdExcept.c"

	if (Semaphore && SemProcessID == GetCurrentProcessId())
		FreeSemaphore();

	}

	strcpy(EXCEPTMSG, "Timer Processing");

	__try 
	{
		if (IPActive) Poll_IP();
		if (RigActive) Rig_Poll();
		if (APRSActive) Poll_APRS();
		CheckWL2KReportTimer();
		
		TIMERINTERRUPT();

		FreeSemaphore();			// SendLocation needs to get the semaphore

		strcpy(EXCEPTMSG, "HTTP Timer Processing");

		HTTPTimer();

		strcpy(EXCEPTMSG, "WL2K Report Timer Processing");

		if (ReportTimer)
		{		
			ReportTimer--;
	
			if (ReportTimer == 0)
			{
				ReportTimer = REPORTINTERVAL;
				SendLocation();
			}
		}
	}
	
	#include "StdExcept.c"

	if (Semaphore && SemProcessID == GetCurrentProcessId())
		FreeSemaphore();

	}

	return;
}

HANDLE NPHandle;

int (WINAPI FAR *GetModuleFileNameExPtr)();

FirstInit()
{
    WSADATA       WsaData;            // receives data from WSAStartup
	HINSTANCE ExtDriver=0;
	RECT cRect;


	// First Time Ports and Timer init

	// Moved from DLLINIT to sort out perl problem, and meet MS Guidelines on minimising DLLMain 

	// Call wsastartup - most systems need winsock, and duplicate statups could be a problem

    WSAStartup(MAKEWORD(2, 0), &WsaData);

	// Load Psapi.dll if possible

	ExtDriver=LoadLibrary("Psapi.dll");

	SetupTrayIcon();

	if (ExtDriver)
		GetModuleFileNameExPtr = (FARPROCX)GetProcAddress(ExtDriver,"GetModuleFileNameExA");
	
	INITIALISEPORTS();

	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
	TimerInst=GetCurrentProcessId();

	OpenReportingSockets();

 	WritetoConsole("\n");
 	WritetoConsole("Port Initialisation Complete\n");

	if (IPRequired)	IPActive = Init_IP();
	
	APRSActive = Init_APRS();

	if (APRSActive)
	{
		hWndBG = CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 0,0,40,546, hConsWnd, NULL, hInstance, NULL);

		CreateWindowEx(0, "STATIC", "Enable IGate", WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
			8,0,90,24, hConsWnd, (HMENU)-1, hInstance, NULL);
		
		CreateWindowEx(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_LEFTTEXT | WS_TABSTOP,
			95,1,18,24, hConsWnd, (HMENU)IDC_ENIGATE, hInstance, NULL);

		CreateWindowEx(0, "STATIC", "IGate State - Disconnected",
			WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 125, 0, 195, 24, hConsWnd, (HMENU)IGATESTATE, hInstance, NULL);

		CreateWindowEx(0, "STATIC", "IGATE Stats - Msgs 0   Local Stns 0",
			WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 320, 0, 240, 24, hConsWnd, (HMENU)IGATESTATS, hInstance, NULL);

		CreateWindowEx(0,  "STATIC", "GPS Off",
			WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 560, 0, 80, 24, hConsWnd, (HMENU)IDC_GPS, hInstance, NULL);
	}

	if (ISPort == 0)
		IGateEnabled = 0;

	CheckDlgButton(hConsWnd, IDC_ENIGATE, IGateEnabled);
	
	GetClientRect(hConsWnd, &cRect); 
	MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);
	if (APRSActive)
		MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
	else
	{
		ShowWindow(GetDlgItem(hConsWnd, IDC_GPS), SW_HIDE); 
		MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);
	}
	InvalidateRect(hConsWnd, NULL, TRUE);

	RigActive = Rig_Init();

	_beginthread(MonitorThread,0,0);
	
	OutputDebugString("BPQ32 Port Initialisation Complete\n");

	return 0;
}

Check_Timer()
{
	if (Closing)
		return 0;

	GetSemaphore();

	if (InitDone == -1)
	{
		Sleep(7000);
		FreeSemaphore();
		exit (0);
	}

	SemHeldByAPI = 3;

	if (FirstInitDone == 0)
	{
		if (_stricmp(pgm, "bpq32.exe") == 0)
		{
			FirstInitDone=1;					// Only init in BPQ32.exe
			FirstInit();
			FreeSemaphore();
			return 0;
		}
		else
		{
			FreeSemaphore();
			return 0;
		}
	}

	if (TimerHandle == 0)
	{
	    WSADATA       WsaData;            // receives data from WSAStartup
		HINSTANCE ExtDriver=0;
		RECT cRect;

		// Only attach timer to bpq32.exe

		if (_stricmp(pgm, "bpq32.exe") != 0)
		{
			FreeSemaphore();
			return 0;
		}

		OutputDebugString("BPQ32 Reinitialising External Ports and Attaching Timer\n");

		if (!ProcessConfig())
		{
			ShowWindow(hConsWnd, SW_RESTORE);
			SendMessage(hConsWnd, WM_PAINT, 0, 0);

			InitDone = -1;
			FreeSemaphore();

			MessageBox(NULL,"Configuration File Error","BPQ32",MB_ICONSTOP);

			exit (0);
		}

		GetVersionInfo("bpq32.dll");

		SetupConsoleWindow();

		Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
		Consoleprintf(VerCopyright);
		Consoleprintf("Reinitialising...");
 
		SetupBPQDirectory();

		Sleep(1000);			// Allow time for sockets to close	

		WSAStartup(MAKEWORD(2, 0), &WsaData);

		// Load Psapi.dll if possible

		ExtDriver = LoadLibrary("Psapi.dll");

		SetupTrayIcon();

		if (ExtDriver)
			GetModuleFileNameExPtr = (FARPROCX)GetProcAddress(ExtDriver,"GetModuleFileNameExA");

		INITIALISEPORTS();

		OpenReportingSockets();

		WritetoConsole("\n\nPort Reinitialisation Complete\n");

		TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);
		TimerInst=GetCurrentProcessId();

		BPQMsg = RegisterWindowMessage(BPQWinMsg);

		CreateMutex(NULL,TRUE,"BPQLOCKMUTEX");

//		NPHandle=CreateNamedPipe("\\\\.\\pipe\\BPQ32pipe",
//					PIPE_ACCESS_DUPLEX,0,64,4096,4096,1000,NULL);

		if (IPRequired)	IPActive = Init_IP();

		RigActive = Rig_Init();

		if (IPRequired)	IPActive = Init_IP();
	
		APRSActive = Init_APRS();

		if (ISPort == 0)
			IGateEnabled = 0;

		CheckDlgButton(hConsWnd, IDC_ENIGATE, IGateEnabled);
	
		GetClientRect(hConsWnd, &cRect); 
		MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);

		if (APRSActive)
			MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
		else
		{
			ShowWindow(GetDlgItem(hConsWnd, IDC_GPS), SW_HIDE); 
			MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);
		}
		InvalidateRect(hConsWnd, NULL, TRUE);

		FreeConfig();

		_beginthread(MonitorThread,0,0);

		ReportTimer = 0;

		OpenReportingSockets();

		FreeSemaphore();
	
		if (StartMinimized)
			if (MinimizetoTray)
				ShowWindow(FrameWnd, SW_HIDE);
			else
				ShowWindow(FrameWnd, SW_SHOWMINIMIZED);	
		else
			ShowWindow(FrameWnd, SW_RESTORE);

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


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	DWORD n;
	char buf[350];

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
			  
		if (sizeof(LINKTABLE) != DataBase->LINK_TABLE_LEN)
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
		SemHeldByAPI = 4;

		BPQHOSTVECPTR = &BPQHOSTVECTOR[0];
		
		LoadToolHelperRoutines();

		Our_PID = GetCurrentProcessId();

		GetProcess(Our_PID, pgm);

		if (_stricmp(pgm, "regsvr32.exe") == 0 || _stricmp(pgm, "bpqcontrol.exe") == 0)
		{
			AttachedProcesses++;			// We will get a detach
			FreeSemaphore();
			return 1;
		}

		if (_stricmp(pgm,"BPQ32.exe") == 0)
			BPQ32_EXE = TRUE;

		if (FirstEntry)				// If loaded by BPQ32.exe, dont close it at end
		{
			FirstEntry = 0;
			if (BPQ32_EXE)
				CloseLast = FALSE;
		}
		else
		{
			if (BPQ32_EXE && AttachingProcess == 0)
			{
				AttachedProcesses++;			// We will get a detach
				FreeSemaphore();
				MessageBox(NULL,"BPQ32.exe is already running\r\n\r\nIt should only be run once", "BPQ32", MB_OK);
				return 0;
			}
		}

		if (_stricmp(pgm,"BPQTelnetServer.exe") == 0)
		{
			MessageBox(NULL,"BPQTelnetServer is no longer supported\r\n\r\nUse the TelnetServer in BPQ32.dll", "BPQ32", MB_OK);
			AttachedProcesses++;			// We will get a detach
			FreeSemaphore();
			return 0;
		}

		AuthorisedProgram = TRUE;

		if (InitDone == 0)
		{
			#pragma warning(push)
			#pragma warning(disable : 4996)

			if (_winver < 0x0600)
			#pragma warning(pop)
			{
				// Below Vista

				REGTREE = HKEY_LOCAL_MACHINE;
				strcpy(REGTREETEXT, "HKEY_LOCAL_MACHINE");
			}

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

			if (!BPQ32_EXE)
			{
				if (CheckifBPQ32isLoaded() == FALSE)		// Start BPQ32.exe if needed
				{
					// Wasn't Loaded, so we have started it, and should let it init system

					goto SkipInit;		
				}
			}

			GetVersionInfo("bpq32.dll");

			wsprintf (SIGNONMSG, "G8BPQ AX25 Packet Switch System Version %s %s\r\n%s\r\n",
				TextVerstring, Datestring, VerCopyright);
				 
			SESSHDDRLEN = wsprintf(SESSIONHDDR, "G8BPQ Network System %s for Win32 (", TextVerstring);

			SetupConsoleWindow();
			SetupBPQDirectory();
		
			if (!ProcessConfig())
			{
				StartMinimized = FALSE;
				MinimizetoTray = FALSE;
				ShowWindow(FrameWnd, SW_RESTORE);
				ShowWindow(hConsWnd, SW_RESTORE);
				SendMessage(hConsWnd, WM_PAINT, 0, 0);

				InitDone = -1;
				FreeSemaphore();

				MessageBox(NULL,"Configuration File Error","BPQ32",MB_ICONSTOP);

				return (0);
			}

			Consoleprintf("G8BPQ AX25 Packet Switch System Version %s %s", TextVerstring, Datestring);
			Consoleprintf(VerCopyright);

			if (START() !=0)
			{
				Sleep(3000);
				FreeSemaphore();
				return (0);
			}
			else
			{
				SetApplPorts();
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

/*
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
*/			
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
SkipInit:

		_beginthread(MonitorTimerThread,0,0);

		FreeSemaphore();

		AttachedPIDList[AttachedProcesses++] = GetCurrentProcessId();

		if (_stricmp(pgm,"bpq32.exe") == 0 &&  AttachingProcess == 1) AttachingProcess = 0;

		GetProcess(GetCurrentProcessId(),pgm);
		n=wsprintf(buf,"BPQ32 DLL Attach complete - Program %s - %d Process(es) Attached\n",pgm,AttachedProcesses);
		OutputDebugString(buf);

		// Set up local variables
		
		MCOM=1;
		MTX=1;
		MMASK=0xffffffff;

//		if (StartMinimized)
//			if (MinimizetoTray)
//				ShowWindow(FrameWnd, SW_HIDE);
//			else
//				ShowWindow(FrameWnd, SW_SHOWMINIMIZED);	
//		else
//			ShowWindow(FrameWnd, SW_RESTORE);

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
			{
				// If connected, disconnect

				SessionControl(i, 2, 0);
				DeallocateStream(i);
			}
		}

		// Remove any Tray Icon Entries

		for( i = 0; i < 100; ++i )
		{
			if (PIDArray[i] == ProcessID)
			{
				char Log[80];
				hWndArray[i] = 0;
				wsprintf(Log,"BPQ32 Removing Tray Item %s\n", PopupText[i]);
				OutputDebugString(Log);
				DeleteMenu(trayMenu,TRAYBASEID+i,MF_BYCOMMAND);
			}
		}

		if (Mutex) CloseHandle(Mutex);

		//	Remove our entry from PID List

		for (i=0;  i< AttachedProcesses; i++)
			if (AttachedPIDList[i] == ProcessID)
				break;

		for (; i< AttachedProcesses; i++)
		{
			AttachedPIDList[i]=AttachedPIDList[i+1];
		}

		AttachedProcesses--;

		if (TimerInst == ProcessID)
		{
			PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
	
			OutputDebugString("BPQ32 Process with Timer closing\n");

			// Call Port Close Routines
			
			for (i=0;i<NUMBEROFPORTS;i++)
			{
				if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
				{
					if (PORTVEC->PORT_EXT_ADDR && PORTVEC->DLLhandle == NULL) // Don't call if real .dll - it's not there!
					{
						SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
						PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);	// Close External Ports
					}	
				}

				PORTVEC->PORTCONTROL.PORTCLOSECODE(&PORTVEC->PORTCONTROL);

				PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
			}


			IPClose();
			APRSClose();
			Rig_Close();
			WSACleanup();
			WSAGetLastError();

			if (MinimizetoTray)
				Shell_NotifyIcon(NIM_DELETE,&niData);

			if (hConsWnd) DestroyWindow(hConsWnd);

			KillTimer(NULL,TimerHandle);
			TimerHandle=0;
			TimerInst=0xffffffff;

			if (AttachedProcesses && Closing == FALSE && AttachingProcess == 0)		// Other processes 
			{
				OutputDebugString("BPQ32 Reloading BPQ32.exe\n");
				StartBPQ32();
			}
		}
		else
		{
			// Not Timer Process

			if (AttachedProcesses == 1 && CloseLast)		// Only bpq32.exe left
			{
				Debugprintf("Only BPQ32.exe running - close it");
				CloseAllNeeded = TRUE;
			}
		}

		if (AUTOSAVE == 1 && AttachedProcesses < 2) SaveNodes();	// Soundmo	

		if (AttachedProcesses == 0)
		{
			KillTimer(NULL,TimerHandle);
						
			if (MinimizetoTray)
				Shell_NotifyIcon(NIM_DELETE,&niData);

			// Unload External Drivers

			{
				PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
				
				for (i=0;i<NUMBEROFPORTS;i++)
				{
					if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10 && PORTVEC->DLLhandle)
						FreeLibrary(PORTVEC->DLLhandle);

					PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;
				}
			}
		}

		GetProcess(GetCurrentProcessId(),pgm);
		n=wsprintf(buf,"BPQ32 DLL Detach complete - Program %s - %d Process(es) Attached\n",pgm,AttachedProcesses);
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
	int ProcessID = GetCurrentProcessId();

	if (Semaphore == 1 && ProcessID == SemProcessID)
	{
		OutputDebugString("BPQ32 Process holding Semaphore called CloseBPQ32 - attempting recovery\r\n");
		Debugprintf("Last Sem Call %d %x %x %x %x %x %x", SemHeldByAPI,
			Sem_eax, Sem_ebx, Sem_ecx, Sem_edx, Sem_esi, Sem_edi); 

		Semaphore = 0;
		SemHeldByAPI = 0;
	}

	if (TimerInst == ProcessID)
	{	
		OutputDebugString("BPQ32 Process with Timer called CloseBPQ32\n");

		if (MinimizetoTray)
			Shell_NotifyIcon(NIM_DELETE,&niData);

		for (i=0;i<NUMBEROFPORTS;i++)
		{
			if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
			{
				if (PORTVEC->PORT_EXT_ADDR)
				{
					PORTVEC->PORT_EXT_ADDR(5,PORTVEC->PORTCONTROL.PORTNUMBER, NULL);
				}
			}
			PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
		}

		KillTimer(NULL,TimerHandle);
		TimerHandle=0;
		TimerInst=0xffffffff;

		IPClose();
		APRSClose();
		Rig_Close();
		WSACleanup();

		if (hConsWnd) DestroyWindow(hConsWnd);

		Debugprintf("AttachedProcesses %d ", AttachedProcesses);

		if (AttachedProcesses > 1 && Closing == FALSE && AttachingProcess == 0)		// Other processes 
		{
			OutputDebugString("BPQ32 Reloading BPQ32.exe\n");
			StartBPQ32();
		}
	}

	return 0;
}

BOOL CopyReg(HKEY hKeyIn, HKEY hKeyOut);

VOID SetupBPQDirectory()
{
	HKEY hKey = 0;
	HKEY hKeyIn = 0;
	HKEY hKeyOut = 0;
	int disp;
	int retCode,Type,Vallen=MAX_PATH,i;
	char msg[512];
	char ValfromReg[MAX_PATH] = "";
	char DLLName[256]="Not Known";

/*
?NT4 was/is '4' 
?Win 95 is 4.00.950 
?Win 98 is 4.10.1998 
?Win 98 SE is 4.10.2222 
?Win ME is 4.90.3000 
?2000 is NT 5.0.2195 
?XP is actually 5.1 
?Vista is 6.0 
?Win7 is 6.1 

	i = _osver;		/ Build
	i = _winmajor;
	i = _winminor;
*/
#pragma warning(push)
#pragma warning(disable : 4996)

	if (_winver < 0x0600)
#pragma warning(pop)
	{
		// Below Vista

		REGTREE = HKEY_LOCAL_MACHINE;
		strcpy(REGTREETEXT, "HKEY_LOCAL_MACHINE");
		ValfromReg[0] = 0;
	}
	else
	{
		if (_stricmp(pgm, "regsvr32.exe") == 0)
		{
			Debugprintf("BPQ32 loaded by regsvr32.exe - Registry not copied");
		}
		else
		{
			// If necessary, move reg from HKEY_LOCAL_MACHINE to HKEY_CURRENT_USER

			retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
				                  "SOFTWARE\\G8BPQ\\BPQ32",
					              0,
						          KEY_READ,
							      &hKeyIn);

			retCode = RegCreateKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\G8BPQ\\BPQ32", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKeyOut, &disp);

			// See if Version Key exists in HKEY_CURRENT_USER - if it does, we have already done the copy

			Vallen = MAX_PATH;
			retCode = RegQueryValueEx(hKeyOut, "Version" ,0 , &Type,(UCHAR *)&msg, &Vallen);

			if (retCode != ERROR_SUCCESS)
				if (hKeyIn)
					CopyReg(hKeyIn, hKeyOut);

			RegCloseKey(hKeyIn);
			RegCloseKey(hKeyOut);
		}
	}

	GetModuleFileName(hInstance,DLLName,256);

	BPQDirectory[0]=0;

	retCode = RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

    if (retCode == ERROR_SUCCESS)
	{
		// Try "BPQ Directory"
		
		Vallen = MAX_PATH;
		retCode = RegQueryValueEx(hKey,"BPQ Directory",0,			
			&Type,(UCHAR *)&ValfromReg,&Vallen);

		if (retCode == ERROR_SUCCESS)
		{
			if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
				ValfromReg[0]=0;
		}

		if (ValfromReg[0] == 0)
		{
			// BPQ Directory absent or = "" - try "Config File Location"

			Vallen = MAX_PATH;
			
			retCode = RegQueryValueEx(hKey,"Config File Location",0,			
				&Type,(UCHAR *)&ValfromReg,&Vallen);

			if (retCode == ERROR_SUCCESS)
			{
				if (strlen(ValfromReg) == 2 && ValfromReg[0] == '"' && ValfromReg[1] == '"')
					ValfromReg[0]=0;
			}
		}

 		if (ValfromReg[0] == 0) GetCurrentDirectory(MAX_PATH, ValfromReg);

		// Get StartMinimized and MinimizetoTray flags

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Start Minimized", 0, &Type, (UCHAR *)&StartMinimized, &Vallen);

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Minimize to Tray", 0, &Type, (UCHAR *)&MinimizetoTray, &Vallen);

		ExpandEnvironmentStrings(ValfromReg, BPQDirectory, MAX_PATH);

		// Also get "BPQ Program Directory"

		ValfromReg[0] = 0;
		Vallen = MAX_PATH;

		retCode = RegQueryValueEx(hKey, "BPQ Program Directory",0 , &Type, (UCHAR *)&ValfromReg, &Vallen);

		RegCloseKey(hKey);
	}

	ExpandEnvironmentStrings(ValfromReg, BPQProgramDirectory, MAX_PATH);

	if (BPQProgramDirectory[0] == 0)
		strcpy(BPQProgramDirectory, BPQDirectory);

	i=sprintf(msg,"BPQ32 Ver %s Loaded from: %s by %s\n", VersionString, DLLName, pgm);
	WritetoConsole(msg);
	OutputDebugString(msg);

#pragma warning(push)
#pragma warning(disable : 4996)

	i=sprintf(msg,"Windows Ver %d.%d, Using Registry Key %s\n" ,_winmajor,  _winminor, REGTREETEXT);

#pragma warning(pop)

 	WritetoConsole(msg);
	OutputDebugString(msg);
	
	i=sprintf(msg,"BPQ32 Using config from: %s\n\n",BPQDirectory);
 	WritetoConsole(&msg[6]);
	msg[i-1]=0;
	OutputDebugString(msg);

	// Don't write the Version Key if loaded by regsvr32.exe (Installer is running with Admin rights,
	//	so will write the wrong tree on )

	if (_stricmp(pgm, "regsvr32.exe") == 0)
	{
		Debugprintf("BPQ32 loaded by regsvr32.exe - Version String not written");
	}
	else
	{
		retCode = RegCreateKeyEx(REGTREE, "SOFTWARE\\G8BPQ\\BPQ32", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

		wsprintf(msg,"%d,%d,%d,%d", Ver[0], Ver[1], Ver[2], Ver[3]);
		retCode = RegSetValueEx(hKey, "Version",0, REG_SZ,(BYTE *)msg, strlen(msg) + 1);

		RegCloseKey(hKey);
	}
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

char * FormatUptime(int Uptime)
 {
	struct tm * TM;
	static char UPTime[50];
	time_t szClock = Uptime * 60;

	TM = gmtime(&szClock);

	wsprintf(UPTime, "Uptime (Days Hours Mins)     %.2d:%.2d:%.2d\r",
		TM->tm_yday, TM->tm_hour, TM->tm_min);

	return UPTime;
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


DllExport struct PORTCONTROL * APIENTRY GetPortTableEntry(int portslot)		// Kept for Legacy apps
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

// Proc below renamed to avoid confusion with GetPortTableEntryFromPortNum

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromSlot(int portslot)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	if (portslot>NUMBEROFPORTS)
		portslot=NUMBEROFPORTS;

	while (--portslot > 0)
		PORTVEC=PORTVEC->PORTPOINTER;

	return PORTVEC;
}

DllExport struct PORTCONTROL * APIENTRY GetPortTableEntryFromPortNum(int portnum)
{
	struct PORTCONTROL * PORTVEC=PORTTABLE;

	do
	{
		if (PORTVEC->PORTNUMBER == portnum)
			return PORTVEC;

		PORTVEC=PORTVEC->PORTPOINTER;
	}
	while (PORTVEC);

	return NULL;
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
	return (&SIGNONMSG[0]);
}

DllExport char * APIENTRY GetVersionString()
{
//	return ((char *)&VersionStringWithBuild);
	return ((char *)&VersionString);
}

DllExport HKEY APIENTRY GetRegistryKey()
{
	return REGTREE;
}

DllExport char * APIENTRY GetRegistryKeyText()
{
	return REGTREETEXT;;
}

DllExport UCHAR * APIENTRY GetBPQDirectory()
{
	while (BPQDirectory[0] == 0)
	{
		Debugprintf("BPQ Directory not set up - waiting");
		Sleep(1000);
	}
	return (&BPQDirectory[0]);
}

DllExport UCHAR * APIENTRY GetProgramDirectory()
{
	return (&BPQProgramDirectory[0]);
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
	struct TRANSPORTENTRY *	HSESSION;

	// Return the Secure Session Flag rather than not connected

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

	}// End of ASM

	HSESSION = BPQHOSTVECTOR[stream-1].HOSTSESSION;
	if (HSESSION)
	{
		HSESSION = HSESSION->L4CROSSLINK;
		if (HSESSION) 
			return HSESSION->Secure_Session;
	}
	return 0;
}

DllExport int APIENTRY SessionControl(int stream, int command, int param)
{
//	Send Session Control command (BPQHOST function 6)
//;	CL=0 CONNECT USING APPL MASK IN DL
//;	CL=1, CONNECT. CL=2 - DISCONNECT. CL=3 RETURN TO NODE

	struct _EXCEPTION_POINTERS exinfo;
	UINT SaveSP;

	_asm {mov SaveSP,ESP}
	
	__try 
	{
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
	}
	__except(memcpy(&exinfo, GetExceptionInformation(), sizeof(struct _EXCEPTION_POINTERS)), EXCEPTION_EXECUTE_HANDLER)
	{
		unsigned int SPPtr;
		unsigned int SPVal;
		unsigned int eip;
		unsigned int rev;

		int i;

		DWORD Stack[16];
		DWORD CodeDump[16];

		eip = exinfo.ContextRecord->Eip;	
		SPPtr = exinfo.ContextRecord->Esp;	

		__asm{

		mov eax, SPPtr
		mov SPVal,eax
		lea edi,Stack
		mov esi,eax
		mov ecx,64
		rep movsb

		lea edi,CodeDump
		mov esi,eip
		mov ecx,64
		rep movsb

		}

		Debugprintf("BPQ32 *** Program Error %x at %x in Session Control",
			exinfo.ExceptionRecord->ExceptionCode, exinfo.ExceptionRecord->ExceptionAddress);

		Debugprintf("EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x ESP %x",
			exinfo.ContextRecord->Eax, exinfo.ContextRecord->Ebx, exinfo.ContextRecord->Ecx,
			exinfo.ContextRecord->Edx, exinfo.ContextRecord->Esi, exinfo.ContextRecord->Edi, SPVal);
		
		Debugprintf("Stack:");

		Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
			SPVal, Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7]);

		Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
			SPVal+32, Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);

		Debugprintf("Code:");

		for (i = 0; i < 16; i++)
		{
			rev = (CodeDump[i] & 0xff) << 24;
			rev |= (CodeDump[i] & 0xff00) << 8;
			rev |= (CodeDump[i] & 0xff0000) >> 8;
			rev |= (CodeDump[i] & 0xff000000) >> 24;

			CodeDump[i] = rev;
		}

		Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
			eip, CodeDump[0], CodeDump[1], CodeDump[2], CodeDump[3], CodeDump[4], CodeDump[5], CodeDump[6], CodeDump[7]);

		Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
			eip+32, CodeDump[8], CodeDump[9], CodeDump[10], CodeDump[11], CodeDump[12], CodeDump[13], CodeDump[14], CodeDump[15]);

		if (Semaphore && SemProcessID == GetCurrentProcessId())
			FreeSemaphore();

	}

	_asm {mov ESP, SaveSP}

	return 0;
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


	struct BPQVECSTRUC * PORTVEC;
	stream--;

	if (stream < 0 || stream > 63)
		return (0);
	
	PORTVEC=&BPQHOSTVECTOR[stream];

	PORTVEC->HOSTAPPLFLAGS = flags;
	PORTVEC->HOSTAPPLMASK = mask;
	
	// If either is non-zero, set allocated and Process. This gets round problem with
	// stations that don't call allocate stream
	
	if (flags || mask)
	{
		PORTVEC->STREAMOWNER=GetCurrentProcessId();
		PORTVEC->HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
		memcpy(&PORTVEC->PgmName[0], pgm, 31);
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
	movzx	edx,MUIONLY		// So it won't be changed accidentally

	call	BPQMONOPTIONS

	popad
	popfd

	}				// End of ASM
	return (0);
}

DllExport int APIENTRY SetTraceOptionsEx(int mask, int mtxparam, int mcomparam, int monUIOnly)
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
	mov	edx,monUIOnly

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

	// if init has not yet been run, wait.

	while (InitDone == 0)
	{
		Debugprintf("Waiting for init to complete");
		Sleep(1000);
	}

	if (InitDone == -1)			// Init failed
		exit(0);

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

OK_TO_ALLOC:

	mov	retcode,eax
}
	if (retcode != 255)
	{
		BPQHOSTVECTOR[retcode-1].STREAMOWNER=GetCurrentProcessId();
		BPQHOSTVECTOR[retcode-1].HOSTFLAGS = 128; // SET ALLOCATED BIT, clear others
		memcpy(&BPQHOSTVECTOR[retcode-1].PgmName[0], pgm, 31);
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

	memcpy(&BPQHOSTVECTOR[stream - 1].PgmName[0], pgm, 31);

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
	UINT * monbuff;

//	Release stream.

	stream--;

	if (stream < 0 || stream > 63)
		return (0);
	
	PORTVEC=&BPQHOSTVECTOR[stream];
	
	PORTVEC->STREAMOWNER=0;
	PORTVEC->PgmName[0] = 0;
	PORTVEC->HOSTAPPLFLAGS=0;
	PORTVEC->HOSTAPPLMASK=0;
	PORTVEC->HOSTHANDLE=0;

	// Clear Trace Queue

	if (PORTVEC->HOSTSESSION)
		SessionControl(stream + 1, 2, 0);


	while (PORTVEC->HOSTTRACEQ)
	{
		monbuff = Q_REM((UINT *)&PORTVEC->HOSTTRACEQ);
		ReleaseBuffer(monbuff);
	}

	PORTVEC->HOSTFLAGS &= 0x60;			// Clear Allocated. Must leave any DISC Pending bits

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

DllExport int APIENTRY ChangeSessionPaclen(int Stream, int Paclen)
{ 
	BPQHOSTVECTOR[Stream-1].HOSTSESSION->SESSPACLEN = Paclen;
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

	return ((PORTVEC->HOSTFLAGS & 0x80));
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
	
	if (strstr(Value, "V4"))
		return (UINT) V4ExtInit;
	
	if (strstr(Value, "TELNET"))
		return (UINT) TelnetExtInit;

	if (strstr(Value, "SOUNDMODEM"))
		return (UINT) SoundModemExtInit;

	if (strstr(Value, "SCSTRACKER"))
		return (UINT) TrackerExtInit;

	if (strstr(Value, "TRKMULTI"))
		return (UINT) TrackerMExtInit;

	if (strstr(Value, "UZ7HO"))
		return (UINT) UZ7HOExtInit;

	if (strstr(Value, "MULTIPSK"))
		return (UINT) MPSKExtInit;

	if (strstr(Value, "BAYCOM"))
		return (UINT) BaycomExtInit;

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


char AppName[] = "BPQ32";
char Title[80] = "BPQ32.dll Console";

int NewLine();

char FrameClassName[]	= TEXT("MdiFrame");

HWND ClientWnd; //This stores the MDI client area window handle

LOGFONT LFTTYFONT ;

HFONT hFont ;

HMENU hPopMenu, hWndMenu;
HMENU hMainFrameMenu = NULL;
HMENU hBaseMenu = NULL;
HMENU hConsMenu = NULL;
HMENU hTermMenu = NULL;
HMENU hMonMenu = NULL;
HMENU hTermActMenu, hTermCfgMenu, hTermEdtMenu, hTermHlpMenu;
HMENU hMonActMenu, hMonCfgMenu, hMonEdtMenu, hMonHlpMenu;


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

DllExport int APIENTRY DeleteTrayMenuItem(HWND hWnd);

#define BPQMonitorAvail 1
#define BPQDataAvail 2
#define BPQStateChange 4

BOOL GetWL2KSYSOPInfo(char * Call, char * SQL, char * ReplyBuffer);
BOOL UpdateWL2KSYSOPInfo(char * Call, char * SQL);

static INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		char ReplyBuffer[1000] = "";
		char * ptr1, * ptr2;
		char SQL[1000];

		wsprintf(SQL, "SELECT SysopName, StreetAddress1, StreetAddress2, City, State, Country, PostalCode, EMail, WEBSite, Phones, AdditionalData FROM SysopRecords WHERE Callsign='%s'",
			WL2KCall);

		if (GetWL2KSYSOPInfo(WL2KCall, SQL, ReplyBuffer))
		{
			int replylen = atoi(&ReplyBuffer[2]);

			if (replylen == 0)
				return (INT_PTR)TRUE;

			ptr1 = ptr2 = &ReplyBuffer[9];
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, NAME, ptr1);

			// Street 1

			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;
			SetDlgItemText(hDlg, ADDR1, ptr1);

			// Street 2

			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;
			SetDlgItemText(hDlg, ADDR2, ptr1);

			// City

			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, CITY, ptr1);

			// State

			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, STATE, ptr1);
			
			// State

			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, COUNTRY, ptr1);

			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, POSTCODE, ptr1);
			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, EMAIL, ptr1);
			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, WEBSITE, ptr1);
			ptr1 = ptr2;
			ptr2 = strchr(ptr1, 1);

			if (ptr2 == 0)
				return (INT_PTR)TRUE;

			*(ptr2++) = 0;

			SetDlgItemText(hDlg, PHONE, ptr1);

			SetDlgItemText(hDlg, ADDITIONALDATA, ptr2);

		}
	
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:

		switch(LOWORD(wParam))
		{

		case ID_SAVE:
		{
			char SQL[1000];

			char Name[100];
			char Addr1[100];
			char Addr2[100];
			char City[100];
			char State[100];
			char Country[100];
			char PostCode[100];
			char Email[100];
			char Website[100];
			char Phone[100];
			char Data[100];

			GetDlgItemText(hDlg, NAME, Name, 99);
			GetDlgItemText(hDlg, ADDR1, Addr1, 99);
			GetDlgItemText(hDlg, ADDR2, Addr2, 99);
			GetDlgItemText(hDlg, CITY, City, 99);
			GetDlgItemText(hDlg, STATE, State, 99);
			GetDlgItemText(hDlg, COUNTRY, Country, 99);
			GetDlgItemText(hDlg, POSTCODE, PostCode, 99);
			GetDlgItemText(hDlg, EMAIL, Email, 99);
			GetDlgItemText(hDlg, WEBSITE, Website, 99);
			GetDlgItemText(hDlg, PHONE, Phone, 99);
			GetDlgItemText(hDlg, ADDITIONALDATA, Data, 99);

			wsprintf(SQL, "REPLACE INTO SysopRecords SET Callsign='%s', GridSquare='%s', SysopName='%s', StreetAddress1='%s', StreetAddress2='%s', City='%s', State='%s', Country='%s', PostalCode='%s', EMail='%s', WEBSite='%s', Phones='%s', AdditionalData='%s'",
				WL2KCall, WL2KLoc, Name, Addr1, Addr2, City, State, Country, PostCode, Email, Website, Phone, Data);

			UpdateWL2KSYSOPInfo(WL2KCall, SQL);

		}

		case ID_CANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
		}
	}
	return (INT_PTR)FALSE;
}



LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT pos;
	BOOL ret;

	CLIENTCREATESTRUCT MDIClientCreateStruct; // Structure to be used for MDI client area
	//HWND m_hwndSystemInformation = 0;

	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(wParam);
				
		if (lParam & BPQMonitorAvail)
			DoMonData(wParam);
				
		if (lParam & BPQStateChange)
			DoStateChange(wParam);

		return (0);
	}

	switch (message)
	{ 
		case MY_TRAY_ICON_MESSAGE:
			
			switch(lParam)
			{
			case WM_RBUTTONUP:	
			case WM_LBUTTONUP:

				GetCursorPos(&pos);

	//			SetForegroundWindow(FrameWnd);

				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, FrameWnd, 0);
				return 0;
			}

			break;

		case WM_CTLCOLORDLG:
			return (LONG)bgBrush;
			
		case WM_SIZING:
		case WM_SIZE:

			SendMessage(ClientWnd, WM_MDIICONARRANGE, 0 ,0);
			break;

		case WM_NCCREATE:

			ret = DefFrameProc(hWnd, ClientWnd, message, wParam, lParam);
			return TRUE;

		case WM_CREATE:

		// On creation of main frame, create the MDI client area

		MDIClientCreateStruct.hWindowMenu	= NULL;
		MDIClientCreateStruct.idFirstChild	= IDM_FIRSTCHILD;
		
		ClientWnd = CreateWindow(TEXT("MDICLIENT"), // predefined value for MDI client area
									NULL, // no caption required
									WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE,
									0, // No need to give any x/y or height/width since this client
									   // will just be used to get client windows created, effectively
									   // in the main window we will be seeing the mainframe window client area itself.
									0, 
									0,
									0,
									hWnd,
									NULL,
									hInstance,
									(void *) &MDIClientCreateStruct);


		return 0;

		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			if (wmId >= TRAYBASEID && wmId < (TRAYBASEID + 100))
			{ 
				handle=hWndArray[wmId-TRAYBASEID];

				if (handle == FrameWnd)
					ShowWindow(handle, SW_NORMAL);

				if (handle == FrameWnd && FrameMaximized == TRUE)
					PostMessage(handle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
				else
					PostMessage(handle, WM_SYSCOMMAND, SC_RESTORE, 0);
				
				SetForegroundWindow(handle);
				return 0;
			}

			switch(wmId)
			{
			struct ConsoleInfo * Cinfo = NULL;

			case ID_NEWWINDOW:
				Cinfo = CreateChildWindow(0, FALSE);
				if (Cinfo)
					SendMessage(ClientWnd, WM_MDIACTIVATE, (WPARAM)Cinfo->hConsole, 0);
				break;

			case ID_WINDOWS_CASCADE:
				SendMessage(ClientWnd, WM_MDICASCADE, 0, 0);
				return 0;
					
			case ID_WINDOWS_TILE:
				SendMessage(ClientWnd, WM_MDITILE , MDITILE_HORIZONTAL, 0);
				return 0;

			case BPQCLOSEALL:
				CloseAllPrograms();
	//			SendMessage(ClientWnd, WM_MDIICONARRANGE, 0 ,0);

				return 0;

			case IDD_WL2KSYSOP:

				if (WL2KCall[0] == 0)
				{
					MessageBox(NULL,"WL2K Reporting is not configured","BPQ32", MB_OK);
					break;
				}
					
				DialogBox(hInstance, MAKEINTRESOURCE(IDD_WL2KSYSOP), hWnd, ConfigWndProc);
				break;

		
			 // Handle MDI Window commands
            
			default:
			{
				if(wmId >= IDM_FIRSTCHILD)
				{
					DefFrameProc(hWnd, ClientWnd, message, wParam, lParam);
				}
				else 
				{
					HWND hChild = (HWND)SendMessage(ClientWnd, WM_MDIGETACTIVE,0,0);

					if(hChild)
						SendMessage(hChild, WM_COMMAND, wParam, lParam);
				}
			}
			}
  
			break;

		case WM_INITMENUPOPUP:
		{
			HWND hChild = (HWND)SendMessage(ClientWnd, WM_MDIGETACTIVE,0,0);

			if(hChild)
				SendMessage(hChild, WM_INITMENUPOPUP, wParam, lParam);
		}

		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{ 
		case SC_MAXIMIZE: 

			FrameMaximized = TRUE;
			break;

		case SC_RESTORE: 

			FrameMaximized = FALSE;
			break;

		case SC_MINIMIZE: 

			if (MinimizetoTray)
			{
				ShowWindow(hWnd, SW_HIDE);
				return TRUE;
			}
		}

		return (DefFrameProc(hWnd, ClientWnd, message, wParam, lParam));

		case WM_CLOSE:
	
			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			break;

		default:
			return (DefFrameProc(hWnd, ClientWnd, message, wParam, lParam));

	}	
	return (DefFrameProc(hWnd, ClientWnd, message, wParam, lParam));
}

int OffsetH, OffsetW;

int SetupConsoleWindow()
{
    WNDCLASS  wc;
	int i;
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Size[80];
	WNDCLASSEX wndclassMainFrame;
	RECT CRect;

	retCode = RegOpenKeyEx (REGTREE,
                "SOFTWARE\\G8BPQ\\BPQ32",    
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"FrameWindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&FRect.left,&FRect.right,&FRect.top,&FRect.bottom);

		if (FRect.top < - 500 || FRect.left < - 500)
		{
			FRect.left = 0;
			FRect.top = 0;
			FRect.right = 600;
			FRect.bottom = 400;
		}


		Vallen=80;
		retCode = RegQueryValueEx(hKey,"WindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &ConsoleMinimized);

		if (Rect.top < - 500 || Rect.left < - 500)
		{
			Rect.left = 0;
			Rect.top = 0;
			Rect.right = 600;
			Rect.bottom = 400;
		}

		Vallen=80;

		retCode = RegQueryValueEx(hKey,"StatusWindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size, "%d,%d,%d,%d,%d", &StatusRect.left, &StatusRect.right,
				&StatusRect.top, &StatusRect.bottom, &StatusMinimized);

		if (StatusRect.top < - 500 || StatusRect.left < - 500)
		{
			StatusRect.left = 0;
			StatusRect.top = 0;
			StatusRect.right = 850;
			StatusRect.bottom = 500;
		}

		// Get StartMinimized and MinimizetoTray flags

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Start Minimized", 0, &Type, (UCHAR *)&StartMinimized, &Vallen);

		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "Minimize to Tray", 0, &Type, (UCHAR *)&MinimizetoTray, &Vallen);
	}

	wndclassMainFrame.cbSize		= sizeof(WNDCLASSEX);
	wndclassMainFrame.style			= CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wndclassMainFrame.lpfnWndProc	= FrameWndProc;
	wndclassMainFrame.cbClsExtra	= 0;
	wndclassMainFrame.cbWndExtra	= 0;
	wndclassMainFrame.hInstance		= hInstance;
    wndclassMainFrame.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON));
	wndclassMainFrame.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wndclassMainFrame.hbrBackground	= (HBRUSH) GetStockObject(GRAY_BRUSH);
	wndclassMainFrame.lpszMenuName	= NULL;
	wndclassMainFrame.lpszClassName	= FrameClassName;
	wndclassMainFrame.hIconSm		= NULL;
	
	if(!RegisterClassEx(&wndclassMainFrame))
	{
		return 0;
	}

	pindex = 0;
	PartLine = FALSE;

	bgBrush = CreateSolidBrush(BGCOLOUR);

//	hMainFrameMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME_MENU));

	hBaseMenu = LoadMenu(hInstance, MAKEINTRESOURCE(CONS_MENU));
	hConsMenu = GetSubMenu(hBaseMenu, 1);
	hWndMenu = GetSubMenu(hBaseMenu, 0);

	hTermMenu = LoadMenu(hInstance, MAKEINTRESOURCE(TERM_MENU));
	hTermActMenu = GetSubMenu(hTermMenu, 1);
	hTermCfgMenu = GetSubMenu(hTermMenu, 2);
	hTermEdtMenu = GetSubMenu(hTermMenu, 3);
	hTermHlpMenu = GetSubMenu(hTermMenu, 4);

	hMonMenu = LoadMenu(hInstance, MAKEINTRESOURCE(MON_MENU));
	hMonCfgMenu = GetSubMenu(hMonMenu, 1);
	hMonEdtMenu = GetSubMenu(hMonMenu, 2);
	hMonHlpMenu = GetSubMenu(hMonMenu, 3);

	hMainFrameMenu = CreateMenu();
	AppendMenu(hMainFrameMenu, MF_STRING + MF_POPUP, (UINT)hWndMenu, "Window");

	//Create the main MDI frame window

	ClientWnd = NULL;

	FrameWnd = CreateWindow(FrameClassName, 
								"BPQ32 Console", 
								WS_OVERLAPPEDWINDOW |WS_CLIPCHILDREN,
								FRect.left,	
								FRect.top,
								FRect.right - FRect.left,
								FRect.bottom - FRect.top,
								NULL,			// handle to parent window
								hMainFrameMenu, // handle to menu
								hInstance,	// handle to the instance of module
								NULL);		// Long pointer to a value to be passed to the window through the 
											// CREATESTRUCT structure passed in the lParam parameter the WM_CREATE message


	// Get Client Params

	if (FrameWnd == 0)
	{
		Debugprintf("SetupConsoleWindow Create Frame failed %d", GetLastError());
		return 0;
	}

	ShowWindow(FrameWnd, SW_RESTORE);


	GetWindowRect(FrameWnd, &FRect);
	OffsetH = FRect.bottom - FRect.top;
	OffsetW = FRect.right - FRect.left;
	GetClientRect(FrameWnd, &CRect);
	OffsetH -= CRect.bottom;
	OffsetW -= CRect.right;
	OffsetH -= 4;

	// Create Console Window
        
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));     
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);    
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	
	wc.lpszMenuName	 = 0;     
	wc.lpszClassName = ClassName; 

	i=RegisterClass(&wc);
	
	wsprintf (Title, "BPQ32.dll Console Version %s", VersionString);

	hConsWnd =  CreateMDIWindow(ClassName, "Console", 0,
		  0,0,0,0, ClientWnd, hInstance, 1234);

	i = GetLastError();

	if (!hConsWnd) {
		return (FALSE);
	}

	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wc.lpfnWndProc   = (WNDPROC)StatusWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = DLGWINDOWEXTRA;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));     
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);    
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	
	wc.lpszMenuName	 = 0;     
	wc.lpszClassName = "Status"; 

	i=RegisterClass(&wc);

	StatusWnd =  CreateMDIWindow("Status", "Stream Status", 0,
		  StatusRect.left,	StatusRect.top, StatusRect.right - StatusRect.left,
		  StatusRect.bottom - StatusRect.top, ClientWnd, hInstance, 1234);

	SetTimer(StatusWnd, 1, 1000, NULL);

	hPopMenu = GetSubMenu(hBaseMenu, 1) ;

	if (MinimizetoTray)
		CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_CHECKED);
	else
		CheckMenuItem(hPopMenu, BPQMINTOTRAY, MF_UNCHECKED);
				
	if (StartMinimized)
		CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_CHECKED);
	else
		CheckMenuItem(hPopMenu, BPQSTARTMIN, MF_UNCHECKED);
	
	DrawMenuBar(hConsWnd);	

	// setup default font information

   LFTTYFONT.lfHeight =			12;
   LFTTYFONT.lfWidth =          8 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        0;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = FIXED_PITCH;
   lstrcpy(LFTTYFONT.lfFaceName, "FIXEDSYS" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
	
	SetWindowText(hConsWnd,Title);

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hConsWnd, &Rect);
	}

	MoveWindow(hConsWnd, Rect.left - (OffsetW /2), Rect.top - OffsetH, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	MoveWindow(StatusWnd, StatusRect.left - (OffsetW /2), StatusRect.top - OffsetH,
		StatusRect.right-StatusRect.left, StatusRect.bottom-StatusRect.top, TRUE);

	hWndCons = CreateWindowEx(WS_EX_CLIENTEDGE, "LISTBOX", "",
		WS_CHILD |  WS_VISIBLE  | LBS_NOINTEGRALHEIGHT | 
                    LBS_DISABLENOSCROLL | LBS_NOSEL | WS_VSCROLL | WS_HSCROLL,
		Rect.left,	Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top,
		hConsWnd, NULL, hInstance, NULL);

//	SendMessage(hWndCons, WM_SETFONT, hFont, 0);

	SendMessage(hWndCons, LB_SETHORIZONTALEXTENT , 1000, 0);

	if (ConsoleMinimized)
		ShowWindow(hConsWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hConsWnd, SW_RESTORE);

	if (StatusMinimized)
		ShowWindow(StatusWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(StatusWnd, SW_RESTORE);

	ShowWindow(FrameWnd, SW_RESTORE);


	LoadLibrary("riched20.dll");
	SessHandle = SetTimer(NULL, 0, 5000, lpSetupTermSessions);

	if (StartMinimized)
		if (MinimizetoTray)
			ShowWindow(FrameWnd, SW_HIDE);
		else
			ShowWindow(FrameWnd, SW_SHOWMINIMIZED);	
	else
		ShowWindow(FrameWnd, SW_RESTORE);

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
			hWndArray[i] = FrameWnd;
			goto doneit;
		}
	}

	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] == 0)
		{
			hWndArray[i] = FrameWnd;
			strcpy(PopupText[i],"BPQ32 Console");
			break;
		}
	}
doneit:

	for( i = 0; i < 100; ++i )
	{
		if (hWndArray[i] != 0)
			AppendMenu(trayMenu,MF_STRING,TRAYBASEID+i,PopupText[i]);
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
			MAKEINTRESOURCE(BPQICON),
			IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_DEFAULTCOLOR);


	// set the window you want to receive event messages

	niData.hWnd = FrameWnd;

	// set the message to send
	// note: the message value should be in the
	// range of WM_APP through 0xBFFF

	niData.uCallbackMessage = MY_TRAY_ICON_MESSAGE;

	//	Call Shell_NotifyIcon. NIM_ADD adds a new tray icon

	if (Shell_NotifyIcon(NIM_ADD,&niData))
		Debugprintf("BPQ32 Create Tray Icon Ok");
//	else
//		Debugprintf("BPQ32 Create Tray Icon failed %d", GetLastError());

	return 0;
}

VOID SaveConfig()
{
	HKEY hKey=0;
	int retCode, disp;

	retCode = RegCreateKeyEx(REGTREE,
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
	RECT cRect;

	switch (message)
	{
	case WM_MDIACTIVATE:
				 
	// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			// GetSubMenu function should retrieve a handle to the drop-down menu or submenu.

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hConsMenu, "Actions");
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);
		}
		else
		{
			 // Deactivate
	
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		}
	 
		DrawMenuBar(FrameWnd);

		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);

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

			break;

		case WM_CTLCOLORDLG:
		   return (LONG)bgBrush;

		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			if (wmId == IDC_ENIGATE)
			{
				int retCode, disp;
				HKEY hKey=0;

				IGateEnabled = IsDlgButtonChecked(hWnd, IDC_ENIGATE); 

				if (IGateEnabled)
					ISDelayTimer = 60;

				retCode = RegCreateKeyEx(REGTREE,
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
					retCode = RegSetValueEx(hKey,"IGateEnabled", 0 , REG_DWORD,(BYTE *)&IGateEnabled, 4);
					RegCloseKey(hKey);
				}

				return 0;
			}		

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

			if (wmId == SCANRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}

				RigReconfigFlag=TRUE;	
				WritetoConsole("Rigcontrol Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}

			if (wmId == APRSRECONFIG)
			{
				if (!ProcessConfig())
				{
					MessageBox(NULL,"Configuration File check falled - will continue with old config","BPQ32",MB_OK);
					return (0);
				}

				APRSReconfigFlag=TRUE;	
				WritetoConsole("APRS Reconfig requested ... Waiting for Timer Poll\n");
				return 0;
			}
			if (wmId == BPQDUMP)
			{
				DumpSystem();
				return 0;
			}

			if (wmId == BPQCLOSEALL)
			{
				CloseAllPrograms();
				return 0;
			}

			if (wmId == BPQSAVEREG)
			{
				CreateRegBackup();
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

			if (wmId >= TRAYBASEID && wmId < (TRAYBASEID + 100))
			{ 
				handle=hWndArray[wmId-TRAYBASEID];

				if (handle == FrameWnd && FrameMaximized == TRUE)
					PostMessage(handle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
				else
					PostMessage(handle, WM_SYSCOMMAND, SC_RESTORE, 0);

				SetForegroundWindow(handle);
				return 0;
			}		
	
		case WM_SYSCOMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!
	
			switch (wmId)
			{ 
			case  SC_MINIMIZE: 

				ConsoleMinimized = TRUE;
				break;

			case  SC_RESTORE: 

				ConsoleMinimized = FALSE;
				SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);

				break;
			}

			return DefMDIChildProc(hWnd, message, wParam, lParam);
	

		case WM_SIZE:

		GetClientRect(hWnd, &cRect); 

		MoveWindow(hWndBG, 0, 0, cRect.right, 26, TRUE);

		if (APRSActive)
			MoveWindow(hWndCons, 2, 26, cRect.right-4, cRect.bottom - 32, TRUE);
		else
			MoveWindow(hWndCons, 2, 2, cRect.right-4, cRect.bottom - 4, TRUE);

//		InvalidateRect(hWnd, NULL, TRUE);
		break;

/*
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
*/

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
	
			return DefMDIChildProc(hWnd, message, wParam, lParam);


	}
			
	return DefMDIChildProc(hWnd, message, wParam, lParam);
}

DllExport BOOL APIENTRY GetMinimizetoTrayFlag()
{
	while (InitDone == 0)
	{
		Debugprintf("Waiting for init to complete");
		Sleep(1000);
	}

	if (InitDone == -1)			// Init failed
		exit(0);
	
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
			AppendMenu(trayMenu,MF_STRING,TRAYBASEID+i,Label);
			CreateNewTrayIcon();
			return 0;
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
			DeleteMenu(trayMenu,TRAYBASEID+i,MF_BYCOMMAND);
			CreateNewTrayIcon();
			return 0;
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
	int len=strlen(buff);
	char Temp[2000]= "";
	char * ptr;

	if (PartLine)
	{
		SendMessage(hWndCons, LB_GETTEXT, pindex, (LPARAM)(LPCTSTR) Temp);
		SendMessage(hWndCons, LB_DELETESTRING, pindex, 0);
		PartLine = FALSE;
	}
	
	strcat(Temp, buff);

	ptr = strchr(Temp, '\n');

	if (ptr)
		*ptr = 0;
	else
		PartLine = TRUE;

	pindex=SendMessage(hWndCons, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) Temp);
	return 0;
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

//	WriteFile(handle,Screen,MAXLINELEN*MAXSCREENLEN,&cnt,NULL);

	WriteFile(handle,&Flag,(&ENDOFDATA - &Flag) * 4,&cnt,NULL);

 	CloseHandle(handle);

	wsprintf(Msg, "Dump to %s Completed\n", fn);
	WritetoConsole(Msg);

	return (0);
}

BOOLEAN CheckifBPQ32isLoaded()
{
	HANDLE Mutex;
	
	// See if BPQ32 is running - if we create it in the NTVDM address space by
	// loading bpq32.dll it will not work.

	Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

	if (Mutex == NULL)
	{	
		if (AttachingProcess == 0)			// Already starting BPQ32
		{
			OutputDebugString("BPQ32 No other bpq32 programs running - Loading BPQ32.exe\n");
			StartBPQ32();
		}
		return FALSE;
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

	AttachingProcess = 1;

// Get address of BPQ Directory

	Value[0]=0;

	ret = RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (ret == ERROR_SUCCESS)
	{
		ret = RegQueryValueEx(hKey, "BPQ Program Directory", 0, &Type,(UCHAR *)&Value, &Vallen);
		
		if (ret == ERROR_SUCCESS)
		{
			if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
				Value[0]=0;
		}


		if (Value[0] == 0)
		{
		
			// BPQ Directory absent or = "" - "try Config File Location"
			
			ret = RegQueryValueEx(hKey,"BPQ Directory",0,			
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
		AttachingProcess = 0;
		return FALSE;		
	}

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
		pushad
		mov	edi,Msg
		call RELBUFF
		popad
	}
}

DllExport PMESSAGE APIENTRY GetBuff()
{
	UINT Temp;
	
	_asm{
		pushad
 		call GETBUFF
		mov Temp, edi
		popad
		mov eax, Temp
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
	return;
}
DllExport BOOL APIENTRY CheckOneTimePassword(char * Password, char * KeyPhrase)
{
	char CheckPassword[17];
	int Offsets[10] = {0, -1, 1, -2, 2, -3, 3, -4, 4};
	int i, Pass;

	if (strlen(Password) < 16)
		Pass = atoi(Password);

	for (i = 0; i < 9; i++)
	{
		CreateOneTimePassword(CheckPassword, KeyPhrase, Offsets[i]);

		if (strlen(Password) < 16)
		{
			// Using a numeric extract 

			long long Val;

			memcpy(&Val, CheckPassword, 8);
			Val = Val %= 1000000;

			if (Pass == Val)
				return TRUE;
		}
		else
			if (memcmp(Password, CheckPassword, 16) == 0)
				return TRUE;
	}

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

int C_Q_COUNT(UINT *Q)
{
	UINT * next;
	int Count = 0;
	
	if (Q[0] == 0)						// Empty
	{
		return(0);
	}

	(int)next=Q[0];

	while (next[0]!=0)
	{
		Count ++;
		next=(UINT *)next[0];
	}

	return Count;

}
unsigned short int compute_crc(unsigned char *buf, int txlen);

SOCKADDR_IN reportdest = {0};

SOCKET ReportSocket = 0;

SOCKADDR_IN Chatreportdest = {0};

DllExport VOID APIENTRY SendChatReport(SOCKET ChatReportSocket, char * buff, int txlen)
{
 	unsigned short int crc = compute_crc(buff, txlen);

	crc ^= 0xffff;

	buff[txlen++] = (crc&0xff);
	buff[txlen++] = (crc>>8);

	sendto(ChatReportSocket, buff, txlen, 0, (LPSOCKADDR)&Chatreportdest, sizeof(Chatreportdest));

}

VOID SendReportMsg(char * buff, int txlen)
{
 	unsigned short int crc = compute_crc(buff, txlen);

	crc ^= 0xffff;

	buff[txlen++] = (crc&0xff);
	buff[txlen++] = (crc>>8);

	sendto(ReportSocket, buff, txlen, 0, (LPSOCKADDR)&reportdest, sizeof(reportdest));

}
VOID SendLocation()
{
	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[512];
	int Len;

	Len = wsprintf(Msg, "%s %s<br>%s", LOCATOR, VersionString, MAPCOMMENT);

	if (Len > 256)
		Len = 256;

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, ReportDest, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->L2DATA, Msg, Len);

	SendReportMsg((char *)&AXMSG.DEST, Len + 16);

	return;

}

VOID SendMH(int Hardware, char * call, char * freq, char * LOC, char * Mode)
{
	MESSAGE AXMSG;
	PMESSAGE AXPTR = &AXMSG;
	char Msg[100];
	int Len;

	if (ReportSocket == 0 || LOCATOR[0] == 0)
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

	SendReportMsg((char *)&AXMSG.DEST, Len + 16) ;

	return;

}

VOID CreateRegBackup()
{
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];
	char RegFileName[MAX_PATH];
	char Msg[80];
	HANDLE handle;
	int len, written;
	char RegLine[300];

//	SHELLEXECUTEINFO   sei;
//	STARTUPINFO SInfo;
//	PROCESS_INFORMATION PInfo;

	wsprintf(RegFileName, "%s\\BPQ32.reg", BPQDirectory);

	// Keep 4 Generations

	strcpy(Backup2, RegFileName);
	strcat(Backup2, ".bak.3");

	strcpy(Backup1, RegFileName);
	strcat(Backup1, ".bak.2");

	DeleteFile(Backup2);			// Remove old .bak.3
	MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

	strcpy(Backup2, RegFileName);
	strcat(Backup2, ".bak.1");

	MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

	strcpy(Backup1, RegFileName);
	strcat(Backup1, ".bak");

	MoveFile(Backup1, Backup2);		//Move .bak to .bak.1

	strcpy(Backup2, RegFileName);
	strcat(Backup2, ".bak");

	CopyFile(RegFileName, Backup2, FALSE);	// Copy to .bak

	handle = CreateFile(RegFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		wsprintf(Msg, "Failed to open Registry Save File\n");
		WritetoConsole(Msg);
		return;
	}

	len = wsprintf(RegLine, "Windows Registry Editor Version 5.00\r\n\r\n");
	WriteFile(handle, RegLine, len, &written, NULL);

	if (SaveReg("Software\\G8BPQ\\BPQ32", handle))
		WritetoConsole("Registry Save complete\n");
	else
		WritetoConsole("Registry Save failed\n");

	CloseHandle(handle);			
	return ;
/*

	if (REGTREE == HKEY_LOCAL_MACHINE)		// < Vista
	{
		wsprintf(cmd,
			"regedit /E \"%s\\BPQ32.reg\" %s\\Software\\G8BPQ\\BPQ32", BPQDirectory, REGTREETEXT);

		ZeroMemory(&SInfo, sizeof(SInfo));

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
  		SInfo.lpReserved2=NULL; 

		if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0 ,NULL, NULL, &SInfo, &PInfo) == 0)
		{
			wsprintf(Msg, "Error: CreateProcess for regedit failed 0%d\n", GetLastError() );
		   	WritetoConsole(Msg);
			return;
		}
	}
	else
	{

		wsprintf(cmd,
			"/E \"%s\\BPQ32.reg\" %s\\Software\\G8BPQ\\BPQ32", BPQDirectory, REGTREETEXT);	

	    ZeroMemory(&sei, sizeof(sei));

		sei.cbSize          = sizeof(SHELLEXECUTEINFOW);
		sei.hwnd            = hWnd;
		sei.fMask           = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
		sei.lpVerb          = "runas";
		sei.lpFile          = "regedit.exe";
		sei.lpParameters    = cmd;
		sei.nShow           = SW_SHOWNORMAL;

		if (!ShellExecuteEx(&sei))
		 {
		    wsprintf(Msg, "Error: ShellExecuteEx for regedit failed %d\n", GetLastError() );
		   	WritetoConsole(Msg);
			return;
		}
	}

	wsprintf(Msg, "Registry Save Initiated\n", fn);
	WritetoConsole(Msg);
			
	return ;
*/
}



VOID OpenReportingSockets()
{
	u_long param=1;
	BOOL bcopt=TRUE;

	if (LOCATOR[0])
	{
		// Enable Node Map Reports

		ReportTimer = 600;

		ReportSocket = socket(AF_INET,SOCK_DGRAM,0);

		if (ReportSocket == INVALID_SOCKET)
		{
			Debugprintf("Failed to create Reporting socket");
			ReportSocket = 0;
  		 	return; 
		}

		ioctlsocket (ReportSocket, FIONBIO, &param);
		setsockopt (ReportSocket, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&bcopt,4);

		reportdest.sin_family = AF_INET;
		reportdest.sin_port = htons(81);
		ConvToAX25("DUMMY-1", ReportDest);
	}

	// Set up Chat Report even if no LOCATOR	reportdest.sin_family = AF_INET;
	// Socket must be opened in MailChat Process

	Chatreportdest.sin_family = AF_INET;
	Chatreportdest.sin_port = htons(10090);

	_beginthread(ResolveUpdateThread,0,(int)0);
}

VOID ResolveUpdateThread()
{
	struct hostent * HostEnt1;
	struct hostent * HostEnt2;

	while (TRUE)
	{
		//	Resolve name to address

		Debugprintf("Resolving %s", "update.g8bpq.net");
		HostEnt1 = gethostbyname ("update.g8bpq.net");
		 
		if (HostEnt1)
			memcpy(&reportdest.sin_addr.s_addr,HostEnt1->h_addr,4);

		Debugprintf("Resolving %s", "chatmap.g8bpq.net");
		HostEnt2 = gethostbyname ("chatmap.g8bpq.net");

		if (HostEnt2)
			memcpy(&Chatreportdest.sin_addr.s_addr,HostEnt2->h_addr,4);

		if (HostEnt1 && HostEnt2)
		{
			Sleep(1000 * 60 * 30);
			continue;
		}

		Debugprintf("Resolve Failed for update.g8bpq.net or chatmap.g8bpq.net");
		Sleep(1000 * 60 * 5);
	}
}

BOOL CALLBACK EnumForCloseProc(HWND hwnd, LPARAM  lParam)
{
	struct TNCINFO * TNC = (struct TNCINFO *)lParam; 
	UINT ProcessId;

	GetWindowThreadProcessId(hwnd, &ProcessId);

	for (i=0; i< AttachedProcesses; i++)
	{
		if (AttachedPIDList[i] == ProcessId)
		{
			Debugprintf("BPQ32 Close All Closing PID %d", ProcessId);
			PostMessage(hwnd, WM_CLOSE, 0, 0);
	//		AttachedPIDList[i] = 0;				// So we don't do it again
			break;
		}
	}
	
	return (TRUE);
}
DllExport BOOL APIENTRY RestoreFrameWindow()
{
	return 	ShowWindow(FrameWnd, SW_RESTORE);
}

DllExport VOID APIENTRY CreateNewTrayIcon()
{
	Shell_NotifyIcon(NIM_DELETE,&niData);
	trayMenu = NULL;
}

DllExport VOID APIENTRY CloseAllPrograms()
{
//	HANDLE hProc;

	// Close all attached BPQ32 programs

	ShowWindow(FrameWnd, SW_RESTORE);

	GetWindowRect(FrameWnd, &FRect);

	SaveBPQ32Windows();
	CloseHostSessions();

	if (AttachedProcesses == 1)
		CloseBPQ32();
		
	Closing  = TRUE;

	Debugprintf("BPQ32 Close All Processes %d PIDS %d %d %d %d", AttachedProcesses, AttachedPIDList[0],
		AttachedPIDList[1], AttachedPIDList[2], AttachedPIDList[3]);

	if (MinimizetoTray)
		Shell_NotifyIcon(NIM_DELETE,&niData);
	
	EnumWindows(EnumForCloseProc, (LPARAM)NULL);
}

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define MAX_VALUE_DATA 65536

BOOL CopyReg(HKEY hKeyIn, HKEY hKeyOut)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string 
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 
 
    DWORD i, retCode; 
 
    TCHAR  achValue[MAX_VALUE_NAME]; 
    DWORD cchValue = MAX_VALUE_NAME; 
 
    // Get the class name and the value count. 
    retCode = RegQueryInfoKey(
        hKeyIn,                    // key handle 
        achClass,                // buffer for class name 
        &cchClassName,           // size of class string 
        NULL,                    // reserved 
        &cSubKeys,               // number of subkeys 
        &cbMaxSubKey,            // longest subkey size 
        &cchMaxClass,            // longest class string 
        &cValues,                // number of values for this key 
        &cchMaxValue,            // longest value name 
        &cbMaxValueData,         // longest value data 
        &cbSecurityDescriptor,   // security descriptor 
        &ftLastWriteTime);       // last write time 
 
    // Enumerate the subkeys, until RegEnumKeyEx fails.
    
    if (cSubKeys)
    {
        Debugprintf( "\nNumber of subkeys: %d\n", cSubKeys);

        for (i=0; i<cSubKeys; i++) 
        { 
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKeyIn, i,
                     achKey, 
                     &cbName, 
                     NULL, 
                     NULL, 
                     NULL, 
                     &ftLastWriteTime); 

            if (retCode == ERROR_SUCCESS) 
            {
                HKEY NextKeyIn;
                HKEY NextKeyOut;
				int disp;

				Debugprintf(TEXT("(%d) %s\n"), i+1, achKey);
				retCode = RegOpenKeyEx(hKeyIn, achKey, 0, KEY_READ, &NextKeyIn);
				retCode += RegCreateKeyEx(hKeyOut, achKey, 0, 0, 0, KEY_ALL_ACCESS, NULL, &NextKeyOut, &disp);

				if (retCode == 0)
					CopyReg(NextKeyIn, NextKeyOut);
            }
        }
    } 
 
    // Enumerate the key values. 

    if (cValues) 
    {
        Debugprintf( "\nNumber of values: %d\n", cValues);

        for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++) 
        { 
			int Type;
			int ValLen = MAX_VALUE_DATA;
			UCHAR Value[MAX_VALUE_DATA] = "";

            cchValue = MAX_VALUE_NAME; 
            achValue[0] = '\0'; 
            retCode = RegEnumValue(hKeyIn, i, 
                achValue, 
                &cchValue, 
                NULL, 
                &Type,
                &Value[0],
                &ValLen);
 
            if (retCode == ERROR_SUCCESS ) 
            { 
				if (ValLen == 4)
					Debugprintf(TEXT("(%d) %s = %x len %d\n"), i+1, achValue, *Value, ValLen); 
				else
					Debugprintf(TEXT("(%d) %s = %s len %d\n"), i+1, achValue, Value, ValLen); 

				retCode = RegSetValueEx(hKeyOut, achValue, 0 , Type, (BYTE *)&Value[0], ValLen);
            } 
        }
    }
	return TRUE;
}


DllExport BOOL APIENTRY SaveReg(char * KeyIn, HANDLE hFile)
{
	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string 
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // size of class string 
    DWORD    cSubKeys=0;               // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 
 
    DWORD i, retCode; 
	HKEY hKeyIn;
    TCHAR  achValue[MAX_VALUE_NAME]; 
    DWORD cchValue = MAX_VALUE_NAME; 
	int len, written;
	char RegLine[300];
	char * ptr1, * ptr2;
	char c;

	retCode = RegOpenKeyEx (REGTREE, KeyIn, 0, KEY_READ, &hKeyIn);
	
	if (retCode != ERROR_SUCCESS)
	{
		Debugprintf("Open Reg Key %s failed", KeyIn);
		return FALSE;
	}

	len = wsprintf(RegLine, "[%s\\%s]\r\n", REGTREETEXT, KeyIn);

	WriteFile(hFile, RegLine, len, &written, NULL);
 
    // Get the class name and the value count. 
    retCode = RegQueryInfoKey(
        hKeyIn,                    // key handle 
        achClass,                // buffer for class name 
        &cchClassName,           // size of class string 
        NULL,                    // reserved 
        &cSubKeys,               // number of subkeys 
        &cbMaxSubKey,            // longest subkey size 
        &cchMaxClass,            // longest class string 
        &cValues,                // number of values for this key 
        &cchMaxValue,            // longest value name 
        &cbMaxValueData,         // longest value data 
        &cbSecurityDescriptor,   // security descriptor 
        &ftLastWriteTime);       // last write time 

	    // Enumerate the key values. 

    if (cValues) 
	{
        for (i=0, retCode=ERROR_SUCCESS; i<cValues; i++) 
		{
			int Type, k;
			int ValLen = MAX_VALUE_DATA;
			UCHAR Value[MAX_VALUE_DATA] = "";
			UCHAR ValCopy[MAX_VALUE_DATA];

			UINT Intval;

            cchValue = MAX_VALUE_NAME; 
            achValue[0] = '\0'; 
            retCode = RegEnumValue(hKeyIn, i, 
                achValue, 
                &cchValue, 
                NULL, 
                &Type,
                &Value[0],
                &ValLen);
 
            if (retCode == ERROR_SUCCESS ) 
            {
				// Encode the param depending on Type

				    switch(Type)
					{
					case REG_NONE:						//( 0 )   // No value type
						break;
					case REG_SZ:						//( 1 )   // Unicode nul terminated string
						
						// Need to escape any \ or " in Value
						
						ptr1 = Value;
						ptr2 = ValCopy;

						c = *ptr1++;

						while (c)
						{	
							switch (c)
							{
								case '\\':
								case '"':
									*ptr2++ = '\\';
							}
							*ptr2++ = c;
							c = *ptr1++;
						}
						*ptr2 = 0;
					
						len = wsprintf(RegLine, "\"%s\"=\"%s\"\r\n", achValue, ValCopy);
						break;

					case REG_EXPAND_SZ:					//( 2 )   // Unicode nul terminated string
														// (with environment variable references)
						break;

					case REG_BINARY:					//( 3 )   // Free form binary - hex:86,50 etc

						len = wsprintf(RegLine, "\"%s\"=hex:%02x,", achValue, Value[0]);
						for (k = 1; k < ValLen; k++)
						{
							if (len > 76)
							{
								len = wsprintf(RegLine, "%s\\\r\n", RegLine);
								WriteFile(hFile, RegLine, len, &written, NULL);
								strcpy(RegLine, "  ");
								len = 2;
							}

							len = wsprintf(RegLine, "%s%02x,", RegLine, Value[k]);
						}
						RegLine[--len] = 0x0d;
						RegLine[++len] = 0x0a;	
						len++;

						break;

					case REG_DWORD:						//( 4 )   // 32-bit number
//					case REG_DWORD_LITTLE_ENDIAN:		//( 4 )   // 32-bit number (same as REG_DWORD)
					
						memcpy(&Intval, Value, 4);
						len = wsprintf(RegLine, "\"%s\"=dword:%08x\r\n", achValue, Intval);
					break;
				
					case REG_DWORD_BIG_ENDIAN:			//( 5 )   // 32-bit number
						break;
					case REG_LINK:						//( 6 )   // Symbolic Link (unicode)
						break;
					case REG_MULTI_SZ:					//( 7 )   // Multiple Unicode strings

						len = wsprintf(RegLine, "\"%s\"=hex(7):%02x,00,", achValue, Value[0]);
						for (k = 1; k < ValLen; k++)
						{
							if (len > 76)
							{
								len = wsprintf(RegLine, "%s\\\r\n", RegLine);
								WriteFile(hFile, RegLine, len, &written, NULL);
								strcpy(RegLine, "  ");
								len = 2;
							}
							len = wsprintf(RegLine, "%s%02x,", RegLine, Value[k]);
							if (len > 76)
							{
								len = wsprintf(RegLine, "%s\\\r\n", RegLine);
								WriteFile(hFile, RegLine, len, &written, NULL);
								strcpy(RegLine, "  ");
							}
							len = wsprintf(RegLine, "%s00,", RegLine);
						}

						RegLine[--len] = 0x0d;
						RegLine[++len] = 0x0a;	
						len++;
						break;

					case REG_RESOURCE_LIST:				//( 8 )   // Resource list in the resource map
						break;
					case REG_FULL_RESOURCE_DESCRIPTOR:	//( 9 )  // Resource list in the hardware description
						break;
					case REG_RESOURCE_REQUIREMENTS_LIST://( 10 )
						break;
					case REG_QWORD:						//( 11 )  // 64-bit number
//					case REG_QWORD_LITTLE_ENDIAN:		//( 11 )  // 64-bit number (same as REG_QWORD)
						break;

					}
				
				WriteFile(hFile, RegLine, len, &written, NULL);
            } 
        }
	}	
	
	WriteFile(hFile, "\r\n", 2, &written, NULL);
	
    // Enumerate the subkeys, until RegEnumKeyEx fails.
    
    if (cSubKeys)
    {
        for (i=0; i<cSubKeys; i++) 
        { 
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKeyIn, i,
                     achKey, 
                     &cbName, 
                     NULL, 
                     NULL, 
                     NULL, 
                     &ftLastWriteTime); 

            if (retCode == ERROR_SUCCESS) 
            {
                char NextKeyIn[MAX_KEY_LENGTH];

				wsprintf(NextKeyIn, "%s\\%s", KeyIn, achKey);
				SaveReg(NextKeyIn, hFile);
            }
        }
    } 
	return TRUE;
}

char Screen[4000];
char NewScreen[4000];

int DoStatus()
{
	int i;
	char callsign[12] = "";
	char flag[3];
	UINT Mask, MaskCopy;
	int Flags;
	int AppNumber;
	int OneBits;
	struct _EXCEPTION_POINTERS exinfo;

	memset(NewScreen, ' ', 33 * 108); 

	strcpy(NewScreen,"    RX  TX MON App Flg Callsign  Program                  RX  TX MON App Flg Callsign  Program");

	strcpy(EXCEPTMSG, "Status Timer Processing");
	__try
	{
	for (i=1;i<65; i++)
	{		
		callsign[0]=0;
		
		if (GetAllocationState(i))

			strcpy(flag,"*");
		else
			strcpy(flag," ");

		GetCallsign(i,callsign);

		Mask = MaskCopy = GetApplMask(i);

		// if only one bit set, convert to number

		AppNumber = 0;
		OneBits = 0;

		while (MaskCopy)
		{
			if (MaskCopy & 1)
				OneBits++;
			
			AppNumber++;
			MaskCopy = MaskCopy >> 1;
		}

		Flags=GetApplFlags(i);

		if (OneBits > 1)
			wsprintf(&NewScreen[(i+1)*54],"%2d%s%3d %3d %3d %03x %3x %10s%-20s",
			i, flag, RXCount(i), TXCount(i), MONCount(i), Mask, Flags, callsign,
			BPQHOSTVECTOR[i-1].PgmName, pgm);
		else
		wsprintf(&NewScreen[(i+1)*54],"%2d%s%3d %3d %3d %3d %3x %10s%-20s",
			i, flag, RXCount(i), TXCount(i), MONCount(i), AppNumber, Flags, callsign,
			BPQHOSTVECTOR[i-1].PgmName, pgm);

	}
	}

	#include "StdExcept.c"

	if (Semaphore && SemProcessID == GetCurrentProcessId())
		FreeSemaphore();

	}

	if (memcmp(Screen, NewScreen, 33 * 108) == 0)	// No Change
		return 0;

	memcpy(Screen, NewScreen, 33 * 108);
	InvalidateRect(StatusWnd,NULL,FALSE);

	return(0);
}

LRESULT CALLBACK StatusWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
    HFONT    hOldFont ;
	HGLOBAL	hMem;
	MINMAXINFO * mmi;
	int i;

	switch (message)
	{
	case WM_TIMER:

		DoStatus();
		break;

	case WM_MDIACTIVATE:
				 
		// Set the system info menu when getting activated
			 
		if (lParam == (LPARAM) hWnd)
		{
			// Activate

			RemoveMenu(hBaseMenu, 1, MF_BYPOSITION);
			AppendMenu(hBaseMenu, MF_STRING + MF_POPUP, (UINT)hConsMenu, "Actions");
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hBaseMenu, (LPARAM) hWndMenu);
		}
		else
		{
			SendMessage(ClientWnd, WM_MDISETMENU, (WPARAM) hMainFrameMenu, (LPARAM) NULL);
		}
	 
		DrawMenuBar(FrameWnd);

		return TRUE; //DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_GETMINMAXINFO:
			
		mmi = (MINMAXINFO *)lParam;
		mmi->ptMaxSize.x = 850;
		mmi->ptMaxSize.y = 500;
		mmi->ptMaxTrackSize.x = 850;
		mmi->ptMaxTrackSize.y = 500;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		//Parse the menu selections:
		
		switch (wmId)
		{

/*
			case BPQSTREAMS:
	
				CheckMenuItem(hMenu,BPQSTREAMS,MF_CHECKED);
				CheckMenuItem(hMenu,BPQIPSTATUS,MF_UNCHECKED);

				StreamDisplay = TRUE;

				break;

			case BPQIPSTATUS:
	
				CheckMenuItem(hMenu,BPQSTREAMS,MF_UNCHECKED);
				CheckMenuItem(hMenu,BPQIPSTATUS,MF_CHECKED);

				StreamDisplay = FALSE;
				memset(Screen, ' ', 4000); 


				break;

*/
	
			case BPQCOPY:
		
			//
			//	Copy buffer to clipboard
			//
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 33*110);
		
			if (hMem != 0)
			{
				if (OpenClipboard(hWnd))
				{
//					CopyScreentoBuffer(GlobalLock(hMem));
					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
				else
				{
					GlobalFree(hMem);
				}

			}

			break;

		}
			
		return DefMDIChildProc(hWnd, message, wParam, lParam);


		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{ 
		case  SC_MAXIMIZE: 

			break;

		case  SC_MINIMIZE: 

			StatusMinimized = TRUE;
			break;

		case  SC_RESTORE: 

			StatusMinimized = FALSE;
			SendMessage(ClientWnd, WM_MDIRESTORE, (WPARAM)hWnd, 0);
			break;
		}

		return DefMDIChildProc(hWnd, message, wParam, lParam);

	case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);
			
			hOldFont = SelectObject( hdc, hFont) ;
			
			for (i=0; i<33; i++)
			{
				TextOut(hdc,0,i*14,&Screen[i*108],108);
			}
			
			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);
	
			break;        

		case WM_DESTROY:
		
//			PostQuitMessage(0);
			
			break;


		default:
		
			return DefMDIChildProc(hWnd, message, wParam, lParam);

	}
	return (0);
}

VOID SaveMDIWindowPos(HWND hWnd, char * RegKey, char * Value, BOOL Minimized)
{
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;
	RECT Rect;

	if (IsWindow(hWnd) == FALSE)
		return;

	ShowWindow(hWnd, SW_RESTORE);

	if (GetWindowRect(hWnd, &Rect) == FALSE)
		return;

	// Make relative to Frame

	Rect.top -= FRect.top ;
	Rect.left -= FRect.left;
	Rect.bottom -= FRect.top;
	Rect.right -= FRect.left;

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\%s", RegKey);
	
	retCode = RegCreateKeyEx(REGTREE, Key, 0, 0, 0,
            KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		wsprintf(Size,"%d,%d,%d,%d,%d", Rect.left, Rect.right, Rect.top ,Rect.bottom, Minimized);
		retCode = RegSetValueEx(hKey, Value, 0, REG_SZ,(BYTE *)&Size, strlen(Size));
		RegCloseKey(hKey);
	}
}

extern int GPSPort;
extern char LAT[];			// in standard APRS Format      
extern char LON[];			// in standard APRS Format

VOID SaveBPQ32Windows()
{
	HKEY hKey=0;
	char Size[80];
	int retCode, disp;
	PEXTPORTDATA PORTVEC=(PEXTPORTDATA)PORTTABLE;
	int i;

	retCode = RegCreateKeyEx(REGTREE, "SOFTWARE\\G8BPQ\\BPQ32", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		wsprintf(Size,"%d,%d,%d,%d", FRect.left, FRect.right, FRect.top, FRect.bottom);
		retCode = RegSetValueEx(hKey, "FrameWindowSize", 0, REG_SZ, (BYTE *)&Size, strlen(Size));

		// Save GPS Position

		if (GPSPort)
		{
			sprintf(Size, "%s, %s", LAT, LON);
			retCode = RegSetValueEx(hKey, "GPS", 0, REG_SZ,(BYTE *)&Size, strlen(Size));
		}

		RegCloseKey(hKey);
	}
	
	SaveMDIWindowPos(StatusWnd, "", "StatusWindowSize", StatusMinimized);
	SaveMDIWindowPos(hConsWnd, "", "WindowSize", ConsoleMinimized);

	for (i=0;i<NUMBEROFPORTS;i++)
	{
		if (PORTVEC->PORTCONTROL.PORTTYPE == 0x10)			// External
		{
			if (PORTVEC->PORT_EXT_ADDR)
			{
				SaveWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
				SaveAXIPWindowPos(PORTVEC->PORTCONTROL.PORTNUMBER);
			}
		}
		PORTVEC=(PEXTPORTDATA)PORTVEC->PORTCONTROL.PORTPOINTER;		
	}

	if (hIPResWnd)
		SaveMDIWindowPos(hIPResWnd, "", "IPResSize", IPMinimized);

	SaveHostSessions();
}

VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);


VOID SendUIModeFrame(struct TRANSPORTENTRY * Sess, PMESSAGE Message, int Port)
{
	DIGIMESSAGE Msg;

	memset(&Msg, 0, sizeof(Msg));

	Msg.PORT = Port;
	Msg.CTL = 3;			// UI
	memcpy(Msg.DEST, Sess->UADDRESS, 7);
	memcpy(Msg.ORIGIN, Sess->L4MYCALL, 7);
	memcpy(Msg.DIGIS, &Sess->UADDRESS[7], Sess->UAddrLen - 7);
	memcpy(&Msg.PID, Message->DEST, Message->LENGTH - 7);
	Send_AX_Datagram(&Msg, Message->LENGTH + Sess->UAddrLen - 14, Port);
}



