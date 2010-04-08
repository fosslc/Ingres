# Microsoft Developer Studio Project File - Name="TkAnimat" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TkAnimat - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tkanimat.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tkanimat.mak" CFG="TkAnimat - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TkAnimat - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 Solaris_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 HPUX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 HPUX_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 Linux_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 AIX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 DebugU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 ReleaseU" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 EDBC_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TkAnimat - Win32 EDBC_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/TkAnimat", FXHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TkAnimat - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# Begin Special Build Tool
OutDir=.\Release
TargetName=tkanimat
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\tkanimat.dll %II_SYSTEM%\ingres\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tkanimat.dll" /pdbtype:sept
# Begin Special Build Tool
OutDir=.\Debug
TargetName=tkanimat
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\tkanimat.dll %II_SYSTEM%\ingres\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TkAnimat___Win32_Solaris_Release0"
# PROP BASE Intermediate_Dir "TkAnimat___Win32_Solaris_Release0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "TkAnimat___Win32_Solaris_Release0"
# PROP Intermediate_Dir "TkAnimat___Win32_Solaris_Release0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TkAnimat___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "TkAnimat___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "TkAnimat___Win32_HPUX_Release"
# PROP Intermediate_Dir "TkAnimat___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"winspool.lib odbccp32"

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TkAnimat___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "TkAnimat___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "TkAnimat___Win32_HPUX_Debug"
# PROP Intermediate_Dir "TkAnimat___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tkanimat.dll" /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /pdbtype:sept

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TkAnimat___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "TkAnimat___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "TkAnimat___Win32_Linux_Release"
# PROP Intermediate_Dir "TkAnimat___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# Begin Special Build Tool
OutDir=.\TkAnimat___Win32_Linux_Release
ProjDir=.
TargetName=tkanimat
SOURCE="$(InputPath)"
PostBuild_Desc=Copy Library file ...
PostBuild_Cmds=copy $(OutDir)\$(TargetName).lib ..\$(ProjDir)\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TkAnimat___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "TkAnimat___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "TkAnimat___Win32_AIX_Release"
# PROP Intermediate_Dir "TkAnimat___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 DebugU"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TkAnimat___Win32_DebugU"
# PROP BASE Intermediate_Dir "TkAnimat___Win32_DebugU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tkanimat.dll" /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tkanimat.dll" /pdbtype:sept
# Begin Special Build Tool
OutDir=.\DebugU
TargetName=tkanimat
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\tkanimat.dll %II_SYSTEM%\ingres\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 ReleaseU"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TkAnimat___Win32_ReleaseU"
# PROP BASE Intermediate_Dir "TkAnimat___Win32_ReleaseU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# Begin Special Build Tool
OutDir=.\ReleaseU
TargetName=tkanimat
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\tkanimat.dll %II_SYSTEM%\ingres\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 EDBC_Debug"

# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tkanimat.dll" /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\tkanimat.dll" /pdbtype:sept
# Begin Special Build Tool
OutDir=.\EDBC_Debug
TargetName=tkanimat
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\tkanimat.dll %EDBC_ROOT%\edbc\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "TkAnimat - Win32 EDBC_Release"

# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_AFXEXT" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# ADD LINK32 /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\tkanimat.dll"
# Begin Special Build Tool
OutDir=.\EDBC_Release
TargetName=tkanimat
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\tkanimat.dll %EDBC_ROOT%\edbc\vdba	copy $(OutDir)\$(TargetName).lib %ING_SRC%\front\lib	copy $(OutDir)\$(TargetName).lib ..\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "TkAnimat - Win32 Release"
# Name "TkAnimat - Win32 Debug"
# Name "TkAnimat - Win32 Solaris_Release"
# Name "TkAnimat - Win32 HPUX_Release"
# Name "TkAnimat - Win32 HPUX_Debug"
# Name "TkAnimat - Win32 Linux_Release"
# Name "TkAnimat - Win32 AIX_Release"
# Name "TkAnimat - Win32 DebugU"
# Name "TkAnimat - Win32 ReleaseU"
# Name "TkAnimat - Win32 EDBC_Debug"
# Name "TkAnimat - Win32 EDBC_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\tkanimat.cpp
# End Source File
# Begin Source File

SOURCE=.\tkanimat.def
# End Source File
# Begin Source File

SOURCE=.\tkanimat.rc
# End Source File
# Begin Source File

SOURCE=.\tkwait.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgwait.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\Inc\tkwait.h
# End Source File
# Begin Source File

SOURCE=.\xdlgwait.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\anclock.avi
# End Source File
# Begin Source File

SOURCE=.\res\anconnec.avi
# End Source File
# Begin Source File

SOURCE=.\res\andelete.avi
# End Source File
# Begin Source File

SOURCE=.\res\andiscon.avi
# End Source File
# Begin Source File

SOURCE=.\res\andragdc.avi
# End Source File
# Begin Source File

SOURCE=.\res\anexamin.avi
# End Source File
# Begin Source File

SOURCE=.\res\anfetchf.avi
# End Source File
# Begin Source File

SOURCE=.\res\anfetchr.avi
# End Source File
# Begin Source File

SOURCE=.\res\anrefres.avi
# End Source File
# Begin Source File

SOURCE=.\res\tkanimat.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
