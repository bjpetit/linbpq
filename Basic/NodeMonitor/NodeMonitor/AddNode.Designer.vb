<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class AddNode
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
      Me.LatBox = New System.Windows.Forms.TextBox
      Me.Label5 = New System.Windows.Forms.Label
      Me.Label4 = New System.Windows.Forms.Label
      Me.UpIconBox = New System.Windows.Forms.TextBox
      Me.PopupBox = New System.Windows.Forms.TextBox
      Me.LonBox = New System.Windows.Forms.TextBox
      Me.Label3 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.Label1 = New System.Windows.Forms.Label
      Me.DownIconBox = New System.Windows.Forms.TextBox
      Me.Label6 = New System.Windows.Forms.Label
      Me.CallBox = New System.Windows.Forms.ComboBox
      Me.LOC = New System.Windows.Forms.Label
      Me.HoverButton = New System.Windows.Forms.RadioButton
      Me.ClickButton = New System.Windows.Forms.RadioButton
      Me.Label7 = New System.Windows.Forms.Label
      Me.IconButton = New System.Windows.Forms.Button
      Me.PictureBox1 = New System.Windows.Forms.PictureBox
      Me.PictureBox2 = New System.Windows.Forms.PictureBox
      Me.IconTune = New System.Windows.Forms.TrackBar
      Me.Label8 = New System.Windows.Forms.Label
      Me.DDMMSS = New System.Windows.Forms.Label
      Me.Cancel_Button = New System.Windows.Forms.Button
      Me.OK_Button = New System.Windows.Forms.Button
      Me.Check = New System.Windows.Forms.Button
      Me.Delete_Button = New System.Windows.Forms.Button
      CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).BeginInit()
      CType(Me.PictureBox2, System.ComponentModel.ISupportInitialize).BeginInit()
      CType(Me.IconTune, System.ComponentModel.ISupportInitialize).BeginInit()
      Me.SuspendLayout()
      '
      'LatBox
      '
      Me.LatBox.Location = New System.Drawing.Point(104, 56)
      Me.LatBox.Name = "LatBox"
      Me.LatBox.Size = New System.Drawing.Size(88, 20)
      Me.LatBox.TabIndex = 20
      '
      'Label5
      '
      Me.Label5.AutoSize = True
      Me.Label5.Location = New System.Drawing.Point(27, 59)
      Me.Label5.Name = "Label5"
      Me.Label5.Size = New System.Drawing.Size(22, 13)
      Me.Label5.TabIndex = 19
      Me.Label5.Text = "Lat"
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(27, 29)
      Me.Label4.Name = "Label4"
      Me.Label4.Size = New System.Drawing.Size(24, 13)
      Me.Label4.TabIndex = 17
      Me.Label4.Text = "Call"
      '
      'UpIconBox
      '
      Me.UpIconBox.Location = New System.Drawing.Point(104, 218)
      Me.UpIconBox.Name = "UpIconBox"
      Me.UpIconBox.Size = New System.Drawing.Size(128, 20)
      Me.UpIconBox.TabIndex = 16
      '
      'PopupBox
      '
      Me.PopupBox.Location = New System.Drawing.Point(104, 116)
      Me.PopupBox.Multiline = True
      Me.PopupBox.Name = "PopupBox"
      Me.PopupBox.Size = New System.Drawing.Size(336, 52)
      Me.PopupBox.TabIndex = 15
      '
      'LonBox
      '
      Me.LonBox.Location = New System.Drawing.Point(104, 86)
      Me.LonBox.Name = "LonBox"
      Me.LonBox.Size = New System.Drawing.Size(88, 20)
      Me.LonBox.TabIndex = 14
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(27, 221)
      Me.Label3.Name = "Label3"
      Me.Label3.Size = New System.Drawing.Size(45, 13)
      Me.Label3.TabIndex = 13
      Me.Label3.Text = "Up Icon"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(27, 119)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(59, 13)
      Me.Label2.TabIndex = 12
      Me.Label2.Text = "PopupText"
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(27, 89)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(25, 13)
      Me.Label1.TabIndex = 11
      Me.Label1.Text = "Lon"
      '
      'DownIconBox
      '
      Me.DownIconBox.Location = New System.Drawing.Point(104, 272)
      Me.DownIconBox.Name = "DownIconBox"
      Me.DownIconBox.Size = New System.Drawing.Size(128, 20)
      Me.DownIconBox.TabIndex = 22
      '
      'Label6
      '
      Me.Label6.AutoSize = True
      Me.Label6.Location = New System.Drawing.Point(27, 275)
      Me.Label6.Name = "Label6"
      Me.Label6.Size = New System.Drawing.Size(59, 13)
      Me.Label6.TabIndex = 21
      Me.Label6.Text = "Down Icon"
      '
      'CallBox
      '
      Me.CallBox.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest
      Me.CallBox.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.ListItems
      Me.CallBox.FormattingEnabled = True
      Me.CallBox.Location = New System.Drawing.Point(104, 26)
      Me.CallBox.Name = "CallBox"
      Me.CallBox.Size = New System.Drawing.Size(121, 21)
      Me.CallBox.Sorted = True
      Me.CallBox.TabIndex = 23
      '
      'LOC
      '
      Me.LOC.AutoSize = True
      Me.LOC.Location = New System.Drawing.Point(346, 59)
      Me.LOC.Name = "LOC"
      Me.LOC.Size = New System.Drawing.Size(37, 13)
      Me.LOC.TabIndex = 24
      Me.LOC.Text = "          "
      '
      'HoverButton
      '
      Me.HoverButton.AutoSize = True
      Me.HoverButton.Location = New System.Drawing.Point(112, 184)
      Me.HoverButton.Name = "HoverButton"
      Me.HoverButton.Size = New System.Drawing.Size(54, 17)
      Me.HoverButton.TabIndex = 25
      Me.HoverButton.TabStop = True
      Me.HoverButton.Text = "Hover"
      Me.HoverButton.UseVisualStyleBackColor = True
      '
      'ClickButton
      '
      Me.ClickButton.AutoSize = True
      Me.ClickButton.Location = New System.Drawing.Point(184, 184)
      Me.ClickButton.Name = "ClickButton"
      Me.ClickButton.Size = New System.Drawing.Size(48, 17)
      Me.ClickButton.TabIndex = 26
      Me.ClickButton.TabStop = True
      Me.ClickButton.Text = "Click"
      Me.ClickButton.UseVisualStyleBackColor = True
      '
      'Label7
      '
      Me.Label7.AutoSize = True
      Me.Label7.Location = New System.Drawing.Point(27, 184)
      Me.Label7.Name = "Label7"
      Me.Label7.Size = New System.Drawing.Size(68, 13)
      Me.Label7.TabIndex = 27
      Me.Label7.Text = "Popup Mode"
      '
      'IconButton
      '
      Me.IconButton.Location = New System.Drawing.Point(352, 272)
      Me.IconButton.Name = "IconButton"
      Me.IconButton.Size = New System.Drawing.Size(75, 23)
      Me.IconButton.TabIndex = 28
      Me.IconButton.Text = "Create Icons"
      Me.IconButton.UseVisualStyleBackColor = True
      '
      'PictureBox1
      '
      Me.PictureBox1.Location = New System.Drawing.Point(250, 216)
      Me.PictureBox1.Name = "PictureBox1"
      Me.PictureBox1.Size = New System.Drawing.Size(34, 32)
      Me.PictureBox1.TabIndex = 29
      Me.PictureBox1.TabStop = False
      '
      'PictureBox2
      '
      Me.PictureBox2.Location = New System.Drawing.Point(248, 264)
      Me.PictureBox2.Name = "PictureBox2"
      Me.PictureBox2.Size = New System.Drawing.Size(36, 32)
      Me.PictureBox2.TabIndex = 30
      Me.PictureBox2.TabStop = False
      '
      'IconTune
      '
      Me.IconTune.Location = New System.Drawing.Point(344, 208)
      Me.IconTune.Name = "IconTune"
      Me.IconTune.Size = New System.Drawing.Size(88, 56)
      Me.IconTune.TabIndex = 31
      Me.IconTune.Value = 5
      '
      'Label8
      '
      Me.Label8.AutoSize = True
      Me.Label8.Location = New System.Drawing.Point(352, 184)
      Me.Label8.Name = "Label8"
      Me.Label8.Size = New System.Drawing.Size(87, 13)
      Me.Label8.TabIndex = 32
      Me.Label8.Text = "Tune Icon Width"
      '
      'DDMMSS
      '
      Me.DDMMSS.AutoSize = True
      Me.DDMMSS.Location = New System.Drawing.Point(210, 59)
      Me.DDMMSS.Name = "DDMMSS"
      Me.DDMMSS.Size = New System.Drawing.Size(97, 13)
      Me.DDMMSS.TabIndex = 33
      Me.DDMMSS.Text = "                              "
      '
      'Cancel_Button
      '
      Me.Cancel_Button.Anchor = System.Windows.Forms.AnchorStyles.None
      Me.Cancel_Button.DialogResult = System.Windows.Forms.DialogResult.Cancel
      Me.Cancel_Button.Location = New System.Drawing.Point(313, 346)
      Me.Cancel_Button.Name = "Cancel_Button"
      Me.Cancel_Button.Size = New System.Drawing.Size(67, 23)
      Me.Cancel_Button.TabIndex = 1
      Me.Cancel_Button.Text = "Cancel"
      '
      'OK_Button
      '
      Me.OK_Button.Anchor = System.Windows.Forms.AnchorStyles.None
      Me.OK_Button.Location = New System.Drawing.Point(167, 346)
      Me.OK_Button.Name = "OK_Button"
      Me.OK_Button.Size = New System.Drawing.Size(67, 23)
      Me.OK_Button.TabIndex = 0
      Me.OK_Button.Text = "Save"
      '
      'Check
      '
      Me.Check.Location = New System.Drawing.Point(94, 346)
      Me.Check.Name = "Check"
      Me.Check.Size = New System.Drawing.Size(67, 23)
      Me.Check.TabIndex = 25
      Me.Check.Text = "Check"
      Me.Check.UseVisualStyleBackColor = True
      '
      'Delete_Button
      '
      Me.Delete_Button.Anchor = System.Windows.Forms.AnchorStyles.None
      Me.Delete_Button.Location = New System.Drawing.Point(240, 346)
      Me.Delete_Button.Name = "Delete_Button"
      Me.Delete_Button.Size = New System.Drawing.Size(67, 23)
      Me.Delete_Button.TabIndex = 34
      Me.Delete_Button.Text = "Delete"
      '
      'AddNode
      '
      Me.AcceptButton = Me.OK_Button
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.CancelButton = Me.Cancel_Button
      Me.ClientSize = New System.Drawing.Size(474, 387)
      Me.Controls.Add(Me.Cancel_Button)
      Me.Controls.Add(Me.Delete_Button)
      Me.Controls.Add(Me.Check)
      Me.Controls.Add(Me.OK_Button)
      Me.Controls.Add(Me.DDMMSS)
      Me.Controls.Add(Me.Label8)
      Me.Controls.Add(Me.IconTune)
      Me.Controls.Add(Me.PictureBox2)
      Me.Controls.Add(Me.PictureBox1)
      Me.Controls.Add(Me.IconButton)
      Me.Controls.Add(Me.Label7)
      Me.Controls.Add(Me.ClickButton)
      Me.Controls.Add(Me.HoverButton)
      Me.Controls.Add(Me.LOC)
      Me.Controls.Add(Me.CallBox)
      Me.Controls.Add(Me.DownIconBox)
      Me.Controls.Add(Me.Label6)
      Me.Controls.Add(Me.LatBox)
      Me.Controls.Add(Me.Label5)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.UpIconBox)
      Me.Controls.Add(Me.PopupBox)
      Me.Controls.Add(Me.LonBox)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
      Me.MaximizeBox = False
      Me.MinimizeBox = False
      Me.Name = "AddNode"
      Me.ShowInTaskbar = False
      Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
      Me.Text = "Maintain Chat Node List"
      CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).EndInit()
      CType(Me.PictureBox2, System.ComponentModel.ISupportInitialize).EndInit()
      CType(Me.IconTune, System.ComponentModel.ISupportInitialize).EndInit()
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents LatBox As System.Windows.Forms.TextBox
   Friend WithEvents Label5 As System.Windows.Forms.Label
   Friend WithEvents Label4 As System.Windows.Forms.Label
   Friend WithEvents UpIconBox As System.Windows.Forms.TextBox
   Friend WithEvents PopupBox As System.Windows.Forms.TextBox
   Friend WithEvents LonBox As System.Windows.Forms.TextBox
   Friend WithEvents Label3 As System.Windows.Forms.Label
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents DownIconBox As System.Windows.Forms.TextBox
   Friend WithEvents Label6 As System.Windows.Forms.Label
   Friend WithEvents CallBox As System.Windows.Forms.ComboBox
   Friend WithEvents LOC As System.Windows.Forms.Label
   Friend WithEvents HoverButton As System.Windows.Forms.RadioButton
   Friend WithEvents ClickButton As System.Windows.Forms.RadioButton
   Friend WithEvents Label7 As System.Windows.Forms.Label
   Friend WithEvents IconButton As System.Windows.Forms.Button
   Friend WithEvents PictureBox1 As System.Windows.Forms.PictureBox
   Friend WithEvents PictureBox2 As System.Windows.Forms.PictureBox
   Friend WithEvents IconTune As System.Windows.Forms.TrackBar
   Friend WithEvents Label8 As System.Windows.Forms.Label
   Friend WithEvents DDMMSS As System.Windows.Forms.Label
   Friend WithEvents Cancel_Button As System.Windows.Forms.Button
   Friend WithEvents OK_Button As System.Windows.Forms.Button
   Friend WithEvents Check As System.Windows.Forms.Button
   Friend WithEvents Delete_Button As System.Windows.Forms.Button

End Class
