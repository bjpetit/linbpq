Imports System.Windows.Forms
Imports System.IO

Public Class AddNode

   Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click

      Dim Callsign As String
      Dim Lat As Double
      Dim Lon As Double
      Dim Comment As String
      Dim upIcon As String
      Dim downIcon As String
      Dim NAlias As String = ""

      Dim i As Integer

      Try

         Lat = DecodeLat(LatBox.Text)
         Lon = DecodeLon(LonBox.Text)


      Catch ex As Exception
         MsgBox("Problem decoding Lat/Lon")
      End Try

      LatBox.Text = Lat.ToString
      LonBox.Text = Lon.ToString

      Comment = PopupBox.Text
      upIcon = UpIconBox.Text
      downIcon = DownIconBox.Text

      If CallBox.SelectedIndex = -1 Then

         Callsign = UCase(CallBox.Text)

         If Callsign = "" Then
            Exit Sub
         End If

         ReDim Preserve Nodes(NodeIndex + 1)

         Nodes(NodeIndex).Callsign = Callsign
         Nodes(NodeIndex).NAlias = NAlias
         Nodes(NodeIndex).Lat = Lat.ToString
         Nodes(NodeIndex).Lon = Lon.ToString
         Nodes(NodeIndex).downIcon = downIcon
         Nodes(NodeIndex).upIcon = upIcon
         Nodes(NodeIndex).Comment = Comment
         If HoverButton.Checked Then
            Nodes(NodeIndex).PopupMode = 0
         Else
            Nodes(NodeIndex).PopupMode = 1
         End If

         NodeIndex = NodeIndex + 1

      Else

         ' Find call 

         For i = 0 To NodeIndex - 1

            If (Nodes(i).Callsign = CallBox.SelectedItem.ToString) Then

               Nodes(i).NAlias = NAlias
               Nodes(i).Lat = Lat.ToString
               Nodes(i).Lon = Lon.ToString
               Nodes(i).downIcon = downIcon
               Nodes(i).upIcon = upIcon
               Nodes(i).Comment = Comment
               If HoverButton.Checked Then
                  Nodes(1).PopupMode = 0
               Else
                  Nodes(1).PopupMode = 1

               End If

            End If

         Next

      End If

         ' Refresh the call combo box

         CallBox.Items.Clear()
         CallBox.Text = ""


         For i = 0 To NodeIndex - 1

            CallBox.Items.Add(Nodes(i).Callsign)

         Next


         My.Computer.FileSystem.DeleteFile(My.Settings.FileName, FileIO.UIOption.OnlyErrorDialogs, _
               FileIO.RecycleOption.SendToRecycleBin, FileIO.UICancelOption.DoNothing)


         Using sw As StreamWriter = New StreamWriter(My.Settings.FileName)

            For i = 0 To NodeIndex - 1

            sw.WriteLine(Nodes(i).Callsign & "," & Nodes(i).NAlias & "," & _
                  Nodes(i).Lat & "," & Nodes(i).Lon & "," & Nodes(i).downIcon & "," & _
                  Nodes(i).upIcon & "," & Nodes(i).PopupMode & "," & Nodes(i).Comment)

            Next

            sw.Close()

         End Using

         Me.DialogResult = System.Windows.Forms.DialogResult.OK

   End Sub

   Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click
      Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
      Me.Close()
   End Sub

   Private Sub AddNode_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      Dim i As Integer
      ' Fill in the call combo box

      For i = 0 To NodeIndex - 1

         CallBox.Items.Add(Nodes(i).Callsign)

      Next


      'N4JOA:
      'Updated: 2009-10-17 13:42:59z
      'Position: 26°33'12" N 80°4'1" W

   End Sub

   Public Function DecodeLat(ByVal LString As String) As Double

      Dim North As Boolean = False
      Dim South As Boolean = False

      If IsNumeric(LString) Then Return CDbl(LString)

      LString = UCase(LString)

      If InStr(LString, "N") <> 0 Then North = True
      If InStr(LString, "S") <> 0 Then South = True

      Return DecodeLatLon(LString, South)

   End Function

   Public Function DecodeLon(ByVal LString As String) As Double

      Dim East As Boolean = False
      Dim West As Boolean = False

      If IsNumeric(LString) Then Return CDbl(LString)

      LString = UCase(LString)

      If InStr(LString, "E") <> 0 Then East = True
      If InStr(LString, "W") <> 0 Then West = True

      Return DecodeLatLon(LString, west)

   End Function


   Public Function DecodeLatLon(ByVal LatString As String, ByVal SW As Boolean) As Double

      Dim Val As Double

      Dim Degpos As Integer = 0
      Dim Minpos As Integer = 0
      Dim Secpos As Integer = 0
      Dim GotSecs As Boolean
      Dim Degs As Integer, Mins As Integer, Secs As Integer

      Degpos = InStr(LatString, "°")
      Minpos = InStr(LatString, "'")
      Secpos = InStr(LatString, """")

      GotSecs = Degpos > 0 And Minpos > 0 And Secpos > 0

      If GotSecs Then

         Degs = CInt(Microsoft.VisualBasic.Left(LatString, Degpos - 1))
         Mins = CInt(Microsoft.VisualBasic.Mid(LatString, Degpos + 1, Minpos - 1 - Degpos))
         Secs = CInt(Microsoft.VisualBasic.Mid(LatString, Minpos + 1, Secpos - 1 - Minpos))

      Else

         Return CDbl(LatString)

      End If

      Val = Degs + Mins / 60 + Secs / 3600

      If SW Then Val = -Val

      Return Val

   End Function
   Private Sub CallBox_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CallBox.SelectedIndexChanged

      Dim i As Integer

      ' Find call 

      For i = 0 To NodeIndex - 1

         If (Nodes(i).Callsign = CallBox.SelectedItem.ToString) Then

            LatBox.Text = Nodes(i).Lat
            LonBox.Text = Nodes(i).Lon
            PopupBox.Text = Nodes(i).Comment
            HoverButton.Checked = (Nodes(i).PopupMode = 0)
            ClickButton.Checked = (Nodes(i).PopupMode = 1)
            UpIconBox.Text = Nodes(i).upIcon
            DownIconBox.Text = Nodes(i).downIcon

            LOC.Text = ToLOC(CDbl(Nodes(i).Lat), CDbl(Nodes(i).Lon))

            PictureBox1.Image = New Bitmap(UpIconBox.Text)
            PictureBox2.Image = New Bitmap(DownIconBox.Text)

            PictureBox1.Size = PictureBox1.Image.Size
            PictureBox2.Size = PictureBox1.Image.Size


         End If

      Next

   End Sub

   Private Sub CallBox_TextChanged(ByVal sender As Object, ByVal e As EventArgs) Handles CallBox.TextChanged

      LatBox.Text = ""
      LonBox.Text = ""
      PopupBox.Text = ""
      UpIconBox.Text = ""
      DownIconBox.Text = ""

   End Sub

   Function ToLOC(ByVal Lat As Double, ByVal Lon As Double) As String

      Dim Locator As String = "XX00XX"
      Dim i As Integer
      Dim S1 As Double, S2 As Double

      Lon = Lon + 180
      Lat = Lat + 90

      S1 = Lon Mod 20

      i = CInt(Int(Lon / 20))
      Mid(Locator, 1, 1) = Chr(65 + i)

      S2 = S1 Mod 2

      i = CInt(Int(S1 / 2))
      Mid(Locator, 3, 1) = Chr(48 + i)

      i = CInt(Int(S2 * 12))
      Mid(Locator, 5, 1) = Chr(65 + i)


      S1 = Lat Mod 10

      i = CInt(Int(Lat / 10))
      Mid(Locator, 2, 1) = Chr(65 + i)

      S2 = S1 Mod 1

      i = CInt(Int(S1))
      Mid(Locator, 4, 1) = Chr(48 + i)

      i = CInt(Int(S2 * 24))
      Mid(Locator, 6, 1) = Chr(65 + i)



      Return Locator

   End Function

   Function ToDDMMSS(ByVal Lat As Double, ByVal Lon As Double) As String

      Dim Locator As String = "XX00XX"
      Dim i As Integer
      Dim S1 As Double, S2 As Double

      Lon = Lon + 180
      Lat = Lat + 90

      S1 = Lon Mod 20

      i = CInt(Int(Lon / 20))
      Mid(Locator, 1, 1) = Chr(65 + i)

      S2 = S1 Mod 2

      i = CInt(Int(S1 / 2))
      Mid(Locator, 3, 1) = Chr(48 + i)

      i = CInt(Int(S2 * 12))
      Mid(Locator, 5, 1) = Chr(65 + i)


      S1 = Lat Mod 10

      i = CInt(Int(Lat / 10))
      Mid(Locator, 2, 1) = Chr(65 + i)

      S2 = S1 Mod 1

      i = CInt(Int(S1))
      Mid(Locator, 4, 1) = Chr(48 + i)

      i = CInt(Int(S2 * 24))
      Mid(Locator, 6, 1) = Chr(65 + i)



      Return Locator

   End Function

   Function ToNMEA(ByVal Lat As Double, ByVal Lon As Double) As String

      Dim Locator As String = "XX00XX"
      Dim i As Integer
      Dim S1 As Double, S2 As Double

      Lon = Lon + 180
      Lat = Lat + 90

      S1 = Lon Mod 20

      i = CInt(Int(Lon / 20))
      Mid(Locator, 1, 1) = Chr(65 + i)

      S2 = S1 Mod 2

      i = CInt(Int(S1 / 2))
      Mid(Locator, 3, 1) = Chr(48 + i)

      i = CInt(Int(S2 * 12))
      Mid(Locator, 5, 1) = Chr(65 + i)


      S1 = Lat Mod 10

      i = CInt(Int(Lat / 10))
      Mid(Locator, 2, 1) = Chr(65 + i)

      S2 = S1 Mod 1

      i = CInt(Int(S1))
      Mid(Locator, 4, 1) = Chr(48 + i)

      i = CInt(Int(S2 * 24))
      Mid(Locator, 6, 1) = Chr(65 + i)



      Return Locator

   End Function
   Private Sub Check_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Check.Click

      Dim Lat As Double
      Dim Lon As Double
      Dim NAlias As String = ""
 
      Try

         Lat = DecodeLat(LatBox.Text)
         Lon = DecodeLon(LonBox.Text)


      Catch ex As Exception
         MsgBox("Problem decoding Lat/Lon")
      End Try

      LatBox.Text = Lat.ToString
      LonBox.Text = Lon.ToString

      LOC.Text = ToLOC(CDbl(Lat), CDbl(Lon))

   End Sub

   Private Sub RadioButton1_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles HoverButton.CheckedChanged

   End Sub

   Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles IconButton.Click


      Dim Callsign As String
      Dim Pos As Integer

      Dim ImageX As Integer
      Dim ImageY As Integer = 25

      If CallBox.SelectedIndex = -1 Then

         Callsign = UCase(CallBox.Text)
      Else
         Callsign = CallBox.SelectedItem.ToString

      End If

      Pos = InStr(Callsign, "-")
      If Pos > 0 Then Callsign = Mid(Callsign, 1, Pos - 1)

      ImageX = Callsign.Length * 14 + IconTune.Value * 4 - 20


      Dim image1 As Bitmap = New Bitmap(ImageX, ImageY)
      Dim image2 As Bitmap = New Bitmap(ImageX, ImageY)


      Dim g1 As Graphics = Graphics.FromImage(image1)
      Dim g2 As Graphics = Graphics.FromImage(image2)

      Dim f As Font = New Font("system", 14, FontStyle.Regular)

      g1.FillRectangle(Brushes.LightGreen, 0, 0, ImageX, ImageY - 3)
      g1.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 2), ImageY - 3, 5, 1)
      g1.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 1), ImageY - 2, 3, 1)

      g1.DrawString(Callsign, f, Brushes.Black, 0, 0)

      g2.FillRectangle(Brushes.Red, 0, 0, ImageX, ImageY - 3)
      g2.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 2), ImageY - 3, 5, 1)
      g2.FillRectangle(Brushes.Black, CInt(ImageX / 2 - 1), ImageY - 2, 3, 1)


      g2.DrawString(Callsign, f, Brushes.White, 0, 0)

      PictureBox1.Image.Dispose()
      PictureBox2.Image.Dispose()

      image1.Save(Callsign & ".ok.png", System.Drawing.Imaging.ImageFormat.Png)
      image2.Save(Callsign & ".down.png", System.Drawing.Imaging.ImageFormat.Png)

      UpIconBox.Text = Callsign & ".ok.png"
      DownIconBox.Text = Callsign & ".down.png"

      PictureBox1.Image = image1
      PictureBox1.Size = image1.Size

      PictureBox2.Image = image2
      PictureBox2.Size = image2.Size

      g1.Dispose()
      g2.Dispose()


      f.Dispose()


   End Sub

   Private Sub PictureBox1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles PictureBox1.Click

   End Sub
End Class
