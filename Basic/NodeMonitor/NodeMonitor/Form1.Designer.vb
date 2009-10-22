<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Form1
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
      Me.ReloadButton = New System.Windows.Forms.Button
      Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
      Me.MenuStrip1 = New System.Windows.Forms.MenuStrip
      Me.ConfigToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.Label1 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.Defined = New System.Windows.Forms.TextBox
      Me.Active = New System.Windows.Forms.TextBox
      Me.Label3 = New System.Windows.Forms.Label
      Me.Lastupdated = New System.Windows.Forms.TextBox
      Me.ConfigMonitorToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.EditNodesListToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.MenuStrip1.SuspendLayout()
      Me.SuspendLayout()
      '
      'ReloadButton
      '
      Me.ReloadButton.Location = New System.Drawing.Point(96, 227)
      Me.ReloadButton.Name = "ReloadButton"
      Me.ReloadButton.Size = New System.Drawing.Size(100, 27)
      Me.ReloadButton.TabIndex = 0
      Me.ReloadButton.Text = "Reload Nodes"
      Me.ReloadButton.UseVisualStyleBackColor = True
      '
      'Timer1
      '
      Me.Timer1.Enabled = True
      Me.Timer1.Interval = 60000
      '
      'MenuStrip1
      '
      Me.MenuStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ConfigToolStripMenuItem})
      Me.MenuStrip1.Location = New System.Drawing.Point(0, 0)
      Me.MenuStrip1.Name = "MenuStrip1"
      Me.MenuStrip1.Size = New System.Drawing.Size(292, 27)
      Me.MenuStrip1.TabIndex = 1
      Me.MenuStrip1.Text = "MenuStrip1"
      '
      'ConfigToolStripMenuItem
      '
      Me.ConfigToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ConfigMonitorToolStripMenuItem, Me.EditNodesListToolStripMenuItem})
      Me.ConfigToolStripMenuItem.Name = "ConfigToolStripMenuItem"
      Me.ConfigToolStripMenuItem.Size = New System.Drawing.Size(67, 23)
      Me.ConfigToolStripMenuItem.Text = "Config"
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(12, 51)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(78, 13)
      Me.Label1.TabIndex = 2
      Me.Label1.Text = "Defined Nodes"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(12, 91)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(71, 13)
      Me.Label2.TabIndex = 3
      Me.Label2.Text = "Active Nodes"
      '
      'Defined
      '
      Me.Defined.Location = New System.Drawing.Point(94, 48)
      Me.Defined.Name = "Defined"
      Me.Defined.Size = New System.Drawing.Size(35, 20)
      Me.Defined.TabIndex = 4
      '
      'Active
      '
      Me.Active.Location = New System.Drawing.Point(94, 88)
      Me.Active.Name = "Active"
      Me.Active.Size = New System.Drawing.Size(36, 20)
      Me.Active.TabIndex = 5
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(12, 131)
      Me.Label3.Name = "Label3"
      Me.Label3.Size = New System.Drawing.Size(71, 13)
      Me.Label3.TabIndex = 6
      Me.Label3.Text = "Last Updated"
      '
      'Lastupdated
      '
      Me.Lastupdated.Location = New System.Drawing.Point(94, 128)
      Me.Lastupdated.Name = "Lastupdated"
      Me.Lastupdated.Size = New System.Drawing.Size(134, 20)
      Me.Lastupdated.TabIndex = 7
      '
      'ConfigMonitorToolStripMenuItem
      '
      Me.ConfigMonitorToolStripMenuItem.Name = "ConfigMonitorToolStripMenuItem"
      Me.ConfigMonitorToolStripMenuItem.Size = New System.Drawing.Size(199, 24)
      Me.ConfigMonitorToolStripMenuItem.Text = "Config Monitor"
      '
      'EditNodesListToolStripMenuItem
      '
      Me.EditNodesListToolStripMenuItem.Name = "EditNodesListToolStripMenuItem"
      Me.EditNodesListToolStripMenuItem.Size = New System.Drawing.Size(199, 24)
      Me.EditNodesListToolStripMenuItem.Text = "Edit Nodes List"
      '
      'Form1
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(292, 266)
      Me.Controls.Add(Me.Lastupdated)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.Active)
      Me.Controls.Add(Me.Defined)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.ReloadButton)
      Me.Controls.Add(Me.MenuStrip1)
      Me.MainMenuStrip = Me.MenuStrip1
      Me.Name = "Form1"
      Me.Text = "NodeMonitor"
      Me.MenuStrip1.ResumeLayout(False)
      Me.MenuStrip1.PerformLayout()
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents ReloadButton As System.Windows.Forms.Button
   Friend WithEvents Timer1 As System.Windows.Forms.Timer
   Friend WithEvents MenuStrip1 As System.Windows.Forms.MenuStrip
   Friend WithEvents ConfigToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents Defined As System.Windows.Forms.TextBox
   Friend WithEvents Active As System.Windows.Forms.TextBox
   Friend WithEvents Label3 As System.Windows.Forms.Label
   Friend WithEvents Lastupdated As System.Windows.Forms.TextBox
   Friend WithEvents ConfigMonitorToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
   Friend WithEvents EditNodesListToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem

End Class
