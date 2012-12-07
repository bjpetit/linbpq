/*

Stuff to make compiling on WINDOWS and LINUX easier

*/


#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include "winsock2.h"
#include "WS2tcpip.h"

#define Dll	__declspec(dllexport)

#define ioctl ioctlsocket

#else

#define Dll
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <syslog.h>


#define BOOL int
#define VOID void
#define UCHAR unsigned char
#define USHORT unsigned short
#define ULONG unsigned long
#define UINT unsigned int
#define SHORT short
#define DWORD long
#define BYTE unsigned char
#define APIENTRY
#define WINAPI
#define WINUSERAPI
#define TCHAR char
#define TRUE 1
#define FALSE 0
#define FAR
#define byte UCHAR
#define Byte UCHAR
#define Word WORD

typedef DWORD   COLORREF;
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((USHORT)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#define GetRValue(rgb)      rgb & 0xFF
#define GetGValue(rgb)      (rgb >> 8) & 0xFF
#define GetBValue(rgb)      (rgb >> 16) & 0xFF


#define HWND unsigned int
#define HINSTANCE unsigned int
#define HKEY unsigned int
#define UINT_PTR unsigned int *

#define HANDLE UINT
#define SOCKET int

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

#define HMENU UINT
#define WNDPROC UINT
#define __cdecl

#define strtok_s strtok_r

#define _memicmp memicmp
#define _stricmp stricmp
#define _strdup strdup
#define _strupr strupr

int memicmp(unsigned char *a, unsigned char *b, int n);
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2);
char * strupr(char* s);

#define WSAGetLastError() errno 
#define closesocket close
#define GetCurrentProcessId getpid

#define LOBYTE(w)           ((BYTE)((ULONG *)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((ULONG *)(w) >> 8))

#endif

#ifdef LINBPQ
#ifdef APIENTRY
#undef APIENTRY
#endif
#define APIENTRY
#endif

