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
        Me.OpenTLOG = New System.Windows.Forms.Button
        Me.RemoveDups = New System.Windows.Forms.Button
        Me.CreateV3TLOG = New System.Windows.Forms.Button
        Me.EditTitles = New System.Windows.Forms.Button
        Me.SortButton = New System.Windows.Forms.Button
        Me.CreateV5CSV = New System.Windows.Forms.Button
        Me.Archive = New System.Windows.Forms.Button
        Me.LoadArchive = New System.Windows.Forms.Button
        Me.DateStamp = New System.Windows.Forms.Button
        Me.Read_CSV = New System.Windows.Forms.Button
        Me.SuspendLayout()
        '
        'OpenTLOG
        '
        Me.OpenTLOG.Location = New System.Drawing.Point(13, 64)
        Me.OpenTLOG.Name = "OpenTLOG"
        Me.OpenTLOG.Size = New System.Drawing.Size(110, 28)
        Me.OpenTLOG.TabIndex = 0
        Me.OpenTLOG.Text = "Read TLOG"
        Me.OpenTLOG.UseVisualStyleBackColor = True
        '
        'RemoveDups
        '
        Me.RemoveDups.Location = New System.Drawing.Point(149, 12)
        Me.RemoveDups.Name = "RemoveDups"
        Me.RemoveDups.Size = New System.Drawing.Size(110, 28)
        Me.RemoveDups.TabIndex = 1
        Me.RemoveDups.Text = "Remove Dups"
        Me.RemoveDups.UseVisualStyleBackColor = True
        '
        'CreateV3TLOG
        '
        Me.CreateV3TLOG.Location = New System.Drawing.Point(13, 116)
        Me.CreateV3TLOG.Name = "CreateV3TLOG"
        Me.CreateV3TLOG.Size = New System.Drawing.Size(110, 28)
        Me.CreateV3TLOG.TabIndex = 3
        Me.CreateV3TLOG.Text = "Create V3 TLOG"
        Me.CreateV3TLOG.UseVisualStyleBackColor = True
        '
        'EditTitles
        '
        Me.EditTitles.Location = New System.Drawing.Point(149, 116)
        Me.EditTitles.Name = "EditTitles"
        Me.EditTitles.Size = New System.Drawing.Size(110, 28)
        Me.EditTitles.TabIndex = 4
        Me.EditTitles.Text = "Edit Titles"
        Me.EditTitles.UseVisualStyleBackColor = True
        '
        'SortButton
        '
        Me.SortButton.Location = New System.Drawing.Point(149, 64)
        Me.SortButton.Name = "SortButton"
        Me.SortButton.Size = New System.Drawing.Size(110, 28)
        Me.SortButton.TabIndex = 5
        Me.SortButton.Text = "Sort"
        Me.SortButton.UseVisualStyleBackColor = True
        '
        'CreateV5CSV
        '
        Me.CreateV5CSV.Location = New System.Drawing.Point(13, 170)
        Me.CreateV5CSV.Name = "CreateV5CSV"
        Me.CreateV5CSV.Size = New System.Drawing.Size(110, 28)
        Me.CreateV5CSV.TabIndex = 6
        Me.CreateV5CSV.Text = "Create V5 CSV"
        Me.CreateV5CSV.UseVisualStyleBackColor = True
        '
        'Archive
        '
        Me.Archive.Location = New System.Drawing.Point(12, 271)
        Me.Archive.Name = "Archive"
        Me.Archive.Size = New System.Drawing.Size(109, 27)
        Me.Archive.TabIndex = 7
        Me.Archive.Text = "Archive Tracks"
        Me.Archive.UseVisualStyleBackColor = True
        '
        'LoadArchive
        '
        Me.LoadArchive.Location = New System.Drawing.Point(13, 13)
        Me.LoadArchive.Name = "LoadArchive"
        Me.LoadArchive.Size = New System.Drawing.Size(109, 27)
        Me.LoadArchive.TabIndex = 8
        Me.LoadArchive.Text = "Load Archive"
        Me.LoadArchive.UseVisualStyleBackColor = True
        '
        'DateStamp
        '
        Me.DateStamp.Location = New System.Drawing.Point(149, 170)
        Me.DateStamp.Name = "DateStamp"
        Me.DateStamp.Size = New System.Drawing.Size(110, 28)
        Me.DateStamp.TabIndex = 9
        Me.DateStamp.Text = "Datestamp Tracks"
        Me.DateStamp.UseVisualStyleBackColor = True
        '
        'Read_CSV
        '
        Me.Read_CSV.Location = New System.Drawing.Point(13, 220)
        Me.Read_CSV.Name = "Read_CSV"
        Me.Read_CSV.Size = New System.Drawing.Size(110, 28)
        Me.Read_CSV.TabIndex = 10
        Me.Read_CSV.Text = "Read CSV"
        Me.Read_CSV.UseVisualStyleBackColor = True
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(278, 325)
        Me.Controls.Add(Me.Read_CSV)
        Me.Controls.Add(Me.DateStamp)
        Me.Controls.Add(Me.LoadArchive)
        Me.Controls.Add(Me.Archive)
        Me.Controls.Add(Me.CreateV5CSV)
        Me.Controls.Add(Me.SortButton)
        Me.Controls.Add(Me.EditTitles)
        Me.Controls.Add(Me.CreateV3TLOG)
        Me.Controls.Add(Me.RemoveDups)
        Me.Controls.Add(Me.OpenTLOG)
        Me.Name = "Form1"
        Me.Text = "Maintain GPS Tracks"
        Me.ResumeLayout(False)

    End Sub
    Friend WithEvents OpenTLOG As System.Windows.Forms.Button
    Friend WithEvents RemoveDups As System.Windows.Forms.Button
    Friend WithEvents CreateV3TLOG As System.Windows.Forms.Button
    Friend WithEvents EditTitles As System.Windows.Forms.Button
    Friend WithEvents SortButton As System.Windows.Forms.Button
    Friend WithEvents CreateV5CSV As System.Windows.Forms.Button
    Friend WithEvents Archive As System.Windows.Forms.Button
    Protected WithEvents LoadArchive As System.Windows.Forms.Button
    Friend WithEvents DateStamp As System.Windows.Forms.Button
    Friend WithEvents Read_CSV As System.Windows.Forms.Button

End Class
