# Microsoft Developer Studio Project File - Name="vnodemgr" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vnodemgr - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vnodemgr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vnodemgr.mak" CFG="vnodemgr - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vnodemgr - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 EDBC Debug" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 EDBC Release" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 DBCS Debug" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 DBCS Release" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 Solaris_Release" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 HPUX_Release" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 HPUX_Debug" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 Linux_Release" (based on "Win32 (x86) Application")
!MESSAGE "vnodemgr - Win32 AIX_Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Vnodemgr", TTFAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\ingnet.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ingnet.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\ingnet.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ingnet.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vnodemgr"
# PROP BASE Intermediate_Dir "vnodemgr"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vnodemgr"
# PROP Intermediate_Dir "vnodemgr"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "EDBC"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/edbcnet.exe" /pdbtype:sept
# ADD LINK32 edbc.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\app\edbcnet.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\edbcnet.exe %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vnodemg0"
# PROP BASE Intermediate_Dir "vnodemg0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vnodemg0"
# PROP Intermediate_Dir "vnodemg0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "EDBC"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /machine:I386 /out:"Release/edbcnet.exe"
# ADD LINK32 edbc.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\app\edbcnet.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\edbcnet.exe %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vnodemgr___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_DBCS_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/ingnet.exe" /pdbtype:sept
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\ingnet.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ingnet.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vnodemgr___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_DBCS_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe"
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\ingnet.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ingnet.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vnodemgr___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vnodemgr___Win32_Solaris_Release"
# PROP Intermediate_Dir "vnodemgr___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe"
# ADD LINK32 ingres.lib elf.lib socket.lib nsl.lib dl.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe" /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vnodemgr___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vnodemgr___Win32_HPUX_Release"
# PROP Intermediate_Dir "vnodemgr___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe"
# ADD LINK32 q.1.lib compat.1.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /out:"vnodemgr___Win32_HPUX_Release/ingnet.exe" /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vnodemgr___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vnodemgr___Win32_HPUX_Debug"
# PROP Intermediate_Dir "vnodemgr___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/ingnet.exe" /pdbtype:sept
# ADD LINK32 q.1.lib compat.1.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /out:"vnodemgr___Win32_HPUX_Debug/ingnet.exe" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib" /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vnodemgr___Win32_Debug64"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "VDBAICONS"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/ingnet.exe" /pdbtype:sept
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres64.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug64/ingnet.exe" /libpath:"$(ING_SRC)\front\lib" /machine:IA64

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vnodemgr___Win32_Release64"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe"
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres64.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"Release64/ingnet.exe" /libpath:"$(ING_SRC)\front\lib" /machine:IA64

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vnodemgr___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vnodemgr___Win32_Linux_Release"
# PROP Intermediate_Dir "vnodemgr___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe"
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe" /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vnodemgr___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "vnodemgr___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vnodemgr___Win32_AIX_Release"
# PROP Intermediate_Dir "vnodemgr___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\vdba\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "VDBAICONS" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe"
# ADD LINK32 $(II_SYSTEM)\ingres\lib\ingres.lib libingll.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ingnet.exe" /libpath:"$(ING_SRC)\front\lib"

!ENDIF 

# Begin Target

# Name "vnodemgr - Win32 Release"
# Name "vnodemgr - Win32 Debug"
# Name "vnodemgr - Win32 EDBC Debug"
# Name "vnodemgr - Win32 EDBC Release"
# Name "vnodemgr - Win32 DBCS Debug"
# Name "vnodemgr - Win32 DBCS Release"
# Name "vnodemgr - Win32 Solaris_Release"
# Name "vnodemgr - Win32 HPUX_Release"
# Name "vnodemgr - Win32 HPUX_Debug"
# Name "vnodemgr - Win32 Debug64"
# Name "vnodemgr - Win32 Release64"
# Name "vnodemgr - Win32 Linux_Release"
# Name "vnodemgr - Win32 AIX_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dglogipt.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dgvndata.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dgvnlogi.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dgvnode.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\maindoc.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mainview.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nodeinfo.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parsearg.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\treenode.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vnode.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vnodemgr.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vnodemgr.rc
# End Source File
# Begin Source File

SOURCE=.\xdgvnatr.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xdgvnpwd.cpp

!IF  "$(CFG)" == "vnodemgr - Win32 Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Release64"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vnodemgr - Win32 AIX_Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dglogipt.h
# End Source File
# Begin Source File

SOURCE=.\dgvndata.h
# End Source File
# Begin Source File

SOURCE=.\dgvnlogi.h
# End Source File
# Begin Source File

SOURCE=.\dgvnode.h
# End Source File
# Begin Source File

SOURCE=.\maindoc.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\mainview.h
# End Source File
# Begin Source File

SOURCE=.\nodeinfo.h
# End Source File
# Begin Source File

SOURCE=.\parsearg.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\treenode.h
# End Source File
# Begin Source File

SOURCE=.\vnode.h
# End Source File
# Begin Source File

SOURCE=.\vnodemgr.h
# End Source File
# Begin Source File

SOURCE=.\xdgvnatr.h
# End Source File
# Begin Source File

SOURCE=.\xdgvnpwd.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\about.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\res\initial.bmp
# End Source File
# Begin Source File

SOURCE=.\res\maindoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\mainfram.bmp
# End Source File
# Begin Source File

SOURCE=.\res\nodetree.bmp
# End Source File
# Begin Source File

SOURCE=.\res\op20imag.bmp
# End Source File
# Begin Source File

SOURCE=.\res\op20tbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vnodemgr.ico
# End Source File
# Begin Source File

SOURCE=.\res\vnodemgr.rc2
# End Source File
# End Group
# Begin Group "Document Files"

# PROP Default_Filter "*.txt; *.doc"
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\vnodemgr.doc
# End Source File
# End Group
# End Target
# End Project
# Section vnodemgr : {51C21334-EBD1-D545-92BD-CF11877200A0}
# 	1:14:IDR_NODE_POPUP:102
# End Section
