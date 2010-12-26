<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class UpdateNodeDialog
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
      Me.CallBox = New System.Windows.Forms.ComboBox
      Me.Label4 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.LocatorBox = New System.Windows.Forms.TextBox
      Me.DDMMSS = New System.Windows.Forms.Label
      Me.LatBox = New System.Windows.Forms.TextBox
      Me.Label5 = New System.Windows.Forms.Label
      Me.LonBox = New System.Windows.Forms.TextBox
      Me.Label1 = New System.Windows.Forms.Label
      Me.Button1 = New System.Windows.Forms.Button
      Me.Button2 = New System.Windows.Forms.Button
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
      Me.OK_Button.Text = "Save"
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
      'CallBox
      '
      Me.CallBox.AutoCompleteMode = System.Windows.Forms.AutoCompleteMode.Suggest
      Me.CallBox.AutoCompleteSource = System.Windows.Forms.AutoCompleteSource.ListItems
      Me.CallBox.FormattingEnabled = True
      Me.CallBox.Location = New System.Drawing.Point(108, 29)
      Me.CallBox.Name = "CallBox"
      Me.CallBox.Size = New System.Drawing.Size(121, 21)
      Me.CallBox.Sorted = True
      Me.CallBox.TabIndex = 24
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(26, 32)
      Me.Label4.Name = "Label4"
      Me.Label4.Size = New System.Drawing.Size(24, 13)
      Me.Label4.TabIndex = 25
      Me.Label4.Text = "Call"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(26, 73)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(43, 13)
      Me.Label2.TabIndex = 27
      Me.Label2.Text = "Locator"
      '
      'LocatorBox
      '
      Me.LocatorBox.Location = New System.Drawing.Point(108, 73)
      Me.LocatorBox.Name = "LocatorBox"
      Me.LocatorBox.Size = New System.Drawing.Size(121, 20)
      Me.LocatorBox.TabIndex = 28
      '
      'DDMMSS
      '
      Me.DDMMSS.AutoSize = True
      Me.DDMMSS.Location = New System.Drawing.Point(214, 117)
      Me.DDMMSS.Name = "DDMMSS"
      Me.DDMMSS.Size = New System.Drawing.Size(97, 13)
      Me.DDMMSS.TabIndex = 38
      Me.DDMMSS.Text = "                              "
      '
      'LatBox
      '
      Me.LatBox.Location = New System.Drawing.Point(108, 114)
      Me.LatBox.Name = "LatBox"
      Me.LatBox.Size = New System.Drawing.Size(88, 20)
      Me.LatBox.TabIndex = 37
      '
      'Label5
      '
      Me.Label5.AutoSize = True
      Me.Label5.Location = New System.Drawing.Point(26, 114)
      Me.Label5.Name = "Label5"
      Me.Label5.Size = New System.Drawing.Size(22, 13)
      Me.Label5.TabIndex = 36
      Me.Label5.Text = "Lat"
      '
      'LonBox
      '
      Me.LonBox.Location = New System.Drawing.Point(108, 144)
      Me.LonBox.Name = "LonBox"
      Me.LonBox.Size = New System.Drawing.Size(88, 20)
      Me.LonBox.TabIndex = 35
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(26, 144)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(25, 13)
      Me.Label1.TabIndex = 34
      Me.Label1.Text = "Lon"
      '
      'Button1
      '
      Me.Button1.Location = New System.Drawing.Point(280, 29)
      Me.Button1.Name = "Button1"
      Me.Button1.Size = New System.Drawing.Size(115, 21)
      Me.Button1.TabIndex = 39
      Me.Button1.Text = "QRZ.COM Lookup"
      Me.Button1.UseVisualStyleBackColor = True
      '
      'Button2
      '
      Me.Button2.Location = New System.Drawing.Point(37, 235)
      Me.Button2.Name = "Button2"
      Me.Button2.Size = New System.Drawing.Size(85, 52)
      Me.Button2.TabIndex = 40
      Me.Button2.Text = "Button2"
      Me.Button2.UseVisualStyleBackColor = True
      '
      'UpdateNodeDialog
      '
      Me.AcceptButton = Me.OK_Button
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.CancelButton = Me.Cancel_Button
      Me.ClientSize = New System.Drawing.Size(435, 315)
      Me.Controls.Add(Me.Button2)
      Me.Controls.Add(Me.Button1)
      Me.Controls.Add(Me.DDMMSS)
      Me.Controls.Add(Me.LatBox)
      Me.Controls.Add(Me.Label5)
      Me.Controls.Add(Me.LonBox)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.LocatorBox)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.CallBox)
      Me.Controls.Add(Me.TableLayoutPanel1)
      Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
      Me.MaximizeBox = False
      Me.MinimizeBox = False
      Me.Name = "UpdateNodeDialog"
      Me.ShowInTaskbar = False
      Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
      Me.Text = "Dialog2"
      Me.TableLayoutPanel1.ResumeLayout(False)
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents TableLayoutPanel1 As System.Windows.Forms.TableLayoutPanel
   Friend WithEvents OK_Button As System.Windows.Forms.Button
   Friend WithEvents Cancel_Button As System.Windows.Forms.Button
   Friend WithEvents CallBox As System.Windows.Forms.ComboBox
   Friend WithEvents Label4 As System.Windows.Forms.Label
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents LocatorBox As System.Windows.Forms.TextBox
   Friend WithEvents DDMMSS As System.Windows.Forms.Label
   Friend WithEvents LatBox As System.Windows.Forms.TextBox
   Friend WithEvents Label5 As System.Windows.Forms.Label
   Friend WithEvents LonBox As System.Windows.Forms.TextBox
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents Button1 As System.Windows.Forms.Button
   Friend WithEvents Button2 As System.Windows.Forms.Button

End Class
