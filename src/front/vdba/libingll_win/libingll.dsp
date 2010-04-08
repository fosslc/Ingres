# Microsoft Developer Studio Project File - Name="libingll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libingll - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libingll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libingll.mak" CFG="libingll - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libingll - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 Solaris_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 HPUX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 HPUX_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 Linux_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 AIX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 DebugU" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 ReleaseU" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 EDBC_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libingll - Win32 EDBC_Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/libingll", HNHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libingll - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libingll.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libingll.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libingll - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\libingll.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libingll.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libingll - Win32 Solaris_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libingll___Win32_Solaris_Release0"
# PROP BASE Intermediate_Dir "libingll___Win32_Solaris_Release0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libingll___Win32_Solaris_Release0"
# PROP Intermediate_Dir "libingll___Win32_Solaris_Release0"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libingll.lib"
# ADD LIB32 /nologo /out:"..\lib\libingll.lib"

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libingll___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "libingll___Win32_HPUX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libingll___Win32_HPUX_Release"
# PROP Intermediate_Dir "libingll___Win32_HPUX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libingll.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libingll___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "libingll___Win32_HPUX_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libingll___Win32_HPUX_Debug"
# PROP Intermediate_Dir "libingll___Win32_HPUX_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libingll.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libingll - Win32 Linux_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libingll___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "libingll___Win32_Linux_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libingll___Win32_Linux_Release"
# PROP Intermediate_Dir "libingll___Win32_Linux_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libingll.lib"
# ADD LIB32 /nologo /out:"..\lib\libingll.lib"

!ELSEIF  "$(CFG)" == "libingll - Win32 AIX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libingll___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "libingll___Win32_AIX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libingll___Win32_AIX_Release"
# PROP Intermediate_Dir "libingll___Win32_AIX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libingll.lib"
# ADD LIB32 /nologo /out:"..\lib\libingll.lib"

!ELSEIF  "$(CFG)" == "libingll - Win32 DebugU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libingll___Win32_DebugU"
# PROP BASE Intermediate_Dir "libingll___Win32_DebugU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libingll.lib"
# ADD LIB32 /nologo /out:"..\Lib\libingll.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libingll.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libingll - Win32 ReleaseU"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libingll___Win32_ReleaseU"
# PROP BASE Intermediate_Dir "libingll___Win32_ReleaseU"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libingll.lib"
# ADD LIB32 /nologo /out:"..\Lib\libingll.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libingll.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Debug"

# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libingll.lib"
# ADD LIB32 /nologo /out:"..\Lib\libingll.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libingll.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Release"

# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\libingll.lib"
# ADD LIB32 /nologo /out:"..\Lib\libingll.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libingll.lib %ING_SRC%\front\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libingll - Win32 Release"
# Name "libingll - Win32 Debug"
# Name "libingll - Win32 Solaris_Release"
# Name "libingll - Win32 HPUX_Release"
# Name "libingll - Win32 HPUX_Debug"
# Name "libingll - Win32 Linux_Release"
# Name "libingll - Win32 AIX_Release"
# Name "libingll - Win32 DebugU"
# Name "libingll - Win32 ReleaseU"
# Name "libingll - Win32 EDBC_Debug"
# Name "libingll - Win32 EDBC_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\a2stream.cpp
# End Source File
# Begin Source File

SOURCE=.\axtracks.cpp
# End Source File
# Begin Source File

SOURCE=.\cmdargs.cpp
# End Source File
# Begin Source File

SOURCE=.\compfile.cpp
# End Source File
# Begin Source File

SOURCE=.\datecnvt.cpp
# End Source File
# Begin Source File

SOURCE=.\detaidml.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlalarm.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlbase.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlcolum.cpp
# End Source File
# Begin Source File

SOURCE=.\dmldbase.cpp
# End Source File
# Begin Source File

SOURCE=.\dmldbeve.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlgrant.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlgroup.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlindex.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlinteg.cpp
# End Source File
# Begin Source File

SOURCE=.\dmllocat.cpp
# End Source File
# Begin Source File

SOURCE=.\dmloburp.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlproc.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlprofi.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlrole.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlrule.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlseq.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlsynon.cpp
# End Source File
# Begin Source File

SOURCE=.\dmltable.cpp
# End Source File
# Begin Source File

SOURCE=.\dmluser.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlview.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlvnode.cpp
# End Source File
# Begin Source File

SOURCE=.\environm.cpp
# End Source File
# Begin Source File

SOURCE=.\ingobdml.cpp
# End Source File
# Begin Source File

SOURCE=.\ingrsess.cpp
# End Source File
# Begin Source File

SOURCE=.\lsselres.cpp
# End Source File
# Begin Source File

SOURCE=.\mibobjs.cpp
# End Source File
# Begin Source File

SOURCE=.\nodes.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\qryinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\regsvice.cpp
# End Source File
# Begin Source File

SOURCE=.\sessimgr.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlselec.cpp
# End Source File
# Begin Source File

SOURCE=.\sqltrace.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\synchron.cpp
# End Source File
# Begin Source File

SOURCE=.\tlsfunct.cpp
# End Source File
# Begin Source File

SOURCE=.\usrexcep.cpp
# End Source File
# Begin Source File

SOURCE=.\vnodedml.cpp
# End Source File
# Begin Source File

SOURCE=.\xmlgener.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Inc\a2stream.h
# End Source File
# Begin Source File

SOURCE=..\Inc\axtracks.h
# End Source File
# Begin Source File

SOURCE=..\Inc\cmdargs.h
# End Source File
# Begin Source File

SOURCE=..\Inc\compfile.h
# End Source File
# Begin Source File

SOURCE=..\Inc\constdef.h
# End Source File
# Begin Source File

SOURCE=..\Inc\datecnvt.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlalarm.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlbase.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlcolum.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmldbase.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmldbeve.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmldetai.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlgrant.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlgroup.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlindex.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlinteg.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmllocat.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmloburp.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlproc.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlprofi.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlrole.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlrule.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlseq.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlsynon.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmltable.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmluser.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlview.h
# End Source File
# Begin Source File

SOURCE=..\Inc\dmlvnode.h
# End Source File
# Begin Source File

SOURCE=..\Inc\environm.h
# End Source File
# Begin Source File

SOURCE=..\Inc\ingobdml.h
# End Source File
# Begin Source File

SOURCE=.\Ingrsess.h
# End Source File
# Begin Source File

SOURCE=..\Inc\lsselres.h
# End Source File
# Begin Source File

SOURCE=..\inc\mibobjs.h
# End Source File
# Begin Source File

SOURCE=.\nodes.h
# End Source File
# Begin Source File

SOURCE=..\Inc\qryinfo.h
# End Source File
# Begin Source File

SOURCE=..\Inc\regsvice.h
# End Source File
# Begin Source File

SOURCE=..\Inc\sessimgr.h
# End Source File
# Begin Source File

SOURCE=..\Inc\sqlselec.h
# End Source File
# Begin Source File

SOURCE=..\Inc\sqltrace.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\Inc\synchron.h
# End Source File
# Begin Source File

SOURCE=..\Inc\tlsfunct.h
# End Source File
# Begin Source File

SOURCE=..\Inc\usrexcep.h
# End Source File
# Begin Source File

SOURCE=..\Inc\vnodedml.h
# End Source File
# Begin Source File

SOURCE=..\Inc\xmlgener.h
# End Source File
# End Group
# Begin Group "SQL Files"

# PROP Default_Filter "*.scc"
# Begin Source File

SOURCE=.\detaidml.scc

!IF  "$(CFG)" == "libingll - Win32 Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Solaris_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Linux_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 AIX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 DebugU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 ReleaseU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\detaidml.scc

"detaidml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fdetaidml.inc detaidml.scc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ingobdml.scc

!IF  "$(CFG)" == "libingll - Win32 Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Solaris_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Linux_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 AIX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 DebugU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 ReleaseU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingobdml.scc

"ingobdml.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingobdml.inc ingobdml.scc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ingrsess.scc

!IF  "$(CFG)" == "libingll - Win32 Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Solaris_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Linux_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 AIX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 DebugU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 ReleaseU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\ingrsess.scc

"ingrsess.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fingrsess.inc ingrsess.scc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlselec.scc

!IF  "$(CFG)" == "libingll - Win32 Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Solaris_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 HPUX_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 Linux_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 AIX_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 DebugU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 ReleaseU"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Debug"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "libingll - Win32 EDBC_Release"

# Begin Custom Build - Performing ESQLCC on $(InputPath)
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
