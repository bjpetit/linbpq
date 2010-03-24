Imports System.IO
Imports Microsoft.VisualBasic.FileIO

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

    Public AGWLabel(16) As Label
    Public ChanLabel(16) As Label
    Public ChannelBox(16) As ComboBox
    Public AGWPortBox(16) As BPQCFG.DTNumTextBox

    Public SaveProc(16) As SavePortValue

    Public PORTTABS(16) As System.Windows.Forms.TabPage

    Dim RadioPort As Integer
    Dim NumberofWinmorPorts As Integer = 0
    Dim CanCreateRigControl As Boolean
    '    Dim NeedRigControl As Boolean = False


    Private Sub SimpleForm_FormClosing(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing

        SavePortInfo()

        Form1.CopyConfigtoArray()

        If CompareConfig() = False Or AGWAppl <> OriginalAGWAppl Then

            If MsgBox("Changes have not been saved - do you want to save before exiting?", _
                     MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then
                If Not Form1.SaveConfigAsBinary() Then
                    e.Cancel = True
                    Return

                End If
            End If
        End If

        DontAskBeforeClose = True  ' So Form1.colsing doesn't ask again
        Form1.Close()

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
        'AddHandler menuItem4.Click, AddressOf Me.Switch_To_Advanced
        AddHandler menuItem7.Click, AddressOf Exit_Click
        ' Assign mainMenu1 to the form.
        '    Me.Menu = mainMenu1


    End Sub


    Private Sub SimpleForm_QueryContinueDrag(ByVal sender As Object, ByVal e As System.Windows.Forms.QueryContinueDragEventArgs) Handles Me.QueryContinueDrag

    End Sub

    Private Sub SimpleForm_Resize(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Resize

        '        TabControl1.Width = Me.Width - 25
        '   TabControl1.Height = Me.Height - 480

    End Sub


    Private Sub LoadConfig_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles LoadConfig.Click

        IDMsgLabel.Visible = False
        IDIntLabel.Visible = False
        IDMsgBox.Visible = False
        IDIntervalBox.Visible = False

        CfgFile = BPQDirectory & "\bpqcfg.bin"
 
        Form1.ReadConfig()
 
        If InStr(CfgFile, ".") > 0 Then CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

        My.Settings.CfgFileOpenName = CfgFile
        My.Settings.BinOrTextOpen = 1
        My.Settings.Save()

        CopyConfigtoSimple()

    End Sub

    Sub CopyConfigtoSimple()

        Dim Port As Integer
        Dim NonSimplePorts As Integer = 0
        Dim NumberofWinmorPorts As Integer = 0

        While TabControl1.Controls.Count
            TabControl1.Controls.RemoveAt(0)
        End While

        TabControl1.Visible = True

        NodeCallBox.Text = Form1.NodeCallBox.Text
        InfoMsgBox.Text = Form1.InfoMsgBox.Text
        CTEXTBox.Text = Form1.CTEXTBox.Text
        IDMsgBox.Text = Form1.IDMsgBox.Text
        IDIntervalBox.Text = Form1.IDIntervalBox.Text

        Appl1.Text = ApplName(1).Text
        Appl2.Text = ApplName(2).Text
        Appl3.Text = ApplName(3).Text
        Appl4.Text = ApplName(4).Text

        Appl1Call.Text = ApplCall(1).Text
        Appl2Call.Text = ApplCall(2).Text
        Appl3Call.Text = ApplCall(3).Text
        Appl4Call.Text = ApplCall(4).Text

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
                ElseIf UCase(TxtPortCfg(DLLNAME).Value(Port)).Contains("BPQTOAGW") Then
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
        CreateFiles.Enabled = True

    End Sub

    Private Sub ReadWINMORCfg()

        Dim Line As String
        Dim Textfile As System.IO.StreamReader

        Try

            Textfile = My.Computer.FileSystem.OpenTextFileReader(BPQDirectory & "\BPQtoWINMOR.cfg", System.Text.Encoding.ASCII)

            Do
                Line = Textfile.ReadLine
                ProcessLine(Line)

            Loop Until Textfile.EndOfStream

            Textfile.Close()

        Catch ex As Exception

        End Try

        ReadRigCfg()

    End Sub


    Dim Port As Integer

    Sub ProcessLine(ByVal buf As String)

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

    Private Sub ReadRigCfg()

        Dim Line As String
        Dim Textfile As System.IO.StreamReader

        Try
            Textfile = My.Computer.FileSystem.OpenTextFileReader(BPQDirectory & "\RigControl.cfg", System.Text.Encoding.ASCII)

            Do
                Line = Textfile.ReadLine
                ProcessRigLine(Line)

            Loop Until Textfile.EndOfStream

            Textfile.Close()

        Catch ex As Exception

        End Try

    End Sub

    Dim SaveCOM As String

    Sub ProcessRigLine(ByVal buf As String)

        '#COM20 19200 PTTONLY

        '# For PTT control, specify only a Name and a BPQ Port Number.

        '#RADIO IC7000 4
        '#RADIO IC706 5

        buf = UCase(buf)
        Dim Fields() As String = Split(buf)

        If buf.StartsWith("COM") Then

            SaveCOM = Fields(0)

            If Fields(2) <> "PTTONLY" Then CanCreateRigControl = False

            Return

        End If

        If buf.StartsWith("RADIO ") Then
            Port = Fields(2)
            If PTTCOMM(Port) IsNot Nothing Then     ' WINMOR Port
                PTTCOMM(Port).SelectedIndex = PTTCOMM(Port).FindStringExact(SaveCOM)
            End If
        End If


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

        ConfigLoaded = True

        If NEWCALL.ShowDialog = Windows.Forms.DialogResult.OK Then

            AddPortButton.Enabled = True
            CreateFiles.Enabled = True

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

        SaveProc(Port) = AddressOf SaveNothing

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

        SaveProc(Port) = AddressOf SavePactor

    End Sub

    Private Sub CreateChannelBox(ByVal Port As Integer, ByVal Column As Integer)

        ChannelBox(Port) = New ComboBox
        ChanLabel(Port) = New Label


        ChannelBox(Port).DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        ChannelBox(Port).Items.AddRange(New Object() {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O"})
        ChannelBox(Port).Location = New System.Drawing.Point(Column + 60, 45)
        ChannelBox(Port).Size = New System.Drawing.Size(37, 21)
        '
        ChanLabel(Port).Location = New System.Drawing.Point(Column, 48)
        ChanLabel(Port).Size = New System.Drawing.Size(46, 13)
        ChanLabel(Port).Text = "Channel"

        PORTTABS(Port).Controls.Add(ChanLabel(Port))
        PORTTABS(Port).Controls.Add(ChannelBox(Port))

        ChannelBox(Port).SelectedIndex = ChannelBox(Port).FindStringExact(TxtPortCfg(CHANNEL).Value(Port))

    End Sub
    Private Sub CreateCOMPortAndSpeed(ByVal Port As Integer)

        Dim FindString As String

        MyLabel1(Port) = New Label
        MyLabel2(Port) = New Label
        ChannelBox(Port) = New ComboBox
        ChanLabel(Port) = New Label

        MyLabel1(Port).Location = New System.Drawing.Point(187, 48)
        MyLabel1(Port).Size = New System.Drawing.Size(38, 13)
        MyLabel1(Port).Text = "Speed"
        '
        MyLabel2(Port).Location = New System.Drawing.Point(8, 48)
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

        CreateChannelBox(Port, 360)

        SaveProc(Port) = AddressOf SaveKISS



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

        PORTTABS(Port).Location = New System.Drawing.Point(4, 22)
        PORTTABS(Port).Name = "AGWPEPort"
        PORTTABS(Port).Padding = New System.Windows.Forms.Padding(3)
        PORTTABS(Port).Size = New System.Drawing.Size(641, 110)
        PORTTABS(Port).TabIndex = Port - 1
        PORTTABS(Port).Text = "AGWPE Port"
        PORTTABS(Port).UseVisualStyleBackColor = True

        AGWLabel(Port) = New Label
        AGWPortBox(Port) = New BPQCFG.DTNumTextBox


        'AGWLabel
        '
        AGWLabel(Port).Location = New System.Drawing.Point(19, 48)
        AGWLabel(Port).Size = New System.Drawing.Size(69, 13)
        AGWLabel(Port).Text = "AGWPE Port"
        '
        'AGWPortBox
        '
        AGWPortBox(Port).Location = New System.Drawing.Point(103, 45)
        AGWPortBox(Port).Max = 65535
        AGWPortBox(Port).Size = New System.Drawing.Size(45, 20)
        AGWPortBox(Port).Text = "0"

        PORTTABS(Port).Controls.Add(AGWPortBox(Port))
        PORTTABS(Port).Controls.Add(AGWLabel(Port))

        AGWPortBox(Port).Text = TxtPortCfg(IOADDR).Value(Port)

        CreateChannelBox(Port, 170)

        SaveProc(Port) = AddressOf SaveAGW

    End Sub

    Private Sub CreateUnsupportedTab(ByVal Port As Integer)

        CreateTab(Port)

        PORTTABS(Port).Name = "NonSimple"
        PORTTABS(Port).Text = "NonSimple"

    End Sub


    Private Sub CreateFiles_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CreateFiles.Click


        If MsgBox("This will update your bpqcfg.bin and your BPQtoWINMOR.cfg and RigControl.cfg (if you have WINMOR port(s) defined)" & vbCrLf & _
        "Two generations of backup are created, with extensions of .save and .old" & vbCrLf & _
        "Do you want to continue ?", _
         MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.No Then Return


        Form1.NodeCallBox.Text = NodeCallBox.Text
        Form1.InfoMsgBox.Text = InfoMsgBox.Text
        Form1.CTEXTBox.Text = CTEXTBox.Text
        Form1.IDMsgBox.Text = IDMsgBox.Text
        Form1.IDIntervalBox.Text = IDIntervalBox.Text

        ApplName(1).Text = Appl1.Text
        ApplName(2).Text = Appl2.Text
        ApplName(3).Text = Appl3.Text
        ApplName(4).Text = Appl4.Text

        ApplCall(1).Text = Appl1Call.Text
        ApplCall(2).Text = Appl2Call.Text
        ApplCall(3).Text = Appl3Call.Text
        ApplCall(4).Text = Appl4Call.Text

        Dim Port As Integer = TabControl1.SelectedIndex + 1

        If Port Then

            TxtPortCfg(PORTID).Value(Port) = IDBox(Port).Text
            SaveProc(Port).Invoke(Port)

        End If

        SaveConfig()

        ' If any WINMOR ports defined. create BPQtoWINMOR.cfg
        Dim i As Integer
        Dim GotWINMOR As Boolean
        Dim NeedRigControl As Boolean = False

        Dim OutFile As System.IO.StreamWriter
        Dim Line As String

        For i = 1 To NumberOfPorts
            If UCase(TxtPortCfg(DLLNAME).Value(i)) = "WINMOR.DLL" Then
                GotWINMOR = True
                Exit For
            End If
        Next

        If GotWINMOR = False Then Return

        Dim FileName As String = BPQDirectory & "\BPQtoWINMOR.cfg"

        Try
            File.Copy(FileName & ".save", FileName & ".old", True)
        Catch ex As Exception
        End Try
        Try
            File.Copy(FileName, FileName & ".save", True)
        Catch ex As Exception
        End Try

        OutFile = My.Computer.FileSystem.OpenTextFileWriter(FileName, False, System.Text.Encoding.ASCII)

        OutFile.WriteLine("#	Config file for WINMOR.dll Created by WinBPQCfg")
        OutFile.WriteLine("")

        For i = 1 To NumberOfPorts
            If UCase(TxtPortCfg(DLLNAME).Value(i)) = "WINMOR.DLL" Then

                Line = "PORT " & i & " 127.0.0.1 " & WINMORPort(i).Text & " PTT " & WINMORPTT(i).Text
                OutFile.WriteLine(Line)

                OutFile.WriteLine("LOG True")
                Line = "DebugLog " & DebugLog(i).Checked
                OutFile.WriteLine(Line)
                Line = "CWID " & CWID(i).Checked
                OutFile.WriteLine(Line)
                Line = "BUSYLOCK " & BusyLock(i).Checked
                OutFile.WriteLine(Line)
                Line = "DRIVELEVEL " & DriveLevel(i).Text
                OutFile.WriteLine(Line)
                Line = "BW " & BW(i).Text
                OutFile.WriteLine(Line)

                OutFile.WriteLine("ROBUST False")
                OutFile.WriteLine("MODE AUTO")
                OutFile.WriteLine("FECRCV True")

                If WINMORPTT(i).Text <> "VOX" Then
                    NeedRigControl = True
                    OutFile.WriteLine("VOX False")
                End If

                OutFile.WriteLine("****")
                OutFile.WriteLine("")

            End If
        Next

        OutFile.Close()

        If NeedRigControl Then

            If CanCreateRigControl Then


                Dim LastCOM As String = ""

                FileName = BPQDirectory & "\RigControl.cfg"

                Try
                    File.Copy(FileName & ".save", FileName & ".old", True)
                Catch ex As Exception
                End Try
                Try
                    File.Copy(FileName, FileName & ".save", True)
                Catch ex As Exception
                End Try

                OutFile = My.Computer.FileSystem.OpenTextFileWriter(FileName, False, System.Text.Encoding.ASCII)

                OutFile.WriteLine("#	Rig Control file for WINMOR PTT Created by WinBPQCfg")
                OutFile.WriteLine("")

                For i = 1 To NumberOfPorts

                    If UCase(TxtPortCfg(DLLNAME).Value(i)) = "WINMOR.DLL" Then

                        If WINMORPTT(i).Text <> "VOX" Then

                            If PTTCOMM(i).Text <> LastCOM Then
                                OutFile.WriteLine("")
                                Line = PTTCOMM(i).Text & " 9600 PTTONLY"
                                OutFile.WriteLine(Line)
                                LastCOM = PTTCOMM(i).Text
                            End If

                            Line = "RADIO WINMOR " & i
                            OutFile.WriteLine(Line)

                        End If
                    End If
                Next

                OutFile.WriteLine("")
                OutFile.Close()

            Else

            End If
        End If


        MsgBox("File creation complete", MsgBoxStyle.OkOnly + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")


    End Sub


    Private Sub Advanced_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Advanced.Click

        If ConfigLoaded Then

            Form1.NodeCallBox.Text = NodeCallBox.Text
            Form1.InfoMsgBox.Text = InfoMsgBox.Text
            Form1.CTEXTBox.Text = CTEXTBox.Text
            Form1.IDMsgBox.Text = IDMsgBox.Text
            Form1.IDIntervalBox.Text = IDIntervalBox.Text

            ApplName(1).Text = Appl1.Text
            ApplName(2).Text = Appl2.Text
            ApplName(3).Text = Appl3.Text
            ApplName(4).Text = Appl4.Text

            ApplCall(1).Text = Appl1Call.Text
            ApplCall(2).Text = Appl2Call.Text
            ApplCall(3).Text = Appl3Call.Text
            ApplCall(4).Text = Appl4Call.Text

            ' Get Port Info

            Dim Port As Integer = TabControl1.SelectedIndex + 1

            If Port Then

                TxtPortCfg(PORTID).Value(Port) = IDBox(Port).Text
                SaveProc(Port).Invoke(Port)

            End If


        Else

            Form1.GetConfigFile()

            If NewConfig Then

                AddPortTab()

                TxtPortCfg(PORTID).Value(1) = "Loopback"
                TxtPortCfg(PORTTYPE).Value(1) = "0"           ' Start with minimal (Internal)
                TYPE.SelectedIndex = 0
                PROTOCOLBox.SelectedIndex = 0

                TxtPortCfg(IOADDR).Value(1) = "0"
                IOADDRBox.Text = "0"
                CurrentPort = 1
                RefreshPortPage()

                TabControl1.SelectedTab = Form1.PortsTab

            End If
        End If

        Me.Visible = False
        Form1.Visible = True

        My.Settings.StartMode = "A"

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

    Private Sub TabControl1_Deselecting(ByVal sender As Object, ByVal e As System.Windows.Forms.TabControlCancelEventArgs) Handles TabControl1.Deselecting

        Dim Port As Integer = TabControl1.SelectedIndex + 1

        If Port = 0 Then Return

        TxtPortCfg(PORTID).Value(Port) = IDBox(Port).Text

        SaveProc(Port).Invoke(Port)

    End Sub

    Private Sub TabControl1_Selecting(ByVal sender As Object, ByVal e As System.Windows.Forms.TabControlCancelEventArgs) Handles TabControl1.Selecting
        TabControl1.SelectedIndex = TabControl1.SelectedIndex
    End Sub

    Delegate Sub SavePortValue(ByVal Port As Integer)

    Sub SavePactor(ByVal Port As Integer)

        TxtPortCfg(IOADDR).Value(Port) = Mid(COMM(Port).Text, 4)
        TxtPortCfg(SPEED).Value(Port) = BAUD(Port).Text

    End Sub

    Sub SaveKISS(ByVal Port As Integer)

        TxtPortCfg(CHANNEL).Value(Port) = ChannelBox(Port).Text
        TxtPortCfg(IOADDR).Value(Port) = Mid(COMM(Port).Text, 4)

        TxtPortCfg(SPEED).Value(Port) = BAUD(Port).Text


    End Sub

    Sub SaveAGW(ByVal Port As Integer)

        TxtPortCfg(IOADDR).Value(Port) = AGWPortBox(Port).Text
        TxtPortCfg(CHANNEL).Value(Port) = ChannelBox(Port).Text

    End Sub
    Sub SaveNothing(ByVal Port As Integer)

    End Sub

    Function SaveConfig() As Boolean

        If Not Form1.PreSaveValidate() Then Return False

        Dim FileName As String

        FileName = BPQDirectory & "\bpqcfg.bin"

        Try
            File.Copy(FileName & ".save", FileName & ".old", True)
        Catch ex As Exception
        End Try
        Try
            File.Copy(FileName, FileName & ".save", True)
        Catch ex As Exception
        End Try

        Form1.CopyConfigtoArray()

        File.WriteAllBytes(CfgFile & ".bin", NewConfigBytes)

        Form1.UpdateAppls()

        ReDim Config(NewConfigBytes.Length - 1)

        Dim i As Integer

        For i = 0 To Config.Length - 1
            Config(i) = NewConfigBytes(i)
        Next

        Return True

    End Function

    Private Sub HelpButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles HelpButton1.Click
        SimpleHelp.ShowDialog()
    End Sub
End Class