# Microsoft Developer Studio Project File - Name="sqlquery" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sqlquery - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sqlquery.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sqlquery.mak" CFG="sqlquery - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sqlquery - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sqlquery - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sqlquery - Win32 DebugU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sqlquery - Win32 ReleaseU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sqlquery - Win32 EDBC_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sqlquery - Win32 EDBC_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Sqlquery", SFIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sqlquery - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\sqlquery.ocx" /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Release
TargetPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\sqlquery.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sqlquery - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\sqlquery.ocx" /pdbtype:sept /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\Debug
TargetPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\sqlquery.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sqlquery - Win32 DebugU"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\sqlquery.ocx" /pdbtype:sept /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\DebugU
TargetPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\sqlquery.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sqlquery - Win32 ReleaseU"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\sqlquery.ocx" /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\ReleaseU
TargetPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\sqlquery.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\sqlquery.ocx %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sqlquery - Win32 EDBC_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sqlquery___Win32_EDBC_Debug"
# PROP BASE Intermediate_Dir "sqlquery___Win32_EDBC_Debug"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /D "EDBC" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib libingll.lib libtree.lib ingres.lib libwctrl.lib tkanimat.lib ws2_32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\sqlquery.ocx" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 edbc.lib libguids.lib libingll.lib libtree.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\edbquery.ocx" /pdbtype:sept /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\EDBC_Debug
TargetPath=\Development\Ingres30\Front\Vdba\App\edbquery.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\edbquery.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbquery %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "sqlquery - Win32 EDBC_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sqlquery___Win32_EDBC_Release"
# PROP BASE Intermediate_Dir "sqlquery___Win32_EDBC_Release"
# PROP BASE Target_Ext "ocx"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Target_Ext "ocx"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /D "EDBC" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib libingll.lib libtree.lib ingres.lib libwctrl.lib tkanimat.lib ws2_32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\sqlquery.ocx" /libpath:"..\lib"
# ADD LINK32 edbc.lib libguids.lib libingll.lib libtree.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\edbquery.ocx" /libpath:"..\lib"
# Begin Custom Build - Registering ActiveX Control...
OutDir=.\EDBC_Release
TargetPath=\Development\Ingres30\Front\Vdba\App\edbquery.ocx
InputPath=\Development\Ingres30\Front\Vdba\App\edbquery.ocx
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbquery %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "sqlquery - Win32 Release"
# Name "sqlquery - Win32 Debug"
# Name "sqlquery - Win32 DebugU"
# Name "sqlquery - Win32 ReleaseU"
# Name "sqlquery - Win32 EDBC_Debug"
# Name "sqlquery - Win32 EDBC_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\parse.cpp
# End Source File
# Begin Source File

SOURCE=.\prevboxn.cpp
# End Source File
# Begin Source File

SOURCE=.\prevboxs.cpp
# End Source File
# Begin Source File

SOURCE=.\qboxleaf.cpp
# End Source File
# Begin Source File

SOURCE=.\qboxstar.cpp
# End Source File
# Begin Source File

SOURCE=.\qboxstlf.cpp
# End Source File
# Begin Source File

SOURCE=.\qepboxdg.cpp
# End Source File
# Begin Source File

SOURCE=.\qepbtree.cpp
# End Source File
# Begin Source File

SOURCE=.\qepdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\qepframe.cpp
# End Source File
# Begin Source File

SOURCE=.\qepview.cpp
# End Source File
# Begin Source File

SOURCE=.\qframe.cpp
# End Source File
# Begin Source File

SOURCE=.\qlowview.cpp
# End Source File
# Begin Source File

SOURCE=.\qpagegen.cpp
# End Source File
# Begin Source File

SOURCE=.\qpageqep.cpp
# End Source File
# Begin Source File

SOURCE=.\qpageqin.cpp
# End Source File
# Begin Source File

SOURCE=.\qpageraw.cpp
# End Source File
# Begin Source File

SOURCE=.\qpageres.cpp
# End Source File
# Begin Source File

SOURCE=.\qpagexml.cpp
# End Source File
# Begin Source File

SOURCE=.\qrecnitm.cpp
# End Source File
# Begin Source File

SOURCE=.\qredoc.cpp
# End Source File
# Begin Source File

SOURCE=.\qresudlg.cpp
# End Source File
# Begin Source File

SOURCE=.\qresurow.cpp
# End Source File
# Begin Source File

SOURCE=.\qreview.cpp
# End Source File
# Begin Source File

SOURCE=.\qsplittr.cpp
# End Source File
# Begin Source File

SOURCE=.\qstatind.cpp
# End Source File
# Begin Source File

SOURCE=.\qstmtdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\qupview.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlpgdis.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlpgtab.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlprop.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlqryct.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlqrypg.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlquery.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlquery.def
# End Source File
# Begin Source File

SOURCE=.\sqlquery.odl
# End Source File
# Begin Source File

SOURCE=.\sqlquery.rc
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\taskxprm.cpp
# End Source File
# Begin Source File

SOURCE=.\webbrows.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\parse.h
# End Source File
# Begin Source File

SOURCE=.\prevboxn.h
# End Source File
# Begin Source File

SOURCE=.\prevboxs.h
# End Source File
# Begin Source File

SOURCE=.\qboxleaf.h
# End Source File
# Begin Source File

SOURCE=.\qboxstar.h
# End Source File
# Begin Source File

SOURCE=.\qboxstlf.h
# End Source File
# Begin Source File

SOURCE=.\qepboxdg.h
# End Source File
# Begin Source File

SOURCE=.\qepbtree.h
# End Source File
# Begin Source File

SOURCE=.\qepdoc.h
# End Source File
# Begin Source File

SOURCE=.\qepframe.h
# End Source File
# Begin Source File

SOURCE=.\qepview.h
# End Source File
# Begin Source File

SOURCE=.\qframe.h
# End Source File
# Begin Source File

SOURCE=.\qlowview.h
# End Source File
# Begin Source File

SOURCE=.\qpagegen.h
# End Source File
# Begin Source File

SOURCE=.\qpageqep.h
# End Source File
# Begin Source File

SOURCE=.\qpageqin.h
# End Source File
# Begin Source File

SOURCE=.\qpageraw.h
# End Source File
# Begin Source File

SOURCE=.\qpageres.h
# End Source File
# Begin Source File

SOURCE=.\qpagexml.h
# End Source File
# Begin Source File

SOURCE=.\qrecnitm.h
# End Source File
# Begin Source File

SOURCE=.\qredoc.h
# End Source File
# Begin Source File

SOURCE=.\qresudlg.h
# End Source File
# Begin Source File

SOURCE=.\qresurow.h
# End Source File
# Begin Source File

SOURCE=.\qreview.h
# End Source File
# Begin Source File

SOURCE=.\qsplittr.h
# End Source File
# Begin Source File

SOURCE=.\qstatind.h
# End Source File
# Begin Source File

SOURCE=.\qstmtdlg.h
# End Source File
# Begin Source File

SOURCE=.\qupview.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sqlpgdis.h
# End Source File
# Begin Source File

SOURCE=.\sqlpgtab.h
# End Source File
# Begin Source File

SOURCE=.\sqlprop.h
# End Source File
# Begin Source File

SOURCE=.\sqlqryct.h
# End Source File
# Begin Source File

SOURCE=.\sqlqrypg.h
# End Source File
# Begin Source File

SOURCE=.\sqlquery.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\taskxprm.h
# End Source File
# Begin Source File

SOURCE=.\webbrows.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\idr_qep_.ico
# End Source File
# Begin Source File

SOURCE=.\res\qep_c.bmp
# End Source File
# Begin Source File

SOURCE=.\res\qep_d.bmp
# End Source File
# Begin Source File

SOURCE=.\res\qep_db.bmp
# End Source File
# Begin Source File

SOURCE=.\res\qep_n.bmp
# End Source File
# Begin Source File

SOURCE=.\res\qep_prst.ico
# End Source File
# Begin Source File

SOURCE=.\res\qep_tabl.bmp
# End Source File
# Begin Source File

SOURCE=.\res\qpage.bmp
# End Source File
# Begin Source File

SOURCE=.\res\qtuplel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\sqlact_t.bmp
# End Source File
# Begin Source File

SOURCE=.\res\sqlqryct.bmp
# End Source File
# Begin Source File

SOURCE=.\res\sqlquery.ico
# End Source File
# Begin Source File

SOURCE=.\res\table.bmp
# End Source File
# End Group
# End Target
# End Project
