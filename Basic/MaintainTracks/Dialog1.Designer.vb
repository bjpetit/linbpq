<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class EditTracksForm
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
        Me.Titles = New System.Windows.Forms.ListBox
        Me.EditItem = New System.Windows.Forms.TextBox
        Me.EditSave = New System.Windows.Forms.Button
        Me.ChangeDate = New System.Windows.Forms.Button
        Me.DateTimePicker1 = New System.Windows.Forms.DateTimePicker
        Me.Delete = New System.Windows.Forms.Button
        Me.Combine = New System.Windows.Forms.Button
        Me.Starttime = New System.Windows.Forms.TextBox
        Me.Endtime = New System.Windows.Forms.TextBox
        Me.NoofPoints = New System.Windows.Forms.TextBox
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
        Me.TableLayoutPanel1.Location = New System.Drawing.Point(561, 451)
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
        Me.OK_Button.Text = "OK"
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
        'Titles
        '
        Me.Titles.Font = New System.Drawing.Font("Microsoft Sans Serif", 11.25!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Titles.FormattingEnabled = True
        Me.Titles.ItemHeight = 18
        Me.Titles.Location = New System.Drawing.Point(1, 1)
        Me.Titles.Name = "Titles"
        Me.Titles.Size = New System.Drawing.Size(645, 292)
        Me.Titles.TabIndex = 1
        '
        'EditItem
        '
        Me.EditItem.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.EditItem.Location = New System.Drawing.Point(1, 299)
        Me.EditItem.Name = "EditItem"
        Me.EditItem.Size = New System.Drawing.Size(533, 26)
        Me.EditItem.TabIndex = 2
        '
        'EditSave
        '
        Me.EditSave.Location = New System.Drawing.Point(562, 324)
        Me.EditSave.Name = "EditSave"
        Me.EditSave.Size = New System.Drawing.Size(63, 26)
        Me.EditSave.TabIndex = 3
        Me.EditSave.Text = "Save"
        Me.EditSave.UseVisualStyleBackColor = True
        '
        'ChangeDate
        '
        Me.ChangeDate.Location = New System.Drawing.Point(6, 402)
        Me.ChangeDate.Name = "ChangeDate"
        Me.ChangeDate.Size = New System.Drawing.Size(84, 26)
        Me.ChangeDate.TabIndex = 4
        Me.ChangeDate.Text = "Change Date"
        Me.ChangeDate.UseVisualStyleBackColor = True
        '
        'DateTimePicker1
        '
        Me.DateTimePicker1.Location = New System.Drawing.Point(110, 405)
        Me.DateTimePicker1.Name = "DateTimePicker1"
        Me.DateTimePicker1.Size = New System.Drawing.Size(126, 20)
        Me.DateTimePicker1.TabIndex = 5
        '
        'Delete
        '
        Me.Delete.Location = New System.Drawing.Point(414, 402)
        Me.Delete.Name = "Delete"
        Me.Delete.Size = New System.Drawing.Size(55, 26)
        Me.Delete.TabIndex = 6
        Me.Delete.Text = "Delete"
        Me.Delete.UseVisualStyleBackColor = True
        '
        'Combine
        '
        Me.Combine.Location = New System.Drawing.Point(293, 402)
        Me.Combine.Name = "Combine"
        Me.Combine.Size = New System.Drawing.Size(109, 26)
        Me.Combine.TabIndex = 7
        Me.Combine.Text = "Combine with Next"
        Me.Combine.UseVisualStyleBackColor = True
        '
        'Starttime
        '
        Me.Starttime.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Starttime.Location = New System.Drawing.Point(1, 331)
        Me.Starttime.Name = "Starttime"
        Me.Starttime.Size = New System.Drawing.Size(235, 26)
        Me.Starttime.TabIndex = 8
        '
        'Endtime
        '
        Me.Endtime.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Endtime.Location = New System.Drawing.Point(224, 331)
        Me.Endtime.Name = "Endtime"
        Me.Endtime.Size = New System.Drawing.Size(270, 26)
        Me.Endtime.TabIndex = 9
        '
        'NoofPoints
        '
        Me.NoofPoints.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.NoofPoints.Location = New System.Drawing.Point(500, 331)
        Me.NoofPoints.Name = "NoofPoints"
        Me.NoofPoints.Size = New System.Drawing.Size(34, 26)
        Me.NoofPoints.TabIndex = 10
        '
        'EditTracksForm
        '
        Me.AcceptButton = Me.EditSave
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.CancelButton = Me.Cancel_Button
        Me.ClientSize = New System.Drawing.Size(719, 492)
        Me.Controls.Add(Me.NoofPoints)
        Me.Controls.Add(Me.Endtime)
        Me.Controls.Add(Me.Starttime)
        Me.Controls.Add(Me.Combine)
        Me.Controls.Add(Me.Delete)
        Me.Controls.Add(Me.DateTimePicker1)
        Me.Controls.Add(Me.ChangeDate)
        Me.Controls.Add(Me.EditSave)
        Me.Controls.Add(Me.EditItem)
        Me.Controls.Add(Me.Titles)
        Me.Controls.Add(Me.TableLayoutPanel1)
        Me.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "EditTracksForm"
        Me.ShowInTaskbar = False
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
        Me.Text = "Edit Track"
        Me.TableLayoutPanel1.ResumeLayout(False)
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents TableLayoutPanel1 As System.Windows.Forms.TableLayoutPanel
    Friend WithEvents OK_Button As System.Windows.Forms.Button
    Friend WithEvents Cancel_Button As System.Windows.Forms.Button
    Friend WithEvents Titles As System.Windows.Forms.ListBox
    Friend WithEvents EditItem As System.Windows.Forms.TextBox
    Friend WithEvents EditSave As System.Windows.Forms.Button
    Friend WithEvents ChangeDate As System.Windows.Forms.Button
    Friend WithEvents DateTimePicker1 As System.Windows.Forms.DateTimePicker
    Friend WithEvents Delete As System.Windows.Forms.Button
    Friend WithEvents Combine As System.Windows.Forms.Button
    Friend WithEvents Starttime As System.Windows.Forms.TextBox
    Friend WithEvents Endtime As System.Windows.Forms.TextBox
    Friend WithEvents NoofPoints As System.Windows.Forms.TextBox

End Class
