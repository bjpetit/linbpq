<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class RigControl
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
      Me.TextBox1 = New System.Windows.Forms.TextBox
      Me.TextBox2 = New System.Windows.Forms.TextBox
      Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
      Me.Label1 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.TextBox3 = New System.Windows.Forms.TextBox
      Me.Label3 = New System.Windows.Forms.Label
      Me.SuspendLayout()
      '
      'TextBox1
      '
      Me.TextBox1.Location = New System.Drawing.Point(78, 24)
      Me.TextBox1.Name = "TextBox1"
      Me.TextBox1.Size = New System.Drawing.Size(131, 20)
      Me.TextBox1.TabIndex = 2
      '
      'TextBox2
      '
      Me.TextBox2.Location = New System.Drawing.Point(78, 68)
      Me.TextBox2.Name = "TextBox2"
      Me.TextBox2.Size = New System.Drawing.Size(131, 20)
      Me.TextBox2.TabIndex = 3
      '
      'Timer1
      '
      Me.Timer1.Enabled = True
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(14, 26)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(37, 13)
      Me.Label1.TabIndex = 4
      Me.Label1.Text = "Yaesu"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(14, 71)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(34, 13)
      Me.Label2.TabIndex = 5
      Me.Label2.Text = "ICOM"
      '
      'TextBox3
      '
      Me.TextBox3.Location = New System.Drawing.Point(78, 109)
      Me.TextBox3.Name = "TextBox3"
      Me.TextBox3.Size = New System.Drawing.Size(131, 20)
      Me.TextBox3.TabIndex = 6
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(17, 112)
      Me.Label3.Name = "Label3"
      Me.Label3.Size = New System.Drawing.Size(52, 13)
      Me.Label3.TabIndex = 7
      Me.Label3.Text = "Kenwood"
      '
      'RigControl
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(221, 163)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.TextBox3)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.TextBox2)
      Me.Controls.Add(Me.TextBox1)
      Me.Name = "RigControl"
      Me.Text = "RigControl Simulator"
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents TextBox1 As System.Windows.Forms.TextBox
   Friend WithEvents TextBox2 As System.Windows.Forms.TextBox
   Friend WithEvents Timer1 As System.Windows.Forms.Timer
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents TextBox3 As System.Windows.Forms.TextBox
   Friend WithEvents Label3 As System.Windows.Forms.Label
End Class
