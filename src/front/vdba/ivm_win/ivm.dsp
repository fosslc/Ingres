# Microsoft Developer Studio Project File - Name="Ivm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Ivm - Win32 Debug64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Ivm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Ivm.mak" CFG="Ivm - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ivm - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 Debug Simulation" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 DBCS Release" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 DBCS Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 Solaris_Release" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 HPUX_Release" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 HPUX_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 Solaris_Debug" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 Linux_Release" (based on "Win32 (x86) Application")
!MESSAGE "Ivm - Win32 AIX_Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Ivm", FVDAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Ivm - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\ivm.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ivm.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ivm.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ivm.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ivm___Wi"
# PROP BASE Intermediate_Dir "Ivm___Wi"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugSim"
# PROP Intermediate_Dir "DebugSim"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "_DEBUG" /D "SIMULATION" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /pdbtype:sept
# ADD LINK32 util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"App/Ivm.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ivm___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "Ivm___Win32_DBCS_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "NDEBUG" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe"
# ADD LINK32 util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\ivm.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ivm.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ivm___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "Ivm___Win32_DBCS_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "_DEBUG" /D "_MBCS" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"App/Ivm.exe" /pdbtype:sept
# ADD LINK32 util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"..\App\ivm.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\ivm.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ivm___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "Ivm___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ivm___Win32_Solaris_Release"
# PROP Intermediate_Dir "Ivm___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe"
# ADD LINK32 compat.lib ingres.lib util.lib elf.lib nsl.lib dl.lib socket.lib posix4.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ivm___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "Ivm___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ivm___Win32_HPUX_Release"
# PROP Intermediate_Dir "Ivm___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe"
# ADD LINK32 q.1.lib compat.1.lib util.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"winspool.lib" /libpath:"$(II_SYSTEM)\ingres\lib"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ivm___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "Ivm___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Ivm___Win32_HPUX_Debug"
# PROP Intermediate_Dir "Ivm___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"App/Ivm.exe" /pdbtype:sept
# ADD LINK32 q.1.lib compat.1.lib util.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"winspool.lib" /pdbtype:sept /libpath:"$(II_SYSTEM)/ingres/lib"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ivm___Win32_Solaris_Debug2"
# PROP BASE Intermediate_Dir "Ivm___Win32_Solaris_Debug2"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Ivm___Win32_Solaris_Debug2"
# PROP Intermediate_Dir "Ivm___Win32_Solaris_Debug2"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"App/Ivm.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 compat.lib ingres.lib util.lib elf.lib nsl.lib dl.lib socket.lib posix4.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"App/Ivm.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Ivm___Win32_Debug64"
# PROP BASE Intermediate_Dir "Ivm___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /out:"App/Ivm.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd.lib" /libpath:"$(ING_SRC)\front\lib" /machine:IA64

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ivm___Win32_Release64"
# PROP BASE Intermediate_Dir "Ivm___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "NDEBUG" /D "WIN64" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:IA64

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ivm___Win32_Linux_Release1"
# PROP BASE Intermediate_Dir "Ivm___Win32_Linux_Release1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ivm___Win32_Linux_Release1"
# PROP Intermediate_Dir "Ivm___Win32_Linux_Release1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 compat.lib ingres.lib iiutil.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"winspool.lib" /out:"App/Ivm.exe" /libpath:"$(ING_SRC)/build/lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Ivm___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "Ivm___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Ivm___Win32_AIX_Release"
# PROP Intermediate_Dir "Ivm___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "DESKTOP" /D "IMPORT_DLL_DATA" /D "NT_GENERIC" /D "INGRESII" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\report\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\st\config" /I "..\inc" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "IVMPROJ" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "rs4_us5" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 compat.lib util.lib ingres.lib config.lib libwctrl.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"App/Ivm.exe"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Ivm - Win32 Release"
# Name "Ivm - Win32 Debug"
# Name "Ivm - Win32 Debug Simulation"
# Name "Ivm - Win32 DBCS Release"
# Name "Ivm - Win32 DBCS Debug"
# Name "Ivm - Win32 Solaris_Release"
# Name "Ivm - Win32 HPUX_Release"
# Name "Ivm - Win32 HPUX_Debug"
# Name "Ivm - Win32 Solaris_Debug"
# Name "Ivm - Win32 Debug64"
# Name "Ivm - Win32 Release64"
# Name "Ivm - Win32 Linux_Release"
# Name "Ivm - Win32 AIX_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\acprcp.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\compbase.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\compcat.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\compdata.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dgevsetb.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dgevsetl.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dgevsetr.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dginstt1.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dginstt2.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dgnewmsg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dlgenvar.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dlgepref.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dlgmain.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dlgmsg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\docevset.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dtagsett.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\edlsinpr.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\evtcats.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fevsetti.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fileioex.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\findinfo.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\getinst.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Ivm.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Ivm.rc
# End Source File
# Begin Source File

SOURCE=.\ivmdisp.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ivmdml.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ivmsgdml.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\levgener.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ll.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lmsgstat.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lsctrevt.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lsscroll.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\maindoc.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\maintab.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mainview.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mgrfrm.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mgrvleft.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mgrvrigh.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msgcateg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msgntree.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msgstat.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msgtreec.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parinext.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parinsta.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parinsys.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parinusr.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ppmsgdes.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\readflg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\readmsg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\regsvice.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\setenvin.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\staautco.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stadupco.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stainsta.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stainstg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stasinco.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\uvscroll.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vevsettb.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vevsettl.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vevsettr.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xdgevset.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xdgsysfo.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xdlgacco.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xdlgmsg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xdlgpref.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\xfindmsg.cpp

!IF  "$(CFG)" == "Ivm - Win32 Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug Simulation"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Solaris_Debug"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Debug64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Release64"

!ELSEIF  "$(CFG)" == "Ivm - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "Ivm - Win32 AIX_Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\acprcp.h
# End Source File
# Begin Source File

SOURCE=.\compbase.h
# End Source File
# Begin Source File

SOURCE=.\compcat.h
# End Source File
# Begin Source File

SOURCE=.\compdata.h
# End Source File
# Begin Source File

SOURCE=.\dgevsetb.h
# End Source File
# Begin Source File

SOURCE=.\dgevsetl.h
# End Source File
# Begin Source File

SOURCE=.\dgevsetr.h
# End Source File
# Begin Source File

SOURCE=.\dginstt1.h
# End Source File
# Begin Source File

SOURCE=.\dginstt2.h
# End Source File
# Begin Source File

SOURCE=.\dgnewmsg.h
# End Source File
# Begin Source File

SOURCE=.\dlgenvar.h
# End Source File
# Begin Source File

SOURCE=.\dlgepref.h
# End Source File
# Begin Source File

SOURCE=.\dlgmain.h
# End Source File
# Begin Source File

SOURCE=.\dlgmsg.h
# End Source File
# Begin Source File

SOURCE=.\docevset.h
# End Source File
# Begin Source File

SOURCE=.\dtagsett.h
# End Source File
# Begin Source File

SOURCE=.\edlsinpr.h
# End Source File
# Begin Source File

SOURCE=.\evtcats.h
# End Source File
# Begin Source File

SOURCE=.\fevsetti.h
# End Source File
# Begin Source File

SOURCE=.\fileioex.h
# End Source File
# Begin Source File

SOURCE=.\findinfo.h
# End Source File
# Begin Source File

SOURCE=.\getinst.h
# End Source File
# Begin Source File

SOURCE=.\Ivm.h
# End Source File
# Begin Source File

SOURCE=.\ivmdisp.h
# End Source File
# Begin Source File

SOURCE=.\ivmdml.h
# End Source File
# Begin Source File

SOURCE=.\ivmsgdml.h
# End Source File
# Begin Source File

SOURCE=.\levgener.h
# End Source File
# Begin Source File

SOURCE=.\ll.h
# End Source File
# Begin Source File

SOURCE=.\lmsgstat.h
# End Source File
# Begin Source File

SOURCE=.\lsctrevt.h
# End Source File
# Begin Source File

SOURCE=.\lsscroll.h
# End Source File
# Begin Source File

SOURCE=.\maindoc.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\maintab.h
# End Source File
# Begin Source File

SOURCE=.\mainview.h
# End Source File
# Begin Source File

SOURCE=.\mgrfrm.h
# End Source File
# Begin Source File

SOURCE=.\mgrvleft.h
# End Source File
# Begin Source File

SOURCE=.\mgrvrigh.h
# End Source File
# Begin Source File

SOURCE=.\msgcateg.h
# End Source File
# Begin Source File

SOURCE=.\msgntree.h
# End Source File
# Begin Source File

SOURCE=.\msgstat.h
# End Source File
# Begin Source File

SOURCE=.\msgtreec.h
# End Source File
# Begin Source File

SOURCE=.\parinext.h
# End Source File
# Begin Source File

SOURCE=.\parinsta.h
# End Source File
# Begin Source File

SOURCE=.\parinsys.h
# End Source File
# Begin Source File

SOURCE=.\parinusr.h
# End Source File
# Begin Source File

SOURCE=.\ppmsgdes.h
# End Source File
# Begin Source File

SOURCE=.\rcdepend.h
# End Source File
# Begin Source File

SOURCE=.\readflg.h
# End Source File
# Begin Source File

SOURCE=.\readmsg.h
# End Source File
# Begin Source File

SOURCE=.\regsvice.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\setenvin.h
# End Source File
# Begin Source File

SOURCE=.\staautco.h
# End Source File
# Begin Source File

SOURCE=.\stadupco.h
# End Source File
# Begin Source File

SOURCE=.\stainsta.h
# End Source File
# Begin Source File

SOURCE=.\stainstg.h
# End Source File
# Begin Source File

SOURCE=.\stasinco.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\uvscroll.h
# End Source File
# Begin Source File

SOURCE=.\vevsettb.h
# End Source File
# Begin Source File

SOURCE=.\vevsettl.h
# End Source File
# Begin Source File

SOURCE=.\vevsettr.h
# End Source File
# Begin Source File

SOURCE=.\xdgevset.h
# End Source File
# Begin Source File

SOURCE=.\xdgsysfo.h
# End Source File
# Begin Source File

SOURCE=.\xdlgacco.h
# End Source File
# Begin Source File

SOURCE=.\xdlgmsg.h
# End Source File
# Begin Source File

SOURCE=.\xdlgpref.h
# End Source File
# Begin Source File

SOURCE=.\xfindmsg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\about.bmp
# End Source File
# Begin Source File

SOURCE=.\res\aboutadv.bmp
# End Source File
# Begin Source File

SOURCE=.\res\anclock.avi
# End Source File
# Begin Source File

SOURCE=.\res\archiver.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bridge26.bmp
# End Source File
# Begin Source File

SOURCE=.\res\dbms26.bmp
# End Source File
# Begin Source File

SOURCE=.\res\dropcat.cur
# End Source File
# Begin Source File

SOURCE=.\res\eventind.bmp
# End Source File
# Begin Source File

SOURCE=.\res\evironme.bmp
# End Source File
# Begin Source File

SOURCE=.\res\folder26.bmp
# End Source File
# Begin Source File

SOURCE=.\res\gateway.bmp
# End Source File
# Begin Source File

SOURCE=.\res\hisstart.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ice26x16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\installa.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Ivm.ico
# End Source File
# Begin Source File

SOURCE=.\res\Ivm.rc2
# End Source File
# Begin Source File

SOURCE=.\res\jdb26x16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\locking.bmp
# End Source File
# Begin Source File

SOURCE=.\res\logging.bmp
# End Source File
# Begin Source File

SOURCE=.\res\maindoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\msgcatse.bmp
# End Source File
# Begin Source File

SOURCE=.\res\name26.bmp
# End Source File
# Begin Source File

SOURCE=.\res\net26x16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\nodropca.cur
# End Source File
# Begin Source File

SOURCE=.\res\recovery.bmp
# End Source File
# Begin Source File

SOURCE=.\res\resizema.bmp
# End Source File
# Begin Source File

SOURCE=.\res\rmcmd26x.bmp
# End Source File
# Begin Source File

SOURCE=.\res\security.bmp
# End Source File
# Begin Source File

SOURCE=.\res\star26.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\transact.bmp
# End Source File
# Begin Source File

SOURCE=.\res\trayimag.bmp
# End Source File
# End Group
# Begin Group "Data for Simulation"

# PROP Default_Filter "*.dat"
# Begin Source File

SOURCE=.\App\componen.dat
# End Source File
# Begin Source File

SOURCE=.\App\eventcat.dat
# End Source File
# Begin Source File

SOURCE=.\App\ingcatms.dat
# End Source File
# Begin Source File

SOURCE=.\App\instance.dat
# End Source File
# Begin Source File

SOURCE=.\App\logevent.dat
# End Source File
# Begin Source File

SOURCE=.\App\othermsg.dat
# End Source File
# Begin Source File

SOURCE=.\App\paramsys.dat
# End Source File
# Begin Source File

SOURCE=.\App\paramusr.dat
# End Source File
# Begin Source File

SOURCE=.\App\usrcateg.dat
# End Source File
# Begin Source File

SOURCE=.\App\usrspec.dat
# End Source File
# End Group
# Begin Group "Document Files"

# PROP Default_Filter "*.doc; *.txt"
# Begin Source File

SOURCE=.\Ivm.doc
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Group
# End Target
# End Project
