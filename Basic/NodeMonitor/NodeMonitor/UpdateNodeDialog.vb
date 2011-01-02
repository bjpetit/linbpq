Imports System.Windows.Forms
Imports System
Imports System.Net
Imports System.IO
Imports System.Text
Imports System.Threading
Imports Microsoft.VisualBasic




Public Class UpdateNodeDialog

   Dim SelectedItem As Integer

   Private Sub OK_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OK_Button.Click

      Dim Lat As Double, Lon As Double

      With CallsignData(SelectedItem)

         .Locator = LocatorBox.Text

         Form1.FromLOC(.Locator, Lat, Lon)

         Lat = Lat + Rnd() / 24
         Lon = Lon + Rnd() / 12

         .Lat = Lat.ToString
         .Lon = Lon.ToString
         .LocType = LOC

         LatBox.Text = .Lat
         LonBox.Text = .Lon


      End With
   End Sub

   Private Sub Cancel_Button_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Cancel_Button.Click
      Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
      Me.Close()
      WebWindow.WebBrowser1.Dispose()
      WebWindow.Close()

   End Sub

   Private Sub UpdateNode_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

      Dim i As Integer

      For i = 0 To CallsignData.Length - 1

         With CallsignData(i)

            If .Lon = "0" Then CallBox.Items.Add(.Callsign)

         End With

      Next

   End Sub

   Private Sub CallBox_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CallBox.SelectedIndexChanged

      Dim i As Integer

      For i = 0 To CallsignData.Length - 1

         With CallsignData(i)

            If (.Callsign = CallBox.SelectedItem.ToString) Then

               LatBox.Text = .Lat
               LonBox.Text = .Lon
               LocatorBox.Text = .Locator

               SelectedItem = i

               Exit For

            End If
         End With
      Next

   End Sub

   Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
      WebWindow.WebBrowser1.Navigate(New Uri("http://www.qrz.com/db/" & CallsignData(SelectedItem).Callsign))
      WebWindow.Visible = True

   End Sub

   Public Shared allDone As New ManualResetEvent(False)

   Private Sub Button2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button2.Click

      Dim client As New WebClient()
      Dim Reply As String = ""
      Dim callsign As String = "gm8bpq"
      Dim text As String
      Dim len As Integer

      LocatorBox.Text = ""

      Try
         text = WebWindow.WebBrowser1.DocumentText
         len = text.Length

         len = InStr(text, "Grid Square")
         text = Mid(text, len)
         len = InStr(text, ">")
         text = Mid(text, len + 1)

         len = InStr(text, ">")
         text = Mid(text, len + 1, 6)

         LocatorBox.Text = text

      Catch ex As Exception

         MsgBox("Find Locator Failed " & ex.Message)

      End Try

   End Sub

End Class




