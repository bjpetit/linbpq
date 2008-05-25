# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=BPQCtrl - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to BPQCtrl - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "BPQCtrl - Win32 Release" && "$(CFG)" !=\
 "BPQCtrl - Win32 Debug" && "$(CFG)" != "BPQCtrl - Win32 Unicode Debug" &&\
 "$(CFG)" != "BPQCtrl - Win32 Unicode Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "BPQCtrl.mak" CFG="BPQCtrl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BPQCtrl - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BPQCtrl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BPQCtrl - Win32 Unicode Debug" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BPQCtrl - Win32 Unicode Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
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
# PROP Target_Last_Scanned "BPQCtrl - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\BPQCtrl.ocx" ".\Release\BPQCtrl.bsc" ".\Release\regsvr32.trg"

CLEAN : 
	-@erase ".\Release\BPQCtrl.bsc"
	-@erase ".\Release\BPQCtrl.sbr"
	-@erase ".\Release\BPQCtrl.pch"
	-@erase ".\Release\BPQCtrlPpg.sbr"
	-@erase ".\Release\StdAfx.sbr"
	-@erase ".\Release\BPQCtrlCtl.sbr"
	-@erase ".\Release\BPQCtrl.lib"
	-@erase ".\Release\BPQCtrlCtl.obj"
	-@erase ".\Release\BPQCtrl.obj"
	-@erase ".\Release\BPQCtrlPpg.obj"
	-@erase ".\Release\StdAfx.obj"
	-@erase ".\Release\BPQCtrl.res"
	-@erase ".\Release\BPQCtrl.tlb"
	-@erase ".\AGWMon.obj"
	-@erase ".\Release\BPQCtrl.exp"
	-@erase ".\Release\BPQCtrl.map"
	-@erase ".\Release\regsvr32.trg"
	-@erase "..\..\..\..\bpq32\BPQCtrl.ocx"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/BPQCtrl.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\Release/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/BPQCtrl.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/BPQCtrl.sbr" \
	"$(INTDIR)/BPQCtrlPpg.sbr" \
	"$(INTDIR)/StdAfx.sbr" \
	"$(INTDIR)/BPQCtrlCtl.sbr"

".\Release\BPQCtrl.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\lib\bpq32.lib /nologo /subsystem:windows /dll /map /machine:I386 /out:"c:\bpq32\BPQCtrl.ocx"
# SUBTRACT LINK32 /incremental:yes
LINK32_FLAGS=..\lib\bpq32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/BPQCtrl.pdb" /map:"$(INTDIR)/BPQCtrl.map" /machine:I386\
 /def:".\BPQCtrl.def" /out:"c:\bpq32\BPQCtrl.ocx"\
 /implib:"$(OUTDIR)/BPQCtrl.lib" 
DEF_FILE= \
	".\BPQCtrl.def"
LINK32_OBJS= \
	"$(INTDIR)/BPQCtrlCtl.obj" \
	"$(INTDIR)/BPQCtrl.obj" \
	"$(INTDIR)/BPQCtrlPpg.obj" \
	"$(INTDIR)/StdAfx.obj" \
	".\AGWMon.obj" \
	"$(INTDIR)/BPQCtrl.res"

"$(OUTDIR)\BPQCtrl.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\Release
TargetPath=\bpq32\BPQCtrl.ocx
InputPath=\bpq32\BPQCtrl.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\BPQCtrl.ocx" "$(OUTDIR)\BPQCtrl.bsc" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\BPQCtrl.pch"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\BPQCtrl.bsc"
	-@erase ".\Debug\StdAfx.sbr"
	-@erase ".\Debug\BPQCtrlPpg.sbr"
	-@erase ".\Debug\BPQCtrlCtl.sbr"
	-@erase ".\Debug\BPQCtrl.sbr"
	-@erase ".\Debug\BPQCtrl.lib"
	-@erase ".\Debug\StdAfx.obj"
	-@erase ".\Debug\BPQCtrlPpg.obj"
	-@erase ".\Debug\BPQCtrlCtl.obj"
	-@erase ".\Debug\BPQCtrl.obj"
	-@erase ".\Debug\BPQCtrl.res"
	-@erase ".\Debug\BPQCtrl.tlb"
	-@erase ".\AGWMon.obj"
	-@erase ".\Debug\BPQCtrl.exp"
	-@erase ".\Debug\BPQCtrl.pdb"
	-@erase ".\Debug\BPQCtrl.map"
	-@erase ".\Debug\regsvr32.trg"
	-@erase ".\Debug\BPQCtrl.ocx"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/BPQCtrl.pch" /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/BPQCtrl.bsc" 
BSC32_SBRS= \
	"$(INTDIR)/StdAfx.sbr" \
	"$(INTDIR)/BPQCtrlPpg.sbr" \
	"$(INTDIR)/BPQCtrlCtl.sbr" \
	"$(INTDIR)/BPQCtrl.sbr"

"$(OUTDIR)\BPQCtrl.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ..\lib\bpq32.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386
LINK32_FLAGS=..\lib\bpq32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/BPQCtrl.pdb" /map:"$(INTDIR)/BPQCtrl.map" /debug /machine:I386\
 /def:".\BPQCtrl.def" /out:"$(OUTDIR)/BPQCtrl.ocx"\
 /implib:"$(OUTDIR)/BPQCtrl.lib" 
DEF_FILE= \
	".\BPQCtrl.def"
LINK32_OBJS= \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/BPQCtrlPpg.obj" \
	"$(INTDIR)/BPQCtrlCtl.obj" \
	"$(INTDIR)/BPQCtrl.obj" \
	".\AGWMon.obj" \
	"$(INTDIR)/BPQCtrl.res"

"$(OUTDIR)\BPQCtrl.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\Debug
TargetPath=.\Debug\BPQCtrl.ocx
InputPath=.\Debug\BPQCtrl.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\DebugU
INTDIR=.\DebugU
# Begin Custom Macros
OutDir=.\DebugU
# End Custom Macros

ALL : "$(OUTDIR)\BPQCtrl.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase ".\DebugU\vc40.pdb"
	-@erase ".\DebugU\BPQCtrl.pch"
	-@erase ".\DebugU\vc40.idb"
	-@erase ".\DebugU\BPQCtrl.lib"
	-@erase ".\DebugU\BPQCtrl.obj"
	-@erase ".\DebugU\BPQCtrlPpg.obj"
	-@erase ".\DebugU\BPQCtrlCtl.obj"
	-@erase ".\DebugU\StdAfx.obj"
	-@erase ".\DebugU\BPQCtrl.res"
	-@erase ".\DebugU\BPQCtrl.tlb"
	-@erase ".\DebugU\BPQCtrl.exp"
	-@erase ".\DebugU\BPQCtrl.pdb"
	-@erase ".\DebugU\BPQCtrl.map"
	-@erase ".\DebugU\regsvr32.trg"
	-@erase ".\DebugU\BPQCtrl.ocx"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/BPQCtrl.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\DebugU/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/BPQCtrl.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 ..\lib\bpq32.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386
LINK32_FLAGS=..\lib\bpq32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/BPQCtrl.pdb" /map:"$(INTDIR)/BPQCtrl.map" /debug /machine:I386\
 /def:".\BPQCtrl.def" /out:"$(OUTDIR)/BPQCtrl.ocx"\
 /implib:"$(OUTDIR)/BPQCtrl.lib" 
DEF_FILE= \
	".\BPQCtrl.def"
LINK32_OBJS= \
	"$(INTDIR)/BPQCtrl.obj" \
	"$(INTDIR)/BPQCtrlPpg.obj" \
	"$(INTDIR)/BPQCtrlCtl.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/BPQCtrl.res"

"$(OUTDIR)\BPQCtrl.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\DebugU
TargetPath=.\DebugU\BPQCtrl.ocx
InputPath=.\DebugU\BPQCtrl.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseU"
# PROP BASE Intermediate_Dir "ReleaseU"
# PROP BASE Target_Dir ""
# PROP BASE Target_Ext "ocx"
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
# PROP Target_Ext "ocx"
OUTDIR=.\ReleaseU
INTDIR=.\ReleaseU
# Begin Custom Macros
OutDir=.\ReleaseU
# End Custom Macros

ALL : "$(OUTDIR)\BPQCtrl.ocx" "$(OUTDIR)\regsvr32.trg"

CLEAN : 
	-@erase ".\ReleaseU\BPQCtrl.lib"
	-@erase ".\ReleaseU\BPQCtrl.obj"
	-@erase ".\ReleaseU\BPQCtrl.pch"
	-@erase ".\ReleaseU\BPQCtrlPpg.obj"
	-@erase ".\ReleaseU\StdAfx.obj"
	-@erase ".\ReleaseU\BPQCtrlCtl.obj"
	-@erase ".\ReleaseU\BPQCtrl.res"
	-@erase ".\ReleaseU\BPQCtrl.tlb"
	-@erase ".\ReleaseU\BPQCtrl.exp"
	-@erase ".\ReleaseU\BPQCtrl.map"
	-@erase ".\ReleaseU\regsvr32.trg"
	-@erase ".\ReleaseU\BPQCtrl.ocx"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/BPQCtrl.pch"\
 /Yu"stdafx.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\ReleaseU/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/BPQCtrl.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\lib\bpq32.lib /nologo /subsystem:windows /dll /map /machine:I386
# SUBTRACT LINK32 /incremental:yes
LINK32_FLAGS=..\lib\bpq32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/BPQCtrl.pdb" /map:"$(INTDIR)/BPQCtrl.map" /machine:I386\
 /def:".\BPQCtrl.def" /out:"$(OUTDIR)/BPQCtrl.ocx"\
 /implib:"$(OUTDIR)/BPQCtrl.lib" 
DEF_FILE= \
	".\BPQCtrl.def"
LINK32_OBJS= \
	"$(INTDIR)/BPQCtrl.obj" \
	"$(INTDIR)/BPQCtrlPpg.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/BPQCtrlCtl.obj" \
	"$(INTDIR)/BPQCtrl.res"

"$(OUTDIR)\BPQCtrl.ocx" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

# Begin Custom Build - Registering OLE control...
OutDir=.\ReleaseU
TargetPath=.\ReleaseU\BPQCtrl.ocx
InputPath=.\ReleaseU\BPQCtrl.ocx
SOURCE=$(InputPath)

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   regsvr32 /s /c "$(TargetPath)"
   echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"

# End Custom Build

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "BPQCtrl - Win32 Release"
# Name "BPQCtrl - Win32 Debug"
# Name "BPQCtrl - Win32 Unicode Debug"
# Name "BPQCtrl - Win32 Unicode Release"

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/BPQCtrl.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\BPQCtrl.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /FR"$(INTDIR)/"\
 /Fp"$(INTDIR)/BPQCtrl.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\StdAfx.sbr" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\BPQCtrl.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/BPQCtrl.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\BPQCtrl.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Fp"$(INTDIR)/BPQCtrl.pch"\
 /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\BPQCtrl.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BPQCtrl.cpp
DEP_CPP_BPQCT=\
	".\StdAfx.h"\
	".\BPQCtrl.h"\
	

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"


"$(INTDIR)\BPQCtrl.obj" : $(SOURCE) $(DEP_CPP_BPQCT) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"

"$(INTDIR)\BPQCtrl.sbr" : $(SOURCE) $(DEP_CPP_BPQCT) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"


"$(INTDIR)\BPQCtrl.obj" : $(SOURCE) $(DEP_CPP_BPQCT) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"

"$(INTDIR)\BPQCtrl.sbr" : $(SOURCE) $(DEP_CPP_BPQCT) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"


"$(INTDIR)\BPQCtrl.obj" : $(SOURCE) $(DEP_CPP_BPQCT) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"


"$(INTDIR)\BPQCtrl.obj" : $(SOURCE) $(DEP_CPP_BPQCT) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BPQCtrl.def

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BPQCtrl.rc

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"

DEP_RSC_BPQCTR=\
	".\BPQCtrl.ico"\
	".\BPQCtrlCtl.bmp"\
	".\Release\BPQCtrl.tlb"\
	

"$(INTDIR)\BPQCtrl.res" : $(SOURCE) $(DEP_RSC_BPQCTR) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.tlb"
   $(RSC) /l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /i "Release" /d "NDEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"

DEP_RSC_BPQCTR=\
	".\BPQCtrl.ico"\
	".\BPQCtrlCtl.bmp"\
	".\Debug\BPQCtrl.tlb"\
	

"$(INTDIR)\BPQCtrl.res" : $(SOURCE) $(DEP_RSC_BPQCTR) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.tlb"
   $(RSC) /l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /i "Debug" /d "_DEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"

DEP_RSC_BPQCTR=\
	".\BPQCtrl.ico"\
	".\BPQCtrlCtl.bmp"\
	
NODEP_RSC_BPQCTR=\
	".\DebugU\BPQCtrl.tlb"\
	

"$(INTDIR)\BPQCtrl.res" : $(SOURCE) $(DEP_RSC_BPQCTR) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.tlb"
   $(RSC) /l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /i "DebugU" /d "_DEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"

DEP_RSC_BPQCTR=\
	".\BPQCtrl.ico"\
	".\BPQCtrlCtl.bmp"\
	
NODEP_RSC_BPQCTR=\
	".\ReleaseU\BPQCtrl.tlb"\
	

"$(INTDIR)\BPQCtrl.res" : $(SOURCE) $(DEP_RSC_BPQCTR) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.tlb"
   $(RSC) /l 0x809 /fo"$(INTDIR)/BPQCtrl.res" /i "ReleaseU" /d "NDEBUG" /d\
 "_AFXDLL" $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BPQCtrl.odl

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"


"$(OUTDIR)\BPQCtrl.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "NDEBUG" /tlb "$(OUTDIR)/BPQCtrl.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"


"$(OUTDIR)\BPQCtrl.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/BPQCtrl.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"


"$(OUTDIR)\BPQCtrl.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "_DEBUG" /tlb "$(OUTDIR)/BPQCtrl.tlb" /win32 $(SOURCE)


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"


"$(OUTDIR)\BPQCtrl.tlb" : $(SOURCE) "$(OUTDIR)"
   $(MTL) /nologo /D "NDEBUG" /tlb "$(OUTDIR)/BPQCtrl.tlb" /win32 $(SOURCE)


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BPQCtrlCtl.cpp
DEP_CPP_BPQCTRL=\
	".\StdAfx.h"\
	".\BPQCtrl.h"\
	".\BPQCtrlCtl.h"\
	".\BPQCtrlPpg.h"\
	".\bpq32.h"\
	

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"


"$(INTDIR)\BPQCtrlCtl.obj" : $(SOURCE) $(DEP_CPP_BPQCTRL) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"

"$(INTDIR)\BPQCtrlCtl.sbr" : $(SOURCE) $(DEP_CPP_BPQCTRL) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"


"$(INTDIR)\BPQCtrlCtl.obj" : $(SOURCE) $(DEP_CPP_BPQCTRL) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"

"$(INTDIR)\BPQCtrlCtl.sbr" : $(SOURCE) $(DEP_CPP_BPQCTRL) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"


"$(INTDIR)\BPQCtrlCtl.obj" : $(SOURCE) $(DEP_CPP_BPQCTRL) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"


"$(INTDIR)\BPQCtrlCtl.obj" : $(SOURCE) $(DEP_CPP_BPQCTRL) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\BPQCtrlPpg.cpp
DEP_CPP_BPQCTRLP=\
	".\StdAfx.h"\
	".\BPQCtrl.h"\
	".\BPQCtrlPpg.h"\
	

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"


"$(INTDIR)\BPQCtrlPpg.obj" : $(SOURCE) $(DEP_CPP_BPQCTRLP) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"

"$(INTDIR)\BPQCtrlPpg.sbr" : $(SOURCE) $(DEP_CPP_BPQCTRLP) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"


"$(INTDIR)\BPQCtrlPpg.obj" : $(SOURCE) $(DEP_CPP_BPQCTRLP) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"

"$(INTDIR)\BPQCtrlPpg.sbr" : $(SOURCE) $(DEP_CPP_BPQCTRLP) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"


"$(INTDIR)\BPQCtrlPpg.obj" : $(SOURCE) $(DEP_CPP_BPQCTRLP) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"


"$(INTDIR)\BPQCtrlPpg.obj" : $(SOURCE) $(DEP_CPP_BPQCTRLP) "$(INTDIR)"\
 "$(INTDIR)\BPQCtrl.pch"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\AGWMon.asm

!IF  "$(CFG)" == "BPQCtrl - Win32 Release"

# Begin Custom Build
InputPath=.\AGWMon.asm
InputName=AGWMon

"$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   masm5  $(InputName) /Mx/z;
   c:\MSDEV\BIN\EDITBIN $(InputName).OBJ

# End Custom Build

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Debug"

# Begin Custom Build
InputPath=.\AGWMon.asm
InputName=AGWMon

"$(InputName).OBJ" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   masm5  $(InputName) /Mx/z/Zi/Zd;
   c:\MSDEV\BIN\EDITBIN $(InputName).OBJ

# End Custom Build

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "BPQCtrl - Win32 Unicode Release"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
