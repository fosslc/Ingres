# Microsoft Developer Studio Project File - Name="MainLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=MainLib - Win32 HPUX_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MainLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MainLib.mak" CFG="MainLib - Win32 HPUX_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MainLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 EDBC_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 EDBC_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 DBCS Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 DBCS Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 Solaris_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 HPUX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 HPUX_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 Linux_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "MainLib - Win32 AIX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/VdbaDomSplitbar/MainLib", DFAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MainLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\mainlib.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\mainlib.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "_DEBUG" /D "NOCOMPRESS" /D "DEBUGMALLOC" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\mainlib.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\mainlib.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MainLib_"
# PROP BASE Intermediate_Dir "MainLib_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "MONITOR" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /I "..\Inc" /D "NDEBUG" /D "MONITOR" /D "EDBC" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\mainlib.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MainLib0"
# PROP BASE Intermediate_Dir "MainLib0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\Hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "NOCOMPRESS" /D "DEBUGMALLOC" /D "MONITOR" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\Hdr" /I "..\Inc" /D "_DEBUG" /D "NOCOMPRESS" /D "DEBUGMALLOC" /D "MONITOR" /D "EDBC" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\mainlib.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MainLib___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "MainLib___Win32_DBCS_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\Hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "NOCOMPRESS" /D "DEBUGMALLOC" /D "MONITOR" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "_DEBUG" /D "NOCOMPRESS" /D "DEBUGMALLOC" /D "MONITOR" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo /out:"..\Lib\mainlib.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\mainlib.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MainLib___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "MainLib___Win32_DBCS_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "MONITOR" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "NDEBUG" /D "MONITOR" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo /out:"..\Lib\mainlib.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\mainlib.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MainLib___Win32_Solaris_Release1"
# PROP BASE Intermediate_Dir "MainLib___Win32_Solaris_Release1"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MainLib___Win32_Solaris_Release1"
# PROP Intermediate_Dir "MainLib___Win32_Solaris_Release1"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Hdr" /I "..\Inc" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /D "NOCOMPRESS" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo /out:"..\Lib\Mainlib.lib"

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MainLib___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "MainLib___Win32_HPUX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MainLib___Win32_HPUX_Release"
# PROP Intermediate_Dir "MainLib___Win32_HPUX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /I "..\Inc" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /D "NOCOMPRESS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "MainLib___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "MainLib___Win32_HPUX_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "MainLib___Win32_HPUX_Debug"
# PROP Intermediate_Dir "MainLib___Win32_HPUX_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\Hdr" /I "..\Inc" /D "_DEBUG" /D "NOCOMPRESS" /D "DEBUGMALLOC" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "_DEBUG" /D "NOCOMPRESS" /D "DEBUGMALLOC" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MainLib___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "MainLib___Win32_Linux_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MainLib___Win32_Linux_Release"
# PROP Intermediate_Dir "MainLib___Win32_Linux_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /I "..\Inc" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "..\inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /D "NOCOMPRESS" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo /out:"..\Lib\Mainlib.lib"

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MainLib___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "MainLib___Win32_AIX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MainLib___Win32_AIX_Release"
# PROP Intermediate_Dir "MainLib___Win32_AIX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /I "..\Inc" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /I "..\Inc" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\ice\hdr" /D "NDEBUG" /D "MONITOR" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /D "NOCOMPRESS" /YX /FD /I /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Mainlib.lib"
# ADD LIB32 /nologo /out:"..\Lib\Mainlib.lib"

!ENDIF 

# Begin Target

# Name "MainLib - Win32 Release"
# Name "MainLib - Win32 Debug"
# Name "MainLib - Win32 EDBC_Release"
# Name "MainLib - Win32 EDBC_Debug"
# Name "MainLib - Win32 DBCS Debug"
# Name "MainLib - Win32 DBCS Release"
# Name "MainLib - Win32 Solaris_Release"
# Name "MainLib - Win32 HPUX_Release"
# Name "MainLib - Win32 HPUX_Debug"
# Name "MainLib - Win32 Linux_Release"
# Name "MainLib - Win32 AIX_Release"
# Begin Group "Sql Files"

# PROP Default_Filter "*.sc"
# Begin Source File

SOURCE=.\dbaginfo.sc

!IF  "$(CFG)" == "MainLib - Win32 Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbaginfo

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dbamvcfg.sc

!IF  "$(CFG)" == "MainLib - Win32 Release"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbamvcfg

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

# Begin Custom Build
InputPath=.\dbamvcfg.sc

"dbamvcfg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbamvcfg

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rmcmdcli.sc

!IF  "$(CFG)" == "MainLib - Win32 Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi rmcmdcli

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqltls.sc

!IF  "$(CFG)" == "MainLib - Win32 Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi sqltls

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\CDDSLOAD.C
# End Source File
# Begin Source File

SOURCE=.\COMPRESS.C

!IF  "$(CFG)" == "MainLib - Win32 Release"

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DBAGDET.C
# End Source File
# Begin Source File

SOURCE=.\DBAGINFO.C
# End Source File
# Begin Source File

SOURCE=.\DBAMISC.C
# End Source File
# Begin Source File

SOURCE=.\DBAMVCFG.C
# End Source File
# Begin Source File

SOURCE=.\DBAPARSE.C
# End Source File
# Begin Source File

SOURCE=.\DBASQLPR.C
# End Source File
# Begin Source File

SOURCE=.\DBASQLV2.C
# End Source File
# Begin Source File

SOURCE=.\DBATIME.C
# End Source File
# Begin Source File

SOURCE=.\DBATLS.C
# End Source File
# Begin Source File

SOURCE=.\DBAUPD.C
# End Source File
# Begin Source File

SOURCE=.\DOM.C
# End Source File
# Begin Source File

SOURCE=.\DOM2.C
# End Source File
# Begin Source File

SOURCE=.\DOMDBKRF.C
# End Source File
# Begin Source File

SOURCE=.\DOMDFILE.C
# End Source File
# Begin Source File

SOURCE=.\DOMDGETD.C
# End Source File
# Begin Source File

SOURCE=.\DOMDISP.C
# End Source File
# Begin Source File

SOURCE=.\DOMDMISC.C
# End Source File
# Begin Source File

SOURCE=.\DOMDTLS.C
# End Source File
# Begin Source File

SOURCE=.\DOMDUPDT.C
# End Source File
# Begin Source File

SOURCE=.\DOMSPLIT.C
# End Source File
# Begin Source File

SOURCE=.\ESLTOOLS.C
# End Source File
# Begin Source File

SOURCE=.\ICEDATA.C
# End Source File
# Begin Source File

SOURCE=.\INGINIT.CH
# End Source File
# Begin Source File

SOURCE=.\MAIN.C
# End Source File
# Begin Source File

SOURCE=.\NODES.C
# End Source File
# Begin Source File

SOURCE=.\PRINT.C
# End Source File
# Begin Source File

SOURCE=.\RMCMDCLI.C
# End Source File
# Begin Source File

SOURCE=.\SETTINGS.C
# End Source File
# Begin Source File

SOURCE=.\SQLTLS.C
# End Source File
# Begin Source File

SOURCE=.\STATBAR.C
# End Source File
# Begin Source File

SOURCE=.\TOOLS.C
# End Source File
# Begin Source File

SOURCE=.\TREE.C
# End Source File
# Begin Source File

SOURCE=.\TREE2.C
# End Source File
# Begin Source File

SOURCE=.\WINUTILS.C
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\CDDSLOAD.H
# End Source File
# Begin Source File

SOURCE=.\DBAPARSE.H
# End Source File
# Begin Source File

SOURCE=.\INGINIT.H
# End Source File
# Begin Source File

SOURCE=.\PRINT.E
# End Source File
# Begin Source File

SOURCE=.\SQLTLS.H
# End Source File
# Begin Source File

SOURCE=.\STATBAR.H
# End Source File
# Begin Source File

SOURCE=.\STDDLG.E
# End Source File
# Begin Source File

SOURCE=.\TYPEDEFS.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ALUS_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\CF_AUT.ICO
# End Source File
# Begin Source File

SOURCE=.\CF_CAL.ICO
# End Source File
# Begin Source File

SOURCE=.\CF_COR.ICO
# End Source File
# Begin Source File

SOURCE=.\CF_PRINT.ICO
# End Source File
# Begin Source File

SOURCE=.\DATAB_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\dbadlg1.rc

!IF  "$(CFG)" == "MainLib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DBEV_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\DBGR_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\DBS_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\GROUP_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\ic____st.ico
# End Source File
# Begin Source File

SOURCE=.\icbu__st.ico
# End Source File
# Begin Source File

SOURCE=.\icbuf_st.ico
# End Source File
# Begin Source File

SOURCE=.\icbul_st.ico
# End Source File
# Begin Source File

SOURCE=.\icbup_st.ico
# End Source File
# Begin Source File

SOURCE=.\icbus_st.ico
# End Source File
# Begin Source File

SOURCE=.\icdbc_st.ico
# End Source File
# Begin Source File

SOURCE=.\icdbr_st.ico
# End Source File
# Begin Source File

SOURCE=.\icdbu_st.ico
# End Source File
# Begin Source File

SOURCE=.\icpr__st.ico
# End Source File
# Begin Source File

SOURCE=.\ics___st.ico
# End Source File
# Begin Source File

SOURCE=.\icse__st.ico
# End Source File
# Begin Source File

SOURCE=.\icsea_st.ico
# End Source File
# Begin Source File

SOURCE=.\icsel_st.ico
# End Source File
# Begin Source File

SOURCE=.\icsev_st.ico
# End Source File
# Begin Source File

SOURCE=.\icwu__st.ico
# End Source File
# Begin Source File

SOURCE=.\INDEX_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\INTEG_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\LOCAT_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\MAIN.ICO
# End Source File
# Begin Source File

SOURCE=.\MAIN.RC

!IF  "$(CFG)" == "MainLib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 EDBC_Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 DBCS Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 Solaris_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 HPUX_Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 Linux_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "MainLib - Win32 AIX_Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\MDI_DOM.ICO
# End Source File
# Begin Source File

SOURCE=.\PROC_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\PROF_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\RE_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\RECD_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\RECDD_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\RECO_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\REMA_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\ROLE_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\RULE_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\SCHEM_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\SEC_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\seque_st.ico
# End Source File
# Begin Source File

SOURCE=.\STARCDB.ICO
# End Source File
# Begin Source File

SOURCE=.\STARDDB.ICO
# End Source File
# Begin Source File

SOURCE=.\staridx.ico
# End Source File
# Begin Source File

SOURCE=.\starproc.ico
# End Source File
# Begin Source File

SOURCE=.\STARTBLN.ICO
# End Source File
# Begin Source File

SOURCE=.\STARTBNA.ICO
# End Source File
# Begin Source File

SOURCE=.\STARVILN.ICO
# End Source File
# Begin Source File

SOURCE=.\STARVINA.ICO
# End Source File
# Begin Source File

SOURCE=.\SYN_RST.ICO
# End Source File
# Begin Source File

SOURCE=.\SYNON_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\TABLE_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\UG_RST.ICO
# End Source File
# Begin Source File

SOURCE=.\UGR_RST.ICO
# End Source File
# Begin Source File

SOURCE=.\USER_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\VIEW_ST.ICO
# End Source File
# Begin Source File

SOURCE=.\VIEWT_ST.ICO
# End Source File
# End Group
# End Target
# End Project
