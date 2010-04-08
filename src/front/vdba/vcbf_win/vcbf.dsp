# Microsoft Developer Studio Project File - Name="vcbf" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vcbf - Win32 Linux_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vcbf.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vcbf.mak" CFG="vcbf - Win32 Linux_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vcbf - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 EDBC Debug" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 EDBC Release" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 DBCS Release" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 DBCS Debug" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 Solaris_Release" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 HPUX_Release" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 HPUX_Debug" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 Linux_Release" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 Linux_Debug" (based on "Win32 (x86) Application")
!MESSAGE "vcbf - Win32 AIX_Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Vcbf New", KJGAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vcbf - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingres.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vcbf.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcbf.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 ingres.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /out:"..\App\vcbf.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcbf.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcbf___Win32_EDBC_Debug0"
# PROP BASE Intermediate_Dir "vcbf___Win32_EDBC_Debug0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inghdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EDBC" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib gver.obj cvaf.obj OIglfnt.lib OIfrsnt.lib OIclfnt.lib OIembdnt.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"..\lib"
# ADD LINK32 ingres.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /out:"..\App\vcbf.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\vcbf.exe %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcbf___Win32_EDBC_Release0"
# PROP BASE Intermediate_Dir "vcbf___Win32_EDBC_Release0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inghdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 config.lib util.lib ingres.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\lib"
# ADD LINK32 ingres.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vcbf.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\vcbf.exe %EDBC_ROOT%\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcbf___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "vcbf___Win32_DBCS_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vcbf.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcbf.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcbf___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "vcbf___Win32_DBCS_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /out:"..\App\vcbf.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\vcbf.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcbf___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "vcbf___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcbf___Win32_Solaris_Release"
# PROP Intermediate_Dir "vcbf___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib compat.lib util.lib elf.lib nsl.lib posix4.lib dl.lib socket.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcbf___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "vcbf___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcbf___Win32_HPUX_Release"
# PROP Intermediate_Dir "vcbf___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 q.1.lib compat.1.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /libpath:"$(II_SYSTEM)\ingres\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcbf___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "vcbf___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcbf___Win32_HPUX_Debug"
# PROP Intermediate_Dir "vcbf___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 q.1.lib compat.1.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd winspool.lib odbccp32" /out:"vcbf___Win32_HPUX_Debug/vcbf" /libpath:"$(II_SYSTEM)\ingres\lib"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcbf___Win32_Debug64"
# PROP BASE Intermediate_Dir "vcbf___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres64.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /libpath:"$(ING_SRC)\front\lib" /machine:IA64
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcbf___Win32_Release64"
# PROP BASE Intermediate_Dir "vcbf___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres64.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:IA64

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcbf___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "vcbf___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcbf___Win32_Linux_Release"
# PROP Intermediate_Dir "vcbf___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD : /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib iiutil.lib compat.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcbf___Win32_Linux_Debug"
# PROP BASE Intermediate_Dir "vcbf___Win32_Linux_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcbf___Win32_Linux_Debug"
# PROP Intermediate_Dir "vcbf___Win32_Linux_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib util.lib config.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"msvcrtd" /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vcbf___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "vcbf___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcbf___Win32_AIX_Release"
# PROP Intermediate_Dir "vcbf___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\config" /I "$(ING_SRC)\front\st\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib config.lib util.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib util.lib compat.lib config.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\front\lib"

!ENDIF 

# Begin Target

# Name "vcbf - Win32 Release"
# Name "vcbf - Win32 Debug"
# Name "vcbf - Win32 EDBC Debug"
# Name "vcbf - Win32 EDBC Release"
# Name "vcbf - Win32 DBCS Release"
# Name "vcbf - Win32 DBCS Debug"
# Name "vcbf - Win32 Solaris_Release"
# Name "vcbf - Win32 HPUX_Release"
# Name "vcbf - Win32 HPUX_Debug"
# Name "vcbf - Win32 Debug64"
# Name "vcbf - Win32 Release64"
# Name "vcbf - Win32 Linux_Release"
# Name "vcbf - Win32 Linux_Debug"
# Name "vcbf - Win32 AIX_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\addvnode.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\brdpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\brdprdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\brdvndlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachedlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachedoc.CPP

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachedrv.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachefrm.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachelst.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cacheprm.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachetab.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachevi1.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cachevi2.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cbfdisp.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cbfitem.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cbfprdta.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cconfdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CLEFTDLG.CPP

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\confdoc.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\conffrm.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ConfigLeftProperties.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\confvr.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\crightdg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CuDlgLogFileAdd.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dbasedlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\derivdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DlgLogSz.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DLGVDOC.CPP

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\DLGVFRM.CPP

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\editlnet.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\editlsco.CPP

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\editlsdp.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\editlsgn.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\FindDlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GenericStartup.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\histdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\infpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\intprdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdbpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\layeddlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lgsdrdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lgsprdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lksdrdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lksprdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LL.CPP

!IF  "$(CFG)" == "vcbf - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

# SUBTRACT CPP /D "NT_GENERIC" /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

# SUBTRACT CPP /D "NT_GENERIC" /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\LogFiDlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\logfsdlg.CPP

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lvleft.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\msqpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mtabdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nampadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\netpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\netprdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\netrgdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\odbpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\orapadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\paramdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\peditctl.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\prefdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\recdrdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\recpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\seauddlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\seaurdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\secdlgsy.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\secmech.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sedrvdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\selogdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\seoptdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\stadrdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\staprdlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sybpadlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vcbf.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vcbf.rc
# End Source File
# Begin Source File

SOURCE=.\vcbfdoc.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vcbfview.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WaitDlg.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\WaitDlgProgress.cpp

!IF  "$(CFG)" == "vcbf - Win32 Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 EDBC Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Solaris_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 HPUX_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Debug64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Release64"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Release"

!ELSEIF  "$(CFG)" == "vcbf - Win32 Linux_Debug"

!ELSEIF  "$(CFG)" == "vcbf - Win32 AIX_Release"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\addvnode.h
# End Source File
# Begin Source File

SOURCE=.\brdpadlg.h
# End Source File
# Begin Source File

SOURCE=.\brdprdlg.h
# End Source File
# Begin Source File

SOURCE=.\brdvndlg.h
# End Source File
# Begin Source File

SOURCE=.\cachedlg.h
# End Source File
# Begin Source File

SOURCE=.\cachedoc.H
# End Source File
# Begin Source File

SOURCE=.\cachedrv.h
# End Source File
# Begin Source File

SOURCE=.\cachefrm.h
# End Source File
# Begin Source File

SOURCE=.\cachelst.h
# End Source File
# Begin Source File

SOURCE=.\cacheprm.h
# End Source File
# Begin Source File

SOURCE=.\cachetab.h
# End Source File
# Begin Source File

SOURCE=.\cachevi1.h
# End Source File
# Begin Source File

SOURCE=.\cachevi2.h
# End Source File
# Begin Source File

SOURCE=.\cbfdisp.h
# End Source File
# Begin Source File

SOURCE=.\cbfitem.h
# End Source File
# Begin Source File

SOURCE=.\cbfprdta.h
# End Source File
# Begin Source File

SOURCE=.\cconfdlg.h
# End Source File
# Begin Source File

SOURCE=.\CLEFTDLG.H
# End Source File
# Begin Source File

SOURCE=.\confdoc.h
# End Source File
# Begin Source File

SOURCE=.\conffrm.h
# End Source File
# Begin Source File

SOURCE=.\ConfigLeftProperties.h
# End Source File
# Begin Source File

SOURCE=.\confvr.h
# End Source File
# Begin Source File

SOURCE=.\crightdg.h
# End Source File
# Begin Source File

SOURCE=.\cudlglogfileadd.h
# End Source File
# Begin Source File

SOURCE=.\dbasedlg.h
# End Source File
# Begin Source File

SOURCE=.\derivdlg.h
# End Source File
# Begin Source File

SOURCE=.\DlgLogSz.h
# End Source File
# Begin Source File

SOURCE=.\DLGVDOC.H
# End Source File
# Begin Source File

SOURCE=.\DLGVFRM.H
# End Source File
# Begin Source File

SOURCE=.\editlnet.h
# End Source File
# Begin Source File

SOURCE=.\editlsco.H
# End Source File
# Begin Source File

SOURCE=.\editlsdp.h
# End Source File
# Begin Source File

SOURCE=.\editlsgn.h
# End Source File
# Begin Source File

SOURCE=.\FindDlg.h
# End Source File
# Begin Source File

SOURCE=.\GenericStartup.h
# End Source File
# Begin Source File

SOURCE=.\histdlg.h
# End Source File
# Begin Source File

SOURCE=.\infpadlg.h
# End Source File
# Begin Source File

SOURCE=.\intprdlg.h
# End Source File
# Begin Source File

SOURCE=.\jdbpadlg.h
# End Source File
# Begin Source File

SOURCE=.\layeddlg.h
# End Source File
# Begin Source File

SOURCE=.\lgsdrdlg.h
# End Source File
# Begin Source File

SOURCE=.\lgsprdlg.h
# End Source File
# Begin Source File

SOURCE=.\lksdrdlg.h
# End Source File
# Begin Source File

SOURCE=.\lksprdlg.h
# End Source File
# Begin Source File

SOURCE=.\ll.h
# End Source File
# Begin Source File

SOURCE=.\LogFiDlg.h
# End Source File
# Begin Source File

SOURCE=.\logfsdlg.H
# End Source File
# Begin Source File

SOURCE=.\lvleft.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\msqpadlg.h
# End Source File
# Begin Source File

SOURCE=.\mtabdlg.h
# End Source File
# Begin Source File

SOURCE=.\nampadlg.h
# End Source File
# Begin Source File

SOURCE=.\netpadlg.h
# End Source File
# Begin Source File

SOURCE=.\netprdlg.h
# End Source File
# Begin Source File

SOURCE=.\netrgdlg.h
# End Source File
# Begin Source File

SOURCE=.\odbpadlg.h
# End Source File
# Begin Source File

SOURCE=.\orapadlg.h
# End Source File
# Begin Source File

SOURCE=.\paramdlg.h
# End Source File
# Begin Source File

SOURCE=.\peditctl.h
# End Source File
# Begin Source File

SOURCE=.\prefdlg.h
# End Source File
# Begin Source File

SOURCE=.\recdrdlg.h
# End Source File
# Begin Source File

SOURCE=.\recpadlg.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\seauddlg.h
# End Source File
# Begin Source File

SOURCE=.\seaurdlg.h
# End Source File
# Begin Source File

SOURCE=.\secdlgsy.h
# End Source File
# Begin Source File

SOURCE=.\secmech.h
# End Source File
# Begin Source File

SOURCE=.\sedrvdlg.h
# End Source File
# Begin Source File

SOURCE=.\selogdlg.h
# End Source File
# Begin Source File

SOURCE=.\seoptdlg.h
# End Source File
# Begin Source File

SOURCE=.\stadrdlg.h
# End Source File
# Begin Source File

SOURCE=.\staprdlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\sybpadlg.h
# End Source File
# Begin Source File

SOURCE=.\vcbf.h
# End Source File
# Begin Source File

SOURCE=.\vcbfdoc.h
# End Source File
# Begin Source File

SOURCE=.\vcbfview.h
# End Source File
# Begin Source File

SOURCE=.\WaitDlg.h
# End Source File
# Begin Source File

SOURCE=.\waitdlgprogress.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\RES\check.bmp
# End Source File
# Begin Source File

SOURCE=.\res\green.bmp
# End Source File
# Begin Source File

SOURCE=.\res\red.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vcbf.ico
# End Source File
# Begin Source File

SOURCE=.\res\vcbf.rc2
# End Source File
# Begin Source File

SOURCE=.\res\vcbfdoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\yellow.bmp
# End Source File
# End Group
# Begin Group "Library Files"

# PROP Default_Filter "*.lib"
# Begin Source File

SOURCE=..\Lib\libwctrl.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\Clock.avi
# End Source File
# Begin Source File

SOURCE=.\res\Copy.avi
# End Source File
# Begin Source File

SOURCE=.\res\Delete.avi
# End Source File
# Begin Source File

SOURCE=.\res\translog.avi
# End Source File
# End Target
# End Project
