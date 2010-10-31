Imports System.IO

Module NodeMonitor

   Public Function DecodeLat(ByVal LString As String) As Double

      Dim North As Boolean = False
      Dim South As Boolean = False
      Dim i As Integer

      If IsNumeric(LString) Then Return CDbl(LString)

      LString = UCase(LString)

      i = InStr(LString, "N")

      If i <> 0 Then
         North = True
         LString = Microsoft.VisualBasic.Left(LString, i - 1)
      End If

      i = InStr(LString, "S")

      If i <> 0 Then
         South = True
         LString = Microsoft.VisualBasic.Left(LString, i - 1)
      End If

      Return DecodeLatLon(LString, South)

   End Function

   Public Function DecodeLon(ByVal LString As String) As Double

      Dim East As Boolean = False
      Dim West As Boolean = False
      Dim i As Integer

      If IsNumeric(LString) Then Return CDbl(LString)

      LString = UCase(LString)

      i = InStr(LString, "E")

      If i <> 0 Then
         East = True
         LString = Microsoft.VisualBasic.Left(LString, i - 1)
      End If

      i = InStr(LString, "W")

      If i <> 0 Then
         West = True
         LString = Microsoft.VisualBasic.Left(LString, i - 1)
      End If

      Return DecodeLatLon(LString, West)

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

      If Degpos = 0 And Minpos = 0 And Secpos = 0 Then

         ' A GPS style position 4903.50N 07201.75W 

         If LatString(4) = "." Then

            ' Lat 

            LatString = "0" & LatString

         End If

         Degs = CInt(Microsoft.VisualBasic.Left(LatString, 3))
         Val = (CDbl(Microsoft.VisualBasic.Mid(LatString, 4))) / 60 + Degs

         If SW Then Val = -Val

         Return Val


      End If

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

   Public Sub SaveNodesFile()

      Dim i As Integer

      My.Computer.FileSystem.DeleteFile(My.Settings.FileName, FileIO.UIOption.OnlyErrorDialogs, _
            FileIO.RecycleOption.SendToRecycleBin, FileIO.UICancelOption.DoNothing)

      Using sw As StreamWriter = New StreamWriter(My.Settings.FileName)

         For i = 0 To ChatNodeIndex - 1

            sw.WriteLine(ChatNodes(i).Callsign & "," & Nodes(i).Locator & "," & _
                  Nodes(i).Lat & "," & Nodes(i).Lon & "," & Nodes(i).downIcon & "," & _
                  Nodes(i).upIcon & "," & ChatNodes(i).PopupMode & "," & ChatNodes(i).Comment)

         Next

         For i = 0 To CallsignData.Length - 1

            With CallsignData(i)

               sw.WriteLine("CALL," & .Callsign & "," & .Locator & "," & .Lat & "," & .Lon & "," & .LocType)

            End With

         Next

         For i = 0 To NodeIndex - 1

            With Nodes(i)

               sw.WriteLine("NODE," & .Callsign & "," & .Version & "," & .PopupMode)

               Dim HeardCount As Integer = .HeardNodes.Length
               Dim HeardIndex As Integer

               For HeardIndex = 0 To HeardCount - 1

                  With .HeardNodes(HeardIndex)

                     Dim j As Integer

                     sw.Write("MH," & Nodes(i).Callsign & "," & .Callsign & "," & .HeardItems.Length & ",")

                     For j = 0 To .HeardItems.Length - 1
                        With .HeardItems(j)
                           sw.Write(.Freq & "," & .Time & "," & .Flags & ",")
                        End With
                     Next

                     sw.Write(vbCrLf)

                  End With

               Next
            End With
         Next

         sw.Close()

      End Using

   End Sub


End Module
