


//#define VVICON 400

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2

BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);

int FAR PASCAL MainWndProc(HWND, UINT, WPARAM, LPARAM);
long FAR PASCAL DiagWndProc(HWND, UINT, WPARAM, LPARAM);
int FAR PASCAL ConnWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
int FAR PASCAL ConfigWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

BOOL APIENTRY About();

struct AGWHeader
{
    int Port;
	byte DataKind;
    byte filler2;
	byte PID;
    byte filler3;
    byte callfrom[10];
    byte callto[10];
    int DataLength;
    int reserved;
};

struct SocketConnectionInfo
{
	int Number;					// Number of record - for Connections display
    SOCKET socket;
	SOCKADDR_IN sin;  
	BOOL SocketActive;
    BOOL RawFlag;
    BOOL MonFlag;
    byte CallSign[10];
    BOOL GotHeader;
    int MsgDataLength;
    struct AGWHeader AGWRXHeader;   
};

struct BPQConnectionInfo
{    
    struct SocketConnectionInfo * SocketIndex;
    int BPQStream;
    byte CallKey[21];					// Port + two calls
    BOOL Connecting;					// Set while waiting for connection to complete
    BOOL Listening;
    int ApplMask;   
} ConInfoRec;

#define Disconnect(stream) SessionControl(stream,2,0)
#define Connect(stream) SessionControl(stream,1,0)
