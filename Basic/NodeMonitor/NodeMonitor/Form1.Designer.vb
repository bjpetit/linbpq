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
      Me.Label8 = New System.Windows.Forms.Label
      Me.Timer3 = New System.Windows.Forms.Timer(Me.components)
      Me.Timer2 = New System.Windows.Forms.Timer(Me.components)
      Me.Label7 = New System.Windows.Forms.Label
      Me.ServerCount = New System.Windows.Forms.TextBox
      Me.ServerStatus = New System.Windows.Forms.TextBox
      Me.Button1 = New System.Windows.Forms.Button
      Me.NodeLastUpdated = New System.Windows.Forms.TextBox
      Me.Label6 = New System.Windows.Forms.Label
      Me.NodeActive = New System.Windows.Forms.TextBox
      Me.NodeDefined = New System.Windows.Forms.TextBox
      Me.Label4 = New System.Windows.Forms.Label
      Me.Label5 = New System.Windows.Forms.Label
      Me.ConfigToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.ConfigMonitorToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.EditNodesListToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.EditBPQ32NodesListToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.Lastupdated = New System.Windows.Forms.TextBox
      Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
      Me.MenuStrip1 = New System.Windows.Forms.MenuStrip
      Me.Label3 = New System.Windows.Forms.Label
      Me.ChatActive = New System.Windows.Forms.TextBox
      Me.ChatDefined = New System.Windows.Forms.TextBox
      Me.Label2 = New System.Windows.Forms.Label
      Me.Label1 = New System.Windows.Forms.Label
      Me.ReloadButton = New System.Windows.Forms.Button
      Me.MenuStrip1.SuspendLayout()
      Me.SuspendLayout()
      '
      'Label8
      '
      Me.Label8.AutoSize = True
      Me.Label8.Location = New System.Drawing.Point(24, 302)
      Me.Label8.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
      Me.Label8.Name = "Label8"
      Me.Label8.Size = New System.Drawing.Size(94, 17)
      Me.Label8.TabIndex = 37
      Me.Label8.Text = "Server Status"
      '
      'Timer3
      '
      '
      'Timer2
      '
      Me.Timer2.Enabled = True
      Me.Timer2.Interval = 3600000
      '
      'Label7
      '
      Me.Label7.AutoSize = True
      Me.Label7.Location = New System.Drawing.Point(24, 341)
      Me.Label7.Name = "Label7"
      Me.Label7.Size = New System.Drawing.Size(45, 17)
      Me.Label7.TabIndex = 36
      Me.Label7.Text = "Count"
      '
      'ServerCount
      '
      Me.ServerCount.Location = New System.Drawing.Point(175, 338)
      Me.ServerCount.Name = "ServerCount"
      Me.ServerCount.ReadOnly = True
      Me.ServerCount.Size = New System.Drawing.Size(70, 22)
      Me.ServerCount.TabIndex = 35
      '
      'ServerStatus
      '
      Me.ServerStatus.Location = New System.Drawing.Point(175, 299)
      Me.ServerStatus.Name = "ServerStatus"
      Me.ServerStatus.ReadOnly = True
      Me.ServerStatus.Size = New System.Drawing.Size(187, 22)
      Me.ServerStatus.TabIndex = 34
      '
      'Button1
      '
      Me.Button1.Location = New System.Drawing.Point(145, 418)
      Me.Button1.Margin = New System.Windows.Forms.Padding(4)
      Me.Button1.Name = "Button1"
      Me.Button1.Size = New System.Drawing.Size(100, 28)
      Me.Button1.TabIndex = 33
      Me.Button1.Text = "Button1"
      Me.Button1.UseVisualStyleBackColor = True
      '
      'NodeLastUpdated
      '
      Me.NodeLastUpdated.Location = New System.Drawing.Point(175, 260)
      Me.NodeLastUpdated.Margin = New System.Windows.Forms.Padding(4)
      Me.NodeLastUpdated.Name = "NodeLastUpdated"
      Me.NodeLastUpdated.Size = New System.Drawing.Size(177, 22)
      Me.NodeLastUpdated.TabIndex = 32
      '
      'Label6
      '
      Me.Label6.AutoSize = True
      Me.Label6.Location = New System.Drawing.Point(24, 263)
      Me.Label6.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
      Me.Label6.Name = "Label6"
      Me.Label6.Size = New System.Drawing.Size(93, 17)
      Me.Label6.TabIndex = 31
      Me.Label6.Text = "Last Updated"
      '
      'NodeActive
      '
      Me.NodeActive.Location = New System.Drawing.Point(175, 221)
      Me.NodeActive.Margin = New System.Windows.Forms.Padding(4)
      Me.NodeActive.Name = "NodeActive"
      Me.NodeActive.Size = New System.Drawing.Size(47, 22)
      Me.NodeActive.TabIndex = 30
      '
      'NodeDefined
      '
      Me.NodeDefined.Location = New System.Drawing.Point(175, 182)
      Me.NodeDefined.Margin = New System.Windows.Forms.Padding(4)
      Me.NodeDefined.Name = "NodeDefined"
      Me.NodeDefined.Size = New System.Drawing.Size(45, 22)
      Me.NodeDefined.TabIndex = 29
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(24, 224)
      Me.Label4.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
      Me.Label4.Name = "Label4"
      Me.Label4.Size = New System.Drawing.Size(95, 17)
      Me.Label4.TabIndex = 28
      Me.Label4.Text = "Active  Nodes"
      '
      'Label5
      '
      Me.Label5.AutoSize = True
      Me.Label5.Location = New System.Drawing.Point(24, 185)
      Me.Label5.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
      Me.Label5.Name = "Label5"
      Me.Label5.Size = New System.Drawing.Size(102, 17)
      Me.Label5.TabIndex = 27
      Me.Label5.Text = "Defined Nodes"
      '
      'ConfigToolStripMenuItem
      '
      Me.ConfigToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ConfigMonitorToolStripMenuItem, Me.EditNodesListToolStripMenuItem, Me.EditBPQ32NodesListToolStripMenuItem})
      Me.ConfigToolStripMenuItem.Name = "ConfigToolStripMenuItem"
      Me.ConfigToolStripMenuItem.Size = New System.Drawing.Size(65, 24)
      Me.ConfigToolStripMenuItem.Text = "Config"
      '
      'ConfigMonitorToolStripMenuItem
      '
      Me.ConfigMonitorToolStripMenuItem.Name = "ConfigMonitorToolStripMenuItem"
      Me.ConfigMonitorToolStripMenuItem.Size = New System.Drawing.Size(225, 24)
      Me.ConfigMonitorToolStripMenuItem.Text = "Config Monitor"
      '
      'EditNodesListToolStripMenuItem
      '
      Me.EditNodesListToolStripMenuItem.Name = "EditNodesListToolStripMenuItem"
      Me.EditNodesListToolStripMenuItem.Size = New System.Drawing.Size(225, 24)
      Me.EditNodesListToolStripMenuItem.Text = "Edit Chat Nodes List"
      '
      'EditBPQ32NodesListToolStripMenuItem
      '
      Me.EditBPQ32NodesListToolStripMenuItem.Name = "EditBPQ32NodesListToolStripMenuItem"
      Me.EditBPQ32NodesListToolStripMenuItem.Size = New System.Drawing.Size(225, 24)
      Me.EditBPQ32NodesListToolStripMenuItem.Text = "Edit BPQ32 Nodes List"
      '
      'Lastupdated
      '
      Me.Lastupdated.Location = New System.Drawing.Point(175, 143)
      Me.Lastupdated.Margin = New System.Windows.Forms.Padding(4)
      Me.Lastupdated.Name = "Lastupdated"
      Me.Lastupdated.Size = New System.Drawing.Size(177, 22)
      Me.Lastupdated.TabIndex = 26
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
      Me.MenuStrip1.Padding = New System.Windows.Forms.Padding(8, 2, 0, 2)
      Me.MenuStrip1.Size = New System.Drawing.Size(389, 28)
      Me.MenuStrip1.TabIndex = 20
      Me.MenuStrip1.Text = "MenuStrip1"
      '
      'Label3
      '
      Me.Label3.AutoSize = True
      Me.Label3.Location = New System.Drawing.Point(24, 147)
      Me.Label3.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
      Me.Label3.Name = "Label3"
      Me.Label3.Size = New System.Drawing.Size(93, 17)
      Me.Label3.TabIndex = 25
      Me.Label3.Text = "Last Updated"
      '
      'ChatActive
      '
      Me.ChatActive.Location = New System.Drawing.Point(175, 104)
      Me.ChatActive.Margin = New System.Windows.Forms.Padding(4)
      Me.ChatActive.Name = "ChatActive"
      Me.ChatActive.Size = New System.Drawing.Size(47, 22)
      Me.ChatActive.TabIndex = 24
      '
      'ChatDefined
      '
      Me.ChatDefined.Location = New System.Drawing.Point(175, 65)
      Me.ChatDefined.Margin = New System.Windows.Forms.Padding(4)
      Me.ChatDefined.Name = "ChatDefined"
      Me.ChatDefined.Size = New System.Drawing.Size(45, 22)
      Me.ChatDefined.TabIndex = 23
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(24, 109)
      Me.Label2.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(124, 17)
      Me.Label2.TabIndex = 22
      Me.Label2.Text = "Active Chat Nodes"
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(24, 68)
      Me.Label1.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(135, 17)
      Me.Label1.TabIndex = 21
      Me.Label1.Text = "Defined Chat Nodes"
      '
      'ReloadButton
      '
      Me.ReloadButton.Location = New System.Drawing.Point(128, 378)
      Me.ReloadButton.Margin = New System.Windows.Forms.Padding(4)
      Me.ReloadButton.Name = "ReloadButton"
      Me.ReloadButton.Size = New System.Drawing.Size(133, 33)
      Me.ReloadButton.TabIndex = 19
      Me.ReloadButton.Text = "Reload Nodes"
      Me.ReloadButton.UseVisualStyleBackColor = True
      '
      'Form1
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(8.0!, 16.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(389, 452)
      Me.Controls.Add(Me.Label8)
      Me.Controls.Add(Me.Label7)
      Me.Controls.Add(Me.ServerCount)
      Me.Controls.Add(Me.ServerStatus)
      Me.Controls.Add(Me.Button1)
      Me.Controls.Add(Me.NodeLastUpdated)
      Me.Controls.Add(Me.Label6)
      Me.Controls.Add(Me.NodeActive)
      Me.Controls.Add(Me.NodeDefined)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.Label5)
      Me.Controls.Add(Me.Lastupdated)
      Me.Controls.Add(Me.MenuStrip1)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.ChatActive)
      Me.Controls.Add(Me.ChatDefined)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.ReloadButton)
      Me.Margin = New System.Windows.Forms.Padding(4)
      Me.Name = "Form1"
      Me.Text = "NodeMonitor"
      Me.MenuStrip1.ResumeLayout(False)
      Me.MenuStrip1.PerformLayout()
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents Timer3 As System.Windows.Forms.Timer
    Friend WithEvents Timer2 As System.Windows.Forms.Timer
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents ServerCount As System.Windows.Forms.TextBox
    Friend WithEvents ServerStatus As System.Windows.Forms.TextBox
    Friend WithEvents Button1 As System.Windows.Forms.Button
    Friend WithEvents NodeLastUpdated As System.Windows.Forms.TextBox
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents NodeActive As System.Windows.Forms.TextBox
    Friend WithEvents NodeDefined As System.Windows.Forms.TextBox
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents ConfigToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents ConfigMonitorToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents EditNodesListToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents EditBPQ32NodesListToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents Lastupdated As System.Windows.Forms.TextBox
    Friend WithEvents Timer1 As System.Windows.Forms.Timer
    Friend WithEvents MenuStrip1 As System.Windows.Forms.MenuStrip
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents ChatActive As System.Windows.Forms.TextBox
    Friend WithEvents ChatDefined As System.Windows.Forms.TextBox
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents ReloadButton As System.Windows.Forms.Button

End Class
