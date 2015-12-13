//	ARDOP Modem Decode Sound Samples

#include "ARDOPC.h"

extern BOOL blnAbort;

int intFECFramesSent;
int FECRepeats;

UCHAR bytFrameType;

extern int intNumCar;
extern int intBaud;
extern int intDataLen;
extern int intRSLen;
extern int intSampleLen;
extern int intDataPtr;
extern int intSampPerSym;
extern int intDataBytesPerCar;
extern BOOL blnOdd;
extern char strType[16];
extern char strMod[16];
extern UCHAR bytMinQualThresh;

UCHAR bytLastFECDataFrameSent;

//char strFECMode[16] = "4FSK.200.50";
//char strFECMode[16] = "4FSK.500.100";
//char strFECMode[16] = "4FSK.1000.100";
//char strFECMode[16] = "4PSK.200.100";
//char strFECMode[16] = "8PSK.200.100";

char strFECMode[16] = "8PSK.500.100";

//char strFECMode[16] = "4PSK.500.100";	// 2 carrier
//char strFECMode[16] = "4PSK.1000.100";	// 4 carrier
//char strFECMode[16] = "4PSK.2000.100";	// 8 carrier


//char strFECMode[16] = "8FSK.200.25";
//char strFECMode[16] = "16FSK.500.25";
//char strFECMode[16] = "4FSK.2000.600";
//char strFECMode[16] = "4FSK.2000.100";

char strCurrentFrameFilename[16];

int dttLastFECIDSent;

extern int intCalcLeader;        // the computed leader to use based on the reported Leader Length

void WriteDebug(char * msg)
{
	Debugprintf(msg);
}


//int intNumCar = 0, intBaud, intDataLen = 0, intRSLen; // Do these need to be static??

// Function to get the next FEC data frame 

BOOL GetNextFECFrame()
{
	UCHAR bytData[512];
	int Len;

	if (blnAbort)
	{
		ClearDataToSend();

		if (DebugLog)
			WriteDebug("[GetNextFECFrame] FECAbort. Going to DISC state");
		KeyPTT(FALSE);  // insurance for PTT off
		SetARDOPProtocolState(DISC);
		return FALSE;
	}
	
	if (intFECFramesSent == -1)
	{
		if (DebugLog) WriteDebug("[GetNextFECFrame] intFECFramesSent = -1.  Going to DISC state");
		
		SetARDOPProtocolState(DISC);
		KeyPTT(FALSE); // insurance for PTT off
		return FALSE;
	}
	
	if (bytDataToSendLength == 0 && (intFECFramesSent % (FECRepeats + 1)) == 0 && ProtocolState == FECSend)
	{
		if (DebugLog) WriteDebug("[GetNextFECFrame] All data and repeats sent.  Going to DISC state");
            
		SetARDOPProtocolState(DISC);
		KeyPTT(FALSE); // insurance for PTT of
		return FALSE;
	}
	
	if (ProtocolState != FECSend)
		return FALSE;

	if (intFECFramesSent == 0)
	{
		// Initialize the first FEC Data frame (even) from the queue and compute the Filtered samples and filename

		char FullType[16];

		strcpy(FullType, strFECMode);
		strcat(FullType, ".E");

		bytFrameType = FrameCode(FullType);

 //           If bytFrameType = bytLastFECDataFrameSent Then ' Added 0.3.4.1 
 //               bytFrameType = bytFrameType Xor 0x1 ' insures a new start is on a different frame type. 
 //           End If

 
		FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType);

		Len = GetDataFromQueue(bytData, intDataLen * intNumCar);

		//'If bytData.Length < (intDataLen * intNumCar) Then ReDim Preserve bytData((intDataLen * intNumCar) - 1)
sendit:
		if (strcmp(strMod, "4FSK") == 0)
		{
			EncLen = EncodeFSKData(bytFrameType, bytData, Len, bytEncodedBytes);

			if (bytFrameType >= 0x7A && bytFrameType <= 0x7D)
				Mod4FSK600BdDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
			else
				Mod4FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
		}
		else if (strcmp(strMod, "16FSK") == 0)
		{
			int EncLen = EncodeFSKData(bytFrameType, bytData, Len, bytEncodedBytes);
			Mod16FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
		}
		else if (strcmp(strMod, "8FSK") == 0)
		{
			int EncLen = EncodeFSKData(bytFrameType, bytData, Len, bytEncodedBytes);          //      intCurrentFrameSamples = Mod8FSKData(bytFrameType, bytData);
			Mod8FSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
		}
		else
		{
			int EncLen = EncodePSKData(bytFrameType, bytData, Len, bytEncodedBytes);

			ModPSKDataAndPlay(bytEncodedBytes[0], bytEncodedBytes, EncLen, intCalcLeader);  // Modulate Data frame 
		}
			
			//CreateWaveStream(intCurrentFrameSamples);
		intFECFramesSent += 1;
		bytLastFECDataFrameSent = bytFrameType;
		return TRUE;
	}
	
	// Not First

	if ((intFECFramesSent % (FECRepeats + 1)) == 0)
	{
		Len = GetDataFromQueue(bytData, intDataLen * intNumCar);
		//'If bytData.Length < (intDataLen * intNumCar) Then ReDim Preserve bytData((intDataLen * intNumCar) - 1)
		//' toggle the frame type Even/Odd
		
		bytFrameType = bytFrameType ^ 1;

		goto sendit;
	}
	
	if ((Now - dttLastFECIDSent) > 600)		// 10 Mins
	{
		// Send ID every 10 Mins

		unsigned char bytEncodedBytes[16];

		EncLen = Encode4FSKIDFrame(Callsign, GridSquare, bytEncodedBytes);
		Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent

		dttLastFECIDSent = Now;
		return TRUE;
	}
	
	// just a repeat of the last frame so no changes to samples...just inc frames Sent

	// This doesn't work as we dont save samples
	
	intFECFramesSent += 1;

	return TRUE;
}

extern int frameLen;

void ProcessRcvdFECDataFrame(int intFrameType, UCHAR * bytData, BOOL blnFrameDecodedOK)
{
	bytData[frameLen] = 0;
	if (blnFrameDecodedOK)
		printf("Good FEC %s\n", bytData);
	else
		printf("Bad FEC %s\n", bytData);
}
		

