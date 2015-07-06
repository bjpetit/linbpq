<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Test
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.rdoDataACK = New System.Windows.Forms.RadioButton()
        Me.Button1 = New System.Windows.Forms.Button()
        Me.nudRepeats = New System.Windows.Forms.NumericUpDown()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.rdoDataNAK = New System.Windows.Forms.RadioButton()
        Me.rdoCONREQ200 = New System.Windows.Forms.RadioButton()
        Me.lblCount = New System.Windows.Forms.Label()
        Me.rdoIDFrame = New System.Windows.Forms.RadioButton()
        Me.rdoBreak = New System.Windows.Forms.RadioButton()
        Me.rdoConAck500 = New System.Windows.Forms.RadioButton()
        Me.rdo4PSK200_16 = New System.Windows.Forms.RadioButton()
        Me.rdo4PSK200_64 = New System.Windows.Forms.RadioButton()
        Me.rdo8PSK200_108 = New System.Windows.Forms.RadioButton()
        Me.rdo4PSK500_128 = New System.Windows.Forms.RadioButton()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.rdo4PSK1000_256 = New System.Windows.Forms.RadioButton()
        Me.rdo4PSK2000_512 = New System.Windows.Forms.RadioButton()
        Me.GroupBox2 = New System.Windows.Forms.GroupBox()
        Me.rdo8FSK200_24 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK2000_200 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK2000_600 = New System.Windows.Forms.RadioButton()
        Me.rdo16FSK500_16 = New System.Windows.Forms.RadioButton()
        Me.rdo16FSK500_32 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK2000_256 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK1000_128 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK500_32 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK200_16 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK500_64 = New System.Windows.Forms.RadioButton()
        Me.rdo4FSK200_32 = New System.Windows.Forms.RadioButton()
        Me.rdo8PSK1000_636 = New System.Windows.Forms.RadioButton()
        Me.rdo8PSK500_318 = New System.Windows.Forms.RadioButton()
        Me.rdo8PSK2000_1272 = New System.Windows.Forms.RadioButton()
        Me.rdo8PSK2000_864 = New System.Windows.Forms.RadioButton()
        Me.rdo8PSK1000_432 = New System.Windows.Forms.RadioButton()
        Me.rdo8PSK500_216 = New System.Windows.Forms.RadioButton()
        Me.Label8 = New System.Windows.Forms.Label()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.rdoDisc = New System.Windows.Forms.RadioButton()
        Me.rdoIdle = New System.Windows.Forms.RadioButton()
        Me.rdoEnd = New System.Windows.Forms.RadioButton()
        Me.rdoConRejBusy = New System.Windows.Forms.RadioButton()
        Me.rdoConAck2000 = New System.Windows.Forms.RadioButton()
        Me.rdoConAck1000 = New System.Windows.Forms.RadioButton()
        Me.rdoConAck200 = New System.Windows.Forms.RadioButton()
        Me.rdoConReq2000 = New System.Windows.Forms.RadioButton()
        Me.rdoConReq1000 = New System.Windows.Forms.RadioButton()
        Me.rdoCONReq500 = New System.Windows.Forms.RadioButton()
        Me.rdo4PSK2000_960 = New System.Windows.Forms.RadioButton()
        Me.rdo4PSK1000_480 = New System.Windows.Forms.RadioButton()
        Me.rdo4PSK500_240 = New System.Windows.Forms.RadioButton()
        Me.GroupBox1 = New System.Windows.Forms.GroupBox()
        Me.bttFECAbort = New System.Windows.Forms.Button()
        Me.lblFECInfo = New System.Windows.Forms.Label()
        Me.btnFECTest = New System.Windows.Forms.Button()
        Me.rdoSounding = New System.Windows.Forms.RadioButton()
        CType(Me.nudRepeats, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.GroupBox2.SuspendLayout()
        Me.GroupBox1.SuspendLayout()
        Me.SuspendLayout()
        '
        'rdoDataACK
        '
        Me.rdoDataACK.AutoSize = True
        Me.rdoDataACK.Checked = True
        Me.rdoDataACK.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoDataACK.Location = New System.Drawing.Point(299, 99)
        Me.rdoDataACK.Name = "rdoDataACK"
        Me.rdoDataACK.Size = New System.Drawing.Size(76, 17)
        Me.rdoDataACK.TabIndex = 23
        Me.rdoDataACK.TabStop = True
        Me.rdoDataACK.Text = "DataACK"
        Me.rdoDataACK.UseVisualStyleBackColor = True
        '
        'Button1
        '
        Me.Button1.Location = New System.Drawing.Point(106, 571)
        Me.Button1.Name = "Button1"
        Me.Button1.Size = New System.Drawing.Size(142, 23)
        Me.Button1.TabIndex = 38
        Me.Button1.Text = "Play Wave "
        Me.Button1.UseVisualStyleBackColor = True
        '
        'nudRepeats
        '
        Me.nudRepeats.Location = New System.Drawing.Point(310, 571)
        Me.nudRepeats.Minimum = New Decimal(New Integer() {1, 0, 0, 0})
        Me.nudRepeats.Name = "nudRepeats"
        Me.nudRepeats.Size = New System.Drawing.Size(42, 20)
        Me.nudRepeats.TabIndex = 39
        Me.nudRepeats.Value = New Decimal(New Integer() {1, 0, 0, 0})
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(254, 576)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(50, 13)
        Me.Label6.TabIndex = 72
        Me.Label6.Text = "Repeats:"
        '
        'rdoDataNAK
        '
        Me.rdoDataNAK.AutoSize = True
        Me.rdoDataNAK.Checked = True
        Me.rdoDataNAK.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoDataNAK.Location = New System.Drawing.Point(299, 122)
        Me.rdoDataNAK.Name = "rdoDataNAK"
        Me.rdoDataNAK.Size = New System.Drawing.Size(77, 17)
        Me.rdoDataNAK.TabIndex = 73
        Me.rdoDataNAK.TabStop = True
        Me.rdoDataNAK.Text = "DataNAK"
        Me.rdoDataNAK.UseVisualStyleBackColor = True
        '
        'rdoCONREQ200
        '
        Me.rdoCONREQ200.AutoSize = True
        Me.rdoCONREQ200.Checked = True
        Me.rdoCONREQ200.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoCONREQ200.Location = New System.Drawing.Point(147, 19)
        Me.rdoCONREQ200.Name = "rdoCONREQ200"
        Me.rdoCONREQ200.Size = New System.Drawing.Size(98, 17)
        Me.rdoCONREQ200.TabIndex = 76
        Me.rdoCONREQ200.TabStop = True
        Me.rdoCONREQ200.Text = "CONREQ200"
        Me.rdoCONREQ200.UseVisualStyleBackColor = True
        '
        'lblCount
        '
        Me.lblCount.AutoSize = True
        Me.lblCount.Location = New System.Drawing.Point(472, 578)
        Me.lblCount.Name = "lblCount"
        Me.lblCount.Size = New System.Drawing.Size(13, 13)
        Me.lblCount.TabIndex = 77
        Me.lblCount.Text = "0"
        '
        'rdoIDFrame
        '
        Me.rdoIDFrame.AutoSize = True
        Me.rdoIDFrame.Checked = True
        Me.rdoIDFrame.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoIDFrame.Location = New System.Drawing.Point(403, 99)
        Me.rdoIDFrame.Name = "rdoIDFrame"
        Me.rdoIDFrame.Size = New System.Drawing.Size(170, 17)
        Me.rdoIDFrame.TabIndex = 78
        Me.rdoIDFrame.TabStop = True
        Me.rdoIDFrame.Text = "ID (with CWID if enabled)"
        Me.rdoIDFrame.UseVisualStyleBackColor = True
        '
        'rdoBreak
        '
        Me.rdoBreak.AutoSize = True
        Me.rdoBreak.Checked = True
        Me.rdoBreak.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoBreak.Location = New System.Drawing.Point(147, 99)
        Me.rdoBreak.Name = "rdoBreak"
        Me.rdoBreak.Size = New System.Drawing.Size(66, 17)
        Me.rdoBreak.TabIndex = 79
        Me.rdoBreak.TabStop = True
        Me.rdoBreak.Text = "BREAK"
        Me.rdoBreak.UseVisualStyleBackColor = True
        '
        'rdoConAck500
        '
        Me.rdoConAck500.AutoSize = True
        Me.rdoConAck500.Checked = True
        Me.rdoConAck500.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoConAck500.Location = New System.Drawing.Point(270, 42)
        Me.rdoConAck500.Name = "rdoConAck500"
        Me.rdoConAck500.Size = New System.Drawing.Size(111, 17)
        Me.rdoConAck500.TabIndex = 80
        Me.rdoConAck500.TabStop = True
        Me.rdoConAck500.Text = "CONACK500+T"
        Me.rdoConAck500.UseVisualStyleBackColor = True
        '
        'rdo4PSK200_16
        '
        Me.rdo4PSK200_16.AutoSize = True
        Me.rdo4PSK200_16.Checked = True
        Me.rdo4PSK200_16.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK200_16.Location = New System.Drawing.Point(6, 194)
        Me.rdo4PSK200_16.Name = "rdo4PSK200_16"
        Me.rdo4PSK200_16.Size = New System.Drawing.Size(173, 17)
        Me.rdo4PSK200_16.TabIndex = 83
        Me.rdo4PSK200_16.TabStop = True
        Me.rdo4PSK200_16.Text = "1Car4PSK,100bd_16Bytes"
        Me.rdo4PSK200_16.UseVisualStyleBackColor = True
        '
        'rdo4PSK200_64
        '
        Me.rdo4PSK200_64.AutoSize = True
        Me.rdo4PSK200_64.Checked = True
        Me.rdo4PSK200_64.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK200_64.Location = New System.Drawing.Point(6, 171)
        Me.rdo4PSK200_64.Name = "rdo4PSK200_64"
        Me.rdo4PSK200_64.Size = New System.Drawing.Size(173, 17)
        Me.rdo4PSK200_64.TabIndex = 84
        Me.rdo4PSK200_64.TabStop = True
        Me.rdo4PSK200_64.Text = "1Car4PSK,100bd_64Bytes"
        Me.rdo4PSK200_64.UseVisualStyleBackColor = True
        '
        'rdo8PSK200_108
        '
        Me.rdo8PSK200_108.AutoSize = True
        Me.rdo8PSK200_108.Checked = True
        Me.rdo8PSK200_108.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8PSK200_108.Location = New System.Drawing.Point(6, 217)
        Me.rdo8PSK200_108.Name = "rdo8PSK200_108"
        Me.rdo8PSK200_108.Size = New System.Drawing.Size(180, 17)
        Me.rdo8PSK200_108.TabIndex = 85
        Me.rdo8PSK200_108.TabStop = True
        Me.rdo8PSK200_108.Text = "1Car8PSK,100bd_108Bytes"
        Me.rdo8PSK200_108.UseVisualStyleBackColor = True
        '
        'rdo4PSK500_128
        '
        Me.rdo4PSK500_128.AutoSize = True
        Me.rdo4PSK500_128.Checked = True
        Me.rdo4PSK500_128.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK500_128.Location = New System.Drawing.Point(370, 171)
        Me.rdo4PSK500_128.Name = "rdo4PSK500_128"
        Me.rdo4PSK500_128.Size = New System.Drawing.Size(180, 17)
        Me.rdo4PSK500_128.TabIndex = 86
        Me.rdo4PSK500_128.TabStop = True
        Me.rdo4PSK500_128.Text = "2Car4PSK,100bd_128Bytes"
        Me.rdo4PSK500_128.UseVisualStyleBackColor = True
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(369, 575)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(97, 13)
        Me.Label1.TabIndex = 87
        Me.Label1.Text = "Frames Remaining:"
        '
        'rdo4PSK1000_256
        '
        Me.rdo4PSK1000_256.AutoSize = True
        Me.rdo4PSK1000_256.Checked = True
        Me.rdo4PSK1000_256.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK1000_256.Location = New System.Drawing.Point(6, 375)
        Me.rdo4PSK1000_256.Name = "rdo4PSK1000_256"
        Me.rdo4PSK1000_256.Size = New System.Drawing.Size(180, 17)
        Me.rdo4PSK1000_256.TabIndex = 87
        Me.rdo4PSK1000_256.TabStop = True
        Me.rdo4PSK1000_256.Text = "4Car4PSK,100bd_256Bytes"
        Me.rdo4PSK1000_256.UseVisualStyleBackColor = True
        '
        'rdo4PSK2000_512
        '
        Me.rdo4PSK2000_512.AutoSize = True
        Me.rdo4PSK2000_512.Checked = True
        Me.rdo4PSK2000_512.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK2000_512.Location = New System.Drawing.Point(370, 373)
        Me.rdo4PSK2000_512.Name = "rdo4PSK2000_512"
        Me.rdo4PSK2000_512.Size = New System.Drawing.Size(180, 17)
        Me.rdo4PSK2000_512.TabIndex = 88
        Me.rdo4PSK2000_512.TabStop = True
        Me.rdo4PSK2000_512.Text = "8Car4PSK,100bd_512Bytes"
        Me.rdo4PSK2000_512.UseVisualStyleBackColor = True
        '
        'GroupBox2
        '
        Me.GroupBox2.Controls.Add(Me.rdoSounding)
        Me.GroupBox2.Controls.Add(Me.rdo8FSK200_24)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK2000_200)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK2000_600)
        Me.GroupBox2.Controls.Add(Me.rdo16FSK500_16)
        Me.GroupBox2.Controls.Add(Me.rdo16FSK500_32)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK2000_256)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK1000_128)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK500_32)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK200_16)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK500_64)
        Me.GroupBox2.Controls.Add(Me.rdo4FSK200_32)
        Me.GroupBox2.Controls.Add(Me.rdo8PSK1000_636)
        Me.GroupBox2.Controls.Add(Me.rdo8PSK500_318)
        Me.GroupBox2.Controls.Add(Me.rdo8PSK2000_1272)
        Me.GroupBox2.Controls.Add(Me.rdo8PSK2000_864)
        Me.GroupBox2.Controls.Add(Me.rdo8PSK1000_432)
        Me.GroupBox2.Controls.Add(Me.rdo8PSK500_216)
        Me.GroupBox2.Controls.Add(Me.Label8)
        Me.GroupBox2.Controls.Add(Me.Label7)
        Me.GroupBox2.Controls.Add(Me.Label5)
        Me.GroupBox2.Controls.Add(Me.Label4)
        Me.GroupBox2.Controls.Add(Me.Label3)
        Me.GroupBox2.Controls.Add(Me.Label2)
        Me.GroupBox2.Controls.Add(Me.rdoDisc)
        Me.GroupBox2.Controls.Add(Me.rdoIdle)
        Me.GroupBox2.Controls.Add(Me.rdoEnd)
        Me.GroupBox2.Controls.Add(Me.rdoConRejBusy)
        Me.GroupBox2.Controls.Add(Me.rdoConAck2000)
        Me.GroupBox2.Controls.Add(Me.rdoConAck1000)
        Me.GroupBox2.Controls.Add(Me.rdoConAck200)
        Me.GroupBox2.Controls.Add(Me.rdoConReq2000)
        Me.GroupBox2.Controls.Add(Me.rdoConReq1000)
        Me.GroupBox2.Controls.Add(Me.rdoCONReq500)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK2000_960)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK1000_480)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK500_240)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK2000_512)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK1000_256)
        Me.GroupBox2.Controls.Add(Me.Label1)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK500_128)
        Me.GroupBox2.Controls.Add(Me.rdo8PSK200_108)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK200_64)
        Me.GroupBox2.Controls.Add(Me.rdo4PSK200_16)
        Me.GroupBox2.Controls.Add(Me.rdoConAck500)
        Me.GroupBox2.Controls.Add(Me.rdoBreak)
        Me.GroupBox2.Controls.Add(Me.rdoIDFrame)
        Me.GroupBox2.Controls.Add(Me.lblCount)
        Me.GroupBox2.Controls.Add(Me.rdoCONREQ200)
        Me.GroupBox2.Controls.Add(Me.rdoDataNAK)
        Me.GroupBox2.Controls.Add(Me.Label6)
        Me.GroupBox2.Controls.Add(Me.nudRepeats)
        Me.GroupBox2.Controls.Add(Me.Button1)
        Me.GroupBox2.Controls.Add(Me.rdoDataACK)
        Me.GroupBox2.Location = New System.Drawing.Point(12, 12)
        Me.GroupBox2.Name = "GroupBox2"
        Me.GroupBox2.Size = New System.Drawing.Size(651, 604)
        Me.GroupBox2.TabIndex = 20
        Me.GroupBox2.TabStop = False
        Me.GroupBox2.Text = "Test Controls "
        '
        'rdo8FSK200_24
        '
        Me.rdo8FSK200_24.AutoSize = True
        Me.rdo8FSK200_24.Checked = True
        Me.rdo8FSK200_24.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8FSK200_24.Location = New System.Drawing.Point(6, 284)
        Me.rdo8FSK200_24.Name = "rdo8FSK200_24"
        Me.rdo8FSK200_24.Size = New System.Drawing.Size(165, 17)
        Me.rdo8FSK200_24.TabIndex = 124
        Me.rdo8FSK200_24.TabStop = True
        Me.rdo8FSK200_24.Text = "1Car8FSK,25bd_24Bytes"
        Me.rdo8FSK200_24.UseVisualStyleBackColor = True
        '
        'rdo4FSK2000_200
        '
        Me.rdo4FSK2000_200.AutoSize = True
        Me.rdo4FSK2000_200.Checked = True
        Me.rdo4FSK2000_200.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK2000_200.Location = New System.Drawing.Point(371, 511)
        Me.rdo4FSK2000_200.Name = "rdo4FSK2000_200"
        Me.rdo4FSK2000_200.Size = New System.Drawing.Size(165, 17)
        Me.rdo4FSK2000_200.TabIndex = 123
        Me.rdo4FSK2000_200.TabStop = True
        Me.rdo4FSK2000_200.Text = "1Car4FSK.600_200Bytes"
        Me.rdo4FSK2000_200.UseVisualStyleBackColor = True
        '
        'rdo4FSK2000_600
        '
        Me.rdo4FSK2000_600.AutoSize = True
        Me.rdo4FSK2000_600.Checked = True
        Me.rdo4FSK2000_600.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK2000_600.Location = New System.Drawing.Point(371, 488)
        Me.rdo4FSK2000_600.Name = "rdo4FSK2000_600"
        Me.rdo4FSK2000_600.Size = New System.Drawing.Size(179, 17)
        Me.rdo4FSK2000_600.TabIndex = 122
        Me.rdo4FSK2000_600.TabStop = True
        Me.rdo4FSK2000_600.Text = "1Car4FSK,600bd_600Bytes"
        Me.rdo4FSK2000_600.UseVisualStyleBackColor = True
        '
        'rdo16FSK500_16
        '
        Me.rdo16FSK500_16.AutoSize = True
        Me.rdo16FSK500_16.Checked = True
        Me.rdo16FSK500_16.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo16FSK500_16.Location = New System.Drawing.Point(370, 329)
        Me.rdo16FSK500_16.Name = "rdo16FSK500_16"
        Me.rdo16FSK500_16.Size = New System.Drawing.Size(171, 17)
        Me.rdo16FSK500_16.TabIndex = 121
        Me.rdo16FSK500_16.TabStop = True
        Me.rdo16FSK500_16.Text = "1Car16FSK,25bd_16bytes"
        Me.rdo16FSK500_16.UseVisualStyleBackColor = True
        '
        'rdo16FSK500_32
        '
        Me.rdo16FSK500_32.AutoSize = True
        Me.rdo16FSK500_32.Checked = True
        Me.rdo16FSK500_32.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo16FSK500_32.Location = New System.Drawing.Point(370, 306)
        Me.rdo16FSK500_32.Name = "rdo16FSK500_32"
        Me.rdo16FSK500_32.Size = New System.Drawing.Size(171, 17)
        Me.rdo16FSK500_32.TabIndex = 120
        Me.rdo16FSK500_32.TabStop = True
        Me.rdo16FSK500_32.Text = "1Car16FSK,25bd_32bytes"
        Me.rdo16FSK500_32.UseVisualStyleBackColor = True
        '
        'rdo4FSK2000_256
        '
        Me.rdo4FSK2000_256.AutoSize = True
        Me.rdo4FSK2000_256.Checked = True
        Me.rdo4FSK2000_256.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK2000_256.Location = New System.Drawing.Point(370, 465)
        Me.rdo4FSK2000_256.Name = "rdo4FSK2000_256"
        Me.rdo4FSK2000_256.Size = New System.Drawing.Size(165, 17)
        Me.rdo4FSK2000_256.TabIndex = 119
        Me.rdo4FSK2000_256.TabStop = True
        Me.rdo4FSK2000_256.Text = "4Car4FSK,100_256Bytes"
        Me.rdo4FSK2000_256.UseVisualStyleBackColor = True
        '
        'rdo4FSK1000_128
        '
        Me.rdo4FSK1000_128.AutoSize = True
        Me.rdo4FSK1000_128.Checked = True
        Me.rdo4FSK1000_128.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK1000_128.Location = New System.Drawing.Point(3, 469)
        Me.rdo4FSK1000_128.Name = "rdo4FSK1000_128"
        Me.rdo4FSK1000_128.Size = New System.Drawing.Size(179, 17)
        Me.rdo4FSK1000_128.TabIndex = 118
        Me.rdo4FSK1000_128.TabStop = True
        Me.rdo4FSK1000_128.Text = "2Car4FSK,100bd_128Bytes"
        Me.rdo4FSK1000_128.UseVisualStyleBackColor = True
        '
        'rdo4FSK500_32
        '
        Me.rdo4FSK500_32.AutoSize = True
        Me.rdo4FSK500_32.Checked = True
        Me.rdo4FSK500_32.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK500_32.Location = New System.Drawing.Point(370, 284)
        Me.rdo4FSK500_32.Name = "rdo4FSK500_32"
        Me.rdo4FSK500_32.Size = New System.Drawing.Size(171, 17)
        Me.rdo4FSK500_32.TabIndex = 117
        Me.rdo4FSK500_32.TabStop = True
        Me.rdo4FSK500_32.Text = "1Car4FSK,100bd_32bytes"
        Me.rdo4FSK500_32.UseVisualStyleBackColor = True
        '
        'rdo4FSK200_16
        '
        Me.rdo4FSK200_16.AutoSize = True
        Me.rdo4FSK200_16.Checked = True
        Me.rdo4FSK200_16.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK200_16.Location = New System.Drawing.Point(6, 263)
        Me.rdo4FSK200_16.Name = "rdo4FSK200_16"
        Me.rdo4FSK200_16.Size = New System.Drawing.Size(165, 17)
        Me.rdo4FSK200_16.TabIndex = 116
        Me.rdo4FSK200_16.TabStop = True
        Me.rdo4FSK200_16.Text = "1Car4FSK,50bd_16Bytes"
        Me.rdo4FSK200_16.UseVisualStyleBackColor = True
        '
        'rdo4FSK500_64
        '
        Me.rdo4FSK500_64.AutoSize = True
        Me.rdo4FSK500_64.Checked = True
        Me.rdo4FSK500_64.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK500_64.Location = New System.Drawing.Point(370, 261)
        Me.rdo4FSK500_64.Name = "rdo4FSK500_64"
        Me.rdo4FSK500_64.Size = New System.Drawing.Size(171, 17)
        Me.rdo4FSK500_64.TabIndex = 115
        Me.rdo4FSK500_64.TabStop = True
        Me.rdo4FSK500_64.Text = "1Car4FSK,100bd_64bytes"
        Me.rdo4FSK500_64.UseVisualStyleBackColor = True
        '
        'rdo4FSK200_32
        '
        Me.rdo4FSK200_32.AutoSize = True
        Me.rdo4FSK200_32.Checked = True
        Me.rdo4FSK200_32.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4FSK200_32.Location = New System.Drawing.Point(6, 240)
        Me.rdo4FSK200_32.Name = "rdo4FSK200_32"
        Me.rdo4FSK200_32.Size = New System.Drawing.Size(165, 17)
        Me.rdo4FSK200_32.TabIndex = 114
        Me.rdo4FSK200_32.TabStop = True
        Me.rdo4FSK200_32.Text = "1Car4FSK,50bd_32Bytes"
        Me.rdo4FSK200_32.UseVisualStyleBackColor = True
        '
        'rdo8PSK1000_636
        '
        Me.rdo8PSK1000_636.AutoSize = True
        Me.rdo8PSK1000_636.Checked = True
        Me.rdo8PSK1000_636.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8PSK1000_636.Location = New System.Drawing.Point(6, 446)
        Me.rdo8PSK1000_636.Name = "rdo8PSK1000_636"
        Me.rdo8PSK1000_636.Size = New System.Drawing.Size(180, 17)
        Me.rdo8PSK1000_636.TabIndex = 113
        Me.rdo8PSK1000_636.TabStop = True
        Me.rdo8PSK1000_636.Text = "4Car8PSK,167bd_636Bytes"
        Me.rdo8PSK1000_636.UseVisualStyleBackColor = True
        '
        'rdo8PSK500_318
        '
        Me.rdo8PSK500_318.AutoSize = True
        Me.rdo8PSK500_318.Checked = True
        Me.rdo8PSK500_318.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8PSK500_318.Location = New System.Drawing.Point(370, 240)
        Me.rdo8PSK500_318.Name = "rdo8PSK500_318"
        Me.rdo8PSK500_318.Size = New System.Drawing.Size(179, 17)
        Me.rdo8PSK500_318.TabIndex = 112
        Me.rdo8PSK500_318.TabStop = True
        Me.rdo8PSK500_318.Text = "2Car8PSK,167bd_318bytes"
        Me.rdo8PSK500_318.UseVisualStyleBackColor = True
        '
        'rdo8PSK2000_1272
        '
        Me.rdo8PSK2000_1272.AutoSize = True
        Me.rdo8PSK2000_1272.Checked = True
        Me.rdo8PSK2000_1272.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8PSK2000_1272.Location = New System.Drawing.Point(370, 442)
        Me.rdo8PSK2000_1272.Name = "rdo8PSK2000_1272"
        Me.rdo8PSK2000_1272.Size = New System.Drawing.Size(187, 17)
        Me.rdo8PSK2000_1272.TabIndex = 111
        Me.rdo8PSK2000_1272.TabStop = True
        Me.rdo8PSK2000_1272.Text = "8Car8PSK,167bd_1272Bytes"
        Me.rdo8PSK2000_1272.UseVisualStyleBackColor = True
        '
        'rdo8PSK2000_864
        '
        Me.rdo8PSK2000_864.AutoSize = True
        Me.rdo8PSK2000_864.Checked = True
        Me.rdo8PSK2000_864.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8PSK2000_864.Location = New System.Drawing.Point(370, 419)
        Me.rdo8PSK2000_864.Name = "rdo8PSK2000_864"
        Me.rdo8PSK2000_864.Size = New System.Drawing.Size(180, 17)
        Me.rdo8PSK2000_864.TabIndex = 110
        Me.rdo8PSK2000_864.TabStop = True
        Me.rdo8PSK2000_864.Text = "8Car8PSK,100bd_864Bytes"
        Me.rdo8PSK2000_864.UseVisualStyleBackColor = True
        '
        'rdo8PSK1000_432
        '
        Me.rdo8PSK1000_432.AutoSize = True
        Me.rdo8PSK1000_432.Checked = True
        Me.rdo8PSK1000_432.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8PSK1000_432.Location = New System.Drawing.Point(6, 423)
        Me.rdo8PSK1000_432.Name = "rdo8PSK1000_432"
        Me.rdo8PSK1000_432.Size = New System.Drawing.Size(180, 17)
        Me.rdo8PSK1000_432.TabIndex = 109
        Me.rdo8PSK1000_432.TabStop = True
        Me.rdo8PSK1000_432.Text = "4Car8PSK,100bd_432Bytes"
        Me.rdo8PSK1000_432.UseVisualStyleBackColor = True
        '
        'rdo8PSK500_216
        '
        Me.rdo8PSK500_216.AutoSize = True
        Me.rdo8PSK500_216.Checked = True
        Me.rdo8PSK500_216.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo8PSK500_216.Location = New System.Drawing.Point(370, 217)
        Me.rdo8PSK500_216.Name = "rdo8PSK500_216"
        Me.rdo8PSK500_216.Size = New System.Drawing.Size(180, 17)
        Me.rdo8PSK500_216.TabIndex = 108
        Me.rdo8PSK500_216.TabStop = True
        Me.rdo8PSK500_216.Text = "2Car8PSK,100bd_216Bytes"
        Me.rdo8PSK500_216.UseVisualStyleBackColor = True
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label8.ForeColor = System.Drawing.Color.Red
        Me.Label8.Location = New System.Drawing.Point(369, 357)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(150, 13)
        Me.Label8.TabIndex = 107
        Me.Label8.Text = "2000 Hz Bandwidth data:"
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label7.ForeColor = System.Drawing.Color.Red
        Me.Label7.Location = New System.Drawing.Point(5, 357)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(150, 13)
        Me.Label7.TabIndex = 106
        Me.Label7.Text = "1000 Hz Bandwidth data:"
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label5.ForeColor = System.Drawing.Color.Red
        Me.Label5.Location = New System.Drawing.Point(369, 155)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(143, 13)
        Me.Label5.TabIndex = 105
        Me.Label5.Text = "500 Hz Bandwidth data:"
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label4.ForeColor = System.Drawing.Color.Red
        Me.Label4.Location = New System.Drawing.Point(0, 155)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(147, 13)
        Me.Label4.TabIndex = 104
        Me.Label4.Text = " 200 Hz Bandwidth data:"
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label3.ForeColor = System.Drawing.Color.Red
        Me.Label3.Location = New System.Drawing.Point(9, 101)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(133, 13)
        Me.Label3.TabIndex = 103
        Me.Label3.Text = "Control and ID Frames"
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label2.ForeColor = System.Drawing.Color.Red
        Me.Label2.Location = New System.Drawing.Point(22, 21)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(119, 13)
        Me.Label2.TabIndex = 102
        Me.Label2.Text = "Connection Control:"
        '
        'rdoDisc
        '
        Me.rdoDisc.AutoSize = True
        Me.rdoDisc.Checked = True
        Me.rdoDisc.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoDisc.Location = New System.Drawing.Point(226, 99)
        Me.rdoDisc.Name = "rdoDisc"
        Me.rdoDisc.Size = New System.Drawing.Size(54, 17)
        Me.rdoDisc.TabIndex = 101
        Me.rdoDisc.TabStop = True
        Me.rdoDisc.Text = "DISC"
        Me.rdoDisc.UseVisualStyleBackColor = True
        '
        'rdoIdle
        '
        Me.rdoIdle.AutoSize = True
        Me.rdoIdle.Checked = True
        Me.rdoIdle.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoIdle.Location = New System.Drawing.Point(226, 122)
        Me.rdoIdle.Name = "rdoIdle"
        Me.rdoIdle.Size = New System.Drawing.Size(53, 17)
        Me.rdoIdle.TabIndex = 100
        Me.rdoIdle.TabStop = True
        Me.rdoIdle.Text = "IDLE"
        Me.rdoIdle.UseVisualStyleBackColor = True
        '
        'rdoEnd
        '
        Me.rdoEnd.AutoSize = True
        Me.rdoEnd.Checked = True
        Me.rdoEnd.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoEnd.Location = New System.Drawing.Point(147, 122)
        Me.rdoEnd.Name = "rdoEnd"
        Me.rdoEnd.Size = New System.Drawing.Size(51, 17)
        Me.rdoEnd.TabIndex = 99
        Me.rdoEnd.TabStop = True
        Me.rdoEnd.Text = "END"
        Me.rdoEnd.UseVisualStyleBackColor = True
        '
        'rdoConRejBusy
        '
        Me.rdoConRejBusy.AutoSize = True
        Me.rdoConRejBusy.Checked = True
        Me.rdoConRejBusy.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoConRejBusy.Location = New System.Drawing.Point(147, 65)
        Me.rdoConRejBusy.Name = "rdoConRejBusy"
        Me.rdoConRejBusy.Size = New System.Drawing.Size(107, 17)
        Me.rdoConRejBusy.TabIndex = 98
        Me.rdoConRejBusy.TabStop = True
        Me.rdoConRejBusy.Text = "CONREJBUSY"
        Me.rdoConRejBusy.UseVisualStyleBackColor = True
        '
        'rdoConAck2000
        '
        Me.rdoConAck2000.AutoSize = True
        Me.rdoConAck2000.Checked = True
        Me.rdoConAck2000.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoConAck2000.Location = New System.Drawing.Point(530, 42)
        Me.rdoConAck2000.Name = "rdoConAck2000"
        Me.rdoConAck2000.Size = New System.Drawing.Size(118, 17)
        Me.rdoConAck2000.TabIndex = 97
        Me.rdoConAck2000.TabStop = True
        Me.rdoConAck2000.Text = "CONACK2000+T"
        Me.rdoConAck2000.UseVisualStyleBackColor = True
        '
        'rdoConAck1000
        '
        Me.rdoConAck1000.AutoSize = True
        Me.rdoConAck1000.Checked = True
        Me.rdoConAck1000.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoConAck1000.Location = New System.Drawing.Point(403, 42)
        Me.rdoConAck1000.Name = "rdoConAck1000"
        Me.rdoConAck1000.Size = New System.Drawing.Size(118, 17)
        Me.rdoConAck1000.TabIndex = 96
        Me.rdoConAck1000.TabStop = True
        Me.rdoConAck1000.Text = "CONACK1000+T"
        Me.rdoConAck1000.UseVisualStyleBackColor = True
        '
        'rdoConAck200
        '
        Me.rdoConAck200.AutoSize = True
        Me.rdoConAck200.Checked = True
        Me.rdoConAck200.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoConAck200.Location = New System.Drawing.Point(147, 42)
        Me.rdoConAck200.Name = "rdoConAck200"
        Me.rdoConAck200.Size = New System.Drawing.Size(111, 17)
        Me.rdoConAck200.TabIndex = 95
        Me.rdoConAck200.TabStop = True
        Me.rdoConAck200.Text = "CONACK200+T"
        Me.rdoConAck200.UseVisualStyleBackColor = True
        '
        'rdoConReq2000
        '
        Me.rdoConReq2000.AutoSize = True
        Me.rdoConReq2000.Checked = True
        Me.rdoConReq2000.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoConReq2000.Location = New System.Drawing.Point(530, 19)
        Me.rdoConReq2000.Name = "rdoConReq2000"
        Me.rdoConReq2000.Size = New System.Drawing.Size(105, 17)
        Me.rdoConReq2000.TabIndex = 94
        Me.rdoConReq2000.TabStop = True
        Me.rdoConReq2000.Text = "CONREQ2000"
        Me.rdoConReq2000.UseVisualStyleBackColor = True
        '
        'rdoConReq1000
        '
        Me.rdoConReq1000.AutoSize = True
        Me.rdoConReq1000.Checked = True
        Me.rdoConReq1000.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoConReq1000.Location = New System.Drawing.Point(403, 19)
        Me.rdoConReq1000.Name = "rdoConReq1000"
        Me.rdoConReq1000.Size = New System.Drawing.Size(105, 17)
        Me.rdoConReq1000.TabIndex = 93
        Me.rdoConReq1000.TabStop = True
        Me.rdoConReq1000.Text = "CONREQ1000"
        Me.rdoConReq1000.UseVisualStyleBackColor = True
        '
        'rdoCONReq500
        '
        Me.rdoCONReq500.AutoSize = True
        Me.rdoCONReq500.Checked = True
        Me.rdoCONReq500.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoCONReq500.Location = New System.Drawing.Point(270, 19)
        Me.rdoCONReq500.Name = "rdoCONReq500"
        Me.rdoCONReq500.Size = New System.Drawing.Size(98, 17)
        Me.rdoCONReq500.TabIndex = 92
        Me.rdoCONReq500.TabStop = True
        Me.rdoCONReq500.Text = "CONREQ500"
        Me.rdoCONReq500.UseVisualStyleBackColor = True
        '
        'rdo4PSK2000_960
        '
        Me.rdo4PSK2000_960.AutoSize = True
        Me.rdo4PSK2000_960.Checked = True
        Me.rdo4PSK2000_960.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK2000_960.Location = New System.Drawing.Point(370, 396)
        Me.rdo4PSK2000_960.Name = "rdo4PSK2000_960"
        Me.rdo4PSK2000_960.Size = New System.Drawing.Size(180, 17)
        Me.rdo4PSK2000_960.TabIndex = 91
        Me.rdo4PSK2000_960.TabStop = True
        Me.rdo4PSK2000_960.Text = "8Car4PSK,167bd_960Bytes"
        Me.rdo4PSK2000_960.UseVisualStyleBackColor = True
        '
        'rdo4PSK1000_480
        '
        Me.rdo4PSK1000_480.AutoSize = True
        Me.rdo4PSK1000_480.Checked = True
        Me.rdo4PSK1000_480.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK1000_480.Location = New System.Drawing.Point(6, 398)
        Me.rdo4PSK1000_480.Name = "rdo4PSK1000_480"
        Me.rdo4PSK1000_480.Size = New System.Drawing.Size(180, 17)
        Me.rdo4PSK1000_480.TabIndex = 90
        Me.rdo4PSK1000_480.TabStop = True
        Me.rdo4PSK1000_480.Text = "4Car4PSK,167bd_480Bytes"
        Me.rdo4PSK1000_480.UseVisualStyleBackColor = True
        '
        'rdo4PSK500_240
        '
        Me.rdo4PSK500_240.AutoSize = True
        Me.rdo4PSK500_240.Checked = True
        Me.rdo4PSK500_240.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdo4PSK500_240.Location = New System.Drawing.Point(370, 194)
        Me.rdo4PSK500_240.Name = "rdo4PSK500_240"
        Me.rdo4PSK500_240.Size = New System.Drawing.Size(180, 17)
        Me.rdo4PSK500_240.TabIndex = 89
        Me.rdo4PSK500_240.TabStop = True
        Me.rdo4PSK500_240.Text = "2Car4PSK,167bd_240Bytes"
        Me.rdo4PSK500_240.UseVisualStyleBackColor = True
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.bttFECAbort)
        Me.GroupBox1.Controls.Add(Me.lblFECInfo)
        Me.GroupBox1.Controls.Add(Me.btnFECTest)
        Me.GroupBox1.Location = New System.Drawing.Point(22, 622)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Size = New System.Drawing.Size(630, 65)
        Me.GroupBox1.TabIndex = 21
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "Basic Protocol Tests"
        '
        'bttFECAbort
        '
        Me.bttFECAbort.Location = New System.Drawing.Point(161, 19)
        Me.bttFECAbort.Name = "bttFECAbort"
        Me.bttFECAbort.Size = New System.Drawing.Size(76, 23)
        Me.bttFECAbort.TabIndex = 89
        Me.bttFECAbort.Text = "FEC  Abort "
        Me.bttFECAbort.UseVisualStyleBackColor = True
        '
        'lblFECInfo
        '
        Me.lblFECInfo.AutoSize = True
        Me.lblFECInfo.Location = New System.Drawing.Point(13, 49)
        Me.lblFECInfo.Name = "lblFECInfo"
        Me.lblFECInfo.Size = New System.Drawing.Size(48, 13)
        Me.lblFECInfo.TabIndex = 88
        Me.lblFECInfo.Text = "FEC Info"
        '
        'btnFECTest
        '
        Me.btnFECTest.Location = New System.Drawing.Point(12, 19)
        Me.btnFECTest.Name = "btnFECTest"
        Me.btnFECTest.Size = New System.Drawing.Size(142, 23)
        Me.btnFECTest.TabIndex = 39
        Me.btnFECTest.Text = "FEC  multicast with ID "
        Me.btnFECTest.UseVisualStyleBackColor = True
        '
        'rdoSounding
        '
        Me.rdoSounding.AutoSize = True
        Me.rdoSounding.Checked = True
        Me.rdoSounding.Font = New System.Drawing.Font("Microsoft Sans Serif", 8.25!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.rdoSounding.Location = New System.Drawing.Point(3, 529)
        Me.rdoSounding.Name = "rdoSounding"
        Me.rdoSounding.Size = New System.Drawing.Size(116, 17)
        Me.rdoSounding.TabIndex = 125
        Me.rdoSounding.TabStop = True
        Me.rdoSounding.Text = "2KHz Sounding "
        Me.rdoSounding.UseVisualStyleBackColor = True
        '
        'Test
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(666, 695)
        Me.Controls.Add(Me.GroupBox1)
        Me.Controls.Add(Me.GroupBox2)
        Me.MaximizeBox = False
        Me.Name = "Test"
        Me.Text = "Frame Test (not used in normal opearation)"
        CType(Me.nudRepeats, System.ComponentModel.ISupportInitialize).EndInit()
        Me.GroupBox2.ResumeLayout(False)
        Me.GroupBox2.PerformLayout()
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        Me.ResumeLayout(False)

    End Sub
    Friend WithEvents rdoDataACK As System.Windows.Forms.RadioButton
    Friend WithEvents Button1 As System.Windows.Forms.Button
    Friend WithEvents nudRepeats As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents rdoDataNAK As System.Windows.Forms.RadioButton
    Friend WithEvents rdoCONREQ200 As System.Windows.Forms.RadioButton
    Friend WithEvents lblCount As System.Windows.Forms.Label
    Friend WithEvents rdoIDFrame As System.Windows.Forms.RadioButton
    Friend WithEvents rdoBreak As System.Windows.Forms.RadioButton
    Friend WithEvents rdoConAck500 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4PSK200_16 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4PSK200_64 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo8PSK200_108 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4PSK500_128 As System.Windows.Forms.RadioButton
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents rdo4PSK1000_256 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4PSK2000_512 As System.Windows.Forms.RadioButton
    Friend WithEvents GroupBox2 As System.Windows.Forms.GroupBox
    Friend WithEvents rdo4PSK500_240 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4PSK2000_960 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4PSK1000_480 As System.Windows.Forms.RadioButton
    Friend WithEvents rdoDisc As System.Windows.Forms.RadioButton
    Friend WithEvents rdoIdle As System.Windows.Forms.RadioButton
    Friend WithEvents rdoEnd As System.Windows.Forms.RadioButton
    Friend WithEvents rdoConRejBusy As System.Windows.Forms.RadioButton
    Friend WithEvents rdoConAck2000 As System.Windows.Forms.RadioButton
    Friend WithEvents rdoConAck1000 As System.Windows.Forms.RadioButton
    Friend WithEvents rdoConAck200 As System.Windows.Forms.RadioButton
    Friend WithEvents rdoConReq2000 As System.Windows.Forms.RadioButton
    Friend WithEvents rdoConReq1000 As System.Windows.Forms.RadioButton
    Friend WithEvents rdoCONReq500 As System.Windows.Forms.RadioButton
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents rdo8PSK500_216 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo8PSK2000_864 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo8PSK1000_432 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo8PSK2000_1272 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo8PSK500_318 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo8PSK1000_636 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK200_32 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK200_16 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK500_64 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK500_32 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK2000_256 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK1000_128 As System.Windows.Forms.RadioButton
    Friend WithEvents GroupBox1 As System.Windows.Forms.GroupBox
    Friend WithEvents btnFECTest As System.Windows.Forms.Button
    Friend WithEvents lblFECInfo As System.Windows.Forms.Label
    Friend WithEvents bttFECAbort As System.Windows.Forms.Button
    Friend WithEvents rdo16FSK500_16 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo16FSK500_32 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK2000_200 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo4FSK2000_600 As System.Windows.Forms.RadioButton
    Friend WithEvents rdo8FSK200_24 As System.Windows.Forms.RadioButton
    Friend WithEvents rdoSounding As System.Windows.Forms.RadioButton
End Class
