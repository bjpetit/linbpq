Public Class Form1
    Dim times(0) As Integer
    Dim lats(0) As Integer
    Dim lons(0) As Integer
    Dim recno As Integer

    Dim ge
    Dim cam
    Dim cam2

    Dim DateOffset As Date
    Dim SelectedDay As Integer
    Dim FirstRec As Integer
    Dim LastRec As Integer

    '    Dim tim As Long, Lat As Double, Lon As Double
    Dim Datx As Date




   Private Sub ProcMyToday_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ProcMyToday.Click

      Dim Datx As DateTime
      Dim tim As Integer, lasttim As Integer = 0, x As Double
      Dim N As Integer, i As Integer, j As Integer, k As Integer, skipped As Integer
      Dim rec(12) As Byte
      Dim Lat As Double, Lon As Double
      Dim bytes() As Byte
      Dim ptr As Integer
      Dim wYear As Integer
      Dim wMonth As Integer
      Dim wDay As Integer
      Dim wHour As Integer
      Dim HH As Integer
      Dim MM As Integer
      Dim SS As Integer

      x = 2 ^ 32

      bytes = My.Computer.FileSystem.ReadAllBytes("C:/GPSTrack.BIN")

      '        FileOpen(2, "c:\Program Files\PicGPS\temp.txt", OpenMode.Output)

      N = bytes.Length / 12

      ptr = 0


      Debug.Print("Processing " & N & " Records")

      Dim newtimes(N) As Integer
      Dim newlats(N) As Integer
      Dim newlons(N) As Integer

      ptr = 0

      k = 1

      For i = 1 To N

         If bytes(ptr) = 255 And bytes(ptr + 1) = 255 Then

            '   Datestamp record

            wYear = bytes(ptr + 3)
            wYear = wYear * 256 + bytes(ptr + 2)

            wMonth = bytes(ptr + 5)
            wMonth = wMonth * 256 + bytes(ptr + 4)

            wDay = bytes(ptr + 9)
            wDay = wDay * 256 + bytes(ptr + 8)

            wHour = bytes(ptr + 11)
            wHour = wHour * 256 + bytes(ptr + 10)

         Else

            ' GPS Record. Time is seconds into current day

            tim = bytes(ptr + 3)
            tim = tim * 256 + bytes(ptr + 2)
            tim = tim * 256 + bytes(ptr + 1)
            tim = tim * 256 + bytes(ptr + 0)

            HH = Int(tim / 3600)
            MM = Int((tim - (3600 * HH)) / 60)
            SS = tim - (3600 * HH) - (60 * MM)

            If wYear > 1980 And wYear < 2050 Then


               Datx = New System.DateTime(wYear, wMonth, wDay, HH, MM, SS)

               Lat = bytes(ptr + 7)
               Lat = Lat * 256 + bytes(ptr + 6)
               Lat = Lat * 256 + bytes(ptr + 5)
               Lat = Lat * 256 + bytes(ptr + 4)

               Lon = bytes(ptr + 11)
               Lon = Lon * 256 + bytes(ptr + 10)
               Lon = Lon * 256 + bytes(ptr + 9)
               Lon = Lon * 256 + bytes(ptr + 8)


               If Lon > 4000000000.0# Then Lon = -(x - Lon)

               tim = (Datx.ToOADate - DateOffset.ToOADate) * 86400

               If tim <> lasttim Then

                  newtimes(k) = tim
                  newlats(k) = Lat
                  newlons(k) = Lon

                  k = k + 1

               End If

               lasttim = tim


            End If

         End If
         ptr = ptr + 12


      Next i

      Debug.Print("Skipped " & N - k & " dups")

      N = k - 1

      ' Sort New Records (shouldn'r really be necessary, but just in case

      Dim keys(N) As Integer
      Dim Pointers(N) As Integer
      Dim Sortedtimes(N) As Integer
      Dim Sortedlats(N) As Integer
      Dim Sortedlons(N) As Integer

      keys(0) = -2000000000

      For i = 1 To N
         keys(i) = newtimes(i)
         Pointers(i) = i
      Next

      Array.Sort(keys, Pointers)

      For i = 1 To N

         Sortedtimes(i) = newtimes(Pointers(i))
         Sortedlats(i) = newlats(Pointers(i))
         Sortedlons(i) = newlons(Pointers(i))

      Next

      newtimes = Sortedtimes
      newlats = Sortedlats
      newlons = Sortedlons

      If recno = 0 Then

         ' First file loaded

         times = newtimes
         lats = newlats
         lons = newlons

         recno = N

         RefreshDisplay()

         Exit Sub

      End If

      ' Merge with existing records

      Dim mergedtimes(recno + N) As Integer
      Dim mergedlats(recno + N) As Integer
      Dim mergedlons(recno + N) As Integer

      i = 1
      j = 1
      k = 1
      skipped = 0

      Do Until i > recno And j > N

         If i > recno Then

            'Copy rest of new

            Do
               mergedtimes(k) = newtimes(j)
               mergedlats(k) = newlats(j)
               mergedlons(k) = newlons(j)
               j = j + 1
               k = k + 1

            Loop Until j > N

         ElseIf j > N Then

            'Copy rest of old

            Do
               mergedtimes(k) = times(i)
               mergedlats(k) = lats(i)
               mergedlons(k) = lons(i)
               i = i + 1
               k = k + 1

            Loop Until i > recno

         Else

            If times(i) < newtimes(j) Then

               mergedtimes(k) = times(i)
               mergedlats(k) = lats(i)
               mergedlons(k) = lons(i)
               i = i + 1
               k = k + 1

            ElseIf times(i) = newtimes(j) Then

               mergedtimes(k) = times(i)
               mergedlats(k) = lats(i)
               mergedlons(k) = lons(i)
               i = i + 1
               j = j + 1
               k = k + 1
               skipped = skipped + 1

            ElseIf times(i) > newtimes(j) Then

               mergedtimes(k) = newtimes(j)
               mergedlats(k) = newlats(j)
               mergedlons(k) = newlons(j)
               j = j + 1
               k = k + 1

            End If

         End If

      Loop

      recno = k - 1

      times = mergedtimes
      lats = mergedlats
      lons = mergedlons

      Debug.Print("Skipped " & skipped & " Records")

      RefreshDisplay()

   End Sub

    Private Sub SaveBinary_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles SaveBinary.Click


        FileOpen(1, "c:\Program Files\PicGPS\saveposns.bin", OpenMode.Binary)

        ReDim Preserve times(recno)
        ReDim Preserve lats(recno)
        ReDim Preserve lons(recno)

        FilePut(1, times, , True)
        FilePut(1, lats, , True)
        FilePut(1, lons, , True)
        FileClose(1)

    End Sub

    Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load

        DateOffset = CDate("january 1, 1970")

     End Sub


 
    Private Sub ReadBinary_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ReadBinary.Click

   
        FileOpen(1, "c:\Program Files\PicGPS\saveposns.bin", OpenMode.Binary)

        FileGet(1, times, , True)
        FileGet(1, lats, , True)
        FileGet(1, lons, , True)
        FileClose(1)


        On Error GoTo 0
        recno = times.Length - 1

        Debug.Print("Read " & recno & " Records from binary")



        RefreshDisplay()


    End Sub

    Sub RefreshDisplay()

        Dim Datx As Date

        RecsBox.Text = recno

        If recno > 0 Then

            Datx = DateTime.FromOADate(times(1) / 86400 + DateOffset.ToOADate)
            FirstBox.Text = Datx

            Datx = DateTime.FromOADate(times(recno) / 86400 + DateOffset.ToOADate)
            LastBox.Text = Datx

            If EnableGE.Checked Then cam2 = ge.SetCameraparams(lats(recno) / 600000, lons(recno) / 600000, 0, 1, 1500, 0, 0, 3)

            setupcalendar()

            '           lasttim = times(recno)

        End If

    End Sub



    Private Sub ReadBPQAISLog_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ReadBPQAISLog.Click
        Dim Bytes() As Byte
        Dim tim As Integer, x As Double
        Dim N As Integer, i As Integer, j As Integer, k As Integer, skipped As Integer
        Dim Lat As Integer, Lon As Double
        Dim ptr As Integer
        x = 2 ^ 32
        Dim lasttim As Integer

        Bytes = My.Computer.FileSystem.ReadAllBytes("c:\BPQAISTrack.bin")

        N = Bytes.Length / 16

        Debug.Print("Processing " & N & " Records")

        Dim newtimes(N) As Integer
        Dim newlats(N) As Integer
        Dim newlons(N) As Integer

        ptr = 0

        k = 1

        For i = 1 To N

            tim = Bytes(ptr + 3)
            tim = tim * 256 + Bytes(ptr + 2)
            tim = tim * 256 + Bytes(ptr + 1)
            tim = tim * 256 + Bytes(ptr + 0)

            Lat = Bytes(ptr + 7)
            Lat = Lat * 256 + Bytes(ptr + 6)
            Lat = Lat * 256 + Bytes(ptr + 5)
            Lat = Lat * 256 + Bytes(ptr + 4)

            Lon = Bytes(ptr + 11)
            Lon = Lon * 256 + Bytes(ptr + 10)
            Lon = Lon * 256 + Bytes(ptr + 9)
            Lon = Lon * 256 + Bytes(ptr + 8)

            If Lon > 4000000000.0# Then Lon = -(x - Lon)

            If tim <> lasttim Then


                newtimes(k) = tim
                newlats(k) = Lat
                newlons(k) = Lon

                k = k + 1

            End If

            lasttim = tim

            ptr = ptr + 16

        Next i

        Debug.Print("Skipped " & N - k & " dups")

        N = k - 1

        ' Sort New Records (shouldn'r really be necessary, but just in case

        Dim keys(N) As Integer
        Dim Pointers(N) As Integer
        Dim Sortedtimes(N) As Integer
        Dim Sortedlats(N) As Integer
        Dim Sortedlons(N) As Integer

        keys(0) = -2000000000

        For i = 1 To N
            keys(i) = newtimes(i)
            Pointers(i) = i
        Next

        Array.Sort(keys, Pointers)

        For i = 1 To N

            Sortedtimes(i) = newtimes(Pointers(i))
            Sortedlats(i) = newlats(Pointers(i))
            Sortedlons(i) = newlons(Pointers(i))

        Next

        newtimes = Sortedtimes
        newlats = Sortedlats
        newlons = Sortedlons

        If recno = 0 Then

            ' First file loaded

            times = newtimes
            lats = newlats
            lons = newlons

            recno = N

            RefreshDisplay()

            Exit Sub

        End If

        ' Merge with existing records

        Dim mergedtimes(recno + N) As Integer
        Dim mergedlats(recno + N) As Integer
        Dim mergedlons(recno + N) As Integer

        i = 1
        j = 1
        k = 1
        skipped = 0

        Do Until i > recno And j > N

            If i > recno Then

                'Copy rest of new

                Do
                    mergedtimes(k) = newtimes(j)
                    mergedlats(k) = newlats(j)
                    mergedlons(k) = newlons(j)
                    j = j + 1
                    k = k + 1

                Loop Until j > N

            ElseIf j > N Then

                'Copy rest of old

                Do
                    mergedtimes(k) = times(i)
                    mergedlats(k) = lats(i)
                    mergedlons(k) = lons(i)
                    i = i + 1
                    k = k + 1

                Loop Until i > recno

            Else




                If times(i) < newtimes(j) Then

                    mergedtimes(k) = times(i)
                    mergedlats(k) = lats(i)
                    mergedlons(k) = lons(i)
                    i = i + 1
                    k = k + 1

                ElseIf times(i) = newtimes(j) Then

                    mergedtimes(k) = times(i)
                    mergedlats(k) = lats(i)
                    mergedlons(k) = lons(i)
                    i = i + 1
                    j = j + 1
                    k = k + 1
                    skipped = skipped + 1

                ElseIf times(i) > newtimes(j) Then

                    mergedtimes(k) = newtimes(j)
                    mergedlats(k) = newlats(j)
                    mergedlons(k) = newlons(j)
                    j = j + 1
                    k = k + 1

                End If

            End If


        Loop


        recno = k - 1

        times = mergedtimes
        lats = mergedlats
        lons = mergedlons

        Debug.Print("Skipped " & skipped & " Records")

        RefreshDisplay()



    End Sub





   Private Sub CheckOrder_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CheckOrder.Click
      Dim i As Integer
      Dim Dups As Integer

      Dups = 0

      For i = 1 To recno - 1
         If times(i) > times(i + 1) Then
            MsgBox("Record(s) out of order")
         End If
         If times(i) = times(i + 1) Then
            Dups = Dups + 1
         End If
      Next

      If Dups > 0 Then
         MsgBox(Dups.ToString & " Duplicate Record(s)")
      End If
   End Sub


    Private Sub enablege_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles EnableGE.CheckedChanged

        If EnableGE.Checked Then
            ge = New EARTHLib.ApplicationGE
        Else
            Try
                ge.dispose()
            Catch ex As Exception

            End Try

        End If
    End Sub


    Private Sub setupcalendar()
        Dim i As Integer, day As Integer, lastday As Integer

        For i = 1 To recno
            day = Int(times(i) / 86400)
            If day > lastday Then
                Datx = DateTime.FromOADate(day + DateOffset.ToOADate)
                MonthCalendar1.AddBoldedDate(Datx)
                lastday = day
            End If

        Next
    End Sub





    Private Sub MonthCalendar1_DateSelected1(ByVal sender As Object, ByVal e As System.Windows.Forms.DateRangeEventArgs) Handles MonthCalendar1.DateSelected
        Dim tim As Integer, i As Integer

        Debug.Print(e.Start)
        tim = (e.Start.ToOADate - DateOffset.ToOADate) * 86400
        Debug.Print(tim)

        FirstRec = 0
        LastRec = recno


        For i = 1 To recno
            If tim < times(i) Then
                If FirstRec = 0 Then
                    FirstRec = i
                    Debug.Print(i)
                End If
            End If
            If tim + 86400 < times(i) Then
                Debug.Print(i - 1)
                LastRec = i - 1
                Exit For
            End If
        Next

        Debug.Print(LastRec)
        For i = 1 To recno
            If tim < times(i) Then
                Try
                    If EnableGE.Checked Then cam2 = ge.SetCameraparams(lats(i) / 600000, lons(i) / 600000, 0, 1, 30000, 0, 0, 3)
                    Debug.Print(lats(i) / 600000)
                    Debug.Print(lons(i) / 600000)

                Catch
                End Try
                Exit Sub
            End If
        Next

    End Sub

    Private Sub SortButton_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles SortButton.Click
        Dim keys(recno) As Integer
        Dim Pointers(recno) As Integer
        Dim Sortedtimes(recno) As Integer
        Dim Sortedlats(recno) As Integer
        Dim Sortedlons(recno) As Integer

        Dim i As Integer

        keys(0) = -2000000000

        For i = 1 To recno
            keys(i) = times(i)
            Pointers(i) = i
        Next

        Array.Sort(keys, Pointers)

        For i = 1 To recno

            Sortedtimes(i) = times(Pointers(i))
            Sortedlats(i) = lats(Pointers(i))
            Sortedlons(i) = lons(Pointers(i))

        Next

        times = Sortedtimes
        lats = Sortedlats
        lons = Sortedlons

        RefreshDisplay()


    End Sub



    Private Sub ReadMMCSV_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ReadMMCSV.Click
        Dim tim As Long, Lat As Double, Lon As Double
        Dim skipped As Integer
        Dim i As Integer
        Dim val As String
        Dim lasttim As Integer
        Dim fileReader As System.IO.StreamReader
        fileReader = My.Computer.FileSystem.OpenTextFileReader("c:\Program Files\PicGPS\tracks2008.csv")
        Dim line As String
 
        Do

            line = fileReader.ReadLine()
            If line Is Nothing Then Exit Do

            '  TP02, 55.641467, -4.819167, 0.0, 1207743623, 0.0, 0.0, 0

            If Microsoft.VisualBasic.Left(line, 4) = "TP02" Then

                i = InStr(Mid(line, 6), ",")

                val = Mid(line, 6, i - 1)

                Lat = val * 600000

                line = Mid(line, i + 6)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                Lon = val * 600000

                line = Mid(line, i + 1)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                line = Mid(line, i + 1)

                i = InStr(line, ",")

                val = Mid(line, 1, i - 1)

                tim = val

            End If


            If tim > lasttim Then

                recno = recno + 1

                If recno >= times.Length Then

                    ReDim Preserve times(recno + 1000)
                    ReDim Preserve lats(recno + 1000)
                    ReDim Preserve lons(recno + 1000)

                End If

                times(recno) = tim
                lats(recno) = Lat
                lons(recno) = Lon

            Else
                skipped = skipped + 1
            End If

            lasttim = tim

        Loop

        Debug.Print(recno & " " & skipped)

        RefreshDisplay()

        FileClose(1)

    End Sub

    Private Sub CreateV3TLOG_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CreateV3TLOG.Click

        Dim textfile As System.IO.StreamWriter
        Dim Line As String
        Dim j As Integer
        Dim Tab As Char = Chr(9)

        '        4002	1	20070619 Isle of Lewis 20070619	 	0	255	0	00:00:10	0.1	0.005	90

        textfile = My.Computer.FileSystem.OpenTextFileWriter("c:\TLOGV3.txt", False, System.Text.Encoding.ASCII)

        Line = "4002" & Tab & recno & Tab & "Track from MYTOday" & Tab & " " & Tab & "0" & Tab & "255" & Tab & "0" & Tab & "00:00:10" & Tab & "0.1" & Tab & "0.005" & Tab & LastRec - FirstRec
        textfile.WriteLine(Line)

        For j = FirstRec To LastRec
            '4001	0	1	1182247303	3478.614532	321.892103	TK0001	 	0	255	0	0.000000	0.000000


            Line = "4001" & Tab & "0" & Tab & j & Tab & times(j) & Tab & lats(j) / 10000 & Tab & -lons(j) / 10000 & Tab & "TK" & j & Tab & " " & Tab & "0" & Tab & "255" & Tab & "0" & Tab & "0" & Tab & "0"

            textfile.WriteLine(Line)

        Next


        textfile.Close()

    End Sub
End Class