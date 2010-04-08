# Microsoft Developer Studio Project File - Name="Vdba" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Vdba - Win32 Linux_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Vdba.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Vdba.mak" CFG="Vdba - Win32 Linux_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Vdba - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 EDBC_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 EDBC_Release" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 DBCS Debug" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 DBCS Release" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 HPUX_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 HPUX_Release" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 Solaris_Release" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 Solaris_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 Linux_Release" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 Linux_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Vdba - Win32 AIX_Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/VdbaDomSplitbar/Mainmfc", CAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Vdba - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\inc4res" /I "..\inc" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingrswin.lib ingres.lib oiiceapi.lib lic98mtdll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe" /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vdba.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Vdba - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc4res" /I "..\inc" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingrswin.lib ingres.lib oiiceapi.lib lic98mtdll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib mfc42d.lib" /out:"..\App\vdba.exe" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vdba.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Vdba___W"
# PROP BASE Intermediate_Dir "Vdba___W"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\Hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "_AFXDLL" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc4res" /I "..\inc" /I "..\Hdr" /D "_DEBUG" /D "_AFXDLL" /D "EDBC" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL" /d "EDBC"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\vdba.exe" /pdbtype:sept
# ADD LINK32 ingrswin.lib edbc.lib ediceapi.lib lic98mtdll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\edbcvdba.exe" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbcvdba.exe %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Vdba___0"
# PROP BASE Intermediate_Dir "Vdba___0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "_AFXDLL" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\inc4res" /I "..\inc" /I "..\Hdr" /D "NDEBUG" /D "_AFXDLL" /D "EDBC" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL" /d "EDBC"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"
# ADD LINK32 ingrswin.lib edbc.lib ediceapi.lib lic98mtdll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\edbcvdba.exe"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbcvdba.exe %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Vdba___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "Vdba___Win32_DBCS_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Hdr" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "_AFXDLL" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /D "_DEBUG" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"..\App\vdba.exe" /pdbtype:sept
# ADD LINK32 ingrswin.lib ingres.lib oiiceapi.lib lic98mtdll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"..\App\vdba.exe" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vdba.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Vdba___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "Vdba___Win32_DBCS_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\Hdr" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "_AFXDLL" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\inc" /I "..\inc4res" /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /D "NDEBUG" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"
# ADD LINK32 ingrswin.lib ingres.lib oiiceapi.lib lic98mtdll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe" /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\cl\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vdba.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Vdba___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "Vdba___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Vdba___Win32_HPUX_Debug"
# PROP Intermediate_Dir "Vdba___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Hdr" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /I "..\inc" /I "..\inc4res" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /D "NOCOMPRESS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"..\App\vdba.exe" /pdbtype:sept
# ADD LINK32 oiicecapi.lib iceclient.lib ddf.lib q.1.lib compat.1.lib lic98.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib winspool.lib odbccp32" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib"

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Vdba___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "Vdba___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Vdba___Win32_HPUX_Release"
# PROP Intermediate_Dir "Vdba___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /I "..\inc" /I "..\inc4res" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /D "NOCOMPRESS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"
# ADD LINK32 oiicecapi.lib iceclient.lib ddf.lib q.1.lib compat.1.lib lic98.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /libpath:"$(II_SYSTEM)\ingres\lib"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Vdba___Win32_Solaris_Release1"
# PROP BASE Intermediate_Dir "Vdba___Win32_Solaris_Release1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Vdba___Win32_Solaris_Release1"
# PROP Intermediate_Dir "Vdba___Win32_Solaris_Release1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /I "..\inc" /I "..\inc4res" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\front\vdba\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"
# ADD LINK32 ingres.lib oiicecapi.lib lic98.lib compat.lib config.lib util.lib elf.lib nsl.lib dl.lib socket.lib posix4.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Vdba___Win32_Solaris_Debug"
# PROP BASE Intermediate_Dir "Vdba___Win32_Solaris_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Vdba___Win32_Solaris_Debug"
# PROP Intermediate_Dir "Vdba___Win32_Solaris_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Hdr" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "..\inc" /I "..\inc4res" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\front\vdba\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"..\App\vdba.exe" /pdbtype:sept
# ADD LINK32 ingres.lib oiicecapi.lib lic98.lib compat.lib config.lib util.lib elf.lib nsl.lib dl.lib socket.lib posix4.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"..\App\vdba.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Vdba___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "Vdba___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Vdba___Win32_Linux_Release"
# PROP Intermediate_Dir "Vdba___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /I "..\inc4res" /I "..\inc" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\containr" /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"
# ADD LINK32 ingres.lib oiicecapi.lib lic98.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Vdba___Win32_Linux_Debug"
# PROP BASE Intermediate_Dir "Vdba___Win32_Linux_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Vdba___Win32_Linux_Debug"
# PROP Intermediate_Dir "Vdba___Win32_Linux_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Hdr" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /I "..\inc4res" /I "..\inc" /D "_DEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINLIB" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib lic98mtdll.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"..\App\vdba.exe" /pdbtype:sept
# ADD LINK32 ingres.lib oiiceapi.lib lic98mtdll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrt.lib" /out:"..\App\vdba.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Vdba - Win32 AIX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Vdba___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "Vdba___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Vdba___Win32_AIX_Release"
# PROP Intermediate_Dir "Vdba___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\Hdr" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\hdr" /I "$(ING_SRC)\front\vdba\containr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(ING_SRC)\cl\clf\ci" /I "..\inc4res" /I "..\inc" /D "NDEBUG" /D "_AFXDLL" /D "WIN32" /D "_WINDOWS" /D "MAINLIB" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /YX"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINLIB" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib oiiceapi.lib lic98mtdll.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"
# ADD LINK32 ingres.lib oiicecapi.lib lic98.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdba.exe"

!ENDIF 

# Begin Target

# Name "Vdba - Win32 Release"
# Name "Vdba - Win32 Debug"
# Name "Vdba - Win32 EDBC_Debug"
# Name "Vdba - Win32 EDBC_Release"
# Name "Vdba - Win32 DBCS Debug"
# Name "Vdba - Win32 DBCS Release"
# Name "Vdba - Win32 HPUX_Debug"
# Name "Vdba - Win32 HPUX_Release"
# Name "Vdba - Win32 Solaris_Release"
# Name "Vdba - Win32 Solaris_Debug"
# Name "Vdba - Win32 Linux_Release"
# Name "Vdba - Win32 Linux_Debug"
# Name "Vdba - Win32 AIX_Release"
# Begin Group "Source Files"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\a2stream.cpp
# End Source File
# Begin Source File

SOURCE=.\Alarms.cpp
# End Source File
# Begin Source File

SOURCE=.\axipm.cpp
# End Source File
# Begin Source File

SOURCE=.\axsql.cpp
# End Source File
# Begin Source File

SOURCE=.\axsqledb.cpp
# End Source File
# Begin Source File

SOURCE=.\bkgdpref.cpp
# End Source File
# Begin Source File

SOURCE=.\BMPBTN.CPP
# End Source File
# Begin Source File

SOURCE=.\buildsvr.cpp
# End Source File
# Begin Source File

SOURCE=.\CellCtrl.CPP
# End Source File
# Begin Source File

SOURCE=.\CellIntC.cpp
# End Source File
# Begin Source File

SOURCE=.\chartini.cpp
# End Source File
# Begin Source File

SOURCE=.\CHBOXBAR.CPP
# End Source File
# Begin Source File

SOURCE=.\CHILDFRM.CPP
# End Source File
# Begin Source File

SOURCE=.\chkpoint.cpp
# End Source File
# Begin Source File

SOURCE=.\chooseob.cpp
# End Source File
# Begin Source File

SOURCE=.\ckplst.cpp
# End Source File
# Begin Source File

SOURCE=.\cmdline.cpp
# End Source File
# Begin Source File

SOURCE=.\cmdline2.cpp
# End Source File
# Begin Source File

SOURCE=.\CMIXFILE.CPP
# End Source File
# Begin Source File

SOURCE=.\collidta.cpp
# End Source File
# Begin Source File

SOURCE=.\Columns.cpp
# End Source File
# Begin Source File

SOURCE=.\COMBOBAR.CPP
# End Source File
# Begin Source File

SOURCE=.\compfile.cpp
# End Source File
# Begin Source File

SOURCE=.\createdb.cpp

!IF  "$(CFG)" == "Vdba - Win32 Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Debug"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Debug"

# ADD BASE CPP /YX
# ADD CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Debug"

# ADD BASE CPP /YX
# ADD CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Debug"

# ADD BASE CPP /YX
# ADD CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\creatloc.cpp

!IF  "$(CFG)" == "Vdba - Win32 Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Debug"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Debug"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Debug"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Debug"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "Vdba - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\creatloc.h
# End Source File
# Begin Source File

SOURCE=.\DBEDOC.CPP
# End Source File
# Begin Source File

SOURCE=.\DBEFRAME.CPP
# End Source File
# Begin Source File

SOURCE=.\DBEVIEW.CPP
# End Source File
# Begin Source File

SOURCE=.\DgArccln.cpp
# End Source File
# Begin Source File

SOURCE=.\dgbkrefr.cpp
# End Source File
# Begin Source File

SOURCE=.\DGDBEP01.CPP
# End Source File
# Begin Source File

SOURCE=.\DGDBEP02.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDomC02.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpaulo.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpauus.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpCd.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpCdTb.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpCo.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDb.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDbAl.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDbEv.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDbGr.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDbPr.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDbSc.CPP
# End Source File
# Begin Source File

SOURCE=.\dgdpdbse.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDbSy.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDbTb.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDbVi.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDd.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDdPr.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDdTb.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDdVi.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDILS.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDPL.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDPLS.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDtL.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDTLX.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpDVL.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDVLS.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDVN.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpDVNS.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpenle.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpEvGr.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpGrp.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXD.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXE.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXP.CPP
# End Source File
# Begin Source File

SOURCE=.\dgdpgrxs.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXT.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXV.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpIdx.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpihtm.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpInt.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDPLoc.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpLocD.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpLocS.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpLoSu.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpPrc.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpPrcG.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpPro.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpRol.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXD.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXE.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXP.CPP
# End Source File
# Begin Source File

SOURCE=.\dgdproxs.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXT.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXV.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRpCd.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRpCo.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRpMu.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRpRt.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpRul.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpScPr.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpScTb.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpScVi.CPP
# End Source File
# Begin Source File

SOURCE=.\dgdpseq.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpseqg.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpstdb.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpstgd.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpstgr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpstlo.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpstpr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpstro.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpstus.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpTb.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpTbAl.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpTbCo.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpTbGr.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpTbIg.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpTbIx.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpTbLo.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpTbPg.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpTbRu.CPP
# End Source File
# Begin Source File

SOURCE=.\dgdptbst.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpTRNA.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdptrow.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDPUsr.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXA.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXD.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXE.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXP.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXR.CPP
# End Source File
# Begin Source File

SOURCE=.\dgdpusxs.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXT.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXV.CPP
# End Source File
# Begin Source File

SOURCE=.\DgDpVi.cpp
# End Source File
# Begin Source File

SOURCE=.\DgDpViGr.CPP
# End Source File
# Begin Source File

SOURCE=.\dgdpwacd.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwb.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwbro.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwbu.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwbus.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwdbc.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwdbu.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwf.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwfnp.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwfpr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwfpu.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwl.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwlo.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwp.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwpco.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwpr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwpro.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwro.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwsbr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwsbu.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwsco.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwsdu.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwspr.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwsro.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwswu.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwva.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwvap.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwvlo.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwvva.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwwco.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwwro.cpp
# End Source File
# Begin Source File

SOURCE=.\dgdpwwu.cpp
# End Source File
# Begin Source File

SOURCE=.\DgErrDet.cpp
# End Source File
# Begin Source File

SOURCE=.\dgexpire.cpp
# End Source File
# Begin Source File

SOURCE=.\dgstarco.cpp
# End Source File
# Begin Source File

SOURCE=.\dgtbjnl.cpp
# End Source File
# Begin Source File

SOURCE=.\DGVNDATA.CPP
# End Source File
# Begin Source File

SOURCE=.\DGVNLOGI.CPP
# End Source File
# Begin Source File

SOURCE=.\DGVNODE.CPP
# End Source File
# Begin Source File

SOURCE=.\DisCon2.CPP
# End Source File
# Begin Source File

SOURCE=.\DisCon3.CPP
# End Source File
# Begin Source File

SOURCE=.\DISCONN.CPP
# End Source File
# Begin Source File

SOURCE=.\dlgmsbox.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgreque.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgrmans.cpp
# End Source File
# Begin Source File

SOURCE=.\dlgrmcmd.cpp
# End Source File
# Begin Source File

SOURCE=.\DLGVDOC.CPP
# End Source File
# Begin Source File

SOURCE=.\DLGVFRM.CPP
# End Source File
# Begin Source File

SOURCE=.\DMLBase.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlindex.cpp
# End Source File
# Begin Source File

SOURCE=.\dmlurp.cpp

!IF  "$(CFG)" == "Vdba - Win32 Release"

# SUBTRACT CPP /O<none>

!ELSEIF  "$(CFG)" == "Vdba - Win32 Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Release"

# SUBTRACT CPP /O<none>

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Release"

# SUBTRACT CPP /O<none>

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Release"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Release"

# SUBTRACT CPP /O<none>

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Release"

# SUBTRACT CPP /O<none>

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "Vdba - Win32 AIX_Release"

# SUBTRACT CPP /O<none>

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DOCVNODE.CPP
# End Source File
# Begin Source File

SOURCE=.\domfast.cpp
# End Source File
# Begin Source File

SOURCE=.\domilist.CPP
# End Source File
# Begin Source File

SOURCE=.\DomPgInf.cpp
# End Source File
# Begin Source File

SOURCE=.\DomRfsh.cpp
# End Source File
# Begin Source File

SOURCE=.\DomSeri.cpp
# End Source File
# Begin Source File

SOURCE=.\domseri2.cpp
# End Source File
# Begin Source File

SOURCE=.\domseri3.cpp
# End Source File
# Begin Source File

SOURCE=.\dtagsett.cpp
# End Source File
# Begin Source File

SOURCE=.\duplicdb.cpp
# End Source File
# Begin Source File

SOURCE=.\edlduslo.cpp
# End Source File
# Begin Source File

SOURCE=.\edlsdbgr.cpp
# End Source File
# Begin Source File

SOURCE=.\extccall.cpp
# End Source File
# Begin Source File

SOURCE=.\fastload.cpp
# End Source File
# Begin Source File

SOURCE=.\font.cpp
# End Source File
# Begin Source File

SOURCE=.\FRMVNODE.CPP
# End Source File
# Begin Source File

SOURCE=.\Granted.cpp
# End Source File
# Begin Source File

SOURCE=.\Grantees.cpp
# End Source File
# Begin Source File

SOURCE=.\icebunfp.cpp
# End Source File
# Begin Source File

SOURCE=.\icebunro.cpp
# End Source File
# Begin Source File

SOURCE=.\icebunus.cpp
# End Source File
# Begin Source File

SOURCE=.\icecocom.cpp
# End Source File
# Begin Source File

SOURCE=.\icecoedt.cpp
# End Source File
# Begin Source File

SOURCE=.\IceDConn.cpp
# End Source File
# Begin Source File

SOURCE=.\IceDUser.cpp
# End Source File
# Begin Source File

SOURCE=.\icefprus.cpp
# End Source File
# Begin Source File

SOURCE=.\icegrs.cpp
# End Source File
# Begin Source File

SOURCE=.\icelocat.cpp
# End Source File
# Begin Source File

SOURCE=.\icepassw.cpp
# End Source File
# Begin Source File

SOURCE=.\IceProfi.cpp
# End Source File
# Begin Source File

SOURCE=.\icerole.cpp
# End Source File
# Begin Source File

SOURCE=.\icesvrdt.cpp
# End Source File
# Begin Source File

SOURCE=.\IceWebU.cpp
# End Source File
# Begin Source File

SOURCE=.\infodb.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmaxdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmaxfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmaxvie.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmdisp.cpp
# End Source File
# Begin Source File

SOURCE=.\ipmicdta.cpp
# End Source File
# Begin Source File

SOURCE=.\IPMPRDTA.CPP
# End Source File
# Begin Source File

SOURCE=.\localter.cpp
# End Source File
# Begin Source File

SOURCE=.\location.cpp
# End Source File
# Begin Source File

SOURCE=.\LOGINDIC.CPP
# End Source File
# Begin Source File

SOURCE=.\lstbstat.cpp
# End Source File
# Begin Source File

SOURCE=.\MAINDOC.CPP
# End Source File
# Begin Source File

SOURCE=.\MAINFRM.CPP
# End Source File
# Begin Source File

SOURCE=.\MAINMFC.CPP
# End Source File
# Begin Source File

SOURCE=.\MAINVI2.CPP
# End Source File
# Begin Source File

SOURCE=.\MAINVIEW.CPP
# End Source File
# Begin Source File

SOURCE=.\modsize.cpp
# End Source File
# Begin Source File

SOURCE=.\MONITOR.CPP
# End Source File
# Begin Source File

SOURCE=.\MultCol.cpp
# End Source File
# Begin Source File

SOURCE=.\MultFlag.cpp
# End Source File
# Begin Source File

SOURCE=.\nodefast.cpp
# End Source File
# Begin Source File

SOURCE=.\parse.cpp
# End Source File
# Begin Source File

SOURCE=.\prodname.cpp
# End Source File
# Begin Source File

SOURCE=.\property.cpp
# End Source File
# Begin Source File

SOURCE=.\qsplittr.cpp
# End Source File
# Begin Source File

SOURCE=.\qsqldoc.cpp
# End Source File
# Begin Source File

SOURCE=.\qsqlfram.cpp
# End Source File
# Begin Source File

SOURCE=.\qsqlview.cpp
# End Source File
# Begin Source File

SOURCE=.\qstatind.cpp
# End Source File
# Begin Source File

SOURCE=.\RegSvice.cpp
# End Source File
# Begin Source File

SOURCE=.\repadddb.cpp
# End Source File
# Begin Source File

SOURCE=.\Replic.cpp
# End Source File
# Begin Source File

SOURCE=.\replikey.cpp
# End Source File
# Begin Source File

SOURCE=.\rpanedrg.cpp
# End Source File
# Begin Source File

SOURCE=.\SAVELOAD.CPP
# End Source File
# Begin Source File

SOURCE=.\SBDBEBAR.CPP
# End Source File
# Begin Source File

SOURCE=.\SBDLGBAR.CPP
# End Source File
# Begin Source File

SOURCE=.\SBIPMBAR.CPP
# End Source File
# Begin Source File

SOURCE=.\sequence.cpp
# End Source File
# Begin Source File

SOURCE=.\SERIALTR.CPP
# End Source File
# Begin Source File

SOURCE=.\SHUTSRV.CPP
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\SPLITTER.CPP
# End Source File
# Begin Source File

SOURCE=.\sqlkeywd.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlselec.cpp
# End Source File
# Begin Source File

SOURCE=.\STARITEM.CPP
# End Source File
# Begin Source File

SOURCE=.\StarLoTb.cpp
# End Source File
# Begin Source File

SOURCE=.\STARPRLN.CPP
# End Source File
# Begin Source File

SOURCE=.\starsel.cpp
# End Source File
# Begin Source File

SOURCE=.\STARTBLN.CPP
# End Source File
# Begin Source File

SOURCE=.\STARUTIL.CPP
# End Source File
# Begin Source File

SOURCE=.\StCellIt.cpp
# End Source File
# Begin Source File

SOURCE=.\STDAFX.CPP
# End Source File
# Begin Source File

SOURCE=.\StMetalB.cpp
# End Source File
# Begin Source File

SOURCE=.\taskxprm.cpp
# End Source File
# Begin Source File

SOURCE=.\tbstatis.cpp
# End Source File
# Begin Source File

SOURCE=.\tcomment.cpp
# End Source File
# Begin Source File

SOURCE=.\TOOLBARS.CPP
# End Source File
# Begin Source File

SOURCE=.\UPDDISP.CPP
# End Source File
# Begin Source File

SOURCE=.\urpectrl.cpp
# End Source File
# Begin Source File

SOURCE=.\usermod.cpp
# End Source File
# Begin Source File

SOURCE=.\VEWVNODE.CPP
# End Source File
# Begin Source File

SOURCE=.\VNITEM.CPP
# End Source File
# Begin Source File

SOURCE=.\vnitem2.cpp
# End Source File
# Begin Source File

SOURCE=.\vnitem3.cpp
# End Source File
# Begin Source File

SOURCE=.\VTREE.CPP
# End Source File
# Begin Source File

SOURCE=.\VTREE1.CPP
# End Source File
# Begin Source File

SOURCE=.\VTREE2.CPP
# End Source File
# Begin Source File

SOURCE=.\VTREE3.CPP
# End Source File
# Begin Source File

SOURCE=.\VTREECTL.CPP
# End Source File
# Begin Source File

SOURCE=.\VTREEDAT.CPP
# End Source File
# Begin Source File

SOURCE=.\VTREEGLB.CPP
# End Source File
# Begin Source File

SOURCE=.\VWDBEP02.CPP
# End Source File
# Begin Source File

SOURCE=.\webbrows.cpp
# End Source File
# Begin Source File

SOURCE=.\xcopydb.cpp
# End Source File
# Begin Source File

SOURCE=.\xdgidxop.cpp
# End Source File
# Begin Source File

SOURCE=.\xdgvnatr.cpp
# End Source File
# Begin Source File

SOURCE=.\xdgvnpwd.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgacco.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgdbgr.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgdrop.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlggset.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgindx.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgpref.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgprof.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgrole.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgrule.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlguser.cpp
# End Source File
# Begin Source File

SOURCE=.\xdlgwait.cpp
# End Source File
# Begin Source File

SOURCE=.\xinstenl.cpp
# End Source File
# Begin Source File

SOURCE=.\xtbpkey.cpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.bmp; *.ico; *.cur; *.avi"
# Begin Source File

SOURCE=.\res\about.bmp
# End Source File
# Begin Source File

SOURCE=.\res\activate.ico
# End Source File
# Begin Source File

SOURCE=.\res\add.ico
# End Source File
# Begin Source File

SOURCE=.\res\alter.ico
# End Source File
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

SOURCE=.\res\BITMAP1.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BITMAP_D.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00013.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00023.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00024.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00025.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00026.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00027.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00028.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00039.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00040.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00053.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00057.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00058.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00059.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\BMP00064.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\BMP00075.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\BMP00076.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00086.BMP
# End Source File
# Begin Source File

SOURCE=.\res\BMP00087.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\BMP00088.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\BMP00089.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\BMP00090.BMP
# End Source File
# Begin Source File

SOURCE=.\res\btn_up_d.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_up_f.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_up_g.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btn_up_u.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btndownd.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btndownf.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btndowng.bmp
# End Source File
# Begin Source File

SOURCE=.\res\btndownu.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\check.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\CHECKNOF.BMP
# End Source File
# Begin Source File

SOURCE=.\res\close.ico
# End Source File
# Begin Source File

SOURCE=.\res\collapal.ico
# End Source File
# Begin Source File

SOURCE=.\res\collapbr.ico
# End Source File
# Begin Source File

SOURCE=.\res\collisio.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CONNECT_.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\counter1.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter2.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter3.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter4.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter5.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter6.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter7.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter8.ico
# End Source File
# Begin Source File

SOURCE=.\Res\counter9.ico
# End Source File
# Begin Source File

SOURCE=.\Res\countr10.ico
# End Source File
# Begin Source File

SOURCE=.\Res\countr11.ico
# End Source File
# Begin Source File

SOURCE=.\Res\countr12.ico
# End Source File
# Begin Source File

SOURCE=.\Res\countr13.ico
# End Source File
# Begin Source File

SOURCE=.\Res\countr14.ico
# End Source File
# Begin Source File

SOURCE=.\Res\countr15.ico
# End Source File
# Begin Source File

SOURCE=.\Res\countr16.ico
# End Source File
# Begin Source File

SOURCE=.\res\dbevent.ico
# End Source File
# Begin Source File

SOURCE=.\Res\dbobject.bmp
# End Source File
# Begin Source File

SOURCE=.\res\disconn.ico
# End Source File
# Begin Source File

SOURCE=.\res\DOM.BMP
# End Source File
# Begin Source File

SOURCE=.\res\dom.ico
# End Source File
# Begin Source File

SOURCE=.\Res\DOM_NOTO.BMP
# End Source File
# Begin Source File

SOURCE=.\res\drop.ico
# End Source File
# Begin Source File

SOURCE=.\res\expandal.ico
# End Source File
# Begin Source File

SOURCE=.\res\expandbr.ico
# End Source File
# Begin Source File

SOURCE=.\res\gdisplay.ico
# End Source File
# Begin Source File

SOURCE=.\res\ICO00001.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00002.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00003.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00004.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00005.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00006.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00007.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00010.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00011.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\ICO00012.ICO
# End Source File
# Begin Source File

SOURCE=.\res\ICO00013.ICO
# End Source File
# Begin Source File

SOURCE=.\res\ICO00014.ICO
# End Source File
# Begin Source File

SOURCE=.\res\ICON1.ICO
# End Source File
# Begin Source File

SOURCE=.\res\IDR_DBEV.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\IDR_IPMT.ICO
# End Source File
# Begin Source File

SOURCE=.\Res\IMGLIST_.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\imglstb1.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\Ingsplsh.bmp
# End Source File
# Begin Source File

SOURCE=.\res\IPM.BMP
# End Source File
# Begin Source File

SOURCE=.\res\ipm.ico
# End Source File
# Begin Source File

SOURCE=.\res\IPM1.BMP
# End Source File
# Begin Source File

SOURCE=.\res\killsess.ico
# End Source File
# Begin Source File

SOURCE=.\res\MAINDOC.ICO
# End Source File
# Begin Source File

SOURCE=.\res\MAINMFC.ICO
# End Source File
# Begin Source File

SOURCE=.\MAINMFC.RC
# ADD BASE RSC /l 0x40c
# ADD RSC /l 0x40c /i "..\hdr" /i "..\winmain" /i "..\mainmfc\res"
# End Source File
# Begin Source File

SOURCE=.\res\MAINMFC.RC2
# End Source File
# Begin Source File

SOURCE=.\res\NOCONNEC.BMP
# End Source File
# Begin Source File

SOURCE=.\res\NODE.BMP
# End Source File
# Begin Source File

SOURCE=.\res\NODE_ADV.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\NODE_ATT.BMP
# End Source File
# Begin Source File

SOURCE=.\res\NODE_CON.BMP
# End Source File
# Begin Source File

SOURCE=.\res\NODE_LOC.BMP
# End Source File
# Begin Source File

SOURCE=.\res\NODE_LOG.BMP
# End Source File
# Begin Source File

SOURCE=.\res\NODE_NOT.BMP
# End Source File
# Begin Source File

SOURCE=.\res\NODE_OPE.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\NODEUSER.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\pagetyp1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\prefsmal.bmp
# End Source File
# Begin Source File

SOURCE=.\res\reset.ico
# End Source File
# Begin Source File

SOURCE=.\res\scratch.ico
# End Source File
# Begin Source File

SOURCE=.\res\SECALARM.BMP
# End Source File
# Begin Source File

SOURCE=.\res\SPLASH.BMP
# End Source File
# Begin Source File

SOURCE=.\res\sql.ico
# End Source File
# Begin Source File

SOURCE=.\res\sqlact.bmp
# End Source File
# Begin Source File

SOURCE=.\res\SQLACT1.BMP
# End Source File
# Begin Source File

SOURCE=.\res\SQLDOC.ICO
# End Source File
# Begin Source File

SOURCE=.\res\STAR_DAT.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\stardbco.bmp
# End Source File
# Begin Source File

SOURCE=.\Res\SVRCLASS.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TBARMAIN.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TM_CONNE.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TM_DB.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TM_DB1.BMP
# End Source File
# Begin Source File

SOURCE=.\res\TOOLBAR.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\USER.BMP
# End Source File
# Begin Source File

SOURCE=.\Res\USER_OPE.BMP
# End Source File
# Begin Source File

SOURCE=.\res\VNODEMDI.ICO
# End Source File
# End Group
# Begin Group "Library Files"

# PROP Default_Filter "*.lib"
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\CATO3CNT.LIB"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\CATO3SPN.LIB"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\Catolbox.lib"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\Catolist.lib"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\Dbadlg1.lib"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\Mainlib.lib"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\libguids.lib"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\libextnc.lib"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\Ingrswin.lib"
# End Source File
# Begin Source File

SOURCE="$(ING_SRC)\front\lib\libwctrl.lib"
# End Source File
# End Group
# Begin Group "Hdr directory"

# PROP Default_Filter "*.e"
# Begin Source File

SOURCE=..\HDR\ASPEN.H
# End Source File
# Begin Source File

SOURCE=..\HDR\CATOCNTR.H
# End Source File
# Begin Source File

SOURCE=..\HDR\CATOLIST.H
# End Source File
# Begin Source File

SOURCE=..\HDR\CATOSPIN.H
# End Source File
# Begin Source File

SOURCE=..\HDR\CATOVER.H
# End Source File
# Begin Source File

SOURCE=..\HDR\CAVOVER.H
# End Source File
# Begin Source File

SOURCE=..\hdr\COLLISIO.H
# End Source File
# Begin Source File

SOURCE=..\HDR\COMPRESS.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DBA.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DBADLG1.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DBAFILE.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DBAGINFO.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DBASET.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DBATIME.H
# End Source File
# Begin Source File

SOURCE=..\hdr\DgErrH.h
# End Source File
# Begin Source File

SOURCE=..\HDR\DLGRES.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DLL.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DOM.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DOMDATA.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DOMDISP.H
# End Source File
# Begin Source File

SOURCE=..\HDR\DOMDLOCA.H
# End Source File
# Begin Source File

SOURCE=..\HDR\ERROR.H
# End Source File
# Begin Source File

SOURCE=..\HDR\ERROUT.H
# End Source File
# Begin Source File

SOURCE=..\HDR\ESLTOOLS.H
# End Source File
# Begin Source File

SOURCE=..\hdr\extccall.h
# End Source File
# Begin Source File

SOURCE=..\HDR\GETDATA.H
# End Source File
# Begin Source File

SOURCE=..\HDR\GLOBAL.H
# End Source File
# Begin Source File

SOURCE=..\hdr\ICE.H
# End Source File
# Begin Source File

SOURCE=..\HDR\INGNET.H
# End Source File
# Begin Source File

SOURCE=..\HDR\LEXICAL.H
# End Source File
# Begin Source File

SOURCE=..\HDR\MAIN.H
# End Source File
# Begin Source File

SOURCE=..\HDR\MONITOR.H
# End Source File
# Begin Source File

SOURCE=..\HDR\MSGHANDL.H
# End Source File
# Begin Source File

SOURCE=..\HDR\NANACT.E
# End Source File
# Begin Source File

SOURCE=..\HDR\NANMEM.E
# End Source File
# Begin Source File

SOURCE=..\HDR\PORT.H
# End Source File
# Begin Source File

SOURCE=..\Hdr\prodname.h
# End Source File
# Begin Source File

SOURCE=..\Hdr\replutil.h
# End Source File
# Begin Source File

SOURCE=..\hdr\reptbdef.h
# End Source File
# Begin Source File

SOURCE=..\HDR\RESOURCE.H
# End Source File
# Begin Source File

SOURCE=..\HDR\RMCMD.H
# End Source File
# Begin Source File

SOURCE=..\hdr\saveload.h
# End Source File
# Begin Source File

SOURCE=..\HDR\SELWIN.E
# End Source File
# Begin Source File

SOURCE=..\HDR\SETTINGS.H
# End Source File
# Begin Source File

SOURCE=..\HDR\STARUTIL.H
# End Source File
# Begin Source File

SOURCE=..\HDR\STMTWIZ.H
# End Source File
# Begin Source File

SOURCE=..\HDR\SUBEDIT.H
# End Source File
# Begin Source File

SOURCE=..\HDR\TOOLS.H
# End Source File
# Begin Source File

SOURCE=..\HDR\TREELB.E
# End Source File
# Begin Source File

SOURCE=..\HDR\WINUTILS.H
# End Source File
# End Group
# Begin Group "Sql Files"

# PROP Default_Filter "scc"
# Begin Source File

SOURCE=.\sqlselec.scc

!IF  "$(CFG)" == "Vdba - Win32 Release"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Debug"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Debug"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Release"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Debug"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Release"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Release"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Debug"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Release"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Debug"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 AIX_Release"

# Begin Custom Build
InputPath=.\sqlselec.scc

"sqlselec.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fsqlselec.inc sqlselec.scc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\starsel.scc

!IF  "$(CFG)" == "Vdba - Win32 Release"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Debug"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Debug"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 DBCS Release"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Debug"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 HPUX_Release"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Release"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Solaris_Debug"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Release"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 Linux_Debug"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ELSEIF  "$(CFG)" == "Vdba - Win32 AIX_Release"

# Begin Custom Build
InputPath=.\starsel.scc

"starsel.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlcc -multi -fstarsel.inc starsel.scc

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\a2stream.h
# End Source File
# Begin Source File

SOURCE=.\Alarms.h
# End Source File
# Begin Source File

SOURCE=.\axipm.h
# End Source File
# Begin Source File

SOURCE=.\axsql.h
# End Source File
# Begin Source File

SOURCE=.\axsqledb.h
# End Source File
# Begin Source File

SOURCE=.\bkgdpref.h
# End Source File
# Begin Source File

SOURCE=.\BMPBTN.H
# End Source File
# Begin Source File

SOURCE=.\buildsvr.h
# End Source File
# Begin Source File

SOURCE=.\CellCtrl.H
# End Source File
# Begin Source File

SOURCE=.\CellIntC.h
# End Source File
# Begin Source File

SOURCE=.\chartini.h
# End Source File
# Begin Source File

SOURCE=.\CHBOXBAR.H
# End Source File
# Begin Source File

SOURCE=.\CHILDFRM.H
# End Source File
# Begin Source File

SOURCE=.\chkpoint.h
# End Source File
# Begin Source File

SOURCE=.\chooseob.h
# End Source File
# Begin Source File

SOURCE=.\ckplst.h
# End Source File
# Begin Source File

SOURCE=.\cmdline.h
# End Source File
# Begin Source File

SOURCE=.\cmdline2.h
# End Source File
# Begin Source File

SOURCE=.\CMIXFILE.H
# End Source File
# Begin Source File

SOURCE=.\collidta.h
# End Source File
# Begin Source File

SOURCE=.\Columns.h
# End Source File
# Begin Source File

SOURCE=.\COMBOBAR.H
# End Source File
# Begin Source File

SOURCE=.\compfile.h
# End Source File
# Begin Source File

SOURCE=.\createdb.h
# End Source File
# Begin Source File

SOURCE=.\DBEDOC.H
# End Source File
# Begin Source File

SOURCE=.\DBEFRAME.H
# End Source File
# Begin Source File

SOURCE=.\DBEVIEW.H
# End Source File
# Begin Source File

SOURCE=.\DgArccln.h
# End Source File
# Begin Source File

SOURCE=.\dgbkrefr.h
# End Source File
# Begin Source File

SOURCE=.\DGDBEP01.H
# End Source File
# Begin Source File

SOURCE=.\DGDBEP02.H
# End Source File
# Begin Source File

SOURCE=.\DgDomC02.h
# End Source File
# Begin Source File

SOURCE=.\dgdpaulo.h
# End Source File
# Begin Source File

SOURCE=.\dgdpauus.h
# End Source File
# Begin Source File

SOURCE=.\DgDpCd.H
# End Source File
# Begin Source File

SOURCE=.\DgDpCdTb.H
# End Source File
# Begin Source File

SOURCE=.\DgDpCo.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDb.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDbAl.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDbEv.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDbGr.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDbPr.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDbSc.H
# End Source File
# Begin Source File

SOURCE=.\dgdpdbse.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDbSy.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDbTb.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDbVi.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDd.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDdPr.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDdTb.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDdVi.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDILS.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDPL.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDPLS.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDtL.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDTLX.H
# End Source File
# Begin Source File

SOURCE=.\DgDpDVL.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDVLS.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDVN.h
# End Source File
# Begin Source File

SOURCE=.\DgDpDVNS.h
# End Source File
# Begin Source File

SOURCE=.\dgdpenle.h
# End Source File
# Begin Source File

SOURCE=.\DgDpEvGr.H
# End Source File
# Begin Source File

SOURCE=.\DgDpGrp.H
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXD.H
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXE.H
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXP.H
# End Source File
# Begin Source File

SOURCE=.\dgdpgrxs.h
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXT.H
# End Source File
# Begin Source File

SOURCE=.\DgDpGrXV.H
# End Source File
# Begin Source File

SOURCE=.\DgDpIdx.h
# End Source File
# Begin Source File

SOURCE=.\dgdpihtm.h
# End Source File
# Begin Source File

SOURCE=.\DgDpInt.h
# End Source File
# Begin Source File

SOURCE=.\DgDPLoc.h
# End Source File
# Begin Source File

SOURCE=.\DgDpLocD.H
# End Source File
# Begin Source File

SOURCE=.\DgDpLocS.H
# End Source File
# Begin Source File

SOURCE=.\DgDpLoSu.H
# End Source File
# Begin Source File

SOURCE=.\DgDpPrc.h
# End Source File
# Begin Source File

SOURCE=.\DgDpPrcG.H
# End Source File
# Begin Source File

SOURCE=.\DgDpPro.h
# End Source File
# Begin Source File

SOURCE=.\DgDpRol.h
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXD.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXE.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXP.H
# End Source File
# Begin Source File

SOURCE=.\dgdproxs.h
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXT.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRoXV.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRpCd.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRpCo.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRpMu.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRpRt.H
# End Source File
# Begin Source File

SOURCE=.\DgDpRul.h
# End Source File
# Begin Source File

SOURCE=.\DgDpScPr.H
# End Source File
# Begin Source File

SOURCE=.\DgDpScTb.H
# End Source File
# Begin Source File

SOURCE=.\DgDpScVi.H
# End Source File
# Begin Source File

SOURCE=.\dgdpseq.h
# End Source File
# Begin Source File

SOURCE=.\dgdpseqg.h
# End Source File
# Begin Source File

SOURCE=.\dgdpstdb.h
# End Source File
# Begin Source File

SOURCE=.\dgdpstgd.h
# End Source File
# Begin Source File

SOURCE=.\dgdpstgr.h
# End Source File
# Begin Source File

SOURCE=.\dgdpstlo.h
# End Source File
# Begin Source File

SOURCE=.\dgdpstpr.h
# End Source File
# Begin Source File

SOURCE=.\dgdpstro.h
# End Source File
# Begin Source File

SOURCE=.\dgdpstus.h
# End Source File
# Begin Source File

SOURCE=.\DgDpTb.h
# End Source File
# Begin Source File

SOURCE=.\DgDpTbAl.H
# End Source File
# Begin Source File

SOURCE=.\DgDpTbCo.h
# End Source File
# Begin Source File

SOURCE=.\DgDpTbGr.H
# End Source File
# Begin Source File

SOURCE=.\DgDpTbIg.H
# End Source File
# Begin Source File

SOURCE=.\DgDpTbIx.H
# End Source File
# Begin Source File

SOURCE=.\DgDpTbLo.H
# End Source File
# Begin Source File

SOURCE=.\DgDpTbPg.H
# End Source File
# Begin Source File

SOURCE=.\DgDpTbRu.H
# End Source File
# Begin Source File

SOURCE=.\dgdptbst.h
# End Source File
# Begin Source File

SOURCE=.\DgDpTRNA.h
# End Source File
# Begin Source File

SOURCE=.\dgdptrow.h
# End Source File
# Begin Source File

SOURCE=.\DgDPUsr.h
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXA.H
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXD.H
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXE.H
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXP.H
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXR.H
# End Source File
# Begin Source File

SOURCE=.\dgdpusxs.h
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXT.H
# End Source File
# Begin Source File

SOURCE=.\DgDpUsXV.H
# End Source File
# Begin Source File

SOURCE=.\DgDpVi.h
# End Source File
# Begin Source File

SOURCE=.\DgDpViGr.H
# End Source File
# Begin Source File

SOURCE=.\dgdpwacd.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwb.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwbro.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwbu.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwbus.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwdbc.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwdbu.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwf.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwfnp.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwfpr.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwfpu.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwl.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwlo.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwp.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwpco.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwpr.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwpro.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwro.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwsbr.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwsbu.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwsco.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwsdu.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwspr.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwsro.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwswu.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwva.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwvap.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwvlo.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwvva.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwwco.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwwro.h
# End Source File
# Begin Source File

SOURCE=.\dgdpwwu.h
# End Source File
# Begin Source File

SOURCE=.\DgErrDet.h
# End Source File
# Begin Source File

SOURCE=.\dgexpire.h
# End Source File
# Begin Source File

SOURCE=.\dgstarco.h
# End Source File
# Begin Source File

SOURCE=.\dgtbjnl.h
# End Source File
# Begin Source File

SOURCE=.\DGVNDATA.H
# End Source File
# Begin Source File

SOURCE=.\DGVNLOGI.H
# End Source File
# Begin Source File

SOURCE=.\DGVNODE.H
# End Source File
# Begin Source File

SOURCE=.\DisCon2.H
# End Source File
# Begin Source File

SOURCE=.\DisCon3.H
# End Source File
# Begin Source File

SOURCE=.\DISCONN.H
# End Source File
# Begin Source File

SOURCE=.\dlgmsbox.h
# End Source File
# Begin Source File

SOURCE=.\dlgreque.h
# End Source File
# Begin Source File

SOURCE=.\dlgrmans.h
# End Source File
# Begin Source File

SOURCE=.\dlgrmcmd.h
# End Source File
# Begin Source File

SOURCE=.\DLGVDOC.H
# End Source File
# Begin Source File

SOURCE=.\DLGVFRM.H
# End Source File
# Begin Source File

SOURCE=.\DMLBase.h
# End Source File
# Begin Source File

SOURCE=.\dmlindex.h
# End Source File
# Begin Source File

SOURCE=.\dmlurp.h
# End Source File
# Begin Source File

SOURCE=.\DOCVNODE.H
# End Source File
# Begin Source File

SOURCE=.\domfast.h
# End Source File
# Begin Source File

SOURCE=.\domilist.H
# End Source File
# Begin Source File

SOURCE=.\DomPgInf.h
# End Source File
# Begin Source File

SOURCE=.\DomRfsh.h
# End Source File
# Begin Source File

SOURCE=.\domseri.h
# End Source File
# Begin Source File

SOURCE=.\domseri2.h
# End Source File
# Begin Source File

SOURCE=.\domseri3.h
# End Source File
# Begin Source File

SOURCE=.\dtagsett.h
# End Source File
# Begin Source File

SOURCE=.\duplicdb.h
# End Source File
# Begin Source File

SOURCE=.\edlduslo.h
# End Source File
# Begin Source File

SOURCE=.\edlsdbgr.h
# End Source File
# Begin Source File

SOURCE=.\fastload.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\FRMVNODE.H
# End Source File
# Begin Source File

SOURCE=.\Granted.h
# End Source File
# Begin Source File

SOURCE=.\Grantees.h
# End Source File
# Begin Source File

SOURCE=.\icebunfp.h
# End Source File
# Begin Source File

SOURCE=.\icebunro.h
# End Source File
# Begin Source File

SOURCE=.\icebunus.h
# End Source File
# Begin Source File

SOURCE=.\icecocom.h
# End Source File
# Begin Source File

SOURCE=.\icecoedt.h
# End Source File
# Begin Source File

SOURCE=.\IceDConn.h
# End Source File
# Begin Source File

SOURCE=.\IceDUser.h
# End Source File
# Begin Source File

SOURCE=.\icefprus.h
# End Source File
# Begin Source File

SOURCE=.\icegrs.h
# End Source File
# Begin Source File

SOURCE=.\icelocat.h
# End Source File
# Begin Source File

SOURCE=.\icepassw.h
# End Source File
# Begin Source File

SOURCE=.\IceProfi.h
# End Source File
# Begin Source File

SOURCE=.\icerole.h
# End Source File
# Begin Source File

SOURCE=.\icesvrdt.h
# End Source File
# Begin Source File

SOURCE=.\IceWebU.h
# End Source File
# Begin Source File

SOURCE=.\infodb.h
# End Source File
# Begin Source File

SOURCE=.\ipmaxdoc.h
# End Source File
# Begin Source File

SOURCE=.\ipmaxfrm.h
# End Source File
# Begin Source File

SOURCE=.\ipmaxvie.h
# End Source File
# Begin Source File

SOURCE=.\IPMDISP.H
# End Source File
# Begin Source File

SOURCE=.\ipmicdta.h
# End Source File
# Begin Source File

SOURCE=.\IPMPRDTA.H
# End Source File
# Begin Source File

SOURCE=.\localter.h
# End Source File
# Begin Source File

SOURCE=.\location.h
# End Source File
# Begin Source File

SOURCE=.\LOGINDIC.H
# End Source File
# Begin Source File

SOURCE=.\lstbstat.h
# End Source File
# Begin Source File

SOURCE=.\MAINDOC.H
# End Source File
# Begin Source File

SOURCE=.\MAINFRM.H
# End Source File
# Begin Source File

SOURCE=.\MAINMFC.H
# End Source File
# Begin Source File

SOURCE=.\MAINVI2.H
# End Source File
# Begin Source File

SOURCE=.\MAINVIEW.H
# End Source File
# Begin Source File

SOURCE=.\modsize.h
# End Source File
# Begin Source File

SOURCE=.\MultCol.h
# End Source File
# Begin Source File

SOURCE=.\MultFlag.h
# End Source File
# Begin Source File

SOURCE=.\nodefast.h
# End Source File
# Begin Source File

SOURCE=.\parse.h
# End Source File
# Begin Source File

SOURCE=.\property.h
# End Source File
# Begin Source File

SOURCE=.\qsplittr.h
# End Source File
# Begin Source File

SOURCE=.\qsqldoc.h
# End Source File
# Begin Source File

SOURCE=.\qsqlfram.h
# End Source File
# Begin Source File

SOURCE=.\qsqlview.h
# End Source File
# Begin Source File

SOURCE=.\qstatind.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\RegSvice.h
# End Source File
# Begin Source File

SOURCE=.\repadddb.h
# End Source File
# Begin Source File

SOURCE=.\Replic.h
# End Source File
# Begin Source File

SOURCE=.\replikey.h
# End Source File
# Begin Source File

SOURCE=.\resmfc.h
# End Source File
# Begin Source File

SOURCE=.\rpanedrg.h
# End Source File
# Begin Source File

SOURCE=.\SBDBEBAR.H
# End Source File
# Begin Source File

SOURCE=.\SBDLGBAR.H
# End Source File
# Begin Source File

SOURCE=.\SBIPMBAR.H
# End Source File
# Begin Source File

SOURCE=.\sequence.h
# End Source File
# Begin Source File

SOURCE=.\SERIALTR.H
# End Source File
# Begin Source File

SOURCE=.\SHUTSRV.H
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\SPLITTER.H
# End Source File
# Begin Source File

SOURCE=.\sqlkeywd.h
# End Source File
# Begin Source File

SOURCE=.\sqlquery.h
# End Source File
# Begin Source File

SOURCE=.\sqlselec.h
# End Source File
# Begin Source File

SOURCE=.\STARITEM.H
# End Source File
# Begin Source File

SOURCE=.\StarLoTb.h
# End Source File
# Begin Source File

SOURCE=.\STARPRLN.H
# End Source File
# Begin Source File

SOURCE=.\starsel.h
# End Source File
# Begin Source File

SOURCE=.\STARTBLN.H
# End Source File
# Begin Source File

SOURCE=.\StCellIt.h
# End Source File
# Begin Source File

SOURCE=.\STDAFX.H
# End Source File
# Begin Source File

SOURCE=.\StMetalB.h
# End Source File
# Begin Source File

SOURCE=.\taskxprm.h
# End Source File
# Begin Source File

SOURCE=.\tbstatis.h
# End Source File
# Begin Source File

SOURCE=.\tcomment.h
# End Source File
# Begin Source File

SOURCE=.\TOOLBARS.H
# End Source File
# Begin Source File

SOURCE=..\hdr\TREE.H
# End Source File
# Begin Source File

SOURCE=.\urpectrl.h
# End Source File
# Begin Source File

SOURCE=.\usermod.h
# End Source File
# Begin Source File

SOURCE=.\VEWVNODE.H
# End Source File
# Begin Source File

SOURCE=.\VNITEM.H
# End Source File
# Begin Source File

SOURCE=.\VOBJTYPE.H
# End Source File
# Begin Source File

SOURCE=.\VTREE.H
# End Source File
# Begin Source File

SOURCE=.\VTREE1.H
# End Source File
# Begin Source File

SOURCE=.\VTREE2.H
# End Source File
# Begin Source File

SOURCE=.\VTREE3.H
# End Source File
# Begin Source File

SOURCE=.\VTREECTL.H
# End Source File
# Begin Source File

SOURCE=.\VTREEDAT.H
# End Source File
# Begin Source File

SOURCE=.\VTREEGLB.H
# End Source File
# Begin Source File

SOURCE=.\VWDBEP02.H
# End Source File
# Begin Source File

SOURCE=.\webbrows.h
# End Source File
# Begin Source File

SOURCE=.\xcopydb.h
# End Source File
# Begin Source File

SOURCE=.\xdgidxop.h
# End Source File
# Begin Source File

SOURCE=.\xdgvnatr.h
# End Source File
# Begin Source File

SOURCE=.\xdgvnpwd.h
# End Source File
# Begin Source File

SOURCE=.\xdlgacco.h
# End Source File
# Begin Source File

SOURCE=.\xdlgdbgr.h
# End Source File
# Begin Source File

SOURCE=.\xdlgdrop.h
# End Source File
# Begin Source File

SOURCE=.\xdlggset.h
# End Source File
# Begin Source File

SOURCE=.\xdlgindx.h
# End Source File
# Begin Source File

SOURCE=.\xdlgpref.h
# End Source File
# Begin Source File

SOURCE=.\xdlgprof.h
# End Source File
# Begin Source File

SOURCE=.\xdlgrole.h
# End Source File
# Begin Source File

SOURCE=.\xdlgrule.h
# End Source File
# Begin Source File

SOURCE=.\xdlguser.h
# End Source File
# Begin Source File

SOURCE=.\xdlgwait.h
# End Source File
# Begin Source File

SOURCE=.\xinstenl.h
# End Source File
# Begin Source File

SOURCE=.\xtbpkey.h
# End Source File
# End Group
# End Target
# End Project
# Section Vdba : {8856F961-340A-11D0-A96B-00C04FD705A2}
# 	2:21:DefaultSinkHeaderFile:webbrows.h
# 	2:16:DefaultSinkClass:CWebBrowser2
# End Section
# Section Vdba : {D30C1661-CDAF-11D0-8A3E-00C04FC9E26E}
# 	2:5:Class:CWebBrowser2
# 	2:10:HeaderFile:webbrows.h
# 	2:8:ImplFile:webbrows.cpp
# End Section
# Section Vdba : {5C616264-6456-6162-446F-6D53706C6974}
# 	1:10:IDB_SPLASH:110
# 	2:21:SplashScreenInsertKey:4.0
# End Section
