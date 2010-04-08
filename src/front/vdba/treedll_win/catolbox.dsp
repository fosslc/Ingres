# Microsoft Developer Studio Project File - Name="catolbox" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=catolbox - Win32 HPUX_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "catolbox.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "catolbox.mak" CFG="catolbox - Win32 HPUX_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "catolbox - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 EDBC Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 EDBC Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 DBCS Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 DBCS Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 Solaris_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 HPUX_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 HPUX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 Linux_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "catolbox - Win32 AIX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "catolbox - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\catolbox.dll" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\catolbox.dll %II_SYSTEM%\ingres\vdba	copy Release\catolbox.lib  %ING_SRC%\front\lib	copy Release\catolbox.lib  ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "catolbox - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 lbtreelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\catolbox.dll" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\catolbox.dll %II_SYSTEM%\ingres\vdba	copy Debug\catolbox.lib  %ING_SRC%\front\lib	copy Debug\catolbox.lib  ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "catolbox - Win32 EDBC Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "catolbox"
# PROP BASE Intermediate_Dir "catolbox"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "catolbox"
# PROP Intermediate_Dir "catolbox"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "EDBC" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "EDBC" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "EDBC"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "EDBC"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\catolbox.dll" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT...
PostBuild_Cmds=copy ..\App\catolbox.dll %EDBC_ROOT%\edbc\vdba	copy Release\catolbox.lib  %ING_SRC%\front\lib	copy Release\catolbox.lib  ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "catolbox - Win32 EDBC Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "catolbo0"
# PROP BASE Intermediate_Dir "catolbo0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "catolbo0"
# PROP Intermediate_Dir "catolbo0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "EDBC" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "EDBC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "EDBC"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "EDBC"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 lbtreelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"..\App\catolbox.dll" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to EDBC_ROOT..
PostBuild_Cmds=copy ..\App\catolbox.dll %EDBC_ROOT%\edbc\vdba	copy Release\catolbox.lib  %ING_SRC%\front\lib	copy Release\catolbox.lib  ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "catolbox - Win32 DBCS Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DBCSDebug"
# PROP BASE Intermediate_Dir "DBCSDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_MBCS" /D "DOUBLEBYTE" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 lbtreelp.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"..\App\catolbox.dll" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\catolbox.dll %II_SYSTEM%\ingres\vdba	copy "DBCS Debug\catolbox.lib"  %ING_SRC%\front\lib	copy "DBCS Debug\catolbox.lib"  ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "catolbox - Win32 DBCS Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DBCSRelease"
# PROP BASE Intermediate_Dir "DBCSRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_MBCS" /D "DOUBLEBYTE" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"..\App\catolbox.dll" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\catolbox.dll %II_SYSTEM%\ingres\vdba	copy "DBCS Release\catolbox.lib"  %ING_SRC%\front\lib	copy "DBCS Release\catolbox.lib"  ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "catolbox - Win32 Solaris_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "catolbox___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "catolbox___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "catolbox___Win32_Solaris_Release"
# PROP Intermediate_Dir "catolbox___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/catolbox.dll"
# ADD LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/catolbox.dll"

!ELSEIF  "$(CFG)" == "catolbox - Win32 HPUX_Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "catolbox___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "catolbox___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "catolbox___Win32_HPUX_Debug"
# PROP Intermediate_Dir "catolbox___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "MW_MSCOMPATIBLE_VARIANT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(II_SYSTEM)\ingres\lib\lbtreelp.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /pdbtype:sept

!ELSEIF  "$(CFG)" == "catolbox - Win32 HPUX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "catolbox___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "catolbox___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "catolbox___Win32_HPUX_Release"
# PROP Intermediate_Dir "catolbox___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/catolbox.dll"
# ADD LINK32 $(II_SYSTEM)\ingres\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"winspool.lib odbccp32"

!ELSEIF  "$(CFG)" == "catolbox - Win32 Linux_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "catolbox___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "catolbox___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "catolbox___Win32_Linux_Release"
# PROP Intermediate_Dir "catolbox___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/catolbox.dll"
# ADD LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/catolbox.dll"

!ELSEIF  "$(CFG)" == "catolbox - Win32 AIX_Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "catolbox___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "catolbox___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "catolbox___Win32_AIX_Release"
# PROP Intermediate_Dir "catolbox___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL" /D "_AFXDLL" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/catolbox.dll"
# ADD LINK32 ..\lib\lbtreelp.lib /nologo /subsystem:windows /dll /machine:I386 /out:"../app/catolbox.dll"

!ENDIF 

# Begin Target

# Name "catolbox - Win32 Release"
# Name "catolbox - Win32 Debug"
# Name "catolbox - Win32 EDBC Release"
# Name "catolbox - Win32 EDBC Debug"
# Name "catolbox - Win32 DBCS Debug"
# Name "catolbox - Win32 DBCS Release"
# Name "catolbox - Win32 Solaris_Release"
# Name "catolbox - Win32 HPUX_Debug"
# Name "catolbox - Win32 HPUX_Release"
# Name "catolbox - Win32 Linux_Release"
# Name "catolbox - Win32 AIX_Release"
# Begin Source File

SOURCE=.\folder.bmp
# End Source File
# Begin Source File

SOURCE=.\icon_clo.ico
# End Source File
# Begin Source File

SOURCE=.\icon_ope.ico
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\tree_dll.cpp
# End Source File
# Begin Source File

SOURCE=.\tree_dll.def
# End Source File
# Begin Source File

SOURCE=.\tree_dll.rc
# End Source File
# End Target
# End Project
