# Microsoft Developer Studio Project File - Name="libextnc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libextnc - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libextnc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libextnc.mak" CFG="libextnc - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libextnc - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 Solaris_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 HPUX_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 HPUX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 Linux_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 AIX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 DebugU" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 ReleaseU" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 EDBC_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libextnc - Win32 EDBC_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/LibextnC", SAIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libextnc - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libextnc.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libextnc.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libextnc - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libextnc.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libextnc.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libextnc - Win32 Solaris_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libextnc___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "libextnc___Win32_Solaris_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libextnc___Win32_Solaris_Release"
# PROP Intermediate_Dir "libextnc___Win32_Solaris_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libextnc.lib"
# ADD LIB32 /nologo /out:"..\lib\libextnc.lib"

!ELSEIF  "$(CFG)" == "libextnc - Win32 HPUX_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libextnc___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "libextnc___Win32_HPUX_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libextnc___Win32_HPUX_Debug"
# PROP Intermediate_Dir "libextnc___Win32_HPUX_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libextnc.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libextnc - Win32 HPUX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libextnc___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "libextnc___Win32_HPUX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libextnc___Win32_HPUX_Release"
# PROP Intermediate_Dir "libextnc___Win32_HPUX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libextnc.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libextnc - Win32 Linux_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libextnc___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "libextnc___Win32_Linux_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libextnc___Win32_Linux_Release"
# PROP Intermediate_Dir "libextnc___Win32_Linux_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libextnc.lib"
# ADD LIB32 /nologo /out:"..\lib\libextnc.lib"

!ELSEIF  "$(CFG)" == "libextnc - Win32 AIX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libextnc___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "libextnc___Win32_AIX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libextnc___Win32_AIX_Release"
# PROP Intermediate_Dir "libextnc___Win32_AIX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libextnc.lib"
# ADD LIB32 /nologo /out:"..\lib\libextnc.lib"

!ELSEIF  "$(CFG)" == "libextnc - Win32 DebugU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libextnc___Win32_DebugU"
# PROP BASE Intermediate_Dir "libextnc___Win32_DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libextnc___Win32_DebugU"
# PROP Intermediate_Dir "libextnc___Win32_DebugU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libextnc.lib"
# ADD LIB32 /nologo /out:"..\Lib\libextnc.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libextnc.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libextnc - Win32 ReleaseU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libextnc___Win32_ReleaseU"
# PROP BASE Intermediate_Dir "libextnc___Win32_ReleaseU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libextnc___Win32_ReleaseU"
# PROP Intermediate_Dir "libextnc___Win32_ReleaseU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libextnc.lib"
# ADD LIB32 /nologo /out:"..\Lib\libextnc.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libextnc.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libextnc - Win32 EDBC_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libextnc___Win32_EDBC_Release"
# PROP BASE Intermediate_Dir "libextnc___Win32_EDBC_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libextnc.lib"
# ADD LIB32 /nologo /out:"..\Lib\libextnc.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libextnc.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libextnc - Win32 EDBC_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libextnc___Win32_EDBC_Debug"
# PROP BASE Intermediate_Dir "libextnc___Win32_EDBC_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libextnc.lib"
# ADD LIB32 /nologo /out:"..\Lib\libextnc.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libextnc.lib %ING_SRC%\front\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libextnc - Win32 Release"
# Name "libextnc - Win32 Debug"
# Name "libextnc - Win32 Solaris_Release"
# Name "libextnc - Win32 HPUX_Debug"
# Name "libextnc - Win32 HPUX_Release"
# Name "libextnc - Win32 Linux_Release"
# Name "libextnc - Win32 AIX_Release"
# Name "libextnc - Win32 DebugU"
# Name "libextnc - Win32 ReleaseU"
# Name "libextnc - Win32 EDBC_Release"
# Name "libextnc - Win32 EDBC_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\iia.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\hdr\libextnc.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Group
# End Target
# End Project
