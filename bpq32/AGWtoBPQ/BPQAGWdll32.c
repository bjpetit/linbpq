
//	BPQAGWFLL32.dll

//	DLL to emulate SV"AGW's DLL Interface

//	Version 1.0 January 2011

#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include "bpq32.h"

#define DllImport	__declspec(dllimport)
#define DllExport	__declspec(dllexport)

#define Disconnect(stream) SessionControl(stream, 2, 0)
#define Connect(stream) SessionControl(stream, 1, 0)


HINSTANCE ExtDriver=0;

int AttachedProcesses = 0;

int controlStream = 0;

int Stream = 0;
int Appl = 0;
int ApplMask = 0;

char MYCALL[10] = "";
char Called[100] = "";			// allow for digis

BOOL WantPorts = FALSE;
char * PortPtr;

BOOL ReportConnected = FALSE;	
BOOL ReportDisconnected = FALSE;

UCHAR AGWMonBuffer[1000];		// Mon Buffer waiting to be read if MonDataKind Set
int MonDataLen;
int AGWMonPort;
char MonDataKind;

UCHAR DataBuffer[300];
int DataLength = 0;

int Connecting = FALSE;
int Connected = FALSE;
int ConnectedPort;

char AGWPorts[1000];
char BPQPorts[1000];

extern int AGWMONDECODE();
extern int AGWMONOPTIONS();

int PollNode();
int DoReceivedData(int Stream);
BOOL DoMonitorData();

int InternalAGWDecodeFrame(char * msg, char * buffer, int Stamp, int * FrameType);

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	HANDLE hInstance;

	char Errbuff[100];
	char buff[20];
	int i;
	hInstance=hInst;

	i=MUTEX_ALL_ACCESS;
	
	switch( ul_reason_being_called )
	{
		case DLL_PROCESS_ATTACH:
			
			AttachedProcesses++;

			_itoa(AttachedProcesses,buff,10);
			strcpy(Errbuff,	"BPQAGWDLL32 V 1.0.0.0 Process Attach - total = ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

            break;

        case DLL_PROCESS_DETACH:

			// Keep track of attaced processes

			AttachedProcesses--;

			_itoa(AttachedProcesses,buff,10);
			strcpy(Errbuff,	"BPQAGWDLL32 Process Detach - total = ");
			strcat(Errbuff,buff);
			OutputDebugString(Errbuff);

			break;

         default:

            break;
	}
	return 1;
}


BOOL WINAPI AGWStartPoll(char * MyCall, char * MyProgramNameg, int LenOfMyCall, int LenOfMyProgram)
{
	char ApplName[11];
	char * ptr;
	
	memcpy(MYCALL, MyCall, LenOfMyCall);

	// See if this is an APPLCALL - if so, save Appl Number

	for (Appl = 1; Appl < 33; Appl++)
	{
		ptr = GetApplCall(Appl);
		memcpy(ApplName, ptr, 10);
		ptr = strchr(ApplName, ' ');
		*ptr = 0;

		if (strcmp(ApplName, MYCALL) == 0)
		{
			ApplMask = 1 << (Appl - 1);
			break;
		}
	}
		
	// Get a BPQ Stream
	
	controlStream = FindFreeStream();

	if (controlStream == 255)
	{
		MessageBox (NULL, "No free BPQ Streams found", NULL, MB_ICONSTOP);
		return(FALSE);
	}

	if (Stream == 0)
		Stream = FindFreeStream();
	
	if (Stream == 255) return 0;

	SetAppl(Stream, 0, ApplMask);

	//
	//	Enable Async notification
	//
	
//	BPQSetHandle(controlStream, MainWnd);

	Connect(controlStream);					// Connect

	SetAppl(controlStream,128,0);			// Request Monitoring

	SendMsg(controlStream, "P\r", 2);		// Request Ports listing

	return TRUE;
}
//1:Fm GM8BPQ To GM8BPQ <RR P/F R0 >[22:43:24]C/R

BOOL WINAPI AGWGetData(char * FromCall, char * Data, int * DataKind, int * DataLen, int * RadioPort)
{
	PollNode();

	if (ReportConnected)
	{
		ReportConnected = FALSE;
		strcpy(FromCall, Called);
		*DataKind = 'c';
		*DataLen = wsprintf(Data, "*** CONNECTED With Station %s\r", Called);
		*RadioPort = ConnectedPort;
		return TRUE;
	}

	if (!DataLength && Stream)
		DoReceivedData( Stream);

	if (DataLength)
	{
		*DataKind = 'D';
		strcpy(FromCall, Called);
		memcpy(Data, DataBuffer, DataLength);
		*DataLen = DataLength;
		*RadioPort = ConnectedPort;
		DataLength = 0;
		return TRUE;
	}

	if (ReportDisconnected)
	{
		ReportDisconnected = FALSE;
		strcpy(FromCall, Called);
		*DataKind = 'd';
		*DataLen = wsprintf(Data, "*** DISCONNECTED From Station %s\r", Called);
		*RadioPort = 0; ConnectedPort;

		return TRUE;
	}

	if (WantPorts && AGWPorts[0])
	{
		// Send a P report with the next port line

		char * ptr = strchr(PortPtr, ';');
		int Len = ptr - PortPtr;

		if (ptr)
		{
			memcpy(Data, PortPtr, Len);
			PortPtr = ++ptr;
			*DataKind = 'P';
			*DataLen = Len;
			*RadioPort = 0;
			return TRUE;
		}
		else
			WantPorts = FALSE;			// All Sent
	}

	if (!MonDataKind)
		DoMonitorData();

	if (MonDataKind)
	{
		// Monitor Frame Available

		memcpy(Data, AGWMonBuffer, MonDataLen);
		*DataKind = MonDataKind;
		*DataLen = MonDataLen;
		*RadioPort = AGWMonPort;
		MonDataKind = 0;
		return TRUE;
	}


//	strcpy(FromCall, "G8BPQ");
//	memcpy(Data, "Test", 4);
//	*DataKind = 'A';
//	*DataLen = 4;
//	*RadioPort = 0;
	return FALSE;
}

BOOL WINAPI AGWPutData(char * ToCall, char * Data, char DataKind, int DataLen, int RadioPort)
{
	char AXCall[10];
	char TXMessage[500];
	int Digis,MsgStart,j;
	byte * TXMessageptr;
	int state, change;
	char * Via = NULL;

	switch (DataKind)
	{
		// 'c' Ask connect using no digis
		// 'v' Ask connect using digis (up to 8)
		// 'd' Ask Disconnect
		// 'D' Send data to a connected station
		// 'U' Send unproto data (Beacons,cq etc)
		// 'P' Ask Radioport information
		// 'V' Send Unproto VIA

	case 'D':
		
		SendMsg(Stream, Data, DataLen);
		break;


	case 'c':
	case 'v':
               
		SessionState(Stream,   &state, &change);		// Clear any old flags

        Connect(Stream);				// Connect
		ConnectedPort = RadioPort;
       
		ConvToAX25(MYCALL, AXCall);
		ChangeSessionCallsign(Stream, AXCall);

		strcpy(Called, ToCall);

		if (DataKind == 'v')	// Call with VIA string
		{
			strtok_s(Called, " ,", &Via);
		}

        if (memcmp(ToCall,"SWITCH",6) == 0)
		{           
		} 
        else
		{
			char ConnectMsg[80];

			Connecting = TRUE;

			// Need to convert port index (used by AGW) to port number

			if (Via)
				wsprintf(ConnectMsg, "C %d %s v %s\r", RadioPort + 1, Called, Via);
			else
				wsprintf(ConnectMsg, "C %d %s\r", RadioPort + 1, ToCall);

            SendMsg(Stream, ConnectMsg, strlen(ConnectMsg));

		}
		break;

	case 'd':

		//SendDisMsgtoAppl("*** DISCONNECTED RETRYOUT With ", sockptr);
    
        Disconnect(Stream);
		break;

	case 'P':		// Request Port List

		WantPorts = TRUE;
		PortPtr = AGWPorts;

		break;

	case 'U':
	case 'V':         // Send UNProto Frame "V" includes Via string
  
        ConvToAX25(ToCall,TXMessage);
		ConvToAX25(MYCALL,&TXMessage[7]);

		Digis=0;
        MsgStart = 0;

		if (DataKind == 'V')	// Unproto with VIA string
		{
			char * Context;
			char * ptr = strtok_s(ToCall, " ;", &Context);
            ptr = strtok_s(NULL, " ,;", &Context);				// Skip To call

			while (ptr)
			{
				ConvToAX25(ptr, &TXMessage[7+(++Digis*7)]);      // No "last" bit
				ptr = strtok_s(NULL, " ;", &Context);
			}
 		}
		
		TXMessageptr=&TXMessage[13 + (Digis * 7)];

		*(TXMessageptr++) |= 1;		// set last bit
        
		*(TXMessageptr++) = 3;     // UI

        *(TXMessageptr++) = 240;		 // PID
   
        memcpy(TXMessageptr, Data, DataLen);
        
		TXMessageptr += DataLen;

        SendRaw(RadioPort + 1, TXMessage, TXMessageptr - &TXMessage[0]);

		return TRUE;

		break;
	}
	return TRUE;
}


VOID WINAPI AGWClose()
{
}

int DoReceivedData(int Stream)
{
	int count;

	GetMsg(Stream, DataBuffer, &DataLength, &count);

	if (DataLength)
	if (Connecting)
	{
		DataBuffer[DataLength] = 0;

		if (strstr(DataBuffer, "Failure with"))
		{
			Connecting = FALSE;
			Disconnect(Stream);
		}
		else if (strstr(DataBuffer, "Connected to"))
		{
			Connecting = FALSE;
			ReportConnected = TRUE;
			DataLength = 0;				// Don't send on message
		}
	}

	return 0;
}




BOOL DoMonitorData()
{
	byte Buffer[500];
	int RawLen, Count;
	byte Port;

	int n;
	int Stamp, Frametype;
	BOOL RXFlag;

	Stamp = GetRaw(controlStream, Buffer, &RawLen, &Count );
 
//'   4 byte chain
//'   1 byte port - top bit = transmit
//'   2 byte length (LO-HI)

	if (RawLen < 7) return 0;

	Port = Buffer[4];

	if (Port > 127)
		return 0;				// Don't think we want mon tx
/*	{
		RXFlag = FALSE;
		Port = Port - 128;
	}
		else
		{
		    RXFlag = TRUE;
		}
*/

//	if (RXFlag || LoopMonFlag)    // only send txed frames if requested

	MonDataLen = InternalAGWDecodeFrame(Buffer, AGWMonBuffer, Stamp, &Frametype);
				
	if (MonDataLen > 0)
	{
		AGWMonPort = Port - 1;       // AGW Ports start from 0
        
		if (Frametype == 3)
		{
			MonDataKind = 'U';
		}
		else
		{
			if (Frametype && 1 == 0)
			{
				MonDataKind = 'I';
			}
			else
			{
				MonDataKind = 'S';
			}
		}
	}
	return FALSE;
}

PollNode()
{
	byte Buffer[400];
	int len, count, i, portcount;
	char * ptr;
	char * ptr2;
	int state, change;

	if (AGWPorts[0] == 0)
	{
		// Need to get Ports String

PortLoop:
	 
		GetMsg(controlStream, Buffer,&len, &count);

        if (AGWPorts[0] == 0)
		{
            strncat(BPQPorts, Buffer, len);
        
			len = strlen(BPQPorts);

			if (BPQPorts[len-1] == '\r')
			{       
				ptr = strchr(BPQPorts, '\r');
                
				ptr++;

				strcpy(BPQPorts, ptr);
        
                portcount = strlen(BPQPorts) / 35;
     
                ptr = _itoa(portcount,AGWPorts,10);
				if (portcount > 9) ptr++;
				*(++ptr)=';';
				*(++ptr)=0;
	
				ptr2=&BPQPorts[4];

                for (i=1; i <= portcount; i++)
				{
					memcpy(ptr, "Port", 4);
					ptr = _itoa(i, ptr+4, 10);
					ptr += 1;
					if (i > 9) ptr++;
					memcpy(ptr," with ",6);
					ptr+=6;
					memcpy(ptr,ptr2,29);		// ";"
					ptr+=29;
					ptr2+=35;
					
					while (*(--ptr) == ' ') {}

					ptr++;

					*(ptr++)=';';
				}
	    
				*(ptr)=0;
              
                Disconnect(controlStream);   // Finished with connection
				return 0;
            }

			if (count > 0)
				goto PortLoop;
		}
	}

	//	Get current Session State. Any state changed is ACK'ed
	
	if (Stream)
	{
		SessionState(Stream, &state, &change);
		
		if (change == 1)
		{
			if (state == 1)
			{
				if (!Connecting)
					ReportConnected = TRUE;					// Only report incoming connects
			}
			else
				ReportDisconnected = TRUE;
		}
	}

	return 0;
 }

int InternalAGWDecodeFrame(char * msg, char * buffer, int Stamp, int * FrameType)
{
	int	returnit;

	_asm {

	pushfd
	cld
	pushad


	mov	esi,msg
	mov	eax,Stamp
	mov	edi,buffer

	call AGWMONDECODE

	mov	edi,FrameType
	mov	[edi],eax
	mov	returnit,ecx

	popad
	popfd

	}				// End of ASM

	return (returnit);
}
