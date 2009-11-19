<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class WINMORSim
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
      Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(WINMORSim))
      Me.AxWinsock1 = New AxMSWinsockLib.AxWinsock
      Me.AxWinsock2 = New AxMSWinsockLib.AxWinsock
      Me.Label1 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.Label3 = New System.Windows.Forms.Label
      Me.Label4 = New System.Windows.Forms.Label
      Me.CmdTextBox = New System.Windows.Forms.TextBox
      Me.DataTextBox = New System.Windows.Forms.TextBox
      Me.SendConnected = New System.Windows.Forms.Button
      Me.DIS = New System.Windows.Forms.Button
      CType(Me.AxWinsock1, System.ComponentModel.ISupportInitialize).BeginInit()
      CType(Me.AxWinsock2, System.ComponentModel.ISupportInitialize).BeginInit()
      Me.SuspendLayout()
      '
      'AxWinsock1
      '
      Me.AxWinsock1.Enabled = True
      Me.AxWinsock1.Location = New System.Drawing.Point(272, 216)
      Me.AxWinsock1.Name = "AxWinsock1"
      Me.AxWinsock1.OcxState = CType(resources.GetObject("AxWinsock1.OcxState"), System.Windows.Forms.AxHost.State)
      Me.AxWinsock1.Size = New System.Drawing.Size(28, 28)
      Me.AxWinsock1.TabIndex = 0
      '
      'AxWinsock2
      '
      Me.AxWinsock2.Enabled = True
      Me.AxWinsock2.Location = New System.Drawing.Point(306, 216)
      Me.AxWinsock2.Name = "AxWinsock2"
      Me.AxWinsock2.OcxState = CType(resources.GetObject("AxWinsock2.OcxState"), System.Windows.Forms.AxHost.State)
      Me.AxWinsock2.Size = New System.Drawing.Size(28, 28)
      Me.AxWinsock2.TabIndex = 1
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(7, 35)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(39, 13)
      Me.Label1.TabIndex = 2
      Me.Label1.Text = "Label1"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(250, 35)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(39, 13)
      Me.Label2.TabIndex = 3
      Me.Label2.Text = "Label2"
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(7, 12)
      Me.Label3.Name = "Label3"
      Me.Label3.Size = New System.Drawing.Size(80, 13)
      Me.Label3.TabIndex = 4
      Me.Label3.Text = "Control Session"
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(250, 12)
      Me.Label4.Name = "Label4"
      Me.Label4.Size = New System.Drawing.Size(70, 13)
      Me.Label4.TabIndex = 5
      Me.Label4.Text = "Data Session"
      '
      'CmdTextBox
      '
      Me.CmdTextBox.Location = New System.Drawing.Point(10, 67)
      Me.CmdTextBox.Multiline = True
      Me.CmdTextBox.Name = "CmdTextBox"
      Me.CmdTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both
      Me.CmdTextBox.Size = New System.Drawing.Size(226, 143)
      Me.CmdTextBox.TabIndex = 6
      '
      'DataTextBox
      '
      Me.DataTextBox.Location = New System.Drawing.Point(253, 67)
      Me.DataTextBox.Multiline = True
      Me.DataTextBox.Name = "DataTextBox"
      Me.DataTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both
      Me.DataTextBox.Size = New System.Drawing.Size(226, 143)
      Me.DataTextBox.TabIndex = 7
      '
      'SendConnected
      '
      Me.SendConnected.Location = New System.Drawing.Point(13, 216)
      Me.SendConnected.Name = "SendConnected"
      Me.SendConnected.Size = New System.Drawing.Size(99, 30)
      Me.SendConnected.TabIndex = 8
      Me.SendConnected.Text = "Send Connected"
      Me.SendConnected.UseVisualStyleBackColor = True
      '
      'DIS
      '
      Me.DIS.Location = New System.Drawing.Point(129, 217)
      Me.DIS.Name = "DIS"
      Me.DIS.Size = New System.Drawing.Size(94, 28)
      Me.DIS.TabIndex = 10
      Me.DIS.Text = "Disconnect"
      Me.DIS.UseVisualStyleBackColor = True
      '
      'WINMORSim
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(492, 254)
      Me.Controls.Add(Me.DIS)
      Me.Controls.Add(Me.SendConnected)
      Me.Controls.Add(Me.DataTextBox)
      Me.Controls.Add(Me.CmdTextBox)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.AxWinsock2)
      Me.Controls.Add(Me.AxWinsock1)
      Me.Name = "WINMORSim"
      Me.Text = "WIMOR TNC Simulator"
      CType(Me.AxWinsock1, System.ComponentModel.ISupportInitialize).EndInit()
      CType(Me.AxWinsock2, System.ComponentModel.ISupportInitialize).EndInit()
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents AxWinsock1 As AxMSWinsockLib.AxWinsock
   Friend WithEvents AxWinsock2 As AxMSWinsockLib.AxWinsock
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents Label3 As System.Windows.Forms.Label
   Friend WithEvents Label4 As System.Windows.Forms.Label
   Friend WithEvents CmdTextBox As System.Windows.Forms.TextBox
   Friend WithEvents DataTextBox As System.Windows.Forms.TextBox
   Friend WithEvents SendConnected As System.Windows.Forms.Button
   Friend WithEvents DIS As System.Windows.Forms.Button

End Class
