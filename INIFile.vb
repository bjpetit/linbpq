#Region "Imports"
Imports System.IO
Imports System.Text
#End Region ' Imports

Public Class INIFile ' A class for reading and writing to an ini file
#Region "Declarations"
    Dim strFilePath As String
    Dim dicSections As New Dictionary(Of String, Dictionary(Of String, String))
#End Region ' Declarations

#Region "Public Subs and Functions"

    ' Subroutine to DeleteKey from the Dictionary
    Public Sub DeleteKey(ByVal strSection As String, ByVal strKey As String)
        If dicSections.ContainsKey(strSection) = False Then Return
        If dicSections(strSection).ContainsKey(strKey) = False Then Return
        Dim dicRecord As Dictionary(Of String, String) = dicSections(strSection)
        dicRecord.Remove(strKey)
        Flush()
    End Sub 'DeleteKey

    'Subroutine to Load the INI file
    Public Sub Load()
        If My.Computer.FileSystem.FileExists(strFilePath) Then
            Dim strContent As String = My.Computer.FileSystem.ReadAllText(strFilePath)
            Dim strCurrentSection As String = ""
            Dim objStringReader As New StringReader(strContent)

            dicSections.Clear()
            Do
                Dim strLine As String = "EOF"
                Try
                    strLine = objStringReader.ReadLine.Trim
                Catch
                    Exit Do
                End Try
                If strLine Is Nothing OrElse strLine = "EOF" Then Exit Do

                If strLine.StartsWith("[") And strLine.EndsWith("]") Then
                    strLine = strLine.Replace("[", "").Replace("]", "")

                    dicSections.Add(strLine, New Dictionary(Of String, String))
                    strCurrentSection = strLine
                Else
                    Dim strTokens() As String = strLine.Split("="c)
                    If strTokens.Length = 2 Then
                        strTokens(0) = strTokens(0).Trim
                        strTokens(1) = strTokens(1).Trim
                        If strTokens(0) <> "" Then
                            dicSections(strCurrentSection).Add(strTokens(0).Trim, strTokens(1).Trim)
                        End If
                    End If
                End If
            Loop
        End If
    End Sub 'Load

    ' Subroutine to force the File system to finish writing changes to the Disc
    Public Sub Flush()
        SyncLock objIniFileLock
            Dim sbdContent As New StringBuilder
            For Each strSection As String In dicSections.Keys
                sbdContent.AppendLine(vbCrLf & "[" & strSection & "]")
                For Each strKey As String In dicSections(strSection).Keys
                    Dim strValue As String = dicSections(strSection)(strKey)
                    sbdContent.AppendLine(strKey & "=" & strValue)
                Next
            Next
            My.Computer.FileSystem.WriteAllText(strFilePath, sbdContent.ToString, False)
        End SyncLock
    End Sub 'Flush

    ' Function to Get a boolean value from the ini file section and key
    Public Function GetBoolean(ByVal strSection As String, ByVal strKey As String, ByVal blnDefault As Boolean) As Boolean
        Dim blnResult As Boolean
        SyncLock objIniFileLock
            Try
                blnResult = CBool(GetRecord(strSection, strKey, blnDefault.ToString))
            Catch
                blnResult = blnDefault
            End Try
        End SyncLock
        Return blnResult
    End Function 'GetBoolean

    ' Function to Get an integer value from the ini file section and key
    Public Function GetInteger(ByVal strSection As String, ByVal strKey As String, ByVal intDefault As Integer) As Integer
        Dim intResult As Integer
        SyncLock objIniFileLock
            Try
                intResult = CInt(GetRecord(strSection, strKey, intDefault.ToString))
            Catch
                intResult = intDefault
            End Try
        End SyncLock
        Return intResult
    End Function 'GetInteger

    ' Function to Get a string value from the ini file section and key
    Public Function GetString(ByVal strSection As String, ByVal strKey As String, ByVal strDefault As String) As String
        Dim strResult As String
        SyncLock objIniFileLock
            strResult = GetRecord(strSection, strKey, strDefault)
        End SyncLock
        Return strResult
    End Function ' GetString

    ' Subroutine to initialize the class
    Public Sub New(ByVal strFilePath As String)
        Me.strFilePath = strFilePath
        Load()
    End Sub ' New 

    'Subroutine to Write a boolean value to the ini file section and key
    Public Sub WriteBoolean(ByVal strSection As String, ByVal strKey As String, ByVal blnValue As Boolean)
        SyncLock objIniFileLock
            WriteRecord(strSection, strKey, blnValue.ToString)
        End SyncLock
    End Sub 'WriteBoolean

    'Subroutine to Write an integer value to the ini file section and key
    Public Sub WriteInteger(ByVal strSection As String, ByVal strKey As String, ByVal intValue As Integer)
        SyncLock objIniFileLock
            WriteRecord(strSection, strKey, intValue.ToString)
        End SyncLock
    End Sub ' WriteInteger 

    'Subroutine to Write a string value to the ini file section and key
    Public Sub WriteString(ByVal strSection As String, ByVal strKey As String, ByVal strValue As String)
        SyncLock objIniFileLock
            WriteRecord(strSection, strKey, strValue)
        End SyncLock
    End Sub ' WriteString

#End Region

#Region "Private Subs and Functions"

    ' Function to Get a Record from a Section and Key
    Private Function GetRecord(ByVal strSection As String, ByVal strKey As String, ByVal strDefault As String) As String
        Dim strValue As String
        Try
            strValue = dicSections(strSection)(strKey)
        Catch
            Return strDefault
        End Try
        If strValue = "" Then Return strDefault
        Return strValue
    End Function 'GetRecord

    ' Subroutine to Write a Record to a Section and Key
    Private Sub WriteRecord(ByVal strSection As String, ByVal strKey As String, ByVal strValue As String)
        If dicSections.ContainsKey(strSection) = False Then
            dicSections.Add(strSection, New Dictionary(Of String, String))
        End If

        If dicSections(strSection).ContainsKey(strKey) = False Then
            dicSections(strSection).Add(strKey, strValue)
        Else
            Dim dicRecord As Dictionary(Of String, String) = dicSections(strSection)
            dicRecord.Remove(strKey)
            dicRecord.Add(strKey, strValue)
        End If
    End Sub ' WriteRecord
#End Region 'Private Subs and Functions
End Class
