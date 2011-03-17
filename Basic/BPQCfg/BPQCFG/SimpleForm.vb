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

   Public BAUD(32) As NonNullCombobox
   Public COMM(32) As NonNullCombobox

   Public AGWLabel(32) As Label
   Public ChanLabel(32) As Label
   Public ChannelBox(32) As ComboBox
   Public AGWPortBox(32) As BPQCFG.DTNumTextBox

   Dim NumberofWinmorPorts As Integer = 0
   Dim NumberofAGWPorts As Integer = 0
   Dim CanCreateRigControl As Boolean = True
   '    Dim NeedRigControl As Boolean =  False


   Private Sub SimpleForm_FormClosing(ByVal sender As Object, ByVal e As System.Windows.Forms.FormClosingEventArgs) Handles Me.FormClosing

      Dim Port As Integer = TabControl1.SelectedIndex + 1

      If ConfigLoaded Then

         If Port Then

            TxtPortCfg(PORTID).Value(Port) = IDBox(Port).Text
            SaveProc(Port).Invoke(Port)

         End If

         If MsgBox("Do you want to save before exiting?", _
                   MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then

         End If
      End If

      DontAskBeforeClose = True  ' So Form1.closing doesn't ask again
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
      'AddHandler menuItem3.Click, AddressOf Form1.Save_Binary
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

      While Form1.TabControl2.Controls.Count
         Form1.TabControl2.Controls.RemoveAt(0)
      End While

      CfgFile = BPQDirectory & "\bpq32.cfg"

      CfgFile = Environment.ExpandEnvironmentVariables(CfgFile)

      Form1.ReadTextConfig()

      If InStr(CfgFile, ".") > 0 Then CfgFile = Microsoft.VisualBasic.Left(CfgFile, InStr(CfgFile, ".") - 1)

      My.Settings.CfgFileOpenName = CfgFile
      My.Settings.BinOrTextOpen = 1
      My.Settings.Save()

      CopyConfigtoSimple()

   End Sub

   Sub CopyConfigtoSimple()

      Dim Port As Integer

      NumberofWinmorPorts = 0
      NumberofAGWPorts = 0

      '    While TabControl1.Controls.Count
      'TabControl1.Controls.RemoveAt(0)
      'End While

      TabControl1.Visible = True

      NodeCallBox.Text = Form1.NodeCallBox.Text
      LocatorBox.Text = Form1.LocatorBox.Text
      PasswordBox.Text = Form1.PasswordBox.Text
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

         If TxtPortCfg(PORTTYPE).Value(Port) = "INTERNAL" Then
            CreateLOOPBACKTab(Port)

         ElseIf TxtPortCfg(PORTTYPE).Value(Port) = "ASYNC" Then
            CreateKISSTab(Port)
            IDMsgLabel.Visible = True
            IDIntLabel.Visible = True
            IDMsgBox.Visible = True
            IDIntervalBox.Visible = True

         ElseIf TxtPortCfg(PORTTYPE).Value(Port) = "EXTERNAL" Then

            If UCase(TxtPortCfg(DLLNAME).Value(Port)).Contains("WINMOR") Then
               CreateWINMORTab(Port)
               NumberofWinmorPorts = NumberofWinmorPorts + 1

            ElseIf UCase(TxtPortCfg(DLLNAME).Value(Port)).Contains("PACTOR") Then
               CreatePactorTab(Port)
            ElseIf UCase(TxtPortCfg(DLLNAME).Value(Port)).Contains("BPQTOAGW") Then
               CreateAGWTab(Port)
               AGWPortBox(Port).Text = TxtPortCfg(IOADDR).Value(Port)

               IDMsgLabel.Visible = True
               IDIntLabel.Visible = True
               IDMsgBox.Visible = True
               IDIntervalBox.Visible = True
               NumberofAGWPorts = NumberofAGWPorts + 1
            Else
               CreateUnsupportedTab(Port)
            End If
         Else
            CreateUnsupportedTab(Port)
         End If

      Next

      '     If NumberofWinmorPorts Then ReadWINMORCfg()

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
            ProcessWinmorLine(Line, Port)

         Loop Until Textfile.EndOfStream

         Textfile.Close()

      Catch ex As Exception

      End Try

      ReadRigCfg()

   End Sub


   Dim Port As Integer

 

   Private Sub ReadRigCfg()

      Dim Line As String
      Dim Textfile As System.IO.StreamReader

      CanCreateRigControl = True

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

      Dim Port As Integer, i As Integer

      Form1.Lines = 0

      ReDim ParamAtLine(0)

      While TabControl1.Controls.Count
         TabControl1.Controls.RemoveAt(0)
      End While

      For Port = 1 To 32
         PORTTABS(Port) = Nothing
         ClearPort(Port)
      Next

      ClearConfig()

      For i = 1 To 32

         ApplName(i).Text = ""
         ApplCmdAlias(i).Text = ""
         ApplType(i).Text = ""
         ApplQual(i).Text = ""
         ApplCall(i).Text = ""
         ApplAlias(i).Text = ""

      Next

      Appl1.Text = ApplName(1).Text
      Appl2.Text = ApplName(2).Text
      Appl3.Text = ApplName(3).Text
      Appl4.Text = ApplName(4).Text

      Appl1Call.Text = ApplCall(1).Text
      Appl2Call.Text = ApplCall(2).Text
      Appl3Call.Text = ApplCall(3).Text
      Appl4Call.Text = ApplCall(4).Text

      NodeCallBox.Text = ""
      LocatorBox.Text = ""
      PasswordBox.Text = ""
      InfoMsgBox.Text = ""
      CTEXTBox.Text = ""
      IDMsgBox.Text = ""
      IDIntervalBox.Text = "0"


      While Form1.TabControl2.Controls.Count
         Form1.TabControl2.Controls.RemoveAt(0)
      End While

      IDMsgLabel.Visible = False
      IDIntLabel.Visible = False
      IDMsgBox.Visible = False
      IDIntervalBox.Visible = False

      TabControl1.Visible = True

      NumberofWinmorPorts = 0
      NumberofAGWPorts = 0

      Form1.CreateSimpleConfig()

      ConfigLoaded = True

      If NEWCALL.ShowDialog = Windows.Forms.DialogResult.OK Then

         AddPortButton.Enabled = True
         CreateFiles.Enabled = True

         NodeCallBox.Text = NEWCALL.NodeCall.Text
         Form1.NodeCallBox.Text = NEWCALL.NodeCall.Text
         LocatorBox.Text = NEWCALL.Locator.Text
         Form1.LocatorBox.Text = NEWCALL.Locator.Text

         CTEXTBox.Text = "Welcome to " & NodeCallBox.Text & "'s BPQ32 Node"
         InfoMsgBox.Text = NodeCallBox.Text & "'s BPQ32 Node. Enter ? for list of commands"
         AddPort_Click(sender, e)

         Advanced_Click(sender, e)
         Form1.Switch_To_Simple(sender, e)


      End If
   End Sub

   Private Sub AddPort_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AddPortButton.Click

      Dim Port As Integer

      If AddPort.ShowDialog = Windows.Forms.DialogResult.OK Then


         NumberOfPortsinConfig = NumberOfPortsinConfig + 1

         NewConfig = True
         AddPortTab()
         NewConfig = False

         Port = NumberOfPorts

         RigControl(Port) = ""
         WL2KReport(Port) = ""

         TxtPortCfg(PORTID).Value(Port) = AddPort.PORTID.Text

         Dim PortTypeString As String = AddPort.PortType.Text

         If PortTypeString = "WINMOR" Then

            CreateWINMORTab(Port)

            TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = "EXTERNAL"
            TxtPortCfg(DLLNAME).Value(NumberOfPorts) = "WINMOR.dll"
            TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = "WINMOR"
            WINMORPTT(NumberOfPorts).SelectedIndex = 0
            CWID(NumberOfPorts).Checked = False
            BW(NumberOfPorts).SelectedIndex = 1
            BusyLock(NumberOfPorts).Checked = True
            DebugLog(NumberOfPorts).Checked = True
            WINMORPort(NumberOfPorts).Text = 8500 + NumberofWinmorPorts * 2
            NumberofWinmorPorts = NumberofWinmorPorts + 1

            CreateWINMORTab(Port)


         ElseIf PortTypeString.Contains("Pactor") Then

            CreatePactorTab(NumberOfPorts)

            Dim DLL As String = Mid(PortTypeString, 1, 3) & "Pactor.dll"

            TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = "EXTERNAL"
            TxtPortCfg(DLLNAME).Value(NumberOfPorts) = DLL
            TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = "WINMOR"

            COMM(Port).SelectedIndex = 0
            BAUD(Port).SelectedIndex = 0

         ElseIf PortTypeString = "AX.25 KISS" Then

            CreateKISSTab(NumberOfPorts)

            TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = "ASYNC"      ' KISS
            TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = "KISS"
            SetDefaultL2Params(NumberOfPorts)
            IDMsgLabel.Visible = True
            IDIntLabel.Visible = True
            IDMsgBox.Visible = True
            IDIntervalBox.Visible = True

            COMM(Port).SelectedIndex = 0
            BAUD(Port).SelectedIndex = 2
            ChannelBox(Port).SelectedIndex = 0


            SetDefaultL2Params(NumberOfPorts)


         ElseIf PortTypeString = "AX.25 AGWPE" Then

            CreateAGWTab(NumberOfPorts)

            TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = "EXTERNAL"
            TxtPortCfg(DLLNAME).Value(NumberOfPorts) = "BPQtoAGW.dll"
            TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = "KISS"
            IDMsgLabel.Visible = True
            IDIntLabel.Visible = True
            IDMsgBox.Visible = True
            IDIntervalBox.Visible = True

            AGWPortBox(NumberOfPorts).Text = 8001
            ChannelBox(NumberOfPorts).SelectedIndex = NumberofAGWPorts
            NumberofAGWPorts = NumberofAGWPorts + 1

            SetDefaultL2Params(NumberOfPorts)

         ElseIf PortTypeString = "LOOPBACK" Then

            CreateLOOPBACKTab(NumberOfPorts)

            TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = "INTERNAL"  ' Internal
            TxtPortCfg(PROTOCOL).Value(NumberOfPorts) = ""

            SetDefaultL2Params(NumberOfPorts)

         End If

         TabControl1.SelectedIndex = NumberOfPorts - 1

         LoadPortParams(NumberOfPorts) ' Copy to Advanced Mode fields

      End If

   End Sub

   Sub CreateTab(ByVal Port As Integer)

      If PORTTABS(Port) IsNot Nothing Then Return

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
      IDLabel(Port).Location = New System.Drawing.Point(21, 8)
      IDLabel(Port).Size = New System.Drawing.Size(18, 13)
      IDLabel(Port).Text = "ID"
      '
      'IDBOX
      '
      PORTTABS(Port).Controls.Add(IDLabel(Port))
      PORTTABS(Port).Controls.Add(IDBox(Port))

      IDBox(Port).Location = New System.Drawing.Point(68, 5)
      IDBox(Port).Name = "IDBOX"
      IDBox(Port).Text = TxtPortCfg(PORTID).Value(Port)
      IDBox(Port).Size = New System.Drawing.Size(266, 20)

      TabControl1.Controls.Add(PORTTABS(Port))

      SaveProc(Port) = AddressOf SaveNothing

   End Sub



   Sub SaveWINMOR(ByVal Port As Integer)

      PortConfig(Port) = "ADDR " & "127.0.0.1 " & WINMORPort(Port).Text & " PTT " & WINMORPTT(Port).SelectedItem
      If PathBox(Port).Text <> "" Then
         PortConfig(Port) = PortConfig(Port) & " PATH " & PathBox(Port).Text & vbCrLf
      Else
         PortConfig(Port) = PortConfig(Port) & vbCrLf

      End If

      If RigControl(Port).Length Then PortConfig(Port) = PortConfig(Port) + "RIGCONTROL " & RigControl(Port) & vbCrLf
      If WL2KReport(Port).Length Then PortConfig(Port) = PortConfig(Port) + "WL2KREPORT " & WL2KReport(Port) & vbCrLf

      PortConfig(Port) = PortConfig(Port) + "DebugLog " & DebugLog(Port).Checked & vbCrLf
      PortConfig(Port) = PortConfig(Port) + "CWID " & CWID(Port).Checked & vbCrLf
      PortConfig(Port) = PortConfig(Port) + "BUSYLOCK " & BusyLock(Port).Checked & vbCrLf
      PortConfig(Port) = PortConfig(Port) + "DRIVELEVEL " & DriveLevel(Port).Text & vbCrLf
      PortConfig(Port) = PortConfig(Port) + "BW " & BW(Port).Text & vbCrLf

      '      If WINMORPTT(Port).Text <> "VOX" Then
      'OutFile.WriteLine("VOX False")
      'End If

      If WINMORPTT(Port).Text <> "VOX" AndAlso RigControl(Port) = "" Then

         PortConfig(Port) = PortConfig(Port) + "RIGCONTROL " & PTTCOMM(Port).Text & " 9600 PTTONLY" & vbCrLf

      End If

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
      BAUD(Port) = New NonNullCombobox
      COMM(Port) = New NonNullCombobox
      '
      'BAUD
      '
      BAUD(Port).FormattingEnabled = True
      BAUD(Port).Location = New System.Drawing.Point(237, 45)
      BAUD(Port).Name = Port
      BAUD(Port).Size = New System.Drawing.Size(109, 21)
      BAUD(Port).TabIndex = 25
      '
      'COM
      '
      Me.COMM(Port).FormattingEnabled = True
      Me.COMM(Port).Location = New System.Drawing.Point(67, 45)
      Me.COMM(Port).Name = Port
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

      COMM(Port).SelectedItem = FindString

      BAUD(Port).Items.Add("38400")
      BAUD(Port).Items.Add("19200")
      BAUD(Port).Items.Add("9600")
      BAUD(Port).Items.Add("4800")
      BAUD(Port).Items.Add("2400")
      BAUD(Port).Items.Add("1200")

      FindString = TxtPortCfg(SPEED).Value(Port)

      BAUD(Port).SelectedItem = FindString

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


      If MsgBox("This will update your bpq32.cfg" & vbCrLf & _
      "Two generations of backup are created, with extensions of .save and .old" & vbCrLf & _
      "Do you want to continue ?", _
       MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.No Then Return


      Form1.NodeCallBox.Text = NodeCallBox.Text
      Form1.LocatorBox.Text = LocatorBox.Text
      Form1.PasswordBox.Text = PasswordBox.Text

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

         PortBeingValidated = Port

         If Not (ValidateChildren()) Then
            Return
         End If

         TxtPortCfg(PORTID).Value(Port) = IDBox(Port).Text
         SaveProc(Port).Invoke(Port)

      End If

      SaveConfig()

      MsgBox("File creation complete", MsgBoxStyle.OkOnly + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config")


   End Sub


   Private Sub Advanced_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Advanced.Click

      If ConfigLoaded Then

         Form1.NodeCallBox.Text = NodeCallBox.Text
         Form1.InfoMsgBox.Text = InfoMsgBox.Text
         Form1.CTEXTBox.Text = CTEXTBox.Text
         Form1.IDMsgBox.Text = IDMsgBox.Text
         Form1.IDIntervalBox.Text = IDIntervalBox.Text
         Form1.PasswordBox.Text = PasswordBox.Text
         Form1.LocatorBox.Text = LocatorBox.Text
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

            PortBeingValidated = Port

            If Not (ValidateChildren()) Then
               Return
            End If


            TxtPortCfg(PORTID).Value(Port) = IDBox(Port).Text
            SaveProc(Port).Invoke(Port)

         End If

         '   PortConfigPointer = 2560 + ((Port - 1) * 512)

         LoadPortParams(Port)
         Form1.TabControl1.SelectedIndex = 0
         '    Form1.TabControl2.SelectedIndex = Port - 1

      Else

         Form1.GetConfigFile()

         If NewConfig Then

            NumberOfPortsinConfig = 1

            AddPortTab()

            NumberOfPorts = 1

            TxtPortCfg(PORTID).Value(1) = "Loopback"
            TxtPortCfg(PORTTYPE).Value(1) = "INTERNAL"           ' Start with minimal (Internal)
            TYPE.SelectedIndex = 0
            PROTOCOLBox.SelectedIndex = 1

            TxtPortCfg(IOADDR).Value(1) = "0"
            IOADDRBox.Text = "0"
            CurrentPort = 1
            RefreshPortPage()

            TabControl1.SelectedTab = Form1.PortsTab

            ConfigLoaded = True


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

      PortBeingValidated = Port

      If Not (ValidateChildren()) Then
         e.Cancel = True
      End If

      TxtPortCfg(PORTID).Value(Port) = IDBox(Port).Text

      SaveProc(Port).Invoke(Port)

   End Sub

   Private Sub TabControl1_Selecting(ByVal sender As Object, ByVal e As System.Windows.Forms.TabControlCancelEventArgs) Handles TabControl1.Selecting
      TabControl1.SelectedIndex = TabControl1.SelectedIndex
   End Sub



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

      '        If Not Form1.PreSaveValidate() Then Return False

      Dim FileName As String

      FileName = BPQDirectory & "\bpq32.cfg"

      Try
         File.Copy(FileName & ".save", FileName & ".old", True)
      Catch ex As Exception
      End Try
      Try
         File.Copy(FileName, FileName & ".save", True)
      Catch ex As Exception
      End Try

      CfgFile = BPQDirectory & "\bpq32"

      Form1.SaveasTextwithComments()
      Form1.UpdateAppls()

      Return True

   End Function

   Private Sub HelpButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles HelpButton1.Click
      SimpleHelp.ShowDialog()
   End Sub


   Private Sub BackgroundWorker1_DoWork(ByVal sender As System.Object, ByVal e As System.ComponentModel.DoWorkEventArgs)

   End Sub


End Class