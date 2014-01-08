#include "CHeaders.h"
#include "tncinfo.h"

// For MAX OS/X


//int SENDNODES() {return 0;}
//int BYECMD() {return 0;}
//int CMDR00() {return 0;}

//int DoNetromConnect() {return 0;}


//int CMDC00() {return 0;}



//int CheckReceivedData() {return 0;}
//int GetLastError() {return 0;}



VOID ETHERExtInit()
{
}

#if __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 1060
char *gcvt(double number, size_t ndigit, char *buf)
{
	sprintf(buf,"%f.6", number);
	return buf;
}
#endif
