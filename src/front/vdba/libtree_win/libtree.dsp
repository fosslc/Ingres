# Microsoft Developer Studio Project File - Name="libtree" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libtree - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libtree.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libtree.mak" CFG="libtree - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libtree - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 Solaris_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 HPUX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 HPUX_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 Linux_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 AIX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 DebugU" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 ReleaseU" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 EDBC_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtree - Win32 EDBC_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Libtree", SPHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libtree - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libtree.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libtree.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libtree - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libtree.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libtree.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libtree - Win32 Solaris_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libtree___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "libtree___Win32_Solaris_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libtree___Win32_Solaris_Release"
# PROP Intermediate_Dir "libtree___Win32_Solaris_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libtree.lib"
# ADD LIB32 /nologo /out:"..\lib\libtree.lib"

!ELSEIF  "$(CFG)" == "libtree - Win32 HPUX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libtree___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "libtree___Win32_HPUX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libtree___Win32_HPUX_Release"
# PROP Intermediate_Dir "libtree___Win32_HPUX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libtree.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libtree - Win32 HPUX_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libtree___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "libtree___Win32_HPUX_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libtree___Win32_HPUX_Debug"
# PROP Intermediate_Dir "libtree___Win32_HPUX_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libtree.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libtree - Win32 Linux_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libtree___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "libtree___Win32_Linux_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libtree___Win32_Linux_Release"
# PROP Intermediate_Dir "libtree___Win32_Linux_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libtree.lib"
# ADD LIB32 /nologo /out:"..\lib\libtree.lib"

!ELSEIF  "$(CFG)" == "libtree - Win32 AIX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libtree___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "libtree___Win32_AIX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libtree___Win32_AIX_Release"
# PROP Intermediate_Dir "libtree___Win32_AIX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libtree.lib"
# ADD LIB32 /nologo /out:"..\lib\libtree.lib"

!ELSEIF  "$(CFG)" == "libtree - Win32 DebugU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libtree___Win32_DebugU"
# PROP BASE Intermediate_Dir "libtree___Win32_DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libtree.lib"
# ADD LIB32 /nologo /out:"..\Lib\libtree.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libtree.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libtree - Win32 ReleaseU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libtree___Win32_ReleaseU"
# PROP BASE Intermediate_Dir "libtree___Win32_ReleaseU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libtree.lib"
# ADD LIB32 /nologo /out:"..\Lib\libtree.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libtree.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libtree - Win32 EDBC_Release"

# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libtree.lib"
# ADD LIB32 /nologo /out:"..\Lib\libtree.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libtree.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libtree - Win32 EDBC_Debug"

# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libtree.lib"
# ADD LIB32 /nologo /out:"..\Lib\libtree.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libtree.lib %ING_SRC%\front\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libtree - Win32 Release"
# Name "libtree - Win32 Debug"
# Name "libtree - Win32 Solaris_Release"
# Name "libtree - Win32 HPUX_Release"
# Name "libtree - Win32 HPUX_Debug"
# Name "libtree - Win32 Linux_Release"
# Name "libtree - Win32 AIX_Release"
# Name "libtree - Win32 DebugU"
# Name "libtree - Win32 ReleaseU"
# Name "libtree - Win32 EDBC_Release"
# Name "libtree - Win32 EDBC_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\comaccnt.cpp
# End Source File
# Begin Source File

SOURCE=.\comcache.cpp
# End Source File
# Begin Source File

SOURCE=.\comcolum.cpp
# End Source File
# Begin Source File

SOURCE=.\comdbase.cpp
# End Source File
# Begin Source File

SOURCE=.\comdbeve.cpp
# End Source File
# Begin Source File

SOURCE=.\comgroup.cpp
# End Source File
# Begin Source File

SOURCE=.\comindex.cpp
# End Source File
# Begin Source File

SOURCE=.\cominteg.cpp
# End Source File
# Begin Source File

SOURCE=.\comlocat.cpp
# End Source File
# Begin Source File

SOURCE=.\comproc.cpp
# End Source File
# Begin Source File

SOURCE=.\comprofi.cpp
# End Source File
# Begin Source File

SOURCE=.\comrole.cpp
# End Source File
# Begin Source File

SOURCE=.\comrule.cpp
# End Source File
# Begin Source File

SOURCE=.\comseq.cpp
# End Source File
# Begin Source File

SOURCE=.\comsessi.cpp
# End Source File
# Begin Source File

SOURCE=.\comsynon.cpp
# End Source File
# Begin Source File

SOURCE=.\comtable.cpp
# End Source File
# Begin Source File

SOURCE=.\comuser.cpp
# End Source File
# Begin Source File

SOURCE=.\comview.cpp
# End Source File
# Begin Source File

SOURCE=.\comvnode.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\trdtldta.cpp
# End Source File
# Begin Source File

SOURCE=.\trfalarm.cpp
# End Source File
# Begin Source File

SOURCE=.\trfcolum.cpp
# End Source File
# Begin Source File

SOURCE=.\trfdbase.cpp
# End Source File
# Begin Source File

SOURCE=.\trfdbeve.cpp
# End Source File
# Begin Source File

SOURCE=.\trfgrant.cpp
# End Source File
# Begin Source File

SOURCE=.\trfgroup.cpp
# End Source File
# Begin Source File

SOURCE=.\trflindex.cpp
# End Source File
# Begin Source File

SOURCE=.\trflinteg.cpp
# End Source File
# Begin Source File

SOURCE=.\trflocat.cpp
# End Source File
# Begin Source File

SOURCE=.\trfproc.cpp
# End Source File
# Begin Source File

SOURCE=.\trfprofi.cpp
# End Source File
# Begin Source File

SOURCE=.\trfrole.cpp
# End Source File
# Begin Source File

SOURCE=.\trfrule.cpp
# End Source File
# Begin Source File

SOURCE=.\trfseq.cpp
# End Source File
# Begin Source File

SOURCE=.\trfsynon.cpp
# End Source File
# Begin Source File

SOURCE=.\trftable.cpp
# End Source File
# Begin Source File

SOURCE=.\trfuser.cpp
# End Source File
# Begin Source File

SOURCE=.\trfview.cpp
# End Source File
# Begin Source File

SOURCE=.\trfvnode.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Inc\comaccnt.h
# End Source File
# Begin Source File

SOURCE=..\Inc\combase.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comcache.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comcolum.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comdbase.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comdbeve.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comgroup.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comindex.h
# End Source File
# Begin Source File

SOURCE=..\Inc\cominteg.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comllses.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comlocat.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comproc.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comprofi.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comrole.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comrule.h
# End Source File
# Begin Source File

SOURCE=..\inc\comseq.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comsessi.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comsynon.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comtable.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comuser.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comview.h
# End Source File
# Begin Source File

SOURCE=..\Inc\comvnode.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trctldta.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfalarm.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfcolum.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfdbase.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfdbeve.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfgrant.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfgroup.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfindex.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfinteg.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trflocat.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfproc.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfprofi.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfrole.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfrule.h
# End Source File
# Begin Source File

SOURCE=..\inc\trfseq.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfsynon.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trftable.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfuser.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfview.h
# End Source File
# Begin Source File

SOURCE=..\Inc\trfvnode.h
# End Source File
# End Group
# End Target
# End Project
