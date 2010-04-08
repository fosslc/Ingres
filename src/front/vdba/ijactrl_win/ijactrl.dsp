# Microsoft Developer Studio Project File - Name="IjaCtrl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=IjaCtrl - Win32 Linux_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ijactrl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ijactrl.mak" CFG="IjaCtrl - Win32 Linux_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "IjaCtrl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Debug Simulation" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Release Simulation" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Debug Vut Configuration" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 DBCS Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 DBCS Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Solaris_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 HPUX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 HPUX_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Solaris_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Linux_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 AIX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IjaCtrl - Win32 Linux_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Ija Control", ADGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "IjaCtrl - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\ijactrl.ocx" /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Release
TargetPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ijactrl.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ijactrl.ocx" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ijactrl.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Unicode Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugU
TargetPath=.\DebugU\ijactrl.ocx
InputPath=.\DebugU\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Unicode Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseU"
# PROP BASE Intermediate_Dir "ReleaseU"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseU
TargetPath=.\ReleaseU\ijactrl.ocx
InputPath=.\ReleaseU\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug Simulation"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "IjaCtrl_"
# PROP BASE Intermediate_Dir "IjaCtrl_"
# PROP BASE Target_Ext "ocx"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugSim"
# PROP Intermediate_Dir "DebugSim"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "SIMULATION" /D "FGCOLOR" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugSim
TargetPath=.\DebugSim\ijactrl.ocx
InputPath=.\DebugSim\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Release Simulation"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IjaCtrl___Win32_Release_Simulation"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_Release_Simulation"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "RelSim"
# PROP Intermediate_Dir "RelSim"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "SIMULATION" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\RelSim
TargetPath=.\RelSim\ijactrl.ocx
InputPath=.\RelSim\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug Vut Configuration"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "IjaCtrl___Win32_Debug_Vut_Configuration"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_Debug_Vut_Configuration"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugVut"
# PROP Intermediate_Dir "DebugVut"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "SIMULATION" /D "FGCOLOR" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "FGCOLOR" /D "SORT_INDICATOR" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugVut
TargetPath=.\DebugVut\ijactrl.ocx
InputPath=.\DebugVut\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 DBCS Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "IjaCtrl___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_DBCS_Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_MBCS" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ijactrl.ocx" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DBCS Debug
TargetPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ijactrl.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 DBCS Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IjaCtrl___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_DBCS_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_MBCS" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\ijactrl.ocx" /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DBCS Release
TargetPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ijactrl.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Solaris_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IjaCtrl___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_Solaris_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IjaCtrl___Win32_Solaris_Release"
# PROP Intermediate_Dir "IjaCtrl___Win32_Solaris_Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /D "MAINWIN" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\IjaCtrl___Win32_Solaris_Release
TargetPath=.\IjaCtrl___Win32_Solaris_Release\ijactrl.ocx
InputPath=.\IjaCtrl___Win32_Solaris_Release\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 HPUX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IjaCtrl___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_HPUX_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IjaCtrl___Win32_HPUX_Release"
# PROP Intermediate_Dir "IjaCtrl___Win32_HPUX_Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 q.1.lib compat.1.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"winspool.lib" /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\IjaCtrl___Win32_HPUX_Release
TargetPath=.\IjaCtrl___Win32_HPUX_Release\ijactrl.ocx
InputPath=.\IjaCtrl___Win32_HPUX_Release\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 HPUX_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "IjaCtrl___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_HPUX_Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "IjaCtrl___Win32_HPUX_Debug"
# PROP Intermediate_Dir "IjaCtrl___Win32_HPUX_Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# ADD LINK32 q.1.lib compat.1.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib winspool.lib" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\IjaCtrl___Win32_HPUX_Debug
TargetPath=.\IjaCtrl___Win32_HPUX_Debug\ijactrl.ocx
InputPath=.\IjaCtrl___Win32_HPUX_Debug\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Solaris_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "IjaCtrl___Win32_Solaris_Debug0"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_Solaris_Debug0"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "IjaCtrl___Win32_Solaris_Debug0"
# PROP Intermediate_Dir "IjaCtrl___Win32_Solaris_Debug0"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\IjaCtrl___Win32_Solaris_Debug0
TargetPath=.\IjaCtrl___Win32_Solaris_Debug0\ijactrl.ocx
InputPath=.\IjaCtrl___Win32_Solaris_Debug0\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Linux_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IjaCtrl___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_Linux_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IjaCtrl___Win32_Linux_Release"
# PROP Intermediate_Dir "IjaCtrl___Win32_Linux_Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib dl.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\IjaCtrl___Win32_Linux_Release
TargetPath=.\IjaCtrl___Win32_Linux_Release\ijactrl.ocx
InputPath=.\IjaCtrl___Win32_Linux_Release\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 AIX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "IjaCtrl___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_AIX_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "IjaCtrl___Win32_AIX_Release"
# PROP Intermediate_Dir "IjaCtrl___Win32_AIX_Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\IjaCtrl___Win32_AIX_Release
TargetPath=.\IjaCtrl___Win32_AIX_Release\ijactrl.ocx
InputPath=.\IjaCtrl___Win32_AIX_Release\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Linux_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "IjaCtrl___Win32_Linux_Debug"
# PROP BASE Intermediate_Dir "IjaCtrl___Win32_Linux_Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "IjaCtrl___Win32_Linux_Debug"
# PROP Intermediate_Dir "IjaCtrl___Win32_Linux_Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\hdr" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ijactrl.ocx" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib"
# ADD LINK32 ingres.lib dl.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ijactrl.ocx" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\IjaCtrl___Win32_Linux_Debug
TargetPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ijactrl.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ijactrl.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "IjaCtrl - Win32 Release"
# Name "IjaCtrl - Win32 Debug"
# Name "IjaCtrl - Win32 Unicode Debug"
# Name "IjaCtrl - Win32 Unicode Release"
# Name "IjaCtrl - Win32 Debug Simulation"
# Name "IjaCtrl - Win32 Release Simulation"
# Name "IjaCtrl - Win32 Debug Vut Configuration"
# Name "IjaCtrl - Win32 DBCS Debug"
# Name "IjaCtrl - Win32 DBCS Release"
# Name "IjaCtrl - Win32 Solaris_Release"
# Name "IjaCtrl - Win32 HPUX_Release"
# Name "IjaCtrl - Win32 HPUX_Debug"
# Name "IjaCtrl - Win32 Solaris_Debug"
# Name "IjaCtrl - Win32 Linux_Release"
# Name "IjaCtrl - Win32 AIX_Release"
# Name "IjaCtrl - Win32 Linux_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\chkpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\choosedb.cpp
# End Source File
# Begin Source File

SOURCE=.\ckplst.cpp
# End Source File
# Begin Source File

SOURCE=.\dglishdr.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgmsbox.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlconst.cpp
# End Source File
# Begin Source File

SOURCE=.\drecover.cpp
# End Source File
# Begin Source File

SOURCE=.\dredo.cpp
# End Source File
# Begin Source File

SOURCE=.\ijactdml.cpp
# End Source File
# Begin Source File

SOURCE=.\ijactppg.cpp
# End Source File
# Begin Source File

SOURCE=.\ijactrl.cpp
# End Source File
# Begin Source File

SOURCE=.\ijactrl.def
# End Source File
# Begin Source File

SOURCE=.\ijactrl.odl
# End Source File
# Begin Source File

SOURCE=.\ijactrl.rc
# End Source File
# Begin Source File

SOURCE=.\ijactrlc.cpp
# End Source File
# Begin Source File

SOURCE=.\ijadbase.cpp
# End Source File
# Begin Source File

SOURCE=.\ijadmlll.cpp
# End Source File
# Begin Source File

SOURCE=.\ijalldml.cpp
# End Source File
# Begin Source File

SOURCE=.\ijatable.cpp
# End Source File
# Begin Source File

SOURCE=.\logadata.cpp
# End Source File
# Begin Source File

SOURCE=.\multflag.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\rcrdtool.cpp
# End Source File
# Begin Source File

SOURCE=.\rdrcmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\remotecm.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\xscript.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\chkpoint.h
# End Source File
# Begin Source File

SOURCE=.\choosedb.h
# End Source File
# Begin Source File

SOURCE=.\ckplst.h
# End Source File
# Begin Source File

SOURCE=.\dglishdr.h
# End Source File
# Begin Source File

SOURCE=.\dlgmsbox.h
# End Source File
# Begin Source File

SOURCE=.\dmlconst.h
# End Source File
# Begin Source File

SOURCE=.\drecover.h
# End Source File
# Begin Source File

SOURCE=.\dredo.h
# End Source File
# Begin Source File

SOURCE=.\ijactdml.h
# End Source File
# Begin Source File

SOURCE=.\ijactppg.h
# End Source File
# Begin Source File

SOURCE=.\ijactrl.h
# End Source File
# Begin Source File

SOURCE=.\ijactrlc.h
# End Source File
# Begin Source File

SOURCE=.\ijadbase.h
# End Source File
# Begin Source File

SOURCE=.\ijadmlll.h
# End Source File
# Begin Source File

SOURCE=.\ijalldml.h
# End Source File
# Begin Source File

SOURCE=.\ijatable.h
# End Source File
# Begin Source File

SOURCE=.\logadata.h
# End Source File
# Begin Source File

SOURCE=.\multflag.h
# End Source File
# Begin Source File

SOURCE=.\parser.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\rcrdtool.h
# End Source File
# Begin Source File

SOURCE=.\rdrcmsg.h
# End Source File
# Begin Source File

SOURCE=.\remotecm.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\xscript.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\about.bmp
# End Source File
# Begin Source File

SOURCE=.\CHECKNOF.BMP
# End Source File
# Begin Source File

SOURCE=.\headord.bmp
# End Source File
# Begin Source File

SOURCE=.\ija.ico
# End Source File
# Begin Source File

SOURCE=.\ijactrl.ico
# End Source File
# Begin Source File

SOURCE=.\ijactrlc.bmp
# End Source File
# Begin Source File

SOURCE=.\ijaimag2.bmp
# End Source File
# Begin Source File

SOURCE=.\ijaimage.bmp
# End Source File
# End Group
# Begin Group "Sql Files"

# PROP Default_Filter "*.scc"
# Begin Source File

SOURCE=.\ijadmlll.scc

!IF  "$(CFG)" == "IjaCtrl - Win32 Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Unicode Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Unicode Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug Simulation"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Release Simulation"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug Vut Configuration"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 DBCS Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 DBCS Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Solaris_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 HPUX_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 HPUX_Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Solaris_Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Linux_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 AIX_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Linux_Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\ijadmlll.scc

"ijadmlll.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fijadmlll.inc ijadmlll.scc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\remotecm.scc

!IF  "$(CFG)" == "IjaCtrl - Win32 Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Unicode Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Unicode Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug Simulation"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Release Simulation"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Debug Vut Configuration"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 DBCS Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 DBCS Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Solaris_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 HPUX_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 HPUX_Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Solaris_Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Linux_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 AIX_Release"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "IjaCtrl - Win32 Linux_Debug"

# Begin Custom Build - Performing ESQLCC
InputPath=.\remotecm.scc

"remotecm.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fremotecm.inc remotecm.scc

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Target
# End Project
