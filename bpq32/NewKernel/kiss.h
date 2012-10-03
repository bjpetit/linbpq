
#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include <io.h>
#include <memory.h>

#define MAXBLOCK        512


typedef struct tagKISSINFO
{
   HANDLE  idComDev ;
   BYTE    bPort;
   DWORD   dwBaudRate ;
	SOCKET	sock;			// for KISS over UDP
	SOCKADDR_IN destaddr;
	struct PORTCONTROL * Portvector;
	DWORD		RXvector;

} KISSINFO, NEAR *NPKISSINFO ;

NPKISSINFO KISSInfo[33] = {0};


#define _fmemset   memset
#define _fmemmove  memmove

// function prototypes (private)

NPKISSINFO CreateKISSINFO( int port, int speed );
BOOL NEAR DestroyKISSINFO(NPKISSINFO npKISSINFO) ;
int NEAR ReadCommBlock(NPKISSINFO npKISSINFO, LPSTR lpszBlock, int nMaxLength);
BOOL NEAR WriteCommBlock(NPKISSINFO npKISSINFO, LPSTR lpByte, DWORD dwBytesToWrite);
BOOL NEAR OpenConnection(NPKISSINFO npKISSINFO, int port);
BOOL NEAR SetupConnection(NPKISSINFO npKISSINFO);
BOOL NEAR CloseConnection(NPKISSINFO npKISSINFO);

//---------------------------------------------------------------------------
//  End of File: tty.h
//---------------------------------------------------------------------------
