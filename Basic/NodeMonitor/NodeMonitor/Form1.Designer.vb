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
      Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(Form1))
      Me.ReloadButton = New System.Windows.Forms.Button
      Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
      Me.MenuStrip1 = New System.Windows.Forms.MenuStrip
      Me.ConfigToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.ConfigMonitorToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.EditNodesListToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.EditBPQ32NodesListToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem
      Me.Label1 = New System.Windows.Forms.Label
      Me.Label2 = New System.Windows.Forms.Label
      Me.ChatDefined = New System.Windows.Forms.TextBox
      Me.ChatActive = New System.Windows.Forms.TextBox
      Me.Label3 = New System.Windows.Forms.Label
      Me.Lastupdated = New System.Windows.Forms.TextBox
      Me.Timer2 = New System.Windows.Forms.Timer(Me.components)
      Me.Timer3 = New System.Windows.Forms.Timer(Me.components)
      Me.NodeActive = New System.Windows.Forms.TextBox
      Me.NodeDefined = New System.Windows.Forms.TextBox
      Me.Label4 = New System.Windows.Forms.Label
      Me.Label5 = New System.Windows.Forms.Label
      Me.NodeLastUpdated = New System.Windows.Forms.TextBox
      Me.Label6 = New System.Windows.Forms.Label
      Me.Button1 = New System.Windows.Forms.Button
      Me.AxBPQCtrl2 = New AxBPQCTRLLib.AxBPQCtrl
      Me.MenuStrip1.SuspendLayout()
      CType(Me.AxBPQCtrl2, System.ComponentModel.ISupportInitialize).BeginInit()
      Me.SuspendLayout()
      '
      'ReloadButton
      '
      Me.ReloadButton.Location = New System.Drawing.Point(96, 302)
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
      Me.MenuStrip1.Size = New System.Drawing.Size(292, 24)
      Me.MenuStrip1.TabIndex = 1
      Me.MenuStrip1.Text = "MenuStrip1"
      '
      'ConfigToolStripMenuItem
      '
      Me.ConfigToolStripMenuItem.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ConfigMonitorToolStripMenuItem, Me.EditNodesListToolStripMenuItem, Me.EditBPQ32NodesListToolStripMenuItem})
      Me.ConfigToolStripMenuItem.Name = "ConfigToolStripMenuItem"
      Me.ConfigToolStripMenuItem.Size = New System.Drawing.Size(50, 20)
      Me.ConfigToolStripMenuItem.Text = "Config"
      '
      'ConfigMonitorToolStripMenuItem
      '
      Me.ConfigMonitorToolStripMenuItem.Name = "ConfigMonitorToolStripMenuItem"
      Me.ConfigMonitorToolStripMenuItem.Size = New System.Drawing.Size(179, 22)
      Me.ConfigMonitorToolStripMenuItem.Text = "Config Monitor"
      '
      'EditNodesListToolStripMenuItem
      '
      Me.EditNodesListToolStripMenuItem.Name = "EditNodesListToolStripMenuItem"
      Me.EditNodesListToolStripMenuItem.Size = New System.Drawing.Size(179, 22)
      Me.EditNodesListToolStripMenuItem.Text = "Edit Chat Nodes List"
      '
      'EditBPQ32NodesListToolStripMenuItem
      '
      Me.EditBPQ32NodesListToolStripMenuItem.Name = "EditBPQ32NodesListToolStripMenuItem"
      Me.EditBPQ32NodesListToolStripMenuItem.Size = New System.Drawing.Size(179, 22)
      Me.EditBPQ32NodesListToolStripMenuItem.Text = "Edit BPQ32 Nodes List"
      '
      'Label1
      '
      Me.Label1.AutoSize = True
      Me.Label1.Location = New System.Drawing.Point(12, 51)
      Me.Label1.Name = "Label1"
      Me.Label1.Size = New System.Drawing.Size(103, 13)
      Me.Label1.TabIndex = 2
      Me.Label1.Text = "Defined Chat Nodes"
      '
      'Label2
      '
      Me.Label2.AutoSize = True
      Me.Label2.Location = New System.Drawing.Point(12, 91)
      Me.Label2.Name = "Label2"
      Me.Label2.Size = New System.Drawing.Size(96, 13)
      Me.Label2.TabIndex = 3
      Me.Label2.Text = "Active Chat Nodes"
      '
      'ChatDefined
      '
      Me.ChatDefined.Location = New System.Drawing.Point(131, 48)
      Me.ChatDefined.Name = "ChatDefined"
      Me.ChatDefined.Size = New System.Drawing.Size(35, 20)
      Me.ChatDefined.TabIndex = 4
      '
      'ChatActive
      '
      Me.ChatActive.Location = New System.Drawing.Point(131, 88)
      Me.ChatActive.Name = "ChatActive"
      Me.ChatActive.Size = New System.Drawing.Size(36, 20)
      Me.ChatActive.TabIndex = 5
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
      Me.Lastupdated.Location = New System.Drawing.Point(131, 128)
      Me.Lastupdated.Name = "Lastupdated"
      Me.Lastupdated.Size = New System.Drawing.Size(134, 20)
      Me.Lastupdated.TabIndex = 7
      '
      'Timer2
      '
      Me.Timer2.Enabled = True
      Me.Timer2.Interval = 3600000
      '
      'Timer3
      '
      '
      'NodeActive
      '
      Me.NodeActive.Location = New System.Drawing.Point(131, 208)
      Me.NodeActive.Name = "NodeActive"
      Me.NodeActive.Size = New System.Drawing.Size(36, 20)
      Me.NodeActive.TabIndex = 11
      '
      'NodeDefined
      '
      Me.NodeDefined.Location = New System.Drawing.Point(131, 168)
      Me.NodeDefined.Name = "NodeDefined"
      Me.NodeDefined.Size = New System.Drawing.Size(35, 20)
      Me.NodeDefined.TabIndex = 10
      '
      'Label4
      '
      Me.Label4.AutoSize = True
      Me.Label4.Location = New System.Drawing.Point(12, 211)
      Me.Label4.Name = "Label4"
      Me.Label4.Size = New System.Drawing.Size(74, 13)
      Me.Label4.TabIndex = 9
      Me.Label4.Text = "Active  Nodes"
      '
      'Label5
      '
      Me.Label5.AutoSize = True
      Me.Label5.Location = New System.Drawing.Point(12, 171)
      Me.Label5.Name = "Label5"
      Me.Label5.Size = New System.Drawing.Size(78, 13)
      Me.Label5.TabIndex = 8
      Me.Label5.Text = "Defined Nodes"
      '
      'NodeLastUpdated
      '
      Me.NodeLastUpdated.Location = New System.Drawing.Point(129, 248)
      Me.NodeLastUpdated.Name = "NodeLastUpdated"
      Me.NodeLastUpdated.Size = New System.Drawing.Size(134, 20)
      Me.NodeLastUpdated.TabIndex = 13
      '
      'Label6
      '
      Me.Label6.AutoSize = True
      Me.Label6.Location = New System.Drawing.Point(10, 251)
      Me.Label6.Name = "Label6"
      Me.Label6.Size = New System.Drawing.Size(71, 13)
      Me.Label6.TabIndex = 12
      Me.Label6.Text = "Last Updated"
      '
      'Button1
      '
      Me.Button1.Location = New System.Drawing.Point(109, 335)
      Me.Button1.Name = "Button1"
      Me.Button1.Size = New System.Drawing.Size(75, 23)
      Me.Button1.TabIndex = 14
      Me.Button1.Text = "Button1"
      Me.Button1.UseVisualStyleBackColor = True
      '
      'AxBPQCtrl2
      '
      Me.AxBPQCtrl2.Enabled = True
      Me.AxBPQCtrl2.Location = New System.Drawing.Point(215, 182)
      Me.AxBPQCtrl2.Name = "AxBPQCtrl2"
      Me.AxBPQCtrl2.OcxState = CType(resources.GetObject("AxBPQCtrl2.OcxState"), System.Windows.Forms.AxHost.State)
      Me.AxBPQCtrl2.Size = New System.Drawing.Size(100, 50)
      Me.AxBPQCtrl2.TabIndex = 15
      Me.AxBPQCtrl2.Visible = False
      '
      'Form1
      '
      Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
      Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
      Me.ClientSize = New System.Drawing.Size(292, 367)
      Me.Controls.Add(Me.AxBPQCtrl2)
      Me.Controls.Add(Me.Button1)
      Me.Controls.Add(Me.NodeLastUpdated)
      Me.Controls.Add(Me.Label6)
      Me.Controls.Add(Me.NodeActive)
      Me.Controls.Add(Me.NodeDefined)
      Me.Controls.Add(Me.Label4)
      Me.Controls.Add(Me.Label5)
      Me.Controls.Add(Me.Lastupdated)
      Me.Controls.Add(Me.Label3)
      Me.Controls.Add(Me.ChatActive)
      Me.Controls.Add(Me.ChatDefined)
      Me.Controls.Add(Me.Label2)
      Me.Controls.Add(Me.Label1)
      Me.Controls.Add(Me.ReloadButton)
      Me.Controls.Add(Me.MenuStrip1)
      Me.MainMenuStrip = Me.MenuStrip1
      Me.Name = "Form1"
      Me.Text = "NodeMonitor"
      Me.MenuStrip1.ResumeLayout(False)
      Me.MenuStrip1.PerformLayout()
      CType(Me.AxBPQCtrl2, System.ComponentModel.ISupportInitialize).EndInit()
      Me.ResumeLayout(False)
      Me.PerformLayout()

   End Sub
   Friend WithEvents ReloadButton As System.Windows.Forms.Button
   Friend WithEvents Timer1 As System.Windows.Forms.Timer
   Friend WithEvents MenuStrip1 As System.Windows.Forms.MenuStrip
   Friend WithEvents ConfigToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents Label2 As System.Windows.Forms.Label
   Friend WithEvents ChatDefined As System.Windows.Forms.TextBox
   Friend WithEvents ChatActive As System.Windows.Forms.TextBox
   Friend WithEvents Label3 As System.Windows.Forms.Label
   Friend WithEvents Lastupdated As System.Windows.Forms.TextBox
   Friend WithEvents ConfigMonitorToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
   Friend WithEvents EditNodesListToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
   Friend WithEvents Timer2 As System.Windows.Forms.Timer
   Friend WithEvents Timer3 As System.Windows.Forms.Timer
   Friend WithEvents NodeActive As System.Windows.Forms.TextBox
   Friend WithEvents NodeDefined As System.Windows.Forms.TextBox
   Friend WithEvents Label4 As System.Windows.Forms.Label
   Friend WithEvents Label5 As System.Windows.Forms.Label
   Friend WithEvents NodeLastUpdated As System.Windows.Forms.TextBox
   Friend WithEvents Label6 As System.Windows.Forms.Label
   Friend WithEvents Button1 As System.Windows.Forms.Button
   Friend WithEvents EditBPQ32NodesListToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
   Friend WithEvents AxBPQCtrl2 As AxBPQCTRLLib.AxBPQCtrl

End Class
