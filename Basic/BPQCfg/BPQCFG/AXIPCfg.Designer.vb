<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class AXIPCfg
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
      Me.Save_Button = New System.Windows.Forms.Button
      Me.Cancel_Button = New System.Windows.Forms.Button
      Me.Label1 = New System.Windows.Forms.Label
      Me.ToolTip1 = New System.Windows.Forms.ToolTip(Me.components)
      Me.UDPPort1 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.UDPPort2 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.UDPPort3 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.UDPPort4 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.UDPPort5 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.UDPPort6 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.UDPPort7 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.UDPPort8 = New BPQCFG.NumOrEmptyTextBox(65535)
      Me.Label2 = New System.Windows.Forms.Label
      Me.EnableMHeard = New System.Windows.Forms.CheckBox
      Me.SuspendLayout()
      '
      'Save_Button
      ' 
      Me.Save_Button.Anchor = System.Windows.Forms.AnchorStyles.Top
      Me.Save_Button.Location = New System.Drawing.Point(14, 3)
      Me.Save_Button.Name = "Save_Button"
      Me.Save_Button.Size = New System.Drawing.Size(67, 23)
      Me.Save_Button.TabIndex = 0
      Me.Save_Button.Text = "Save"
      '
      'Cancel_Button
      '
      Me.Cancel_Button.Anchor = System.Windows.Forms.AnchorStyles.Top
      Me.Cancel_Button.DialogResult = System.Windows.Forms.DialogResult.Cancel
      Me.Cancel_Button.Location = New System.Drawing.Point(87, 3)
      Me.Cancel_Button.Name = "Cancel_Button"
      Me.Cancel_Button.Size = New System.Drawing.Size(67, 23)
      Me.Cancel_Button.TabIndex = 1
      Me.Cancel_Button.Text = "Cancel"
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(11, 40)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(57, 13)
      Me.Label1.TabIndex = 1
      Me.Label1.Text = "UDP Ports"
      Me.ToolTip1.SetToolTip(Me.Label1, "Ports to listen on for UDP packets")
      '
      'UDPPort1
      '
      Me.UDPPort1.Location = New System.Drawing.Point(74, 37)
      Me.UDPPort1.Max = 0
      Me.UDPPort1.Name = "UDPPort1"
      Me.UDPPort1.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort1.TabIndex = 2
      Me.UDPPort1.Text = "10093"
      '
      'UDPPort2
      '
      Me.UDPPort2.Location = New System.Drawing.Point(119, 37)
      Me.UDPPort2.Max = 65535
      Me.UDPPort2.Name = "UDPPort2"
      Me.UDPPort2.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort2.TabIndex = 3
      '
      'UDPPort3
      '
      Me.UDPPort3.Location = New System.Drawing.Point(164, 37)
      Me.UDPPort3.Max = 65535
      Me.UDPPort3.Name = "UDPPort3"
      Me.UDPPort3.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort3.TabIndex = 4
      '
      'UDPPort4
      '
      Me.UDPPort4.Location = New System.Drawing.Point(209, 37)
      Me.UDPPort4.Max = 65535
      Me.UDPPort4.Name = "UDPPort4"
      Me.UDPPort4.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort4.TabIndex = 5
      '
      'UDPPort5
      '
      Me.UDPPort5.Location = New System.Drawing.Point(254, 37)
      Me.UDPPort5.Max = 65535
      Me.UDPPort5.Name = "UDPPort5"
      Me.UDPPort5.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort5.TabIndex = 6
      '
      'UDPPort6
      '
      Me.UDPPort6.Location = New System.Drawing.Point(299, 37)
      Me.UDPPort6.Max = 65535
      Me.UDPPort6.Name = "UDPPort6"
      Me.UDPPort6.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort6.TabIndex = 7
      '
      'UDPPort7
      '
      Me.UDPPort7.Location = New System.Drawing.Point(344, 37)
      Me.UDPPort7.Max = 65535
      Me.UDPPort7.Name = "UDPPort7"
      Me.UDPPort7.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort7.TabIndex = 8
      '
      'UDPPort8
      '
      Me.UDPPort8.Location = New System.Drawing.Point(389, 37)
      Me.UDPPort8.Max = 65535
      Me.UDPPort8.Name = "UDPPort8"
      Me.UDPPort8.Size = New System.Drawing.Size(39, 20)
      Me.UDPPort8.TabIndex = 9
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(14, 80)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(340, 13)
      Me.Label2.TabIndex = 14
      Me.Label2.Text = "Call              Name/IP Address                   Port          Keepalive  Dele" & _
          "te"
      '
      'EnableMHeard
      '
      Me.EnableMHeard.AutoSize = True
      Me.EnableMHeard.Location = New System.Drawing.Point(212, 7)
      Me.EnableMHeard.Name = "EnableMHeard"
      Me.EnableMHeard.Size = New System.Drawing.Size(98, 17)
      Me.EnableMHeard.TabIndex = 15
      Me.EnableMHeard.Text = "Enable Mheard"
      Me.EnableMHeard.TextAlign = System.Drawing.ContentAlignment.MiddleRight
      Me.EnableMHeard.UseVisualStyleBackColor = True
      '
      'AXIPCfg
      '
      Me.AcceptButton = Me.Save_Button
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.AutoScroll = True
      Me.CancelButton = Me.Cancel_Button
      Me.ClientSize = New System.Drawing.Size(458, 547)
      Me.ControlBox = False
      Me.Controls.Add(Me.EnableMHeard)
      Me.Controls.Add(Me.Cancel_Button)
      Me.Controls.Add(Me.Save_Button)
      Me.Controls.Add(Me.UDPPort8)
      Me.Controls.Add(Me.UDPPort7)
      Me.Controls.Add(Me.UDPPort6)
      Me.Controls.Add(Me.UDPPort5)
      Me.Controls.Add(Me.UDPPort4)
      Me.Controls.Add(Me.UDPPort3)
      Me.Controls.Add(Me.UDPPort2)
      Me.Controls.Add(Me.UDPPort1)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.Label2)
      Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
      Me.MaximizeBox = False
      Me.MinimizeBox = False
      Me.Name = "AXIPCfg"
      Me.ShowInTaskbar = False
      Me.StartPosition = System.Windows.Forms.FormStartPosition.Manual
      Me.Text = "AXIPCfg"
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents Save_Button As System.Windows.Forms.Button
   Friend WithEvents Cancel_Button As System.Windows.Forms.Button
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents ToolTip1 As System.Windows.Forms.ToolTip
   Friend WithEvents UDPPort1 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents UDPPort2 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents UDPPort3 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents UDPPort4 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents UDPPort5 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents UDPPort6 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents UDPPort7 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents UDPPort8 As BPQCFG.NumOrEmptyTextBox
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents EnableMHeard As System.Windows.Forms.CheckBox

End Class
