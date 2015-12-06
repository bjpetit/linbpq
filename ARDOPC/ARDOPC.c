// ARDOPC.cpp : Defines the entry point for the console application.
//


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#include "ARDOPC.h"


void CompressCallsign(char * Callsign, UCHAR * Compressed);
void CompressGridSquare(char * Square, UCHAR * Compressed);
void  ASCIIto6Bit(char * Padded, UCHAR * Compressed);
void GetTwoToneLeaderWithSync(int intSymLen, short * intLeader);
void SendID(BOOL blnEnableCWID);
void PollReceivedSamples();
void CheckTimers();


// Config parameters

char GridSquare[7] = "IO68VL";
char Callsign[10] = "G8BPQ-2";
BOOL wantCWID = FALSE;
int LeaderLength = 200;
int TrailerLength = 0;
int ARQTimeout = 120;

//

int tmrSendTimeout;

int intCalcLeader;        // the computed leader to use based on the reported Leader Length
int intRmtLeaderMeasure = 0;

enum _ReceiveState State;
enum _ARDOPState ProtocolState;
//enum _ARDOPState ARDOPState;

BOOL SoundIsPlaying = FALSE;
BOOL Capturing = TRUE;

BOOL blnAbort = FALSE;
int intRepeatCount;
BOOL blnARQDisconnect = FALSE;

enum _ProtocolMode ProtocolMode = FEC;

extern BOOL blnEnbARQRpt;
extern BOOL blnDISCRepeating;
extern char strRemoteCallsign[10];
extern char strLocalCallsign[10];
extern char strFinalIDCallsign[10];
extern int dttTimeoutTrip;
extern int dttLastFECIDSent;
extern int intFrameRepeatInterval;


int Now = 0;

const UCHAR bytValidFrameTypes[]=
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 35, 38, 41, 44, 45,
 46, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 64, 65, 66,
 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
 84, 85, 86, 87, 88, 89, 90, 91, 96, 97, 98, 99, 100, 101, 102, 103,
 104, 105, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
 124, 125, 208, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
 249, 250, 251, 252, 253, 254, 255};

int bytValidFrameTypesLength = sizeof(bytValidFrameTypes);

UCHAR isValidFrame[256]= 
{
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 00 - 0F    ACK and NAK
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 10 - 1F
	0,0,0,1,0,0,1,0,0,1,0,0,1,1,1,0,	// BREAK=23, IDLE=26, DISC=29, END=2C, ConRejBusy=2D, ConRejBW=2E

	1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,	// 30 - 3F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 40 - 4F
	1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,	// 50 - 5F
	1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	// 60 - 6F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,	// 70 - 7F
			

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 80 - 8F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 90 - 9F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// A0 - AF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// B0 - BF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// C0 - CF

	// experimental SOUNDINGs D0

	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// D0 - DF
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// e0 - eF    ACK and NAK
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1		// f0 - ff
};


BOOL blnTimeoutTriggered= FALSE;

int MaxCorrections;

char TXQueue[100] = "HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n";					// May malloc this, or change to cyclic buffer
//char TXQueue[100] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUHelloHelloHelloHelloHello\r\n";					// May malloc this, or change to cyclic buffer
int TXQueueLen = 96;

extern UCHAR bytSessionID;

int intLastRcvdFrameQuality;

int intAmp = 26000;	   // Selected to have some margin in calculations with 16 bit values (< 32767) this must apply to all filters as well. 


const char strFrameType[256][16] = {
	"DataNAK", //  Range &H00 to &H1F includes 5 bits for quality 1 Car, 200Hz,4FSK
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","",

        //Short Control Frames 1 Car, 200Hz,4FSK  ' Reassigned May 22, 2015 for maximum "distance"
 
	"BREAK", "", "",
	"IDLE", "", "",
	"DISC", "", "",
	"END",
	"ConRejBusy",
	"ConRejBW",
	"",

	//Special frames 1 Car, 200Hz,4FSK 0x30 +

	"IDFrame",
	"ConReq200M",
	"ConReq500M",
	"ConReq1000M",
	"ConReq2000M",
	"ConReq200F",
	"ConReq500F",
	"ConReq1000F",
	"ConReq2000F",
	"ConAck200",
	"ConAck500",
	"ConAck1000",
	"ConAck2000",
	// Types &H3D to &H3F reserved
	"","","",
	// 200 Hz Bandwidth Data 
	// 1 Car PSK Data Modes 200 HzBW  100 baud 
	
	"4PSK.200.100.E",	// 0x40
	"4PSK.200.100.O",
	"4PSK.200.100S.E",
	"4PSK.200.100S.O",
	"8PSK.200.100.E",
	"8PSK.200.100.O",

	// 1 Car 4FSK Data mode 200 HzBW, 50 baud 

	"4FSK.200.50.E",
	"4FSK.200.50.O",
	"4FSK.200.50S.E", // 48
	"4FSK.200.50S.O",
	"4FSK.500.100.E",
	"4FSK.500.100.O",
	"4FSK.500.100S.E",
	"4FSK.500.100S.O",
	"8FSK.200.25.E",
	"8FSK.200.25.O",

	//' 2 Car PSK Data Modes 100 baud
	"4PSK.500.100.E", // 50
	"4PSK.500.100.O",
	"8PSK.500.100.E",
	"8PSK.500.100.O",

	// 2 Car Data modes 167 baud  
	"4PSK.500.167.E",
	"4PSK.500.167.O",
	"8PSK.500.167.E",
	"8PSK.500.167.O",

	// 1 Car 16FSK mode 25 baud

	"16FSK.500.25.E", // 58
	"16FSK.500.25.O",
	"16FSK.500.25S.E",
	"16FSK.500.25S.O", "","","","",

	//1 Khz Bandwidth Data Modes 
	//  4 Car 100 baud PSK
	"4PSK.1000.100.E", //60
	"4PSK.1000.100.O",
	"8PSK.1000.100.E",
	"8PSK.1000.100.O",
	// 4 car 167 baud PSK
	"4PSK.1000.167.E",
	"4PSK.1000.167.O",
	"8PSK.1000.167.E",
	"8PSK.1000.167.O",
	// 2 Car 4FSK 100 baud
	"4FSK.1000.100.E", //68
	"4FSK.1000.100.O","","","","","","",

	// 2Khz Bandwidth Data Modes 
	//  8 Car 100 baud PSK
	"4PSK.2000.100.E", //70 
	"4PSK.2000.100.O",
	"8PSK.2000.100.E",
	"8PSK.2000.100.O",
	//  8 Car 167 baud PSK
	"4PSK.2000.167.E",
	"4PSK.2000.167.O",
	"8PSK.2000.167.E",
	"8PSK.2000.167.O",
	// 4 Car 4FSK 100 baud
	"4FSK.2000.100.E",
	"4FSK.2000.100.O",
	// 1 Car 4FSK 600 baud (FM only)
	"4FSK.2000.600.E", // Experimental 
	"4FSK.2000.600.O", // Experimental
	"4FSK.2000.600S.E", // Experimental
	"4FSK.2000.600S.O", // Experimental //7d
	"","",	// 7e-7f
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","", //C0

	//Frame Types &HA0 to &HDF reserved for experimentation 
	"SOUND2K" //D0
	"","","","","","","","","","","","","","","","",
    //Data ACK  1 Car, 200Hz,4FSK
	"DataACK"		// Range &HE0 to &HFF includes 5 bits for quality 
};

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

#ifdef WIN32
float round(float x)
{
	return floorf(x + 0.5f);
}
#endif

void GetSemaphore()
{
}

void FreeSemaphore()
{
}

BOOL CheckValidCallsignSyntax(char * strTargetCallsign)
{
	return TRUE;
}


// Function polled by Main polling loop to see if time to play next wave stream

BOOL GetNextFrame()
{
	// returning TRUE sets frame pending in Main

	if (ProtocolMode == FEC || ProtocolState == FECSend)
	{
		if (ProtocolState == FECSend || ProtocolState == FECRcv || ProtocolState == DISC)
			return GetNextFECFrame();
		else
			return FALSE;
	}
	if (ProtocolMode == ARQ)
		if (ARQState == None)
			return FALSE;
		else
            return GetNextARQFrame();

	return FALSE;
}
     
//  Function to Get the next ARQ frame returns TRUE if frame repeating is enable 

BOOL GetNextARQFrame()
{
	//Dim bytToMod(-1) As Byte

	char HostCmd[80];

	if (blnAbort)  // handles ABORT (aka Dirty Disconnect)
	{
		//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.GetNextARQFrame] ABORT...going to ProtocolState DISC, return FALSE")

		ClearDataToSend();
		
		ProtocolState = DISC;
		InitializeConnection();
		blnAbort = FALSE;
		blnEnbARQRpt = FALSE;
		blnDISCRepeating = FALSE;
		return FALSE;
	}

	if (blnDISCRepeating)	// handle the repeating DISC reply 
	{
		intRepeatCount += 1;
		blnEnbARQRpt = FALSE;

		if (intRepeatCount > 5)  // do 5 tries then force disconnect 
		{
			sprintf(HostCmd, "STATUS END NOT RECEIVED CLOSING ARQ SESSION WITH %s", strRemoteCallsign);
			QueueCommandToHost(HostCmd);
			blnDISCRepeating = FALSE;
			blnEnbARQRpt = FALSE;
			ClearDataToSend();
			ProtocolState = DISC;
			InitializeConnection();
			return FALSE;			 //indicates end repeat
		}
		return TRUE;			// continue with DISC repeats
	}
/*        If GetARDOPProtocolState = ProtocolState.ISS And ARQState = ARQSubStates.ISSConReq Then ' Handles Repeating ConReq frames 
            intRepeatCount += 1
            If intRepeatCount > MCB.ARQConReqRepeats Then
                ClearDataToSend()
                SetARDOPProtocolState(ProtocolState.DISC)
                If stcConnection.strRemoteCallsign.Trim <> "" Then
                    objMain.objHI.QueueCommandToHost("STATUS CONNECT TO " & stcConnection.strRemoteCallsign & " FAILED!")
                    InitializeConnection() : return FALSE 'indicates end repeat
                Else
                    objMain.objHI.QueueCommandToHost("STATUS END ARQ CALL")
                    InitializeConnection() : return FALSE 'indicates end repeat
                End If
                ' Clear the mnuBusy status on the main form
                Dim stcStatus As Status = Nothing
                stcStatus.ControlName = "mnuBusy"
                queTNCStatus.Enqueue(stcStatus)
            Else
                return TRUE ' continue with repeats
            End If

        ElseIf GetARDOPProtocolState = ProtocolState.ISS And ARQState = ARQSubStates.IRSConAck Then ' Handles ISS repeat of ConAck
            intRepeatCount += 1
            If intRepeatCount <= MCB.ARQConReqRepeats Then
                return TRUE
            Else
                SetARDOPProtocolState(ProtocolState.DISC) : ARQState = ARQSubStates.DISCArqEnd
                objMain.objHI.QueueCommandToHost("STATUS CONNECT TO " & stcConnection.strRemoteCallsign & " FAILED!") : InitializeConnection() : return FALSE
            End If
			*/

	// Handles a timeout from an ARQ conneceted State

	if (ProtocolState == ISS || ProtocolState == IDLE || ProtocolState == IRS)
	{
		if ((Now - dttTimeoutTrip) / 1000 > ARQTimeout) // (Handles protocol rule 1.7)
		{
            if (!blnTimeoutTriggered)
			{
				//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.GetNexARQFrame] Timeout setting SendTimeout timer to start.")

				blnEnbARQRpt = FALSE;
				blnTimeoutTriggered = TRUE; // prevents a retrigger
                tmrSendTimeout = 100;
				return FALSE;
			}
		}
	}

	// Handles the DISC state (no repeats)
 
	if (ProtocolState == DISC) // never repeat in DISC state
	{
		blnARQDisconnect = FALSE;
		return FALSE;
	}

	// ' Handles all other possibly repeated Frames

	return blnEnbARQRpt;  // not all frame types repeat...blnEnbARQRpt is set/cleared in ProcessRcvdARQFrame
}

extern LARGE_INTEGER Frequency;
extern LARGE_INTEGER StartTicks;
extern LARGE_INTEGER NewTicks;
	
void ardopmain()
{
	blnTimeoutTriggered = FALSE;
	ProtocolState = DISC;

//	GenerateTwoToneLeaderTemplate();
//	GenerateFSKTemplates();
//	GeneratePSKTemplates();

	InitSound();

	ProtocolMode = ARQ;
	SendARQConnectRequest("GM8BPQ", "GM8BPQ-2");

//	ProtocolState = FECSend;
//	GetNextFECFrame();
//	ProtocolState = FECSend;
//	GetNextFECFrame();
//	ProtocolState = FECSend;
//	GetNextFECFrame();

//	ProtocolState = DISC;

	ProtocolMode = ARQ;

	while(1)
	{
		// Get Time Now is millisecs since program start

#ifdef WIN32
		QueryPerformanceCounter(&NewTicks);
		Now = (NewTicks.QuadPart - StartTicks.QuadPart) / Frequency.QuadPart;
#endif
	//	if (Capturing)
			PollReceivedSamples();

		CheckTimers();	

		if (!SoundIsPlaying)
			GetNextARQFrame();
	

		Sleep(10);
	}

	SendID(0);

#ifdef WIN32
	 
	ProtocolState = FECSend;
//	GetNextFECFrame();

	ProtocolState = FECSend;
//	GetNextFECFrame();

	ProtocolState = FECSend;
//	GetNextFECFrame();

#endif
// 
//  SendID(0);
//  ModTwoToneTest();

  return ;
}


void SendCWID(char * Callsign, BOOL x)
{
}

// Subroutine to generate 1 symbol of leader

//	 returns pointer to Frame Type Name

const char * Name(UCHAR bytID)
{
	if (bytID < 0x20)
		return strFrameType[0];
	else if (bytID >= 0xE0)
		return strFrameType[0xE0];
	else
		return strFrameType[bytID];
}

// Function to look up frame info from bytFrameType

BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
			   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType)
{
	//Used to "lookup" all parameters by frame Type. 
	// returns TRUE if all fields updated otherwise FALSE (improper bytFrameType)

	// 1 Carrier 4FSK control frames 
   
	if ((bytFrameType >= 0 &&  bytFrameType <= 0x1F) || bytFrameType >= 0xE0)
	{
		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 40;
	}
	else
	{

	switch(bytFrameType)
	{
	case 0x23:
	case 0x26:
	case 0x29:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;
	
	case 0x2C:
	case 0x2D:
	case 0x2E:
		
		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;

	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 12;
		*intRSLen = 2;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 50;
		break;

	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 3;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 50;
		break;

	case 0xe0:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;

	// 1 Carrier Data modes
    //  100 baud PSK (200 baud not compatible with 200 Hz bandwidth)  (Note 1 carrier modes Qual Threshold reduced to 30 (was 50) for testing April 20, 2015

	case 0x40:
	case 0x41:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 30;	
		break;
	
	case 0x42:
	case 0x43:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 16;
		*intRSLen = 8;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;
	
	case 0x44:
	case 0x45:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;

	// 50 baud 4FSK 200 Hz bandwidth

	case 0x46:
	case 0x47:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 30;
		break;

	case 0x48:
	case 0x49:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 16;
		*intRSLen = 4;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 30;
		break;
	
	// 100 baud 4FSK 500 Hz bandwidth

	case 0x4A:
	case 0x4B:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;
 
 	case 0x4C:
	case 0x4D:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;

	// 25 baud 8FSK 200 Hz bandwidth (in testing)
 
 	case 0x4E:
	case 0x4F:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 24;
		*intRSLen = 6;
		strcpy(strMod, "8FSK");
		*intBaud = 25;
		*bytQualThres = 30; 
		break;
		
	// 25 baud 16FSK 500 Hz bandwidth 

 	case 0x58:
	case 0x59:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "16FSK");
		*intBaud = 25;
		*bytQualThres = 30;
 		break;

	// 600 baud 4FSK 2000 Hz bandwidth 

	case 0x7a:
	case 0x7b:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 600;
		*intRSLen = 150;
		strcpy(strMod, "4FSK");
		*intBaud = 600;
		*bytQualThres = 30;
		break;
 
	case 0x7C:
	case 0x7D:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 200;
		*intRSLen = 50;
		strcpy(strMod, "4FSK");
		*intBaud = 600;
		*bytQualThres = 30;
		break;
 
	// 2 Carrier Data Modes
	// 100 baud 

	case 0x50:
	case 0x51:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 2;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
 		break;

	case 0x52:
	case 0x53:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;
						
	// 167 baud testing
    
	case 0x54:
	case 0x55:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
		
	case 0x56:
	case 0x57:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 150;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	//' 4 Carrier Data Modes
	//	100 baud
     	
	case 0x60:
	case 0x61:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;
	
	case 0x62:
	case 0x63:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	//	167 Baud

	case 0x64:
	case 0x65:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	case 0x66:
	case 0x67:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 159;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
	
	// 100 baud 2 carrier 4FSK
     
	case 0x68:
	case 0x69:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	// 8 Carrier Data modes
	//	100 baud

	case 0x70:
	case 0x71:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	case 0x72:
	case 0x73:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	// 167 baud

	case 0x74:
	case 0x75:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
		
	case 0x76:
	case 0x77:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 159;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	// 100 baud 4 carrier 4FSK
 
	case 0x78:
	case 0x79:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	default:
		//'Logs.Exception("[PSKDataInfo] No data for frame type= H" & Format(bytFrameType, "x"))
        return FALSE;
	}
	}
	
	if (bytFrameType >= 0 && bytFrameType <= 0x1F)
		strcpy(strType,strFrameType[0]);
	else
		if (bytFrameType >= 0xE0) 
			strcpy(strType,strFrameType[0xE0]);
        else
			strcpy(strType,strFrameType[bytFrameType]);

	return TRUE;
}

int NPAR = -1;	// Number of Parity Bytes - used in RS Code

int RSEncode(UCHAR * bytToRS, UCHAR * RSBytes, int DataLen, int RSLen)
{
	// This just returns the Parity Bytes. I don't see the point
	// in copying the message about

	unsigned char Padded[256];		// The padded Data

	int Length = DataLen + RSLen;	// Final Length of packet
	int PadLength = 255 - Length;	// Padding bytes needed for shortened RS codes

	//	subroutine to do the RS encode. For full length and shortend RS codes up to 8 bit symbols (mm = 8)

	if (NPAR != RSLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = RSLen;
		initialize_ecc();
	}

	// Copy the supplied data to end of data array.

	memset(Padded, 0, PadLength);
	memcpy(&Padded[PadLength], bytToRS, DataLen); 

	encode_data(Padded, 255-RSLen, RSBytes);

	return RSLen;
}

//	Main RS decode function


BOOL RSDecode(UCHAR * bytRcv, int Length, int CheckLen, BOOL * blnRSOK)
{	
	// Using a modified version of Henry Minsky's code
	
	//Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009

	// Rick's Implementation processes the byte array in reverse. and also 
	//	has the check bytes in the opposite order. I've modified the encoder
	//	to allow for this, but so far haven't found a way to mske the decoder
	//	work, so I have to reverse the data and checksum to decode G8BPQ Nov 2015

	//	returns TRUE if was ok or correction succeeded, FALSE if correction impossible

	UCHAR intTemp[256];				// WOrk Area to pass to Decoder		
	int i;
	UCHAR * ptr2 = intTemp;
	UCHAR * ptr1 = &bytRcv[Length - CheckLen -1]; // Last Byte of Data

	int DataLen = Length - CheckLen;
	int PadLength = 255 - Length;		// Padding bytes needed for shortened RS codes

	*blnRSOK = FALSE;

	if (Length > 255 || Length < (1 + CheckLen))		//Too long or too short 
		return FALSE;

	if (NPAR != CheckLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = CheckLen;
		initialize_ecc();
	}


	//	We reverse the data while zero padding it to speed things up

	//	We Need (Data Reversed) (Zero Padding) (Checkbytes Reversed)

	// Reverse Data

	for (i = 0; i < DataLen; i++)
	{
	  *(ptr2++) = *(ptr1--);
	}

	//	Clear padding

	memset(ptr2, 0, PadLength);	

	ptr2+= PadLength;
	
	// Error Bits

	ptr1 = &bytRcv[Length - 1];			// End of check bytes

	for (i = 0; i < CheckLen; i++)
	{
	  *(ptr2++) = *(ptr1--);
	}
	
	decode_data(intTemp, 255);

	// check if syndrome is all zeros 

	if (check_syndrome() == 0)
	{
		// RS ok, so no need to correct

		*blnRSOK = TRUE;
		return TRUE;		// No Need to Correct
	}

    if (correct_errors_erasures (intTemp, 255, 0, 0) == 0) // Dont support erasures at the momnet

		// Uncorrectable

		return FALSE;

	// Data has been corrected, so need to reverse again

	ptr1 = &intTemp[DataLen - 1];
	ptr2 = bytRcv; // Last Byte of Data

	for (i = 0; i < Length; i++)
	{
	  *(ptr2++) = *(ptr1--);
	}

	// ?? Do we need to return the check bytes ??
 
	return TRUE;
}

/*

Old code

	// convert to indexed form

	for(i = 0; i < 256; i++)
	{
		intIsave = i;
		intIndexSave = index_of[intTemp[i]];
		recd[i] = index_of[intTemp[i]];
	}

	printtick("entering decode_rs");

	decode_rs();

	printtick("decode_rs Done");

	*blnRSOK = blnErrorsCorrected;

	if(blnErrorsCorrected)
	{
		for (i = 0; i < Length - (2 * tt); i++)
		{
			Corrected[i] = recd[i + intStartIndex];
		}
	}
	else
	{
		// Just return input minus RS (can't we just use input??

		memcpy(Corrected, bytRcv, Length - 2 * tt);
		*blnRSOK = TRUE;
	}
}
*/

// Function to encode data for all PSK frame types

int EncodePSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedData)
{
	// Objective is to use this to use this to send all PSK data frames 
	// 'Output is a byte array which includes:
	//  1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
	//  2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
	//  Ininitial implementation:
	//    intNum Car may be 1, 2, 4 or 8
	//    intBaud may be 100, 167
	//    intPSKMode may be 4 (4PSK) or 8 (8PSK) 
	//    bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned

	//  First determine if bytDataToSend is compatible with the requested modulation mode.

	int intNumCar, intBaud, intDataLen, intRSLen, intDataToSendPtr, intEncodedDataPtr;

	int intCarDataCnt, intStartIndex;
	BOOL blnOdd;
	char strType[16];
	char strMod[16];
	BOOL blnFrameTypeOK;
	UCHAR bytQualThresh;
	int i;
	UCHAR * bytToRS = &bytEncodedData[2]; 

	blnFrameTypeOK = FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	if (intDataLen == 0 || Length == 0 || !blnFrameTypeOK)
	{
		//Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
		return 0;
	}
		
	//	Generate the 2 bytes for the frame type data:
	
	bytEncodedData[0] = bytFrameType;
	bytEncodedData[1] = bytFrameType ^ bytSessionID;


	intDataToSendPtr = 0;
	intEncodedDataPtr = 2;

	// Now compute the RS frame for each carrier in sequence and move it to bytEncodedData 
		
	for (i = 0; i < intNumCar; i++)		//  across all carriers
	{
			intCarDataCnt = Length - intDataToSendPtr;
			
			if (intCarDataCnt >= intDataLen) // why not > ??
			{
				// Won't all fit 

				bytToRS[0] = intDataLen;
				intStartIndex = intEncodedDataPtr;
				memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intDataLen);
				intDataToSendPtr += intDataLen;
			}
			else
			{
				// Last bit

				bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

				if (intCarDataCnt > 0)
				{
					memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intCarDataCnt);
                    intDataToSendPtr += intCarDataCnt;
				}	
			}
		
			GenCRC16FrameType(bytToRS, intDataLen + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

 			RSEncode(bytToRS, bytToRS+intDataLen+3, intDataLen + 3, intRSLen);  // Generate the RS encoding ...now 14 bytes total
     
 			//  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)

 			intEncodedDataPtr += intDataLen + 3 + intRSLen;

			bytToRS += intDataLen + 3 + intRSLen;
		
	}
	return intEncodedDataPtr;
}

 

// Function to encode data for all FSK frame types
  
int EncodeFSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedData)
{
	// Objective is to use this to use this to send all 4FSK data frames 
	// 'Output is a byte array which includes:
	//  1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
	//  2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
	//  Ininitial implementation:
	//    intNum Car may be 1, 2, 4 or 8
	//    intBaud may be 50, 100
	//    strMod is 4FSK) 
	//    bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned

	//  First determine if bytDataToSend is compatible with the requested modulation mode.

	int intNumCar, intBaud, intDataLen, intRSLen, intDataToSendPtr, intEncodedDataPtr;

	int intCarDataCnt, intStartIndex;
	BOOL blnOdd;
	char strType[16];
	char strMod[16];
	BOOL blnFrameTypeOK;
	UCHAR bytQualThresh;
	int i;
	UCHAR * bytToRS = &bytEncodedData[2]; 

	blnFrameTypeOK = FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	if (intDataLen == 0 || Length == 0 || !blnFrameTypeOK)
	{
		//Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
		return 0;
	}
	
	//	Generate the 2 bytes for the frame type data:
	
	bytEncodedData[0] = bytFrameType;
	bytEncodedData[1] = bytFrameType ^ bytSessionID;

     //   Dim bytToRS(intDataLen + 3 - 1) As Byte ' Data + Count + 2 byte CRC

	intDataToSendPtr = 0;
	intEncodedDataPtr = 2;

	if (intBaud < 600 || intDataLen < 600)
	{
		// Now compute the RS frame for each carrier in sequence and move it to bytEncodedData 
		
		for (i = 0; i < intNumCar; i++)		//  across all carriers
		{
			intCarDataCnt = Length - intDataToSendPtr;
			
			if (intCarDataCnt >= intDataLen) // why not > ??
			{
				// Won't all fit 

				bytToRS[0] = intDataLen;
				intStartIndex = intEncodedDataPtr;
				memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intDataLen);
				intDataToSendPtr += intDataLen;
			}
			else
			{
				// Last bit

				bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

				if (intCarDataCnt > 0)
				{
					memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intCarDataCnt);
                    intDataToSendPtr += intCarDataCnt;
				}	
			}
		
			GenCRC16FrameType(bytToRS, intDataLen + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

			RSEncode(bytToRS, bytToRS+intDataLen+3, intDataLen + 3, intRSLen);  // Generate the RS encoding ...now 14 bytes total

 			//  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)

 			intEncodedDataPtr += intDataLen + 3 + intRSLen;

			bytToRS += intDataLen + 3 + intRSLen;
		}
	}

	// special case for 600 baud 4FSK which has 600 byte data field sent as three sequencial (200 byte + 50 byte RS) groups

	for (i = 0; i < 3; i++)		 // for three blocks of RS data
	{
		intCarDataCnt = Length - intDataToSendPtr;
			
		if (intCarDataCnt >= intDataLen /3 ) // why not > ??
		{
			// Won't all fit 

			bytToRS[0] = intDataLen / 3;
			intStartIndex = intEncodedDataPtr;
			memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intDataLen / 3);
			intDataToSendPtr += intDataLen /3;
		}
		else
		{
			// Last bit

			bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

			if (intCarDataCnt > 0)
			{
				memcpy(&bytToRS[1], &bytDataToSend[intDataToSendPtr], intCarDataCnt);
                intDataToSendPtr += intCarDataCnt;
			}	
		}
		GenCRC16FrameType(bytToRS, intDataLen / 3 + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

 		RSEncode(bytToRS, bytToRS + intDataLen / 3 + 3, intDataLen / 3 + 3, intRSLen / 3);  // Generate the RS encoding ...now 14 bytes total
		intEncodedDataPtr += intDataLen / 3  + 3 + intRSLen / 3;
		bytToRS += intDataLen / 3  + 3 + intRSLen / 3;
	}		
	return intEncodedDataPtr;
}
	
//  Function to encode ConnectRequest frame 

BOOL EncodeARQConRequest(char * strMyCallsign, char * strTargetCallsign, enum _ARQBandwidth ARQBandwidth, UCHAR * bytReturn)
{
	//  Encodes a 4FSK 200 Hz BW Connect Request frame ( ~ 1950 ms with default leader/trailer)

	UCHAR * bytToRS= &bytReturn[2];

	if (strcmp(strTargetCallsign, "CQ") != 0)  // skip syntax checking for psuedo call "CQ"
	{
		if (!CheckValidCallsignSyntax(strMyCallsign))
		{
			//Logs.Exception("[EncodeModulate.EncodeARQConnectRequest] Illegal Call sign syntax. MyCallsign = " & strMyCallsign & ", TargetCallsign = " & strTargetCallsign)

			return 0;
		}
		if (!CheckValidCallsignSyntax(strTargetCallsign) || !CheckValidCallsignSyntax(strMyCallsign))
		{            
			//Logs.Exception("[EncodeModulate.EncodeARQConnectRequest] Illegal Call sign syntax. MyCallsign = " & strMyCallsign & ", TargetCallsign = " & strTargetCallsign)
			
			return 0;
		}
	}		
	if (ARQBandwidth == B200MAX)
		bytReturn[0] = 0x31;
	else if (ARQBandwidth == B500MAX)
		bytReturn[0] = 0x32;
	else if (ARQBandwidth == B1000MAX)
		bytReturn[0] = 0x33;
	else if (ARQBandwidth == B2000MAX)
		bytReturn[0] = 0x34;

	else if (ARQBandwidth == B200FORCED)
		bytReturn[0] = 0x35;
	else if (ARQBandwidth == B500FORCED)
		bytReturn[0] = 0x36;
	else if (ARQBandwidth == B1000FORCED)
		bytReturn[0] = 0x37;
	else if (ARQBandwidth == B2000FORCED)
		bytReturn[0] = 0x38;
	else
	{
		//Logs.Exception("[EncodeModulate.EncodeFSK500_1S] Bandwidth error.  Bandwidth = " & strBandwidth)
		return 0;
	}

	bytReturn[1] = bytReturn[0] ^ 0xFF;  // Connect Request always uses session ID of &HFF

	// Modified May 24, 2015 to use RS instead of 2 byte CRC. (same as ID frame)

	CompressCallsign(strMyCallsign, &bytToRS[0]);
	CompressCallsign(strTargetCallsign, &bytToRS[6]);  //this uses compression to accept 4, 6, or 8 character Grid squares.

	RSEncode(bytToRS, &bytReturn[14], 12, 2);  // Generate the RS encoding ...now 14 bytes total
 
	return 16;
}


 
int Encode4FSKIDFrame(char * Callsign, char * Square, unsigned char * bytreturn)
{
	// Function to encodes ID frame 
	// returns length of encoded message 

	UCHAR * bytToRS= &bytreturn[2];

	 if (!CheckValidCallsignSyntax(Callsign))
	 {
		//       Logs.Exception("[EncodeModulate.EncodeIDFrame] Illegal Callsign syntax or Gridsquare length. MyCallsign = " & strMyCallsign & ", Gridsquare = " & strGridSquare)
		
		 return 0;
	 }

	bytreturn[0] = 0x30;
	bytreturn[1] = 0x30 ^ 0xFF;

	// Modified May 9, 2015 to use RS instead of 2 byte CRC.
       
	CompressCallsign(Callsign, &bytToRS[0]);

    if (Square[0])
		CompressGridSquare(Square, &bytToRS[6]);  //this uses compression to accept 4, 6, or 8 character Grid squares.

	RSEncode(bytToRS, &bytreturn[14], 12, 2);  // Generate the RS encoding ...now 14 bytes total

	return 16;
}

//  Funtion to encodes a short 4FSK 50 baud Control frame  (2 bytes total) BREAK, END, DISC, IDLE, ConRejBusy, ConRegBW  

int Encode4FSKControl(UCHAR bytFrameType, UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes a short control frame (normal length ~320 ms with default 160 ms leader+trailer) 
    
	//If IsShortControlFrame(intFrameCode) Then
    //        Logs.Exception("[EncodeModulate.EncodeFSKControl] Illegal control frame code: H" & Format(intFrameCode, "X"))
    //        return Nothing
    //    End If

	bytreturn[0] = bytFrameType;
	bytreturn[1] = bytFrameType ^ bytSessionID;

	return 2;		// Length
}

//  Function to encode a CONACK frame with Timing data  (6 bytes total)  

int EncodeConACKwTiming(UCHAR bytFrameType, int intRcvdLeaderLenMs, UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes a Connect ACK with one byte Timing info. (Timing info repeated 2 times for redundancy) 

	//If intFrameCode < &H39 Or intFrameCode > &H3C Then
    //        Logs.Exception("[EncodeConACKwTiming] Illegal Frame code: " & Format(intFrameCode, "X"))
    //        return Nothing
    //    End If

	UCHAR bytTiming = min(255, intRcvdLeaderLenMs / 10);  // convert to 10s of ms.

	if (intRcvdLeaderLenMs > 2550 || intRcvdLeaderLenMs < 0)
	{
		// Logs.Exception("[EncodeConACKwTiming] Timing value out of range: " & intRcvdLeaderLenMs.ToString & " continue with forced value = 0")
        bytTiming = 0;
	}

	bytreturn[0] = bytFrameType;
	bytreturn[1] = bytFrameType ^ bytSessionID;

	bytreturn[2] = bytTiming;
	bytreturn[3] = bytTiming;

	return 4;
}

//	' Function to encode an ACK control frame  (2 bytes total) ...with 5 bit Quality code 

int EncodeDATAACK(int intQuality, UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes intQuality and DataACK frame (normal length ~320 ms with default leader/trailer)

	int intScaledQuality;

	intScaledQuality = max(0, (intQuality / 2) - 19); // scale quality value to fit 5 bit field of 0 represents Q <= of 38 (pretty poor)
	
	bytreturn[0] = 0xE0 + intScaledQuality;		//ACKs 0xE0 - 0xFF
	bytreturn[1] = bytreturn[0] ^ bytSessionID;

	return 2;
}

//  Function to encode a NAK frame  (2 bytes total) ...with 5 bit Quality code 

int EncodeDATANAK(int intQuality , UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes intQuality and DataACK frame (normal length ~320 ms with default leader/trailer)

	int intScaledQuality;

	intScaledQuality = max(0, (intQuality / 2) - 19); // scale quality value to fit 5 bit field of 0 represents Q <= of 38 (pretty poor)
	
	bytreturn[0] = intScaledQuality;		// NAKS 00 - 0x1F
	bytreturn[1] = bytreturn[0] ^ bytSessionID;

	return 2;
}



void SendID(BOOL blnEnableCWID)
{
	unsigned char bytEncodedBytes[16];
	unsigned char bytIDSent[80];
	int Len;

	// Schedular needs to ensure this isnt called if already playing

	if (SoundIsPlaying)
		return;

	return;

    if (GridSquare[0] == 0)
	{
		Len = Encode4FSKIDFrame(Callsign, "No GS", bytEncodedBytes);
		sprintf(bytIDSent," %s:[No GS] ", Callsign);
	}
	else
	{
		Len = Encode4FSKIDFrame(Callsign, GridSquare, bytEncodedBytes);
		Len = sprintf(bytIDSent," %s:[%s] ", Callsign, GridSquare);
	}

	AddTagToDataAndSendToHost(bytIDSent, "IDF", Len);

	// On embedded platforms we don't have the memory to create full sound stream before playiong,
	// so this is structured differently from Rick's code

	Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent

    if (blnEnableCWID)
		sendCWID(Callsign, FALSE);

}

// Function to generate a 5 second burst of two tone (1450 and 1550 Hz) used for setting up drive level
 
void ModTwoToneTest()
{
	short intLeader[100000];
	GetTwoToneLeaderWithSync(500, &intLeader[0]);
	SampleSink(0);	// 5 secs
	SoundFlush();
}


//  Subroutine to send 5 seconds of two tone

void Send5SecTwoTone()
{
	if (!SoundIsPlaying)
	{
//		CreateWaveStream(ModTwoToneTest());
	}
}



void  ASCIIto6Bit(char * Padded, UCHAR * Compressed)
{
	// Input must be 8 bytes which will convert to 6 bytes of packed 6 bit characters and
	// inputs must be the ASCII character set values from 32 to 95....
    
	unsigned long long intSum = 0;

	int i;

	for (i=0; i<4; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[0] = (UCHAR)(intSum >> 16) & 255;
	Compressed[1] = (UCHAR)(intSum >> 8) &  255;
	Compressed[2] = (UCHAR)intSum & 255;

	intSum = 0;

	for (i=4; i<8; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[3] = (UCHAR)(intSum >> 16) & 255;
	Compressed[4] = (UCHAR)(intSum >> 8) &  255;
	Compressed[5] = (UCHAR)intSum & 255;
}

void Bit6ToASCII(UCHAR * Padded, UCHAR * UnCompressed)
{
	// uncompress 6 to 8

	// Input must be 6 bytes which represent packed 6 bit characters that well 
	// result will be 8 ASCII character set values from 32 to 95...

	unsigned long long intSum = 0;

	int i;

	for (i=0; i<3; i++)
	{
		intSum = (intSum << 8) + Padded[i];
	}

	UnCompressed[0] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[1] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[2] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[3] = (UCHAR)(intSum & 63) + 32;

	intSum = 0;

	for (i=3; i<6; i++)
	{
		intSum = (intSum << 8) + Padded[i] ;
	}

	UnCompressed[4] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[5] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[6] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[7] = (UCHAR)(intSum & 63) + 32;
}


// Function to compress callsign (up to 7 characters + optional "-"SSID   (-0 to -15 or -A to -Z) 
    
void CompressCallsign(char * inCallsign, UCHAR * Compressed)
{
	char Callsign[10];
	char Padded[16];
	int SSID;
	char * Dash;

	memcpy(Callsign, inCallsign, 10);
	Dash = strchr(Callsign, '-');
	
	if (Dash == 0)		// if No SSID
	{
		strcpy(Padded, Callsign);
		strcat(Padded, "    ");
		Padded[7] = '0';			//  "0" indicates no SSID
	}
	else
	{
		*(Dash++) = 0;
		SSID = atoi(Dash);

		strcpy(Padded, Callsign);
		strcat(Padded, "    ");

		if (SSID >= 10)		// ' handles special case of -10 to -15 : ; < = > ? '
			Padded[7] = ':' + *(Dash) - 10;
		else
			Padded[7] = *(Dash);
	}

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

// Function to compress Gridsquare (up to 8 characters)

void CompressGridSquare(char * Square, UCHAR * Compressed)
{
	char Padded[17];
        
	if (strlen(Square) > 8)
		return;

	strcpy(Padded, Square);
	strcat(Padded, "        ");

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

// Function to decompress 6 byte call sign to 7 characters plus optional -SSID of -0 to -15 or -A to -Z
  
void DeCompressCallsign(char * bytCallsign, char * returned)
{
	char bytTest[10] = "";
	char SSID[8] = "";
    
	Bit6ToASCII(bytCallsign, bytTest);

	memcpy(returned, bytTest, 6);
	returned[6] = 0;
	strlop(returned, ' ');		// remove trailing space

	if (bytTest[7] == '0') // Value of "0" so No SSID
		returned[6] = 0;
	else if (bytTest[7] >= 58 && bytTest[7] <= 63) //' handles special case for -10 to -15
		sprintf(SSID, "-%d", bytTest[7] - 48);
	else
		sprintf(SSID, "-%c", bytTest[7]);
	
	strcat(returned, SSID);
}


// Function to decompress 6 byte Grid square to 4, 6 or 8 characters

void DeCompressGridSquare(char * bytGS, char * returned)
{
	char bytTest[10] = "";
	Bit6ToASCII(bytGS, bytTest);

	strlop(bytTest, ' ');
	strcpy(returned, bytTest);
}

// A function to compute the parity symbol used in the frame type encoding

UCHAR ComputeTypeParity(UCHAR bytFrameType)
{
	UCHAR bytMask = 0xC0;
	UCHAR bytParitySum = 1;
	UCHAR bytSym = 0;
	int k;

	for (k = 0; k < 4; k++)
	{
		bytSym = (bytMask & bytFrameType) >> (2 * (3 - k));
		bytParitySum = bytParitySum ^ bytSym;
		bytMask = bytMask >> 2;
	}
    
	return bytParitySum & 0x3;
}

// Subroutine to make a CW ID Wave File

void sendCWID(char * Call, BOOL Play)
{
}

void CWID(char * strID, short * intSamples, BOOL blnPlay)
{
	// This generates a phase synchronous FSK MORSE keying of strID
	// FSK used to maintain VOX on some sound cards
	// Sent at 90% of  max ampllitude

    char strAlphabe[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/";

	// Look up table for strAlphabet...each bit represents one dot time, 3 adjacent dots = 1 dash
	// one dot spacing between dots or dashes
/*
        Dim intCW() As Integer = {&H17, &H1D5, &H75D, &H75, &H1, &H15D, _
           &H1DD, &H55, &H5, &H1777, &H1D7, &H175, _
           &H77, &H1D, &H777, &H5DD, &H1DD7, &H5D, _
           &H15, &H7, &H57, &H157, &H177, &H757, _
           &H1D77, &H775, &H77777, &H17777, &H5777, &H1577, _
           &H557, &H155, &H755, &H1DD5, &H7775, &H1DDDD, &H1D57, &H1D57}

        If strID.IndexOf("-") <> -1 Then
            ' strip off the -ssid for CWID
            strID = strID.Substring(0, strID.IndexOf("-"))
        End If

        Dim dblHiPhaseInc As float = 2 * PI * 1609.375 / 12000 ' 1609.375 Hz High tone
        Dim dblLoPhaseInc As float = 2 * PI * 1390.625 / 12000 ' 1390.625  low tone
        Dim dblHiPhase As float
        Dim dblLoPhase As float
        Dim intDotSampCnt As Integer = 768 ' about 12 WPM or so (should be a multiple of 256
        Dim intDot(intDotSampCnt - 1) As Int16
        Dim intSpace(intDotSampCnt - 1) As Int16

        ' Generate the dot samples (high tone) and space samples (low tone) 
        For i As Integer = 0 To intDotSampCnt - 1
            intSpace(i) = CShort(Sin(dblLoPhase) * 0.9 * intAmp)
            intDot(i) = CShort(Sin(dblHiPhase) * 0.9 * intAmp)
            dblHiPhase += dblHiPhaseInc
            If dblHiPhase > 2 * PI Then dblHiPhase -= 2 * PI
            dblLoPhase += dblLoPhaseInc
            If dblLoPhase > 2 * PI Then dblLoPhase -= 2 * PI
        Next i
        Dim intMask As Int32
        Dim intWav((6 * intDotSampCnt) - 1) As Int32
        ' Generate leader for VOX 6 dots long
        For k As Integer = 6 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k

        For j As Integer = 0 To strID.Length - 1 ' for each character in the string
            intMask = &H40000000
            Dim intIdx As Integer = strAlphabet.IndexOf(strID.ToUpper.Substring(j, 1))
            If intIdx = -1 Then
                ' process this as a space adding 6 dots worth of space to the wave file
                ReDim Preserve intWav(intWav.Length + (6 * intDotSampCnt) - 1)
                For k As Integer = 6 To 1 Step -1
                    Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
                Next k
            Else
                While intMask > 0 ' search for the first non 0 bit
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Exit While ' intMask is pointing to the first non 0 entry
                    Else
                        intMask = intMask \ 2 ' Right shift mask
                    End If
                End While
                While intMask > 0
                    ReDim Preserve intWav(intWav.Length + intDotSampCnt - 1)
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Array.Copy(intDot, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    Else
                        Array.Copy(intSpace, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    End If
                    intMask = intMask \ 2 ' Right shift mask
                End While
            End If
            ' add 3 dot spaces for inter letter spacing
            ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
            For k As Integer = 3 To 1 Step -1
                Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
            Next k
        Next
        ' add 3 spaces for the end tail
        ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
        For k As Integer = 3 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k
        If Not blnPlay Then
            intSamples = intWav
            Exit Sub
        End If
        ' Convert the integer array to bytes
        Dim aryWave(2 * intWav.Length - 1) As Byte
        For j As Integer = 0 To intWav.Length - 1
            aryWave(2 * j) = CByte(intWav(j) And &HFF) ' LSByte
            aryWave(1 + 2 * j) = CByte((intWav(j) And &HFF00) \ 256) ' MSbyte
        Next j
        ' *********************************
        ' Debug code to look at wave file 
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = FALSE Then
        '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
        'End If
        'objWave.WriteRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\CWID.wav", 12000, 16, aryWave)
        '' End of debug code
        '************************************
        objWave.WriteRIFFStream(memWaveStream, 12000, 16, aryWave)
*/
}


// Function to look up the byte value from the frame string name

UCHAR FrameCode(char * strFrameName)
{
	int i;

    for (i = 0; i < 256; i++)
	{
		if (strcmp(strFrameType[i], strFrameName) == 0)
		{
			return i;
		}
	}
	return 0;
}

unsigned int GenCRC16(unsigned char * Data, unsigned short length)
{
	// For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j < length; j++)	
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}
 
	return intRegister;
}

BOOL checkcrc16(unsigned char * Data, unsigned short length)
{
	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j <  (length - 2); j++)		// ' 2 bytes short of data length
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}

    if (Data[length - 2] == intRegister >> 8)
		if (Data[length - 1] == (intRegister & 0xFF))
			return TRUE;
   
	return FALSE;
}


//	Subroutine to compute a 16 bit CRC value and append it to the Data... With LS byte XORed by bytFrameType
    
void GenCRC16FrameType(char * Data, int Length, UCHAR bytFrameType)
{
	unsigned int CRC = GenCRC16(Data, Length);

	// Put the two CRC bytes after the stop index

	Data[Length++] = (CRC >> 8);		 // MS 8 bits of Register
	Data[Length] = (CRC & 0xFF) ^ bytFrameType;  // LS 8 bits of Register
}

// Function to compute a 16 bit CRC value and check it against the last 2 bytes of Data (the CRC) XORing LS byte with bytFrameType..
 
BOOL  CheckCRC16FrameType(unsigned char * Data, int Length, UCHAR bytFrameType)
{
	// returns TRUE if CRC matches, else FALSE
    // For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

	unsigned int CRC = GenCRC16(Data, Length);
  
	// Compare the register with the last two bytes of Data (the CRC) 
    
	if ((CRC >> 8) == Data[Length])
		if (((CRC & 0xFF) ^ bytFrameType) == Data[Length + 1])
			return TRUE;

	return FALSE;
}

// Subroutine to get intDataLen bytes from outbound queue (bytDataToSend)

void ClearDataToSend()
{
	TXQueueLen = 0;
}

int GetDataFromQueue(UCHAR * Data, int MaxLen)
{
	int returned = MaxLen;

	if (MaxLen == 0)
		return 0;

	GetSemaphore();

	if (MaxLen > TXQueueLen)
		returned = TXQueueLen;

	memcpy(Data, TXQueue, returned);

	TXQueueLen -= returned;

	if (TXQueueLen)
		memmove(TXQueue, &TXQueue[returned], TXQueueLen);

	FreeSemaphore();

	return returned;
}

void KeyPTT(BOOL State)
{
}

// Timer Rotines

void CheckTimers()
{
	//  Event triggered by tmrSendTimeout elapse. Ends an ARQ session and sends a DISC frame 
    
	if (tmrSendTimeout)
	{
		tmrSendTimeout--;

		if (tmrSendTimeout == 0)
		{
			unsigned char bytEncodedBytes[16];
			int Len;
			char HostCmd[80];

			// (Handles protocol rule 1.7)
       
			//Dim dttStartWait As Date = Now
			//While objMain.blnLastPTT And Now.Subtract(dttStartWait).TotalSeconds < 10
			// Thread.Sleep(50)
			// End While

			//if (DebugLog) Then Logs.WriteDebug("ARDOPprotocol.tmrSendTimeout]  ARQ Timeout from ProtocolState: " & GetARDOPProtocolState.ToString & "  Going to DISC state")
        
			// Confirmed proper operation of this timeout and rule 4.0 May 18, 2015
			// Send an ID frame (Handles protocol rule 4.0)

            Len = Encode4FSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes);
			Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent
			dttLastFECIDSent = Now;
			
			// If MCB.AccumulateStats Then LogStats()

			QueueCommandToHost("DISCONNECTED");
			
			sprintf(HostCmd, "STATUS ARQ Timeout from Protocol State:  %d", ProtocolState);
			QueueCommandToHost(HostCmd);
			blnEnbARQRpt = FALSE;
			//Thread.Sleep(2000)
			ClearDataToSend();

			Len = Encode4FSKControl(0x29, bytSessionID, bytEncodedBytes);
			Mod4FSKDataAndPlay(0x29, &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent

			intFrameRepeatInterval = 2000;
			ProtocolState = DISC;
			
			InitializeConnection(); // reset all Connection data
				
			// Clear the mnuBusy status on the main form
			//Dim stcStatus As Status = Nothing
		    //stcStatus.ControlName = "mnuBusy"
			//stcStatus.Text = "FALSE"
		    //queTNCStatus.Enqueue(stcStatus)
        
			blnTimeoutTriggered = FALSE ;// prevents a retrigger
		}
	}

    // Elapsed Subroutine for Pending timeout

	/*
    Private Sub tmrIRSPendingTimeout_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrIRSPendingTimeout.Elapsed
        tmrIRSPendingTimeout.Stop()
       
        If MCB.DebugLog Then Logs.WriteDebug("ARDOPprotocol.tmrIRSPendingTimeout]  ARQ Timeout from ProtocolState: " & GetARDOPProtocolState.ToString & "  Going to DISC state")
        objMain.objHI.QueueCommandToHost("DISCONNECTED")
        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECT REQUEST TIMEOUT FROM PROTOCOL STATE: " & GetARDOPProtocolState.ToString & " @ " & TimestampEx())
        blnEnbARQRpt = False
        Thread.Sleep(2000)
        SetARDOPProtocolState(ProtocolState.DISC)
        InitializeConnection() ' reset all Connection data
        ' Clear the mnuBusy status on the main form
        Dim stcStatus As Status = Nothing
        stcStatus.ControlName = "mnuBusy"
        stcStatus.Text = "FALSE"
        queTNCStatus.Enqueue(stcStatus)
    End Sub  'tmrIRSPendingTimeout_Elapsed

    ' Subroutine for tmrFinalIDElapsed
    Private Sub tmrFinalID_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrFinalID.Elapsed
        Dim bytDataToMod() As Byte

        tmrFinalID.Stop()
        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.tmrFinalID_Elapsed]  Send Final ID (" & strFinalIDCallsign & ", [" & MCB.GridSquare & "])")
        If CheckValidCallsignSyntax(strFinalIDCallsign) Then
            bytDataToMod = objMain.objMod.Encode4FSKIDFrame(strFinalIDCallsign, MCB.GridSquare, strCurrentFrameFilename)
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod)
            dttLastFECIDSent = Now
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 1000)
        End If
    End Sub  ' tmrFinalIDElapsed

    ' Function to send 10 minute ID
    Private Function Send10MinID() As Boolean
        Dim dttSafetyBailout As Date = Now
        If Now.Subtract(dttLastFECIDSent).TotalMinutes > 10 And Not blnDISCRepeating Then
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPptocol.Send10MinID] Send ID Frame")
            ' Send an ID frame (Handles protocol rule 4.0)
            blnEnbARQRpt = False
            Dim bytEncodedBytes() As Byte = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytEncodedBytes)
            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
            dttLastFECIDSent = Now
            ' Hold until PTT goes true
            While objMain.blnLastPTT = False And Now.Subtract(dttSafetyBailout).TotalMilliseconds < 4000
                Thread.Sleep(100)
            End While
            ' Now hold until PTT goes false
            While objMain.blnLastPTT = True And Now.Subtract(dttSafetyBailout).TotalMilliseconds < 4000
                Thread.Sleep(100)
            End While
            If Now.Subtract(dttSafetyBailout).TotalMilliseconds > 4000 Then
                Logs.Exception("[ARDOPprotocol.Send10MinID] Safety bailout!")
                Return True
            Else
                Thread.Sleep(200)
                Return True
            End If
        Else
            Return False
        End If
    End Function  'Send10MinID()
*/
}