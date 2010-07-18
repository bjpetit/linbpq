// Mail and Chat Server for BPQ32 Packet Switch
//
//

// Version 1.0.0.17

//	Split Messasge, User and BBS Editing from Main Config.
//	Add word wrap to Console input and output
//  Flash Console on chat user connect
//	Fix procesing Name response in chat mode
//	Fix processing of *RTL from station not defined as a Chat Node
//	Fix overlength lines ln List responses
//  Housekeeping expires BIDs 
//  Killing a message removes it from the forwarding counts

// Version 1.0.0.18

// Save User Database when name is entered or updated so it is not lost on a crash
// Fix Protocol Error in Compressed Forwarding when switching direction
// Add Housekeeping results dialog.

// Version 1.0.0.19

// Allow PACLEN in forward scripts.
// Store and forward messages with CRLF as line ends
// Send Disconnect after FQ ( for LinFBB)
// "Last Listed" is saved if MailChat is closed without closing Console
// Maximum acceptable message length can be specified (in Forwarding Config)

// Version 1.0.0.20

// Fix error in saving forwarding config (introduced in .19)
// Limit size of FBB forwarding block.
// Clear old connection (instead of new) if duplicate connect on Chat Node-Node link
// Send FA for Compressed Mail (was sending FB for both Compressed and Uncompressed)

// Version 1.0.0.21

// Fix Connect Script Processing (wasn't waiting for CONNECTED from last step)
// Implement Defer
// Fix MBL-style forwarding
// Fix Add User (Params were not saved)
// Add SC (Send Copy) Command
// Accept call@bbs as well as call @ bbs

// Version 1.0.0.22

// Implement RB RP LN LR LF LN L$ Commands.
// Implement QTH and ZIP Commands.
// Entering an empty Title cancels the message.
// Uses HomeBBS field to set @ field for local users.
// Creates basic WP Database.
// Uses WP to lookup @ field for non-local calls.
// Console "Actions" Menu renamed "Options".
// Excluded flag is actioned.
// Asks user to set HomeBBS if not already set.
// Fix "Shrinking Message" problem, where message got shorter each time it was read Initroduced in .19).
// Flash Server window when anyone connects to chat (If Console Option "Flash on Chat User Connect" set).

// Version 1.0.0.23

// Fix R: line scan bug

// Version 1.0.0.24

// Fix closing console window on 'B'.
// Fix Message Creation time.
// Enable Delete function in WP edit dialog

// Version 1.0.0.25

// Implement K< and K> commands
// Experimental support for B1 and B2 forwarding
// Experimental UI System
// Fix extracting QTH from WP updates

// Version 1.0.0.26

// Add YN etc responses for FBB B1/B2

// Version 1.0.0.27

// Fix crash if NULL received as start of a packet.
// Add Save WP command
// Make B2 flag BBS-specific.
// Implement B2 Send

// Version 1.0.0.28

// Fix parsing of smtp to addresses - eg smtp:john.wiseman@cantab.net
// Flag messages as Held if smtp server rejects from or to addresses
// Fix Kill to (K> Call)
// Edit Message dialog shows latest first
// Add chat debug window to try to track down occasional chat connection problems

// Version 1.0.0.29

// Add loads of try/excspt

// Version 1.0.0.30

// Writes Debug output to LOG_DEBUG and Monitor Window

// Version 1.0.0.32

// Allow use of GoogleMail for ISP functions
// Accept SYSOP as alias for SYSOPCall - ie user can do SP SYSOP, and it will appear in sysop's LM, RM, etc
// Email Housekeeping Results to SYSOP

// Version 1.0.0.33

// Housekeeping now runs at Maintenance Time. Maintenance Interval removed. 
// Allow multiple numbers on R and K commands
// Fix L command with single number
// Log if Forward count is out of step with messages to forward.
// UI Processing improved and F< command implemented

// Version 1.0.0.34

// Semaphore Chat Messages
// Display Semaphore Clashes
// More Program Error Traps
// Kill Messages more than BIDLifetime old

// Version 1.0.0.35

// Test for Mike - Remove B1 check from Parse_SID

// Version 1.0.0.36

// Fix calculation of Housekeeping Time.
// Set dialog box background explicitly.
// Remove tray entry for chat debug window.
// Add date to log file name.
// Add Actions Menu option to disable logging.
// Fix size of main window when it changes between versions.

// Version 1.0.0.37

// Implement Paging.
// Fix L< command (was giving no messages).
// Implement LR LR mmm-nnn LR nnn- (and L nnn-)
// KM should no longer kill SYSOP bulls.
// ISP interfaces allows SMTP Auth to be configured
// SMTP Client would fail to send any more messages if a connection failed

// Version 1.0.0.38

// Don't include killed messages in L commands (except LK!)
// Implement l@
// Add forwarding timebands
// Allow resizing of main window.
// Add Ver command.

// Version 1.0.1.1

// First Public Beta

// Fix part line handling in Console
// Maintenance deletes old log files.
// Add option to delete files to the recycle bin.

// Version 1.0.2.1

// Allow all Node SYSOP commands in connect scripts.
// Implement FBB B1 Protocol with Resume
// Make FBB Max Block size settable for each BBS.
// Add extra logging when Chat Sessions refused.
// Fix Crash on invalid housekeeping override.
// Add Hold Messages option.
// Trap CRT Errors
// Sort Actions/Start Forwarding List

// Version 1.0.2.2

// Fill in gaps in BBS Number sequence
// Fix PE if ctext contains }
// Run Houskeeping at startup if previous Housekeeping was missed

// Version 1.0.2.3

// Add configured nodes to /p listing

// Version 1.0.2.4

// Fix RMS (it wanted B2 not B12)
// Send messages if available after rejecting all proposals
// Dont try to send msg back to originator.

// Version 1.0.2.5

// Fix timeband processing when none specified.
// Improved Chat Help display.
// Add helpful responses to /n /q and /t

// Version 1.0.2.6

// Kill Personal WP messages after processing
// Make sure a node doesnt try to "join" or "leave" a node as a user.
// More tracing to try to track down lost topic links.
// Add command recall to Console
// Show users in new topic when changing topic
// Add Send From Clipboard" Action

// Version 1.0.2.7

// Hold messages from the future, or with invalid dates.
// Add KH (kill held) command.
// Send Message to SYSOP when a new user connects.

// Version 1.0.2.8

// Don't reject personal message on Dup BID unless we already have an unforwarded copy.
// Hold Looping messages.
// Warn SYSOP of held messages.

// Version 1.0.2.9

// Close connecton on receipt of *** DONE (MBL style forwarding).
// Improved validation in link_drop (Chat Node)
// Change to welcome prompt and Msg Header for Outpost.
// Fix Connect Script processing for KA Nodes

// Version 1.0.3.1

// Fix incorrect sending of NO - BID.
// Fix problems caused by a user being connected to more than one chat node.
// Show idle time on Chat /u display.
// Rewrite forwarding by HA.
// Add "Bad Words" Test.
// Add reason for holding to SYSOP "Message Held" Message.
// Make topics case-insensitive.
// Allow SR for smtp mail.
// Try to fix some user's "Add User" problem.


// Version 1.0.3.2

// Fix program error when prcessing - response in FBB forwarding.
// Fix code to flag messages as sent.


// Version 1.0.3.3

// Attempt to fix message loop on topic_change
// Fix loop if compressed size is greater than 32K when receiving with B1 protocol.
// Fix selection of B1

// Version 1.0.3.4

// Add "KISS ONLY" Flag to R: Lines (Needs Node Version 4.10.12 (4.10l) or above)
// Add Basic NNTP Interface
// Fix possible loop in lzhuf encode

// Version 1.0.3.5

// Fix forwarding of Held Messages
// More attempts to fix Chat crashes.
// Limit join/leave problem with mismatched nodes.
// Add Chat Node Monitoring System.
// Change order of elements in nntp addresses (now to.at, was at.to)

// Version 1.0.3.6

// Restart and Exit if too many errors
// Fix forwarding of killed messages.
// Fix Forwarding to PaKet.
// Fix problem if BBS signon contains words from the "Fail" list

// Version 1.0.3.7

// re-fix loop if compressed size is greater than 32K - reintroduced in 1.0.3.4
// Add last message to edit users
// Change Console and Monitor Buffer sizes
// Don't flag msg as 'Y' on read if it was Held or Killed

// Version 1.0.3.8

// Don't connect if all messages for a BBS are held.
// Hold message if From or To are missing.
// Fix parsing of /n and /q commands
// fix possible loop on changing name or qth

// Version 1.0.3.9

// More Chat fixes and monitoring
// Added additional console for chat

// Version 1.0.3.10

// Fix for corruption of CIrcuit-Node chain.

// Version 1.0.3.11

// Fix flow control for SMTP and NNTP 

// Version 1.0.3.12

// Fix crash in SendChatStatus if no Chat Links Defined.
// Disable Chat Mode if there is no ApplCall for ChatApplNum,
// Add Edit Message to Manage Messages Dialog
// NNTP needs authentication


// Version 1.0.3.13

// Fix Chat ApplCall warning when ChatAppl = 0
// Add NNTP NEWGROUPS Command
// Fix MBL Forwarding (remove extra > prompt after SP)

// Version 1.0.3.14

// Fix topic switch code.
// Send SYSOP messages on POP3 interface if User SYSOP flag is set.
// NNTP only needs Authentication for posting, not reading.

// Version 1.0.3.15

// Fix reset of First to Forward after househeeping

// Version 1.0.3.16

// Fix check of HA for terminating WW
// MBL Mode remove extra > prompts
// Fix program error if WP record has unexpected format
// Connect Script changes for WINMOR
// Fix typo in unconfigured node has connected message

// Version 1.0.3.17

// Fix forwarding of Personals 

// Version 1.0.3.18

// Fix detection of misconfigured nodes to work with new nodes.
// Limit connection attempt rate when a chat node is unavailable.
// Fix Program Error on long input lines (> ~250 chars).

// Version 1.0.3.19

// Fix Restart of B2 mode transfers.
// Fix error if other end offers B1 and you are configured for B2 only.


// Version 1.0.3.20

// Fix Paging in Chat Mode.
// Report Node Versions.

// Version 1.0.3.21

// Check node is not already known when processing OK
// Add option to suppress emailing of housekeeping results

// Version 1.0.3.22

// Correct Version processing when user connects via the network
// Add time controlled forwarding scripts

// Version 1.0.3.23

// Changes to RMS forwarding

// Version 1.0.3.24

// Fix RMS: from SMTP interface
// Accept RMS/ instead of RMS: for Thunderbird

// Version 1.0.3.25

// Accept smtp: addresses from smtp client, and route to ISP gateway.
// Set FROM address of messages from RMS that are delivered to smtp client so a reply will go back via RMS.

// Version 1.0.3.26

// Improve display of rms and smtp messages in message lists and message display.

// Version 1.0.3.27

// Correct code that prevents mail being retured to originating BBS.
// Tidy stuck Nodes and Topics when all links close
// Fix B2 handling of @ to TO Address.

// Version 1.0.3.28

// Ensure user Record for the BBS Call has BBS bit set.
// Don't send messages addressed @winlink.org if addressee is a local user with Poll RMS set.
// Add user configurable welcome messages.

// Version 1.0.3.29

// Add AUTH feature to Rig Control

// Version 1.0.3.30

// Process Paclink Header (;FW:)

// Version 1.0.3.31

// Process Messages with attachments.
// Add inactivity timeout to Chat Console sessions.

// Version 1.0.3.32

// Fix for Paclink > BBS Addresses 

// Version 1.0.3.33

// Fix multiple transfers per session for B2.
// Kill messages eent to paclink.
// Add option to forward messages on arrival.

// Version 1.0.3.34

// Fix bbs addresses to winlink.
// Fix adding @winlink.org to imcoming paclink msgs

// Version 1.0.3.35

// Fix bbs addresses to winlink. (Again)

// Version 1.0.3.36

// Restart changes for RMS/paclink

// Version 1.0.3.37

// Fix for RMS Express forwarding

// Version 1.0.3.38

// Fixes for smtp and lower case packet addresses from Airmail
// Fix missing > afer NO - Bid in MBL mode

// Version 1.0.3.39

// Use ;FW: for RMS polling.

// Version 1.0.3.40

// Add ELSE Option to connect scripts.

// Version 1.0.3.41

// Improved handling of Multiple Addresses
// Add user colours to chat.

// Version 1.0.3.42

// Poll multiple SSID's for RMS
// Colour support for BPQTEerminal
// New /C chat command to toggle colour on or off.

// Version 1.0.3.43

// Add SKIPPROMPT command to forward scripts

// Version 1.0.4.1

// Non - Beta Release
// Fix possible crash/corruption with long B2 messages

// Version 1.0.4.2

// Add @winlink.org to the B2 From addresss if it is just a callsign
// Route Flood Bulls on TO as well as @

// Version 1.0.4.3

// Handle Packet Addresses from RMS Express
// Fix for Housekeeping B$ messages

// Version 1.0.4.4

// Remove B2 header and all but the Body part from messages forwared using MBL
// Fix handling of ;FW: from RMS Express

// Version 1.0.4.5

// Disable Paging on forwarding sessions.
// Kill Msgs sent to RMS Exxpress
// Add Name to Chat *** Joined msg

// Version 1.0.4.6

// Pass smtp:winlink.org messages from Airmail to local user check
// Only apply local user check to RMS: messages @winlink.org
// Check locally input smtp: messages for local winlink.org users
// Provide facility to allow only one connect on a port

// Version 1.0.4.8

//	Only reset last listed on L or LR commands.

// Version 1.0.4.9

// Fix error in handling smtp: messages to winlink.org addresses from Airmail

// Version 1.0.4.10

// Fix Badwords processing
// Add Connect Script PAUSE command

// Version 1.0.4.11

// Suppress display amd listing of held messages
// Add option to exclude SYSOP messages from LM, KM, etc
// Fix crash whan receiving messages with long lines via plain text forwarding


// Use Windows Sound Events for (Chat "user join" alert)

#include "stdafx.h"

//#define SPECIALVERSION "Ken"

#include "GetVersion.h"

#define MAX_LOADSTRING 100

INT_PTR CALLBACK UserEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MsgEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FwdEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WPEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

UCHAR * (FAR WINAPI * GetVersionStringptr)();
struct _EXTPORTDATA * (FAR WINAPI * GetPortTableEntryptr)();

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

int LastVer[4] = {0, 0, 0, 0};					// In case we need to do somthing the first time a version is run

HWND MainWnd;
HWND hWndSess;
RECT MainRect;
HMENU hActionMenu;
static HMENU hMenu;
HMENU hDisMenu;									// Disconnect Menu Handle
HMENU hFWDMenu;									// Forward Menu Handle

int SessX, SessY, SessWidth;					// Params for Session Window

char szBuff[80];

#define MaxSockets 64

ConnectionInfo Connections[MaxSockets+1];


struct SEM ChatSemaphore = {0, 0};

struct SEM AllocSemaphore = {0, 0};

struct UserInfo ** UserRecPtr=NULL;
int NumberofUsers=0;

struct UserInfo * BBSChain = NULL;						// Chain of users that are BBSes

struct MsgInfo ** MsgHddrPtr=NULL;
int NumberofMessages=0;

int FirstMessageIndextoForward=0;					// Lowest Message wirh a forward bit set - limits search

BIDRec ** BIDRecPtr=NULL;
int NumberofBIDs=0;

BIDRec ** TempBIDRecPtr=NULL;
int NumberofTempBIDs=0;

WPRec ** WPRecPtr=NULL;
int NumberofWPrecs=0;

char ** BadWords=NULL;
int NumberofBadWords=0;
char * BadFile = NULL;

int LatestMsg = 0;
struct SEM MsgNoSemaphore = {0, 0};			// For locking updates to LatestMsg
int HighestBBSNumber = 0;

int MaxMsgno = 60000;
int BidLifetime = 60;
int MaintInterval = 24;
int MaintTime = 0;

BOOL cfgMinToTray;

BOOL DisconnectOnClose=FALSE;


char PasswordMsg[100]="Password:";

char cfgHOSTPROMPT[100];

char cfgCTEXT[100];

char cfgLOCALECHO[100];

char AttemptsMsg[] = "Too many attempts - Disconnected\r\r";
char disMsg[] = "Disconnected by SYSOP\r\r";

char LoginMsg[]="user:";

char BlankCall[]="         ";


UCHAR BBSApplMask;
UCHAR ChatApplMask;

int BBSApplNum=0;
int ChatApplNum=0;

//int	StartStream=0;
int	NumberofStreams=0;
int MaxStreams=0;

char BBSSID[]="[BPQ-%d.%d.%d.%d-%s%s%s%sH$]\r";
//char BBSSID[]="[BPQ-1.00-AB1FHMRX$]\r";

char ChatSID[]="[BPQChatServer-%d.%d.%d.%d]\r";

char NewUserPrompt[100]="Please enter your Name: ";

char * WelcomeMsg = NULL;
char * NewWelcomeMsg = NULL;
char * ChatWelcomeMsg = NULL;
char * NewChatWelcomeMsg = NULL;

char BBSName[100];

char HRoute[100];

char SignoffMsg[100];

char AbortedMsg[100]="\rOutput aborted\r";

char UserDatabaseName[MAX_PATH] = "BPQBBSUsers.dat";
char UserDatabasePath[MAX_PATH];

char MsgDatabasePath[MAX_PATH];
char MsgDatabaseName[MAX_PATH] = "DIRMES.SYS";

char BIDDatabasePath[MAX_PATH];
char BIDDatabaseName[MAX_PATH] = "WFBID.SYS";

char WPDatabasePath[MAX_PATH];
char WPDatabaseName[MAX_PATH] = "WP.SYS";

char BadWordsPath[MAX_PATH];
char BadWordsName[MAX_PATH] = "BADWORDS.SYS";

char BaseDir[MAX_PATH];

char MailDir[MAX_PATH];

char RlineVer[50];

BOOL KISSOnly = FALSE;

BOOL ALLOWCOMPRESSED = TRUE;

BOOL EnableUI = FALSE;

UCHAR * OtherNodes=NULL;

char zeros[NBMASK];						// For forward bitmask tests

time_t MaintClock;						// Time to run housekeeping

struct MsgInfo * MsgnotoMsg[100000];	// Message Number to Message Slot List.

int ProgramErrors = 0;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
ATOM				RegisterMainWindowClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ClpMsgDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    ChatMapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct _EXCEPTION_POINTERS exinfox;
	
CONTEXT ContextRecord;
EXCEPTION_RECORD ExceptionRecord;

	DWORD Stack[16];

Dump_Process_State(struct _EXCEPTION_POINTERS * exinfo, char * Msg)
{
	unsigned int SPPtr;
	unsigned int SPVal;

	memcpy(&ContextRecord, exinfo->ContextRecord, sizeof(ContextRecord));
	memcpy(&ExceptionRecord, exinfo->ExceptionRecord, sizeof(ExceptionRecord));
		
	SPPtr = ContextRecord.Esp;

	Debugprintf("BPQMailChat *** Program Error %x at %x in %s",
	ExceptionRecord.ExceptionCode, ExceptionRecord.ExceptionAddress, Msg);	


	__asm{

		mov eax, SPPtr
		mov SPVal,eax
		lea edi,Stack
		mov esi,eax
		mov ecx,64
		rep movsb

	}

	Debugprintf("EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x ESP %x",
		ContextRecord.Eax, ContextRecord.Ebx, ContextRecord.Ecx,
		ContextRecord.Edx, ContextRecord.Esi, ContextRecord.Edi, SPVal);
		
	Debugprintf("Stack:");

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal, Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7]);

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal+32, Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);

}



void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
	Logprintf(LOG_DEBUG, NULL, '!', "*** Error **** C Run Time Invalid Parameter Handler Called");

	if (expression && function && file)
	{
		Logprintf(LOG_DEBUG, NULL, '!', "Expression = %S", expression);
		Logprintf(LOG_DEBUG, NULL, '!', "Function %S", function);
		Logprintf(LOG_DEBUG, NULL, '!', "File %S Line %d", file, line);
	}
}

// If program gets too many program errors, it will restart itself  and shut down

VOID CheckProgramErrors()
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
	ProgramErrors++;

	if (ProgramErrors > 25)
	{
		Logprintf(LOG_DEBUG, NULL, '!', "Too Many Program Errors - Closing");

		if (cfgMinToTray)
		{
			DeleteTrayMenuItem(MainWnd);
			if (ConsHeader[0]->hConsole)
				DeleteTrayMenuItem(ConsHeader[0]->hConsole);
			if (ConsHeader[1]->hConsole)
				DeleteTrayMenuItem(ConsHeader[1]->hConsole);
			if (hMonitor)
				DeleteTrayMenuItem(hMonitor);
			if (hDebug)
				DeleteTrayMenuItem(hDebug);
		}

		
		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
  		SInfo.lpReserved2=NULL; 

		CreateProcess("BPQMailChat.exe" ,"BPQMailChat.exe WAIT" , NULL, NULL, FALSE,0 ,NULL ,NULL, &SInfo, &PInfo);
					
		exit(0);
	}
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	int BPQStream, n;
	struct UserInfo * user;
	struct _EXCEPTION_POINTERS exinfo;
	_invalid_parameter_handler oldHandler, newHandler;
	char Msg[100];
	int i = 60;

	if (_stricmp(lpCmdLine, "Wait") == 0)				// If AutoRestart then Delay 60 Secs
	{	
		hWnd = CreateWindow("STATIC", "MailChat Restarting after Failure - Please Wait", 0,
		CW_USEDEFAULT, 100, 550, 70,
		NULL, NULL, hInstance, NULL);

		ShowWindow(hWnd, nCmdShow);

		while (i-- > 0)
		{
			wsprintf(Msg, "MailChat Restarting after Failure - Please Wait %d secs.", i);
			SetWindowText(hWnd, Msg);
			
			Sleep(1000);
		}

		DestroyWindow(hWnd);
	}

	// Trap CRT Errors
	
	newHandler = myInvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BPQMailChat, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BPQMailChat));

	// Main message loop:

	Logprintf(LOG_DEBUG, NULL, '!', "Program Starting");

	while (GetMessage(&msg, NULL, 0, 0))
	{
		__try
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		My__except_Routine("GetMessageLoop");
	}

	__try
	{
	for (n = 0; n < NumberofStreams; n++)
	{
		BPQStream=Connections[n].BPQStream;
		
		if (BPQStream)
		{
			SetAppl(BPQStream, 0, 0);
			Disconnect(BPQStream);
			DeallocateStream(BPQStream);
		}
	}

	ClearChatLinkStatus();
	SendChatLinkStatus();

	Sleep(1000);				// A bit of time for links to close

	SendChatLinkStatus();		// Send again to reduce chance of being missed

	if (ConsHeader[0]->hConsole)
		DestroyWindow(ConsHeader[0]->hConsole);
	if (ConsHeader[1]->hConsole)
		DestroyWindow(ConsHeader[1]->hConsole);

	SaveUserDatabase();
	SaveMessageDatabase();
	SaveBIDDatabase();

	if (cfgMinToTray)
	{
		DeleteTrayMenuItem(MainWnd);
		if (ConsHeader[0]->hConsole)
			DeleteTrayMenuItem(ConsHeader[0]->hConsole);
		if (ConsHeader[1]->hConsole)
			DeleteTrayMenuItem(ConsHeader[1]->hConsole);
		if (hMonitor)
			DeleteTrayMenuItem(hMonitor);
		if (hDebug)
			DeleteTrayMenuItem(hDebug);
	}

	// Free all allocated memory

	for (n = 0; n <= NumberofUsers; n++)
	{
		user = UserRecPtr[n];

		if (user->ForwardingInfo)
		{
			FreeForwardingStruct(user);
			free(user->ForwardingInfo); 
		}

		free(user);
	}
	
	free(UserRecPtr);

	for (n = 0; n <= NumberofMessages; n++)
		free(MsgHddrPtr[n]);

	free(MsgHddrPtr);

	for (n = 0; n <= NumberofWPrecs; n++)
		free(WPRecPtr[n]);

	free(WPRecPtr);

	for (n = 0; n <= NumberofBIDs; n++)
		free(BIDRecPtr[n]);

	free(BIDRecPtr);

	if (TempBIDRecPtr)
		free(TempBIDRecPtr);

	for (n = 1; n <= NumberofNNTPRecs; n++)
		free(NNTPRecPtr[n]);

	free(NNTPRecPtr);

	free(OtherNodes);

	FreeChatMemory();

	if (BadWords) free(BadWords);
	if (BadFile) free(BadFile);

	n = 0;

	if (Aliases)
	{
		while(Aliases[n])
		{
			free(Aliases[n]->Dest);
			free(Aliases[n]);
			n++;
		}

		free(Aliases);
		FreeList(AliasText);
	}

	FreeOverrides();

	Free_UI();

	for (n=1; n<20; n++)
	{
		if (MyElements[n]) free(MyElements[n]);
	}

	_CrtDumpMemoryLeaks();

	}
	My__except_Routine("Close Processing");

	SaveWindowConfig();
	
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
//
#define BGCOLOUR RGB(236,233,216)
//#define BGCOLOUR RGB(245,245,245)

HBRUSH bgBrush;

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= DLGWINDOWEXTRA;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BPQMailChat));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= bgBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BPQMailChat);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND hWnd;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	char Title[80];
	WSADATA WsaData;
	HMENU hTopMenu;		// handle of menu 
	HKEY hKey=0;
	int retCode,Type,Vallen;
	char Size[80];
	RECT InitRect;
	RECT SessRect;

	HMODULE ExtDriver=LoadLibrary("bpq32.dll");

	GetVersionStringptr = (UCHAR *(__stdcall *)())GetProcAddress(ExtDriver,"_GetVersionString@0");
	GetPortTableEntryptr = (struct _EXTPORTDATA *(__stdcall *)())GetProcAddress(ExtDriver,"_GetPortTableEntry@4");

	if (GetPortTableEntryptr)
	{
		int n;
		struct _EXTPORTDATA * PORTVEC;

		KISSOnly = TRUE;
		
		for (n=1; n <= GetNumberofPorts(); n++)
		{
			PORTVEC = GetPortTableEntryptr(n);

			if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			{
				KISSOnly = FALSE;

				if (_memicmp(PORTVEC->PORT_DLL_NAME, "BPQAXIP.DLL", 11) == 0)
					AXIPPort = PORTVEC->PORTCONTROL.PORTNUMBER;
			}
		}
	}

	// Get Window Size  From Registry

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;
		RegQueryValueEx(hKey,"WindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&MainRect.left,&MainRect.right,&MainRect.top,&MainRect.bottom);
		RegCloseKey(hKey);
	}

	hInst = hInstance;

	hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	MainWnd=hWnd;

	if (MainRect.right < 100 || MainRect.bottom < 100)
	{
		GetWindowRect(hWnd,	&MainRect);
	}

	MoveWindow(hWnd,MainRect.left,MainRect.top, MainRect.right-MainRect.left, MainRect.bottom-MainRect.top, TRUE);

	GetVersionInfo(NULL);

	wsprintf(Title,"G8BPQ Mail and Chat Server Version %s", VersionString);

	wsprintf(RlineVer, "BPQ%s%d.%d.%d", (KISSOnly) ? "K" : "", Ver[0], Ver[1], Ver[2]);

	SetWindowText(hWnd,Title);

	hWndSess = GetDlgItem(hWnd, 100); 

	GetWindowRect(hWnd,	&InitRect);
	GetWindowRect(hWndSess, &SessRect);

	SessX = SessRect.left - InitRect.left ;
	SessY = SessRect.top -InitRect.top;
	SessWidth = SessRect.right - SessRect.left;

   	// Get handles fou updating menu items

	hTopMenu=GetMenu(MainWnd);
	hActionMenu=GetSubMenu(hTopMenu,0);

	hFWDMenu=GetSubMenu(hActionMenu,0);
	hMenu=GetSubMenu(hActionMenu,1);
	hDisMenu=GetSubMenu(hActionMenu,2);

   CheckTimer();

 	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
		{
			ShowWindow(hWnd, SW_HIDE);
		}
		else
		{
			ShowWindow(hWnd, nCmdShow);
		}
	else
		ShowWindow(hWnd, nCmdShow);

   UpdateWindow(hWnd);

   WSAStartup(MAKEWORD(2, 0), &WsaData);
    
   return Initialise();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int state,change;
	ConnectionInfo * conn;
	struct _EXCEPTION_POINTERS exinfo;


	if (message == BPQMsg)
	{
		if (lParam & BPQMonitorAvail)
		{
			__try
			{
				DoMonitorData(wParam);
			}
			My__except_Routine("DoMonitorData");

			return 0;
			
		}
		if (lParam & BPQDataAvail)
		{
			__try
			{
				DoReceivedData(wParam);
			}
			My__except_Routine("DoReceivedData")
			return 0;
		}
		if (lParam & BPQStateChange)
		{
			//	Get current Session State. Any state changed is ACK'ed
			//	automatically. See BPQHOST functions 4 and 5.
	
			__try
			{
				SessionState(wParam, &state, &change);
		
				if (change == 1)
				{
					if (state == 1) // Connected	
					{__try {Connected(wParam);} My__except_Routine("Connected");}
					else
						__try{Disconnected(wParam);} My__except_Routine("Disconnected");
				}
			}
			My__except_Routine("DoStateChange");

		}

		return 0;
	}


	switch (message)
	{

	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case NNTP_DATA: // Notification on data socket

		NNTP_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(wParam);
		return 0;

	case NNTP_ACCEPT: /* Notification if a socket connection is pending. */

		NNTP_Accept(wParam);
		return 0;

	case WSA_CONNECT: /* Notification if a socket connection completed. */

		Socket_Connect(wParam, WSAGETSELECTERROR(lParam));
		return 0;


 			
	case WM_TIMER:

		if (wParam == 1)		// Slow = 10 secs
		{
			__try
			{
				RefreshMainWindow();
				CheckTimer();
				TCPTimer();
				FWDTimerProc();
				ChatTimer();
				if (MaintClock < time(NULL))
				{				
					DoHouseKeeping(FALSE);
					MaintClock += 86400;					
				}
			}
			My__except_Routine("Slow Timer");
		}
		else
			__try
			{
				TrytoSend();
			}
			My__except_Routine("TrytoSend");
		
		return (0);

	
	case WM_CTLCOLORDLG:
        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }

	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)hActionMenu)
		{
			if (IsClipboardFormatAvailable(CF_TEXT))
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_DISABLED);
			
			return TRUE;
		}

		if (wParam == (WPARAM)hFWDMenu)
		{
			// Set up Forward Menu

			struct UserInfo * user;
			char MenuLine[30];

			for (user = BBSChain; user; user = user->BBSNext)
			{
				wsprintf(MenuLine, "%s %d Msgs", user->Call, CountMessagestoForward(user->BBSNumber));

				if (ModifyMenu(hFWDMenu, IDM_FORWARD_ALL + user->BBSNumber, 
					MF_BYCOMMAND | MF_STRING, IDM_FORWARD_ALL + user->BBSNumber, MenuLine) == 0)
	
				AppendMenu(hFWDMenu, MF_STRING,IDM_FORWARD_ALL + user->BBSNumber, MenuLine);
			}
			return TRUE;
		}

		if (wParam == (WPARAM)hDisMenu)
		{
			// Set up Forward Menu

			CIRCUIT * conn;
			char MenuLine[30];
			int n;

			for (n = 0; n <= NumberofStreams-1; n++)
			{
				conn=&Connections[n];

				RemoveMenu(hDisMenu, IDM_DISCONNECT + n, MF_BYCOMMAND);

				if (conn->Active)
				{
					sprintf_s(MenuLine, 30, "%d %s", conn->BPQStream, conn->Callsign);
					AppendMenu(hDisMenu, MF_STRING, IDM_DISCONNECT + n, MenuLine);
				}
			}
			return TRUE;
		}
		break;


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmEvent == LBN_DBLCLK)

				break;

		if (wmId >= IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			conn=&Connections[wmId-IDM_DISCONNECT];
		
			if (conn->Active)
			{	
				Disconnect(conn->BPQStream);
			}
		}

		if (wmId >= IDM_FORWARD_ALL && wmId < IDM_FORWARD_ALL + 100)
		{
			StartForwarding(wmId - IDM_FORWARD_ALL);
			return 0;
		}

		switch (wmId)
		{
		case IDM_LOGBBS:

			ToggleParam(hMenu, hWnd, &LogBBS, IDM_LOGBBS);
			break;

		case IDM_LOGCHAT:

			ToggleParam(hMenu, hWnd, &LogCHAT, IDM_LOGCHAT);
			break;

		case IDM_LOGTCP:

			ToggleParam(hMenu, hWnd, &LogTCP, IDM_LOGTCP);
			break;

		case IDM_HOUSEKEEPING:

			DoHouseKeeping(TRUE);
			break;

		case IDM_CONSOLE:

			CreateConsole(-1);
			break;

		case IDM_CHATCONSOLE:

			CreateConsole(-2);
			break;

		case IDM_MONITOR:

			CreateMonitor();
			break;

		case IDM_DEBUG:

			CreateDebugWindow();
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_CONFIG:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
			break;

		case IDM_USERS:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_USEREDIT), hWnd, UserEditDialogProc);
			break;

		case IDM_FWD:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_FORWARDING), hWnd, FwdEditDialogProc);
			break;

		case IDM_MESSAGES:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_MSGEDIT), hWnd, MsgEditDialogProc);
			break;

		case IDM_WP:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_EDITWP), hWnd, WPEditDialogProc);
			break;

		case ID_ACTIONS_SENDMSGFROMCLIPBOARD:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_MSGFROMCLIPBOARD), hWnd, ClpMsgDialogProc);
			break;


		case ID_ACTIONS_UPDATECHATMAPINFO:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_UPDATECHATMAP), hWnd, ChatMapDialogProc);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

  
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		return (0);

	
	case WM_SIZING:
	{
		LPRECT lprc = (LPRECT) lParam;
		int Height = lprc->bottom-lprc->top;
		int Width = lprc->right-lprc->left;

		MoveWindow(hWndSess, 0, 30, SessWidth, Height - 100, TRUE);
			
		return TRUE;
	}


	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:

		GetWindowRect(MainWnd,	&MainRect);	// For save soutine
		if (ConsHeader[0]->hConsole)
			GetWindowRect(ConsHeader[0]->hConsole, &ConsHeader[0]->ConsoleRect);	// For save soutine
		if (ConsHeader[1]->hConsole)
			GetWindowRect(ConsHeader[1]->hConsole, &ConsHeader[1]->ConsoleRect);	// For save soutine
		if (hMonitor)
			GetWindowRect(hMonitor,	&MonitorRect);	// For save soutine

		KillTimer(hWnd,1);
		KillTimer(hWnd,2);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


INT_PTR CALLBACK ClpMsgDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HGLOBAL   hglb; 
    LPTSTR    lptstr; 

	switch (message)
	{
	case WM_INITDIALOG:

       if (!IsClipboardFormatAvailable(CF_TEXT)) 
            return TRUE; 

        if (!OpenClipboard(hDlg)) 
            return TRUE; 
 
        hglb = GetClipboardData(CF_TEXT); 

        if (hglb != NULL) 
        { 
            lptstr = GlobalLock(hglb);

            if (lptstr != NULL) 
            { 
				SetDlgItemText(hDlg, IDC_EDIT1, lptstr);
				GlobalUnlock(hglb); 
            } 
        } 
        CloseClipboard(); 

		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "B");
		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "P");
		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "T");

		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_SETCURSEL, 0, 0);

		return TRUE; 

	case WM_SIZING:
	{
		HWND hWndEdit = GetDlgItem(hDlg, IDC_EDIT1); 

		LPRECT lprc = (LPRECT) lParam;
		int Height = lprc->bottom-lprc->top;
		int Width = lprc->right-lprc->left;

		MoveWindow(hWndEdit, 5, 90, Width-20, Height - 140, TRUE);
			
		return TRUE;
	}

	case WM_COMMAND:

		if (LOWORD(wParam) == IDSEND)
		{
			char status [3];
			struct MsgInfo * Msg;
			char * via = NULL;
			char BID[13];
			BIDRec * BIDRec;
			int MsgLen;
			char * MailBuffer;
			char MsgFile[MAX_PATH];
			HANDLE hFile = INVALID_HANDLE_VALUE;
			int WriteLen=0;
			char HDest[61];
			char * Vptr;


			MsgLen = SendDlgItemMessage(hDlg, IDC_EDIT1, WM_GETTEXTLENGTH, 0 ,0);

			if (MsgLen)
			{
				MailBuffer = malloc(MsgLen+1);
				GetDlgItemText(hDlg, IDC_EDIT1, MailBuffer, MsgLen+1);
			}

			GetDlgItemText(hDlg, IDC_MSGTO, HDest, 60);

			GetDlgItemText(hDlg, IDC_MSGBID, BID, 13);

			if (strlen(HDest) == 0)
			{		
				MessageBox(NULL, "To: Call Missing!", "BPQMailChat", MB_ICONERROR);
				return TRUE;
			}

			if (strlen(BID))
			{		
				if (LookupBID(BID))
				{
					// Duplicate bid

					MessageBox(NULL, "Duplicate BID", "BPQMailChat", MB_ICONERROR);
					return TRUE;
				}
			}

			Msg = AllocateMsgRecord();
		
			// Set number here so they remain in sequence
		
			Msg->number = ++LatestMsg;
			Msg->length = MsgLen;
			MsgnotoMsg[Msg->number] = Msg;

			strcpy(Msg->from, SYSOPCall);
			Vptr = strlop(HDest, '@');
	
			if (Vptr)
			{
				if (strlen(Vptr) > 40)
					Vptr[40] = 0;

				strcpy(Msg->via, Vptr);
			}

			strcpy(Msg->to, HDest);

			GetDlgItemText(hDlg, IDC_MSGTITLE, Msg->title, 61);
			GetDlgItemText(hDlg, IDC_MSGTYPE, status, 2);
			Msg->type = status[0];
			Msg->status = 'N';

			if (strlen(BID) == 0)
				sprintf_s(BID, sizeof(BID), "%d_%s", LatestMsg, BBSName);

			strcpy(Msg->bid, BID);

			Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

			BIDRec = AllocateBIDRecord();

			strcpy(BIDRec->BID, Msg->bid);
			BIDRec->mode = Msg->type;
			BIDRec->u.msgno = LOWORD(Msg->number);
			BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

			sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes", MailDir, Msg->number);
	
			hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				WriteFile(hFile, MailBuffer, Msg->length, &WriteLen, NULL);
				CloseHandle(hFile);
			}

			free(MailBuffer);

			MatchMessagetoBBSList(Msg, 0);

			BuildNNTPList(Msg);				// Build NNTP Groups list

			EndDialog(hDlg, LOWORD(wParam));

			return TRUE;
		}

		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ChatMapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Position[81] = "";
	char PopupText[251] = "";
	char Msg[500];
	int len;
	int Click = 0, Hover = 0;
	char ClickHover[3] = "";
	HKEY hKey=0;
	int retCode,Type,Vallen;


	switch (message)
	{
	case WM_INITDIALOG:

		retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
			"SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, KEY_ALL_ACCESS, &hKey);

		if (retCode == ERROR_SUCCESS)
		{
			Vallen=80;
			RegQueryValueEx(hKey,"MapPosition", 0 , &Type,(UCHAR *)&Position, &Vallen);

			Vallen=250;
			RegQueryValueEx(hKey,"MapPopup", 0 , &Type,(UCHAR *)&PopupText, &Vallen);

			Vallen=4;
			RegQueryValueEx(hKey,"PopupMode", 0, &Type, (UCHAR *)&Click, &Vallen);
		
			RegCloseKey(hKey);
		}

		SetDlgItemText(hDlg, IDC_MAPPOSITION, Position);
		SetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText);

		SendDlgItemMessage(hDlg, IDC_CLICK, BM_SETCHECK, Click , Click);
		SendDlgItemMessage(hDlg, IDC_HOVER, BM_SETCHECK, !Click , !Click);

		return TRUE; 

	case WM_COMMAND:

		switch LOWORD(wParam)
		{
		case IDSENDTOMAP:

			GetDlgItemText(hDlg, IDC_MAPPOSITION, Position, 80);
			GetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText, 250);
		
			Click = SendDlgItemMessage(hDlg, IDC_CLICK, BM_GETCHECK, 0 ,0);
			Hover = SendDlgItemMessage(hDlg, IDC_HOVER, BM_GETCHECK, 0 ,0);
		
			if (AXIPPort)
			{
				if (Click) ClickHover[0] = '1';
				else 
					if (Hover) ClickHover[0] = '0';

				len = wsprintf(Msg, "INFO %s|%s|%s|\r", Position, PopupText, ClickHover);

				if (len < 256)
					Send_MON_Datagram(Msg, len);
			}
			
		break;

		case IDOK:

			GetDlgItemText(hDlg, IDC_MAPPOSITION, Position, 80);
			GetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText, 250);
		
			Click = SendDlgItemMessage(hDlg, IDC_CLICK, BM_GETCHECK, 0 ,0);
			Hover = SendDlgItemMessage(hDlg, IDC_HOVER, BM_GETCHECK, 0 ,0);

			retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
				"SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, KEY_ALL_ACCESS, &hKey);

			if (retCode == ERROR_SUCCESS)
			{
				RegSetValueEx(hKey, "MapPosition", 0 , REG_SZ, (UCHAR *)&Position, strlen(Position));
				RegSetValueEx(hKey, "MapPopup", 0, REG_SZ, (UCHAR *)&PopupText, strlen(PopupText));
				RegSetValueEx(hKey, "PopupMode", 0, REG_DWORD, (UCHAR *)&Click, 4);
		
				RegCloseKey(hKey);
			}

		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		
		case IDC_MAPHELP:
			
			ShellExecute(hDlg,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQChatMap.htm",
				"", NULL, SW_SHOWNORMAL); 

			return TRUE;
		}
	}
	return FALSE;
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
			return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

int RefreshMainWindow()
{
	char msg[80];
	CIRCUIT * conn;
	int i,n, SYSOPMsgs = 0, HeldMsgs = 0, SMTPMsgs = 0;
	time_t now;
	struct tm * tm;
	char tim[20];

	SendDlgItemMessage(MainWnd,100,LB_RESETCONTENT,0,0);

	for (n = 0; n < NumberofStreams; n++)
	{
		conn=&Connections[n];

		if (!conn->Active)
		{
			strcpy(msg,"Idle");
		}
		else
		{
			if (conn->Flags & CHATLINK)
			{
				i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
					"Chat Link", conn->u.link->alias, conn->BPQStream,
					"", conn->OutputQueueLength - conn->OutputGetPointer);
			}
			else
			if ((conn->Flags & CHATMODE)  && conn->topic)
			{
				i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
					conn->UserPointer->Name, conn->u.user->call, conn->BPQStream,
					conn->topic->topic->name, conn->OutputQueueLength - conn->OutputGetPointer);
			}
			else
			{
				if (conn->UserPointer == 0)
					strcpy(msg,"Logging in");
				else
				{
					i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
						conn->UserPointer->Name, conn->UserPointer->Call, conn->BPQStream,
						"BBS", conn->OutputQueueLength - conn->OutputGetPointer);
				}
			}
		}
		SendDlgItemMessage(MainWnd,100,LB_ADDSTRING,0,(LPARAM)msg);
	}

	SetDlgItemInt(hWnd, IDC_MSGS, NumberofMessages, FALSE);

	n = 0;

	for (i=1; i <= NumberofMessages; i++)
	{
		if (MsgHddrPtr[i]->status == 'N')
		{
			if (_stricmp(MsgHddrPtr[i]->to, SYSOPCall) == 0  || _stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0)
				SYSOPMsgs++;
			else
			if (MsgHddrPtr[i]->to[0] == 0)
				SMTPMsgs++;
		}
		else
		{
			if (MsgHddrPtr[i]->status == 'H')
				HeldMsgs++;
		}
	}

	SetDlgItemInt(hWnd, IDC_SYSOPMSGS, SYSOPMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_HELD, HeldMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_SMTP, SMTPMsgs, FALSE);

	SetDlgItemInt(hWnd, IDC_CHATSEM, ChatSemaphore.Clashes, FALSE);
	SetDlgItemInt(hWnd, IDC_MSGSEM, MsgNoSemaphore.Clashes, FALSE);
	SetDlgItemInt(hWnd, IDC_ALLOCSEM, AllocSemaphore.Clashes, FALSE);

	now = time(NULL);

	tm = gmtime(&now);	
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_UTC, tim);

	tm = localtime(&now);
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_LOCAL, tim);


	return 0;
}

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

SOCKADDR_IN local_sin;  /* Local socket - internet style */

PSOCKADDR_IN psin;

SOCKET sock;

BOOL Initialise()
{
	int i, ptr, len;
	ConnectionInfo * conn;
	struct UserInfo * user = NULL;
	HKEY hKey=0;
	char * ptr1;
	int Attrs, ret;
	char msg[MAX_PATH + 50];


	//	Register message for posting by BPQDLL

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	if (!GetConfigFromRegistry())
		return FALSE;

	wsprintf(SignoffMsg, "73 de %s\r", BBSName);

	// Make Sure BASEDIR Exists

	Attrs = GetFileAttributes(BaseDir);

	if (Attrs == -1)
	{
		sprintf_s(msg, sizeof(msg), "Base Directory %s not found - should it be created?", BaseDir);
		ret = MessageBox(NULL, msg, "BPQMailChat", MB_YESNO);

		if (ret == IDYES)
		{
			ret = CreateDirectory(BaseDir, NULL);
			if (ret == 0)
			{
				MessageBox(NULL, "Failed to created Base Directory - exiting", "BPQMailChat", MB_ICONSTOP);
				return FALSE;
			}
		}
		else
		{
			MessageBox(NULL, "Can't Continue without a Base Directory - exiting", "BPQMailChat", MB_ICONSTOP);
			return FALSE;
		}
	}
	else
	{
		if (!(Attrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			sprintf_s(msg, sizeof(msg), "Base Directory %s is a file not a directory - exiting", BaseDir);
			ret = MessageBox(NULL, msg, "BPQMailChat", MB_ICONSTOP);

			return FALSE;
		}
	}

	// Set up file and directory names
		
	strcpy(UserDatabasePath, BaseDir);
	strcat(UserDatabasePath, "\\");
	strcat(UserDatabasePath, UserDatabaseName);

	strcpy(MsgDatabasePath, BaseDir);
	strcat(MsgDatabasePath, "\\");
	strcat(MsgDatabasePath, MsgDatabaseName);

	strcpy(BIDDatabasePath, BaseDir);
	strcat(BIDDatabasePath, "\\");
	strcat(BIDDatabasePath, BIDDatabaseName);

	strcpy(WPDatabasePath, BaseDir);
	strcat(WPDatabasePath, "\\");
	strcat(WPDatabasePath, WPDatabaseName);

	strcpy(BadWordsPath, BaseDir);
	strcat(BadWordsPath, "\\");
	strcat(BadWordsPath, BadWordsName);

	strcpy(MailDir, BaseDir);
	strcat(MailDir, "\\");
	strcat(MailDir, "Mail");

	CreateDirectory(MailDir, NULL);		// Just in case

	strcpy(RtUsr, BaseDir);
	strcat(RtUsr, "\\ChatUsers.txt");

	strcpy(RtUsrTemp, BaseDir);
	strcat(RtUsrTemp, "\\ChatUsers.tmp");

	strcpy(RtKnown, BaseDir);
	strcat(RtKnown, "\\RTKnown.txt");

	BBSApplMask = 1<<(BBSApplNum-1);

	if (ChatApplNum)
	{
		ptr1=GetApplCall(ChatApplNum);

		if (*ptr1 > 0x20)
		{
			memcpy(OurNode, ptr1, 10);
			strlop(OurNode, ' ');

			ptr1=GetApplAlias(ChatApplNum);
			memcpy(OurAlias, ptr1,10);
			strlop(OurAlias, ' ');

			ChatApplMask = 1<<(ChatApplNum-1);
		
			// Set up other nodes list. rtlink messes with the string so pass copy
	
			ptr=0;

			while (OtherNodes[ptr])
			{
				len=strlen(&OtherNodes[ptr]);		
				rtlink(_strdup(&OtherNodes[ptr]));			
				ptr+= (len + 1);
			}

			SetupChat();
		}
	}

	// Make backup copies of Databases
	
	CopyBIDDatabase();
	CopyMessageDatabase();
	CopyUserDatabase();
	CopyWPDatabase();

	SetupMyHA();
	SetupFwdAliases();

	GetWPDatabase();
	GetMessageDatabase();
	GetUserDatabase();
	GetBIDDatabase();
	GetBadWordFile();

	// Make sure there is a user record for the BBS, with BBS bit set.

	user = LookupCall(BBSName);
		
	if (user == NULL)
		user = AllocateUserRecord(BBSName);

	if ((user->flags & F_BBS) == 0)
	{
		// Not Defined as a BBS

		if(SetupNewBBS(user))
			user->flags |= F_BBS;
	}

	// Allocate Streams

	for (i=0; i < MaxStreams; i++)
	{
		conn = &Connections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		NumberofStreams++;

		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, (i == 0 && EnableUI) ? 0x82 : 2, BBSApplMask | ChatApplMask);
		Disconnect(conn->BPQStream);
	}

	InitialiseTCP();

	InitialiseNNTP();

	if (EnableUI)
		SetupUIInterface();

	if (cfgMinToTray)
	{
		AddTrayMenuItem(MainWnd, "Mail/Chat Server");
	}
	
	SetTimer(hWnd,1,10000,NULL);	// Slow Timer (10 Secs)
	SetTimer(hWnd,2,100,NULL);		// Send to Node (100 ms)

	// Calulate time to run Housekeeping

	{
		struct tm *tm;
		time_t now;

		now = time(NULL);

		tm = gmtime(&now);

		tm->tm_hour = MaintTime / 100;
		tm->tm_min = MaintTime % 100;
		tm->tm_sec = 0;

		MaintClock = _mkgmtime(tm);

		if (MaintClock < now)
			MaintClock += 86400;

		if (LastFWDTime)
		{
			if ((MaintClock - LastFWDTime) > 86400)
				DoHouseKeeping(FALSE);
		}
	}

	CheckMenuItem(hMenu,IDM_LOGBBS, (LogBBS) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_LOGTCP, (LogTCP) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_LOGCHAT, (LogCHAT) ? MF_CHECKED : MF_UNCHECKED);

	RefreshMainWindow();

	return TRUE;
}

int Connected(Stream)
{
	int n, Mask;
	CIRCUIT * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, sesstype, paclen, maxframe, l4window;
	char ConnectedMsg[] = "*** CONNECTED    ";
	char Msg[100];
	char Title[100];

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (Stream == conn->BPQStream)
		{
			if (conn->Active)
			{
				// Probably an outgoing connect

				if (conn->BBSFlags & RunningConnectScript)
				{
					// BBS Outgoing Connect

					conn->paclen = 128;

					// Run first line of connect script

					ProcessBBSConnectScript(conn, ConnectedMsg, 15);
					return 0;
				}
		
				if (conn->rtcflags == p_linkini)
				{
					conn->paclen = 128;
					nprintf(conn, "c %s\r", conn->u.link->call);
					return 0;
				}
			}
	
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->Active = TRUE;
			conn->BPQStream = Stream;

			GetConnectionInfo(Stream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);

			strlop(callsign, ' ');		// Remove trailing spaces

			memcpy(conn->Callsign, callsign, 10);

			strlop(callsign, '-');		// Remove any SSID

			user = LookupCall(callsign);

			if (user == NULL)
			{
				int Length=0;
				char * MailBuffer = malloc(100);

				user = AllocateUserRecord(callsign);

				Length += wsprintf(MailBuffer, "New User %s Connected to Mailbox\r\n", callsign);

				wsprintf(Title, "New User %s", callsign);

				SendMessageToSYSOP(Title, MailBuffer, Length);

				if (user == NULL) return 0; //		Cant happen??

				user->flags |= F_HOLDMAIL;

				conn->NewUser = TRUE;

			}

			time(&user->TimeLastConnected);
			user->nbcon++;

			conn->UserPointer = user;

			conn->lastmsg = user->lastmsg;

			conn->PageLen = user->PageLen;
			conn->Paging = (user->PageLen > 0);

			conn->NextMessagetoForward = FirstMessageIndextoForward;

			if (paclen == 0)
				paclen = 236;
			
			conn->paclen=paclen;

			//	Set SYSOP flag if user is defined as SYSOP and Host Session 
			
			if (((sesstype & 0x20) == 0x20) && (user->flags & F_SYSOP))
				conn->sysop = TRUE;

			Mask = 1 << (GetApplNum(Stream) - 1);

			if (user->flags & F_Excluded)
			{
				n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s Rejected by Exclude Flag", user->Call);
				WriteLogLine(conn, '|',Msg, n, LOG_CHAT);
				Disconnect(Stream);
				return 0;
			}

			n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s", user->Call);
			
			// Send SID and Prompt

			if (Mask == ChatApplMask)
			{
				WriteLogLine(conn, '|',Msg, n, LOG_CHAT);
				conn->Flags |= CHATMODE;

				nodeprintf(conn, ChatSID, Ver[0], Ver[1], Ver[2], Ver[3]);
			}
			else
			{
				BOOL B1 = FALSE, B2 = FALSE;

				if(conn->UserPointer->ForwardingInfo)
				{
					B1 = conn->UserPointer->ForwardingInfo->AllowB1;
					B2 = conn->UserPointer->ForwardingInfo->AllowB2;
				}
				WriteLogLine(conn, '|',Msg, n, LOG_BBS);
				nodeprintf(conn, BBSSID, Ver[0], Ver[1], Ver[2], Ver[3],
					ALLOWCOMPRESSED ? "B" : "", B1 ? "1" : "", B2 ? "2" : "", "F");
			}

			if (user->Name[0] == 0)
			{
				conn->Flags |= GETTINGUSER;
				BBSputs(conn, NewUserPrompt);
			}
			else
				SendWelcomeMsg(Stream, conn, user);

			RefreshMainWindow();
			
			return 0;
		}
	}

	return 0;
}

int Disconnected (Stream)
{
	struct UserInfo * user = NULL;
	CIRCUIT * conn;
	int n;
	char Msg[255];
	int len;
	struct _EXCEPTION_POINTERS exinfo;

	for (n = 0; n <= NumberofStreams-1; n++)
	{
		conn=&Connections[n];

		if (Stream == conn->BPQStream)
		{
			if (conn->Active == FALSE)
				return 0;

			ClearQueue(conn);

			if (conn->PacLinkCalls)
				free(conn->PacLinkCalls);

			if (conn->InputBuffer)
			{
				free(conn->InputBuffer);
				conn->InputBuffer = NULL;
				conn->InputBufferLen = 0;
			}

			if (conn->InputMode == 'B')
			{
				// Save partly received message for a restart
						
				if (conn->BBSFlags & FBBB1Mode)
					if (conn->Paclink == 0)			// Paclink doesn't do restarts
						if (strcmp(conn->Callsign, "RMS") != 0)	// Neither does RMS Packet.
							SaveFBBBinary(conn);		
			}

			conn->Active = FALSE;
			RefreshMainWindow();

			if (conn->Flags & CHATMODE)
			{
				if (conn->Flags & CHATLINK)
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat Node %s Disconnected", conn->u.link->call);
					WriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try {link_drop(conn);} My__except_Routine("link_drop");
				}
				else
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat User %s Disconnected", conn->Callsign);
					WriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try {logout(conn);} My__except_Routine("logout");

				}

				return 0;
			}

			RemoveTempBIDS(conn);

			len=sprintf_s(Msg, sizeof(Msg), "%s Disconnected", conn->Callsign);
			WriteLogLine(conn, '|',Msg, len, LOG_BBS);

			if (conn->FBBHeaders)
			{
				free(conn->FBBHeaders);
				conn->FBBHeaders = NULL;
			}

			if (conn->UserPointer)
			{
				struct	BBSForwardingInfo * FWDInfo = conn->UserPointer->ForwardingInfo;

				if (FWDInfo)
				{
					FWDInfo->Forwarding = FALSE;

//					if (FWDInfo->UserCall[0])			// Will be set if RMS
//					{
//						FindNextRMSUser(FWDInfo);
//					}
//					else
						FWDInfo->FwdTimer = 0;
				}
			}
			return 0;
		}
	}

	return 0;
}

int DoReceivedData(int Stream)
{
	int count, InputLen;
	UINT MsgLen;
	int n;
	CIRCUIT * conn;
	struct UserInfo * user;
	char * ptr, * ptr2;
	char * Buffer;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];

		if (Stream == conn->BPQStream)
		{
			do
			{ 
				// May have several messages per packet, or message split over packets

				if (conn->InputLen + 1000 > conn->InputBufferLen )	// Shouldnt have lines longer  than this in text mode
				{
					conn->InputBufferLen += 1000;
					conn->InputBuffer = realloc(conn->InputBuffer, conn->InputBufferLen);
				}
				
				GetMsg(Stream, &conn->InputBuffer[conn->InputLen], &InputLen, &count);

				if (InputLen == 0) return 0;

				conn->InputLen += InputLen;

				if (conn->InputMode == 'B')
				{
					__try
					{
						UnpackFBBBinary(conn);
					}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{
						conn->InputBuffer[conn->InputLen] = 0;
						Debugprintf("MAILCHAT *** Program Error in UnpackFBBBinary");
						Disconnect(conn->BPQStream);
						conn->InputLen=0;
						CheckProgramErrors();
						return 0;
					}
				}
				else
				{

			loop:
				ptr = memchr(conn->InputBuffer, '\r', conn->InputLen);

				if (ptr)	//  CR in buffer
				{
					user = conn->UserPointer;
				
					ptr2 = &conn->InputBuffer[conn->InputLen];
					
					if (++ptr == ptr2)
					{
						// Usual Case - single meg in buffer

						__try
						{
							if (conn->rtcflags == p_linkini)		// Chat Connect
								ProcessConnecting(conn, conn->InputBuffer, conn->InputLen);
							else if (conn->BBSFlags & RunningConnectScript)
								ProcessBBSConnectScript(conn, conn->InputBuffer, conn->InputLen);
							else
								ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							conn->InputBuffer[conn->InputLen] = 0;
							Debugprintf("MAILCHAT *** Program Error Processing input %s ", conn->InputBuffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
							return 0;
						}
						conn->InputLen=0;
					}
					else
					{
						// buffer contains more that 1 message

						MsgLen = conn->InputLen - (ptr2-ptr);

						Buffer = malloc(MsgLen + 100);

						memcpy(Buffer, conn->InputBuffer, MsgLen);
						__try
						{
							if (conn->rtcflags == p_linkini)
								ProcessConnecting(conn, Buffer, MsgLen);
							else if (conn->BBSFlags & RunningConnectScript)
								ProcessBBSConnectScript(conn, Buffer, MsgLen);
							else
								ProcessLine(conn, user, Buffer, MsgLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							Buffer[MsgLen] = 0;
							Debugprintf("MAILCHAT *** Program Error Processing input %s ", Buffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
							return 0;
						}

						free(Buffer);

						if (*ptr == 0 || *ptr == '\n')
						{
							/// CR LF or CR Null

							ptr++;
							conn->InputLen--;
						}

						memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

						conn->InputLen -= MsgLen;

						goto loop;

					}
				}
				}
			} while (count > 0);

			return 0;
		}
	}

	// Socket not found

	return 0;

}
int DoMonitorData(int Stream)
{
//	UCHAR Buffer[1000];
	UCHAR buff[500];

	int len,count=0;
	int stamp;

			do { 

			stamp=GetRaw(Stream, buff,&len,&count);

			if (len == 0) return 0;

			SeeifBBSUIFrame((struct _MESSAGEX *)buff, len);
			
		}


		while (count > 0);	
 
	

	return 0;

}
int ConnectState(Stream)
{
	int state;

	SessionStateNoAck(Stream, &state);
	return state;
}
UCHAR * EncodeCall(UCHAR * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}


int WriteLog(char * msg)
{
	FILE *file;
	char timebuf[128];

	time_t ltime;
	struct tm today;
 
	if ((file = fopen("BPQTelnetServer.log","a")) == NULL)
		return FALSE;

	time(&ltime);

	_localtime32_s( &today, &ltime );

	strftime( timebuf, 128,
		"%d/%m/%Y %H:%M:%S ", &today );
    
	fputs(timebuf, file);

	fputs(msg, file);

	fclose(file);

	return 0;
}




struct UserInfo * AllocateUserRecord(char * Call)
{
	struct UserInfo * User = zalloc(sizeof (struct UserInfo));
		
	strcpy(User->Call, Call);

	GetSemaphore(&AllocSemaphore);

	UserRecPtr=realloc(UserRecPtr,(++NumberofUsers+1)*4);
	UserRecPtr[NumberofUsers]= User;

	FreeSemaphore(&AllocSemaphore);

	return User;
}

struct MsgInfo * AllocateMsgRecord()
{
	struct MsgInfo * Msg = zalloc(sizeof (struct MsgInfo));

	GetSemaphore(&AllocSemaphore);

	MsgHddrPtr=realloc(MsgHddrPtr,(++NumberofMessages+1)*4);
	MsgHddrPtr[NumberofMessages] = Msg;

	FreeSemaphore(&AllocSemaphore);

	return Msg;
}

BIDRec * AllocateBIDRecord()
{
	BIDRec * BID = zalloc(sizeof (BIDRec));
	
	GetSemaphore(&AllocSemaphore);

	BIDRecPtr=realloc(BIDRecPtr,(++NumberofBIDs+1)*4);
	BIDRecPtr[NumberofBIDs] = BID;

	FreeSemaphore(&AllocSemaphore);

	return BID;
}

BIDRec * AllocateTempBIDRecord()
{
	BIDRec * BID = zalloc(sizeof (BIDRec));
	
	GetSemaphore(&AllocSemaphore);

	TempBIDRecPtr=realloc(TempBIDRecPtr,(++NumberofTempBIDs+1)*4);
	TempBIDRecPtr[NumberofTempBIDs] = BID;

	FreeSemaphore(&AllocSemaphore);

	return BID;
}

struct UserInfo * LookupCall(char * Call)
{
	struct UserInfo * ptr = NULL;
	int i;

	for (i=1; i <= NumberofUsers; i++)
	{
		ptr = UserRecPtr[i];

		if (_stricmp(ptr->Call, Call) == 0) return ptr;

	}

	return NULL;
}

VOID GetUserDatabase()
{
	struct UserInfo UserRec;
	HANDLE Handle;
	int ReadLen;
	struct UserInfo * user;

	Handle = CreateFile(UserDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		UserRecPtr=malloc(4);
		UserRecPtr[0]= malloc(sizeof (struct UserInfo));
		memset(UserRecPtr[0], 0, sizeof (struct UserInfo));
		NumberofUsers = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &UserRec, sizeof (UserRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&UserRec, 0, sizeof (struct UserInfo));
	}

	// Set up control record

	UserRecPtr=malloc(4);
	UserRecPtr[0]= malloc(sizeof (struct UserInfo));
	memcpy(UserRecPtr[0], &UserRec,  sizeof (UserRec));

	NumberofUsers = 0;

Next:

	ReadFile(Handle, &UserRec, sizeof (UserRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		user = AllocateUserRecord(UserRec.Call);
		memcpy(user, &UserRec,  sizeof (UserRec));

		user->ForwardingInfo = NULL;	// In case left behind on crash
		user->BBSNext = NULL;
		user->POP3Locked = FALSE;


		if (user->flags & F_BBS)
		{
			// Defined as BBS - allocate and initialise forwarding structure

			SetupForwardingStruct(user);

			// Add to BBS Chain;

			user->BBSNext = BBSChain;
			BBSChain = user;

			// Save Highest BBS Number

			if (user->BBSNumber > HighestBBSNumber) HighestBBSNumber = user->BBSNumber;

		}
		goto Next;
	}

	SortBBSChain();


	CloseHandle(Handle);	
}

VOID CopyUserDatabase()
{
	char Backup[MAX_PATH];

	strcpy(Backup, UserDatabasePath);
	strcat(Backup, ".bak");

	CopyFile(UserDatabasePath, Backup, FALSE);
}

VOID SaveUserDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;

	Handle = CreateFile(UserDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	UserRecPtr[0]->nbcon = NumberofUsers;

	for (i=0; i <= NumberofUsers; i++)
	{
		WriteFile(Handle, UserRecPtr[i], sizeof (struct UserInfo), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

VOID GetMessageDatabase()
{
	struct MsgInfo MsgRec;
	HANDLE Handle;
	int ReadLen;
	struct MsgInfo * Msg;
	char * MsgBytes;
	int Resize = 0;				// Used to resize file if format changes

	Handle = CreateFile(MsgDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		MsgHddrPtr=malloc(4);
		MsgHddrPtr[0]= zalloc(sizeof (MsgRec));
		NumberofMessages = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &MsgRec, sizeof (MsgRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&MsgRec, 0, sizeof (MsgRec));
	}

	// Set up control record

	MsgHddrPtr=malloc(4);
	MsgHddrPtr[0]= malloc(sizeof (MsgRec));
	memcpy(MsgHddrPtr[0], &MsgRec,  sizeof (MsgRec));

	LatestMsg=MsgHddrPtr[0]->length;

	NumberofMessages = 0;

	if (MsgRec.status == 0)		// Used a file format version 0 = original, 1 = Extra email from addr.
	{
		Resize = 41;
		MsgHddrPtr[0]->status = 1;
		SetFilePointer (Handle, -41, NULL, FILE_CURRENT);
		memset(MsgRec.emailfrom, 0, 41);
	}

Next: 

	ReadFile(Handle, &MsgRec, sizeof (MsgRec) - Resize, &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		// Validate Header

		if (MsgRec.type == 0)
			goto Next;

		MsgBytes = ReadMessageFile(MsgRec.number);

		if (MsgBytes)
		{
			MsgRec.length = strlen(MsgBytes);
			free(MsgBytes);
		}
		else
			goto Next;


		Msg = AllocateMsgRecord();
		memcpy(Msg, &MsgRec,  sizeof (MsgRec));

		MsgnotoMsg[Msg->number] = Msg;

		BuildNNTPList(Msg);				// Build NNTP Groups list

		// If any forward bits are set, increment count on corresponding BBS record.

		if (memcmp(MsgRec.fbbs, zeros, NBMASK) != 0)
		{
			if (FirstMessageIndextoForward == 0)
				FirstMessageIndextoForward = NumberofMessages;			// limit search
		}

		goto Next;
	}

	if (FirstMessageIndextoForward == 0)
		FirstMessageIndextoForward = NumberofMessages;			// limit search


	CloseHandle(Handle);

}

VOID CopyMessageDatabase()
{
	char Backup1[MAX_PATH];
	char Backup2[MAX_PATH];

	// Keep 4 Generations

	strcpy(Backup2, MsgDatabasePath);
	strcat(Backup2, ".bak.3");

	strcpy(Backup1, MsgDatabasePath);
	strcat(Backup1, ".bak.2");

	DeleteFile(Backup2);			// Remove old .bak.3
	MoveFile(Backup1, Backup2);		// Move .bak.2 to .bak.3

	strcpy(Backup2, MsgDatabasePath);
	strcat(Backup2, ".bak.1");

	MoveFile(Backup2, Backup1);		// Move .bak.1 to .bak.2

	strcpy(Backup1, MsgDatabasePath);
	strcat(Backup1, ".bak");

	MoveFile(Backup1, Backup2);		//Move .bak to .bak.1

	strcpy(Backup2, MsgDatabasePath);
	strcat(Backup2, ".bak");

	CopyFile(MsgDatabasePath, Backup2, FALSE);	// Copy to .bak

}

VOID SaveMessageDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;

	Handle = CreateFile(MsgDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	MsgHddrPtr[0]->number = NumberofMessages;
	MsgHddrPtr[0]->length = LatestMsg;

	for (i=0; i <= NumberofMessages; i++)
	{
		WriteFile(Handle, MsgHddrPtr[i], sizeof (struct MsgInfo), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

VOID GetBIDDatabase()
{
	BIDRec BIDRec;
	HANDLE Handle;
	int ReadLen;
	BIDRecP BID;

	Handle = CreateFile(BIDDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		BIDRecPtr=malloc(4);
		BIDRecPtr[0]= malloc(sizeof (BIDRec));
		memset(BIDRecPtr[0], 0, sizeof (BIDRec));
		NumberofBIDs = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &BIDRec, sizeof (BIDRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&BIDRec, 0, sizeof (BIDRec));
	}

	// Set up control record

	BIDRecPtr=malloc(4);
	BIDRecPtr[0]= malloc(sizeof (BIDRec));
	memcpy(BIDRecPtr[0], &BIDRec,  sizeof (BIDRec));

	NumberofBIDs = 0;

Next:

	ReadFile(Handle, &BIDRec, sizeof (BIDRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		BID = AllocateBIDRecord();
		memcpy(BID, &BIDRec,  sizeof (BIDRec));

		if (BID->u.timestamp == 0) 	
			BID->u.timestamp = LOWORD(time(NULL)/86400);

		goto Next;
	}

	CloseHandle(Handle);
}

VOID CopyBIDDatabase()
{
	char Backup[MAX_PATH];

	strcpy(Backup, BIDDatabasePath);
	strcat(Backup, ".bak");

	CopyFile(BIDDatabasePath, Backup, FALSE);
}

VOID SaveBIDDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;

	Handle = CreateFile(BIDDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	BIDRecPtr[0]->u.msgno = NumberofBIDs;			// First Record has file size

	for (i=0; i <= NumberofBIDs; i++)
	{
		WriteFile(Handle, BIDRecPtr[i], sizeof (BIDRec), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

BIDRec * LookupBID(char * BID)
{
	BIDRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofBIDs; i++)
	{
		ptr = BIDRecPtr[i];

		if (_stricmp(ptr->BID, BID) == 0)
			return ptr;
	}

	return NULL;
}

BIDRec * LookupTempBID(char * BID)
{
	BIDRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofTempBIDs; i++)
	{
		ptr = TempBIDRecPtr[i];

		if (_stricmp(ptr->BID, BID) == 0) return ptr;
	}

	return NULL;
}

VOID RemoveTempBIDS(CIRCUIT * conn)
{
	// Remove any Temp BID records for conn. Called when connection closes - Msgs will be complete or failed
	
	if (NumberofTempBIDs == 0)
		return;
	else
	{
		BIDRec * ptr = NULL;
		BIDRec ** NewTempBIDRecPtr = zalloc((NumberofTempBIDs+1) * 4);
		int i = 0, n;

		for (n = 1; n <= NumberofTempBIDs; n++)
		{
			ptr = TempBIDRecPtr[n];

			if (ptr->u.conn == conn)
				// Remove this entry 
				free(ptr);
			else
				NewTempBIDRecPtr[++i] = ptr;
		}

		NumberofTempBIDs = i;

		free(TempBIDRecPtr);

		TempBIDRecPtr = NewTempBIDRecPtr;
	}
}

VOID GetBadWordFile()
{
	HANDLE Handle;
	int ReadLen;
	DWORD FileSize;
	char * ptr1, * ptr2;

	Handle = CreateFile(BadWordsPath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	FileSize = GetFileSize(Handle, NULL);

	BadFile = malloc(FileSize+1);

	ReadFile(Handle, BadFile, FileSize, &ReadLen, NULL); 

	CloseHandle(Handle);

	BadFile[FileSize]=0;

	_strlwr(BadFile);								// Compares are case-insensitive

	ptr1 = BadFile;

	while (ptr1)
	{
		if (*ptr1 == '\n') ptr1++;

		ptr2 = strtok_s(NULL, "\r\n", &ptr1);
		if (ptr2)
		{
			if (*ptr2 != '#')
			{
				BadWords = realloc(BadWords,(++NumberofBadWords+1)*4);
				BadWords[NumberofBadWords] = ptr2;
			}
		}
		else
			break;
	}
}

BOOL CheckBadWord(char * Word, char * Msg)
{
	char * ptr1 = Msg, * ptr2;
	int len = strlen(Word);

	while (*ptr1)					// Stop at end
	{
		ptr2 = strstr(ptr1, Word);

		if (ptr2 == NULL)
			return FALSE;				// OK

		// Only bad if it ia not part of a longer word

		if ((ptr2 == Msg) || !(isalpha(*(ptr2 - 1))))	// No alpha before
			if (!(isalpha(*(ptr2 + len))))			// No alpha after
				return TRUE;					// Bad word
	
		// Keep searching

		ptr1 = ptr2 + len;
	}

	return FALSE;					// OK
}

BOOL CheckBadWords(char * Msg)
{
	char * dupMsg = _strlwr(_strdup(Msg));
	int i;

	for (i = 1; i <= NumberofBadWords; i++)
	{
		if (CheckBadWord(BadWords[i], dupMsg))
		{
			free(dupMsg);
			return TRUE;			// Bad
		}
	}

	free(dupMsg);
	return FALSE;					// OK

}

VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user)
{
	LINK    *link;
	KNOWNNODE *node;
	char Welcome[] = "Hello $I. Latest Message is $L, Last listed is $Z$W";


	if (conn->Flags & CHATMODE)
	{	
		// See if from a defined node
		
		for (link = link_hd; link; link = link->next)
		{
			if (matchi(conn->Callsign, link->call))
			{
				conn->rtcflags = p_linkwait;
				return;						// Wait for *RTL
			}
		}

		// See if from a previously known node

		node = knownnode_find(conn->Callsign);

		if (node)
		{
			// A node is trying to link, but we don't have it defined - close

			Logprintf(LOG_CHAT, conn, '!', "Node %s connected, but is not defined as a Node - closing",
				conn->Callsign);

			nodeprintf(conn, "Node %s does not have %s defined as a node to link to - closing.\r",
				OurNode, conn->Callsign);

			Flush(conn);

			Sleep(500);

			Disconnect(conn->BPQStream);

			return;
		}

		// Not a defined or known node - pretty safe to assume it's a user

		if (!rtloginu (conn, TRUE))
		{
			// Already connected - close
			
			Flush(conn);
			Sleep(1000);
			Disconnect(conn->BPQStream);
		}
		return;
	}

	if (conn->NewUser)
		ExpandAndSendMessage(conn, NewWelcomeMsg, LOG_BBS);
	else
		ExpandAndSendMessage(conn, WelcomeMsg, LOG_BBS);

	if (user->HomeBBS[0] == 0)
		BBSputs(conn, "Please enter your Home BBS using the Home command.\rYou may also enter your QTH and ZIP/Postcode using qth and zip commands.\r");

	nodeprintf(conn, "Type H for help.\rde %s>\r", BBSName);
}

VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user)
{
	nodeprintf(conn, "de %s>\r", BBSName);
}

VOID ProcessLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	char * Cmd, * Arg1;
	char * Context;
	char seps[] = " \t\r";
	int CmdLen;
	struct _EXCEPTION_POINTERS exinfo;

	if (conn->Flags & CHATMODE)
	{
		GetSemaphore(&ChatSemaphore);

		__try 
		{
			ProcessChatLine(conn, user, Buffer, len);
		}
		My__except_RoutineWithDisconnect("ProcessChatLine");
		
		FreeSemaphore(&ChatSemaphore);
		return;
	}

	WriteLogLine(conn, '<',Buffer, len-1, LOG_BBS);


	if (conn->BBSFlags & FBBForwarding)
	{
		__try 
		{
			ProcessFBBLine(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessFBBLine");
			Disconnect(conn->BPQStream);
			CheckProgramErrors();
		}
		return;
	}

	if (conn->Flags & GETTINGMESSAGE)
	{
		__try 
		{
			ProcessMsgLine(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessMSGLine");
			Disconnect(conn->BPQStream);
			CheckProgramErrors();
		}
		return;	}

	if (conn->Flags & GETTINGTITLE)
	{
		__try 
		{
			ProcessMsgTitle(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessMsgTitle");
			Disconnect(conn->BPQStream);
			CheckProgramErrors();
		}
		return;
	}

	if (conn->BBSFlags & MBLFORWARDING)
	{
		__try 
		{
			ProcessMBLLine(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessMBLLine");
			Disconnect(conn->BPQStream);
			CheckProgramErrors();
		}
		return;
	}
	if (conn->Flags & GETTINGUSER)
	{
		memcpy(user->Name, Buffer, len-1);
		conn->Flags &=  ~GETTINGUSER;
		SendWelcomeMsg(conn->BPQStream, conn, user);
		UpdateWPWithUserInfo(user);
		SaveUserDatabase();

		return;
	}


	// Process Command

	if (conn->Paging && (conn->LinesSent >= conn->PageLen))
	{
		// Waiting for paging prompt

		if (len > 1)
		{
			if (_memicmp(Buffer, "Abort", 1) == 0)
			{
				ClearQueue(conn);
				conn->LinesSent = 0;

				QueueMsg(conn, AbortedMsg, strlen(AbortedMsg));
				SendPrompt(conn, user);
				return;
			}
		}

		conn->LinesSent = 0;
		return;
	}

	if (len == 1)
	{
		SendPrompt(conn, user);
		return;
	}

	Buffer[len] = 0;

	if (memcmp(Buffer, ";FW:", 4) == 0)
	{
		// Paclink User Select (poll for list)
		
		char * ptr1,* ptr2, * ptr3;
		int index=0;

		// Convert string to Multistring

		Buffer[len-1] = 0;

		conn->PacLinkCalls = zalloc(len*3);

		ptr1 = &Buffer[5];
		ptr2 = (char *)conn->PacLinkCalls;
		ptr2 += (len * 2);
		strcpy(ptr2, ptr1);

		while (ptr2)
		{
			ptr3 = strlop(ptr2, ' ');

			if (strlen(ptr2))
				conn->PacLinkCalls[index++] = ptr2;
		
			ptr2 = ptr3;
		}

		if (!conn->RMSExpress)
			conn->Paclink = TRUE;

		
		return;	
	}

	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		// If a BBS, set BBS Flag

		if (user->flags & F_BBS)
		{
			if (user->ForwardingInfo)
			{
				if (user->ForwardingInfo->Forwarding)
				{
					BBSputs(conn, "Already Connected\r");
					Flush(conn);
					Sleep(500);
					Disconnect(conn->BPQStream);
					return;
				}

			}

			if (user->ForwardingInfo)
			{
				user->ForwardingInfo->Forwarding = TRUE;
				user->ForwardingInfo->FwdTimer = 0;			// So we dont send to immediately
			}

			Parse_SID(conn, &Buffer[1], len-4);
			
			if (conn->BBSFlags & FBBForwarding)
			{
				conn->FBBIndex = 0;		// ready for first block;
				conn->FBBChecksum = 0;
				memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
			}
			else
				BBSputs(conn, ">\r");

		}
		
		return;
	}

	Cmd = strtok_s(Buffer, seps, &Context);

	if (Cmd == NULL)
	{
		BBSputs(conn, "Invalid Command\r");
		SendPrompt(conn, user);
		return;
	}

	Arg1 = strtok_s(NULL, seps, &Context);
	CmdLen = strlen(Cmd);

	if (_memicmp(Cmd, "Abort", 1) == 0)
	{
		ClearQueue(conn);
		conn->LinesSent = 0;

		QueueMsg(conn, AbortedMsg, strlen(AbortedMsg));
		SendPrompt(conn, user);
		return;
	}
	if (_memicmp(Cmd, "Bye", CmdLen) == 0)
	{
		SendUnbuffered(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
		user->lastmsg = conn->lastmsg;
		Sleep(1000);

		if (conn->BPQStream > 0)
			Disconnect(conn->BPQStream);
		else
			CloseConsole(conn->BPQStream);

		return;
	}

	if (_memicmp(Cmd, "K", 1) == 0)
	{
		DoKillCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "L", 1) == 0)
	{
		DoListCommand(conn, user, Cmd, Arg1);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "Name", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 17)
				Arg1[17] = 0;

			strcpy(user->Name, Arg1);
			UpdateWPWithUserInfo(user);

		}

		SendWelcomeMsg(conn->BPQStream, conn, user);
		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "OP", 2) == 0)
	{
		int Lines;
		
		// Paging Control. Param is number of lines per page

		if (Arg1)
		{
			Lines = atoi(Arg1);
			
			if (Lines)				// Sanity Check
			{
				if (Lines < 10)
				{
					nodeprintf(conn,"Page Length %d is too short\r", Lines);
					SendPrompt(conn, user);
					return;
				}
			}

			user->PageLen = Lines;
			conn->PageLen = Lines;
			conn->Paging = (Lines > 0);
			SaveUserDatabase();
		}
		
		nodeprintf(conn,"Page Length is %d\r", user->PageLen);
		SendPrompt(conn, user);

		return;
	}

	if (_memicmp(Cmd, "QTH", CmdLen) == 0)
	{
		if (Arg1)
		{
			// QTH may contain spaces, so put back together, and just split at cr
			
			Arg1[strlen(Arg1)] = ' ';
			strtok_s(Arg1, "\r", &Context);

			if (strlen(Arg1) > 60)
				Arg1[60] = 0;

			strcpy(user->Address, Arg1);
			UpdateWPWithUserInfo(user);

		}

		nodeprintf(conn,"QTH is %s\r", user->Address);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "ZIP", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 8)
				Arg1[8] = 0;

			strcpy(user->ZIP, _strupr(Arg1));
			UpdateWPWithUserInfo(user);
		}

		nodeprintf(conn,"ZIP is %s\r", user->ZIP);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "R", 1) == 0)
	{
		DoReadCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "S", 1) == 0)
	{
		if (!DoSendCommand(conn, user, Cmd, Arg1, Context))
			SendPrompt(conn, user);
		return;
	}

	if ((_memicmp(Cmd, "Help", CmdLen) == 0) || (_memicmp(Cmd, "?", 1) == 0))
	{
		BBSputs(conn, "A - Abort Output\r");
		BBSputs(conn, "B - Logoff\r");
		BBSputs(conn, "K - Kill Message(s) - K num, KM (Kill my read messages)\r");
		BBSputs(conn, "L - List Message(s) - L = List New, LR = List New (Oldest first)\r");
		BBSputs(conn, "                      LM = List Mine L> Call, L< Call = List to or from\r");
		BBSputs(conn, "                      LL num = List Last, L num-num = List Range\r");
		BBSputs(conn, "                      LN LY LH LK LF L$ - List Mesaage with corresponding Status\r");
		BBSputs(conn, "                      LB LP - List Mesaage with corresponding Type\r");
		BBSputs(conn, "N Name - Set Name\r");
		BBSputs(conn, "OP n - Set Page Length (Output will pause every n lines)\r");
		BBSputs(conn, "Q QTH - Set QTH\r");
		BBSputs(conn, "R - Read Message(s) - R num, RM (Read new messages to me)\r");
		BBSputs(conn, "S - Send Message - S or SP Send Personal, SB Send Bull, SR Num - Send Reply, SC Num - Send Copy\r");

		SendPrompt(conn, user);

		return;

	}

	if (_memicmp(Cmd, "Ver", CmdLen) == 0)
	{
		if (GetVersionStringptr)
			nodeprintf(conn, "BBS Version %s\rNode Version %s\r", VersionStringWithBuild, GetVersionStringptr());
		else
			nodeprintf(conn, "BBS Version %s\r", VersionStringWithBuild);

		SendPrompt(conn, user);

		return;
	}

	if (_memicmp(Cmd, "HOMEBBS", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 40) Arg1[40] = 0;

			strcpy(user->HomeBBS, _strupr(Arg1));
			UpdateWPWithUserInfo(user);
	
			if (!strchr(Arg1, '.'))
				BBSputs(conn, "Please enter HA with HomeBBS eg g8bpq.gbr.eu - this will help message routing\r");
		}

		nodeprintf(conn,"HomeBBS is %s\r", user->HomeBBS);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}


	if (_memicmp(Cmd, "Chat", 4) == 0)
	{
		if(ChatApplMask == 0)
		{
			BBSputs(conn, "Chat Node is disabled\r");
			SendPrompt(conn, user);
			return;
		}

		if (rtloginu (conn, TRUE))
			conn->Flags |= CHATMODE;
		else
			SendPrompt(conn, user);

		return;
	}

/*	if (_memicmp(Cmd, "POLLRMS", 6) == 0)
	{
		struct UserInfo * RMS;

		RMS = FindRMS();

		if (!RMS)
		{
			BBSputs(conn, "Forwarding via RMS is not configured\r");
			SendPrompt(conn, user);
			return;
		}
		else
		{
			if (RMS->ForwardingInfo->Forwarding)
			{
				BBSputs(conn, "RMS is busy - try again in a minute or so\r");
				SendPrompt(conn, user);
				return;
			}

			strcpy(RMS->ForwardingInfo->UserCall, user->Call);
			RMS->ForwardingInfo->UserIndex = -1;		// Not part of user scan

			if (ConnecttoBBS(RMS))
			{
				RMS->ForwardingInfo->Forwarding = TRUE;
				BBSputs(conn, "RMS poll initiated\r");
			}
			else
				BBSputs(conn, "RMS poll failed\r");

			SendPrompt(conn, user);
			return;
		}

		return;
	}
*/
	if (conn->Flags == 0)
	{
		BBSputs(conn, "Invalid Command\r");
		SendPrompt(conn, user);
	}

	//	Send if possible

	Flush(conn);
}


VOID SendUnbuffered(int stream, char * msg, int len)
{
	if (stream < 0)
		WritetoConsoleWindow(stream, msg, len);
	else
		SendMsg(stream, msg, len);
}


int QueueMsg(ConnectionInfo * conn, char * msg, int len)
{
	// Add Message to queue for this connection

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	// Create or extend buffer

	conn->OutputQueue=realloc(conn->OutputQueue, conn->OutputQueueLength + len);

	if (conn->OutputQueue == NULL)
	{
		// relloc failed - should never happen, but clean up

		CriticalErrorHandler("realloc failed to expand output queue");
		return 0;
	}

	memcpy(&conn->OutputQueue[conn->OutputQueueLength], msg, len);

	conn->OutputQueueLength+=len;

	return len;
}

void TrytoSend()
{
	// call Flush on any connected streams with queued data

	ConnectionInfo * conn;
	struct ConsoleInfo * Cons;

	int n;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active == TRUE)
			Flush(conn);
	}

	for (Cons = ConsHeader[0]; Cons; Cons = Cons->next)
	{
		if (Cons->Console)
			Flush(Cons->Console);
	}
}


void Flush(CIRCUIT * conn)
{
	int tosend, len, sent;
	
	// Try to send data to user. May be stopped by user paging or node flow control

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	//	BOOL Paging;				// Set if user wants paging
	//	int LinesSent;				// Count when paging
	//	int PageLen;				// Lines per page


	if (conn->OutputQueue == NULL)
	{
		// Nothing to send. If Close after Flush is set, disconnect

		if (conn->CloseAfterFlush)
		{
			conn->CloseAfterFlush--;
			
			if (conn->CloseAfterFlush)
				return;

			Disconnect(conn->BPQStream);
		}

		return;						// Nothing to send
	}
	tosend = conn->OutputQueueLength - conn->OutputGetPointer;

	sent=0;

	while (tosend > 0)
	{
		if (TXCount(conn->BPQStream) > 4)
			return;						// Busy

		if (conn->Paging && (conn->LinesSent >= conn->PageLen))
			return;

		if (tosend <= conn->paclen)
			len=tosend;
		else
			len=conn->paclen;

		if (conn->Paging)
		{
			// look for CR chars in message to send. Increment LinesSent, and stop if at limit

			char * ptr1 = &conn->OutputQueue[conn->OutputGetPointer];
			char * ptr2;
			int lenleft = len;

			ptr2 = memchr(ptr1, 0x0d, len);

			while (ptr2)
			{
				conn->LinesSent++;
				ptr2++;
				lenleft = len - (ptr2 - ptr1);

				if (conn->LinesSent >= conn->PageLen)
				{
					len = ptr2 - &conn->OutputQueue[conn->OutputGetPointer];
					
					SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);
					conn->OutputGetPointer+=len;
					tosend-=len;
					SendUnbuffered(conn->BPQStream, "<A>bort, <CR> Continue..>", 25);

					return;

				}
				ptr2 = memchr(ptr2, 0x0d, lenleft);
			}
		}

		SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);

		conn->OutputGetPointer+=len;

		tosend-=len;

		
	
		sent++;

		if (sent > 4)
			return;

	}

	// All Sent. Free buffers and reset pointers

	conn->LinesSent = 0;

	ClearQueue(conn);
}

VOID ClearQueue(ConnectionInfo * conn)
{
	if (conn->OutputQueue == NULL)
		return;

	free(conn->OutputQueue);

	conn->OutputQueue=NULL;
	conn->OutputGetPointer=0;
	conn->OutputQueueLength=0;
}



VOID FlagAsKilled(struct MsgInfo * Msg)
{
	struct UserInfo * user;

	Msg->status='K';
	Msg->datechanged=time(NULL);

	// Remove any forwarding references

	if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
	{	
		for (user = BBSChain; user; user = user->BBSNext)
		{
			if (check_fwd_bit(Msg->fbbs, user->BBSNumber))
			{
				user->ForwardingInfo->MsgCount--;
				clear_fwd_bit(Msg->fbbs, user->BBSNumber);
			}
		}
	}
}



void DoKillCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;
	
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just K

		while (Arg1)
		{
			msgno = atoi(Arg1);
			KillMessage(conn, user, msgno);

			Arg1 = strtok_s(NULL, " \r", &Context);
		}

		return;

	case 'M':					// Kill Mine

		for (i=NumberofMessages; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if ((_stricmp(Msg->to, user->Call) == 0) || (conn->sysop && _stricmp(Msg->to, "SYSOP") == 0 && user->flags & F_SYSOP_IN_LM))
			{
				if (Msg->type == 'P' && Msg->status == 'Y')
				{
					FlagAsKilled(Msg);
					nodeprintf(conn, "Message #%d Killed\r", Msg->number);
				}
			}
		}
		return;

	case 'H':					// Kill Held

		if (conn->sysop)
		{
			for (i=NumberofMessages; i>0; i--)
			{
				Msg = MsgHddrPtr[i];

				if (Msg->status == 'H')
				{
					FlagAsKilled(Msg);
					nodeprintf(conn, "Message #%d Killed\r", Msg->number);
				}
			}
		}
		return;

	case '>':			// K> - Kill to 

		if (conn->sysop)
		{
			if (Arg1)
				if (KillMessagesTo(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
			return;
		}

	case '<':

		if (conn->sysop)
		{
			if (Arg1)
				if (KillMessagesFrom(conn, user, Arg1) == 0);
					BBSputs(conn, "No Messages found\r");

					return;
		}
	}

	nodeprintf(conn, "*** Error: Invalid Kill option %c\r", Cmd[1]);

	return;

}

int KillMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;
	struct MsgInfo * Msg;

	for (i=NumberofMessages; i>0; i--)
	{
		Msg = MsgHddrPtr[i];
		if (Msg->status != 'K' && _stricmp(Msg->to, Call) == 0)
		{
			Msgs++;
			KillMessage(conn, user, MsgHddrPtr[i]->number);
		}
	}
	
	return(Msgs);
}

int KillMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;
	struct MsgInfo * Msg;


	for (i=NumberofMessages; i>0; i--)
	{
		Msg = MsgHddrPtr[i];
		if (Msg->status != 'K' && _stricmp(Msg->from, Call) == 0)
		{
			Msgs++;
			KillMessage(conn, user, MsgHddrPtr[i]->number);
		}
	}
	
	return(Msgs);
}



void KillMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;

	Msg = FindMessage(user->Call, msgno, conn->sysop);

	if (Msg == NULL)
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	FlagAsKilled(Msg);

	nodeprintf(conn, "Message #%d Killed\r", msgno);

}


VOID ListMessage(struct MsgInfo * Msg, ConnectionInfo * conn)
{
	char FullFrom[80];
	char FullTo[80];


	strcpy(FullFrom, Msg->from);
	strcat(FullFrom, Msg->emailfrom);

	if (_stricmp(Msg->to, "RMS") == 0)
	{
		wsprintf(FullTo, "RMS:%s", Msg->via);
		nodeprintf(conn, "%-6d %s %c%c   %5d %-7s %-6s %-s\r",
				Msg->number, FormatDateAndTime(Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, FullTo, FullFrom, Msg->title);
	}
	else

	if (Msg->to[0] == 0 && Msg->via[0] != 0)
	{
		wsprintf(FullTo, "smtp:%s", Msg->via);
		nodeprintf(conn, "%-6d %s %c%c   %5d %-7s %-6s %-s\r",
				Msg->number, FormatDateAndTime(Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, FullTo, FullFrom, Msg->title);
	}

	else
		if (Msg->via[0] != 0)
			nodeprintf(conn, "%-6d %s %c%c   %5d %-7s@%-6s %-6s %-s\r",
				Msg->number, FormatDateAndTime(Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, Msg->via, FullFrom, Msg->title);
	else
		nodeprintf(conn, "%-6d %s %c%c   %5d %-7s        %-6s %-s\r",
				Msg->number, FormatDateAndTime(Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, FullFrom, Msg->title);

}

void DoListCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1)
{
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just L
	case 'R':			// LR = List Reverse

		if (Arg1)
		{
			// Range nnn-nnn or single value

			char * Arg2, * Arg3;
			char * Context;
			char seps[] = " -\t\r";
			int From=LatestMsg, To=0;
			char * Range = strchr(Arg1, '-');
			
			Arg2 = strtok_s(Arg1, seps, &Context);
			Arg3 = strtok_s(NULL, seps, &Context);

			if (Arg2)
				To = From = atoi(Arg2);

			if (Arg3)
				From = atoi(Arg3);
			else
				if (Range)
					From = LatestMsg;


			if (Cmd[1])
				ListMessagesInRangeForwards(conn, user, user->Call, From, To);
			else
				ListMessagesInRange(conn, user, user->Call, From, To);

		}
		else

			if (Cmd[1])
				ListMessagesInRangeForwards(conn, user, user->Call, LatestMsg, conn->lastmsg);
			else
				ListMessagesInRange(conn, user, user->Call, LatestMsg, conn->lastmsg);

			conn->lastmsg = LatestMsg;

		return;


	case 'L':				// List Last

		if (Arg1)
		{
			int i = atoi(Arg1);
			int m = NumberofMessages;
				
			for (; i>0 && m != 0; i--)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					ListMessage(MsgHddrPtr[m], conn);
					m--;
				}
			}
		}
		return;

	case 'M':			// LM - List Mine

		if (ListMessagesTo(conn, user, user->Call) == 0)
			BBSputs(conn, "No Messages found\r");
		return;

	case '>':			// L> - List to 

		if (Arg1)
			if (ListMessagesTo(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
		
		return;

	case '<':

		if (Arg1)
			if (ListMessagesFrom(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
		return;

	case '@':

		if (Arg1)
			if (ListMessagesAT(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
		return;

	case 'H':				// List Status
	case 'N':
	case 'Y':
	case 'F':
	case '%':
		{
			int m = NumberofMessages;
				
			while (m > 0)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					if (MsgHddrPtr[m]->status == toupper(Cmd[1]))
						ListMessage(MsgHddrPtr[m], conn);
					m--;
				}
			}
		}
		return;

	case 'K':
		{
			int i, Msgs = 0;

			for (i=NumberofMessages; i>0; i--)
			{
				if (MsgHddrPtr[i]->status == 'K')
				{
					Msgs++;
					ListMessage(MsgHddrPtr[i], conn);
				}
			}

			if (Msgs == 0)
				BBSputs(conn, "No Messages found\r");

		}

	case 'P':
	case 'B':
	case 'T':
		{
			int m = NumberofMessages;
				
			while (m > 0)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					if (MsgHddrPtr[m]->type == toupper(Cmd[1]))
						ListMessage(MsgHddrPtr[m], conn);
					m--;
				}
			}

			return;
		}
		}
	
	nodeprintf(conn, "*** Error: Invalid List option %c\r", Cmd[1]);

}
	
int ListMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if ((_stricmp(MsgHddrPtr[i]->to, Call) == 0) ||
			((conn->sysop) && ((_stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0)) && (user->flags & F_SYSOP_IN_LM)))
		{
			Msgs++;
			ListMessage(MsgHddrPtr[i], conn);
		}
	}
	
	return(Msgs);
}

int ListMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (_stricmp(MsgHddrPtr[i]->from, Call) == 0)
		{
			Msgs++;
			ListMessage(MsgHddrPtr[i], conn);
		}
	}
	
	return(Msgs);
}

int ListMessagesAT(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (_stricmp(MsgHddrPtr[i]->via, Call) == 0)
		{
			Msgs++;
			ListMessage(MsgHddrPtr[i], conn);
		}
	}
	
	return(Msgs);
}
int GetUserMsg(int m, char * Call, BOOL SYSOP)
{
	struct MsgInfo * Msg;
	
	// Get Next (usually backwards) message which should be shown to this user
	//	ie Not Deleted, and not Private unless to or from Call

	do
	{
		Msg=MsgHddrPtr[m];

		if (Msg->status != 'K')
		{
			if (SYSOP) return m;			// Sysop can list or read anything
	
			if (Msg->status != 'H')
			{
				if (Msg->type == 'B' || Msg->type == 'T') return m;

				if (Msg->type == 'P')
				{
					if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
						return m;
				}
			}
		}

		m--;

	} while (m> 0);

	return 0;

}

BOOL CheckUserMsg(struct MsgInfo * Msg, char * Call, BOOL SYSOP)
{
	// Return TRUE if user is allowed to read message
	
	if (SYSOP) return TRUE;			// Sysop can list or read anything

	if ((Msg->status != 'K') && (Msg->status != 'H'))
	{
		if (Msg->type == 'B' || Msg->type == 'T') return TRUE;

		if (Msg->type == 'P')
		{
			if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
				return TRUE;
		}
	}

	return FALSE;
}

int GetUserMsgForwards(int m, char * Call, BOOL SYSOP)
{
	struct MsgInfo * Msg;
	
	// Get Next (usually backwards) message which should be shown to this user
	//	ie Not Deleted, and not Private unless to or from Call

	do
	{
		Msg=MsgHddrPtr[m];
		
		if (Msg->status != 'K')
		{
			if (SYSOP) return m;			// Sysop can list or read anything

			if (Msg->status != 'H')
			{
				if (Msg->type == 'B' || Msg->type == 'T') return m;

				if (Msg->type == 'P')
				{
					if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
						return m;
				}
			}
		}

		m++;

	} while (m <= NumberofMessages);

	return 0;

}


void ListMessagesInRange(ConnectionInfo * conn, struct UserInfo * user, char * Call, int Start, int End)
{
	int m;
	struct MsgInfo * Msg;

	for (m = Start; m >= End; m--)
	{
		Msg = MsgnotoMsg[m];
		
		if (Msg && CheckUserMsg(Msg, user->Call, conn->sysop))
			ListMessage(Msg, conn);
	}
}


void ListMessagesInRangeForwards(ConnectionInfo * conn, struct UserInfo * user, char * Call, int End, int Start)
{
	int m;
	struct MsgInfo * Msg;

	for (m = Start; m <= End; m++)
	{
		Msg = MsgnotoMsg[m];
		
		if (Msg && CheckUserMsg(Msg, user->Call, conn->sysop))
			ListMessage(Msg, conn);
	}
}


void DoReadCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;

	
	switch (toupper(Cmd[1]))
	{
	case 0:					// Just R

		while (Arg1)
		{
			msgno = atoi(Arg1);
			ReadMessage(conn, user, msgno);
			Arg1 = strtok_s(NULL, " \r", &Context);
		}

		return;

	case 'M':					// Read Mine (Unread Messages)

		for (i=NumberofMessages; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if ((_stricmp(Msg->to, user->Call) == 0) || (conn->sysop && _stricmp(Msg->to, "SYSOP") == 0 && user->flags & F_SYSOP_IN_LM))
				if (Msg->status == 'N')
					ReadMessage(conn, user, Msg->number);
		}

		return;
	}
	
	nodeprintf(conn, "*** Error: Invalid Read option %c\r", Cmd[1]);
	
	return;
}

int RemoveLF(char * Message, int len)
{
	// Remove lf chars

	char * ptr1, * ptr2;

	ptr1 = ptr2 = Message;

	while (len-- > 0)
	{
		*ptr2 = *ptr1;
	
		if (*ptr1 == '\r')
			if (*(ptr1+1) == '\n')
			{
				ptr1++;
				len--;
			}
		ptr1++;
		ptr2++;
	}

	return (ptr2 - Message);
}

void ReadMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;
	char * MsgBytes, * Save;
	char FullTo[100];

	Msg = MsgnotoMsg[msgno];

	if (Msg == NULL)
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	if (!CheckUserMsg(Msg, user->Call, conn->sysop))
	{
		nodeprintf(conn, "Message %d not for you\r", msgno);
		return;
	}

	if (_stricmp(Msg->to, "RMS") == 0)
		 wsprintf(FullTo, "RMS:%s", Msg->via);
	else
	if (Msg->to[0] == 0)
		wsprintf(FullTo, "smtp:%s", Msg->via);
	else
		strcpy(FullTo, Msg->to);


	nodeprintf(conn, "From: %s%s\rTo: %s\rType/Status: %c%c\rDate/Time: %s\rBid: %s\rTitle: %s\r\r",
		Msg->from, Msg->emailfrom, FullTo, Msg->type, Msg->status, FormatDateAndTime(Msg->datecreated, FALSE), Msg->bid, Msg->title);

	MsgBytes = Save = ReadMessageFile(msgno);

	if (MsgBytes)
	{
		int Length;

		if (Msg->B2Flags)
		{
			// Remove B2 Headers (up to the File: Line)
			
			char * ptr;
			ptr = strstr(MsgBytes, "Body:");

			if (ptr)
				MsgBytes = ptr;
		}

		// Remove lf chars

		Length = RemoveLF(MsgBytes, strlen(MsgBytes));

		user->MsgsSent ++;
		user->BytesForwardedOut += Length;

		QueueMsg(conn, MsgBytes, Length);
		free(Save);

		nodeprintf(conn, "\r\r[End of Message #%d from %s]\r", msgno, Msg->from);

		if ((_stricmp(Msg->to, user->Call) == 0) || ((conn->sysop) && (_stricmp(Msg->to, "SYSOP") == 0)))
		{
			if ((Msg->status != 'K') && (Msg->status != 'H'))
			{
				Msg->status = 'Y';
				Msg->datechanged=time(NULL);
			}
		}
	}
	else
	{
		nodeprintf(conn, "File for Message %d not found\r", msgno);
	}

}
 struct MsgInfo * FindMessage(char * Call, int msgno, BOOL sysop)
 {
	int m=NumberofMessages;

	struct MsgInfo * Msg;

	do
	{
		m = GetUserMsg(m, Call, sysop);

		if (m == 0)
			return NULL;

		Msg=MsgHddrPtr[m];

		if (Msg->number == msgno)
			return Msg;

		m--;

	} while (m> 0);

	return NULL;

}

char * ReadMessageFile(int msgno)
{
	int FileSize;
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	char * MsgBytes;
	int ReadLen;
 
	sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes", MailDir, msgno);
	
	hFile = CreateFile(MsgFile,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	FileSize = GetFileSize(hFile, NULL);

	MsgBytes=malloc(FileSize+1);

	ReadFile(hFile, MsgBytes, FileSize, &ReadLen, NULL); 

	CloseHandle(hFile);

	MsgBytes[FileSize]=0;

	return MsgBytes;


}

/*
r 1378
From         : G8BPQ
To           : G8BPQ
Type/Status  : PN
Date/Time    : 09-May 13:01
Bid          : 1378_G8BPQ
Message #    : 1378
Title        : Error-report
> *** System boot on Sun 01/03/09 00:01 ***
> *** System boot on Tue 05/05/09 14:21 ***
> *** System boot on Sat 09/05/09 11:23 ***
> *** System boot on Sat 09/05/09 11:30 ***
> *** System boot on Sat 09/05/09 11:49 ***
> *** System boot on Sat 09/05/09 12:24 ***
***************



[End of Message #1378 from G8BPQ]
*/

char * FormatDateAndTime(time_t Datim, BOOL DateOnly)
{
	struct tm *tm;
	static char Date[]="xx-xxx hh:mmZ";

	tm = gmtime(&Datim);
	
	if (tm)
		sprintf_s(Date, sizeof(Date), "%02d-%3s %02d:%02dZ",
					tm->tm_mday, month[tm->tm_mon], tm->tm_hour, tm->tm_min);

	if (DateOnly)
	{
		Date[6]=0;
		return Date;
	}
	
	return Date;
}

BOOL DecodeSendParams(CIRCUIT * conn, char * Context, char ** From, char ** To, char ** ATBBS, char ** BID);


BOOL DoSendCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	// SB WANT @ ALLCAN < N6ZFJ $4567_N0ARY
	
	char * From = NULL;
	char * BID = NULL;
	char * ATBBS = NULL;
	char seps[] = " \t\r";
	struct MsgInfo * OldMsg;
	char OldTitle[62];
	char NewTitle[62];
	char SMTPTO[100]= "";
	int msgno;

	switch (toupper(Cmd[1]))
	{

	case 0:					// Just S means SP
		
		Cmd[1] = 'P';

	case 'P':
	case 'B':
	case 'T':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: The 'TO' callsign is missing\r");
			return FALSE;
		}

		if (!DecodeSendParams(conn, Context, &From, &Arg1, &ATBBS, &BID))
			return FALSE;

		return CreateMessage(conn, From, Arg1, ATBBS, toupper(Cmd[1]), BID, NULL);	

	case 'R':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: Message Number is missing\r");
			return FALSE;
		}

		msgno = atoi(Arg1);

		OldMsg = FindMessage(user->Call, msgno, conn->sysop);

		if (OldMsg == NULL)
		{
			nodeprintf(conn, "Message %d not found\r", msgno);
			return FALSE;
		}

		Arg1=&OldMsg->from[0];

		if (_stricmp(Arg1, "SMTP:") == 0 || _stricmp(Arg1, "RMS:") == 0)
		{
			// SMTP message. Need to get the real sender from the message

			sprintf(SMTPTO, "%s%s", Arg1, OldMsg->emailfrom);

			Arg1 = SMTPTO;
		}

		if (!DecodeSendParams(conn, "", &From, &Arg1, &ATBBS, &BID))
			return FALSE;

		strcpy(OldTitle, OldMsg->title);

		if (strlen(OldTitle) > 57) OldTitle[57] = 0;

		strcpy(NewTitle, "Re:");
		strcat(NewTitle, OldTitle);

		return CreateMessage(conn, From, Arg1, ATBBS, 'P', BID, NewTitle);	

		return TRUE;

	case 'C':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: Message Number is missing\r");
			return FALSE;
		}

		msgno = atoi(Arg1);

		Arg1 = strtok_s(NULL, seps, &Context);

		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: The 'TO' callsign is missing\r");
			return FALSE;
		}

		if (!DecodeSendParams(conn, Context, &From, &Arg1, &ATBBS, &BID))
			return FALSE;
	
		OldMsg = FindMessage(user->Call, msgno, conn->sysop);

		if (OldMsg == NULL)
		{
			nodeprintf(conn, "Message %d not found\r", msgno);
			return FALSE;
		}

		strcpy(OldTitle, OldMsg->title);

		if (strlen(OldTitle) > 56) OldTitle[56] = 0;

		strcpy(NewTitle, "Fwd:");
		strcat(NewTitle, OldTitle);

		conn->CopyBuffer = ReadMessageFile(msgno);

		return CreateMessage(conn, From, Arg1, ATBBS, 'P', BID, NewTitle);	
	}


	nodeprintf(conn, "*** Error: Invalid Send option %c\r", Cmd[1]);

	return FALSE;
}

BOOL DecodeSendParams(CIRCUIT * conn, char * Context, char ** From, char ** To, char ** ATBBS, char ** BID)
{
	char * ptr;
	char seps[] = " \t\r";
	WPRecP WP;

	// Accept call@call (without spaces) - but check for smtp addresses

	if (_memicmp(*To, "smtp:", 5) != 0 && _memicmp(*To, "rms:", 4) != 0  && _memicmp(*To, "rms/", 4) != 0)
	{
		ptr = strchr(*To, '@');

		if (ptr)
		{
			*ATBBS = strlop(*To, '@');
		}
	}

	// Look for Optional fields;

	ptr = strtok_s(NULL, seps, &Context);

	while (ptr)
	{
		if (strcmp(ptr, "@") == 0)
		{
			*ATBBS = _strupr(strtok_s(NULL, seps, &Context));
		}
		else if(strcmp(ptr, "<") == 0)
		{
			*From = strtok_s(NULL, seps, &Context);
		}
		else if (ptr[0] == '$')
			*BID = &ptr[1];
		else
		{
			nodeprintf(conn, "*** Error: Invalid Format\r");
			return FALSE;
		}
		ptr = strtok_s(NULL, seps, &Context);
	}

	// Only allow < from a BBS

	if (*From)
	{
		if (!(conn->BBSFlags & BBS))
		{
			nodeprintf(conn, "*** < can only be used by a BBS\r");
			return FALSE;
		}
	}

	if (!*From)
		*From = conn->UserPointer->Call;

	if (!(conn->BBSFlags & BBS))
	{
		// if a normal user, check that TO and/or AT are known and warn if not.

		if (_stricmp(*To, "SYSOP") == 0)
		{
			conn->LocalMsg = TRUE;
			return TRUE;
		}

		if (!*ATBBS)
		{
			// No routing, if not a user and not known to forwarding or WP warn

			struct UserInfo * ToUser = LookupCall(*To);

			if (ToUser)
			{
				// Local User. If Home BBS is specified, use it

				if (ToUser->HomeBBS[0])
				{
					*ATBBS = ToUser->HomeBBS;
					nodeprintf(conn, "Address @%s added from HomeBBS\r", *ATBBS);
				}
				else
				{
					conn->LocalMsg = TRUE;
				}
			}
			else
			{
				conn->LocalMsg = FALSE;
				WP = LookupWP(*To);

				if (WP)
				{
					*ATBBS = WP->first_homebbs;
					nodeprintf(conn, "Address @%s added from WP\r", *ATBBS);
				}
			}
		}
	}
	return TRUE;
}

BOOL CreateMessage(CIRCUIT * conn, char * From, char * ToCall, char * ATBBS, char MsgType, char * BID, char * Title)
{
	struct MsgInfo * Msg;
	char * via = NULL;

	// Create a temp msg header entry

	Msg = malloc(sizeof (struct MsgInfo));

	if (Msg == 0)
	{
		CriticalErrorHandler("malloc failed for new message header");
		return FALSE;
	}
	
	memset(Msg, 0, sizeof (struct MsgInfo));

	conn->TempMsg = Msg;

	Msg->type = MsgType;
	
	if (conn->UserPointer->flags & F_HOLDMAIL)
		Msg->status = 'H';
	else
		Msg->status = 'N';
	
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	if (BID)
	{
		BIDRec * TempBID;
		
		if ((Msg->type == 'B') && LookupBID(BID))			// Only reject Bulls on BID 
		{
			// Duplicate bid
	
			if ((conn->BBSFlags & BBS))
			{
				nodeprintf(conn, "NO - BID\r");
				nodeprintf(conn, ">\r");
			}
			else
				nodeprintf(conn, "*** Error- Duplicate BID\r");

			return FALSE;
		}

		if (strlen(BID) > 12) BID[12] = 0;
		strcpy(Msg->bid, BID);

		// Save BID in temp list in case we are offered it again before completion
			
		TempBID = AllocateTempBIDRecord();
		strcpy(TempBID->BID, BID);
		TempBID->u.conn = conn;

	}

	if (_memicmp(ToCall, "rms:", 4) == 0)
	{
		if (!FindRMS())
		{
			nodeprintf(conn, "*** Error - Forwarding via RMS is not configured on this BBS\r");
			return FALSE;
		}

		via=strlop(ToCall, ':');
		_strupr(ToCall);
	}
	else if (_memicmp(ToCall, "rms/", 4) == 0)
	{
		if (!FindRMS())
		{
			nodeprintf(conn, "*** Error - Forwarding via RMS is not configured on this BBS\r");
			return FALSE;
		}

		via=strlop(ToCall, '/');
		_strupr(ToCall);
	}
	else if (_memicmp(ToCall, "smtp:", 5) == 0)
	{
		if (ISP_Gateway_Enabled)
		{
			if ((conn->UserPointer->flags & F_EMAIL) == 0)
			{
				nodeprintf(conn, "*** Error - You need to ask the SYSOP to allow you to use Internet Mail\r");
				return FALSE;
			}
			via=strlop(ToCall, ':');
			ToCall[0] = 0;
		}
		else
		{
			nodeprintf(conn, "*** Error - Sending mail to smtp addresses is disabled\r");
			return FALSE;
		}
	}
	else
	{
		_strupr(ToCall);
		if (ATBBS)
			via=_strupr(ATBBS);
	}

	if (strlen(ToCall) > 6) ToCall[6] = 0;
	
	strcpy(Msg->to, ToCall);

	if (via)
	{
		if (strlen(via) > 40) via[40] = 0;

		strcpy(Msg->via, via);
	}

	strcpy(Msg->from, From);

	if (Title)					// Only used by SR and SC
	{
		strcpy(Msg->title, Title);
		conn->Flags |= GETTINGMESSAGE;
		nodeprintf(conn, "Enter Message Text (end with /ex or ctrl/z)\r");
		return TRUE;
	}

	if (!(conn->BBSFlags & FBBCompressed))
		conn->Flags |= GETTINGTITLE;

	if (!(conn->BBSFlags & BBS))
		nodeprintf(conn, "Enter Title (only):\r");
	else
		if (!(conn->BBSFlags & FBBForwarding))
			nodeprintf(conn, "OK\r");

	return TRUE;
}

VOID ProcessMsgTitle(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int msglen)
{
		
	conn->Flags &= ~GETTINGTITLE;

	if (msglen == 1)
	{
		nodeprintf(conn, "*** Message Cancelled\r");
		SendPrompt(conn, user);
		return;
	}

	if (msglen > 60) msglen = 60;

	Buffer[msglen-1] = 0;

	strcpy(conn->TempMsg->title, Buffer);

	// Create initial buffer of 10K. Expand if needed later

	conn->MailBuffer=malloc(10000);
	conn->MailBufferSize=10000;

	if (conn->MailBuffer == NULL)
	{
		nodeprintf(conn, "Failed to create Message Buffer\r");
		return;
	}

	conn->Flags |= GETTINGMESSAGE;

	if (!conn->BBSFlags & BBS)
		nodeprintf(conn, "Enter Message Text (end with /ex or ctrl/z)\r");

}

VOID ProcessMsgLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int msglen)
{
	char * ptr2 = NULL;

	if (((msglen < 3) && (Buffer[0] == 0x1a)) || ((msglen == 4) && (_memicmp(Buffer, "/ex", 3) == 0)))
	{
		conn->Flags &= ~GETTINGMESSAGE;

		user->MsgsReceived++;
		user->BytesForwardedIn += conn->TempMsg->length;

		CreateMessageFromBuffer(conn);
		return;

	}

	Buffer[msglen++] = 0x0a;

	if ((conn->TempMsg->length + msglen) > conn->MailBufferSize)
	{
		conn->MailBufferSize += 10000;
		conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
		if (conn->MailBuffer == NULL)
		{
			nodeprintf(conn, "Failed to extend Message Buffer\r");

			conn->Flags &= ~GETTINGMESSAGE;
			return;
		}
	}

	memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, msglen);

	conn->TempMsg->length += msglen;
}

VOID CreateMessageFromBuffer(CIRCUIT * conn)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;
	char * ptr1, * ptr2 = NULL;
	char * ptr3, * ptr4;
	int FWDCount;
	char OldMess[] = "\r\n\r\nOriginal Message:\r\n\r\n";
	struct _EXCEPTION_POINTERS exinfo;
	int Age, OurCount;
	char * HoldReason = "User has Hold Messages flag set";

	// If doing SC, Append Old Message

	if (conn->CopyBuffer)
	{
		if ((conn->TempMsg->length + (int) strlen(conn->CopyBuffer) + 80 )> conn->MailBufferSize)
		{
			conn->MailBufferSize += strlen(conn->CopyBuffer) + 80;
			conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
			if (conn->MailBuffer == NULL)
			{
				nodeprintf(conn, "Failed to extend Message Buffer\r");

				conn->Flags &= ~GETTINGMESSAGE;
				return;
			}
		}

		memcpy(&conn->MailBuffer[conn->TempMsg->length], OldMess, strlen(OldMess));

		conn->TempMsg->length += strlen(OldMess);

		memcpy(&conn->MailBuffer[conn->TempMsg->length], conn->CopyBuffer, strlen(conn->CopyBuffer));

		conn->TempMsg->length += strlen(conn->CopyBuffer);

		free(conn->CopyBuffer);
		conn->CopyBuffer = NULL;
	}

		// Allocate a message Record slot

		Msg = AllocateMsgRecord();
		memcpy(Msg, conn->TempMsg, sizeof(struct MsgInfo));

		free(conn->TempMsg);

		// Set number here so they remain in sequence
		
		GetSemaphore(&MsgNoSemaphore);
		Msg->number = ++LatestMsg;
		FreeSemaphore(&MsgNoSemaphore);
		MsgnotoMsg[Msg->number] = Msg;

		// Create BID if non supplied

		if (Msg->bid[0] == 0)
			sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

		// if message body had R: lines, get date created from last (not very accurate, but best we can do)

		// Also check if we have had message beofre to detect loops


		ptr1 = conn->MailBuffer;
		OurCount = 0;

nextline:

		if (memcmp(ptr1, "R:", 2) == 0)
		{
			// Is if ours?

			// BPQ RLINE Format R:090920/1041Z 6542@N4JOA.#WPBFL.FL.USA.NOAM BPQ1.0.2

			ptr3 = strchr(ptr1, '@');
			ptr4 = strchr(ptr1, '.');

			if (ptr3 && ptr4 && (ptr4 > ptr3))
			{
				if (memcmp(ptr3+1, BBSName, ptr4-ptr3-1) == 0)
					OurCount++;
			}
			
			// see if another

			ptr2 = ptr1;			// save
			ptr1 = strchr(ptr1, '\r');
			ptr1++;
			if (*ptr1 == '\n') ptr1++;

			goto nextline;
		}

		// ptr2 points to last R: line (if any)

		if (ptr2)
		{
			struct tm rtime;
			time_t result;

			memset(&rtime, 0, sizeof(struct tm));

			if (ptr2[10] == '/')
			{
				// Dodgy 4 char year
			
				sscanf(&ptr2[2], "%04d%02d%02d/%02d%02d",
					&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);
				rtime.tm_year -= 1900;
				rtime.tm_mon--;
			}
			else if (ptr2[8] == '/')
			{
				sscanf(&ptr2[2], "%02d%02d%02d/%02d%02d",
					&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);

				if (rtime.tm_year < 90)
					rtime.tm_year += 100;		// Range 1990-2089
				rtime.tm_mon--;
			}

			// Otherwise leave date as zero, which should be rejected

//			result = _mkgmtime(&rtime);

			if ((result = _mkgmtime(&rtime)) != (time_t)-1 )
			{
				Msg->datecreated =  result;	
				Age = (time(NULL) - result)/86400;

				if ( Age < -7)
				{
					Msg->status = 'H';
					HoldReason = "Suspect Date Sent";
				}
				else if (Age > BidLifetime)
				{
					Msg->status = 'H';
					HoldReason = "Message too old";

				}
				else
					GetWPInfoFromRLine(Msg->from, ptr2, result);
			}
			else
			{
				// Can't decode R: Datestamp

				Msg->status = 'H';
				HoldReason = "Corrupt R: Line - can't determine age";
			}

			if (OurCount > 1)
			{
				// Message is looping 

				Msg->status = 'H';
				HoldReason = "Message may be looping";

			}

			if (Msg->status == 'N' && strcmp(Msg->to, "WP") == 0)
			{
				ProcessWPMsg(conn->MailBuffer, Msg->length, ptr2);
	
				if (Msg->type == 'P')			// Kill any processed private WP messages.
					Msg->status = 'K';

			}
		}

		conn->MailBuffer[Msg->length] = 0;

		if (CheckBadWords(Msg->title) || CheckBadWords(conn->MailBuffer))
		{
			Msg->status = 'H';
			HoldReason = "Bad word in title or body";
		}

		CreateMessageFile(conn, Msg);

		BIDRec = AllocateBIDRecord();

		strcpy(BIDRec->BID, Msg->bid);
		BIDRec->mode = Msg->type;
		BIDRec->u.msgno = LOWORD(Msg->number);
		BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

		if (Msg->length > MaxTXSize)
		{
			Msg->status = 'H';
			HoldReason = "Message too long";

			if (!(conn->BBSFlags & BBS))
				nodeprintf(conn, "*** Warning Message length exceeds sysop-defined maximum of %d - Message will be held\r", MaxTXSize);
		}

		if (Msg->to[0])
			FWDCount = MatchMessagetoBBSList(Msg, conn);
		else
		{
			// If addressed @winlink.org, and to a local user, Keep here.
			
			char * Call;
			char * AT;

			Call = _strupr(_strdup(Msg->via));
			AT = strlop(Call, '@');

			if (_stricmp(AT, "WINLINK.ORG") == 0)
			{
				struct UserInfo * user = LookupCall(Call);

				if (user)
				{
					if (user->flags & F_POLLRMS)
					{
						Logprintf(LOG_BBS, conn, '?', "SMTP Message @ winlink.org, but local RMS user - leave here");
						strcpy(Msg->to, Call);
						strcpy(Msg->via, AT);
					}
				}
			}
			free(Call);
		}

		if (!(conn->BBSFlags & BBS))
		{
			nodeprintf(conn, "Message: %d Bid:  %s Size: %d\r", Msg->number, Msg->bid, Msg->length);

			if (Msg->via[0])
			{	
				if (FWDCount ==  0 &&  Msg->to[0] != 0)		// unless smtp msg
					nodeprintf(conn, "@BBS specified, but no forwarding info is available - msg may not be delivered\r");
			}
			else
			{
				if (FWDCount ==  0 && conn->LocalMsg == 0)
					// Not Local and no forward route
					nodeprintf(conn, "Message is not for a local user, and no forwarding info is available - msg may not be delivered\r");
			}
			SendPrompt(conn, conn->UserPointer);
		}
		else
			if (!(conn->BBSFlags & FBBForwarding))
				BBSputs(conn, ">\r");

		if(Msg->to[0] == 0)
			SMTPMsgCreated=TRUE;


		if (Msg->status == 'H')
		{
			int Length=0;
			char * MailBuffer = malloc(100);
			char Title[100];

			Length += wsprintf(MailBuffer, "Message %d Held\r\n", Msg->number);
			wsprintf(Title, "Message %d Held - %s", Msg->number, HoldReason);
			SendMessageToSYSOP(Title, MailBuffer, Length);
		}

		BuildNNTPList(Msg);				// Build NNTP Groups list

		SaveMessageDatabase();
		SaveBIDDatabase();

		__try
		{
			SendMsgUI(Msg);
		}
		My__except_Routine("SendMsgUI");

		return;
}

VOID CreateMessageFile(ConnectionInfo * conn, struct MsgInfo * Msg)
{
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int WriteLen=0;
	char Mess[255];
	int len;

	sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes", MailDir, Msg->number);
	
	hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, conn->MailBuffer, Msg->length, &WriteLen, NULL);
		CloseHandle(hFile);
	}

	free(conn->MailBuffer);
	conn->MailBufferSize=0;
	conn->MailBuffer=0;

	if (WriteLen != Msg->length)
	{
		len = sprintf_s(Mess, sizeof(Mess), "Failed to create Message File\r");
		QueueMsg(conn, Mess, len);
		CriticalErrorHandler(Mess);
	}
	return;
}


void chat_link_out (LINK *link)
{
	int n, p;
	CIRCUIT * conn;
	char Msg[80];

	for (n = NumberofStreams-1; n >= 0 ; n--)
	{
		conn = &Connections[n];
		
		if (conn->Active == FALSE)
		{
			p = conn->BPQStream;
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->BPQStream = p;

			conn->Active = TRUE;
			circuit_new(conn,p_linkini);
			conn->u.link = link;
			conn->Flags = CHATMODE | CHATLINK;

			ConnectUsingAppl(conn->BPQStream, ChatApplMask);

			n=sprintf_s(Msg, sizeof(Msg), "Connecting to Chat Node %s", conn->u.link->alias);

			strcpy(conn->Callsign, conn->u.link->alias);

			WriteLogLine(conn, '|',Msg, n, LOG_CHAT);
	
			//	Connected Event will trigger connect to remote system

			return;
		}
	}

	return;
	

}

ProcessConnecting(CIRCUIT * circuit, char * Buffer, int Len)
{
	WriteLogLine(circuit, '<' ,Buffer, Len-1, LOG_CHAT);

	Buffer = _strupr(Buffer);

	if (memcmp(Buffer, "[BPQCHATSERVER-", 15) == 0)
	{
		char * ptr = strchr(Buffer, ']');
		if (ptr)
		{
			*ptr = 0;
			strcpy(circuit->FBBReplyChars, &Buffer[15]);
		}
		else
			circuit->FBBReplyChars[0] = 0;

		return 0;
	}

	if (memcmp(Buffer, "OK", 2) == 0)
	{
		// Make sure node isn't known. There is a window here that could cause a loop

		if (node_find(circuit->u.link->call))
		{
			Logprintf(LOG_CHAT, circuit, '|', "Dropping link with %s to prevent a loop", circuit->Callsign);
			Disconnect(circuit->BPQStream);
			return FALSE;
		}

		circuit->u.link->flags = p_linked;
 	  	circuit->rtcflags = p_linked;
		state_tell(circuit, circuit->FBBReplyChars);
		NeedStatus = TRUE;

		return TRUE;
	}

	
	if (strstr(Buffer, "CONNECTED") || strstr(Buffer, "LINKED"))
	{
		// Connected - Send *RTL 
		
		nputs(circuit, "*RTL\r");  // Log in to the remote RT system.
		nprintf(circuit, "%c%c%s %s %s\r", FORMAT, id_keepalive, OurNode, circuit->u.link->call, Verstring);

		return TRUE;

	}

	if (strstr(Buffer, "BUSY") || strstr(Buffer, "FAILURE") || strstr(Buffer, "DOWNLINK")|| strstr(Buffer, "SORRY"))
	{
		link_drop(circuit);
		Disconnect(circuit->BPQStream);
	}
	
	return FALSE;

}

VOID SetupFwdTimes(struct BBSForwardingInfo * ForwardingInfo)
{
	char ** Times = ForwardingInfo->FWDTimes;
	int Start, End;
	int Count = 0;

	ForwardingInfo->FWDBands = zalloc(sizeof(struct FWDBAND));

	if (Times)
	{
		while(Times[0])
		{
			ForwardingInfo->FWDBands = realloc(ForwardingInfo->FWDBands, (Count+2)* sizeof(struct FWDBAND));
			ForwardingInfo->FWDBands[Count] = zalloc(sizeof(struct FWDBAND));

			Start = atoi(Times[0]);
			End = atoi(&Times[0][5]);

			ForwardingInfo->FWDBands[Count]->FWDStartBand =  (time_t)(Start / 100) * 3600 + (Start % 100) * 60; 
			ForwardingInfo->FWDBands[Count]->FWDEndBand =  (time_t)(End / 100) * 3600 + (End % 100) * 60 + 59; 

			Count++;
			Times++;
		}
		ForwardingInfo->FWDBands[Count] = NULL;
	}
}

VOID SetupForwardingStruct(struct UserInfo * user)
{
	struct	BBSForwardingInfo * ForwardingInfo;
	HKEY hKey=0;
	int retCode,Type,Vallen;
	char Key[100] =  "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\BBSForwarding\\";
	int m;
	struct MsgInfo * Msg;

	ForwardingInfo = user->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));

	strcat(Key, user->Call);
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		ForwardingInfo->ConnectScript = GetMultiStringValue(hKey,  "Connect Script");
		ForwardingInfo->TOCalls = GetMultiStringValue(hKey,  "TOCalls");
		ForwardingInfo->ATCalls = GetMultiStringValue(hKey,  "ATCalls");
		ForwardingInfo->Haddresses = GetMultiStringValue(hKey,  "HRoutes");
		ForwardingInfo->HaddressesP = GetMultiStringValue(hKey,  "HRoutesP");
		ForwardingInfo->FWDTimes = GetMultiStringValue(hKey,  "FWD Times");

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "Enabled", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->Enabled,(ULONG *)&Vallen);
				
		Vallen=4;
		retCode += RegQueryValueEx(hKey, "RequestReverse", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->ReverseFlag,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "Use B1 Protocol", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->AllowB1,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "Use B2 Protocol", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->AllowB2,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "FWD Personals Only", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->PersonalOnly,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "FWD New Immediately", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->SendNew,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"FWDInterval",0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->FwdInterval,(ULONG *)&Vallen);

		RegQueryValueEx(hKey,"MaxFBBBlock",0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->MaxFBBBlockSize,(ULONG *)&Vallen);

		if (ForwardingInfo->MaxFBBBlockSize == 0)
			ForwardingInfo->MaxFBBBlockSize = 10000;

		if (ForwardingInfo->FwdInterval == 0)
				ForwardingInfo->FwdInterval = 3600;

		Vallen=0;
		retCode = RegQueryValueEx(hKey,"BBSHA",0 , (ULONG *)&Type,NULL, (ULONG *)&Vallen);

		if (retCode != 0)
		{
			// No Key - Get from WP??
				
			WPRec * ptr = LookupWP(user->Call);

			if (ptr)
			{
				if (ptr->first_homebbs)
				{
					ForwardingInfo->BBSHA = _strdup(ptr->first_homebbs);
				}
			}
		}

		if (Vallen)
		{
			ForwardingInfo->BBSHA = malloc(Vallen);
			RegQueryValueEx(hKey, "BBSHA", 0, (ULONG *)&Type, ForwardingInfo->BBSHA,(ULONG *)&Vallen);
		}

		RegCloseKey(hKey);

		// Convert FWD Times and H Addresses

		if (ForwardingInfo->FWDTimes)
			SetupFwdTimes(ForwardingInfo);

		if (ForwardingInfo->Haddresses)
			SetupHAddreses(ForwardingInfo);

		if (ForwardingInfo->HaddressesP)
			SetupHAddresesP(ForwardingInfo);

		if (ForwardingInfo->BBSHA)
			if (ForwardingInfo->BBSHA[0])
				SetupHAElements(ForwardingInfo);
			else
			{
				free(ForwardingInfo->BBSHA);
				ForwardingInfo->BBSHA = NULL;
			}
	}

	for (m = FirstMessageIndextoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		// If any forward bits are set, increment count on  BBS record.

		if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		{
			if (Msg->type && check_fwd_bit(Msg->fbbs, user->BBSNumber))
			{
				user->ForwardingInfo->MsgCount++;
			}
		}
	}
}

VOID * GetMultiStringValue(HKEY hKey, char * ValueName)
{
	int retCode,Type,Vallen;
	char * MultiString;
	int ptr, len;
	int Count = 0;
	char ** Value;

	Value = zalloc(4);				// always NULL entry on end even if no values

	Value[0] = NULL;

	Vallen=0;

	retCode = RegQueryValueEx(hKey, ValueName, 0, (ULONG *)&Type, NULL, (ULONG *)&Vallen);

	if ((retCode != 0)  || (Vallen < 3))		// Two nulls means empty multistring
	{
		free(Value);
		return FALSE;
	}

	MultiString = malloc(Vallen);

	retCode = RegQueryValueEx(hKey, ValueName, 0,			
			(ULONG *)&Type,(UCHAR *)MultiString,(ULONG *)&Vallen);

	ptr=0;

	while (MultiString[ptr])
	{
		len=strlen(&MultiString[ptr]);

		Value = realloc(Value, (Count+2)*4);
		Value[Count++] = _strupr(_strdup(&MultiString[ptr]));
		ptr+= (len + 1);
	}

	Value[Count] = NULL;

	free(MultiString);

	return Value;
}

VOID FreeForwardingStruct(struct UserInfo * user)
{
	struct	BBSForwardingInfo * ForwardingInfo;
	int i;


	ForwardingInfo = user->ForwardingInfo;

	FreeList(ForwardingInfo->TOCalls);
	FreeList(ForwardingInfo->ATCalls);
	FreeList(ForwardingInfo->Haddresses);
	FreeList(ForwardingInfo->HaddressesP);

	i=0;
	if(ForwardingInfo->HADDRS)
	{
		while(ForwardingInfo->HADDRS[i])
		{
			FreeList(ForwardingInfo->HADDRS[i]);
			i++;
		}
		free(ForwardingInfo->HADDRS);
		free(ForwardingInfo->HADDROffet);
	}

	i=0;
	if(ForwardingInfo->HADDRSP)
	{
		while(ForwardingInfo->HADDRSP[i])
		{
			FreeList(ForwardingInfo->HADDRSP[i]);
			i++;
		}
		free(ForwardingInfo->HADDRSP);
	}

	FreeList(ForwardingInfo->ConnectScript);
	FreeList(ForwardingInfo->FWDTimes);
	if (ForwardingInfo->FWDBands)
	{
		i=0;
		while(ForwardingInfo->FWDBands[i])
		{
			free(ForwardingInfo->FWDBands[i]);
			i++;
		}
		free(ForwardingInfo->FWDBands);
	}
	if (ForwardingInfo->BBSHAElements)
	{
		i=0;
		while(ForwardingInfo->BBSHAElements[i])
		{
			free(ForwardingInfo->BBSHAElements[i]);
			i++;
		}
		free(ForwardingInfo->BBSHAElements);
	}
	free(ForwardingInfo->BBSHA);

}

VOID FreeList(char ** Hddr)
{
	VOID ** Save;
	
	if (Hddr)
	{
		Save = Hddr;
		while(Hddr[0])
		{
			free(Hddr[0]);
			Hddr++;
		}	
		free(Save);
	}
}

BOOL ConnecttoBBS (struct UserInfo * user)
{
	int n, p;
	CIRCUIT * conn;

	for (n = NumberofStreams-1; n >= 0 ; n--)
	{
		conn = &Connections[n];
		
		if (conn->Active == FALSE)
		{
			p = conn->BPQStream;
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->BPQStream = p;

			conn->Active = TRUE;
			strcpy(conn->Callsign, user->Call); 
			conn->BBSFlags |= RunningConnectScript;
			conn->UserPointer = user;

			ConnectUsingAppl(conn->BPQStream, BBSApplMask);

			Logprintf(LOG_BBS, conn, '|', "Connecting to BBS %s", user->Call);

			strcpy(conn->Callsign, user->Call);

			//	Connected Event will trigger connect to remote system

			RefreshMainWindow();

			return TRUE;
		}
	}

	Logprintf(LOG_BBS, conn, '|', "No Free Streams for connect to BBS %s", user->Call);

	return FALSE;
	
}

struct DelayParam
{
	struct UserInfo * User;
	CIRCUIT * conn;
	int Delay;
};

struct DelayParam DParam;		// Not 100% safe, but near enough

VOID ConnectDelayThread(struct DelayParam * DParam)
{
	struct UserInfo * User = DParam->User;
	int Delay = DParam->Delay;

	Sleep(Delay);

	ConnecttoBBS(User);
	
	return;
}

VOID ConnectPauseThread(struct DelayParam * DParam)
{
	CIRCUIT * conn = DParam->conn;
	int Delay = DParam->Delay;
	char Msg[] = "Pause Ok\r    ";

	Sleep(Delay);

	ProcessBBSConnectScript(conn, Msg, 9);
	
	return;
}

unsigned long _beginthread( void( *start_address )(struct DelayParam * DParam),
				unsigned stack_size, struct DelayParam * DParam);

BOOL ProcessBBSConnectScript(CIRCUIT * conn, char * Buffer, int len)
{
	struct	BBSForwardingInfo * ForwardingInfo = conn->UserPointer->ForwardingInfo;
	char ** Scripts;
	char callsign[10];
	int port, sesstype, paclen, maxframe, l4window;
	char * ptr, * ptr2;
	BOOL MoreLines = TRUE;

	WriteLogLine(conn, '<',Buffer, len-1, LOG_BBS);

	Buffer[len]=0;
	_strupr(Buffer);

	Scripts = ForwardingInfo->ConnectScript;	

	if (ForwardingInfo->ScriptIndex == -1)
	{
		// First Entry - if first line is TIMES, check and skip forward if necessary
	
		int n = 0;
		int Start, End;
		time_t now = time(NULL), StartSecs, EndSecs;
		char * Line;

		now %= 86400;
		Line = Scripts[n];

		if (memcmp(Line, "TIMES", 5) == 0)
		{
		NextBand:
			Start = atoi(&Line[6]);
			End = atoi(&Line[11]);

			StartSecs =  (time_t)(Start / 100) * 3600 + (Start % 100) * 60; 
			EndSecs =  (time_t)(End / 100) * 3600 + (End % 100) * 60 + 59;

			if ((StartSecs <= now) && (EndSecs >= now))
				goto InBand;	// In band

			// Look for next TIME
		NextLine:
			Line = Scripts[++n];

			if (Line == NULL)
			{
				// No more lines - Disconnect
			
				Disconnect(conn->BPQStream);
				return FALSE;
			}

			if (memcmp(Line, "TIMES", 5) != 0)
				goto NextLine;
			else
				goto NextBand;
InBand:
			ForwardingInfo->ScriptIndex = n;	
		}

	}
	else
	{
		// Dont check first time through

		if (strcmp(Buffer, "*** CONNECTED  ") != 0)
		{
			if (Scripts[ForwardingInfo->ScriptIndex] == NULL ||
				memcmp(Scripts[ForwardingInfo->ScriptIndex], "TIMES", 5) == 0	||		// Only Check until script is finished
				memcmp(Scripts[ForwardingInfo->ScriptIndex], "ELSE", 4) == 0)			// Only Check until script is finished
			{
				MoreLines = FALSE;
			}
			if (!MoreLines)
				goto CheckForSID;
			}
	}

	if (strstr(Buffer, "BUSY") || strstr(Buffer, "FAILURE") || strstr(Buffer, "DOWNLINK") ||
		strstr(Buffer, "SORRY") || strstr(Buffer, "INVALID") || strstr(Buffer, "RETRIED") ||
		strstr(Buffer, "NO CONNECTION TO") || strstr(Buffer, "ERROR - PORT IN USE") ||
		strstr(Buffer, "DISCONNECTED"))
	{
		// Connect Failed

		char * Cmd = Scripts[++ForwardingInfo->ScriptIndex];
		int Delay = 1000;
	
		// Look for an alternative connect block (Starting with ELSE)

	ElseLoop:

		if (Cmd == 0 || memcmp(Cmd, "TIMES", 5) == 0)			// Only Check until script is finished
		{
			Disconnect(conn->BPQStream);
			return FALSE;
		}

		if (memcmp(Cmd, "ELSE", 4) != 0)
		{
			Cmd = Scripts[++ForwardingInfo->ScriptIndex];
			goto ElseLoop;
		}

		if (memcmp(&Cmd[5], "DELAY", 5) == 0)
			Delay = atoi(&Cmd[10]) * 1000;
		else
			Delay = 1000;

		Disconnect(conn->BPQStream);

		DParam.Delay = Delay;
		DParam.User = conn->UserPointer;

		_beginthread(ConnectDelayThread, 0, &DParam);
		
		return FALSE;
	}

	// The pointer is only updated when we get the connect, so we can tell when the last line is acked
	// The first entry is always from Connected event, so don't have to worry about testing entry -1 below


	// NETROM to  KA node returns

	//c 1 milsw
	//WIRAC:N9PMO-2} Connected to MILSW
	//###CONNECTED TO NODE MILSW(N9ZXS) CHANNEL A
	//You have reached N9ZXS's KA-Node MILSW
	//ENTER COMMAND: B,C,J,N, or Help ?

	//C KB9PRF-7
	//###LINK MADE
	//###CONNECTED TO NODE KB9PRF-7(KB9PRF-4) CHANNEL A

	// Look for (Space)Connected so we aren't fooled by ###CONNECTED TO NODE, which is not
	// an indication of a connect.


	if (strstr(Buffer, " CONNECTED") || strstr(Buffer, "PACLEN") ||
			strstr(Buffer, "OK") || strstr(Buffer, "###LINK MADE"))
	{
		char * Cmd;
	LoopBack:
		Cmd = Scripts[++ForwardingInfo->ScriptIndex];
		
		if (Cmd && memcmp(Cmd, "TIMES", 5) != 0 && memcmp(Cmd, "ELSE", 4) != 0)			// Only Check until script is finished
		{
			if (memcmp(Cmd, "INTERLOCK ", 10) == 0)
			{
				// Used to limit connects on a port to 1

				int Port;
				char Option[80];

				Logprintf(LOG_BBS, conn, '?', "Script %s", Cmd);

				sscanf(&Cmd[10], "%d %s", &Port, &Option);

				if (CountConnectionsOnPort(Port))
				{
					// if option is FAIL, look for an ELSE, otherwise if WAIT n, set poll timer and exit
								
					Logprintf(LOG_BBS, conn, '?', "Interlocked Port is busy - quitting");
					Disconnect(conn->BPQStream);
					return FALSE;
				}

				goto LoopBack;

			}
			else

			if (memcmp(Cmd, "RADIO AUTH", 10) == 0)
			{
				// Generate a Password to enable RADIO commands on a remote node
				char AuthCommand[80];

				strcpy(AuthCommand, Cmd);

				CreateOneTimePassword(&AuthCommand[11], &Cmd[11], 0); 

				nodeprintf(conn, "%s\r", AuthCommand);
				return TRUE;
			}

			if (memcmp(Cmd, "SKIPPROMPT", 10) == 0)
			{
				// Remote Node sends > at end of CTEXT - we need to swallow it

				conn->SkipPrompt = TRUE;
				return TRUE;
			}

			if (memcmp(Cmd, "PAUSE", 5) == 0)
			{
				// Pause script

				Logprintf(LOG_BBS, conn, '?', "Script %s", Cmd);

				DParam.Delay = atoi(&Cmd[6]) * 1000;
				DParam.conn = conn;

				_beginthread(ConnectPauseThread, 0, &DParam);

				return TRUE;
			}


			nodeprintf(conn, "%s\r", Cmd);
		}
		return TRUE;
	}

	ptr = strchr(Buffer, '}');

	if (ptr && MoreLines) // Beware it could be part of ctext
	{
		// Could be respsonse to Node Command 

		ptr+=2;
		
		ptr2 = strchr(&ptr[0], ' ');

		if (ptr2)
		{
			if (memcmp(ptr, Scripts[ForwardingInfo->ScriptIndex], ptr2-ptr) == 0)	// Reply to last sscript command
			{
				ForwardingInfo->ScriptIndex++;
		
				if (Scripts[ForwardingInfo->ScriptIndex])
					if (memcmp(Scripts[ForwardingInfo->ScriptIndex], "TIMES", 5) != 0)	
					nodeprintf(conn, "%s\r", Scripts[ForwardingInfo->ScriptIndex]);

				return TRUE;
			}
		}
	}

	// Not Success or Fail. If last line is still outstanding, wait fot Response
	//		else look for SID or Prompt

	if (MoreLines)
		return TRUE;

	// No more steps, Look for SID or Prompt

CheckForSID:

	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		// Update PACLEN

		GetConnectionInfo(conn->BPQStream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);

		if (paclen > 0)
			conn->paclen = paclen;

		
		Parse_SID(conn, &Buffer[1], len-4);
			
		if (conn->BBSFlags & FBBForwarding)
		{
			conn->FBBIndex = 0;		// ready for first block;
			memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
			conn->FBBChecksum = 0;
		}

		return TRUE;
	}

	if (memcmp(Buffer, "[PAKET ", 7) == 0)
	{
		conn->BBSFlags |= BBS;
		conn->BBSFlags |= MBLFORWARDING;
	}

	if (Buffer[len-2] == '>')
	{
		if (conn->SkipPrompt)
		{
			conn->SkipPrompt = FALSE;
			return TRUE;
		}
	
		conn->BBSFlags &= ~RunningConnectScript;

		if (strcmp(conn->Callsign, "RMS") == 0)
		{
			// Build a ;FW: line with all calls with PollRMS Set

			int i, s;
			char FWLine[10000] = ";FW:";
			struct UserInfo * user;
			char RMSCall[20];
			
			for (i = 0; i <= NumberofUsers; i++)
			{
				user = UserRecPtr[i];

				if (user->flags & F_POLLRMS)
				{
					if (user->RMSSSIDBits == 0) user->RMSSSIDBits = 1;

					for (s = 0; s < 16; s++)
					{
						if (user->RMSSSIDBits & (1 << s))
						{
							strcat(FWLine, " ");
							if (s)
							{
								wsprintf(RMSCall, "%s-%d", user->Call, s);
								strcat(FWLine, RMSCall);
							}
							else
								strcat(FWLine, user->Call);
							
						}
					}
				}
			}
			
			strcat(FWLine, "\r");	

			nodeprintf(conn, FWLine);

			//nodeprintf(conn,";FW: GM8BPQ-1 G8BPQ G8BPQ-5 GM8BPQ G8BPQ-1 GM8BPQ-2 BPQTST\r");
		}

		// Only delare B1 and B2 if other end did, and we are configued for it

		nodeprintf(conn, BBSSID, Ver[0], Ver[1], Ver[2], Ver[3],
			(conn->BBSFlags & FBBCompressed) ? "B" : "", 
			(conn->BBSFlags & FBBB1Mode && !(conn->BBSFlags & FBBB2Mode)) ? "1" : "",
			(conn->BBSFlags & FBBB2Mode) ? "2" : "",
			(conn->BBSFlags & FBBForwarding) ? "F" : ""); 

		conn->NextMessagetoForward = FirstMessageIndextoForward;

		conn->UserPointer->ConnectsOut++;

		if (conn->BBSFlags & FBBForwarding)
		{
			if (!FBBDoForward(conn))				// Send proposal if anthing to forward
				BBSputs(conn, "FF\r");

			return TRUE;
		}

		return TRUE;
	}

	return TRUE;
}

VOID Parse_SID(CIRCUIT * conn, char * SID, int len)
{
	// scan backwards for first '-'

	if (strstr(SID, "RMS Ex"))
	{
		conn->RMSExpress = TRUE;
		conn->Paclink = FALSE;
	}

	while (len > 0)
	{
		switch (SID[len--])
		{
		case '-':

			len=0;
			break;

		case '$':

			conn->BBSFlags |= BBS | MBLFORWARDING;
			conn->Paging = FALSE;

			break;

		case 'F':			// FBB Blocked Forwarding

			conn->BBSFlags |= FBBForwarding | BBS;
			conn->BBSFlags &= ~MBLFORWARDING;
		
			conn->Paging = FALSE;

			// Allocate a Header Block

			conn->FBBHeaders = zalloc(5 * sizeof(struct FBBHeaderLine));
			break;

		case 'B':

			if (ALLOWCOMPRESSED)
			{
				conn->BBSFlags |= FBBCompressed;

//				if (conn->UserPointer->ForwardingInfo->AllowB1) // !!!!! Testing !!!!
//					conn->BBSFlags |= FBBB1Mode;

				
				// Look for 1 or 2 or 12 as next 2 chars

				if (SID[len+2] == '1')
				{
					if (conn->UserPointer->ForwardingInfo->AllowB1 ||
						conn->UserPointer->ForwardingInfo->AllowB2)		// B2 implies B1
						conn->BBSFlags |= FBBB1Mode;

					if (SID[len+3] == '2')
						if (conn->UserPointer->ForwardingInfo->AllowB2)
							conn->BBSFlags |= FBBB1Mode | FBBB2Mode;	// B2 uses B1 mode (crc on front of file)

					break;
				}

				if (SID[len+2] == '2')
				{
					if (conn->UserPointer->ForwardingInfo->AllowB2)
							conn->BBSFlags |= FBBB1Mode | FBBB2Mode;	// B2 uses B1 mode (crc on front of file)
	
					if (conn->UserPointer->ForwardingInfo->AllowB1)
							conn->BBSFlags |= FBBB1Mode;				// B2 should allow fallback to B1 (but RMS doesnt!)

				}
				break;
			}

			break;
		}
	}
	return;
}
int	CriticalErrorHandler(char * error)
{
	return 0;
}

VOID FWDTimerProc()
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo ;

	for (user = BBSChain; user; user = user->BBSNext)
	{
		// See if any messages are queued for this BBS

		ForwardingInfo = user->ForwardingInfo;
		ForwardingInfo->FwdTimer+=10;

		if (ForwardingInfo->FwdTimer >= ForwardingInfo->FwdInterval)
		{
			ForwardingInfo->FwdTimer=0;

			if (ForwardingInfo->FWDBands && ForwardingInfo->FWDBands[0])
			{
				// Check Timebands

				struct FWDBAND ** Bands = ForwardingInfo->FWDBands;
				int Count = 0;
				time_t now = time(NULL);

				now %= 86400;		// Secs in to day

				while(Bands[Count])
				{
					if ((Bands[Count]->FWDStartBand < now) && (Bands[Count]->FWDEndBand >= now))
						goto FWD;	// In band

				Count++;
				}
				continue;				// Out of bands
			}
		FWD:	

				if (ForwardingInfo->Enabled)
					if (ForwardingInfo->ConnectScript  && (ForwardingInfo->Forwarding == 0) && ForwardingInfo->ConnectScript[0])
						if (SeeifMessagestoForward(user->BBSNumber) || ForwardingInfo->ReverseFlag)
/*
							if (strcmp(user->Call, "RMS") == 0)
							{
								if (ForwardingInfo->UserIndex == 0)
									FindNextRMSUser(ForwardingInfo);
								
								if (ForwardingInfo->UserCall[0] == 0)	// No Users to poll
									continue;
								if (ConnecttoBBS(user))
									ForwardingInfo->Forwarding = TRUE;
							}
							else
*/							{
								user->ForwardingInfo->ScriptIndex = -1;			 // Incremented before being used

								if (ConnecttoBBS(user))
									ForwardingInfo->Forwarding = TRUE;
							}
		}
	}
}

void StartForwarding(int BBSNumber)
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo ;

	for (user = BBSChain; user; user = user->BBSNext)
	{
		// See if any messages are queued for this BBS

		ForwardingInfo = user->ForwardingInfo;

		if ((BBSNumber == 0) || (user->BBSNumber == BBSNumber))
			if (ForwardingInfo)
				if (ForwardingInfo->Enabled || BBSNumber)		// Menu Command overrides enable
					if (ForwardingInfo->ConnectScript  && (ForwardingInfo->Forwarding == 0) && ForwardingInfo->ConnectScript[0])
						if (SeeifMessagestoForward(BBSNumber) || ForwardingInfo->ReverseFlag || BBSNumber) // Menu Command overrides Reverse
/*							if (strcmp(user->Call, "RMS") == 0)
							{
								if (ForwardingInfo->UserIndex == 0)
									FindNextRMSUser(ForwardingInfo);
								
								if (ForwardingInfo->UserCall[0] == 0)	// No Users to poll
									continue;
								if (ConnecttoBBS(user))
									ForwardingInfo->Forwarding = TRUE;
							}
							else
*/							{
								user->ForwardingInfo->ScriptIndex = -1;			 // Incremented before being used

								if (ConnecttoBBS(user))
									ForwardingInfo->Forwarding = TRUE;
							}

	}

	return;
}

// R:090209/0128Z 33040@N4JOA.#WPBFL.FL.USA.NOAM [164113] FBB7.01.35 alpha


char * DateAndTimeForHLine(time_t Datim)
{
	struct tm *newtime;
    char Time[80];
	static char Date[]="yymmdd/hhmmZ";
  
	newtime = gmtime(&Datim);
	asctime_s(Time, sizeof(Time), newtime);
	Date[0]=Time[22];
	Date[1]=Time[23];
	Date[3]=Time[4];
	Date[4]=Time[5];
	Date[5]=Time[6];
	
	return Date;
}


BOOL FindMessagestoForward (CIRCUIT * conn)
{
	// See if any messages are queued for this BBS

	int m;
	struct MsgInfo * Msg;
	struct UserInfo * user = conn->UserPointer;
	struct FBBHeaderLine * FBBHeader;
	BOOL Found = FALSE;
	char RLine[100];
	int TotalSize = 0;

	conn->FBBIndex = 0;

//	if (user->ForwardingInfo->MsgCount == 0)
//		return FALSE;

	for (m = conn->NextMessagetoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		// If forwarding to Paclink or RMS Express, look for any message matching the requested call list with statis 'N'

		if (conn->PacLinkCalls)
		{
			int index = 1;

			char * Call = conn->PacLinkCalls[0];

			while (Call)
			{
				if (_stricmp(Msg->to, Call) == 0)
					if (Msg->status == 'N') 
					goto Forwardit;
				
				Call = conn->PacLinkCalls[index++];
			}
			continue;
		}

		if ((Msg->status != 'H') && Msg->type && check_fwd_bit(Msg->fbbs, user->BBSNumber))
		{
			// Message to be sent - do a consistancy check (State, etc)

		Forwardit:
		
			if ((Msg->from[0] == 0) || (Msg->to[0] == 0))
			{
				int Length=0;
				char * MailBuffer = malloc(100);
				char Title[100];

				Length += wsprintf(MailBuffer, "Message %d Held\r\n", Msg->number);
				wsprintf(Title, "Message %d Held - %s", Msg->number, "Missing From: or To: field");
				SendMessageToSYSOP(Title, MailBuffer, Length);
			
				Msg->status = 'H';
				continue;
			}

			conn->NextMessagetoForward = m + 1;			// So we don't offer again if defered

			// if FBB forwarding add to list, eise save pointer

			if (conn->BBSFlags & FBBForwarding)
			{
				struct tm *tm;

				FBBHeader = &conn->FBBHeaders[conn->FBBIndex++];

				FBBHeader->FwdMsg = Msg;
				FBBHeader->MsgType = Msg->type;
				FBBHeader->Size = Msg->length;
				TotalSize += Msg->length;
				strcpy(FBBHeader->From, Msg->from);
				strcpy(FBBHeader->To, Msg->to);
				strcpy(FBBHeader->ATBBS, Msg->via);
				strcpy(FBBHeader->BID, Msg->bid);

				// Set up R:Line, so se can add its length to the sise

				tm = gmtime(&Msg->datecreated);	
	
				FBBHeader->Size += sprintf_s(RLine, sizeof(RLine),"R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
					tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
					Msg->number, BBSName, HRoute, RlineVer);

				// If using B2 forwarding we need the message size and Compressed size for FC proposal

				if (conn->BBSFlags & FBBB2Mode)
					CreateB2Message(conn, FBBHeader, RLine);

				if (conn->FBBIndex == 5  || TotalSize > user->ForwardingInfo->MaxFBBBlockSize)
					return TRUE;							// Got max number or too big

				Found = TRUE;								// Remember we have some
			}
			else
			{
				conn->FwdMsg = Msg;
				return TRUE;
			}
		}
	}

	return Found;
}

BOOL SeeifMessagestoForward (int BBSNumber)
{
	// See if any messages are queued for this BBS

	int m;
	struct MsgInfo * Msg;

	for (m = FirstMessageIndextoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		if ((Msg->status != 'H') && Msg->type && check_fwd_bit(Msg->fbbs, BBSNumber))
			return TRUE;
	}

	return FALSE;
}

int CountMessagestoForward (int BBSNumber)
{
	// See if any messages are queued for this BBS

	int m, n=0;
	struct MsgInfo * Msg;

	for (m = FirstMessageIndextoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		if ((Msg->status != 'H') && Msg->type && check_fwd_bit(Msg->fbbs, BBSNumber))
			n++;
	}

	return n;
}
int check_fwd_bit(char *mask, int bbsnumber)
{
	return (mask[(bbsnumber - 1) / 8] & (1 << ((bbsnumber - 1) % 8)));
}


void set_fwd_bit(char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] |= (1 << ((bbsnumber - 1) % 8));
}


void clear_fwd_bit (char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] &= (~(1 << ((bbsnumber - 1) % 8)));
}


VOID SendMessageToSYSOP(char * Title, char * MailBuffer, int Length)
{
	struct MsgInfo * Msg = AllocateMsgRecord();
	BIDRec * BIDRec;

	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int WriteLen=0;

	Msg->length = Length;

	GetSemaphore(&MsgNoSemaphore);
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;

	FreeSemaphore(&MsgNoSemaphore);
 
	strcpy(Msg->from, "SYSTEM");
	strcpy(Msg->to, "SYSOP");
	strcpy(Msg->title, Title);

	Msg->type = 'P';
	Msg->status = 'N';
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	BIDRec = AllocateBIDRecord();
	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

	sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes", MailDir, Msg->number);
	
	hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, MailBuffer, Msg->length, &WriteLen, NULL);
		CloseHandle(hFile);
	}

	free(MailBuffer);
}

struct UserInfo * FindRMS()
{
	struct UserInfo * bbs;
	
	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		if (strcmp(bbs->Call, "RMS") == 0)
			return bbs;
	}
	
	return NULL;
}

int CountConnectionsOnPort(int CheckPort)
{
	int n, Count = 0;
	CIRCUIT * conn;
	int port, sesstype, paclen, maxframe, l4window;
	char callsign[11];

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active)
		{
			GetConnectionInfo(conn->BPQStream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);
			if (port == CheckPort)
				Count++;
		}
	}

	return Count;
}

/*
VOID FindNextRMSUser(struct BBSForwardingInfo * FWDInfo)
{
	struct UserInfo * user;

	int i = FWDInfo->UserIndex;

	if (i == -1)
	{
		FWDInfo->UserIndex = FWDInfo->UserCall[0] = 0;	// Not scanning users
	}

	for (i++; i <= NumberofUsers; i++)
	{
		user = UserRecPtr[i];

		if (user->flags & F_POLLRMS)
		{
			FWDInfo->UserIndex = i;
			strcpy(FWDInfo->UserCall, user->Call);
			FWDInfo->FwdTimer = FWDInfo->FwdInterval - 20;
			return ;
		}
	}

	// Finished Scan

	FWDInfo->UserIndex = FWDInfo->FwdTimer = FWDInfo->UserCall[0] = 0;	
}
*/

#ifndef NEWROUTING

VOID SetupHAddreses(struct BBSForwardingInfo * ForwardingInfo)
{
}
VOID SetupMyHA()
{
}
VOID SetupFwdAliases()
{
}

int MatchMessagetoBBSList(struct MsgInfo * Msg, CIRCUIT * conn)
{
	struct UserInfo * bbs;
	struct	BBSForwardingInfo * ForwardingInfo;
	char ATBBS[41];
	char * HRoute;
	int Count =0;

	strcpy(ATBBS, Msg->via);
	HRoute = strlop(ATBBS, '.');

	if (Msg->type == 'P')
	{
		// P messages are only sent to one BBS, but check the TO and AT of all BBSs before routing on HA

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSToList(Msg, bbs, ForwardingInfo))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSAtList(Msg, bbs, ForwardingInfo, ATBBS))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSHList(Msg, bbs, ForwardingInfo, ATBBS, HRoute))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		return FALSE;
	}

	// Bulls go to all matching BBSs, so the order of checking doesn't matter

	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		ForwardingInfo = bbs->ForwardingInfo;

		if (CheckABBS(Msg, bbs, ForwardingInfo, ATBBS, HRoute))		
		{
			if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
			{
				if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
			}
			Count++;
		}
	}

	return Count;
}
BOOL CheckABBS(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** Calls;
	char ** HRoutes;
	int i, j;

	if (strcmp(ATBBS, bbs->Call) == 0)					// @BBS = BBS
		return TRUE;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}

	// Check AT distributions

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}


	return FALSE;

}

BOOL CheckBBSToList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo)
{
	char ** Calls;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}

BOOL CheckBBSAtList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS)
{
	char ** Calls;

	// Check AT distributions

	if (strcmp(ATBBS, bbs->Call) == 0)			// @BBS = BBS
		return TRUE;

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}

BOOL CheckBBSHList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** HRoutes;
	int i, j;

	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}
	return FALSE;
}

#endif

