
//////////////////////////////////////////////////////////////
//
// FBB driver for BPQ
//
// File : fbb_bpq.dll
//
// (C) F6FBB 1999
//
//////////////////////////////////////////////////////////////

// Modifed by John Wiseman G8BPQ Jan 2006
//
//		Enable monitoring in routine InitChannel
//
//		Clear applmask when terminating
//
//		Get APPLMASK from Registry
//
//		Send UI frames to all bpq ports

#define _CRT_SECURE_NO_DEPRECATE

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "fbb_drv.h"

extern "C" {
#include "bpq32.h"
}

#define BPQ_MAJOR		1
#define BPQ_MINOR		4 // G8BPQ was 3

#define DISCONNECTED	0
#define CONNECTED		1
#define CONNECT_PENDING 2
#define WAITING_CALL	3
#define CONNECT_CALL	4

#define MAXITEMS 6

//------------

typedef struct DATAIN_t {
	char	*pData;
	int		nChannel;
	int		nType;
	int		nLen;
	DRVUI	*pUi;
	struct	DATAIN_t *pNext;
} DATAIN;

typedef struct CHANNEL_t {
	int		nStream;
	int		nState;
	int		nLastState;
	int		nLastNbFrames;
	int		nFrameNb;
	BOOL	bBinary;
	char	szMyCall[10];
} CHANNEL;

typedef struct FBBDRV_t
{
	HWND	hWnd;
	int		nPort;
	int		nNbChan;
	int		nLastChan;
	int		nBpqPort;
	int		nAppli;
	BOOL	bNext;
	BOOL	bMonitor;
	BOOL	bReconnect;
	DRVUI	*pUi;
	DATAIN	*pHostHead;	// Head of data_in list
	DATAIN	*pHostTail;	// Tail of data_in list
	CRITICAL_SECTION	hCritical;
	char	szMyCall[10];
	CHANNEL	*pChan;
	struct	FBBDRV_t *pNext;
} FBBDRV;

struct PORTS
{
	int	nNbPorts;
	char **szName;
} Port;


//////////////////////////////////////////////////////////////
// Local prototypes
//////////////////////////////////////////////////////////////

static DWORD WINAPI BpqLoop( LPSTR lpData );
static FBBDRV *SearchPort(int nPort);
static FBBDRV *SearchBpqPort(int nPort);
static DATAIN *GetHost(FBBDRV *pFbbDrv);
static BOOL ToTnc(FBBDRV *pFbbDrv, int nChannel, int nType, char *pData, int nLen, DRVUI *pUi);
static BOOL BpqInit(FBBDRV *pFbbDrv, char *pPtr);
static BOOL Connected(FBBDRV *pFbbDrv, char *szCall);
static BOOL Disconnect(FBBDRV *pFbbDrv, int nChan);
static BOOL Disconnected(FBBDRV *pFbbDrv, int nChan);
static BOOL ConnectTo(FBBDRV *pFbbDrv, int nChan, char *szRemoteCall);
static void ToHost(FBBDRV *pFbbDrv, int nChannel, int nType, char *pData, int nLen, DRVUI *pUi);
static void InitChannel(FBBDRV *pFbbDrv, int nChan);

static void SendDataUN(FBBDRV *pFbbDrv, DRVUI *pUi, char *szData, int nLen);
static void SendDataTo(FBBDRV *pFbbDrv, int nChan, char *szData, int nLen);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////

HINSTANCE hInstance;
HWND hWndDll;
FBBDRV	*pDrvHead = NULL;	// Head of port structures
static char szTemp[4000];

int BPQPorts;				// Number of BPQ Ports
int BPQUIMask=0xffff;		// Ports to send UI messages on

//int nMonStream;

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


BOOL APIENTRY DllMain( HINSTANCE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
	hInstance = hModule;

    switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
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
	FBBDRV	*pFbbDrv;
//	HANDLE	hFbbThread;
//	DWORD	dwThreadID;
	DRVINFO *pInfo;
	
	HKEY hKey=0;
	int retCode,Type,Vallen=4;
	int APPLMASK;

 	OutputDebugString("FBBBPQ Open Driver\n");

	// Already used ?
	pFbbDrv = SearchPort(nPort);
	if (pFbbDrv)
		return FALSE;
	
	pInfo = (DRVINFO *)pDrvInfo;
	
	// Connect to the BPQ if not already done
	if (pDrvHead == NULL)
	{
	    WNDCLASS  wc;
		char *AppName = "fbb_bpq";

		// Create a dummy window to receive the BPQ messages
		// Fill in window class structure with parameters that describe
		// the main window.
        wc.style         = CS_NOCLOSE;
        wc.lpfnWndProc   = (WNDPROC)WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "fbb_bpq";

		int ret = RegisterClass(&wc);
      
		hWndDll = CreateWindow("fbb_bpq", "", WS_DISABLED,
			0, 0, 100, 100,
			HWND_DESKTOP, NULL, hInstance, NULL);

		if (!hWndDll) {
			DWORD dw = GetLastError();
			return (FALSE);
		}

		//	Register message for posting by BPQDLL
		BPQMsg = RegisterWindowMessage(BPQWinMsg);

		// FBB message number for notifications
		if (hWnd)
			WM_NOTIFY_MSG = RegisterWindowMessage(FBBDRV_NOTIFY);

		// Timer to check the nb of outgoing frames of streams
		SetTimer(hWndDll, 1, 1000, NULL);


		// Monitoring stream;
		//nMonStream = FindFreeStream();
		//BPQSetHandle(nMonStream, hWndDll);
		//SetAppl(nMonStream,0x80,0); 
	}
	else
	{
		// Only one BPQ port allowed. Maybe more ports later...
		return FALSE;
	}
	
	pFbbDrv = (FBBDRV *)LocalAlloc(LPTR, sizeof(FBBDRV));
	if (pFbbDrv == NULL)
		return FALSE;
	
	strcpy(pFbbDrv->szMyCall, pInfo->szMyCall);
	pFbbDrv->pUi		= NULL;
	pFbbDrv->bNext		= FALSE;
	pFbbDrv->hWnd		= hWnd;
	pFbbDrv->nPort		= nPort;
	pFbbDrv->nBpqPort	= pInfo->nMultCh;
	pFbbDrv->nNbChan	= pInfo->nNbChan;

	
	// Get APPL and UI Port Mask from Registry - G8BPQ March 2006

	APPLMASK=1;

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		OutputDebugString("FBBBPQ Reading Appl Mask\n");

		retCode = RegQueryValueEx(hKey,"FBBAPPL",0,			
			(ULONG *)&Type,(UCHAR *)&APPLMASK,(ULONG *)&Vallen);
		
	
	  	if (retCode == ERROR_SUCCESS)
			OutputDebugString("FBBBPQ Got Appl Mask\n");

		OutputDebugString("FBBBPQ Reading UI Mask\n");

		retCode = RegQueryValueEx(hKey,"FBBUIMask",0,			
			(ULONG *)&Type,(UCHAR *)&BPQUIMask,(ULONG *)&Vallen);
		
	
  		if (retCode == ERROR_SUCCESS)
			OutputDebugString("FBBBPQ Got UI Mask\n");

	}

	
	pFbbDrv->nAppli = APPLMASK; // BBS application
	
	pFbbDrv->bMonitor   = TRUE;
	pFbbDrv->bReconnect = FALSE;
	
	// Includes the unused channel 0
	pFbbDrv->pChan      = (CHANNEL *)LocalAlloc(LPTR, (pFbbDrv->nNbChan+1) * sizeof(CHANNEL));
	if (pFbbDrv->pChan == NULL)
	{
		LocalFree(pFbbDrv);
		return FALSE;
	}

	for (int n = 1 ; n <= pFbbDrv->nNbChan ; n++)
		InitChannel(pFbbDrv, n);
	
	// Insert the structure in the list
	pFbbDrv->pNext = pDrvHead;
	pDrvHead = pFbbDrv;
	
	InitializeCriticalSection(&pFbbDrv->hCritical);

	BPQPorts=GetNumberofPorts();
	
	return TRUE;
	
} // end of OpenFbbDriver()

BOOL WINAPI CloseFbbDriver(int nPort)
{
	int nNb,i,Stream;
	FBBDRV *pFbbDrv = SearchPort(nPort);
	if (pFbbDrv == NULL)
		return FALSE;

	// Detach monitoring stream;
	//  DeallocateStream(nMonStream);
	// Disconnect all connected channels

	for (nNb = 1; nNb <= pFbbDrv->nNbChan ; nNb++)
	{
		if (pFbbDrv->pChan[nNb].nState != DISCONNECTED)
			Disconnect(pFbbDrv, nNb);
		// Detach stream;
		
		Stream=pFbbDrv->pChan[nNb].nStream;

		SetAppl(Stream, 0, 0);
		DeallocateStream(Stream);
		SessionState(Stream, &i, &i);	  // Ack the change

	}
	
	DeleteCriticalSection(&pFbbDrv->hCritical);
	
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
	
	// Close the BPQ connection if last
	if (pDrvHead == NULL)
	{
	}
	
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
		return BpqInit(pFbbDrv, (char *)pPtr);
	case DRV_SNDCMD:
	case DRV_ECHOCMD:
		return (ToTnc(pFbbDrv, nChan, DRV_COMMAND, (char *)pPtr, strlen((char *)pPtr), NULL));
	case DRV_PORTCMD:
		return (ToTnc(pFbbDrv, 0, DRV_COMMAND, (char *)pPtr, strlen((char *)pPtr), NULL));
	case DRV_PACLEN:
		*((int *) pPtr) = 250;
		return TRUE;
	case DRV_VERSION:
		wsprintf((char *)pPtr, 
			"v%d.%02d FBB driver for BPQ32 (F6FBB-%s)", 
			BPQ_MAJOR, BPQ_MINOR, __DATE__);
		return TRUE;
	}
	
	return FALSE;
} // end of StatFbbDriver()


//////////////////////////////////////////////////////////////
//
// Local functions
//
//////////////////////////////////////////////////////////////

static void InitChannel(FBBDRV *pFbbDrv, int nChan)
{
	int nFlags;

	CHANNEL *pChan = &pFbbDrv->pChan[nChan];
	
	memset(pChan, 0, sizeof(CHANNEL));
	pChan->nState = DISCONNECTED;
	strcpy(pChan->szMyCall, pFbbDrv->szMyCall);

	int nStream = FindFreeStream();
	if (nStream != 255)
	{
		pChan->nStream = nStream;

		if (nChan == 1)
		{
			nFlags = (pFbbDrv->bMonitor) ? 0x82 : 0x02;
		}
		else
			nFlags = 0x02;

		SetAppl(pChan->nStream, nFlags, pFbbDrv->nAppli);
		BPQSetHandle(nStream, hWndDll);
	}
}

static void ReInitChannel(FBBDRV *pFbbDrv, int nChan)
{
	int nFlags;
	CHANNEL *pChan = &pFbbDrv->pChan[nChan];
	
	if (nChan == 1)
	{
		nFlags = (pFbbDrv->bMonitor) ? 0x82 : 0x02;
	}
	else
		nFlags = 0x02;

	SetAppl(pChan->nStream, nFlags, pFbbDrv->nAppli);
}

static BOOL BpqInit(FBBDRV *pFbbDrv, char *pPtr)
{
	int n;
	BOOL bRes = FALSE;
	char cCmd = *pPtr++;

	while (*pPtr && isspace(*pPtr))
		++pPtr;

	switch (toupper(cCmd))
	{
	case 'A':
		pFbbDrv->nAppli = atoi (pPtr);
		for (n = 1 ; n <= pFbbDrv->nNbChan ; n++)
			ReInitChannel(pFbbDrv, n);
		bRes = TRUE;
		break;
	case 'M':
		pFbbDrv->bMonitor = (*pPtr != '0');
		ReInitChannel(pFbbDrv, 1);
		bRes = TRUE;
		break;
	}
	
	return bRes;
}

static BOOL BpqCommand(FBBDRV *pFbbDrv, int nChan, char *pPtr)
{
	char cCmd = *pPtr++;

	while (*pPtr && isspace(*pPtr))
		++pPtr;

	switch (toupper(cCmd))
	{
	case 'I':
		strncpy(pFbbDrv->pChan[nChan].szMyCall, pPtr, 10);
		pFbbDrv->pChan[nChan].szMyCall[9] = '\0';
		break;
	case 'C':
		// Connect
		if (nChan > 0)
			return ConnectTo(pFbbDrv, nChan, pPtr);
		break;
	case 'D':
		// Disconnect
		if (nChan > 0)
			return Disconnect(pFbbDrv, nChan);
		break;
	}

	return FALSE;
}

static BOOL Disconnect(FBBDRV *pFbbDrv, int nChan)
{
	if (nChan < 1 || nChan > pFbbDrv->nNbChan)
		return FALSE;

	if (pFbbDrv->pChan[nChan].nState == DISCONNECTED)
		return FALSE;

	int nStream = pFbbDrv->pChan[nChan].nStream;
	SessionControl(nStream, 2, 0);

	return TRUE;
}

static FBBDRV *SearchPort(int nPort)
{
	FBBDRV *pTmp = pDrvHead;
	
	/* while (pTmp)
	{
		if (pTmp->nPort == nPort)
			break;
		pTmp = pTmp->pNext;
	} */
	
	return pTmp;
}

static FBBDRV *SearchBpqPort(int nBpqPort)
{
	FBBDRV *pTmp = pDrvHead;
	
	/* while (pTmp)
	{
		if (pTmp->nBpqPort == nBpqPort)
			break;
		pTmp = pTmp->pNext;
	} */

	return pTmp;
}

static int Stream2Chan(FBBDRV *pFbbDrv, int nStream)
{
	int nChan;
	
	for (nChan = 1 ; nChan <= pFbbDrv->nNbChan ; nChan++)
	{
		if (pFbbDrv->pChan[nChan].nStream == nStream)
			return (nChan);
	}
	return -1;
}

static BOOL SendStats(FBBDRV *pFbbDrv, int nChannel, int nNbFrames)
{
	DRVSTATS DrvStats;
	BOOL bOk = FALSE;
				
	DrvStats.nFrameNb = pFbbDrv->pChan[nChannel].nFrameNb;

	bOk |= (pFbbDrv->pChan[nChannel].nLastNbFrames != nNbFrames);
	pFbbDrv->pChan[nChannel].nLastNbFrames = nNbFrames;
	DrvStats.nNbFrames = nNbFrames;
	
	DrvStats.nNbRetry = 0;

	switch (pFbbDrv->pChan[nChannel].nState)
	{
	case CONNECTED :
		DrvStats.nState = DRV_CONNECTED;
		break;
	case DISCONNECTED :
		DrvStats.nState = DRV_DISCONNECTED;
		break;
	case CONNECT_PENDING :
	case WAITING_CALL :
	case CONNECT_CALL :
		DrvStats.nState = DRV_CONNPENDING;
		break;
	}

	bOk |= (pFbbDrv->pChan[nChannel].nLastState != DrvStats.nState);
	pFbbDrv->pChan[nChannel].nLastState = DrvStats.nState;

	DrvStats.nBuffers = GetFreeBuffs();
				
	if (bOk)	// Notify if any change
		ToHost(pFbbDrv, nChannel, DRV_STATS, (char *)&DrvStats, sizeof(DRVSTATS), NULL);

	return TRUE;
}

static void BPQStats()
{
	int nStream;
	int nChan;
	FBBDRV *pFbbDrv = SearchBpqPort(1);

	if (pFbbDrv==NULL)
		return;

	for (nChan = 1 ; nChan <= pFbbDrv->nNbChan ; nChan++)
	{
		if (pFbbDrv->pChan[nChan].nState != DRV_DISCONNECTED)
		{
			nStream = pFbbDrv->pChan[nChan].nStream;
			pFbbDrv->pChan[nChan].nFrameNb = 0;
			SendStats(pFbbDrv, nChan, TXCount(nStream));
		}
	}
}

static BOOL ToTnc(FBBDRV *pFbbDrv, int nChannel, int nType, char *pData, int nLen, DRVUI *pUi)
{
	if (nLen <= 0 || nLen > 256)
		return FALSE;
	
	switch (nType)
	{
	case DRV_UNPROTO:
		if (pUi)
			SendDataUN(pFbbDrv, pUi, pData, nLen);
		break;
	case DRV_DATA:
		if (pFbbDrv->pChan[nChannel].nState == CONNECTED)
		{
			// Frame number.
			pFbbDrv->pChan[nChannel].nFrameNb = *((int *)pUi);

			SendDataTo(pFbbDrv, nChannel, pData, nLen);
			++pFbbDrv->pChan[nChannel].nLastNbFrames;
			SendStats(pFbbDrv, nChannel, 0);
		}
		break;
	case DRV_COMMAND:
		return BpqCommand(pFbbDrv, nChannel, pData);
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

///////////////////////////////////////////////////////////////////////////////
static BOOL ConnectTo(FBBDRV *pFbbDrv, int nChan, char *szRemoteCall)
{
	char szData[80];
	int nStream = pFbbDrv->pChan[nChan].nStream;

	// Connect to the switch
	SessionControl(nStream, 1, 0);
	sprintf (szData, "*** Linked to %s\r", pFbbDrv->pChan[nChan].szMyCall);
	SendDataTo(pFbbDrv, nChan, szData, strlen(szData));
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
static void BpqSetCall (char *pTrame, char *pCall, int bRep)
{
	int c;
	int i;
	int nSsid = 0;

	for (i = 0; i < 6; i++)
	{
		c = *pCall & 0xff;

		if (isalnum(c))
		{
			*pTrame++ = (c << 1);
			++pCall;
		}
		else
			*pTrame++ = 0x40;
	}
	if (c == '-')
	{
		++pCall;
		nSsid = atoi(pCall) & 0xf;
	}

	c = (nSsid << 1) | 0x60;

	if (bRep)
		c |= 0x01;

	*pTrame++ = c;
}

//////////////////////////////////////////////////////////////////////////////
static void SendDataUN(FBBDRV *pFbbDrv, DRVUI *pUi, char *szData, int nDataLen)
{
	int nLen = 0,i,UIMask=BPQUIMask;
	char szBuf[350];
	char *pCtl = szBuf;
	char *pScan;

	BpqSetCall (pCtl, pUi->szTo, 0);
	pCtl += 7;
	nLen += 7;

	pScan = strtok(pUi->szVia, " ");

	BpqSetCall (pCtl, pUi->szFrom, (pScan) ? 0 : 1);
	pCtl += 7;
	nLen += 7;

	while (pScan)
	{
		char *pDigi = pScan;
		pScan = strtok(NULL, " ");
		BpqSetCall (pCtl, pDigi, (pScan) ? 0 : 1);
		pCtl += 7;
		nLen += 7;
	}

	*pCtl++ = (char)0x03;
	*pCtl++ = (char)0xf0;
	nLen += 2;

	if (nDataLen > 255)
		nDataLen = 255;

	memcpy(pCtl, szData, nDataLen);
	nLen += nDataLen;

	//	Send to all BPQ Ports

	for (i=1;i<=BPQPorts;i++)
	{
		if (UIMask & 1)
		{
			SendRaw(GetPortNumber(i), szBuf, nLen);
		} 
		UIMask>>=1;
	}
}

//////////////////////////////////////////////////////////////////////////////
static void SendDataTo(FBBDRV *pFbbDrv, int nChan, char *szData, int nLen)
{
	int nStream = pFbbDrv->pChan[nChan].nStream;

	SendMsg(nStream, szData, nLen);
}

static int BpqGetCall (char *pTrame, char *pCall, int mode)
{
	int c, i, nSsid;

	for (i = 0; i < 6; i++)
	{
		c = ((*pTrame++) >> 1) & 0x7f;
		if (isalnum (c))
			*pCall++ = c;
	}

	nSsid = *pTrame;

	if ((c = ((nSsid >> 1) & 0xf)) != 0)
	{
		*pCall++ = '-';
		if (c >= 10)
		{
			c -= 10;
			*pCall++ = '1';
		}
		*pCall++ = c + '0';
	}

	*pCall = '\0';
	return (nSsid);
}


//////////////////////////////////////////////////////////////////////////////
static void DoReceiveMonitoring(int nStream)
{
	char szBuf[500];
	char *bpq_ptr;
	int nPort;
	int nLen;
	int nCount;
	int nStamp;
	int nCtrl;
	int nSsid;
	int nSsid_d;
	int nV2;
	DRVUI *pUi;
	FBBDRV *pFbbDrv;
	
	do {
		pUi = (DRVUI *)LocalAlloc(LPTR, sizeof(DRVUI));
		
		nStamp = GetRaw(nStream, szBuf, &nLen, &nCount);
		
		bpq_ptr = szBuf;
		
		nPort = bpq_ptr[4] & 0x7f;

		wsprintf(pUi->szComment, "{port %d}", nPort);
		
		bpq_ptr += 7;
		nLen -= 7;
		nLen -= 14;
		if (nLen < 0)
			return;
		
		nSsid_d = BpqGetCall (bpq_ptr, pUi->szTo, 0);
		bpq_ptr += 7;
		nSsid = BpqGetCall (bpq_ptr, pUi->szFrom, 0);
		bpq_ptr += 7;
		nV2 = ((nSsid_d & 0x80) != (nSsid & 0x80));
		
		if ((nSsid & 1) == 0)
		{
			char temp[80];
			
			do
			{
				nLen -= 7;
				if (nLen < 1)
					return;
				
				nSsid = BpqGetCall (bpq_ptr, temp, 0);
				bpq_ptr += 7;
				if (*temp)
				{
					strcat (pUi->szVia, temp);
					if (nSsid & 0x80)
					{
						if ((nSsid & 1) || (((nSsid & 1) == 0) && ((bpq_ptr[6] & 0x80) == 0)))
							strcat (pUi->szVia, "*");
					}
					strcat (pUi->szVia, " ");
				}
			}
			while ((nSsid & 1) == 0);
		}
		
		nCtrl = *bpq_ptr++ & 0xff;
		nLen--;
		
		char *pCtl = pUi->szCtl;
		
		if ((nCtrl & 0x1) == 0)
		{
			/* I frame */
			*pCtl++ = 'I';
			*pCtl++ = (nCtrl >> 5) + '0';
			*pCtl++ = ((nCtrl >> 1) & 0x7) + '0';
			
		}
		else if ((nCtrl & 0x3) == 1)
		{
			/* S frame */
			switch (nCtrl & 0xf)
			{
			case 0x1:
				*pCtl++ = 'R';
				*pCtl++ = 'R';
				break;
			case 0x5:
				*pCtl++ = 'R';
				*pCtl++ = 'N';
				*pCtl++ = 'R';
				break;
			case 0x9:
				*pCtl++ = 'R';
				*pCtl++ = 'E';
				*pCtl++ = 'J';
				break;
			}
			*pCtl++ = (nCtrl >> 5) + '0';
			
		}
		else
		{
			/* U frame */
			switch (nCtrl & 0xec)
			{
			case 0x2c:
				*pCtl++ = 'S';
				*pCtl++ = 'A';
				*pCtl++ = 'B';
				*pCtl++ = 'M';
				break;
			case 0x40:
				*pCtl++ = 'D';
				*pCtl++ = 'I';
				*pCtl++ = 'S';
				*pCtl++ = 'C';
				break;
			case 0x0c:
				*pCtl++ = 'D';
				*pCtl++ = 'M';
				break;
			case 0x60:
				*pCtl++ = 'U';
				*pCtl++ = 'A';
				break;
			case 0x84:
				*pCtl++ = 'F';
				*pCtl++ = 'R';
				*pCtl++ = 'M';
				*pCtl++ = 'R';
				break;
			case 0x00:
				*pCtl++ = 'U';
				*pCtl++ = 'I';
				pUi->bUi = 1;
				break;
			}
		}
		
		if (nV2)
		{
			if (nCtrl & 0x10)
			{
				if (nSsid_d & 0x80)
					*pCtl++ = '+';
				else
					*pCtl++ = '-';
			}
			else
			{
				if (nSsid_d & 0x80)
					*pCtl++ = '^';
				else
					*pCtl++ = 'v';
			}
		}
		else if (nCtrl & 0x10)
			*pCtl++ = '!';
		
		*pCtl = '\0';
		
		if (((nCtrl & 1) == 0) || ((nCtrl & 0xef) == 0x3))
		{
			nLen--;
			if (nLen < 0)
				return;
			
			pUi->nPid = *bpq_ptr++ & 0xff;
		}
		else
			pUi->nPid = 0;
		
		pFbbDrv = SearchBpqPort(1);
		if (pFbbDrv)
		{
			pUi->nPort = pFbbDrv->nPort;
			ToHost(pFbbDrv, 0, DRV_UNPROTO, bpq_ptr, nLen, pUi);
		}
		
	} while (nCount > 0);
}

//////////////////////////////////////////////////////////////////////////////
static void DoReceivedData(int nStream)
{
	int nLen;
	int nCount;
	char szBuf[300];
	FBBDRV *pFbbDrv;
	
	pFbbDrv = SearchBpqPort(1);
	
	do {
		
		GetMsg(nStream, szBuf, &nLen, &nCount);
		
		int nChan = Stream2Chan(pFbbDrv, nStream);
		if (nLen > 0)
			ToHost(pFbbDrv, nChan, DRV_DATA, szBuf, nLen, NULL);
		
	} while (nCount > 0);
}

//////////////////////////////////////////////////////////////////////////////
static void DoStateChange(int nStream)
{
	char szMessage[80];
	char szCallsign[20];
	int nChange;
	int nState;
	int nChan;
	FBBDRV *pFbbDrv;

	pFbbDrv = SearchBpqPort(1);
	nChan = Stream2Chan(pFbbDrv, nStream);

	SessionState(nStream, &nState, &nChange);
		
	if (nChange == 1)
	{
		if (nState == 1)
		{
			// Connected
			GetCallsign(nStream, szCallsign);
			szCallsign[9] = '\0';	// It seems that the string is not terminated
			pFbbDrv->pChan[nChan].nState = DRV_CONNECTED;
			wsprintf(szMessage,"(%d) CONNECTED to %s", nChan, szCallsign);
			ToHost(pFbbDrv, nChan, DRV_COMMAND, szMessage, -1, NULL);
		}
		else
		{
			pFbbDrv->pChan[nChan].nState = DRV_DISCONNECTED;
			wsprintf(szMessage,"(%d) DISCONNECTED fm BPQ", nChan);
			ToHost(pFbbDrv, nChan, DRV_COMMAND, szMessage, -1, NULL);
			strcpy(pFbbDrv->pChan[nChan].szMyCall, pFbbDrv->szMyCall);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
static LRESULT CALLBACK WndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (nMsg == BPQMsg)
	{
		if (lParam & BPQMonitorAvail)
			DoReceiveMonitoring(wParam);

		if (lParam & BPQDataAvail)
			DoReceivedData(wParam);
				
		if (lParam & BPQStateChange)
			DoStateChange(wParam);

		BPQStats();
		return 0;
	}

	if (nMsg == WM_TIMER)
	{
		BPQStats();
		return 0;
	}

	return (DefWindowProc(hWnd, nMsg, wParam, lParam));
}

