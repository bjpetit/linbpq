
//////////////////////////////////////////////////////////////
//
// FBB driver for telnet TCP-IP access
//
// File : fbb_tln.dll
//
// (C) F6FBB 1999-2000
//
//////////////////////////////////////////////////////////////
//
//  Version history
//
//  1.5 - 25/11/2000 - EB5AGF
//		Password was not processed if received in the same frame
//		of callsign (forward session).
//
//  1.6 - 06/12/2000 - EB5AGF
//		Hostname lookup
//
//	    - 06/01/2001 - EB5AGF
//		Added version resource and code to read it at runtime
//
//////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <winsock.h>
#include <winver.h>
#include "fbb_drv.h"

#define DISCONNECTED	0
#define CONNECTED		1
#define CONNECT_PENDING 2
#define WAITING_CALL	3
#define WAITING_PASS	4
#define CONNECT_CALL	5

#define	FBBDLLNAME	"tln.dll"

//------------

int TLN_MAJOR = 0;
int TLN_MINOR = 0;

typedef struct DATAOUT_t {
	char	*pData;
	int		nChannel;
	int		nType;
	int		nLen;
	struct	DATAOUT_t *pNext;
} DATAOUT;

typedef struct DATAIN_t {
	char	*pData;
	int		nChannel;
	int		nType;
	int		nLen;
	DRVUI	*pUi;
	struct	DATAIN_t *pNext;
} DATAIN;

typedef struct CHANNEL_t {
	SOCKET	hSock;
	int		nChan;
	int		nState;
	int		nLastState;
	int		nLastNbFrames;
	int		nFrameNb;
	int		nErrors;
	BOOL	bBinary;
	BOOL	bReadOnly;
	char	szMyCall[10];
	char	szCall[100];
	char	szLine[256];
	int		nLinePos;
} CHANNEL;

typedef struct FBBDRV_t
{
	HWND	hWnd;
	int		nBufSize;
	int		nPort;
	int		nNbChan;
	int		nMyTlnPort;
	SOCKET	hConnSock;
	HANDLE	hThread;
	DWORD	dwThreadId;
	BOOL	bNext;
	BOOL	bUp;
	DRVUI	*pUi;
	DATAOUT	*pTncHead;	// Head of data_out list
	DATAOUT	*pTncTail;	// Tail of data_out list
	DATAIN	*pHostHead;	// Head of data_in list
	DATAIN	*pHostTail;	// Tail of data_in list
	CRITICAL_SECTION	hCritical;
	char	szMyCall[10];
	CHANNEL	*pChan;
	struct	FBBDRV_t *pNext;
} FBBDRV;


//////////////////////////////////////////////////////////////
// Local prototypes
//////////////////////////////////////////////////////////////

static DWORD WINAPI TlnLoop( LPSTR lpData );
static FBBDRV *SearchPort(int nPort);
static DATAIN *GetHost(FBBDRV *pFbbDrv);
static BOOL ToTnc(FBBDRV *pFbbDrv, int nChannel, int nType, char *pData, int nLen, DRVUI *pUi);
static LPSTR SockerrToString(DWORD serr );
static int SockMsgBox(UINT  fuType, LPSTR pszFormat,	... );
static BOOL TlnInit(FBBDRV *pFbbDrv, char *pPtr);
static BOOL Disconnected(FBBDRV *pFbbDrv, int nChan);
static BOOL ConnectTo(FBBDRV *pFbbDrv, int nChan, char *szRemoteCall);
static void ToHost(FBBDRV *pFbbDrv, int nChannel, int nType, char *pData, int nLen, DRVUI *pUi);
static void TlnOut(FBBDRV *pFbbDrv, int nChan, char *szBuf, int nLen);
static BOOL Disconnect(FBBDRV *pFbbDrv, int nChan);
static BOOL GetDrvVersion(int *nVMaj, int *nVMin);

//////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////

FBBDRV	*pDrvHead = NULL;	// Head of port structures

//////////////////////////////////////////////////////////////
//
// Exported functions (5).
// They are :
// BOOL OpenFbbDriver(int nPort, HWND hWnd, void *pDrvInfo);
// BOOL CloseFbbDriver(int nPort);
// BOOL ReadFbbDriver(int nPort, int *nType, int *nChan, char *pszBuf, int *nLen, DRVUI *pUi);
// BOOL WriteFbbDriver(int nPort, int *nType, int nChan, char *pszBuf, int nLen, DRVUI *pUi);
// BOOL StatFbbDriver(int nPort, int nChan, int nCmd, void *pPtr, int nLen);
//
//////////////////////////////////////////////////////////////


BOOL APIENTRY DllMain( HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
    switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
		GetDrvVersion(&TLN_MAJOR, &TLN_MINOR);
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		// Clean all linked ports
		while (pDrvHead)
			CloseFbbDriver(pDrvHead->nPort);
		break;
    }
    return TRUE;
}


BOOL WINAPI OpenFbbDriver(int nPort, HWND hWnd, void *pDrvInfo)
{
	int		nBufSize;
	int		nOptLen;
	FBBDRV	*pFbbDrv;
	HANDLE	hFbbThread;
	DWORD	dwThreadID;
	DRVINFO *pInfo;
	
	DWORD 	serr;
	WSADATA	wsadata;
	SOCKET	hSock;
	SOCKADDR_IN SockAddr;
	
	// Already used ?
	pFbbDrv = SearchPort(nPort);
	if (pFbbDrv)
		return FALSE;
	
	pInfo = (DRVINFO *)pDrvInfo;
	
	// Start the DLL if not already done
	if (pDrvHead == NULL)
	{

#define DESIRED_WINSOCK_VERSION         0x0101  // we'd like winsock ver 1.1...
#define MINIMUM_WINSOCK_VERSION         0x0001  // ...but we'll take ver 1.0

		serr = WSAStartup( DESIRED_WINSOCK_VERSION, &wsadata );
		if( serr != 0 )
		{
			SockMsgBox(	MB_ICONSTOP | MB_OK,
						"Cannot initialize socket library, error %d: %s",
						serr,
						SockerrToString( serr ) );

			return FALSE;
		}

		if( wsadata.wVersion < MINIMUM_WINSOCK_VERSION )
		{
			SockMsgBox(	MB_ICONSTOP | MB_OK,
					"Windows Sockets version %02X.%02X, I need at least %02X.%02X",
					LOBYTE(wsadata.wVersion),
					HIBYTE(wsadata.wVersion),
					LOBYTE(MINIMUM_WINSOCK_VERSION),
					HIBYTE(MINIMUM_WINSOCK_VERSION) );

			return FALSE;
		}

		if (hWnd)
		{
			WM_REQUEST_MSG = RegisterWindowMessage(FBBDRV_REQUEST);
			WM_NOTIFY_MSG = RegisterWindowMessage(FBBDRV_NOTIFY);
		}
		
	}

	// Create the listening socket
	hSock = socket(AF_INET, SOCK_STREAM, 0);
	if (hSock == INVALID_SOCKET)
	{
		return FALSE;
	}
	
	//
	//  Bind an address to the socket.
	//
	
	SockAddr.sin_family      = AF_INET;
	SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	SockAddr.sin_port        = htons(pInfo->nBaudrate);
	
	serr = bind(hSock, (SOCKADDR FAR *)&SockAddr, sizeof(SOCKADDR_IN));
	if (serr != 0 )
	{
		SockMsgBox(	MB_ICONSTOP | MB_OK, "bind con_sock : %s", SockerrToString(serr));
		closesocket(hSock);
		return FALSE;
	}

	serr = listen(hSock, 5);
	if (serr != 0 )
	{
		//
		//  Cannot listen on the socket.
		//
		SockMsgBox(	MB_ICONSTOP | MB_OK, "listen con_sock : %s", SockerrToString(serr));
		closesocket(hSock);
		return FALSE;
	}

	pFbbDrv = (FBBDRV *)LocalAlloc(LPTR, sizeof(FBBDRV));
	if (pFbbDrv == NULL)
		return FALSE;
	
	// Size of the send buffer
	nOptLen = sizeof(int);
	getsockopt(hSock, SOL_SOCKET, SO_SNDBUF, (char *)&nBufSize, &nOptLen);
	
	strcpy(pFbbDrv->szMyCall, pInfo->szMyCall);
	pFbbDrv->nBufSize	= nBufSize;
	pFbbDrv->pUi		= NULL;
	pFbbDrv->bNext		= FALSE;
	pFbbDrv->hConnSock	= hSock;
	pFbbDrv->bUp		= TRUE;
	pFbbDrv->hWnd		= hWnd;
	pFbbDrv->nPort		= nPort;
	pFbbDrv->nMyTlnPort	= pInfo->nBaudrate;
	pFbbDrv->nNbChan	= pInfo->nNbChan;
	
	strcpy(pFbbDrv->szMyCall, pInfo->szMyCall);
	
	// Includes the unused channel 0
	pFbbDrv->pChan      = (CHANNEL *)LocalAlloc(LPTR, (pFbbDrv->nNbChan+1) * sizeof(CHANNEL));
	if (pFbbDrv->pChan == NULL)
	{
		closesocket(pFbbDrv->hConnSock);
		LocalFree(pFbbDrv);
		return FALSE;
	}
	ZeroMemory(pFbbDrv->pChan, (pFbbDrv->nNbChan+1) * sizeof(CHANNEL));
	
	// Insert the structure in the list
	pFbbDrv->pNext = pDrvHead;
	pDrvHead = pFbbDrv;
	
	InitializeCriticalSection(&pFbbDrv->hCritical);
	
	// Create a thread to process the input frames.
	hFbbThread = CreateThread( 
		(LPSECURITY_ATTRIBUTES) NULL,
		0,
		(LPTHREAD_START_ROUTINE) TlnLoop,
		(LPVOID) pFbbDrv,
		0, 
		&dwThreadID );
	
	
	if (hFbbThread == NULL)
	{
		closesocket(pFbbDrv->hConnSock);
		DeleteCriticalSection(&pFbbDrv->hCritical);
		
		// Remove structure from list
		pDrvHead = pDrvHead->pNext;

		// Free the structure
		LocalFree(pFbbDrv);

		return FALSE;
	}
	
	pFbbDrv->dwThreadId	= dwThreadID;
	pFbbDrv->hThread	= hFbbThread;
	
	return TRUE;
	
} // end of OpenFbbDriver()

BOOL WINAPI CloseFbbDriver(int nPort)
{
	int nNb;
	FBBDRV *pFbbDrv = SearchPort(nPort);
	if (pFbbDrv == NULL)
		return FALSE;

	// block until thread has been halted
	pFbbDrv->bUp = FALSE;
	WaitForSingleObject(pFbbDrv->hThread, INFINITE);
	
	DeleteCriticalSection(&pFbbDrv->hCritical);
	
	// Close all opened sockets
	closesocket(pFbbDrv->hConnSock);

	// Close all connected channels
	for (nNb = 0 ; nNb <= pFbbDrv->nNbChan ; nNb++)
	{
		if (pFbbDrv->pChan[nNb].nState != DISCONNECTED)
			closesocket(pFbbDrv->pChan[nNb].hSock);
	}

	// Remove structure from list
	if (pDrvHead == pFbbDrv)
		pDrvHead = pDrvHead->pNext;
	else
	{
		FBBDRV *pTmp = pDrvHead;
		while (pTmp->pNext)
		{
			if (pTmp->pNext == pFbbDrv)
			{
				pTmp->pNext = pTmp->pNext->pNext;
				break;
			}
			pTmp = pTmp->pNext;
		}
	}
	
	// Close the Windows socket library if last
	if (pDrvHead == NULL)
		WSACleanup();
	
	LocalFree(pFbbDrv);
	
	return ( TRUE ) ;
	
} // end of CloseFbbDriver()

BOOL WINAPI ReadFbbDriver(int *nPort, int *nType, int *nChan, char *pszBuf, int *nLen, DRVUI *pUi)
{
	DATAIN *pPtr;
	FBBDRV *pFbbDrv = SearchPort(*nPort);
	if (pFbbDrv == NULL)
		return FALSE;
	
	pPtr = GetHost(pFbbDrv);
	if (pPtr == NULL)
		return FALSE;
	
	if (pPtr->nLen)
	{
		memcpy(pszBuf, pPtr->pData, pPtr->nLen);
		LocalFree(pPtr->pData);
	}
	*nLen = pPtr->nLen;
	*nChan = pPtr->nChannel;
	*nType = pPtr->nType;
	
	if (pPtr->pUi)
	{
		*pUi = *pPtr->pUi;
		LocalFree(pPtr->pUi);
	}
	LocalFree(pPtr);
	
	return TRUE;
} // end of ReadFbbDriver()

BOOL WINAPI WriteFbbDriver(int nPort, int nType, int nChan, char *pszBuf, int nLen, DRVUI *pUi)
{
	FBBDRV *pFbbDrv = SearchPort(nPort);
	if (pFbbDrv == NULL)
		return FALSE;
	
	return ToTnc(pFbbDrv, nChan, nType, pszBuf, nLen, pUi);
} // end of WriteFbbDriver()

BOOL WINAPI StatFbbDriver(int nPort, int nCmd, int nChan, void *pPtr, int nLen)
{
	FBBDRV *pFbbDrv = SearchPort(nPort);
	if (pFbbDrv == NULL)
		return 0;
	
	switch (nCmd)
	{
	case DRV_INIT:
		return TlnInit(pFbbDrv, (char *)pPtr);
	case DRV_SNDCMD:
	case DRV_ECHOCMD:
		return (ToTnc(pFbbDrv, nChan, DRV_COMMAND, (char *)pPtr, strlen((char *)pPtr), NULL));
	case DRV_PORTCMD:
		return (ToTnc(pFbbDrv, 0, DRV_COMMAND, (char *)pPtr, strlen((char *)pPtr), NULL));
	case DRV_PACLEN:
		*((int *) pPtr) = 250;
		return TRUE;
	case DRV_SYSRDY:
		// System is ready to accept connections
		return TRUE;
	case DRV_VERSION:
		wsprintf((char *)pPtr, 
			"v%d.%02d FBB driver for TELNET (F6FBB-%s)", 
			TLN_MAJOR, TLN_MINOR, __DATE__);
		return TRUE;
	}
	
	return FALSE;
} // end of StatFbbDriver()


//////////////////////////////////////////////////////////////
//
// Local functions
//
//////////////////////////////////////////////////////////////

static BOOL TlnInit(FBBDRV *pFbbDrv, char *pPtr)
{
	char cCmd = *pPtr++;

	while (*pPtr && isspace(*pPtr))
		++pPtr;

	return FALSE;
}

static BOOL TlnCommand(FBBDRV *pFbbDrv, int nChan, char *pPtr)
{
	char cCmd = *pPtr++;

	while (*pPtr && isspace(*pPtr))
		++pPtr;

	switch (toupper(cCmd))
	{
	case 'C':
		// Connect
		if (nChan > 0)
			return ConnectTo(pFbbDrv, nChan, pPtr);
		break;
	case 'D':
		// Disconnect
		if (nChan > 0)
			return Disconnected(pFbbDrv, nChan);
		break;
	}

	return FALSE;
}

static BOOL Disconnect(FBBDRV *pFbbDrv, int nChan)
{
	closesocket(pFbbDrv->pChan[nChan].hSock);
					
	// Clean the channel structure
	memset(&pFbbDrv->pChan[nChan], 0, sizeof(CHANNEL));
	pFbbDrv->pChan[nChan].nState = DISCONNECTED;

	return TRUE;
}

static BOOL Disconnected(FBBDRV *pFbbDrv, int nChan)
{
	char szBuf[256];

	if (nChan < 0 || nChan > pFbbDrv->nNbChan)
		return FALSE;

	// Send disconnect message
	wsprintf(szBuf, "(%d) DISCONNECTED fm %s", nChan, pFbbDrv->pChan[nChan].szCall);
	ToHost(pFbbDrv, nChan, DRV_COMMAND, szBuf, -1, NULL);

	return Disconnect(pFbbDrv, nChan);
}

static BOOL ConnectTo(FBBDRV *pFbbDrv, int nChan, char *szRemoteCall)
{
	unsigned long nArg;
	SOCKET hSock;
	SOCKADDR_IN SockAddrIn;
	char szCall[256];
	char szInet[256];
	int nPort;
	int nNb;

	if (pFbbDrv->pChan[nChan].nState != DISCONNECTED)
	{
		ToHost(pFbbDrv, nChan, DRV_COMMAND, "CHANNEL ALREADY CONNECTED", -1, NULL);
		return TRUE;
	}

	// Default to port 23
	nPort = 23;
	nNb = sscanf(szRemoteCall, "%s %s %d", szCall, szInet, &nPort);
	if (nNb < 2)
		return FALSE;

	// Open the socket
	hSock = socket(AF_INET, SOCK_STREAM, 0);
	if (hSock == INVALID_SOCKET)
		return FALSE;

	// Non blocking connection
	nArg = 1;
	ioctlsocket(hSock, FIONBIO, &nArg);

	SockAddrIn.sin_family      = AF_INET;
	SockAddrIn.sin_addr.s_addr = inet_addr(szInet);
	SockAddrIn.sin_port        = htons(nPort);

	// Name lookup
    struct hostent *pRemoteHost = NULL;
    pRemoteHost = gethostbyname(szInet);
	if(pRemoteHost)
		SockAddrIn.sin_addr.s_addr = *(unsigned long *) *(pRemoteHost->h_addr_list);
  

	if (connect(hSock, (SOCKADDR FAR *)&SockAddrIn, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			closesocket(hSock);
			return FALSE;
		}
	}

	// No character processing on output connection
	pFbbDrv->pChan[nChan].bBinary = TRUE;
	pFbbDrv->pChan[nChan].hSock = hSock;
	pFbbDrv->pChan[nChan].nState = CONNECT_PENDING;
	strcpy(pFbbDrv->pChan[nChan].szMyCall, pFbbDrv->szMyCall);
	strcpy(pFbbDrv->pChan[nChan].szCall, _strupr(szCall));

	return TRUE;
}

static FBBDRV *SearchPort(int nPort)
{
	FBBDRV *pTmp = pDrvHead;
	
	while (pTmp)
	{
		if (pTmp->nPort == nPort)
			break;
		pTmp = pTmp->pNext;
	}
	
	return pTmp;
}

static BOOL SendStats(FBBDRV *pFbbDrv, int nChannel)
{
	int nOptLen;
	int nBufSize;
	DRVSTATS DrvStats;
	BOOL bOk = FALSE;
				

	if (pFbbDrv->pChan[nChannel].hSock)
	{
		// Size of the send buffer only on valid sockets
		nOptLen = sizeof(int);
		getsockopt(pFbbDrv->pChan[nChannel].hSock, SOL_SOCKET, SO_SNDBUF, (char *)&nBufSize, &nOptLen);
		DrvStats.nNbFrames = (pFbbDrv->nBufSize - nBufSize + 249) / 250;
	}
	else
		DrvStats.nNbFrames = 0;
				
	DrvStats.nFrameNb = pFbbDrv->pChan[nChannel].nFrameNb;

	bOk |= (pFbbDrv->pChan[nChannel].nLastNbFrames != DrvStats.nNbFrames);
	pFbbDrv->pChan[nChannel].nLastNbFrames = DrvStats.nNbFrames;

	DrvStats.nNbRetry = 0;
	switch (pFbbDrv->pChan[nChannel].nState)
	{
	case CONNECTED :
		DrvStats.nState = DRV_CONNECTED;
		break;
	case DISCONNECTED :
	case CONNECT_PENDING :
	case WAITING_CALL :
	case WAITING_PASS :
	case CONNECT_CALL :
		DrvStats.nState = DRV_DISCONNECTED;
		break;
	}

	bOk |= (pFbbDrv->pChan[nChannel].nLastState != DrvStats.nState);
	pFbbDrv->pChan[nChannel].nLastState = DrvStats.nState;

	DrvStats.nBuffers = 500;	// Why not ?
				
	if (bOk)	// Send the message if any change
		ToHost(pFbbDrv, nChannel, DRV_STATS, (char *)&DrvStats, sizeof(DRVSTATS), NULL);

	return TRUE;
}

static BOOL ToTnc(FBBDRV *pFbbDrv, int nChannel, int nType, char *pData, int nLen, DRVUI *pUi)
{
	if (nLen <= 0 || nLen > 256)
		return FALSE;
	
	switch (nType)
	{
	case DRV_UNPROTO:
		break;
	case DRV_DATA:
		if (pFbbDrv->pChan[nChannel].nState == CONNECTED)
		{
			// Frame number.
			pFbbDrv->pChan[nChannel].nFrameNb = *((int *)pUi);

			// Output the frame
			TlnOut(pFbbDrv, nChannel, pData, nLen);
			++pFbbDrv->pChan[nChannel].nLastNbFrames;
			SendStats(pFbbDrv, nChannel);
		}
		break;
	case DRV_COMMAND:
		return TlnCommand(pFbbDrv, nChannel, pData);
	}
	
	return TRUE;
}

static void ToHost(FBBDRV *pFbbDrv, int nChannel, int nType, char *pData, int nLen, DRVUI *pUi)
{
	DATAIN *pPtr;
	
	if (nLen == -1)
		nLen = strlen(pData);
	
	if (nLen < 0 || nLen > 256)
		return;
	
	pPtr           = (DATAIN *)LocalAlloc( LMEM_FIXED, sizeof(DATAIN));
	pPtr->nChannel = nChannel;
	pPtr->nType    = nType;
	pPtr->nLen     = nLen;
	pPtr->pUi      = pUi;
	pPtr->pNext    = NULL;
	
	if (nLen > 0)
	{
		pPtr->pData    = (char *)LocalAlloc( LMEM_FIXED, nLen);
		memcpy(pPtr->pData, pData, nLen);
	}
	
	// Must be thread safe
	EnterCriticalSection(&pFbbDrv->hCritical);
	
	if (pFbbDrv->pHostHead == NULL)
		pFbbDrv->pHostHead = pPtr;
	else
		pFbbDrv->pHostTail->pNext = pPtr;
	
	// Update tail information
	pFbbDrv->pHostTail = pPtr;
	
	// Must be thread safe
	LeaveCriticalSection(&pFbbDrv->hCritical);
	
	// Send Notification message
	if (pFbbDrv->hWnd)
		PostMessage(pFbbDrv->hWnd, WM_NOTIFY_MSG, nType, MAKELONG(pFbbDrv->nPort, nChannel));
}

static DATAOUT *GetTnc(FBBDRV *pFbbDrv)
{
	DATAOUT *pPtr;
	
	// Must be thread safe
	EnterCriticalSection(&pFbbDrv->hCritical);
	
	pPtr = pFbbDrv->pTncHead;
	if (pPtr)
		pFbbDrv->pTncHead = pFbbDrv->pTncHead->pNext;
	
	// Must be thread safe
	LeaveCriticalSection(&pFbbDrv->hCritical);
	
	return pPtr;
}

static DATAIN *GetHost(FBBDRV *pFbbDrv)
{
	DATAIN *pPtr;
	
	// Must be thread safe
	EnterCriticalSection(&pFbbDrv->hCritical);
	
	pPtr = pFbbDrv->pHostHead;
	if (pPtr)
		pFbbDrv->pHostHead = pFbbDrv->pHostHead->pNext;
	
	// Must be thread safe
	LeaveCriticalSection(&pFbbDrv->hCritical);
	
	return pPtr;
}

static int TncQueueSize(FBBDRV *pFbbDrv)
{
	DATAOUT *pPtr;
	int nNb = 0;
	
	// Must be thread safe
	EnterCriticalSection(&pFbbDrv->hCritical);
	
	pPtr = pFbbDrv->pTncHead;
	while (pPtr)
	{
		pPtr = pPtr->pNext;
		++nNb;
	}
	
	// Must be thread safe
	LeaveCriticalSection(&pFbbDrv->hCritical);
	
	return nNb;
}

static void HostInfo(FBBDRV *pFbbDrv, int nChannel, char *sFormat, ...)
{
	char	szStr[512];
	va_list pArgPtr;
	int nCnt;
	
	va_start(pArgPtr, sFormat);
	nCnt = wvsprintf(szStr, sFormat, pArgPtr);
	va_end(pArgPtr);
	
	if (nCnt > 0)
		ToHost(pFbbDrv, nChannel, DRV_INFO, szStr, nCnt, NULL);
}

static BOOL InitPort(FBBDRV *pFbbDrv)
{
	int nNb;
	char *szInit[] = {
		"IF6FBB",
			"MIUSC",
			"I",
			"M",
			NULL
	};
	
	for (nNb = 0 ; szInit[nNb] ; nNb++)
		ToTnc(pFbbDrv, 0, DRV_COMMAND, szInit[nNb], strlen(szInit[nNb]), NULL);
	return TRUE;
}

static LPSTR SockerrToString( DWORD serr )
{
	switch( serr )
	{
	case WSAENAMETOOLONG :
		return "Name too long";
		
	case WSANOTINITIALISED :
		return "Not initialized";
		
	case WSASYSNOTREADY :
		return "System not ready";
		
	case WSAVERNOTSUPPORTED :
		return "Version is not supported";
		
	case WSAESHUTDOWN :
		return "Can't send after socket shutdown";
		
	case WSAEINTR :
		return "Interrupted system call";
		
	case WSAHOST_NOT_FOUND :
		return "Host not found";
		
	case WSATRY_AGAIN :
		return "Try again";
		
	case WSANO_RECOVERY :
		return "Non-recoverable error";
		
	case WSANO_DATA :
		return "No data record available";
		
	case WSAEBADF :
		return "Bad file number";
		
	case WSAEWOULDBLOCK :
		return "Operation would block";
		
	case WSAEINPROGRESS :
		return "Operation now in progress";
		
	case WSAEALREADY :
		return "Operation already in progress";
		
	case WSAEFAULT :
		return "Bad address";
		
	case WSAEDESTADDRREQ :
		return "Destination address required";
		
	case WSAEMSGSIZE :
		return "Message too long";
		
	case WSAEPFNOSUPPORT :
		return "Protocol family not supported";
		
	case WSAENOTEMPTY :
		return "Directory not empty";
		
	case WSAEPROCLIM :
		return "EPROCLIM returned";
		
	case WSAEUSERS :
		return "EUSERS returned";
		
	case WSAEDQUOT :
		return "Disk quota exceeded";
		
	case WSAESTALE :
		return "ESTALE returned";
		
	case WSAEINVAL :
		return "Invalid argument";
		
	case WSAEMFILE :
        return "Too many open files";
		
	case WSAEACCES :
		return "Access denied";
		
	case WSAELOOP :
		return "Too many levels of symbolic links";
		
	case WSAEREMOTE :
		return "The object is remote";
		
	case WSAENOTSOCK :
		return "Socket operation on non-socket";
		
	case WSAEADDRNOTAVAIL :
		return "Can't assign requested address";
		
	case WSAEADDRINUSE :
		return "Address already in use";
		
	case WSAEAFNOSUPPORT :
		return "Address family not supported by protocol family";
		
	case WSAESOCKTNOSUPPORT :
		return "Socket type not supported";
		
	case WSAEPROTONOSUPPORT :
		return "Protocol not supported";
		
	case WSAENOBUFS :
		return "No buffer space is supported";
		
	case WSAETIMEDOUT :
		return "Connection timed out";
		
	case WSAEISCONN :
		return "Socket is already connected";
		
	case WSAENOTCONN :
		return "Socket is not connected";
		
	case WSAENOPROTOOPT :
		return "Bad protocol option";
		
	case WSAECONNRESET :
		return "Connection reset by peer";
		
	case WSAECONNABORTED :
		return "Software caused connection abort";
		
	case WSAENETDOWN :
		return "Network is down";
		
	case WSAENETRESET :
		return "Network was reset";
		
	case WSAECONNREFUSED :
		return "Connection refused";
		
	case WSAEHOSTDOWN :
		return "Host is down";
		
	case WSAEHOSTUNREACH :
		return "Host is unreachable";
		
	case WSAEPROTOTYPE :
		return "Protocol is wrong type for socket";
		
	case WSAEOPNOTSUPP :
		return "Operation not supported on socket";
		
	case WSAENETUNREACH :
		return "ICMP network unreachable";
		
	case WSAETOOMANYREFS :
		return "Too many references";
		
	default :
		return "Unknown";
	 }
}

int SockMsgBox(UINT  fuType, LPSTR pszFormat,	... )
{
	CHAR    szOutput[256];
	va_list ArgList;
	
	va_start( ArgList, pszFormat );
	wvsprintf( szOutput, pszFormat, ArgList );
	va_end( ArgList );
	
	return MessageBox(NULL, szOutput, "Socket", fuType);
}

/*
static int TlnGetUi(FBBDRV *pFbbDrv, char *pPtr, DRVUI *pUi)
{
	int nLen = 0;
	char *pScan;
	char szTmp[80];
	
	// port is not used. Could be remapped in future evolutions
	while (*pPtr != '^')
	{
		++pPtr;
		++nLen;
	}
	++pPtr;
	++nLen;
	pUi->nPort = pFbbDrv->nPort;
	
	// from
	pScan = pUi->szFrom;
	while (*pPtr != '^')
	{
		*pScan++ = *pPtr++;
		++nLen;
	}
	*pScan = '\0';
	++pPtr;
	++nLen;
	
	// to
	pScan = pUi->szTo;
	while (*pPtr != '^')
	{
		if (*pPtr == ' ')
		{
			// Digis ...
			break;
		}
		*pScan++ = *pPtr++;
		++nLen;
	}
	*pScan = '\0';
	
	pScan = pUi->szVia;
	if (*pPtr == ' ')
	{
		// Digis ...
		++pPtr;
		++nLen;
		
		while (*pPtr != '^')
		{
			*pScan++ = *pPtr++;
			++nLen;
		}
		*pScan++ = ' ';
	}
	*pScan = '\0';
	++pPtr;
	++nLen;
	
	// ctrl
	pScan = pUi->szCtl;
	while (*pPtr != '^')
	{
		*pScan++ = *pPtr++;
		++nLen;
	}
	*pScan = '\0';
	pUi->bUi = (strncmp("UI", pUi->szCtl, 2) == 0);
	++pPtr;
	++nLen;
	
	// pid
	pScan = szTmp;
	while (*pPtr != '^')
	{
		*pScan++ = *pPtr++;
		++nLen;
	}
	*pScan = '\0';
	sscanf(szTmp, "%x", &pUi->nPid);
	++pPtr;
	++nLen;

	//Skip length
	while (*pPtr != '^')
	{
		*pScan++ = *pPtr++;
		++nLen;
	}
	++pPtr;
	++nLen;

	return nLen;
}
*/

static void TlnOut(FBBDRV *pFbbDrv, int nChan, char *szBuf, int nLen)
{
	if (pFbbDrv->pChan[nChan].bBinary)
		send(pFbbDrv->pChan[nChan].hSock, szBuf, nLen, 0);
	else
	{
		char *szTmp = (char *)LocalAlloc(LMEM_FIXED, nLen*2);
		if (szTmp)
		{
			int nI;
			int nL = 0;
			char *szScan = szTmp;

			for (nI = 0 ; nI < nLen ; nI++)
			{
				if (*szBuf == '\r')
				{
					*szScan++ = '\r';
					*szScan++ = '\n';
					nL += 2;
				}
				else if (*szBuf != '\n')
				{
					*szScan++ = *szBuf;
					nL++;
				}
				++szBuf;
			}
			send(pFbbDrv->pChan[nChan].hSock, szTmp, nL, 0);
			LocalFree(szTmp);
		}
	}
}

// Tln Thread

DWORD WINAPI TlnLoop(LPSTR lpData)
{
	FBBDRV	*pFbbDrv = (FBBDRV*) lpData ;
	struct timeval TimeOut;
	fd_set	ReadFs;
	fd_set	WriteFs;
	fd_set	ExcepFs;
	int nNb;
	char szBuf[400];
	
	// TimeOut is 1 second
	TimeOut.tv_sec  = 1;
	TimeOut.tv_usec = 0;
	
	while ( pFbbDrv->bUp )
	{
		FD_ZERO(&ReadFs);
		FD_ZERO(&WriteFs);
		FD_ZERO(&ExcepFs);
		
		// Mark listening socket
		FD_SET(pFbbDrv->hConnSock, &ReadFs);
		
		// Mark connected sockets
		for (nNb = 1 ; nNb <= pFbbDrv->nNbChan ; nNb++)
		{
			SendStats(pFbbDrv, nNb);
			switch (pFbbDrv->pChan[nNb].nState)
			{
			case CONNECTED:
			case WAITING_CALL:
			case WAITING_PASS:
			case CONNECT_CALL:
				FD_SET(pFbbDrv->pChan[nNb].hSock, &ReadFs);
				break;
			case CONNECT_PENDING:
				FD_SET(pFbbDrv->pChan[nNb].hSock, &WriteFs);
				FD_SET(pFbbDrv->pChan[nNb].hSock, &ExcepFs);
				break;
			}
		}
		
		nNb = select(0, &ReadFs, &WriteFs, &ExcepFs, &TimeOut);
		if (nNb == 0)
		{
			// Timeout
			continue;
		}
		
		// Incoming data
		for (nNb = 1 ; nNb <= pFbbDrv->nNbChan ; nNb++)
		{
			if (FD_ISSET(pFbbDrv->pChan[nNb].hSock, &ExcepFs))
			{
				// Connect pending failed
				pFbbDrv->pChan[nNb].nState = DISCONNECTED;
				closesocket(pFbbDrv->pChan[nNb].hSock);
				
				wsprintf(szBuf, "(%d) LINK FAILURE with %s", nNb, pFbbDrv->pChan[nNb].szCall);
				ToHost(pFbbDrv, nNb, DRV_COMMAND,szBuf, -1, NULL);
				wsprintf(szBuf, "(%d) DISCONNECTED fm %s", nNb, pFbbDrv->pChan[nNb].szCall);
				ToHost(pFbbDrv, nNb, DRV_COMMAND,szBuf, -1, NULL);

				// Clean the channel structure
				memset(&pFbbDrv->pChan[nNb], 0, sizeof(CHANNEL));
			}
			if (FD_ISSET(pFbbDrv->pChan[nNb].hSock, &WriteFs))
			{
				unsigned long nArg;

				// Connect pending accepted
				pFbbDrv->pChan[nNb].nState = CONNECTED;

				// Back to blocking mode
				nArg = 0;
				ioctlsocket(pFbbDrv->pChan[nNb].hSock, FIONBIO, &nArg);

// Padded call with spaces to try to stop callsign corruption G8BPQ Jan 2009
				wsprintf(szBuf, "(%d) CONNECTED to %-9s", nNb, pFbbDrv->pChan[nNb].szCall);
				ToHost(pFbbDrv, nNb, DRV_COMMAND,szBuf, -1, NULL);

			}
			if (FD_ISSET(pFbbDrv->pChan[nNb].hSock, &ReadFs))
			{
				int nLen = recv(pFbbDrv->pChan[nNb].hSock, szBuf, 256, 0);
				if (nLen <= 0)
				{
					Disconnected(pFbbDrv, nNb);
				}
 				else
				{
					int nPos = pFbbDrv->pChan[nNb].nLinePos;
					int nPosI = 0;	
							
					if ((pFbbDrv->pChan[nNb].nState != WAITING_CALL) &&
						(pFbbDrv->pChan[nNb].nState != WAITING_PASS))
					{
						// Data received
						if (pFbbDrv->pChan[nNb].bBinary)
						{
							ToHost(pFbbDrv, nNb, DRV_DATA, szBuf, nLen, NULL);
						}
						else
						{
							int nI;
							int nLg;

							char *szLine = pFbbDrv->pChan[nNb].szLine;

							// process the received characters
							for (nLg = 0, nI = 0 ; nI < nLen ; nI++)
							{
								if (szBuf[nI] == '\n') // LineFeed
									continue;

								if (szBuf[nI] == '\0') // Null Character
									continue;

								if (szBuf[nI] == '\b') // BackSpace
								{
									if (nPos > 0)
									{
										nPos--;
										TlnOut(pFbbDrv, nNb, "\b \b", 3);
									}
									continue;
								}

								szLine[nPos++] = szBuf[nI];

								// Echo...
								TlnOut(pFbbDrv, nNb, szBuf+nI, 1);
								if (szBuf[nI] == '\r')
								{
									ToHost(pFbbDrv, nNb, DRV_DATA, szLine, nPos, NULL);
									nPos = 0;
								}
								else if (nPos == sizeof(pFbbDrv->pChan[nNb].szLine))
								{
									ToHost(pFbbDrv, nNb, DRV_DATA, szLine, nPos, NULL);
									nPos = 0;
								}
							}
						}
					}
										
					if (pFbbDrv->pChan[nNb].nState == WAITING_CALL)
					{
						int nI;
						char *pPtr;
						char *pScan = szBuf;
						BOOL bOk = FALSE;

						// Get the callsign
						pPtr = pFbbDrv->pChan[nNb].szLine + nPos;
						for (nI = 0; nI < nLen ; nI++)
						{
							if (pFbbDrv->pChan[nNb].nLinePos >= sizeof(pFbbDrv->pChan[nNb].szLine)-1)
								break;

							// Binary mode ?
							if ((*pScan == '.') && (pFbbDrv->pChan[nNb].nLinePos == 0))
							{
								pFbbDrv->pChan[nNb].bBinary = TRUE;
								++pScan;
								//--nLen;
							}
							else if (*pScan == '\r')
							{
								if(!pFbbDrv->pChan[nNb].bBinary)
									TlnOut(pFbbDrv, nNb, pScan, 1);

								bOk = TRUE;
								*pPtr = '\0';
								if((nI < nLen) && nI)		
									nPosI = nI+1;
								*szBuf = '\0';
								break;
							}
							else if (*pScan == '\b') // BackSpace
							{
								pScan++;
								if (nPos > 0)
								{
									--pPtr;
									--nPos;

									if(!pFbbDrv->pChan[nNb].bBinary)
										TlnOut(pFbbDrv, nNb, "\b \b", 3);
								}
								continue;
							}
							else if (isgraph((UCHAR)*pScan))
							{
								*pPtr++ = *pScan++;
								++nPos;
							}
							else
							{
								pScan++;
								continue;
							}

							if(!pFbbDrv->pChan[nNb].bBinary)
								TlnOut(pFbbDrv, nNb, pScan-1, 1);
						}

						if (bOk)
						{
							char szTmp[256];
							char *szAnswer;

							strcpy(pFbbDrv->pChan[nNb].szCall, _strupr(pFbbDrv->pChan[nNb].szLine));

							wsprintf(szTmp, "%s^password", pFbbDrv->pChan[nNb].szCall);
							switch(SendMessage(pFbbDrv->hWnd, WM_REQUEST_MSG, PASSWD_CHECK, (LPARAM)szTmp))
							{
							case PWDCHK_INVALID_CALLSIGN:
								// Invalid callsign -> ask again
								if (pFbbDrv->pChan[nNb].nErrors++ > 3)
								{
									Disconnect(pFbbDrv, nNb);
								}
								else
								{
									wsprintf(szTmp, "Invalid callsign \"%s\" !\r\n\r\nCallsign : ", pFbbDrv->pChan[nNb].szCall);
									//szAnswer = "Wrong callsign !\r\n\r\nCallsign : ";
									szAnswer = szTmp;
									send(pFbbDrv->pChan[nNb].hSock, szAnswer, strlen(szAnswer), 0);
								}
								break;
							case PWDCHK_UNKNOWN_CALLSIGN:
							case PWDCHK_NO_MODEMFLAG:
							case PWDCHK_WRONG_PASSWORD:
							case PWDCHK_ACCEPTED:
								// Now ask for password
								szAnswer = "Password : ";
								send(pFbbDrv->pChan[nNb].hSock, szAnswer, strlen(szAnswer), 0);
								pFbbDrv->pChan[nNb].nState = WAITING_PASS;
								pFbbDrv->pChan[nNb].nErrors = 0;
								break;
							case PWDCHK_ERROR:
							case PWDCHK_EXCLUDED_CALLSIGN:
							default:
								// Internal error or excluded callsign -> disconnect
								Disconnect(pFbbDrv, nNb);
								break;
							}
							nPos = 0;
						}
					}

					if (pFbbDrv->pChan[nNb].nState == WAITING_PASS)
					{
						int nI;
						char *pPtr;
						char *pScan = szBuf + nPosI;
						BOOL bOk = FALSE;

						// Get the password
						pPtr = pFbbDrv->pChan[nNb].szLine + nPos;
						for (nI = nPosI; nI < nLen ; nI++)
						{
							if (pFbbDrv->pChan[nNb].nLinePos >= sizeof(pFbbDrv->pChan[nNb].szLine)-1)
								break;

							if (*pScan == '\r')
							{
								//if(!pFbbDrv->pChan[nNb].bBinary)
									TlnOut(pFbbDrv, nNb, pScan, 1);

								*pPtr = '\0';
								bOk = TRUE;
								break;
							}
							else if (*pScan == '\b') // BackSpace
							{
								pScan++;
								if (nPos > 0)
								{
									--pPtr;
									--nPos;
									//if(!pFbbDrv->pChan[nNb].bBinary)
										TlnOut(pFbbDrv, nNb, "\b \b", 3);
								}
								continue;
							}
							else if (isgraph(*pScan))
							{
								*pPtr++ = *pScan++;
								++nPos;
							}
							else
							{
								pScan++;
								continue;
							}

							if(!pFbbDrv->pChan[nNb].bBinary)
								//TlnOut(pFbbDrv, nNb, pScan-1, 1);
								TlnOut(pFbbDrv, nNb, "*", 1);
						}

						if (bOk)
						{
							char szTmp[80];
							char *szAnswer;

							wsprintf(szTmp, "%s^%s", pFbbDrv->pChan[nNb].szCall, pFbbDrv->pChan[nNb].szLine);
							switch(SendMessage(pFbbDrv->hWnd, WM_REQUEST_MSG, PASSWD_CHECK, (LPARAM)szTmp))
							{
							case PWDCHK_UNKNOWN_CALLSIGN:
							case PWDCHK_NO_MODEMFLAG:
								szAnswer = "\r\nUnregistered callsign. Read-only access\r\n\r\n";
								send(pFbbDrv->pChan[nNb].hSock, szAnswer, strlen(szAnswer), 0);

								pFbbDrv->pChan[nNb].bReadOnly = TRUE;
								pFbbDrv->pChan[nNb].nState = CONNECTED;
								// Send connect message
								wsprintf(szBuf, "(%d) READONLY to %s", nNb, pFbbDrv->pChan[nNb].szCall);
								ToHost(pFbbDrv, nNb, DRV_COMMAND, szBuf, -1, NULL);
								break;
							case PWDCHK_ACCEPTED:
								pFbbDrv->pChan[nNb].nState = CONNECTED;
								// Send connect message				
								wsprintf(szBuf, "(%d) CONNECTED to %s", nNb, pFbbDrv->pChan[nNb].szCall);
								ToHost(pFbbDrv, nNb, DRV_COMMAND, szBuf, -1, NULL);
								break;
							case PWDCHK_WRONG_PASSWORD:
								// Wrong password
								if (pFbbDrv->pChan[nNb].nErrors++ > 3)
								{
									Disconnect(pFbbDrv, nNb);
								}
								else
								{
									szAnswer = "Password : ";
									send(pFbbDrv->pChan[nNb].hSock, szAnswer, strlen(szAnswer), 0);
								}
								break;
							case PWDCHK_ERROR:
							case PWDCHK_INVALID_CALLSIGN:
							case PWDCHK_EXCLUDED_CALLSIGN:
							default:
								// Internal error or excluded callsign -> disconnect
								Disconnect(pFbbDrv, nNb);
								break;
							}
							nPos = 0;
						}
					}

					pFbbDrv->pChan[nNb].nLinePos = nPos;
				}
			}
		}
		
		// New connection
		if (FD_ISSET(pFbbDrv->hConnSock, &ReadFs))
		{
			int nC;
			SOCKET hNewSock;
			
			hNewSock = accept(pFbbDrv->hConnSock, NULL, NULL);
			for (nC = 1 ; nC <= pFbbDrv->nNbChan	; nC++)
			{
				if (pFbbDrv->pChan[nC].nState == DISCONNECTED)
				{
					char szStr[80];

					pFbbDrv->pChan[nC].hSock = hNewSock;

					// Send the "Callsign" request
					wsprintf(szStr, "%s BBS - TELNET Access\r\n\r\nCallsign : ", pFbbDrv->szMyCall);
					send(hNewSock, szStr, strlen(szStr), 0);
					pFbbDrv->pChan[nC].nState = WAITING_CALL;
					pFbbDrv->pChan[nC].nLinePos = 0;
					break;
				}
			}
			
			if (nC > pFbbDrv->nNbChan)
			{
				// All channels are busy ... Cannot connect
				char *pPtr = "Sorry, no more channel available. Disconnecting...\r";

				// Send the "Sorry" answer
				send(hNewSock, pPtr, strlen(pPtr), 0);
				closesocket(hNewSock);
			}
		}
	}

//	pFbbDrv->dwThreadId = 0 ;
		
	return( TRUE ) ;
		
} // end of TlnLoop()

	
BOOL GetDrvVersion(int *nVMaj, int *nVMin)
{
	DWORD dwVersionInfoSize;
	DWORD dwTmpHandle;
	LPVOID lpVersionInfo; 
	BOOL bRet = false;


	dwVersionInfoSize = GetFileVersionInfoSize(FBBDLLNAME, &dwTmpHandle);

	if(dwVersionInfoSize)
	{
		lpVersionInfo = LocalAlloc(LPTR, dwVersionInfoSize);
		if(lpVersionInfo)
		{
			if(GetFileVersionInfo(FBBDLLNAME, dwTmpHandle, dwVersionInfoSize, lpVersionInfo))
			{
//				LPVOID lpBuffer = LocalAlloc(LPTR, dwVersionInfoSize);		// G8BPQ VerQueryValue returns pointer
				LPVOID lpBuffer;
				UINT dwBytes;

				if( VerQueryValue(lpVersionInfo, 
						TEXT("\\StringFileInfo\\000004B0\\FileVersion"), 
						&lpBuffer, 
						&dwBytes)) 
				{
						sscanf((TCHAR *)lpBuffer, "%d,%d", nVMaj, nVMin);
						bRet = true;
				}

//				LocalFree(lpBuffer);											// G8BPQ
			}

			LocalFree(lpVersionInfo);
 		}
 	}

	return bRet;
}
