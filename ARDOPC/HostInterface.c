// ARDOP TNC Host Interface
//

#include "ARDOPC.h"

BOOL blnProcessingCmdData = FALSE; // Processing a Command or Data frame
BOOL blnHostRDY = FALSE;
extern int intFECFramesSent;

void SendData();
BOOL CheckForDisconnect();
int Encode4FSKControl(UCHAR bytFrameType, UCHAR bytSessionID, UCHAR * bytreturn);
int ComputeInterFrameInterval(int intRequestedIntervalMS);
void Break();

#ifndef WIN32

#define strtok_s strtok_r
#define _strupr strupr

char * strupr(char* s)
{
  char* p = s;

  if (s == 0)
	  return 0;

  while (*p = toupper( *p )) p++;
  return s;
}

int _memicmp(unsigned char *a, unsigned char *b, int n)
{
	if (n)
	{
		while (n && toupper(*a) == toupper(*b))
			n--, a++, b++;

		if (n)
			return toupper(*a) - toupper(*b);
   }
   return 0;
}

#endif

extern unsigned int dttTimeoutTrip;
#define BREAK 0x23
extern UCHAR bytSessionID;


//	Subroutine to add data to outbound queue (bytDataToSend)

void AddDataToDataToSend(UCHAR * bytNewData, int Len)
{
	char HostCmd[32];

	if (Len == 0)
		return;

	GetSemaphore();

	memcpy(&bytDataToSend[bytDataToSendLength], bytNewData, Len);
	bytDataToSendLength += Len;

	FreeSemaphore();

	sprintf(HostCmd, "BUFFER %d", bytDataToSendLength);
	QueueCommandToHost(HostCmd);
}


// Subroutine for processing a command from Host

void ProcessCommandFromHost(char * strCMD)
{
	char * ptrParams;
	char cmdCopy[80] = "";
	char strFault[100] = "";
	char cmdReply[1024];

	memcpy(cmdCopy, strCMD, 79);	// save before we split it up

	_strupr(strCMD);

	if (CommandTrace) WriteDebugLog("[obCommand Trace FROM host: C:%s", strCMD);

	ptrParams = strlop(strCMD, ' ');

	if (strcmp(strCMD, "ABORT") == 0)
	{
		blnAbort = TRUE;
		goto cmddone;
	}

	if (strcmp(strCMD, "ARQBW") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "ARQBW %s", ARQBandwidths[ARQBandwidth]);
			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		else
		{
			for (i = 0; i < 8; i++)
			{
				if (strcmp(ptrParams, ARQBandwidths[i]) == 0)
					break;
			}

			if (i == 8)
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			else
				ARQBandwidth = i;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "ARQCALL") == 0)
	{
		char * strCallParam = NULL;
		if (ptrParams)
			strCallParam = strlop(ptrParams, ' ');

		if (strCallParam)
		{
			if ((strcmp(ptrParams, "CQ") == 0) || CheckValidCallsignSyntax(ptrParams))
			{
				int param = atoi(strCallParam);

				if (param > 1 && param < 16)
				{
					if (ProtocolMode == ARQ)
					{
						ARQConReqRepeats = param;
						SendARQConnectRequest(Callsign, ptrParams);
						goto cmddone;
					}
					sprintf(strFault, "Not from mode FEC");
					goto cmddone;
				}
			}
		}
		sprintf(strFault, "Syntax Err: %s", cmdCopy);
		goto cmddone;
	}

	if (strcmp(strCMD, "ARQTIMEOUT") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, ARQTimeout);
			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		else
		{
			i = atoi(ptrParams);

			if (i > 29 && i < 241)	
				ARQTimeout = i;
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}
   
	if (strcmp(strCMD, "AUTOBREAK") == 0)
	{
		if (ptrParams == NULL)
		{
			if (AutoBreak)
				sprintf(cmdReply, "AUTOBREAK TRUE");
			else
				sprintf(cmdReply, "AUTOBREAK FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			AutoBreak = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			AutoBreak = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "BREAK") == 0)
	{
		Break();
		goto cmddone;  
	}

	if (strcmp(strCMD, "BUFFER") == 0)
	{
		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, bytDataToSendLength);
			SendCommandToHost(cmdReply);
		}
		else
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);

		goto cmddone;
	}
			
	if (strcmp(strCMD, "BUSYBLOCK") == 0)
	{
		if (ptrParams == NULL)
		{
			if (BusyBlock)
				sprintf(cmdReply, "BUSYBLOCK TRUE");
			else
				sprintf(cmdReply, "BUSYBLOCK FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			BusyBlock = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			BusyBlock = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}
 
	if (strcmp(strCMD, "BUSYDET") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, BusyDet);
			SendCommandToHost(cmdReply);
		}
		else
		{
			i = atoi(ptrParams);

			if (i >= 1  && i <= 10)
			{
				BusyDet = i;
			}
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}




	if (strcmp(strCMD, "CAPTURE") == 0)
	{
		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %s", strCMD, CaptureDevice);
			SendCommandToHost(cmdReply);
		}
		else
			strcpy(CaptureDevice, ptrParams);

		goto cmddone;
	}
     
	if (strcmp(strCMD, "CAPTUREDEVICES") == 0)
	{
		sprintf(cmdReply, "%s %s", strCMD, CaptureDevices);
		SendCommandToHost(cmdReply);
		goto cmddone;
	}

	if (strcmp(strCMD, "CLOSE") == 0)
	{
		blnClosing = TRUE;
		goto cmddone;
	}

	if (strcmp(strCMD, "CMDTRACE") == 0)
	{
		if (ptrParams == NULL)
		{
			if (CommandTrace)
				sprintf(cmdReply, "CMDTRACE TRUE");
			else
				sprintf(cmdReply, "CMDTRACE FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			CommandTrace = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			CommandTrace = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "CODEC") == 0)
	{
		if (ptrParams == NULL)
		{
			if (blnCodecStarted)
				sprintf(cmdReply, "CODEC TRUE");
			else
				sprintf(cmdReply, "CODEC FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			StartCodec(strFault);
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			StopCodec(strFault);
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "CWID") == 0)
	{
		if (ptrParams == NULL)
		{
			if (wantCWID)
				sprintf(cmdReply, "CWID TRUE");
			else
				sprintf(cmdReply, "CWID FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			wantCWID = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			wantCWID = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "DATATOSEND") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, bytDataToSendLength);
			goto cmddone;
		}
		else
		{
			i = atoi(ptrParams);

			if (i == 0)	
				bytDataToSendLength = 0;
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}
  
	if (strcmp(strCMD, "DEBUGLOG") == 0)
	{
		if (ptrParams == NULL)
		{
			if (DebugLog)
				sprintf(cmdReply, "DEBUGLOG TRUE");
			else
				sprintf(cmdReply, "DEBUGLOG FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			DebugLog = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			DebugLog = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "DISCONNECT") == 0)
	{
		if (ProtocolState == IDLE || ProtocolState == IRS || ProtocolState == ISS || ProtocolState == IRStoISS)
		{
			blnARQDisconnect = TRUE;
//			CheckForDisconnect();
		}
		goto cmddone;
	}
/*
            Case "DISPLAY"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & Format(MCB.DisplayFreq, "#.000"))
                ElseIf IsNumeric(strParameters) Then
                    MCB.DisplayFreq = Val(strParameters)
                    Dim stcStatus As Status = Nothing
                    If Val(strParameters) > 100000 Then
                        stcStatus.Text = "Dial: " & Format(Val(strParameters) / 1000, "##0.000") & "MHz"
                    Else
                        stcStatus.Text = "Dial: " & strParameters & "KHz"
                    End If
                    stcStatus.ControlName = "lblCF"
                    queTNCStatus.Enqueue(stcStatus)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
				*/

	if (strcmp(strCMD, "DRIVELEVEL") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, DriveLevel);
			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		else
		{
			i = atoi(ptrParams);

			if (i >= 0 && i <= 100)	
				DriveLevel = i;
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "FECID") == 0)
	{
		if (ptrParams == NULL)
		{
			if (FECId)
				sprintf(cmdReply, "FECID TRUE");
			else
				sprintf(cmdReply, "FECID FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			FECId = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			FECId = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "FASTSTART") == 0)
	{
		if (ptrParams == NULL)
		{
			if (fastStart)
				sprintf(cmdReply, "FASTSTART TRUE");
			else
				sprintf(cmdReply, "FASTSTART FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			fastStart = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			fastStart = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		
		SendCommandToHost("Ok");
		goto cmddone;
	}




	if (strcmp(strCMD, "FECMODE") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %s", strCMD, strFECMode);
			SendCommandToHost(cmdReply);
		}
		else
		{
			for (i = 0;  i < strAllDataModesLen; i++)
			{
				if (strcmp(ptrParams, strAllDataModes[i]) == 0)
				{
					strcpy(strFECMode, ptrParams);
					intFECFramesSent = 0;		// Force mode to be reevaluated
					goto cmddone;
				}
			}
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
		}	
		goto cmddone;
	}
		
	if (strcmp(strCMD, "FECREPEATS") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, FECRepeats);
			SendCommandToHost(cmdReply);
		}
		else
		{
			i = atoi(ptrParams);

			if (i >= 0 && i <= 5)	
				FECRepeats = i;
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "FECSEND") == 0)
	{
		if (ptrParams == 0)
			sprintf(strFault, "Syntax Err: %s", strCMD);	
	
		if (strcmp(ptrParams, "TRUE") == 0)
		{				
			StartFEC(NULL, 0, strFECMode, FECRepeats, FECId);
		}
		else if (strcmp(ptrParams, "FALSE") == 0)
			blnAbort = TRUE;
		else
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);

		goto cmddone;
	}

	if (strcmp(strCMD, "FSKONLY") == 0)
	{
		if (ptrParams == NULL)
		{
			if (FSKOnly)
				sprintf(cmdReply, "FSKONLY TRUE");
			else
				sprintf(cmdReply, "FSKONLY FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			FSKOnly = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			FSKOnly = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}



	if (strcmp(strCMD, "GRIDSQUARE") == 0)
	{
		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %s", strCMD, GridSquare);
			SendCommandToHost(cmdReply);
		}
		else
			if (CheckGSSyntax(ptrParams))
				strcpy(GridSquare, ptrParams);
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);

		goto cmddone;
	}

	if (strcmp(strCMD, "INITIALIZE") == 0)
	{
		blnInitializing = TRUE;
		ClearDataToSend();

//		tmrPollQueue = 0;

         //       queDataForHost.Clear()

		blnProcessingCmdData = FALSE; // Processing a Command or Data frame
		blnHostRDY = TRUE;
		blnInitializing = FALSE;

//		tmrPollQueue = 10;

		goto cmddone;
	}

	if (strcmp(strCMD, "LEADER") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, LeaderLength);
			SendCommandToHost(cmdReply);
		}
		else
		{
			i = atoi(ptrParams);

			if (i >= 100  && i <= 1200)
			{
				LeaderLength = (i + 9) /10;
				LeaderLength *= 10;				// round to 10 mS
			}
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "LISTEN") == 0)
	{
		if (ptrParams == NULL)
		{
			if (blnListen)
				sprintf(cmdReply, "LISTEN TRUE");
			else
				sprintf(cmdReply, "LISTEN FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			blnListen = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			blnListen = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "MONITOR") == 0)
	{
		if (ptrParams == NULL)
		{
			if (blnListen)
				sprintf(cmdReply, "MONITOR TRUE");
			else
				sprintf(cmdReply, "MONITOR FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			Monitor = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			Monitor = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}
   
	if (strcmp(strCMD, "MYAUX") == 0)
	{
		int i, len;
		char * ptr, * context;

		if (ptrParams == 0)
		{
			len = sprintf(cmdReply, "%s", "MYAUX ");

			for (i = 0; i < AuxCallsLength; i++)
			{
				len += sprintf(&cmdReply[len], "%s,", AuxCalls[i]);
			}
			cmdReply[len - 1] = 0;	// remove trailing space or ,
			SendCommandToHost(cmdReply);	
			goto cmddone;
		}

		ptr = strtok_s(ptrParams, ", ", &context);

		AuxCallsLength = 0;

		while (ptr && AuxCallsLength < 10)
		{
			if (CheckValidCallsignSyntax(ptr))
				strcpy(AuxCalls[AuxCallsLength++], ptr);

			ptr = strtok_s(NULL, ", ", &context);
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "MYCALL") == 0)
	{
		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %s", strCMD, Callsign);
			SendCommandToHost(cmdReply);
		}
		else
			if (CheckValidCallsignSyntax(ptrParams))
				strcpy(Callsign, ptrParams);
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);

		goto cmddone;
	}

	if (strcmp(strCMD, "PLAYBACK") == 0)
	{
		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %s", strCMD, PlaybackDevice);
			SendCommandToHost(cmdReply);
		}
		else
			strcpy(PlaybackDevice, ptrParams);

		goto cmddone;
	}
     
	if (strcmp(strCMD, "PLAYBACKDEVICES") == 0)
	{
		sprintf(cmdReply, "%s %s", strCMD, PlaybackDevices);
		SendCommandToHost(cmdReply);
		goto cmddone;
	}

	if (strcmp(strCMD, "PROTOCOLMODE") == 0)
	{
		if (ptrParams == NULL)
		{
			if (ProtocolMode == ARQ)
				sprintf(cmdReply, "PROTOCOLMODE ARQ");
			else
				sprintf(cmdReply, "PROTOCOLMODE FEC");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "ARQ") == 0)
			ProtocolMode = ARQ;
		else 
		if (strcmp(ptrParams, "FEC") == 0)
			ProtocolMode = FEC;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		SetARDOPProtocolState(DISC);	// ' set state to DISC on any Protocol mode change. 
		goto cmddone;
	}

	if (strcmp(strCMD, "PURGEBUFFER") == 0)
	{
		ClearDataToSend();  // Should precipitate an asynchonous BUFFER 0 reponse. 
		goto cmddone;  
	}

/*
            Case "RADIOANT"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.Ant.ToString)
                ElseIf strParameters = "0" Or strParameters = "1" Or strParameters = "2" Then
                    RCB.Ant = CInt(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOCTRL"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.RadioControl.ToString)
                ElseIf strParameters = "TRUE" Then
                    If IsNothing(objMain.objRadio) Then
                        objMain.SetNewRadio()
                        objMain.objRadio.InitRadioPorts()
                    End If
                    RCB.RadioControl = CBool(strParameters)
                ElseIf strParameters = "FALSE" Then
                    If Not IsNothing(objMain.objRadio) Then
                        objMain.objRadio = Nothing
                    End If
                    RCB.RadioControl = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOCTRLBAUD"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.CtrlPortBaud.ToString)
                ElseIf IsNumeric(strParameters) Then  ' Later expand to tighter syntax checking
                    RCB.CtrlPortBaud = CInt(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOCTRLDTR"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.CtrlPortDTR.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    RCB.CtrlPortDTR = CBool(strParameters)
                    objMain.objRadio.InitRadioPorts()
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOCTRLPORT"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.CtrlPort)
                Else ' Later expand to tighter syntax checking
                    RCB.CtrlPort = strParameters
                    objMain.objRadio.InitRadioPorts()
                End If
            Case "RADIOCTRLRTS"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.CtrlPortRTS.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    RCB.CtrlPortRTS = CBool(strParameters)
                    objMain.objRadio.InitRadioPorts()
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOFILTER"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.Filter.ToString)
                ElseIf IsNumeric(strParameters) AndAlso (CInt(strParameters) >= 0 And CInt(strParameters <= 3)) Then
                    RCB.Filter = CInt(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOFREQ"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.Frequency.ToString)
                ElseIf IsNumeric(strParameters) Then ' Later expand to tighter syntax checking
                    RCB.Frequency = CInt(strParameters)
                    If Not IsNothing(objMain.objRadio) Then
                        objMain.objRadio.SetDialFrequency(RCB.Frequency)
                    End If
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOICOMADD"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.IcomAdd)
                ElseIf strParameters.Length = 2 AndAlso ("0123456789ABCDEF".IndexOf(strParameters(0)) <> -1) AndAlso _
                        ("0123456789ABCDEF".IndexOf(strParameters(1)) <> -1) Then
                    RCB.IcomAdd = strParameters
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOISC"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.InternalSoundCard)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    RCB.InternalSoundCard = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOMENU"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & objMain.RadioMenu.Enabled.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    objMain.RadioMenu.Enabled = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOMODE"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.Mode)
                ElseIf strParameters = "USB" Or strParameters = "USBD" Or strParameters = "FM" Then
                    RCB.Mode = strParameters
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOMODEL"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.Model)
                Else
                    Dim strRadios() As String = objMain.objRadio.strSupportedRadios.Split(",")
                    Dim strRadioModel As String = ""
                    For Each strModel As String In strRadios
                        If strModel.ToUpper = strParameters.ToUpper Then
                            strRadioModel = strParameters
                            Exit For
                        End If
                    Next
                    If strRadioModel.Length > 0 Then
                        RCB.Model = strParameters
                    Else
                        strFault = "Model not supported :" & strCMD
                    End If
                End If

            Case "RADIOMODELS"
                If ptrSpace = -1 And Not IsNothing(objMain.objRadio) Then
                    ' Send a comma delimited list of models?
                    SendCommandToHost(strCommand & " " & objMain.objRadio.strSupportedRadios) ' Need to insure this isn't too long for Interfaces:
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOPTT"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.PTTPort)
                Else ' need syntax check
                    If strParameters.ToUpper = "CATPTT" Then
                        RCB.PTTPort = "CAT PTT"
                        objMain.objRadio.InitRadioPorts()
                    ElseIf strParameters.ToUpper = "VOX/SIGNALINK" Then
                        RCB.PTTPort = "VOX/Signalink"
                        objMain.objRadio.InitRadioPorts()
                    ElseIf strParameters.ToUpper.StartsWith("COM") Then
                        RCB.PTTPort = strParameters.ToUpper
                        objMain.objRadio.InitRadioPorts()
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                End If
            Case "RADIOPTTDTR"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.PTTDTR.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    RCB.PTTDTR = CBool(strParameters)
                    objMain.objRadio.InitRadioPorts()
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "RADIOPTTRTS"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & RCB.PTTRTS.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    RCB.PTTRTS = CBool(strParameters)
                    objMain.objRadio.InitRadioPorts()
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
                ' End of optional Radio Commands
*/
	if (strcmp(strCMD, "SENDID") == 0)
	{
		if (ProtocolState == DISC)
			SendID(wantCWID);
		else
			sprintf(strFault, "Not from State %s", ARDOPStates[ProtocolState]);

		goto cmddone;
	}
/*
            Case "SETUPMENU"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & objMain.SetupMenu.Enabled.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    objMain.SetupMenu.Enabled = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If

*/

	if (strcmp(strCMD, "NO2000.167") == 0)
	{
		if (ptrParams == NULL)
		{
			if (skip167)
				sprintf(cmdReply, "NO2000.167 TRUE");
			else
				sprintf(cmdReply, "NO2000.167 FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			skip167 = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			skip167 = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		
		SendCommandToHost("Ok");
		goto cmddone;
	}


	if (strcmp(strCMD, "SQUELCH") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, Squelch);
			SendCommandToHost(cmdReply);
		}
		else
		{
			i = atoi(ptrParams);

			if (i >= 1  && i <= 10)
			{
				Squelch = i;
			}
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}


	if (strcmp(strCMD, "STATE") == 0)
	{
		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %s", strCMD, ARDOPStates[ProtocolState]);
			SendCommandToHost(cmdReply);
		}
		else
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);

		goto cmddone;
	}

	if (strcmp(strCMD, "TRAILER") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, TrailerLength);
			SendCommandToHost(cmdReply);
		}
		else
		{
			i = atoi(ptrParams);

			if (i >= 0  && i <= 200)
			{
				TrailerLength = (i + 9) /10;
				TrailerLength *= 10;				// round to 10 mS
			}
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}

/*
            Case "TUNERANGE"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.TuningRange.ToString)
                Else
                    Select Case strParameters.Trim
                        Case "0", "50", "100", "150", "200"
                            MCB.TuningRange = CInt(strParameters)
                        Case Else
                            strFault = "Syntax Err:" & strCMD
                    End Select
                End If
            Case "TWOTONETEST"
                If objMain.objProtocol.GetARDOPProtocolState = ProtocolState.DISC Then
                    objMain.Send5SecTwoTone()
                Else
                    strFault = "Not from state " & objMain.objProtocol.GetARDOPProtocolState.ToString
                End If
*/


	if (strcmp(strCMD, "TUNINGRANGE") == 0)
	{
		int i;

		if (ptrParams == 0)
		{
			sprintf(cmdReply, "%s %d", strCMD, TuningRange);
			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		else
		{
			i = atoi(ptrParams);

			if (i >= 0 && i <= 200)	
				TuningRange = i;
			else
				sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);	
		}
		goto cmddone;
	}

	if (strcmp(strCMD, "USE600MODES") == 0)
	{
		if (ptrParams == NULL)
		{
			if (Use600Modes)
				sprintf(cmdReply, "USE600MODES TRUE");
			else
				sprintf(cmdReply, "USE600MODES FALSE");

			SendCommandToHost(cmdReply);
			goto cmddone;
		}
		
		if (strcmp(ptrParams, "TRUE") == 0)
			Use600Modes = TRUE;
		else 
		if (strcmp(ptrParams, "FALSE") == 0)
			Use600Modes = FALSE;
		else
		{
			sprintf(strFault, "Syntax Err: %s %s", strCMD, ptrParams);
			goto cmddone;
		}
		goto cmddone;
	}


	if (strcmp(strCMD, "VERSION") == 0)
	{
		sprintf(cmdReply, "VERSION %s_%s", ProductName, ProductVersion);
		SendCommandToHost(cmdReply);
		goto cmddone;
	} 
	// RDY processed earlier Case "RDY" ' no response required for RDY

	sprintf(strFault, "CMD not recoginized");

cmddone:

	if (strFault[0])
	{
		//Logs.Exception("[ProcessCommandFromHost] Cmd Rcvd=" & strCommand & "   Fault=" & strFault)
		sprintf(cmdReply, "FAULT %s", strFault);
		SendCommandToHost(cmdReply);
	}
//	SendCommandToHost("RDY");		// signals host a new command may be sent
}

 


