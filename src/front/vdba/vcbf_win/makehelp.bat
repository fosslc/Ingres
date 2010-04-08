@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by VCBF.HPJ. >"hlp\vcbf.hm"
echo. >>"hlp\vcbf.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\vcbf.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\vcbf.hm"
echo. >>"hlp\vcbf.hm"
echo // Prompts (IDP_*) >>"hlp\vcbf.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\vcbf.hm"
echo. >>"hlp\vcbf.hm"
echo // Resources (IDR_*) >>"hlp\vcbf.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\vcbf.hm"
echo. >>"hlp\vcbf.hm"
echo // Dialogs (IDD_*) >>"hlp\vcbf.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\vcbf.hm"
echo. >>"hlp\vcbf.hm"
echo // Frame Controls (IDW_*) >>"hlp\vcbf.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\vcbf.hm"
REM -- Make help for Project VCBF


echo Building Win32 Help files
start /wait hcrtf -x "hlp\vcbf.hpj"
echo.
if exist Debug\nul copy "hlp\vcbf.hlp" Debug
if exist Debug\nul copy "hlp\vcbf.cnt" Debug
if exist Release\nul copy "hlp\vcbf.hlp" Release
if exist Release\nul copy "hlp\vcbf.cnt" Release
echo.


