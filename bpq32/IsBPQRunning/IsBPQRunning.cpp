// IsBPQRunning.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE Mutex;

	Mutex=OpenMutex(MUTEX_ALL_ACCESS,FALSE,"BPQLOCKMUTEX");

	if (Mutex == NULL)	
		return 0;
	else
	{
		CloseHandle(Mutex);
		return 1;
	}
}

