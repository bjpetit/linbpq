<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class SimpleForm
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        If disposing AndAlso components IsNot Nothing Then
            components.Dispose()
        End If
        MyBase.Dispose(disposing)
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
      Me.components = New System.ComponentModel.Container
      Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(SimpleForm))
      Me.Label1 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.Create = New System.Windows.Forms.Button
      Me.LoadConfig = New System.Windows.Forms.Button
      Me.Label36 = New System.Windows.Forms.Label
      Me.Label35 = New System.Windows.Forms.Label
      Me.Label3 = New System.Windows.Forms.Label
      Me.Label8 = New System.Windows.Forms.Label
      Me.CheckBox1 = New System.Windows.Forms.CheckBox
      Me.CheckBox2 = New System.Windows.Forms.CheckBox
      Me.CheckBox3 = New System.Windows.Forms.CheckBox
      Me.Label9 = New System.Windows.Forms.Label
      Me.Label10 = New System.Windows.Forms.Label
      Me.Label11 = New System.Windows.Forms.Label
      Me.ComboBox1 = New System.Windows.Forms.ComboBox
      Me.ComboBox2 = New System.Windows.Forms.ComboBox
      Me.Label14 = New System.Windows.Forms.Label
      Me.Label15 = New System.Windows.Forms.Label
      Me.CreateFiles = New System.Windows.Forms.Button
      Me.Advanced = New System.Windows.Forms.Button
      Me.AddPortButton = New System.Windows.Forms.Button
      Me.IDIntLabel = New System.Windows.Forms.Label
      Me.IDMsgLabel = New System.Windows.Forms.Label
      Me.Label4 = New System.Windows.Forms.Label
      Me.Appl1 = New System.Windows.Forms.TextBox
      Me.Label5 = New System.Windows.Forms.Label
      Me.Label6 = New System.Windows.Forms.Label
      Me.Appl2 = New System.Windows.Forms.TextBox
      Me.Label7 = New System.Windows.Forms.Label
      Me.Label16 = New System.Windows.Forms.Label
      Me.Label17 = New System.Windows.Forms.Label
      Me.HelpButton1 = New System.Windows.Forms.Button
      Me.ErrorProvider1 = New System.Windows.Forms.ErrorProvider(Me.components)
      Me.PasswordBox = New System.Windows.Forms.TextBox
      Me.LocatorBox = New System.Windows.Forms.TextBox
      Me.Label18 = New System.Windows.Forms.Label
      Me.Label38 = New System.Windows.Forms.Label
      Me.TabControl1 = New System.Windows.Forms.TabControl
      Me.Label12 = New System.Windows.Forms.Label
      Me.Appl4 = New System.Windows.Forms.TextBox
      Me.Label13 = New System.Windows.Forms.Label
      Me.Appl3 = New System.Windows.Forms.TextBox
      Me.Appl4Call = New BPQCFG.CallsignTextBox
      Me.Appl3Call = New BPQCFG.CallsignTextBox
      Me.Appl2Call = New BPQCFG.CallsignTextBox
      Me.Appl1Call = New BPQCFG.CallsignTextBox
      Me.IDIntervalBox = New BPQCFG.DTNumTextBox
      Me.IDMsgBox = New BPQCFG.MultiLineTextBox
      Me.CTEXTBox = New BPQCFG.MultiLineTextBox
      Me.InfoMsgBox = New BPQCFG.MultiLineTextBox
      Me.NodeCallBox = New BPQCFG.CallsignTextBox
      Me.CallsignTextBox1 = New BPQCFG.CallsignTextBox
      Me.CallsignTextBox2 = New BPQCFG.CallsignTextBox
      CType(Me.ErrorProvider1, System.ComponentModel.ISupportInitialize).BeginInit()
      Me.SuspendLayout()
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
      Me.Label1.Location = New System.Drawing.Point(13, 7)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(550, 16)
      Me.Label1.TabIndex = 1
      Me.Label1.Text = "This form is used to create or update a basic BPQ32 Configuration without NETROM " & _
          "support"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
      Me.Label2.Location = New System.Drawing.Point(-2, 33)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(0, 16)
      Me.Label2.TabIndex = 2
      '
      'Create
      '
      Me.Create.Location = New System.Drawing.Point(139, 29)
      Me.Create.Name = "Create"
      Me.Create.Size = New System.Drawing.Size(105, 23)
      Me.Create.TabIndex = 3
      Me.Create.Text = "Create New Config"
      Me.Create.UseVisualStyleBackColor = True
      '
      'LoadConfig
      '
      Me.LoadConfig.Location = New System.Drawing.Point(57, 30)
      Me.LoadConfig.Name = "LoadConfig"
      Me.LoadConfig.Size = New System.Drawing.Size(76, 23)
      Me.LoadConfig.TabIndex = 4
      Me.LoadConfig.Text = "Load Config"
      Me.LoadConfig.UseVisualStyleBackColor = True
      '
      'Label36
      '
      Me.Label36.AutoSize = True
      Me.Label36.Location = New System.Drawing.Point(8, 174)
      Me.Label36.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label36.Name = "Label36"
      Me.Label36.Size = New System.Drawing.Size(218, 13)
      Me.Label36.TabIndex = 52
      Me.Label36.Text = "CTEXT Message (Sent when user connects)"
      '
      'Label35
      '
      Me.Label35.AutoSize = True
      Me.Label35.Location = New System.Drawing.Point(8, 114)
      Me.Label35.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label35.Name = "Label35"
      Me.Label35.Size = New System.Drawing.Size(259, 13)
      Me.Label35.TabIndex = 50
      Me.Label35.Text = "INFO Message (Sent in response to INFO Command )"
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(8, 66)
      Me.Label3.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label3.Name = "Label3"
      Me.Label3.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label3.Size = New System.Drawing.Size(53, 13)
      Me.Label3.TabIndex = 47
      Me.Label3.Text = "Node Call"
      '
      'Label8
      '
      Me.Label8.AutoSize = True
      Me.Label8.Location = New System.Drawing.Point(186, 33)
      Me.Label8.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label8.Name = "Label8"
      Me.Label8.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label8.Size = New System.Drawing.Size(61, 13)
      Me.Label8.TabIndex = 52
      Me.Label8.Text = "Drive Level"
      '
      'CheckBox1
      '
      Me.CheckBox1.AutoSize = True
      Me.CheckBox1.Location = New System.Drawing.Point(489, 33)
      Me.CheckBox1.Name = "CheckBox1"
      Me.CheckBox1.Size = New System.Drawing.Size(85, 17)
      Me.CheckBox1.TabIndex = 51
      Me.CheckBox1.Text = "Enble CWID"
      Me.CheckBox1.UseVisualStyleBackColor = True
      '
      'CheckBox2
      '
      Me.CheckBox2.AutoSize = True
      Me.CheckBox2.Location = New System.Drawing.Point(407, 33)
      Me.CheckBox2.Name = "CheckBox2"
      Me.CheckBox2.Size = New System.Drawing.Size(76, 17)
      Me.CheckBox2.TabIndex = 50
      Me.CheckBox2.Text = "Busy Lock"
      Me.CheckBox2.UseVisualStyleBackColor = True
      '
      'CheckBox3
      '
      Me.CheckBox3.AutoSize = True
      Me.CheckBox3.Location = New System.Drawing.Point(322, 33)
      Me.CheckBox3.Name = "CheckBox3"
      Me.CheckBox3.Size = New System.Drawing.Size(79, 17)
      Me.CheckBox3.TabIndex = 49
      Me.CheckBox3.Text = "Debug Log"
      Me.CheckBox3.UseVisualStyleBackColor = True
      '
      'Label9
      '
      Me.Label9.AutoSize = True
      Me.Label9.Location = New System.Drawing.Point(21, 33)
      Me.Label9.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label9.Name = "Label9"
      Me.Label9.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label9.Size = New System.Drawing.Size(90, 13)
      Me.Label9.TabIndex = 47
      Me.Label9.Text = "Winmor TNC Port"
      '
      'Label10
      '
      Me.Label10.AutoSize = True
      Me.Label10.Location = New System.Drawing.Point(187, 45)
      Me.Label10.Name = "Label10"
      Me.Label10.Size = New System.Drawing.Size(38, 13)
      Me.Label10.TabIndex = 27
      Me.Label10.Text = "Speed"
      '
      'Label11
      '
      Me.Label11.AutoSize = True
      Me.Label11.Location = New System.Drawing.Point(8, 45)
      Me.Label11.Name = "Label11"
      Me.Label11.Size = New System.Drawing.Size(53, 13)
      Me.Label11.TabIndex = 26
      Me.Label11.Text = "COM Port"
      '
      'ComboBox1
      '
      Me.ComboBox1.FormattingEnabled = True
      Me.ComboBox1.Location = New System.Drawing.Point(237, 42)
      Me.ComboBox1.Name = "ComboBox1"
      Me.ComboBox1.Size = New System.Drawing.Size(109, 21)
      Me.ComboBox1.TabIndex = 25
      '
      'ComboBox2
      '
      Me.ComboBox2.FormattingEnabled = True
      Me.ComboBox2.Location = New System.Drawing.Point(67, 42)
      Me.ComboBox2.Name = "ComboBox2"
      Me.ComboBox2.Size = New System.Drawing.Size(109, 21)
      Me.ComboBox2.TabIndex = 24
      '
      'Label14
      '
      Me.Label14.AutoSize = True
      Me.Label14.Location = New System.Drawing.Point(187, 45)
      Me.Label14.Name = "Label14"
      Me.Label14.Size = New System.Drawing.Size(38, 13)
      Me.Label14.TabIndex = 27
      Me.Label14.Text = "Speed"
      '
      'Label15
      '
      Me.Label15.AutoSize = True
      Me.Label15.Location = New System.Drawing.Point(8, 45)
      Me.Label15.Name = "Label15"
      Me.Label15.Size = New System.Drawing.Size(53, 13)
      Me.Label15.TabIndex = 26
      Me.Label15.Text = "COM Port"
      '
      'CreateFiles
      '
      Me.CreateFiles.Enabled = False
      Me.CreateFiles.Location = New System.Drawing.Point(358, 29)
      Me.CreateFiles.Name = "CreateFiles"
      Me.CreateFiles.Size = New System.Drawing.Size(75, 22)
      Me.CreateFiles.TabIndex = 53
      Me.CreateFiles.Text = "Save Config"
      Me.CreateFiles.UseVisualStyleBackColor = True
      '
      'Advanced
      '
      Me.Advanced.Location = New System.Drawing.Point(439, 29)
      Me.Advanced.Name = "Advanced"
      Me.Advanced.Size = New System.Drawing.Size(148, 22)
      Me.Advanced.TabIndex = 54
      Me.Advanced.Text = "Switch to Advanced Mode"
      Me.Advanced.UseVisualStyleBackColor = True
      '
      'AddPortButton
      '
      Me.AddPortButton.Enabled = False
      Me.AddPortButton.Location = New System.Drawing.Point(250, 29)
      Me.AddPortButton.Name = "AddPortButton"
      Me.AddPortButton.Size = New System.Drawing.Size(58, 23)
      Me.AddPortButton.TabIndex = 55
      Me.AddPortButton.Text = "Add Port"
      Me.AddPortButton.UseVisualStyleBackColor = True
      '
      'IDIntLabel
      '
      Me.IDIntLabel.AutoSize = True
      Me.IDIntLabel.Location = New System.Drawing.Point(488, 235)
      Me.IDIntLabel.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.IDIntLabel.Name = "IDIntLabel"
      Me.IDIntLabel.Size = New System.Drawing.Size(56, 13)
      Me.IDIntLabel.TabIndex = 59
      Me.IDIntLabel.Text = "ID Interval"
      '
      'IDMsgLabel
      '
      Me.IDMsgLabel.AutoSize = True
      Me.IDMsgLabel.Location = New System.Drawing.Point(8, 235)
      Me.IDMsgLabel.Margin = New System.Windows.Forms.Padding(2)
      Me.IDMsgLabel.Name = "IDMsgLabel"
      Me.IDMsgLabel.Size = New System.Drawing.Size(194, 13)
      Me.IDMsgLabel.TabIndex = 57
      Me.IDMsgLabel.Text = "ID Message (Sent in AX.25 ID Packets)"
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(13, 454)
      Me.Label4.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label4.Name = "Label4"
      Me.Label4.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label4.Size = New System.Drawing.Size(68, 13)
      Me.Label4.TabIndex = 60
      Me.Label4.Text = "Application 1"
      '
      'Appl1
      '
      Me.Appl1.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl1.Location = New System.Drawing.Point(91, 451)
      Me.Appl1.Name = "Appl1"
      Me.Appl1.Size = New System.Drawing.Size(63, 20)
      Me.Appl1.TabIndex = 62
      '
      'Label5
      '
      Me.Label5.AutoSize = True
      Me.Label5.Location = New System.Drawing.Point(163, 454)
      Me.Label5.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label5.Name = "Label5"
      Me.Label5.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label5.Size = New System.Drawing.Size(24, 13)
      Me.Label5.TabIndex = 63
      Me.Label5.Text = "Call"
      '
      'Label6
      '
      Me.Label6.AutoSize = True
      Me.Label6.Location = New System.Drawing.Point(456, 454)
      Me.Label6.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label6.Name = "Label6"
      Me.Label6.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label6.Size = New System.Drawing.Size(24, 13)
      Me.Label6.TabIndex = 67
      Me.Label6.Text = "Call"
      '
      'Appl2
      '
      Me.Appl2.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl2.Location = New System.Drawing.Point(384, 451)
      Me.Appl2.Name = "Appl2"
      Me.Appl2.Size = New System.Drawing.Size(63, 20)
      Me.Appl2.TabIndex = 66
      '
      'Label7
      '
      Me.Label7.AutoSize = True
      Me.Label7.Location = New System.Drawing.Point(306, 454)
      Me.Label7.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label7.Name = "Label7"
      Me.Label7.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label7.Size = New System.Drawing.Size(68, 13)
      Me.Label7.TabIndex = 64
      Me.Label7.Text = "Application 2"
      '
      'Label16
      '
      Me.Label16.AutoSize = True
      Me.Label16.Location = New System.Drawing.Point(163, 480)
      Me.Label16.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label16.Name = "Label16"
      Me.Label16.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label16.Size = New System.Drawing.Size(24, 13)
      Me.Label16.TabIndex = 71
      Me.Label16.Text = "Call"
      '
      'Label17
      '
      Me.Label17.AutoSize = True
      Me.Label17.Location = New System.Drawing.Point(13, 480)
      Me.Label17.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label17.Name = "Label17"
      Me.Label17.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label17.Size = New System.Drawing.Size(68, 13)
      Me.Label17.TabIndex = 68
      Me.Label17.Text = "Application 3"
      '
      'HelpButton1
      '
      Me.HelpButton1.Location = New System.Drawing.Point(13, 31)
      Me.HelpButton1.Name = "HelpButton1"
      Me.HelpButton1.Size = New System.Drawing.Size(37, 21)
      Me.HelpButton1.TabIndex = 76
      Me.HelpButton1.Text = "Help"
      Me.HelpButton1.UseVisualStyleBackColor = True
      '
      'ErrorProvider1
      '
      Me.ErrorProvider1.ContainerControl = Me
      '
      'PasswordBox
      '
      Me.PasswordBox.Location = New System.Drawing.Point(73, 88)
      Me.PasswordBox.Name = "PasswordBox"
      Me.PasswordBox.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.PasswordBox.Size = New System.Drawing.Size(446, 20)
      Me.PasswordBox.TabIndex = 80
      '
      'LocatorBox
      '
      Me.LocatorBox.Location = New System.Drawing.Point(197, 63)
      Me.LocatorBox.Name = "LocatorBox"
      Me.LocatorBox.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.LocatorBox.Size = New System.Drawing.Size(167, 20)
      Me.LocatorBox.TabIndex = 79
      '
      'Label18
      '
      Me.Label18.AutoSize = True
      Me.Label18.Location = New System.Drawing.Point(146, 66)
      Me.Label18.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label18.Name = "Label18"
      Me.Label18.Size = New System.Drawing.Size(43, 13)
      Me.Label18.TabIndex = 78
      Me.Label18.Text = "Locator"
      '
      'Label38
      '
      Me.Label38.AutoSize = True
      Me.Label38.Location = New System.Drawing.Point(8, 91)
      Me.Label38.Name = "Label38"
      Me.Label38.Size = New System.Drawing.Size(53, 13)
      Me.Label38.TabIndex = 77
      Me.Label38.Text = "Password"
      '
      'TabControl1
      '
      Me.TabControl1.Location = New System.Drawing.Point(11, 296)
      Me.TabControl1.Name = "TabControl1"
      Me.TabControl1.SelectedIndex = 0
      Me.TabControl1.Size = New System.Drawing.Size(580, 151)
      Me.TabControl1.TabIndex = 0
      Me.TabControl1.Visible = False
      '
      'Label12
      '
      Me.Label12.AutoSize = True
      Me.Label12.Location = New System.Drawing.Point(456, 480)
      Me.Label12.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label12.Name = "Label12"
      Me.Label12.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label12.Size = New System.Drawing.Size(24, 13)
      Me.Label12.TabIndex = 75
      Me.Label12.Text = "Call"
      '
      'Appl4
      '
      Me.Appl4.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl4.Location = New System.Drawing.Point(384, 477)
      Me.Appl4.Name = "Appl4"
      Me.Appl4.Size = New System.Drawing.Size(63, 20)
      Me.Appl4.TabIndex = 74
      '
      'Label13
      '
      Me.Label13.AutoSize = True
      Me.Label13.Location = New System.Drawing.Point(306, 480)
      Me.Label13.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
      Me.Label13.Name = "Label13"
      Me.Label13.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Label13.Size = New System.Drawing.Size(68, 13)
      Me.Label13.TabIndex = 72
      Me.Label13.Text = "Application 4"
      '
      'Appl3
      '
      Me.Appl3.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl3.Location = New System.Drawing.Point(91, 477)
      Me.Appl3.Name = "Appl3"
      Me.Appl3.Size = New System.Drawing.Size(63, 20)
      Me.Appl3.TabIndex = 70
      '
      'Appl4Call
      '
      Me.Appl4Call.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl4Call.Location = New System.Drawing.Point(485, 477)
      Me.Appl4Call.Margin = New System.Windows.Forms.Padding(0)
      Me.Appl4Call.Name = "Appl4Call"
      Me.Appl4Call.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Appl4Call.Size = New System.Drawing.Size(72, 20)
      Me.Appl4Call.TabIndex = 73
      '
      'Appl3Call
      '
      Me.Appl3Call.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl3Call.Location = New System.Drawing.Point(192, 477)
      Me.Appl3Call.Margin = New System.Windows.Forms.Padding(0)
      Me.Appl3Call.Name = "Appl3Call"
      Me.Appl3Call.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Appl3Call.Size = New System.Drawing.Size(72, 20)
      Me.Appl3Call.TabIndex = 69
      '
      'Appl2Call
      '
      Me.Appl2Call.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl2Call.Location = New System.Drawing.Point(485, 451)
      Me.Appl2Call.Margin = New System.Windows.Forms.Padding(0)
      Me.Appl2Call.Name = "Appl2Call"
      Me.Appl2Call.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Appl2Call.Size = New System.Drawing.Size(72, 20)
      Me.Appl2Call.TabIndex = 65
      '
      'Appl1Call
      '
      Me.Appl1Call.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.Appl1Call.Location = New System.Drawing.Point(192, 451)
      Me.Appl1Call.Margin = New System.Windows.Forms.Padding(0)
      Me.Appl1Call.Name = "Appl1Call"
      Me.Appl1Call.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.Appl1Call.Size = New System.Drawing.Size(72, 20)
      Me.Appl1Call.TabIndex = 61
      '
      'IDIntervalBox
      '
      Me.IDIntervalBox.Location = New System.Drawing.Point(562, 230)
      Me.IDIntervalBox.Margin = New System.Windows.Forms.Padding(2)
      Me.IDIntervalBox.Max = 60
      Me.IDIntervalBox.Name = "IDIntervalBox"
      Me.IDIntervalBox.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.IDIntervalBox.Size = New System.Drawing.Size(25, 20)
      Me.IDIntervalBox.TabIndex = 58
      Me.IDIntervalBox.Text = "0"
      '
      'IDMsgBox
      '
      Me.IDMsgBox.Location = New System.Drawing.Point(13, 257)
      Me.IDMsgBox.Margin = New System.Windows.Forms.Padding(2)
      Me.IDMsgBox.MaxLen = 512
      Me.IDMsgBox.Multiline = True
      Me.IDMsgBox.Name = "IDMsgBox"
      Me.IDMsgBox.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.IDMsgBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
      Me.IDMsgBox.Size = New System.Drawing.Size(574, 35)
      Me.IDMsgBox.TabIndex = 56
      '
      'CTEXTBox
      '
      Me.CTEXTBox.Location = New System.Drawing.Point(11, 194)
      Me.CTEXTBox.Margin = New System.Windows.Forms.Padding(2)
      Me.CTEXTBox.MaxLen = 512
      Me.CTEXTBox.Multiline = True
      Me.CTEXTBox.Name = "CTEXTBox"
      Me.CTEXTBox.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.CTEXTBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
      Me.CTEXTBox.Size = New System.Drawing.Size(576, 35)
      Me.CTEXTBox.TabIndex = 51
      '
      'InfoMsgBox
      '
      Me.InfoMsgBox.Location = New System.Drawing.Point(11, 133)
      Me.InfoMsgBox.Margin = New System.Windows.Forms.Padding(2)
      Me.InfoMsgBox.MaxLen = 512
      Me.InfoMsgBox.Multiline = True
      Me.InfoMsgBox.Name = "InfoMsgBox"
      Me.InfoMsgBox.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.InfoMsgBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
      Me.InfoMsgBox.Size = New System.Drawing.Size(576, 35)
      Me.InfoMsgBox.TabIndex = 49
      '
      'NodeCallBox
      '
      Me.NodeCallBox.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
      Me.NodeCallBox.Location = New System.Drawing.Point(73, 63)
      Me.NodeCallBox.Margin = New System.Windows.Forms.Padding(0)
      Me.NodeCallBox.Name = "NodeCallBox"
      Me.NodeCallBox.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.NodeCallBox.Size = New System.Drawing.Size(63, 20)
      Me.NodeCallBox.TabIndex = 48
      '
      'CallsignTextBox1
      '
      Me.CallsignTextBox1.Location = New System.Drawing.Point(253, 30)
      Me.CallsignTextBox1.Margin = New System.Windows.Forms.Padding(0)
      Me.CallsignTextBox1.Name = "CallsignTextBox1"
      Me.CallsignTextBox1.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.CallsignTextBox1.Size = New System.Drawing.Size(39, 20)
      Me.CallsignTextBox1.TabIndex = 53
      Me.CallsignTextBox1.Text = "100"
      '
      'CallsignTextBox2
      '
      Me.CallsignTextBox2.Location = New System.Drawing.Point(118, 30)
      Me.CallsignTextBox2.Margin = New System.Windows.Forms.Padding(0)
      Me.CallsignTextBox2.Name = "CallsignTextBox2"
      Me.CallsignTextBox2.RightToLeft = System.Windows.Forms.RightToLeft.No
      Me.CallsignTextBox2.Size = New System.Drawing.Size(49, 20)
      Me.CallsignTextBox2.TabIndex = 48
      Me.CallsignTextBox2.Text = "8500"
      '
      'SimpleForm
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.AutoScroll = True
      Me.ClientSize = New System.Drawing.Size(628, 545)
      Me.Controls.Add(Me.PasswordBox)
      Me.Controls.Add(Me.LocatorBox)
      Me.Controls.Add(Me.Label18)
      Me.Controls.Add(Me.Label38)
      Me.Controls.Add(Me.HelpButton1)
      Me.Controls.Add(Me.Label12)
      Me.Controls.Add(Me.Appl4)
      Me.Controls.Add(Me.Appl4Call)
      Me.Controls.Add(Me.Label13)
      Me.Controls.Add(Me.Label16)
      Me.Controls.Add(Me.Appl3)
      Me.Controls.Add(Me.Appl3Call)
      Me.Controls.Add(Me.Label17)
      Me.Controls.Add(Me.Label6)
      Me.Controls.Add(Me.Appl2)
      Me.Controls.Add(Me.Appl2Call)
      Me.Controls.Add(Me.Label7)
      Me.Controls.Add(Me.Label5)
      Me.Controls.Add(Me.Appl1)
      Me.Controls.Add(Me.Appl1Call)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.IDIntLabel)
      Me.Controls.Add(Me.IDIntervalBox)
      Me.Controls.Add(Me.IDMsgLabel)
      Me.Controls.Add(Me.IDMsgBox)
      Me.Controls.Add(Me.AddPortButton)
      Me.Controls.Add(Me.Advanced)
      Me.Controls.Add(Me.CreateFiles)
      Me.Controls.Add(Me.Label36)
      Me.Controls.Add(Me.CTEXTBox)
      Me.Controls.Add(Me.Label35)
      Me.Controls.Add(Me.InfoMsgBox)
      Me.Controls.Add(Me.NodeCallBox)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.LoadConfig)
      Me.Controls.Add(Me.Create)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.TabControl1)
      Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
      Me.Name = "SimpleForm"
      Me.Text = "WinBPQCfg Simple Configuration Mode"
      CType(Me.ErrorProvider1, System.ComponentModel.ISupportInitialize).EndInit()
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents Create As System.Windows.Forms.Button
    Friend WithEvents LoadConfig As System.Windows.Forms.Button
    Friend WithEvents Label36 As System.Windows.Forms.Label
    Friend WithEvents CTEXTBox As BPQCFG.MultiLineTextBox
    Friend WithEvents Label35 As System.Windows.Forms.Label
    Friend WithEvents InfoMsgBox As BPQCFG.MultiLineTextBox
    Friend WithEvents NodeCallBox As BPQCFG.CallsignTextBox
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents CallsignTextBox1 As BPQCFG.CallsignTextBox
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents CheckBox1 As System.Windows.Forms.CheckBox
    Friend WithEvents CheckBox2 As System.Windows.Forms.CheckBox
    Friend WithEvents CheckBox3 As System.Windows.Forms.CheckBox
    Friend WithEvents CallsignTextBox2 As BPQCFG.CallsignTextBox
    Friend WithEvents Label9 As System.Windows.Forms.Label
    Friend WithEvents Label10 As System.Windows.Forms.Label
    Friend WithEvents Label11 As System.Windows.Forms.Label
    Friend WithEvents ComboBox1 As System.Windows.Forms.ComboBox
    Friend WithEvents ComboBox2 As System.Windows.Forms.ComboBox
   Friend WithEvents Label14 As System.Windows.Forms.Label
    Friend WithEvents Label15 As System.Windows.Forms.Label

   Friend WithEvents CreateFiles As System.Windows.Forms.Button
    Friend WithEvents Advanced As System.Windows.Forms.Button
    Friend WithEvents AddPortButton As System.Windows.Forms.Button
    Friend WithEvents IDIntLabel As System.Windows.Forms.Label
    Friend WithEvents IDIntervalBox As BPQCFG.DTNumTextBox
    Friend WithEvents IDMsgBox As BPQCFG.MultiLineTextBox
    Private WithEvents IDMsgLabel As System.Windows.Forms.Label
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents Appl1Call As BPQCFG.CallsignTextBox
    Friend WithEvents Appl1 As System.Windows.Forms.TextBox
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents Appl2 As System.Windows.Forms.TextBox
    Friend WithEvents Appl2Call As BPQCFG.CallsignTextBox
    Friend WithEvents Label7 As System.Windows.Forms.Label
   Friend WithEvents Label16 As System.Windows.Forms.Label
   Friend WithEvents Label17 As System.Windows.Forms.Label
   Friend WithEvents HelpButton1 As System.Windows.Forms.Button
   Friend WithEvents ErrorProvider1 As System.Windows.Forms.ErrorProvider
   Friend WithEvents PasswordBox As System.Windows.Forms.TextBox
    Friend WithEvents LocatorBox As System.Windows.Forms.TextBox
    Friend WithEvents Label18 As System.Windows.Forms.Label
   Friend WithEvents Label38 As System.Windows.Forms.Label
   Friend WithEvents TabControl1 As System.Windows.Forms.TabControl
   Friend WithEvents Label12 As System.Windows.Forms.Label
   Friend WithEvents Appl4 As System.Windows.Forms.TextBox
   Friend WithEvents Appl4Call As BPQCFG.CallsignTextBox
   Friend WithEvents Label13 As System.Windows.Forms.Label
   Friend WithEvents Appl3 As System.Windows.Forms.TextBox
   Friend WithEvents Appl3Call As BPQCFG.CallsignTextBox

End Class
