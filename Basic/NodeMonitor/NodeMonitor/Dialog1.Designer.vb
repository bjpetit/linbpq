<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class ConfigBox
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
      Me.TableLayoutPanel1 = New System.Windows.Forms.TableLayoutPanel
      Me.OK_Button = New System.Windows.Forms.Button
      Me.Cancel_Button = New System.Windows.Forms.Button
      Me.Label1 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.Label3 = New System.Windows.Forms.Label
      Me.URLBox = New System.Windows.Forms.TextBox
      Me.UserBox = New System.Windows.Forms.TextBox
      Me.PasswordBox = New System.Windows.Forms.TextBox
      Me.Label4 = New System.Windows.Forms.Label
      Me.FileNameBox = New System.Windows.Forms.TextBox
      Me.Label5 = New System.Windows.Forms.Label
      Me.OutputFileBox = New System.Windows.Forms.TextBox
      Me.MonitorNode = New System.Windows.Forms.CheckBox
      Me.MonitorUDP = New System.Windows.Forms.CheckBox
      Me.Label6 = New System.Windows.Forms.Label
      Me.PortNum = New System.Windows.Forms.TextBox
      Me.TableLayoutPanel1.SuspendLayout()
      Me.SuspendLayout()
      '
      'TableLayoutPanel1
      '
      Me.TableLayoutPanel1.Anchor = CType((System.Windows.Forms.AnchorStyles.Bottom Or System.Windows.Forms.AnchorStyles.Right), System.Windows.Forms.AnchorStyles)
      Me.TableLayoutPanel1.ColumnCount = 2
      Me.TableLayoutPanel1.ColumnStyles.Add(New System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
      Me.TableLayoutPanel1.ColumnStyles.Add(New System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
      Me.TableLayoutPanel1.Controls.Add(Me.OK_Button, 0, 0)
      Me.TableLayoutPanel1.Controls.Add(Me.Cancel_Button, 1, 0)
      Me.TableLayoutPanel1.Location = New System.Drawing.Point(277, 274)
      Me.TableLayoutPanel1.Name = "TableLayoutPanel1"
      Me.TableLayoutPanel1.RowCount = 1
      Me.TableLayoutPanel1.RowStyles.Add(New System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50.0!))
      Me.TableLayoutPanel1.Size = New System.Drawing.Size(146, 29)
      Me.TableLayoutPanel1.TabIndex = 0
      '
      'OK_Button
      '
      Me.OK_Button.Anchor = System.Windows.Forms.AnchorStyles.None
      Me.OK_Button.Location = New System.Drawing.Point(3, 3)
      Me.OK_Button.Name = "OK_Button"
      Me.OK_Button.Size = New System.Drawing.Size(67, 23)
      Me.OK_Button.TabIndex = 0
      Me.OK_Button.Text = "OK"
      '
      'Cancel_Button
      '
      Me.Cancel_Button.Anchor = System.Windows.Forms.AnchorStyles.None
      Me.Cancel_Button.DialogResult = System.Windows.Forms.DialogResult.Cancel
      Me.Cancel_Button.Location = New System.Drawing.Point(76, 3)
      Me.Cancel_Button.Name = "Cancel_Button"
      Me.Cancel_Button.Size = New System.Drawing.Size(67, 23)
      Me.Cancel_Button.TabIndex = 1
      Me.Cancel_Button.Text = "Cancel"
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(16, 125)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(52, 13)
      Me.Label1.TabIndex = 1
      Me.Label1.Text = "FTP URL"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(16, 155)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(60, 13)
      Me.Label2.TabIndex = 2
      Me.Label2.Text = "User Name"
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(16, 185)
      Me.Label3.Name = "Label3"
      Me.Label3.Size = New System.Drawing.Size(53, 13)
      Me.Label3.TabIndex = 3
      Me.Label3.Text = "Password"
      '
      'URLBox
      '
      Me.URLBox.Location = New System.Drawing.Point(128, 122)
      Me.URLBox.Name = "URLBox"
      Me.URLBox.Size = New System.Drawing.Size(233, 20)
      Me.URLBox.TabIndex = 4
      '
      'UserBox
      '
      Me.UserBox.Location = New System.Drawing.Point(128, 152)
      Me.UserBox.Name = "UserBox"
      Me.UserBox.Size = New System.Drawing.Size(168, 20)
      Me.UserBox.TabIndex = 5
      '
      'PasswordBox
      '
      Me.PasswordBox.Location = New System.Drawing.Point(128, 182)
      Me.PasswordBox.Name = "PasswordBox"
      Me.PasswordBox.PasswordChar = Global.Microsoft.VisualBasic.ChrW(42)
      Me.PasswordBox.Size = New System.Drawing.Size(100, 20)
      Me.PasswordBox.TabIndex = 6
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(16, 65)
      Me.Label4.Name = "Label4"
      Me.Label4.Size = New System.Drawing.Size(99, 13)
      Me.Label4.TabIndex = 7
      Me.Label4.Text = "Node Definition File"
      '
      'FileNameBox
      '
      Me.FileNameBox.Location = New System.Drawing.Point(128, 62)
      Me.FileNameBox.Name = "FileNameBox"
      Me.FileNameBox.Size = New System.Drawing.Size(268, 20)
      Me.FileNameBox.TabIndex = 8
      '
      'Label5
      '
      Me.Label5.AutoSize = True
      Me.Label5.Location = New System.Drawing.Point(16, 95)
      Me.Label5.Name = "Label5"
      Me.Label5.Size = New System.Drawing.Size(59, 13)
      Me.Label5.TabIndex = 9
      Me.Label5.Text = "Output FIle"
      '
      'OutputFileBox
      '
      Me.OutputFileBox.Location = New System.Drawing.Point(128, 92)
      Me.OutputFileBox.Name = "OutputFileBox"
      Me.OutputFileBox.Size = New System.Drawing.Size(268, 20)
      Me.OutputFileBox.TabIndex = 10
      '
      'MonitorNode
      '
      Me.MonitorNode.AutoSize = True
      Me.MonitorNode.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
      Me.MonitorNode.Location = New System.Drawing.Point(16, 30)
      Me.MonitorNode.Name = "MonitorNode"
      Me.MonitorNode.Size = New System.Drawing.Size(119, 17)
      Me.MonitorNode.TabIndex = 11
      Me.MonitorNode.Text = "Monitor Local Node"
      Me.MonitorNode.UseVisualStyleBackColor = True
      '
      'MonitorUDP
      '
      Me.MonitorUDP.AutoSize = True
      Me.MonitorUDP.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
      Me.MonitorUDP.Location = New System.Drawing.Point(167, 30)
      Me.MonitorUDP.Name = "MonitorUDP"
      Me.MonitorUDP.Size = New System.Drawing.Size(87, 17)
      Me.MonitorUDP.TabIndex = 12
      Me.MonitorUDP.Text = "Monitor UDP"
      Me.MonitorUDP.UseVisualStyleBackColor = True
      '
      'Label6
      '
      Me.Label6.AutoSize = True
      Me.Label6.Location = New System.Drawing.Point(274, 32)
      Me.Label6.Name = "Label6"
      Me.Label6.Size = New System.Drawing.Size(52, 13)
      Me.Label6.TabIndex = 13
      Me.Label6.Text = "UDP Port"
      '
      'PortNum
      '
      Me.PortNum.Location = New System.Drawing.Point(332, 30)
      Me.PortNum.Name = "PortNum"
      Me.PortNum.Size = New System.Drawing.Size(67, 20)
      Me.PortNum.TabIndex = 14
      '
      'ConfigBox
      '
      Me.AcceptButton = Me.OK_Button
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.CancelButton = Me.Cancel_Button
      Me.ClientSize = New System.Drawing.Size(435, 315)
      Me.Controls.Add(Me.PortNum)
      Me.Controls.Add(Me.Label6)
      Me.Controls.Add(Me.MonitorUDP)
      Me.Controls.Add(Me.MonitorNode)
      Me.Controls.Add(Me.OutputFileBox)
      Me.Controls.Add(Me.Label5)
      Me.Controls.Add(Me.FileNameBox)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.PasswordBox)
      Me.Controls.Add(Me.UserBox)
      Me.Controls.Add(Me.URLBox)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.TableLayoutPanel1)
      Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
      Me.MaximizeBox = False
      Me.MinimizeBox = False
      Me.Name = "ConfigBox"
      Me.ShowInTaskbar = False
      Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
      Me.Text = "Configuration"
      Me.TableLayoutPanel1.ResumeLayout(False)
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents TableLayoutPanel1 As System.Windows.Forms.TableLayoutPanel
   Friend WithEvents OK_Button As System.Windows.Forms.Button
   Friend WithEvents Cancel_Button As System.Windows.Forms.Button
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents Label3 As System.Windows.Forms.Label
   Friend WithEvents URLBox As System.Windows.Forms.TextBox
   Friend WithEvents UserBox As System.Windows.Forms.TextBox
   Friend WithEvents PasswordBox As System.Windows.Forms.TextBox
   Friend WithEvents Label4 As System.Windows.Forms.Label
   Friend WithEvents FileNameBox As System.Windows.Forms.TextBox
   Friend WithEvents Label5 As System.Windows.Forms.Label
   Friend WithEvents OutputFileBox As System.Windows.Forms.TextBox
   Friend WithEvents MonitorNode As System.Windows.Forms.CheckBox
   Friend WithEvents MonitorUDP As System.Windows.Forms.CheckBox
   Friend WithEvents Label6 As System.Windows.Forms.Label
   Friend WithEvents PortNum As System.Windows.Forms.TextBox

End Class
