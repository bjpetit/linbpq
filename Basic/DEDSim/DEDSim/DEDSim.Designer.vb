<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class DEDSIM
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
      Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
      Me.TextBox1 = New System.Windows.Forms.TextBox
      Me.SuspendLayout()
      '
      'Timer1
      '
      Me.Timer1.Enabled = True
      '
      'TextBox1
      '
      Me.TextBox1.Location = New System.Drawing.Point(12, 12)
      Me.TextBox1.Multiline = True
      Me.TextBox1.Name = "TextBox1"
      Me.TextBox1.ScrollBars = System.Windows.Forms.ScrollBars.Both
      Me.TextBox1.Size = New System.Drawing.Size(380, 232)
      Me.TextBox1.TabIndex = 0
      '
      'DEDSIM
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(404, 256)
      Me.Controls.Add(Me.TextBox1)
      Me.Name = "DEDSIM"
      Me.Text = "DED HostMode Simulator"
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents Timer1 As System.Windows.Forms.Timer
   Friend WithEvents TextBox1 As System.Windows.Forms.TextBox

End Class
