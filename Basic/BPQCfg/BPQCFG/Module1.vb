Module Module1

    Public Declare Function GetAdapterList Lib "c:\bpq32\bpqadaptersdll.dll" _
         Alias "_GetAdapterList@4" (ByVal ErrMsg As String) As Integer

    Public Declare Function GetNextAdapter Lib "c:\bpq32\bpqadaptersdll.dll" _
         Alias "_GetNextAdapter@8" (ByVal Name As String, ByVal Desc As String) As Integer

    Public BPQDirectory As String
    Public CfgFile As String
    Public AGWAppl As Integer
    Public OriginalAGWAppl As Integer
    Public DEDMask(8) As Integer    ' As held in registry

    Public NewConfig As Boolean
    Public Adding As Boolean

    Public Config As Byte()
    Public NewConfigBytes() As Byte

    Public ConfigLength As Integer
    Public NumberOfPorts As Integer
    Public NumberOfPortsinConfig As Integer
    Public NumberofTNCPorts As Integer



 
    Public Structure TxtCfgInfoStruct

        Public Key As String
        '      Public Value As String
        Public CfgField As Object ' Textbox (or Whatever) containing value
        Public Checkbox As Boolean ' Set if Checkbox
        Public MultiLineText As Boolean
        Public LineNo As Integer  ' Line No in Source config

    End Structure

    Public Structure TxtPortCfgInfoStruct

        Public Key As String
        Public Value() As String    ' Text value of param
        Public CfgField As Object ' Textbox (or Whatever) containing value
        Public Offset As Integer
        Public Len As Integer
        Public TextCfgProc As GetTextPortValue
        Public BinCfgProc As GetPortValue
        Public Checkbox As Boolean ' Set if Checkbox
        Public SetValue As String   ' Value for boolean param
        Public UnSetValue As String
        Public LineNo() As Integer  ' Line No in Source config

    End Structure

  

    Public TxtCfg(72) As TxtCfgInfoStruct
    Public TxtPortCfg(44) As TxtPortCfgInfoStruct
    Public SavePortNo As Integer
    Public SaveTNCPortNo As Integer

    Public Const BBSCALL = 7
    Public Const BBSALIAS = 8
    Public Const BBSQUAL = 30

    Public Const APPL1CALL = 45
    Public Const APPL1ALIAS = 46
    Public Const APPL1QUAL = 47

    Public Const NumofPortConfigParams = 44
    Public Const PORTNUM = 1
    Public Const PORTID = 2
    Public Const PORTTYPE = 3
    Public Const DLLNAME = 4
    Public Const PROTOCOL = 5
    Public Const KISSOPTIONS = 6
    Public Const IOADDR = 7
    Public Const INTLEVEL = 8
    Public Const SPEED = 9
    Public Const CHANNEL = 10
    Public Const MHEARD = 11
    Public Const QUALITY = 12
    Public Const MAXFRAME = 13
    Public Const TXDELAY = 14
    Public Const TXTAIL = 15
    Public Const SLOTTIME = 16
    Public Const PERSIST = 17
    Public Const FULLDUP = 18
    Public Const SOFTDCD = 19
    Public Const FRACK = 20
    Public Const RESPTIME = 21
    Public Const RETRIES = 22
    Public Const PACLEN = 23
    Public Const MAXDIGIS = 24
    Public Const DIGIFLAG = 25
    Public Const DIGIPORT = 26
    Public Const CWID = 27
    Public Const CWIDTYPE = 28
    Public Const PORTCALL = 29
    Public Const PORTALIAS = 30
    Public Const PORTALIAS2 = 31
    Public Const ALIAS_IS_BBS = 32
    Public Const BBSFLAG = 33
    Public Const MINQUAL = 34
    Public Const NODESPACLEN = 35
    Public Const QUALADJUST = 36
    Public Const BCALL = 37
    Public Const UNPROTO = 38
    Public Const L3ONLY = 39
    Public Const INTERLOCK = 40
    Public Const TXPORT = 41
    Public Const USERS = 42
    Public Const VALIDCALLS = 43
    Public Const DIGIMASK = 44


    Public PortConfigPointer As Integer
    Public CurrRow As Integer, CurrCol As Integer

    Public RouteCall(32) As TextBox
    Public RouteQual(32) As NumTextBox
    Public RoutePort(32) As NumTextBox
    Public RouteMaxFrame(32) As NumTextBox
    Public RouteFrack(32) As NumTextBox
    Public RoutePaclen(32) As NumTextBox
    Public Routeinp3(32) As NumTextBox
    Public RouteComment(32) As String
    Public RouteLineno(32) As Integer



    Public COMPort(16) As TextBox
    Public COMType(16) As ComboBox
    Public COMKISSMask(16) As TextBox
    Public COMAPPLMask(16) As TextBox
    Public COMAPPLFlags(16) As TextBox

    Public COMLabel(10) As Label

    Public ApplLabel(8) As Label
    Public ApplName(8) As BPQCFG.ApplNameTextBox
    Public ApplType(8) As System.Windows.Forms.ComboBox
    Public ApplQual(8) As BPQCFG.NumTextBox
    Public ApplCall(8) As BPQCFG.CallsignTextBox
    Public ApplAlias(8) As BPQCFG.AliasTextBox

    Public PortTab(16) As TabPage

    Public CurrentPort As Integer

    Public PortIDBox As TextBox = New TextBox
    Public WithEvents TYPE As ComboBox = New ComboBox
    Public WithEvents PROTOCOLBox As ComboBox = New ComboBox
    Public IOADDRBox As TextBox = New TextBox
    Public INTLEVELBox As NumTextBox = New NumTextBox(255)
    Public SPEEDBox As NumTextBox = New NumTextBox(65535)
    Public CHANNELBox As TextBox = New TextBox
    Public DLLNAMEBox As TextBox = New TextBox
    Public WithEvents PortConfigButton As Button = New Button
    Public BBSFLAGBox As CheckBox = New CheckBox
    Public QUALITYBox As NumTextBox = New NumTextBox(255)
    Public MAXFRAMEBox As NumTextBox = New NumTextBox(7)
    Public TXDELAYBox As NumTextBox = New NumTextBox(2000)
    Public SLOTTIMEBox As NumTextBox = New NumTextBox(1000)
    Public PERSISTBox As NumTextBox = New NumTextBox(255)
    Public FULLDUPBox As CheckBox = New CheckBox
    Public SOFTDCDBox As CheckBox = New CheckBox
    Public FRACKBox As NumTextBox = New NumTextBox(30000)
    Public RESPTIMEBox As NumTextBox = New NumTextBox(10000)
    Public RETRIESBox As NumTextBox = New NumTextBox(100)
    Public PACLENBox As NumTextBox = New NumTextBox(256)
    Public CWIDBox As CallsignTextBox = New CallsignTextBox
    Public PORTCALLBox As CallsignTextBox = New CallsignTextBox
    Public PORTALIASBox As AliasTextBox = New AliasTextBox
    Public VALIDCALLSBox As TextBox = New TextBox
    Public QUALADJUSTBox As NumTextBox = New NumTextBox(255)
    Public DIGIFLAGBox As NumTextBox = New NumTextBox(255)
    Public DIGIPORTBox As NumTextBox = New NumTextBox(16)
    Public DIGIMASKBox As NumTextBox = New NumTextBox(16)
    Public USERSBox As NumTextBox = New NumTextBox(255)
    Public UNPROTOBox As TextBox = New TextBox
    Public PORTNUMBox As NumTextBox = New NumTextBox(16)
    Public TXTAILBox As NumTextBox = New NumTextBox(100)
    Public ALIAS_IS_BBSBox As CheckBox = New CheckBox
    Public L3ONLYBox As CheckBox = New CheckBox
    'Public KISSOPTIONSBox As TextBox = New TextBox
    Public WithEvents KISSOptionsBox As New System.Windows.Forms.CheckedListBox


    Public INTERLOCKBox As NumTextBox = New NumTextBox(16)
    Public NODESPACLENBox As NumTextBox = New NumTextBox(256)
    Public TXPORTBox As NumTextBox = New NumTextBox(16)
    Public MHEARDBox As CheckBox = New CheckBox
    Public CWIDTYPEBox As CheckBox = New CheckBox
    Public MINQUALBox As NumTextBox = New NumTextBox(255)
    Public MAXDIGISBox As NumTextBox = New NumTextBox(8)
    Public PORTALIAS2Box As AliasTextBox = New AliasTextBox
    Public BCALLBox As CallsignTextBox = New CallsignTextBox

    Public PortIDLabel As Label = New Label
    Public TYPELabel As Label = New Label
    Public PROTOCOLLabel As Label = New Label
    Public IOADDRLabel As Label = New Label
    Public INTLEVELLabel As Label = New Label
    Public SPEEDLabel As Label = New Label
    Public CHANNELLabel As Label = New Label
    Public QUALITYLabel As Label = New Label
    Public MAXFRAMELabel As Label = New Label
    Public TXDELAYLabel As Label = New Label
    Public SLOTTIMELabel As Label = New Label
    Public PERSISTLabel As Label = New Label
    Public FRACKLabel As Label = New Label
    Public RESPTIMELabel As Label = New Label
    Public RETRIESLabel As Label = New Label
    Public PACLENLabel As Label = New Label
    Public CWIDLabel As Label = New Label
    Public PORTCALLLabel As Label = New Label
    Public PORTALIASLabel As Label = New Label
    Public VALIDCALLSLabel As Label = New Label
    Public QUALADJUSTLabel As Label = New Label
    Public DIGIFLAGLabel As Label = New Label
    Public DIGIPORTLabel As Label = New Label
    Public DIGIMASKLabel As Label = New Label
    Public USERSLabel As Label = New Label
    Public UNPROTOLabel As Label = New Label
    Public PORTNUMLabel As Label = New Label
    Public TXTAILLabel As Label = New Label
    Public KISSOPTIONSLabel As Label = New Label
    Public INTERLOCKLabel As Label = New Label
    Public NODESPACLENLabel As Label = New Label
    Public TXPORTLabel As Label = New Label
    Public MINQUALLabel As Label = New Label
    Public MAXDIGISLabel As Label = New Label
    Public PORTALIAS2Label As Label = New Label
    Public DLLNAMELabel As Label = New Label
    Public BCALLLabel As Label = New Label

    '   Public Const ASYNC = 0
    '   Public Const PC120 = 1
    '   Public Const DRSI = 2
    '   Public Const DE56 = 2
    '  Public Const TOSH = 3
    '  Public Const QUAD = 4
    '   Public Const RLC100 = 5
    '   Public Const RLC400 = 6
    '  Public Const INTERNAL = 7
    '   Public Const EXTERNAL = 8
    '  Public Const BAYCOM = 9
    '  Public Const PA0HZP = 10

    Public Types() As String = New String() _
    {"ASYNC", "INTERNAL", "EXTERNAL", "PC120", "DRSI", "DE56", "TOSH", "QUAD", _
    "RLC100", "RLC400", "BAYCOM", _
    "PA0HZP"}

    Public Protos() As String = New String() {"KISS", "NETROM", "BPQKISS", "HDLC", "L2", "WINMOR", "PACTOR"}

    Public MapTypes() As Integer = {0, 3, 4, 5, 6, 7, 8, 1, 2, 9, 10}

    Public unMapTypes() As Integer = {0, 7, 8, 1, 2, 3, 4, 5, 6, 9, 10}

    Public ComboBoxSize = New System.Drawing.Size(110, 22)
    Public TextBoxSize = New System.Drawing.Size(50, 22)
    Public SmallTextBoxSize = New System.Drawing.Size(30, 22)
    Public CallTextBoxSize = New System.Drawing.Size(70, 22)
    Public LargeTextBoxSize = New System.Drawing.Size(90, 22)
    Public CheckBoxSize = New System.Drawing.Size(130, 22)
    Public LabelSize = New System.Drawing.Size(65, 17)

    Const RowSpacing = 28
    Const Row1 = 16, Row2 = Row1 + RowSpacing, Row3 = Row2 + RowSpacing, Row4 = Row3 + RowSpacing
    Const Row5 = Row4 + RowSpacing, Row6 = Row5 + RowSpacing

    Public Const Col1 = 8, Col2 = 158, Col3 = 308, Col4 = 458
    Public Const Col2a = 208, Col3a = 408

    Public TNCTypes() As String = New String() {"TNC2", "KISS", "PK232/UFQ", "PK232/AA4RE"}


    Public Sub CreateRoutePage()

        Dim i As Integer
        Dim TextBoxSize = New System.Drawing.Size(45, 22)
        Dim SmallTextBoxSize = New System.Drawing.Size(25, 22)

        CurrRow = Row2
        CurrCol = 0

        For i = 0 To 31
            RouteCall(i) = New CallsignTextBox
            RouteQual(i) = New NumTextBox(255)
            RoutePort(i) = New NumTextBox(16)
            RouteMaxFrame(i) = New NumTextBox(7)
            RouteFrack(i) = New NumTextBox(10000)
            RoutePaclen(i) = New NumTextBox(256)
            Routeinp3(i) = New NumTextBox(1)

            RouteCall(i).Location = New System.Drawing.Point(CurrCol + 8, CurrRow)
            RouteQual(i).Location = New System.Drawing.Point(CurrCol + 98, CurrRow)
            RoutePort(i).Location = New System.Drawing.Point(CurrCol + 128, CurrRow)
            RouteMaxFrame(i).Location = New System.Drawing.Point(CurrCol + 158, CurrRow)
            RouteFrack(i).Location = New System.Drawing.Point(CurrCol + 188, CurrRow)
            RoutePaclen(i).Location = New System.Drawing.Point(CurrCol + 238, CurrRow)
            Routeinp3(i).Location = New System.Drawing.Point(CurrCol + 268, CurrRow)

            RouteCall(i).Size = CallTextBoxSize
            RouteQual(i).Size = SmallTextBoxSize
            RoutePort(i).Size = SmallTextBoxSize
            RouteMaxFrame(i).Size = SmallTextBoxSize
            RouteFrack(i).Size = TextBoxSize
            RoutePaclen(i).Size = SmallTextBoxSize
            Routeinp3(i).Size = SmallTextBoxSize

            AddHandler RoutePort(i).Validating, AddressOf ValidatingPort
            AddHandler RoutePort(i).Validated, AddressOf ValidatedSub

            Form1.RoutesTab.Controls.Add(RouteCall(i))
            Form1.RoutesTab.Controls.Add(RouteQual(i))
            Form1.RoutesTab.Controls.Add(RoutePort(i))
            Form1.RoutesTab.Controls.Add(RouteMaxFrame(i))
            Form1.RoutesTab.Controls.Add(RouteFrack(i))
            Form1.RoutesTab.Controls.Add(RoutePaclen(i))
            Form1.RoutesTab.Controls.Add(Routeinp3(i))

            CurrRow = CurrRow + 22

            If i = 15 Then

                CurrRow = Row2
                CurrCol = 300

            End If

        Next

    End Sub

    Public Sub CreateTNCPage()

        Dim i As Integer
        Dim ComboBoxSize = New System.Drawing.Size(100, 22)
        Dim SmallTextBoxSize = New System.Drawing.Size(30, 22)
        Const col1 = 8, col2 = 48, col3 = 158
        Const col4 = col3 + 45
        Const col5 = col4 + 45
        Const row3 = 80

        For i = 1 To 10
            COMLabel(i) = New Label
            COMLabel(i).Size = New System.Drawing.Size(40, 30)
            Form1.BPQ16Tab.Controls.Add(COMLabel(i))

        Next

        COMLabel(1).Text = "COM"
        COMLabel(1).Location = New System.Drawing.Point(col1, row3)
        COMLabel(2).Text = "TYPE"
        COMLabel(2).Location = New System.Drawing.Point(col2, row3)
        COMLabel(3).Text = "APPL" & vbCrLf & "Mask"
        COMLabel(3).Location = New System.Drawing.Point(col3, row3)
        COMLabel(4).Text = "KISS" & vbCrLf & "Mask"
        COMLabel(4).Location = New System.Drawing.Point(col4, row3)
        COMLabel(5).Text = "APPL" & vbCrLf & "Flags"
        COMLabel(5).Location = New System.Drawing.Point(col5, row3)

        COMLabel(6).Text = "COM"
        COMLabel(6).Location = New System.Drawing.Point(col1 + 320, row3)
        COMLabel(7).Text = "TYPE"
        COMLabel(7).Location = New System.Drawing.Point(col2 + 320, row3)
        COMLabel(8).Text = "APPL" & vbCrLf & "Mask"
        COMLabel(8).Location = New System.Drawing.Point(col3 + 320, row3)
        COMLabel(9).Text = "KISS" & vbCrLf & "Mask"
        COMLabel(9).Location = New System.Drawing.Point(col4 + 320, row3)
        COMLabel(10).Text = "APPL" & vbCrLf & "Flags"
        COMLabel(10).Location = New System.Drawing.Point(col5 + 320, row3)

        CurrRow = 120
        CurrCol = 0

        For i = 0 To 15
            COMPort(i) = New NumTextBox(255)
            COMType(i) = New ComboBox
            COMAPPLMask(i) = New NumTextBox(255)
            COMKISSMask(i) = New NumTextBox(65535)
            COMAPPLFlags(i) = New NumTextBox(255)

            COMPort(i).Location = New System.Drawing.Point(CurrCol + col1, CurrRow)
            COMType(i).Location = New System.Drawing.Point(CurrCol + col2, CurrRow)
            COMAPPLMask(i).Location = New System.Drawing.Point(CurrCol + col3, CurrRow)
            COMKISSMask(i).Location = New System.Drawing.Point(CurrCol + col4, CurrRow)
            COMAPPLFlags(i).Location = New System.Drawing.Point(CurrCol + col5, CurrRow)

            COMPort(i).Size = SmallTextBoxSize
            COMType(i).Size = ComboBoxSize
            COMAPPLMask(i).Size = SmallTextBoxSize
            COMKISSMask(i).Size = SmallTextBoxSize
            COMAPPLFlags(i).Size = SmallTextBoxSize

            COMType(i).Items.AddRange(TNCTypes)
            COMType(i).Text = TNCTypes(0)
            '        COMType(i).locked = True

            Form1.BPQ16Tab.Controls.Add(COMPort(i))
            Form1.BPQ16Tab.Controls.Add(COMType(i))
            Form1.BPQ16Tab.Controls.Add(COMAPPLMask(i))
            Form1.BPQ16Tab.Controls.Add(COMKISSMask(i))
            Form1.BPQ16Tab.Controls.Add(COMAPPLFlags(i))

            CurrRow = CurrRow + 30

            If i = 7 Then

                CurrRow = 120
                CurrCol = 320

            End If

        Next

    End Sub
    Public Sub AddPortTab()

        If NumberOfPorts = 16 Then Exit Sub

        Adding = True ' Suppress triggereing updates to TYPE

        NumberOfPorts = NumberOfPorts + 1

        TxtPortCfg(PORTTYPE).Value(NumberOfPorts) = 1          ' Start with minimal (Internal)
        TxtPortCfg(PORTNUM).Value(NumberOfPorts) = NumberOfPorts
        TxtPortCfg(PORTID).Value(NumberOfPorts) = "New Port"
        TxtPortCfg(IOADDR).Value(NumberOfPorts) = 0
        TxtPortCfg(KISSOPTIONS).Value(NumberOfPorts) = 0

        Form1.TabControl2.TabPages.Remove(Form1.NewPortPage)

        PortTab(NumberOfPorts) = New TabPage
        PortTab(NumberOfPorts).Text = "Port " & NumberOfPorts
        PortTab(NumberOfPorts).AutoScroll = True

        Form1.TabControl2.TabPages.Add(PortTab(NumberOfPorts))

        Form1.TabControl2.TabPages.Add(Form1.NewPortPage)

        Form1.TabControl2.SelectedIndex = NumberOfPorts - 1

        '   DrawPortTab(NumberOfPorts)

        Adding = False

    End Sub

    Public Sub SavePortInfo()

        Dim i As Integer

        If CurrentPort = 0 Then Return

        For i = 1 To NumofPortConfigParams

            If TxtPortCfg(i).Checkbox Then

                TxtPortCfg(i).Value(CurrentPort) = TxtPortCfg(i).CfgField.Checked

            Else

                TxtPortCfg(i).Value(CurrentPort) = TxtPortCfg(i).CfgField.text

            End If
        Next

        TxtPortCfg(PORTTYPE).Value(CurrentPort) = TYPE.SelectedIndex
        If PROTOCOLBox.SelectedIndex >= 0 Then TxtPortCfg(PROTOCOL).Value(CurrentPort) = PROTOCOLBox.SelectedIndex
        TxtPortCfg(IOADDR).Value(CurrentPort) = Convert.ToInt32(IOADDRBox.Text, 16)
        TxtPortCfg(KISSOPTIONS).Value(CurrentPort) = GetKissMode()

    End Sub
    Public Sub LoadPortParams(ByVal Port As Integer)

        Dim i As Integer

        If Port = 0 Then Exit Sub

        CurrentPort = Port

        For i = 1 To NumofPortConfigParams

            If TxtPortCfg(i).Checkbox Then

                TxtPortCfg(i).CfgField.Checked = TxtPortCfg(i).Value(CurrentPort)
            Else
                TxtPortCfg(i).CfgField.text = TxtPortCfg(i).Value(CurrentPort)

            End If

        Next

        TYPE.SelectedIndex = TxtPortCfg(PORTTYPE).Value(CurrentPort)
        PROTOCOLBox.SelectedIndex = TxtPortCfg(PROTOCOL).Value(CurrentPort)
        IOADDRBox.Text = Hex(TxtPortCfg(IOADDR).Value(CurrentPort))

        For i = 0 To 3
            KISSOptionsBox.SetItemChecked(i, ((TxtPortCfg(KISSOPTIONS).Value(CurrentPort) >> i) And 1) = 1)
        Next

        ' Clear all pages

        For i = 0 To NumberOfPorts - 1

            Try
                PortTab(i).Controls.Clear()
            Catch ex As Exception

            End Try

        Next

    End Sub

    Public Sub RefreshPortPage()

        If CurrentPort = 0 Then Exit Sub

        PortTab(CurrentPort).Controls.Clear() '  RemoveAt(0)

        PortTab(CurrentPort).Controls.Add(PORTNUMLabel)
        PortTab(CurrentPort).Controls.Add(PORTNUMBox)
        PortTab(CurrentPort).Controls.Add(PortIDLabel)
        PortTab(CurrentPort).Controls.Add(PortIDBox)
        PortTab(CurrentPort).Controls.Add(TYPELabel)
        PortTab(CurrentPort).Controls.Add(TYPE)

        CurrRow = Row2
        CurrCol = Col2a

        If TxtPortCfg(PORTTYPE).Value(CurrentPort) <> "1" And TxtPortCfg(PORTTYPE).Value(CurrentPort) <> "2" Then ' External 
            DrawLabel(PROTOCOLLabel)
            DrawComboBox(PROTOCOLBox)
        End If


        If TxtPortCfg(PORTTYPE).Value(CurrentPort) = "2" Then ' External 
            DrawLabel(DLLNAMELabel)
            DrawBox(DLLNAMEBox)
            CurrCol = CurrCol + 15

            If LCase(DLLNAMEBox.Text) = "bpqtoagw.dll" Then

                DrawLabel(CHANNELLabel)
                DrawBox(CHANNELBox)

                DrawLabel(IOADDRLabel)
                DrawBox(IOADDRBox)

            End If
            If LCase(DLLNAMEBox.Text) = "bpqaxip.dll" Then
                PortTab(CurrentPort).Controls.Add(PortConfigButton)

            End If
        End If

        CurrRow = Row3
        CurrCol = 8

        If TxtPortCfg(PORTTYPE).Value(CurrentPort) <> "1" And TxtPortCfg(PORTTYPE).Value(CurrentPort) <> "2" Then ' External 

            If TxtPortCfg(PROTOCOL).Value(CurrentPort) = "0" Then

                KISSOptionsBox.Location = New System.Drawing.Point(Col4 + 70, Row1)
                KISSOPTIONSLabel.Location = New System.Drawing.Point(Col4, Row1 + 10)
                PortTab(CurrentPort).Controls.Add(KISSOptionsBox)
                PortTab(CurrentPort).Controls.Add(KISSOPTIONSLabel)

            End If

            DrawLabel(CHANNELLabel)
            DrawBox(CHANNELBox)


            DrawLabel(IOADDRLabel)
            DrawBox(IOADDRBox)
            DrawLabel(INTLEVELLabel)
            DrawBox(INTLEVELBox)
            DrawLabel(SPEEDLabel)
            DrawBox(SPEEDBox)

            DrawLabel(TXDELAYLabel)
            DrawBox(TXDELAYBox)
            DrawLabel(TXTAILLabel)
            DrawBox(TXTAILBox)
            DrawLabel(SLOTTIMELabel)
            DrawBox(SLOTTIMEBox)
            DrawLabel(PERSISTLabel)
            DrawBox(PERSISTBox)

            DrawcheckBox(FULLDUPBox)


        End If

        If TxtPortCfg(PORTTYPE).Value(CurrentPort) = "2" Then ' External 
            PortTab(CurrentPort).Controls.Add(DLLNAMELabel)
            PortTab(CurrentPort).Controls.Add(DLLNAMEBox)
        End If

        If TxtPortCfg(PORTTYPE).Value(CurrentPort) > "2" Then     ' HDLC Cards

            DrawcheckBox(SOFTDCDBox)
            DrawLabel(CWIDLabel)
            DrawBox(CWIDBox)
            DrawcheckBox(CWIDTYPEBox)

        End If

        If CurrCol <> 8 Then
            CurrCol = 8
            CurrRow = CurrRow + RowSpacing
        End If

        DrawLabel(PORTCALLLabel)
        DrawBox(PORTCALLBox)
        DrawLabel(PORTALIASLabel)
        DrawBox(PORTALIASBox)
        DrawLabel(PORTALIAS2Label)
        DrawBox(PORTALIAS2Box)
        DrawLabel(BCALLLabel)
        DrawBox(BCALLBox)

        DrawLabel(FRACKLabel)
        DrawBox(FRACKBox)
        DrawLabel(RESPTIMELabel)
        DrawBox(RESPTIMEBox)

        DrawLabel(RETRIESLabel)
        DrawBox(RETRIESBox)
        DrawLabel(PACLENLabel)
        DrawBox(PACLENBox)

        DrawLabel(MAXFRAMELabel)
        DrawBox(MAXFRAMEBox)

        DrawLabel(QUALITYLabel)
        DrawBox(QUALITYBox)
        DrawLabel(MINQUALLabel)
        DrawBox(MINQUALBox)

        DrawLabel(NODESPACLENLabel)
        DrawBox(NODESPACLENBox)


        DrawLabel(QUALADJUSTLabel)
        DrawBox(QUALADJUSTBox)
        DrawLabel(DIGIFLAGLabel)
        DrawBox(DIGIFLAGBox)
        DrawLabel(DIGIPORTLabel)
        DrawBox(DIGIPORTBox)
        DrawLabel(DIGIMASKLabel)
        DrawBox(DIGIMASKBox)
        DrawcheckBox(ALIAS_IS_BBSBox)
        DrawcheckBox(L3ONLYBox)
        DrawcheckBox(MHEARDBox)
        DrawLabel(USERSLabel)
        DrawBox(USERSBox)

        DrawLabel(INTERLOCKLabel)
        DrawBox(INTERLOCKBox)
        DrawLabel(TXPORTLabel)
        DrawBox(TXPORTBox)
        DrawLabel(MAXDIGISLabel)
        DrawBox(MAXDIGISBox)

        DrawcheckBox(BBSFLAGBox)

        If CurrCol <> 8 Then
            CurrCol = 8
            CurrRow = CurrRow + RowSpacing
        End If

        DrawLabel(VALIDCALLSLabel)
        DrawBox(VALIDCALLSBox)

        If CurrCol <> 8 Then
            CurrCol = 8
            CurrRow = CurrRow + RowSpacing
        End If

        DrawLabel(UNPROTOLabel)
        DrawBox(UNPROTOBox)

    End Sub
    Public Sub InitLabels()


        PORTNUMLabel.Text = "Portnum"
        PORTNUMLabel.Location = New System.Drawing.Point(Col1, Row1 + 3)
        PORTNUMLabel.Size = LabelSize

        PORTNUMBox.Location = New System.Drawing.Point(Col1 + 70, Row1)
        PORTNUMBox.Size = SmallTextBoxSize

        PortIDLabel.Text = "ID"
        PortIDLabel.Location = New System.Drawing.Point(Col1 + 120, Row1 + 3)
        PortIDLabel.Size = New System.Drawing.Size(30, 17)
        PortIDBox.Location = New System.Drawing.Point(Col1 + 150, Row1)
        PortIDBox.Size = New System.Drawing.Size(240, 22)


        TYPELabel.Text = "Type"
        TYPELabel.Location = New System.Drawing.Point(Col1, Row2 + 3)
        TYPELabel.Size = LabelSize

        TYPE.Location = New System.Drawing.Point(Col1 + 70, Row2)
        TYPE.Size = ComboBoxSize
        TYPE.Items.AddRange(Types)
        TYPE.DropDownStyle = ComboBoxStyle.DropDownList

        PROTOCOLBox = New ComboBox
        PROTOCOLBox.Location = New System.Drawing.Point(Col2a + 70, Row2)
        PROTOCOLBox.Size = ComboBoxSize

        PROTOCOLBox.Items.AddRange(Protos)
        PROTOCOLBox.DropDownStyle = ComboBoxStyle.DropDownList


        PROTOCOLLabel.Text = "Protocol"
        PROTOCOLLabel.Location = New System.Drawing.Point(Col2a, Row2 + 3)
        PROTOCOLLabel.Size = LabelSize

        DLLNAMELabel.Text = "DLLName"
        DLLNAMELabel.Location = New System.Drawing.Point(Col3a, Row2 + 3)
        DLLNAMELabel.Size = New System.Drawing.Size(70, 17)

        AddHandler PortConfigButton.Click, AddressOf PortConfigButton_Click
        PortConfigButton.Location = New System.Drawing.Point(Col4, Row2)
        PortConfigButton.Text = "Config AXIP"

        KISSOptionsBox.Location = New System.Drawing.Point(Col4 + 70, Row1)
        KISSOPTIONSLabel.Location = New System.Drawing.Point(Col4, Row1 + 10)


        DLLNAMEBox.Location = New System.Drawing.Point(Col3a + 70, Row2)
        DLLNAMEBox.Size = LargeTextBoxSize

        IOADDRLabel.Text = "IOAddr"
        IOADDRLabel.Size = New System.Drawing.Size(70, 17)
        IOADDRBox.Size = TextBoxSize

        INTLEVELLabel.Text = "IntLevel"
        INTLEVELLabel.Size = New System.Drawing.Size(70, 17)
        INTLEVELBox.Size = SmallTextBoxSize

        SPEEDLabel.Text = "Speed"
        SPEEDLabel.Size = LabelSize
        SPEEDBox.Size = TextBoxSize

        CHANNELLabel.Text = "Channel"
        CHANNELLabel.Size = LabelSize
        CHANNELBox.Size = SmallTextBoxSize

        BBSFLAGBox.Text = "BBS Banned"
        BBSFLAGBox.CheckAlign = ContentAlignment.MiddleRight
        BBSFLAGBox.Size = CheckBoxSize

        QUALITYLabel.Text = "Quality"
        QUALITYLabel.Size = LabelSize
        QUALITYBox.Size = SmallTextBoxSize

        MAXFRAMELabel.Text = "MaxFrame"
        MAXFRAMELabel.Size = LabelSize
        MAXFRAMEBox.Size = SmallTextBoxSize

        TXDELAYLabel.Text = "TXDelay"
        TXDELAYLabel.Size = LabelSize
        TXDELAYBox.Size = TextBoxSize

        SLOTTIMELabel.Text = "Slottime"
        SLOTTIMELabel.Size = LabelSize
        SLOTTIMEBox.Size = SmallTextBoxSize

        PERSISTLabel.Text = "Persist"
        PERSISTLabel.Size = LabelSize
        PERSISTBox.Size = SmallTextBoxSize

        FULLDUPBox.Text = "FullDup"
        FULLDUPBox.CheckAlign = ContentAlignment.MiddleRight
        FULLDUPBox.Size = CheckBoxSize

        SOFTDCDBox.Text = "SoftDCD"
        SOFTDCDBox.CheckAlign = ContentAlignment.MiddleRight
        SOFTDCDBox.Size = CheckBoxSize

        FRACKLabel.Text = "Frack"
        FRACKLabel.Size = LabelSize
        FRACKBox.Size = TextBoxSize

        RESPTIMELabel.Text = "Resptime"
        RESPTIMELabel.Size = LabelSize
        RESPTIMEBox.Size = TextBoxSize

        RETRIESLabel.Text = "Retries"
        RETRIESLabel.Size = LabelSize
        RETRIESBox.Size = SmallTextBoxSize

        PACLENLabel.Text = "Paclen"
        PACLENLabel.Size = LabelSize
        PACLENBox.Size = SmallTextBoxSize

        CWIDLabel.Text = "CWID"
        CWIDLabel.Size = LabelSize
        CWIDBox.Size = CallTextBoxSize

        PORTCALLLabel.Text = "Port Call"
        PORTCALLLabel.Size = LabelSize
        PORTCALLBox.Size = CallTextBoxSize

        PORTALIASLabel.Text = "Port Alias"
        PORTALIASLabel.Size = LabelSize
        PORTALIASBox.Size = CallTextBoxSize

        VALIDCALLSLabel.Text = "ValidCalls"
        VALIDCALLSLabel.Size = LabelSize
        VALIDCALLSBox.Size = New System.Drawing.Size(460, 17)

        QUALADJUSTLabel.Text = "QualAdjust"
        QUALADJUSTLabel.Size = LabelSize
        QUALADJUSTBox.Size = SmallTextBoxSize

        DIGIFLAGLabel.Text = "DigiFlag"
        DIGIFLAGLabel.Size = LabelSize
        DIGIFLAGBox.Size = SmallTextBoxSize
        Form1.ToolTip1.SetToolTip(DIGIFLAGBox, "0 - Digipeat disabled, 1 - Digipeat all frame types, 255 - digipeat UI frames only")


        DIGIPORTLabel.Text = "Digiport"
        DIGIPORTLabel.Size = LabelSize
        DIGIPORTBox.Size = SmallTextBoxSize
        Form1.ToolTip1.SetToolTip(DIGIPORTBox, "Port on which frames received here will be transmitted. Zero means tranmit on receiving port")

        DIGIMASKLabel.Text = "Digimask"
        DIGIMASKLabel.Size = LabelSize
        DIGIMASKBox.Size = SmallTextBoxSize
        Form1.ToolTip1.SetToolTip(DIGIMASKBox, "Used for digi'ing UI frames to more than one port")

        USERSLabel.Text = "Users"
        USERSLabel.Size = LabelSize
        USERSBox.Size = SmallTextBoxSize

        UNPROTOLabel.Text = "Unproto"
        UNPROTOLabel.Size = LabelSize
        UNPROTOBox.Size = New System.Drawing.Size(460, 17)

        TXTAILLabel.Text = "TxTail"
        TXTAILLabel.Size = LabelSize
        TXTAILBox.Size = TextBoxSize

        ALIAS_IS_BBSBox.Text = "Alias is BBS"
        ALIAS_IS_BBSBox.CheckAlign = ContentAlignment.MiddleRight
        ALIAS_IS_BBSBox.Size = CheckBoxSize

        L3ONLYBox.Text = "L3Only"
        L3ONLYBox.CheckAlign = ContentAlignment.MiddleRight
        L3ONLYBox.Size = CheckBoxSize

        KISSOPTIONSLabel.Text = "KissOptions"
        KISSOPTIONSLabel.Size = LabelSize
        '       KISSOptionsBox.Size = TextBoxSize

        KISSOptionsBox.CheckOnClick = True
        KISSOptionsBox.FormattingEnabled = True
        KISSOptionsBox.Items.AddRange(New Object() {"Checksum", "Polled", "ACKMode", "Slave"})
        KISSOptionsBox.Name = "KISSOptionsBox"
        KISSOptionsBox.Size = New System.Drawing.Size(112, 55)
        KISSOptionsBox.TabIndex = 37

        INTERLOCKLabel.Text = "Interlock"
        INTERLOCKLabel.Size = LabelSize
        INTERLOCKBox.Size = SmallTextBoxSize

        NODESPACLENLabel.Text = "NodePacLen"
        NODESPACLENLabel.Size = LabelSize
        NODESPACLENBox.Size = SmallTextBoxSize

        TXPORTLabel.Text = "TxPort"
        TXPORTLabel.Size = LabelSize
        TXPORTBox.Size = SmallTextBoxSize

        MHEARDBox.Text = "Suppress MHeard"
        MHEARDBox.CheckAlign = ContentAlignment.MiddleRight
        MHEARDBox.Size = CheckBoxSize
        Form1.ToolTip1.SetToolTip(MHEARDBox, "Set to disble HMeard processing on this port")

        CWIDTYPEBox.Text = "CWID OnOff"
        CWIDTYPEBox.CheckAlign = ContentAlignment.MiddleRight
        CWIDTYPEBox.Size = CheckBoxSize
        Form1.ToolTip1.SetToolTip(CWIDTYPEBox, "Set for On/Off keying of CWID" & vbCrLf & "(Default is FSK)")

        MINQUALLabel.Text = "MinQual"
        MINQUALLabel.Size = LabelSize
        MINQUALBox.Size = SmallTextBoxSize

        MAXDIGISLabel.Text = "MaxDigis"
        MAXDIGISLabel.Size = LabelSize
        MAXDIGISBox.Size = SmallTextBoxSize

        PORTALIAS2Label.Text = "Port Alias 2"
        PORTALIAS2Label.Size = New System.Drawing.Size(70, 17)
        PORTALIAS2Box.Size = TextBoxSize

        BCALLLabel.Text = "BCALL"
        BCALLLabel.Size = LabelSize
        BCALLBox.Size = CallTextBoxSize

    End Sub

    Public Sub InitHandlers()

        AddHandler TYPE.SelectedIndexChanged, AddressOf TYPESelectedIndexChanged
        AddHandler PROTOCOLBox.SelectedIndexChanged, AddressOf PROTOCOLSelectedIndexChanged

    End Sub


    Private Sub TYPESelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

        TxtPortCfg(PORTTYPE).Value(CurrentPort) = TYPE.SelectedIndex
        If Adding = False Then RefreshPortPage()

    End Sub
    Private Sub PROTOCOLSelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

        TxtPortCfg(PROTOCOL).Value(CurrentPort) = PROTOCOLBox.SelectedIndex
        If Adding = False Then RefreshPortPage()

    End Sub
    Public Function Get16Bits(ByVal i As Integer)

        Get16Bits = Config(i) + 256 * Config(i + 1)

    End Function

    Public Function GetString(ByVal n As Integer, ByVal len As Integer)

        Dim i As Integer
        Dim callsign As String
        Dim x As Char


        callsign = ""

        For i = 0 To len - 1

            If Config(i + n) = 0 Then
                x = " "
            Else
                x = Chr(Config(i + n))
            End If

            callsign = callsign & x
        Next

        GetString = RTrim(callsign)

    End Function

    Public Function GetMultiLineString(ByVal Offset, ByVal Len) As String

        Dim msg As String, i As Integer

        msg = ""

        For i = Offset To Offset + Len

            If Config(i) = 0 Then Return Mid(msg, 1, msg.Length - 2) ' remove trailing crlf

            If Config(i) = 13 Then
                msg = msg & vbCrLf
            Else
                msg = msg & Chr(Config(i))
            End If

        Next

        Return msg

    End Function

    Public Sub Putbyte(ByVal n As Integer, ByVal val As Byte)
        NewConfigBytes(n) = val
    End Sub

    Public Sub PutBool(ByVal n As Integer, ByVal val As Boolean)
        If val Then NewConfigBytes(n) = 1 Else NewConfigBytes(n) = 0
    End Sub

    Public Sub Put16bits(ByVal n As Integer, ByVal val As Integer)

        Dim i As Integer, j As Integer

        i = val
        If i = -1 Then i = 1

        j = Int(i / 256)
        i = i - j * 256
        NewConfigBytes(n) = i
        NewConfigBytes(n + 1) = j

    End Sub

    Public Sub PutString(ByVal n As Integer, ByVal len As Integer, ByVal val As String, Optional ByVal EmptyAsNull As Boolean = False)

        Dim i As Integer, j As Integer

        j = val.Length

        If EmptyAsNull And (j = 0) Then Exit Sub

        If j > len Then j = len

        For i = 0 To j - 1

            NewConfigBytes(i + n) = Asc(Mid(val, i + 1, 1))

        Next

        For i = j To len - 1

            NewConfigBytes(i + n) = 32

        Next

    End Sub

    Public Sub PutMultilineString(ByVal n As Integer, ByVal len As Integer, ByVal val As String)

        Dim offset As Integer

        Dim c As Byte, i As Integer, j As Integer

        j = val.Length

        If j > len Then j = len

        offset = n

        For i = 1 To j

            c = Asc(Mid(val, i, 1))

            If c = 0 Then
                NewConfigBytes(offset) = 13         ' CR on end
                Exit Sub
            End If

            If c <> 10 Then
                NewConfigBytes(offset) = c
                offset = offset + 1
            End If

        Next
        NewConfigBytes(offset) = 13         ' CR on end

    End Sub


    Public Function GetPortString(ByVal n As Integer, ByVal len As Integer) As Object
        Dim i As Integer
        Dim retval As String

        retval = ""

        For i = 0 To len - 1

            If Config(PortConfigPointer + i + n) = 0 Then
                GetPortString = RTrim(retval)
                Exit Function
            End If

            retval = retval & Chr(Config(PortConfigPointer + i + n))

        Next

        Return RTrim(retval)

    End Function
    Public Function GetPort16Bits(ByVal i As Integer, ByVal dummy As Integer) As Object

        Return Config(PortConfigPointer + i) + 256 * Config(PortConfigPointer + i + 1)

    End Function

    Public Function GetPort8Bits(ByVal i As Integer, ByVal dummy As Integer) As Object

        Return Config(PortConfigPointer + i)

    End Function

    Public Function GetMHeardBin(ByVal i As Integer, ByVal dummy As Integer) As Object

        Dim MH As Byte

        ' Null = Y

        MH = Config(PortConfigPointer + i)

        If MH = Asc("N") Then Return 1

        Return 0

    End Function

    Public Function GetPortType(ByVal i As Integer, ByVal dummy As Integer) As Object

        Return MapTypes(Config(PortConfigPointer + i) / 2)

    End Function


    Public Function GetKissOptions(ByVal i As Integer, ByVal dummy As Integer) As Object

        Return Config(PortConfigPointer + i) + 256 * Config(PortConfigPointer + i + 1)

    End Function

    Public Function GetChannel(ByVal i As Integer, ByVal dummy As Integer) As Object

        If Config(PortConfigPointer + i) = 0 Then Return "" Else Return Chr(Config(PortConfigPointer + i))

    End Function
    Public Function GetProtocol(ByVal i As Integer, ByVal dummy As Integer) As Object

        Return Config(PortConfigPointer + i) / 2

    End Function
    Public Sub PutPortString(ByVal n As Integer, ByVal len As Integer, ByVal val As String, Optional ByVal Padding As Integer = 32)

        Dim i As Integer, j As Integer

        If val = Nothing Then
            j = 0
        Else
            j = val.Length
        End If

        If j > len Then j = len

        For i = 0 To j - 1

            NewConfigBytes(PortConfigPointer + i + n) = Asc(Mid(val, i + 1, 1))

        Next

        For i = j To len - 1

            NewConfigBytes(PortConfigPointer + i + n) = Padding

        Next

    End Sub

    Public Sub Putport8bits(ByVal n As Integer, ByVal val As Byte)
        NewConfigBytes(PortConfigPointer + n) = val
    End Sub


    Public Sub PutPort16bits(ByVal n As Integer, ByVal val As Integer)

        Dim i As Integer, j As Integer

        i = val
        If i = -1 Then i = 1

        j = Int(i / 256)
        i = i - j * 256
        NewConfigBytes(PortConfigPointer + n) = i
        NewConfigBytes(PortConfigPointer + n + 1) = j

    End Sub

    Public Sub PutPortBoolean(ByVal Key As Integer, ByVal Port As Integer)

        If TxtPortCfg(Key).UnSetValue <> Nothing Then

            If TxtPortCfg(Key).Value(Port) = "False" Or TxtPortCfg(Key).Value(Port) = "0" Then
                NewConfigBytes(PortConfigPointer + TxtPortCfg(Key).Offset) = Asc(TxtPortCfg(Key).UnSetValue)
                Return
            End If

        End If

        If TxtPortCfg(Key).Value(Port) = "False" Or TxtPortCfg(Key).Value(Port) = "0" Then Return

        NewConfigBytes(PortConfigPointer + TxtPortCfg(Key).Offset) = Asc(TxtPortCfg(Key).SetValue)

    End Sub

    Public Sub PutPortMH(ByVal Key As Integer, ByVal Port As Integer)


        If TxtPortCfg(Key).Value(Port) = "False" Or TxtPortCfg(Key).Value(Port) = "0" Then
            NewConfigBytes(PortConfigPointer + TxtPortCfg(Key).Offset) = 0
        Else
            NewConfigBytes(PortConfigPointer + TxtPortCfg(Key).Offset) = Asc("N")
        End If

    End Sub
    Public Sub DrawLabel(ByVal obj As Label)

        obj.Location = New System.Drawing.Point(CurrCol, CurrRow + 3)

        PortTab(CurrentPort).Controls.Add(obj)

    End Sub

    Public Sub DrawBox(ByVal obj As TextBox)

        obj.Location = New System.Drawing.Point(CurrCol + 70, CurrRow)

        PortTab(CurrentPort).Controls.Add(obj)

        CurrCol = CurrCol + 150
        If CurrCol > 550 Then

            CurrCol = 8
            CurrRow = CurrRow + RowSpacing

        End If

    End Sub

    Public Sub DrawComboBox(ByVal obj As ComboBox)

        obj.Location = New System.Drawing.Point(CurrCol + 70, CurrRow)

        PortTab(CurrentPort).Controls.Add(obj)

        CurrCol = CurrCol + 200
        If CurrCol > 500 Then

            CurrCol = 8
            CurrRow = CurrRow + RowSpacing

        End If

    End Sub
    Public Sub DrawcheckBox(ByVal obj As CheckBox)

        obj.Location = New System.Drawing.Point(CurrCol, CurrRow)

        PortTab(CurrentPort).Controls.Add(obj)

        CurrCol = CurrCol + 150
        If CurrCol > 500 Then

            CurrCol = 8
            CurrRow = CurrRow + RowSpacing

        End If

    End Sub


    Public Sub ValidatedSub(ByVal sender As Object, ByVal e As System.EventArgs)

        Form1.ErrorProvider1.SetError(sender, "")

    End Sub

    Public Sub OptionalCallValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        sender.Text = LTrim(RTrim(UCase(sender.Text)))

        If sender.Text.Length > 9 Then

            Form1.ErrorProvider1.SetError(sender, "Too Long (max 9 chars")
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

        End If

    End Sub
    Public Sub AliasValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        sender.Text = LTrim(RTrim(UCase(sender.Text)))

        If sender.Text.Length > 6 Then

            Form1.ErrorProvider1.SetError(sender, "Too Long (max 6 chars")
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

        End If

    End Sub
    Public Sub ApplValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        sender.Text = LTrim(RTrim(UCase(sender.Text)))

        If sender.Text.Length > 16 Then

            Form1.ErrorProvider1.SetError(sender, "Too Long (max 16 chars")
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

        End If

    End Sub

    Private Sub ValidatingPort(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        Dim row As Integer

        Dim c As TextBox, port As Integer, minport As Integer
        c = DirectCast(sender, TextBox)

        For row = 0 To 31

            If RoutePort(row) Is c Then Exit For

        Next

        If RouteCall(row).Text = String.Empty Then minport = 0 Else minport = 1

        port = Val(c.Text)

        If port < minport Or port > NumberOfPorts Then

            Form1.ErrorProvider1.SetError(c, "Must be between " & minport & " and  " & NumberOfPorts)
            e.Cancel = True
            c.Select(0, c.Text.Length)

        End If

    End Sub

    Public Sub ValidatingNumber(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        Dim port As Integer

        port = Val(sender.Text)
        sender.Text = port

        If port < 0 Or port > sender.Max Then


            Form1.ErrorProvider1.SetError(sender, "Must be between 0 and " & sender.Max)
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

        End If

    End Sub

    Public Sub ValidatingNumberorNull(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        Dim port As Integer

        If LTrim(RTrim(sender.text)) = "" Then Exit Sub

        port = Val(sender.Text)
        sender.Text = port

        If port < 0 Or port > sender.Max Then


            Form1.ErrorProvider1.SetError(sender, "Must be between 0 and " & sender.Max)
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

        End If

    End Sub


    Public Sub SetupAppl()

        Dim i As Integer, Row As Integer, QualCol As Integer, TypeCol As Integer
        Dim LabelCol As Integer, NameCol As Integer, CallCol As Integer, AliasCol As Integer
        Dim RowIncr As Integer

        '        Dim factor As Integer = 120 / 96
        Dim factor As Integer = 1


        RowIncr = 28 * factor
        Row = 140 - RowIncr '151 - RowIncr
        LabelCol = 10
        NameCol = 60
        CallCol = 170
        AliasCol = 260
        QualCol = 355 * factor
        TypeCol = 415 * factor

        Form1.ApplNameLabel.Location = New System.Drawing.Point(NameCol, Row)
        Form1.ApplCallLabel.Location = New System.Drawing.Point(CallCol, Row)
        Form1.ApplAliasLabel.Location = New System.Drawing.Point(AliasCol, Row)
        Form1.ApplQualLabel.Location = New System.Drawing.Point(QualCol, Row)
        Form1.ApplTypeLabel.Location = New System.Drawing.Point(TypeCol, Row)

        For i = 1 To 8

            ApplLabel(i) = New Label
            ApplLabel(i).Location = New System.Drawing.Point(LabelCol, Row + 3 + RowIncr * i)
            ApplLabel(i).Size = New System.Drawing.Size(40, 22)
            ApplLabel(i).Text = "Appl " & i
            Form1.ApplsTab.Controls.Add(ApplLabel(i))


            ApplName(i) = New BPQCFG.ApplNameTextBox

            ApplName(i).Location = New System.Drawing.Point(NameCol, Row + RowIncr * i)
            ApplName(i).Size = New System.Drawing.Size(100, 22)
            ApplName(i).Text = ""

            Form1.ApplsTab.Controls.Add(ApplName(i))

            ApplCall(i) = New BPQCFG.CallsignTextBox

            ApplCall(i).Location = New System.Drawing.Point(CallCol, Row + RowIncr * i)
            ApplCall(i).Size = CallTextBoxSize
            ApplCall(i).Text = ""

            Form1.ApplsTab.Controls.Add(ApplCall(i))

            ApplAlias(i) = New BPQCFG.AliasTextBox

            ApplAlias(i).Location = New System.Drawing.Point(AliasCol, Row + RowIncr * i)
            ApplAlias(i).Size = CallTextBoxSize
            ApplAlias(i).Text = ""

            Form1.ApplsTab.Controls.Add(ApplAlias(i))

            ApplQual(i) = New BPQCFG.NumTextBox(255)

            ApplQual(i).Location = New System.Drawing.Point(QualCol, Row + RowIncr * i)
            ApplQual(i).Size = New System.Drawing.Size(39, 22)
            ApplQual(i).Text = "0"

            Form1.ApplsTab.Controls.Add(ApplQual(i))

            ApplType(i) = New System.Windows.Forms.ComboBox

            ApplType(i).DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
            ApplType(i).FormattingEnabled = True
            ApplType(i).Items.AddRange(New Object() {"BPQ32", "AGWPE", "DED Host"})
            ApplType(i).Location = New System.Drawing.Point(TypeCol, Row + RowIncr * i)
            ApplType(i).Size = New System.Drawing.Size(74, 21)

            AddHandler ApplType(i).SelectedIndexChanged, AddressOf ApplTypeChanged
            AddHandler ApplType(i).Validated, AddressOf ValidatedSub

            Form1.ApplsTab.Controls.Add(ApplType(i))
            Debug.Print(Row + RowIncr * i)
        Next

        Dim mask As Integer

        mask = OriginalAGWAppl

        For i = 1 To 8

            If (mask And 1) = 1 Then ApplType(i).SelectedIndex = 1
            mask = mask >> 1

        Next

    End Sub


    Public Sub MultiLineValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        '      sender.Text = LTrim(RTrim(UCase(sender.Text)))

        If sender.Text.Length > sender.maxlen Then

            Form1.ErrorProvider1.SetError(sender, "Too Long (max " & sender.maxlen & " chars")
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

        End If

    End Sub

    Sub ApplTypeChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

        Dim i As Integer

        AGWAppl = 0
        For i = 8 To 1 Step -1

            AGWAppl = AGWAppl << 1
            If ApplType(i).SelectedIndex = 1 Then AGWAppl = AGWAppl + 1

        Next

        Form1.AGWApplVal.Text = "0x" & Hex(AGWAppl)

    End Sub
    Public Sub SetAGWAppl()

        My.Computer.Registry.SetValue _
        ("HKEY_LOCAL_MACHINE\Software\G8BPQ\BPQ32\AGWtoBPQ", "ApplMask", AGWAppl)

    End Sub

    Public Function CompareConfig() As Boolean

        CompareConfig = False

        If Config Is Nothing Then Exit Function

        If Config.Length <> NewConfigBytes.Length Then Exit Function

        Dim i As Integer
        For i = 0 To Config.Length - 1

            If Config(i) <> NewConfigBytes(i) Then
                Exit Function
            End If


        Next

        CompareConfig = True

    End Function

    Function GetKissMode()
        Dim indexChecked As Integer
        Dim KissMode As Integer = 0

        For Each indexChecked In KISSOptionsBox.CheckedIndices
            ' The indexChecked variable contains the index of the item.
            KissMode = KissMode + (1 << indexChecked)
        Next

        GetKissMode = KissMode

    End Function
    Public Function DEDApplChanged() As Boolean

        Dim i As Integer

        For i = 1 To 8

            If ((DEDMask(i) >> (i - 1)) And 1) = 1 And ApplType(i).SelectedIndex <> 2 Then
                DEDApplChanged = True
                Exit Function
            End If

            If ((DEDMask(i) >> (i - 1)) And 1) <> 1 And ApplType(i).SelectedIndex = 2 Then
                DEDApplChanged = True
                Exit Function
            End If

        Next

        DEDApplChanged = False

    End Function

    Public Sub SetDEDAppl()

        Dim i As Integer

        Try
            My.Computer.Registry.LocalMachine.DeleteSubKey("Software\G8BPQ\BPQ32\DEDAppl")

        Catch ex As Exception
        End Try

        For i = 1 To 8

            If ApplType(i).SelectedIndex = 2 Then

                My.Computer.Registry.SetValue _
                    ("HKEY_LOCAL_MACHINE\Software\G8BPQ\BPQ32\DEDAppl", ApplCall(i).Text, 1 << (i - 1))
            End If

        Next

    End Sub
    Private Sub PortConfigButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)

        AXIPCfg.Visible = True
        AXIPCfg.Activate()

    End Sub

    Public Sub InitTextCfgKeywords()

        Dim i As Integer

        TxtCfg(1).Key = "HOSTINTERRUPT"
        TxtCfg(1).CfgField = Form1.HostInterruptBox

        TxtCfg(2).Key = "EMS"
        TxtCfg(2).CfgField = Form1.EMSBox
        TxtCfg(2).Checkbox = True

        TxtCfg(3).Key = "DESQVIEW"
        TxtCfg(3).CfgField = Form1.DesqViewBox
        TxtCfg(3).Checkbox = True

        TxtCfg(4).Key = "AUTOSAVE"
        TxtCfg(4).CfgField = Form1.AutoSaveBox
        TxtCfg(4).Checkbox = True

        TxtCfg(5).Key = "NODECALL"
        TxtCfg(5).CfgField = Form1.NodeCallBox

        TxtCfg(6).Key = "NODEALIAS"
        TxtCfg(6).CfgField = Form1.NodeAliasBox

        TxtCfg(7).Key = "BBSCALL"
        TxtCfg(7).CfgField = ApplCall(1)

        TxtCfg(8).Key = "BBSALIAS"
        TxtCfg(8).CfgField = ApplAlias(1)

        TxtCfg(9).Key = "IDMSG:"
        TxtCfg(9).CfgField = Form1.IDMsgBox
        TxtCfg(9).MultiLineText = True

        TxtCfg(10).Key = "UNPROTO"
        TxtCfg(10).CfgField = Form1.UnprotoBox

        TxtCfg(11).Key = "INFOMSG:"
        TxtCfg(11).CfgField = Form1.InfoMsgBox
        TxtCfg(11).MultiLineText = True

        TxtCfg(12).Key = "BTEXT:"
        TxtCfg(12).CfgField = Form1.BTEXTBox
        TxtCfg(12).MultiLineText = True

        TxtCfg(13).Key = "CTEXT:"
        TxtCfg(13).CfgField = Form1.CTEXTBox
        TxtCfg(13).MultiLineText = True

        TxtCfg(14).Key = "FULL_CTEXT"
        TxtCfg(14).CfgField = Form1.FullCTEXT
        TxtCfg(14).Checkbox = True

        TxtCfg(15).Key = "OBSINIT"
        TxtCfg(15).CfgField = Form1.ObsInitBox

        TxtCfg(16).Key = "OBSMIN"
        TxtCfg(16).CfgField = Form1.ObsMinBox

        TxtCfg(17).Key = "NODESINTERVAL"
        TxtCfg(17).CfgField = Form1.NodesIntervalBox

        TxtCfg(18).Key = "IDINTERVAL"
        TxtCfg(18).CfgField = Form1.IDIntervalBox

        TxtCfg(19).Key = "BTINTERVAL"
        TxtCfg(19).CfgField = Form1.BTIntervalBox

        TxtCfg(20).Key = "L3TIMETOLIVE"
        TxtCfg(20).CfgField = Form1.L3TTLBox

        TxtCfg(21).Key = "L4RETRIES"
        TxtCfg(21).CfgField = Form1.L4RetriesBox

        TxtCfg(22).Key = "L4TIMEOUT"
        TxtCfg(22).CfgField = Form1.L4TimeOutBox

        TxtCfg(23).Key = "L4DELAY"
        TxtCfg(23).CfgField = Form1.L4DelayBox

        TxtCfg(24).Key = "L4WINDOW"
        TxtCfg(24).CfgField = Form1.L4WindowBox

        TxtCfg(25).Key = "MAXLINKS"
        TxtCfg(25).CfgField = Form1.MaxLinksBox

        TxtCfg(26).Key = "MAXNODES"
        TxtCfg(26).CfgField = Form1.MaxNodesBox

        TxtCfg(27).Key = "MAXROUTES"
        TxtCfg(27).CfgField = Form1.MaxRoutesBox

        TxtCfg(28).Key = "MAXCIRCUITS"
        TxtCfg(28).CfgField = Form1.MaxCircuitsBox

        TxtCfg(29).Key = "MINQUAL"
        TxtCfg(29).CfgField = Form1.MinQualBox

        TxtCfg(30).Key = "BBSQUAL"
        TxtCfg(30).CfgField = ApplQual(1)

        TxtCfg(31).Key = "BUFFERS"
        TxtCfg(31).CfgField = Form1.BuffersBox

        TxtCfg(32).Key = "PACLEN"
        TxtCfg(32).CfgField = Form1.PACLENBox

        TxtCfg(33).Key = "TRANSDELAY"
        TxtCfg(33).CfgField = Form1.TransDelayBox

        TxtCfg(34).Key = "T3"
        TxtCfg(34).CfgField = Form1.T3Box

        TxtCfg(35).Key = "IDLETIME"
        TxtCfg(35).CfgField = Form1.IdleTimeBox

        TxtCfg(36).Key = "BBS"
        TxtCfg(36).CfgField = Form1.BBSBox
        TxtCfg(36).Checkbox = True

        TxtCfg(37).Key = "NODE"
        TxtCfg(37).CfgField = Form1.NodeBox
        TxtCfg(37).Checkbox = True

        TxtCfg(38).Key = "HIDENODES"
        TxtCfg(38).CfgField = Form1.HideNodesBox
        TxtCfg(38).Checkbox = True

        TxtCfg(39).Key = "ENABLE_LINKED"
        TxtCfg(39).CfgField = Form1.EnableLinked

        TxtCfg(40).Key = "TNCPORT"
        TxtCfg(41).Key = "PORT"
        TxtCfg(42).Key = "ENDPORT"
        TxtCfg(43).Key = "ROUTES:"
        TxtCfg(44).Key = "APPLICATIONS"

        TxtCfg(45).Key = "APPL1CALL"
        TxtCfg(45).CfgField = ApplCall(1)

        TxtCfg(46).Key = "APPL1ALIAS"
        TxtCfg(46).CfgField = ApplAlias(1)

        TxtCfg(47).Key = "APPL1QUAL"
        TxtCfg(47).CfgField = ApplQual(1)

        TxtCfg(48).Key = "APPL2CALL"
        TxtCfg(48).CfgField = ApplCall(2)

        TxtCfg(49).Key = "APPL2ALIAS"
        TxtCfg(49).CfgField = ApplAlias(2)

        TxtCfg(50).Key = "APPL2QUAL"
        TxtCfg(50).CfgField = ApplQual(2)

        TxtCfg(51).Key = "APPL3CALL"
        TxtCfg(51).CfgField = ApplCall(3)

        TxtCfg(52).Key = "APPL3ALIAS"
        TxtCfg(52).CfgField = ApplAlias(3)

        TxtCfg(53).Key = "APPL3QUAL"
        TxtCfg(53).CfgField = ApplQual(3)

        TxtCfg(54).Key = "APPL4CALL"
        TxtCfg(54).CfgField = ApplCall(4)

        TxtCfg(55).Key = "APPL4ALIAS"
        TxtCfg(55).CfgField = ApplAlias(4)

        TxtCfg(56).Key = "APPL4QUAL"
        TxtCfg(56).CfgField = ApplQual(4)

        TxtCfg(57).Key = "APPL5CALL"
        TxtCfg(57).CfgField = ApplCall(5)

        TxtCfg(58).Key = "APPL5ALIAS"
        TxtCfg(58).CfgField = ApplAlias(5)

        TxtCfg(59).Key = "APPL5QUAL"
        TxtCfg(59).CfgField = ApplQual(5)

        TxtCfg(60).Key = "APPL6CALL"
        TxtCfg(60).CfgField = ApplCall(6)

        TxtCfg(61).Key = "APPL6ALIAS"
        TxtCfg(61).CfgField = ApplAlias(6)

        TxtCfg(62).Key = "APPL6QUAL"
        TxtCfg(62).CfgField = ApplQual(6)

        TxtCfg(63).Key = "APPL7CALL"
        TxtCfg(63).CfgField = ApplCall(7)

        TxtCfg(64).Key = "APPL7ALIAS"
        TxtCfg(64).CfgField = ApplAlias(7)

        TxtCfg(65).Key = "APPL7QUAL"
        TxtCfg(65).CfgField = ApplQual(7)

        TxtCfg(66).Key = "APPL8CALL"
        TxtCfg(66).CfgField = ApplCall(8)

        TxtCfg(67).Key = "APPL8ALIAS"
        TxtCfg(67).CfgField = ApplAlias(8)

        TxtCfg(68).Key = "APPL8QUAL"
        TxtCfg(68).CfgField = ApplQual(8)

        TxtCfg(69).Key = "MAXHOPS"
        TxtCfg(69).CfgField = Form1.MaxHopsBox

        TxtCfg(70).Key = "MAXRTT"
        TxtCfg(70).CfgField = Form1.MaxRTTBox

        TxtCfg(71).Key = "IPGATEWAY"
        TxtCfg(71).CfgField = Form1.IPGatewayBox
        TxtCfg(71).Checkbox = True

        TxtCfg(72).Key = "C_IS_CHAT"
        TxtCfg(72).CfgField = Form1.CIsChatBox
        TxtCfg(72).Checkbox = True

        For i = 1 To NumofPortConfigParams
            ReDim TxtPortCfg(i).Value(16)
            ReDim TxtPortCfg(i).LineNo(16)
        Next

        TxtPortCfg(1).Key = "PORTNUM"
        TxtPortCfg(1).CfgField = PORTNUMBox
        TxtPortCfg(1).Offset = 0
        TxtPortCfg(1).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(1).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(2).Key = "ID"
        TxtPortCfg(2).CfgField = PortIDBox
        TxtPortCfg(2).Offset = 2
        TxtPortCfg(2).Len = 30
        TxtPortCfg(2).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(2).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(3).Key = "TYPE"
        TxtPortCfg(3).CfgField = TYPE
        TxtPortCfg(3).Offset = 32
        TxtPortCfg(3).TextCfgProc = AddressOf GetTextType
        TxtPortCfg(3).BinCfgProc = AddressOf GetPortType

        TxtPortCfg(4).Key = "DLLNAME"
        TxtPortCfg(4).CfgField = DLLNAMEBox
        TxtPortCfg(4).Offset = 210
        TxtPortCfg(4).Len = 16
        TxtPortCfg(4).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(4).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(5).Key = "PROTOCOL"
        TxtPortCfg(5).CfgField = PROTOCOLBox
        TxtPortCfg(5).Offset = 34
        TxtPortCfg(5).TextCfgProc = AddressOf GetTextProtocol
        TxtPortCfg(5).BinCfgProc = AddressOf GetProtocol

        TxtPortCfg(6).Key = "KISSOPTIONS"
        TxtPortCfg(6).CfgField = KISSOptionsBox
        TxtPortCfg(6).Offset = 112
        TxtPortCfg(6).TextCfgProc = AddressOf GetTextKissOptions
        TxtPortCfg(6).BinCfgProc = AddressOf GetKissOptions

        TxtPortCfg(7).Key = "IOADDR"
        TxtPortCfg(7).CfgField = IOADDRBox
        TxtPortCfg(7).Offset = 36
        TxtPortCfg(7).TextCfgProc = AddressOf GetTextPort16BitsHex
        TxtPortCfg(7).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(8).Key = "INTLEVEL"
        TxtPortCfg(8).CfgField = INTLEVELBox
        TxtPortCfg(8).Offset = 38
        TxtPortCfg(8).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(8).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(9).Key = "SPEED"
        TxtPortCfg(9).CfgField = SPEEDBox
        TxtPortCfg(9).Offset = 40
        TxtPortCfg(9).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(9).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(10).Key = "CHANNEL"
        TxtPortCfg(10).CfgField = CHANNELBox
        TxtPortCfg(10).Offset = 42
        TxtPortCfg(10).TextCfgProc = AddressOf GetTextPortChannel
        TxtPortCfg(10).BinCfgProc = AddressOf GetChannel

        TxtPortCfg(11).Key = "MHEARD"
        TxtPortCfg(11).CfgField = MHEARDBox
        TxtPortCfg(11).Offset = 120
        TxtPortCfg(11).TextCfgProc = AddressOf GetTextPortMHeard
        TxtPortCfg(11).BinCfgProc = AddressOf GetMHeardBin
        TxtPortCfg(11).Checkbox = True
        TxtPortCfg(11).SetValue = "N"
        TxtPortCfg(11).UnSetValue = "Y"

        TxtPortCfg(12).Key = "QUALITY"
        TxtPortCfg(12).CfgField = QUALITYBox
        TxtPortCfg(12).Offset = 46
        TxtPortCfg(12).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(12).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(13).Key = "MAXFRAME"
        TxtPortCfg(13).CfgField = MAXFRAMEBox
        TxtPortCfg(13).Offset = 48
        TxtPortCfg(13).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(13).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(14).Key = "TXDELAY"
        TxtPortCfg(14).CfgField = TXDELAYBox
        TxtPortCfg(14).Offset = 50
        TxtPortCfg(14).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(14).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(15).Key = "TXTAIL"
        TxtPortCfg(15).CfgField = TXTAILBox
        TxtPortCfg(15).Offset = 76
        TxtPortCfg(15).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(15).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(16).Key = "SLOTTIME"
        TxtPortCfg(16).CfgField = SLOTTIMEBox
        TxtPortCfg(16).Offset = 52
        TxtPortCfg(16).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(16).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(17).Key = "PERSIST"
        TxtPortCfg(17).CfgField = PERSISTBox
        TxtPortCfg(17).Offset = 54
        TxtPortCfg(17).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(17).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(18).Key = "FULLDUP"
        TxtPortCfg(18).CfgField = FULLDUPBox
        TxtPortCfg(18).Offset = 56
        TxtPortCfg(18).TextCfgProc = AddressOf GetTextPortCheckBox
        TxtPortCfg(18).BinCfgProc = AddressOf GetPort16Bits
        TxtPortCfg(18).Checkbox = True
        TxtPortCfg(18).SetValue = "1"

        TxtPortCfg(19).Key = "SOFTDCD"
        TxtPortCfg(19).CfgField = SOFTDCDBox
        TxtPortCfg(19).Offset = 58
        TxtPortCfg(19).TextCfgProc = AddressOf GetTextPortCheckBox
        TxtPortCfg(19).BinCfgProc = AddressOf GetPort16Bits
        TxtPortCfg(19).Checkbox = True
        TxtPortCfg(19).SetValue = "1"

        TxtPortCfg(20).Key = "FRACK"
        TxtPortCfg(20).CfgField = FRACKBox
        TxtPortCfg(20).Offset = 60
        TxtPortCfg(20).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(20).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(21).Key = "RESPTIME"
        TxtPortCfg(21).CfgField = RESPTIMEBox
        TxtPortCfg(21).Offset = 62
        TxtPortCfg(21).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(21).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(22).Key = "RETRIES"
        TxtPortCfg(22).CfgField = RETRIESBox
        TxtPortCfg(22).Offset = 64
        TxtPortCfg(22).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(22).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(23).Key = "PACLEN"
        TxtPortCfg(23).CfgField = PACLENBox
        TxtPortCfg(23).Offset = 66
        TxtPortCfg(23).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(23).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(24).Key = "MAXDIGIS"
        TxtPortCfg(24).CfgField = MAXDIGISBox
        TxtPortCfg(24).Offset = 123
        TxtPortCfg(24).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(24).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(25).Key = "DIGIFLAG"
        TxtPortCfg(25).CfgField = DIGIFLAGBox
        TxtPortCfg(25).Offset = 70
        TxtPortCfg(25).TextCfgProc = AddressOf GetTextPort8Bits
        TxtPortCfg(25).BinCfgProc = AddressOf GetPort8Bits

        TxtPortCfg(26).Key = "DIGIPORT"
        TxtPortCfg(26).CfgField = DIGIPORTBox
        TxtPortCfg(26).Offset = 71
        TxtPortCfg(26).TextCfgProc = AddressOf GetTextPort8Bits
        TxtPortCfg(26).BinCfgProc = AddressOf GetPort8Bits


        TxtPortCfg(27).Key = "CWID"
        TxtPortCfg(27).CfgField = CWIDBox
        TxtPortCfg(27).Offset = 80
        TxtPortCfg(27).Len = 10
        TxtPortCfg(27).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(27).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(28).Key = "CWIDTYPE"
        TxtPortCfg(28).CfgField = CWIDTYPEBox
        TxtPortCfg(28).Offset = 121
        TxtPortCfg(28).TextCfgProc = AddressOf GetTextPortCheckBox
        TxtPortCfg(28).BinCfgProc = AddressOf GetPort8Bits
        TxtPortCfg(28).Checkbox = True
        TxtPortCfg(28).SetValue = "ONOFF"

        TxtPortCfg(29).Key = "PORTCALL"
        TxtPortCfg(29).CfgField = PORTCALLBox
        TxtPortCfg(29).Offset = 90
        TxtPortCfg(29).Len = 10
        TxtPortCfg(29).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(29).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(30).Key = "PORTALIAS"
        TxtPortCfg(30).CfgField = PORTALIASBox
        TxtPortCfg(30).Offset = 100
        TxtPortCfg(30).Len = 10
        TxtPortCfg(30).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(30).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(31).Key = "PORTALIAS2"
        TxtPortCfg(31).CfgField = PORTALIAS2Box
        TxtPortCfg(31).Offset = 200
        TxtPortCfg(31).Len = 10
        TxtPortCfg(31).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(31).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(32).Key = "ALIAS_IS_BBS"
        TxtPortCfg(32).CfgField = ALIAS_IS_BBSBox
        TxtPortCfg(32).Offset = 78
        TxtPortCfg(32).TextCfgProc = AddressOf GetTextPortCheckBox
        TxtPortCfg(32).BinCfgProc = AddressOf GetPort16Bits
        TxtPortCfg(32).Checkbox = True
        TxtPortCfg(32).SetValue = "1"

        TxtPortCfg(33).Key = "BBSFLAG"
        TxtPortCfg(33).CfgField = BBSFLAGBox
        TxtPortCfg(33).Offset = 44
        TxtPortCfg(33).TextCfgProc = AddressOf GetTextPortCheckBox
        TxtPortCfg(33).BinCfgProc = AddressOf GetPort16Bits
        TxtPortCfg(33).Checkbox = True
        TxtPortCfg(33).SetValue = "1"

        TxtPortCfg(34).Key = "MINQUAL"
        TxtPortCfg(34).CfgField = MINQUALBox
        TxtPortCfg(34).Offset = 122
        TxtPortCfg(34).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(34).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(35).Key = "NODESPACLEN"
        TxtPortCfg(35).CfgField = NODESPACLENBox
        TxtPortCfg(35).Offset = 116
        TxtPortCfg(35).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(35).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(36).Key = "QUALADJUST"
        TxtPortCfg(36).CfgField = QUALADJUSTBox
        TxtPortCfg(36).Offset = 68
        TxtPortCfg(36).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(36).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(37).Key = "BCALL"
        TxtPortCfg(37).CfgField = BCALLBox
        TxtPortCfg(37).Offset = 226
        TxtPortCfg(37).Len = 10
        TxtPortCfg(37).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(37).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(38).Key = "UNPROTO"
        TxtPortCfg(38).CfgField = UNPROTOBox
        TxtPortCfg(38).Offset = 128
        TxtPortCfg(38).Len = 72
        TxtPortCfg(38).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(38).BinCfgProc = AddressOf GetPortString

        TxtPortCfg(39).Key = "L3ONLY"
        TxtPortCfg(39).CfgField = L3ONLYBox
        TxtPortCfg(39).Offset = 110
        TxtPortCfg(39).TextCfgProc = AddressOf GetTextPortCheckBox
        TxtPortCfg(39).BinCfgProc = AddressOf GetPort16Bits
        TxtPortCfg(39).Checkbox = True
        TxtPortCfg(39).SetValue = "1"

        TxtPortCfg(40).Key = "INTERLOCK"
        TxtPortCfg(40).CfgField = INTERLOCKBox
        TxtPortCfg(40).Offset = 114
        TxtPortCfg(40).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(40).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(41).Key = "TXPORT"
        TxtPortCfg(41).CfgField = TXPORTBox
        TxtPortCfg(41).Offset = 118
        TxtPortCfg(41).TextCfgProc = AddressOf GetTextPort8Bits
        TxtPortCfg(41).BinCfgProc = AddressOf GetPort8Bits

        TxtPortCfg(42).Key = "USERS"
        TxtPortCfg(42).CfgField = USERSBox
        TxtPortCfg(42).Offset = 74
        TxtPortCfg(42).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(42).BinCfgProc = AddressOf GetPort16Bits

        TxtPortCfg(43).Key = "VALIDCALLS"
        TxtPortCfg(43).CfgField = VALIDCALLSBox
        TxtPortCfg(43).Offset = 256
        TxtPortCfg(43).Len = 256
        TxtPortCfg(43).TextCfgProc = AddressOf GetTextPortString
        TxtPortCfg(43).BinCfgProc = AddressOf GetPortString


        TxtPortCfg(44).Key = "DIGIMASK"
        TxtPortCfg(44).CfgField = DIGIMASKBox
        TxtPortCfg(44).Offset = 72
        TxtPortCfg(44).TextCfgProc = AddressOf GetTextPort16Bits
        TxtPortCfg(44).BinCfgProc = AddressOf GetPort16Bits



    End Sub

    Delegate Function GetTextPortValue(ByVal Value As String, ByVal Param As Integer) As Boolean
    Delegate Function GetPortValue(ByVal Offset As Integer, ByVal length As Integer) As Object

    Function GetTextPort8Bits(ByVal Value As String, ByVal Param As Integer) As Boolean

        If Not IsNumeric(Value) Then Return False

        Config(PortConfigPointer + TxtPortCfg(Param).Offset) = Value

        Return True

    End Function

    Function GetTextPortCheckBox(ByVal Value As String, ByVal Param As Integer) As Boolean

        If Value = TxtPortCfg(Param).SetValue Then

            Config(PortConfigPointer + TxtPortCfg(Param).Offset) = 1

        End If

        Return True

    End Function

    Function GetTextPortMHeard(ByVal Value As String, ByVal Param As Integer) As Boolean

        ' Unspecifed is Y, but logic is inverted, so N set to 1

        If Value = "N" Then
            Config(PortConfigPointer + TxtPortCfg(Param).Offset) = Asc("N")
        End If

        Return True

    End Function
    Function GetTextPort16Bits(ByVal Value As String, ByVal Param As Integer) As Boolean

        Dim i As Integer, j As Integer

        If Not IsNumeric(Value) Then Return False

        i = Value

        If i = -1 Then i = 1

        j = Int(i / 256)
        i = i - j * 256
        Config(PortConfigPointer + TxtPortCfg(Param).Offset) = i
        Config(PortConfigPointer + TxtPortCfg(Param).Offset + 1) = j

        Return True

    End Function

    Function GetTextPort16BitsHex(ByVal Value As String, ByVal Param As Integer) As Boolean

        Dim i As Integer, j As Integer

        ' Allow a trailing "H"

        Value = UCase(Value)

        For i = 1 To Value.Length

            If Not Uri.IsHexDigit(Mid(Value, i, 1)) Then

                If i = Value.Length And Mid(Value, i, 1) = "H" Then

                    Value = Microsoft.VisualBasic.Left(Value, i - 1)
                Else
                    Return False
                End If

            End If

        Next

        i = Convert.ToInt32(Value, 16)

        If i = -1 Then i = 1

        j = Int(i / 256)
        i = i - j * 256
        Config(PortConfigPointer + TxtPortCfg(Param).Offset) = i
        Config(PortConfigPointer + TxtPortCfg(Param).Offset + 1) = j

        Return True

    End Function

    Function GetTextPortString(ByVal Value As String, ByVal Param As Integer) As Boolean

        Dim i As Integer, j As Integer

        If Value = Nothing Then
            j = 0
        Else
            j = Value.Length
        End If

        If j > TxtPortCfg(Param).Len Then j = TxtPortCfg(Param).Len

        For i = 0 To j - 1

            Config(PortConfigPointer + i + TxtPortCfg(Param).Offset) = Asc(Mid(Value, i + 1, 1))

        Next

        For i = j To TxtPortCfg(Param).Len - 1

            Config(PortConfigPointer + i + TxtPortCfg(Param).Offset) = 0 'Padding

        Next

        Return True

    End Function

    Function GetTextType(ByVal Value As String, ByVal Param As Integer) As Boolean

        Dim i As Integer

        For i = 0 To Types.Length
            If Types(i) = Value Then
                Config(PortConfigPointer + TxtPortCfg(Param).Offset) = 2 * unMapTypes(i)
                Return True
            End If
        Next

        Return False

    End Function

    Public Function GetTextPortChannel(ByVal Value As String, ByVal Param As Integer) As Boolean

        Value = Asc(Value)

        If Not IsNumeric(Value) Then Return False

        Config(PortConfigPointer + TxtPortCfg(Param).Offset) = Value

        Return True

    End Function

    Function GetTextProtocol(ByVal Value As String, ByVal Param As Integer) As Boolean

        Dim i As Integer

        For i = 0 To Protos.Length
            If Protos(i) = Value Then
                If i = 6 Then i = 5 ' PACTOR = WINMOR
                Config(PortConfigPointer + TxtPortCfg(Param).Offset) = 2 * i
                Return True
            End If
        Next

        Return False

    End Function

    Function GetTextKissOptions(ByVal Value As String, ByVal Param As Integer) As Boolean

        Dim i As Integer, KissParam As String, KissOption As Integer = 0

        Value = Value & ","

        Do

            i = InStr(Value, ",")

            KissParam = Microsoft.VisualBasic.Left(Value, i - 1)
            Value = Mid(Value, i + 1)

            If KissParam = "CHECKSUM" Then KissOption = KissOption + 1 _
             Else If KissParam = "POLLED" Then KissOption = KissOption + 2 _
             Else If KissParam = "ACKMODE" Then KissOption = KissOption + 4 _
             Else If KissParam = "SLAVE" Then KissOption = KissOption + 8 Else Return False

        Loop While Value <> ""

        Config(PortConfigPointer + TxtPortCfg(Param).Offset) = KissOption

        Return True

    End Function

    Public Sub Exit_Click(ByVal sender As Object, ByVal e As System.EventArgs)

        SavePortInfo()

        Form1.CopyConfigtoArray()

        If CompareConfig() = False Or AGWAppl <> OriginalAGWAppl Then

            If MsgBox("Changes have not been saved - do you want to save before exiting?", _
                 MsgBoxStyle.YesNo + MsgBoxStyle.MsgBoxSetForeground, "WinBPQ Config") = MsgBoxResult.Yes Then
                If Not Form1.SaveConfigAsBinary() Then Exit Sub
            End If
        End If

        End

    End Sub

 
End Module