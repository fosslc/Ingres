# Microsoft Developer Studio Project File - Name="ctrltool" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ctrltool - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ctrltool.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ctrltool.mak" CFG="ctrltool - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ctrltool - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 Solaris_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 HPUX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 HPUX_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 Linux_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 Debug64" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 Release64" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 AIX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 DebugU" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 ReleaseU" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 EDBC_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ctrltool - Win32 EDBC_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Libwctrl", STHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libwctrl.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libwctrl.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ctrltool___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "ctrltool___Win32_Solaris_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ctrltool___Win32_Solaris_Release"
# PROP Intermediate_Dir "ctrltool___Win32_Solaris_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ctrltool___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "ctrltool___Win32_HPUX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ctrltool___Win32_HPUX_Release"
# PROP Intermediate_Dir "ctrltool___Win32_HPUX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../inc" /I "../inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"ctrltool___Win32_HPUX_Release\libwctrl.lib"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ctrltool___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "ctrltool___Win32_HPUX_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ctrltool___Win32_HPUX_Debug"
# PROP Intermediate_Dir "ctrltool___Win32_HPUX_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../inc" /I "../inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"ctrltool___Win32_HPUX_Debug\libwctrl.lib"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ctrltool___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "ctrltool___Win32_Linux_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ctrltool___Win32_Linux_Release"
# PROP Intermediate_Dir "ctrltool___Win32_Linux_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ctrltool___Win32_Debug64"
# PROP BASE Intermediate_Dir "ctrltool___Win32_Debug64"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ctrltool___Win32_Release64"
# PROP BASE Intermediate_Dir "ctrltool___Win32_Release64"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN64" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ctrltool___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "ctrltool___Win32_AIX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ctrltool___Win32_AIX_Release"
# PROP Intermediate_Dir "ctrltool___Win32_AIX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ctrltool___Win32_DebugU"
# PROP BASE Intermediate_Dir "ctrltool___Win32_DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libwctrl.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ctrltool___Win32_ReleaseU"
# PROP BASE Intermediate_Dir "ctrltool___Win32_ReleaseU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "_UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libwctrl.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ctrltool___Win32_EDBC_Release"
# PROP BASE Intermediate_Dir "ctrltool___Win32_EDBC_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Inc" /I "..\Inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libwctrl.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ctrltool___Win32_EDBC_Debug"
# PROP BASE Intermediate_Dir "ctrltool___Win32_EDBC_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Inc" /I "..\Inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# ADD LIB32 /nologo /out:"..\Lib\libwctrl.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libwctrl.lib %ING_SRC%\front\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ctrltool - Win32 Release"
# Name "ctrltool - Win32 Debug"
# Name "ctrltool - Win32 Solaris_Release"
# Name "ctrltool - Win32 HPUX_Release"
# Name "ctrltool - Win32 HPUX_Debug"
# Name "ctrltool - Win32 Linux_Release"
# Name "ctrltool - Win32 Debug64"
# Name "ctrltool - Win32 Release64"
# Name "ctrltool - Win32 AIX_Release"
# Name "ctrltool - Win32 DebugU"
# Name "ctrltool - Win32 ReleaseU"
# Name "ctrltool - Win32 EDBC_Release"
# Name "ctrltool - Win32 EDBC_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\256bmp.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\barinfo.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\calscbox.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\calsctrl.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\colorind.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\combodlg.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\editlsct.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fminifrm.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\imagbtn.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\layeddlg.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\layspin.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lsctlhug.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\peditctl.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\piedoc.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pieview.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ppgedit.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\psheet.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\psheet.h
# End Source File
# Begin Source File

SOURCE=.\schkmark.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sortlist.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\statfrm.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\statview.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\titlebmp.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\transbmp.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\treestat.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\uchklbox.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\usplittr.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vlegend.cpp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Inc\256bmp.h
# End Source File
# Begin Source File

SOURCE=..\Inc\calscbox.h
# End Source File
# Begin Source File

SOURCE=..\Inc\calsctrl.h
# End Source File
# Begin Source File

SOURCE=..\Inc\colorind.h
# End Source File
# Begin Source File

SOURCE=.\combodlg.h
# End Source File
# Begin Source File

SOURCE=..\Inc\editlsct.h
# End Source File
# Begin Source File

SOURCE=..\Inc\fminifrm.h
# End Source File
# Begin Source File

SOURCE=..\Inc\imagbtn.h
# End Source File
# Begin Source File

SOURCE=..\Inc\imageidx.h
# End Source File
# Begin Source File

SOURCE=.\layeddlg.h
# End Source File
# Begin Source File

SOURCE=.\layspin.h
# End Source File
# Begin Source File

SOURCE=..\Inc\lsctlhug.h
# End Source File
# Begin Source File

SOURCE=..\Inc\peditctl.h
# End Source File
# Begin Source File

SOURCE=.\piedoc.h
# End Source File
# Begin Source File

SOURCE=.\pieview.h
# End Source File
# Begin Source File

SOURCE=..\inc\portto64.h
# End Source File
# Begin Source File

SOURCE=.\ppgedit.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=..\Inc\schkmark.h
# End Source File
# Begin Source File

SOURCE=..\Inc\sortlist.h
# End Source File
# Begin Source File

SOURCE=..\Inc\statfrm.h
# End Source File
# Begin Source File

SOURCE=.\statview.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=..\Inc\titlebmp.h
# End Source File
# Begin Source File

SOURCE=..\Inc\transbmp.h
# End Source File
# Begin Source File

SOURCE=..\Inc\treestat.h
# End Source File
# Begin Source File

SOURCE=..\Inc\uchklbox.h
# End Source File
# Begin Source File

SOURCE=..\Inc\usplittr.h
# End Source File
# Begin Source File

SOURCE=.\vlegend.h
# End Source File
# Begin Source File

SOURCE=..\Inc\wmusrmsg.h
# End Source File
# End Group
# Begin Group "Resource Files (no build)"

# PROP Default_Filter "*.rc;*.bmp;*.ico;*.cur"
# Begin Source File

SOURCE=..\Inc4res\95check.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\alarms.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\astsql.ico
# End Source File
# Begin Source File

SOURCE=..\Inc4res\blocking.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\check4ls.bmp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Inc4res\chknofrm.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\database.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\dbcoordi.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\dbdistri.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\dberror.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\fldclose.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\grantees.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\import1.bmp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Inc4res\index.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\ingobj2.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\ingobj3.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\ingresob.bmp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Inc4res\lock.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\locks.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\node.ico
# End Source File
# Begin Source File

SOURCE=..\Inc4res\nodes.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\open.ico
# End Source File
# Begin Source File

SOURCE=..\Inc4res\orientat.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\page.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\procedur.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\rctools.h

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Inc4res\rctools.rc

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Inc4res\refresh.ico
# End Source File
# Begin Source File

SOURCE=..\Inc4res\save.ico
# End Source File
# Begin Source File

SOURCE=..\Inc4res\schkmark.bmp

!IF  "$(CFG)" == "ctrltool - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Debug64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 Release64"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 DebugU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 ReleaseU"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "ctrltool - Win32 EDBC_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Inc4res\sesblock.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\session.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\stateico.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrice.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svringre.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrjdbc.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrname.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrnet.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrother.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrrecov.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrrmcmd.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\svrstar.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\table.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\transact.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\user.bmp
# End Source File
# Begin Source File

SOURCE=..\Inc4res\view.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Readme.txt
# End Source File
# End Target
# End Project
