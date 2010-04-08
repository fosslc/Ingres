# Microsoft Developer Studio Project File - Name="Ija" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Ija - Win32 Linux_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ija.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ija.mak" CFG="Ija - Win32 Linux_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ija - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Debug Simulation" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Release Simulation" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 DBCS Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 DBCS Release" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Solaris_Release" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 HPUX_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 HPUX_Release" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Solaris_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Linux_Release" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 AIX_Release" (based on "Win32 (x86) Application")
!MESSAGE "Ija - Win32 Linux_Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Ija", REGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Ija - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\ija.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ija.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib tkanimat.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ija.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ija.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Debug Simulation"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ija___Wi"
# PROP BASE Intermediate_Dir "Ija___Wi"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugSim"
# PROP Intermediate_Dir "DebugSim"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "SIMULATION" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\DebugSim
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Release Simulation"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ija___Win32_Release_Simulation"
# PROP BASE Intermediate_Dir "Ija___Win32_Release_Simulation"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "RelSim"
# PROP Intermediate_Dir "RelSim"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "SIMULATION" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\RelSim
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Unicode Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ija___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "Ija___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "UnicodeDebug"
# PROP Intermediate_Dir "UnicodeDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"..\lib"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
OutDir=.\UnicodeDebug
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Unicode Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ija___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "Ija___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "UnicodeRelease"
# PROP Intermediate_Dir "UnicodeRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\front\vdba\inc4res" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"..\lib"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
OutDir=.\UnicodeRelease
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 DBCS Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ija___Win32_DBCS Debug"
# PROP BASE Intermediate_Dir "Ija___Win32_DBCS Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ija.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ija.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 DBCS Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ija___Win32_DBCS Release"
# PROP BASE Intermediate_Dir "Ija___Win32_DBCS Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\ija.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ija.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ija___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "Ija___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ija___Win32_Solaris_Release"
# PROP Intermediate_Dir "Ija___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# ADD LINK32 ingres.lib nsl.lib elf.lib dl.lib socket.lib posix4.lib comctl32.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\Ija___Win32_Solaris_Release
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ija___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "Ija___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Ija___Win32_HPUX_Debug"
# PROP Intermediate_Dir "Ija___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"..\lib"
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 q.1.lib compat.1.lib comctl32.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib winspool.lib" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "Ija - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ija___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "Ija___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ija___Win32_HPUX_Release"
# PROP Intermediate_Dir "Ija___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# ADD LINK32 q.1.lib compat.1.lib comctl32.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"winspool.lib" /libpath:"$(II_SYSTEM)\ingres\lib"

!ELSEIF  "$(CFG)" == "Ija - Win32 Solaris_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ija___Win32_Solaris_Debug2"
# PROP BASE Intermediate_Dir "Ija___Win32_Solaris_Debug2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Ija___Win32_Solaris_Debug2"
# PROP Intermediate_Dir "Ija___Win32_Solaris_Debug2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"..\lib"
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ingres.lib nsl.lib elf.lib dl.lib socket.lib posix4.lib comctl32.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept /libpath:"..\lib"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
OutDir=.\Ija___Win32_Solaris_Debug2
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ija___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "Ija___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ija___Win32_Linux_Release"
# PROP Intermediate_Dir "Ija___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\Ija___Win32_Linux_Release
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ija___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "Ija___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ija___Win32_AIX_Release"
# PROP Intermediate_Dir "Ija___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# Begin Special Build Tool
OutDir=.\Ija___Win32_AIX_Release
SOURCE="$(InputPath)"
PostBuild_Desc=Copy File to App ...
PostBuild_Cmds=Copy $(OutDir)\ija.exe App
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ija - Win32 Linux_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ija___Win32_Linux_Debug"
# PROP BASE Intermediate_Dir "Ija___Win32_Linux_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Ija___Win32_Linux_Debug"
# PROP Intermediate_Dir "Ija___Win32_Linux_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\front\vdba\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\vdba\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /i "$(ING_SRC)\front\vdba\inc4res" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ija.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT BASE LINK32 /nodefaultlib
# ADD LINK32 ingres.lib libguids.lib libingll.lib libwctrl.lib ws2_32.lib tkanimat.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ija.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT LINK32 /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ija.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "Ija - Win32 Release"
# Name "Ija - Win32 Debug"
# Name "Ija - Win32 Debug Simulation"
# Name "Ija - Win32 Release Simulation"
# Name "Ija - Win32 Unicode Debug"
# Name "Ija - Win32 Unicode Release"
# Name "Ija - Win32 DBCS Debug"
# Name "Ija - Win32 DBCS Release"
# Name "Ija - Win32 Solaris_Release"
# Name "Ija - Win32 HPUX_Debug"
# Name "Ija - Win32 HPUX_Release"
# Name "Ija - Win32 Solaris_Debug"
# Name "Ija - Win32 Linux_Release"
# Name "Ija - Win32 AIX_Release"
# Name "Ija - Win32 Linux_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\drefresh.cpp
# End Source File
# Begin Source File

SOURCE=.\ija.cpp
# End Source File
# Begin Source File

SOURCE=.\ija.rc
# End Source File
# Begin Source File

SOURCE=.\ijactrl.cpp
# End Source File
# Begin Source File

SOURCE=.\ijadml.cpp
# End Source File
# Begin Source File

SOURCE=.\ijatree.cpp
# End Source File
# Begin Source File

SOURCE=.\maindoc.cpp
# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\parsearg.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\viewleft.cpp
# End Source File
# Begin Source File

SOURCE=.\viewrigh.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\drefresh.h
# End Source File
# Begin Source File

SOURCE=.\ija.h
# End Source File
# Begin Source File

SOURCE=.\ijactrl.h
# End Source File
# Begin Source File

SOURCE=.\ijadml.h
# End Source File
# Begin Source File

SOURCE=.\ijatree.h
# End Source File
# Begin Source File

SOURCE=.\maindoc.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\parsearg.h
# End Source File
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

SOURCE=.\viewleft.h
# End Source File
# Begin Source File

SOURCE=.\viewrigh.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\hotmainf.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ija.ico
# End Source File
# Begin Source File

SOURCE=.\res\ija.rc2
# End Source File
# Begin Source File

SOURCE=.\res\maindoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# End Group
# Begin Group "Simulation Files"

# PROP Default_Filter "*.dat;*.dta"
# Begin Source File

SOURCE=.\App\dbtran.dta
# End Source File
# Begin Source File

SOURCE=.\App\detailtr.dta
# End Source File
# Begin Source File

SOURCE=.\App\listdbas.dta
# End Source File
# Begin Source File

SOURCE=.\App\listnode.dta
# End Source File
# Begin Source File

SOURCE=.\App\listtabl.dta
# End Source File
# Begin Source File

SOURCE=.\App\listuser.dta
# End Source File
# Begin Source File

SOURCE=.\App\tabltran.dta
# End Source File
# End Group
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Target
# End Project
# Section Ija : {C92B8427-B176-11D3-A322-00C04F1F754A}
# 	2:21:DefaultSinkHeaderFile:ijactrl.h
# 	2:16:DefaultSinkClass:CIjaCtrl
# End Section
# Section Ija : {C92B8425-B176-11D3-A322-00C04F1F754A}
# 	2:5:Class:CIjaCtrl
# 	2:10:HeaderFile:ijactrl.h
# 	2:8:ImplFile:ijactrl.cpp
# End Section
