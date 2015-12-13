// ARDOP TNC Host Interface
//

#include "ARDOPC.h"

void QueueCommandToHost(char * Cmd)
{
	printf("Command to Host %s\n", Cmd);
}

void SendCommandToHost(char * Cmd)
{
	printf("Command to Host %s\n", Cmd);
}


void AddTagToDataAndSendToHost(char * Msg, char * Type, int Len)
{
	Msg[Len] = 0;
	printf("RX Data %s %s\n", Type, Msg);
}


// Subroutine for processing a command from Host

void ProcessCommandFromHost(char * strCMD)
{
	char * ptrParams;
	char cmdCopy[80] = "";
	char strFault[100];
	char cmdReply[120];

	memcpy(cmdCopy, strCMD, 79);	// save before we split it up

	_strupr(strCMD);

	ptrParams = strlop(strCMD, ' ');


  //      Dim strParameters As String = ""
 //       Dim strFault As String = ""

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
			sprintf(cmdReply, "ARQBW %S", ARQBandwidths[ARQBandwidth]);
			SendCommandToHost(cmdReply);
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
	/*
            Case "ARQTIMEOUT"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.ARQTimeout.ToString)
                ElseIf IsNumeric(strParameters) AndAlso (CInt(strParameters) > 29 And CInt(strParameters) < 241) Then
                    MCB.ARQTimeout = CInt(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "BUFFER"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & objMain.objProtocol.DataToSend.ToString)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "CAPTURE"
                If strParameters <> "" Then
                    MCB.CaptureDevice = strParameters.Trim
                Else
                    SendCommandToHost(strCommand & " " & MCB.CaptureDevice)
                End If
            Case "CAPTUREDEVICES"
                SendCommandToHost(strCommand & " " & objMain.CaptureDevices)
            Case "CLOSE"
                objMain.blnClosing = True
            Case "CMDTRACE"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.CommandTrace.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    MCB.CommandTrace = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "CODEC"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & objMain.blnCodecStarted.ToString.ToUpper)
                ElseIf strParameters = "TRUE" Then
                    objMain.StopCodec(strFault)
                    If strFault = "" Then
                        objMain.StartCodec(strFault)
                    End If
                ElseIf strParameters = "FALSE" Then
                    objMain.StopCodec(strFault)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "CWID"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.CWID.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    MCB.CWID = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "DATATOSEND"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & bytDataToSend.Length.ToString)
                ElseIf strParameters = 0 Then
                    objMain.objProtocol.ClearDataToSend()
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "DEBUGLOG"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.DebugLog.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    MCB.DebugLog = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "DISCONNECT"
                With objMain.objProtocol ' Ignore if not ARQ connected states
                    If .GetARDOPProtocolState = ProtocolState.IDLE Or .GetARDOPProtocolState = ProtocolState.IRS Or .GetARDOPProtocolState = ProtocolState.ISS Then
                        .blnARQDisconnect = True
                    End If
                End With
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
            Case "DRIVELEVEL"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.DriveLevel.ToString)
                Else
                    If IsNumeric(strParameters) Then
                        If CInt(strParameters) >= 0 And CInt(strParameters) <= 100 Then
                            MCB.DriveLevel = CInt(strParameters)
                        Else
                            strFault = "Syntax Err:" & strCMD
                        End If
                    End If
                End If
            Case "FECID"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.FECId.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    MCB.FECId = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "FECMODE"
                Dim blnModeOK As Boolean = False
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.FECMode)
                Else
                    For i As Integer = 0 To strAllDataModes.Length - 1
                        If strParameters = strAllDataModes(i) Then
                            MCB.FECMode = strParameters
                            blnModeOK = True
                            Exit For
                        End If
                    Next i
                    If blnModeOK = False Then strFault = "Syntax Err:" & strCMD
                End If
            Case "FECREPEATS"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.FECRepeats.ToString)
                Else
                    If IsNumeric(strParameters) Then
                        If CInt(strParameters) >= 0 And CInt(strParameters) <= 5 Then
                            MCB.FECRepeats = CInt(strParameters)
                        Else
                            strFault = "Syntax Err:" & strCMD
                        End If
                    End If
                End If
            Case "FECSEND"
                If ptrSpace = -1 Then
                    strFault = "Syntax Err:" & strCMD
                Else
                    If strParameters = "TRUE" Then
                        Dim bytData(-1) As Byte ' this will force using the data in the current inbound buffer
                        objMain.objProtocol.StartFEC(bytData, MCB.FECMode, MCB.FECRepeats, MCB.FECId)
                    ElseIf strParameters = "FALSE" Then
                        objMain.objProtocol.Abort()
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                End If
            Case "GRIDSQUARE"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.GridSquare)
                Else
                    If CheckGSSyntax(strParameters.Trim) Then
                        MCB.GridSquare = strParameters.Trim
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                End If
            Case "INITIALIZE"
                blnInitializing = True
                Thread.Sleep(200)
                tmrPollQueue.Stop()
                queDataForHost.Clear()
                blnProcessingCmdData = False ' Processing a Command or Data frame
                blnHostRDY = True
                blnInitializing = False
                tmrPollQueue.Start()
            Case "LEADER"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.LeaderLength.ToString)
                Else
                    If IsNumeric(strParameters) Then
                        If CInt(strParameters) >= 100 And CInt(strParameters) <= 1200 Then
                            MCB.LeaderLength = 10 * Math.Round(CInt(strParameters) / 10) ' Round to nearest 10 ms
                        Else
                            strFault = "Syntax Err:" & strCMD
                        End If
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                End If
            Case "LISTEN"
                If ptrSpace = -1 Then
                    strFault = "Syntax Err:" & strCMD
                Else
                    If strParameters = "TRUE" Then
                        objMain.objProtocol.Listen = True
                    ElseIf strParameters = "FALSE" Then
                        objMain.objProtocol.Listen = False
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                End If
            Case "MYAUX"
                If strParameters <> "" Then
                    Dim strAux() As String = strParameters.Split(","c)
                    ReDim MCB.AuxCalls(9)
                    For i As Integer = 0 To Math.Min(9, strAux.Length - 1)
                        If CheckValidCallsignSyntax(strAux(i).Trim.ToUpper) Then
                            MCB.AuxCalls(i) = strAux(i).Trim.ToUpper
                        End If
                    Next
                Else
                    Dim strReply As String = ""
                    For i As Integer = 0 To 9
                        If MCB.AuxCalls IsNot Nothing AndAlso MCB.AuxCalls(i) <> "" Then
                            strReply &= MCB.AuxCalls(i) & ","
                        End If
                    Next
                    If strReply.EndsWith(",") Then strReply = strReply.Substring(0, strReply.Length - 1) ' Trim the trailing ","
                    SendCommandToHost(strCommand & " " & strReply)
                End If
            Case "MYCALL"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.Callsign)
                ElseIf CheckValidCallsignSyntax(strParameters) Then
                    MCB.Callsign = strParameters
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "PLAYBACK"
                If strParameters <> "" Then
                    MCB.PlaybackDevice = strParameters.ToUpper.Trim
                Else
                    SendCommandToHost(strCommand & " " & MCB.PlaybackDevice)
                End If
            Case "PLAYBACKDEVICES"
                SendCommandToHost(strCommand & " " & objMain.PlaybackDevices)

                ' The following Radio commands are optional to allow the TNC to control the radio
                '  (All radio commands begin with "RADIO"
            Case "PROTOCOLMODE"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.ProtocolMode)
                Else
                    If strParameters.Trim = "ARQ" Or strParameters.Trim = "FEC" Then
                        MCB.ProtocolMode = strParameters.Trim
                        objMain.objProtocol.SetARDOPProtocolState(ProtocolState.DISC) ' set state to DISC on any Protocol mode change. 
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                End If
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

            Case "SENDID"
                If Main.objProtocol.GetARDOPProtocolState = ProtocolState.DISC Then
                    objMain.SendID(MCB.CWID)
                Else
                    strFault = "Not From State " & Main.objProtocol.GetARDOPProtocolState.ToString
                End If
            Case "SETUPMENU"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & objMain.SetupMenu.Enabled.ToString)
                ElseIf strParameters = "TRUE" Or strParameters = "FALSE" Then
                    objMain.SetupMenu.Enabled = CBool(strParameters)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "SQUELCH"
                If strParameters <> "" Then
                    If IsNumeric(strParameters) AndAlso (CInt(strParameters) >= 1 And CInt(strParameters) <= 10) Then
                        MCB.Squelch = CInt(strParameters)
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                Else
                    SendCommandToHost(strCommand & " " & MCB.Squelch.ToString)
                End If
            Case "STATE"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & ARDOPState.ToString)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
            Case "TRAILER"
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.TrailerLength.ToString)
                ElseIf IsNumeric(strParameters) AndAlso (CInt(strParameters) >= 0 And CInt(strParameters) <= 200) Then
                    MCB.TrailerLength = 10 * Math.Round(CInt(strParameters) / 10)
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
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
            Case "VERSION"
                SendCommandToHost("VERSION " & Application.ProductName & "_" & Application.ProductVersion)
            Case "RDY" ' no response required for RDY
*/

	else
		sprintf(strFault, "CMD not recoginized");
cmddone:

	if (strFault[0])
	{
		//Logs.Exception("[ProcessCommandFromHost] Cmd Rcvd=" & strCommand & "   Fault=" & strFault)
		sprintf(cmdReply, "FAULT %s", strFault);
		SendCommandToHost(cmdReply);
	}
	SendCommandToHost("RDY");		// signals host a new command may be sent
}

 


