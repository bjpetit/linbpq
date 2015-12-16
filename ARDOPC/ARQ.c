// ARDOP TNC ARQ Code
//

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#define BREAK 0x23
#define IDLEFRAME 0x26
#define DISCFRAME 0x29
#define END 0x2C
#define ConRejBusy 0x2D
#define ConRejBW 0x2E


#include "ARDOPC.h"

extern UCHAR bytData[];
extern int intLastRcvdFrameQuality;
extern int intRmtLeaderMeasure;
extern BOOL blnAbort;
extern int intRepeatCount;
extern int dttLastFECIDSent;

int intLastFrameIDToHost = 0;
int	intLastFailedFrameID = 0;
int	intLastARQDataFrameToHost = -1;
int	intARQDefaultDlyMs = 300;  // Not sure if this really need with optimized leader length. 100 ms doesn't add much overhead.
int	intAvgQuality;		 // the filtered average reported quality (0 to 100) practical range 50 to 96 
int	intShiftUpDn = 0;
int intFrameTypePtr = 0;	 // Pointer to the current data mode in bytFrameTypesForBW() 
int	intRmtLeaderMeas = 0;
int intTrackingQuality = -1;
UCHAR bytLastARQDataFrameSent = 0;  // initialize to an improper data frame
UCHAR bytLastARQDataFrameAcked = 0;  // initialize to an improper data frame

int bytQDataInProcessLen = 0;		// Lenght of frame to send/last sent

BOOL blnLastFrameSentData = FALSE;

extern BOOL blnARQDisconnect;

// ARQ State Variables

char AuxCalls[10][10] = {0};
int AuxCallsLength = 0;

int intBW;			// Requested connect speed
int intSessionBW;	// Negotiated speed

const char ARQBandwidths[8][12] = {"200FORCED", "500FORCED", "1000FORCED", "2000FORCED", "200MAX", "500MAX", "1000MAX", "2000MAX"};
enum _ARQSubStates ARQState;

const char ARQSubStates[10][11] = {"ISSConReq", "ISSConAck", "ISSData", "ISSId", "IRSConAck", "IRSData", "IRSBreak", "IRSfromISS", "DISCArqEnd", "None"};

char strRemoteCallsign[10];
char strLocalCallsign[10];
char strFinalIDCallsign[10];

UCHAR bytLastARQSessionID;
BOOL blnEnbARQRpt;
BOOL blnListen = TRUE;
UCHAR bytPendingSessionID;
UCHAR bytSessionID = 0xff;
BOOL blnARQConnected;

UCHAR bytCurrentFrameType = 0;	// The current frame type used for sending
UCHAR * bytFrameTypesForBW;		// Holds the byte array for Data modes for a session bandwidth. First are most robust, last are fastest
int bytFrameTypesForBWLength = 0;

BOOL blnPending;
int dttTimeoutTrip;
int intLastARQDataFrameToHost;
int intAvgQuality;
int intReceivedLeaderLen;
int tmrFinalID = 0;
int tmrIRSPendingTimeout = 0;
int tmrPollOBQueue = 1000;
UCHAR bytLastDataFrameType;
BOOL blnDISCRepeating;
int intRmtLeaderMeas;

int	intOBBytesToConfirm = 0;	// remaining bytes to confirm  
int	intBytesConfirmed = 0;		// Outbound bytes confirmed by ACK and squenced
int	intReportedLeaderLen = 0;	// Zero out the Reported leader length the length reported to the remote station 
BOOL blnLastPSNPassed = FALSE;	// the last PSN passed True for Odd, FALSE for even. 
BOOL blnInitiatedConnection = FALSE; // flag to indicate if this station initiated the connection
short dblAvgPECreepPerCarrier = 0; // computed phase error creep
int dttLastIDSent;				// date/time of last ID
int	intTotalSymbols = 0;		// To compute the sample rate error

extern int bytDataToSendLength;
int intFrameRepeatInterval;


extern int intLeaderRcvdMs;	

int intTrackingQuality;
int intNAKctr;


int Encode4FSKControl(UCHAR bytFrameType, UCHAR bytSessionID, UCHAR * bytreturn);
int EncodeConACKwTiming(UCHAR bytFrameType, int intRcvdLeaderLenMs, UCHAR bytSessionID, UCHAR * bytreturn);
int IRSNegotiateBW(int intConReqFrameType);
int GetNextFrameData(int * intUpDn, UCHAR * bytFrameTypeToSend, UCHAR * strMod, BOOL blnInitialize);
void RestoreDataToQueue(UCHAR * bytData,  int Len);
BOOL CheckForDisconnect();
BOOL Send10MinID();


// Subroutine to compute a 8 bit CRC value and append it to the Data...

UCHAR GenCRC8(char * Data)
{
	//For  CRC-8-CCITT =    x^8 + x^7 +x^3 + x^2 + 1  intPoly = 1021 Init FFFF

	int intPoly = 0xC6; // This implements the CRC polynomial  x^8 + x^7 +x^3 + x^2 + 1
	int intRegister  = 0xFF;
	int i; 
	unsigned int j;
	BOOL blnBit;

	for (j = 0; j < strlen(Data); j++)
	{
		int Val = Data[j];
		
		for (i = 7; i >= 0; i--) // for each bit processing MS bit first
		{
            blnBit = (Val & 0x80) != 0;
			Val = Val << 1;

			if ((intRegister & 0x80) == 0x80)  // the MSB of the register is set
			{
				// Shift left, place data bit as LSB, then divide
				// Register := shiftRegister left shift 1
				// Register := shiftRegister xor polynomial

				if (blnBit) 
					intRegister = 0xFF & (1 + 2 * intRegister);
				else
					intRegister = 0xFF & (2 * intRegister);
                 
				intRegister = intRegister ^ intPoly;
			}
			else
			{
				// the MSB is not set
				// Register is not divisible by polynomial yet.
				// Just shift left and bring current data bit onto LSB of shiftRegister

				if (blnBit)
					intRegister = 0xFF & (1 + 2 * intRegister);
				else
					intRegister = 0xFF & (2 * intRegister);
			}
		}
	}
	return intRegister & 0xFF; // LS 8 bits of Register 

}


//  Subroutine to Set the protocol state 

void SetARDOPProtocolState(int value)
{
	char HostCmd[24];

	if (ProtocolState == value)
		return;

	ProtocolState = value;

        //Dim stcStatus As Status
        //stcStatus.ControlName = "lblState"
        //stcStatus.Text = ARDOPState.ToString

	switch(ProtocolState)
	{
	case DISC:

		blnARQDisconnect = FALSE; // always clear the ARQ Disconnect Flag from host.
		//stcStatus.BackColor = System.Drawing.Color.White
		blnARQConnected = FALSE;
		blnPending = FALSE;
		ClearDataToSend();
		break;

	case FECRcv:
		//stcStatus.BackColor = System.Drawing.Color.PowderBlue
		break;
		
	case FECSend:

		InitializeConnection();
		intLastFrameIDToHost = -1;
		intLastFailedFrameID = -1;
		//ReDim bytFailedData(-1)
		//stcStatus.BackColor = System.Drawing.Color.Orange
		break;

        //    Case ProtocolState.IRS
        //        stcStatus.BackColor = System.Drawing.Color.LightGreen
        //    Case ProtocolState.ISS
        //        stcStatus.BackColor = System.Drawing.Color.LightSalmon
        //    Case ProtocolState.IDLE
        //        stcStatus.BackColor = System.Drawing.Color.NavajoWhite
        //    Case ProtocolState.OFFLINE
         //       stcStatus.BackColor = System.Drawing.Color.Silver
	}
	//queTNCStatus.Enqueue(stcStatus)

	sprintf(HostCmd, "NEWSTATE %s ", ARDOPStates[ProtocolState]);
	QueueCommandToHost(HostCmd);
}

  

 
// function to generate 8 bit session ID

UCHAR GenerateSessionID(char * strCallingCallSign, char *strTargetCallsign)
{
	char bytToCRC[20];
	
	int Len = sprintf(bytToCRC, "%s%s", strCallingCallSign, strTargetCallsign);

	UCHAR ID = GenCRC8(bytToCRC);

    if (ID == 255)

		// rare case where the computed session ID woudl be FF
		// Remap a SessionID of FF to 0...FF reserved for FEC mode

		return 0;

	return ID;
}

// Function to compute the optimum leader based on the Leader sent and the reported Received leader

void CalculateOptimumLeader(int intReportedReceivedLeaderMS,int  intLeaderSentMS)
{
	intCalcLeader = max(160, 120 + intLeaderSentMS - intReportedReceivedLeaderMS);  //  This appears to work well on HF sim tests May 31, 2015
    //    if (DebugLog) Debugprintf(("[ARDOPprotocol.CalcualteOptimumLeader] Leader Sent=" & intLeaderSentMS.ToString & "  ReportedReceived=" & intReportedReceivedLeaderMS.ToString & "  Calculated=" & stcConnection.intCalcLeader.ToString)
}

 

// Function to determine if call is to Callsign or one of the AuxCalls

BOOL IsCallToMe(char * strCallsign, UCHAR * bytReplySessionID)
{
	// returns true and sets bytReplySessionID if is to me.

	int i;
	
	if (strcmp(strCallsign, Callsign) == 0)
	{
		*bytReplySessionID = GenerateSessionID(bytData, strCallsign);
		return TRUE;
	}
	
	for (i = 0; i < AuxCallsLength; i++)
	{
		if (strcmp(strCallsign, AuxCalls[i]) == 0)
		{
			*bytReplySessionID = GenerateSessionID(bytData, strCallsign);
			return TRUE;
		}
	}

	return FALSE;
}

// Function to get base (even) data modes by bandwidth for ARQ sessions

// Streamlined 0.3.1.6
//	 200  8FSK.200.25, 4FSK.200.50, 4PSK.200.100, 8PSK.200.100
//  (288, 429, 768, 1296 byte/min)
static UCHAR DataModes200[] = {0x4E, 0x46, 0x40, 0x44};


	// 500  streamlined 0.3.1.6
	//16FSK.500.25S, 16FSK.500.25, 4FSK.500.100, 4PSK.500.100, 8PSK.500.100, 8PSK.500.167)
	//' (329, 429, 881, 1536, 2592, 4305 bytes/min)
static UCHAR DataModes500[] = {0x5A, 0x58, 0x4A, 0x50, 0x52, 0x56};


// 1000 ' Streamlined 0.3.1.6
	//'16FSK.500.25S, 16FSK.500.25, 4FSK.500.100, 4FSK.1000.100, 4PSK.1000.100, 8PSK.1000.100, 8PSK.1000.167
	//'(329, 429, 881, 1762, 3072, 5184, 8610 bytes/min) 
static UCHAR DataModes1000[] = {0x5A, 0x58, 0x4A, 0x68, 0x60, 0x62, 0x66};

// 2000 Non-FM
//' These do not include the 600 baud modes for FM only.
//'16FSK.500.25S, 16FSK.500.25, 4FSK.500.100, 4FSK.1000.100, 4FSK.2000.100, 4PSK.2000.100, 8PSK.2000.100, 8PSK.2000.167)
//'(329, 429, 881, 1762, 3624, 6144, 10386, 17220 bytes/min) 

static UCHAR DataModes2000[] = {0x5A, 0x58, 0x4A, 0x68, 0x78, 0x70, 0x72, 0x76};

//2000 FM
//' These include the 600 baud modes for FM only.
//' The following is temporary, Plan to replace 8PSK 8 carrier modes with high baud 4PSK and 8PSK.

//' 4FSK.500.100S, 4FSK.500.100, 4FSK.2000.600S, 4FSK.200.600, 4PSK.2000.100, 8PSK.2000.100)
//' (696, 881, 4338, 5863, 6144, 10386 bytes/min)

static UCHAR DataModes2000FM[] = {0x4C, 0x4A, 0x7C, 0x7A, 0x70, 0x72};

static UCHAR NoDataModes[1] = {0};

UCHAR  * GetDataModes(int intBW)
{
	// Revised version 0.3.5
	// idea is to use this list in the gear shift algorithm to select modulation mode based on bandwidth and robustness.
    // Sequence modes in approximate order of robustness ...most robust first, shorter frames of same modulation first

	if (intBW == 200)
	{
		bytFrameTypesForBWLength = sizeof(DataModes200) - 1;
		return DataModes200;
	}
	if (intBW == 500) 
	{
		bytFrameTypesForBWLength = sizeof(DataModes500) - 1;
		return DataModes500;
	}
	if (intBW == 1000) 
	{
		bytFrameTypesForBWLength = sizeof(DataModes1000) - 1;
		return DataModes1000;
	}
	if (intBW == 2000) 
	{
		if (TuningRange > 0)
		{
			bytFrameTypesForBWLength = sizeof(DataModes2000) - 1;
			return DataModes2000;
		}
		else
		{
			bytFrameTypesForBWLength = sizeof(DataModes2000FM) - 1;
			return DataModes2000FM;
		}
	}
	bytFrameTypesForBWLength = 0;
	return NoDataModes;
}

//  Subroutine to shift up to the next higher throughput or down to the next more robust data modes based on average reported quality 

void Gearshift_5()
{
	//' More complex mechanism to gear shift based on intAvgQuality, current state and bytes remaining.
	//' This can be refined later with different or dynamic Trip points etc. 

	int intTripHi = 79;		// Modified in revision 0.4.0 (was 82)
	int intTripLow = 69;	// Modified in revision 0.4.0 (was 72)

	int intBytesRemaining = bytDataToSendLength;

	if (intNAKctr >= 5 && intFrameTypePtr > 0)	//  NAK threshold changed from 10 to 6 on rev 0.3.5.2
	{
		if (DebugLog) Debugprintf("[ARDOPprotocol.Gearshift_5] intNAKCtr=%d ShiftUpDn = -1", intNAKctr);
        
		intShiftUpDn = -1;  //Shift down if 5 NAKs without ACK.
		intAvgQuality = (intTripHi + intTripLow) / 2;	 // init back to mid way
		intNAKctr = 0;
	}
	else if (intAvgQuality > intTripHi && intFrameTypePtr < bytFrameTypesForBWLength) // ' if above Hi Trip setup so next call of GetNextFrameData will select a faster mode if one is available 
	{
		intShiftUpDn = 0;
		
		if (TuningRange == 0)
		{
			switch (intFrameTypePtr)
			{
			case 0:
				
				if (intBytesRemaining > 64)
					intShiftUpDn = 2;
				else if (intBytesRemaining > 32)
					intShiftUpDn = 1;

				break;

			case 1:
		
				if (intBytesRemaining > 200)
					intShiftUpDn = 2;
				else if (intBytesRemaining > 64)
					intShiftUpDn = 1;

				break;
 
			case 2:
	
				if (intBytesRemaining > 400)
					intShiftUpDn = 2;
				else if (intBytesRemaining > 200)
					intShiftUpDn = 1;

				break;

			case 3:
				
				if (intBytesRemaining > 600) intShiftUpDn = 1;
				break;
		
			case 4:
				
				if (intBytesRemaining > 512) intShiftUpDn = 1;
				break;
			}
		}
		
		else if (intSessionBW == 200)
			intShiftUpDn = 1;
		else if (intFrameTypePtr == 0 && intBytesRemaining > 32)
			intShiftUpDn = 2;
		else
			intShiftUpDn = 1;

		if (DebugLog) Debugprintf("[ARDOPprotocol.Gearshift_5] ShiftUpDn = %d, AvgQuality=%d Resetting to %d New Mode: %s",
			intShiftUpDn, intAvgQuality, (intTripHi + intTripLow) / 2, Name(bytFrameTypesForBW[intFrameTypePtr + intShiftUpDn]));
	
		intAvgQuality = (intTripHi + intTripLow) / 2;	 // init back to mid way
		intNAKctr = 0;
	}
	else if (intAvgQuality < intTripLow && intFrameTypePtr > 0)   // if below Low Trip setup so next call of GetNextFrameData will select a more robust mode if one is available 
	{
		intShiftUpDn = 0;
		
		if (TuningRange == 0)
		{
			switch (intFrameTypePtr)
			{
			case 1:
				
				if (intBytesRemaining < 33)  intShiftUpDn = -1;
				break;
 
			case 2:
			case 4:
			case 5:
	
				intShiftUpDn = -1;
				break;

			case 3:
				
				intShiftUpDn = -2;
				break;
			}
		}

		else if  (intSessionBW == 200)
			intShiftUpDn = -1;
		else
		{
			if (intFrameTypePtr == 2 && intBytesRemaining < 17)
				intShiftUpDn = -2;
			else
				intShiftUpDn = -1;
		}

		if (DebugLog) Debugprintf("[ARDOPprotocol.Gearshift_5] ShiftUpDn = %d, AvgQuality=%d Resetting to %d New Mode: %s",
			intShiftUpDn, intAvgQuality, (intTripHi + intTripLow) / 2, Name(bytFrameTypesForBW[intFrameTypePtr + intShiftUpDn]));
			
		intAvgQuality = (intTripHi + intTripLow) / 2;  // init back to mid way
		intNAKctr = 0;
	}
	
//	if (intShiftUpDn < 0)
//		intShiftDNs++;
//	else if (intShiftUpDn > 0)
//		intShiftUPs++;
}

// Subroutine to provide exponential averaging for reported received quality from ACK/NAK to data frame.

void ComputeQualityAvg(int intReportedQuality)
{
	float dblAlpha = 0.5f;	 // adjust this for exponential averaging speed.  smaller alpha = slower response & smoother averages but less rapid shifting. 

	if (intAvgQuality == 0)
	{
		intAvgQuality = intReportedQuality;
        //    if (DebugLog) Debugprintf(("[ARDOPprotocol.ComputeQualityAvg] Initialize AvgQuality=" & intAvgQuality.ToString)
	}
	else
	{
		intAvgQuality = intAvgQuality * (1 - dblAlpha) + (dblAlpha * intReportedQuality) + 0.5f; // exponential averager 
        //    if (DebugLog) Debugprintf(("[ARDOPprotocol.ComputeQualityAvg] Reported Quality=" & intReportedQuality.ToString & "  New Avg Quality=" & intAvgQuality.ToString)
	}
}

 // Subroutine to determine the next data frame to send (or IDLE if none) 

void SendDataOrIDLE()
{
	char strMod[16];
	int Len;

	// Check for ID frame required (every 10 minutes)
	
	if (blnDISCRepeating)
		return;

	switch (ProtocolState)
	{
	case IDLE:

		if (CheckForDisconnect())
			return;
		
		if (bytDataToSendLength == 0 && blnEnbARQRpt)
			return;		// let repeats (Data or IDLE) continue

		if (bytDataToSendLength == 0 && !blnEnbARQRpt)
		{
			Send10MinID();	 // Send ID if 10 minutes since last
			// Send First IDLE and setup repeat

			EncLen = Encode4FSKControl(IDLEFRAME, bytSessionID, bytEncodedBytes);
			Mod4FSKDataAndPlay(IDLEFRAME, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

			blnLastFrameSentData = FALSE;
			
			if (ProtocolState != IDLE)
			{
				if (DebugLog) Debugprintf("[ARDOPprotocol.SendDataOrIDLE] DataToSend = 0, Go to ProtocolState IDLE");
				SetARDOPProtocolState(IDLE);
			}
			intFrameRepeatInterval = 2000; // Keep IDLE/BREAK repeats fairly long 
			blnEnbARQRpt = TRUE;
			dttTimeoutTrip = Now;
			return;
		}
		if (bytDataToSendLength > 0)
		{			
			if (DebugLog) Debugprintf("[ARDOPprotocol.SendDataOrIDLE] DataToSend = %d bytes, ProtocolState IDLE > ISS", bytDataToSendLength);
			
			SetARDOPProtocolState(ISS);
			ARQState = ISSData;
			Send10MinID();		 // Send ID if 10 minutes since last

			Len = GetNextFrameData(&intShiftUpDn, &bytCurrentFrameType, strMod, FALSE);

			printf("Sending Type %x Len %d\n", bytCurrentFrameType, Len);

			if (strcmp(strMod, "4FSK") == 0)
			{
				EncLen = EncodeFSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);
				if (bytCurrentFrameType >= 0x7A && bytCurrentFrameType <= 0x7D)
					Mod4FSK600BdDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
				else
					Mod4FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}
			else if (strcmp(strMod, "16FSK") == 0)
			{
				EncLen = EncodeFSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);
				Mod16FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}
			else if (strcmp(strMod, "8FSK") == 0)
			{
				EncLen = EncodeFSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);          //      intCurrentFrameSamples = Mod8FSKData(bytFrameType, bytData);
				Mod8FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}
			else
			{
				EncLen = EncodePSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);
				ModPSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}

			blnLastFrameSentData = TRUE;
			intFrameRepeatInterval = 2000;  // set up 2000 ms repeat interval for Data may be able to shorten
			//'intFrameRepeatInterval = ComputeInterFrameInterval(1300) 'Fairly conservative...evaluate (based on measured leader from remote.
			dttTimeoutTrip = Now;
			blnEnbARQRpt = TRUE;
			return;
		}
		break;		// Shouldnt ger here
	
	case ISS:
			
		if (CheckForDisconnect())
			return;
		
		Send10MinID();  // Send ID if 10 minutes since last

		if (bytDataToSendLength > 0)
		{
			//if (DebugLog) Debugprintf(("[ARDOPprotocol.SendDataOrIDLE] DataToSend = " & bytDataToSendLength.ToString & " bytes, In ProtocolState ISS")
			//' Get the data from the buffer here based on current data frame type
			//' (Handles protocol Rule 2.1)

			Len = bytQDataInProcessLen = GetNextFrameData(&intShiftUpDn, &bytCurrentFrameType, strMod, FALSE);

			if (strcmp(strMod, "4FSK") == 0)
			{
				EncLen = EncodeFSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);
				if (bytCurrentFrameType >= 0x7A && bytCurrentFrameType <= 0x7D)
					Mod4FSK600BdDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
				else
					Mod4FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}
			else if (strcmp(strMod, "16FSK") == 0)
			{
				EncLen = EncodeFSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);
				Mod16FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}
			else if (strcmp(strMod, "8FSK") == 0)
			{
				EncLen = EncodeFSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);          //      intCurrentFrameSamples = Mod8FSKData(bytFrameType, bytData);
				Mod8FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}
			else
			{
				EncLen = EncodePSKData(bytCurrentFrameType, bytDataToSend, Len, bytEncodedBytes);
				ModPSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			}

			blnLastFrameSentData = TRUE;
			intFrameRepeatInterval = 2000;  // set up 2000 ms repeat interval for Data may be able to shorten
			//'intFrameRepeatInterval = ComputeInterFrameInterval(1300) 'Fairly conservative...evaluate (based on measured leader from remote.
			dttTimeoutTrip = Now;
			blnEnbARQRpt = TRUE;
			ARQState = ISSData;		 // Should not be necessary
			return;
		}
		else
		{
			// Send IDLE

			EncLen = Encode4FSKControl(IDLEFRAME, bytSessionID, bytEncodedBytes);
			Mod4FSKDataAndPlay(IDLEFRAME, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

			blnLastFrameSentData = FALSE;
			
			if (DebugLog) Debugprintf("[ARDOPprotocol.SendDataOrIDLE] DataToSend = 0, ProtocolState ISS > IDLE");
 	
			SetARDOPProtocolState(IDLE);
			intFrameRepeatInterval = 2000; // Keep IDLE/BREAK repeats fairly long 
			blnEnbARQRpt = TRUE;
			dttTimeoutTrip = Now;
			return;
		}
	}
}


 

//	a simple function to get an available frame type for the session bandwidth. 
    
int GetNextFrameData(int * intUpDn, UCHAR * bytFrameTypeToSend, UCHAR * strMod, BOOL blnInitialize)
{
	// Initialize if blnInitialize = true
	// Then call with intUpDn and blnInitialize = FALSE:
	//       intUpDn = 0 ' use the current mode pointed to by intFrameTypePtr
	//       intUpdn < 0    ' Go to a more robust mode if available limited to the most robust mode for the bandwidth 
	//       intUpDn > 0    ' Go to a less robust (faster) mode if avaialble, limited to the fastest mode for the bandwidth

	BOOL blnOdd;
	int intNumCar, intBaud, intDataLen, intRSLen;
	UCHAR bytQualThresh;
	char strType[16];
    char * strShift = NULL;
	int MaxLen;

	if (blnInitialize)	//' Get the array of supported frame types in order of Most robust to least robust
	{
		bytFrameTypesForBW = GetDataModes(intSessionBW);
		intFrameTypePtr = 0;
		bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];
		if(DebugLog) Debugprintf("[ARDOPprotocol.GetNextFrameData] Initial Frame Type: %s", Name(bytCurrentFrameType));
		*intUpDn = 0;
		return 0;
	}
	if (*intUpDn < 0)		// go to a more robust mode
	{
		if (intFrameTypePtr > 0)
		{
			intFrameTypePtr = max(0, intFrameTypePtr + *intUpDn);
			bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];
			strShift = "Shift Down";
		}
		*intUpDn = 0;
	}
	else if (*intUpDn > 0)	//' go to a faster mode
	{
		if (intFrameTypePtr < bytFrameTypesForBWLength)
		{
			intFrameTypePtr = min(bytFrameTypesForBWLength, intFrameTypePtr + *intUpDn);
			bytCurrentFrameType = bytFrameTypesForBW[intFrameTypePtr];
			strShift = "Shift Up";
		}
		*intUpDn = 0;
	}
        //If Not objFrameInfo.IsDataFrame(bytCurrentFrameType) Then
        //    Logs.Exception("[ARDOPprotocol.GetNextFrameData] Frame Type " & Format(bytCurrentFrameType, "X") & " not a data type.")
        //    Return Noth
	
	if ((bytCurrentFrameType & 1) == (bytLastARQDataFrameAcked & 1))
	{
		*bytFrameTypeToSend = bytCurrentFrameType ^ 1;  // This insures toggle of  Odd and Even 
		bytLastARQDataFrameSent = *bytFrameTypeToSend;
	}
	else
	{
		*bytFrameTypeToSend = bytCurrentFrameType;
		bytLastARQDataFrameSent = *bytFrameTypeToSend;
	}
	
	if (DebugLog)
	{
		if (strShift == 0)
			Debugprintf("[ARDOPprotocol.GetNextFrameData] No shift, Frame Type: %s", Name(bytCurrentFrameType));
		else
			Debugprintf("[ARDOPprotocol.GetNextFrameData] %s, Frame Type: %s", strShift, Name(bytCurrentFrameType));
	}

	FrameInfo(bytCurrentFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	MaxLen = intDataLen * intNumCar;

	if (MaxLen > bytDataToSendLength)
		MaxLen = bytDataToSendLength;

	return MaxLen;
}
 

void InitializeConnection()
{
	// Sub to Initialize before a new Connection

	strRemoteCallsign[0] = 0; // remote station call sign
	intOBBytesToConfirm = 0; // remaining bytes to confirm  
	intBytesConfirmed = 0; // Outbound bytes confirmed by ACK and squenced
	intReceivedLeaderLen = 0; // Zero out received leader length (the length of the leader as received by the local station
	intReportedLeaderLen = 0; // Zero out the Reported leader length the length reported to the remote station 
	bytSessionID = 0xFF; //  Session ID 
	blnLastPSNPassed = FALSE; //  the last PSN passed True for Odd, FALSE for even. 
	blnInitiatedConnection = FALSE; //  flag to indicate if this station initiated the connection
	dblAvgPECreepPerCarrier = 0; //  computed phase error creep
	dttLastIDSent = Now ; //  date/time of last ID
	intTotalSymbols = 0; //  To compute the sample rate error
	strLocalCallsign[0] = 0; //  this stations call sign
	intSessionBW = 0 ; //  ExtractARQBandwidth()
	intCalcLeader = LeaderLength;

	//ClearQualityStats();
	//ClearTuningStats();
}

// This sub processes a correctly decoded ConReq frame, decodes it an passed to host for display if it doesn't duplicate the prior passed frame. 

void ProcessUnconnectedConReqFrame(int intFrameType, UCHAR * bytData)
{
	static char strLastStringPassedToHost[80] = "";
	
	if (!(intFrameType >= 0x31 && intFrameType <= 0x38))
		return;
 
   /*
   Dim strDisplay As String = " [" & objFrameInfo.Name(intFrameType) & ": "
        Dim strCallsigns() As String = GetString(bytData).Split(" ")
        strDisplay &= strCallsigns(0) & " > " & strCallsigns(1) & "] "
        If strDisplay <> strLastStringPassedToHost Then ' suppresses repeats
            AddTagToDataAndSendToHost(GetBytes(strDisplay), "ARQ")
            strLastStringPassedToHost = strDisplay
        End If
*/
}

 
//	This is the main subroutine for processing ARQ frames 

void ProcessRcvdARQFrame(UCHAR intFrameType, UCHAR * bytData, int DataLen, BOOL blnFrameDecodedOK)
{
	//	blnFrameDecodedOK should always be true except in the case of a failed data frame ...Which is then NAK'ed if in IRS Data state
    
	int intReply;
	static UCHAR * strCallsign;
	int intReportedLeaderMS = 0;
	char HostCmd[80];

	// Allow for link turnround before responding

	Sleep(500);		// May need to turn
	
	switch (ProtocolState)
	{
	case DISC:
		
		// DISC State *******************************************************************************************

		if (blnFrameDecodedOK && intFrameType == 0x29) 
		{
			// Special case to process DISC from previous connection (Ending station must have missed END reply to DISC) Handles protocol rule 1.5
    
			if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState DISC, Send END with SessionID= %XX Stay in DISC state", bytLastARQSessionID);

			EncLen = Encode4FSKControl(0x2C, bytLastARQSessionID, bytEncodedBytes);
			Mod4FSKDataAndPlay(0x2C, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

			tmrFinalID = 500;
			
			blnEnbARQRpt = FALSE;
			return;
		}
		if (!blnListen)
			return;			 // ignore connect request if not blnListen
    
		// Process Connect request to MyCallsign or Aux Call signs  (Handles protocol rule 1.2)
   
		if (!blnFrameDecodedOK || intFrameType < 0x31 || intFrameType > 0x38)
			return;			// No decode or not a ConReq

		strCallsign  = strlop(bytData, ' '); // "fromcall tocall"

		// see if connect request is to MyCallsign or any Aux call sign
        
		if (IsCallToMe(strCallsign, &bytPendingSessionID)) // (Handles protocol rules 1.2, 1.3)
		{
			//Logs.WriteDebug("[ProcessRcvdARQFrame]1 strCallsigns(0)=" & strCallsigns(0) & "  strCallsigns(1)=" & strCallsigns(1) & "  bytPendnigSessionID=" & Format(bytPendingSessionID, "X"))
            
			intReply = IRSNegotiateBW(intFrameType); // NegotiateBandwidth

			if (intReply != 0x2E)	// If not ConRejBW the bandwidth is compatible so answer with correct ConAck frame
			{
				sprintf(HostCmd, "TARGET %s", strCallsign);
				QueueCommandToHost(HostCmd);
                       
				blnPending = TRUE;
				
				tmrIRSPendingTimeout = 1000;  // Triggers a 10 second timeout before auto abort from pending

				// (Handles protocol rule 1.2)
                            
				dttTimeoutTrip = Now;
                            
				SetARDOPProtocolState(IRS);
				ARQState = IRSConAck; // now connected 

				intLastARQDataFrameToHost = -1;	 // precondition to an illegal frame type
   
				strcpy(strRemoteCallsign, bytData);
				strcpy(strLocalCallsign, strCallsign);
				strcpy(strFinalIDCallsign, strCallsign);

				intAvgQuality = 0;		// initialize avg quality 
				intReceivedLeaderLen = intLeaderRcvdMs;		 // capture the received leader from the remote ISS's ConReq (used for timing optimization)

				EncLen = EncodeConACKwTiming(intReply, intLeaderRcvdMs, bytPendingSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(intReply, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
			}
			else
			{
				// ' ConRejBW  (Incompatible bandwidths)

				// ' (Handles protocol rule 1.3)
             
				EncLen = Encode4FSKControl(intReply, bytPendingSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(intReply, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

				sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
			}
		}
		else
		{
			// Not for us - cancel pending
			
			QueueCommandToHost("CANCELPENDING");
			ProcessUnconnectedConReqFrame(intFrameType, bytData);  //  displays data if not connnected.  
		}
		blnEnbARQRpt = FALSE;
		return;
	

	case IRS:

		//IRS State ****************************************************************************************
		//  Process ConReq, ConAck, DISC, END, Host DISCONNECT, DATA, IDLE, BREAK 

		if (ARQState == IRSConAck)		// Process ConAck or ConReq if reply ConAck sent above in Case ProtocolState.DISC was missed by ISS
		{         
			if (!blnFrameDecodedOK)
				return;					// no reply if no correct decode

			// ConReq processing (to handle case of ISS missing initial ConAck from IRS)

			if (intFrameType >= 0x31 && intFrameType <= 0x38) // Process Connect request to MyCallsign or Aux Call signs as for DISC state above (ISS must have missed initial ConACK from ProtocolState.DISC state)
			{
				if (!blnListen)
					return;
				
				// see if connect request is to MyCallsign or any Aux call sign

				strCallsign  = strlop(bytData, ' '); // "fromcall tocall"
       
				if (IsCallToMe(strCallsign, &bytPendingSessionID)) // (Handles protocol rules 1.2, 1.3)
				{
					//Logs.WriteDebug("[ProcessRcvdARQFrame]1 strCallsigns(0)=" & strCallsigns(0) & "  strCallsigns(1)=" & strCallsigns(1) & "  bytPendnigSessionID=" & Format(bytPendingSessionID, "X"))
            
					intReply = IRSNegotiateBW(intFrameType); // NegotiateBandwidth

					if (intReply != 0x2E)	// If not ConRejBW the bandwidth is compatible so answer with correct ConAck frame
					{
						// Note: CONNECTION and STATUS notices were already sent from  Case ProtocolState.DISC above...no need to duplicate

  						SetARDOPProtocolState(IRS);
						ARQState = IRSConAck; // now connected 

						intLastARQDataFrameToHost = -1;	 // precondition to an illegal frame type
  
						intAvgQuality = 0;		// initialize avg quality 
						intReceivedLeaderLen = intLeaderRcvdMs;		 // capture the received leader from the remote ISS's ConReq (used for timing optimization)
						InitializeConnection();
						dttTimeoutTrip = Now;

						//Stop and restart the Pending timer upon each ConReq received to ME
 						tmrIRSPendingTimeout= 1000;  // Triggers a 10 second timeout before auto abort from pending

						strcpy(strRemoteCallsign, bytData);
						strcpy(strLocalCallsign, strCallsign);
						strcpy(strFinalIDCallsign, strCallsign);

						EncLen = EncodeConACKwTiming(intReply, intLeaderRcvdMs, bytPendingSessionID, bytEncodedBytes);
						Mod4FSKDataAndPlay(intReply, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
						// ' No delay to allow ISS to measure its TX>RX delay}
						return;
					}
		
					// ' ConRejBW  (Incompatible bandwidths)

					// ' (Handles protocol rule 1.3)
             
					//	if (DebugLog) Debugprintf(("[ProcessRcvdARQFrame] Incompatible bandwidth connect request. Frame type: " & objFrameInfo.Name(intFrameType) & "   MCB.ARQBandwidth:  " & MCB.ARQBandwidth)
  				
					EncLen = Encode4FSKControl(intReply, bytPendingSessionID, bytEncodedBytes);
					Mod4FSKDataAndPlay(intReply, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
	
					sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
					QueueCommandToHost(HostCmd);
					sprintf(HostCmd, "STATUS ARQ CONNECTION FROM %s REJECTED, INCOMPATIBLE BANDWIDTHS.", strRemoteCallsign);
					QueueCommandToHost(HostCmd);

					return;
				}

				//this normally shouldn't happen but is put here in case another Connect request to a different station also on freq...may want to change or eliminate this
				
				//if (DebugLog) WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] Call to another target while in ProtocolState.IRS, ARQSubStates.IRSConAck...Ignore");

				return;
			}
      
			// ConAck processing from ISS
                
			if (intFrameType >= 0x39 && intFrameType <= 0x3C)	// Process ConACK frames from ISS confirming Bandwidth and providing ISS's received leader info.
			{
				// if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame] IRS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")
                      
				switch (intFrameType)
				{
				case 0x39:
					intSessionBW = 200;
					break;
				case 0x3A:
					intSessionBW = 500;
					break;
				case 0x3B:
					intSessionBW = 1000;
					break;
				case 0x3C:
					intSessionBW = 2000;
					break;
				}
					
				CalculateOptimumLeader(10 * bytData[0], LeaderLength);

				bytSessionID = bytPendingSessionID; // This sets the session ID now 
               
				EncLen = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
				blnARQConnected = TRUE;
				blnPending = FALSE;
				tmrIRSPendingTimeout = 0;

				sprintf(HostCmd, "CONNECTED %s %d", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);
 
				sprintf(HostCmd, "STATUS ARQ CONNECTION FROM %s: SESSION BW = %d HZ", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);

				// Initialize the frame type and pointer based on bandwidth (added 0.3.1.3)
				
				GetNextFrameData(&intShiftUpDn, 0, NULL, TRUE);		//' just sets the initial data, frame type, and sets intShiftUpDn= 0
          
				//       ' Update the main form menu status lable 
                //        Dim stcStatus As Status = Nothing
                //        stcStatus.ControlName = "mnuBusy"
                //       stcStatus.Text = "Connected " & .strRemoteCallsign
                 //       queTNCStatus.Enqueue(stcStatus)

				dttTimeoutTrip = Now;
				ARQState = IRSData;
				intLastARQDataFrameToHost = -1;
				intTrackingQuality = -1;
				intNAKctr = 0;
				
				blnEnbARQRpt = FALSE;
				return;
			}
		}

		if (ARQState == IRSData)  // Process Data or ConAck if ISS failed to receive ACK confirming bandwidth so ISS repeated ConAck
		{
			// ConAck processing from ISS

			if (intFrameType >= 0x39 && intFrameType <= 0x3C)	// Process ConACK frames from ISS confirming Bandwidth and providing ISS's received leader info.
			{
				//  Process ConACK frames (ISS failed to receive prior ACK confirming session bandwidth so repeated ConACK)

				// if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame] IRS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")
                      
				switch (intFrameType)
				{
				case 0x39:
					intSessionBW = 200;
					break;
				case 0x3A:
					intSessionBW = 500;
					break;
				case 0x3B:
					intSessionBW = 1000;
					break;
				case 0x3C:
					intSessionBW = 2000;
					break;
				}

				EncLen = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
				dttTimeoutTrip = Now;
				return;
			}

			// handles DISC from ISS

			if (blnFrameDecodedOK && intFrameType == 0x29) //  IF DISC received from ISS Handles protocol rule 1.5
			{
				if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState IRS, IRSData...going to DISC state");
                 //       If MCB.AccumulateStats Then LogStats()
				
				QueueCommandToHost("DISCONNECTED");		// Send END
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				
				EncLen = Encode4FSKControl(0x2C, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(0x2C, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

				bytLastARQSessionID = bytSessionID;  // capture this session ID to allow answering DISC from DISC state if ISS missed Sent END
                       
				ClearDataToSend();
				tmrFinalID = 500;
				blnDISCRepeating = FALSE;
				
				SetARDOPProtocolState(DISC);
                InitializeConnection();
				blnEnbARQRpt = FALSE;
				return;
			}
			
			// handles END from ISS
			
			if (blnFrameDecodedOK && intFrameType == END) //  IF END received from ISS 
			{
				if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame]  END frame received in ProtocolState IRS, IRSData...going to DISC state");
				//If MCB.AccumulateStats Then LogStats()
				
				QueueCommandToHost("DISCONNECTED");
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				blnDISCRepeating = FALSE;
				ClearDataToSend();
				if (CheckValidCallsignSyntax(strLocalCallsign))
				{
					EncLen = Encode4FSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes);
					Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent
					dttLastFECIDSent = Now;
				}
				
				SetARDOPProtocolState(DISC);
				InitializeConnection();
				blnEnbARQRpt = FALSE;
				return;
			}

			// handles DISCONNECT command from host
			
			if (CheckForDisconnect())
				return;

			// This handles normal data frames

			if (blnFrameDecodedOK && IsDataFrame(intFrameType)) // Frame even/odd toggling will prevent duplicates in case of missed ACK
			{
				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure;  // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
					if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] IRS (receiving data) RmtLeaderMeas=%d ms", intRmtLeaderMeas);
				}

				if (intFrameType != intLastARQDataFrameToHost) // protects against duplicates if ISS missed IRS's ACK and repeated the same frame  
				{
					AddTagToDataAndSendToHost(bytData, "ARQ", DataLen); // only correct data in proper squence passed to host   
					intLastARQDataFrameToHost = intFrameType;						dttTimeoutTrip = Now;
				}
					
				// Always ACK if it is a data frame ...ISS may have missed last ACK

				EncLen = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
				blnEnbARQRpt = FALSE;					return;
			}

			// handles IDLE from ISS

			if (blnFrameDecodedOK && intFrameType == IDLEFRAME)  //  IF IDLE received from ISS indicating ISS has no more data to send
			{
				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure; // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
;
					if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] IRS (receiving IDLE) RmtLeaderMeas=%d ms", intRmtLeaderMeas);
				}
				
				if (bytDataToSendLength == 0)  // no data pending at IRS so send ACK
				{
					EncLen = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
					Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
					blnEnbARQRpt = FALSE;
                    //blnIRSBreakSent = FALSE
					return;
				}

                // Data pending so send BREAK
                //  This implements the tricky and important IRS>ISS changeover...may have to adjust parameters here for reliability 
                
				dttTimeoutTrip = Now;
				EncLen = Encode4FSKControl(0x23, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(0x23, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

				intFrameRepeatInterval = 2000; //'keep IDLE/BREAK Repeats fairly long
				ARQState = IRSBreak;   //(ONLY IRS State where repeats are used)
				blnEnbARQRpt = TRUE;// setup for repeats until changeover 
                return;
			}
			
			// This handles the final transition from IRS to ISS

			if (blnFrameDecodedOK && intFrameType == BREAK) //if BREAK (Can only come from the ISS that has now transitioned to IRS)
			{
				if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd in IRS ARQState IRSData... indicates remote side has transitioned to IRS");
           
				EncLen = EncodeDATAACK(100, bytSessionID, bytEncodedBytes); // Send ACK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

	   			blnEnbARQRpt = FALSE;
				return;
			}
			
			//handles Data frame which did not decode correctly (Failed CRC)
			
			if (!blnFrameDecodedOK && IsDataFrame(intFrameType))	// 'Incorrectly decoded frame. Send NAK with Quality
			{
				EncLen = EncodeDATANAK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send NAK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);
			
				blnEnbARQRpt = FALSE;
				return;
			}
			return;   // can we get here ??
		} 
		if( ARQState == IRSBreak)
		{
			if (blnFrameDecodedOK && intFrameType >= 0xE0)		// ' If ACK
			{
				// ACK received while in IRSBreak state completes transition to ISS

				blnEnbARQRpt = FALSE;	 // stops repeat and force new data frame or IDLE

				intLastARQDataFrameToHost = -1; // initialize to illegal value to capture first new ISS frame and pass to host
				
				if (bytCurrentFrameType == 0)	 //' hasn't been initialized yet
				{
					//' Initialize the frame type based on bandwidth

					GetNextFrameData(&intShiftUpDn, 0, NULL, TRUE); // just sets the initial data, frame type, and sets intShiftUpDn= 0
				}
				if (bytDataToSendLength > 0)
				{
					//if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame] ISS ACK Rcvd from IRS BREAK, IRS DataToSend > 0,  IRS > ISS, Substate ISSData")

					SetARDOPProtocolState(ISS);
					ARQState = ISSData;
					
					SendDataOrIDLE();
					intNAKctr = 0;
					return;
				}
				else
				{
					if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] ISS ACK Rcvd from IRS BREAK, IRS DataToSend = 0,  IRS > IDLE, Substate ISSData");
					SetARDOPProtocolState(IDLE);
					ARQState = ISSData;
				}
				SendDataOrIDLE();
				intNAKctr = 0;
			}
			return;
		}

	// IDLE State  *********************************************************************************************

	case IDLE:		  // The state where the ISS has no data to send and is looking for an ACK or BREAK from the IRS
 
		if (!blnFrameDecodedOK)
			return;				// ' No decode so continue repeating IDLE

		// process ACK, or  BREAK here Send ID if over 10 min. 

		if (intFrameType >= 0xE0)  //' if ACK
		{
			SendDataOrIDLE();
			return;
		}
		if (intFrameType ==  BREAK)
		{
			// Initiate the transisiton to IRS

			dttTimeoutTrip = Now;
			blnEnbARQRpt = FALSE;
				              
			EncLen = EncodeDATAACK(100, bytSessionID, bytEncodedBytes); // Send ACK
			Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);

			if(DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from IDLE, Go to IRS, Substate IRSData");
				
			SetARDOPProtocolState(IRS);
			ARQState = IRSData;
			intLastARQDataFrameToHost = -1;	 // precondition to an illegal frame type (insures the new IRS does not reject a frame)
			return;
		}
		if (intFrameType == DISCFRAME)  //  IF DISC received from IRS Handles protocol rule 1.5
		{
			//if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state")
               //    If MCB.AccumulateStats Then LogStats()

			QueueCommandToHost("DISCONNECTED");
			sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
			QueueCommandToHost(HostCmd);

			EncLen = Encode4FSKControl(END, bytSessionID, bytEncodedBytes);
			Mod4FSKDataAndPlay(END, &bytEncodedBytes[0], EncLen, LeaderLength);	

			bytLastARQSessionID = bytSessionID;	// ' capture this session ID to allow answering DISC from DISC state
			tmrFinalID = 500;
			blnDISCRepeating = FALSE;
			ClearDataToSend();
			SetARDOPProtocolState(DISC);
			InitializeConnection();
			blnEnbARQRpt = FALSE;
			return;
		}
		if (intFrameType == END)
		{
			if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state");
               //    If MCB.AccumulateStats Then LogStats()
	
			QueueCommandToHost("DISCONNECTED");
			sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
			QueueCommandToHost(HostCmd);

			ClearDataToSend();
   		
			if (CheckValidCallsignSyntax(strLocalCallsign))
			{
				EncLen = Encode4FSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes);
				Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent
				dttLastFECIDSent = Now;
			}
				
			SetARDOPProtocolState(DISC);
			InitializeConnection();
			blnEnbARQRpt = FALSE;
			blnDISCRepeating = FALSE;
			return;
		}

		printf("Shouldn't get here");

	// ISS state **************************************************************************************

	case ISS:
		
		if (ARQState == ISSConReq)  //' The ISS is sending Connect requests waiting for a ConAck from the remote IRS
		{
			// Session ID should be correct already (set by the ISS during first Con Req to IRS)
			// Process IRS Conack and capture IRS received leader for timing optimization
			// Process ConAck from IRS (Handles protocol rule 1.4)

			if (blnFrameDecodedOK && intFrameType >= 0x39 && intFrameType <= 0x3C)  // Process ConACK frames from IRS confirming BW is compatible and providing received leader info.
			{
				UCHAR bytDummy = 0;

				//if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame] ISS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")

				switch (intFrameType)
				{
				case 0x39:
					intSessionBW = 200;
                    break;
				case 0x3A:
					intSessionBW = 500;
                    break;
				case 0x3B:
					intSessionBW = 1000;
                    break;
				case 0x3C:
					intSessionBW = 2000;
                    break;
				}
				
				CalculateOptimumLeader(10 * bytData[0], LeaderLength);
	
				// Initialize the frame type based on bandwidth
			
				GetNextFrameData(&intShiftUpDn, &bytDummy, NULL, TRUE);	// just sets the initial data frame type and sets intShiftUpDn = 0

				// prepare the ConACK answer with received leader length

				intReceivedLeaderLen = intLeaderRcvdMs;
				
				EncLen = EncodeConACKwTiming(intFrameType, intReceivedLeaderLen, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(intFrameType, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

				intFrameRepeatInterval = 2000;
				blnEnbARQRpt = TRUE;	// Setup for repeats of the ConACK if no answer from IRS
				if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] Compatible bandwidth received from IRS ConAck: %d Hz", intSessionBW);
				ARQState = ISSConAck;
				dttLastFECIDSent = Now;

				return;
			}
		}
		if (ARQState == ISSConAck)
		{
			if (blnFrameDecodedOK && intFrameType >= 0xE0)  // if ACK received then IRS correctly received the ISS ConACK 
			{
				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure; // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
					if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] ISS RmtLeaderMeas=%d ms", intRmtLeaderMeas);
				}                       
		        intAvgQuality = 0;		// initialize avg quality
				blnEnbARQRpt = FALSE;	// stop the repeats of ConAck and enables SendDataOrIDLE to get next IDLE or Data frame

					//        if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame] ACK received in ARQState " & ARQState.ToString)
	
	 			SendDataOrIDLE();	// this should start a repeat of either IDLE (if no data to send) or Data (if outbound queue not empty) 
				blnARQConnected = TRUE;
				blnPending = FALSE;

				sprintf(HostCmd, "CONNECTED %s %d", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION ESTABLISHED WITH %s, SESSION BW = %d HZ", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);

				ARQState = ISSData;
				//' Update the main form menu status lable 
				//Dim stcStatus As Status = Nothing
				//stcStatus.ControlName = "mnuBusy"
				//stcStatus.Text = "Connected " & stcConnection.strRemoteCallsign
				//queTNCStatus.Enqueue(stcStatus)

				intTrackingQuality = -1;	 //initialize tracking quality to illegal value
				intNAKctr = 0;
			}
			else if (blnFrameDecodedOK && intFrameType == 0x2D)  // ConRejBusy
			{
				if (DebugLog) Debugprintf("[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBusy received in ARQState %s. Going to Protocol State DISC", ARQSubStates[ARQState]);

				sprintf(HostCmd, "REJECTEDBUSY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);

				SetARDOPProtocolState(DISC);
				InitializeConnection();
			}
			else if (blnFrameDecodedOK && intFrameType == 0x2E)	 // ConRejBW
			{
				//if (DebugLog) WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBW received in ARQState " & ARQState.ToString & " Going to Protocol State DISC")

				sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);

				SetARDOPProtocolState(DISC);
				InitializeConnection();
			}
			return;
		}
		
		if (ARQState == ISSData)
		{
			if (CheckForDisconnect())
				return;					// DISC sent
			
			if (!blnFrameDecodedOK)
				return;					// No decode so continue repeating either data or idle
                    
			// process ACK, NAK, DISC, END or BREAK here. Send ID if over 10 min. 
 
			if (intFrameType >= 0xE0)	// if ACK
			{
				dttTimeoutTrip = Now;
				bytLastARQDataFrameAcked = bytLastARQDataFrameSent;

				if (blnLastFrameSentData)
				{
					if (bytQDataInProcessLen)
					{
						RemoveDataFromQueue(bytQDataInProcessLen);
						bytQDataInProcessLen = 0;
					}
					ComputeQualityAvg(38 + 2 * (intFrameType - 0xE0)); // Average ACK quality to exponential averager.
					Gearshift_5();		// gear shift based on average quality
				}
				intNAKctr = 0;
				blnEnbARQRpt = FALSE;	// stops repeat and forces new data frame or IDLE
				SendDataOrIDLE();		// Send new data from outbound queue and set up repeats
			}
			else if (intFrameType <= 0x1F)		 // if NAK
			{
				if (blnLastFrameSentData)
				{
			        intNAKctr += 1;
				
					ComputeQualityAvg(38 + 2 * intFrameType);	 // Average in NAK quality to exponential averager.  
					Gearshift_5();		//' gear shift based on average quality or Shift Down if intNAKcnt >= 10
					
					if (intShiftUpDn != 0)
					{
						dttTimeoutTrip = Now;	 // Retrigger the timeout on a shift and clear the NAK counter
						intNAKctr = 0;
						SendDataOrIDLE();	//Added 0.3.5.2     Restore the last frames data, Send new data from outbound queue and set up repeats
					}
				}
                
				//     For now don't try and change the current data frame the simple gear shift will change it on the next frame 
				//           add data being transmitted back to outbound queue
			}

			else if (intFrameType == DISCFRAME) // if DISC  Handles protocol rule 1.5
			{
				//if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state")
                   //    If MCB.AccumulateStats Then LogStats()
					
				QueueCommandToHost("DISCONNECTED");
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);

				EncLen = Encode4FSKControl(END, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(END, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
      
				bytLastARQSessionID = bytSessionID; // capture this session ID to allow answering DISC from DISC state
				blnDISCRepeating = FALSE;
				tmrFinalID = 500;
				ClearDataToSend();
				SetARDOPProtocolState(DISC);
				InitializeConnection();
				blnEnbARQRpt = FALSE;
			}
				
			else if (intFrameType == END)	// ' if END
			{
				//if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state")
				//If MCB.AccumulateStats Then LogStats()
					
				QueueCommandToHost("DISCONNECTED");
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				ClearDataToSend();
				blnDISCRepeating = FALSE;

				if (CheckValidCallsignSyntax(strLocalCallsign))
				{
					EncLen = Encode4FSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes);
					Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent
					dttLastFECIDSent = Now;
				}
					
				SetARDOPProtocolState(DISC);
				InitializeConnection();
			}
			else if (intFrameType == BREAK)  // if BREAK
			{
				//' Initiate the transisiton to IRS
				//if (DebugLog) Debugprintf(("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from ARQState ISSData, Go to ProtocolState IDLE, send IDLE")
                      
				EncLen = Encode4FSKControl(IDLEFRAME, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(IDLEFRAME, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
					
				dttTimeoutTrip = Now;                      
				SetARDOPProtocolState(IDLE);		//': ARQState = ARQSubStates.ISSIdle
				intRepeatCount = 0;
				intFrameRepeatInterval = 2000;
				blnEnbARQRpt = TRUE;	//  setup for repeats if no IRS answer
			}
			return;
		}

	default:
		printf("Shouldnt get Here\n" );           
		//Logs.Exception("[ARDOPprotocol.ProcessRcvdARQFrame] 
	}
		// Unhandled Protocol state=" & GetARDOPProtocolState.ToString & "  ARQState=" & ARQState.ToString)
}


// Function to determine the IRS ConAck to reply based on intConReqFrameType received and local MCB.ARQBandwidth setting

int IRSNegotiateBW(int intConReqFrameType)
{
	//	returns the correct ConAck frame number to establish the session bandwidth to the ISS or the ConRejBW frame number if incompatible 
    //  if acceptable bandwidth sets stcConnection.intSessionBW

	switch (ARQBandwidth)
	{
	case B200FORCED:

		if ((intConReqFrameType >= 0x31 && intConReqFrameType <= 0x34)|| intConReqFrameType == 0x35)
		{
			intSessionBW = 200;
			return 0x39;		 // ConAck200
		}
		break;

	case B500FORCED:
		
		if ((intConReqFrameType >= 0x32 && intConReqFrameType <= 0x34) || intConReqFrameType == 0x36)
		{
			intSessionBW = 500;
			return 0x3A;	// ' ConAck500
		}
		break;

	case B1000FORCED:
		
		if ((intConReqFrameType >= 0x33 && intConReqFrameType <= 0x34) || intConReqFrameType == 0x37)
		{
			intSessionBW = 1000;
			return 0x3B;		 // ConAck1000
		}
		break;

	case B2000FORCED:
		
		if (intConReqFrameType == 0x34 || intConReqFrameType == 0x38)
		{
			intSessionBW = 2000;
			return 0x3C;		//  ConAck2000
		}
		break;

	case B200MAX:
		
		if (intConReqFrameType == 0x31 || intConReqFrameType == 0x35)
		{
			intSessionBW = 200;
			return 0x39;		 // ConAck200
		}
		break;

	case B500MAX:
		
		if (intConReqFrameType == 0x31)
		{
			intSessionBW = 200;
			return 0x39;		 // ConAck200
		}
		if (intConReqFrameType >= 0x32 && intConReqFrameType <= 0x34)
		{
			intSessionBW = 500;
			return 0x3A;		 // ConAck500
		}
		break;
           
	case B2000MAX:
		
		if (intConReqFrameType == 0x34 || intConReqFrameType == 0x38)
		{
			intSessionBW = 2000;
			return 0x3C;		 // ConAck2000
		}
		if (intConReqFrameType == 0x31)
		{
			intSessionBW = 200;
			return 0x39;		 // ConAck200
		}
		if (intConReqFrameType == 0x32)
		{
			intSessionBW = 500;
			return 0x3A;		 // ConAck500
		}
		if (intConReqFrameType == 0x33)
		{
			intSessionBW = 1000;
			return 0x3B;		 // ConAck1000
		}
	}

	return 0x2E;		// ConRejBW
}

//	Function to send and ARQ connect request for the current MCB.ARQBandwidth
 
BOOL SendARQConnectRequest(char * strMycall, char * strTargetCall)
{
	// Psuedo Code:
	//  Determine the proper bandwidth and target call
	//  Go to the ISS State and ISSConREq sub state
	//  Encode the connect frame with extended Leader
	//  initialize the ConReqCount and set the Frame repeat interval
	//  (Handles protocol rule 1.1) 

	InitializeConnection();
	intRmtLeaderMeas = 0;
	strcpy(strRemoteCallsign, strTargetCall);
	strcpy(strLocalCallsign, strMycall);
	strcpy(strFinalIDCallsign, strLocalCallsign);

	EncLen = EncodeARQConRequest(strMycall, strTargetCall, ARQBandwidth, bytEncodedBytes);

	if (EncLen == 0)
		return FALSE;
	
	// generate the modulation with 2 x the default FEC leader length...Should insure reception at the target
	// Note this is sent with session ID 0xFF

	Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

	blnAbort = FALSE;
	dttTimeoutTrip = Now;
	SetARDOPProtocolState(ISS);
	ARQState = ISSConReq;    
	intRepeatCount = 1;
	
	bytSessionID = GenerateSessionID(strMycall, strTargetCall);  // Now set bytSessionID to receive ConAck (note the calling staton is the first entry in GenerateSessionID) 
	bytPendingSessionID = bytSessionID;
	
	//Logs.WriteDebug("[SendARQConnectRequest] strMycall=" & strMycall & "  strTargetCall=" & strTargetCall & "  bytPendnigSessionID=" & Format(bytPendingSessionID, "X"))
	
	blnPending = TRUE;
	blnARQConnected = FALSE;
	
	intFrameRepeatInterval = 2000;  // ms ' Finn reported 7/4/2015 that 1600 was too short ...need further evaluation but temporarily moved to 2000 ms

	//' Update the main form menu status lable 
    //        Dim stcStatus As Status = Nothing
    //        stcStatus.ControlName = "mnuBusy"
    //        stcStatus.Text = "Calling " & strTargetCall
    //        queTNCStatus.Enqueue(stcStatus)

	return TRUE;
}


// Function to send 10 minute ID

BOOL Send10MinID()
{
	int dttSafetyBailout = 40;	// 100 mS intervals

	if (Now - dttLastFECIDSent > 600000 && !blnDISCRepeating)
	{

		// if (DebugLog) Debugprintf(("[ARDOPptocol.Send10MinID] Send ID Frame")
		// Send an ID frame (Handles protocol rule 4.0)

		blnEnbARQRpt = FALSE;

		EncLen = Encode4FSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes);
		Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent
		dttLastFECIDSent = Now;
		
		// Hold until PTT goes true

		while (blnLastPTT == FALSE && dttSafetyBailout)		{
		{
			Sleep(100);
			dttSafetyBailout--;
		}
		//  Now hold until PTT goes FALSE

		if (dttSafetyBailout <= 0)
		{
			//Logs.Exception("[ARDOPprotocol.Send10MinID] Safety bailout!")
 
			return TRUE;
		}
		Sleep(200);
		return TRUE;
		}
	}
	return FALSE;
}

// Function to check for and initiate disconnect from a Host DISCONNECT command

BOOL CheckForDisconnect()
{
	if (blnARQDisconnect)
	{
		//    if (DebugLog) Debugprintf(("[ARDOPprotocol.CheckForDisconnect]  ARQ Disconnect ...Sending DISC (repeat)")
		
		QueueCommandToHost("STATUS INITIATING ARQ DISCONNECT");

		EncLen = Encode4FSKControl(0x29, bytSessionID, bytEncodedBytes);
		Mod4FSKDataAndPlay(0x29, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent
		intFrameRepeatInterval = 2000;
		intRepeatCount = 1;
		blnARQDisconnect = FALSE;
		blnDISCRepeating = TRUE;
		blnEnbARQRpt = FALSE;
		return TRUE;
	}
	return FALSE;
}

//	Subroutine to safely restore data to queue to be used after a failed attempt to send data and a change of mode requiring a different data size

void RestoreDataToQueue(UCHAR * bytData,  int Len)
{
	char HostCmd[32];

	if (Len == 0)
		return;
	
	GetSemaphore();

	if (bytDataToSendLength == 0)
	{
		memcpy(bytDataToSend, bytData, Len);
		bytDataToSendLength = Len;
	}
	else
	{
		// Move data down buffer and prepend requeued

		memmove(&bytDataToSend[Len], bytDataToSend, bytDataToSendLength);	
		memcpy(bytDataToSend, bytData, Len);
		bytDataToSendLength += Len;
	}
	
	FreeSemaphore();

	sprintf(HostCmd, "BUFFER %d", bytDataToSendLength);
	QueueCommandToHost(HostCmd);
	
	tmrPollOBQueue = 1000;		// 10 Secs
}
 

