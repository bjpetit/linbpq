
//	Program to Sleep for the speicied time

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T
#define BPQICON 2

#include <windows.h>


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int Time = 1000;

	if (lpCmdLine[0]) Time = atoi(lpCmdLine);

	OutputDebugString("Pause Starting");
	Sleep(Time);
	OutputDebugString("Pause Finishing");
	return 0;
}

