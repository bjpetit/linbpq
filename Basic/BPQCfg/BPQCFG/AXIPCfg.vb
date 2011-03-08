Imports System.Windows.Forms

Public Class AXIPCfg

    Public AXIPPort(128) As NumTextBox
    Public AXIPKeepalive(128) As NumTextBox
    Public AXIPCall(128) As CallsignTextBox
    Public AXIPHost(128) As TextBox
    Public AXIPDel(128) As CheckBox

    Public NoofAXIPMaps As Integer
    Public NumberofUDPPorts As Integer
    Const MaxUDPPorts = 8
    Dim UDPPort(MaxUDPPorts) As Integer

    Public HostSize = New System.Drawing.Size(150, 17)

    Private Sub Save_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Save_Button.Click

        Dim i As Integer
        Dim File As System.IO.StreamWriter
        Dim Mapline As String

        If Not (Me.ValidateChildren()) Then

            MsgBox("Validation failed", MsgBoxStyle.MsgBoxSetForeground, "WinBPQ AXIP Config")
            Exit Sub

        End If

        File = My.Computer.FileSystem.OpenTextFileWriter(BPQDirectory & "\bpqaxip.cfg", False, System.Text.Encoding.ASCII)

        File.WriteLine("#	Config file for BPQAXIP")
        File.WriteLine("")

        If UDPPort1.Text <> "0" And UDPPort1.Text <> "" Then File.WriteLine("UDP " & UDPPort1.Text)
        If UDPPort2.Text <> "0" And UDPPort2.Text <> "" Then File.WriteLine("UDP " & UDPPort2.Text)
        If UDPPort3.Text <> "0" And UDPPort3.Text <> "" Then File.WriteLine("UDP " & UDPPort3.Text)
        If UDPPort4.Text <> "0" And UDPPort4.Text <> "" Then File.WriteLine("UDP " & UDPPort4.Text)
        If UDPPort5.Text <> "0" And UDPPort5.Text <> "" Then File.WriteLine("UDP " & UDPPort5.Text)
        If UDPPort6.Text <> "0" And UDPPort6.Text <> "" Then File.WriteLine("UDP " & UDPPort6.Text)
        If UDPPort7.Text <> "0" And UDPPort7.Text <> "" Then File.WriteLine("UDP " & UDPPort7.Text)
        If UDPPort8.Text <> "0" And UDPPort8.Text <> "" Then File.WriteLine("UDP " & UDPPort8.Text)

        File.WriteLine("")

        For i = 1 To NoofAXIPMaps

            If AXIPCall(i).Text <> "" Then

                Mapline = "MAP " & AXIPCall(i).Text & Chr(9) & AXIPHost(i).Text
                If AXIPPort(i).Text <> "" And AXIPPort(i).Text <> "0" Then Mapline = Mapline & " UDP " & AXIPPort(i).Text
                If AXIPKeepalive(i).Text <> "" And AXIPKeepalive(i).Text <> "0" Then Mapline = Mapline & " KEEPALIVE " & AXIPKeepalive(i).Text
                File.WriteLine(Mapline)

            End If

        Next

        File.WriteLine("")

        If EnableMHeard.Checked = True Then File.WriteLine("MHEARD ON")

        File.Close()

        Me.DialogResult = System.Windows.Forms.DialogResult.OK
        Me.Close()
    End Sub

    Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click
        Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
        Me.Close()
    End Sub




    Private Sub AXIPCfg_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      '  Dim Line As String
      ' Dim Textfile As System.IO.StreamReader
      '       Textfile = My.Computer.FileSystem.OpenTextFileReader(BPQDirectory & "\bpqaxip.cfg", System.Text.Encoding.ASCII)


      '      Do
      'Line = Textfile.ReadLine
      'ProcessLine(Line)

      ' Loop Until Textfile.EndOfStream

      'Textfile.Close()

      'AddAXIPLine()

      'If UDPPort(1) <> 0 Then UDPPort1.Text = UDPPort(1)
      'If UDPPort(2) <> 0 Then UDPPort2.Text = UDPPort(2)
      'If UDPPort(3) <> 0 Then UDPPort3.Text = UDPPort(3)
      'If UDPPort(4) <> 0 Then UDPPort4.Text = UDPPort(4)
      'If UDPPort(5) <> 0 Then UDPPort5.Text = UDPPort(5)
      'If UDPPort(6) <> 0 Then UDPPort6.Text = UDPPort(6)
      'If UDPPort(7) <> 0 Then UDPPort7.Text = UDPPort(7)
      'If UDPPort(8) <> 0 Then UDPPort8.Text = UDPPort(8)

    End Sub

    Private Sub AddAXIPLine()

        Dim Rowspacing As Integer = 21

        NoofAXIPMaps = NoofAXIPMaps + 1

        AXIPCall(NoofAXIPMaps) = New CallsignTextBox
        AXIPHost(NoofAXIPMaps) = New TextBox
        AXIPPort(NoofAXIPMaps) = New NumTextBox(65535)
        AXIPKeepalive(NoofAXIPMaps) = New NumTextBox(3600)
        AXIPDel(NoofAXIPMaps) = New CheckBox


        AXIPCall(NoofAXIPMaps).Location = New System.Drawing.Point(Col1, Rowspacing * NoofAXIPMaps + 90)
        AXIPHost(NoofAXIPMaps).Location = New System.Drawing.Point(Col1 + 75, Rowspacing * NoofAXIPMaps + 90)
        AXIPPort(NoofAXIPMaps).Location = New System.Drawing.Point(Col1 + 230, Rowspacing * NoofAXIPMaps + 90)
        AXIPKeepalive(NoofAXIPMaps).Location = New System.Drawing.Point(Col1 + 283, Rowspacing * NoofAXIPMaps + 90)
        AXIPDel(NoofAXIPMaps).Location = New System.Drawing.Point(Col1 + 323, Rowspacing * NoofAXIPMaps + 87)

        AXIPCall(NoofAXIPMaps).Size = calltextboxsize
        AXIPHost(NoofAXIPMaps).Size = HostSize
        AXIPPort(NoofAXIPMaps).Size = TextBoxSize
        AXIPKeepalive(NoofAXIPMaps).Size = SmallTextBoxSize

        AXIPHost(NoofAXIPMaps).Tag = NoofAXIPMaps
        AddHandler AXIPHost(NoofAXIPMaps).Validating, AddressOf HostNameValidating
        AddHandler AXIPHost(NoofAXIPMaps).Validated, AddressOf ValidatedSub

        AXIPDel(NoofAXIPMaps).Tag = NoofAXIPMaps
        AddHandler AXIPDel(NoofAXIPMaps).CheckedChanged, AddressOf AXIPDel_CheckedChanged

        Me.Controls.Add(AXIPCall(NoofAXIPMaps))
        Me.Controls.Add(AXIPHost(NoofAXIPMaps))
        Me.Controls.Add(AXIPPort(NoofAXIPMaps))
        Me.Controls.Add(AXIPKeepalive(NoofAXIPMaps))
        Me.Controls.Add(AXIPDel(NoofAXIPMaps))

    End Sub

    Private Sub AXIPDel_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)
        Debug.Print(sender.tag)


    End Sub

    Public Sub HostNameValidating(ByVal sender As Object, ByVal e As System.ComponentModel.CancelEventArgs)

        sender.Text = LTrim(RTrim(UCase(sender.Text)))

        If sender.tag = NoofAXIPMaps And AXIPCall(sender.tag).Text <> "" Then AddAXIPLine()

        If sender.Text.Length > 80 Then

            Form1.ErrorProvider1.SetError(sender, "Too Long (max 80 chars")
            e.Cancel = True
            sender.Select(0, sender.Text.Length)

        End If

    End Sub

sub ProcessLine(buf as String)

        Dim i As Integer

        Do

            i = InStr(buf, Chr(9))

            If i > 0 Then Mid(buf, i, 1) = " "

        Loop Until i = 0


        If (buf = "") Or (buf Is Nothing) Then Exit Sub

        If Mid(buf, 1, 1) = "#" Then Exit Sub
        If Mid(buf, 1, 1) = ";" Then Exit Sub

        buf = buf & "                     "

        If UCase(Mid(buf, 1, 4)) = "UDP " Then

            If NumberofUDPPorts > MAXUDPPORTS Then NumberofUDPPorts = NumberofUDPPorts - 1

            i = InStr(5, buf, " ")

            'if (p_udpport == NULL) return (FALSE);

            NumberofUDPPorts = NumberofUDPPorts + 1

            Try
                UDPPort(NumberofUDPPorts) = Convert.ToInt32(Mid(buf, 5, i - 5))

            Catch ex As Exception

            End Try


            Exit Sub

        End If

        If UCase(Mid(buf, 1, 7)) = "MHEARD " Then

            EnableMHeard.Checked = True
            Exit Sub

        End If

        If UCase(Mid(buf, 1, 4)) = "MAP " Then

            AddAXIPLine()

            i = InStr(5, buf, " ")

            AXIPCall(NoofAXIPMaps).Text = RTrim(Mid(buf, 5, i - 5))

            buf = LTrim(Mid(buf, i))

            i = InStr(1, buf, " ")

            AXIPHost(NoofAXIPMaps).Text = RTrim(Mid(buf, 1, i))

            buf = LTrim(Mid(buf, i))

            If buf = "" Then Exit Sub

            TryDynamic(buf)
            TryKeepalive(buf)
            TryUDP(buf)

            If buf = "" Then Exit Sub

            TryDynamic(buf)
            TryKeepalive(buf)
            TryUDP(buf)

            If buf = "" Then Exit Sub

            TryDynamic(buf)
            TryKeepalive(buf)
            TryUDP(buf)

            '	if (convtoax25(p_call,axcall,&calllen))

 

        End If

    End Sub

    Sub TryDynamic(ByRef buf As String)

        If UCase(Mid(buf, 1, 7)) = "DYNAMIC" Then

            buf = LTrim(Mid(buf, 8))

        End If
    End Sub

    Sub TryKeepalive(ByRef buf As String)

        Dim i As Integer

        If UCase(Mid(buf, 1, 9)) = "KEEPALIVE" Then

            buf = LTrim(Mid(buf, 10))

            i = InStr(1, buf, " ")

            Try
                AXIPKeepalive(NoofAXIPMaps).Text = Convert.ToInt32(Mid(buf, 1, i))

            Catch ex As Exception

            End Try

            buf = LTrim(Mid(buf, i))

        End If

    End Sub

    Sub TryUDP(ByRef buf As String)

        Dim i As Integer

        If UCase(Mid(buf, 1, 4)) = "UDP " Then

            buf = LTrim(Mid(buf, 5))

            If buf = "" Then Exit Sub

            i = InStr(1, buf, " ")

            Try
                AXIPPort(NoofAXIPMaps).Text = Convert.ToInt32(Mid(buf, 1, i))

            Catch ex As Exception

            End Try

            buf = LTrim(Mid(buf, i))


        End If



    End Sub

    Private Sub UDPPort2_TextChanged(ByVal sender As System.Object, ByVal e As System.EventArgs)

    End Sub
End Class