Public Class About
   Inherits System.Windows.Forms.Form

#Region " Windows Form Designer generated code "

   Public Sub New()
      MyBase.New()

      'This call is required by the Windows Form Designer.
      InitializeComponent()

      'Add any initialization after the InitializeComponent() call

   End Sub

   'Form overrides dispose to clean up the component list.
   Protected Overloads Overrides Sub Dispose(ByVal disposing As Boolean)
      If disposing Then
         If Not (components Is Nothing) Then
            components.Dispose()
         End If
      End If
      MyBase.Dispose(disposing)
   End Sub

   'Required by the Windows Form Designer
   Private components As System.ComponentModel.IContainer

   'NOTE: The following procedure is required by the Windows Form Designer
   'It can be modified using the Windows Form Designer.  
   'Do not modify it using the code editor.
   Public WithEvents lblDescription As System.Windows.Forms.Label
   Public WithEvents lblCopyright As System.Windows.Forms.Label
   Friend WithEvents Label1 As System.Windows.Forms.Label
   Friend WithEvents PictureBox1 As System.Windows.Forms.PictureBox
    Friend WithEvents lnkARSFI As System.Windows.Forms.LinkLabel
    Friend WithEvents LinkARDOP As System.Windows.Forms.LinkLabel
    Public WithEvents lblVersion As System.Windows.Forms.Label
    <System.Diagnostics.DebuggerStepThrough()> Private Sub InitializeComponent()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(About))
        Me.lblDescription = New System.Windows.Forms.Label()
        Me.lblCopyright = New System.Windows.Forms.Label()
        Me.lblVersion = New System.Windows.Forms.Label()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.PictureBox1 = New System.Windows.Forms.PictureBox()
        Me.lnkARSFI = New System.Windows.Forms.LinkLabel()
        Me.LinkARDOP = New System.Windows.Forms.LinkLabel()
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'lblDescription
        '
        Me.lblDescription.Location = New System.Drawing.Point(12, 53)
        Me.lblDescription.Name = "lblDescription"
        Me.lblDescription.Size = New System.Drawing.Size(356, 39)
        Me.lblDescription.TabIndex = 8
        Me.lblDescription.Text = "ARDOP_Win is a Windows virtual TNC implementation of the Amateur Radio Digital Op" & _
    "en Protocol.   For more information visit:"
        Me.lblDescription.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'lblCopyright
        '
        Me.lblCopyright.Location = New System.Drawing.Point(65, 113)
        Me.lblCopyright.Name = "lblCopyright"
        Me.lblCopyright.Size = New System.Drawing.Size(232, 23)
        Me.lblCopyright.TabIndex = 6
        Me.lblCopyright.Text = " Copyright 2015 Rick Muething, KN6KB   "
        Me.lblCopyright.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'lblVersion
        '
        Me.lblVersion.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle
        Me.lblVersion.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lblVersion.Location = New System.Drawing.Point(105, 9)
        Me.lblVersion.Name = "lblVersion"
        Me.lblVersion.Size = New System.Drawing.Size(176, 24)
        Me.lblVersion.TabIndex = 5
        Me.lblVersion.Text = "Version: 0.0.0.0"
        Me.lblVersion.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'Label1
        '
        Me.Label1.Location = New System.Drawing.Point(54, 136)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(269, 47)
        Me.Label1.TabIndex = 9
        Me.Label1.Text = "ARDOP_Win is made possible through the Amateur Radio Safety Foundation Inc.  Your" & _
    " support for the ARSF make programs like ARDOP_Win possible. "
        '
        'PictureBox1
        '
        Me.PictureBox1.ErrorImage = CType(resources.GetObject("PictureBox1.ErrorImage"), System.Drawing.Image)
        Me.PictureBox1.Image = CType(resources.GetObject("PictureBox1.Image"), System.Drawing.Image)
        Me.PictureBox1.Location = New System.Drawing.Point(35, 204)
        Me.PictureBox1.Name = "PictureBox1"
        Me.PictureBox1.Size = New System.Drawing.Size(321, 303)
        Me.PictureBox1.TabIndex = 10
        Me.PictureBox1.TabStop = False
        '
        'lnkARSFI
        '
        Me.lnkARSFI.AutoSize = True
        Me.lnkARSFI.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.lnkARSFI.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.lnkARSFI.Location = New System.Drawing.Point(133, 183)
        Me.lnkARSFI.Name = "lnkARSFI"
        Me.lnkARSFI.Size = New System.Drawing.Size(120, 18)
        Me.lnkARSFI.TabIndex = 12
        Me.lnkARSFI.TabStop = True
        Me.lnkARSFI.Text = "http://www.arsfi.org"
        '
        'LinkARDOP
        '
        Me.LinkARDOP.AutoSize = True
        Me.LinkARDOP.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.LinkARDOP.Font = New System.Drawing.Font("Microsoft Sans Serif", 9.75!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.LinkARDOP.Location = New System.Drawing.Point(15, 92)
        Me.LinkARDOP.Name = "LinkARDOP"
        Me.LinkARDOP.Size = New System.Drawing.Size(345, 18)
        Me.LinkARDOP.TabIndex = 13
        Me.LinkARDOP.TabStop = True
        Me.LinkARDOP.Text = "  https://groups.yahoo.com/neo/groups/ardop_users/info"
        '
        'About
        '
        Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
        Me.AutoSize = True
        Me.ClientSize = New System.Drawing.Size(380, 507)
        Me.Controls.Add(Me.LinkARDOP)
        Me.Controls.Add(Me.lnkARSFI)
        Me.Controls.Add(Me.PictureBox1)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.lblDescription)
        Me.Controls.Add(Me.lblCopyright)
        Me.Controls.Add(Me.lblVersion)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "About"
        Me.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
        Me.Text = " About ARDOP_Win virtual TNC"
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub

#End Region

    ' Shows about box for this application...
    Private Sub About_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Text = "About " & Application.ProductName
        lblVersion.Text = "Version: " & Application.ProductVersion
        Label1.Enabled = True
    End Sub ' About_Load

    ' Close the about box...
    Private Sub btnClose_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
        Close()
    End Sub ' btnClose_Click

    Private Sub lnkARSFI_LinkClicked(ByVal sender As System.Object, ByVal e As System.Windows.Forms.LinkLabelLinkClickedEventArgs) Handles lnkARSFI.LinkClicked
        System.Diagnostics.Process.Start(lnkARSFI.Text)
    End Sub ' lnkARSFI_LinkClicked

    Private Sub LinkARDOP_LinkClicked(sender As System.Object, e As System.Windows.Forms.LinkLabelLinkClickedEventArgs) Handles LinkARDOP.LinkClicked
        System.Diagnostics.Process.Start(LinkARDOP.Text)
    End Sub
End Class
