# Microsoft Developer Studio Project File - Name="vcda" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vcda - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vcda.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vcda.mak" CFG="vcda - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vcda - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vcda - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vcda - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vcda - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Vcda", YCMAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vcda - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libwctrl.lib libingll.lib tkanimat.lib ingres.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/vcda.ocx" /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Release
TargetPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
InputPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcda - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libwctrl.lib libingll.lib tkanimat.lib ingres.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../app/vcda.ocx" /pdbtype:sept /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
InputPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcda - Win32 Unicode Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libwctrl.lib libingll.lib tkanimat.lib ingres.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../app/vcda.ocx" /pdbtype:sept /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugU
TargetPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
InputPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcda - Win32 Unicode Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libwctrl.lib libingll.lib tkanimat.lib ingres.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/vcda.ocx" /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseU
TargetPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
InputPath=\Development\Ingres30\Front\Vdba\app\vcda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "vcda - Win32 Release"
# Name "vcda - Win32 Debug"
# Name "vcda - Win32 Unicode Debug"
# Name "vcda - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\calsmain.cpp
# End Source File
# Begin Source File

SOURCE=.\pagediff.cpp
# End Source File
# Begin Source File

SOURCE=.\ppagpat1.cpp
# End Source File
# Begin Source File

SOURCE=.\ppagpat2.cpp
# End Source File
# Begin Source File

SOURCE=.\ppgrfina.cpp
# End Source File
# Begin Source File

SOURCE=.\ppgropti.cpp
# End Source File
# Begin Source File

SOURCE=.\pshresto.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\udlgmain.cpp
# End Source File
# Begin Source File

SOURCE=.\vcda.cpp
# End Source File
# Begin Source File

SOURCE=.\vcda.def
# End Source File
# Begin Source File

SOURCE=.\vcda.odl
# End Source File
# Begin Source File

SOURCE=.\vcda.rc
# End Source File
# Begin Source File

SOURCE=.\vcdactl.cpp
# End Source File
# Begin Source File

SOURCE=.\vcdadml.cpp
# End Source File
# Begin Source File

SOURCE=.\vcdappg.cpp
# End Source File
# Begin Source File

SOURCE=.\xmaphost.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\calsmain.h
# End Source File
# Begin Source File

SOURCE=.\pagediff.h
# End Source File
# Begin Source File

SOURCE=.\ppagpat1.h
# End Source File
# Begin Source File

SOURCE=.\ppagpat2.h
# End Source File
# Begin Source File

SOURCE=.\ppgrfina.h
# End Source File
# Begin Source File

SOURCE=.\ppgropti.h
# End Source File
# Begin Source File

SOURCE=.\pshresto.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\udlgmain.h
# End Source File
# Begin Source File

SOURCE=.\vcda.h
# End Source File
# Begin Source File

SOURCE=.\vcdactl.h
# End Source File
# Begin Source File

SOURCE=.\vcdadml.h
# End Source File
# Begin Source File

SOURCE=.\vcdappg.h
# End Source File
# Begin Source File

SOURCE=.\xmaphost.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\differen.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vcdactl.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Target
# End Project
