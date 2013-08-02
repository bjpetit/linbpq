/*

Stuff to make compiling on WINDOWS and LINUX easier

*/

#ifdef WIN32

#else

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <stdarg.h>

#define BOOL int

#define VOID void
#define UCHAR unsigned char
#define USHORT unsigned short
#define ULONG unsigned long
#define UINT unsigned int
#define SHORT short
#define DWORD long

#define APIENTRY

#define TRUE 1
#define FALSE 0
#define FAR

#define HWND unsigned int
#define HINSTANCE unsigned int

#define strtok_s strtok_r


int memicmp(unsigned char *a, unsigned char *b, int n)
{
	if (n)
	{
		while (n && toupper(*a) == toupper(*b))
			n--, a++, b++;

		if (n)
			return toupper(*a) - toupper(*b);
   }
   return 0;
}
int stricmp(const unsigned char * pStr1, const unsigned char *pStr2)
{
    unsigned char c1, c2;
    int  v;

	if (pStr1 == NULL)
	{
		if (pStr2)
			Debugprintf("stricmp called with NULL 1st param - 2nd %s ", pStr2);
		else
			Debugprintf("stricmp called with two NULL params");

		return 1;
	}


    do {
        c1 = *pStr1++;
        c2 = *pStr2++;
        /* The casts are necessary when pStr1 is shorter & char is signed */
        v = tolower(c1) - tolower(c2);
    } while ((v == 0) && (c1 != '\0') && (c2 != '\0') );

    return v;
}
char * strupr(char* s)
{
  char* p = s;

  if (s == 0)
	  return 0;

  while (*p = toupper( *p )) p++;
  return s;
}

char * strlwr(char* s)
{
  char* p = s;
  while (*p = tolower( *p )) p++;
  return s;
}

int sprintf_s(char * string, int plen, const char * format, ...)
{
	va_list(arglist);
	int Len;

	va_start(arglist, format);
	Len = vsprintf(string, format, arglist);
	va_end(arglist);
	return Len;
}



#include <pthread.h>

unsigned long _beginthread(void(*start_address)(), unsigned stack_size, VOID * arglist)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, (void * (*)(void *))start_address, (void*) arglist) != 0)
		perror("New Thread");
	else
		pthread_detach(thread);

	return thread;
}

int Sleep(int ms)
{
	usleep(ms * 1000);
	return 0;
}

VOID OutputDebugString(char * string)
{
	syslog(LOG_DEBUG, "%s", string);
}

#endif
