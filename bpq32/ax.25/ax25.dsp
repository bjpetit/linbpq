# Microsoft Developer Studio Project File - Name="ax25" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ax25 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ax25.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ax25.mak" CFG="ax25 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ax25 - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ax25 - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ax25 - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ax25.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ax25.exe"
# PROP BASE Bsc_Name "ax25.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "NMAKE /f ax25.mak"
# PROP Rebuild_Opt "/a"
# PROP Target_File "ax25.exe"
# PROP Bsc_Name "ax25.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ax25 - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f ax25.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ax25.exe"
# PROP BASE Bsc_Name "ax25.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "NMAKE /f makefile.nmake"
# PROP Rebuild_Opt "clean"
# PROP Target_File "ax25.exe"
# PROP Bsc_Name "ax25.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ax25 - Win32 Release"
# Name "ax25 - Win32 Debug"

!IF  "$(CFG)" == "ax25 - Win32 Release"

!ELSEIF  "$(CFG)" == "ax25 - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=.\asmbits.c
# End Source File
# Begin Source File

SOURCE=.\ax.25.c
# End Source File
# End Target
# End Project
