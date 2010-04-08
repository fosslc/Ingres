# Microsoft Developer Studio Project File - Name="svriia" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=svriia - Win32 DebugU
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "svriia.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "svriia.mak" CFG="svriia - Win32 DebugU"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "svriia - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 Solaris_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 HPUX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 HPUX_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 Linux_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 Linux_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 AIX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 DebugU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "svriia - Win32 ReleaseU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/svriia", QUHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "svriia - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\svriia.dll" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\svriia.dll %II_SYSTEM%\ingres\vdba	copy Release\svriia.lib %ING_SRC%\front\lib	copy Release\svriia.lib ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svriia - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\svriia.dll" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\svriia.dll %II_SYSTEM%\ingres\vdba	copy Debug\svriia.lib %ING_SRC%\front\lib	copy Debug\svriia.lib ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svriia - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "svriia___Win32_Solaris_Release1"
# PROP BASE Intermediate_Dir "svriia___Win32_Solaris_Release1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "svriia___Win32_Solaris_Release1"
# PROP Intermediate_Dir "svriia___Win32_Solaris_Release1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /D "NOCOMPRESS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svriia.dll" /libpath:"..\Lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svriia.dll" /libpath:"..\Lib"

!ELSEIF  "$(CFG)" == "svriia - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "svriia___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "svriia___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "svriia___Win32_HPUX_Release"
# PROP Intermediate_Dir "svriia___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I ".." /I "..\hdr" /I "..\inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svriia.dll" /libpath:"..\Lib"
# ADD LINK32 q.1.lib compat.1.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /libpath:"$(II_SYSTEM)/ingres/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "svriia - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "svriia___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "svriia___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "svriia___Win32_HPUX_Debug"
# PROP Intermediate_Dir "svriia___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I ".." /I "..\hdr" /I "..\inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\svriia.dll" /pdbtype:sept /libpath:"..\Lib"
# ADD LINK32 q.1.lib compat.1.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /pdbtype:sept /libpath:"$(II_SYSTEM)/ingres/lib"

!ELSEIF  "$(CFG)" == "svriia - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "svriia___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "svriia___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "svriia___Win32_Linux_Release"
# PROP Intermediate_Dir "svriia___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /D "int_lnx" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svriia.dll" /libpath:"..\Lib"
# ADD LINK32 ingres.lib dl.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svriia.dll" /libpath:"..\Lib"

!ELSEIF  "$(CFG)" == "svriia - Win32 Linux_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "svriia___Win32_Linux_Debug"
# PROP BASE Intermediate_Dir "svriia___Win32_Linux_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "svriia___Win32_Linux_Debug"
# PROP Intermediate_Dir "svriia___Win32_Linux_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\svriia.dll" /pdbtype:sept /libpath:"..\Lib"
# ADD LINK32 ingres.lib dl.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\svriia.dll" /pdbtype:sept /libpath:"..\Lib"

!ELSEIF  "$(CFG)" == "svriia - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "svriia___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "svriia___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "svriia___Win32_AIX_Release"
# PROP Intermediate_Dir "svriia___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svriia.dll" /libpath:"..\Lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\svriia.dll" /libpath:"..\Lib"

!ELSEIF  "$(CFG)" == "svriia - Win32 DebugU"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "svriia___Win32_DebugU"
# PROP BASE Intermediate_Dir "svriia___Win32_DebugU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\svriia.dll" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\svriia.dll" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\svriia.dll %II_SYSTEM%\ingres\vdba	copy Debug\svriia.lib %ING_SRC%\front\lib	copy Debug\svriia.lib ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "svriia - Win32 ReleaseU"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "svriia___Win32_ReleaseU"
# PROP BASE Intermediate_Dir "svriia___Win32_ReleaseU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I ".." /I "..\Hdr" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\Inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libguids.lib ingres.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\svriia.dll" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libtree.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\svriia.dll" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\svriia.dll %II_SYSTEM%\ingres\vdba	copy Release\svriia.lib %ING_SRC%\front\lib	copy Release\svriia.lib ..\Lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "svriia - Win32 Release"
# Name "svriia - Win32 Debug"
# Name "svriia - Win32 Solaris_Release"
# Name "svriia - Win32 HPUX_Release"
# Name "svriia - Win32 HPUX_Debug"
# Name "svriia - Win32 Linux_Release"
# Name "svriia - Win32 Linux_Debug"
# Name "svriia - Win32 AIX_Release"
# Name "svriia - Win32 DebugU"
# Name "svriia - Win32 ReleaseU"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dmlcsv.cpp

!IF  "$(CFG)" == "svriia - Win32 Release"

!ELSEIF  "$(CFG)" == "svriia - Win32 Debug"

!ELSEIF  "$(CFG)" == "svriia - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "svriia - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "svriia - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "svriia - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "svriia - Win32 Linux_Debug"

# ADD CPP /D "int_lnx"

!ELSEIF  "$(CFG)" == "svriia - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "svriia - Win32 DebugU"

!ELSEIF  "$(CFG)" == "svriia - Win32 ReleaseU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dmldbf.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlexpor.cpp
# End Source File
# Begin Source File

SOURCE=.\eappagc2.cpp
# End Source File
# Begin Source File

SOURCE=.\eappage1.cpp
# End Source File
# Begin Source File

SOURCE=.\eappage2.cpp
# End Source File
# Begin Source File

SOURCE=.\eapsheet.cpp
# End Source File
# Begin Source File

SOURCE=.\eapstep2.cpp
# End Source File
# Begin Source File

SOURCE=.\exportdt.cpp
# End Source File
# Begin Source File

SOURCE=.\fformat1.cpp
# End Source File
# Begin Source File

SOURCE=.\fformat2.cpp
# End Source File
# Begin Source File

SOURCE=.\fformat3.cpp
# End Source File
# Begin Source File

SOURCE=.\formdata.cpp
# End Source File
# Begin Source File

SOURCE=.\fryeiass.cpp
# End Source File
# Begin Source File

SOURCE=.\hdfixedw.cpp
# End Source File
# Begin Source File

SOURCE=.\iappage1.cpp
# End Source File
# Begin Source File

SOURCE=.\iappage2.cpp
# End Source File
# Begin Source File

SOURCE=.\iappage3.cpp
# End Source File
# Begin Source File

SOURCE=.\iappage4.cpp
# End Source File
# Begin Source File

SOURCE=.\iapsheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ieatree.cpp
# End Source File
# Begin Source File

SOURCE=.\impexpob.cpp
# End Source File
# Begin Source File

SOURCE=.\lbfixedw.cpp
# End Source File
# Begin Source File

SOURCE=.\lschrows.cpp
# End Source File
# Begin Source File

SOURCE=.\msgnosol.cpp
# End Source File
# Begin Source File

SOURCE=.\server.cpp
# End Source File
# Begin Source File

SOURCE=.\staruler.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\svriia.cpp
# End Source File
# Begin Source File

SOURCE=.\svriia.def
# End Source File
# Begin Source File

SOURCE=.\svriia.rc
# End Source File
# Begin Source File

SOURCE=.\taskanim.cpp
# End Source File
# Begin Source File

SOURCE=.\wnfixedw.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgdete.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bthread.h
# End Source File
# Begin Source File

SOURCE=.\dmlcsv.h
# End Source File
# Begin Source File

SOURCE=.\dmldbf.h
# End Source File
# Begin Source File

SOURCE=.\dmlexpor.h
# End Source File
# Begin Source File

SOURCE=.\eappagc2.h
# End Source File
# Begin Source File

SOURCE=.\eappage1.h
# End Source File
# Begin Source File

SOURCE=.\eappage2.h
# End Source File
# Begin Source File

SOURCE=.\eapsheet.h
# End Source File
# Begin Source File

SOURCE=.\eapstep2.h
# End Source File
# Begin Source File

SOURCE=.\exportdt.h
# End Source File
# Begin Source File

SOURCE=.\fformat1.h
# End Source File
# Begin Source File

SOURCE=.\fformat2.h
# End Source File
# Begin Source File

SOURCE=.\fformat3.h
# End Source File
# Begin Source File

SOURCE=.\formdata.h
# End Source File
# Begin Source File

SOURCE=.\fryeiass.h
# End Source File
# Begin Source File

SOURCE=.\hdfixedw.h
# End Source File
# Begin Source File

SOURCE=.\iappage1.h
# End Source File
# Begin Source File

SOURCE=.\iappage2.h
# End Source File
# Begin Source File

SOURCE=.\iappage3.h
# End Source File
# Begin Source File

SOURCE=.\iappage4.h
# End Source File
# Begin Source File

SOURCE=.\iapsheet.h
# End Source File
# Begin Source File

SOURCE=.\ieatree.h
# End Source File
# Begin Source File

SOURCE=..\Inc\impexpas.h
# End Source File
# Begin Source File

SOURCE=.\impexpob.h
# End Source File
# Begin Source File

SOURCE=.\lbfixedw.h
# End Source File
# Begin Source File

SOURCE=.\lschrows.h
# End Source File
# Begin Source File

SOURCE=.\msgnosol.h
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

SOURCE=.\staruler.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\svriia.h
# End Source File
# Begin Source File

SOURCE=.\taskanim.h
# End Source File
# Begin Source File

SOURCE=.\wnfixedw.h
# End Source File
# Begin Source File

SOURCE=.\xdlgdete.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\load.ico
# End Source File
# Begin Source File

SOURCE=.\res\save.ico
# End Source File
# Begin Source File

SOURCE=.\res\sqlassis.ico
# End Source File
# Begin Source File

SOURCE=.\res\svriia.rc2
# End Source File
# End Group
# Begin Group "Doc"

# PROP Default_Filter "*.txt;*.doc"
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Group
# End Target
# End Project
