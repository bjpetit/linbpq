//
//	DLL to get list of adapters - used in BPQ32 BPQEther Config Utility
//

//  Version 1.0	January 2008
//

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include <stdio.h>
#include <process.h>
#include "Iphlpapi.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

HANDLE hInstance;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	hInstance=hInst;

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
}
PIP_ADAPTER_INFO pAdapterInfo;
PIP_ADAPTER_INFO pAdapter = NULL;
DWORD dwRetVal = 0;

DllExport int APIENTRY GetAdapterList()
{	
	UINT ulOutBufLen;

	pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
	ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) {
		free (pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen);
	}

	pAdapter = pAdapterInfo;

	return GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);	
}
DllExport int APIENTRY GetNextAdapter(unsigned char * name, unsigned char * desc, unsigned char * addr)	
{
	if (pAdapter)
	{
	  	strcpy(name, pAdapter->AdapterName);
		strcpy(desc, pAdapter->Description);
 		memcpy(addr, pAdapter->Address, 6); 
 
		pAdapter = pAdapter->Next;
		if (pAdapter==0) free (pAdapterInfo);

		return 0;
	}
	else
		return 1;
}

	
