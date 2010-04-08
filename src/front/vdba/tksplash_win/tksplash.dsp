# Microsoft Developer Studio Project File - Name="tksplash" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=tksplash - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tksplash.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tksplash.mak" CFG="tksplash - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tksplash - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 DebugU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 ReleaseU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 Linux_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 Linux_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 Solaris_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 HPUX_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 HPUX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 Debug64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 Release64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 AIX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 EDBC_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "tksplash - Win32 EDBC_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Tksplash", YJIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tksplash - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tksplash.dll" /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
OutDir=.\Release
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\tksplash.dll %II_SYSTEM%\ingres\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tksplash.dll" /pdbtype:sept /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
OutDir=.\Debug
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\tksplash.dll %II_SYSTEM%\ingres\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 DebugU"

# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\tksplash.dll" /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\DebugU
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 ReleaseU"

# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_UNICODE" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\lib"
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\ReleaseU
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tksplash___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "tksplash___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tksplash___Win32_Linux_Release"
# PROP Intermediate_Dir "tksplash___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"
# ADD LINK32 libwctrl.lib lic98.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
OutDir=.\tksplash___Win32_Linux_Release
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "tksplash___Win32_Linux_Debug"
# PROP BASE Intermediate_Dir "tksplash___Win32_Linux_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tksplash___Win32_Linux_Debug"
# PROP Intermediate_Dir "tksplash___Win32_Linux_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\tksplash.dll" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\tksplash.dll" /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\tksplash___Win32_Linux_Debug
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tksplash___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "tksplash___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tksplash___Win32_Solaris_Release"
# PROP Intermediate_Dir "tksplash___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"
# ADD LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"odbc32.lib" /out:"..\app\tksplash.dll" /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\tksplash___Win32_Solaris_Release
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "tksplash___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "tksplash___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "tksplash___Win32_HPUX_Debug"
# PROP Intermediate_Dir "tksplash___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\tksplash.dll" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\tksplash___Win32_HPUX_Debug
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tksplash___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "tksplash___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tksplash___Win32_HPUX_Release"
# PROP Intermediate_Dir "tksplash___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"
# ADD LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\tksplash___Win32_HPUX_Release
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "tksplash___Win32_Debug64"
# PROP BASE Intermediate_Dir "tksplash___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\treasure\nt\ca_lic\98\h" /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\tksplash.dll" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib" /libpath:"$(ING_SRC)\treasure\nt\ca_lic\98\i64_win\utils" /machine:IA64
# Begin Special Build Tool
OutDir=.\Debug64
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 Release64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tksplash___Win32_Release64"
# PROP BASE Intermediate_Dir "tksplash___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\treasure\nt\ca_lic\98\h" /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"
# ADD LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libc.lib" /out:"..\app\tksplash.dll" /libpath:"..\lib" /libpath:"$(ING_SRC)\treasure\nt\ca_lic\98\i64_win\utils" /machine:IA64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
OutDir=.\Release64
ProjDir=.
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copy library file...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tksplash___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "tksplash___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "tksplash___Win32_AIX_Release"
# PROP Intermediate_Dir "tksplash___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"
# ADD LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\app\tksplash.dll" /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "tksplash___Win32_EDBC_Debug"
# PROP BASE Intermediate_Dir "tksplash___Win32_EDBC_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tksplash.dll" /pdbtype:sept /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tksplash.dll" /pdbtype:sept /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
OutDir=.\EDBC_Debug
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT..
PostBuild_Cmds=copy ..\App\tksplash.dll %EDBC_ROOT%\edbc\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "tksplash___Win32_EDBC_Release"
# PROP BASE Intermediate_Dir "tksplash___Win32_EDBC_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\cl\clf\ci" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tksplash.dll" /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 lic98mtdll.lib libwctrl.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tksplash.dll" /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
OutDir=.\EDBC_Release
TargetName=tksplash
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\tksplash.dll %EDBC_ROOT%\edbc\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "tksplash - Win32 Release"
# Name "tksplash - Win32 Debug"
# Name "tksplash - Win32 DebugU"
# Name "tksplash - Win32 ReleaseU"
# Name "tksplash - Win32 Linux_Release"
# Name "tksplash - Win32 Linux_Debug"
# Name "tksplash - Win32 Solaris_Release"
# Name "tksplash - Win32 HPUX_Debug"
# Name "tksplash - Win32 HPUX_Release"
# Name "tksplash - Win32 Debug64"
# Name "tksplash - Win32 Release64"
# Name "tksplash - Win32 AIX_Release"
# Name "tksplash - Win32 EDBC_Debug"
# Name "tksplash - Win32 EDBC_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\stdafx.cpp

!IF  "$(CFG)" == "tksplash - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 DebugU"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 ReleaseU"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Solaris_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Release64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 AIX_Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tksplash.cpp

!IF  "$(CFG)" == "tksplash - Win32 Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 DebugU"

!ELSEIF  "$(CFG)" == "tksplash - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug64"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Release64"

!ELSEIF  "$(CFG)" == "tksplash - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tksplash.def
# End Source File
# Begin Source File

SOURCE=.\tksplash.rc
# End Source File
# Begin Source File

SOURCE=.\ustatcpr.cpp

!IF  "$(CFG)" == "tksplash - Win32 Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 DebugU"

!ELSEIF  "$(CFG)" == "tksplash - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug64"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Release64"

!ELSEIF  "$(CFG)" == "tksplash - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xdgabout.cpp

!IF  "$(CFG)" == "tksplash - Win32 Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 DebugU"

!ELSEIF  "$(CFG)" == "tksplash - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Debug64"

!ELSEIF  "$(CFG)" == "tksplash - Win32 Release64"

!ELSEIF  "$(CFG)" == "tksplash - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Debug"

!ELSEIF  "$(CFG)" == "tksplash - Win32 EDBC_Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\tksplash.h
# End Source File
# Begin Source File

SOURCE=.\ustatcpr.h
# End Source File
# Begin Source File

SOURCE=.\xdgabout.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\advanced.bmp
# End Source File
# Begin Source File

SOURCE=.\res\curhand.cur
# End Source File
# Begin Source File

SOURCE=.\res\tksplash.rc2
# End Source File
# End Group
# End Target
# End Project
