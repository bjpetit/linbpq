#Region "Imports"
Imports System.Windows.Forms
Imports Microsoft.DirectX
Imports Microsoft.DirectX.DirectSound
Imports System.IO
Imports System.IO.Ports
#End Region ' Imports


Public Class TNCSetup ' A Windows Dialog form to display/Edit ARDOP TNC parameters and values
    Dim strAllDataModes() As String = {"8FSK.200.25", "4FSK.200.50S", "4FSK.200.50", "4PSK.200.100S", "4PSK.200.100", "8PSK.200.100", _
                                       "16FSK.500.25S", "16FSK.500.25", "4FSK.500.100S", "4FSK.500.100", "4PSK.500.100", "8PSK.500.100", "4PSK.500.167", "8PSK.500.167", _
                                       "4FSK.1000.100", "4PSK.1000.100", "4PSK.1000.167", "8PSK.1000.100", "8PSK.1000.167", _
                                      "4FSK.2000.600S", "4FSK.2000.600", "4FSK.2000.100", "4PSK.2000.100", "4PSK.2000.167", "8PSK.2000.100", "8PSK.2000.167"}
    Private strEntryCaptureDevice As String = ""
    Private strEntryPlaybackDevice As String = ""

#Region "Structures"

    Private Structure DeviceDescription
        Dim info As DeviceInformation
        Overrides Function ToString() As String
            Return info.Description
        End Function
        Sub New(ByVal d As DeviceInformation)
            info = d
        End Sub
    End Structure

#End Region ' Structures

#Region " Private Subs and Functions"

    Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click

        Dim intTCPPort As Int16
        Try
            intTCPPort = CShort(txtTCIPControl.Text.Trim)
        Catch
            MsgBox("TCPIP Control port must be an integer < 65536!", MsgBoxStyle.Exclamation, "Invalid TCP Control Port number!")
            Return
        End Try
        If chkSecureLogin.Checked And txtPassword.Text.Trim.Length < 8 Then
            MsgBox("If Secure TCP Login is enabled password must be entered and be between 8 and 16 printable characters (no spaces, case sensitive)", MsgBoxStyle.Exclamation, "Improper TCP Login password!")
            Return
        End If
        If Not CheckValidCallsignSyntax(txtCallsign.Text.Trim.ToUpper) Then
            MsgBox("Invalid call sign syntax! 3 to 7 characters (0-9, A-Z) + optional ssid -1 to -15 or -A to -Z", MsgBoxStyle.Exclamation, "Invalid Call sign syntax!")
        End If
        ' This causes a redrawing of the tuning lines on the Spectrum display
        intSavedTuneLineHi = 0
        intSavedTuneLineLow = 0

        ' update the MCB values

        MCB.Callsign = txtCallsign.Text.Trim.ToUpper
        MCB.StartMinimized = chkStartMinimized.Checked
        MCB.DebugLog = chkDebugLog.Checked
        MCB.CommandTrace = chkCMDTrace.Checked
        MCB.CaptureDevice = cmbCapture.Text.Trim
        MCB.PlaybackDevice = cmbPlayback.Text.Trim

        MCB.LeaderLength = CInt(nudLeaderLength.Value)
        MCB.TrailerLength = CInt(nudTrailerLength.Value)
        MCB.ARQBandwidth = cmbBandwidth.Text.ToUpper.Trim
        MCB.DriveLevel = CInt(nudDriveLevel.Value)
        MCB.CWID = chkEnableCWID.Checked
        MCB.StartMinimized = CInt(nudSquelch.Value)
        MCB.DisplaySpectrum = rdoSpectrum.Checked
        MCB.DisplayWaterfall = rdoWaterfall.Checked
        MCB.Squelch = CInt(nudSquelch.Value)
        MCB.Password = txtPassword.Text.Trim
        MCB.SecureHostLogin = chkSecureLogin.Checked
        MCB.TuningRange = nudTuning.Value
        RCB.RadioControl = chkRadioControl.Checked
        MCB.FECMode = cmbFECType.Text
        MCB.FECRepeats = CInt(nudFECRepeats.Value)
        MCB.FECId = chkFECId.Checked
        MCB.ARQConReqRepeats = CInt(nudARQConReqRpt.Value)
        MCB.ProtocolMode = cmbProtocolMode.Text
        MCB.ARQTimeout = nudARQTimeout.Value
        ' update the ini file and flush
        If rdoTCP.Checked Then
            MCB.TCPPort = CInt(txtTCIPControl.Text)
            MCB.TCPAddress = txtTCPAddress.Text.Trim
            objIniFile.WriteInteger("ARDOP_Win TNC", "TCPIP Port", intTCPPort)
            objIniFile.WriteString("ARDOP_Win TNC", "TCPIP Address", txtTCPAddress.Text.Trim)
            MCB.HostTCPIP = True
            MCB.HostBlueTooth = False
            MCB.HostSerial = False
        ElseIf rdoSerial.Checked Then
            MCB.SerCOMPort = cmbCOM.Text
            MCB.SerBaud = CInt(cmbBaud.Text)
            objIniFile.WriteInteger("ARDOP_Win TNC", "SerialBaud", MCB.SerBaud)
            objIniFile.WriteString("ARDOP_Win TNC", "SerialCOMPort", MCB.SerCOMPort)
            MCB.HostTCPIP = False
            MCB.HostBlueTooth = False
            MCB.HostSerial = True
        ElseIf rdoBlueTooth.Checked Then
            MCB.HostPairing = cmbPairing.Text
            objIniFile.WriteString("ARDOP_Win TNC", "Host Pairing", cmbPairing.Text)
            MCB.HostTCPIP = False
            MCB.HostBlueTooth = True
            MCB.HostSerial = False
        End If
        objIniFile.WriteString("ARDOP_Win TNC", "HostTCPIP", MCB.HostTCPIP.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "HostSerial", MCB.HostSerial.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "HostBlueTooth", MCB.HostBlueTooth.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "Callsign", txtCallsign.Text.Trim.ToUpper)
        objIniFile.WriteString("ARDOP_Win TNC", "StartMinimized", chkStartMinimized.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "DebugLog", chkDebugLog.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "CommandTrace", chkCMDTrace.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "CaptureDevice", cmbCapture.Text.Trim)
        objIniFile.WriteString("ARDOP_Win TNC", "PlaybackDevice", cmbPlayback.Text.Trim)
        objIniFile.WriteInteger("ARDOP_Win TNC", "LeaderLength", CInt(nudLeaderLength.Value))
        objIniFile.WriteInteger("ARDOP_Win TNC", "TrailerLength", CInt(nudTrailerLength.Value))
        objIniFile.WriteString("ARDOP_Win TNC", "ARQBandwidth", cmbBandwidth.Text)
        objIniFile.WriteInteger("ARDOP_Win TNC", "DriveLevel", CInt(nudDriveLevel.Value))
        objIniFile.WriteString("ARDOP_Win TNC", "EnableCWID", chkEnableCWID.Checked.ToString)
        objIniFile.WriteInteger("ARDOP_Win TNC", "Squelch", CInt(nudSquelch.Value))
        objIniFile.WriteString("ARDOP_Win TNC", "Accum Stats", chkAccumStats.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "Display Waterfall", rdoWaterfall.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "Display Spectrum", rdoSpectrum.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "SecureHostLogin", chkSecureLogin.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "LoginPassword", txtPassword.Text.Trim)
        objIniFile.WriteInteger("ARDOP_Win TNC", "TuningRange", nudTuning.Value)
        objIniFile.WriteString("Radio", "Enable Radio Control", chkRadioControl.Checked.ToString)
        objIniFile.WriteInteger("ARDOP_Win TNC", "FECRepeats", nudFECRepeats.Value)
        objIniFile.WriteString("ARDOP_Win TNC", "FECMode", cmbFECType.Text)
        objIniFile.WriteString("ARDOP_Win TNC", "FECId", chkFECId.Checked.ToString)
        objIniFile.WriteString("ARDOP_Win TNC", "Version", Application.ProductVersion)
        objIniFile.WriteInteger("ARDOP_Win TNC", "ARQConReqRepeats", CInt(nudARQConReqRpt.Value))
        objIniFile.WriteString("ARDOP_Win TNC", "Protoco Mode", cmbProtocolMode.Text)
        objIniFile.WriteInteger("ARDOP_Win TNC", "ARQTimeout", CInt(nudARQTimeout.Value))
        objIniFile.Flush()
        If MCB.CaptureDevice = strEntryCaptureDevice And MCB.PlaybackDevice = strEntryPlaybackDevice Then
            Me.DialogResult = Windows.Forms.DialogResult.Ignore ' don't restart the Sound card
        Else
            Me.DialogResult = System.Windows.Forms.DialogResult.OK
        End If
        Me.Close()
    End Sub

    Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click
        Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Close()
    End Sub

    Private Sub BasicSetup_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

        cmbCOM.Items.Clear()
        cmbCOM.Items.Add("None")

        Dim strPorts() As String = SerialPort.GetPortNames
        If strPorts.Length > 0 Then
            For Each strPort As String In strPorts
                cmbCOM.Items.Add(strPort)
            Next
        End If
        cmbFECType.Items.Clear()
        For i As Integer = 0 To strAllDataModes.Length - 1
            cmbFECType.Items.Add(strAllDataModes(i))
        Next i

        Dim blnCaptureDeviceOK, blnPlaybackDeviceOK, blnCOMDeviceOK As Boolean


        Dim intCtr As Int32 = 0
     
        If Not MCB.HostTCPIP Then
            If blnCOMDeviceOK Then
                cmbPairing.BackColor = Color.LightGreen
            Else
                cmbPairing.BackColor = Color.LightSalmon
            End If
        End If


        ' initialize all the display from the MCB values 
        txtTCIPControl.Text = MCB.TCPPort.ToString
        txtCallsign.Text = MCB.Callsign
        chkStartMinimized.Checked = MCB.StartMinimized
        chkDebugLog.Checked = MCB.DebugLog
        chkCMDTrace.Checked = MCB.CommandTrace
        chkEnableCWID.Checked = MCB.CWID
        rdoTCP.Checked = MCB.HostTCPIP
        rdoSerial.Checked = MCB.HostSerial
        rdoBlueTooth.Checked = MCB.HostBlueTooth
        chkAccumStats.Checked = MCB.AccumulateStats
        cmbCapture.Text = MCB.CaptureDevice
        txtTCPAddress.Text = MCB.TCPAddress
        cmbBandwidth.Text = MCB.ARQBandwidth
        nudDriveLevel.Value = MCB.DriveLevel
        nudLeaderLength.Value = MCB.LeaderLength
        nudTrailerLength.Value = MCB.TrailerLength
        nudSquelch.Value = MCB.Squelch
        rdoSpectrum.Checked = MCB.DisplaySpectrum
        rdoWaterfall.Checked = MCB.DisplayWaterfall
        rdoDisable.Checked = Not (MCB.DisplayWaterfall Or MCB.DisplaySpectrum)
        txtPassword.Text = MCB.Password
        chkSecureLogin.Checked = MCB.SecureHostLogin
        nudTuning.Value = MCB.TuningRange
        cmbPairing.Text = MCB.HostPairing
        txtTCIPControl.Enabled = rdoTCP.Checked
        txtTCPAddress.Enabled = rdoTCP.Checked
        cmbBaud.Text = MCB.SerBaud
        cmbCOM.Text = MCB.SerCOMPort
        cmbPairing.Enabled = rdoBlueTooth.Checked
        cmbCOM.Enabled = rdoSerial.Checked
        cmbBaud.Enabled = rdoSerial.Checked
        chkRadioControl.Checked = RCB.RadioControl
        nudFECRepeats.Value = MCB.FECRepeats
        cmbFECType.Text = MCB.FECMode
        chkFECId.Checked = MCB.FECId
        nudARQConReqRpt.Value = MCB.ARQConReqRepeats
        nudARQTimeout.Value = MCB.ARQTimeout
        cmbProtocolMode.Text = MCB.ProtocolMode
        ' Get the Windows enumerated Playback devices and add to the Playback device combo box
        Dim strPlaybackDevices() As String = EnumeratePlaybackDevices()
        blnPlaybackDeviceOK = False
        For Each strDevice As String In strPlaybackDevices
            If strDevice = MCB.PlaybackDevice Then blnPlaybackDeviceOK = True : strEntryPlaybackDevice = MCB.PlaybackDevice
            cmbPlayback.Items.Add(strDevice)
        Next
        cmbPlayback.Text = MCB.PlaybackDevice
        strEntryPlaybackDevice = MCB.PlaybackDevice
        If blnPlaybackDeviceOK Then
            cmbPlayback.BackColor = Color.LightGreen
        Else
            cmbPlayback.BackColor = Color.LightSalmon
        End If
        ' Get the Windows enumerated Capture devices and add to the Capture device combo box
        Dim strCaptureDevices() As String = EnumerateCaptureDevices()
        blnCaptureDeviceOK = False
        For Each strDevice As String In strCaptureDevices
            If strDevice = MCB.CaptureDevice Then blnCaptureDeviceOK = True
            cmbCapture.Items.Add(strDevice)
        Next
        cmbCapture.Text = MCB.CaptureDevice
        strEntryCaptureDevice = MCB.CaptureDevice
        If blnCaptureDeviceOK Then
            cmbCapture.BackColor = Color.LightGreen
        Else
            cmbCapture.BackColor = Color.LightSalmon
        End If

    End Sub


#End Region ' Privates Subs and Functions

    Private Sub rdoTCP_CheckedChanged(sender As Object, e As System.EventArgs) Handles rdoTCP.CheckedChanged
        txtTCPAddress.Enabled = rdoTCP.Checked
        txtTCIPControl.Enabled = rdoTCP.Checked
    End Sub

    Private Sub rdoSerial_CheckedChanged_1(sender As System.Object, e As System.EventArgs) Handles rdoSerial.CheckedChanged
        cmbBaud.Enabled = rdoSerial.Checked
        cmbCOM.Enabled = rdoSerial.Checked
    End Sub

    Private Sub rdoBlueTooth_CheckedChanged(sender As System.Object, e As System.EventArgs) Handles rdoBlueTooth.CheckedChanged
        cmbPairing.Enabled = rdoBlueTooth.Checked
    End Sub

    Private Sub Label13_Click(sender As System.Object, e As System.EventArgs) Handles Label13.Click

    End Sub

    Private Sub btnDefault_Click_1(sender As System.Object, e As System.EventArgs)

    End Sub

    Private Sub cmbFECType_SelectedIndexChanged(sender As System.Object, e As System.EventArgs) Handles cmbFECType.SelectedIndexChanged

    End Sub

End Class
