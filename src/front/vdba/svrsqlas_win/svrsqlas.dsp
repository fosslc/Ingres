# Microsoft Developer Studio Project File - Name="svrsqlas" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=svrsqlas - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "svrsqlas.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "svrsqlas.mak" CFG="svrsqlas - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "svrsqlas - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svrsqlas - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svrsqlas - Win32 DebugU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svrsqlas - Win32 ReleaseU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svrsqlas - Win32 EDBC_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svrsqlas - Win32 EDBC_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Svrsqlas", FCIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "svrsqlas - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svrsqlas.dll" /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\svrsqlas.dll %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svrsqlas - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\svrsqlas.dll" /pdbtype:sept /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\svrsqlas.dll %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svrsqlas - Win32 DebugU"

# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib libingll.lib ingres.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\svrsqlas.dll" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\svrsqlas.dll" /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\svrsqlas.dll %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svrsqlas - Win32 ReleaseU"

# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib libingll.lib ingres.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svrsqlas.dll" /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svrsqlas.dll" /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\svrsqlas.dll %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svrsqlas - Win32 EDBC_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "svrsqlas___Win32_EDBC_Release"
# PROP BASE Intermediate_Dir "svrsqlas___Win32_EDBC_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /D "EDBC" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib libingll.lib ingres.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svrsqlas.dll" /libpath:"..\lib"
# ADD LINK32 edbc.lib libguids.lib libingll.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\edbsvras.dll" /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbsvras.dll %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svrsqlas - Win32 EDBC_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "svrsqlas___Win32_EDBC_Debug"
# PROP BASE Intermediate_Dir "svrsqlas___Win32_EDBC_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /D "EDBC" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib libingll.lib ingres.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\svrsqlas.dll" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 edbc.lib libguids.lib libingll.lib libwctrl.lib tkanimat.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\edbsvras.dll" /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbsvras.dll %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "svrsqlas - Win32 Release"
# Name "svrsqlas - Win32 Debug"
# Name "svrsqlas - Win32 DebugU"
# Name "svrsqlas - Win32 ReleaseU"
# Name "svrsqlas - Win32 EDBC_Release"
# Name "svrsqlas - Win32 EDBC_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\edlsinsv.cpp
# End Source File
# Begin Source File

SOURCE=.\edlssele.cpp
# End Source File
# Begin Source File

SOURCE=.\edlsupdv.cpp
# End Source File
# Begin Source File

SOURCE=.\frysqlas.cpp
# End Source File
# Begin Source File

SOURCE=.\server.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlassob.cpp
# End Source File
# Begin Source File

SOURCE=.\sqleapar.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlecpar.cpp
# End Source File
# Begin Source File

SOURCE=.\sqleepar.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlefpar.cpp
# End Source File
# Begin Source File

SOURCE=.\sqleipar.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlemain.cpp
# End Source File
# Begin Source File

SOURCE=.\sqleopar.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlepsht.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlkeywd.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwdel1.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwins1.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwins2.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwmain.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwpsh.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwsel1.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwsel2.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwsel3.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwupd1.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwupd2.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlwupd3.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\svrsqlas.cpp
# End Source File
# Begin Source File

SOURCE=.\svrsqlas.def
# End Source File
# Begin Source File

SOURCE=.\svrsqlas.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bthread.h
# End Source File
# Begin Source File

SOURCE=.\edlsinsv.h
# End Source File
# Begin Source File

SOURCE=.\edlssele.h
# End Source File
# Begin Source File

SOURCE=.\edlsupdv.h
# End Source File
# Begin Source File

SOURCE=.\frysqlas.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\server.h
# End Source File
# Begin Source File

SOURCE=..\Inc\sqlasinf.h
# End Source File
# Begin Source File

SOURCE=.\sqlassob.h
# End Source File
# Begin Source File

SOURCE=.\sqleapar.h
# End Source File
# Begin Source File

SOURCE=.\sqlecpar.h
# End Source File
# Begin Source File

SOURCE=.\sqleepar.h
# End Source File
# Begin Source File

SOURCE=.\sqlefpar.h
# End Source File
# Begin Source File

SOURCE=.\sqleipar.h
# End Source File
# Begin Source File

SOURCE=.\sqlemain.h
# End Source File
# Begin Source File

SOURCE=.\sqleopar.h
# End Source File
# Begin Source File

SOURCE=.\sqlepsht.h
# End Source File
# Begin Source File

SOURCE=.\sqlkeywd.h
# End Source File
# Begin Source File

SOURCE=.\sqlwdel1.h
# End Source File
# Begin Source File

SOURCE=.\sqlwins1.h
# End Source File
# Begin Source File

SOURCE=.\sqlwins2.h
# End Source File
# Begin Source File

SOURCE=.\sqlwmain.h
# End Source File
# Begin Source File

SOURCE=.\sqlwpsht.h
# End Source File
# Begin Source File

SOURCE=.\sqlwsel1.h
# End Source File
# Begin Source File

SOURCE=.\sqlwsel2.h
# End Source File
# Begin Source File

SOURCE=.\sqlwsel3.h
# End Source File
# Begin Source File

SOURCE=.\sqlwupd1.h
# End Source File
# Begin Source File

SOURCE=.\sqlwupd2.h
# End Source File
# Begin Source File

SOURCE=.\sqlwupd3.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\svrsqlas.h
# End Source File
# Begin Source File

SOURCE=.\wizadata.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\svrsqlas.rc2
# End Source File
# End Group
# End Target
# End Project
