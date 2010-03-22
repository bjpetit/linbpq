<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class NEWCALL
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
        Me.OK_Button = New System.Windows.Forms.Button
        Me.Label2 = New System.Windows.Forms.Label
        Me.NodeCall = New System.Windows.Forms.TextBox
        Me.SuspendLayout()
        '
        'OK_Button
        '
        Me.OK_Button.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.OK_Button.Enabled = False
        Me.OK_Button.Location = New System.Drawing.Point(38, 105)
        Me.OK_Button.Name = "OK_Button"
        Me.OK_Button.Size = New System.Drawing.Size(75, 23)
        Me.OK_Button.TabIndex = 0
        Me.OK_Button.Text = "Next"
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(19, 18)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(53, 13)
        Me.Label2.TabIndex = 3
        Me.Label2.Text = "Node Call"
        '
        'NodeCall
        '
        Me.NodeCall.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper
        Me.NodeCall.Location = New System.Drawing.Point(95, 17)
        Me.NodeCall.Name = "NodeCall"
        Me.NodeCall.Size = New System.Drawing.Size(79, 20)
        Me.NodeCall.TabIndex = 4
        '
        'NEWCALL
        '
        Me.AcceptButton = Me.OK_Button
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(255, 154)
        Me.Controls.Add(Me.OK_Button)
        Me.Controls.Add(Me.NodeCall)
        Me.Controls.Add(Me.Label2)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "NEWCALL"
        Me.ShowInTaskbar = False
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
        Me.Text = "Enter Node Callsign"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents OK_Button As System.Windows.Forms.Button
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents NodeCall As System.Windows.Forms.TextBox

End Class
