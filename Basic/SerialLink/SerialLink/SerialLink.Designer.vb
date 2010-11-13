<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class SerialLink
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
      Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(SerialLink))
      Me.SerialPort1 = New System.IO.Ports.SerialPort(Me.components)
      Me.SerialPort2 = New System.IO.Ports.SerialPort(Me.components)
      Me.CheckBox1 = New System.Windows.Forms.CheckBox
      Me.CheckBox2 = New System.Windows.Forms.CheckBox
      Me.Open = New System.Windows.Forms.Button
      Me.CloseButton = New System.Windows.Forms.Button
      Me.AxWinsock1 = New AxMSWinsockLib.AxWinsock
      Me.TCPState = New System.Windows.Forms.Label
      Me.TCPHostName = New System.Windows.Forms.TextBox
      Me.OptSerial = New System.Windows.Forms.RadioButton
      Me.OptTCPMaster = New System.Windows.Forms.RadioButton
      Me.OptTCPSlave = New System.Windows.Forms.RadioButton
      Me.Label1 = New System.Windows.Forms.Label
      Me.TCPPort = New System.Windows.Forms.TextBox
      Me.Label2 = New System.Windows.Forms.Label
      Me.Label3 = New System.Windows.Forms.Label
      Me.LocalCOM = New System.Windows.Forms.ComboBox
      Me.LocallBaud = New System.Windows.Forms.ComboBox
      Me.Label4 = New System.Windows.Forms.Label
      Me.RemoteCOM = New System.Windows.Forms.ComboBox
      Me.RemoteBAUD = New System.Windows.Forms.ComboBox
      Me.COMTypeR = New System.Windows.Forms.RadioButton
      Me.COMTypeV = New System.Windows.Forms.RadioButton
      Me.GroupBox1 = New System.Windows.Forms.GroupBox
      Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
      CType(Me.AxWinsock1, System.ComponentModel.ISupportInitialize).BeginInit()
      Me.GroupBox1.SuspendLayout()
      Me.SuspendLayout()
      '
      'SerialPort1
      '
      Me.SerialPort1.BaudRate = 38400
      Me.SerialPort1.DtrEnable = True
      Me.SerialPort1.PortName = "COM100"
      Me.SerialPort1.RtsEnable = True
      '
      'SerialPort2
      '
      Me.SerialPort2.BaudRate = 38400
      Me.SerialPort2.DtrEnable = True
      Me.SerialPort2.RtsEnable = True
      '
      'CheckBox1
      '
      Me.CheckBox1.AutoSize = True
      Me.CheckBox1.Location = New System.Drawing.Point(14, 129)
      Me.CheckBox1.Name = "CheckBox1"
      Me.CheckBox1.Size = New System.Drawing.Size(68, 17)
      Me.CheckBox1.TabIndex = 3
      Me.CheckBox1.Text = "COM100"
      Me.CheckBox1.UseVisualStyleBackColor = True
      '
      'CheckBox2
      '
      Me.CheckBox2.AutoSize = True
      Me.CheckBox2.Location = New System.Drawing.Point(204, 129)
      Me.CheckBox2.Name = "CheckBox2"
      Me.CheckBox2.Size = New System.Drawing.Size(56, 17)
      Me.CheckBox2.TabIndex = 4
      Me.CheckBox2.Text = "COM1"
      Me.CheckBox2.UseVisualStyleBackColor = True
      '
      'Open
      '
      Me.Open.Location = New System.Drawing.Point(35, 299)
      Me.Open.Name = "Open"
      Me.Open.Size = New System.Drawing.Size(99, 25)
      Me.Open.TabIndex = 5
      Me.Open.Text = "Open"
      Me.Open.UseVisualStyleBackColor = True
      '
      'CloseButton
      '
      Me.CloseButton.Location = New System.Drawing.Point(186, 299)
      Me.CloseButton.Name = "CloseButton"
      Me.CloseButton.Size = New System.Drawing.Size(99, 25)
      Me.CloseButton.TabIndex = 6
      Me.CloseButton.Text = "Close"
      Me.CloseButton.UseVisualStyleBackColor = True
      '
      'AxWinsock1
      '
      Me.AxWinsock1.Enabled = True
      Me.AxWinsock1.Location = New System.Drawing.Point(282, 266)
      Me.AxWinsock1.Name = "AxWinsock1"
      Me.AxWinsock1.OcxState = CType(resources.GetObject("AxWinsock1.OcxState"), System.Windows.Forms.AxHost.State)
      Me.AxWinsock1.Size = New System.Drawing.Size(28, 28)
      Me.AxWinsock1.TabIndex = 9
      '
      'TCPState
      '
      Me.TCPState.AutoSize = True
      Me.TCPState.Location = New System.Drawing.Point(50, 268)
      Me.TCPState.Name = "TCPState"
      Me.TCPState.Size = New System.Drawing.Size(39, 13)
      Me.TCPState.TabIndex = 10
      Me.TCPState.Text = "Label1"
      '
      'TCPHostName
      '
      Me.TCPHostName.Location = New System.Drawing.Point(12, 234)
      Me.TCPHostName.Name = "TCPHostName"
      Me.TCPHostName.Size = New System.Drawing.Size(122, 20)
      Me.TCPHostName.TabIndex = 12
      '
      'OptSerial
      '
      Me.OptSerial.AutoSize = True
      Me.OptSerial.Location = New System.Drawing.Point(20, 27)
      Me.OptSerial.Name = "OptSerial"
      Me.OptSerial.Size = New System.Drawing.Size(55, 17)
      Me.OptSerial.TabIndex = 13
      Me.OptSerial.TabStop = True
      Me.OptSerial.Text = "Seria1"
      Me.OptSerial.UseVisualStyleBackColor = True
      '
      'OptTCPMaster
      '
      Me.OptTCPMaster.AutoSize = True
      Me.OptTCPMaster.Location = New System.Drawing.Point(20, 166)
      Me.OptTCPMaster.Name = "OptTCPMaster"
      Me.OptTCPMaster.Size = New System.Drawing.Size(81, 17)
      Me.OptTCPMaster.TabIndex = 14
      Me.OptTCPMaster.TabStop = True
      Me.OptTCPMaster.Text = "TCP Master"
      Me.OptTCPMaster.UseVisualStyleBackColor = True
      '
      'OptTCPSlave
      '
      Me.OptTCPSlave.AutoSize = True
      Me.OptTCPSlave.Location = New System.Drawing.Point(20, 190)
      Me.OptTCPSlave.Name = "OptTCPSlave"
      Me.OptTCPSlave.Size = New System.Drawing.Size(76, 17)
      Me.OptTCPSlave.TabIndex = 15
      Me.OptTCPSlave.TabStop = True
      Me.OptTCPSlave.Text = "TCP Slave"
      Me.OptTCPSlave.UseVisualStyleBackColor = True
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(17, 213)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(53, 13)
      Me.Label1.TabIndex = 16
      Me.Label1.Text = "TCP Host"
      '
      'TCPPort
      '
      Me.TCPPort.Location = New System.Drawing.Point(141, 234)
      Me.TCPPort.Name = "TCPPort"
      Me.TCPPort.Size = New System.Drawing.Size(37, 20)
      Me.TCPPort.TabIndex = 17
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(17, 9)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(255, 13)
      Me.Label2.TabIndex = 18
      Me.Label2.Text = "Remote Port                                               Local Port"
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(134, 213)
      Me.Label3.Name = "Label3"
      Me.Label3.Size = New System.Drawing.Size(50, 13)
      Me.Label3.TabIndex = 19
      Me.Label3.Text = "TCP Port"
      '
      'LocalCOM
      '
      Me.LocalCOM.FormattingEnabled = True
      Me.LocalCOM.Location = New System.Drawing.Point(204, 59)
      Me.LocalCOM.Name = "LocalCOM"
      Me.LocalCOM.Size = New System.Drawing.Size(109, 21)
      Me.LocalCOM.TabIndex = 20
      '
      'LocallBaud
      '
      Me.LocallBaud.FormattingEnabled = True
      Me.LocallBaud.Location = New System.Drawing.Point(204, 93)
      Me.LocallBaud.Name = "LocallBaud"
      Me.LocallBaud.Size = New System.Drawing.Size(109, 21)
      Me.LocallBaud.TabIndex = 21
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(11, 268)
      Me.Label4.Name = "Label4"
      Me.Label4.Size = New System.Drawing.Size(35, 13)
      Me.Label4.TabIndex = 22
      Me.Label4.Text = "State:"
      '
      'RemoteCOM
      '
      Me.RemoteCOM.FormattingEnabled = True
      Me.RemoteCOM.Location = New System.Drawing.Point(12, 59)
      Me.RemoteCOM.Name = "RemoteCOM"
      Me.RemoteCOM.Size = New System.Drawing.Size(109, 21)
      Me.RemoteCOM.TabIndex = 23
      '
      'RemoteBAUD
      '
      Me.RemoteBAUD.FormattingEnabled = True
      Me.RemoteBAUD.Location = New System.Drawing.Point(12, 93)
      Me.RemoteBAUD.Name = "RemoteBAUD"
      Me.RemoteBAUD.Size = New System.Drawing.Size(109, 21)
      Me.RemoteBAUD.TabIndex = 24
      '
      'COMTypeR
      '
      Me.COMTypeR.AutoSize = True
      Me.COMTypeR.Location = New System.Drawing.Point(0, 14)
      Me.COMTypeR.Name = "COMTypeR"
      Me.COMTypeR.Size = New System.Drawing.Size(74, 17)
      Me.COMTypeR.TabIndex = 25
      Me.COMTypeR.TabStop = True
      Me.COMTypeR.Text = "Real COM"
      Me.COMTypeR.UseVisualStyleBackColor = True
      '
      'COMTypeV
      '
      Me.COMTypeV.AutoSize = True
      Me.COMTypeV.Location = New System.Drawing.Point(0, 38)
      Me.COMTypeV.Name = "COMTypeV"
      Me.COMTypeV.Size = New System.Drawing.Size(81, 17)
      Me.COMTypeV.TabIndex = 26
      Me.COMTypeV.TabStop = True
      Me.COMTypeV.Text = "Virtual COM"
      Me.COMTypeV.UseVisualStyleBackColor = True
      '
      'GroupBox1
      '
      Me.GroupBox1.Controls.Add(Me.COMTypeR)
      Me.GroupBox1.Controls.Add(Me.COMTypeV)
      Me.GroupBox1.Location = New System.Drawing.Point(204, 152)
      Me.GroupBox1.Name = "GroupBox1"
      Me.GroupBox1.Size = New System.Drawing.Size(109, 74)
      Me.GroupBox1.TabIndex = 27
      Me.GroupBox1.TabStop = False
      '
      'Timer1
      '
      Me.Timer1.Interval = 10
      '
      'SerialLink
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(323, 333)
      Me.Controls.Add(Me.GroupBox1)
      Me.Controls.Add(Me.RemoteBAUD)
      Me.Controls.Add(Me.RemoteCOM)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.LocallBaud)
      Me.Controls.Add(Me.LocalCOM)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.TCPPort)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.OptTCPSlave)
      Me.Controls.Add(Me.OptTCPMaster)
      Me.Controls.Add(Me.OptSerial)
      Me.Controls.Add(Me.TCPHostName)
      Me.Controls.Add(Me.TCPState)
      Me.Controls.Add(Me.AxWinsock1)
      Me.Controls.Add(Me.CloseButton)
      Me.Controls.Add(Me.Open)
      Me.Controls.Add(Me.CheckBox2)
      Me.Controls.Add(Me.CheckBox1)
      Me.Name = "SerialLink"
      Me.Text = "Serial Link"
      CType(Me.AxWinsock1, System.ComponentModel.ISupportInitialize).EndInit()
      Me.GroupBox1.ResumeLayout(False)
      Me.GroupBox1.PerformLayout()
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents SerialPort1 As System.IO.Ports.SerialPort
   Friend WithEvents SerialPort2 As System.IO.Ports.SerialPort
   Friend WithEvents CheckBox1 As System.Windows.Forms.CheckBox
   Friend WithEvents CheckBox2 As System.Windows.Forms.CheckBox
   Friend WithEvents Open As System.Windows.Forms.Button
   Friend WithEvents CloseButton As System.Windows.Forms.Button
   Friend WithEvents AxWinsock1 As AxMSWinsockLib.AxWinsock
   Friend WithEvents TCPState As System.Windows.Forms.Label
   Friend WithEvents TCPHostName As System.Windows.Forms.TextBox
   Friend WithEvents OptSerial As System.Windows.Forms.RadioButton
   Friend WithEvents OptTCPMaster As System.Windows.Forms.RadioButton
   Friend WithEvents OptTCPSlave As System.Windows.Forms.RadioButton
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents TCPPort As System.Windows.Forms.TextBox
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents Label3 As System.Windows.Forms.Label
   Friend WithEvents LocalCOM As System.Windows.Forms.ComboBox
   Friend WithEvents LocallBaud As System.Windows.Forms.ComboBox
   Friend WithEvents Label4 As System.Windows.Forms.Label
   Friend WithEvents RemoteCOM As System.Windows.Forms.ComboBox
   Friend WithEvents RemoteBAUD As System.Windows.Forms.ComboBox
   Friend WithEvents COMTypeR As System.Windows.Forms.RadioButton
   Friend WithEvents COMTypeV As System.Windows.Forms.RadioButton
   Friend WithEvents GroupBox1 As System.Windows.Forms.GroupBox
   Friend WithEvents Timer1 As System.Windows.Forms.Timer

End Class
