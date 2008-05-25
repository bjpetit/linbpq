
#define WIN32_LEAN_AND_MEAN
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>


FARPROC lpDllEntryPoint;
HINSTANCE hLib;
int ret;

int main(int argc, char **argv)
{
	printf ("G8BPQ .ocx Unregistration Utility Version 1.0 December 2007\n\n");

	if (argc == 1) 
	{
		printf ("Usage: bpqregsvr contol.ocx\n\n");
		return 0;
	}
	
	hLib = LoadLibrary(argv[1]);

	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
	{
		printf ("Could not load .dll %s Error Code %d",argv[1],GetLastError());
		return 0;						 //unable to load DLL
	}

// Find the entry point.

	lpDllEntryPoint = GetProcAddress(hLib, "DllUnregisterServer");


	if (lpDllEntryPoint == NULL)

		printf ("Could find entry point DllUnregisterServer Error %d",GetLastError());

	ret = (*lpDllEntryPoint)();

	return 0;

}

  
