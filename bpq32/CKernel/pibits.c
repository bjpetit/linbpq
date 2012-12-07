#include "CHeaders.h"
#include "tncinfo.h"




//int SENDNODES() {return 0;}
//int BYECMD() {return 0;}
//int CMDR00() {return 0;}

//int DoNetromConnect() {return 0;}


//int CMDC00() {return 0;}



//int CheckReceivedData() {return 0;}
//int GetLastError() {return 0;}
int Sleep(int ms)
{
	usleep(ms * 1000);
	return 0;
}
//int WriteFile() {return 0;}
//int CloseHandle() {return 0;}

VOID OutputDebugString(char * string)
{
	syslog(LOG_DEBUG, string);
}



