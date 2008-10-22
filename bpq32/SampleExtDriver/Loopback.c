//
//	Demo Loopback driver, using  BPQ EXTERNAL interface
//

//	Version 1.0 October 2008

//	This is a minimal driver. Each message sent to it is saved and returned on the next poll
//	Only one buffer is used, so driver must report TX Busy if a message is already queued.

#include "windows.h"

#include "AsmStrucs.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

DllExport int ExtProc(int fn, int port,unsigned char * buff);

BOOL MsgAvailable=FALSE;
char SavedMsg[340];
int MsgLen;


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	switch( ul_reason_being_called )
	{
	case DLL_PROCESS_ATTACH:
		
		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		return 1;
	}
	return 1;	
}

DllExport int APIENTRY ExtInit(struct PORTCONTROL * PortEntry)
{
	char msg[80];
	
	GetAPI();				// Get BPQ32 Proc Addresses

	wsprintf(msg,"Loopback Demo Port %d", PortEntry->PORTNUMBER);
	WritetoConsole(msg);

	return ((int) ExtProc);
}

DllExport int ExtProc(int fn, int port,unsigned char * buff)
{
	switch (fn)
	{
	case 1:				// poll

		if (MsgAvailable)
		{
			memcpy(&buff[7], SavedMsg, MsgLen);
			MsgLen+=7;

			buff[5]=(MsgLen & 0xff);
			buff[6]=(MsgLen >> 8);

			MsgAvailable=FALSE;

			return 1;
		}

		return 0;

		
	case 2:				// send
		
 		MsgLen=(buff[6]<<8) + buff[5];
		MsgLen-=7;

		memcpy(SavedMsg,&buff[7],MsgLen);

		MsgAvailable=TRUE;

		return (0);

	case 3:				// CHECK IF OK TO SEND

		//	As we only have one buffer, must report busy if buffer is in use
	
		if (MsgAvailable)
			return 1;	// Busy
		
		return (0);		// OK
			
		break;

	case 4:				// reinit

		return 0;

	case 5:				// Close

		return 0;
	}

	return (0);
}
