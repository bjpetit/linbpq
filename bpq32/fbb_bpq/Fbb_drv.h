
#ifndef FBBDRV_H
#define FBBDRV_H

#ifndef PUBLIC
#define PUBLIC
#endif

//////////////////////////////////////////////////
// Driver notification message
#define FBBDRV_NOTIFY "FBBDRV_NOTIFY"
PUBLIC UINT WM_NOTIFY_MSG;

//////////////////////////////////////////////////
// Driver request message
#define FBBDRV_REQUEST "FBBDRV_REQUEST"
PUBLIC UINT WM_REQUEST_MSG;

// Callsign / Password check for modem or telnet
// Request in wParam : (lParam points to a Callsign^Password string)
#define PASSWD_CHECK 1
// Answers :
#define PWDCHK_ERROR				0	// Internal error
#define PWDCHK_ACCEPTED				1	// Password is accepted
#define PWDCHK_INVALID_CALLSIGN		2	// Callsign is invalid
#define PWDCHK_UNKNOWN_CALLSIGN		3	// Callsign is unknown
#define PWDCHK_EXCLUDED_CALLSIGN	4	// Callsign is excluded
#define PWDCHK_NO_MODEMFLAG			5	// Modem/Telnet flag is not set
#define PWDCHK_WRONG_PASSWORD		6	// Password is wrong

// IO Constants
#define DRV_INFO	10
#define DRV_COMMAND 11
#define DRV_DATA    12
#define DRV_UNPROTO 13
#define DRV_DISPLAY 14
#define DRV_TOR     15
#define DRV_STATS   16
#define DRV_NBBUF   17
#define DRV_NBCHR   18
#define DRV_INIT	19
#define DRV_PROCESS	20
#define DRV_VERSION	21

// Status constants
#define	DRV_INVCMD	0xff
#define	DRV_NOCMD	0
#define	DRV_TNCSTAT	1
#define	DRV_PACLEN	2
#define	DRV_CMDE	3
#define	DRV_SETFLG	4
#define	DRV_SNDCMD	5
#define	DRV_ECHOCMD	6
#define	DRV_PORTCMD	7
#define	DRV_ERRCMD	8
#define	DRV_BINCMD	9
#define	DRV_BSCMD	10
#define	DRV_SUSPCMD	11
#define DRV_SYSRDY	12

typedef struct
{
	int		nCom;
	int		nBaudrate;
	int		nNbChan;
	int		nAddr;
	int		nMultCh;
	char	szMyCall[10];
	char	szSystemPath[256];
} DRVINFO;

typedef struct
{
	int		nPort;
	char	szFrom[10];
	char	szTo[10];
	char	szVia[82];
	char	szCtl[10];
	char	szComment[20];
	int		nPid;
	BOOL	bUi;
} DRVUI;

// List of states
#define DRV_DISCONNECTED	0
#define DRV_CONNECTED		1
#define DRV_DISCPENDING		2
#define DRV_CONNPENDING		3
#define DRV_CONNWAIT		4
#define DRV_CONNCHECK		5

typedef struct
{
	int nNbFrames;	// Number of frames not sent
	int nNbRetry;	// Number of retries
	int nState;		// Status of the channel
	int nBuffers;	// Number of available global buffers
	int nFrameNb;	// Number of the sent frame
} DRVSTATS;

//////////////////////////////////////////////////////////////
//
// Generic DLL functions
//
//////////////////////////////////////////////////////////////

BOOL OpenFbbDll(int nPort, char *szName);
BOOL CloseFbbDll(int nPort);

// Declare the functions WINAPI when used by the DLL
#ifdef __WIN32__
#define DRV_WINAPI
#else
#define DRV_WINAPI WINAPI
#endif

//////////////////////////////////////////////////////////////
//
// Generic driver functions
//
//////////////////////////////////////////////////////////////
BOOL DRV_WINAPI OpenFbbDriver(int nPort, HWND hWnd, void *pDrvInfo);
BOOL DRV_WINAPI CloseFbbDriver(int nPort);
BOOL DRV_WINAPI ReadFbbDriver(int *nPort, int *nType, int *nChan, char *pszBuf, int *nLen, DRVUI *pUi);
BOOL DRV_WINAPI WriteFbbDriver(int nPort, int nType, int nChan, char *pszBuf, int nLen, DRVUI *pUi);
BOOL DRV_WINAPI StatFbbDriver(int nPort, int nCmd, int nChan, void *pPtr, int nLen);
//////////////////////////////////////////////////////////////

#endif FBBDRV_H