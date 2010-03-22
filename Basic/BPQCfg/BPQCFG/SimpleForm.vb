Public Class SimpleForm

    Dim mainMenu1 As New MainMenu()
    Dim topMenuItem As New MenuItem()
    Dim menuItem1 As New MenuItem()
    Dim menuItem2 As New MenuItem()
    Dim menuItem3 As New MenuItem()
    Dim menuItem4 As New MenuItem()
    Dim menuItem5 As New MenuItem()
    Dim menuItem6 As New MenuItem()

    Dim menuItem7 As New MenuItem()

    Public WINMORPort(16) As NumTextBox
    Public WINMORPTT(16) As ComboBox
    Public PTTCOMM(16) As ComboBox

    Public DriveLevel(16) As BPQCFG.NumTextBox
    Public BW(16) As ComboBox
    Public DebugLog(16) As CheckBox
    Public CWID(16) As CheckBox
    Public BusyLock(16) As CheckBox
    Public MyLabel1(16) As Label
    Public MyLabel2(16) As Label
    Public MyLabel3(16) As Label
    Public MyLabel4(16) As Label
    Public IDBox(16) As TextBox
    Public IDLabel(16) As Label


    Public BAUD(16) As System.Windows.Forms.ComboBox
    Public COMM(16) As System.Windows.Forms.ComboBox

    Public PORTTABS(16) As System.Windows.Forms.TabPage

    Dim RadioPort As Integer
    Dim NumberofWinmorPorts As Integer = 0


    Private Sub SimpleForm_FormClosing(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing

        SavePortInfo()

        Form1.CopyConfigtoArray()

        If CompareConfig() = False Or AGWAppl <> OriginalAGWAppl Then

            If MsgBox("Changes have not been saved - do you want to save before exiting?", _
                     MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then
                If Not Form1.SaveConfigAsBinary() Then
                    e.Cancel = True
                Else
                    Form1.Close()
                End If
            Else
                Form1.Close()
            End If
        End If
        '
    End Sub



    Private Sub SimpleForm_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

        Dim errmsg As String

        errmsg = Space(256)

        ' Set the caption of the menu items.

        topMenuItem.Text = "&File"
        menuItem1.Text = "&Open"
        menuItem2.Text = "&Validate"
        menuItem3.Text = "&Save"
        menuItem4.Text = "Switch to &Advanced Configuration Mode"
        menuItem5.Text = "&Exit"

        menuItem3.Enabled = False

        ' Add the menu items to the main menu.
        topMenuItem.MenuItems.Add(menuItem1)
        topMenuItem.MenuItems.Add(menuItem2)
        topMenuItem.MenuItems.Add(menuItem3)
        topMenuItem.MenuItems.Add(menuItem4)
        topMenuItem.MenuItems.Add(menuItem5)

        mainMenu1.MenuItems.Add(topMenuItem)

        ' Add functionality to the menu items using the Click event. 

        AddHandler menuItem1.Click, AddressOf Form1.Open_Click
        AddHandler menuItem2.Click, AddressOf Form1.Validate_Click
        AddHandler menuItem3.Click, AddressOf Form1.Save_Binary
        AddHandler menuItem4.Click, AddressOf Me.Switch_To_Advanced
        AddHandler menuItem7.Click, AddressOf Exit_Click
        ' Assign mainMenu1 to the form.
        '    Me.Menu = mainMenu1


    End Sub

    Private Sub Switch_To_Advanced(ByVal sender As Object, ByVal e As System.EventArgs)

        Me.Visible = False
        Form1.Visible = True
        Return
    End Sub

    Private Sub SimpleForm_QueryContinueDrag(ByVal sender As Object, ByVal e As System.Windows.Forms.QueryContinueDragEventArgs) Handles Me.QueryContinueDrag

    End Sub

    Private Sub SimpleForm_Resize(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Resize

        TabControl1.Width = Me.Width - 25
        TabControl1.Height = Me.Height - 420

    End Sub


    Private Sub LoadConfig_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LoadConfig.Click

        Dim Port As Integer
        Dim NonSimplePorts As Integer = 0
        Dim NumberofWinmorPorts As Integer = 0

        IDMsgLabel.Visible = False
        IDIntLabel.Visible = False
        IDMsgBox.Visible = False
        IDIntervalBox.Visible = False

        While TabControl1.Controls.Count
            TabControl1.Controls.RemoveAt(0)
        End While

        TabControl1.Visible = True

        Form1.GetConfigFile()

        NodeCallBox.Text = Form1.NodeCallBox.Text
        InfoMsgBox.Text = Form1.InfoMsgBox.Text
        CTEXTBox.Text = Form1.CTEXTBox.Text
        IDMsgBox.Text = Form1.IDMsgBox.Text
        IDIntervalBox.Text = Form1.IDIntervalBox.Text

        For Port = 1 To NumberOfPorts

            If TxtPortCfg(PORTTYPE).Value(Port) = "1" Then
                CreateLOOPBACKTab(Port)
            ElseIf TxtPortCfg(PORTTYPE).Value(Port) = "0" Then
                CreateKISSTab(Port)
                IDMsgLabel.Visible = True
                IDIntLabel.Visible = True
                IDMsgBox.Visible = True
                IDIntervalBox.Visible = True

            ElseIf TxtPortCfg(PORTTYPE).Value(Port) = "2" Then

                If UCase(TxtPortCfg(DLLNAME).Value(Port)).Contains("WINMOR") Then
                    CreateWINMORTab(Port)
                    NumberofWinmorPorts = NumberofWinmorPorts + 1

                ElseIf UCase(TxtPortCfg(DLLNAME).Value(Port)).Contains("PACTOR") Then
                    CreatePactorTab(Port)
                ElseIf UCase(TxtPortCfg(DLLNAME).Value(Port)).Contains("BPQtoAGW") Then
                    CreateAGWTab(Port)
                    IDMsgLabel.Visible = True
                    IDIntLabel.Visible = True
                    IDMsgBox.Visible = True
                    IDIntervalBox.Visible = True
                Else
                    CreateUnsupportedTab(Port)
                    NonSimplePorts = NonSimplePorts + 1
                End If
            Else
                NonSimplePorts = NonSimplePorts + 1
                CreateUnsupportedTab(Port)
            End If

        Next

        If NumberofWinmorPorts Then ReadWINMORCfg()

        AddPortButton.Enabled = True

        Return


        If NonSimplePorts Then

            If MsgBox("Configuration has ports not supported by Simple Mode - switching to Advanced COnfiguration Mode", _
    MsgBoxStyle.OkCancel + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Ok Then

                Me.Visible = False
                Form1.Visible = True
            Else
                End

            End If
        End If

    End Sub

    Private Sub ReadWINMORCfg()

        Dim Line As String
        Dim Textfile As System.IO.StreamReader
        Textfile = My.Computer.FileSystem.OpenTextFileReader(BPQDirectory & "\BPQtoWINMOR.cfg", System.Text.Encoding.ASCII)

        Do
            Line = Textfile.ReadLine
            ProcessLine(Line)

        Loop Until Textfile.EndOfStream

        Textfile.Close()

    End Sub


    Dim Port As Integer

    Sub ProcessLine(ByVal buf As String)

        Dim i As Integer

        Try


            buf = UCase(buf)

            If buf.StartsWith("PORT") Then

                Dim Fields() As String = Split(buf)

                Port = Fields(1)
                WINMORPort(Port).Text = Fields(3)

                If Fields(4) = "PTT" Then
                    WINMORPTT(Port).SelectedIndex = WINMORPTT(Port).FindStringExact(Fields(5))
                End If

                Return

            End If

            If buf.Contains("BUSYBLOCK") Then
                BusyLock(Port).Checked = buf.Contains("TRUE")
            ElseIf buf.Contains("CWID") Then
                CWID(Port).Checked = buf.Contains("TRUE")
            ElseIf buf.Contains("DEBUGLOG") Then
                DebugLog(Port).Checked = buf.Contains("TRUE")
            ElseIf buf.Contains("DRIVELEVEL") Then
                DriveLevel(Port).Text = Mid(buf, 12)
            ElseIf buf.Contains("BW ") Then
                BW(Port).SelectedIndex = BW(Port).FindStringExact(Mid(buf, 4))
            End If

        Catch ex As Exception

        End Try

    End Sub


    Private Sub Create_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Create.Click

        While TabControl1.Controls.Count
            TabControl1.Controls.RemoveAt(0)
        End While

        IDMsgLabel.Visible = False
        IDIntLabel.Visible = False
        IDMsgBox.Visible = False
        IDIntervalBox.Visible = False

        TabControl1.Visible = True

        NumberofWinmorPorts = 0

        Form1.CreateSimpleConfig()

        If NEWCALL.ShowDialog = Windows.Forms.DialogResult.OK Then

            AddPortButton.Enabled = True
            NodeCallBox.Text = NEWCALL.NodeCall.Text
            Form1.NodeCallBox.Text = NEWCALL.NodeCall.Text
            AddPort_Click(sender, e)

        End If
    End Sub


    Private Sub AddPort_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AddPortButton.Click

        If AddPort.ShowDialog = Windows.Forms.DialogResult.OK Then


            ReDim Preserve Config(Config.Length + 512)

            NumberOfPortsinConfig = Int((ConfigLength - 2560) / 512)

            NewConfig = True
            AddPortTab()
            NewConfig = False

            TxtPortCfg(PORTID).Value(NumberOfPorts) = AddPort.PORTID.Text

            Dim PortTypeString As String = AddPort.PortType.Text

            If PortTypeString = "WINMOR" Then

                CreateWINMORTab(NumberOfPorts)

                TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = 2  ' External
                TxtPortCfg(DLLNAME).Value(NumberOfPorts) = "WINMOR.dll"
                TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = 5
                WINMORPTT(NumberOfPorts).SelectedIndex = 0
                CWID(NumberOfPorts).Checked = False
                BW(NumberOfPorts).SelectedIndex = 1
                BusyLock(NumberOfPorts).Checked = True
                DebugLog(NumberOfPorts).Checked = True
                WINMORPort(NumberOfPorts).Text = 8500 + NumberofWinmorPorts * 2
                NumberofWinmorPorts = NumberofWinmorPorts + 1

            ElseIf PortTypeString.Contains("Pactor") Then

                CreatePactorTab(NumberOfPorts)

                Dim DLL As String = Mid(PortTypeString, 1, 3) & "Pactor.dll"

                TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = 2   ' External
                TxtPortCfg(DLLNAME).Value(NumberOfPorts) = DLL
                TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = 6

            ElseIf PortTypeString = "AX.25 KISS" Then

                CreateKISSTab(NumberOfPorts)
                TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = 0      ' KISS
                TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = 0
                SetDefaultL2Params(NumberOfPorts)
                IDMsgLabel.Visible = True
                IDIntLabel.Visible = True
                IDMsgBox.Visible = True
                IDIntervalBox.Visible = True

                SetDefaultL2Params(NumberOfPorts)


            ElseIf PortTypeString = "AX.25 AGWPE" Then

                TabControl1.Controls.Add(AX25AGW)
                TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = 2          ' Start with minimal (Internal)
                TxtPortCfg(DLLNAME).Value(NumberOfPorts) = "BPQtoAGW.dll"
                TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = 0
                IDMsgLabel.Visible = True
                IDIntLabel.Visible = True
                IDMsgBox.Visible = True
                IDIntervalBox.Visible = True

                SetDefaultL2Params(NumberOfPorts)

            ElseIf PortTypeString = "LOOPBACK" Then

                CreateLOOPBACKTab(NumberOfPorts)

                TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = 1  ' Internal
                TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = 0

                SetDefaultL2Params(NumberOfPorts)

            End If

            TabControl1.SelectedIndex = NumberOfPorts - 1

        End If

    End Sub

    Private Sub SetDefaultL2Params(ByVal NumberOfPorts As Integer)

        TxtPortCfg(FRACK).Value(NumberOfPorts) = 7000
        TxtPortCfg(RESPTIME).Value(NumberOfPorts) = 1000
        TxtPortCfg(RETRIES).Value(NumberOfPorts) = 10
        TxtPortCfg(PACLEN).Value(NumberOfPorts) = 236
        TxtPortCfg(MAXFRAME).Value(NumberOfPorts) = 4
        TxtPortCfg(CHANNEL).Value(NumberOfPorts) = "A"

    End Sub

    Private Sub CreateTab(ByVal Port As Integer)

        PORTTABS(Port) = New TabPage

        PORTTABS(Port).Location = New System.Drawing.Point(4, 22)
        PORTTABS(Port).Padding = New System.Windows.Forms.Padding(3)
        PORTTABS(Port).Size = New System.Drawing.Size(580, 110)
        PORTTABS(Port).TabIndex = Port - 1
        PORTTABS(Port).UseVisualStyleBackColor = True

        IDLabel(Port) = New Label
        IDBox(Port) = New TextBox
        '
        'IDLable
        '
        IDLabel(Port).Location = New System.Drawing.Point(21, 14)
        IDLabel(Port).Size = New System.Drawing.Size(18, 13)
        IDLabel(Port).Text = "ID"
        '
        'IDBOX
        '
        PORTTABS(Port).Controls.Add(IDLabel(Port))
        PORTTABS(Port).Controls.Add(IDBox(Port))

        IDBox(Port).Location = New System.Drawing.Point(68, 10)
        IDBox(Port).Name = "IDBOX"
        IDBox(Port).Text = TxtPortCfg(PORTID).Value(Port)
        IDBox(Port).Size = New System.Drawing.Size(266, 20)

        TabControl1.Controls.Add(PORTTABS(Port))

    End Sub

    Private Sub CreateWINMORTab(ByVal Port As Integer)

        CreateTab(Port)

        MyLabel1(Port) = New Label
        MyLabel2(Port) = New Label
        MyLabel3(Port) = New Label
        MyLabel4(Port) = New Label

        MyLabel1(Port).Location = New System.Drawing.Point(360, 41)
        MyLabel1(Port).Size = New System.Drawing.Size(28, 13)
        MyLabel1(Port).Text = "PTT"
        '
        MyLabel2(Port).Location = New System.Drawing.Point(161, 41)
        MyLabel2(Port).Size = New System.Drawing.Size(61, 13)
        MyLabel2(Port).Text = "Drive Level"
        '
        MyLabel3(Port).Location = New System.Drawing.Point(21, 41)
        MyLabel3(Port).Size = New System.Drawing.Size(90, 13)
        MyLabel3(Port).Text = "Winmor TNC Port"

        MyLabel4(Port).Location = New System.Drawing.Point(270, 41)
        MyLabel4(Port).Size = New System.Drawing.Size(28, 13)
        MyLabel4(Port).Text = "BW"

        PORTTABS(Port).Controls.Add(MyLabel1(Port))
        PORTTABS(Port).Controls.Add(MyLabel2(Port))
        PORTTABS(Port).Controls.Add(MyLabel3(Port))
        PORTTABS(Port).Controls.Add(MyLabel4(Port))

        PORTTABS(Port).Name = "WINMOR"
        PORTTABS(Port).Text = "WINMOR"

        WINMORPTT(Port) = New System.Windows.Forms.ComboBox
        DriveLevel(Port) = New BPQCFG.NumTextBox(100)
        WINMORPort(Port) = New BPQCFG.NumTextBox(65535)
        CWID(Port) = New System.Windows.Forms.CheckBox
        BusyLock(Port) = New System.Windows.Forms.CheckBox
        DebugLog(Port) = New System.Windows.Forms.CheckBox
        BW(Port) = New System.Windows.Forms.ComboBox
        PTTCOMM(Port) = New System.Windows.Forms.ComboBox

        PORTTABS(Port).Controls.Add(Me.WINMORPTT(Port))
        PORTTABS(Port).Controls.Add(Me.DriveLevel(Port))
        PORTTABS(Port).Controls.Add(Me.WINMORPort(Port))
        PORTTABS(Port).Controls.Add(Me.CWID(Port))
        PORTTABS(Port).Controls.Add(Me.BusyLock(Port))
        PORTTABS(Port).Controls.Add(Me.DebugLog(Port))
        PORTTABS(Port).Controls.Add(Me.BW(Port))
        PORTTABS(Port).Controls.Add(Me.PTTCOMM(Port))

        'DriveLevel
        '
        Me.DriveLevel(Port).Location = New System.Drawing.Point(223, 37)
        Me.DriveLevel(Port).Name = "DriveLevel"
        Me.DriveLevel(Port).Size = New System.Drawing.Size(39, 20)
        Me.DriveLevel(Port).Text = "100"
        '
        'WinmorPort
        '
        Me.WINMORPort(Port).Location = New System.Drawing.Point(118, 37)
        Me.WINMORPort(Port).Name = "WinmorPort"
        Me.WINMORPort(Port).Size = New System.Drawing.Size(39, 20)
        Me.WINMORPort(Port).Text = "8500"
        '
        'WINMORPTT
        '
        Me.WINMORPTT(Port).DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.WINMORPTT(Port).FormattingEnabled = True
        Me.WINMORPTT(Port).Items.AddRange(New Object() {"VOX", "CI-V", "RTS", "DTR", "DTRRTS"})
        Me.WINMORPTT(Port).Location = New System.Drawing.Point(390, 37)
        Me.WINMORPTT(Port).Name = Port
        Me.WINMORPTT(Port).Size = New System.Drawing.Size(75, 21)
        Me.WINMORPTT(Port).TabIndex = 55

        AddHandler WINMORPTT(Port).SelectedIndexChanged, AddressOf PTTChanged

        PTTCOMM(Port).Location = New System.Drawing.Point(475, 37)
        PTTCOMM(Port).Size = New System.Drawing.Size(70, 21)

        For Each sp As String In My.Computer.Ports.SerialPortNames
            PTTCOMM(Port).Items.Add(sp)
        Next

        BW(Port).DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        BW(Port).FormattingEnabled = True
        BW(Port).Items.AddRange(New Object() {"500", "1600"})
        BW(Port).Location = New System.Drawing.Point(300, 37)
        BW(Port).Size = New System.Drawing.Size(55, 21)
        '
        'CWID
        '
        Me.CWID(Port).Location = New System.Drawing.Point(198, 68)
        Me.CWID(Port).Name = "CWID"
        Me.CWID(Port).Size = New System.Drawing.Size(85, 17)
        Me.CWID(Port).Text = "Enble CWID"
        '
        'BusyLock
        '
        Me.BusyLock(Port).AutoSize = True
        Me.BusyLock(Port).Location = New System.Drawing.Point(109, 68)
        Me.BusyLock(Port).Name = "BusyLock"
        Me.BusyLock(Port).Size = New System.Drawing.Size(76, 17)
        Me.BusyLock(Port).TabIndex = 50
        Me.BusyLock(Port).Text = "Busy Lock"
        Me.BusyLock(Port).UseVisualStyleBackColor = True
        '
        'DebugLog
        '
        Me.DebugLog(Port).AutoSize = True
        Me.DebugLog(Port).Location = New System.Drawing.Point(24, 68)
        Me.DebugLog(Port).Name = "DebugLog"
        Me.DebugLog(Port).Size = New System.Drawing.Size(79, 17)
        Me.DebugLog(Port).TabIndex = 49
        Me.DebugLog(Port).Text = "Debug Log"
        Me.DebugLog(Port).UseVisualStyleBackColor = True

    End Sub


    Private Sub CreatePactorTab(ByVal Port As Integer)

        CreateTab(Port)

        PORTTABS(Port).Name = "Pactor"
        PORTTABS(Port).Text = "Pactor"

        CreateCOMPortAndSpeed(Port)

    End Sub


    Private Sub CreateCOMPortAndSpeed(ByVal Port As Integer)

        Dim FindString As String

        MyLabel1(Port) = New Label
        MyLabel2(Port) = New Label
        '
        'Label14
        '
        MyLabel1(Port).Location = New System.Drawing.Point(187, 47)
        MyLabel1(Port).Size = New System.Drawing.Size(38, 13)
        MyLabel1(Port).Text = "Speed"
        '
        'Label15
        '
        MyLabel2(Port).Location = New System.Drawing.Point(8, 47)
        MyLabel2(Port).Size = New System.Drawing.Size(53, 13)
        MyLabel2(Port).Text = "COM Port"
        '
        Me.BAUD(Port) = New System.Windows.Forms.ComboBox
        Me.COMM(Port) = New System.Windows.Forms.ComboBox
        '
        'BAUD
        '
        Me.BAUD(Port).FormattingEnabled = True
        Me.BAUD(Port).Location = New System.Drawing.Point(237, 45)
        Me.BAUD(Port).Name = "BAUD"
        Me.BAUD(Port).Size = New System.Drawing.Size(109, 21)
        Me.BAUD(Port).TabIndex = 25
        '
        'COM
        '
        Me.COMM(Port).FormattingEnabled = True
        Me.COMM(Port).Location = New System.Drawing.Point(67, 45)
        Me.COMM(Port).Name = "COM"
        Me.COMM(Port).Size = New System.Drawing.Size(109, 21)
        Me.COMM(Port).TabIndex = 24

        PORTTABS(Port).Controls.Add(MyLabel1(Port))
        PORTTABS(Port).Controls.Add(MyLabel2(Port))
        PORTTABS(Port).Controls.Add(Me.BAUD(Port))
        PORTTABS(Port).Controls.Add(Me.COMM(Port))

        For Each sp As String In My.Computer.Ports.SerialPortNames
            COMM(Port).Items.Add(sp)
        Next

        FindString = "COM" & TxtPortCfg(IOADDR).Value(Port)

        COMM(Port).SelectedIndex = COMM(Port).FindStringExact(FindString)

        BAUD(Port).Items.Add("38400")
        BAUD(Port).Items.Add("19200")
        BAUD(Port).Items.Add("9600")
        BAUD(Port).Items.Add("4800")
        BAUD(Port).Items.Add("2400")
        BAUD(Port).Items.Add("1200")

        FindString = TxtPortCfg(SPEED).Value(Port)

        BAUD(Port).SelectedIndex = BAUD(Port).FindStringExact(FindString)

    End Sub

    Private Sub CreateKISSTab(ByVal Port As Integer)

        CreateTab(Port)

        PORTTABS(Port).Name = "KISS"
        PORTTABS(Port).Text = "KISS"

        CreateCOMPortAndSpeed(Port)

    End Sub

    Private Sub CreateLOOPBACKTab(ByVal Port As Integer)

        CreateTab(Port)

        '      PORTTABS(Port).Controls.Add(Me.Label12)
        '      PORTTABS(Port).Controls.Add(Me.Label5)
        '      PORTTABS(Port).Controls.Add(Me.Label4)

        PORTTABS(Port).Name = "LOOPBACK"
        PORTTABS(Port).Text = "LOOPBACK"

    End Sub

    Private Sub CreateAGWTab(ByVal Port As Integer)

        CreateTab(Port)

        '       PORTTABS(Port).Controls.Add(Me.Label12)
        '       PORTTABS(Port).Controls.Add(Me.Label5)
        '       PORTTABS(Port).Controls.Add(Me.Label4)
        PORTTABS(Port).Location = New System.Drawing.Point(4, 22)
        PORTTABS(Port).Name = "AGWPEPort"
        PORTTABS(Port).Padding = New System.Windows.Forms.Padding(3)
        PORTTABS(Port).Size = New System.Drawing.Size(641, 110)
        PORTTABS(Port).TabIndex = Port - 1
        PORTTABS(Port).Text = "AGWPE Port"
        PORTTABS(Port).UseVisualStyleBackColor = True

    End Sub

    Private Sub CreateUnsupportedTab(ByVal Port As Integer)

        CreateTab(Port)

        PORTTABS(Port).Name = "NonSimple"
        PORTTABS(Port).Text = "NonSimple"

    End Sub


    Private Sub CreateFiles_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CreateFiles.Click

        Form1.NodeCallBox.Text = NodeCallBox.Text
        Form1.InfoMsgBox.Text = InfoMsgBox.Text
        Form1.CTEXTBox.Text = CTEXTBox.Text
        Form1.IDMsgBox.Text = IDMsgBox.Text
        Form1.IDIntervalBox.Text = IDIntervalBox.Text

        Form1.SaveConfigAsBinary()

        ' If and WINMOR ports defined. create BPQtoWINMOR.cfg
        Dim i As Integer
        Dim GotWINMOR As Boolean
        Dim File As System.IO.StreamWriter
        Dim Line As String

        For i = 1 To NumberOfPorts
            If TxtPortCfg(DLLNAME).Value(i) = "WINMOR.dll" Then
                GotWINMOR = True
                Exit For
            End If
        Next

        If GotWINMOR = False Then Return


        File = My.Computer.FileSystem.OpenTextFileWriter(BPQDirectory & "\xBPQtoWINMOR.cfg", False, System.Text.Encoding.ASCII)

        File.WriteLine("#	Config file for WINMOR.dll Created by WinBPQCfg")
        File.WriteLine("")

        For i = 1 To NumberOfPorts
            If TxtPortCfg(DLLNAME).Value(i) = "WINMOR.dll" Then

                Line = "PORT " & i & " 127.0.0.1 " & WINMORPort(i).Text & " PTT " & WINMORPTT(i).Text
                File.WriteLine(Line)


                File.WriteLine("LOG True")
                Line = "DebugLog " & DebugLog(i).Checked
                File.WriteLine(Line)
                Line = "CWID " & CWID(i).Checked
                File.WriteLine(Line)
                Line = "BUSYLOCK " & BusyLock(i).Checked
                File.WriteLine(Line)
                Line = "DRIVELEVEL " & DriveLevel(i).Text
                File.WriteLine(Line)
                Line = "BW " & BW(i).Text

                File.WriteLine(Line)

                File.WriteLine("ROBUST False")
                File.WriteLine("MODE AUTO")
                File.WriteLine("FECRCV True")
                File.WriteLine("****")
                File.WriteLine("")

            End If
        Next

        File.Close()

        'PORT 4 127.0.0.1 8506 PTT CI-V
        'CAPTURE BT SOFTPHONE USB HEADSET-02
        'PLAYBACK BT SOFTPHONE USB HEADSET-02
        '       CWID True
        '      DebugLog True
        '     
        '    BW 1600 
        '   DriveLevel 100
        '  
        '  
        'Show True 
        ' BusyLock True 
        ' 
        '




    End Sub


    Private Sub Button2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button2.Click

        Form1.NodeCallBox.Text = NodeCallBox.Text
        Form1.InfoMsgBox.Text = InfoMsgBox.Text
        Form1.CTEXTBox.Text = CTEXTBox.Text
        Form1.IDMsgBox.Text = IDMsgBox.Text
        Form1.IDIntervalBox.Text = IDIntervalBox.Text

        Me.Visible = False
        Form1.Visible = True

        Return

    End Sub

    Private Sub PTTChanged(ByVal sender As Object, ByVal e As System.EventArgs)
        Port = sender.Name

        If sender.selectedindex = 0 Then
            PTTCOMM(Port).Visible = False
        Else
            PTTCOMM(Port).Visible = True
            If PTTCOMM(Port).SelectedIndex = -1 Then PTTCOMM(Port).SelectedIndex = 0
        End If

    End Sub
End Class