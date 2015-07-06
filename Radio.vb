Imports System.IO
Imports System.IO.Ports
Imports System.Threading
Imports System.Text
Imports System.Timers

Public Class Radio
    ' Booleans...

    Private blnInitialized As Boolean
    Private blnLastPTTState As Boolean = False
    Private blnControlPortOpen As Boolean
    Private blnPttSerialPortOpen As Boolean
    Private blnUSBMod As Boolean = False ' used to track switching of internal sound card on Icom 7200, Kenwood TS-590S and similar radios
    Private blnUSBModInitialized As Boolean = False
    Private blnInPTTCAT As Boolean = False
    Private blnReadingIcomState As Boolean = False
    Public blnRequestingRadioFreq As Boolean = False
    Private blnEnableCATPTT As Boolean = False

    ' Bytes
    Private bytRadioFeatures As Byte

    ' Events...
    Public Event RadioCommand(ByVal bytCommand() As Byte)
    Public Event RadioResponse(ByVal strResponse As String)

    ' Integers...
    Private intHertz As Integer = 14100000
    Private intLastSelectedChannel As Integer = -1
    Public intLastReportedFreq As Integer = 0
    Private intRadioReportedFreq As Integer = 0

    ' Objects...
    Private WithEvents objControlPort As SerialPort
    Private WithEvents objPttSerialPort As SerialPort
    Private WithEvents tmrPTTWatchdog As Timers.Timer
    Private WithEvents tmrPollFreq As Timers.Timer
    Private WithEvents tmrPTTDblOff As Timers.Timer
    Private objMain As Main

    ' Strings...

    Private strDTRAntenna As String
    Private strRadioReply As String = ""
    Private strRadioPTTClass As String = ""


    Public strSupportedRadios As String = "none,Elecraft Radios,Elecraft K2,Elecraft K3,Flex radios," _
                                          & "Icom Amateur Radios,Icom 7000,Icom 7100,Icom 7200,Icom 7600,Icom 9100,Icom 746,Icom 746Pro,Icom Amateur Radios (Early),Icom HF Marine Radios," _
                                          & "Kenwood Amateur,Kenwood TS-2000,Kenwood TS-480HX,Kenwood TS-480SAT,Kenwood TS-870,Kenwood TS-570,Kenwood TS-590S,Kenwood Commercial," _
                                          & "Micom 3F,Ten-Tec Orion,Ten-Tec Omni-7,Ten-Tec Eagle,Ten-Tec Jupiter," _
                                          & "Yaesu FT-100,Yaesu FT-450,Yaesu FT-600,Yaesu FT-817,Yaesu FT-840,Yaesu FT-847,Yaesu FT-857,Yaesu FT-897,Yaesu FT-920,Yaesu FT-950,Yaesu FT-1000,Yaesu FT-2000"

    ' note strRadios()  is used to populate the cmbModel drop down list
    Private strRadios() As String = strSupportedRadios.Split(",")

    ' Bytes
    ' This is a coded byte array that should match the strSupportedRadios above.  One byte per string entry (radio type).  The code is as follows:
    '  Code assignments:
    '   1st bit (MSbit)  = Enable/disable internal Tuner checkbox.  Enable is bit is set.
    '   2nd Bit = Enable/Disable antenna selection combo box
    '   3rd bit = Enable/Disable Filter selecion combo box 
    '   4th bit = Enable/Disble Icom Address field.
    '   5th bit = Enable/Disable Internal Sound card check box
    '   6th bit = Enable/Disable USB Digital option
    '   7th bit = Enable/Disble FM option
    '   8th bit (LSbit) = Enable CAT PTT

    ' NOTE:  Additional work needed here:
    '   1) Correct/Add details to RadioOptions for each radio type
    '   2) Implement those options in control code for each radio class
    '   3) Flesh out radio list and develop a structure/template on how to add radios and drivers.

    ' Radio options last updated April 19, 2015
    Private bytRadioOptions() As Byte = {0, 1, 1, 1, 5, _
                                         &H10, &H10, &H3F, &H3D, &H3F, &H3F, &H10, &H10, &H10, 0, _
                                         5, 5, 5, 5, 5, 5, &HEF, 5, _
                                         1, 1, 1, 1, 1, _
                                         4, 5, 4, 4, 5, 5, 5, 5, 0, 5, 1, 5}
    ' Dates
    Private dttLastPTTOn As Date

    ' Micom
    Private intMicomRcvIndex As Integer = 0
    Const SizeofACK As Short = 6
    Const SizeofSSBRpt As Short = 7
    Const SizeofFrpt As Short = 10
    Const FrmTo As Byte = &H18
    Private bytMicomRcv(100) As Byte
    Private WithEvents MiTimer As System.Timers.Timer
    Private Timer2Popped As Boolean

    ' Property used to switch modulation audio multiplexer on Radios with built in sound card.
    Public Property USBMod As Boolean ' used to switch audio input multiplexer on radios like the ICom 7100, 7200, 7600, 9100, and  Kenwood TS-590
        Get
            Return blnUSBMod
        End Get
        Set(value As Boolean)
            If objControlPort IsNot Nothing AndAlso objControlPort.IsOpen Then
                Select Case RCB.Model
                    Case "Icom 7100"  ' verified 
                        ' Configure basic command data
                        Dim bytCommand(9) As Byte
                        bytCommand(0) = &HFE
                        bytCommand(1) = &HFE
                        bytCommand(2) = CByte("&H" & RCB.IcomAdd)
                        bytCommand(3) = &HF1
                        bytCommand(4) = &H1A          ' Set Command 1A
                        bytCommand(5) = &H5           ' Sub command 5 Mux control for Data1
                        bytCommand(6) = &H0    ' Set MOD input for USB Digital (Data) mode
                        If RCB.Mode = "USBD" Then
                            bytCommand(7) = &H91 ' Set mux when datamode is on 
                        Else
                            bytCommand(7) = &H90 ' Set mux when datamode is off
                        End If
                        If value And RCB.InternalSoundCard Then
                            bytCommand(8) = 3 ' select USB (Sound card) modulation
                        Else
                            bytCommand(8) = 1 ' Select Aux input modulation
                        End If
                        bytCommand(9) = &HFD
                        SendBinaryCommand(bytCommand, 0)
                    Case "Icom 7200"
                        ' Configure basic command data
                        Dim bytCommand(8) As Byte
                        bytCommand(0) = &HFE
                        bytCommand(1) = &HFE
                        bytCommand(2) = CByte("&H" & RCB.IcomAdd)
                        bytCommand(3) = &HF1
                        bytCommand(4) = &H1A          ' Set Command 1A
                        bytCommand(5) = &H3           ' Sub command 3
                        bytCommand(6) = &H24    ' Set MOD input for USB Digital (Data) mode' TODO
                        If value And RCB.InternalSoundCard Then
                            bytCommand(7) = 3 ' select USB (Sound card) modulation
                        Else
                            bytCommand(7) = 1 ' Select Aux input modulation
                        End If
                        bytCommand(8) = &HFD
                        SendBinaryCommand(bytCommand, 0)

                    Case "Icom 7600" ' not verified
                        ' Configure basic command data
                        Dim bytCommand(9) As Byte
                        bytCommand(0) = &HFE
                        bytCommand(1) = &HFE
                        bytCommand(2) = CByte("&H" & RCB.IcomAdd)
                        bytCommand(3) = &HF1
                        bytCommand(4) = &H1A          ' Set Command 1A
                        bytCommand(5) = &H5           ' Sub command 5 Mux control for Data1
                        bytCommand(6) = &H0    ' Set MOD input for USB Digital (Data) mode
                        bytCommand(7) = &H31
                        If value And RCB.InternalSoundCard Then
                            bytCommand(8) = 3 ' select USB (Sound card) modulation
                        Else
                            bytCommand(8) = 1 ' Select Aux input modulation
                        End If
                        bytCommand(9) = &HFD
                        SendBinaryCommand(bytCommand, 0)

                    Case "Icom 9100" ' not verified
                        ' Configure basic command data
                        Dim bytCommand(9) As Byte
                        bytCommand(0) = &HFE
                        bytCommand(1) = &HFE
                        bytCommand(2) = CByte("&H" & RCB.IcomAdd)
                        bytCommand(3) = &HF1
                        bytCommand(4) = &H1A          ' Set Command 1A
                        bytCommand(5) = &H5           ' Sub command 5 Mux control for Data1
                        bytCommand(6) = &H0    ' Set MOD input for USB Digital (Data) mode
                        bytCommand(7) = &H57
                        If value And RCB.InternalSoundCard Then
                            bytCommand(8) = 3 ' select USB (Sound card) modulation
                        Else
                            bytCommand(8) = 1 ' Select Aux input modulation
                        End If
                        bytCommand(9) = &HFD
                        SendBinaryCommand(bytCommand, 0)

                    Case "Kenwood TS-590S"
                        If value And RCB.InternalSoundCard Then
                            SendASCIICommand("EX06300001;")  ' select USB for Audio Mux 
                        Else
                            SendASCIICommand("EX06300000;")  ' select ACC2 for Audio Mux
                        End If
                End Select
            End If
            blnUSBMod = value
            blnUSBModInitialized = True
        End Set
    End Property ' USBMod

    ' Subroutine to Close (actually hide) the radio form
    Private Sub btnClose_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnClose.Click
        Me.Hide()
    End Sub ' btnClose_Click

    ' Subroutine to handle btnUpdate 
    Private Sub btnUpdate_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btnUpdate.Click
        If chkRadioCtrl.Checked Then
            If cmbPTTCtrl.Text.StartsWith("COM") And cmbPTTCtrl.Text = cmbControlPort.Text Then
                If (chkControlDTR.Checked And chkKeyDTR.Checked) Or (chkControlRTS.Checked And chkKeyRTS.Checked) Then
                    MsgBox(" DTR or RTS cannot be shared for both radio control and PTT on the same COM port", MsgBoxStyle.Information, "DTR or RTS Illegal sharing")
                    Return
                End If
            End If
            If txtIcomAddress.Text.Trim.Length <> 2 Then
                MsgBox("Icom address must be Hex value 00-FF", MsgBoxStyle.Information, "Illegal Radio Address")
                Return
            End If
            If "0123456789ABCDEF".IndexOf((txtIcomAddress.Text.Trim.ToUpper).Substring(0, 1)) = -1 Or _
               "0123456789ABCDEF".IndexOf((txtIcomAddress.Text.Trim.ToUpper).Substring(1, 1)) = -1 Then
                MsgBox("Icom address must be Hex value 00-FF", MsgBoxStyle.Information, "Illegal Radio Address")
                Return
            End If

            If (chkInternalSC.Enabled) And Not chkInternalSC.Checked Then
                If MsgBox("Using the " & cmbModel.Text & "'s internal sound card is the preferred method for ARDOP!" & vbCr & "Continue with update?", MsgBoxStyle.YesNo) = MsgBoxResult.No Then Return
            End If
            RefreshPropertyValues()
            WriteRadioSettings()
            Me.DialogResult = Windows.Forms.DialogResult.OK
        Else
            RCB.RadioControl = chkRadioCtrl.Checked
            objIniFile.WriteString("Radio", "Enable Radio Control", chkRadioCtrl.Checked.ToString)
            objIniFile.Flush()
        End If
        Me.Hide()
    End Sub  '  btnUpdate_Click

    '  Subroutine to close the Radio class
    Public Sub CloseRadio()
        If blnLastPTTState Then PTT(False) ' insurance to insure radio is unkeyed on shutting down Radio class. 
        Try
            If RCB.Model = "Icom HF Marine Radios" Then
                SendNMEACommand(ComputeNMEACommand("REMOTE,OFF"))
                Dim dttDelay As Date = Now.AddMilliseconds(1000)
                Do While dttDelay > Now
                    Application.DoEvents()
                Loop
            End If
        Catch
            Logs.Exception("[Radio.CloseRadio] Icom Shutdown Err: " & Err.Description)
        End Try
        Try
            If objControlPort IsNot Nothing Then
                objControlPort.Close()
                objControlPort.Dispose()
                objControlPort = Nothing
            End If
            If objPttSerialPort IsNot Nothing Then
                objPttSerialPort.Close()
                objPttSerialPort.Dispose()
                objPttSerialPort = Nothing
            End If
        Catch
            Logs.Exception("[Radio.CloseRadio] Serial Port Shutdown Err: " & Err.Description)
        End Try
        Me.Close()
    End Sub  '   CloseRadio

    ' Subroutine to handle change of Combo Control Port 
    Private Sub cmbControlPort_TextChanged(ByVal sender As Object, ByVal e As System.EventArgs) Handles cmbControlPort.TextChanged
        Dim strPorts() As String = SerialPort.GetPortNames

        If cmbControlPort.Text = "None" Or cmbControlPort.Text = "" Then
            cmbControlBaud.Enabled = False
            chkControlDTR.Enabled = False
            chkControlRTS.Enabled = False
        Else
            cmbControlBaud.Enabled = True
            chkControlDTR.Enabled = True
            chkControlRTS.Enabled = True
        End If
    End Sub 'cmbControlPort_TextChanged

    ' Subroutine to handle Radio Model change
    Private Sub cmbModel_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cmbModel.TextChanged

        Dim intIndex As Integer
        Dim bytCode As Byte = 0
        Dim strModel As String
        For i As Integer = 0 To strRadios.Length - 1
            If strRadios(i) = cmbModel.Text Then
                intIndex = i
                bytCode = bytRadioOptions(i) And &H1F '  Temporarily mask off Tuner, filter, and antenna swithchin options for all models.
                strModel = strRadios(i)
                bytRadioFeatures = bytRadioOptions(i) ' used for tuner, filter, antenna control 
                Exit For
            End If
        Next
        ' bytCode bit assignments:
        '   1st bit (MSBit)  = Enable/disable internal Tuner checkbox.  Enable is bit is set.
        '   2nd Bit = Enable/Disable antenna selection combo box
        '   3rd bit = Enable/Disable Filter selecion combo box 
        '   4th bit = Enable/Disble Icom Address field.
        '   5th bit = Enable/Disable Internal Sound card check box
        '   6th bit = Enable/Disable USB Digital option
        '   7th bit = Enable/Disble FM option
        '   8th bit = Enable/Disable CAT PTT control

        ' Enable Disable internal tuner
        chkInternalTuner.Enabled = ((bytCode And &H80) <> 0)

        ' Enable Disable Antenna selection option 
        cmbAntenna.Enabled = ((bytCode And &H40) <> 0)
        lblAntenna.Enabled = ((bytCode And &H40) <> 0)

        ' Enable Disable Filter 
        cmbFilter.Enabled = ((bytCode And &H20) <> 0)
        lblFilter.Enabled = ((bytCode And &H20) <> 0)

        ' Enable Disable Icom address 
        lblIcomAddress.Enabled = ((bytCode And &H10) <> 0)
        txtIcomAddress.Enabled = ((bytCode And &H10) <> 0)

        ' Enable Disable Internal Sound card option
        chkInternalSC.Enabled = ((bytCode And &H8) <> 0)
        If Not chkInternalSC.Enabled Then chkInternalSC.Checked = False

        ' Enable Disable USB Digital radio button 
        rdoUSBDigital.Enabled = ((bytCode And &H4) <> 0)
        If Not rdoUSBDigital.Enabled Then rdoUSBDigital.Checked = False

        ' Enable Disable FM radio button 
        rdoFM.Enabled = ((bytCode And &H2) <> 0)
        If Not rdoFM.Enabled Then rdoFM.Checked = False

        blnEnableCATPTT = (bytCode And &H1) <> 0
        InitializePTTCtrlcmb()
        grpRadioControlPort.Enabled = cmbModel.Text <> "none"
    End Sub  '  cmbModel_TextChanged

    '  Function to compute NMEA Command 
    Private Function ComputeNMEACommand(ByVal strCommand As String) As String
        Dim strBuffer As String
        Dim strCheckSum As String
        Dim intCheckSum As Integer = 0
        Dim intIndex As Integer

        strBuffer = "$PICOA,90," & RCB.IcomAdd & "," & strCommand
        For intIndex = 1 To strBuffer.Length - 1
            intCheckSum = intCheckSum Xor Asc(strBuffer.Substring(intIndex, 1))
        Next intIndex
        strCheckSum = "*" & Strings.Right("0" & Hex$(intCheckSum), 2)
        ComputeNMEACommand = strBuffer & strCheckSum & vbCrLf
    End Function ' ComputeNMEACommand

    '  Function to Concatenate Byte Arrays
    Private Function ConcatByteArrays(ByVal bytFirst() As Byte, ByVal bytSecond() As Byte) As Byte()
        Dim aryResult As New ArrayList
        Dim bytSingle As Byte
        With aryResult
            For Each bytSingle In bytFirst
                .Add(bytSingle)
            Next
            For Each bytSingle In bytSecond
                .Add(bytSingle)
            Next
            Dim bytResult(bytFirst.Length + bytSecond.Length) As Byte
            aryResult.CopyTo(bytResult)
            Return bytResult
        End With
    End Function ' ConcatByteArrays

    '  Function to convert Text string to Bytes
    Private Function GetBytes(ByVal strText As String) As Byte()
        ' Converts a text string to a byte array...
        Dim bytBuffer(strText.Length - 1) As Byte
        For intIndex As Integer = 0 To bytBuffer.Length - 1
            bytBuffer(intIndex) = CByte(Asc(strText.Substring(intIndex, 1)))
        Next
        Return bytBuffer
    End Function 'GetBytes

    '  Subroutine to request frequency from the Radio 
    Public Sub GetFrequency()

        If IsNothing(objControlPort) Then Exit Sub
        If Not objControlPort.IsOpen Then Exit Sub
        tmrPollFreq.Stop()
        blnRequestingRadioFreq = True
        '  Send a frequency request command
        strRadioReply = "" ' initialize radio response parameters
        intRadioReportedFreq = 0

        ' Icom radios 
        If RCB.Model.StartsWith("Icom") Then
            Dim bytCommand() As Byte = {&HFE, &HFE, &H0, &HE0, &H3, &HFD} ' Read frequency command
            bytCommand(2) = CByte("&H" & RCB.IcomAdd)
            blnReadingIcomState = True
            SendBinaryCommand(bytCommand, 0)

            'Kenwood Radios ' Confirmed for TS-590S,
        ElseIf (RCB.Model.StartsWith("Kenwood") Or RCB.Model.StartsWith("Flex")) Then
            SendASCIICommand("FA;")   ' Read VFO A

            ' Yaesu and Elcraft radios 
        ElseIf RCB.Model.StartsWith("Elecraft") Or RCB.Model = "Yaesu FT-450" Or RCB.Model = "Yaesu FT-2000" Or RCB.Model = "Yaesu FT-950" Then
            SendASCIICommand("FA;")   ' Read VFO A
        ElseIf RCB.Model = "Yaesu FT-897" Or RCB.Model = "Yaesu FT-817" Or RCB.Model = "Yaesu FT-847" Or RCB.Model = "Yaesu FT-857" Then
            Dim bytCommand() As Byte = {0, 0, 0, 0, 3}
            SendBinaryCommand(bytCommand, 0)
        End If
        tmrPollFreq.Start()
        blnRequestingRadioFreq = False
    End Sub ' GetFrequency

    ' Subroutine to get radio settings from teh ini File
    Public Sub GetRadioSettings()
        Dim strSection As String = "Radio"

        RCB.CtrlPortDTR = CBool(objIniFile.GetString(strSection, "Control DTR", "True"))
        RCB.CtrlPortRTS = CBool(objIniFile.GetString(strSection, "Control RTS", "True"))
        RCB.Mode = objIniFile.GetString(strSection, "Mode", "USB")
        RCB.InternalTuner = CBool(objIniFile.GetString(strSection, "Tuner", "False"))
        RCB.Model = objIniFile.GetString(strSection, "Model", "none")
        RCB.CtrlPortBaud = objIniFile.GetInteger(strSection, "Control Baud", 9600)
        RCB.IcomAdd = objIniFile.GetString(strSection, "Icom Address", "00").Trim.ToUpper
        RCB.CtrlPort = objIniFile.GetString(strSection, "Control Port", "None")
        RCB.Filter = objIniFile.GetInteger(strSection, "Channel Filter Control", 0)
        RCB.InternalSoundCard = CBool(objIniFile.GetString(strSection, "Use Internal SC", "False"))
        RCB.PTTDTR = CBool(objIniFile.GetString(strSection, "KeyPTT DTR", "False"))
        RCB.PTTRTS = CBool(objIniFile.GetString(strSection, "KeyPTT RTS", "False"))
        RCB.PTTPort = objIniFile.GetString(strSection, "PTT Control Port", "VOX/SignaLink")
        RCB.Ant = objIniFile.GetInteger(strSection, "Antenna Select", 0)
    End Sub ' GetRadioSettings

    ' Subroutine to Initialize the Radio form display
    Private Sub InitializeDisplay()
        GetRadioSettings()
        ' Initialized the radio model drop down...
        cmbModel.Items.Clear()

        For j As Integer = 0 To strRadios.Length - 1
            cmbModel.Items.Add(strRadios(j))
        Next
        If RCB.Model <> "None" Then
            cmbModel.Text = RCB.Model
        End If
        ' Initializes the serial control port drop down...
        cmbControlPort.Items.Clear()
        cmbControlPort.Items.Add("None")

        Dim strPorts() As String = SerialPort.GetPortNames
        If strPorts.Length > 0 Then
            For Each strPort As String In strPorts
                cmbControlPort.Items.Add(strPort)
            Next
            If RCB.CtrlPort = "" Then
                cmbControlPort.Text = cmbControlPort.Items(0).ToString
            Else
                cmbControlPort.Text = RCB.CtrlPort
            End If
        End If
        InitializePTTCtrlcmb()
        ShowPropertyValues()
        RefreshPropertyValues()
    End Sub  '  InitializeDisplay

    ' Subroutine called upon a new instance of the Radio class
    Public Sub New(objRef As Main)
        objMain = objRef
        InitializeComponent()
        InitializeDisplay()
        tmrPTTWatchdog = New Timers.Timer
        tmrPTTWatchdog.AutoReset = False
        tmrPTTWatchdog.Interval = 8000 ' 8 seconds ...should cover all frame types. 
        tmrPollFreq = New Timers.Timer
        tmrPollFreq.AutoReset = False
        tmrPollFreq.Interval = 2000 '  2 sec
        tmrPTTDblOff = New Timers.Timer
        tmrPTTDblOff.AutoReset = False
        tmrPTTDblOff.Interval = 200 ' 200 ms
    End Sub  '  New


    ' Subroutine to initializes the cmbPTTCtrl port drop down to accomodate radios that may/may not allow CAT PTT control.
    Private Sub InitializePTTCtrlcmb()
        ' note blnEnableCATPTT gets set anytime there is a change of model in the cmbModel drop down text. 
        cmbPTTCtrl.Items.Clear()
        If blnEnableCATPTT Then cmbPTTCtrl.Items.Add("CAT PTT") ' only shows CAT PTT in the drop down if enabled for this radio
        cmbPTTCtrl.Items.Add("VOX/SignaLink")

        Dim strPorts() As String = SerialPort.GetPortNames
        If strPorts.Length > 0 Then
            For Each strPort As String In strPorts
                cmbPTTCtrl.Items.Add(strPort)
            Next
        End If
        If RCB.PTTPort = "" Then
            cmbPTTCtrl.Text = cmbPTTCtrl.Items(0)
        Else
            cmbPTTCtrl.Text = RCB.PTTPort
        End If
    End Sub  '  InitializePTTCtrlcmb

    ' Subroutine to handle Radio Response event 
    Private Sub OnRadioResponse(ByVal s As Object, ByVal e As SerialDataReceivedEventArgs) Handles objControlPort.DataReceived

        '  Handle data received from the radio control port.
        Dim intBytesToRead As Integer
        Dim strRead As String = ""
        Dim strIcomInput As String = ""
        Dim strReply() As String
        Static strFreq As String = ""

        ' Icom radios
        If RCB.Model = "Icom 7100" Or RCB.Model = "Icom 7200" Or RCB.Model = "Icom 7600" Or RCB.Model = "Icom 9100" Or _
            RCB.Model = "Icom Amateur Radios" Or RCB.Model = "Icom Amateur Radios (Early)" Then
            intBytesToRead = objControlPort.BytesToRead
            For i As Integer = 1 To intBytesToRead
                strRead = Hex(objControlPort.ReadByte).ToUpper
                If strRead = "FD" Then
                    strIcomInput = strRadioReply & strRead
                    strReply = strIcomInput.Split(","c)
                    If blnReadingIcomState And strReply.Length > 6 Then
                        If ((strReply.Length = 10) Or (strReply.Length = 11)) AndAlso strReply(4) = "3" Then
                            ' Read frequency
                            blnReadingIcomState = False
                            strRadioReply = ParseIcomFreq(strReply)
                            If IsNumeric(strRadioReply) Then
                                objControlPort.DiscardInBuffer()
                                intRadioReportedFreq = CInt(strRadioReply)
                                PostNewFrequency(intRadioReportedFreq)
                            End If
                        End If
                    End If
                    strRadioReply = ""
                Else
                    strRadioReply &= strRead & ","
                End If
            Next i

            ' Kenwood, Elecraft and Yaesu 450, 950 and 2000
        ElseIf RCB.Model.StartsWith("Kenwood") Or RCB.Model.StartsWith("Elecraft") Or RCB.Model = "Yaesu FT-450" Or _
            RCB.Model = "Yaesu FT-950" Or RCB.Model = "Yaesu FT-2000" Then
            intBytesToRead = objControlPort.BytesToRead
            '  reported inconsistities with this by Jouko on Kenwood TS-48. Fixed with requirement for 11 return bytes below. May 5, 2015

            For i As Integer = 1 To intBytesToRead
                strRead = Chr(objControlPort.ReadByte)
                If strRead = "A" Then
                    strFreq = ""
                ElseIf IsNumeric(strRead) Then
                    strFreq &= strRead
                ElseIf strRead = ";" Then
                    objControlPort.DiscardInBuffer()
                    If (IsNumeric(strFreq) And (strFreq.Length = 11)) Then
                        intRadioReportedFreq = CInt(strFreq)
                        PostNewFrequency(intRadioReportedFreq)
                    Else
                        strFreq = ""
                    End If
                    Exit For
                End If
            Next i

            'Yaesu 817. 847. 857, 897
        ElseIf RCB.Model = "Yaesu FT-817" Or RCB.Model = "Yaesu FT-847" Or RCB.Model = "Yaesu FT-857" Or RCB.Model = "Yaesu FT-897" Then ' Not yet verified" Then
            intBytesToRead = objControlPort.BytesToRead
            If intBytesToRead >= 4 Then
                Dim bytReadback(3) As Byte
                Dim intFreq As Int32 = 0
                objControlPort.Read(bytReadback, 0, 4)
                For i As Integer = 0 To 3
                    intFreq = 10 * intFreq + ((bytReadback(i) And &HF0) >> 4)
                    intFreq = 10 * intFreq + (bytReadback(i) And &HF)
                Next
                objControlPort.DiscardInBuffer()
                intRadioReportedFreq = 10 * intFreq ' To return freq in integer Hz
                PostNewFrequency(intRadioReportedFreq)
            End If

            ' Micom
        ElseIf RCB.Model.StartsWith("Micom") Then
            ' Store bytes into Micom receive buffer.  intMicomRcvIndex has number of received bytes.
            intBytesToRead = objControlPort.BytesToRead
            For i As Integer = 1 To intBytesToRead
                If intMicomRcvIndex < bytMicomRcv.GetUpperBound(0) Then
                    bytMicomRcv(intMicomRcvIndex) = Convert.ToByte(objControlPort.ReadByte)
                    intMicomRcvIndex += 1
                End If
            Next i
        End If
    End Sub  'OnRadioResponse

    ' Function to parse reply of Icom for 8 0r 10 digit (4 or 5 byte) frequency readback
    Private Function ParseIcomFreq(strFreq() As String) As String
        If Not (strFreq.Length = 10 Or strFreq.Length = 11) Then Return ""
        If strFreq(4) <> 3 Then Return ""

        Dim strIcomFreq As String = ""
        Dim bytTemp(0) As Byte
        Dim blnSurpressingLeadingZeros As Boolean = True ' suppress leading 0's 
        For j As Integer = 0 To strFreq.Length - 7 ' for 4 or 5 bytes
            bytTemp(0) = 48 + ((CByte("&H" & strFreq(strFreq.Length - (2 + j))) And &HF0) >> 4)
            If Not (blnSurpressingLeadingZeros And (bytTemp(0) = 48)) Then
                strIcomFreq &= GetString(bytTemp)
                blnSurpressingLeadingZeros = False
            End If
            bytTemp(0) = 48 + (CByte("&H" & strFreq(strFreq.Length - (2 + j))) And &HF)
            If Not (blnSurpressingLeadingZeros And (bytTemp(0) = 48)) Then
                strIcomFreq &= GetString(bytTemp)
                blnSurpressingLeadingZeros = False
            End If
        Next j
        Return strIcomFreq
    End Function  ' ParseIcomFreq

    ' Subroutine to process ICOM control
    Private Sub ProcessIcom(ByVal intHertz As Integer)
        Dim bytCIVAddress As Byte
        Dim bytCommand(10) As Byte
        Dim strHertz As String
        Dim intPa1 As Integer, intPa2 As Integer, intPa3 As Integer, intPa4 As Integer, intPa5 As Integer

        Try
            bytCIVAddress = CByte("&H" & RCB.IcomAdd)
        Catch
            If MsgBox(RCB.IcomAdd & " is not a valid Icom address. Icom address is set to default of 00. Continue?", MsgBoxStyle.YesNo) = MsgBoxResult.Yes Then
                bytCIVAddress = 0
            Else
                Return
            End If
        End Try
        If intHertz < 1801500 Then Return
        Select Case RCB.Model
            Case "Icom Amateur Radios", "Icom 7000", "Icom 7100", "Icom 7200", "Icom 746", "Icom 746Pro", "Icom 7600", "Icom 9100"
                ' Encode HF/VHF frequency...
                Dim intDial As Integer
                intDial = intHertz
                strHertz = Format(intDial, "000000000")
                intPa1 = 16 * CInt(strHertz.Substring(7, 1)) + CInt(strHertz.Substring(8, 1))
                intPa2 = 16 * CInt(strHertz.Substring(5, 1)) + CInt(strHertz.Substring(6, 1))
                intPa3 = 16 * CInt(strHertz.Substring(3, 1)) + CInt(strHertz.Substring(4, 1))
                intPa4 = 16 * CInt(strHertz.Substring(1, 1)) + CInt(strHertz.Substring(2, 1))
                If intDial > 0 Then
                    intPa5 = CInt(strHertz.Substring(0, 1))
                Else
                    intPa5 = 0
                End If
                ' Select HF/VHF frequency...
                bytCommand(0) = &HFE
                bytCommand(1) = &HFE
                bytCommand(2) = bytCIVAddress
                bytCommand(3) = &HF1
                bytCommand(4) = &H5 ' Set frequency command
                bytCommand(5) = CByte(intPa1)
                bytCommand(6) = CByte(intPa2)
                bytCommand(7) = CByte(intPa3)
                bytCommand(8) = CByte(intPa4)
                bytCommand(9) = CByte(intPa5)
                bytCommand(10) = &HFD
                SendHybridCommand(bytCommand)
                If RCB.Mode <> "FM" Then
                    ' Select USB mode
                    ReDim bytCommand(6)
                    bytCommand(0) = &HFE
                    bytCommand(1) = &HFE
                    bytCommand(2) = bytCIVAddress
                    bytCommand(3) = &HF1
                    bytCommand(4) = &H6
                    bytCommand(5) = &H1 ' command to set USB mode
                    bytCommand(6) = &HFD
                    SendHybridCommand(bytCommand) ' set USB mode
                    If RCB.Model = "Icom 7200" Then
                        ReDim Preserve bytCommand(8)
                        bytCommand(4) = &H1A
                        bytCommand(5) = &H4
                        bytCommand(8) = &HFD
                        If RCB.Mode = "USB" Then
                            bytCommand(6) = &H0 ' command to exit data mode
                            bytCommand(7) = &H0 ' command to exit data mode
                        ElseIf RCB.Mode = "USBD" Then
                            bytCommand(6) = &H1 ' command to enter data mode
                            If RCB.Filter > 0 Then
                                'If blnWideChannel(intCurrentChannel) Then
                                '    bytCommand(7) = &H2 ' Enable middle filter
                                'Else
                                '    bytCommand(7) = &H3 ' Enable Narrow filter
                                'End If
                            End If
                        End If
                    Else
                        ReDim Preserve bytCommand(7)
                        bytCommand(4) = &H1A
                        bytCommand(5) = &H6
                        bytCommand(7) = &HFD
                        If RCB.Mode = "USB" Then
                            bytCommand(6) = &H0 ' command to exit data mode
                        ElseIf RCB.Mode = "USBD" Then
                            bytCommand(6) = &H1 ' command to enter data mode
                        End If
                    End If
                    SendHybridCommand(bytCommand) ' set or clear digital mode
                    ' End 1.1.5.1 modification
                Else
                    ' Select VHF FM mode (ICOM-7000 and similar)...
                    ReDim bytCommand(6)
                    bytCommand(0) = &HFE
                    bytCommand(1) = &HFE
                    bytCommand(2) = bytCIVAddress
                    bytCommand(3) = &HF1
                    bytCommand(4) = 6        ' Set mode
                    bytCommand(5) = 5        ' Set FM
                    bytCommand(6) = &HFD
                    SendHybridCommand(bytCommand)
                End If

            Case "Icom Amateur Radios (Early)"
                ' Encode HF frequency...
                strHertz = Format(intHertz, "00000000")
                intPa1 = 16 * CInt(strHertz.Substring(6, 1)) + CInt(strHertz.Substring(7, 1))
                intPa2 = 16 * CInt(strHertz.Substring(4, 1)) + CInt(strHertz.Substring(5, 1))
                intPa3 = 16 * CInt(strHertz.Substring(2, 1)) + CInt(strHertz.Substring(3, 1))
                intPa4 = 16 * CInt(strHertz.Substring(0, 1)) + CInt(strHertz.Substring(1, 1))

                ' Select HF frequency...
                ReDim bytCommand(9)
                bytCommand(0) = &HFE
                bytCommand(1) = &HFE
                bytCommand(2) = bytCIVAddress
                bytCommand(3) = &HF1
                bytCommand(4) = &H5 ' Set frequency command
                bytCommand(5) = CByte(intPa1)
                bytCommand(6) = CByte(intPa2)
                bytCommand(7) = CByte(intPa3)
                bytCommand(8) = CByte(intPa4)
                bytCommand(9) = &HFD
                SendHybridCommand(bytCommand)

                ' Select USB mode...
                ReDim bytCommand(6)
                bytCommand(0) = &HFE
                bytCommand(1) = &HFE
                bytCommand(2) = bytCIVAddress
                bytCommand(3) = &HF1
                bytCommand(4) = &H6  ' Mode command
                bytCommand(5) = &H1  ' USB (was missing in rev 0.5.7.0 which set to LSB)
                bytCommand(6) = &HFD
                SendHybridCommand(bytCommand)

            Case "Icom HF Marine Radios"
                ' Send radio commands in NMEA format...
                SendNMEACommand(ComputeNMEACommand("REMOTE,ON"))
                SendNMEACommand(ComputeNMEACommand("MODE,USB"))
                SendNMEACommand(ComputeNMEACommand("RXF," & Format(intHertz / 1000000, "#0.000000")))
                SendNMEACommand(ComputeNMEACommand("TXF," & Format(intHertz / 1000000, "#0.000000")))
        End Select

        ' antenna switching by radio
        Select Case RCB.Model
            Case "Icom 7000", "Icom 746", "Icom 746pro"
                ReDim bytCommand(6)
                bytCommand(0) = &HFE
                bytCommand(1) = &HFE
                bytCommand(2) = bytCIVAddress
                bytCommand(3) = &HF1
                bytCommand(4) = &H12  ' Mode command
                bytCommand(6) = &HFD
                'If strAntenna(intCurrentChannel) = "Int_1" Then
                '    bytCommand(5) = 0 ' Antenna 1
                '    SendHybridCommand(bytCommand)
                'ElseIf strAntenna(intCurrentChannel) = "Int_2" Then
                '    bytCommand(5) = 1 ' Antenna 2
                '    SendHybridCommand(bytCommand)
                'End If
            Case "Icom 7600", "Icom 9100" ' not yet verified
                ReDim bytCommand(7)
                bytCommand(0) = &HFE
                bytCommand(1) = &HFE
                bytCommand(2) = bytCIVAddress
                bytCommand(3) = &HF1
                bytCommand(4) = &H12  ' Mode command
                bytCommand(7) = &HFD
                'If strAntenna(intCurrentChannel) = "Int_1" Then
                '    bytCommand(5) = 0 ' Antenna 1
                '    bytCommand(6) = 1  ' Rx antenna ON ???
                '    SendHybridCommand(bytCommand)
                'ElseIf strAntenna(intCurrentChannel) = "Int_2" Then
                '    bytCommand(5) = 1 ' Antenna 2
                '    bytCommand(6) = 1  ' Rx antenna ON ???
                '    SendHybridCommand(bytCommand)
                'End If
        End Select

        ' Tuner Control
        'If strTuner(intCurrentChannel) <> "None" Then
        '    Select Case RCB.Model
        '        Case "Icom 7600", "Icom 9100" ' not yet verified

        '            ReDim bytCommand(7)
        '            bytCommand(0) = &HFE
        '            bytCommand(1) = &HFE
        '            bytCommand(2) = CByte("&H" & RCB.IcomAdd)
        '            bytCommand(3) = &HF1
        '            bytCommand(7) = &HFD
        '            bytCommand(4) = &H1C          ' Set Transceiver Ant Tuner command
        '            bytCommand(5) = 1
        '            If strTuner(intCurrentChannel) = "Thru" Then
        '                bytCommand(6) = 0
        '            ElseIf strTuner(intCurrentChannel) = "Tx" Then
        '                bytCommand(6) = 1
        '            ElseIf strTuner(intCurrentChannel) = "Tx&Rx" Then
        '                bytCommand(6) = 1
        '            End If
        '            SendHybridCommand(bytCommand)
        '    End Select
        'End If

        ' optional filter control 
        If RCB.Filter > 0 Then
            Select Case RCB.Model

                Case "Icom 7200"
                    ' Veriified on 1.0.2.3 with 7200
                    ReDim Preserve bytCommand(8)
                    bytCommand(0) = &HFE
                    bytCommand(1) = &HFE
                    bytCommand(2) = bytCIVAddress
                    bytCommand(3) = &HF1
                    bytCommand(4) = &H1A ' Mode command
                    bytCommand(5) = &H4   ' Set data mode)
                    bytCommand(6) = &H1   ' Set Filter when data mode on
                    bytCommand(8) = &HFD
                    'If blnWideChannel(intCurrentChannel) Then
                    '    bytCommand(7) = &H2  'Middle width  filter  width (nominal 2400) is preset by user using 7200 Filter menu
                    'Else
                    '    bytCommand(7) = &H3  ' Narrow filter Filter width (nominal 600) is preset by user using 7200 Filter menu
                    'End If
                    'SendHybridCommand(bytCommand)

                Case "Icom 7100", "Icom 7600", "Icom 9100" ' 
                    ReDim Preserve bytCommand(8)
                    bytCommand(0) = &HFE
                    bytCommand(1) = &HFE
                    bytCommand(2) = bytCIVAddress
                    bytCommand(3) = &HF1
                    bytCommand(4) = &H1A ' Mode command
                    bytCommand(5) = &H6   ' Set data mode)
                    bytCommand(6) = &H1   ' Set Filter when data mode on USB
                    bytCommand(8) = &HFD
                    'If blnWideChannel(intCurrentChannel) Then
                    '    bytCommand(7) = &H2  'Middle width  filter  width (nominal 2400) is preset by user using Icom Filter menu
                    'Else
                    '    bytCommand(7) = &H3  ' Narrow filter Filter width (nominal 600) is preset by user using Icom Filter menu
                    'End If
                    'SendHybridCommand(bytCommand)
            End Select
        End If
    End Sub  ' ProcessIcom

    ' Subroutine to process Kenwood control
    Private Sub ProcessKenwood(ByVal intHertz As Integer)
        Dim strHertz As String
        Static strLastMode As String = ""
        If intHertz < 1801500 Then Return


        Dim chrEOC As Char = ";"c
        If RCB.Model = "Kenwood Commercial" Then chrEOC = Chr(13) ' CR

        ' Set mode and freq non FM.  Confirmed for TS-590S, 
        If RCB.Mode <> "FM" Then
            strHertz = Format(intHertz, "00000000000")
            If RCB.Model = "Kenwood Commercial" Then SendASCIICommand("A00" & chrEOC) ' Turn off ALE on commercial versions
            'SendASCIICommand("BC0" & chrEOC)                ' Beat canceller off
            'SendASCIICommand("NB0" & chrEOC)                ' Noise blanker off
            'SendASCIICommand("NR0" & chrEOC)                ' Noise reducer off
            SendASCIICommand("FR0" & chrEOC)                ' Select VFO A receive
            SendASCIICommand("FT0" & chrEOC)                ' Select VFO A transmit
            SendASCIICommand("RC" & chrEOC)                 ' Clear RIT
            SendASCIICommand("FA" & strHertz & chrEOC)      ' Select frequency

            If RCB.Mode = "USB" And strLastMode <> "USB" Then
                SendASCIICommand("MD2" & chrEOC) ' Select USB
                If RCB.Model = "Kenwood TS-590S" Then SendASCIICommand("DA0" & chrEOC)
                strLastMode = "USB"
            ElseIf RCB.Mode = "USBD" And strLastMode <> "USBD" Then ' Select Data off
                strLastMode = "USBD"
                If RCB.Model = "Kenwood TS-590S" Then
                    SendASCIICommand("MD2" & chrEOC) ' Select USB
                    SendASCIICommand("DA1" & chrEOC) ' Select Data
                Else
                    SendASCIICommand("MD9" & chrEOC) ' Switch the Flex radios to Digtial mode
                End If
            End If


            ' antenna switching by radio
            If (bytRadioFeatures & &H40 <> 0) Then  ' If the antenna switch feature is present 
                'Select Case RCB.Model
                '    Case "Kenwood TS-2000", "Kenwood TS-480HX", "Kenwood TS-480SAT", "Kenwood TS-870", "Kenwood TS-570"
                '        Select Case RCB.Ant
                '            Case 1 : SendASCIICommand("AN1" & chrEOC)
                '            Case 2 : SendASCIICommand("AN2" & chrEOC)
                '        End Select
                '    Case "Kenwood TS-590S"
                '        Select Case RCB.Ant
                '            Case 1 : SendASCIICommand("AN199" & chrEOC)
                '            Case 2 : SendASCIICommand("AN299" & chrEOC)
                '        End Select
                'End Select
            End If

            '
        Else
            ' For FM mode
            strLastMode = "FM"
            strHertz = Format(intHertz, "00000000000")
            SendASCIICommand("MD4" & chrEOC)            ' Select FM
            SendASCIICommand("FR0" & chrEOC)            ' Select VFO A receive
            SendASCIICommand("FT0" & chrEOC)            ' Select VFO A transmit
            SendASCIICommand("FA" & strHertz & chrEOC)  ' Select frequency
        End If


        ' Tuner Control
        If (bytRadioFeatures & &H80 <> 0) Then  ' If the internal antenna tuner feature is present 
            'Select Case RCB.Model
            '    Case "Kenwood TS-2000", "Kenwood TS-480SAT", "Kenwood TS-870", "Kenwood TS-570", "Kenwood TS-590S"
            '        ' Read the current tuner status (this necessary as a set command may toggle settings!)
            '        If RCB.InternalTuner Then
            '            SendASCIICommand("AC110;") ' RX AT-In, TX AT-In
            '        Else
            '            SendASCIICommand("AC000;") ' RX and TX through
            '        End If
            'End Select
        End If

        ' Filter control  0 is no filter, 1 is narrow filter, 2 is wide filter
        'If (bytRadioFeatures & &H20 <> 0) Then  ' If Filter control feature is present 
        '        Select RCB.Model

        '        Case "Kenwood TS-480HX", "Kenwood TS-480SAT"
        '            If RCB.Filter = 1 Then SendASCIICommand("FW0001" & chrEOC)
        '            If RCB.Filter = 2 Then SendASCIICommand("FW0000" & chrEOC)
        '        Case "Kenwood TS-2000"
        '            If RCB.Filter = 1 Then
        '                SendASCIICommand("SL11" & chrEOC) ' 1800 Hz Hi cutoff
        '                SendASCIICommand("SL04" & chrEOC) '  1000 Hz low cutoff
        '            End If

        '            If RCB.Filter = 2 Then
        '                SendASCIICommand("SH07" & chrEOC) ' 2800 Hz Hi cutoff
        '                SendASCIICommand("SL04" & chrEOC) '  300 Hz low cutoff
        '            End If
        '        Case "Kenwood TS-570"
        '            If RCB.Filter = 1 Then SendASCIICommand("FW0000" & chrEOC)
        '            If RCB.Filter = 2 Then SendASCIICommand("FW0001" & chrEOC)
        '        Case "Kenwood TS-870"
        '            If RCB.Filter = 1 Then SendASCIICommand("FW0050" & chrEOC)
        '            If RCB.Filter = 2 Then SendASCIICommand("FW0270" & chrEOC)
        '        Case "Kenwood TS-590S"
        '            If RCB.Filter = 1 Then SendASCIICommand("FL1" & chrEOC)
        '            If RCB.Filter = 2 Then SendASCIICommand("FL2" & chrEOC)
        '    End Select
        'End If
    End Sub  '  ProcessKenwood

    ' Subroutine to process Yaesu control
    Private Sub ProcessYaesu(ByVal intHertz As Integer)
        If intHertz < 1801500 Then Return
        Dim strHertz As String
        Dim bytCommand() As Byte = {0, 0, 0, 0, 0}

        If RCB.Mode = "FM" Then Return ' FM not supported

        SendHybridCommand(bytCommand) ' CAT On  
        strHertz = Format(intHertz, "000000000")
        Select Case RCB.Model
            Case "Yaesu FT-450"
                If intHertz < 1801500 Then Return
                strHertz = Format(intHertz, "00000000")
                If RCB.Mode = "USBD" Then SendASCIICommand("MD0C;") ' Select User U ' This confirmed to be correct for FT-450
                If RCB.Mode = "USB" Then SendASCIICommand("MD02;") ' Select USB
                SendASCIICommand("FA" & strHertz & ";")   ' Select frequency
            Case "Yaesu FT-2000", "Yaesu FT-950"
                If intHertz < 1801500 Then Return
                strHertz = Format(intHertz, "00000000")
                If RCB.Mode = "USB" Then SendASCIICommand("MD02;") ' Select USB
                If RCB.Mode = "USBD" Then SendASCIICommand("MD0C;") ' Select USB/Data
                SendASCIICommand("FA" & strHertz & ";")   ' Select frequency
            Case "Yaesu FT-857"
                If RCB.Mode = "USB" Then bytCommand(0) = 1 ' Set USB...
                If RCB.Mode = "USBD" Then bytCommand(0) = &HA ' Set USB Digital...
                bytCommand(4) = 7
                SendHybridCommand(bytCommand)

                ' Turn off split for 857...
                bytCommand(0) = 0
                bytCommand(4) = &H82
                SendHybridCommand(bytCommand)

                ' Set frequency...
                bytCommand(0) = CByte(CInt(strHertz.Substring(0, 1)) * 16 + CInt(strHertz.Substring(1, 1)))
                bytCommand(1) = CByte(CInt(strHertz.Substring(2, 1)) * 16 + CInt(strHertz.Substring(3, 1)))
                bytCommand(2) = CByte(CInt(strHertz.Substring(4, 1)) * 16 + CInt(strHertz.Substring(5, 1)))
                bytCommand(3) = CByte(CInt(strHertz.Substring(6, 1)) * 16 + CInt(strHertz.Substring(7, 1)))
                bytCommand(4) = 1
                SendHybridCommand(bytCommand) ' Set the frequency 

            Case "Yaesu FT-897", "Yaesu FT-817", "Yaesu FT-847"
                ' Set USB...
                If RCB.Mode = "USB" Then bytCommand(0) = 1
                If RCB.Mode = "USBD" Then bytCommand(0) = &HA
                bytCommand(4) = 7
                SendHybridCommand(bytCommand)

                ' Set frequency...
                bytCommand(0) = CByte(CInt(strHertz.Substring(0, 1)) * 16 + CInt(strHertz.Substring(1, 1)))
                bytCommand(1) = CByte(CInt(strHertz.Substring(2, 1)) * 16 + CInt(strHertz.Substring(3, 1)))
                bytCommand(2) = CByte(CInt(strHertz.Substring(4, 1)) * 16 + CInt(strHertz.Substring(5, 1)))
                bytCommand(3) = CByte(CInt(strHertz.Substring(6, 1)) * 16 + CInt(strHertz.Substring(7, 1)))
                bytCommand(4) = 1
                SendHybridCommand(bytCommand) ' Set the frequency 

            Case "Yaesu FT-600", "Yaesu FT-840", "Yaesu FT-100", "Yaesu FT-890"
                bytCommand(0) = 0
                bytCommand(1) = 0
                bytCommand(2) = 0
                If RCB.Mode = "USB" Then bytCommand(3) = 1 ' USB 
                If RCB.Mode = "USBD" Then bytCommand(3) = &HA ' Data USB
                bytCommand(4) = &HC
                SendHybridCommand(bytCommand, 100)

                Logs.WriteDebug("[Radio.Process Yaesu] Yaesu 100,100,840,890 Send Mode command  for " & RCB.Mode)
                bytCommand(0) = CByte(CInt(strHertz.Substring(6, 1)) * 16 + CInt(strHertz.Substring(7, 1)))
                bytCommand(1) = CByte(CInt(strHertz.Substring(4, 1)) * 16 + CInt(strHertz.Substring(5, 1)))
                bytCommand(2) = CByte(CInt(strHertz.Substring(2, 1)) * 16 + CInt(strHertz.Substring(3, 1)))
                bytCommand(3) = CByte(CInt(strHertz.Substring(0, 1)) * 16 + CInt(strHertz.Substring(1, 1)))
                bytCommand(4) = &HA
                SendHybridCommand(bytCommand, 100)
                Logs.WriteDebug("[Radio.Process Yaesu] Yaesu 100,100,840,890 Send Frequency for for " & strHertz)

                '' Set frequency...
                'bytCommand(0) = CByte(CInt(strHertz.Substring(6, 1)) * 16 + CInt(strHertz.Substring(7, 1)))
                'bytCommand(1) = CByte(CInt(strHertz.Substring(4, 1)) * 16 + CInt(strHertz.Substring(5, 1)))
                'bytCommand(2) = CByte(CInt(strHertz.Substring(2, 1)) * 16 + CInt(strHertz.Substring(3, 1)))
                'bytCommand(3) = CByte(CInt(strHertz.Substring(0, 1)) * 16 + CInt(strHertz.Substring(1, 1)))
                'bytCommand(4) = &H8A
                'SendHybridCommand(bytCommand)

            Case Else
                ' Set USB...
                bytCommand(0) = 0
                bytCommand(1) = 0
                bytCommand(2) = 0
                If RCB.Mode = "USB" Then bytCommand(3) = 1 ' USB 
                If RCB.Mode = "USBD" Then bytCommand(3) = &HA ' Data USB
                bytCommand(4) = &HC
                SendHybridCommand(bytCommand, 100)

                bytCommand(0) = CByte(CInt(strHertz.Substring(6, 1)) * 16 + CInt(strHertz.Substring(7, 1)))
                bytCommand(1) = CByte(CInt(strHertz.Substring(4, 1)) * 16 + CInt(strHertz.Substring(5, 1)))
                bytCommand(2) = CByte(CInt(strHertz.Substring(2, 1)) * 16 + CInt(strHertz.Substring(3, 1)))
                bytCommand(3) = CByte(CInt(strHertz.Substring(0, 1)) * 16 + CInt(strHertz.Substring(1, 1)))
                bytCommand(4) = 10
                SendHybridCommand(bytCommand, 100)
        End Select

        ' Set optional filter (code lifted from RMS Pactor...not yet verified) 
        'If blnFilterControl Then
        '    Dim strFilterCommand As String = ""
        '    Select Case RCB.Model
        '        Case "Yaesu FT-1000"
        '            If blnWideChannel(intCurrentChannel) Then
        '                strFilterCommand = Chr(0) + Chr(0) + Chr(0) + Chr(84) + Chr(140)
        '            Else
        '                strFilterCommand = Chr(0) + Chr(0) + Chr(0) + Chr(82) + Chr(140)
        '            End If
        '        Case "Yaesu FT-920"
        '            If blnWideChannel(intCurrentChannel) Then
        '                strFilterCommand = Chr(0) + Chr(0) + Chr(0) + Chr(0) + Chr(140)
        '            Else
        '                strFilterCommand = Chr(0) + Chr(0) + Chr(0) + Chr(2) + Chr(140)
        '            End If

        '        Case Else
        '            If blnWideChannel(intCurrentChannel) Then
        '                strFilterCommand = Chr(0) + Chr(0) + Chr(0) + Chr(0) + Chr(140)
        '            Else
        '                strFilterCommand = Chr(0) + Chr(0) + Chr(0) + Chr(2) + Chr(140)
        '            End If
        '    End Select
        '    SendHybridCommand(GetBytes(strFilterCommand))
        'End If

    End Sub  ' ProcessYaesu

    ' Subroutine to process Elecraft control
    Private Sub ProcessElecraft(ByVal intHertz As Integer)
        Dim strHertz As String

        If intHertz < 1801500 Then Return

        If RCB.Mode <> "FM" Then
            strHertz = Format(intHertz, "00000000000")
            SendASCIICommand("FA" & strHertz & ";", 100)   ' Select frequency
            SendASCIICommand("FR0;", 100)                  ' Select VFO A (split off)
            SendASCIICommand("RT0;", 100)                  ' RIT off
            SendASCIICommand("XT0;", 100)                  ' XIT off

            If RCB.Mode = "USB" Then cmbModel.Items.Add("Elecraft Radios")
            If RCB.Mode = "USBD" Then
                If RCB.Model = "Elecraft K2" Then
                    SendASCIICommand("MD9;", 100) ' Select USB Digital (RTTY Reverse)  1.2.8.2
                ElseIf RCB.Model = "Elecraft K3" Then
                    SendASCIICommand("MD6;", 100) ' Select USB Data mode revision 1.2.8.2
                    SendASCIICommand("DT0;", 0)     ' revision 1.2.8.2
                Else
                    cmbModel.Items.Add("Elecraft Radios") ' Same as for USB selected
                End If
            End If
        End If
    End Sub ' ProcessElecraft

    ' Subroutine to handle closing of Radio form
    Private Sub Radio_FormClosed(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosedEventArgs) Handles Me.FormClosed
        If RCB.Model = "Icom HF Marine Radios" Then
            If objControlPort IsNot Nothing AndAlso objControlPort.IsOpen Then
                RaiseEvent RadioCommand(GetBytes(ComputeNMEACommand("REMOTE,OFF")))
            End If
            Threading.Thread.Sleep(2000)
        End If
    End Sub ' Radio_FormClosed

    ' Subroutine to Refresh Property values
    Private Sub RefreshPropertyValues()
        RCB.CtrlPortDTR = chkControlDTR.Checked
        RCB.CtrlPortRTS = chkControlRTS.Checked
        RCB.PTTDTR = chkKeyDTR.Checked
        RCB.PTTRTS = chkKeyRTS.Checked

        RCB.Filter = CInt(cmbFilter.Text)
        If rdoFM.Checked Then RCB.Mode = "FM"
        If rdoUSB.Checked Then RCB.Mode = "USB"
        If rdoUSBDigital.Checked Then RCB.Mode = "USBD"
        RCB.InternalSoundCard = chkInternalSC.Checked
        RCB.InternalTuner = chkInternalTuner.Checked
        RCB.Model = cmbModel.Text
        RCB.Ant = CInt(cmbAntenna.Text)
        If cmbControlBaud.Text = "" Then
            RCB.CtrlPortBaud = 9600
        Else
            RCB.CtrlPortBaud = CInt(cmbControlBaud.Text)
        End If
        RCB.IcomAdd = txtIcomAddress.Text.Trim.ToUpper
        RCB.CtrlPort = cmbControlPort.Text
        RCB.PTTPort = cmbPTTCtrl.Text
        RCB.RadioControl = chkRadioCtrl.Checked
    End Sub '  RefreshPropertyValues

    ' Function to Set Radio parameters
    Private Function SetRadio() As Boolean
        If IsNothing(objControlPort) Then Return False
        If Not objControlPort.IsOpen Then Return False
        SetRadio = True


        ' This handles DTR Antenna switching
        'If strAntenna(intCurrentChannel).StartsWith("DTR") Then
        '    If strDTRAntenna <> "None" Then
        '        If IsNothing(objDTRSerial) Then
        '            Try
        '                objDTRSerial = New SerialPort
        '                objDTRSerial.PortName = strDTRAntenna
        '                objDTRSerial.Open()
        '            Catch ex As Exception
        '                Logs.Exception("[Radio.SetRadio] Error in setting DTR Serial port on " & strDTRAntenna & "  Err:" & ex.ToString)
        '            End Try
        '        End If
        '        Try
        '            If strAntenna(intCurrentChannel) = "DTR_On" Then
        '                objDTRSerial.DtrEnable = True
        '            ElseIf strAntenna(intCurrentChannel) = "DTR_Off" Then
        '                objDTRSerial.DtrEnable = False
        '            End If

        '        Catch
        '            Logs.Exception("[Radio.SetRadio] Error setting DTR antenna port value. Err" & Err.Description)
        '        End Try
        '    Else
        '        Logs.Exception("[Radio.SetRadio] Channel Set to DTR switching without DTR port assigned")
        '    End If
        'End If

        tmrPollFreq.Stop()
        objControlPort.DiscardOutBuffer()
        intLastReportedFreq = 0

        If RCB.Model.StartsWith("Icom") Then
            ProcessIcom(intHertz)
        ElseIf (RCB.Model.StartsWith("Kenwood") Or RCB.Model.StartsWith("Flex")) Then
            ProcessKenwood(intHertz)
        ElseIf RCB.Model.StartsWith("Yaesu") Then
            ProcessYaesu(intHertz)
        ElseIf RCB.Model.StartsWith("Elecraft") Then
            ProcessElecraft(intHertz)
            ' New Ten-Tec code...from Phil...not tested 
        ElseIf RCB.Model.StartsWith("Ten-Tec") Then
            If RCB.Model = "Ten-Tec Orion" Or RCB.Model = "Ten-Tec Eagle" Then
                ProcessTenTecOrion(intHertz)
            ElseIf RCB.Model = "Ten-Tec Omni-7" Or RCB.Model = "Ten-Tec Jupiter" Then
                ProcessTenTecOmni(intHertz)
            End If
        ElseIf RCB.Model.StartsWith("Micom") Then
            ProcessMicom(intHertz)
        Else
            SetRadio = False
        End If
        tmrPollFreq.Start()
    End Function ' SetRadio

    ' Function to initialize Radio ports
    Public Function InitRadioPorts() As Boolean
        ' close the ports if open
        Try
            If Not IsNothing(objPttSerialPort) Then
                objPttSerialPort.Close()
                objPttSerialPort = Nothing
            End If
            If Not IsNothing(objControlPort) Then
                objControlPort.Close()
                objControlPort = Nothing
            End If
        Catch ex As Exception
            Logs.Exception("[Radio.InitiRadioPorts] Error closing ports: " & ex.ToString)
        End Try

        ' initialize and open the Control port.

        If RCB.CtrlPort.StartsWith("COM") Then
            Try
                If Not IsNothing(objControlPort) Then
                    objControlPort.Close()
                    objControlPort = Nothing
                End If
                objControlPort = New SerialPort

                objControlPort.WriteTimeout = 1000
                objControlPort.ReceivedBytesThreshold = 1
                objControlPort.Handshake = Handshake.None
                objControlPort.DataBits = 8
                objControlPort.BaudRate = RCB.CtrlPortBaud
                objControlPort.DtrEnable = RCB.CtrlPortDTR
                objControlPort.RtsEnable = RCB.CtrlPortRTS
                objControlPort.StopBits = 2 ' 1
                objControlPort.Parity = Parity.None
                objControlPort.PortName = RCB.CtrlPort
                objControlPort.Open()
                objControlPort.DiscardInBuffer()
                objControlPort.DiscardOutBuffer()
            Catch ex As Exception
                Logs.Exception("[Radio.InitRadioPorts] Control port " & RCB.CtrlPort & " Failed to open!  Err: " & ex.ToString)
                Return False
            End Try
            Logs.WriteDebug("[Radio.InitRadioPorts] Control port " & RCB.CtrlPort & " opened.")
        End If

        If RCB.PTTPort.StartsWith("COM") AndAlso RCB.PTTPort <> RCB.CtrlPort Then
            Try
                If Not IsNothing(objPttSerialPort) Then
                    objPttSerialPort.Close()
                    objPttSerialPort = Nothing
                End If
                objPttSerialPort = New SerialPort
                objPttSerialPort.PortName = RCB.PTTPort
                objPttSerialPort.DtrEnable = False
                objPttSerialPort.RtsEnable = False
                objPttSerialPort.Open()
                objPttSerialPort.DiscardInBuffer()
                objPttSerialPort.DiscardOutBuffer()
            Catch ex As Exception
                Logs.Exception("[Radio.InitRadioPorts] PTT port " & RCB.PTTPort & " Failed to open!  Err: " & ex.ToString)
                Return False
            End Try
            Logs.WriteDebug("[Radio.InitRadioPorts] PTT port " & RCB.PTTPort & " opened.")
        ElseIf RCB.PTTPort.StartsWith("COM") AndAlso RCB.PTTPort = RCB.CtrlPort Then
            If RCB.PTTDTR Then objControlPort.DtrEnable = False
            If RCB.PTTRTS Then objControlPort.RtsEnable = False
        End If
        tmrPollFreq.Start()
        Return True
    End Function  '  InitRadioPorts

    ' Function to send ASCII command to radio 
    Private Function SendASCIICommand(ByVal strCommand As String, Optional ByVal intDelayMs As Integer = 50) As Boolean
        ' Sends a string command to the radio
        ' The delay is needed for most radios to allow time for processing the command...
        Try
            objControlPort.Write(strCommand)
            If intDelayMs > 0 Then Thread.Sleep(intDelayMs)
            Return True
        Catch
            Return False
        End Try
    End Function '  SendASCIICommand

    ' Function to send Binary command to radio 
    Private Function SendBinaryCommand(ByVal bytCommand() As Byte, Optional ByVal intDelayMs As Integer = 50) As Boolean
        ' Sends a binary command to the radio
        ' The delay is needed for most radios to allow time for processing the command...
        Try
            objControlPort.Write(bytCommand, 0, bytCommand.Length)
            If intDelayMs > 0 Then Thread.Sleep(intDelayMs)
            Return True
        Catch
            Return False
        End Try
    End Function  '  SendBinaryCommand

    ' Function to send Binary Command to Micom Radios 
    Private Function SendBinaryCommandLength(ByVal bytCommand() As Byte, ByVal intLength As Integer, Optional ByVal intDelayMs As Integer = 50) As Boolean
        '
        ' Sends a binary command to the radio.  Specify the number of bytes to send.  This routine is used primarily for Micom commands.
        ' The delay is needed for most radios to allow time for processing the command...
        '
        Try
            objControlPort.Write(bytCommand, 0, intLength)
            If intDelayMs > 0 Then Thread.Sleep(intDelayMs)
            Return True
        Catch
            Return False
        End Try
    End Function  '  SendBinaryCommandLength

    ' Function to send Hybrid Command to radio
    Private Function SendHybridCommand(ByVal bytCommand() As Byte, Optional ByVal intDelayMs As Integer = 50) As Boolean
        Try
            objControlPort.Write(bytCommand, 0, bytCommand.Length)
            If intDelayMs > 0 Then Thread.Sleep(intDelayMs)
            Return True
        Catch
            Return False
        End Try
    End Function '  SendHybridCommand

    ' Function to send NMEA command to radio
    Private Function SendNMEACommand(ByVal strCommand As String) As Boolean
        ' Sends an NMEA command to the radio...
        Try
            If objControlPort IsNot Nothing Then objControlPort.Write(GetBytes(strCommand), 0, strCommand.Length)
            Thread.Sleep(100)
            Return True
        Catch
            Return False
        End Try
    End Function '  SendNMEACommand

    ' Subroutine to set the Dial frequency of the radio
    Public Sub SetDialFrequency(ByVal intHertz As Integer)
        Dim intWait As Int32 = 0
        While blnRequestingRadioFreq And intWait < 10
            Thread.Sleep(50)
            intWait += 1
        End While
        If RCB.Model = "none" Then Return
        Me.intHertz = intHertz
        'USBMod = False ' insure modulation is switched to Aux input on frequency change for radios with built in sound cards (no effect on other radios)
        SetRadio()
        GetFrequency() ' This causes the new frequency to be read back and sent to the Host and update the CF display
    End Sub  'SetDialFrequency

    ' Subroutine to show proerty values on form
    Private Sub ShowPropertyValues()
        chkControlDTR.Checked = RCB.CtrlPortDTR
        chkControlRTS.Checked = RCB.CtrlPortRTS
        chkKeyDTR.Checked = RCB.PTTDTR
        chkKeyRTS.Checked = RCB.PTTRTS
        cmbFilter.Text = RCB.Filter.ToString
        rdoFM.Checked = RCB.Mode = "FM"
        rdoUSB.Checked = RCB.Mode = "USB"
        rdoUSBDigital.Checked = RCB.Mode = "USBD"
        cmbControlBaud.Text = RCB.CtrlPortBaud.ToString
        cmbControlPort.Text = RCB.CtrlPort
        cmbModel.Text = RCB.Model
        cmbPTTCtrl.Text = RCB.PTTPort
        txtIcomAddress.Text = RCB.IcomAdd
        chkRadioCtrl.Checked = RCB.RadioControl
        cmbAntenna.Text = RCB.Ant.ToString
        chkInternalTuner.Checked = RCB.InternalTuner
        chkInternalSC.Checked = RCB.InternalSoundCard
    End Sub '  ShowPropertyValues

    ' Subroutine to Write radio settings to ini file
    Private Sub WriteRadioSettings()
        Dim strSection As String = "Radio"

        objIniFile.WriteInteger(strSection, "Control Baud", RCB.CtrlPortBaud)
        objIniFile.WriteString(strSection, "Control DTR", RCB.CtrlPortDTR.ToString)
        objIniFile.WriteInteger(strSection, "Channel Filter Control", RCB.Filter)
        objIniFile.WriteString(strSection, "Control RTS", RCB.CtrlPortRTS.ToString)
        If RCB.CtrlPort.Trim <> "" Then objIniFile.WriteString(strSection, "Control Port", RCB.CtrlPort)
        objIniFile.WriteString(strSection, "Mode", RCB.Mode)
        objIniFile.WriteString(strSection, "Icom Address", RCB.IcomAdd)
        objIniFile.WriteString(strSection, "Model", RCB.Model)
        objIniFile.WriteString(strSection, "Tuner", RCB.InternalTuner.ToString)
        objIniFile.WriteString(strSection, "Use Internal SC", RCB.InternalSoundCard.ToString)
        objIniFile.WriteString(strSection, "KeyPTT DTR", RCB.PTTDTR.ToString)
        objIniFile.WriteString(strSection, "KeyPTT RTS", RCB.PTTRTS.ToString)
        objIniFile.WriteString(strSection, "PTT Control Port", RCB.PTTPort)
        objIniFile.WriteInteger(strSection, "Ant", RCB.Ant)
        objIniFile.WriteString("Radio", "Enable Radio Control", chkRadioCtrl.Checked.ToString)
        objIniFile.Flush()
    End Sub '  WriteRadioSettings

    ' Public Sub to Key the PTT if radio control is used. 
    Public Sub PTT(blnPTT As Boolean)
        If RCB.PTTPort.StartsWith("COM") Then
            PTTRTSDTR(RCB.PTTRTS, RCB.PTTDTR, blnPTT)
        ElseIf RCB.PTTPort.StartsWith("CAT") Then
            PTTCAT(RCB.Model, blnPTT)
        End If
    End Sub  'PTT

    ' This Public Subroutine handles PTT Keying when the COM Port RTS or DTR signal is used) 
    Private Sub PTTRTSDTR(ByVal blnEnbRTS As Boolean, blnEnbDTR As Boolean, blnPTT As Boolean)
        If RCB.PTTPort <> RCB.CtrlPort Then
            If blnEnbDTR = True Then objPttSerialPort.DtrEnable = blnPTT
            If blnEnbRTS = True Then objPttSerialPort.RtsEnable = blnPTT
        Else ' This allows using the same port as the control port for PTT control (some radios)
            If blnEnbDTR = True Then objControlPort.DtrEnable = blnPTT
            If blnEnbRTS = True Then objControlPort.RtsEnable = blnPTT
        End If
        blnLastPTTState = blnPTT
        If Not blnPTT Then tmrPTTWatchdog.Stop()
        If blnPTT Then RetriggerPTTWatchdog()
    End Sub  'PTTRTSDTR

    ' This Public Subroutine handles PTT via CAT on some radios 
    Private Sub PTTCAT(strRadioClass As String, blnPTT As Boolean)
        blnInPTTCAT = True
        ' not all models are supported 
        Static dttLastPTTCmd As Date = Now

        While Now.Subtract(dttLastPTTCmd).TotalMilliseconds < 200 ' safety mechanism to insure no immedate back to back PTT commands.
            Thread.Sleep(20)
        End While
        dttLastPTTCmd = Now
        Try
            If (strRadioClass = "none" Or strRadioClass = "") Then Exit Sub
            If IsNothing(objControlPort) Then Logs.Exception("[Radio.PTTCAT] ControlPort " & RCB.CtrlPort & " was Nothing") : Exit Sub
            If Not objControlPort.IsOpen Then Logs.Exception("[Radio.PTTCAT] ControlPort " & RCB.CtrlPort & " not open") : Exit Sub

            ' Icom Radios 
            If strRadioClass = "CI-V" Then
                Dim bytCommand(7) As Byte
                bytCommand(0) = &HFE
                bytCommand(1) = &HFE
                bytCommand(2) = CByte("&H" & RCB.IcomAdd)
                bytCommand(3) = &HF1
                bytCommand(4) = &H1C          ' Set Transceiver PTT
                bytCommand(5) = 0
                If blnPTT Then
                    bytCommand(6) = 1          ' Set to Transmit PTT on
                Else
                    bytCommand(6) = 0          ' Set to transmit PTT Off
                End If
                bytCommand(7) = &HFD
                SendBinaryCommand(bytCommand, 20)

                ' Confirmed for 7100, 
            ElseIf strRadioClass = "Icom 7100" Or strRadioClass = "Icom 7200" Or strRadioClass = "Icom 7600" Or strRadioClass = "Icom 9100" Then
                If (Not blnUSBMod) Or Not blnUSBModInitialized Then
                    USBMod = True ' switch the modulation input if not already switched 
                    Thread.Sleep(20)
                End If
                ' Configure basic command data
                Dim bytCommand(7) As Byte
                bytCommand(0) = &HFE
                bytCommand(1) = &HFE
                bytCommand(2) = CByte("&H" & RCB.IcomAdd)
                bytCommand(3) = &HF1

                bytCommand(4) = &H1C          ' Set Transceiver PTT command
                bytCommand(5) = 0
                If blnPTT Then
                    bytCommand(6) = 1          ' Set to Transmit PTT on
                Else
                    bytCommand(6) = 0          ' Set to transmit PTT Off
                End If
                bytCommand(7) = &HFD
                SendBinaryCommand(bytCommand, 20)


            ElseIf strRadioClass = "Kenwood TS480HX" Or strRadioClass = "Kenwood TS-480SAT" Then
                If blnPTT Then
                    SendASCIICommand("TX1;", 20)
                Else
                    SendASCIICommand("RX;", 20)
                End If

            ElseIf strRadioClass = "Kenwood TS-2000" Then ' not sure if this works documentation not clear
                If blnPTT Then
                    SendASCIICommand("TX0;", 20)
                Else
                    SendASCIICommand("RX0;", 20)
                End If

                ' Confirmed for TS-590S
            ElseIf strRadioClass = "Kenwood TS-590S" Then
                If (Not blnUSBMod) Or Not blnUSBModInitialized Then
                    USBMod = True ' switch the modulation input if not already switched 
                    Thread.Sleep(20)
                End If
                ' Configure basic command data
                If blnPTT Then
                    SendASCIICommand("TX1;", 20) ' Data send using USB or ACC2 input (depending on MUX value)
                Else
                    SendASCIICommand("RX;", 20) ' Set to Receive
                End If

                ' Confirmed for TS-450
            ElseIf strRadioClass.StartsWith("Kenwood") Then  '  A temporary catch all to key other kenwoods. 
                If blnPTT Then
                    SendASCIICommand("TX;", 20)
                Else
                    SendASCIICommand("RX;", 20)
                End If

                ' Yaesu Radios 
            ElseIf strRadioClass = "Yaesu FT-450" Or strRadioClass = "Yaesu FT-2000" Or strRadioClass = "Yaesu FT-950" Then
                If blnPTT Then
                    SendASCIICommand("TX1;", 20)
                Else
                    SendASCIICommand("TX0;", 20)
                End If

            ElseIf strRadioClass = "Yaesu FT-847" Or strRadioClass = "Yaesu FT-857" Or strRadioClass = "Yaesu FT-897" Then
                Dim bytCommand() As Byte = {0, 0, 0, 0, 0}
                If blnPTT Then
                    bytCommand(4) = &H8
                Else
                    bytCommand(4) = &H88
                End If
                SendBinaryCommand(bytCommand, 20)

            ElseIf strRadioClass = "Yaesu FT-1000" Or strRadioClass = "Yaesu FT-840" Then ' FT-840 confirmed
                Dim bytCommand() As Byte = {0, 0, 0, 0, 0}
                If blnPTT Then bytCommand(3) = 1
                bytCommand(4) = &HF
                SendBinaryCommand(bytCommand, 20)
                'Logs.WriteDebug("[Radio.PTTCAAT] Send Yaesu 840,1000 PTT = " & blnPTT.ToString) ' Temp ...remove after debugging. 

                ' Elecraft Radios,Elecraft K2,Elecraft K3,Flex radios 
            ElseIf (strRadioClass = "Flex radios" Or strRadioClass.StartsWith("Elecraft")) Then ' 
                If blnPTT Then
                    SendASCIICommand("TX;", 20)
                Else
                    SendASCIICommand("RX;", 20)
                End If
                '
                '  Ten-Tec Orion,Ten-Tec Omni-7,Ten-Tec Eagle,Ten-Tec Jupiter 
            ElseIf strRadioClass = "Ten-Tec Orion" Or strRadioClass = "Ten-Tec Eagle" Then
                If blnPTT Then
                    SendASCIICommand("*TK" & vbCr, 20)
                Else
                    SendASCIICommand("*TU" & vbCr, 20)
                End If

            ElseIf strRadioClass = "Ten-Tec Omni-7" Or strRadioClass = "Ten-Tec Jupiter" Then
                If blnPTT Then
                    SendASCIICommand("*T" & Convert.ToString(&H20) & vbCr, 20)
                Else
                    SendASCIICommand("*T" & Convert.ToString(&H0) & vbCr, 20)
                End If
            End If
        Catch ex As Exception
            Logs.Exception("[Radio.PTT] Err: " & ex.ToString)
        End Try
        If blnLastPTTState And Not blnPTT Then
            tmrPTTDblOff.Start() ' start the double PTT safety off timer.
        End If
        blnLastPTTState = blnPTT
        If blnPTT Then RetriggerPTTWatchdog()
        If Not blnPTT Then tmrPTTWatchdog.Stop()
        blnInPTTCAT = False
    End Sub ' PTTCAT

    ' Subroutine to retrigger the PTT watchdog timer
    Private Sub RetriggerPTTWatchdog()
        dttLastPTTOn = Date.UtcNow
        tmrPTTWatchdog.Stop()
        tmrPTTWatchdog.Start()
    End Sub  '  RetriggerPTTWatchdog

    ' Subroutine for tmrPTTWatchdog elapsed
    Private Sub tmrPTTWatchdog_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrPTTWatchdog.Elapsed
        Dim dttStartWait As Date = Now

        While blnInPTTCAT And Now.Subtract(dttStartWait).TotalMilliseconds < 1000
            Thread.Sleep(50)
        End While
        Logs.Exception("[Radio.tmrPTTWatchdog_Elapsed] PTT Watchdog Timer trip... Force PTT Off for " & strRadioPTTClass & " Last PTT-on:" & Format(dttLastPTTOn, "HH:mm:ss") & "  blnInPTTCAT=" & blnInPTTCAT.ToString)
        PTT(False)
        Thread.Sleep(1000)
        PTT(False)
    End Sub  '  tmrPTTWatchdog_Elapsed

    ' Subroutine to handle commands for TenTec Omni radio
    Private Sub ProcessTenTecOmni(ByVal intHertz As Integer)
        '  Commands for a Ten-Tec Omni 7 and Jupiter.
        If intHertz < 1801500 Then Return
        If RCB.Mode <> "FM" Then
            ' Make 7 byte command to set frequency: *AddddCr
            Dim bytFreqCommand(0 To 6) As Byte
            bytFreqCommand(0) = &H2A    ' *
            bytFreqCommand(1) = &H41    ' A
            MakeByteArray((intHertz), bytFreqCommand, 4, 2)    ' 4 bytes for the frequency
            bytFreqCommand(6) = &HD    ' cr
            SendHybridCommand(bytFreqCommand)

            If RCB.Mode = "USB" Then SendASCIICommand("*M11" & vbCr) ' Select USB
            'If blnFilterControl Then
            '    Select Case RCB.Model
            '        Case "Ten-Tec Jupiter"
            '            If blnWideChannel(intCurrentChannel) Then
            '                SendASCIICommand("*W" & BinaryString(15, 1) & vbCr)  ' Receive filter = 2400 Hz
            '            Else
            '                SendASCIICommand("*W" & BinaryString(28, 1) & vbCr)  ' Receive filter = 600 Hz
            '            End If
            '        Case Else
            '            If blnWideChannel(intCurrentChannel) Then
            '                SendASCIICommand("*W" & BinaryString(19, 1) & vbCr)  ' Receive filter = 2400 Hz
            '            Else
            '                SendASCIICommand("*W" & BinaryString(30, 1) & vbCr)  ' Receive filter = 600 Hz
            '            End If
            '    End Select
            'End If
            ' SendASCIICommand("*L" & BinaryString(0, 3) & vbCr)  ' RIT, XIT off
            'SendASCIICommand("*G3" & vbCr)                      ' AGC fast
            'SendASCIICommand("*K" & vbNullChar & vbNullChar & vbNullChar & vbCr)    ' Noise, blanker, notch off
            ' antenna switching by radio
            'If strAntenna(intCurrentChannel) = "Int_1" Then
            '    SendASCIICommand("*KABNN" & vbCr)
            'ElseIf strAntenna(intCurrentChannel) = "Int_2" Then
            '    SendASCIICommand("*KANBN" & vbCr)
            'End If
        End If
    End Sub  '  ProcessTenTecOmni

    ' Subroutine to process setup of TenTec Orion
    Private Sub ProcessTenTecOrion(ByVal intHertz As Integer)
        '  Commands for a Ten-Tec Orion and Eagle.
        Dim strHertz As String
        If intHertz < 1801500 Then Return
        If RCB.Mode <> "FM" Then
            SendASCIICommand("*KVAAA" & vbCr)               ' Select VFO A
            strHertz = ConvertDoubleToString(CDbl(intHertz / 1000000), "#0.000000")    ' Format frequency in MHz
            SendASCIICommand("*AF" & strHertz & vbCr)       ' Select frequency
            If RCB.Mode = "USB" Then SendASCIICommand("*RMM0" & vbCr) ' Select USB
            'If blnFilterControl Then
            '    If blnWideChannel(intCurrentChannel) Then
            '        SendASCIICommand("*RMF2400" & vbCr)             ' Receive filter = 2400 Hz
            '    Else
            '        SendASCIICommand("*RMF600" & vbCr)             ' Receive filter = 600 Hz
            '    End If
            'End If
            REM SendASCIICommand("*RMR0" & vbCr)                ' RIT off
            REM SendASCIICommand("*RMX0" & vbCr)                ' XIT off
            'SendASCIICommand("*RMAF" & vbCr)                ' AGC fast
            'SendASCIICommand("*RMNA0" & vbCr)               ' Automatic notch filter off
            REM SendASCIICommand("*RMNN0" & vbCr)               ' Noise reduction off
            REM SendASCIICommand("*RMNB0" & vbCr)               ' Noise blanker off
            'If strAntenna(intCurrentChannel) = "Int_1" Then
            '    SendASCIICommand("*KABNN" & vbCr)
            'ElseIf strAntenna(intCurrentChannel) = "Int_2" Then
            '    SendASCIICommand("*KANBN" & vbCr)
            'End If
        End If
    End Sub  'ProcessTenTecOrion

    ' Subroutine to Make a byte array from integer value
    Private Sub MakeByteArray(ByVal intValue As Integer, ByVal bytArray() As Byte, Optional ByVal intNumBytes As Integer = 4, Optional ByVal intOffset As Integer = 0)
        '
        '  Store an integer value into the bytArray byte array.
        '  The most significant byte is stored in the starting element of the array specified by intOffset (0 based).
        '
        Dim i As Integer
        For i = 0 To intNumBytes - 1
            bytArray(intNumBytes - 1 - i + intOffset) = CByte((intValue >> (8 * i)) And &HFF)
        Next
    End Sub  '  MakeByteArray

    ' Function to create Binary string from integer 
    Private Function BinaryString(ByVal intValue As Integer, ByVal intLength As Integer) As String
        '  Convert a binary value to a String with a specified number of characters.
        '  Each byte of the value is returned as a binary value in the string; it is NOT converted to ASCII digits.
        '  The most significant byte is the first character in the returned string.
        Dim strResult As String = ""
        Dim i As Integer
        Dim intByte As Integer

        ' Build output string from each byte of input value.  Most significant byte goes first.
        For i = intLength - 1 To 0 Step -1
            intByte = (intValue >> (8 * i)) And &HFF
            strResult &= Convert.ToChar(intByte)
        Next
        Return strResult
    End Function '  BinaryString

    ' Function to convert Double to string 
    Public Function ConvertDoubleToString(ByVal d As Double, ByVal f As String) As String
        '
        ' Convert a double to a string. If the localization uses a ',' instead of a decimal point, we'll replace it
        ' so as not to cause issues with other parts of the program that depend on a decimal point
        '
        Dim strRet As String = Format(d, f)
        strRet = strRet.Replace(",", ".")
        Return strRet
    End Function ' ConvertDoubleToString

    ' Subroutine to process the Micom radio
    Private Sub ProcessMicom(Optional ByVal intHertz As Integer = 0)
        '  Perform control for a Micom 3F radio.
        Dim retc As Integer
        Dim intDial As Integer

        '  Perform basic initialization and mode setting.
        If MicomInitialize() = False Then
            Return
        End If
        '  Tune the radio to the desired frequency.
        Try
            intDial = intHertz
            retc = MICOMSetRXFreq(intDial)
            If retc <> 0 Then                   ' If it worked
                retc = MICOMSetTXFreq(intDial)
                If retc <> 0 Then                ' If it worked
                    Return
                End If
            End If
            MsgBox("[RadioMicom.SetFrequency]" & " Failed")
            Return
        Catch
            MsgBox("[RadioMicom.SetFrequency] " & Err.Description)
            Return
        End Try
        Return
    End Sub ' ProcessMicom

    ' Function to Initialize Micom Radio
    Private Function MicomInitialize() As Boolean
        '
        '  Initialize a Micom radio.
        '
        Dim retc As Integer
        If objControlPort Is Nothing Then Return False
        If cmbControlPort.Text = "Via TNC" Then
            MsgBox("The MICOM radio models must be connected to a separate serial port on the host computer...", MsgBoxStyle.Critical)
            Return False
        End If
        '
        '  Initialize a timer for received data.
        '
        MiTimer = New System.Timers.Timer

        Try
            '*****************************************************************
            '* Perform initial radio setup.
            '  1 set the radio into frequency mode vs channel mode
            '  2 check to make sure it is in USB mode and report an error if not
            '  3 turn off clarifier to prevent hard to find problems 
            '*****************************************************************

            ' Put the radio into frequency mode...
            retc = MICOMSetMode(2)
            retc = MICOMSetSSBState(1)
            retc = MICOMSetClarifier(0)
            'retc = MICOMSetTXPwr(100)      ' Set transmitter power
        Catch
            MsgBox("[RadioMicom.InitializeSerialPort] " & Err.Description)
            Return False
        End Try
        Return True
    End Function   ' MicomInitialize 

    ' Function to set MICOM RX Freq 
    Private Function MICOMSetRXFreq(ByVal RXFreq As Integer) As Integer
        ' Set receive frequency in HZ. Input is an unsigned 32-bit binary number
        ' that represents the desired frequency in HZ. Range: 100 kHz to 30 mHz.
        ' Returns the set frequency or 0 on failure.
        Const DataLen As Byte = 6 '0x06
        Const OpCode As Byte = &H5 '5 '0x05

        Const RptReq As Byte = 0 ' 0x00
        'Length of an ACk message
        Dim rc As Byte
        Dim freqval(3) As Byte
        Dim Mydata(4) As Byte
        Dim MyResult(SizeofACK) As Byte

        If RXFreq > 100000 And RXFreq < 30000001 Then
            rc = LongToLine(RXFreq, freqval) ' convert input freq to 4-byte big endian
            Mydata(0) = RptReq
            Mydata(1) = freqval(0)
            Mydata(2) = freqval(1)
            Mydata(3) = freqval(2)
            Mydata(4) = freqval(3)
            rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
            If rc <> 0 Then
                rc = GetMICOMData(SizeofACK, MyResult) 'Get the response from MICOM
                If rc <> 0 Then
                    MICOMSetRXFreq = RXFreq
                Else
                    MICOMSetRXFreq = 0
                End If
            Else
                MICOMSetRXFreq = 0
            End If
        Else
            MICOMSetRXFreq = 0
        End If
    End Function  '  MICOMSetRXFreq

    ' Function to set Micom TX freq
    Private Function MICOMSetTXFreq(ByVal TXFreq As Integer) As Integer
        ' Set traansmit frequency in HZ. Input is an unsigned 32-bit binary number
        ' that represents the desired frequency in HZ. Range: 1.6 mHz to 30 mHz.
        ' Returns the set frequency or 0 on failure.
        Const DataLen As Byte = 6 '0x06
        Const OpCode As Byte = 7 '0x07
        Const RptReq As Byte = 0 ' 0x00

        Dim rc As Byte
        Dim freqval(3) As Byte
        Dim Mydata(4) As Byte
        Dim Result(SizeofACK) As Byte

        If TXFreq > 1600000 And TXFreq < 30000001 Then
            rc = LongToLine(TXFreq, freqval) ' convert input freq to 4-byte big endian
            Mydata(0) = RptReq
            Mydata(1) = freqval(0)
            Mydata(2) = freqval(1)
            Mydata(3) = freqval(2)
            Mydata(4) = freqval(3)
            rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
            If rc <> 0 Then
                rc = GetMICOMData(SizeofACK, Result) 'Get the response from MICOM
                If rc <> 0 Then
                    MICOMSetTXFreq = TXFreq
                Else
                    MICOMSetTXFreq = 0
                End If
            Else
                MICOMSetTXFreq = 0
            End If
        Else
            MICOMSetTXFreq = 0
        End If
    End Function '  MICOMSetTXFreq

    ' Funtion to set MICOM SSB State
    Private Function MICOMSetSSBState(ByRef SSBState As Byte) As Byte
        '* Set MICOM Side Band
        '* ARG = 1 = USB = 0 = LSB
        '* returns 1 = USB 2 = LSB 0 = error
        Const DataLen As Byte = 3 '0x03
        Const OpCode As Byte = 3 '0x03

        Dim Mydata(2) As Byte
        Dim Result(SizeofACK + SizeofSSBRpt) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' Don't Get SSB Report
        Mydata(1) = SSBState ' 1 = USB 0 = LSB

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            ' If Result(9) = OpCode Then
            '   MICOMSetSSBState = Result(9) + 1 ' return 1 for LSB or 2 for USB
            ' Else
            '   MICOMSetSSBState = 0            ' fail
            ' End If
            MICOMSetSSBState = 1 ' pass
        Else
            MICOMSetSSBState = 0 ' fail
        End If
    End Function '   MICOMSetSSBState

    ' Function to report active sideband for MICOM
    Private Function MICOMRptSSBState() As Byte
        '* Report active side band
        '* returns 2 = USB, 1 = LSB, 0 = Error
        Const DataLen As Byte = 1 '0x01
        Const OpCode As Byte = 4 '0x04

        Dim Mydata(2) As Byte
        Dim Result(SizeofACK + SizeofSSBRpt) As Byte
        Dim rc As Byte

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK + SizeofSSBRpt, Result)
            If Result(9) = OpCode - 1 Then
                MICOMRptSSBState = Result(10) + CByte(1) ' return 1 for LSB or 2 for USB
            Else
                MICOMRptSSBState = 0 ' fail
            End If
        Else
            MICOMRptSSBState = 0 ' fail
        End If
    End Function '  MICOMRptSSBState

    ' Function to set MICOM radio squelch
    Private Function MICOMSetSquelch(ByRef SQState As Byte) As Byte
        '* Set MICOM Squelch state
        '* ARG = 1 = on = 0 = off
        Const DataLen As Byte = 3 '0x03
        Const OpCode As Byte = 9 '0x09

        Dim Mydata(2) As Byte
        Dim Result(SizeofACK) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' No Report required
        Mydata(1) = SQState ' 1 = on 0 = off

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            MICOMSetSquelch = rc ' Pass or fail
        Else
            MICOMSetSquelch = 0 ' fail
        End If
    End Function  'MICOMSetSquelch

    Private Function MICOMSetNB(ByRef NBState As Byte) As Byte
        '
        '* Set MICOM Noise blanker state
        '* ARG = 1 = on = 0 = off
        '
        Const DataLen As Byte = 6 '0x06
        Const OpCode As Byte = &H15 '21 '0x15

        Dim Mydata(5) As Byte
        Dim Result(SizeofACK) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' No Report required
        Mydata(1) = 0 ' Notch filter off
        Mydata(2) = 0 ' Clipper off
        Mydata(3) = NBState ' 1 = on 0 = off
        Mydata(4) = &HFF '255 ' No Change

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            MICOMSetNB = rc ' Pass or fail
        Else
            MICOMSetNB = 0 ' fail
        End If
    End Function

    Private Function MICOMSetAten(ByRef AtenState As Byte) As Byte
        '
        '* Set MICOM Attenuator state
        '* ARG = 1 = on = 0 = off
        '
        Const DataLen As Byte = 6 '0x06
        Const OpCode As Byte = &H15 '21 '0x15

        Dim Mydata(5) As Byte
        Dim Result(SizeofACK) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' No Report required
        Mydata(1) = 0 ' Notch filter off
        Mydata(2) = 0 ' Clipper off
        Mydata(3) = &HFF
        Mydata(4) = AtenState

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            MICOMSetAten = rc ' Pass or fail
        Else
            MICOMSetAten = 0 ' fail
        End If
    End Function

    Private Function MICOMSetAGC(ByRef AGCState As Byte) As Byte
        '
        '* Set MICOM AGC State
        '* ARG = 0 = slow = 1`= fast
        '
        Const DataLen As Byte = 4 '0x04
        Const OpCode As Byte = &HD '13 '0x0D

        Dim Mydata(4) As Byte
        Dim Result(SizeofACK) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' No Report required
        Mydata(1) = 0 ' SSB 2700
        Mydata(2) = AGCState ' 0 = Slow 1 = fast

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            MICOMSetAGC = rc ' Pass or fail
        Else
            MICOMSetAGC = 0
        End If
    End Function

    Private Function MICOMSetMode(ByRef Mode As Byte) As Byte
        '
        '* Set MICOM Radio Op
        '* This command is used to set "frquency Mode ARG = 2"
        '
        Const DataLen As Byte = 3 '0x03
        Const OpCode As Byte = &H17 '23 '0x17

        Dim Mydata(2) As Byte
        Dim Result(SizeofACK) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' No Report required
        Mydata(1) = Mode ' Mode: 0=ALE Channel Mode 1=ALE Net scan 2=Freq Mode 3=channel mode
        ' 4 = channel scan mode

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            MICOMSetMode = rc ' Pass or fail
        Else
            MICOMSetMode = 0 ' fail
        End If
    End Function

    Private Function MICOMSetClarifier(ByRef ClarifierState As Byte) As Byte
        '
        '* Set MICOM Clarifier state
        '* arg = 0: clarifier off else clairifier on
        '* This command included so the program can ensure the clarifier is off.
        '
        Const DataLen As Byte = 3 '0x03
        Const OpCode As Byte = &HF '15 '0x0F

        Dim Mydata(2) As Byte
        Dim Result(SizeofACK) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' No Report required
        Mydata(1) = 0 ' Clarifier value

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            MICOMSetClarifier = rc ' Pass or fail
        Else
            MICOMSetClarifier = 0 ' fail
        End If
    End Function

    Private Function MICOMSetTXPwr(ByRef PWR As Short) As Byte
        '
        '* Set MICOM transmitter power
        '* ARG is a value 0f 25, 62, 100, or 125  It is a 16-bit binary value.
        '
        Const DataLen As Byte = 4 '0x04
        Const OpCode As Byte = &H11 '17 '0x11

        Dim Mydata(2) As Byte
        Dim Result(SizeofACK) As Byte
        Dim rc As Byte

        Mydata(0) = 0 ' No Report required
        Mydata(1) = CByte(PWR >> 8)  ' Power out 25,62,100 or 125
        Mydata(2) = CByte(PWR And &HFF)
        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK, Result)
            MICOMSetTXPwr = rc ' Pass or fail
        Else
            MICOMSetTXPwr = 0 ' fail
        End If
    End Function

    Private Function MICOMRptRXFreq() As Integer
        '
        '* Report MICOM receiver Freq
        '
        Const DataLen As Byte = 1 '0x01
        Const OpCode As Byte = 6 '0x06

        Dim rc As Byte
        Dim freqval(3) As Byte
        Dim Mydata(1) As Byte
        Dim WrkData(3) As Byte
        Dim Result(SizeofACK + SizeofFrpt) As Byte

        rc = Cmd2MICOM(DataLen, FrmTo, OpCode, Mydata) 'send command to MICOM
        If rc <> 0 Then
            rc = GetMICOMData(SizeofACK + SizeofFrpt, Result)
            If rc <> 0 Then
                If Result(9) = OpCode - 1 Then ' check for proper opcode in response
                    WrkData(0) = Result(10) 'extract the reported frequency
                    WrkData(1) = Result(11) 'in line format
                    WrkData(2) = Result(12)
                    WrkData(3) = Result(13)
                    MICOMRptRXFreq = LineToLong(WrkData) 'convert from line format to 32-bit binary value
                Else
                    MICOMRptRXFreq = 0
                End If
            Else
                MICOMRptRXFreq = 0
            End If
        Else
            MICOMRptRXFreq = 0
        End If
    End Function

    Private Function LongToLine(ByRef NR As Integer, ByRef Result() As Byte) As Byte
        '
        '* Convert 32-bit number to an array of 4 bytes to take care of the little
        '* endian to big endian problem with Intel processors. This little function
        '* will work on both little and big endian formats.
        '
        Dim NR1 As Integer = NR
        Result(3) = CByte(NR1 And &HFF)
        NR1 = NR1 >> 8
        Result(2) = (CByte(NR1 And &HFF))
        NR1 = NR1 >> 8
        Result(1) = (CByte(NR1 And &HFF))
        NR1 = NR1 >> 8
        Result(0) = (CByte(NR1 And &HFF))
        LongToLine = 0
    End Function

    Private Function LineToLong(ByRef Line() As Byte) As Integer
        '
        '* Convert four byte array to a 32-bit binary number.
        '* This little function will work on both little and big endian formats.
        '
        Dim WrkVal As Integer
        LineToLong = 0
        WrkVal = 0
        WrkVal = CInt(Line(3))
        WrkVal = WrkVal + (CInt(Line(2)) << 8)
        WrkVal = WrkVal + (CInt(Line(1)) << 16)
        WrkVal = WrkVal + (CInt(Line(0)) << 24)
        LineToLong = WrkVal
    End Function

    Private Function Cmd2MICOM(ByRef DataLen As Byte, ByRef FMTO As Byte, ByRef OpCode As Byte, ByRef data() As Byte) As Byte
        '
        '* Send a command to the MICOM radio
        '
        Dim I As Short
        Const SOM As Byte = &H24 '36 '0x24
        Const EOM As Byte = &H3
        Dim OutMsg(80) As Byte
        '	Dim OutStr As String

        '	OutStr = ""
        OutMsg(0) = SOM
        OutMsg(1) = DataLen
        OutMsg(2) = FMTO
        OutMsg(3) = OpCode
        If DataLen > 1 Then
            For I = 0 To DataLen - CByte(2)
                OutMsg(4 + I) = data(I)
            Next I
        End If
        OutMsg(3 + DataLen) = ChkSum(OutMsg, DataLen)
        OutMsg(4 + DataLen) = EOM
        ' Reset input data buffer
        intMicomRcvIndex = 0
        ' Send command to the radio.
        SendBinaryCommandLength(OutMsg, DataLen + 5)
        Return DataLen
    End Function

    Private Function GetMICOMData(ByRef NRBytes As Short, ByRef Result() As Byte) As Byte
        '
        '  Get a command response returned by a Micom.
        '  Return the number of bytes received or 0 if a timeout occurs.
        '
        Dim I As Integer

        If objControlPort.IsOpen = True Then
            System.Windows.Forms.Application.DoEvents() 'Give up control to Op sys temporarily
            MiTimer.Stop()
            Timer2Popped = False 'The timer interval is is milliseconds.
            MiTimer.Interval = NRBytes * 50 'This assumes a character time of 1 ms.
            MiTimer.Start()                 'Start the timer, runs in background.
            Do
                System.Windows.Forms.Application.DoEvents() 'Give up control to Op sys temporarily
            Loop Until intMicomRcvIndex >= NRBytes Or Timer2Popped = True
            MiTimer.Stop()
            ' See if a timeout occurred.
            If Timer2Popped Then
                GetMICOMData = 0 ' failed!
            Else
                ' Move data to caller's buffer
                For I = 0 To NRBytes - 1
                    Result(I) = bytMicomRcv(I)
                Next
                If (Result(2) And 15) <> 0 Then 'need an ACK?
                    I = MICOMSendACK()
                End If
                GetMICOMData = CByte(NRBytes)
            End If
        Else
            GetMICOMData = 0 'failed
        End If
    End Function

    Private Sub MiTimer_Elapsed(ByVal Sender As Object, ByVal e As System.Timers.ElapsedEventArgs) Handles MiTimer.Elapsed
        '
        '  The Micom data-receive timer triggered.
        '
        MiTimer.Stop()
        Timer2Popped = True
    End Sub

    Private Function MICOMSendACK() As Byte
        '
        '  Send an Ack to a Micom.
        '
        Const DataLen As Byte = 1 '0x01
        Const OpCode As Byte = &HF3 '243 '0xF3
        Const FMTO As Byte = &H10 '16 '0x10

        Dim rc As Byte
        ' Dim freqval(3) As Byte
        Dim Mydata(1) As Byte
        ' Dim WrkData(3) As Byte
        ' Dim Result(SizeofACK + SizeofFrpt) As Byte

        rc = Cmd2MICOM(DataLen, FMTO, OpCode, Mydata) 'send command to Terminal
        If rc <> 0 Then
            MICOMSendACK = 1
        Else
            MICOMSendACK = 0
        End If
    End Function

    Private Function ChkSum(ByRef msg() As Byte, ByRef DataLen As Byte) As Byte
        '
        '* generate a checksum for a message
        '
        Dim CkSumLen As Short
        Dim I As Short
        Dim Sum As Short
        CkSumLen = DataLen + CShort(2)
        Sum = 0 'initialize checksum

        For I = 0 To CkSumLen
            Sum = Sum + msg(I)
            Sum = CShort(Sum And &HFF) '255
        Next I
        ChkSum = CByte(Sum And &HFF) '255
    End Function

    Private Sub cmbSerialPTT_TextChanged(sender As Object, e As System.EventArgs) Handles cmbPTTCtrl.TextChanged
        lblHold.Visible = cmbPTTCtrl.Text.StartsWith("VOX")
        If cmbPTTCtrl.Text.StartsWith("VOX") Or cmbPTTCtrl.Text.StartsWith("CAT") Then
            chkKeyDTR.Enabled = False
            chkKeyRTS.Enabled = False
            chkKeyDTR.Checked = False
            chkKeyRTS.Checked = False
        Else
            chkKeyDTR.Enabled = True
            chkKeyRTS.Enabled = True
        End If
    End Sub

    Private Sub cmbAntenna_SelectedIndexChanged(sender As System.Object, e As System.EventArgs) Handles cmbAntenna.SelectedIndexChanged

    End Sub

    Private Sub tmrPollFreq_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrPollFreq.Elapsed
        tmrPollFreq.Stop()
        GetFrequency()
        'Dim objStatus As Status = Nothing
        'objStatus.ControlName = "lblCF"
        'If (intFreq <> intFreqSentToHost) And (intFreq > 0) Then
        '    If intFreq > 100000000 Then '  if over 100 Mhz
        '        If RCB.Mode = "FM" Then
        '            objStatus.Text = "FM dial " & Format(intFreq / 1000000, "#.000") & " MHz"
        '        Else
        '            objStatus.Text = "USB dial " & Format(intFreq / 1000000, "#.000") & " MHz"
        '        End If
        '    ElseIf intFreq > 1000000 Then
        '        If RCB.Mode = "FM" Then
        '            objStatus.Text = "FM dial " & Format(intFreq / 1000, "#.000") & " KHz"
        '        Else
        '            objStatus.Text = "USB dial " & Format(intFreq / 1000, "#.000") & " KHz"
        '        End If
        '    End If
        '    queTNCStatus.Enqueue(objStatus)
        '    objMain.objHI.QueueCommandToHost("FREQUENCY " & intFreq.ToString)
        '    intFreqSentToHost = intFreq
        'End If
        tmrPollFreq.Start()
    End Sub

    Private Sub PostNewFrequency(intFreq As Int32)

        Dim objStatus As Status = Nothing
        objStatus.ControlName = "lblCF"
        If intFreq > 0 And intFreq <> intLastReportedFreq Then
            If intFreq > 100000000 Then '  if over 100 Mhz
                If RCB.Mode = "FM" Then
                    objStatus.Text = "FM dial " & Format(intFreq / 1000000, "#.000") & " MHz"
                Else
                    objStatus.Text = "USB dial " & Format(intFreq / 1000000, "#.000") & " MHz"
                End If
            ElseIf intFreq > 1000000 Then
                If RCB.Mode = "FM" Then
                    objStatus.Text = "FM dial " & Format(intFreq / 1000, "#.000") & " KHz"
                Else
                    objStatus.Text = "USB dial " & Format(intFreq / 1000, "#.000") & " KHz"
                End If
            End If
            queTNCStatus.Enqueue(objStatus)
            objMain.objHI.QueueCommandToHost("FREQUENCY " & intFreq.ToString)
            intFreqSentToHost = intFreq
            intLastReportedFreq = intFreq
        End If
    End Sub

    Private Sub tmrPTTDblOff_Elapsed(sender As Object, e As System.Timers.ElapsedEventArgs) Handles tmrPTTDblOff.Elapsed
        tmrPTTDblOff.Stop()
        Dim dttStartWait As Date = Now
        While blnInPTTCAT And Now.Subtract(dttStartWait).TotalMilliseconds < 1000 ' safety mechanism should not ever be in PTTCAT
            Thread.Sleep(20)
        End While
        PTT(False) ' issue a backup PTT off
    End Sub
End Class