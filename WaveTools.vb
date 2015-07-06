Imports System.Math
Imports System.Text
Imports System.IO
Imports System.BitConverter
Public Class WaveTools

    ' This class is used to Write or Read a 16  bit sample mono wave file in RIFF format

    '  Subroutine to Write a RIFF waveform file 
    Public Function WriteRIFF(ByVal strFilename As String, ByVal intSampleRate As Integer, ByVal intFmtChunkLength As Integer, ByRef aryWaveData() As Byte) As Boolean
        '*************************************************************************
        '	Here is where the file will be created. A
        '	wave file is a RIFF file, which has chunks
        '	of data that describe what the file contains.
        '	A wave RIFF file is put together like this:
        '	The 12 byte RIFF chunk is constructed like this:
        '     Bytes(0 - 3) 'R' 'I' 'F' 'F'
        '	   Bytes 4 - 7 :	Length of file, minus the first 8 bytes of the RIFF description.
        '					(4 bytes for "WAVE" + 24 bytes for format chunk length +
        '					8 bytes for data chunk description + actual sample data size.)
        '     Bytes(8 - 11) 'W' 'A' 'V' 'E'
        '	The 24 byte FORMAT chunk is constructed like this:
        '     Bytes(0 - 3) 'f' 'm' 't' ' '
        '	   Bytes 4 - 7 :	The format chunk length. This is 16 or 18
        '	   Bytes 8 - 9 :	File padding. Always 1.
        '	   Bytes 10- 11:	Number of channels. Either 1 for mono,  or 2 for stereo.
        '	   Bytes 12- 15:	Sample rate.
        '	   Bytes 16- 19:	Number of bytes per second.
        '	   Bytes 20- 21:	Bytes per sample. 1 for 8 bit mono, 2 for 8 bit stereo or
        '					16 bit mono, 4 for 16 bit stereo.
        '	   Bytes 22- 23:	Number of bits per sample.
        '	The DATA chunk is constructed like this:
        '     Bytes(0 - 3) 'd' 'a' 't' 'a'
        '	   Bytes 4 - 7 :	Length of data, in bytes.
        '	   Bytes 8 -...:	Actual sample data.
        '**************************************************************************
        Dim Writer As BinaryWriter
        Dim WaveFile As FileStream

        Try
            If IO.File.Exists(strFilename) Then IO.File.Delete(strFilename)
            WaveFile = New FileStream(strFilename, FileMode.Create)
            Writer = New BinaryWriter(WaveFile)

            ' Set up file with RIFF chunk info.
            Dim ChunkRiff As Char() = {CChar("R"), CChar("I"), CChar("F"), CChar("F")}
            Dim ChunkType As Char() = {CChar("W"), CChar("A"), CChar("V"), CChar("E")}
            Dim ChunkFmt As Char() = {CChar("f"), CChar("m"), CChar("t"), CChar(" ")}
            Dim ChunkData As Char() = {CChar("d"), CChar("a"), CChar("t"), CChar("a")}
            Dim shPad As Short = 1 ' File padding
            Dim intLength As Integer
            If intFmtChunkLength = 16 Then
                intLength = aryWaveData.Length + 36 ' File length, minus first 8 bytes of RIFF description.
            ElseIf intFmtChunkLength = 18 Then
                intLength = aryWaveData.Length + 38 ' File length, minus first 8 bytes of RIFF description.
            End If
            Dim shBytesPerSample As Short = 2 ' Bytes per sample.
            ' Fill in the riff info for the wave file.
            Writer.Write(ChunkRiff)
            Writer.Write(intLength)
            Writer.Write(ChunkType)
            ' Fill in the format info for the wave file.
            Writer.Write(ChunkFmt)
            Writer.Write(intFmtChunkLength)
            Writer.Write(shPad)
            Writer.Write(CShort(1)) ' mono
            Writer.Write(CInt(intSampleRate)) ' sample rate in samples per second
            Writer.Write(CInt(2 * intSampleRate)) ' bytes per second
            Writer.Write(shBytesPerSample)
            Writer.Write(CShort(16))
            If intFmtChunkLength = 18 Then
                Writer.Write(CShort(0))
            End If
            ' Now fill in the data chunk.
            Writer.Write(ChunkData)
            Writer.Write(CInt(aryWaveData.Length)) ' The length of a the following data file.
            Writer.Write(aryWaveData, 0, aryWaveData.Length)
            Writer.Flush()  ' Flush out any file buffers
            Writer.Close()  ' Close the file now.
            Return True
        Catch ex As Exception
            Logs.Exception("[WaveTools.WriteRIFF] Exception: " & ex.ToString)
            Return False
        End Try

    End Function 'WriteRIFF

    '  Subroutine to Write a floating point file in RIFF waveform file format for debugging
    Public Function WriteFloatingRIFF(ByVal strFilename As String, ByVal intSampleRate As Integer, ByVal intFmtChunkLength As Integer, ByRef aryFloatingData() As Double) As Boolean
        Dim intSample As Integer
        ' First find the max value of the floating to use as a nomalizer
        Dim dblDataMax As Double
        For j As Integer = 0 To aryFloatingData.Length - 1
            dblDataMax = Math.Max(dblDataMax, Math.Abs(aryFloatingData(j)))
        Next j
        Dim dblScale As Double = 32000 / dblDataMax ' will scale the value to 32000 max (16 bit)
        Dim aryWaveData(2 * aryFloatingData.Length - 1) As Byte
        For j As Integer = 0 To aryFloatingData.Length - 1
            intSample = CInt(dblScale * aryFloatingData(j))
            aryWaveData(2 * j) = CByte(intSample And &HFF) ' LSByte
            aryWaveData(2 * j + 1) = CByte((intSample And &HFF00) >> 8) ' MSbyte
        Next j
        '*************************************************************************
        '	Here is where the file will be created. A
        '	wave file is a RIFF file, which has chunks
        '	of data that describe what the file contains.
        '	A wave RIFF file is put together like this:
        '	The 12 byte RIFF chunk is constructed like this:
        '     Bytes(0 - 3) 'R' 'I' 'F' 'F'
        '	   Bytes 4 - 7 :	Length of file, minus the first 8 bytes of the RIFF description.
        '					(4 bytes for "WAVE" + 24 bytes for format chunk length +
        '					8 bytes for data chunk description + actual sample data size.)
        '     Bytes(8 - 11) 'W' 'A' 'V' 'E'
        '	The 24 byte FORMAT chunk is constructed like this:
        '     Bytes(0 - 3) 'f' 'm' 't' ' '
        '	   Bytes 4 - 7 :	The format chunk length. This is 16 or 18
        '	   Bytes 8 - 9 :	File padding. Always 1.
        '	   Bytes 10- 11:	Number of channels. Either 1 for mono,  or 2 for stereo.
        '	   Bytes 12- 15:	Sample rate.
        '	   Bytes 16- 19:	Number of bytes per second.
        '	   Bytes 20- 21:	Bytes per sample. 1 for 8 bit mono, 2 for 8 bit stereo or
        '					16 bit mono, 4 for 16 bit stereo.
        '	   Bytes 22- 23:	Number of bits per sample.
        '	The DATA chunk is constructed like this:
        '     Bytes(0 - 3) 'd' 'a' 't' 'a'
        '	   Bytes 4 - 7 :	Length of data, in bytes.
        '	   Bytes 8 -...:	Actual sample data.
        '**************************************************************************
        Dim Writer As BinaryWriter
        Dim WaveFile As FileStream

        Try
            If IO.File.Exists(strFilename) Then IO.File.Delete(strFilename)
            WaveFile = New FileStream(strFilename, FileMode.Create)
            Writer = New BinaryWriter(WaveFile)

            ' Set up file with RIFF chunk info.
            Dim ChunkRiff As Char() = {CChar("R"), CChar("I"), CChar("F"), CChar("F")}
            Dim ChunkType As Char() = {CChar("W"), CChar("A"), CChar("V"), CChar("E")}
            Dim ChunkFmt As Char() = {CChar("f"), CChar("m"), CChar("t"), CChar(" ")}
            Dim ChunkData As Char() = {CChar("d"), CChar("a"), CChar("t"), CChar("a")}
            Dim shPad As Short = 1 ' File padding
            Dim intLength As Integer
            If intFmtChunkLength = 16 Then
                intLength = aryWaveData.Length + 36 ' File length, minus first 8 bytes of RIFF description.
            ElseIf intFmtChunkLength = 18 Then
                intLength = aryWaveData.Length + 38 ' File length, minus first 8 bytes of RIFF description.
            End If
            Dim shBytesPerSample As Short = 2 ' Bytes per sample.
            ' Fill in the riff info for the wave file.
            Writer.Write(ChunkRiff)
            Writer.Write(intLength)
            Writer.Write(ChunkType)
            ' Fill in the format info for the wave file.
            Writer.Write(ChunkFmt)
            Writer.Write(intFmtChunkLength)
            Writer.Write(shPad)
            Writer.Write(CShort(1)) ' mono
            Writer.Write(CInt(intSampleRate)) ' sample rate in samples per second
            Writer.Write(CInt(2 * intSampleRate)) ' bytes per second
            Writer.Write(shBytesPerSample)
            Writer.Write(CShort(16))
            If intFmtChunkLength = 18 Then
                Writer.Write(CShort(0))
            End If
            ' Now fill in the data chunk.
            Writer.Write(ChunkData)
            Writer.Write(CInt(aryWaveData.Length)) ' The length of a the following data file.
            Writer.Write(aryWaveData, 0, aryWaveData.Length)
            Writer.Flush()  ' Flush out any file buffers
            Writer.Close()  ' Close the file now.
            Return True
        Catch ex As Exception
            Logs.Exception("[WaveTools.WriteRIFF] Exception: " & ex.ToString)
            Return False
        End Try
    End Function 'WriteFloatingRIFF

    '  Subroutine to Write a RIFF waveform to a memory Stream (no disc access) 
    Public Function WriteRIFFStream(ByRef WaveStream As MemoryStream, ByVal intSampleRate As Integer, ByVal intFmtChunkLength As Integer, ByRef aryWaveData() As Byte) As Boolean
        '*************************************************************************
        '	Here is where the file will be created. A
        '	wave file is a RIFF file, which has chunks
        '	of data that describe what the file contains.
        '	A wave RIFF file is put together like this:
        '	The 12 byte RIFF chunk is constructed like this:
        '     Bytes(0 - 3) 'R' 'I' 'F' 'F'
        '	   Bytes 4 - 7 :	Length of file, minus the first 8 bytes of the RIFF description.
        '					(4 bytes for "WAVE" + 24 bytes for format chunk length +
        '					8 bytes for data chunk description + actual sample data size.)
        '     Bytes(8 - 11) 'W' 'A' 'V' 'E'
        '	The 24 byte FORMAT chunk is constructed like this:
        '     Bytes(0 - 3) 'f' 'm' 't' ' '
        '	   Bytes 4 - 7 :	The format chunk length. This is 16 or 18
        '	   Bytes 8 - 9 :	File padding. Always 1.
        '	   Bytes 10- 11:	Number of channels. Either 1 for mono,  or 2 for stereo.
        '	   Bytes 12- 15:	Sample rate.
        '	   Bytes 16- 19:	Number of bytes per second.
        '	   Bytes 20- 21:	Bytes per sample. 1 for 8 bit mono, 2 for 8 bit stereo or
        '					16 bit mono, 4 for 16 bit stereo.
        '	   Bytes 22- 23:	Number of bits per sample.
        '	The DATA chunk is constructed like this:
        '     Bytes(0 - 3) 'd' 'a' 't' 'a'
        '	   Bytes 4 - 7 :	Length of data, in bytes.
        '	   Bytes 8 -...:	Actual sample data.
        '**************************************************************************
        'TODO: Test of "back porch" extension for PTT
        'ReDim Preserve aryWaveData(aryWaveData.Length + 1023) ' Adds 42.6 ms additional dead time before removal of PTT
        Dim bytTemp(-1) As Byte
        If intFmtChunkLength = 16 Then
            If Not IsNothing(WaveStream) Then WaveStream.Flush()
            'WaveStream = New MemoryStream(43 + aryWaveData.Length)
            WaveStream = New MemoryStream()


        ElseIf intFmtChunkLength = 18 Then
            If Not IsNothing(WaveStream) Then WaveStream.Flush()
            ' WaveStream = New MemoryStream(45 + aryWaveData.Length)
            WaveStream = New MemoryStream()
        Else
            Return False
        End If

        ' Set up file with RIFF chunk info.
        Dim shPad As Short = 1 ' File padding
        Dim intLength As Integer
        If intFmtChunkLength = 16 Then
            intLength = aryWaveData.Length + 36 ' File length, minus first 8 bytes of RIFF description.
        ElseIf intFmtChunkLength = 18 Then
            intLength = aryWaveData.Length + 38 ' File length, minus first 8 bytes of RIFF description.
        End If
        Dim shBytesPerSample As Short = 2 ' Bytes per sample.

        ' Fill in the riff info for the wave file.
        AppendBytes(bytTemp, GetBytes("RIFF"))
        AppendBytes(bytTemp, Int32ToBytes(intLength))
        AppendBytes(bytTemp, GetBytes("WAVE"))
        AppendBytes(bytTemp, GetBytes("fmt "))
        AppendBytes(bytTemp, Int32ToBytes(intFmtChunkLength))
        AppendBytes(bytTemp, Int16ToBytes(shPad))
        AppendBytes(bytTemp, Int16ToBytes(1)) ' mono
        AppendBytes(bytTemp, Int32ToBytes(intSampleRate))
        AppendBytes(bytTemp, Int32ToBytes(2 * intSampleRate))
        AppendBytes(bytTemp, Int16ToBytes(2)) ' bytes/sample
        If intFmtChunkLength = 18 Then
            AppendBytes(bytTemp, Int16ToBytes(2)) ' bytes/sample
        End If
        AppendBytes(bytTemp, Int16ToBytes(16)) ' bits/sample
        ' Now fill in the data chunk.
        AppendBytes(bytTemp, GetBytes("data"))
        AppendBytes(bytTemp, Int32ToBytes(aryWaveData.Length))
        AppendBytes(bytTemp, aryWaveData)
        WaveStream.Write(bytTemp, 0, bytTemp.Length)
        Return True
    End Function 'WriteRIFFStream


    Protected Overrides Sub Finalize()
        MyBase.Finalize()
    End Sub

    ' Function to read a 16 bit/sample mono wave file and return intSampleRate and the sampled data in bytWaveData()
    Public Function ReadRIFF(ByVal strFilename As String, ByRef intSampleRate As Integer, ByRef bytWaveData() As Byte) As Boolean

        ' returns true if successful, false if not. intSampleRate and bytWaveData updated by reference
        If Not IO.File.Exists(strFilename) Then Return False
        Try
            Dim fs As New FileStream(strFilename, FileMode.Open)
            Dim bytHeader(45) As Byte
            fs.Read(bytHeader, 0, 46)
            Dim intFmtChunkLength As Integer = System.BitConverter.ToInt32(bytHeader, 16)
            intSampleRate = System.BitConverter.ToInt32(bytHeader, 24)
            Dim intDataBytes As Integer
            Dim bytBuffer(-1) As Byte
            If intFmtChunkLength = 16 Then
                intDataBytes = System.BitConverter.ToInt32(bytHeader, 40)
                ReDim bytBuffer(intDataBytes + 43)
                fs.Read(bytBuffer, 0, intDataBytes + 44)
                ReDim bytWaveData(intDataBytes - 1)
                Array.Copy(bytBuffer, 44, bytWaveData, 0, intDataBytes)
            ElseIf intFmtChunkLength = 18 Then
                intDataBytes = System.BitConverter.ToInt32(bytHeader, 42)
                ReDim bytBuffer(intDataBytes + 45)
                fs.Read(bytBuffer, 0, intDataBytes + 46)
                ReDim bytWaveData(intDataBytes - 1)
                Array.Copy(bytBuffer, 46, bytWaveData, 0, intDataBytes)
            End If
            fs.Close()

        Catch
            Return False
        End Try
        Return True
    End Function   'ReadRIFF

    Private Function Int32ToBytes(ByVal int32 As Int32) As Byte()
        Dim bytTemp(3) As Byte
        '  LSByte first
        bytTemp(0) = CByte(int32 And &HFF)
        bytTemp(1) = CByte((int32 And &HFF00) \ CInt(2 ^ 8))
        bytTemp(2) = CByte((int32 And &HFF0000) \ CInt(2 ^ 16))
        bytTemp(3) = CByte((int32 And &HFF000000) \ CInt(2 ^ 24))
        Return bytTemp
    End Function

    Private Function Int16ToBytes(ByVal int16 As Int16) As Byte()
        Dim bytTemp(1) As Byte
        '  LS byte first 
        bytTemp(0) = CByte(int16 And &HFF)
        bytTemp(1) = CByte((int16 And &HFF00) \ 256)
        Return bytTemp
    End Function

    Private Sub AppendBytes(ByRef Buffer() As Byte, ByVal NewBytes() As Byte)
        If NewBytes.Length <> 0 Then
            ReDim Preserve Buffer(Buffer.Length + NewBytes.Length - 1)
            Array.Copy(NewBytes, 0, Buffer, Buffer.Length - NewBytes.Length, NewBytes.Length)
        End If
    End Sub

    Public Sub ComputePeakToRMS(intSamples() As Int32, ByRef intPeak As Int32, ByRef dblRMS As Double)
        intPeak = 0
        Dim dblSum As Double = 0
        For i As Integer = 0 To intSamples.Length - 1
            If Abs(intSamples(i)) > intPeak Then intPeak = Abs(intSamples(i))
            dblSum += CDbl(intSamples(i)) * CDbl(intSamples(i))
        Next i
        dblRMS = Sqrt(dblSum / intSamples.Length)
    End Sub
End Class
