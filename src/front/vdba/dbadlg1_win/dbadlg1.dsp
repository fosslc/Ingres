# Microsoft Developer Studio Project File - Name="Dbadlg1" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Dbadlg1 - Win32 HPUX_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Dbadlg1.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Dbadlg1.mak" CFG="Dbadlg1 - Win32 HPUX_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Dbadlg1 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 EDBC_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 EDBC_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 DBCS Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 DBCS Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 Solaris_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 HPUX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 HPUX_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 Linux_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Dbadlg1 - Win32 AIX_Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/VdbaDomSplitbar/Dbadlg1", EAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Dbadlg1 - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O1 /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\dbadlg1.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\dbadlg1.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Lib\dbadlg1.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\dbadlg1.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 EDBC_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dbadlg1_"
# PROP BASE Intermediate_Dir "Dbadlg1_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "EDBC" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\dbadlg1.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 EDBC_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Dbadlg10"
# PROP BASE Intermediate_Dir "Dbadlg10"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\hdr" /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "EDBC" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\dbadlg1.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 DBCS Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Dbadlg1___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "Dbadlg1___Win32_DBCS_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "_DEBUG" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo /out:"..\Lib\dbadlg1.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\dbadlg1.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 DBCS Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dbadlg1___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "Dbadlg1___Win32_DBCS_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "NDEBUG" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo /out:"..\Lib\dbadlg1.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\dbadlg1.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 Solaris_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dbadlg1___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "Dbadlg1___Win32_Solaris_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dbadlg1___Win32_Solaris_Release"
# PROP Intermediate_Dir "Dbadlg1___Win32_Solaris_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 HPUX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dbadlg1___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "Dbadlg1___Win32_HPUX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dbadlg1___Win32_HPUX_Release"
# PROP Intermediate_Dir "Dbadlg1___Win32_HPUX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 HPUX_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Dbadlg1___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "Dbadlg1___Win32_HPUX_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Dbadlg1___Win32_HPUX_Debug"
# PROP Intermediate_Dir "Dbadlg1___Win32_HPUX_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Z7 /Od /I "..\hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Z7 /Od /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x409
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 Linux_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dbadlg1___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "Dbadlg1___Win32_Linux_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dbadlg1___Win32_Linux_Release"
# PROP Intermediate_Dir "Dbadlg1___Win32_Linux_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"

!ELSEIF  "$(CFG)" == "Dbadlg1 - Win32 AIX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dbadlg1___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "Dbadlg1___Win32_AIX_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dbadlg1___Win32_AIX_Release"
# PROP Intermediate_Dir "Dbadlg1___Win32_AIX_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "..\hdr" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"
# ADD LIB32 /nologo /out:"..\Lib\Dbadlg1.lib"

!ENDIF 

# Begin Target

# Name "Dbadlg1 - Win32 Release"
# Name "Dbadlg1 - Win32 Debug"
# Name "Dbadlg1 - Win32 EDBC_Release"
# Name "Dbadlg1 - Win32 EDBC_Debug"
# Name "Dbadlg1 - Win32 DBCS Debug"
# Name "Dbadlg1 - Win32 DBCS Release"
# Name "Dbadlg1 - Win32 Solaris_Release"
# Name "Dbadlg1 - Win32 HPUX_Release"
# Name "Dbadlg1 - Win32 HPUX_Debug"
# Name "Dbadlg1 - Win32 Linux_Release"
# Name "Dbadlg1 - Win32 AIX_Release"
# Begin Group "Source Files"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\ADDREPL.C
# End Source File
# Begin Source File

SOURCE=.\ALTERDB.C
# End Source File
# Begin Source File

SOURCE=.\ALTERGRO.C
# End Source File
# Begin Source File

SOURCE=.\ALTERTBL.C
# End Source File
# Begin Source File

SOURCE=.\AUDITDB.C
# End Source File
# Begin Source File

SOURCE=.\AUDITDBF.C
# End Source File
# Begin Source File

SOURCE=.\AUDITDBT.C
# End Source File
# Begin Source File

SOURCE=.\CDDS.C
# End Source File
# Begin Source File

SOURCE=.\CDDSV11.C
# End Source File
# Begin Source File

SOURCE=.\CHANGEST.C
# End Source File
# Begin Source File

SOURCE=.\CHKPT.C
# End Source File
# Begin Source File

SOURCE=.\CNTUTIL.C
# End Source File
# Begin Source File

SOURCE=.\CONFSVR.C
# End Source File
# Begin Source File

SOURCE=.\COPYDB.C
# End Source File
# Begin Source File

SOURCE=.\CRDBEVEN.C
# End Source File
# Begin Source File

SOURCE=.\CREATGRO.C
# End Source File
# Begin Source File

SOURCE=.\CREATIDX.C
# End Source File
# Begin Source File

SOURCE=.\CREATSYN.C
# End Source File
# Begin Source File

SOURCE=.\CREATTBL.C
# End Source File
# Begin Source File

SOURCE=.\CRINTEGR.C
# End Source File
# Begin Source File

SOURCE=.\CRVIEW.C
# End Source File
# Begin Source File

SOURCE=.\CTCHECK.C
# End Source File
# Begin Source File

SOURCE=.\CTUNIQUE.C
# End Source File
# Begin Source File

SOURCE=.\DISPLAY.C
# End Source File
# Begin Source File

SOURCE=.\FIND.C
# End Source File
# Begin Source File

SOURCE=.\FINDOBJ.C
# End Source File
# Begin Source File

SOURCE=.\GETDATA.C
# End Source File
# Begin Source File

SOURCE=.\GNREFDBE.C
# End Source File
# Begin Source File

SOURCE=.\GNREFPRO.C
# End Source File
# Begin Source File

SOURCE=.\GNREFTAB.C
# End Source File
# Begin Source File

SOURCE=.\GRANTDBE.C
# End Source File
# Begin Source File

SOURCE=.\GRANTPRO.C
# End Source File
# Begin Source File

SOURCE=.\GRANTROL.C
# End Source File
# Begin Source File

SOURCE=.\GRANTTAB.C
# End Source File
# Begin Source File

SOURCE=.\LEXICAL.C
# End Source File
# Begin Source File

SOURCE=.\LOCSEL01.C
# End Source File
# Begin Source File

SOURCE=.\MAIL.C
# End Source File
# Begin Source File

SOURCE=.\MOBILEPA.C
# End Source File
# Begin Source File

SOURCE=.\MODIFSTR.C
# End Source File
# Begin Source File

SOURCE=.\MSGHANDL.C
# End Source File
# Begin Source File

SOURCE=.\OPENFILE.C
# End Source File
# Begin Source File

SOURCE=.\OTMIZEDB.C
# End Source File
# Begin Source File

SOURCE=.\PROCEDUR.C
# End Source File
# Begin Source File

SOURCE=.\PROPAGAT.C
# End Source File
# Begin Source File

SOURCE=.\RECONCIL.C
# End Source File
# Begin Source File

SOURCE=.\REFER.C
# End Source File
# Begin Source File

SOURCE=.\REFSECUR.C
# End Source File
# Begin Source File

SOURCE=.\RELOCATE.C
# End Source File
# Begin Source File

SOURCE=.\REPLSERV.C
# End Source File
# Begin Source File

SOURCE=.\ROLLFREL.C
# End Source File
# Begin Source File

SOURCE=.\ROLLFWD.C
# End Source File
# Begin Source File

SOURCE=.\RVKDB.C
# End Source File
# Begin Source File

SOURCE=.\RVKGDB.C
# End Source File
# Begin Source File

SOURCE=.\RVKGDBE.C
# End Source File
# Begin Source File

SOURCE=.\RVKGPROC.C
# End Source File
# Begin Source File

SOURCE=.\RVKGROL.C
# End Source File
# Begin Source File

SOURCE=.\RVKGTAB.C
# End Source File
# Begin Source File

SOURCE=.\RVKGVIEW.C
# End Source File
# Begin Source File

SOURCE=.\RVKTABLE.C
# End Source File
# Begin Source File

SOURCE=.\SALARM01.C
# End Source File
# Begin Source File

SOURCE=.\SALARM02.C
# End Source File
# Begin Source File

SOURCE=.\SAVEFILE.C
# End Source File
# Begin Source File

SOURCE=.\SESSION.C
# End Source File
# Begin Source File

SOURCE=.\SPCALC.C
# End Source File
# Begin Source File

SOURCE=.\SPCFYCOL.C
# End Source File
# Begin Source File

SOURCE=.\STATDUMP.C
# End Source File
# Begin Source File

SOURCE=.\SUBEDIT.C
# End Source File
# Begin Source File

SOURCE=.\SVRDIS.C
# End Source File
# Begin Source File

SOURCE=.\SYNOBJ.C
# End Source File
# Begin Source File

SOURCE=.\SYSMOD.C
# End Source File
# Begin Source File

SOURCE=.\TBLSTR.C
# End Source File
# Begin Source File

SOURCE=.\UNLOADDB.C
# End Source File
# Begin Source File

SOURCE=.\USR2GRP.C
# End Source File
# Begin Source File

SOURCE=.\UTIL.C
# End Source File
# Begin Source File

SOURCE=.\VERIFDB.C
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\CNTUTIL.H
# End Source File
# End Group
# End Target
# End Project
