
//	Glue to allow apps using 16 bit BPQ API to run in XP VDM
//
//	Called from 16 bit environment using BOP

//	Just maps calls to BPQAPI in BPQ32.dll

//	Replaces previous system using a DOS Device Driver and an NT VDD

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"

#define i386
#include "vddsvc.h"

//#define DllImport	__declspec( dllimport )
//#define DllExport	__declspec( dllexport )


unsigned short _ax;
unsigned short _bx;
unsigned short _cx;
unsigned short _dx;
unsigned short _si;
unsigned short _di;
unsigned short _es;


int Flag=(int) &Flag;			//	 for Dump Analysis
int MAJORVERSION=4;
int MINORVERSION=8;
int Semaphore = 0;

int InitDone=0;

int AttachedProcesses=0;

UCHAR StreamMap[65];
BOOL NotFirstUse[65];


UCHAR BPQDirectory[MAX_PATH];

HINSTANCE ExtDriver=0;

HKEY REGTREE = HKEY_CURRENT_USER;
char REGTREETEXT[100] = "HKEY_CURRENT_USER";

typedef int (FAR *FARPROCX)();

int *BPQAPI=0;


FARPROCX GETBPQAPI;
int (FAR WINAPI * FindFreeStream) ();
BOOL (FAR WINAPI * GetAllocationState) (int Stream);

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:
		
		AttachedProcesses++;

		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
		
		return 1;
    
	case DLL_PROCESS_DETACH:
	
				
		AttachedProcesses--;
		return 1;
    
	}
 
	return 1;
	
}
BOOL __declspec(dllexport) __cdecl InitialiseBPQ32()
{
	HANDLE Mutex;
	UCHAR Value[100];

	char bpq[]="BPQ32.exe";
	char *fn=(char *)&bpq;
	HKEY hKey=0;
	int i,ret,Type,Vallen=99;

	char Errbuff[100];
	char buff[20];

	STARTUPINFO  StartupInfo;	// pointer to STARTUPINFO 
    PROCESS_INFORMATION  ProcessInformation; 	// pointer to PROCESS_INFORMATION 
	
	// Initialise Stream Mapping Table
	
	for (i=0; i< 66; i++)
	{
		StreamMap[i] = i;
		NotFirstUse[i] = FALSE;
	}

	// See if BPQ32 is running - if we create it in the NTVDM address space by
	// loading bpq32.dll it will not work.

	Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

	if (Mutex == NULL)
	{	
		OutputDebugString("BPQ1632 bpq32.dll is not already loaded - Loading BPQ32.exe\n");

#pragma warning(push)
#pragma warning(disable : 4996)

		if (_winver < 0x0600)

#pragma warning(pop)
		{
			// Below Vista

			REGTREE = HKEY_LOCAL_MACHINE;
			strcpy(REGTREETEXT, "HKEY_LOCAL_MACHINE");
		}

		// Get address of BPQ Directory

		Value[0]=0;


		ret = RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

		if (ret == ERROR_SUCCESS)
		{
			ret = RegQueryValueEx(hKey,"BPQ Program Directory",0,			
								&Type,(UCHAR *)&Value,&Vallen);
		
			if (ret == ERROR_SUCCESS)
			{
				if (strlen(Value) == 2 && Value[0] == '"' && Value[1] == '"')
					Value[0]=0;
			}

			if (Value[0] == 0)
			{
		
				// BPQ Program Directory absent or = "" - "try Config File Location"
			
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

			strcpy(Errbuff,	"BPQ1632 Load ");
			strcat(Errbuff,Value);
			strcat(Errbuff," failed ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

			return FALSE;
				
		}

		// Wait up to 20 secs for BPQ32 to start

		for ( i = 0; i < 20; i++ ) 
		{
			Sleep(1000);

			Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

			if (!(Mutex == NULL))

				break;
		}
	
		if (Mutex == NULL)
		{
				
			OutputDebugString("BPQ1632 bpq32.exe failed to initialize within 20 seconds\n");

			return FALSE;

		}

	}
	
	CloseHandle(Mutex);


	ExtDriver=LoadLibrary("bpq32.dll");		// Will attach to existing process


	if (ExtDriver == NULL)
	{
				
		OutputDebugString("BPQ1632 Error Loading bpq32.dll\n");

		return FALSE;

	}

	GETBPQAPI = (FARPROCX)GetProcAddress(ExtDriver,"_GETBPQAPI@0");
	GetAllocationState = (int (__stdcall *)(int Stream))GetProcAddress(ExtDriver,"_GetAllocationState@4");
	FindFreeStream = (int (__stdcall *)())GetProcAddress(ExtDriver,"_FindFreeStream@0");

	
	if (GETBPQAPI == NULL)
	{
		OutputDebugString("BPQ1632 Error finding BPQ32 API Entry Point\n");

		return FALSE;
	}

	BPQAPI=(int *)GETBPQAPI();

	return TRUE;

}



BOOL Initialized=FALSE;
char Msg[100];

void __declspec(dllexport) __cdecl CallFrom16()
{
    LPVOID  Buffer;
	LPVOID  APIBuffer;
    ULONG   APIVDMAddress;
	UINT stamp;
	UINT Command, Stream, NewStream;

	if (!Initialized) Initialized=InitialiseBPQ32();

	if (!Initialized) return;

/*;
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

 
// BOP interface uses AX for Handle. Fortunately no BPQAPI calls
// use BX as an input param, so pass function in BH

		_ax=getBX();
		_cx=getCX();
		_dx=getDX();
		_si=getSI();
		_di=getDI();
		_es=getES();

		// Attempt to detect use of unallocated ports sessions.
		// The first time a stream is used, check if allocated. If it is, find a free stream and remap.

		Stream = (_ax & 0xff);
		Command = _ax>>8;

		if ((Command == 13) && (Stream == 0)) goto HOSTNOPORT;	// Find Free

		_asm {

		mov	ax,_ax

		CMP	AH,11
		JE SHORT NEEDHOSTPORT

		CMP	AH,13
		JE SHORT NEEDHOSTPORT

		CMP	AH,10
		JAE SHORT HOSTNOPORT
;
		CMP	AH,2
		JNE	NEEDHOSTPORT			; SEND ALLOWS ZERO STREAM, MEANING UNPROTO
	
		OR	AL,AL
		JZ SHORT HOSTNOPORT			;  BROADCAST STREAM

NEEDHOSTPORT:
;
;	Is this first use of this stream
;
	}
	

	if (NotFirstUse[Stream] == FALSE)
	{
		NotFirstUse[Stream] = TRUE;

		if (GetAllocationState(Stream) != 0)
		{
			// Stream in use - Find another

			NewStream = FindFreeStream();

			if (NewStream != 255)
			{
				StreamMap[Stream] = NewStream;
				wsprintf(Msg,"BPQ1632 Stream %d remapped to Stream %d\n", Stream, NewStream);
				OutputDebugString(Msg);
			}
		}
	}

	_ax = (_ax & 0xff00) | StreamMap[Stream];


HOSTNOPORT:

		switch (_ax>>8)
		{
			case 1:

				// ApplMask

				_dx = (_dx & 0xff);		// Mask to 8 Bits

			case 0:
			case 4:
 			case 5:
			case 6:
			case 7:
			case 13:
			case 15:

				// All params in registers so can pass directly to bpq32

			_asm {
				
				movzx	eax,_ax
				movzx	ecx,_cx
				movzx	edx,_dx
				
				call BPQAPI

				mov	_ax,ax
				mov	_bx,bx
				mov	_cx,cx
				mov	_dx,dx
				mov	_si,si
				mov	_di,di

				}

			if ((Command == 13) && (Stream == 0)) // Find Free
			{
				// Clear First Use of Allocated Stream

				if (_ax != 255) NotFirstUse[_ax] = TRUE;
				wsprintf(Msg,"BPQ1632 Find Free Allocated Stream %d\n", _ax);
				OutputDebugString(Msg);
			}

			setAX(_ax);
			setBX(_bx);
			setCX(_cx);
			setDX(_dx);
			setSI(_si);
			setDI(_di);

			return;

			case 2:

//	AH = 2	Send frame in ES:SI (length CX)

			case 10:

//	AH = 10	Unproto transmit frame.  Data pointed to by ES:SI, of
//		length CX, is transmitted as a HDLC frame on the radio
//		port (not stream) in AL.

			case 14:

//	AH = 14 Internal Interface for IP Router
//
//		Send frame - to NETROM L3 if DL=0
//			     to L2 Session if DL<>0
//

		        APIVDMAddress = (ULONG) (_es<<16 | _si);

				APIBuffer = (LPVOID) GetVDMPointer (APIVDMAddress, _cx, FALSE);
				
				_asm {


				movzx	eax,_ax
				movzx	ecx,_cx
				movzx	edx,_dx

				mov	esi,APIBuffer
				
				call BPQAPI

				}

	            FreeVDMPointer (APIVDMAddress, _cx, APIBuffer, FALSE);

				return;

			case 3:


//;	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
//;		in CX.  BX returns the number of outstanding frames still to
//;		be received (ie. after this one) or zero if no more frames
//;		(ie. this is last one).
//;
//;

				
		        APIVDMAddress = (ULONG) (_es<<16 | _di);

				APIBuffer = (LPVOID) GetVDMPointer (APIVDMAddress, 350, FALSE);

				_asm{

				mov	edi,APIBuffer
				movzx	eax,_ax
				
				call BPQAPI

				mov	_bx,bx
				mov	_cx,cx

				}

				setBX(_bx);
				setCX(_cx);

	            FreeVDMPointer (APIVDMAddress, 350, APIBuffer, FALSE);

				return;

			case 8:


//;AH = 8		Port control/information.  Called with a stream number
//;		in AL returns:
//;
//;		AL = Radio port on which channel is connected (or zero)
//;		AH = SESSION TYPE BITS
//;		BX = L2 paclen for the radio port
//;		CX = L2 maxframe for the radio port
//;		DX = L4 window size (if L4 circuit, or zero)
//;		ES:DI = CALLSIGN


  		        APIVDMAddress = (ULONG) (_es<<16 | _di);

				APIBuffer = (LPVOID) GetVDMPointer (APIVDMAddress, 10, FALSE);

				_asm{

					mov	edi,APIBuffer
					movzx	eax,_ax

					call BPQAPI

					mov	_ax,ax
					mov	_bx,bx
					mov	_cx,cx
					mov	_dx,dx
				}
			
				setAX(_ax);
				setBX(_bx);
				setCX(_cx);
				setDX(_dx);

	            FreeVDMPointer (APIVDMAddress, 10, APIBuffer, FALSE);

				return;



			case 11:


//			;	AH = 11 Get Trace (RAW Data) Frame into ES:DI,
//			;			 Length to CX, Timestamp to AX,frams still to get in BX
//			;				
//			;

		        APIVDMAddress = (ULONG) (_es<<16 | _di);

				APIBuffer = (LPVOID) GetVDMPointer (APIVDMAddress, 350, FALSE);

				_asm{

					mov	esi,Buffer

					movzx eax,_ax
					mov	edi,APIBuffer

					call BPQAPI

					mov	_bx,bx
					mov	_cx,cx
					mov	_dx,dx
	
					cmp	ecx,0
					je	rawnodata		// Nothing to copy

					mov	stamp,eax

//	Need to reformat - BPQ16 data offset and length are two bytes less
				
					mov	esi,APIBuffer
					mov	edi,APIBuffer
					inc esi
					inc esi
					sub	ecx,7		//  only have 16 bit chain word and moving 5 bytes below


					movsw
					movsb			// chain and port
					lodsw
					dec ax
					dec ax
					stosw			// reduce length
					rep movsb
				}

				 // Convert Timestamp
   
				stamp=stamp%86400;		// Secs into day
				stamp=stamp%3600;		// Secs into hour
    
				_ax=LOWORD(stamp/60);
				_ax+=LOWORD(stamp%60)<<8;

				_cx-=2;

rawnodata:
				
				setAX(_ax);
				setBX(_bx);
				setCX(_cx);

	            FreeVDMPointer (APIVDMAddress, 350, APIBuffer, FALSE);

				return;

			case 12:


//	AH = 12 Update Switch. At the moment only Beacon Text may be updated

//		DX = Function
//		     1=update BT. ES:SI, Len CX = Text
//		     2=kick off nodes broadcast
//

				if (_dx == 1)
				{
					APIVDMAddress = (ULONG) (_es<<16 | _si);

					APIBuffer = (LPVOID) GetVDMPointer (APIVDMAddress, _cx, FALSE);
				}

				_asm {

					mov	esi,Buffer

					movzx eax,_ax	
					movzx ecx,_cx
					movzx edx,_dx

					mov	esi,APIBuffer

					call BPQAPI

				}

		        FreeVDMPointer (APIVDMAddress, _cx, APIBuffer, FALSE);

				return;

        default:

		break;

    }
    return;
}

