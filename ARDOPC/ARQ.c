// ARDOP TNC ARQ Code
//

#include "ARDOPC.h"

extern UCHAR bytData[];
extern int intLastRcvdFrameQuality;
extern int intRmtLeaderMeasure;
extern BOOL blnAbort;
extern int intRepeatCount;
extern int dttLastFECIDSent;

// ARQ State Variables

char AuxCalls[10][10] = {0};
int AuxCallsLength = 0;

int intBW;			// Requested connect speed
int intSessionBW;	// Negotiated speed

enum _ARQBandwidth ARQBandwidth = B1000MAX;
enum _ARQSubStates ARQState;

char strRemoteCallsign[10];
char strLocalCallsign[10];
char strFinalIDCallsign[10];

UCHAR bytLastARQSessionID;
BOOL blnEnbARQRpt;
BOOL blnListen = TRUE;
UCHAR bytPendingSessionID;
UCHAR bytSessionID = 0xff;
BOOL blnARQConnected;
int intShiftUpDn;

BOOL blnPending;
int dttTimeoutTrip;
int intLastARQDataFrameToHost;
int intAvgQuality;
int intReceivedLeaderLen;
int tmrFinalID;
tmrIRSPendingTimeout;
UCHAR bytLastDataFrameType;
BOOL blnDISCRepeating;
int intRmtLeaderMeas;

int	intOBBytesToConfirm = 0;	// remaining bytes to confirm  
int	intBytesConfirmed = 0;		// Outbound bytes confirmed by ACK and squenced
int	intReportedLeaderLen = 0;	// Zero out the Reported leader length the length reported to the remote station 
BOOL blnLastPSNPassed = FALSE;	// the last PSN passed True for Odd, False for even. 
BOOL blnInitiatedConnection = FALSE; // flag to indicate if this station initiated the connection
short dblAvgPECreepPerCarrier = 0; // computed phase error creep
int dttLastIDSent;				// date/time of last ID
int	intTotalSymbols = 0;		// To compute the sample rate error


int intDataToSend = 0;
int intFrameRepeatInterval;



extern int intLeaderRcvdMs;	

int intTrackingQuality;
int intNAKctr;


int Encode4FSKControl(UCHAR bytFrameType, UCHAR bytSessionID, UCHAR * bytreturn);
int EncodeConACKwTiming(UCHAR bytFrameType, int intRcvdLeaderLenMs, UCHAR bytSessionID, UCHAR * bytreturn);
int IRSNegotiateBW(int intConReqFrameType);


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
    //    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.CalcualteOptimumLeader] Leader Sent=" & intLeaderSentMS.ToString & "  ReportedReceived=" & intReportedReceivedLeaderMS.ToString & "  Calculated=" & stcConnection.intCalcLeader.ToString)
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


void InitializeConnection()
{
	// Sub to Initialize before a new Connection

	strRemoteCallsign[0] = 0; // remote station call sign
	intOBBytesToConfirm = 0; // remaining bytes to confirm  
	intBytesConfirmed = 0; // Outbound bytes confirmed by ACK and squenced
	intReceivedLeaderLen = 0; // Zero out received leader length (the length of the leader as received by the local station
	intReportedLeaderLen = 0; // Zero out the Reported leader length the length reported to the remote station 
	bytSessionID = 0xFF; //  Session ID 
	blnLastPSNPassed = FALSE; //  the last PSN passed True for Odd, False for even. 
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
	char  strDisplay[80];
	
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
	UCHAR bytDataToMod[256];
	unsigned char bytEncodedBytes[32];
	static UCHAR * strCallsign;
	int intReportedLeaderMS = 0;
	char HostCmd[80];
	int Len;

	switch (ProtocolState)
	{
	case DISC:
		
		// DISC State *******************************************************************************************

		if (blnFrameDecodedOK && intFrameType == 0x29) 
		{
			// Special case to process DISC from previous connection (Ending station must have missed END reply to DISC) Handles protocol rule 1.5
    
			//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState DISC, Send END with SessionID=" & Format(bytLastARQSessionID, "X") & " Stay in DISC state")

			Len = Encode4FSKControl(0x2C, bytLastARQSessionID, bytEncodedBytes);
			Mod4FSKDataAndPlay(0x2C, &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent

			tmrFinalID = 0;
			tmrFinalID = 5000;
			
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
				
				tmrIRSPendingTimeout= 10000;  // Triggers a 10 second timeout before auto abort from pending

				// (Handles protocol rule 1.2)
                            
				dttTimeoutTrip = Now;
                            
				ProtocolState = IRS;
				ARQState = IRSConAck; // now connected 

				intLastARQDataFrameToHost = -1;	 // precondition to an illegal frame type
   
				strcpy(strRemoteCallsign, bytData);
				strcpy(strLocalCallsign, strCallsign);
				strcpy(strFinalIDCallsign, strCallsign);

				intAvgQuality = 0;		// initialize avg quality 
				intReceivedLeaderLen = intLeaderRcvdMs;		 // capture the received leader from the remote ISS's ConReq (used for timing optimization)

				Len = EncodeConACKwTiming(intReply, intLeaderRcvdMs, bytPendingSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(intReply, &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent
			}
			else
			{
				// ' ConRejBW  (Incompatible bandwidths)

				// ' (Handles protocol rule 1.3)
             
				Len = Encode4FSKControl(intReply, bytPendingSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(intReply, &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent

				//QueueCommandToHost("REJECTEDBW " & strCallsigns(0))
				// QueueCommandToHost("STATUS ARQ CONNECTION FROM " & strCallsigns(0) & " REJECTED, INCOMPATIBLE BANDWIDTHS.")
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
				
				/*Then Exit Sub
                        ' see if connect request is to MyCallsign or any Aux call sign
                        strCallsigns = GetString(bytData).Split(" ") 'strCallerCallsign & " " & strTargetCallsign
                        If IsCallToMe(strCallsigns, bytPendingSessionID) Then ' Establishes the stcConnection Remote, Target Calls and session ID
                            Logs.WriteDebug("[ProcessRcvdARQFrame]2 strCallsigns(0)=" & strCallsigns(0) & "  strCallsigns(1)=" & strCallsigns(1) & "  bytPendnigSessionID=" & Format(bytPendingSessionID, "X"))
                            intRmtLeaderMeas = 0 ' insure remote leader measure is 0 
                            intReply = IRSNegotiateBW(intFrameType) ' NegotiateBandwidth
                            With stcConnection
                                If intReply <> &H2E Then ' If not ConRejBW
                                    ' Note: CONNECTION and STATUS notices were already sent from  Case ProtocolState.DISC above...no need to duplicate
                                    SetARDOPProtocolState(ProtocolState.IRS) : ARQState = ARQSubStates.IRSConAck
                                    intLastARQDataFrameToHost = -1
                                    InitializeConnection()
                                    dttTimeoutTrip = Now
                                    tmrIRSPendingTimeout.Stop() 'Stop and restart the Pending timer upon each ConReq received to ME
                                    tmrIRSPendingTimeout.Start()
                                    .strRemoteCallsign = strCallsigns(0) : .strLocalCallsign = strCallsigns(1) : strFinalIDCallsign = .strLocalCallsign
                                    .intReceivedLeaderLen = objMain.objDemod.intLeaderRcvdMs ' capture the received leader from the ISS's ConReq
                                    bytDataToMod = objMain.objMod.EncodeConACKwTiming(intReply, objMain.objDemod.intLeaderRcvdMs, strCurrentFrameFilename, bytPendingSessionID)
                                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(intReply, bytDataToMod, MCB.LeaderLength)  ' Send ConAck based on negotiated Bandwidth and Received leader timing
                                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 0) ' No delay to allow ISS to measure its TX>RX delay 
                                ElseIf intReply = &H2E Then ' ConRejBW
                                    If MCB.DebugLog Then Logs.WriteDebug("[ProcessRcvdARQFrame] Incompatible bandwidth connect request. Frame type: " & objFrameInfo.Name(intFrameType) & "   MCB.ARQBandwidth:  " & MCB.ARQBandwidth)
                                    bytDataToMod = objMain.objMod.Encode4FSKControl(intReply, strCurrentFrameFilename, bytPendingSessionID)
                                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(intReply, bytDataToMod, MCB.LeaderLength)  ' Send ConRejBW based on negotiated Bandwidth 
                                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                                    objMain.objHI.QueueCommandToHost("REJECTEDBW " & strCallsigns(0))
                                    objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION FROM " & strCallsigns(0) & " REJECTED, INCOMPATIBLE BANDWIDTHS.")
                                End If
                            End With
                        Else
                            ' this normally shouldn't happen but is put here in case another Connect request to a different station also on freq...may want to change or eliminate this
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] Call to another target while in ProtocolState.IRS, ARQSubStates.IRSConAck...Ignore")
                            Exit Sub
                        End If
						*/

			}

			// ConAck processing from ISS
                
			if (intFrameType >= 0x39 && intFrameType <= 0x3C)	// Process ConACK frames from ISS confirming Bandwidth and providing ISS's received leader info.
			{
				// If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] IRS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")
                      
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
               
				Len = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent
				blnARQConnected = TRUE;
				blnPending = FALSE;
				tmrIRSPendingTimeout = 0;

				sprintf(HostCmd, "CONNECTED %s %d", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);
 
				sprintf(HostCmd, "STATUS ARQ CONNECTION FROM %s: SESSION BW = %d HZ", strRemoteCallsign, intSessionBW);
				QueueCommandToHost(HostCmd);

				// Initialize the frame type and pointer based on bandwidth (added 0.3.1.3)

				//Dim bytDummy As Byte
                   //Dim blnDummy As Boolean
                    //GetNextFrameData(intShiftUpDn, bytDummy, blnDummy, TRUE) ' just sets the initial data, frame type, and sets intShiftUpDn= 0
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

				// If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] IRS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")
                      
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

				Len = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent
				dttTimeoutTrip = Now;
				return;
			}

			// handles DISC from ISS

			if (blnFrameDecodedOK && intFrameType == 0x29) //  IF DISC received from ISS Handles protocol rule 1.5
			{
				//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received in ProtocolState IRS, IRSData...going to DISC state")
                 //       If MCB.AccumulateStats Then LogStats()
				
				QueueCommandToHost("DISCONNECTED");		// Send END
				sprintf(HostCmd, "STATUS ARQ CONNECTION ENDED WITH %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				
				Len = Encode4FSKControl(0x2C, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(0x2C, &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent

				bytLastARQSessionID = bytSessionID;  // capture this session ID to allow answering DISC from DISC state if ISS missed Sent END
                       
				ClearDataToSend();
				tmrFinalID = 5000;
				blnDISCRepeating = FALSE;
				
				ProtocolState = DISC;
                InitializeConnection();
				blnEnbARQRpt = FALSE;
				return;
			}
			
			// handles END from ISS
/*                    ElseIf blnFrameDecodedOK And intFrameType = &H2C Then '  IF END received from ISS 
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  END frame received in ProtocolState IRS, IRSData...going to DISC state")
                        If MCB.AccumulateStats Then LogStats()
                        objMain.objHI.QueueCommandToHost("DISCONNECTED")
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                        blnDISCRepeating = FALSE
                        ClearDataToSend()
                        If CheckValidCallsignSyntax(stcConnection.strLocalCallsign) Then
                            bytDataToMod = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod)
                            dttLastFECIDSent = Now
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                        End If
                        SetARDOPProtocolState(ProtocolState.DISC)
                        InitializeConnection()
                        blnEnbARQRpt = FALSE
                        Exit Sub

                        ' handles DISCONNECT command from host
                    ElseIf CheckForDisconnect() Then
                        Exit Sub
						*/

			// This handles normal data frames

			if (blnFrameDecodedOK && IsDataFrame(intFrameType)) // Frame even/odd toggling will prevent duplicates in case of missed ACK
			{
				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure;  // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
                    //if (DebugLog) Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] IRS (receiving data) RmtLeaderMeas=" & intRmtLeaderMeas.ToString & " ms")
				}

				if (intFrameType != intLastARQDataFrameToHost) // protects against duplicates if ISS missed IRS's ACK and repeated the same frame  
				{
					AddTagToDataAndSendToHost(bytData, "ARQ", DataLen); // only correct data in proper squence passed to host   
					intLastARQDataFrameToHost = intFrameType;						dttTimeoutTrip = Now;
				}
					
				// Always ACK if it is a data frame ...ISS may have missed last ACK

				Len = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
				Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent
				blnEnbARQRpt = FALSE;					return;
			}

			// handles IDLE from ISS

			if (blnFrameDecodedOK && intFrameType == 0x26)  //  IF IDLE received from ISS indicating ISS has no more data to send
			{
				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure; // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 

					//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] IRS (receiving IDLE) RmtLeaderMeas=" & intRmtLeaderMeas.ToString & " ms")
				}
				
				if (intDataToSend == 0)  // no data pending at IRS so send ACK
				{
					Len = EncodeDATAACK(intLastRcvdFrameQuality, bytSessionID, bytEncodedBytes); // Send ACK
					Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent
					blnEnbARQRpt = FALSE;
                    //blnIRSBreakSent = FALSE
					return;
				}
                // Data pending so send BREAK
                //  This implements the tricky and important IRS>ISS changeover...may have to adjust parameters here for reliability 
                
				dttTimeoutTrip = Now;
				Len = Encode4FSKControl(0x23, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(0x23, &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent

				intFrameRepeatInterval = 2000; //'keep IDLE/BREAK Repeats fairly long
				ARQState = IRSBreak;   //(ONLY IRS State where repeats are used)
				blnEnbARQRpt = TRUE;// setup for repeats until changeover 
                return;
			}
			printf("got here");
		}
			
		printf("got here");
		
/*
      
                        ' This handles the final transition from IRS to ISS
                    ElseIf blnFrameDecodedOK And intFrameType = &H23 Then ' if BREAK (Can only come from the ISS that has now transitioned to IRS)
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd in IRS ARQState IRSData... indicates remote side has transitioned to IRS")
                        bytDataToMod = objMain.objMod.EncodeDATAACK(100, strCurrentFrameFilename, stcConnection.bytSessionID) '
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send ACK... With quality 
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) ' default  delay 
                        blnEnbARQRpt = FALSE
                        ' handles Data frame which did not decode correctly (Failed CRC)
                    ElseIf (Not blnFrameDecodedOK) And objFrameInfo.IsDataFrame(intFrameType) Then ' Incorrectly decoded frame. Send NAK with Quality
                        bytDataToMod = objMain.objMod.EncodeDATANAK(objMain.objDemod.intLastRcvdFrameQuality, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send ACK
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) ' default delay 
                        blnEnbARQRpt = FALSE
                    End If
                    Exit Sub

                ElseIf ARQState = ARQSubStates.IRSBreak Then
                    If blnFrameDecodedOK And intFrameType >= &HE0 Then ' If ACK
                        ' ACK received while in IRSBreak state completes transition to ISS
                        blnEnbARQRpt = FALSE ' stops repeat and force new data frame or IDLE
                        intLastARQDataFrameToHost = -1 ' initialize to illegal value to capture first new ISS frame and pass to host
                        If bytCurrentFrameType = 0 Then ' hasn't been initialized yet
                            Dim bytDummy As Byte ' Initialize the frame type based on bandwidth
                            Dim blnDummy As Boolean
                            GetNextFrameData(intShiftUpDn, bytDummy, blnDummy, TRUE) ' just sets the initial data, frame type, and sets intShiftUpDn= 0
                        End If
                        If intDataToSend > 0 Then
                            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS ACK Rcvd from IRS BREAK, IRS DataToSend > 0,  IRS > ISS, Substate ISSData")
                            SetARDOPProtocolState(ProtocolState.ISS) : ARQState = ARQSubStates.ISSData
                            SendDataOrIDLE(FALSE)
                            intNAKctr = 0
                        Else
                            If MCB.DebugLog Then
                                Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS ACK Rcvd from IRS BREAK, IRS DataToSend = 0,  IRS > IDLE, Substate ISSData")
                                SetARDOPProtocolState(ProtocolState.IDLE) : ARQState = ARQSubStates.ISSData
                            End If
                            SendDataOrIDLE(FALSE)
                            intNAKctr = 0
                        End If
                        Exit Sub
                    End If
                End If


                'IDLE State  *********************************************************************************************
            Case ProtocolState.IDLE  ' The state where the ISS has no data to send and is looking for an ACK or BREAK from the IRS
                If Not blnFrameDecodedOK Then Exit Sub ' No decode so continue repeating IDLE
                ' process ACK, or  BREAK here Send ID if over 10 min. 
                If intFrameType >= &HE0 Then ' if ACK
                    SendDataOrIDLE(FALSE)

                ElseIf intFrameType = &H23 Then ' if BREAK
                    ' Initiate the transisiton to IRS
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = FALSE
                    bytDataToMod = objMain.objMod.EncodeDATAACK(100, strCurrentFrameFilename, stcConnection.bytSessionID)
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from IDLE, Go to IRS, Substate IRSData")
                    SetARDOPProtocolState(ProtocolState.IRS) : ARQState = ARQSubStates.IRSData
                    intLastARQDataFrameToHost = -1 ' precondition to an illegal frame type (insures the new IRS does not reject a frame)

                ElseIf intFrameType = &H29 Then '  IF DISC received from IRS Handles protocol rule 1.5
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state")
                    If MCB.AccumulateStats Then LogStats()
                    objMain.objHI.QueueCommandToHost("DISCONNECTED")
                    objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                    bytDataToMod = objMain.objMod.Encode4FSKControl(&H2C, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send END
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    bytLastARQSessionID = stcConnection.bytSessionID ' capture this session ID to allow answering DISC from DISC state
                    tmrFinalID.Stop() : tmrFinalID.Start()
                    blnDISCRepeating = FALSE
                    ClearDataToSend()
                    SetARDOPProtocolState(ProtocolState.DISC)
                    InitializeConnection()
                    blnEnbARQRpt = FALSE

                ElseIf intFrameType = &H2C Then ' if END
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state")
                    If MCB.AccumulateStats Then LogStats()
                    objMain.objHI.QueueCommandToHost("DISCONNECTED")
                    objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                    ClearDataToSend()
                    If CheckValidCallsignSyntax(stcConnection.strLocalCallsign) Then
                        bytDataToMod = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod, stcConnection.intCalcLeader)
                        dttLastFECIDSent = Now
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    End If
                    SetARDOPProtocolState(ProtocolState.DISC)
                    InitializeConnection()
                    blnEnbARQRpt = FALSE
                    blnDISCRepeating = FALSE
                End If
*/

	// ISS state **************************************************************************************

	case ISS:
		
		if (ARQState == ISSConReq)  //' The ISS is sending Connect requests waiting for a ConAck from the remote IRS
		{
			// Session ID should be correct already (set by the ISS during first Con Req to IRS)
			// Process IRS Conack and capture IRS received leader for timing optimization
			// Process ConAck from IRS (Handles protocol rule 1.4)

			if (blnFrameDecodedOK && intFrameType >= 0x39 && intFrameType <= 0x3C)  // Process ConACK frames from IRS confirming BW is compatible and providing received leader info.
			{
				UCHAR bytDummy;
				BOOL blnDummy;

				//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS Measured RoundTrip = " & intARQRTmeasuredMs.ToString & " ms")

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
			
				GetNextFrameData(intShiftUpDn, bytDummy, blnDummy, TRUE);	// just sets the initial data frame type and sets intShiftUpDn = 0

				// prepare the ConACK answer with received leader length

				intReceivedLeaderLen = intLeaderRcvdMs;
				
				Len = EncodeConACKwTiming(intFrameType, intReceivedLeaderLen, bytSessionID, bytEncodedBytes);
				Mod4FSKDataAndPlay(intReply, &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent

				intFrameRepeatInterval = 2000;
				blnEnbARQRpt = TRUE;	// Setup for repeats of the ConACK if no answer from IRS
				//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] Compatible bandwidth received from IRS ConAck: " & stcConnection.intSessionBW.ToString & " Hz")
				ARQState = ISSConAck;
				dttLastFECIDSent = Now;

				return;
			}
		}
		if (ARQState = ISSConAck)
		{
			if (blnFrameDecodedOK && intFrameType >= 0xE0)  // if ACK received then IRS correctly received the ISS ConACK 
			{
				if (intRmtLeaderMeas == 0)
				{
					intRmtLeaderMeas = intRmtLeaderMeasure; // capture the leader timing of the first ACK from IRS, use this value to help compute repeat interval. 
					//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ISS RmtLeaderMeas=" & intRmtLeaderMeas.ToString & " ms")
				}                       
		        intAvgQuality = 0;		// initialize avg quality
				blnEnbARQRpt = FALSE;	// stop the repeats of ConAck and enables SendDataOrIDLE to get next IDLE or Data frame

					//        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ACK received in ARQState " & ARQState.ToString)
	
				SendDataOrIDLE(FALSE);	// this should start a repeat of either IDLE (if no data to send) or Data (if outbound queue not empty) 
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
				// If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBusy received in ARQState " & ARQState.ToString & " Going to Protocol State DISC")

				sprintf(HostCmd, "REJECTEDBUSY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);

				ProtocolState = DISC;
				InitializeConnection();
			}
			else if (blnFrameDecodedOK && intFrameType == 0x2E)	 // ConRejBW
			{
				//If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] ConRejBW received in ARQState " & ARQState.ToString & " Going to Protocol State DISC")

				sprintf(HostCmd, "REJECTEDBW %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);
				sprintf(HostCmd, "STATUS ARQ CONNECTION REJECTED BY %s", strRemoteCallsign);
				QueueCommandToHost(HostCmd);

				ProtocolState = DISC;
				InitializeConnection();
			}
			return;
		}
         /*
                ElseIf ARQState = ARQSubStates.ISSData Then
                    ' Process a Host command to DISCONNECT
                    If CheckForDisconnect() Then Exit Sub
                    If Not blnFrameDecodedOK Then Exit Sub ' No decode so continue repeating either data or idle
                    ' process ACK, NAK, DISC, END or BREAK here. Send ID if over 10 min. 
                    If intFrameType >= &HE0 Then ' if ACK
                        dttTimeoutTrip = Now
                        If blnLastFrameSentData Then
                            ComputeQualityAvg(38 + 2 * (intFrameType - &HE0)) ' Average ACK quality to exponential averager.
                            Gearshift_5() ' gear shift based on average quality
                        End If
                        intNAKctr = 0
                        blnEnbARQRpt = FALSE ' stops repeat and forces new data frame or IDLE
                        SendDataOrIDLE(FALSE) '       Send new data from outbound queue and set up repeats
                    ElseIf intFrameType <= &H1F Then ' if NAK
                        If blnLastFrameSentData Then
                            intNAKctr += 1
                            ComputeQualityAvg(38 + 2 * intFrameType) 'Average in NAK quality to exponential averager.  
                            Gearshift_5() ' gear shift based on average quality or Shift Down if intNAKcnt >= 10
                            If intShiftUpDn <> 0 Then
                                dttTimeoutTrip = Now ' Retrigger the timeout on a shift and clear the NAK counter
                                intNAKctr = 0
                                SendDataOrIDLE(TRUE) '  Added 0.3.5.2     Restore the last frames data, Send new data from outbound queue and set up repeats
                            End If
                        End If
                        '     For now don't try and change the current data frame the simple gear shift will change it on the next frame     '           add data being transmitted back to outbound queue
                    ElseIf intFrameType = &H29 Then ' if DISC  Handles protocol rule 1.5
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  DISC frame received Send END...go to DISC state")
                        If MCB.AccumulateStats Then LogStats()
                        objMain.objHI.QueueCommandToHost("DISCONNECTED")
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                        bytDataToMod = objMain.objMod.Encode4FSKControl(&H2C, strCurrentFrameFilename, stcConnection.bytSessionID) ' Send END
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                        bytLastARQSessionID = stcConnection.bytSessionID ' capture this session ID to allow answering DISC from DISC state
                        blnDISCRepeating = FALSE
                        tmrFinalID.Stop() : tmrFinalID.Start()
                        ClearDataToSend()
                        SetARDOPProtocolState(ProtocolState.DISC)
                        InitializeConnection()
                        blnEnbARQRpt = FALSE
                    ElseIf intFrameType = &H2C Then ' if END
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame]  END received ... going to DISC state")
                        If MCB.AccumulateStats Then LogStats()
                        objMain.objHI.QueueCommandToHost("DISCONNECTED")
                        objMain.objHI.QueueCommandToHost("STATUS ARQ CONNECTION ENDED WITH " & stcConnection.strRemoteCallsign)
                        ClearDataToSend()
                        blnDISCRepeating = FALSE
                        If CheckValidCallsignSyntax(stcConnection.strLocalCallsign) Then
                            bytDataToMod = objMain.objMod.Encode4FSKIDFrame(stcConnection.strLocalCallsign, MCB.GridSquare, strCurrentFrameFilename)
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(&H30, bytDataToMod, stcConnection.intCalcLeader)
                            dttLastFECIDSent = Now
                            objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                        End If
                        SetARDOPProtocolState(ProtocolState.DISC)
                        InitializeConnection()

                    ElseIf intFrameType = &H23 Then ' if BREAK
                        ' Initiate the transisiton to IRS
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.ProcessRcvdARQFrame] BREAK Rcvd from ARQState ISSData, Go to ProtocolState IDLE, send IDLE")
                        bytDataToMod = objMain.objMod.Encode4FSKControl(&H26, strCurrentFrameFilename, stcConnection.bytSessionID)
                        intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send IDLE
                        dttTimeoutTrip = Now
                        SetARDOPProtocolState(ProtocolState.IDLE) ': ARQState = ARQSubStates.ISSIdle
                        objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, 500) '
                        intRepeatCount = 0
                        intFrameRepeatInterval = 2000
                        blnEnbARQRpt = TRUE ' setup for repeats if no IRS answer
                    End If
                    Exit Sub
                End If
            Case Else
                Logs.Exception("[ARDOPprotocol.ProcessRcvdARQFrame] 
				*/

		// Unhandled Protocol state=" & GetARDOPProtocolState.ToString & "  ARQState=" & ARQState.ToString)
	}
}
/*
    ' Subroutine to determine the next data frame to send (or IDLE if none) 
    Private Sub SendDataOrIDLE(blnRestoreQueue As Boolean)

        Dim blnPSK As Boolean
        Dim bytFrameToSend As Byte
        Dim bytDataToMod() As Byte
        Static bytQDataInProcess(-1) As Byte

        ' Check for ID frame required (every 10 minutes)
        If blnDISCRepeating Then Exit Sub
        If blnRestoreQueue Then
            If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] Restoring " & bytQDataInProcess.Length.ToString & " bytes to OB Queue")
            RestoreDataToQueue(bytQDataInProcess)
            ReDim bytQDataInProcess(-1)
        End If
        Select Case GetARDOPProtocolState
            Case ProtocolState.IDLE
                If CheckForDisconnect() Then Exit Sub
                If intDataToSend = 0 And blnEnbARQRpt Then Exit Sub 'let repeats (Data or ILDE) continue
                If intDataToSend = 0 And Not blnEnbARQRpt Then
                    Send10MinID() ' Send ID if 10 minutes since last
                    '     Send First IDLE and setup repeat
                    bytDataToMod = objMain.objMod.Encode4FSKControl(&H26, strCurrentFrameFilename, stcConnection.bytSessionID)
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send IDLE
                    blnLastFrameSentData = FALSE
                    If GetARDOPProtocolState <> ProtocolState.IDLE Then
                        If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = 0, Go to ProtocolState IDLE")
                        SetARDOPProtocolState(ProtocolState.IDLE)
                    End If
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrame, intARQDefaultDlyMs)
                    intFrameRepeatInterval = 2000 ' Keep IDLE/BREAK repeats fairly long 
                    blnEnbARQRpt = TRUE
                    dttTimeoutTrip = Now
                    Exit Sub
                End If
                If intDataToSend > 0 Then
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = " & intDataToSend.ToString & " bytes, ProtocolState IDLE > ISS")
                    SetARDOPProtocolState(ProtocolState.ISS) : ARQState = ARQSubStates.ISSData
                    Send10MinID() ' Send ID if 10 minutes since last
                    bytQDataInProcess = GetNextFrameData(intShiftUpDn, bytFrameToSend, blnPSK, FALSE)
                    If blnPSK Then
                        bytDataToMod = objMain.objMod.EncodePSK(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        intCurrentFrameSamples = objMain.objMod.ModPSK(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("4FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        If bytFrameToSend >= &H7A And bytFrameToSend <= &H7D Then
                            intCurrentFrameSamples = objMain.objMod.Mod4FSK600BdData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        Else
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        End If
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("16FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod16FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("8FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod8FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    End If
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs)
                    blnLastFrameSentData = TRUE
                    intFrameRepeatInterval = 2000 ' set up 2000 ms repeat interval for Data may be able to shorten
                    'intFrameRepeatInterval = ComputeInterFrameInterval(1300) 'Fairly conservative...evaluate (based on measured leader from remote.
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = TRUE
                    Exit Sub
                End If

            Case ProtocolState.ISS
                If CheckForDisconnect() Then Exit Sub
                Send10MinID() ' Send ID if 10 minutes since last
                If intDataToSend > 0 Then
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = " & intDataToSend.ToString & " bytes, In ProtocolState ISS")
                    ' Get the data from the buffer here based on current data frame type
                    ' (Handles protocol Rule 2.1)
                    bytQDataInProcess = GetNextFrameData(intShiftUpDn, bytFrameToSend, blnPSK, FALSE)
                    If blnPSK Then
                        bytDataToMod = objMain.objMod.EncodePSK(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        intCurrentFrameSamples = objMain.objMod.ModPSK(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("4FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename)
                        If bytFrameToSend >= &H7A And bytFrameToSend <= &H7D Then
                            intCurrentFrameSamples = objMain.objMod.Mod4FSK600BdData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        Else
                            intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                        End If
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("16FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4, 8 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod16FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    ElseIf objFrameInfo.Name(bytFrameToSend).IndexOf("8FSK") <> -1 Then
                        bytDataToMod = objMain.objMod.EncodeFSKData(bytFrameToSend, bytQDataInProcess, strCurrentFrameFilename) ' same data encodeing can be used for 4, 8 or 16 FSK
                        intCurrentFrameSamples = objMain.objMod.Mod8FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Modulate Data frame 
                    End If
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrameFilename, intARQDefaultDlyMs) '
                    blnLastFrameSentData = TRUE
                    intFrameRepeatInterval = 2000 ' set up 2000 ms repeat interval for Data may be able to shorten
                    'intFrameRepeatInterval = ComputeInterFrameInterval(1300) ' fairly conservative based on measured leader from remote end 
                    ARQState = ARQSubStates.ISSData ' Should not be necessary
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = TRUE
                Else
                    '     Send First IDLE and setup repeat
                    bytDataToMod = objMain.objMod.Encode4FSKControl(&H26, strCurrentFrameFilename, stcConnection.bytSessionID)
                    intCurrentFrameSamples = objMain.objMod.Mod4FSKData(bytDataToMod(0), bytDataToMod, stcConnection.intCalcLeader)  ' Send IDLE
                    If MCB.DebugLog Then Logs.WriteDebug("[ARDOPprotocol.SendDataOrIDLE] DataToSend = 0, ProtocolState ISS > IDLE")
                    SetARDOPProtocolState(ProtocolState.IDLE)
                    objMain.SendFrame(intCurrentFrameSamples, strCurrentFrame, intARQDefaultDlyMs)
                    blnLastFrameSentData = FALSE
                    intFrameRepeatInterval = 2000 ' use long interval for IDLE repeats
                    dttTimeoutTrip = Now
                    blnEnbARQRpt = TRUE
                    ReDim bytQDataInProcess(-1) ' added 0.3.1.3
                End If
                Exit Sub
        End Select

    End Sub  'SendDataOrIDLE

	*/


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
/*
                Case "500FORCED"
                    If (intConReqFrameType >= &H32 And intConReqFrameType <= &H34) Or (intConReqFrameType = &H36) Then
                        .intSessionBW = 500 : return &H3A ' ConAck500
                    End If
                Case "1000FORCED"
                    If (intConReqFrameType >= &H33 And intConReqFrameType <= &H34) Or (intConReqFrameType = &H37) Then
                        .intSessionBW = 1000 : return &H3B ' ConAck1000
                    End If
                Case "2000FORCED"
                    If (intConReqFrameType = &H34) Or (intConReqFrameType = &H38) Then
                        .intSessionBW = 2000 : return &H3C ' ConAck2000
                    End If
                       .intSessionBW = 200 : return &H39 ' ConAck200
                    End If

*/

	case B500MAX:
		
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

	int Len;
	unsigned char bytEncodedBytes[32];

	InitializeConnection();
	intRmtLeaderMeas = 0;
	strcpy(strRemoteCallsign, strTargetCall);
	strcpy(strLocalCallsign, strMycall);
	strcpy(strFinalIDCallsign, strLocalCallsign);

	Len = EncodeARQConRequest(strMycall, strTargetCall, ARQBandwidth, bytEncodedBytes);

	if (Len == 0)
		return FALSE;
	
	// generate the modulation with 2 x the default FEC leader length...Should insure reception at the target
	// Note this is sent with session ID 0xFF

	Mod4FSKDataAndPlay(bytEncodedBytes[0], &bytEncodedBytes[0], Len, LeaderLength);		// only returns when all sent

	blnAbort = FALSE;
	dttTimeoutTrip = Now;
	ProtocolState = ISS;
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
