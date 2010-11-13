<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class SimpleHelp
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
      Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(SimpleHelp))
      Me.OK_Button = New System.Windows.Forms.Button
      Me.TextBox1 = New System.Windows.Forms.TextBox
      Me.SuspendLayout()
      '
      'OK_Button
      '
      Me.OK_Button.Anchor = System.Windows.Forms.AnchorStyles.None
      Me.OK_Button.Location = New System.Drawing.Point(250, 272)
      Me.OK_Button.Name = "OK_Button"
      Me.OK_Button.Size = New System.Drawing.Size(67, 23)
      Me.OK_Button.TabIndex = 0
      Me.OK_Button.Text = "OK"
      '
      'TextBox1
      '
      Me.TextBox1.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
      Me.TextBox1.Location = New System.Drawing.Point(16, 17)
      Me.TextBox1.Multiline = True
      Me.TextBox1.Name = "TextBox1"
      Me.TextBox1.Size = New System.Drawing.Size(534, 238)
      Me.TextBox1.TabIndex = 1
      Me.TextBox1.Text = resources.GetString("TextBox1.Text")
      '
      'SimpleHelp
      '
      Me.AcceptButton = Me.OK_Button
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(567, 319)
      Me.Controls.Add(Me.TextBox1)
      Me.Controls.Add(Me.OK_Button)
      Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
      Me.MaximizeBox = False
      Me.MinimizeBox = False
      Me.Name = "SimpleHelp"
      Me.ShowInTaskbar = False
      Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
      Me.Text = "WinBPQCfg Simple Mode Help"
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
    Friend WithEvents OK_Button As System.Windows.Forms.Button
    Friend WithEvents TextBox1 As System.Windows.Forms.TextBox

End Class
