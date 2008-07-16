# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

!IF "$(CFG)" == ""
CFG=yyy - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to yyy - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "yyy - Win32 Release" && "$(CFG)" != "yyy - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "yyy.mak" CFG="yyy - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "yyy - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "yyy - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "yyy - Win32 Debug"

!IF  "$(CFG)" == "yyy - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f yyy.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "yyy.exe"
# PROP BASE Bsc_Name "yyy.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# PROP Cmd_Line "build -cZ"
# PROP Rebuild_Opt "/a"
# PROP Target_File "i386\bpqvirtualcom.sys"
# PROP Bsc_Name "yyy.bsc"
OUTDIR=.\Release
INTDIR=.\Release

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "yyy - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f yyy.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "yyy.exe"
# PROP BASE Bsc_Name "yyy.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# PROP Cmd_Line "build -cZ"
# PROP Rebuild_Opt "/a"
# PROP Target_File "i386\bpqvirtualcom.sys"
# PROP Bsc_Name "yyy.bsc"
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ENDIF 

################################################################################
# Begin Target

# Name "yyy - Win32 Release"
# Name "yyy - Win32 Debug"

!IF  "$(CFG)" == "yyy - Win32 Release"

"$(OUTDIR)\bpqvirtualcom.sys" : 
   CD C:\msdev\Projects\BPQVirtualCOM
   build -cZ

!ELSEIF  "$(CFG)" == "yyy - Win32 Debug"

"$(OUTDIR)\bpqvirtualcom.sys" : 
   CD C:\msdev\Projects\BPQVirtualCOM
   build -cZ

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\BPQVirtualCOM.c

!IF  "$(CFG)" == "yyy - Win32 Release"

!ELSEIF  "$(CFG)" == "yyy - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
