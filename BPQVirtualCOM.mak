# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

!IF "$(CFG)" == ""
CFG=BPQVirtualCOM - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to BPQVirtualCOM - Win32\
 Debug.
!ENDIF 

!IF "$(CFG)" != "BPQVirtualCOM - Win32 Release" && "$(CFG)" !=\
 "BPQVirtualCOM - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "BPQVirtualCOM.mak" CFG="BPQVirtualCOM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BPQVirtualCOM - Win32 Release" (based on\
 "Win32 (x86) External Target")
!MESSAGE "BPQVirtualCOM - Win32 Debug" (based on "Win32 (x86) External Target")
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
# PROP Target_Last_Scanned "BPQVirtualCOM - Win32 Debug"

!IF  "$(CFG)" == "BPQVirtualCOM - Win32 Release"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f BPQVirtualCOM.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "BPQVirtualCOM.exe"
# PROP BASE Bsc_Name "BPQVirtualCOM.bsc"
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# PROP Cmd_Line "build -cZ"
# PROP Rebuild_Opt ""
# PROP Target_File "i386\BPQVirtualCOM.sys"
# PROP Bsc_Name "BPQVirtualCOM.bsc"
OUTDIR=.\Release
INTDIR=.\Release

ALL : 

CLEAN : 
	-@erase 

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

!ELSEIF  "$(CFG)" == "BPQVirtualCOM - Win32 Debug"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP BASE Cmd_Line "NMAKE /f BPQVirtualCOM.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "BPQVirtualCOM.exe"
# PROP BASE Bsc_Name "BPQVirtualCOM.bsc"
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# PROP Cmd_Line "build -cZ"
# PROP Rebuild_Opt ""
# PROP Target_File "i386\BPQVirtualCOM.sys"
# PROP Bsc_Name "BPQVirtualCOM.bsc"
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

# Name "BPQVirtualCOM - Win32 Release"
# Name "BPQVirtualCOM - Win32 Debug"

!IF  "$(CFG)" == "BPQVirtualCOM - Win32 Release"

"$(OUTDIR)\BPQVirtualCOM.sys" : 
   CD C:\msdev\Projects\BPQVirtualCOM
   build -cZ

!ELSEIF  "$(CFG)" == "BPQVirtualCOM - Win32 Debug"

"$(OUTDIR)\BPQVirtualCOM.sys" : 
   CD C:\msdev\Projects\BPQVirtualCOM
   build -cZ

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\BPQVirtualCOM.c

!IF  "$(CFG)" == "BPQVirtualCOM - Win32 Release"

!ELSEIF  "$(CFG)" == "BPQVirtualCOM - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BPQVCOMmsgs.mc

!IF  "$(CFG)" == "BPQVirtualCOM - Win32 Release"

!ELSEIF  "$(CFG)" == "BPQVirtualCOM - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\bpqvirtualcom.h

!IF  "$(CFG)" == "BPQVirtualCOM - Win32 Release"

!ELSEIF  "$(CFG)" == "BPQVirtualCOM - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\sources

!IF  "$(CFG)" == "BPQVirtualCOM - Win32 Release"

!ELSEIF  "$(CFG)" == "BPQVirtualCOM - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
