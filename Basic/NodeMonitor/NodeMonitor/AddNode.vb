Imports System.Windows.Forms
Imports System.IO

Public Class AddNode

   Dim SelectedItem As Integer = -1

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

         ReDim Preserve ChatNodes(ChatNodeIndex + 1)

         ChatNodes(ChatNodeIndex).Callsign = Callsign
         ChatNodes(ChatNodeIndex).NAlias = NAlias
         ChatNodes(ChatNodeIndex).Lat = Lat.ToString
         ChatNodes(ChatNodeIndex).Lon = Lon.ToString
         ChatNodes(ChatNodeIndex).downIcon = downIcon
         ChatNodes(ChatNodeIndex).upIcon = upIcon
         ChatNodes(ChatNodeIndex).Comment = Comment
         If HoverButton.Checked Then
            ChatNodes(ChatNodeIndex).PopupMode = 0
         Else
            ChatNodes(ChatNodeIndex).PopupMode = 1
         End If

         ChatNodeIndex = ChatNodeIndex + 1

      Else

         ' Find call 

         For i = 0 To ChatNodeIndex - 1

            If (ChatNodes(i).Callsign = CallBox.SelectedItem.ToString) Then

               ChatNodes(i).NAlias = NAlias
               ChatNodes(i).Lat = Lat.ToString
               ChatNodes(i).Lon = Lon.ToString
               ChatNodes(i).downIcon = downIcon
               ChatNodes(i).upIcon = upIcon
               ChatNodes(i).Comment = Comment
               If HoverButton.Checked Then
                  ChatNodes(1).PopupMode = 0
               Else
                  ChatNodes(1).PopupMode = 1

               End If

            End If

         Next

      End If

      ' Refresh the call combo box

      CallBox.Items.Clear()
      CallBox.Text = ""


      For i = 0 To ChatNodeIndex - 1

         If ChatNodes(i).Deleted = False Then CallBox.Items.Add(ChatNodes(i).Callsign)

      Next

      SaveNodesFile()

      Me.DialogResult = System.Windows.Forms.DialogResult.OK

   End Sub

   Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click
      Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
      Me.Close()
   End Sub

   Private Sub AddNode_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      Dim i As Integer
      ' Fill in the call combo box

      For i = 0 To ChatNodeIndex - 1

         If ChatNodes(i).Deleted = False Then CallBox.Items.Add(ChatNodes(i).Callsign)

      Next


      'N4JOA:
      'Updated: 2009-10-17 13:42:59z
      'Position: 26°33'12" N 80°4'1" W

   End Sub

   Private Sub CallBox_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CallBox.SelectedIndexChanged

      Dim i As Integer

      ' Find call 

      For i = 0 To ChatNodeIndex - 1

         If (ChatNodes(i).Callsign = CallBox.SelectedItem.ToString) Then

            LatBox.Text = ChatNodes(i).Lat
            LonBox.Text = ChatNodes(i).Lon
            PopupBox.Text = ChatNodes(i).Comment
            HoverButton.Checked = (ChatNodes(i).PopupMode = 0)
            ClickButton.Checked = (ChatNodes(i).PopupMode = 1)
            UpIconBox.Text = ChatNodes(i).upIcon
            DownIconBox.Text = ChatNodes(i).downIcon

            LOC.Text = ToLOC(CDbl(ChatNodes(i).Lat), CDbl(ChatNodes(i).Lon))
            DDMMSS.Text = ToDDMMSS(CDbl(ChatNodes(i).Lat), CDbl(ChatNodes(i).Lon))

            Try

               PictureBox1.Image = New Bitmap(UpIconBox.Text)
               PictureBox2.Image = New Bitmap(DownIconBox.Text)

               PictureBox1.Size = PictureBox1.Image.Size
               PictureBox2.Size = PictureBox1.Image.Size

            Catch ex As Exception

            End Try

            SelectedItem = i

            Exit For

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

      Dim Position As String
      Dim DD As Integer, MM As Integer, SS As Integer
      Dim NS As String = "N"
      Dim EW As String = "E"
      Dim Mins As Double

      If Lat < 0 Then
         Lat = -Lat
         NS = "S"
      End If

      If Lon < 0 Then
         Lon = -Lon
         EW = "W"
      End If

      DD = CInt(Int(Lat))
      Mins = (Lat - DD) * 60
      MM = CInt(Int(Mins))
      SS = CInt((Mins - MM) * 60)

      Position = Format(DD, "D") & "°" & Format(MM, "D") & "'" & Format(SS, "D") & """ " & NS

      DD = CInt(Int(Lon))
      Mins = (Lon - DD) * 60
      MM = CInt(Int(Mins))
      SS = CInt((Mins - MM) * 60)

      Position = Position & " " & Format(DD, "D") & "°" & Format(MM, "D") & "'" & Format(SS, "D") & """ " & EW

      Return Position

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
      DDMMSS.Text = ToDDMMSS(CDbl(Lat), CDbl(Lon))


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

      Try

         PictureBox1.Image.Dispose()
         PictureBox2.Image.Dispose()

      Catch ex As Exception

      End Try

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

   Private Sub Delete_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Delete_Button.Click

      ' Refresh the call combo box

      Dim i As Integer

      ChatNodes(SelectedItem).Deleted = True

      CallBox.Items.Clear()
      CallBox.Text = ""

      For i = 0 To ChatNodeIndex - 1

         If ChatNodes(i).Deleted = False Then CallBox.Items.Add(ChatNodes(i).Callsign)

      Next


   End Sub


 
End Class