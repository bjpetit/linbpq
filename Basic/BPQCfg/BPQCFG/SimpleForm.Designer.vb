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
        Me.PACTOR = New System.Windows.Forms.TabPage
        Me.Label14 = New System.Windows.Forms.Label
        Me.Label15 = New System.Windows.Forms.Label
        Me.AX25KISS = New System.Windows.Forms.TabPage
        Me.WINMOR = New System.Windows.Forms.TabPage
        Me.TabControl1 = New System.Windows.Forms.TabControl
        Me.AX25AGW = New System.Windows.Forms.TabPage
        Me.CreateFiles = New System.Windows.Forms.Button
        Me.Button2 = New System.Windows.Forms.Button
        Me.AddPortButton = New System.Windows.Forms.Button
        Me.IDIntLabel = New System.Windows.Forms.Label
        Me.IDMsgLabel = New System.Windows.Forms.Label
        Me.IDIntervalBox = New BPQCFG.DTNumTextBox
        Me.IDMsgBox = New BPQCFG.MultiLineTextBox
        Me.CTEXTBox = New BPQCFG.MultiLineTextBox
        Me.InfoMsgBox = New BPQCFG.MultiLineTextBox
        Me.NodeCallBox = New BPQCFG.CallsignTextBox
        Me.CallsignTextBox1 = New BPQCFG.CallsignTextBox
        Me.CallsignTextBox2 = New BPQCFG.CallsignTextBox
        Me.TabControl1.SuspendLayout()
        Me.SuspendLayout()
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.Location = New System.Drawing.Point(13, 10)
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
        Me.Create.Location = New System.Drawing.Point(93, 44)
        Me.Create.Name = "Create"
        Me.Create.Size = New System.Drawing.Size(115, 23)
        Me.Create.TabIndex = 3
        Me.Create.Text = "Create New Config"
        Me.Create.UseVisualStyleBackColor = True
        '
        'LoadConfig
        '
        Me.LoadConfig.Location = New System.Drawing.Point(11, 45)
        Me.LoadConfig.Name = "LoadConfig"
        Me.LoadConfig.Size = New System.Drawing.Size(76, 23)
        Me.LoadConfig.TabIndex = 4
        Me.LoadConfig.Text = "Load Config"
        Me.LoadConfig.UseVisualStyleBackColor = True
        '
        'Label36
        '
        Me.Label36.AutoSize = True
        Me.Label36.Location = New System.Drawing.Point(8, 200)
        Me.Label36.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
        Me.Label36.Name = "Label36"
        Me.Label36.Size = New System.Drawing.Size(218, 13)
        Me.Label36.TabIndex = 52
        Me.Label36.Text = "CTEXT Message (Sent when user connects)"
        '
        'Label35
        '
        Me.Label35.AutoSize = True
        Me.Label35.Location = New System.Drawing.Point(8, 117)
        Me.Label35.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
        Me.Label35.Name = "Label35"
        Me.Label35.Size = New System.Drawing.Size(259, 13)
        Me.Label35.TabIndex = 50
        Me.Label35.Text = "INFO Message (Sent in response to INFO Command )"
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(8, 84)
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
        'PACTOR
        '
        Me.PACTOR.Location = New System.Drawing.Point(4, 22)
        Me.PACTOR.Name = "PACTOR"
        Me.PACTOR.Size = New System.Drawing.Size(572, 125)
        Me.PACTOR.TabIndex = 4
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
        'AX25KISS
        '
        Me.AX25KISS.Location = New System.Drawing.Point(4, 22)
        Me.AX25KISS.Name = "AX25KISS"
        Me.AX25KISS.Size = New System.Drawing.Size(572, 125)
        Me.AX25KISS.TabIndex = 2
        Me.AX25KISS.Text = "AX.25 KISS"
        Me.AX25KISS.UseVisualStyleBackColor = True
        '
        'WINMOR
        '
        Me.WINMOR.Location = New System.Drawing.Point(4, 22)
        Me.WINMOR.Name = "WINMOR"
        Me.WINMOR.Padding = New System.Windows.Forms.Padding(3)
        Me.WINMOR.Size = New System.Drawing.Size(572, 125)
        Me.WINMOR.TabIndex = 0
        Me.WINMOR.Text = "WINMOR"
        Me.WINMOR.UseVisualStyleBackColor = True
        '
        'TabControl1
        '
        Me.TabControl1.Controls.Add(Me.WINMOR)
        Me.TabControl1.Controls.Add(Me.AX25AGW)
        Me.TabControl1.Controls.Add(Me.AX25KISS)
        Me.TabControl1.Controls.Add(Me.PACTOR)
        Me.TabControl1.Location = New System.Drawing.Point(11, 370)
        Me.TabControl1.Name = "TabControl1"
        Me.TabControl1.SelectedIndex = 0
        Me.TabControl1.Size = New System.Drawing.Size(580, 151)
        Me.TabControl1.TabIndex = 0
        Me.TabControl1.Visible = False
        '
        'AX25AGW
        '
        Me.AX25AGW.Location = New System.Drawing.Point(4, 22)
        Me.AX25AGW.Name = "AX25AGW"
        Me.AX25AGW.Size = New System.Drawing.Size(572, 125)
        Me.AX25AGW.TabIndex = 3
        Me.AX25AGW.Text = "AX.25 via AGWPE"
        Me.AX25AGW.UseVisualStyleBackColor = True
        '
        'CreateFiles
        '
        Me.CreateFiles.Location = New System.Drawing.Point(352, 44)
        Me.CreateFiles.Name = "CreateFiles"
        Me.CreateFiles.Size = New System.Drawing.Size(81, 22)
        Me.CreateFiles.TabIndex = 53
        Me.CreateFiles.Text = "Create Files"
        Me.CreateFiles.UseVisualStyleBackColor = True
        '
        'Button2
        '
        Me.Button2.Location = New System.Drawing.Point(439, 44)
        Me.Button2.Name = "Button2"
        Me.Button2.Size = New System.Drawing.Size(148, 22)
        Me.Button2.TabIndex = 54
        Me.Button2.Text = "Switch to Advanced Mode"
        Me.Button2.UseVisualStyleBackColor = True
        '
        'AddPortButton
        '
        Me.AddPortButton.Enabled = False
        Me.AddPortButton.Location = New System.Drawing.Point(214, 44)
        Me.AddPortButton.Name = "AddPortButton"
        Me.AddPortButton.Size = New System.Drawing.Size(73, 23)
        Me.AddPortButton.TabIndex = 55
        Me.AddPortButton.Text = "Add Port"
        Me.AddPortButton.UseVisualStyleBackColor = True
        '
        'IDIntLabel
        '
        Me.IDIntLabel.AutoSize = True
        Me.IDIntLabel.Location = New System.Drawing.Point(488, 284)
        Me.IDIntLabel.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
        Me.IDIntLabel.Name = "IDIntLabel"
        Me.IDIntLabel.Size = New System.Drawing.Size(56, 13)
        Me.IDIntLabel.TabIndex = 59
        Me.IDIntLabel.Text = "ID Interval"
        '
        'IDMsgLabel
        '
        Me.IDMsgLabel.AutoSize = True
        Me.IDMsgLabel.Location = New System.Drawing.Point(8, 284)
        Me.IDMsgLabel.Margin = New System.Windows.Forms.Padding(2)
        Me.IDMsgLabel.Name = "IDMsgLabel"
        Me.IDMsgLabel.Size = New System.Drawing.Size(194, 13)
        Me.IDMsgLabel.TabIndex = 57
        Me.IDMsgLabel.Text = "ID Message (Sent in AX.25 ID Packets)"
        '
        'IDIntervalBox
        '
        Me.IDIntervalBox.Location = New System.Drawing.Point(562, 281)
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
        Me.IDMsgBox.Location = New System.Drawing.Point(13, 303)
        Me.IDMsgBox.Margin = New System.Windows.Forms.Padding(2)
        Me.IDMsgBox.MaxLen = 512
        Me.IDMsgBox.Multiline = True
        Me.IDMsgBox.Name = "IDMsgBox"
        Me.IDMsgBox.RightToLeft = System.Windows.Forms.RightToLeft.No
        Me.IDMsgBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
        Me.IDMsgBox.Size = New System.Drawing.Size(574, 50)
        Me.IDMsgBox.TabIndex = 56
        '
        'CTEXTBox
        '
        Me.CTEXTBox.Location = New System.Drawing.Point(11, 221)
        Me.CTEXTBox.Margin = New System.Windows.Forms.Padding(2)
        Me.CTEXTBox.MaxLen = 512
        Me.CTEXTBox.Multiline = True
        Me.CTEXTBox.Name = "CTEXTBox"
        Me.CTEXTBox.RightToLeft = System.Windows.Forms.RightToLeft.No
        Me.CTEXTBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
        Me.CTEXTBox.Size = New System.Drawing.Size(576, 50)
        Me.CTEXTBox.TabIndex = 51
        '
        'InfoMsgBox
        '
        Me.InfoMsgBox.Location = New System.Drawing.Point(11, 139)
        Me.InfoMsgBox.Margin = New System.Windows.Forms.Padding(2)
        Me.InfoMsgBox.MaxLen = 512
        Me.InfoMsgBox.Multiline = True
        Me.InfoMsgBox.Name = "InfoMsgBox"
        Me.InfoMsgBox.RightToLeft = System.Windows.Forms.RightToLeft.No
        Me.InfoMsgBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
        Me.InfoMsgBox.Size = New System.Drawing.Size(576, 47)
        Me.InfoMsgBox.TabIndex = 49
        '
        'NodeCallBox
        '
        Me.NodeCallBox.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
        Me.NodeCallBox.Location = New System.Drawing.Point(78, 81)
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
        Me.ClientSize = New System.Drawing.Size(603, 533)
        Me.Controls.Add(Me.IDIntLabel)
        Me.Controls.Add(Me.IDIntervalBox)
        Me.Controls.Add(Me.IDMsgLabel)
        Me.Controls.Add(Me.IDMsgBox)
        Me.Controls.Add(Me.AddPortButton)
        Me.Controls.Add(Me.Button2)
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
        Me.TabControl1.ResumeLayout(False)
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
    Friend WithEvents PACTOR As System.Windows.Forms.TabPage
    Friend WithEvents Label14 As System.Windows.Forms.Label
    Friend WithEvents Label15 As System.Windows.Forms.Label
    Friend WithEvents AX25KISS As System.Windows.Forms.TabPage
    Friend WithEvents WINMOR As System.Windows.Forms.TabPage

    Friend WithEvents TabControl1 As System.Windows.Forms.TabControl
    Friend WithEvents AX25AGW As System.Windows.Forms.TabPage
    Friend WithEvents CreateFiles As System.Windows.Forms.Button
    Friend WithEvents Button2 As System.Windows.Forms.Button
    Friend WithEvents AddPortButton As System.Windows.Forms.Button
    Friend WithEvents IDIntLabel As System.Windows.Forms.Label
    Friend WithEvents IDIntervalBox As BPQCFG.DTNumTextBox
    Friend WithEvents IDMsgBox As BPQCFG.MultiLineTextBox
    Private WithEvents IDMsgLabel As System.Windows.Forms.Label

End Class
