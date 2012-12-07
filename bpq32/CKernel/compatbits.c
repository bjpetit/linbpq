/*

Stuff to make compiling on WINDOWS and LINUX easier

*/

#ifdef WIN32

#else

#include <stdio.h>
#include <ctype.h>

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
  while (*p = toupper( *p )) p++;
  return s;
}

#include <pthread.h>


unsigned long _beginthread(void( *start_address)(), unsigned stack_size, VOID * arglist)
{
	pthread_t thread;

	pthread_create(&thread, NULL, start_address, (void*) arglist);
	return thread;
}

#endif
