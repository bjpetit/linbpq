<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class TNCSetup
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container()
        Me.OK_Button = New System.Windows.Forms.Button()
        Me.Cancel_Button = New System.Windows.Forms.Button()
        Me.ToolTip1 = New System.Windows.Forms.ToolTip(Me.components)
        Me.txtPassword = New System.Windows.Forms.TextBox()
        Me.txtTCPAddress = New System.Windows.Forms.TextBox()
        Me.txtTCIPControl = New System.Windows.Forms.TextBox()
        Me.chkSecureLogin = New System.Windows.Forms.CheckBox()
        Me.rdoBlueTooth = New System.Windows.Forms.RadioButton()
        Me.rdoTCP = New System.Windows.Forms.RadioButton()
        Me.cmbPairing = New System.Windows.Forms.ComboBox()
        Me.cmbCOM = New System.Windows.Forms.ComboBox()
        Me.rdoSerial = New System.Windows.Forms.RadioButton()
        Me.cmbBaud = New System.Windows.Forms.ComboBox()
        Me.rdoDisable = New System.Windows.Forms.RadioButton()
        Me.rdoSpectrum = New System.Windows.Forms.RadioButton()
        Me.rdoWaterfall = New System.Windows.Forms.RadioButton()
        Me.chkRadioControl = New System.Windows.Forms.CheckBox()
        Me.chkEnableCWID = New System.Windows.Forms.CheckBox()
        Me.chkCMDTrace = New System.Windows.Forms.CheckBox()
        Me.chkDebugLog = New System.Windows.Forms.CheckBox()
        Me.cmbPlayback = New System.Windows.Forms.ComboBox()
        Me.cmbCapture = New System.Windows.Forms.ComboBox()
        Me.chkAccumStats = New System.Windows.Forms.CheckBox()
        Me.cmbBandwidth = New System.Windows.Forms.ComboBox()
        Me.Label13 = New System.Windows.Forms.Label()
        Me.nudTuning = New System.Windows.Forms.NumericUpDown()
        Me.Label10 = New System.Windows.Forms.Label()
        Me.nudSquelch = New System.Windows.Forms.NumericUpDown()
        Me.nudDriveLevel = New System.Windows.Forms.NumericUpDown()
        Me.Label19 = New System.Windows.Forms.Label()
        Me.cmbFECType = New System.Windows.Forms.ComboBox()
        Me.Label18 = New System.Windows.Forms.Label()
        Me.nudFECRepeats = New System.Windows.Forms.NumericUpDown()
        Me.nudTrailerLength = New System.Windows.Forms.NumericUpDown()
        Me.nudLeaderLength = New System.Windows.Forms.NumericUpDown()
        Me.txtCallsign = New System.Windows.Forms.TextBox()
        Me.chkFECId = New System.Windows.Forms.CheckBox()
        Me.Label12 = New System.Windows.Forms.Label()
        Me.nudARQConReqRpt = New System.Windows.Forms.NumericUpDown()
        Me.cmbProtocolMode = New System.Windows.Forms.ComboBox()
        Me.grpHost = New System.Windows.Forms.GroupBox()
        Me.Label17 = New System.Windows.Forms.Label()
        Me.Label16 = New System.Windows.Forms.Label()
        Me.Label15 = New System.Windows.Forms.Label()
        Me.Label14 = New System.Windows.Forms.Label()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.Label9 = New System.Windows.Forms.Label()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.GroupBox1 = New System.Windows.Forms.GroupBox()
        Me.Label21 = New System.Windows.Forms.Label()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.Label20 = New System.Windows.Forms.Label()
        Me.Label11 = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.Label8 = New System.Windows.Forms.Label()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.chkStartMinimized = New System.Windows.Forms.CheckBox()
        Me.grpDisplay = New System.Windows.Forms.GroupBox()
        Me.Label22 = New System.Windows.Forms.Label()
        Me.nudARQTimeout = New System.Windows.Forms.NumericUpDown()
        CType(Me.nudTuning, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudSquelch, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudDriveLevel, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudFECRepeats, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudTrailerLength, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudLeaderLength, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudARQConReqRpt, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.grpHost.SuspendLayout()
        Me.GroupBox1.SuspendLayout()
        Me.grpDisplay.SuspendLayout()
        CType(Me.nudARQTimeout, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'OK_Button
        '
        Me.OK_Button.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.OK_Button.BackColor = System.Drawing.Color.LightSalmon
        Me.OK_Button.Location = New System.Drawing.Point(526, 475)
        Me.OK_Button.Name = "OK_Button"
        Me.OK_Button.Size = New System.Drawing.Size(94, 23)
        Me.OK_Button.TabIndex = 0
        Me.OK_Button.Text = "Save to ini File"
        Me.ToolTip1.SetToolTip(Me.OK_Button, "Save settings to TNC.ini")
        Me.OK_Button.UseVisualStyleBackColor = False
        '
        'Cancel_Button
        '
        Me.Cancel_Button.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.Cancel_Button.BackColor = System.Drawing.Color.LightGreen
        Me.Cancel_Button.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Cancel_Button.Location = New System.Drawing.Point(363, 476)
        Me.Cancel_Button.Name = "Cancel_Button"
        Me.Cancel_Button.Size = New System.Drawing.Size(116, 23)
        Me.Cancel_Button.TabIndex = 1
        Me.Cancel_Button.Text = "Abandon edits/Close"
        Me.ToolTip1.SetToolTip(Me.Cancel_Button, "Cancel edits and close form.")
        Me.Cancel_Button.UseVisualStyleBackColor = False
        '
        'txtPassword
        '
        Me.txtPassword.Location = New System.Drawing.Point(513, 26)
        Me.txtPassword.Name = "txtPassword"
        Me.txtPassword.Size = New System.Drawing.Size(105, 20)
        Me.txtPassword.TabIndex = 114
        Me.txtPassword.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        Me.ToolTip1.SetToolTip(Me.txtPassword, "The optional password for secure Host login")
        '
        'txtTCPAddress
        '
        Me.txtTCPAddress.Location = New System.Drawing.Point(178, 59)
        Me.txtTCPAddress.Name = "txtTCPAddress"
        Me.txtTCPAddress.Size = New System.Drawing.Size(221, 20)
        Me.txtTCPAddress.TabIndex = 112
        Me.txtTCPAddress.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        Me.ToolTip1.SetToolTip(Me.txtTCPAddress, "The address for the TCP connection (default = 127.0.0.1")
        '
        'txtTCIPControl
        '
        Me.txtTCIPControl.Location = New System.Drawing.Point(551, 59)
        Me.txtTCIPControl.Name = "txtTCIPControl"
        Me.txtTCIPControl.Size = New System.Drawing.Size(61, 20)
        Me.txtTCIPControl.TabIndex = 110
        Me.txtTCIPControl.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        Me.ToolTip1.SetToolTip(Me.txtTCIPControl, "The base TCPIP port # (may need to be permitted by firewall program)")
        '
        'chkSecureLogin
        '
        Me.chkSecureLogin.AutoSize = True
        Me.chkSecureLogin.Location = New System.Drawing.Point(329, 28)
        Me.chkSecureLogin.Name = "chkSecureLogin"
        Me.chkSecureLogin.Size = New System.Drawing.Size(125, 17)
        Me.chkSecureLogin.TabIndex = 116
        Me.chkSecureLogin.Text = "Enable Secure Login"
        Me.ToolTip1.SetToolTip(Me.chkSecureLogin, "Check to enable secure TCP login (normally only requried in remote applications)")
        Me.chkSecureLogin.UseVisualStyleBackColor = True
        '
        'rdoBlueTooth
        '
        Me.rdoBlueTooth.AutoSize = True
        Me.rdoBlueTooth.Enabled = False
        Me.rdoBlueTooth.Location = New System.Drawing.Point(7, 127)
        Me.rdoBlueTooth.Name = "rdoBlueTooth"
        Me.rdoBlueTooth.Size = New System.Drawing.Size(74, 17)
        Me.rdoBlueTooth.TabIndex = 118
        Me.rdoBlueTooth.Text = "BlueTooth"
        Me.ToolTip1.SetToolTip(Me.rdoBlueTooth, "Enable Blue Tooth interface for host")
        Me.rdoBlueTooth.UseVisualStyleBackColor = True
        '
        'rdoTCP
        '
        Me.rdoTCP.AutoSize = True
        Me.rdoTCP.Checked = True
        Me.rdoTCP.Location = New System.Drawing.Point(6, 59)
        Me.rdoTCP.Name = "rdoTCP"
        Me.rdoTCP.Size = New System.Drawing.Size(61, 17)
        Me.rdoTCP.TabIndex = 117
        Me.rdoTCP.TabStop = True
        Me.rdoTCP.Text = "TCP/IP"
        Me.ToolTip1.SetToolTip(Me.rdoTCP, "Enable TCP/IP interface for host")
        Me.rdoTCP.UseVisualStyleBackColor = True
        '
        'cmbPairing
        '
        Me.cmbPairing.FormattingEnabled = True
        Me.cmbPairing.Location = New System.Drawing.Point(159, 121)
        Me.cmbPairing.Name = "cmbPairing"
        Me.cmbPairing.Size = New System.Drawing.Size(177, 21)
        Me.cmbPairing.TabIndex = 119
        Me.ToolTip1.SetToolTip(Me.cmbPairing, "Sets the pairing for Bluetooth")
        '
        'cmbCOM
        '
        Me.cmbCOM.FormattingEnabled = True
        Me.cmbCOM.Location = New System.Drawing.Point(118, 89)
        Me.cmbCOM.Name = "cmbCOM"
        Me.cmbCOM.Size = New System.Drawing.Size(73, 21)
        Me.cmbCOM.TabIndex = 123
        Me.ToolTip1.SetToolTip(Me.cmbCOM, "Sets the Serial interface COM port. ")
        '
        'rdoSerial
        '
        Me.rdoSerial.AutoSize = True
        Me.rdoSerial.Location = New System.Drawing.Point(7, 93)
        Me.rdoSerial.Name = "rdoSerial"
        Me.rdoSerial.Size = New System.Drawing.Size(51, 17)
        Me.rdoSerial.TabIndex = 122
        Me.rdoSerial.Text = "Serial"
        Me.ToolTip1.SetToolTip(Me.rdoSerial, "Enable Blue Tooth interface for host")
        Me.rdoSerial.UseVisualStyleBackColor = True
        '
        'cmbBaud
        '
        Me.cmbBaud.FormattingEnabled = True
        Me.cmbBaud.Items.AddRange(New Object() {"9600", "19200", "38400", "57600", "115200"})
        Me.cmbBaud.Location = New System.Drawing.Point(254, 89)
        Me.cmbBaud.Name = "cmbBaud"
        Me.cmbBaud.Size = New System.Drawing.Size(73, 21)
        Me.cmbBaud.TabIndex = 125
        Me.cmbBaud.Text = "19200"
        Me.ToolTip1.SetToolTip(Me.cmbBaud, "Sets the serial interface baud rate (19200 minimum) ")
        '
        'rdoDisable
        '
        Me.rdoDisable.AutoSize = True
        Me.rdoDisable.Location = New System.Drawing.Point(11, 52)
        Me.rdoDisable.Name = "rdoDisable"
        Me.rdoDisable.Size = New System.Drawing.Size(60, 17)
        Me.rdoDisable.TabIndex = 2
        Me.rdoDisable.TabStop = True
        Me.rdoDisable.Text = "Disable"
        Me.ToolTip1.SetToolTip(Me.rdoDisable, "Disable graphics")
        Me.rdoDisable.UseVisualStyleBackColor = True
        '
        'rdoSpectrum
        '
        Me.rdoSpectrum.AutoSize = True
        Me.rdoSpectrum.Location = New System.Drawing.Point(11, 36)
        Me.rdoSpectrum.Name = "rdoSpectrum"
        Me.rdoSpectrum.Size = New System.Drawing.Size(70, 17)
        Me.rdoSpectrum.TabIndex = 1
        Me.rdoSpectrum.TabStop = True
        Me.rdoSpectrum.Text = "Spectrum"
        Me.ToolTip1.SetToolTip(Me.rdoSpectrum, "Enable Spectrum graphics")
        Me.rdoSpectrum.UseVisualStyleBackColor = True
        '
        'rdoWaterfall
        '
        Me.rdoWaterfall.AutoSize = True
        Me.rdoWaterfall.Location = New System.Drawing.Point(11, 20)
        Me.rdoWaterfall.Name = "rdoWaterfall"
        Me.rdoWaterfall.Size = New System.Drawing.Size(67, 17)
        Me.rdoWaterfall.TabIndex = 0
        Me.rdoWaterfall.TabStop = True
        Me.rdoWaterfall.Text = "Waterfall"
        Me.ToolTip1.SetToolTip(Me.rdoWaterfall, "Enable Waterfall graphics")
        Me.rdoWaterfall.UseVisualStyleBackColor = True
        '
        'chkRadioControl
        '
        Me.chkRadioControl.AutoSize = True
        Me.chkRadioControl.Location = New System.Drawing.Point(27, 275)
        Me.chkRadioControl.Name = "chkRadioControl"
        Me.chkRadioControl.Size = New System.Drawing.Size(193, 17)
        Me.chkRadioControl.TabIndex = 117
        Me.chkRadioControl.Text = "Enable Optional TNC Radio Control"
        Me.ToolTip1.SetToolTip(Me.chkRadioControl, "Enable Optional Radio Control and Radio Control Menu")
        Me.chkRadioControl.UseVisualStyleBackColor = True
        '
        'chkEnableCWID
        '
        Me.chkEnableCWID.AutoSize = True
        Me.chkEnableCWID.Location = New System.Drawing.Point(15, 54)
        Me.chkEnableCWID.Name = "chkEnableCWID"
        Me.chkEnableCWID.Size = New System.Drawing.Size(94, 17)
        Me.chkEnableCWID.TabIndex = 116
        Me.chkEnableCWID.Text = "Enable CW ID"
        Me.ToolTip1.SetToolTip(Me.chkEnableCWID, "enable FSK CW ID on ID frames")
        Me.chkEnableCWID.UseVisualStyleBackColor = True
        '
        'chkCMDTrace
        '
        Me.chkCMDTrace.AutoSize = True
        Me.chkCMDTrace.Location = New System.Drawing.Point(15, 108)
        Me.chkCMDTrace.Name = "chkCMDTrace"
        Me.chkCMDTrace.Size = New System.Drawing.Size(140, 17)
        Me.chkCMDTrace.TabIndex = 119
        Me.chkCMDTrace.Text = "Enable Command Trace"
        Me.ToolTip1.SetToolTip(Me.chkCMDTrace, "Enable Host command logging (for debugging Host TNC interfaces) ")
        Me.chkCMDTrace.UseVisualStyleBackColor = True
        '
        'chkDebugLog
        '
        Me.chkDebugLog.AutoSize = True
        Me.chkDebugLog.Location = New System.Drawing.Point(15, 90)
        Me.chkDebugLog.Name = "chkDebugLog"
        Me.chkDebugLog.Size = New System.Drawing.Size(154, 17)
        Me.chkDebugLog.TabIndex = 118
        Me.chkDebugLog.Text = "Enable TNC debug logging"
        Me.ToolTip1.SetToolTip(Me.chkDebugLog, "Enable verbose debug logging with time tags")
        Me.chkDebugLog.UseVisualStyleBackColor = True
        '
        'cmbPlayback
        '
        Me.cmbPlayback.FormattingEnabled = True
        Me.cmbPlayback.Location = New System.Drawing.Point(294, 116)
        Me.cmbPlayback.Name = "cmbPlayback"
        Me.cmbPlayback.Size = New System.Drawing.Size(321, 21)
        Me.cmbPlayback.TabIndex = 123
        Me.ToolTip1.SetToolTip(Me.cmbPlayback, "Windows installed Playback devices.  Green background indicates device in ini fil" & _
        "e is installed. ")
        '
        'cmbCapture
        '
        Me.cmbCapture.FormattingEnabled = True
        Me.cmbCapture.Location = New System.Drawing.Point(296, 73)
        Me.cmbCapture.Name = "cmbCapture"
        Me.cmbCapture.Size = New System.Drawing.Size(320, 21)
        Me.cmbCapture.TabIndex = 122
        Me.ToolTip1.SetToolTip(Me.cmbCapture, "Windows installed Capture devices  Green background indicates device in ini file " & _
        "is installed. ")
        '
        'chkAccumStats
        '
        Me.chkAccumStats.AutoSize = True
        Me.chkAccumStats.Location = New System.Drawing.Point(15, 126)
        Me.chkAccumStats.Name = "chkAccumStats"
        Me.chkAccumStats.Size = New System.Drawing.Size(109, 17)
        Me.chkAccumStats.TabIndex = 124
        Me.chkAccumStats.Text = "Accumulate Stats"
        Me.ToolTip1.SetToolTip(Me.chkAccumStats, "Accumulate Statistics (used during testing only)")
        Me.chkAccumStats.UseVisualStyleBackColor = True
        '
        'cmbBandwidth
        '
        Me.cmbBandwidth.FormattingEnabled = True
        Me.cmbBandwidth.Items.AddRange(New Object() {"200MAX", "500MAX", "1000MAX", "2000MAX", "200FORCED", "500FORCED", "1000FORCED", "2000FORCED"})
        Me.cmbBandwidth.Location = New System.Drawing.Point(105, 193)
        Me.cmbBandwidth.Name = "cmbBandwidth"
        Me.cmbBandwidth.Size = New System.Drawing.Size(109, 21)
        Me.cmbBandwidth.TabIndex = 126
        Me.ToolTip1.SetToolTip(Me.cmbBandwidth, "Sets the highest bandwidth this station will use or the forced bandwidth. ")
        '
        'Label13
        '
        Me.Label13.AutoSize = True
        Me.Label13.Location = New System.Drawing.Point(288, 223)
        Me.Label13.Name = "Label13"
        Me.Label13.Size = New System.Drawing.Size(108, 13)
        Me.Label13.TabIndex = 132
        Me.Label13.Text = "Tuning Range +/- Hz"
        Me.ToolTip1.SetToolTip(Me.Label13, "Squelch ...lower = more sensitive (default = 5)")
        '
        'nudTuning
        '
        Me.nudTuning.Increment = New Decimal(New Integer() {50, 0, 0, 0})
        Me.nudTuning.Location = New System.Drawing.Point(310, 242)
        Me.nudTuning.Maximum = New Decimal(New Integer() {200, 0, 0, 0})
        Me.nudTuning.Name = "nudTuning"
        Me.nudTuning.Size = New System.Drawing.Size(47, 20)
        Me.nudTuning.TabIndex = 131
        Me.ToolTip1.SetToolTip(Me.nudTuning, "DSP Tuning Range in Hz (adjust position of Green lines on Waterfall/Spectrum)  0 " & _
        "is for FM mode only.")
        Me.nudTuning.Value = New Decimal(New Integer() {100, 0, 0, 0})
        '
        'Label10
        '
        Me.Label10.AutoSize = True
        Me.Label10.Location = New System.Drawing.Point(138, 223)
        Me.Label10.Name = "Label10"
        Me.Label10.Size = New System.Drawing.Size(125, 13)
        Me.Label10.TabIndex = 130
        Me.Label10.Text = "Busy Det Squelch: (1-10)"
        Me.ToolTip1.SetToolTip(Me.Label10, "Squelch ...lower = more sensitive (default = 5)")
        '
        'nudSquelch
        '
        Me.nudSquelch.Location = New System.Drawing.Point(182, 242)
        Me.nudSquelch.Maximum = New Decimal(New Integer() {10, 0, 0, 0})
        Me.nudSquelch.Minimum = New Decimal(New Integer() {1, 0, 0, 0})
        Me.nudSquelch.Name = "nudSquelch"
        Me.nudSquelch.Size = New System.Drawing.Size(37, 20)
        Me.nudSquelch.TabIndex = 129
        Me.ToolTip1.SetToolTip(Me.nudSquelch, "Busy detector squelch ...lower = more sensitive (default of 5 should work for mos" & _
        "t applications)")
        Me.nudSquelch.Value = New Decimal(New Integer() {5, 0, 0, 0})
        '
        'nudDriveLevel
        '
        Me.nudDriveLevel.Location = New System.Drawing.Point(26, 242)
        Me.nudDriveLevel.Name = "nudDriveLevel"
        Me.nudDriveLevel.Size = New System.Drawing.Size(52, 20)
        Me.nudDriveLevel.TabIndex = 127
        Me.ToolTip1.SetToolTip(Me.nudDriveLevel, "Drive level 0 to 100, Default = 90")
        '
        'Label19
        '
        Me.Label19.AutoSize = True
        Me.Label19.Location = New System.Drawing.Point(16, 160)
        Me.Label19.Name = "Label19"
        Me.Label19.Size = New System.Drawing.Size(89, 13)
        Me.Label19.TabIndex = 136
        Me.Label19.Text = "FEC Frame Type:"
        Me.ToolTip1.SetToolTip(Me.Label19, "Number of frame repeats in FSK (multicast) transmission")
        '
        'cmbFECType
        '
        Me.cmbFECType.FormattingEnabled = True
        Me.cmbFECType.Location = New System.Drawing.Point(106, 155)
        Me.cmbFECType.Name = "cmbFECType"
        Me.cmbFECType.Size = New System.Drawing.Size(126, 21)
        Me.cmbFECType.TabIndex = 135
        Me.ToolTip1.SetToolTip(Me.cmbFECType, "Sets the Frame type (modulation and bandwidth) used for FEC frames")
        '
        'Label18
        '
        Me.Label18.AutoSize = True
        Me.Label18.Location = New System.Drawing.Point(242, 159)
        Me.Label18.Name = "Label18"
        Me.Label18.Size = New System.Drawing.Size(73, 13)
        Me.Label18.TabIndex = 134
        Me.Label18.Text = "FEC Repeats:"
        Me.ToolTip1.SetToolTip(Me.Label18, "Number of frame repeats in FSK (multicast) transmission")
        '
        'nudFECRepeats
        '
        Me.nudFECRepeats.Location = New System.Drawing.Point(316, 156)
        Me.nudFECRepeats.Maximum = New Decimal(New Integer() {5, 0, 0, 0})
        Me.nudFECRepeats.Name = "nudFECRepeats"
        Me.nudFECRepeats.Size = New System.Drawing.Size(37, 20)
        Me.nudFECRepeats.TabIndex = 133
        Me.ToolTip1.SetToolTip(Me.nudFECRepeats, "Sets the number of repeates for FEC frames (0-5)")
        Me.nudFECRepeats.Value = New Decimal(New Integer() {5, 0, 0, 0})
        '
        'nudTrailerLength
        '
        Me.nudTrailerLength.Increment = New Decimal(New Integer() {10, 0, 0, 0})
        Me.nudTrailerLength.Location = New System.Drawing.Point(535, 242)
        Me.nudTrailerLength.Maximum = New Decimal(New Integer() {200, 0, 0, 0})
        Me.nudTrailerLength.Name = "nudTrailerLength"
        Me.nudTrailerLength.Size = New System.Drawing.Size(52, 20)
        Me.nudTrailerLength.TabIndex = 139
        Me.ToolTip1.SetToolTip(Me.nudTrailerLength, "0 to 200 ms, Default = 0 (may be needed for some Radios)")
        '
        'nudLeaderLength
        '
        Me.nudLeaderLength.Increment = New Decimal(New Integer() {20, 0, 0, 0})
        Me.nudLeaderLength.Location = New System.Drawing.Point(417, 241)
        Me.nudLeaderLength.Maximum = New Decimal(New Integer() {1200, 0, 0, 0})
        Me.nudLeaderLength.Minimum = New Decimal(New Integer() {100, 0, 0, 0})
        Me.nudLeaderLength.Name = "nudLeaderLength"
        Me.nudLeaderLength.Size = New System.Drawing.Size(52, 20)
        Me.nudLeaderLength.TabIndex = 137
        Me.ToolTip1.SetToolTip(Me.nudLeaderLength, "Leader + Sync (100 - 1200 ms)  Default = 160 ms")
        Me.nudLeaderLength.Value = New Decimal(New Integer() {100, 0, 0, 0})
        '
        'txtCallsign
        '
        Me.txtCallsign.Location = New System.Drawing.Point(493, 28)
        Me.txtCallsign.Name = "txtCallsign"
        Me.txtCallsign.Size = New System.Drawing.Size(123, 20)
        Me.txtCallsign.TabIndex = 144
        Me.txtCallsign.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        Me.ToolTip1.SetToolTip(Me.txtCallsign, "7 character max callsign + optional -SSID (-1 to -15 or -A to -Z) ")
        '
        'chkFECId
        '
        Me.chkFECId.AutoSize = True
        Me.chkFECId.Location = New System.Drawing.Point(363, 158)
        Me.chkFECId.Name = "chkFECId"
        Me.chkFECId.Size = New System.Drawing.Size(55, 17)
        Me.chkFECId.TabIndex = 146
        Me.chkFECId.Text = "FECId"
        Me.ToolTip1.SetToolTip(Me.chkFECId, "enable auto ID Frame on FEC transmissions")
        Me.chkFECId.UseVisualStyleBackColor = True
        '
        'Label12
        '
        Me.Label12.AutoSize = True
        Me.Label12.Location = New System.Drawing.Point(238, 196)
        Me.Label12.Name = "Label12"
        Me.Label12.Size = New System.Drawing.Size(162, 13)
        Me.Label12.TabIndex = 148
        Me.Label12.Text = "ARQ Connect Request Repeats:"
        Me.ToolTip1.SetToolTip(Me.Label12, "Number of frame repeats in FSK (multicast) transmission")
        '
        'nudARQConReqRpt
        '
        Me.nudARQConReqRpt.Location = New System.Drawing.Point(402, 193)
        Me.nudARQConReqRpt.Maximum = New Decimal(New Integer() {15, 0, 0, 0})
        Me.nudARQConReqRpt.Minimum = New Decimal(New Integer() {2, 0, 0, 0})
        Me.nudARQConReqRpt.Name = "nudARQConReqRpt"
        Me.nudARQConReqRpt.Size = New System.Drawing.Size(37, 20)
        Me.nudARQConReqRpt.TabIndex = 147
        Me.ToolTip1.SetToolTip(Me.nudARQConReqRpt, "Sets the number of repeates for ARQ Connect Requests before timeout (2-15)")
        Me.nudARQConReqRpt.Value = New Decimal(New Integer() {5, 0, 0, 0})
        '
        'cmbProtocolMode
        '
        Me.cmbProtocolMode.FormattingEnabled = True
        Me.cmbProtocolMode.Items.AddRange(New Object() {"ARQ", "FEC"})
        Me.cmbProtocolMode.Location = New System.Drawing.Point(560, 155)
        Me.cmbProtocolMode.Name = "cmbProtocolMode"
        Me.cmbProtocolMode.Size = New System.Drawing.Size(55, 21)
        Me.cmbProtocolMode.TabIndex = 149
        Me.cmbProtocolMode.Text = "ARQ"
        Me.ToolTip1.SetToolTip(Me.cmbProtocolMode, "Select protocol Mode ARQ or FEC")
        '
        'grpHost
        '
        Me.grpHost.Controls.Add(Me.Label17)
        Me.grpHost.Controls.Add(Me.Label16)
        Me.grpHost.Controls.Add(Me.cmbBaud)
        Me.grpHost.Controls.Add(Me.Label15)
        Me.grpHost.Controls.Add(Me.cmbCOM)
        Me.grpHost.Controls.Add(Me.rdoSerial)
        Me.grpHost.Controls.Add(Me.Label14)
        Me.grpHost.Controls.Add(Me.cmbPairing)
        Me.grpHost.Controls.Add(Me.rdoBlueTooth)
        Me.grpHost.Controls.Add(Me.rdoTCP)
        Me.grpHost.Controls.Add(Me.chkSecureLogin)
        Me.grpHost.Controls.Add(Me.Label5)
        Me.grpHost.Controls.Add(Me.txtPassword)
        Me.grpHost.Controls.Add(Me.Label9)
        Me.grpHost.Controls.Add(Me.txtTCPAddress)
        Me.grpHost.Controls.Add(Me.Label3)
        Me.grpHost.Controls.Add(Me.txtTCIPControl)
        Me.grpHost.Location = New System.Drawing.Point(7, 7)
        Me.grpHost.Name = "grpHost"
        Me.grpHost.Size = New System.Drawing.Size(629, 157)
        Me.grpHost.TabIndex = 113
        Me.grpHost.TabStop = False
        Me.grpHost.Text = "Host Interface"
        '
        'Label17
        '
        Me.Label17.ForeColor = System.Drawing.Color.Red
        Me.Label17.Location = New System.Drawing.Point(7, 14)
        Me.Label17.Name = "Label17"
        Me.Label17.Size = New System.Drawing.Size(316, 32)
        Me.Label17.TabIndex = 127
        Me.Label17.Text = "These host interface parameters are normally set in the command line when the Hos" & _
    "t launches the ARDOP Win TNC."
        '
        'Label16
        '
        Me.Label16.AutoSize = True
        Me.Label16.Location = New System.Drawing.Point(213, 95)
        Me.Label16.Name = "Label16"
        Me.Label16.Size = New System.Drawing.Size(35, 13)
        Me.Label16.TabIndex = 126
        Me.Label16.Text = "Baud:"
        '
        'Label15
        '
        Me.Label15.AutoSize = True
        Me.Label15.Location = New System.Drawing.Point(62, 95)
        Me.Label15.Name = "Label15"
        Me.Label15.Size = New System.Drawing.Size(56, 13)
        Me.Label15.TabIndex = 124
        Me.Label15.Text = "COM Port:"
        '
        'Label14
        '
        Me.Label14.AutoSize = True
        Me.Label14.Location = New System.Drawing.Point(100, 127)
        Me.Label14.Name = "Label14"
        Me.Label14.Size = New System.Drawing.Size(42, 13)
        Me.Label14.TabIndex = 121
        Me.Label14.Text = "Pairing:"
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Location = New System.Drawing.Point(454, 29)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(56, 13)
        Me.Label5.TabIndex = 115
        Me.Label5.Text = "Password:"
        '
        'Label9
        '
        Me.Label9.AutoSize = True
        Me.Label9.Location = New System.Drawing.Point(89, 62)
        Me.Label9.Name = "Label9"
        Me.Label9.Size = New System.Drawing.Size(72, 13)
        Me.Label9.TabIndex = 113
        Me.Label9.Text = "TCP Address:"
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(475, 62)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(70, 13)
        Me.Label3.TabIndex = 111
        Me.Label3.Text = "TCPIP Port#:"
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.Label22)
        Me.GroupBox1.Controls.Add(Me.nudARQTimeout)
        Me.GroupBox1.Controls.Add(Me.Label21)
        Me.GroupBox1.Controls.Add(Me.cmbProtocolMode)
        Me.GroupBox1.Controls.Add(Me.Label12)
        Me.GroupBox1.Controls.Add(Me.nudARQConReqRpt)
        Me.GroupBox1.Controls.Add(Me.chkFECId)
        Me.GroupBox1.Controls.Add(Me.Label1)
        Me.GroupBox1.Controls.Add(Me.txtCallsign)
        Me.GroupBox1.Controls.Add(Me.Label20)
        Me.GroupBox1.Controls.Add(Me.Label11)
        Me.GroupBox1.Controls.Add(Me.nudTrailerLength)
        Me.GroupBox1.Controls.Add(Me.Label2)
        Me.GroupBox1.Controls.Add(Me.nudLeaderLength)
        Me.GroupBox1.Controls.Add(Me.Label19)
        Me.GroupBox1.Controls.Add(Me.cmbFECType)
        Me.GroupBox1.Controls.Add(Me.Label18)
        Me.GroupBox1.Controls.Add(Me.nudFECRepeats)
        Me.GroupBox1.Controls.Add(Me.Label13)
        Me.GroupBox1.Controls.Add(Me.nudTuning)
        Me.GroupBox1.Controls.Add(Me.Label10)
        Me.GroupBox1.Controls.Add(Me.nudSquelch)
        Me.GroupBox1.Controls.Add(Me.Label6)
        Me.GroupBox1.Controls.Add(Me.nudDriveLevel)
        Me.GroupBox1.Controls.Add(Me.cmbBandwidth)
        Me.GroupBox1.Controls.Add(Me.Label4)
        Me.GroupBox1.Controls.Add(Me.chkAccumStats)
        Me.GroupBox1.Controls.Add(Me.cmbPlayback)
        Me.GroupBox1.Controls.Add(Me.cmbCapture)
        Me.GroupBox1.Controls.Add(Me.Label8)
        Me.GroupBox1.Controls.Add(Me.Label7)
        Me.GroupBox1.Controls.Add(Me.chkCMDTrace)
        Me.GroupBox1.Controls.Add(Me.chkDebugLog)
        Me.GroupBox1.Controls.Add(Me.chkRadioControl)
        Me.GroupBox1.Controls.Add(Me.chkEnableCWID)
        Me.GroupBox1.Controls.Add(Me.chkStartMinimized)
        Me.GroupBox1.Controls.Add(Me.grpDisplay)
        Me.GroupBox1.Location = New System.Drawing.Point(10, 175)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Size = New System.Drawing.Size(626, 301)
        Me.GroupBox1.TabIndex = 119
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "TNC Parameters"
        '
        'Label21
        '
        Me.Label21.AutoSize = True
        Me.Label21.Location = New System.Drawing.Point(481, 158)
        Me.Label21.Name = "Label21"
        Me.Label21.Size = New System.Drawing.Size(82, 13)
        Me.Label21.TabIndex = 150
        Me.Label21.Text = "Protocol Mode: "
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(434, 32)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(51, 13)
        Me.Label1.TabIndex = 145
        Me.Label1.Text = "Call Sign:"
        '
        'Label20
        '
        Me.Label20.ForeColor = System.Drawing.Color.Navy
        Me.Label20.Location = New System.Drawing.Point(10, 16)
        Me.Label20.Name = "Label20"
        Me.Label20.Size = New System.Drawing.Size(340, 32)
        Me.Label20.TabIndex = 143
        Me.Label20.Text = "Most of these TNC parameters are normally set by the host program but may be view" & _
    "ed/initialized here for development and testing. "
        '
        'Label11
        '
        Me.Label11.AutoSize = True
        Me.Label11.Location = New System.Drawing.Point(529, 223)
        Me.Label11.Name = "Label11"
        Me.Label11.Size = New System.Drawing.Size(97, 13)
        Me.Label11.TabIndex = 140
        Me.Label11.Text = "Trailer Length (ms):"
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(406, 223)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(101, 13)
        Me.Label2.TabIndex = 138
        Me.Label2.Text = "Leader Length (ms):"
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(17, 223)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(100, 13)
        Me.Label6.TabIndex = 128
        Me.Label6.Text = "Drive Level: (0-100)"
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Location = New System.Drawing.Point(17, 197)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(86, 13)
        Me.Label4.TabIndex = 125
        Me.Label4.Text = "ARQ Bandwidth:"
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Location = New System.Drawing.Point(293, 102)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(150, 13)
        Me.Label8.TabIndex = 121
        Me.Label8.Text = "Sound Card Playback Device:"
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Location = New System.Drawing.Point(294, 56)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(143, 13)
        Me.Label7.TabIndex = 120
        Me.Label7.Text = "Sound Card Capture Device:"
        '
        'chkStartMinimized
        '
        Me.chkStartMinimized.AutoSize = True
        Me.chkStartMinimized.Location = New System.Drawing.Point(15, 72)
        Me.chkStartMinimized.Name = "chkStartMinimized"
        Me.chkStartMinimized.Size = New System.Drawing.Size(122, 17)
        Me.chkStartMinimized.TabIndex = 115
        Me.chkStartMinimized.Text = "Start TNC Minimized"
        Me.chkStartMinimized.UseVisualStyleBackColor = True
        '
        'grpDisplay
        '
        Me.grpDisplay.Controls.Add(Me.rdoDisable)
        Me.grpDisplay.Controls.Add(Me.rdoSpectrum)
        Me.grpDisplay.Controls.Add(Me.rdoWaterfall)
        Me.grpDisplay.Location = New System.Drawing.Point(174, 52)
        Me.grpDisplay.Name = "grpDisplay"
        Me.grpDisplay.Size = New System.Drawing.Size(106, 74)
        Me.grpDisplay.TabIndex = 108
        Me.grpDisplay.TabStop = False
        Me.grpDisplay.Text = "Graphics Options"
        '
        'Label22
        '
        Me.Label22.AutoSize = True
        Me.Label22.Location = New System.Drawing.Point(468, 196)
        Me.Label22.Name = "Label22"
        Me.Label22.Size = New System.Drawing.Size(100, 13)
        Me.Label22.TabIndex = 152
        Me.Label22.Text = "ARQ Timeout (sec):"
        Me.ToolTip1.SetToolTip(Me.Label22, "Number of frame repeats in FSK (multicast) transmission")
        '
        'nudARQTimeout
        '
        Me.nudARQTimeout.Increment = New Decimal(New Integer() {10, 0, 0, 0})
        Me.nudARQTimeout.Location = New System.Drawing.Point(567, 193)
        Me.nudARQTimeout.Maximum = New Decimal(New Integer() {240, 0, 0, 0})
        Me.nudARQTimeout.Minimum = New Decimal(New Integer() {30, 0, 0, 0})
        Me.nudARQTimeout.Name = "nudARQTimeout"
        Me.nudARQTimeout.Size = New System.Drawing.Size(49, 20)
        Me.nudARQTimeout.TabIndex = 151
        Me.ToolTip1.SetToolTip(Me.nudARQTimeout, "Sets the ARQ Timeout value (seconds) ")
        Me.nudARQTimeout.Value = New Decimal(New Integer() {30, 0, 0, 0})
        '
        'TNCSetup
        '
        Me.AcceptButton = Me.OK_Button
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.AutoSize = True
        Me.CancelButton = Me.Cancel_Button
        Me.ClientSize = New System.Drawing.Size(638, 504)
        Me.ControlBox = False
        Me.Controls.Add(Me.GroupBox1)
        Me.Controls.Add(Me.grpHost)
        Me.Controls.Add(Me.Cancel_Button)
        Me.Controls.Add(Me.OK_Button)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "TNCSetup"
        Me.ShowInTaskbar = False
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
        Me.Text = "ARDOP Win TNC Setup"
        CType(Me.nudTuning, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudSquelch, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudDriveLevel, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudFECRepeats, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudTrailerLength, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudLeaderLength, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudARQConReqRpt, System.ComponentModel.ISupportInitialize).EndInit()
        Me.grpHost.ResumeLayout(False)
        Me.grpHost.PerformLayout()
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        Me.grpDisplay.ResumeLayout(False)
        Me.grpDisplay.PerformLayout()
        CType(Me.nudARQTimeout, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)

    End Sub
    Friend WithEvents OK_Button As System.Windows.Forms.Button
    Friend WithEvents ToolTip1 As System.Windows.Forms.ToolTip
    Friend WithEvents Cancel_Button As System.Windows.Forms.Button
    Friend WithEvents grpHost As System.Windows.Forms.GroupBox
    Friend WithEvents Label14 As System.Windows.Forms.Label
    Friend WithEvents cmbPairing As System.Windows.Forms.ComboBox
    Friend WithEvents rdoBlueTooth As System.Windows.Forms.RadioButton
    Friend WithEvents rdoTCP As System.Windows.Forms.RadioButton
    Friend WithEvents chkSecureLogin As System.Windows.Forms.CheckBox
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents txtPassword As System.Windows.Forms.TextBox
    Friend WithEvents Label9 As System.Windows.Forms.Label
    Friend WithEvents txtTCPAddress As System.Windows.Forms.TextBox
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents txtTCIPControl As System.Windows.Forms.TextBox
    Friend WithEvents Label16 As System.Windows.Forms.Label
    Friend WithEvents cmbBaud As System.Windows.Forms.ComboBox
    Friend WithEvents Label15 As System.Windows.Forms.Label
    Friend WithEvents cmbCOM As System.Windows.Forms.ComboBox
    Friend WithEvents rdoSerial As System.Windows.Forms.RadioButton
    Friend WithEvents Label17 As System.Windows.Forms.Label
    Friend WithEvents GroupBox1 As System.Windows.Forms.GroupBox
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents txtCallsign As System.Windows.Forms.TextBox
    Friend WithEvents Label20 As System.Windows.Forms.Label
    Friend WithEvents Label11 As System.Windows.Forms.Label
    Friend WithEvents nudTrailerLength As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents nudLeaderLength As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label19 As System.Windows.Forms.Label
    Friend WithEvents cmbFECType As System.Windows.Forms.ComboBox
    Friend WithEvents Label18 As System.Windows.Forms.Label
    Friend WithEvents nudFECRepeats As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label13 As System.Windows.Forms.Label
    Friend WithEvents nudTuning As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label10 As System.Windows.Forms.Label
    Friend WithEvents nudSquelch As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents nudDriveLevel As System.Windows.Forms.NumericUpDown
    Friend WithEvents cmbBandwidth As System.Windows.Forms.ComboBox
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents chkAccumStats As System.Windows.Forms.CheckBox
    Friend WithEvents cmbPlayback As System.Windows.Forms.ComboBox
    Friend WithEvents cmbCapture As System.Windows.Forms.ComboBox
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents chkCMDTrace As System.Windows.Forms.CheckBox
    Friend WithEvents chkDebugLog As System.Windows.Forms.CheckBox
    Friend WithEvents chkRadioControl As System.Windows.Forms.CheckBox
    Friend WithEvents chkEnableCWID As System.Windows.Forms.CheckBox
    Friend WithEvents chkStartMinimized As System.Windows.Forms.CheckBox
    Friend WithEvents grpDisplay As System.Windows.Forms.GroupBox
    Friend WithEvents rdoDisable As System.Windows.Forms.RadioButton
    Friend WithEvents rdoSpectrum As System.Windows.Forms.RadioButton
    Friend WithEvents rdoWaterfall As System.Windows.Forms.RadioButton
    Friend WithEvents chkFECId As System.Windows.Forms.CheckBox
    Friend WithEvents Label12 As System.Windows.Forms.Label
    Friend WithEvents nudARQConReqRpt As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label21 As System.Windows.Forms.Label
    Friend WithEvents cmbProtocolMode As System.Windows.Forms.ComboBox
    Friend WithEvents Label22 As System.Windows.Forms.Label
    Friend WithEvents nudARQTimeout As System.Windows.Forms.NumericUpDown

End Class
