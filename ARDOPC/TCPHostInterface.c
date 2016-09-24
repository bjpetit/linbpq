// ARDOP TNC Host Interface using TCP
//

#ifdef WIN32
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T
#include <windows.h>
#pragma comment(lib, "WS2_32.Lib")

#define ioctl ioctlsocket
#else

#define UINT unsigned int
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
#include <pthread.h>

#define SOCKET int

#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#define WSAGetLastError() errno
#define GetLastError() errno 
#define closesocket close
#endif

#define MAX_PENDING_CONNECTS 4

#include "ARDOPC.h"

#define GetBuff() _GetBuff(__FILE__, __LINE__)
#define ReleaseBuffer(s) _ReleaseBuffer(s, __FILE__, __LINE__)
#define Q_REM(s) _Q_REM(s, __FILE__, __LINE__)
#define C_Q_ADD(s, b) _C_Q_ADD(s, b, __FILE__, __LINE__)

VOID * _Q_REM(VOID *Q, char * File, int Line);
int _C_Q_ADD(VOID *Q, VOID *BUFF, char * File, int Line);
UINT _ReleaseBuffer(VOID *BUFF, char * File, int Line);
VOID * _GetBuff(char * File, int Line);
int C_Q_COUNT(VOID *Q);

void ProcessCommandFromHost(char * strCMD);
BOOL checkcrc16(unsigned char * Data, unsigned short length);

extern int port;

SOCKET TCPControlSock, TCPDataSock, ListenSock, DataListenSock;

BOOL CONNECTED = FALSE;
BOOL DATACONNECTED = FALSE;

// Host to TNC RX Buffer

UCHAR ARDOPBuffer[8192];
int InputLen = 0;

UCHAR ARDOPDataBuffer[8192];
int DataInputLen = 0;

//	Main TX Buffer

UCHAR bytDataToSend[100000] = 
		"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
		"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
		"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
		"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
		"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n";

;					// May malloc this, or change to cyclic buffer

int bytDataToSendLength = 0;

/*UINT FREE_Q = 0;

int MAXBUFFS = 0;
int QCOUNT = 0;
int MINBUFFCOUNT = 65535;
int NOBUFFCOUNT = 0;
int BUFFERWAITS = 0;
int NUMBEROFBUFFERS = 0;

unsigned int Host_Q;			// Frames for Host
*/

UCHAR bytLastCMD_DataSent[256];

//	Function to send a text command to the Host

void SendCommandToHost(char * strText)
{
	// This is from TNC side as identified by the leading "c:"   (Host side sends "C:")
	// Subroutine to send a line of text (terminated with <Cr>) on the command port... All commands beging with "c:" and end with <Cr>
	// A two byte CRC appended following the <Cr>
	// The strText cannot contain a "c:" sequence or a <Cr>
	// Returns TRUE if command sent successfully.
	// Form byte array to send with CRC

	UCHAR bytToSend[256];
	int len;
	int ret;
	
	len = sprintf(bytToSend,"c:%s\r", strText);

	if (CONNECTED)
	{
		ret = send(TCPControlSock, bytToSend, len, 0);
		ret = WSAGetLastError();

		if (CommandTrace) Debugprintf(" Command Trace TO Host  c:%s", strText);
		return;
	}
	return;
}

//	Function to queue a text command to the Host used for all asynchronous Commmands (e.g. BUSY etc)

void QueueCommandToHost(char * strText)
{
	SendCommandToHost(strText);		// no queuing in lastest code
}

//  Subroutine to add a short 3 byte tag (ARQ, FEC, ERR, or IDF) to data and send to the host 

void AddTagToDataAndSendToHost(UCHAR * bytData, char * strTag, int Len)
{
	//  This is from TNC side as identified by the leading "d:"   (Host side sends data with leading  "D:")
	// includes 16 bit CRC check on Data Len + Data (does not CRC the leading "d:")
	// strTag has the type Tag to prepend to data  "ARQ", "FEC" or "ERR" which are examined and stripped by Host (optionally used by host for display)
	// Max data size should be 2000 bytes or less for timing purposes
	// I think largest apcet is about 1360 bytes

	UCHAR * bytToSend;
	UCHAR buff[1500];

	int ret;

	if (blnInitializing)
		return;

	if (CommandTrace) Debugprintf("[AddTagToDataAndSendToHost] bytes=%d Tag %s", Len, strTag);

	//	Have to save copy for possible retry (and possibly until previous 
	//	command is acked

	bytToSend = buff;

	bytToSend[0] = 'd';			// indicates data from TNC
	bytToSend[1] = ':';
	Len += 3;					// Tag
	bytToSend[2] = Len >> 8;	//' MS byte of count  (Includes strDataType but does not include the two trailing CRC bytes)
	bytToSend[3] = Len  & 0xFF;// LS Byte
	memcpy(&bytToSend[4], strTag, 3);
	memcpy(&bytToSend[7], bytData, Len - 3);
	Len +=4;				// d: and len

	ret = send(TCPDataSock, bytToSend, Len, 0);
	ret = WSAGetLastError();

	return;
}

VOID ARDOPProcessCommand(UCHAR * Buffer, int MsgLen)
{
	Buffer[MsgLen - 1] = 0;		// Remove CR
	
	Buffer+=2;					// Skip c:

	if (_memicmp(Buffer, "RDY", 3) == 0)
	{
		//	Command ACK. Remove from buffer and send next if a

		return;
	}
	ProcessCommandFromHost(Buffer);
}

BOOL InReceiveProcess = FALSE;		// Flag to stop reentry


void ProcessReceivedControl()
{
	int Len, MsgLen;
	char * ptr, * ptr2;
	char Buffer[8192];

	if (InReceiveProcess)
		return;

	// shouldn't get several messages per packet, as each should need an ack
	// May get message split over packets

	//	Both command and data arrive here, which complicated things a bit

	//	Commands start with c: and end with CR.
	//	Data starts with d: and has a length field
	//	“d:ARQ|FEC|ERR|, 2 byte count (Hex 0001 – FFFF), binary data, +2 Byte CRC”

	//	As far as I can see, shortest frame is “c:RDY<Cr> + 2 byte CRC” = 8 bytes

	//	I don't think it likely we will get packets this long, but be aware...

	//	We can get pretty big ones in the faster 
				
	Len = recv(TCPControlSock, &ARDOPBuffer[InputLen], 8192 - InputLen, 0);

	if (Len == 0 || Len == SOCKET_ERROR)
	{
		// Does this mean closed?
		
		closesocket(TCPControlSock);
		TCPControlSock = 0;

		CONNECTED = FALSE;
		return;					
	}

	InputLen += Len;

loop:

	if (InputLen < 6)
		return;					// Wait for more to arrive (?? timeout??)

	if (ARDOPBuffer[1] = ':')	// At least message looks reasonable
	{
		if (ARDOPBuffer[0] == 'C')
		{
			// Command = look for CR

			ptr = memchr(ARDOPBuffer, '\r', InputLen);

			if (ptr == 0)	//  CR in buffer
				return;		// Wait for it

			ptr2 = &ARDOPBuffer[InputLen];

			if ((ptr2 - ptr) == 1)	// CR + CRC
			{
				// Usual Case - single meg in buffer
	
				MsgLen = InputLen;

				// We may be reentered as a result of processing,
				//	so reset InputLen Here

				InputLen=0;
				InReceiveProcess = TRUE;
				ARDOPProcessCommand(ARDOPBuffer, MsgLen);
				InReceiveProcess = FALSE;
				return;
			}
			else
			{
				// buffer contains more that 1 message

				//	I dont think this should happen, but...

				MsgLen = InputLen - (ptr2-ptr) + 1;	// Include CR and CRC

				memcpy(Buffer, ARDOPBuffer, MsgLen);

				memmove(ARDOPBuffer, ptr + 1,  InputLen-MsgLen);
				InputLen -= MsgLen;

				InReceiveProcess = TRUE;
				ARDOPProcessCommand(Buffer, MsgLen);
				InReceiveProcess = FALSE;

				if (InputLen < 0)
				{
					InputLen = 0;
					InReceiveProcess = FALSE;
					return;
				}
				goto loop;
			}
		}
	}

	// Getting bad data ?? Should we just reset ??
	
	Debugprintf("ARDOP BadHost Message ?? %s", ARDOPBuffer);
	return;
}





void ProcessReceivedData()
{
	int Len, MsgLen;
	char Buffer[8192];

	if (InReceiveProcess)
		return;

	// shouldn't get several messages per packet, as each should need an ack
	// May get message split over packets

	//	Both command and data arrive here, which complicated things a bit

	//	Commands start with c: and end with CR.
	//	Data starts with d: and has a length field
	//	“d:ARQ|FEC|ERR|, 2 byte count (Hex 0001 – FFFF), binary data, +2 Byte CRC”

	//	As far as I can see, shortest frame is “c:RDY<Cr> + 2 byte CRC” = 8 bytes

	//	I don't think it likely we will get packets this long, but be aware...

	//	We can get pretty big ones in the faster 
				
	Len = recv(TCPDataSock, &ARDOPDataBuffer[DataInputLen], 8192 - DataInputLen, 0);

	if (Len == 0 || Len == SOCKET_ERROR)
	{
		// Does this mean closed?
		
		closesocket(TCPDataSock);
		TCPDataSock = 0;

		DATACONNECTED = FALSE;
		return;					
	}

	DataInputLen += Len;

loop:

	if (DataInputLen < 5)
		return;					// Wait for more to arrive (?? timeout??)

	if (ARDOPDataBuffer[1] == ':')	// At least message looks reasonable
	{
		if (ARDOPDataBuffer[0] == 'D')
		{
			// Data = check we have it all

			int DataLen = (ARDOPDataBuffer[2] << 8) + ARDOPDataBuffer[3]; // HI First
			
			if (DataInputLen < DataLen + 4)
				return;					// Wait for more

			MsgLen = DataLen + 4;		// D: Len

			memcpy(Buffer, &ARDOPDataBuffer[4], DataLen);

			DataInputLen -= MsgLen;

			if (DataInputLen > 0)
				memmove(ARDOPDataBuffer, &ARDOPDataBuffer[MsgLen],  DataInputLen);

			InReceiveProcess = TRUE;
			AddDataToDataToSend(Buffer, DataLen);
			InReceiveProcess = FALSE;
	
			// See if anything else in buffer

			if (DataInputLen > 0)
				goto loop;

			if (DataInputLen < 0)
				DataInputLen = 0;

			return;

		}
	}

	// Getting bad data ?? Should we just reset ??
	
	Debugprintf("ARDOP BadHost Message ?? %c %c %s",
		ARDOPBuffer[0], ARDOPBuffer[1], &ARDOPBuffer[4]);
	return;
}



SOCKET OpenSocket4(int port)
{
	struct sockaddr_in  local_sin;  /* Local socket - internet style */
	struct sockaddr_in * psin;
	SOCKET sock = 0;
	u_long param=1;

	psin=&local_sin;
	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;

	if (port)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);

	    if (sock == INVALID_SOCKET)
		{
	        Debugprintf("socket() failed error %d", WSAGetLastError());
			return 0;
		}

		setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&param,4);

		psin->sin_port = htons(port);        // Convert to network ordering 

		if (bind( sock, (struct sockaddr *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
		{
			Debugprintf("bind(sock) failed port %d Error %d", port, WSAGetLastError());

		    closesocket(sock);
			return FALSE;
		}

		if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
		{
			Debugprintf("listen(sock) failed port %d Error %d", port, WSAGetLastError());
			return FALSE;
		}
		ioctl(sock, FIONBIO, &param);
	}
	return sock;
}

VOID InitQueue();

BOOL HostInit()
{
#ifdef WIN32
	WSADATA WsaData;			 // receives data from WSAStartup

	WSAStartup(MAKEWORD(2, 0), &WsaData);
#endif

	Debugprintf("ARDOPC listening on port %d", port);
//	InitQueue();

	ListenSock = OpenSocket4(port);
	DataListenSock = OpenSocket4(port + 1);
	return ListenSock;
}

void HostPoll()
{
	// Check for incoming connect or data

	fd_set readfs;
	fd_set errorfs;
	struct timeval timeout;
	int ret;
	int addrlen = sizeof(struct sockaddr_in);
	struct sockaddr_in sin;  
	u_long param=1;

	FD_ZERO(&readfs);	
	FD_ZERO(&errorfs);

	FD_SET(ListenSock,&readfs);

	timeout.tv_sec = 0;				// No wait
	timeout.tv_usec = 0;	

	ret = select(ListenSock + 1, &readfs, NULL, NULL, &timeout);

	if (ret == -1)
	{
		ret = WSAGetLastError();
		Debugprintf("%d ", ret);
		perror("listen select");
	}
	else
	{
		if (ret)
		{
			if (FD_ISSET(ListenSock, &readfs))
			{
				TCPControlSock = accept(ListenSock, (struct sockaddr * )&sin, &addrlen);
	    
				if (TCPControlSock == INVALID_SOCKET)
				{
					Debugprintf("accept() failed error %d", WSAGetLastError());
					return;
				}
				Debugprintf("Connected to host");
					
				ioctl(TCPControlSock, FIONBIO, &param);
				CONNECTED = TRUE;
				SendCommandToHost("RDY");
			}
		}
	}

	FD_SET(DataListenSock,&readfs);

	timeout.tv_sec = 0;				// No wait
	timeout.tv_usec = 0;	

	ret = select(DataListenSock + 1, &readfs, NULL, NULL, &timeout);

	if (ret == -1)
	{
		ret = WSAGetLastError();
		Debugprintf("%d ", ret);
		perror("listen select");
	}
	else
	{
		if (ret)
		{
			if (FD_ISSET(DataListenSock, &readfs))
			{
				TCPDataSock = accept(DataListenSock, (struct sockaddr * )&sin, &addrlen);
	    
				if (TCPDataSock == INVALID_SOCKET)
				{
					Debugprintf("accept() failed error %d", WSAGetLastError());
					return;
				}
				Debugprintf("Connected to host");
					
				ioctl(TCPDataSock, FIONBIO, &param);
				DATACONNECTED = TRUE;
			}
		}
	}

	if (CONNECTED)
	{

	FD_ZERO(&readfs);	
	FD_ZERO(&errorfs);

	FD_SET(TCPControlSock,&readfs);
	FD_SET(TCPControlSock,&errorfs);

	timeout.tv_sec = 0;				// No wait
	timeout.tv_usec = 0;	
		
	ret = select(TCPControlSock + 1, &readfs, NULL, &errorfs, &timeout);

	if (ret == SOCKET_ERROR)
	{
		Debugprintf("Data Select failed %d ", WSAGetLastError());
		goto Lost;
	}
	if (ret > 0)
	{
		//	See what happened

		if (FD_ISSET(TCPControlSock, &readfs))
		{
			GetSemaphore();
			ProcessReceivedControl();
			FreeSemaphore();
		}
								
		if (FD_ISSET(TCPControlSock, &errorfs))
		{
Lost:	
			Debugprintf("TCP Connection lost");
			
			CONNECTED = FALSE;

			closesocket(TCPControlSock);
			TCPControlSock= 0;
			return;
		}
	}
	}
	if (DATACONNECTED)
	{

	FD_ZERO(&readfs);	
	FD_ZERO(&errorfs);

	FD_SET(TCPDataSock,&readfs);
	FD_SET(TCPDataSock,&errorfs);

	timeout.tv_sec = 0;				// No wait
	timeout.tv_usec = 0;	
		
	ret = select(TCPDataSock + 1, &readfs, NULL, &errorfs, &timeout);

	if (ret == SOCKET_ERROR)
	{
		Debugprintf("Data Select failed %d ", WSAGetLastError());
		goto DCLost;
	}
	if (ret > 0)
	{
		//	See what happened

		if (FD_ISSET(TCPDataSock, &readfs))
		{
			GetSemaphore();
			ProcessReceivedData();
			FreeSemaphore();
		}
								
		if (FD_ISSET(TCPDataSock, &errorfs))
		{
DCLost:	
			Debugprintf("TCP Connection lost");
			
			DATACONNECTED = FALSE;

			closesocket(TCPControlSock);
			TCPDataSock= 0;
			return;
		}
	}
	}
}

/*

// Buffer handling routines
	
#define BUFFLEN 1500
#define NUMBUFFS 64

UCHAR DATAAREA[BUFFLEN * NUMBUFFS] = "";

UCHAR * NEXTFREEDATA = DATAAREA;

VOID InitQueue()
{
	int i;

	NEXTFREEDATA = DATAAREA;
	NUMBEROFBUFFERS = MAXBUFFS = 0;
	
	for (i = 0; i < NUMBUFFS; i++)
	{
		ReleaseBuffer((UINT *)NEXTFREEDATA);
		NEXTFREEDATA += BUFFLEN;

		NUMBEROFBUFFERS++;
		MAXBUFFS++;
	}
}


VOID * _Q_REM(VOID *PQ, char * File, int Line)
{
	UINT * Q;
	UINT * first;
	UINT next;

	//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = (UINT *) PQ;

//	if (Semaphore.Flag == 0)
//		Debugprintf("Q_REM called without semaphore from %s Line %d", File, Line);

	first = (UINT *)Q[0];

	if (first == 0) return (0);			// Empty

	next= first[0];						// Address of next buffer

	Q[0] = next;

	return (first);
}


UINT _ReleaseBuffer(VOID *pBUFF, char * File, int Line)
{
	UINT * pointer, * BUFF = pBUFF;
	int n = 0;

//	if (Semaphore.Flag == 0)
//		Debugprintf("ReleaseBuffer called without semaphore from %s Line %d", File, Line);

	pointer = (UINT *)FREE_Q;

	*BUFF=(UINT)pointer;

	FREE_Q=(UINT)BUFF;

	QCOUNT++;

	return 0;
}

int _C_Q_ADD(VOID *PQ, VOID *PBUFF, char * File, int Line)
{
	UINT * Q;
	UINT * BUFF = (UINT *)PBUFF;
	UINT * next;
	int n = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = (UINT *) PQ;

//	if (Semaphore.Flag == 0)
//		Debugprintf("C_Q_ADD called without semaphore from %s Line %d", File, Line);


	BUFF[0]=0;							// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=(UINT)BUFF;				// New one on front
		return(0);
	}

	next = (UINT *)Q[0];

	while (next[0]!=0)
	{
		next=(UINT *)next[0];			// Chain to end of queue
	}
	next[0]=(UINT)BUFF;					// New one on end

	return(0);
}

int C_Q_COUNT(VOID *PQ)
{
	UINT * Q;
	int count = 0;

//	PQ may not be word aligned, so copy as bytes (for ARM5)

	Q = (UINT *) PQ;

	//	SEE HOW MANY BUFFERS ATTACHED TO Q HEADER

	while (*Q)
	{
		count++;
		if ((count + QCOUNT) > MAXBUFFS)
		{
			Debugprintf("C_Q_COUNT Detected corrupt Q %p len %d", PQ, count);
			return count;
		}
		Q = (UINT *)*Q;
	}

	return count;
}

VOID * _GetBuff(char * File, int Line)
{
	UINT * Temp = Q_REM(&FREE_Q);

//	FindLostBuffers();

//	if (Semaphore.Flag == 0)
//		Debugprintf("GetBuff called without semaphore from %s Line %d", File, Line);

	if (Temp)
	{
		QCOUNT--;

		if (QCOUNT < MINBUFFCOUNT)
			MINBUFFCOUNT = QCOUNT;

	}
	else
		Debugprintf("Warning - Getbuff returned NULL");

	return Temp;
}

*/