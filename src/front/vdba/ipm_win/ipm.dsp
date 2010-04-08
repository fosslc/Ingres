# Microsoft Developer Studio Project File - Name="ipm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ipm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ipm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ipm.mak" CFG="ipm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ipm - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ipm - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ipm - Win32 Unicode Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ipm - Win32 Unicode Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/ipm", NOIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ipm - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib tkanimat.lib libmon.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App/ipm.ocx" /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Release
TargetPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\ipm.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ipm - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib tkanimat.lib libmon.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App/ipm.ocx" /pdbtype:sept /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\ipm.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ipm - Win32 Unicode Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib tkanimat.lib libmon.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App/ipm.ocx" /pdbtype:sept /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugU
TargetPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\ipm.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ipm - Win32 Unicode Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libingll.lib libwctrl.lib tkanimat.lib libmon.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App/ipm.ocx" /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseU
TargetPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\ipm.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\ipm.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ipm - Win32 Release"
# Name "ipm - Win32 Debug"
# Name "ipm - Win32 Unicode Debug"
# Name "ipm - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cmddrive.cpp
# End Source File
# Begin Source File

SOURCE=.\collidta.cpp
# End Source File
# Begin Source File

SOURCE=.\ctltree.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiclien.cpp
# End Source File
# Begin Source File

SOURCE=.\dgidausr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgidcpag.cpp
# End Source File
# Begin Source File

SOURCE=.\dgidcurs.cpp
# End Source File
# Begin Source File

SOURCE=.\dgidcusr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiddbco.cpp
# End Source File
# Begin Source File

SOURCE=.\dgidhttp.cpp
# End Source File
# Begin Source File

SOURCE=.\dgidtran.cpp
# End Source File
# Begin Source File

SOURCE=.\dgilcsum.cpp
# End Source File
# Begin Source File

SOURCE=.\dgilgsum.cpp
# End Source File
# Begin Source File

SOURCE=.\dgillist.cpp
# End Source File
# Begin Source File

SOURCE=.\dgilocks.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipacdb.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipcusr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiphead.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipicac.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipiccp.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipiccu.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipicdc.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipicht.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipictr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipllis.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiplock.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiplogf.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiplres.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipltbl.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipmc02.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipproc.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipross.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipsess.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipstdb.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipstre.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipstsv.cpp
# End Source File
# Begin Source File

SOURCE=.\dgipstus.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiptran.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiresou.cpp
# End Source File
# Begin Source File

SOURCE=.\dgiservr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgisess.cpp
# End Source File
# Begin Source File

SOURCE=.\dgitrafi.cpp
# End Source File
# Begin Source File

SOURCE=.\dgitrans.cpp
# End Source File
# Begin Source File

SOURCE=.\dgrepco2.cpp
# End Source File
# Begin Source File

SOURCE=.\dgrepcor.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgvdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgvfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\editlsgn.cpp
# End Source File
# Begin Source File

SOURCE=.\frepcoll.cpp
# End Source File
# Begin Source File

SOURCE=.\ipm.cpp
# End Source File
# Begin Source File

SOURCE=.\ipm.def
# End Source File
# Begin Source File

SOURCE=.\ipm.odl
# End Source File
# Begin Source File

SOURCE=.\ipm.rc
# End Source File
# Begin Source File

SOURCE=.\ipmctl.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmdisp.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmdml.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmfast.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmframe.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmicdta.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmppg.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmprdta.cpp
# End Source File
# Begin Source File

SOURCE=.\logindic.cpp
# End Source File
# Begin Source File

SOURCE=.\lsrgbitm.cpp
# End Source File
# Begin Source File

SOURCE=.\monrepli.cpp
# End Source File
# Begin Source File

SOURCE=.\montree.cpp
# End Source File
# Begin Source File

SOURCE=.\montreei.cpp
# End Source File
# Begin Source File

SOURCE=.\repcodoc.cpp
# End Source File
# Begin Source File

SOURCE=.\repevent.cpp
# End Source File
# Begin Source File

SOURCE=.\repinteg.cpp
# End Source File
# Begin Source File

SOURCE=.\repitem.cpp
# End Source File
# Begin Source File

SOURCE=.\rsactidb.cpp
# End Source File
# Begin Source File

SOURCE=.\rsactitb.cpp
# End Source File
# Begin Source File

SOURCE=.\rsactivi.cpp
# End Source File
# Begin Source File

SOURCE=.\rscollis.cpp
# End Source File
# Begin Source File

SOURCE=.\rsdistri.cpp
# End Source File
# Begin Source File

SOURCE=.\rsintegr.cpp
# End Source File
# Begin Source File

SOURCE=.\rsrevent.cpp
# End Source File
# Begin Source File

SOURCE=.\rsserver.cpp
# End Source File
# Begin Source File

SOURCE=.\runnode.cpp
# End Source File
# Begin Source File

SOURCE=.\rvassign.cpp
# End Source File
# Begin Source File

SOURCE=.\rvrevent.cpp
# End Source File
# Begin Source File

SOURCE=.\rvstartu.cpp
# End Source File
# Begin Source File

SOURCE=.\rvstatus.cpp
# End Source File
# Begin Source File

SOURCE=.\statleg2.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\taskanim.cpp
# End Source File
# Begin Source File

SOURCE=.\treerepl.cpp
# End Source File
# Begin Source File

SOURCE=.\viewleft.cpp
# End Source File
# Begin Source File

SOURCE=.\viewrigh.cpp
# End Source File
# Begin Source File

SOURCE=.\vrepcolf.cpp
# End Source File
# Begin Source File

SOURCE=.\vrepcort.cpp
# End Source File
# Begin Source File

SOURCE=.\vtree.cpp
# End Source File
# Begin Source File

SOURCE=.\vtreectl.cpp
# End Source File
# Begin Source File

SOURCE=.\vtreedat.cpp
# End Source File
# Begin Source File

SOURCE=.\vtreeglb.cpp
# End Source File
# Begin Source File

SOURCE=.\xdgreque.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgacco.cpp
# End Source File
# Begin Source File

SOURCE=.\xrepsvrl.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cmddrive.h
# End Source File
# Begin Source File

SOURCE=.\collidta.h
# End Source File
# Begin Source File

SOURCE=.\ctltree.h
# End Source File
# Begin Source File

SOURCE=.\dgiclien.h
# End Source File
# Begin Source File

SOURCE=.\dgidausr.h
# End Source File
# Begin Source File

SOURCE=.\dgidcpag.h
# End Source File
# Begin Source File

SOURCE=.\dgidcurs.h
# End Source File
# Begin Source File

SOURCE=.\dgidcusr.h
# End Source File
# Begin Source File

SOURCE=.\dgiddbco.h
# End Source File
# Begin Source File

SOURCE=.\dgidhttp.h
# End Source File
# Begin Source File

SOURCE=.\dgidtran.h
# End Source File
# Begin Source File

SOURCE=.\dgilcsum.h
# End Source File
# Begin Source File

SOURCE=.\dgilgsum.h
# End Source File
# Begin Source File

SOURCE=.\dgillist.h
# End Source File
# Begin Source File

SOURCE=.\dgilocks.h
# End Source File
# Begin Source File

SOURCE=.\dgipacdb.h
# End Source File
# Begin Source File

SOURCE=.\dgipcusr.h
# End Source File
# Begin Source File

SOURCE=.\dgiphead.h
# End Source File
# Begin Source File

SOURCE=.\dgipicac.h
# End Source File
# Begin Source File

SOURCE=.\dgipiccp.h
# End Source File
# Begin Source File

SOURCE=.\dgipiccu.h
# End Source File
# Begin Source File

SOURCE=.\dgipicdc.h
# End Source File
# Begin Source File

SOURCE=.\dgipicht.h
# End Source File
# Begin Source File

SOURCE=.\dgipictr.h
# End Source File
# Begin Source File

SOURCE=.\dgipllis.h
# End Source File
# Begin Source File

SOURCE=.\dgiplock.h
# End Source File
# Begin Source File

SOURCE=.\dgiplogf.h
# End Source File
# Begin Source File

SOURCE=.\dgiplres.h
# End Source File
# Begin Source File

SOURCE=.\dgipltbl.h
# End Source File
# Begin Source File

SOURCE=.\dgipmc02.h
# End Source File
# Begin Source File

SOURCE=.\dgipproc.h
# End Source File
# Begin Source File

SOURCE=.\dgipross.h
# End Source File
# Begin Source File

SOURCE=.\dgipsess.h
# End Source File
# Begin Source File

SOURCE=.\dgipstdb.h
# End Source File
# Begin Source File

SOURCE=.\dgipstre.h
# End Source File
# Begin Source File

SOURCE=.\dgipstsv.h
# End Source File
# Begin Source File

SOURCE=.\dgipstus.h
# End Source File
# Begin Source File

SOURCE=.\dgiptran.h
# End Source File
# Begin Source File

SOURCE=.\dgiresou.h
# End Source File
# Begin Source File

SOURCE=.\dgiservr.h
# End Source File
# Begin Source File

SOURCE=.\dgisess.h
# End Source File
# Begin Source File

SOURCE=.\dgitrafi.h
# End Source File
# Begin Source File

SOURCE=.\dgitrans.h
# End Source File
# Begin Source File

SOURCE=.\dgrepco2.h
# End Source File
# Begin Source File

SOURCE=.\dgrepcor.h
# End Source File
# Begin Source File

SOURCE=.\dlgvdoc.h
# End Source File
# Begin Source File

SOURCE=.\dlgvfrm.h
# End Source File
# Begin Source File

SOURCE=.\editlsgn.h
# End Source File
# Begin Source File

SOURCE=.\frepcoll.h
# End Source File
# Begin Source File

SOURCE=.\ipm.h
# End Source File
# Begin Source File

SOURCE=.\ipmctl.h
# End Source File
# Begin Source File

SOURCE=.\ipmdisp.h
# End Source File
# Begin Source File

SOURCE=.\ipmdml.h
# End Source File
# Begin Source File

SOURCE=.\ipmdoc.h
# End Source File
# Begin Source File

SOURCE=.\ipmfast.h
# End Source File
# Begin Source File

SOURCE=.\ipmframe.h
# End Source File
# Begin Source File

SOURCE=.\ipmicdta.h
# End Source File
# Begin Source File

SOURCE=.\ipmppg.h
# End Source File
# Begin Source File

SOURCE=.\ipmprdta.h
# End Source File
# Begin Source File

SOURCE=.\ipmstruc.h
# End Source File
# Begin Source File

SOURCE=.\logindic.h
# End Source File
# Begin Source File

SOURCE=.\lsrgbitm.h
# End Source File
# Begin Source File

SOURCE=.\monrepli.h
# End Source File
# Begin Source File

SOURCE=.\montree.h
# End Source File
# Begin Source File

SOURCE=.\montreei.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\repcodoc.h
# End Source File
# Begin Source File

SOURCE=.\repevent.h
# End Source File
# Begin Source File

SOURCE=.\repinteg.h
# End Source File
# Begin Source File

SOURCE=.\repitem.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\rsactidb.h
# End Source File
# Begin Source File

SOURCE=.\rsactitb.h
# End Source File
# Begin Source File

SOURCE=.\rsactivi.h
# End Source File
# Begin Source File

SOURCE=.\rscollis.h
# End Source File
# Begin Source File

SOURCE=.\rsdistri.h
# End Source File
# Begin Source File

SOURCE=.\rsintegr.h
# End Source File
# Begin Source File

SOURCE=.\rsrevent.h
# End Source File
# Begin Source File

SOURCE=.\rsserver.h
# End Source File
# Begin Source File

SOURCE=.\runnode.h
# End Source File
# Begin Source File

SOURCE=.\rvassign.h
# End Source File
# Begin Source File

SOURCE=.\rvrevent.h
# End Source File
# Begin Source File

SOURCE=.\rvstartu.h
# End Source File
# Begin Source File

SOURCE=.\rvstatus.h
# End Source File
# Begin Source File

SOURCE=.\statleg2.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\taskanim.h
# End Source File
# Begin Source File

SOURCE=.\treerepl.h
# End Source File
# Begin Source File

SOURCE=.\viewleft.h
# End Source File
# Begin Source File

SOURCE=.\viewrigh.h
# End Source File
# Begin Source File

SOURCE=.\vrepcolf.h
# End Source File
# Begin Source File

SOURCE=.\vrepcort.h
# End Source File
# Begin Source File

SOURCE=.\vtree.h
# End Source File
# Begin Source File

SOURCE=.\vtreectl.h
# End Source File
# Begin Source File

SOURCE=.\vtreedat.h
# End Source File
# Begin Source File

SOURCE=.\vtreeglb.h
# End Source File
# Begin Source File

SOURCE=.\xdgreque.h
# End Source File
# Begin Source File

SOURCE=.\xdlgacco.h
# End Source File
# Begin Source File

SOURCE=.\xrepsvrl.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\collisio.bmp
# End Source File
# Begin Source File

SOURCE=.\res\datauser.bmp
# End Source File
# Begin Source File

SOURCE=.\res\dbobject.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icecurso.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icecuser.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icedbcon.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icefileu.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icehttpc.bmp
# End Source File
# Begin Source File

SOURCE=.\res\icetrans.bmp
# End Source File
# Begin Source File

SOURCE=.\res\iceusera.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ipmctl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\loginfo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\logprogr.bmp
# End Source File
# Begin Source File

SOURCE=.\res\svrr.bmp
# End Source File
# Begin Source File

SOURCE=.\res\svrrdown.bmp
# End Source File
# Begin Source File

SOURCE=.\res\svrrerr.bmp
# End Source File
# Begin Source File

SOURCE=.\res\svrrup.bmp
# End Source File
# End Group
# End Target
# End Project
