<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Radio
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
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(Radio))
        Me.btnUpdate = New System.Windows.Forms.Button()
        Me.grpRadioControlPort = New System.Windows.Forms.GroupBox()
        Me.chkControlDTR = New System.Windows.Forms.CheckBox()
        Me.chkControlRTS = New System.Windows.Forms.CheckBox()
        Me.cmbControlBaud = New System.Windows.Forms.ComboBox()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.cmbControlPort = New System.Windows.Forms.ComboBox()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.grpRadioSettings = New System.Windows.Forms.GroupBox()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.chkInternalSC = New System.Windows.Forms.CheckBox()
        Me.cmbAntenna = New System.Windows.Forms.ComboBox()
        Me.lblAntenna = New System.Windows.Forms.Label()
        Me.lblFilter = New System.Windows.Forms.Label()
        Me.cmbFilter = New System.Windows.Forms.ComboBox()
        Me.chkRadioCtrl = New System.Windows.Forms.CheckBox()
        Me.rdoUSBDigital = New System.Windows.Forms.RadioButton()
        Me.txtIcomAddress = New System.Windows.Forms.TextBox()
        Me.lblIcomAddress = New System.Windows.Forms.Label()
        Me.chkInternalTuner = New System.Windows.Forms.CheckBox()
        Me.rdoFM = New System.Windows.Forms.RadioButton()
        Me.rdoUSB = New System.Windows.Forms.RadioButton()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.cmbModel = New System.Windows.Forms.ComboBox()
        Me.ToolTip1 = New System.Windows.Forms.ToolTip(Me.components)
        Me.chkKeyDTR = New System.Windows.Forms.CheckBox()
        Me.chkKeyRTS = New System.Windows.Forms.CheckBox()
        Me.cmbPTTCtrl = New System.Windows.Forms.ComboBox()
        Me.btnClose = New System.Windows.Forms.Button()
        Me.grpPTTControl = New System.Windows.Forms.GroupBox()
        Me.lblHold = New System.Windows.Forms.Label()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.grpRadioControlPort.SuspendLayout()
        Me.grpRadioSettings.SuspendLayout()
        Me.grpPTTControl.SuspendLayout()
        Me.SuspendLayout()
        '
        'btnUpdate
        '
        Me.btnUpdate.BackColor = System.Drawing.Color.LightSalmon
        Me.btnUpdate.Location = New System.Drawing.Point(449, 263)
        Me.btnUpdate.Name = "btnUpdate"
        Me.btnUpdate.Size = New System.Drawing.Size(105, 21)
        Me.btnUpdate.TabIndex = 5
        Me.btnUpdate.Text = "Save to ini File"
        Me.btnUpdate.UseVisualStyleBackColor = False
        '
        'grpRadioControlPort
        '
        Me.grpRadioControlPort.Controls.Add(Me.chkControlDTR)
        Me.grpRadioControlPort.Controls.Add(Me.chkControlRTS)
        Me.grpRadioControlPort.Controls.Add(Me.cmbControlBaud)
        Me.grpRadioControlPort.Controls.Add(Me.Label7)
        Me.grpRadioControlPort.Controls.Add(Me.cmbControlPort)
        Me.grpRadioControlPort.Controls.Add(Me.Label6)
        Me.grpRadioControlPort.Location = New System.Drawing.Point(12, 123)
        Me.grpRadioControlPort.Name = "grpRadioControlPort"
        Me.grpRadioControlPort.Size = New System.Drawing.Size(542, 52)
        Me.grpRadioControlPort.TabIndex = 1
        Me.grpRadioControlPort.TabStop = False
        Me.grpRadioControlPort.Text = "Radio Control Port"
        '
        'chkControlDTR
        '
        Me.chkControlDTR.AutoSize = True
        Me.chkControlDTR.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.chkControlDTR.Location = New System.Drawing.Point(447, 22)
        Me.chkControlDTR.Name = "chkControlDTR"
        Me.chkControlDTR.Size = New System.Drawing.Size(85, 17)
        Me.chkControlDTR.TabIndex = 8
        Me.chkControlDTR.Text = "Enable DTR"
        Me.ToolTip1.SetToolTip(Me.chkControlDTR, "Check to enable DTR Signal (may be required for some radios)")
        Me.chkControlDTR.UseVisualStyleBackColor = True
        '
        'chkControlRTS
        '
        Me.chkControlRTS.AutoSize = True
        Me.chkControlRTS.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.chkControlRTS.Location = New System.Drawing.Point(332, 22)
        Me.chkControlRTS.Name = "chkControlRTS"
        Me.chkControlRTS.Size = New System.Drawing.Size(84, 17)
        Me.chkControlRTS.TabIndex = 7
        Me.chkControlRTS.Text = "Enable RTS"
        Me.ToolTip1.SetToolTip(Me.chkControlRTS, "Check to enable RTS Signal (may be required for some radios)")
        Me.chkControlRTS.UseVisualStyleBackColor = True
        '
        'cmbControlBaud
        '
        Me.cmbControlBaud.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cmbControlBaud.FormattingEnabled = True
        Me.cmbControlBaud.Items.AddRange(New Object() {"1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200"})
        Me.cmbControlBaud.Location = New System.Drawing.Point(236, 20)
        Me.cmbControlBaud.Name = "cmbControlBaud"
        Me.cmbControlBaud.Size = New System.Drawing.Size(69, 21)
        Me.cmbControlBaud.TabIndex = 6
        Me.ToolTip1.SetToolTip(Me.cmbControlBaud, "Select Serial port baud rate (recommend 9600 or above)")
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Location = New System.Drawing.Point(198, 23)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(32, 13)
        Me.Label7.TabIndex = 47
        Me.Label7.Text = "Baud"
        '
        'cmbControlPort
        '
        Me.cmbControlPort.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cmbControlPort.FormattingEnabled = True
        Me.cmbControlPort.Location = New System.Drawing.Point(105, 20)
        Me.cmbControlPort.Name = "cmbControlPort"
        Me.cmbControlPort.Size = New System.Drawing.Size(69, 21)
        Me.cmbControlPort.TabIndex = 5
        Me.ToolTip1.SetToolTip(Me.cmbControlPort, "Select radio control COM port ")
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(11, 23)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(89, 13)
        Me.Label6.TabIndex = 45
        Me.Label6.Text = "Serial Port to Use"
        '
        'grpRadioSettings
        '
        Me.grpRadioSettings.Controls.Add(Me.Label1)
        Me.grpRadioSettings.Controls.Add(Me.chkInternalSC)
        Me.grpRadioSettings.Controls.Add(Me.cmbAntenna)
        Me.grpRadioSettings.Controls.Add(Me.lblAntenna)
        Me.grpRadioSettings.Controls.Add(Me.lblFilter)
        Me.grpRadioSettings.Controls.Add(Me.cmbFilter)
        Me.grpRadioSettings.Controls.Add(Me.rdoUSBDigital)
        Me.grpRadioSettings.Controls.Add(Me.txtIcomAddress)
        Me.grpRadioSettings.Controls.Add(Me.lblIcomAddress)
        Me.grpRadioSettings.Controls.Add(Me.chkInternalTuner)
        Me.grpRadioSettings.Controls.Add(Me.rdoFM)
        Me.grpRadioSettings.Controls.Add(Me.rdoUSB)
        Me.grpRadioSettings.Controls.Add(Me.Label2)
        Me.grpRadioSettings.Controls.Add(Me.cmbModel)
        Me.grpRadioSettings.Location = New System.Drawing.Point(12, 12)
        Me.grpRadioSettings.Name = "grpRadioSettings"
        Me.grpRadioSettings.Size = New System.Drawing.Size(542, 101)
        Me.grpRadioSettings.TabIndex = 2
        Me.grpRadioSettings.TabStop = False
        Me.grpRadioSettings.Text = "Radio Selection"
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Italic, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.ForeColor = System.Drawing.Color.Red
        Me.Label1.Location = New System.Drawing.Point(127, 0)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(377, 13)
        Me.Label1.TabIndex = 47
        Me.Label1.Text = "Note: Radio control features Filter, Antenna, and Tuner disabled in this revision" & _
    ""
        '
        'chkInternalSC
        '
        Me.chkInternalSC.AutoSize = True
        Me.chkInternalSC.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.chkInternalSC.Enabled = False
        Me.chkInternalSC.Location = New System.Drawing.Point(332, 75)
        Me.chkInternalSC.Name = "chkInternalSC"
        Me.chkInternalSC.Size = New System.Drawing.Size(183, 17)
        Me.chkInternalSC.TabIndex = 60
        Me.chkInternalSC.Text = "Use Radio's Internal  Sound Card"
        Me.ToolTip1.SetToolTip(Me.chkInternalSC, "Enable Radio Control ")
        Me.chkInternalSC.UseVisualStyleBackColor = True
        '
        'cmbAntenna
        '
        Me.cmbAntenna.FormattingEnabled = True
        Me.cmbAntenna.Items.AddRange(New Object() {"0 ", "1", "2"})
        Me.cmbAntenna.Location = New System.Drawing.Point(111, 73)
        Me.cmbAntenna.Name = "cmbAntenna"
        Me.cmbAntenna.Size = New System.Drawing.Size(39, 21)
        Me.cmbAntenna.TabIndex = 58
        Me.cmbAntenna.Text = "0 "
        Me.ToolTip1.SetToolTip(Me.cmbAntenna, "Antenna selector (some radios)  0 = use Default setting")
        '
        'lblAntenna
        '
        Me.lblAntenna.AutoSize = True
        Me.lblAntenna.Location = New System.Drawing.Point(10, 76)
        Me.lblAntenna.Name = "lblAntenna"
        Me.lblAntenna.Size = New System.Drawing.Size(94, 13)
        Me.lblAntenna.TabIndex = 59
        Me.lblAntenna.Text = "Antenna Selection"
        '
        'lblFilter
        '
        Me.lblFilter.Location = New System.Drawing.Point(314, 16)
        Me.lblFilter.Name = "lblFilter"
        Me.lblFilter.Size = New System.Drawing.Size(155, 31)
        Me.lblFilter.TabIndex = 57
        Me.lblFilter.Text = "Filter Bandwidth (some radios):     0 disables filter control. "
        '
        'cmbFilter
        '
        Me.cmbFilter.FormattingEnabled = True
        Me.cmbFilter.Items.AddRange(New Object() {"0", "300", "600", "1200", "2200"})
        Me.cmbFilter.Location = New System.Drawing.Point(475, 16)
        Me.cmbFilter.Name = "cmbFilter"
        Me.cmbFilter.Size = New System.Drawing.Size(57, 21)
        Me.cmbFilter.TabIndex = 56
        Me.cmbFilter.Text = "0"
        Me.ToolTip1.SetToolTip(Me.cmbFilter, "Set Filter freq (some radios)  0 disables filter control ")
        '
        'chkRadioCtrl
        '
        Me.chkRadioCtrl.AutoSize = True
        Me.chkRadioCtrl.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.chkRadioCtrl.Location = New System.Drawing.Point(12, 267)
        Me.chkRadioCtrl.Name = "chkRadioCtrl"
        Me.chkRadioCtrl.Size = New System.Drawing.Size(202, 17)
        Me.chkRadioCtrl.TabIndex = 55
        Me.chkRadioCtrl.Text = "Enable TNC Control of  Radio or PTT"
        Me.ToolTip1.SetToolTip(Me.chkRadioCtrl, "Check to allow TNC to control radio or PTT.")
        Me.chkRadioCtrl.UseVisualStyleBackColor = True
        '
        'rdoUSBDigital
        '
        Me.rdoUSBDigital.AutoSize = True
        Me.rdoUSBDigital.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.rdoUSBDigital.Location = New System.Drawing.Point(211, 51)
        Me.rdoUSBDigital.Name = "rdoUSBDigital"
        Me.rdoUSBDigital.Size = New System.Drawing.Size(79, 17)
        Me.rdoUSBDigital.TabIndex = 52
        Me.rdoUSBDigital.Text = "USB Digital"
        Me.ToolTip1.SetToolTip(Me.rdoUSBDigital, "Select USB Digital mode (some radios)")
        Me.rdoUSBDigital.UseVisualStyleBackColor = True
        '
        'txtIcomAddress
        '
        Me.txtIcomAddress.Location = New System.Drawing.Point(83, 50)
        Me.txtIcomAddress.Name = "txtIcomAddress"
        Me.txtIcomAddress.Size = New System.Drawing.Size(27, 20)
        Me.txtIcomAddress.TabIndex = 13
        Me.txtIcomAddress.Text = "00"
        Me.txtIcomAddress.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        Me.ToolTip1.SetToolTip(Me.txtIcomAddress, "Icom CI-V hex address 00 to FF (must match setting in radio)")
        '
        'lblIcomAddress
        '
        Me.lblIcomAddress.AutoSize = True
        Me.lblIcomAddress.Location = New System.Drawing.Point(11, 53)
        Me.lblIcomAddress.Name = "lblIcomAddress"
        Me.lblIcomAddress.Size = New System.Drawing.Size(71, 13)
        Me.lblIcomAddress.TabIndex = 51
        Me.lblIcomAddress.Text = "Icom Address"
        '
        'chkInternalTuner
        '
        Me.chkInternalTuner.AutoSize = True
        Me.chkInternalTuner.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.chkInternalTuner.Enabled = False
        Me.chkInternalTuner.Location = New System.Drawing.Point(183, 75)
        Me.chkInternalTuner.Name = "chkInternalTuner"
        Me.chkInternalTuner.Size = New System.Drawing.Size(114, 17)
        Me.chkInternalTuner.TabIndex = 16
        Me.chkInternalTuner.Text = "Use Internal Tuner"
        Me.ToolTip1.SetToolTip(Me.chkInternalTuner, "Enable Internal Tuner")
        Me.chkInternalTuner.UseVisualStyleBackColor = True
        '
        'rdoFM
        '
        Me.rdoFM.AutoSize = True
        Me.rdoFM.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.rdoFM.Location = New System.Drawing.Point(317, 53)
        Me.rdoFM.Name = "rdoFM"
        Me.rdoFM.Size = New System.Drawing.Size(40, 17)
        Me.rdoFM.TabIndex = 15
        Me.rdoFM.Text = "FM"
        Me.ToolTip1.SetToolTip(Me.rdoFM, "Select FM mode(some radios)")
        Me.rdoFM.UseVisualStyleBackColor = True
        '
        'rdoUSB
        '
        Me.rdoUSB.AutoSize = True
        Me.rdoUSB.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.rdoUSB.Checked = True
        Me.rdoUSB.Location = New System.Drawing.Point(137, 53)
        Me.rdoUSB.Name = "rdoUSB"
        Me.rdoUSB.Size = New System.Drawing.Size(47, 17)
        Me.rdoUSB.TabIndex = 14
        Me.rdoUSB.TabStop = True
        Me.rdoUSB.Text = "USB"
        Me.ToolTip1.SetToolTip(Me.rdoUSB, "Select Normal USB Mode")
        Me.rdoUSB.UseVisualStyleBackColor = True
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(5, 22)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(100, 13)
        Me.Label2.TabIndex = 41
        Me.Label2.Text = "Select Radio Model"
        '
        'cmbModel
        '
        Me.cmbModel.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cmbModel.FormattingEnabled = True
        Me.cmbModel.Location = New System.Drawing.Point(111, 19)
        Me.cmbModel.Name = "cmbModel"
        Me.cmbModel.Size = New System.Drawing.Size(163, 21)
        Me.cmbModel.TabIndex = 10
        Me.ToolTip1.SetToolTip(Me.cmbModel, "Select Radio model from drop down list.")
        '
        'chkKeyDTR
        '
        Me.chkKeyDTR.AutoSize = True
        Me.chkKeyDTR.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.chkKeyDTR.Location = New System.Drawing.Point(395, 19)
        Me.chkKeyDTR.Name = "chkKeyDTR"
        Me.chkKeyDTR.Size = New System.Drawing.Size(101, 17)
        Me.chkKeyDTR.TabIndex = 8
        Me.chkKeyDTR.Text = "PTT using DTR"
        Me.ToolTip1.SetToolTip(Me.chkKeyDTR, "Check to enable PTT Keying with DTR Signal")
        Me.chkKeyDTR.UseVisualStyleBackColor = True
        '
        'chkKeyRTS
        '
        Me.chkKeyRTS.AutoSize = True
        Me.chkKeyRTS.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.chkKeyRTS.Location = New System.Drawing.Point(252, 19)
        Me.chkKeyRTS.Name = "chkKeyRTS"
        Me.chkKeyRTS.Size = New System.Drawing.Size(100, 17)
        Me.chkKeyRTS.TabIndex = 7
        Me.chkKeyRTS.Text = "PTT using RTS"
        Me.ToolTip1.SetToolTip(Me.chkKeyRTS, "Check to enable PTT keying with RTS Signal")
        Me.chkKeyRTS.UseVisualStyleBackColor = True
        '
        'cmbPTTCtrl
        '
        Me.cmbPTTCtrl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cmbPTTCtrl.FormattingEnabled = True
        Me.cmbPTTCtrl.Location = New System.Drawing.Point(120, 20)
        Me.cmbPTTCtrl.Name = "cmbPTTCtrl"
        Me.cmbPTTCtrl.Size = New System.Drawing.Size(113, 21)
        Me.cmbPTTCtrl.TabIndex = 5
        Me.ToolTip1.SetToolTip(Me.cmbPTTCtrl, "Select PTT Mode:  CAT PTT, VOX/SignaLink or COM Port to use for Direct PTT using " & _
        "RTS or DTR")
        '
        'btnClose
        '
        Me.btnClose.BackColor = System.Drawing.Color.LightGreen
        Me.btnClose.Location = New System.Drawing.Point(296, 263)
        Me.btnClose.Name = "btnClose"
        Me.btnClose.Size = New System.Drawing.Size(118, 21)
        Me.btnClose.TabIndex = 6
        Me.btnClose.Text = "Abandon Edits/Close"
        Me.btnClose.UseVisualStyleBackColor = False
        '
        'grpPTTControl
        '
        Me.grpPTTControl.Controls.Add(Me.lblHold)
        Me.grpPTTControl.Controls.Add(Me.chkKeyDTR)
        Me.grpPTTControl.Controls.Add(Me.chkKeyRTS)
        Me.grpPTTControl.Controls.Add(Me.cmbPTTCtrl)
        Me.grpPTTControl.Controls.Add(Me.Label3)
        Me.grpPTTControl.Location = New System.Drawing.Point(11, 187)
        Me.grpPTTControl.Name = "grpPTTControl"
        Me.grpPTTControl.Size = New System.Drawing.Size(542, 70)
        Me.grpPTTControl.TabIndex = 7
        Me.grpPTTControl.TabStop = False
        Me.grpPTTControl.Text = "PTT Control"
        '
        'lblHold
        '
        Me.lblHold.AutoSize = True
        Me.lblHold.ForeColor = System.Drawing.Color.Red
        Me.lblHold.Location = New System.Drawing.Point(79, 46)
        Me.lblHold.Name = "lblHold"
        Me.lblHold.Size = New System.Drawing.Size(217, 13)
        Me.lblHold.TabIndex = 46
        Me.lblHold.Text = "Set VOX Hold or SignaLink DLY to minimum!"
        Me.lblHold.Visible = False
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(11, 23)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(103, 13)
        Me.Label3.TabIndex = 45
        Me.Label3.Text = "PTT Mode/ComPort"
        '
        'Radio
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.AutoSize = True
        Me.ClientSize = New System.Drawing.Size(564, 289)
        Me.Controls.Add(Me.grpPTTControl)
        Me.Controls.Add(Me.btnClose)
        Me.Controls.Add(Me.grpRadioSettings)
        Me.Controls.Add(Me.grpRadioControlPort)
        Me.Controls.Add(Me.btnUpdate)
        Me.Controls.Add(Me.chkRadioCtrl)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "Radio"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
        Me.Text = " Radio Settings"
        Me.grpRadioControlPort.ResumeLayout(False)
        Me.grpRadioControlPort.PerformLayout()
        Me.grpRadioSettings.ResumeLayout(False)
        Me.grpRadioSettings.PerformLayout()
        Me.grpPTTControl.ResumeLayout(False)
        Me.grpPTTControl.PerformLayout()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents btnUpdate As System.Windows.Forms.Button
    Friend WithEvents grpRadioControlPort As System.Windows.Forms.GroupBox
    Friend WithEvents chkControlDTR As System.Windows.Forms.CheckBox
    Friend WithEvents chkControlRTS As System.Windows.Forms.CheckBox
    Friend WithEvents cmbControlBaud As System.Windows.Forms.ComboBox
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents cmbControlPort As System.Windows.Forms.ComboBox
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents grpRadioSettings As System.Windows.Forms.GroupBox
    Friend WithEvents txtIcomAddress As System.Windows.Forms.TextBox
    Friend WithEvents lblIcomAddress As System.Windows.Forms.Label
    Friend WithEvents chkInternalTuner As System.Windows.Forms.CheckBox
    Friend WithEvents rdoFM As System.Windows.Forms.RadioButton
    Friend WithEvents rdoUSB As System.Windows.Forms.RadioButton
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents cmbModel As System.Windows.Forms.ComboBox
    Friend WithEvents rdoUSBDigital As System.Windows.Forms.RadioButton
    Friend WithEvents ToolTip1 As System.Windows.Forms.ToolTip
    Friend WithEvents btnClose As System.Windows.Forms.Button
    Friend WithEvents chkRadioCtrl As System.Windows.Forms.CheckBox
    Friend WithEvents grpPTTControl As System.Windows.Forms.GroupBox
    Friend WithEvents chkKeyDTR As System.Windows.Forms.CheckBox
    Friend WithEvents chkKeyRTS As System.Windows.Forms.CheckBox
    Friend WithEvents cmbPTTCtrl As System.Windows.Forms.ComboBox
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents lblFilter As System.Windows.Forms.Label
    Friend WithEvents cmbFilter As System.Windows.Forms.ComboBox
    Friend WithEvents lblHold As System.Windows.Forms.Label
    Friend WithEvents cmbAntenna As System.Windows.Forms.ComboBox
    Friend WithEvents lblAntenna As System.Windows.Forms.Label
    Friend WithEvents chkInternalSC As System.Windows.Forms.CheckBox
    Friend WithEvents Label1 As System.Windows.Forms.Label
End Class
