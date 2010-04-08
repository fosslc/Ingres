# Microsoft Developer Studio Project File - Name="iipostinst" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=iipostinst - Win32 DebugAMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iipostinst.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iipostinst.mak" CFG="iipostinst - Win32 DebugAMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iipostinst - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 DebugSDK" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 ReleaseSDK" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 DBCS Release" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 DBCS Debug" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 DebugAMD64" (based on "Win32 (x86) Application")
!MESSAGE "iipostinst - Win32 ReleaseAMD64" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "iipostinst - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy Release\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy Debug\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iipostinst___Win32_Debug64"
# PROP BASE Intermediate_Dir "iipostinst___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /machine:IA64
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy Debug64\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Release64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iipostinst___Win32_Release64"
# PROP BASE Intermediate_Dir "iipostinst___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /machine:IA64
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy Release64\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugSDK"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iipostinst___Win32_DebugSDK"
# PROP BASE Intermediate_Dir "iipostinst___Win32_DebugSDK"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugSDK"
# PROP Intermediate_Dir "DebugSDK"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EVALUATION_RELEASE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy DebugSDK\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseSDK"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iipostinst___Win32_ReleaseSDK"
# PROP BASE Intermediate_Dir "iipostinst___Win32_ReleaseSDK"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseSDK"
# PROP Intermediate_Dir "ReleaseSDK"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "EVALUATION_RELEASE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ReleaseSDK\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iipostinst___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "iipostinst___Win32_DBCS_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCSRelease"
# PROP Intermediate_Dir "DBCSRelease"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "DOUBLEBYTE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy DBCSRelease\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iipostinst___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "iipostinst___Win32_DBCS_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCSDebug"
# PROP Intermediate_Dir "DBCSDebug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "DOUBLEBYTE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "DOUBLEBYTE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy DBCSDebug\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugAMD64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iipostinst___Win32_DebugAMD64"
# PROP BASE Intermediate_Dir "iipostinst___Win32_DebugAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugAMD64"
# PROP Intermediate_Dir "DebugAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /machine:IA64
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy DebugAMD64\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseAMD64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iipostinst___Win32_ReleaseAMD64"
# PROP BASE Intermediate_Dir "iipostinst___Win32_ReleaseAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseAMD64"
# PROP Intermediate_Dir "ReleaseAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "_AFX_NO_DAO_SUPPORT" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /machine:IA64
# ADD LINK32 Netapi32.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libc" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ReleaseAMD64\ingconfig.exe %II_SYSTEM%\ingres\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "iipostinst - Win32 Release"
# Name "iipostinst - Win32 Debug"
# Name "iipostinst - Win32 Debug64"
# Name "iipostinst - Win32 Release64"
# Name "iipostinst - Win32 DebugSDK"
# Name "iipostinst - Win32 ReleaseSDK"
# Name "iipostinst - Win32 DBCS Release"
# Name "iipostinst - Win32 DBCS Debug"
# Name "iipostinst - Win32 DebugAMD64"
# Name "iipostinst - Win32 ReleaseAMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\256bmp.cpp

!IF  "$(CFG)" == "iipostinst - Win32 Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Release64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugAMD64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseAMD64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\iipostinst.cpp

!IF  "$(CFG)" == "iipostinst - Win32 Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Release64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugAMD64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseAMD64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\iipostinst.rc
# End Source File
# Begin Source File

SOURCE=.\install.cpp

!IF  "$(CFG)" == "iipostinst - Win32 Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Release64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugAMD64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseAMD64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PostInst.cpp

!IF  "$(CFG)" == "iipostinst - Win32 Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Release64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugAMD64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseAMD64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\PSheet.cpp

!IF  "$(CFG)" == "iipostinst - Win32 Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Release64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseSDK"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Release"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Debug"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugAMD64"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseAMD64"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "iipostinst - Win32 Release"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Debug64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 Release64"

# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugSDK"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseSDK"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Release"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DBCS Debug"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 DebugAMD64"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ELSEIF  "$(CFG)" == "iipostinst - Win32 ReleaseAMD64"

# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\256Bmp.h
# End Source File
# Begin Source File

SOURCE=.\iipostinst.h
# End Source File
# Begin Source File

SOURCE=.\install.h
# End Source File
# Begin Source File

SOURCE=.\PostInst.h
# End Source File
# Begin Source File

SOURCE=.\PSheet.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Block01.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Block05.bmp
# End Source File
# Begin Source File

SOURCE=.\res\iipostinst.ico
# End Source File
# Begin Source File

SOURCE=.\res\iipostinst.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ingsplash.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\clock.avi
# End Source File
# End Target
# End Project
