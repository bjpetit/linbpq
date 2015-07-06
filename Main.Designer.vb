<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Main
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
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(Main))
        Me.MenuStrip1 = New System.Windows.Forms.MenuStrip()
        Me.FileToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.SetupMenu = New System.Windows.Forms.ToolStripMenuItem()
        Me.RadioMenu = New System.Windows.Forms.ToolStripMenuItem()
        Me.AboutMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.CloseMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.ShowTestFormMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.DisplayToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.WaterfallToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.SpectrumToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.DisableToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.SendIDToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.IDFrameToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.TwoToneTestToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.CWIDToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.AbortToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.HelpToolStripMenuItem1 = New System.Windows.Forms.ToolStripMenuItem()
        Me.LogsToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.HelpIndexToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.HelpContentsToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.ChannelBusyToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.pnlConstellation = New System.Windows.Forms.Panel()
        Me.pnlWaterfall = New System.Windows.Forms.Panel()
        Me.Panel2 = New System.Windows.Forms.Panel()
        Me.lblOffset = New System.Windows.Forms.Label()
        Me.Panel3 = New System.Windows.Forms.Panel()
        Me.Panel1 = New System.Windows.Forms.Panel()
        Me.prgReceiveLevel = New System.Windows.Forms.ProgressBar()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.lblXmtFrame = New System.Windows.Forms.Label()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.lblRcvFrame = New System.Windows.Forms.Label()
        Me.lblState = New System.Windows.Forms.Label()
        Me.lblQuality = New System.Windows.Forms.Label()
        Me.tmrPoll = New System.Windows.Forms.Timer(Me.components)
        Me.tmrStartCODEC = New System.Windows.Forms.Timer(Me.components)
        Me.ToolTip1 = New System.Windows.Forms.ToolTip(Me.components)
        Me.lblCF = New System.Windows.Forms.Label()
        Me.lblHost = New System.Windows.Forms.Label()
        Me.tmrLogStats = New System.Windows.Forms.Timer(Me.components)
        Me.Label8 = New System.Windows.Forms.Label()
        Me.Label9 = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.Ipdaemon1 = New nsoftware.IPWorks.Ipdaemon(Me.components)
        Me.ACKToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.MenuStrip1.SuspendLayout()
        Me.SuspendLayout()
        '
        'MenuStrip1
        '
        Me.MenuStrip1.Font = New System.Drawing.Font("Segoe UI", 9.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.MenuStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.FileToolStripMenuItem, Me.DisplayToolStripMenuItem, Me.SendIDToolStripMenuItem, Me.AbortToolStripMenuItem, Me.HelpToolStripMenuItem1, Me.LogsToolStripMenuItem, Me.ChannelBusyToolStripMenuItem})
        Me.MenuStrip1.Location = New System.Drawing.Point(0, 0)
        Me.MenuStrip1.Name = "MenuStrip1"
        Me.MenuStrip1.Size = New System.Drawing.Size(535, 24)
        Me.MenuStrip1.TabIndex = 17
        Me.MenuStrip1.Text = "MenuStrip1"
        '
        'FileToolStripMenuItem
        '
        Me.FileToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.SetupMenu, Me.RadioMenu, Me.AboutMenuItem, Me.CloseMenuItem, Me.ShowTestFormMenuItem})
        Me.FileToolStripMenuItem.Name = "FileToolStripMenuItem"
        Me.FileToolStripMenuItem.Size = New System.Drawing.Size(37, 20)
        Me.FileToolStripMenuItem.Text = "File"
        '
        'SetupMenu
        '
        Me.SetupMenu.Name = "SetupMenu"
        Me.SetupMenu.Size = New System.Drawing.Size(186, 22)
        Me.SetupMenu.Text = "Virtual TNC Setup"
        '
        'RadioMenu
        '
        Me.RadioMenu.Name = "RadioMenu"
        Me.RadioMenu.Size = New System.Drawing.Size(186, 22)
        Me.RadioMenu.Text = "Optional Radio Setup"
        '
        'AboutMenuItem
        '
        Me.AboutMenuItem.Name = "AboutMenuItem"
        Me.AboutMenuItem.Size = New System.Drawing.Size(186, 22)
        Me.AboutMenuItem.Text = "About "
        '
        'CloseMenuItem
        '
        Me.CloseMenuItem.Name = "CloseMenuItem"
        Me.CloseMenuItem.Size = New System.Drawing.Size(186, 22)
        Me.CloseMenuItem.Text = "Close"
        '
        'ShowTestFormMenuItem
        '
        Me.ShowTestFormMenuItem.Enabled = False
        Me.ShowTestFormMenuItem.Name = "ShowTestFormMenuItem"
        Me.ShowTestFormMenuItem.Size = New System.Drawing.Size(186, 22)
        Me.ShowTestFormMenuItem.Text = "Show Test Form"
        '
        'DisplayToolStripMenuItem
        '
        Me.DisplayToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.WaterfallToolStripMenuItem, Me.SpectrumToolStripMenuItem, Me.DisableToolStripMenuItem})
        Me.DisplayToolStripMenuItem.Name = "DisplayToolStripMenuItem"
        Me.DisplayToolStripMenuItem.Size = New System.Drawing.Size(65, 20)
        Me.DisplayToolStripMenuItem.Text = "Graphics"
        '
        'WaterfallToolStripMenuItem
        '
        Me.WaterfallToolStripMenuItem.Name = "WaterfallToolStripMenuItem"
        Me.WaterfallToolStripMenuItem.Size = New System.Drawing.Size(125, 22)
        Me.WaterfallToolStripMenuItem.Text = "Waterfall"
        '
        'SpectrumToolStripMenuItem
        '
        Me.SpectrumToolStripMenuItem.Name = "SpectrumToolStripMenuItem"
        Me.SpectrumToolStripMenuItem.Size = New System.Drawing.Size(125, 22)
        Me.SpectrumToolStripMenuItem.Text = "Spectrum"
        '
        'DisableToolStripMenuItem
        '
        Me.DisableToolStripMenuItem.Name = "DisableToolStripMenuItem"
        Me.DisableToolStripMenuItem.Size = New System.Drawing.Size(125, 22)
        Me.DisableToolStripMenuItem.Text = "Disable"
        '
        'SendIDToolStripMenuItem
        '
        Me.SendIDToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.IDFrameToolStripMenuItem, Me.TwoToneTestToolStripMenuItem, Me.CWIDToolStripMenuItem, Me.ACKToolStripMenuItem})
        Me.SendIDToolStripMenuItem.Name = "SendIDToolStripMenuItem"
        Me.SendIDToolStripMenuItem.Size = New System.Drawing.Size(45, 20)
        Me.SendIDToolStripMenuItem.Text = "Send"
        '
        'IDFrameToolStripMenuItem
        '
        Me.IDFrameToolStripMenuItem.Name = "IDFrameToolStripMenuItem"
        Me.IDFrameToolStripMenuItem.Size = New System.Drawing.Size(152, 22)
        Me.IDFrameToolStripMenuItem.Text = "ID Frame"
        '
        'TwoToneTestToolStripMenuItem
        '
        Me.TwoToneTestToolStripMenuItem.Name = "TwoToneTestToolStripMenuItem"
        Me.TwoToneTestToolStripMenuItem.Size = New System.Drawing.Size(152, 22)
        Me.TwoToneTestToolStripMenuItem.Text = "Two tone Test"
        '
        'CWIDToolStripMenuItem
        '
        Me.CWIDToolStripMenuItem.Name = "CWIDToolStripMenuItem"
        Me.CWIDToolStripMenuItem.Size = New System.Drawing.Size(152, 22)
        Me.CWIDToolStripMenuItem.Text = "CWID (FSK)"
        '
        'AbortToolStripMenuItem
        '
        Me.AbortToolStripMenuItem.Name = "AbortToolStripMenuItem"
        Me.AbortToolStripMenuItem.Size = New System.Drawing.Size(49, 20)
        Me.AbortToolStripMenuItem.Text = "Abort"
        '
        'HelpToolStripMenuItem1
        '
        Me.HelpToolStripMenuItem1.Name = "HelpToolStripMenuItem1"
        Me.HelpToolStripMenuItem1.Size = New System.Drawing.Size(44, 20)
        Me.HelpToolStripMenuItem1.Text = "Logs"
        '
        'LogsToolStripMenuItem
        '
        Me.LogsToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.HelpIndexToolStripMenuItem, Me.HelpContentsToolStripMenuItem})
        Me.LogsToolStripMenuItem.Name = "LogsToolStripMenuItem"
        Me.LogsToolStripMenuItem.Size = New System.Drawing.Size(44, 20)
        Me.LogsToolStripMenuItem.Text = "Help"
        '
        'HelpIndexToolStripMenuItem
        '
        Me.HelpIndexToolStripMenuItem.Name = "HelpIndexToolStripMenuItem"
        Me.HelpIndexToolStripMenuItem.Size = New System.Drawing.Size(150, 22)
        Me.HelpIndexToolStripMenuItem.Text = "Help Index"
        '
        'HelpContentsToolStripMenuItem
        '
        Me.HelpContentsToolStripMenuItem.Name = "HelpContentsToolStripMenuItem"
        Me.HelpContentsToolStripMenuItem.Size = New System.Drawing.Size(150, 22)
        Me.HelpContentsToolStripMenuItem.Text = "Help Contents"
        '
        'ChannelBusyToolStripMenuItem
        '
        Me.ChannelBusyToolStripMenuItem.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right
        Me.ChannelBusyToolStripMenuItem.BackColor = System.Drawing.Color.Gold
        Me.ChannelBusyToolStripMenuItem.Name = "ChannelBusyToolStripMenuItem"
        Me.ChannelBusyToolStripMenuItem.Size = New System.Drawing.Size(91, 20)
        Me.ChannelBusyToolStripMenuItem.Text = "Channel Busy"
        Me.ChannelBusyToolStripMenuItem.Visible = False
        '
        'pnlConstellation
        '
        Me.pnlConstellation.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.pnlConstellation.Location = New System.Drawing.Point(6, 27)
        Me.pnlConstellation.MaximumSize = New System.Drawing.Size(91, 91)
        Me.pnlConstellation.MinimumSize = New System.Drawing.Size(69, 75)
        Me.pnlConstellation.Name = "pnlConstellation"
        Me.pnlConstellation.Size = New System.Drawing.Size(91, 91)
        Me.pnlConstellation.TabIndex = 58
        Me.ToolTip1.SetToolTip(Me.pnlConstellation, "PSK or 4FSK Constellation")
        '
        'pnlWaterfall
        '
        Me.pnlWaterfall.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.pnlWaterfall.BackColor = System.Drawing.Color.Black
        Me.pnlWaterfall.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.pnlWaterfall.Location = New System.Drawing.Point(111, 51)
        Me.pnlWaterfall.Name = "pnlWaterfall"
        Me.pnlWaterfall.Size = New System.Drawing.Size(211, 53)
        Me.pnlWaterfall.TabIndex = 59
        Me.ToolTip1.SetToolTip(Me.pnlWaterfall, "Graphics display (waterfall or spectrum) Audio freq range : 300-2700Hz.  For prop" & _
        "er detection and tuning signal must fit within green lines. ")
        '
        'Panel2
        '
        Me.Panel2.BackColor = System.Drawing.Color.LightGreen
        Me.Panel2.Location = New System.Drawing.Point(203, 37)
        Me.Panel2.Name = "Panel2"
        Me.Panel2.Size = New System.Drawing.Size(57, 7)
        Me.Panel2.TabIndex = 63
        Me.ToolTip1.SetToolTip(Me.Panel2, "Receive Level: keep ""in the green""")
        '
        'lblOffset
        '
        Me.lblOffset.AutoSize = True
        Me.lblOffset.Location = New System.Drawing.Point(273, 31)
        Me.lblOffset.Name = "lblOffset"
        Me.lblOffset.Size = New System.Drawing.Size(66, 13)
        Me.lblOffset.TabIndex = 66
        Me.lblOffset.Text = "Offset:     Hz"
        Me.ToolTip1.SetToolTip(Me.lblOffset, "Carrier offset of remote station (Hz) ")
        '
        'Panel3
        '
        Me.Panel3.BackColor = System.Drawing.Color.LightSalmon
        Me.Panel3.Location = New System.Drawing.Point(260, 37)
        Me.Panel3.Name = "Panel3"
        Me.Panel3.Size = New System.Drawing.Size(10, 7)
        Me.Panel3.TabIndex = 64
        '
        'Panel1
        '
        Me.Panel1.BackColor = System.Drawing.Color.SkyBlue
        Me.Panel1.Location = New System.Drawing.Point(170, 37)
        Me.Panel1.Name = "Panel1"
        Me.Panel1.Size = New System.Drawing.Size(33, 7)
        Me.Panel1.TabIndex = 62
        '
        'prgReceiveLevel
        '
        Me.prgReceiveLevel.Location = New System.Drawing.Point(170, 27)
        Me.prgReceiveLevel.Maximum = 181
        Me.prgReceiveLevel.Name = "prgReceiveLevel"
        Me.prgReceiveLevel.Size = New System.Drawing.Size(100, 10)
        Me.prgReceiveLevel.Step = 1
        Me.prgReceiveLevel.Style = System.Windows.Forms.ProgressBarStyle.Continuous
        Me.prgReceiveLevel.TabIndex = 60
        Me.ToolTip1.SetToolTip(Me.prgReceiveLevel, "Receive Level: keep ""in the green""")
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(110, 31)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(59, 13)
        Me.Label3.TabIndex = 61
        Me.Label3.Text = "Rcv Level:"
        '
        'lblXmtFrame
        '
        Me.lblXmtFrame.BackColor = System.Drawing.SystemColors.Control
        Me.lblXmtFrame.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.lblXmtFrame.Location = New System.Drawing.Point(333, 96)
        Me.lblXmtFrame.Name = "lblXmtFrame"
        Me.lblXmtFrame.Size = New System.Drawing.Size(200, 17)
        Me.lblXmtFrame.TabIndex = 67
        Me.lblXmtFrame.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        Me.ToolTip1.SetToolTip(Me.lblXmtFrame, "Frame being transmitted")
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Location = New System.Drawing.Point(330, 82)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(60, 13)
        Me.Label5.TabIndex = 69
        Me.Label5.Text = "Xmt Frame:"
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(328, 48)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(62, 13)
        Me.Label1.TabIndex = 70
        Me.Label1.Text = "Rcv Frame:"
        '
        'lblRcvFrame
        '
        Me.lblRcvFrame.BackColor = System.Drawing.SystemColors.Control
        Me.lblRcvFrame.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.lblRcvFrame.Location = New System.Drawing.Point(331, 61)
        Me.lblRcvFrame.Name = "lblRcvFrame"
        Me.lblRcvFrame.Size = New System.Drawing.Size(202, 17)
        Me.lblRcvFrame.TabIndex = 72
        Me.lblRcvFrame.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        Me.ToolTip1.SetToolTip(Me.lblRcvFrame, "Last received frame: Yellow=Detected & Capturing, Green=Decoded CRC OK, Red=Decod" & _
        "ed CRC Fail")
        '
        'lblState
        '
        Me.lblState.BackColor = System.Drawing.Color.Silver
        Me.lblState.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.lblState.Font = New System.Drawing.Font("Microsoft Sans Serif", 11.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lblState.Location = New System.Drawing.Point(401, 26)
        Me.lblState.Name = "lblState"
        Me.lblState.Size = New System.Drawing.Size(132, 20)
        Me.lblState.TabIndex = 73
        Me.lblState.Text = "OFFLINE"
        Me.lblState.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        Me.ToolTip1.SetToolTip(Me.lblState, "Protocol State")
        '
        'lblQuality
        '
        Me.lblQuality.AutoSize = True
        Me.lblQuality.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lblQuality.ForeColor = System.Drawing.SystemColors.ControlText
        Me.lblQuality.Location = New System.Drawing.Point(2, 123)
        Me.lblQuality.Name = "lblQuality"
        Me.lblQuality.Size = New System.Drawing.Size(42, 13)
        Me.lblQuality.TabIndex = 75
        Me.lblQuality.Text = "Quality:"
        Me.ToolTip1.SetToolTip(Me.lblQuality, "Frame Quality ""score"" (0-100)  Quality > 65 generally required for good decodes. " & _
        "")
        '
        'tmrPoll
        '
        Me.tmrPoll.Interval = 42
        '
        'tmrStartCODEC
        '
        Me.tmrStartCODEC.Interval = 1000
        '
        'lblCF
        '
        Me.lblCF.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.lblCF.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lblCF.Location = New System.Drawing.Point(143, 116)
        Me.lblCF.Name = "lblCF"
        Me.lblCF.Size = New System.Drawing.Size(148, 20)
        Me.lblCF.TabIndex = 78
        Me.lblCF.Text = "CF: 1.5 KHz"
        Me.lblCF.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        Me.ToolTip1.SetToolTip(Me.lblCF, "Center frequency of Waterfall or Spectrum ")
        '
        'lblHost
        '
        Me.lblHost.BackColor = System.Drawing.Color.DarkGray
        Me.lblHost.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.lblHost.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lblHost.Location = New System.Drawing.Point(403, 120)
        Me.lblHost.Name = "lblHost"
        Me.lblHost.Size = New System.Drawing.Size(130, 19)
        Me.lblHost.TabIndex = 79
        Me.lblHost.Text = "TCPIP"
        Me.lblHost.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        Me.ToolTip1.SetToolTip(Me.lblHost, "Host Interface Status: Yellow: Enabled, waiting for host, Green OK/Connected;  Re" & _
        "d Failed/Disconnected")
        '
        'tmrLogStats
        '
        Me.tmrLogStats.Interval = 1000
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Location = New System.Drawing.Point(94, 117)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(37, 13)
        Me.Label8.TabIndex = 76
        Me.Label8.Text = "-1200 "
        '
        'Label9
        '
        Me.Label9.AutoSize = True
        Me.Label9.Location = New System.Drawing.Point(296, 116)
        Me.Label9.Name = "Label9"
        Me.Label9.Size = New System.Drawing.Size(37, 13)
        Me.Label9.TabIndex = 77
        Me.Label9.Text = "+1200"
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(358, 123)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(32, 13)
        Me.Label2.TabIndex = 80
        Me.Label2.Text = "Host:"
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(366, 31)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(35, 13)
        Me.Label6.TabIndex = 81
        Me.Label6.Text = "State:"
        '
        'Ipdaemon1
        '
        Me.Ipdaemon1.About = "IP*Works! V9 [Build 5050]"
        '
        'ACKToolStripMenuItem
        '
        Me.ACKToolStripMenuItem.Name = "ACKToolStripMenuItem"
        Me.ACKToolStripMenuItem.Size = New System.Drawing.Size(152, 22)
        Me.ACKToolStripMenuItem.Text = "ACK"
        '
        'Main
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.AutoSize = True
        Me.ClientSize = New System.Drawing.Size(535, 148)
        Me.ControlBox = False
        Me.Controls.Add(Me.Label6)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.lblHost)
        Me.Controls.Add(Me.lblCF)
        Me.Controls.Add(Me.Label9)
        Me.Controls.Add(Me.Label8)
        Me.Controls.Add(Me.lblQuality)
        Me.Controls.Add(Me.lblState)
        Me.Controls.Add(Me.lblRcvFrame)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.Label5)
        Me.Controls.Add(Me.lblXmtFrame)
        Me.Controls.Add(Me.Panel2)
        Me.Controls.Add(Me.lblOffset)
        Me.Controls.Add(Me.Panel3)
        Me.Controls.Add(Me.Panel1)
        Me.Controls.Add(Me.prgReceiveLevel)
        Me.Controls.Add(Me.Label3)
        Me.Controls.Add(Me.pnlWaterfall)
        Me.Controls.Add(Me.pnlConstellation)
        Me.Controls.Add(Me.MenuStrip1)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.MainMenuStrip = Me.MenuStrip1
        Me.Name = "Main"
        Me.Text = "ARDOP Win Virtual TNC"
        Me.MenuStrip1.ResumeLayout(False)
        Me.MenuStrip1.PerformLayout()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents MenuStrip1 As System.Windows.Forms.MenuStrip
    Friend WithEvents FileToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents SetupMenu As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents AboutMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents DisplayToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents WaterfallToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents SpectrumToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents DisableToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents pnlConstellation As System.Windows.Forms.Panel
    Friend WithEvents pnlWaterfall As System.Windows.Forms.Panel
    Friend WithEvents Panel2 As System.Windows.Forms.Panel
    Friend WithEvents lblOffset As System.Windows.Forms.Label
    Friend WithEvents Panel3 As System.Windows.Forms.Panel
    Friend WithEvents Panel1 As System.Windows.Forms.Panel
    Friend WithEvents prgReceiveLevel As System.Windows.Forms.ProgressBar
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents lblXmtFrame As System.Windows.Forms.Label
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents lblRcvFrame As System.Windows.Forms.Label
    Friend WithEvents lblState As System.Windows.Forms.Label
    Friend WithEvents CloseMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents SendIDToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents AbortToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents lblQuality As System.Windows.Forms.Label
    Friend WithEvents tmrPoll As System.Windows.Forms.Timer
    Friend WithEvents tmrStartCODEC As System.Windows.Forms.Timer
    Friend WithEvents HelpToolStripMenuItem1 As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents LogsToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents HelpIndexToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents HelpContentsToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ToolTip1 As System.Windows.Forms.ToolTip
    Friend WithEvents tmrLogStats As System.Windows.Forms.Timer
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents Label9 As System.Windows.Forms.Label
    Friend WithEvents lblCF As System.Windows.Forms.Label
    Friend WithEvents RadioMenu As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents lblHost As System.Windows.Forms.Label
    Friend WithEvents IDFrameToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents TwoToneTestToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ShowTestFormMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents CWIDToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents Ipdaemon1 As nsoftware.IPWorks.Ipdaemon
    Friend WithEvents ChannelBusyToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ACKToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem

End Class
