Imports System.Math
Imports System.IO
Imports System.Threading
Imports Microsoft.DirectX
Imports Microsoft.DirectX.DirectSound


Public Class Main
   
    ' Objects, classes & forms
    Private objCodecLock As New Object
    Public objMod As EncodeModulate
    Public objDemod As New DemodulateDecode12K(Me)
    ' Private objTestReceive As New TestReceive(Me) ' for testing and place holder for protocol class
    Public objProtocol As New ARDOPprotocol(Me) ' Final protocol implementation.
    Private objBusy As New BusyDetector(Me)
    Private frmTest As Test
    Public WithEvents objHI As New HostInterface(Me)
    Public WithEvents objRadio As Radio = Nothing
    ' Sound Card/Direct Sound 
    Public intCaptureBufferSize As Int32
    Private intNextCaptureOffset As Int32
    
    Private intNotifySize As Int32 = 2048 ' 2048 bytes or 1024 16 bit samples (nominally every 85.3333 ms @ 12000 sample rate)
    ' May be possible to go down to 1024 byte buffer which reduces the latency by 43 ms
    'Private intNotifySize As Int32 = 1024 ' 1024 bytes or 512 16 bit samples (nominally every 42.666 ms @ 12000 sample rate)

    Private bufPlayback As SecondaryBuffer = Nothing
    Private cllCaptureDevices As CaptureDevicesCollection
    Private cllPlaybackDevices As Microsoft.DirectX.DirectSound.DevicesCollection
    Private devCaptureDevice As Capture
    Private devSelectedPlaybackDevice As Microsoft.DirectX.DirectSound.Device
    Private objApplicationNotify As Notify = Nothing
    Private objCapture As CaptureBuffer = Nothing
    Private objCaptureDeviceGuid As Guid = Guid.Empty
    Public objNotificationEvent As AutoResetEvent = Nothing
    Public objPlayback As SecondaryBuffer = Nothing
    Private objPlaybackDeviceGuid As Guid = Guid.Empty
    Private stcPositionNotify(intNumberRecordNotifications) As BufferPositionNotify
    Private stcSCFormat As WaveFormat
    ' Strings
    Private strTCPIPConnectionID As String

    ' Integers
    Private intRepeatCnt As Int32
    Private Const intNumberRecordNotifications As Int32 = 32 '(creates appox ~2.73 second circular buffer)
    Private intBMPSpectrumWidth As Int32 = 256
    Private intBMPSpectrumHeight As Int32 = 62

    'Booleans
    Public blnSCCapturing As Boolean
    Public blnCodecStarted As Boolean
    Public blnInTestMode As Boolean = False
    Public blnClosing As Boolean = False
    Public blnGraphicsCleared As Boolean = False
    Private blnFramePending As Boolean = False
    Public blnLastPTT As Boolean = False
    Private blnSoundStreamPlay As Boolean = False

    ' Doubles
    Private dblPhase As Double

    'Dates
    Private dttNextPlay As Date
    Public dttLastSoundCardSample As Date
    Public dttCodecStarted As Date
    Private dttNextGraphicsReset As Date
    ' Arrays
    Private dblCarFreq() As Double
    Private bytToSend() As Byte
    Private bytSymToSend() As Byte
    Private intSamples() As Int32
    Private dblPhaseInc() As Double
    Private dblCPPhaseOffset() As Double
    Private bytHostIBData_CmdBuffer(-1) As Byte

    ' Graphics
    Private bmpSpectrum As Bitmap
    Private bmpNewSpectrum As Bitmap
    Private graConstellation As Graphics
    Private graFrequency As Graphics
    Private destRect1 As Rectangle
    Private destRect2 As Rectangle

    ' Threads
    Public thrNotify As Thread = Nothing  ' Notification thread for capturing data in the sound card

    ' Structures
    Private Structure DeviceDescription
        Dim info As DeviceInformation
        Overrides Function ToString() As String
            Return info.Description
        End Function
        Sub New(ByVal d As DeviceInformation)
            info = d
        End Sub
    End Structure

    ' Properties
    Public ReadOnly Property SoundIsPlaying As Boolean
        Get
            If ALSA Then
                Return ALSASoundIsPlaying()
            Else
                If IsNothing(objPlayback) OrElse objPlayback.Status.Playing = False Then Return False
            End If
            Return True
        End Get
    End Property

    Public ReadOnly Property CaptureDevices() As String
        Get
            Dim strDevices As String = ""
            If IsNothing(cllCaptureDevices) Then
                cllCaptureDevices = New CaptureDevicesCollection
            End If
            cllCaptureDevices.Reset()
            For i As Integer = 0 To cllCaptureDevices.Count - 1
                strDevices &= cllCaptureDevices(i).Description.ToString.Trim & "-" & cllCaptureDevices(i).DriverGuid.ToString.Substring(cllCaptureDevices(i).DriverGuid.ToString.Length - 2) & ","
            Next i
            Return strDevices.Substring(0, strDevices.Length - 1).ToUpper
        End Get
    End Property

    Public ReadOnly Property PlaybackDevices() As String
        Get
            Dim strDevices As String = ""
            cllPlaybackDevices.Reset()
            For i As Integer = 0 To cllPlaybackDevices.Count - 1
                strDevices &= cllPlaybackDevices(i).Description.ToString.Trim & "-" & cllPlaybackDevices(i).DriverGuid.ToString.Substring(cllPlaybackDevices(i).DriverGuid.ToString.Length - 2) & ","
            Next i
            Return strDevices.Substring(0, strDevices.Length - 1).ToUpper
        End Get
    End Property

    '  Subroutine to initialze parameters from the ini file
    Private Sub InitializeFromIni()
        objIniFile.Load()

        MCB.Callsign = objIniFile.GetString("ARDOP_Win TNC", "Callsign", "")
        MCB.StartMinimized = CBool(objIniFile.GetString("ARDOP_Win TNC", "StartMinimized", "False"))
        MCB.DebugLog = CBool(objIniFile.GetString("ARDOP_Win TNC", "DebugLog", "True"))
        MCB.CommandTrace = CBool(objIniFile.GetString("ARDOP_Win TNC", "CommandTrace", "False"))
        MCB.CaptureDevice = objIniFile.GetString("ARDOP_Win TNC", "CaptureDevice", "")
        MCB.PlaybackDevice = objIniFile.GetString("ARDOP_Win TNC", "PlaybackDevice", "")
        MCB.LeaderLength = objIniFile.GetInteger("ARDOP_Win TNC", "LeaderLength", 160)
        MCB.TrailerLength = objIniFile.GetInteger("ARDOP_Win TNC", "TrailerLength", 0)
        MCB.ARQBandwidth = objIniFile.GetString("ARDOP_Win TNC", "ARQBandwidth", "500MAX")
        MCB.ProtocolMode = objIniFile.GetString("ARDOP_Win TNC", "Protoco Mode", "FEC")
        MCB.DriveLevel = objIniFile.GetInteger("ARDOP_Win TNC", "DriveLevel", 90)
        MCB.Squelch = objIniFile.GetInteger("ARDOP_Win TNC", "Squelch", 5)
        MCB.AccumulateStats = CBool(objIniFile.GetString("ARDOP_Win TNC", "Accum Stats", "True"))
        MCB.DisplayWaterfall = CBool(objIniFile.GetString("ARDOP_Win TNC", "Display Waterfall", "True"))
        MCB.DisplaySpectrum = CBool(objIniFile.GetString("ARDOP_Win TNC", "Display Spectrum", "False"))
        MCB.SecureHostLogin = CBool(objIniFile.GetString("ARDOP_Win TNC", "SecureHostLogin", "False"))
        MCB.Password = objIniFile.GetString("ARDOP_Win TNC", "LoginPassword", "")
        MCB.TuningRange = objIniFile.GetInteger("ARDOP_Win TNC", "TuningRange", 100)
        MCB.FECRepeats = objIniFile.GetInteger("ARDOP_Win TNC", "FECRepeats", 2)
        MCB.FECMode = objIniFile.GetString("ARDOP_Win TNC", "FECMode", "4PSK.500.100")
        MCB.FECId = CBool(objIniFile.GetString("ARDOP_Win TNC", "FECId", "True"))
        MCB.CWID = CBool(objIniFile.GetString("ARDOP_Win TNC", "EnableCWID", "True"))
        MCB.ARQConReqRepeats = objIniFile.GetInteger("ARDOP_Win TNC", "ARQConReqRepeats", 5)
        RCB.RadioControl = CBool(objIniFile.GetString("Radio", "Enable Radio Control", "False"))
        MCB.ARQTimeout = objIniFile.GetInteger("ARDOP_Win TNC", "ARQTimeout", 120)
        If RCB.RadioControl Then

            RadioMenu.Enabled = True
            If IsNothing(objRadio) Then
                objRadio = New Radio(Me)
                objRadio.GetRadioSettings()
                objRadio.InitRadioPorts()
            End If
        Else
            'RadioMenu.Enabled = False
        End If
        ' This insures window placement is on screen independent of .ini values
        Dim screen As System.Windows.Forms.Screen() = System.Windows.Forms.Screen.AllScreens
        Dim blnLocOK As Boolean = False
        Dim intTop, intLeft As Int32
        ' Set inital window position and size...
        intTop = objIniFile.GetInteger("ARDOP_Win TNC", "Top", 100)
        intLeft = objIniFile.GetInteger("ARDOP_Win TNC", "Left", 100)
        For i As Integer = 0 To screen.Length - 1
            If screen(i).Bounds.Top <= intTop And _
               screen(i).Bounds.Bottom >= (intTop + Me.Height) And _
               screen(i).Bounds.Left <= intLeft And _
               screen(i).Bounds.Right >= (intLeft + Me.Width) Then
                ' Position window in its last location only if it is within the bounds of the screen
                Me.Top = intTop
                Me.Left = intLeft
                blnLocOK = True
                Exit For
            End If
        Next
        Me.Text = "ARDOP_Win Virtual TNC Ver: " & Application.ProductVersion.ToString & " BPQ"
        ClearTuningStats()
        ClearQualityStats()
        InitializeConnection()
    End Sub  'InitializeFromIni

    ' Subroutine to establish the objRadio instance
    Public Sub SetNewRadio()
        objRadio = New Radio(Me)
    End Sub 'SetNewRadio

    ' Subroutine to close te Radio instance
    Public Sub CloseRadio()
        objRadio = Nothing
    End Sub
    Private Sub Main_FormClosed(sender As Object, e As System.Windows.Forms.FormClosedEventArgs) Handles Me.FormClosed
        End
    End Sub 'CloseRadio

    ' Subroutine to cleanup before closing the main form
    Private Sub Main_FormClosing(sender As Object, e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing
        If Not IsNothing(objRadio) Then
            objRadio.CloseRadio()
            objRadio = Nothing
        End If
        objHI.TerminateHostLink()
        Dim strFault As String = ""
        StopCodec(strFault)
        If strFault.Length > 0 Then Logs.Exception("[Main_FormClosing] Fault: " & strFault)
        If Me.WindowState = FormWindowState.Normal Then
            Try
                objIniFile.WriteInteger("ARDOP_Win TNC", "Top", Me.Top)
                objIniFile.WriteInteger("ARDOP_Win TNC", "Left", Me.Left)
            Catch ex As Exception
                Logs.Exception("[ARDOP_Win TNC form Closed]: " & ex.ToString)
            End Try
        End If
        objIniFile.Flush()
    End Sub  ' Main_FormClosing

    ' Subroutine event Main_Load
    Private Sub Main_Load(sender As Object, e As System.EventArgs) Handles Me.Load

        ' See if ALSA is available. 

        ALSA = CheckforALSA()

        ' Create subdirectories as required...
        If Directory.Exists(strExecutionDirectory & "Logs") = False Then Directory.CreateDirectory(strExecutionDirectory & "Logs")
        If Directory.Exists(strExecutionDirectory & "Wav") = False Then Directory.CreateDirectory(strExecutionDirectory & "Wav")
        strWavDirectory = strExecutionDirectory & "Wav\"
        ' Set inital window position and size...
        objIniFile = New INIFile(strExecutionDirectory & "ARDOP_Win TNC.ini")
        InitializeFromIni()
        objMod = New EncodeModulate(Me) ' initializes all the leaders etc. 
        dttLastSoundCardSample = Now
        tmrPoll.Start()
        Dim strFault As String = ""
        dttNextGraphicsReset = Now.AddMinutes(60)
        'Dim objPA As New PortAudioVB '  Enable only for testing PortAudio

        ShowTestFormMenuItem.Enabled = strExecutionDirectory.ToLower.IndexOf("debug") <> -1

        ' This keeps the size of the graphics panels constant to handle cases where font size (in Control Panel, Display is 125% or 150% and recenters the waterfall panel. 
        Dim dpWaterfallCorner As Drawing.Point = pnlWaterfall.Location
        pnlWaterfall.Left = (dpWaterfallCorner.X + pnlWaterfall.Width \ 2) - 105
        pnlWaterfall.Height = 63
        pnlWaterfall.Width = 210
        pnlConstellation.Height = 91
        pnlConstellation.Width = 91
        Logs.WriteDebug(" ")
        Logs.WriteDebug("[ARDOP_Win TNC.Main.Load] Command line =" & Microsoft.VisualBasic.Command().Trim)

        If My.Application.CommandLineArgs.Count = 0 Then
            '    If Microsoft.VisualBasic.Command().Trim = "" Then ' nothing in command line so use ini values
            tmrStartCODEC.Interval = 2000 ' use short interval if not started with a command line.
            tmrStartCODEC.Start()
            ' use ini file values for host and port
            MCB.TCPAddress = objIniFile.GetString("ARDOP_Win TNC", "TCPIP Address", "127.0.0.1")
            MCB.TCPPort = objIniFile.GetInteger("ARDOP_Win TNC", "TCPIP Port", 8515)
            MCB.HostTCPIP = CBool(objIniFile.GetString("ARDOP_Win TNC", "HostTCPIP", "True"))
            If MCB.HostTCPIP Then objHI.TCPIPProperties(MCB.TCPAddress, MCB.TCPPort)
            '
            MCB.HostSerial = CBool(objIniFile.GetString("ARDOP_Win TNC", "HostSerial", "False"))
            MCB.SerCOMPort = objIniFile.GetString("ARDOP_Win TNC", "SerialCOMPort", "none")
            MCB.SerBaud = objIniFile.GetInteger("ARDOP_Win TNC", "SerialBaud", 19200)
            If MCB.HostSerial Then objHI.SerialProperties(MCB.SerCOMPort, MCB.SerBaud)
            '
            MCB.HostBlueTooth = CBool(objIniFile.GetString("ARDOP_Win TNC", "HostBlueTooth", "False"))
            MCB.HostPairing = objIniFile.GetString("ARDOP_Win TNC", "Host Pairing", "none")
            If MCB.HostBlueTooth Then objHI.BluetoothProperties(MCB.HostPairing)
            CloseMenuItem.Enabled = True
        Else
            tmrStartCODEC.Interval = 5000 ' give 5 sec for TNC startup from host before doing auto start. 
            tmrStartCODEC.Start()
            ' test command line parameters for validity: "TCPIP TCPIPPort# TCPIPAddress", Serial POrt,   or  "BLUETOOTH BlueToothPairing"

            If My.Application.CommandLineArgs.Count = 3 AndAlso My.Application.CommandLineArgs(0).ToUpper = "TCPIP" And IsNumeric(My.Application.CommandLineArgs(1).Trim) And CInt(My.Application.CommandLineArgs(1).Trim) >= 0 And CInt(My.Application.CommandLineArgs(1).Trim) < 65536 Then
                ' TCPIP parameters OK so use these in place of ini values
                MCB.HostTCPIP = True
                MCB.HostSerial = False
                MCB.HostBlueTooth = False
                objHI.TCPIPProperties(My.Application.CommandLineArgs(2).Trim, CInt(My.Application.CommandLineArgs(1).Trim))
                MCB.TCPPort = CInt(My.Application.CommandLineArgs(1).Trim)
                MCB.TCPAddress = My.Application.CommandLineArgs(2).Trim
            ElseIf My.Application.CommandLineArgs.Count = 3 AndAlso (My.Application.CommandLineArgs(0).ToUpper = "SERIAL" And IsNumeric(My.Application.CommandLineArgs(1).Trim) And (CInt(My.Application.CommandLineArgs(1).Trim) >= 9600)) Then
                MCB.HostTCPIP = False
                MCB.HostSerial = True
                MCB.HostBlueTooth = False
                objHI.SerialProperties(My.Application.CommandLineArgs(1).Trim.ToUpper, CInt(My.Application.CommandLineArgs(2).Trim))
                MCB.SerCOMPort = My.Application.CommandLineArgs(1).Trim.ToUpper
                MCB.SerBaud = CInt(My.Application.CommandLineArgs(2).Trim)
            ElseIf My.Application.CommandLineArgs.Count = 2 AndAlso My.Application.CommandLineArgs(0).ToUpper = "BLUETOOTH" Then ' Preliminay ....may need work for bluetooth
                MCB.HostTCPIP = False
                MCB.HostSerial = False
                MCB.HostBlueTooth = True
                objHI.BluetoothProperties(My.Application.CommandLineArgs(1))
                MCB.HostPairing = My.Application.CommandLineArgs(1)
            Else
                Logs.Exception("[Main.Load] Syntax error in command line: " & Microsoft.VisualBasic.Command() & "   ... ini file values used.")
            End If
            CloseMenuItem.Enabled = False
        End If
        objHI.EnableHostLink()
    End Sub  ' Main_Load

    ' Subroutine for ToolStripMenuItem_Click
    Private Sub BasicSetupToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles SetupMenu.Click
        Dim dlgSetup As New TNCSetup
        Dim strFault As String = ""
        dlgSetup.ShowDialog()
        If (dlgSetup.DialogResult = Windows.Forms.DialogResult.OK) Or (Now.Subtract(dttLastSoundCardSample).TotalSeconds > 5) Then
            If MCB.DebugLog Then Logs.WriteDebug("[BasicSetupDialog] Restarting Codecs")
            If StopCodec(strFault) Then
                strFault = ""
                If Not StartCodec(strFault) Then
                    Logs.Exception("[BasicSetupDialog] Failure to restart Codec after setup change. Fault: " & strFault)
                End If
            Else
                Logs.Exception("[BasicSetupDialog] Failure to stop Codec after setup change. Fault: " & strFault)
            End If
        End If

        If dlgSetup.DialogResult = Windows.Forms.DialogResult.Ignore Or dlgSetup.DialogResult = Windows.Forms.DialogResult.OK Then
            If IsNothing(objMod) Then objMod = New EncodeModulate(Me)
            RadioMenu.Enabled = RCB.RadioControl
            If RCB.RadioControl Then
                If IsNothing(objRadio) Then
                    objRadio = New Radio(Me)
                    objRadio.InitRadioPorts()
                End If
            End If
        End If
    End Sub '  BasicSetupToolStripMenuItem_Click

    '   Subroutine for tmrPoll_Tick Event
    Private Sub tmrPoll_Tick(sender As Object, e As System.EventArgs) Handles tmrPoll.Tick
        tmrPoll.Stop()
        If MainPoll() Then tmrPoll.Start()
    End Sub 'tmrPoll_Tick(sender

    ' Main polling Function returns True or False if closing 
    Private Function MainPoll() As Boolean
        Dim stcStatus As Status = Nothing

        ' Check the sound card to insure still sampling
        If (Now.Subtract(dttLastSoundCardSample).TotalSeconds > 10) And objProtocol.GetARDOPProtocolState() <> ProtocolState.OFFLINE Then
            tmrStartCODEC.Interval = 1000
            dttLastSoundCardSample = Now
            Logs.Exception("[tmrPoll_Tick] > 10 seconds with no sound card samples...Restarting Codec")
            tmrStartCODEC.Start() ' Start the timer to retry starting sound card
        End If
        Try ' was getting rare unhandled exceptions in this area May 7, 2015

            ' This handles the normal condition of going from Sending (Playback) to Receiving (Recording)
            If (ALSA = False AndAlso blnSoundStreamPlay AndAlso Not SoundIsPlaying) Or (ALSA AndAlso PlayComplete) Then

                If (ALSA) Then
                    PlayComplete = False
                End If
                'Debug.WriteLine("[tmrPoll.Tick] Play stop. Length = " & Format(Now.Subtract(dttTestStart).TotalMilliseconds, "#") & " ms")
                If objPlayback IsNot Nothing Then

                    objPlayback.Dispose() ' added 0.1.4.2
                    objPlayback = Nothing
                End If

                blnSoundStreamPlay = False
                If blnInTestMode Then
                    intRepeatCnt -= 1
                    Try
                        frmTest.UpdateFrameCounter(intRepeatCnt)
                    Catch
                    End Try
                    '
                    If intRepeatCnt > 0 Then
                        dttNextPlay = Now.AddSeconds(2)
                    ElseIf MCB.AccumulateStats Then
                        tmrLogStats.Start()
                    End If
                End If
                KeyPTT(False) ' Unkey the Transmitter
                ' clear the transmit label 
                stcStatus.BackColor = SystemColors.Control
                stcStatus.ControlName = "lblXmtFrame" ' clear the transmit label
                queTNCStatus.Enqueue(stcStatus)
                stcStatus.ControlName = "lblRcvFrame" ' clear the Receive label
                queTNCStatus.Enqueue(stcStatus)
                ' Check if the protocol has a frame to send
                If objProtocol.GetNextFrame Then
                    dttNextPlay = Now.AddMilliseconds(objProtocol.intFrameRepeatInterval)
                    'Debug.WriteLine("[Main.tmrPoll] Frame Pending")
                    blnFramePending = True
                Else
                    'Debug.WriteLine("[Main.tmrPoll] No Frame Pending")
                    blnFramePending = False
                End If
                ' Repeat mechanism for Test mode
            ElseIf blnInTestMode And (Not SoundIsPlaying) And intRepeatCnt > 0 And Now.Subtract(dttNextPlay).TotalMilliseconds > 0 Then
                State = ReceiveState.SearchingForLeader
                PlaySoundStream()
                'Repeat mechanism for normal repeated FEC or ARQ frames
            ElseIf (Not SoundIsPlaying) And blnFramePending And Now.Subtract(dttNextPlay).TotalMilliseconds > 0 Then
                PlaySoundStream()
                blnFramePending = False
                ' Checks to see if frame ready for playing
                'ElseIf (IsNothing(objPlayback)) And Not blnFramePending Then
            ElseIf (Not SoundIsPlaying) And Not blnFramePending Then
                If objProtocol.GetNextFrame Then
                    blnFramePending = True
                    dttNextPlay = Now.AddMilliseconds(objProtocol.intFrameRepeatInterval)
                End If
            End If
            ' Update Enable/disable for the Send ID menu item
            If Not SoundIsPlaying Then
                SendIDToolStripMenuItem.Enabled = objProtocol.GetARDOPProtocolState = ProtocolState.DISC
            Else
                SendIDToolStripMenuItem.Enabled = False
            End If
            ' Update any form Main display items from the TNCStatus queue
            While queTNCStatus.Count > 0
                Try
                    stcStatus = CType(queTNCStatus.Dequeue, Status)
                    Select Case stcStatus.ControlName
                        ' Receive controls:
                        Case "lblQuality"
                            lblQuality.Text = stcStatus.Text
                        Case "ConstellationPlot"
                            DisplayPlot()
                            intRepeatCnt += 0
                        Case "lblXmtFrame"
                            lblXmtFrame.Text = stcStatus.Text
                            lblXmtFrame.BackColor = stcStatus.BackColor
                        Case "lblRcvFrame"
                            lblRcvFrame.Text = stcStatus.Text
                            lblRcvFrame.BackColor = stcStatus.BackColor
                        Case "prgReceiveLevel"
                            prgReceiveLevel.Value = stcStatus.Value
                            If stcStatus.Value < 64 Then ' < 12% of Full scale (16 bit A/D)
                                prgReceiveLevel.ForeColor = Color.SkyBlue
                            ElseIf stcStatus.Value > 170 Then ' > 88% of full scale (16 bit A/D)
                                prgReceiveLevel.ForeColor = Color.LightSalmon
                            Else
                                prgReceiveLevel.ForeColor = Color.LightGreen
                            End If
                        Case "lblOffset"
                            lblOffset.Text = stcStatus.Text
                        Case "lblHost"
                            If stcStatus.Text <> "" Then lblHost.Text = stcStatus.Text
                            lblHost.BackColor = stcStatus.BackColor
                        Case "lblState"
                            lblState.Text = stcStatus.Text
                            lblState.BackColor = stcStatus.BackColor
                        Case "lblCF"
                            lblCF.Text = stcStatus.Text
                        Case "mnuBusy"
                            If stcStatus.Text.ToUpper = "TRUE" Or stcStatus.Text.ToUpper = "FALSE" Then
                                ChannelBusyToolStripMenuItem.Text = "Channel Busy"
                                ChannelBusyToolStripMenuItem.Visible = CBool(stcStatus.Text)
                            Else
                                ChannelBusyToolStripMenuItem.Text = stcStatus.Text
                                ChannelBusyToolStripMenuItem.Visible = True
                            End If
                    End Select
                Catch
                    Logs.Exception("[Main.tmrPoll.Tick] queTNCStatus Err: " & Err.Description)
                    Exit While
                End Try
            End While
            ' This hourly periodic shut down and restart of the graphics is used to handle a memory leak prevalent on some 
            ' Graphic chipsets  e.g. Intel G41 and possibly other.
            If Now.Subtract(dttNextGraphicsReset).TotalSeconds > 0 Then
                Dim blnSpectrum As Boolean = MCB.DisplaySpectrum
                Dim blnWaterfall As Boolean = MCB.DisplayWaterfall
                MCB.DisplaySpectrum = False
                MCB.DisplayWaterfall = False
                InitializeGraphics()
                Thread.Sleep(1000)
                intSavedTuneLineHi = 0
                intSavedTuneLineLow = 0
                MCB.DisplaySpectrum = blnSpectrum
                ClearSpectrum()
                MCB.DisplayWaterfall = blnWaterfall
                dttNextGraphicsReset = Now.AddMinutes(60)
            End If
        Catch ex As Exception
            Logs.Exception("[Main.tmrPollTick] Err: " & ex.ToString)
        End Try

        If blnClosing Then ' Check for closing
            Me.Close()
            Return False
        Else
            Return True
        End If
    End Function 'MainPoll

    ' Subroutine to repaint the FSK Quality Plot
    Private Sub DisplayPlot()
        Try
            If Not IsNothing(graConstellation) Then
                graConstellation.Dispose()
            End If
            graConstellation = pnlConstellation.CreateGraphics
            graConstellation.Clear(Color.Black)
            If Not IsNothing(bmpConstellation) Then
                graConstellation.DrawImage(bmpConstellation, 0, 0) ' Display the constellation
            End If
        Catch ex As Exception

        End Try
    End Sub  'DisplayPlot

    ' Subroutine to repaint the Constellation graphic
    Private Sub DisplayConstellation()
        Dim Trace As Integer = 0
        If Me.WindowState = FormWindowState.Minimized Then Exit Sub
        Trace = 1
        Try
            If Not IsNothing(graConstellation) Then
                Trace = 2
                graConstellation.Dispose()
            End If
            Trace = 3
            graConstellation = pnlConstellation.CreateGraphics
            Trace = 4
            graConstellation.Clear(Color.Black)
            Trace = 5
            If Not IsNothing(bmpConstellation) Then
                Trace = 6
                graConstellation.DrawImage(bmpConstellation, 0, 0) ' Display the constellation
            End If
        Catch ex As Exception
            Logs.Exception("[DisplayConstellation] Trace=" & Trace.ToString & "  Err: " & ex.ToString)
        End Try
    End Sub ' DisplayConstellation

    ' Function to play the Wave stream, updates the form display and keys the PTT.
    Public Function PlaySoundStream() As Boolean
        '   Plays the .wav stream with the selected Playback device
        '   Returns True if no errors False otherwise...

        If (ALSA) Then
            Return AlsaPlaySoundStream()
        End If
        Dim intTrace As Integer = 0
        Dim dttStartPlay As Date
        Dim stcStatus As Status = Nothing
        Static bufPlaybackFlags As New Microsoft.DirectX.DirectSound.BufferDescription

        If Not blnCodecStarted Then Return False
        If IsNothing(memWaveStream) Then
            Logs.Exception("[Main.PlaySoundStream] memWaveStream is nothing")
            Return False
        ElseIf SoundIsPlaying Then
            Logs.Exception("[Main.PlaySoundStream] Sound is playing, Call AbortWaveStream")
            AbortSoundStream()
        End If
        intTrace = 1
        Try
            devSelectedPlaybackDevice.SetCooperativeLevel(Me.Handle, CooperativeLevel.Priority)
            KeyPTT(True) ' Activate PTT before starting sound play
            If Not bufPlaybackFlags.ControlVolume Then
                ' The following flags required to allow playing when not in focus and 
                ' to allow adjusting the volume...
                intTrace = 2
                bufPlaybackFlags.Flags = CType(BufferDescriptionFlags.GlobalFocus + BufferDescriptionFlags.ControlVolume, BufferDescriptionFlags)
            End If
            intTrace = 3
            memWaveStream.Seek(0, SeekOrigin.Begin) ' reset the pointer to the origin
            intTrace = 4
            If Not IsNothing(objPlayback) Then
                objPlayback.Dispose()
                objPlayback = Nothing
            End If
            objPlayback = New SecondaryBuffer(memWaveStream, bufPlaybackFlags, devSelectedPlaybackDevice)
            intTrace = 5
            objPlayback.Volume = Math.Min(-5000 + 50 * MCB.DriveLevel, 0)  ' -5000=off, 0=full volume
            intTrace = 6
            objPlayback.Play(0, BufferPlayFlags.Default)
            intTrace = 7
            dttTestStart = Now
            dttStartPlay = Now
            intTrace = 8
            If MCB.DebugLog Then Logs.WriteDebug("[PlaySoundStream] Stream: " & strLastWavStream)
            stcStatus.ControlName = "lblXmtFrame"
            stcStatus.Text = strLastWavStream
            stcStatus.BackColor = Color.LightSalmon
            queTNCStatus.Enqueue(stcStatus)
            intTrace = 9
            PlaySoundStream = True
            blnSoundStreamPlay = True
        Catch e As Exception
            Logs.Exception("[Main.PlaySoundStream] Trace =" & intTrace.ToString & "  Kill PTT on exception: " & e.ToString)
            If Not IsNothing(memWaveStream) Then
                Logs.Exception("[Main.PlaySoundStream] Trace =" & intTrace.ToString & "  memWaveStream Length =" & memWaveStream.Length.ToString & "  strLastWaveStream=" & strLastWavStream)
            Else
                Logs.Exception("[Main.PlaySoundStream] Trace =" & intTrace.ToString & "  memWaveStream was Nothing  strLastWavStream=" & strLastWavStream)
            End If
            KeyPTT(False)
            PlaySoundStream = False
            blnSoundStreamPlay = False
        End Try
    End Function  'PlaySoundStream

    '  Function to send PTT TRUE or PTT FALSE comannad to Host or if local Radio control Keys radio PTT 
    Public Function KeyPTT(ByVal blnPTT As Boolean) As Boolean
        '  Returns TRUE if successful False otherwise
        If (blnLastPTT And Not blnPTT) Then objProtocol.dttStartRTMeasure = Now ' start a measurement on release of PTT.
        If Not RCB.RadioControl Then
            objHI.QueueCommandToHost("PTT " & blnPTT.ToString.ToUpper)
        ElseIf Not (IsNothing(objRadio)) Then
            objRadio.PTT(blnPTT)
        End If
        If MCB.DebugLog Then Logs.WriteDebug("[Main.KeyPTT]  PTT-" & blnPTT.ToString)
        blnLastPTT = blnPTT
        Return True
    End Function 'KeyPTT

    ' Function to start the Codec (sound card capture)
    Public Function StartCodec(ByRef strFault As String) As Boolean

        If ALSA Then
            Return AlsaStartCodec(strFault)
        End If

        'Returns true if successful
        Thread.Sleep(100) ' This delay is necessary for reliable starup following a StopCodec
        SyncLock objCodecLock
            dttLastSoundCardSample = Now
            Dim blnSpectrumSave As Boolean = MCB.DisplaySpectrum
            Dim blnWaterfallSave As Boolean = MCB.DisplayWaterfall
            Dim dttStartWait As Date = Now
            MCB.DisplayWaterfall = False
            MCB.DisplaySpectrum = False
            Dim strCaptureDevices() As String = EnumerateCaptureDevices()
            Dim strPlaybackDevices() As String = EnumeratePlaybackDevices()
            StartCodec = False
            Dim objDI As New DeviceInformation
            Dim intPtr As Integer = 0
            ' Playback devices
            Try
                cllPlaybackDevices = Nothing
                cllPlaybackDevices = New Microsoft.DirectX.DirectSound.DevicesCollection
                If Not IsNothing(devSelectedPlaybackDevice) Then
                    devSelectedPlaybackDevice.Dispose()
                    devSelectedPlaybackDevice = Nothing
                End If
                For Each objDI In cllPlaybackDevices
                    Dim objDD As New DeviceDescription(objDI)
                    If strPlaybackDevices(intPtr).ToUpper = MCB.PlaybackDevice.ToUpper Then
                        If MCB.DebugLog Then Logs.WriteDebug("[Main.StartCodec] Setting SelectedPlaybackDevice = " & MCB.PlaybackDevice)
                        devSelectedPlaybackDevice = New Device(objDD.info.DriverGuid)
                        StartCodec = True
                        Exit For
                    End If
                    intPtr += 1
                Next objDI
                If Not StartCodec Then
                    strFault = "Playback Device setup, Device " & MCB.PlaybackDevice & " not found in Windows enumerated Playback Devices"
                End If
            Catch ex As Exception
                strFault = "Start Codec: " & Err.Number.ToString & "/" & Err.Description
                Logs.Exception("[StartCodec], Playback Device setup] Err: " & ex.ToString)
                StartCodec = False
            End Try
            If StartCodec Then
                ' Capture Device
                Dim dscheckboxd As New CaptureBufferDescription
                Try
                    StartCodec = False
                    cllCaptureDevices = Nothing
                    cllCaptureDevices = New CaptureDevicesCollection
                    intPtr = 0
                    For i As Integer = 0 To cllCaptureDevices.Count - 1
                        If MCB.CaptureDevice.ToUpper = strCaptureDevices(i).ToUpper Then
                            objCaptureDeviceGuid = cllCaptureDevices(i).DriverGuid
                            devCaptureDevice = New Capture(objCaptureDeviceGuid)
                            stcSCFormat.SamplesPerSecond = 12000   ' 12000 Hz sample rate 
                            stcSCFormat.Channels = 1
                            stcSCFormat.BitsPerSample = 16
                            stcSCFormat.BlockAlign = 2
                            stcSCFormat.AverageBytesPerSecond = 2 * 12000 ' 12000
                            stcSCFormat.FormatTag = WaveFormatTag.Pcm
                            objApplicationNotify = Nothing
                            objCapture = Nothing
                            ' Set the buffer sizes
                            intCaptureBufferSize = intNotifySize * intNumberRecordNotifications
                            ' Create the capture buffer
                            dscheckboxd.BufferBytes = intCaptureBufferSize
                            stcSCFormat.FormatTag = WaveFormatTag.Pcm
                            dscheckboxd.Format = stcSCFormat ' Set the format during creatation
                            If Not IsNothing(objCapture) Then
                                objCapture.Dispose()
                                objCapture = Nothing
                            End If
                            intNextCaptureOffset = 0
                            dttCodecStarted = Now
                            WriteTextToSpectrum("CODEC Start OK", Brushes.LightGreen)
                            blnGraphicsCleared = False
                            objCapture = New CaptureBuffer(dscheckboxd, devCaptureDevice)
                            InititializeNotifications()
                            objCapture.Start(True) ' start with looping
                            StartCodec = True
                        End If
                    Next i
                    If Not StartCodec Then
                        strFault = "Start Codec:Could not find DirectSound capture device " & MCB.CaptureDevice.ToUpper
                    End If
                Catch ex As Exception
                    strFault = "Start Codec: " & Err.Number.ToString & "/" & Err.Description
                    StartCodec = False
                End Try
            End If
            If StartCodec Then
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StartCodec] Successful start of codec")
                objProtocol.SetARDOPProtocolState(ProtocolState.DISC)
                If Not IsNothing(objRadio) Then objRadio.PTT(False) ' insurance for PTT off
                blnCodecStarted = True
                strFault = ""
            Else
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StartCodec] CODEC Start Failed")
                WriteTextToSpectrum("CODEC Start Failed", Brushes.Red)
                blnGraphicsCleared = False
                objProtocol.SetARDOPProtocolState(ProtocolState.OFFLINE)
                tmrStartCODEC.Interval = 5000
                tmrStartCODEC.Start()
                blnCodecStarted = False
            End If
            MCB.DisplayWaterfall = blnWaterfallSave
            MCB.DisplaySpectrum = blnSpectrumSave
        End SyncLock
    End Function  'StartCodec

    ' Function to stop the Codec (sound card capture)
    Public Function StopCodec(ByRef strFault As String) As Boolean

        If ALSA Then
            Return AlsaStopCodec(strFault)
        End If
        ' Stop the capture
        SyncLock objCodecLock
            Try
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] Stop thrNotify with blnSCCapturing = False")
                blnSCCapturing = False ' this should end the wait thread if it is still running
                Thread.Sleep(200)
                If thrNotify IsNot Nothing AndAlso thrNotify.IsAlive Then
                    If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] Aborting thrNotify")
                    thrNotify.Abort()
                    Thread.Sleep(100)
                    thrNotify.Join(3000)
                End If
                thrNotify = Nothing
                ' Stop the buffer
                If objCapture IsNot Nothing Then
                    objCapture.Stop()
                    objCapture.Dispose()
                End If
                objCapture = Nothing
                If devCaptureDevice IsNot Nothing Then devCaptureDevice.Dispose()
                devCaptureDevice = Nothing
                If devSelectedPlaybackDevice IsNot Nothing Then
                    devSelectedPlaybackDevice.Dispose()
                End If
                devSelectedPlaybackDevice = Nothing
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] = True")
                StopCodec = True
                WriteTextToSpectrum("CODEC Stopped", Brushes.Yellow)
                objProtocol.SetARDOPProtocolState(ProtocolState.OFFLINE)
                tmrStartCODEC.Stop()
                strFault = ""
            Catch ex As Exception
                Logs.Exception("[Main.StopCodec] Err: " & ex.ToString)
                If MCB.DebugLog Then Logs.WriteDebug("[Main.StopCodec] = False")
                strFault = ("StopCodec:" & Err.Number.ToString & "/" & Err.Description).Trim
                StopCodec = False
            End Try
        End SyncLock
    End Function  ' StopCodec

    ' Subroutine to initialize the Capture thread
    Private Sub InititializeNotifications()
        '  Subroutine to initialize the notifications on the capture buffer...
        If Nothing Is objCapture Then
            Logs.Exception("[Main.InitializeNotifications] Capture Buffer is Nothing")
            Return
        End If

        ' Create a thread to monitor the notify events
        If Nothing Is thrNotify Then
            thrNotify = New Thread(New ThreadStart(AddressOf CaptureWaitThread))
            thrNotify.Priority = ThreadPriority.Highest
            ' Create a notification event, for when the sound stops playing
            objNotificationEvent = New AutoResetEvent(False)
            thrNotify.Start()
        End If
        ' Setup the notification positions
        Dim i As Integer
        For i = 0 To intNUMBERRECORDNOTIFICATIONS - 1
            stcPositionNotify(i).Offset = intNotifySize * i + intNotifySize - 1
            ' The following recommended in place of "objNotificationEvent.Handle" as a more reliable handle 
            stcPositionNotify(i).EventNotifyHandle = objNotificationEvent.SafeWaitHandle.DangerousGetHandle
        Next i
        objApplicationNotify = New Notify(objCapture)

        ' Tell DirectSound when to notify the application. The notification will come in the from 
        ' of signaled events that are handled in the notify thread...
        objApplicationNotify.SetNotificationPositions(stcPositionNotify, intNUMBERRECORDNOTIFICATIONS)
    End Sub 'InititializeNotifications

    ' This is the main thread to capture sound card data which is also used as a polling function
    Public Sub CaptureWaitThread()
        ' It is active whenever the Sound card is enabled and sampling data
        ' It should receive a buffer notification every 960 samples/12000 sample rate  or every 80 ms...
        blnSCCapturing = True
        If MCB.DebugLog Then Logs.WriteDebug("[CaptureWaitThread] Startup")
        Try
            While blnSCCapturing
                '  Sit here and wait for a notification message to arrive
                '  this should be about every 85.3 ms or  (1024 samples @ 12000 Samples/sec)

                If ALSA Then
                    objNotificationEvent.WaitOne(83, True)
                    If ALSASoundIsPlaying() Then
                        '                AlsaClearInputSamples()     ' Discard input if sending
                    Else
                        ProcessCapturedData() ' Looks for incoming data and commands
                    End If
                Else
                    objNotificationEvent.WaitOne(-1, True)
                    ProcessCapturedData() ' Looks for incoming data and commands
                End If

            End While
        Catch ex As Exception
            Logs.Exception("[Main.CaptureWaitThread] Err: " & ex.ToString)
        End Try
        If MCB.DebugLog Then Logs.WriteDebug("[Main.CaptureWaitThread] Exit")
    End Sub ' CaptureWaitThread

    ' Subroutine to abort the sound stream playing
    Public Sub AbortSoundStream()

        If ALSA Then
            AlsaAbortSoundStream()
        Else

            If SoundIsPlaying Then
                Try
                    objPlayback.Stop()
                    blnSoundStreamPlay = False
                    KeyPTT(False)
                    objPlayback.Dispose() ' added 0.1.4.2
                    objPlayback = Nothing
                    'Logs.Exception("[Main.AbortSoundStream] Stream Stopped, PTT=False, and objPlayback set to Nothing")
                Catch ex As Exception
                    Logs.Exception("[Main.AbortSoundStream] Err: " & ex.ToString)
                End Try
            Else
                KeyPTT(False)
            End If
        End If

    End Sub  'AbortSoundStream



    ' Subroutine to Initialize the Spectrum Display
    Private Sub InititializeSpectrum(ByVal Color As System.Drawing.Color)
        Try
            If bmpSpectrum IsNot Nothing Then
                bmpSpectrum.Dispose()
                bmpSpectrum = Nothing
            End If
            graFrequency = pnlWaterfall.CreateGraphics
            graFrequency.Clear(Color)
            intBMPSpectrumWidth = 256
            intBMPSpectrumHeight = 62
            bmpSpectrum = New Bitmap(intBMPSpectrumWidth, intBMPSpectrumHeight)

            ' Set each pixel in bmpSpectrum to black.
            Dim intX As Integer
            For intX = 0 To intBMPSpectrumWidth - 1
                Dim intY As Integer
                For intY = 0 To intBMPSpectrumHeight - 1
                    If intX = 103 Then
                        bmpSpectrum.SetPixel(intX, intY, Color.Tomato)
                    Else
                        bmpSpectrum.SetPixel(intX, intY, Color.Black)
                    End If
                Next intY
            Next intX
        Catch
        End Try
    End Sub  'InititializeSpectrum

    ' Subroutine to initialize the Spectrum/Waterfall display to black. 
    Private Sub ClearSpectrum()
        Try
            If bmpSpectrum IsNot Nothing Then
                bmpSpectrum.Dispose()
                bmpSpectrum = Nothing
            End If
            graFrequency = pnlWaterfall.CreateGraphics
            graFrequency.Clear(Color.Black)
            intBMPSpectrumWidth = 256
            intBMPSpectrumHeight = 62
            bmpSpectrum = New Bitmap(intBMPSpectrumWidth, intBMPSpectrumHeight)
        Catch
        End Try
    End Sub  ' ClearSpectrum

    ' Subroutine to write text to the spectrum display
    Public Sub WriteTextToSpectrum(strText As String, objBrush As Brush)
        Try
            ClearSpectrum()
            Dim graComposit As Graphics = Graphics.FromImage(bmpSpectrum)
            Dim objFont As Font
            objFont = New System.Drawing.Font("Microsoft Sans Serif", 12, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
            graComposit.DrawString(strText, objFont, objBrush, 100 - 5 * strText.Length, 20)
            If Not IsNothing(graFrequency) Then
                graFrequency.Dispose() ' this permits writing back to the graFrequency without a GDI+ fault.
            End If

            graComposit = pnlWaterfall.CreateGraphics
            graComposit.DrawImage(bmpSpectrum, 0, 0) ' Draw the new bitmap in one update to avoid a possible GDI+ fault
        Catch ex As Exception
            Logs.Exception("[WriteTextToSpectrum] Err:  " & ex.ToString)
        End Try
    End Sub 'WriteTextToSpectrum

    ' Subroutine to Update the PhaseConstellation
    Friend Sub UpdatePhaseConstellation(ByRef intPhases() As Int16, ByRef intMag() As Int16, ByVal strMod As String, ByRef intQuality As Int32, Optional blnQAM As Boolean = False)
        ' Subroutine to update bmpConstellation plot for PSK modes...
        ' Skip plotting and calulations of intPSKPhase(0) as this is a reference phase (9/30/2014)
        Try
            Dim intPSKPhase As Integer = CInt(strMod.Substring(0, 1))
            Dim dblPhaseError As Double
            Dim dblPhaseErrorSum As Double
            Dim intPSKIndex As Int32
            Dim intX, intY, intP As Int32
            Dim dblRad As Double = 0
            Dim dblAvgRad As Double = 0
            Dim intMagMax As Double = 0
            Dim dblPi4 As Double = 0.25 * Math.PI
            Dim dbPhaseStep As Double = 2 * Math.PI / intPSKPhase
            Dim dblRadError As Double
            Dim dblPlotRotation As Double = 0
            Dim stcStatus As Status
            Dim yCenter, xCenter As Integer

            Select Case intPSKPhase
                Case 4 : intPSKIndex = 0
                Case 8 : intPSKIndex = 1
            End Select

            bmpConstellation = New Bitmap(89, 89)
            ' Draw the axis and paint the black background area
            yCenter = (bmpConstellation.Height - 1) \ 2
            xCenter = (bmpConstellation.Width - 1) \ 2
            For x As Integer = 0 To bmpConstellation.Width - 1
                For y As Integer = 0 To bmpConstellation.Height - 1
                    If y = yCenter Or x = xCenter Then
                        bmpConstellation.SetPixel(x, y, Color.Tomato)
                    End If
                Next y
            Next x

            For j As Integer = 1 To intMag.Length - 1  ' skip the magnitude of the reference in calculation
                intMagMax = Math.Max(intMagMax, intMag(j)) ' find the max magnitude to auto scale
                dblAvgRad += intMag(j)
            Next
            dblAvgRad = dblAvgRad / (intMag.Length - 1) ' the average radius
            ' For i As Integer = 0 To intPhases.Length - 1
            For i As Integer = 1 To intPhases.Length - 1 ' Don't plot the first phase (reference)
                dblRad = 40 * intMag(i) / intMagMax ' scale the radius dblRad based on intMagMax
                intX = CInt(xCenter + dblRad * Math.Cos(dblPlotRotation + intPhases(i) / 1000))
                intY = CInt(yCenter + dblRad * Math.Sin(dblPlotRotation + intPhases(i) / 1000))
                intP = CInt(Math.Round(0.001 * intPhases(i) / dbPhaseStep))
                ' compute the Phase and Raduius errors
                dblRadError += (dblAvgRad - intMag(i)) ^ 2
                dblPhaseError = Abs(((0.001 * intPhases(i)) - intP * dbPhaseStep)) ' always positive and < .5 *  dblPhaseStep
                dblPhaseErrorSum += dblPhaseError
                If intX <> xCenter And intY <> yCenter Then bmpConstellation.SetPixel(intX, intY, Color.Yellow) ' don't plot on top of axis
            Next i
            dblRadError = Sqrt(dblRadError / (intPhases.Length - 1)) / dblAvgRad
            If blnQAM Then
                ' include Radius error for QAM ...Lifted from WINMOR....may need work
                intQuality = CInt(Math.Max(0, (1 - dblRadError) * (100 - 200 * (dblPhaseErrorSum / (intPhases.Length - 1)) / dbPhaseStep)))
            Else
                ' This gives good quality with probable seccessful decoding threshold around quality value of 60 to 70 
                intQuality = Math.Max(0, CInt((100 - 200 * (dblPhaseErrorSum / (intPhases.Length - 1)) / dbPhaseStep))) ' ignore radius error for (PSK) but include for QAM
                'Debug.WriteLine("  Avg Radius Error: " & Format(dblRadError, "0.0"))
            End If

            If MCB.AccumulateStats Then
                stcQualityStats.intPSKQualityCnts(intPSKIndex) += 1
                stcQualityStats.intPSKQuality(intPSKIndex) += intQuality
                stcQualityStats.intPSKSymbolsDecoded += intPhases.Length
            End If
            stcStatus.ControlName = "lblQuality"
            stcStatus.Text = strMod & " Quality: " & intQuality.ToString
            queTNCStatus.Enqueue(stcStatus)
            stcStatus.ControlName = "ConstellationPlot"
            queTNCStatus.Enqueue(stcStatus)
        Catch ex As Exception
            Logs.Exception("[Main.UpdatePhaseConstellation] Err: " & ex.ToString)
        End Try
    End Sub  ' UpdatePhaseConstellation

    ' Subroutine to update the 4FSK Constellation
    Friend Sub Update4FSKConstellation(ByRef intToneMags() As Int32, ByRef intQuality As Int32)
        ' Subroutine to update bmpConstellation plot for 4FSK modes...
        If intToneMags.Length < 40 Then Exit Sub
        Try
            Dim dblRad As Double = 0
            Dim intToneSum As Int32 = 0
            Dim intMagMax As Double = 0
            Dim dblPi4 As Double = 0.25 * Math.PI
            Dim dblDistanceSum As Double
            Dim dblPlotRotation As Double = 0
            Dim stcStatus As Status
            Dim yCenter, xCenter As Integer
            Dim intRad As Int32
            Dim clrPixel As System.Drawing.Color
            bmpConstellation = New Bitmap(89, 89)
            ' Draw the axis and paint the black background area
            yCenter = (bmpConstellation.Height - 2) \ 2
            xCenter = (bmpConstellation.Width - 2) \ 2
            For x As Integer = 0 To bmpConstellation.Width - 1
                For y As Integer = 0 To bmpConstellation.Height - 1
                    If y = yCenter Or x = xCenter Then
                        bmpConstellation.SetPixel(x, y, Color.DeepSkyBlue)
                    End If
                Next y
            Next x
            For i As Integer = 0 To (intToneMags.Length - 1) Step 4  ' for the number of symbols represented by intToneMags
                intToneSum = intToneMags(i) + intToneMags(i + 1) + intToneMags(i + 2) + intToneMags(i + 3)
                If intToneMags(i) > intToneMags(i + 1) And intToneMags(i) > intToneMags(i + 2) And intToneMags(i) > intToneMags(i + 3) Then
                    If intToneSum > 0 Then
                        intRad = Max(5, 42 - 80 * (intToneMags(i + 1) + intToneMags(i + 2) + intToneMags(i + 3)) / intToneSum)
                        If intRad < 15 Then
                            clrPixel = Color.Tomato
                        ElseIf intRad < 30 Then
                            clrPixel = Color.Gold
                        Else
                            clrPixel = Color.Lime
                        End If
                        bmpConstellation.SetPixel(xCenter + intRad, yCenter + 1, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter + intRad, yCenter - 1, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter + intRad, yCenter + 2, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter + intRad, yCenter - 2, clrPixel) ' don't plot on top of axis
                    End If
                ElseIf intToneMags(i + 1) > intToneMags(i) And intToneMags(i + 1) > intToneMags(i + 2) And intToneMags(i + 1) > intToneMags(i + 3) Then
                    If intToneSum > 0 Then
                        intRad = Max(5, 42 - 80 * (intToneMags(i) + intToneMags(i + 2) + intToneMags(i + 3)) / intToneSum)
                        If intRad < 15 Then
                            clrPixel = Color.Tomato
                        ElseIf intRad < 30 Then
                            clrPixel = Color.Gold
                        Else
                            clrPixel = Color.Lime
                        End If
                        bmpConstellation.SetPixel(xCenter + 1, yCenter + intRad, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter - 1, yCenter + intRad, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter + 2, yCenter + intRad, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter - 2, yCenter + intRad, clrPixel) ' don't plot on top of axis
                    End If
                ElseIf intToneMags(i + 2) > intToneMags(i) And intToneMags(i + 2) > intToneMags(i + 1) And intToneMags(i + 2) > intToneMags(i + 3) Then
                    If intToneSum > 0 Then
                        intRad = Max(5, 42 - 80 * (intToneMags(i + 1) + intToneMags(i) + intToneMags(i + 3)) / intToneSum)
                        If intRad < 15 Then
                            clrPixel = Color.Tomato
                        ElseIf intRad < 30 Then
                            clrPixel = Color.Gold
                        Else
                            clrPixel = Color.Lime
                        End If
                        bmpConstellation.SetPixel(xCenter - intRad, yCenter + 1, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter - intRad, yCenter - 1, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter - intRad, yCenter + 2, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter - intRad, yCenter - 2, clrPixel) ' don't plot on top of axis
                    End If
                Else
                    If intToneSum > 0 Then
                        intRad = Max(5, 42 - 80 * (intToneMags(i + 1) + intToneMags(i + 2) + intToneMags(i)) / intToneSum)
                        If intRad < 15 Then
                            clrPixel = Color.Tomato
                        ElseIf intRad < 30 Then
                            clrPixel = Color.Gold
                        Else
                            clrPixel = Color.Lime
                        End If
                        bmpConstellation.SetPixel(xCenter + 1, yCenter - intRad, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter - 1, yCenter - intRad, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter + 2, yCenter - intRad, clrPixel) ' don't plot on top of axis
                        bmpConstellation.SetPixel(xCenter - 2, yCenter - intRad, clrPixel) ' don't plot on top of axis
                    End If
                End If
                dblDistanceSum += (43 - intRad)
            Next i
            intQuality = Math.Max(0, CInt(100 - 2.7 * (dblDistanceSum / (intToneMags.Length \ 4)))) ' factor 2.7 emperically chosen for calibration (Qual range 25 to 100)
            stcStatus.ControlName = "lblQuality"
            stcStatus.Text = "4FSK Quality: " & intQuality.ToString
            queTNCStatus.Enqueue(stcStatus)

            If MCB.AccumulateStats Then
                stcQualityStats.int4FSKQualityCnts += 1
                stcQualityStats.int4FSKQuality += intQuality
            End If
            stcStatus.ControlName = "ConstellationPlot"
            queTNCStatus.Enqueue(stcStatus)
        Catch ex As Exception
            Logs.Exception("[Main.Update4FSKConstellation] Err: " & ex.ToString)
        End Try
    End Sub  ' Update4FSKConstellation

    '  Subroutine to update the 16FSK constallation
    Friend Sub Update16FSKConstellation(ByRef intToneMags() As Int32, ByRef intQuality As Int32)
        ' Subroutine to update bmpConstellation plot for 16FSK modes...

        Try
            Dim dblRad As Double = 0
            Dim intToneSum As Int32 = 0
            Dim intMagMax As Double = 0
            Dim dblAng As Double
            Dim dblDistanceSum As Double
            Dim dblPlotRotation As Double = 0
            Dim stcStatus As Status
            Dim yCenter, xCenter As Integer
            Dim intRad As Int32
            Dim clrPixel As System.Drawing.Color
            Dim intJatMaxMag As Int32
            bmpConstellation = New Bitmap(89, 89)
            ' Draw the axis and paint the black background area
            yCenter = (bmpConstellation.Height - 2) \ 2
            xCenter = (bmpConstellation.Width - 2) \ 2
            For x As Integer = 0 To bmpConstellation.Width - 1
                For y As Integer = 0 To bmpConstellation.Height - 1
                    If y = yCenter Or x = xCenter Then
                        bmpConstellation.SetPixel(x, y, Color.DeepSkyBlue)
                    End If
                Next y
            Next x
            For i As Integer = 0 To (intToneMags.Length - 1) Step 16  ' for the number of symbols represented by intToneMags
                intToneSum = 0
                intMagMax = 0
                For j As Integer = 0 To 15
                    If intToneMags(i + j) > intMagMax Then
                        intMagMax = intToneMags(i + j)
                        intJatMaxMag = j
                    End If
                    intToneSum += intToneMags(i + j)
                Next j
                intRad = Max(5, 42 - 40 * (intToneSum - intMagMax) / intToneSum)
                If intRad < 15 Then
                    clrPixel = Color.Tomato
                ElseIf intRad < 30 Then
                    clrPixel = Color.Gold
                Else
                    clrPixel = Color.Lime
                End If
                ' plot the symbols rotated to avoid the axis
                dblAng = PI / 16 + (intJatMaxMag * PI / 8)
                bmpConstellation.SetPixel(CInt(xCenter + intRad * Cos(dblAng)), CInt(yCenter + intRad * Sin(dblAng)), clrPixel) ' don't plot on top of axis
                dblDistanceSum += (43 - intRad)
            Next i
            intQuality = Math.Max(0, CInt(100 - 2.2 * (dblDistanceSum / (intToneMags.Length \ 16)))) ' factor 2.2 emperically chosen for calibration (Qual range 25 to 100)
            stcStatus.ControlName = "lblQuality"
            stcStatus.Text = "16FSK Quality: " & intQuality.ToString
            queTNCStatus.Enqueue(stcStatus)
            If MCB.AccumulateStats Then
                stcQualityStats.int16FSKQualityCnts += 1
                stcQualityStats.int16FSKQuality += intQuality
            End If
            stcStatus.ControlName = "ConstellationPlot"
            queTNCStatus.Enqueue(stcStatus)
        Catch ex As Exception
            Logs.Exception("[Main.Update16FSKConstellation] Err: " & ex.ToString)
        End Try
    End Sub ' Update16FSKConstellation

    ' Subroutine to udpate the 8FSK Constellation
    Friend Sub Update8FSKConstellation(ByRef intToneMags() As Int32, ByRef intQuality As Int32)
        ' Subroutine to update bmpConstellation plot for 8FSK modes...
        Try
            Dim dblRad As Double = 0
            Dim intToneSum As Int32 = 0
            Dim intMagMax As Double = 0
            Dim dblAng As Double
            Dim dblDistanceSum As Double
            Dim dblPlotRotation As Double = 0
            Dim stcStatus As Status
            Dim yCenter, xCenter As Integer
            Dim intRad As Int32
            Dim clrPixel As System.Drawing.Color
            Dim intJatMaxMag As Int32
            bmpConstellation = New Bitmap(89, 89)
            ' Draw the axis and paint the black background area
            yCenter = (bmpConstellation.Height - 2) \ 2
            xCenter = (bmpConstellation.Width - 2) \ 2
            For x As Integer = 0 To bmpConstellation.Width - 1
                For y As Integer = 0 To bmpConstellation.Height - 1
                    If y = yCenter Or x = xCenter Then
                        bmpConstellation.SetPixel(x, y, Color.DeepSkyBlue)
                    End If
                Next y
            Next x
            For i As Integer = 0 To (intToneMags.Length - 1) Step 8  ' for the number of symbols represented by intToneMags
                intToneSum = 0
                intMagMax = 0
                For j As Integer = 0 To 7
                    If intToneMags(i + j) > intMagMax Then
                        intMagMax = intToneMags(i + j)
                        intJatMaxMag = j
                    End If
                    intToneSum += intToneMags(i + j)
                Next j
                intRad = Max(5, 42 - 40 * (intToneSum - intMagMax) / intToneSum)
                If intRad < 15 Then
                    clrPixel = Color.Tomato
                ElseIf intRad < 30 Then
                    clrPixel = Color.Gold
                Else
                    clrPixel = Color.Lime
                End If
                ' plot the symbols rotated to avoid the axis
                dblAng = PI / 9 + (intJatMaxMag * PI / 4)
                bmpConstellation.SetPixel(CInt(xCenter + intRad * Cos(dblAng)), CInt(yCenter + intRad * Sin(dblAng)), clrPixel)
                dblDistanceSum += (43 - intRad)
            Next i
            intQuality = Math.Max(0, CInt(100 - 2.0 * (dblDistanceSum / (intToneMags.Length \ 8)))) ' factor 2.0 emperically chosen for calibration (Qual range 25 to 100)
            stcStatus.ControlName = "lblQuality"
            stcStatus.Text = "8FSK Quality: " & intQuality.ToString
            queTNCStatus.Enqueue(stcStatus)
            If MCB.AccumulateStats Then
                stcQualityStats.int8FSKQualityCnts += 1
                stcQualityStats.int8FSKQuality += intQuality
            End If
            stcStatus.ControlName = "ConstellationPlot"
            queTNCStatus.Enqueue(stcStatus)
        Catch ex As Exception
            Logs.Exception("[Main.Update8FSKConstellation] Err: " & ex.ToString)
        End Try
    End Sub ' Update8FSKConstellation


    ' Subroutine to update the Busy detector when not displaying Spectrum or Waterfall (graphics disabled)
    Private Sub UpdateBusyDetector(ByRef bytNewSamples() As Byte)

        Static dblI(1023) As Double
        Static dblQ(1023) As Double
        Static dblReF(1023) As Double
        Static dblImF(1023) As Double
        Static aryLastY(255) As Integer
        Static intPtr As Integer = 0
        Static dblMag(206) As Double
        Static FFT As New FFT
        Static blnLastBusyStatus As Boolean

        Dim dblMagAvg As Double
        Dim intTuneLineLow, intTuneLineHi, intDelta As Int32
        Dim blnBusyStatus As Boolean
        Dim stcStatus As Status = Nothing
        stcStatus.ControlName = "mnuBusy"
        For i As Integer = 0 To bytNewSamples.Length - 1 Step 2
            dblI(intPtr) = CDbl(System.BitConverter.ToInt16(bytNewSamples, i))
            intPtr += 1
            If intPtr > 1023 Then Exit For
        Next

        If intPtr < 1024 Then Return
        ' If Now.Subtract(dttCodecStarted).TotalSeconds < 2 Then Exit Sub
        intPtr = 0
        FFT.FourierTransform(1024, dblI, dblQ, dblReF, dblImF, False)
        For i As Integer = 0 To dblMag.Length - 1
            'starting at ~300 Hz to ~2700 Hz Which puts the center of the signal in the center of the window (~1500Hz)
            dblMag(i) = (dblReF(i + 25) ^ 2 + dblImF(i + 25) ^ 2) ' first pass 
            dblMagAvg += dblMag(i)
        Next i
        intDelta = CInt((ExtractARQBandwidth() \ 2 + MCB.TuningRange) / 11.719)
        intTuneLineLow = Max((103 - intDelta), 3)
        intTuneLineHi = Min((103 + intDelta), 203)
        If objProtocol.GetARDOPProtocolState = ProtocolState.DISC Then ' Only process busy when in DISC state
            blnBusyStatus = objBusy.BusyDetect(dblMag, intTuneLineLow, intTuneLineHi)
            If blnBusyStatus And Not blnLastBusyStatus Then
                objHI.QueueCommandToHost("BUSY TRUE")
                stcStatus.Text = "True"
                queTNCStatus.Enqueue(stcStatus)
                'Debug.WriteLine("BUSY TRUE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))
            ElseIf blnLastBusyStatus And Not blnBusyStatus Then
                objHI.QueueCommandToHost("BUSY FALSE")
                stcStatus.Text = "False"
                queTNCStatus.Enqueue(stcStatus)
                'Debug.WriteLine("BUSY FALSE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))
            End If
            blnLastBusyStatus = blnBusyStatus
        End If
    End Sub ' UpdateBusyDetector

    ' Subroutine to update the waterfall display
    Private Sub UpdateWaterfall(ByRef bytNewSamples() As Byte)

        Static dblI(1023) As Double
        Static dblQ(1023) As Double
        Static dblReF(1023) As Double
        Static dblImF(1023) As Double
        Static aryLastY(255) As Integer
        Static intPtr As Integer = 0
        Static dblMag(206) As Double
        Static FFT As New FFT
        Static intWaterfallRow As Int32 ' pointer to the current waterfall row 
        Static blnLastBusyStatus As Boolean
        Dim dblMagAvg As Double
        Dim intTuneLineLow, intTuneLineHi, intDelta As Int32
        Dim clrTLC As System.Drawing.Color = Color.Chartreuse
        Dim blnBusyStatus As Boolean
        Dim stcStatus As Status = Nothing

        stcStatus.ControlName = "mnuBusy"
        For i As Integer = 0 To bytNewSamples.Length - 1 Step 2
            dblI(intPtr) = CDbl(System.BitConverter.ToInt16(bytNewSamples, i))
            intPtr += 1
            If intPtr > 1023 Then Exit For
        Next
        If intPtr < 1024 Then Return
        ' If Now.Subtract(dttCodecStarted).TotalSeconds < 2 Then Exit Sub
        intPtr = 0
        FFT.FourierTransform(1024, dblI, dblQ, dblReF, dblImF, False)
        For i As Integer = 0 To dblMag.Length - 1
            'starting at ~300 Hz to ~2700 Hz Which puts the center of the signal in the center of the window (~1500Hz)
            dblMag(i) = (dblReF(i + 25) ^ 2 + dblImF(i + 25) ^ 2) ' first pass 
            dblMagAvg += dblMag(i)
        Next i
        intDelta = CInt((ExtractARQBandwidth() \ 2 + MCB.TuningRange) / 11.719)
        intTuneLineLow = Max((103 - intDelta), 3)
        intTuneLineHi = Min((103 + intDelta), 203)
        If objProtocol.GetARDOPProtocolState = ProtocolState.DISC Then ' Only process busy when in DISC state
            blnBusyStatus = objBusy.BusyDetect(dblMag, intTuneLineLow, intTuneLineHi)
            If blnBusyStatus Then
                clrTLC = Color.Fuchsia
            End If
            If blnBusyStatus And Not blnLastBusyStatus Then
                objHI.QueueCommandToHost("BUSY TRUE")
                stcStatus.Text = "True"
                queTNCStatus.Enqueue(stcStatus)
                'Debug.WriteLine("BUSY TRUE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))
            ElseIf blnLastBusyStatus And Not blnBusyStatus Then
                objHI.QueueCommandToHost("BUSY FALSE")
                stcStatus.Text = "False"
                queTNCStatus.Enqueue(stcStatus)
                'Debug.WriteLine("BUSY FALSE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))
            End If
            blnLastBusyStatus = blnBusyStatus
        End If

        Try
            dblMagAvg = Math.Log10(dblMagAvg / 5000)
            For i As Integer = 0 To dblMag.Length - 1
                ' The following provides some AGC over the waterfall to compensate for avg input level.
                Dim y1 As Double = (0.25 + 2.5 / dblMagAvg) * Math.Log10(0.01 + dblMag(i))
                Dim objColor As System.Drawing.Color
                '  Set the pixel color based on the intensity (log) of the spectral line
                If y1 > 6.5 Then
                    objColor = Color.Orange ' Strongest spectral line 
                ElseIf y1 > 6 Then
                    objColor = Color.Khaki
                ElseIf y1 > 5.5 Then
                    objColor = Color.Cyan
                ElseIf y1 > 5 Then
                    objColor = Color.DeepSkyBlue
                ElseIf y1 > 4.5 Then
                    objColor = Color.RoyalBlue
                ElseIf y1 > 4 Then
                    objColor = Color.Navy
                Else
                    objColor = Color.Black  ' Weakest spectral line
                End If
                If i = 103 Then
                    bmpSpectrum.SetPixel(i, intWaterfallRow, Color.Tomato) ' 1500 Hz line (center)
                ElseIf (i = intTuneLineLow Or i = intTuneLineLow - 1 Or i = intTuneLineHi Or i = intTuneLineHi + 1) Then
                    bmpSpectrum.SetPixel(i, intWaterfallRow, clrTLC)
                Else
                    bmpSpectrum.SetPixel(i, intWaterfallRow, objColor) ' Else plot the pixel as received
                End If
            Next i
            ' Using a new bit map allows merging the two parts of bmpSpectrum and plotting all at one time to eliminate GDI+ fault.
            bmpNewSpectrum = New Bitmap(bmpSpectrum.Width, bmpSpectrum.Height)
            destRect1 = New Rectangle(0, 0, bmpSpectrum.Width, bmpSpectrum.Height - intWaterfallRow)
            ' Now create rectangle for the bottom part of the waterfall. Top of bmpSpectrum to intWaterfallRow -1
            destRect2 = New Rectangle(0, bmpSpectrum.Height - intWaterfallRow, bmpSpectrum.Width, intWaterfallRow)
            ' Create a new graphics area to draw into the new bmpNewSpectrum
            Dim graComposit As Graphics = Graphics.FromImage(bmpNewSpectrum)
            ' add the two rectangles to the graComposit
            graComposit.DrawImage(bmpSpectrum, destRect1, 0, intWaterfallRow, bmpSpectrum.Width, bmpSpectrum.Height - intWaterfallRow, GraphicsUnit.Pixel)
            graComposit.DrawImage(bmpSpectrum, destRect2, 0, 0, bmpSpectrum.Width, intWaterfallRow, GraphicsUnit.Pixel)
            graComposit.Dispose() '  the new composit bitmap has been constructed
            If Not IsNothing(graFrequency) Then
                graFrequency.Dispose() ' this permits writing back to the graFrequency without a GDI+ fault.
            End If
            graFrequency = pnlWaterfall.CreateGraphics
            graFrequency.DrawImage(bmpNewSpectrum, 0, 0) ' Draw the new bitmap in one update to avoid a possible GDI+ fault
            intWaterfallRow -= 1 ' Move the WaterFallRow back to point to the oldest received spectrum 
            If intWaterfallRow < 0 Then intWaterfallRow = bmpSpectrum.Height - 1 ' Makes the bitmap a circular buffer
        Catch ex As Exception
            Logs.Exception("[Main.UpdateWaterfall] Err #: " & Err.Number.ToString & "  Exception: " & ex.ToString)
        End Try
    End Sub ' UpdateWaterfall

    ' Subroutine to update the spectrum display
    Private Sub UpdateSpectrum(ByRef bytNewSamples() As Byte)

        Static dblI(1023) As Double
        Static dblQ(1023) As Double
        Static dblReF(1023) As Double
        Static dblImF(1023) As Double
        Static aryLastY(255) As Integer
        Static intPtr As Integer = 0
        Static dblMag(206) As Double
        Static FFT As New FFT
        Static intPriorY(256) As Integer
        Static blnLastBusyStatus As Boolean
        Static dblMaxScale As Double
        Dim dblMagBusy(206) As Double
        Dim dblMagMax As Double = 0.0000000001
        Dim dblMagMin As Double = 10000000000.0
        Dim intTuneLineLow, intTuneLineHi, intDelta As Int32
        Dim blnBusyStatus As Boolean
        Dim Trace As Integer = 0
        Dim clrTLC As System.Drawing.Color = Color.Chartreuse
        Dim stcStatus As Status

        For i As Integer = 0 To bytNewSamples.Length - 1 Step 2
            dblI(intPtr) = CDbl(System.BitConverter.ToInt16(bytNewSamples, i))
            intPtr += 1
            If intPtr > 1023 Then Exit For
        Next
        If intPtr < 1024 Then Return
        intPtr = 0
        FFT.FourierTransform(1024, dblI, dblQ, dblReF, dblImF, False)
        Trace = 1
        intDelta = CInt((ExtractARQBandwidth() \ 2 + MCB.TuningRange) / 11.719)
        intTuneLineLow = Max((103 - intDelta), 3)
        intTuneLineHi = Min((103 + intDelta), 203)
        stcStatus.ControlName = "mnuBusy"

        For i As Integer = 0 To dblMag.Length - 1
            'starting at ~300 Hz to ~2700 Hz Which puts the center of the window at 1500Hz
            dblMagBusy(i) = (dblReF(i + 25) ^ 2 + dblImF(i + 25) ^ 2)
            dblMag(i) = 0.2 * dblMagBusy(i) + 0.8 * dblMag(i) ' first pass 
            dblMagMax = Math.Max(dblMagMax, dblMag(i))
            dblMagMin = Math.Min(dblMagMin, dblMag(i))
        Next i
        If objProtocol.GetARDOPProtocolState = ProtocolState.DISC Then ' Only process busy when in DISC state
            blnBusyStatus = objBusy.BusyDetect(dblMag, intTuneLineLow, intTuneLineHi)
            If blnBusyStatus And Not blnLastBusyStatus Then
                objHI.QueueCommandToHost("BUSY TRUE")
                stcStatus.Text = "True"
                clrTLC = Color.Fuchsia
                queTNCStatus.Enqueue(stcStatus)
            ElseIf blnLastBusyStatus And Not blnBusyStatus Then
                objHI.QueueCommandToHost("BUSY FALSE")
                stcStatus.Text = "False"
                clrTLC = Color.LightGreen
                queTNCStatus.Enqueue(stcStatus)
            End If
        End If
        ' This performs an auto scaling mechansim with fast attack and slow release
        If dblMagMin / dblMagMax < 0.0001 Then ' more than 10000:1 difference Max:Min
            dblMaxScale = Max(dblMagMax, dblMaxScale * 0.9)
        Else
            dblMaxScale = Max(10000 * dblMagMin, dblMagMax)
        End If
        Trace = 2
        Try
            '            'InititializeSpectrum(Color.Black)
            bmpNewSpectrum = New Bitmap(intBMPSpectrumWidth, intBMPSpectrumHeight)
            For i As Integer = 0 To dblMag.Length - 1
                ' The following provides some AGC over the waterfall to compensate for avg input level.
                Dim y1 As Integer = CInt(-0.25 * (intBMPSpectrumHeight - 1) * Log10((Max(dblMag(i), dblMaxScale / 10000)) / dblMaxScale)) ' range should be 0 to bmpSpectrumHeight -1
                Dim objColor As System.Drawing.Color = Color.Yellow
                If (intTuneLineLow <> intSavedTuneLineLow) Or (blnLastBusyStatus <> blnBusyStatus) Then ' Redraw center and bandwidth lines if change in display
                    For j As Integer = 0 To intBMPSpectrumHeight - 1
                        bmpNewSpectrum.SetPixel(103, j, Color.Tomato)
                        bmpNewSpectrum.SetPixel(intSavedTuneLineLow, j, Color.Black)
                        bmpNewSpectrum.SetPixel(intSavedTuneLineHi, j, Color.Black)
                        bmpNewSpectrum.SetPixel(Max(0, intSavedTuneLineLow - 1), j, Color.Black)
                        bmpNewSpectrum.SetPixel(intSavedTuneLineHi + 1, j, Color.Black)
                        bmpNewSpectrum.SetPixel(intTuneLineLow, j, clrTLC)
                        bmpNewSpectrum.SetPixel(intTuneLineHi, j, clrTLC)
                        bmpNewSpectrum.SetPixel(intTuneLineLow - 1, j, clrTLC)
                        bmpNewSpectrum.SetPixel(intTuneLineHi + 1, j, clrTLC)
                    Next j
                    intSavedTuneLineHi = intTuneLineHi
                    intSavedTuneLineLow = intTuneLineLow
                End If
                ' Clear the old pixels and put in new ones if not on a center or Tuning lines
                If Not ((i = 103) Or (i = intTuneLineHi) Or (i = intTuneLineLow) Or (i = intTuneLineHi + 1) Or (i = intTuneLineLow - 1)) Then
                    ' Set the prior plotted pixels to black
                    bmpNewSpectrum.SetPixel(i, intPriorY(i), Color.Black)
                    If intPriorY(i) < (intBMPSpectrumHeight - 2) Then bmpNewSpectrum.SetPixel(i, intPriorY(i) + 1, Color.Black)
                    intPriorY(i) = y1
                    bmpNewSpectrum.SetPixel(i, y1, Color.Yellow)
                    If y1 < (intBMPSpectrumHeight - 2) Then bmpNewSpectrum.SetPixel(i, y1 + 1, Color.Gold)
                End If
            Next i
            Trace = 10
            If Not IsNothing(graFrequency) Then
                Trace = 11
                graFrequency.Dispose() ' this permits writing back to the graFrequency without a GDI+ fault.
            End If
            graFrequency = pnlWaterfall.CreateGraphics
            graFrequency.DrawImage(bmpNewSpectrum, 0, 0) ' Draw the new bitmap in one update to avoid a possible GDI+ fault
            ' Tis to manage memory leak May 13, 2015
            If Not IsNothing(bmpNewSpectrum) Then
                bmpNewSpectrum.Dispose()
                bmpNewSpectrum = Nothing
            End If
        Catch ex As Exception
            Logs.Exception("[Main.UpdateSpectrum] Err #: " & Err.Number.ToString & "  Exception: " & ex.ToString & "  Trace=" & Trace.ToString)
        End Try
        blnLastBusyStatus = blnBusyStatus
    End Sub ' UpdateSpectrum

    ' Subroutine to process Sound card Data capture events
    Private Sub ProcessCapturedData()
        Dim bytCaptureData(-1) As Byte
        Dim intReadPos As Integer
        Dim intCapturePos As Integer
        Dim intLockSize As Integer
        Dim stcStatus As Status = Nothing
        Static intRcvPeak As Integer = 0
        Static intGraphicsCtr As Int32 = 0
        Static intRecoveryCnt As Int32
        Static intPkRcvLevelCnt As Int32 = 0

        If ALSA Then

            If AlsaGetSamples(bytCaptureData) > 0 Then
                dttLastSoundCardSample = Now
            End If

        Else

            ' Get Data from DirectSound
            Try
                If Nothing Is objCapture Then Return
                ' Get the data in the CaptureBuffer
                objCapture.GetCurrentPosition(intCapturePos, intReadPos)
                intLockSize = intReadPos - intNextCaptureOffset
                If intLockSize < 0 Then intLockSize += intCaptureBufferSize
                ' Block align lock size so that we are always write on a boundary
                intLockSize -= intLockSize Mod intNotifySize
                If 0 = intLockSize Then Return
                dttLastSoundCardSample = Now
                ' Read the capture buffer.
                If Not blnSCCapturing Then Exit Sub
                bytCaptureData = CType(objCapture.Read(intNextCaptureOffset, GetType(Byte), LockFlag.None, intLockSize), Byte())
            Catch ex As Exception
                Logs.Exception("[Main.ProcessCapturedData]1 Err:  " & ex.ToString)
                Return
            End Try

        End If

        If bytCaptureData.Length < 2 Then
            Return
        End If

        Try
            Dim intSample As Integer
            For i As Integer = 0 To bytCaptureData.Length - 1 Step 2
                intSample = System.BitConverter.ToInt16(bytCaptureData, i)
                intRcvPeak = Math.Max(intRcvPeak, Math.Abs(intSample))
            Next
            If Not blnSCCapturing Then Exit Sub
            ' The following code blocks waterfall operation and processing if playing.  
            ' The intRecovery count holds off the samples until the receiver recovers (about 85 ms if intRecoveryCnt = 1) this may need 
            If SoundIsPlaying Then
                intRecoveryCnt = 0 ' testing with no holdoff 100 ms Busy processing holdoff from PTT release added to Busy detector May 23, 2015
                intNextCaptureOffset += bytCaptureData.Length
                intNextCaptureOffset = intNextCaptureOffset Mod intCaptureBufferSize ' Circular buffer
                Exit Sub  ' if playing don't update waterfall or Receive level indicator
            ElseIf intRecoveryCnt > 0 Then
                intRecoveryCnt -= 1
                intNextCaptureOffset += bytCaptureData.Length
                intNextCaptureOffset = intNextCaptureOffset Mod intCaptureBufferSize ' Circular buffer
                Exit Sub
            End If
            ' ********************
            If intPkRcvLevelCnt >= 4 Then ' This mechanism slows down the updating on the form and makes the peak easier to read.
                stcStatus.Value = Math.Min(CInt(Math.Sqrt(intRcvPeak)), 181)
                stcStatus.ControlName = "prgReceiveLevel"
                queTNCStatus.Enqueue(stcStatus)
                intPkRcvLevelCnt = 0
                intRcvPeak = intRcvPeak * 0.9
            Else
                intPkRcvLevelCnt += 1
            End If
            objProtocol.ProcessNewSamples(bytCaptureData)
            If MCB.DisplayWaterfall And Now.Subtract(dttCodecStarted).TotalMilliseconds > 2000 Then
                If Not blnGraphicsCleared Then
                    ClearSpectrum()
                    blnGraphicsCleared = True
                End If
                UpdateWaterfall(bytCaptureData)
            ElseIf MCB.DisplaySpectrum And Now.Subtract(dttCodecStarted).TotalMilliseconds > 2000 Then
                If Not blnGraphicsCleared Then
                    ClearSpectrum()
                    blnGraphicsCleared = True
                End If
                UpdateSpectrum(bytCaptureData)
            ElseIf Now.Subtract(dttCodecStarted).TotalMilliseconds > 2000 Then
                intGraphicsCtr += 1
                If intGraphicsCtr > 20 Then
                    WriteTextToSpectrum("Graphics Disabled", Brushes.Yellow)
                    intGraphicsCtr = 0
                End If
                UpdateBusyDetector(bytCaptureData)
            End If
            intNextCaptureOffset += bytCaptureData.Length
            intNextCaptureOffset = intNextCaptureOffset Mod intCaptureBufferSize ' Circular buffer
        Catch ex As Exception
            Logs.Exception("[Main.ProcessCapturedData]3 Err: " & ex.ToString)
        End Try
    End Sub  '  ProcessCapturedData

    ' Test code Subroutine to send a frame from the Test Form...throw away code
    Public Sub SendTestFrame(intFilteredSamples() As Int32, strFileStream As String, intRepeats As Int32)
        strLastWavStream = strFileStream
        ClearTuningStats()
        ClearQualityStats()
        objMod.CreateWaveStream(intFilteredSamples)
        State = ReceiveState.SearchingForLeader
        If Not SoundIsPlaying Then
            PlaySoundStream()
            dblMaxLeaderSN = 0
            intRepeatCnt = intRepeats
        End If
    End Sub  'SendTestFrame

    ' Subroutine called by the protocol to send a data or command frame
    Public Sub SendFrame(intFilteredSamples() As Int32, strLastFileStream As String, Optional intDelayMs As Int32 = 0)
        Dim dttStartWait As Date = Now
        If SoundIsPlaying Then
            While SoundIsPlaying And Now.Subtract(dttStartWait).TotalSeconds < 10
                Thread.Sleep(50)
            End While
        End If
        objMod.CreateWaveStream(intFilteredSamples)
        dttNextPlay = Now.AddMilliseconds(intDelayMs)
        blnFramePending = True
    End Sub 'SendFrame

    'Subroutine to send ID (with optional CWID)
    Public Sub SendID(blnEnableCWID As Boolean)
        Dim strFilename As String = ""
        Dim bytEncodedBytes() As Byte
        Dim bytIDSent() As Byte
        If IsNothing(MCB.GridSquare) Then
            bytEncodedBytes = objMod.Encode4FSKIDFrame(MCB.Callsign, "No GS", strFilename)
            bytIDSent = GetBytes(" " & MCB.Callsign & ":[No GS] ")
        Else
            bytEncodedBytes = objMod.Encode4FSKIDFrame(MCB.Callsign, MCB.GridSquare, strFilename)
            bytIDSent = GetBytes(" " & MCB.Callsign & ":[" & MCB.GridSquare & "] ")
        End If
        objProtocol.AddTagToDataAndSendToHost(bytIDSent, "IDF")
        intSamples = objMod.Mod4FSKData(&H30, bytEncodedBytes)
        If MCB.CWID Then
            ReDim Preserve intSamples(intSamples.Length + 4800)
            Dim intCWID(-1) As Int32
            Dim intPtr As Integer = intSamples.Length
            objMod.CWID("DE " & MCB.Callsign, intCWID, False)
            ReDim Preserve intSamples(intSamples.Length + intCWID.Length)
            Array.Copy(intCWID, 0, intSamples, intPtr, intCWID.Length)
            strLastWavStream &= " + CWID"
        End If
        If Not SoundIsPlaying Then
            objMod.CreateWaveStream(intSamples)
            PlaySoundStream()
        End If
    End Sub  '  SendID

    '  Subroutine to send 5 seconds of two tone
    Public Sub Send5SecTwoTone()
        Try
            If Not SoundIsPlaying Then
                objMod.CreateWaveStream(objMod.ModTwoToneTest())
                strLastWavStream = "5 Sec Two Tone Test"
                PlaySoundStream()
            End If
        Catch ex As Exception
            Logs.Exception("[Main.Send5SecTwoTone] Err: " & ex.ToString)
        End Try
    End Sub ' Send5SecTwoTone

    ' Subroutine to Switch Display mode to Waterfall
    Private Sub WaterfallToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles WaterfallToolStripMenuItem.Click
        MCB.DisplaySpectrum = False
        ClearSpectrum()
        MCB.DisplayWaterfall = True
        objIniFile.WriteString("ARDOP_Win TNC", "Display Waterfall", MCB.DisplayWaterfall.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "Display Spectrum", MCB.DisplaySpectrum.ToString)
        objIniFile.Flush()
        intSavedTuneLineHi = 0
        intSavedTuneLineLow = 0
    End Sub 'WaterfallToolStripMenuItem_Click

    ' Subroutine to Switch Display mode to Spectrum
    Private Sub SpectrumToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles SpectrumToolStripMenuItem.Click
        MCB.DisplayWaterfall = False
        ClearSpectrum()
        MCB.DisplaySpectrum = True
        objIniFile.WriteString("ARDOP_Win TNC", "Display Waterfall", MCB.DisplayWaterfall.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "Display Spectrum", MCB.DisplaySpectrum.ToString)
        objIniFile.Flush()
        intSavedTuneLineHi = 0
        intSavedTuneLineLow = 0
    End Sub ' SpectrumToolStripMenuItem_Click

    ' Subroutine to Disable graphic display
    Private Sub DisableToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles DisableToolStripMenuItem.Click
        MCB.DisplaySpectrum = False
        MCB.DisplayWaterfall = False
        InitializeGraphics()

        WriteTextToSpectrum("Graphics Disabled", Brushes.Yellow)
        objIniFile.WriteString("ARDOP_Win TNC", "Display Waterfall", MCB.DisplayWaterfall.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "Display Spectrum", MCB.DisplaySpectrum.ToString)
        objIniFile.Flush()
        intSavedTuneLineHi = 0
        intSavedTuneLineLow = 0
    End Sub  ' DisableToolStripMenuItem_Click

    ' Subroutine to handle tmrStartCODEC
    Private Sub tmrStartCODEC_Tick(sender As Object, e As System.EventArgs) Handles tmrStartCODEC.Tick
        Dim strFault As String = ""
        tmrStartCODEC.Stop()

        If Not StartCodec(strFault) Then
            Logs.Exception("[tmrStartCodec_Tick] Failure to start Codec! Fault= " & strFault)
            blnCodecStarted = False
        Else
            ' Debug.WriteLine("Codec Started OK")
            blnCodecStarted = True
        End If
    End Sub 'tmrStartCODEC_Tick

    ' Subroutine to show the About form
    Private Sub HelpToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles AboutMenuItem.Click
        Dim frmAbout As New About
        frmAbout.Show()
    End Sub  ' HelpToolStripMenuItem_Click

    ' Subroutine to open the Log dialog
    Private Sub HelpToolStripMenuItem1_Click(sender As System.Object, e As System.EventArgs) Handles HelpToolStripMenuItem1.Click
        ' Opens a file dialog to access log files for reading or deletion...
        Try
            Dim dlgLogs As New OpenFileDialog
            dlgLogs.Title = "Select a Log File to View..."
            dlgLogs.InitialDirectory = strExecutionDirectory & "Logs\"
            dlgLogs.Filter = "Log File(.log;.txt)|*.log;*.txt"
            dlgLogs.RestoreDirectory = True
            dlgLogs.Multiselect = True
            If dlgLogs.ShowDialog() = Windows.Forms.DialogResult.OK Then
                Try
                    Process.Start(dlgLogs.FileName)
                Catch
                    MsgBox(Err.Description, MsgBoxStyle.Information)
                End Try
            End If
        Catch
            Logs.Exception("[Main.mnuLogs_Click] " & Err.Description)
        End Try
    End Sub  ' HelpToolStripMenuItem1_Click

    '  Subroutine to provide delay before logging
    Private Sub tmrLogStats_Tick(sender As Object, e As System.EventArgs) Handles tmrLogStats.Tick
        ' This timer provides a 1 sec delay on looping test frames to allow stats to complete before logging
        tmrLogStats.Stop()
        LogStats()
    End Sub ' tmrLogStats_Tick

    ' Subroutine to hande RadioMenu.Click 
    Private Sub ToolStripMenuItem1_Click(sender As System.Object, e As System.EventArgs) Handles RadioMenu.Click
        If IsNothing(objRadio) Then objRadio = New Radio(Me)
        objRadio.ShowDialog()
        If objRadio.DialogResult = Windows.Forms.DialogResult.OK Then
            objRadio.CloseRadio()
            objRadio.Close()
            objRadio = Nothing
            If RCB.RadioControl Then
                objRadio = New Radio(Me)
                objRadio.InitRadioPorts()
            End If
        End If
    End Sub ' ToolStripMenuItem1_Click

    ' Subroutine to handle ABORT click 
    Private Sub AbortToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles AbortToolStripMenuItem.Click
        If objProtocol.GetARDOPProtocolState = ProtocolState.FECSend Then
            If MsgBox("Abort FEC Sending?", MsgBoxStyle.OkCancel, "Abort FEC!") = MsgBoxResult.Cancel Then Exit Sub
            objProtocol.Abort() ' FEC()
        ElseIf objProtocol.GetARDOPProtocolState = ProtocolState.IDLE Or objProtocol.GetARDOPProtocolState = ProtocolState.IRS Or _
            objProtocol.GetARDOPProtocolState = ProtocolState.ISS Then
            If MsgBox("Abort ARQ Connection?", MsgBoxStyle.OkCancel, "Abort ARQ!") = MsgBoxResult.Cancel Then Exit Sub
            objProtocol.Abort()
        End If
    End Sub ' AbortToolStripMenuItem_Click

    ' Subroutine to handle Send Two Tone Click 
    Private Sub TwoToneTestToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles TwoToneTestToolStripMenuItem.Click
        SendIDToolStripMenuItem.Enabled = False
        Send5SecTwoTone()
    End Sub 'TwoToneTestToolStripMenuItem_Click

    ' Subroutine to handle Close Click 
    Private Sub CloseToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles CloseMenuItem.Click
        If Not IsNothing(frmTest) Then frmTest.Close()
        Me.Close()
    End Sub 'CloseToolStripMenuItem_Click

    ' Subroutine to show Test Form 
    Private Sub ShowTestFormToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles ShowTestFormMenuItem.Click
        If IsNothing(frmTest) Then
            frmTest = New Test(Me)
            frmTest.Show()
        Else
            frmTest.BringToFront()
        End If
    End Sub  '  ShowTestFormToolStripMenuItem_Click

    ' Subroutine to handle CWID Click 
    Private Sub CWIDToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles CWIDToolStripMenuItem.Click
        SendIDToolStripMenuItem.Enabled = False
        Dim intSamp(-1) As Int32
        'If IsNothing(objPlayback) Then
        If Not SoundIsPlaying Then
            objMod.CWID("DE " & MCB.Callsign, intSamp, True)
            strLastWavStream = "FSK CWID:  DE " & MCB.Callsign
            PlaySoundStream()
        End If
    End Sub  'CWIDToolStripMenuItem_Click

    ' Subroutine to bring up Help Table of Contents
    Private Sub HelpContentsToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles HelpContentsToolStripMenuItem.Click
        Try
            Help.ShowHelp(Me, strExecutionDirectory & "ARDOP_Win TNC.chm", HelpNavigator.TableOfContents)
        Catch
            Logs.Exception("[ARDOP_Win Main mnuHelp_Click] " & Err.Description)
        End Try
    End Sub 'HelpContentsToolStripMenuItem_Click

    ' Subroutine to bring up Help Index
    Private Sub HelpIndexToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles HelpIndexToolStripMenuItem.Click
        Try
            Help.ShowHelp(Me, strExecutionDirectory & "ARDOP_Win TNC.chm", HelpNavigator.Index)
        Catch
            Logs.Exception("[ARDOP_Win Main mnuHelp_Click] " & Err.Description)
        End Try
    End Sub  ' HelpIndexToolStripMenuItem_Click

    '  Subroutine to Send ID frame 
    Private Sub IDFrameToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles IDFrameToolStripMenuItem.Click
        SendIDToolStripMenuItem.Enabled = False
        SendID(MCB.CWID)
    End Sub 'Sub IDFrameToolStripMenuItem_Click

    ' Subroutine to handel Mouse Click on Waterfall
    Private Sub pnlWaterfall_MouseClick(sender As Object, e As System.Windows.Forms.MouseEventArgs) Handles pnlWaterfall.MouseClick
        Dim intFreqOffset As Int32
        Dim dttClickStart As Date = Now
        Dim intNewFreq As Int32
        If RCB.Mode = "FM" Then Exit Sub
        Select Case objProtocol.GetARDOPProtocolState
            Case ProtocolState.DISC, ProtocolState.OFFLINE
                ' Each pixel= 1 FFT bin = represents 11.718275 Hz  (12000/1024)
                intFreqOffset = Round((e.X - 103) * 11.71875)
                If (RCB.RadioControl And Not IsNothing(objRadio)) Then
                    If objRadio.intLastReportedFreq > 0 And intFreqOffset <> 0 Then
                        intNewFreq = objRadio.intLastReportedFreq + intFreqOffset
                        objRadio.SetDialFrequency(intNewFreq)
                        If MCB.DebugLog Then Logs.WriteDebug("[Mouse click on waterfall] New Radio Freq =" & intNewFreq.ToString)
                    End If
                ElseIf intFreqOffset <> 0 Then
                    ' The resulting TUNE value is what is required to bring the clicked spectrum to the center of the waterfall
                    objHI.QueueCommandToHost("TUNE " & intFreqOffset.ToString)
                    If MCB.DebugLog Then Logs.WriteDebug("[Mouse click on waterfall] TUNE " & CInt((e.X - 103) * 11.71875).ToString)
                End If

            Case Else
        End Select
    End Sub ' pnlWaterfall_MouseClick

    ' Subroutine to Initialize Graphics 
    Private Sub InitializeGraphics()
        If Not IsNothing(bmpSpectrum) Then bmpSpectrum.Dispose()
        If Not IsNothing(bmpNewSpectrum) Then bmpNewSpectrum.Dispose()
        If Not IsNothing(graFrequency) Then graFrequency.Dispose()
        If Not IsNothing(graFrequency) Then graFrequency.Dispose() ' this permits writing back to the graFrequency without a GDI+ fault.
    End Sub  '   InitializeGraphics

    Private Function Bitmap(p1 As Integer, p2 As Integer) As Bitmap
        Throw New NotImplementedException
    End Function

End Class
