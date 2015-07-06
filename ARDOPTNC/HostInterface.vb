Imports System.IO
Imports System.IO.Ports
Imports System.Threading
Imports System.Text
Imports System.Timers
Imports nsoftware.IPWorks
Public Class HostInterface
    ' This class provides the TNC side of the TNC Host interface.  I
    ' It is intended to work with TCPIP, SERIAL, or BLUETOOTH interfaces although currently on TCPIP is fully implemented. 
    ' A similar (though not exactly the same) class is used in the Host to establish the Host to TNC interface. 

#Region "Declarations"
    ' Timers
    Private WithEvents tmrPollQueue As New Timers.Timer

    '  Objects
    Private objMain As Main
    Private WithEvents objTCPIP As New Ipdaemon
    Private WithEvents objSerial As New SerialPort
    ' Private WithEvents objBlueTooth As New 

    ' Strings
    Private strTCPIPConnectionID As String
    Private strInterfaceType As String = ""
    Private strTCPIPAddress As String
    Private strSerialPort As String
    Private strBlueToothPairing As String

    ' Integers
    Private intHostIBData_CmdPtr As Int32 = 0
    Private intTCPIPPort As Int32
    Private intSerialBaud As Int32
    Private intCRCRetries As Int32 = 0

    'Booleans 
    Private blnProcessingCmdData As Boolean = False ' Processing a Command or Data frame
    Private blnHostRDY As Boolean = True
    Private blnInitializing As Boolean = False
    Private blnTerminatingConnection As Boolean = False

    'Arrays
    Private bytHostIBData_CmdBuffer(-1) As Byte
    Private bytLastCMD_DataSent(-1) As Byte
    Private bytToSend() As Byte
    Dim strAllDataModes() As String = {"8FSK.200.25", "4FSK.200.50S", "4FSK.200.50", "4PSK.200.100S", "4PSK.200.100", "8PSK.200.100", _
                                      "16FSK.500.25S", "16FSK.500.25", "4FSK.500.100S", "4FSK.500.100", "4PSK.500.100", "8PSK.500.100", "4PSK.500.167", "8PSK.500.167", _
                                       "4FSK.1000.100", "4PSK.1000.100", "8PSK.1000.100", "4PSK.1000.167", "8PSK.1000.167", _
                                  "4FSK.2000.600S", "4FSK.2000.600", "4FSK.2000.100", "4PSK.2000.100", "8PSK.2000.100", "4PSK.2000.167", "8PSK.2000.167"}

    Private strARQBW() As String = {"200MAX", "500MAX", "1000MAX", "2000MAX", "200FORCED", "500FORCED", "1000FORCED", "2000FORCED"}

    'Queues
    Private queDataForHost As Queue = Queue.Synchronized(New Queue) 'a queue of byte arrays waiting to be sent to host
#End Region

   

#Region "Public Subs/Functions"

    ' Subroutine to set TCPIP Address, port# and interface type
    Public Sub TCPIPProperties(strAddress As String, intPort As Int32)
        strTCPIPAddress = strAddress
        intTCPIPPort = intPort
        strInterfaceType = "TCPIP"
    End Sub 'TCPIPProperties

    ' Subroutine to set Serial COM port, baud and interface type
    Public Sub SerialProperties(strComPort As String, intBaud As Int32)
        strSerialPort = strComPort
        intSerialBaud = intBaud
        strInterfaceType = "SERIAL"
    End Sub  ' SerialProperties

    ' Subroutine to set BlueTooth pairing and interface type
    Public Sub BluetoothProperties(strPairing As String)
        strBlueToothPairing = strPairing
        strInterfaceType = "BLUETOOTH"
    End Sub 'BluetoothProperties

    ' Enable a TCPIP, Serial,  or BlueTooth Link with the Host (Opens the port for listening...Does NOT initiate the connection!) 
    Public Function EnableHostLink() As Boolean
        blnTerminatingConnection = False
        Dim stcStatus As Status = Nothing
        MCB.LinkedToHost = False
        stcStatus.BackColor = Color.Khaki ' preset to yellow ....connection switches to green.
        stcStatus.ControlName = "lblHost"
        If strInterfaceType = "TCPIP" Then
            stcStatus.Text = "TCPIP"
            Try
                If Not IsNothing(objTCPIP) Then
                    objTCPIP.Listening = False
                    objTCPIP.Shutdown()
                    objTCPIP.Dispose()
                    objTCPIP = Nothing
                    objTCPIP = New Ipdaemon
                End If
                strTCPIPConnectionID = ""
                objTCPIP.LocalPort = intTCPIPPort
                objTCPIP.LocalHost = strTCPIPAddress
                objTCPIP.Listening = True
                queTNCStatus.Enqueue(stcStatus)
                Logs.WriteDebug("[HostInterface.EnableHostLink] objTCPIP.Listening = " & objTCPIP.Listening.ToString & " on " & strTCPIPAddress & " Port:" & intTCPIPPort.ToString)
            Catch Ex As Exception
                Logs.Exception("[HostInterface.EnableHostLink]  TCPIP link startup, Err: " & Ex.ToString)
                stcStatus.BackColor = Color.LightSalmon ' set to red ....
                queTNCStatus.Enqueue(stcStatus)
                Return False
            End Try
            bytHostIBData_CmdBuffer = Nothing
            intHostIBData_CmdPtr = 0
            Return True
        ElseIf strInterfaceType = "SERIAL" Then ' incomplete 
            stcStatus.Text = "Host Serial"
            If IsNothing(objSerial) Then
                objSerial = New SerialPort
            End If
            objSerial.PortName = MCB.SerCOMPort
            objSerial.BaudRate = MCB.SerBaud
            objSerial.Parity = Parity.None
            objSerial.StopBits = StopBits.One
            Try
                objSerial.Open()
                objSerial.ReadExisting()
            Catch ex As Exception
                Logs.Exception("[HostInterface.EnableHostLink]  Serial link startup, Err: " & ex.ToString)
                stcStatus.BackColor = Color.LightSalmon ' set to red ....
                queTNCStatus.Enqueue(stcStatus)
                Return False
            End Try
        ElseIf strInterfaceType = "BLUETOOTH" Then
            stcStatus.Text = "Host BlueTooth"
            ' BlueTooth link initialization here????
            Return True
        End If
        Return False
    End Function  'EnableHostLink

    ' Function to send a text command to the Host
    Private Function SendCommandToHost(ByVal strText As String) As Boolean
        '  This is from TNC side as identified by the leading "c:"   (Host side sends "C:")
        ' Subroutine to send a line of text (terminated with <Cr>) on the command port... All commands beging with "c:" and end with <Cr>
        ' A two byte CRC appended following the <Cr>
        ' The strText cannot contain a "c:" sequence or a <Cr>
        ' Returns TRUE if command sent successfully.
        ' Form byte array to send with CRC
        ' TODO:  Complete for Serial and BlueTooth

        Dim bytToSend() As Byte = GetBytes("c:" & strText.Trim.ToUpper & vbCr)
        ReDim Preserve bytToSend(bytToSend.Length + 1) ' resize 2 bytes larger for CRC
        GenCRC16(bytToSend, 2, bytToSend.Length - 3, &HFFFF) ' Generate CRC starting after "c:"  
        bytLastCMD_DataSent = bytToSend
        If strInterfaceType = "TCPIP" Then
            If objTCPIP Is Nothing Then Return False
            If strTCPIPConnectionID = "" Then Return False
            Try
                objTCPIP.Send(strTCPIPConnectionID, bytToSend)
                If MCB.CommandTrace Then Logs.WriteDebug(" Command Trace TO Host  c:" & strText.Trim.ToUpper)
                Return True
            Catch
                Logs.Exception("[HostInterface.SendCommandToHost] TCPIP Port,  c:" & strText & "  Err:" & Err.Number.ToString & " " & Err.Description)
                Return False
            End Try
        ElseIf strInterfaceType = "SERIAL" Then
            If (Not IsNothing(objSerial)) AndAlso objSerial.IsOpen Then
                Try
                    objSerial.Write(bytToSend, 0, bytToSend.Length)
                    Return True
                Catch ex As Exception
                    Logs.Exception("[HostInterface.SendCommandToHost] Serial Port,  c:" & strText & "  Err:" & Err.Number.ToString & " " & Err.Description)
                    Return False
                End Try
            Else
                Logs.Exception("[HostInterface.SendCommandToHost] Serial Port " & MCB.SerCOMPort & " not open")
            End If

        ElseIf strInterfaceType = "BLUETOOTH" Then
            ' This will handle BlueTooth connections ... TODO: Add BlueTooth
            Return False ' Temporary
        Else
            Logs.Exception("[HostInterface.SendCommandToHost]  No TCPIP, serial,  or BlueTooth parameters")
        End If
        Return False
    End Function ' SendCommandToHost

    ' Function to queue a text command to the Host used for all asynchronous Commmands (e.g. BUSY etc)
    Public Sub QueueCommandToHost(ByVal strText As String)
        '  This is from TNC side as identified by the leading "c:"   (Host side sends "C:")
        ' A two byte CRC appended following the <Cr>
        ' The strText cannot contain a "c:" sequence or a <Cr>

        If blnInitializing Then Exit Sub
        Dim bytToSend() As Byte = GetBytes("c:" & strText.Trim.ToUpper & vbCr)   ' Form byte array to send with CRC
        ReDim Preserve bytToSend(bytToSend.Length + 1) ' resize 2 bytes larger for CRC
        GenCRC16(bytToSend, 2, bytToSend.Length - 3, &HFFFF) ' Generate CRC starting after "c:"  
        queDataForHost.Enqueue(bytToSend)
    End Sub  ' QueueCommandToHost

    ' Function to queue a byte array to the host with Data type tag 
    Public Sub QueueDataToHost(ByVal bytData() As Byte)
        '  This is from TNC side as identified by the leading "d:"   (Host side sends data with leading  "D:")
        ' includes 16 bit CRC check on Data Len + Data (does not CRC the leading "d:")
        ' bytData contains a "tag" in its leading 3 bytes: "ARQ", "FEC" or "ERR" which are examined and stripped by Host (optionally used by host for display)
        ' Max data size should be 2000 bytes or less for timing purposes

        If blnInitializing Then Exit Sub
        Dim bytToSend(2 + 2 + bytData.Length + 2 - 1) As Byte  ' add bytes for: "d:", 2 byte data count,  and 2 byte CRC
        Array.Copy(bytData, 0, bytToSend, 4, bytData.Length)
        bytToSend(0) = &H64 '  "d" indicates data from TNC
        bytToSend(1) = &H3A ' ":"
        bytToSend(2) = CByte(((bytData.Length) And &HFF00) >> 8) ' MS byte of count  (Includes strDataType but does not include the two trailing CRC bytes)
        bytToSend(3) = CByte((bytData.Length) And &HFF)  'LS byte of count
        ' Compute the CRC starting with the bytCount + Data tag + Data (skipping over the "d:")
        GenCRC16(bytToSend, 2, bytToSend.Length - 3, &HFFFF)
        queDataForHost.Enqueue(bytToSend)
    End Sub  ' QueueDataToHost

    ' Function to terminate the Host link
    Public Function TerminateHostLink() As Boolean
        blnTerminatingConnection = True
        Dim stcStatus As Status = Nothing
        MCB.LinkedToHost = False
        stcStatus.BackColor = Color.DarkGray
        stcStatus.ControlName = "lblHost"
        If strInterfaceType = "TCPIP" Then
            Try
                If Not IsNothing(objTCPIP) Then
                    objTCPIP.Linger = False
                    objTCPIP.Shutdown()
                    objTCPIP.Dispose()
                    objTCPIP = Nothing
                End If
                strTCPIPConnectionID = ""
                stcStatus.Text = "Closed"
                queTNCStatus.Enqueue(stcStatus)
            Catch Ex As Exception
                Logs.Exception("[HostInterface.TerminateHostLink]  Err: " & Ex.ToString)
                stcStatus.BackColor = Color.LightSalmon ' set to red ....
                queTNCStatus.Enqueue(stcStatus)
                Return False
            End Try
            Return True
        ElseIf strInterfaceType = "SERIAL" Then

        ElseIf strInterfaceType = "BLUETOOTH" Then
        Else
            Logs.Exception("[HostInterface.TerminateHostLink]  Interface Type not set")
        End If
        Return False
    End Function  'TerminateHostLink

    ' Subroutine to establish pointer back to main calling program
    Public Sub New(objM As Main)
        objMain = objM
        tmrPollQueue.Interval = 100 ' poll the queue every 100 ms
    End Sub
#End Region


#Region "Private Subs/Functions"

    ' Subroutine to handle new TCPIP Connection
    Private Sub objTCPIP_OnConnected(sender As Object, e As nsoftware.IPWorks.IpdaemonConnectedEventArgs) Handles objTCPIP.OnConnected
        Dim stcStatus As Status = Nothing
        stcStatus.BackColor = Color.LightGreen
        stcStatus.ControlName = "lblHost"
        stcStatus.Text = "TCPIP on port " & objTCPIP.LocalPort.ToString
        If strTCPIPConnectionID = "" Then
            strTCPIPConnectionID = e.ConnectionId
            If MCB.CommandTrace Then Logs.WriteDebug(" Connected to host with ID=" & strTCPIPConnectionID)
            If SendCommandToHost("RDY") Then
                queTNCStatus.Enqueue(stcStatus)
                strTCPIPConnectionID = e.ConnectionId
                MCB.LinkedToHost = True
                ReDim bytHostIBData_CmdBuffer(-1)
                intHostIBData_CmdPtr = 0
            Else
                Logs.Exception("[HostInterface.objTCPIP_OnConnected] Failure to send c:RDY reply on ConnectionId " & e.ConnectionId)
                objTCPIP.Disconnect(e.ConnectionId)
                MCB.LinkedToHost = False
                strTCPIPConnectionID = ""
            End If
        Else
            Logs.Exception("[HostInterface.objTCPIP_OnConnected] Connection request received while already connected. Reject connection.")
            objTCPIP.Disconnect(e.ConnectionId)
        End If
    End Sub  'objTCPIP_OnConnected

    ' Subroutine to handle TCPIP Data in 
    Private Sub objTCPIP_OnDataIn(sender As Object, e As nsoftware.IPWorks.IpdaemonDataInEventArgs) Handles objTCPIP.OnDataIn
        ' This creates a first-in first-out buffer and pointers to handle receiving commands or data.
        ' Data may be received on NON CMD or Data frame boundaries. (intended to handle buffer and latency issues)

        Static strCommandFromHost As String = ""
        Static bytDataFromHost() As Byte = Nothing
        Static intDataBytesToReceive As Int32 = 0
        Static blnReceivingCMD As Boolean = False
        Static blnReceivingData As Boolean = False
        Static intDataBytePtr As Int32 = 0
        Static intCMDStartPtr As Int32 = 0
        Static intDataStartPtr As Int32 = 0

        AppendDataToBuffer(e.TextB, bytHostIBData_CmdBuffer)

        ' look for start of Command ("C:")  or Data (D:") and establish start pointer (Capital C or D indicates from Host)
SearchForStart:
        If Not (blnReceivingCMD Or blnReceivingData) Then
            For i As Integer = intHostIBData_CmdPtr To bytHostIBData_CmdBuffer.Length - 2
                If bytHostIBData_CmdBuffer(i) = &H43 And bytHostIBData_CmdBuffer(i + 1) = &H3A Then ' search for ASCII "C:"
                    ' start of command.
                    blnHostRDY = False
                    intCMDStartPtr = i
                    blnReceivingCMD = True
                    blnProcessingCmdData = True
                    blnReceivingData = False
                    Exit For
                ElseIf bytHostIBData_CmdBuffer(i) = &H44 And bytHostIBData_CmdBuffer(i + 1) = &H3A Then ' search for ASCII "D:"
                    ' start of Data
                    blnHostRDY = False
                    intDataStartPtr = i
                    blnReceivingCMD = False
                    blnReceivingData = True
                    blnProcessingCmdData = True
                    intDataBytesToReceive = 0
                    Exit For
                End If
            Next i
        End If
        If blnReceivingCMD Then
            For i As Integer = intCMDStartPtr To bytHostIBData_CmdBuffer.Length - 3 ' Look for <Cr> with room for 2 byte CRC
                If bytHostIBData_CmdBuffer(i) = &HD Then ' search for Carriage Return which signals the end of a Command (note 2 CRC bytes to follow)
                    Dim bytCmd(i - intCMDStartPtr) As Byte ' 2 bytes added for CRC, and "C:" skipped
                    Array.Copy(bytHostIBData_CmdBuffer, intCMDStartPtr + 2, bytCmd, 0, bytCmd.Length) 'copy over the Command (less :C:") and the 2 byte CRC
                    If CheckCRC16(bytCmd, &HFFFF) Then ' check the CRC
                        'CRC OK:
                        ReDim Preserve bytCmd(bytCmd.Length - 3) ' Drop off the CRC
                        strCommandFromHost = GetString(bytCmd).ToUpper.Trim
                        If MCB.CommandTrace Then Logs.WriteDebug("[objTCPIP.OnDataIn] Command Trace FROM host: C:" & strCommandFromHost)
                        ' Process the received and CRC checked command here:
                        If strCommandFromHost = "CRCFAULT" Then
                            If intCRCRetries > 2 Then
                                Logs.Exception("[HostInterface.objTCPIP_OnDataIn] 3 CRC Faults on last CMD/Data " & bytLastCMD_DataSent.Length.ToString & " bytes ...aborting transfer")
                                intCRCRetries = 0
                                blnHostRDY = True
                                ReDim bytLastCMD_DataSent(-1)
                            Else
                                objTCPIP.Send(strTCPIPConnectionID, bytLastCMD_DataSent)
                                If MCB.CommandTrace Then Logs.WriteDebug(" Repeat last CMD/Data " & bytLastCMD_DataSent.Length.ToString & " bytes")
                                Logs.Exception("[HostInterface.objTCPIP_OnDataIn] CRCFAULT received,  Retry last CMD_DataSent FROM host")
                                intCRCRetries += 1
                            End If
                        ElseIf strCommandFromHost <> "RDY" Then 'host can receive commands or data
                            ProcessCommandFromHost(strCommandFromHost) '(sends (reply or Fault) + RDY)
                        ElseIf strCommandFromHost = "RDY" Then
                            blnHostRDY = True
                        End If
                    Else
                        If MCB.CommandTrace Then Logs.WriteDebug("[objTCPIP.OnDataIn] Command Trace FROM host with CRCFAULT")
                        SendCommandToHost("CRCFAULT") ' indicates to Host to repeat the command
                    End If
                    ' resize buffer and reset pointer
                    blnProcessingCmdData = False
                    blnReceivingCMD = False
                    ' resize the bufffer, and set pointer to it's start.
                    Dim bytTemp(bytHostIBData_CmdBuffer.Length - i - 4) As Byte 'skip past the 2 byte CRC
                    If bytTemp.Length > 0 Then Array.Copy(bytHostIBData_CmdBuffer, i + 3, bytTemp, 0, bytTemp.Length)
                    bytHostIBData_CmdBuffer = bytTemp
                    intHostIBData_CmdPtr = 0
                    If bytHostIBData_CmdBuffer.Length > 0 Then GoTo SearchForStart
                End If
            Next i
        End If

        If blnReceivingData Then
            If intDataBytesToReceive = 0 Then ' Data length must always be >0 for a legitimate data frame:
                If bytHostIBData_CmdBuffer.Length - intDataStartPtr >= 4 Then
                    ' Compute the byte count to receive plus 2 additional bytes for the 16 bit CRC
                    intDataBytesToReceive = CInt(bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 2)) << 8
                    intDataBytesToReceive += (CInt(bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 3)) + 2) ' includes  data + 2 byte CRC
                    ReDim bytDataFromHost(intDataBytesToReceive + 1) ' make 2 larger to include the byte count (CRC computed starting with the byte Count)
                    bytDataFromHost(0) = bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 2) ' MSB of count
                    bytDataFromHost(1) = bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 3) ' LSB of count
                    intDataBytePtr = 2
                    intHostIBData_CmdPtr = intHostIBData_CmdPtr + 4 ' advance pointer past "D:" and byte count
                End If
            End If
            If intDataBytesToReceive > 0 And (intHostIBData_CmdPtr < bytHostIBData_CmdBuffer.Length) Then
                For i As Integer = 0 To bytHostIBData_CmdBuffer.Length - intHostIBData_CmdPtr - 1
                    bytDataFromHost(intDataBytePtr) = bytHostIBData_CmdBuffer(intHostIBData_CmdPtr)
                    intDataBytePtr += 1
                    intHostIBData_CmdPtr += 1
                    intDataBytesToReceive -= 1
                    If intDataBytesToReceive = 0 Then Exit For
                Next i
                If intDataBytesToReceive = 0 Then
                    ' Process bytDataFromHost here (check CRC etc) 
                    If CheckCRC16(bytDataFromHost) Then
                        If MCB.CommandTrace Then Logs.WriteDebug(" Data Trace from Host: " & (bytDataFromHost.Length).ToString & " bytes. CRC OK")
                        Dim bytDataToAppend(bytDataFromHost.Length - 5) As Byte
                        Array.Copy(bytDataFromHost, 2, bytDataToAppend, 0, bytDataFromHost.Length - 4)
                        objMain.objProtocol.AddDataToDataToSend(bytDataToAppend) ' Append data here to inbound queue
                        SendCommandToHost("RDY") ' This signals the host more data or commands can be accepted
                    Else
                        If MCB.CommandTrace Then Logs.WriteDebug(" Data Trace FROM Host: " & (bytDataFromHost.Length).ToString & " bytes. CRC Fail")
                        Logs.Exception("[HostInterface.objTCPIP.OnDataIn] Data Trace from Host:" & (bytDataFromHost.Length).ToString & " bytes. CRC Fail")
                        SendCommandToHost("CRCFAULT")
                    End If
                    blnProcessingCmdData = False
                    blnReceivingData = False
                    ' resize the bufffer, and set pointer to it's start.
                    If intHostIBData_CmdPtr >= bytHostIBData_CmdBuffer.Length - 1 Then
                        ReDim bytHostIBData_CmdBuffer(-1) ' clear the buffer and zero the pointer
                        intHostIBData_CmdPtr = 0
                    Else  ' resize the bufffer, and set pointer to it's start.
                        Dim bytTemp(bytHostIBData_CmdBuffer.Length - intHostIBData_CmdPtr - 2) As Byte
                        Array.Copy(bytHostIBData_CmdBuffer, intHostIBData_CmdPtr, bytTemp, 0, bytTemp.Length)
                        bytHostIBData_CmdBuffer = bytTemp
                        intHostIBData_CmdPtr = 0
                    End If
                End If
            End If
        End If
    End Sub  ' objTCPIP_OnDataIn

    ' Subroutine to append new byte data to a byte buffer
    Private Sub AppendDataToBuffer(ByRef bytNewData() As Byte, ByRef bytBuffer() As Byte)
        If bytNewData.Length = 0 Then Exit Sub
        Dim intStartPtr As Integer = bytBuffer.Length
        ReDim Preserve bytBuffer(bytBuffer.Length + bytNewData.Length - 1)
        Array.Copy(bytNewData, 0, bytBuffer, intStartPtr, bytNewData.Length)
    End Sub  ' AppendDataToBuffer

    ' Subroutine to handle TCPIP disconnect
    Private Sub objTCPIP_OnDisconnected(sender As Object, e As nsoftware.IPWorks.IpdaemonDisconnectedEventArgs) Handles objTCPIP.OnDisconnected
        strTCPIPConnectionID = ""
        Dim stcStatus As Status = Nothing
        stcStatus.BackColor = Color.LightSalmon
        stcStatus.ControlName = "lblHost"
        queTNCStatus.Enqueue(stcStatus)
    End Sub '  objTCPIP_OnDisconnected

    ' Subroutine to log TCPIP errors
    Private Sub objTCPIP_OnError(sender As Object, e As nsoftware.IPWorks.IpdaemonErrorEventArgs) Handles objTCPIP.OnError
        ' Supress error log if due to shutdown of connection. 
        If e.Description.IndexOf("connection was aborted") = -1 Then Logs.Exception("[HostInterface.objTCPIP_OnError] Err: " & e.Description)
    End Sub  'objTCPIP_OnError

    ' Subroutine for processing a command from Host
    Private Sub ProcessCommandFromHost(strCMD As String)

        Dim strCommand As String = strCMD.ToUpper.Trim
        Dim strParameters As String = ""
        Dim strFault As String = ""
        Dim ptrSpace As Integer = strCommand.IndexOf(" ")

        If ptrSpace <> -1 Then
            strParameters = strCommand.Substring(ptrSpace).Trim
            strCommand = strCommand.Substring(0, ptrSpace)
        End If

        Select Case strCommand
            Case "ABORT"
                'If MCB.ProtocolMode = "FEC" Then
                objMain.objProtocol.Abort()
                ' Else
                ' objMain.objProtocol.AbortARQ()
                'End If
            Case "ARQBW"
                Dim blnModeOK As Boolean = False
                If ptrSpace = -1 Then
                    SendCommandToHost(strCommand & " " & MCB.ARQBandwidth)
                Else
                    For i As Integer = 0 To strARQBW.Length - 1
                        If strParameters = strARQBW(i) Then
                            MCB.ARQBandwidth = strParameters
                            blnModeOK = True
                            Exit For
                        End If
                    Next i
                    If blnModeOK = False Then strFault = "Syntax Err:" & strCMD
                End If
            Case "ARQCALL"
                Dim strCallParam() As String = strParameters.Split(" ")
                If strCallParam.Length = 2 AndAlso ((strCallParam(0).Trim.ToUpper = "CQ") Or CheckValidCallsignSyntax(strCallParam(0).Trim.ToUpper)) _
                        AndAlso IsNumeric(strCallParam(1)) Then
                    If CInt(strCallParam(1)) > 1 And CInt(strCallParam(1) < 16) Then
                        If MCB.ProtocolMode <> "ARQ" Then
                            strFault = "Not from mode " & MCB.ProtocolMode
                        Else
                            MCB.ARQConReqRepeats = CInt(strCallParam(1))
                            objMain.objProtocol.SendARQConnectRequest(MCB.Callsign, strCallParam(0).Trim.ToUpper)
                        End If
                    Else
                        strFault = "Syntax Err:" & strCMD
                    End If
                Else
                    strFault = "Syntax Err:" & strCMD
                End If
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
                objTCPIP.Linger = False
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
                        If MCB.AuxCalls(i) <> "" Then strReply &= MCB.AuxCalls(i) & ","
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

            Case Else
                strFault = "CMD not recoginized"
        End Select
        If strFault.Length > 0 Then
            Logs.Exception("[ProcessCommandFromHost] Cmd Rcvd=" & strCommand & "   Fault=" & strFault)
            SendCommandToHost("FAULT " & strFault)
        End If
        SendCommandToHost("RDY") ' signals host a new command may be sent
    End Sub  'ProcessCommandFromHost

    ' Subroutine to compute a 16 bit CRC value and append it to the Data...
    Private Sub GenCRC16(ByRef Data() As Byte, intStartIndex As Int32, intStopIndex As Int32, Optional intSeed As Int32 = &HFFFF)
        ' For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
        ' intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

        Dim intPoly As Integer = &H8810 ' This implements the CRC polynomial  x^16 + x^12 +x^5 + 1
        Dim intRegister As Int32 = intSeed

        For j As Integer = intStartIndex To intStopIndex
            For i As Integer = 7 To 0 Step -1 ' for each bit processing MS bit first
                Dim blnBit As Boolean = (Data(j) And CByte(2 ^ i)) <> 0
                If (intRegister And &H8000) = &H8000 Then ' the MSB of the register is set
                    ' Shift left, place data bit as LSB, then divide
                    ' Register := shiftRegister left shift 1
                    ' Register := shiftRegister xor polynomial
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                    intRegister = intRegister Xor intPoly
                Else ' the MSB is not set
                    ' Register is not divisible by polynomial yet.
                    ' Just shift left and bring current data bit onto LSB of shiftRegister
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                End If
            Next i
        Next j
        ' Put the two CRC bytes after the stop index
        Data(intStopIndex + 1) = CByte((intRegister And &HFF00) \ 256) ' MS 8 bits of Register
        Data(intStopIndex + 2) = CByte(intRegister And &HFF) ' LS 8 bits of Register
    End Sub 'GenCRC16

    ' Function to compute a 16 bit CRC value and check it against the last 2 bytes of Data (the CRC) ..
    Private Function CheckCRC16(ByRef Data() As Byte, Optional intSeed As Int32 = &HFFFF) As Boolean
        ' Returns True if CRC matches, else False
        ' For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
        ' intSeed is the seed value for the shift register and must be in the range 0-&HFFFF

        Dim intPoly As Integer = &H8810 ' This implements the CRC polynomial  x^16 + x^12 +x^5 + 1
        Dim intRegister As Int32 = intSeed

        For j As Integer = 0 To Data.Length - 3 ' 2 bytes short of data length
            For i As Integer = 7 To 0 Step -1 ' for each bit processing MS bit first
                Dim blnBit As Boolean = (Data(j) And CByte(2 ^ i)) <> 0
                If (intRegister And &H8000) = &H8000 Then ' the MSB of the register is set
                    ' Shift left, place data bit as LSB, then divide
                    ' Register := shiftRegister left shift 1
                    ' Register := shiftRegister xor polynomial
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                    intRegister = intRegister Xor intPoly
                Else ' the MSB is not set
                    ' Register is not divisible by polynomial yet.
                    ' Just shift left and bring current data bit onto LSB of shiftRegister
                    If blnBit Then
                        intRegister = &HFFFF And (1 + 2 * intRegister)
                    Else
                        intRegister = &HFFFF And (2 * intRegister)
                    End If
                End If
            Next i
        Next j

        ' Compare the register with the last two bytes of Data (the CRC) 
        If Data(Data.Length - 2) = CByte((intRegister And &HFF00) \ 256) Then
            If Data(Data.Length - 1) = CByte(intRegister And &HFF) Then
                Return True
            End If
        End If
        Return False
    End Function 'CheckCRC16

    ' Function to convert string Text (ASCII) to byte array
    Private Function GetBytes(ByVal strText As String) As Byte()
        ' Converts a text string to a byte array...

        Dim bytBuffer(strText.Length - 1) As Byte
        For intIndex As Integer = 0 To bytBuffer.Length - 1
            bytBuffer(intIndex) = CByte(Asc(strText.Substring(intIndex, 1)))
        Next
        Return bytBuffer
    End Function 'GetBytes

    ' Function to Get ASCII string from a byte array
    Private Function GetString(ByVal bytBuffer() As Byte, _
       Optional ByVal intFirst As Integer = 0, Optional ByVal intLast As Integer = -1) As String
        ' Converts a byte array to a text string...

        If intFirst > bytBuffer.GetUpperBound(0) Then Return ""
        If intLast = -1 Or intLast > bytBuffer.GetUpperBound(0) Then intLast = bytBuffer.GetUpperBound(0)

        Dim sbdInput As New StringBuilder
        For intIndex As Integer = intFirst To intLast
            Dim bytSingle As Byte = bytBuffer(intIndex)
            If bytSingle <> 0 Then sbdInput.Append(Chr(bytSingle))
        Next
        Return sbdInput.ToString
    End Function  'GetString

    ' Function to extract summary of Info from byte array command or data
    Private Function GetInfoFromDataCMDSent(bytSent() As Byte) As String
        If bytSent.Length < 4 Then Return ""
        If bytSent(0) = &H64 Then ' Data frame
            Return "Data (length = " & (bytSent(2) * 256 + bytSent(3)).ToString & ")"
        ElseIf bytSent(0) = &H63 Then 'CMD frame
            ReDim Preserve bytSent(bytSent.Length - 3) ' remove the CRC
            Return GetString(bytSent)
        Else
            Logs.WriteDebug("[GetInfoFromDataCMDSent] bytSent Len = " & bytSent.Length & "   " & GetString(bytSent).Substring(0, 2))
            Return ""
        End If
    End Function  ' GetInfoFromDataCMDSent

    ' Subroutine to handle the Queue polling
    Private Sub tmrPollQueue_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrPollQueue.Elapsed

        Dim bytToSend() As Byte
        Static intWaitingForRDYRetries As Int32 = 0
        Static bytLastQueueDataCMDSent(-1) As Byte
        Static blnWaitingForRDY As Boolean = False
        Static dttStartWaitForReady As Date = Now

        tmrPollQueue.Stop()
        If blnInitializing Or blnTerminatingConnection Then Exit Sub
        If blnWaitingForRDY And blnHostRDY Then
            If MCB.CommandTrace Then Logs.WriteDebug(" Queued CMD/Data Acknowledged: " & GetInfoFromDataCMDSent(bytLastQueueDataCMDSent))
            blnWaitingForRDY = False
            intWaitingForRDYRetries = 0
            If (queDataForHost.Count > 0) Then
                bytToSend = queDataForHost.Dequeue
            Else
                tmrPollQueue.Start()
                Exit Sub
            End If
        ElseIf blnWaitingForRDY And (Now.Subtract(dttStartWaitForReady).TotalMilliseconds > 2000) Then
            If intWaitingForRDYRetries > 0 Then
                Logs.Exception("[HostInterface.tmrPollQueue] 2 sec timeout waiting for RDY, Retry Count= " & intWaitingForRDYRetries.ToString & " Frame: " & GetInfoFromDataCMDSent(bytLastQueueDataCMDSent))
                If MCB.CommandTrace Then Logs.WriteDebug(" 2 sec timeout waiting for RDY, Retry Count= " & intWaitingForRDYRetries.ToString & " Frame: " & GetInfoFromDataCMDSent(bytLastQueueDataCMDSent))
            End If
            intWaitingForRDYRetries += 1
            If intWaitingForRDYRetries < 3 Then
                'If MCB.CommandTrace Then Logs.WriteDebug(" Repeat last CMD/Data: " & GetInfoFromDataCMDSent(bytLastQueueDataCMDSent))
                'Logs.Exception("[HostInterface.tmrPollQueueElapsed] Timeout ,  Retry " & GetInfoFromDataCMDSent(bytLastQueueDataCMDSent) & " FROM TNC, Retry cnt=" & intWaitingForRDYRetries.ToString)
                bytToSend = bytLastQueueDataCMDSent
            Else
                If MCB.CommandTrace Then Logs.WriteDebug(" Failure after 3 retries of last CMD/Data: " & GetInfoFromDataCMDSent(bytLastQueueDataCMDSent))
                Logs.Exception("[HostInterface.tmrPollQueueElapsed] Failure retrying: " & GetInfoFromDataCMDSent(bytLastQueueDataCMDSent))
                intWaitingForRDYRetries = 0
                tmrPollQueue.Start()
                Exit Sub
            End If
        ElseIf (queDataForHost.Count > 0) Then
            bytToSend = queDataForHost.Dequeue
        Else
            tmrPollQueue.Start()
            Exit Sub
        End If

        If strInterfaceType = "TCPIP" Then
            Try
                If Not IsNothing(objTCPIP) Then ' handles case where host is shutting down the TCP IP interface
                    objTCPIP.Send(strTCPIPConnectionID, bytToSend)
                    bytLastQueueDataCMDSent = bytToSend ' capture in case CRC error or timeout requires repeat
                    If MCB.CommandTrace Then Logs.WriteDebug(" Queued Data/Cmd TO Host: " & GetInfoFromDataCMDSent(bytToSend))
                    dttStartWaitForReady = Now
                    blnHostRDY = False
                    blnWaitingForRDY = True
                End If
            Catch ex As Exception
                Logs.Exception("[HostInterface.tmrPollQueue] TCPIP Interface Exception: " & ex.ToString)
            End Try
        ElseIf strInterfaceType = "SERIAL" Then ' Needs to be reworked in light of above

        ElseIf strInterfaceType = "BLUETOOTH" Then
            ' This will handle BlueTooth connections ... TODO: Add BlueTooth
        End If
        tmrPollQueue.Start()
    End Sub  'tmrPollQueue_Elapsed

#End Region


    Private Sub objSerial_DataReceived(sender As Object, e As System.IO.Ports.SerialDataReceivedEventArgs) Handles objSerial.DataReceived
        ' This creates a first-in first-out buffer and pointers to handle receiving commands or data.
        ' Data may be received on NON CMD or Data frame boundaries. (intended to handle buffer and latency issues)

        Static strCommandFromHost As String = ""
        Static bytDataFromHost() As Byte = Nothing
        Static intDataBytesToReceive As Int32 = 0
        Static blnReceivingCMD As Boolean = False
        Static blnReceivingData As Boolean = False
        Static intDataBytePtr As Int32 = 0
        Static intCMDStartPtr As Int32 = 0
        Static intDataStartPtr As Int32 = 0
        Dim bytInBuffer(objSerial.ReadBufferSize - 1) As Byte

        objSerial.Read(bytInBuffer, 0, bytInBuffer.Length)
        AppendDataToBuffer(bytInBuffer, bytHostIBData_CmdBuffer)

        ' look for start of Command ("C:")  or Data (D:") and establish start pointer (Capital C or D indicates from Host)
SearchForStart:
        If Not (blnReceivingCMD Or blnReceivingData) Then
            For i As Integer = intHostIBData_CmdPtr To bytHostIBData_CmdBuffer.Length - 2
                If bytHostIBData_CmdBuffer(i) = &H43 And bytHostIBData_CmdBuffer(i + 1) = &H3A Then ' search for ASCII "C:"
                    ' start of command.
                    intCMDStartPtr = i
                    blnReceivingCMD = True
                    blnProcessingCmdData = True
                    blnReceivingData = False
                    Exit For
                ElseIf bytHostIBData_CmdBuffer(i) = &H44 And bytHostIBData_CmdBuffer(i + 1) = &H3A Then ' search for ASCII "D:"
                    ' start of Data
                    intDataStartPtr = i
                    blnReceivingCMD = False
                    blnReceivingData = True
                    blnProcessingCmdData = True
                    intDataBytesToReceive = 0
                    Exit For
                End If
            Next i
        End If

        If blnReceivingCMD Then
            For i As Integer = intCMDStartPtr To bytHostIBData_CmdBuffer.Length - 3 ' Look for <Cr> with room for 2 byte CRC
                If bytHostIBData_CmdBuffer(i) = &HD Then ' search for Carriage Return which signals the end of a Command (note 2 CRC bytes to follow)
                    Dim bytCmd(i - intCMDStartPtr) As Byte ' 2 bytes added for CRC, and "C:" skipped
                    Array.Copy(bytHostIBData_CmdBuffer, intCMDStartPtr + 2, bytCmd, 0, bytCmd.Length) 'copy over the Command (less :C:") and the 2 byte CRC
                    If CheckCRC16(bytCmd, &HFFFF) Then ' check the CRC
                        'CRC OK:
                        ReDim Preserve bytCmd(bytCmd.Length - 3) ' Drop off the CRC
                        strCommandFromHost = GetString(bytCmd).ToUpper.Trim
                        If MCB.CommandTrace Then Logs.WriteDebug("[obCommand Trace FROM host: C:" & strCommandFromHost)
                        ' Process the received and CRC checked command here:
                        If strCommandFromHost = "CRCFAULT" Then
                            If intCRCRetries > 2 Then
                                Logs.Exception("[HostInterface.objSerial_OnDataReceived] 3 CRC Faults on last CMD/Data " & bytLastCMD_DataSent.Length.ToString & " bytes ...aborting transfer")
                                intCRCRetries = 0
                                'blnWaitingForRDY = False
                                ReDim bytLastCMD_DataSent(-1)
                            Else
                                objTCPIP.Send(strTCPIPConnectionID, bytLastCMD_DataSent)
                                If (Not IsNothing(objSerial)) AndAlso objSerial.IsOpen Then
                                    Try
                                        objSerial.Write(bytLastCMD_DataSent, 0, bytLastCMD_DataSent.Length)
                                    Catch ex As Exception
                                        Logs.Exception("[HostInterface.objSerial.OnDataReceived] Serial Port Interface Exception: " & ex.ToString)
                                    End Try
                                End If
                                If MCB.CommandTrace Then Logs.WriteDebug(" Repeat last CMD/Data " & bytLastCMD_DataSent.Length.ToString & " bytes")
                                Logs.Exception("[HostInterface.objSerial_OnDataReceived] CRCFAULT received,  Retry last CMD_DataSent FROM host")
                                intCRCRetries += 1
                            End If
                            blnHostRDY = True
                        ElseIf strCommandFromHost <> "RDY" Then 'host can receive commands or data
                            ProcessCommandFromHost(strCommandFromHost) '(sends reply or Fault + RDY)
                        ElseIf strCommandFromHost = "RDY" Then
                            'blnWaitingForRDY = False
                            blnHostRDY = True
                        End If
                    Else
                        SendCommandToHost("CRCFAULT") ' indicates to Host to repeat the command
                        ' SendCommandToHost("RDY")
                    End If
                    ' resize buffer and reset pointer
                    blnProcessingCmdData = False
                    blnReceivingCMD = False
                    ' resize the bufffer, and set pointer to it's start.
                    Dim bytTemp(bytHostIBData_CmdBuffer.Length - i - 4) As Byte 'skip past the 2 byte CRC
                    If bytTemp.Length > 0 Then Array.Copy(bytHostIBData_CmdBuffer, i + 3, bytTemp, 0, bytTemp.Length)
                    bytHostIBData_CmdBuffer = bytTemp
                    intHostIBData_CmdPtr = 0
                    If bytHostIBData_CmdBuffer.Length > 0 Then GoTo SearchForStart
                End If
            Next i
        End If

        If blnReceivingData Then
            If intDataBytesToReceive = 0 Then ' Data length must always be >0 for a legitimate data frame:
                If bytHostIBData_CmdBuffer.Length - intDataStartPtr >= 4 Then
                    ' Compute the byte count to receive plus 2 additional bytes for the 16 bit CRC
                    intDataBytesToReceive = CInt(bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 2)) << 8
                    intDataBytesToReceive += (CInt(bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 3)) + 2) ' includes  data + 2 byte CRC
                    ReDim bytDataFromHost(intDataBytesToReceive + 1) ' make 2 larger to include the byte count (CRC computed starting with the byte Count)
                    bytDataFromHost(0) = bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 2) ' MSB of count
                    bytDataFromHost(1) = bytHostIBData_CmdBuffer(intHostIBData_CmdPtr + 3) ' LSB of count
                    intDataBytePtr = 2
                    intHostIBData_CmdPtr = intHostIBData_CmdPtr + 4 ' advance pointer past "D:" and byte count
                End If
            End If
            If intDataBytesToReceive > 0 And (intHostIBData_CmdPtr < bytHostIBData_CmdBuffer.Length) Then
                For i As Integer = 0 To bytHostIBData_CmdBuffer.Length - intHostIBData_CmdPtr - 1
                    bytDataFromHost(intDataBytePtr) = bytHostIBData_CmdBuffer(intHostIBData_CmdPtr)
                    intDataBytePtr += 1
                    intHostIBData_CmdPtr += 1
                    intDataBytesToReceive -= 1
                    If intDataBytesToReceive = 0 Then Exit For
                Next i
                If intDataBytesToReceive = 0 Then
                    ' Process bytDataFromHost here (check CRC etc) 
                    If CheckCRC16(bytDataFromHost) Then
                        If MCB.CommandTrace Then Logs.WriteDebug(" Data Trace from Host: " & (bytDataFromHost.Length).ToString & " bytes. CRC OK")
                        Dim bytDataToAppend(bytDataFromHost.Length - 5) As Byte
                        Array.Copy(bytDataFromHost, 2, bytDataToAppend, 0, bytDataFromHost.Length - 4)
                        objMain.objProtocol.AddDataToDataToSend(bytDataToAppend) ' Append data here to inbound queue
                        blnProcessingCmdData = False
                        SendCommandToHost("RDY") ' This signals the host more data or commands can be accepted
                    Else
                        If MCB.CommandTrace Then Logs.WriteDebug(" Data Trace FROM Host: " & (bytDataFromHost.Length).ToString & " bytes. CRC Fail")
                        Logs.Exception("[HostInterface.objSerial.OnDataREceived] Data Trace from Host:" & (bytDataFromHost.Length).ToString & " bytes. CRC Fail")
                        SendCommandToHost("CRCFAULT")
                        blnProcessingCmdData = False
                    End If

                    blnReceivingData = False
                    ' resize the bufffer, and set pointer to it's start.
                    If intHostIBData_CmdPtr >= bytHostIBData_CmdBuffer.Length - 1 Then
                        ReDim bytHostIBData_CmdBuffer(-1) ' clear the buffer and zero the pointer
                        intHostIBData_CmdPtr = 0
                    Else  ' resize the bufffer, and set pointer to it's start.
                        Dim bytTemp(bytHostIBData_CmdBuffer.Length - intHostIBData_CmdPtr - 2) As Byte
                        Array.Copy(bytHostIBData_CmdBuffer, intHostIBData_CmdPtr, bytTemp, 0, bytTemp.Length)
                        bytHostIBData_CmdBuffer = bytTemp
                        intHostIBData_CmdPtr = 0
                    End If
                End If
            End If
        End If
    End Sub  ' objSerial_DataReceived

    ' Function to check for proper syntax of a 4, 6 or 8 character GS
    Private Function CheckGSSyntax(strGS As String) As Boolean
        If Not (strGS.Length = 4 Or strGS.Length = 6 Or strGS.Length = 8) Then Return False
        For i As Integer = 0 To strGS.Length - 1
            If i < 2 Then
                If ("ABCDEFGHIJKLMNOPQR".IndexOf(strGS.ToUpper.Substring(i, 1)) = -1) Then Return False
            ElseIf (i > 1 And i < 4) Then
                If ("0123456789".IndexOf(strGS.Substring(i, 1)) = -1) Then Return False
            ElseIf (i > 3 And i < 6) Then
                If ("ABCDEFGHIJKLMNOPQRSTUVWX".IndexOf(strGS.ToUpper.Substring(i, 1)) = -1) Then Return False
            Else
                If ("0123456789".IndexOf(strGS.Substring(i, 1)) = -1) Then Return False
            End If
        Next i
        Return True
    End Function  '  CheckGSSyntax
End Class
