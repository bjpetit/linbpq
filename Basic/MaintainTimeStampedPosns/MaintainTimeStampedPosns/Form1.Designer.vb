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
        Me.ProcMyToday = New System.Windows.Forms.Button
        Me.SaveBinary = New System.Windows.Forms.Button
        Me.Label1 = New System.Windows.Forms.Label
        Me.Label2 = New System.Windows.Forms.Label
        Me.Label3 = New System.Windows.Forms.Label
        Me.RecsBox = New System.Windows.Forms.TextBox
        Me.FirstBox = New System.Windows.Forms.TextBox
        Me.LastBox = New System.Windows.Forms.TextBox
        Me.ProcessSorted = New System.Windows.Forms.Button
        Me.ReadBinary = New System.Windows.Forms.Button
        Me.ReadBPQAISLog = New System.Windows.Forms.Button
        Me.CheckOrder = New System.Windows.Forms.Button
        Me.EnableGE = New System.Windows.Forms.CheckBox
        Me.MonthCalendar1 = New System.Windows.Forms.MonthCalendar
        Me.SortButton = New System.Windows.Forms.Button
        Me.ReadMMCSV = New System.Windows.Forms.Button
        Me.CreateV3TLOG = New System.Windows.Forms.Button
        Me.SuspendLayout()
        '
        'ProcMyToday
        '
        Me.ProcMyToday.Location = New System.Drawing.Point(6, 182)
        Me.ProcMyToday.Margin = New System.Windows.Forms.Padding(2)
        Me.ProcMyToday.Name = "ProcMyToday"
        Me.ProcMyToday.Size = New System.Drawing.Size(102, 26)
        Me.ProcMyToday.TabIndex = 0
        Me.ProcMyToday.Text = "Process MyToday"
        Me.ProcMyToday.UseVisualStyleBackColor = True
        '
        'SaveBinary
        '
        Me.SaveBinary.Location = New System.Drawing.Point(114, 214)
        Me.SaveBinary.Margin = New System.Windows.Forms.Padding(2)
        Me.SaveBinary.Name = "SaveBinary"
        Me.SaveBinary.Size = New System.Drawing.Size(102, 26)
        Me.SaveBinary.TabIndex = 1
        Me.SaveBinary.Text = "Save Binary"
        Me.SaveBinary.UseVisualStyleBackColor = True
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(12, 20)
        Me.Label1.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(47, 13)
        Me.Label1.TabIndex = 2
        Me.Label1.Text = "Records"
        Me.Label1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(12, 46)
        Me.Label2.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(26, 13)
        Me.Label2.TabIndex = 3
        Me.Label2.Text = "First"
        Me.Label2.TextAlign = System.Drawing.ContentAlignment.MiddleLeft
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(12, 72)
        Me.Label3.Margin = New System.Windows.Forms.Padding(2, 0, 2, 0)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(27, 13)
        Me.Label3.TabIndex = 4
        Me.Label3.Text = "Last"
        Me.Label3.TextAlign = System.Drawing.ContentAlignment.MiddleLeft
        '
        'RecsBox
        '
        Me.RecsBox.Location = New System.Drawing.Point(66, 20)
        Me.RecsBox.Margin = New System.Windows.Forms.Padding(2)
        Me.RecsBox.Name = "RecsBox"
        Me.RecsBox.Size = New System.Drawing.Size(67, 20)
        Me.RecsBox.TabIndex = 5
        '
        'FirstBox
        '
        Me.FirstBox.Location = New System.Drawing.Point(66, 46)
        Me.FirstBox.Margin = New System.Windows.Forms.Padding(2)
        Me.FirstBox.Name = "FirstBox"
        Me.FirstBox.Size = New System.Drawing.Size(130, 20)
        Me.FirstBox.TabIndex = 6
        '
        'LastBox
        '
        Me.LastBox.Location = New System.Drawing.Point(66, 72)
        Me.LastBox.Margin = New System.Windows.Forms.Padding(2)
        Me.LastBox.Name = "LastBox"
        Me.LastBox.Size = New System.Drawing.Size(130, 20)
        Me.LastBox.TabIndex = 7
        '
        'ProcessSorted
        '
        Me.ProcessSorted.Location = New System.Drawing.Point(114, 182)
        Me.ProcessSorted.Margin = New System.Windows.Forms.Padding(2)
        Me.ProcessSorted.Name = "ProcessSorted"
        Me.ProcessSorted.Size = New System.Drawing.Size(102, 26)
        Me.ProcessSorted.TabIndex = 8
        Me.ProcessSorted.Text = "Process Sorted"
        Me.ProcessSorted.UseVisualStyleBackColor = True
        '
        'ReadBinary
        '
        Me.ReadBinary.Location = New System.Drawing.Point(6, 214)
        Me.ReadBinary.Margin = New System.Windows.Forms.Padding(2)
        Me.ReadBinary.Name = "ReadBinary"
        Me.ReadBinary.Size = New System.Drawing.Size(102, 26)
        Me.ReadBinary.TabIndex = 9
        Me.ReadBinary.Text = "Read Binary"
        Me.ReadBinary.UseVisualStyleBackColor = True
        '
        'ReadBPQAISLog
        '
        Me.ReadBPQAISLog.Location = New System.Drawing.Point(222, 182)
        Me.ReadBPQAISLog.Margin = New System.Windows.Forms.Padding(2)
        Me.ReadBPQAISLog.Name = "ReadBPQAISLog"
        Me.ReadBPQAISLog.Size = New System.Drawing.Size(102, 26)
        Me.ReadBPQAISLog.TabIndex = 10
        Me.ReadBPQAISLog.Text = "Read BPQAIS Log"
        Me.ReadBPQAISLog.UseVisualStyleBackColor = True
        '
        'CheckOrder
        '
        Me.CheckOrder.Location = New System.Drawing.Point(222, 214)
        Me.CheckOrder.Margin = New System.Windows.Forms.Padding(2)
        Me.CheckOrder.Name = "CheckOrder"
        Me.CheckOrder.Size = New System.Drawing.Size(102, 26)
        Me.CheckOrder.TabIndex = 11
        Me.CheckOrder.Text = "Check Order"
        Me.CheckOrder.UseVisualStyleBackColor = True
        '
        'EnableGE
        '
        Me.EnableGE.AutoSize = True
        Me.EnableGE.Location = New System.Drawing.Point(18, 104)
        Me.EnableGE.Margin = New System.Windows.Forms.Padding(2)
        Me.EnableGE.Name = "EnableGE"
        Me.EnableGE.Size = New System.Drawing.Size(133, 17)
        Me.EnableGE.TabIndex = 12
        Me.EnableGE.Text = "Show on Google Earth"
        Me.EnableGE.UseVisualStyleBackColor = True
        '
        'MonthCalendar1
        '
        Me.MonthCalendar1.Location = New System.Drawing.Point(222, 5)
        Me.MonthCalendar1.Margin = New System.Windows.Forms.Padding(7)
        Me.MonthCalendar1.MaxSelectionCount = 1
        Me.MonthCalendar1.Name = "MonthCalendar1"
        Me.MonthCalendar1.ShowToday = False
        Me.MonthCalendar1.ShowTodayCircle = False
        Me.MonthCalendar1.TabIndex = 13
        '
        'SortButton
        '
        Me.SortButton.Location = New System.Drawing.Point(9, 245)
        Me.SortButton.Name = "SortButton"
        Me.SortButton.Size = New System.Drawing.Size(99, 26)
        Me.SortButton.TabIndex = 14
        Me.SortButton.Text = "Sort"
        Me.SortButton.UseVisualStyleBackColor = True
        '
        'ReadMMCSV
        '
        Me.ReadMMCSV.Location = New System.Drawing.Point(114, 244)
        Me.ReadMMCSV.Margin = New System.Windows.Forms.Padding(2)
        Me.ReadMMCSV.Name = "ReadMMCSV"
        Me.ReadMMCSV.Size = New System.Drawing.Size(102, 26)
        Me.ReadMMCSV.TabIndex = 15
        Me.ReadMMCSV.Text = "Read MM CSV"
        Me.ReadMMCSV.UseVisualStyleBackColor = True
        '
        'CreateV3TLOG
        '
        Me.CreateV3TLOG.Location = New System.Drawing.Point(225, 244)
        Me.CreateV3TLOG.Name = "CreateV3TLOG"
        Me.CreateV3TLOG.Size = New System.Drawing.Size(98, 26)
        Me.CreateV3TLOG.TabIndex = 16
        Me.CreateV3TLOG.Text = "Create V3 TLOG"
        Me.CreateV3TLOG.UseVisualStyleBackColor = True
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(408, 286)
        Me.Controls.Add(Me.CreateV3TLOG)
        Me.Controls.Add(Me.ReadMMCSV)
        Me.Controls.Add(Me.SortButton)
        Me.Controls.Add(Me.MonthCalendar1)
        Me.Controls.Add(Me.EnableGE)
        Me.Controls.Add(Me.CheckOrder)
        Me.Controls.Add(Me.ReadBPQAISLog)
        Me.Controls.Add(Me.ReadBinary)
        Me.Controls.Add(Me.ProcessSorted)
        Me.Controls.Add(Me.LastBox)
        Me.Controls.Add(Me.FirstBox)
        Me.Controls.Add(Me.RecsBox)
        Me.Controls.Add(Me.Label3)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.SaveBinary)
        Me.Controls.Add(Me.ProcMyToday)
        Me.Margin = New System.Windows.Forms.Padding(2)
        Me.Name = "Form1"
        Me.Text = "Form1"
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents ProcMyToday As System.Windows.Forms.Button
    Friend WithEvents SaveBinary As System.Windows.Forms.Button
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents RecsBox As System.Windows.Forms.TextBox
    Friend WithEvents FirstBox As System.Windows.Forms.TextBox
    Friend WithEvents LastBox As System.Windows.Forms.TextBox
    Friend WithEvents ProcessSorted As System.Windows.Forms.Button
    Friend WithEvents ReadBinary As System.Windows.Forms.Button
    Friend WithEvents ReadBPQAISLog As System.Windows.Forms.Button
    Friend WithEvents CheckOrder As System.Windows.Forms.Button
    Friend WithEvents EnableGE As System.Windows.Forms.CheckBox
    Friend WithEvents MonthCalendar1 As System.Windows.Forms.MonthCalendar
    Friend WithEvents SortButton As System.Windows.Forms.Button
    Friend WithEvents ReadMMCSV As System.Windows.Forms.Button
    Friend WithEvents CreateV3TLOG As System.Windows.Forms.Button

End Class
