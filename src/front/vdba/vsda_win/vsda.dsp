# Microsoft Developer Studio Project File - Name="vsda" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vsda - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vsda.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vsda.mak" CFG="vsda - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vsda - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vsda - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vsda - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vsda - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Vsda", RXLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vsda - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libwctrl.lib libingll.lib libtree.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../App/vsda.ocx" /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Release
TargetPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vsda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vsda - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libwctrl.lib libingll.lib libtree.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../App/vsda.ocx" /pdbtype:sept /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vsda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vsda - Win32 Unicode Debug"

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
# ADD LINK32 ingres.lib libwctrl.lib libingll.lib libtree.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"../App/vsda.ocx" /pdbtype:sept /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugU
TargetPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vsda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vsda - Win32 Unicode Release"

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
# ADD LINK32 ingres.lib libwctrl.lib libingll.lib libtree.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../App/vsda.ocx" /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseU
TargetPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\vsda.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vsda.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "vsda - Win32 Release"
# Name "vsda - Win32 Debug"
# Name "vsda - Win32 Unicode Debug"
# Name "vsda - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dcdtdiff.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdiffls.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdtdiff.cpp
# End Source File
# Begin Source File

SOURCE=.\frdtdiff.cpp
# End Source File
# Begin Source File

SOURCE=.\loadsave.cpp
# End Source File
# Begin Source File

SOURCE=.\serdbase.cpp
# End Source File
# Begin Source File

SOURCE=.\serdbev.cpp
# End Source File
# Begin Source File

SOURCE=.\serproc.cpp
# End Source File
# Begin Source File

SOURCE=.\serseq.cpp
# End Source File
# Begin Source File

SOURCE=.\sertable.cpp
# End Source File
# Begin Source File

SOURCE=.\serview.cpp
# End Source File
# Begin Source File

SOURCE=.\snaparam.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\taskanim.cpp
# End Source File
# Begin Source File

SOURCE=.\viewleft.cpp
# End Source File
# Begin Source File

SOURCE=.\viewrigh.cpp
# End Source File
# Begin Source File

SOURCE=.\vsda.cpp
# End Source File
# Begin Source File

SOURCE=.\vsda.def
# End Source File
# Begin Source File

SOURCE=.\vsda.odl
# End Source File
# Begin Source File

SOURCE=.\vsda.rc
# End Source File
# Begin Source File

SOURCE=.\vsdactl.cpp
# End Source File
# Begin Source File

SOURCE=.\vsdadoc.cpp
# End Source File
# Begin Source File

SOURCE=.\vsdafrm.cpp
# End Source File
# Begin Source File

SOURCE=.\vsdappg.cpp
# End Source File
# Begin Source File

SOURCE=.\vsddml.cpp
# End Source File
# Begin Source File

SOURCE=.\vsdtree.cpp
# End Source File
# Begin Source File

SOURCE=.\vwdtdiff.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dcdtdiff.h
# End Source File
# Begin Source File

SOURCE=.\dgdiffls.h
# End Source File
# Begin Source File

SOURCE=.\dgdtdiff.h
# End Source File
# Begin Source File

SOURCE=.\frdtdiff.h
# End Source File
# Begin Source File

SOURCE=.\loadsave.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\serdbase.h
# End Source File
# Begin Source File

SOURCE=.\serdbev.h
# End Source File
# Begin Source File

SOURCE=.\serproc.h
# End Source File
# Begin Source File

SOURCE=.\serseq.h
# End Source File
# Begin Source File

SOURCE=.\sertable.h
# End Source File
# Begin Source File

SOURCE=.\serview.h
# End Source File
# Begin Source File

SOURCE=.\snaparam.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\taskanim.h
# End Source File
# Begin Source File

SOURCE=.\viewleft.h
# End Source File
# Begin Source File

SOURCE=.\viewrigh.h
# End Source File
# Begin Source File

SOURCE=.\vsda.h
# End Source File
# Begin Source File

SOURCE=.\vsdactl.h
# End Source File
# Begin Source File

SOURCE=.\vsdadoc.h
# End Source File
# Begin Source File

SOURCE=.\vsdafrm.h
# End Source File
# Begin Source File

SOURCE=.\vsdappg.h
# End Source File
# Begin Source File

SOURCE=.\vsddml.h
# End Source File
# Begin Source File

SOURCE=.\vsdtree.h
# End Source File
# Begin Source File

SOURCE=.\vwdtdiff.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\adatabas.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ainstall.bmp
# End Source File
# Begin Source File

SOURCE=.\res\alarms.bmp
# End Source File
# Begin Source File

SOURCE=.\res\atable.bmp
# End Source File
# Begin Source File

SOURCE=.\res\coordata.bmp
# End Source File
# Begin Source File

SOURCE=.\res\distdata.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gdatabas.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gdbevent.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ginstall.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gproc.bmp
# End Source File
# Begin Source File

SOURCE=.\res\grantees.bmp
# End Source File
# Begin Source File

SOURCE=.\res\grseq.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gtable.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gview.bmp
# End Source File
# Begin Source File

SOURCE=.\res\starinde.bmp
# End Source File
# Begin Source File

SOURCE=.\res\starltbl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\starlviw.bmp
# End Source File
# Begin Source File

SOURCE=.\res\starntbl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\starnviw.bmp
# End Source File
# Begin Source File

SOURCE=.\res\starproc.bmp
# End Source File
# Begin Source File

SOURCE=.\res\startabl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfdbase.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfdbeve.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfdiscd.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfempty.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trffoldr.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfgroup.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfindex.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfinsta.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfinteg.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trflocat.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfproc.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfprofi.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfrole.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfrule.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfsequ.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfsynon.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trftable.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfuser.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trfview.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vsdactl.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
