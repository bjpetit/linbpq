@echo off
REM -- First, make map file from resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by BPQCTRL.HPJ. >hlp\BPQCtrl.hm
echo. >>hlp\BPQCtrl.hm
echo // Commands (ID_* and IDM_*) >>hlp\BPQCtrl.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\BPQCtrl.hm
echo. >>hlp\BPQCtrl.hm
echo // Prompts (IDP_*) >>hlp\BPQCtrl.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\BPQCtrl.hm
echo. >>hlp\BPQCtrl.hm
echo // Resources (IDR_*) >>hlp\BPQCtrl.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\BPQCtrl.hm
echo. >>hlp\BPQCtrl.hm
echo // Dialogs (IDD_*) >>hlp\BPQCtrl.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\BPQCtrl.hm
echo. >>hlp\BPQCtrl.hm
echo // Frame Controls (IDW_*) >>hlp\BPQCtrl.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\BPQCtrl.hm
REM -- Make help for Project BPQCTRL
call hc31 BPQCtrl.hpj
echo.
