Imports System.Text
Imports System
Imports System.IO

Public Class Form1

   Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click

      Dim returnValue As String

      Using sw As StreamWriter = New StreamWriter("c:\aeaport.log.txt")

         Try

            returnValue = File.ReadAllText("c:\aeaport.log")

            Dim strLines() As String = returnValue.Split(Chr(10))
            Dim Elements() As String
            Dim Fields() As String
            Dim HexBytes() As String
            Dim i As Integer, j As Integer
            Dim DispChars As String
            Dim spaces As String = Space(100)



            For Each strLine As String In strLines

               Elements = Split(strLine, vbTab)
               If Elements(3) = "IRP_MJ_READ" Or Elements(3) = "IRP_MJ_WRITE" Then

                  Fields = Split(Elements(6), ":")
                  HexBytes = Split(Fields(1), " ")

                  DispChars = "                          "
                  For i = 1 To HexBytes.Length - 2
                     j = Convert.ToInt32(HexBytes(i), 16)
                     If j > 31 And j < 127 Then
                        Mid(DispChars, i, 1) = Chr(j)
                     Else
                        Mid(DispChars, i, 1) = "."
                     End If

                  Next


                  sw.Write(Elements(1) & vbTab & Mid(Elements(3), 8) & vbTab & Mid(Fields(1) & spaces, 1, 70) & DispChars & vbCrLf)

               End If

            Next

         Catch ex As Exception

         End Try

      End Using

   End Sub
End Class
