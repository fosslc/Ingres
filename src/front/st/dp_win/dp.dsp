# Microsoft Developer Studio Project File - Name="dp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=dp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dp.mak" CFG="dp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "dp - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dp - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 winmm.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "dp - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "dp - Win32 Release"
# Name "dp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\256bmp.cpp
# End Source File
# Begin Source File

SOURCE=.\AddFileName.cpp
# End Source File
# Begin Source File

SOURCE=.\AddStructName.cpp
# End Source File
# Begin Source File

SOURCE=.\AuthPage.cpp
# End Source File
# Begin Source File

SOURCE=.\dp.cpp
# End Source File
# Begin Source File

SOURCE=.\dp.rc
# End Source File
# Begin Source File

SOURCE=.\FailDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\InPlaceEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\LocPage.cpp
# End Source File
# Begin Source File

SOURCE=.\NewListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\Options2Page.cpp
# End Source File
# Begin Source File

SOURCE=.\Options3Page.cpp
# End Source File
# Begin Source File

SOURCE=.\Options4Page.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\PropSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\SuccessDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\WaitDlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\256Bmp.h
# End Source File
# Begin Source File

SOURCE=.\AddFileName.h
# End Source File
# Begin Source File

SOURCE=.\AddStructName.h
# End Source File
# Begin Source File

SOURCE=.\AuthPage.h
# End Source File
# Begin Source File

SOURCE=.\dp.h
# End Source File
# Begin Source File

SOURCE=.\FailDlg.h
# End Source File
# Begin Source File

SOURCE=.\InPlaceEdit.h
# End Source File
# Begin Source File

SOURCE=.\LocPage.h
# End Source File
# Begin Source File

SOURCE=.\NewListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\Options2Page.h
# End Source File
# Begin Source File

SOURCE=.\Options3Page.h
# End Source File
# Begin Source File

SOURCE=.\Options4Page.h
# End Source File
# Begin Source File

SOURCE=.\OptionsPage.h
# End Source File
# Begin Source File

SOURCE=.\PropSheet.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\SuccessDlg.h
# End Source File
# Begin Source File

SOURCE=.\WaitDlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Block01.bmp
# End Source File
# Begin Source File

SOURCE=.\res\dp.ico
# End Source File
# Begin Source File

SOURCE=.\res\dp.rc2
# End Source File
# Begin Source File

SOURCE=.\res\excl.ico
# End Source File
# Begin Source File

SOURCE=.\res\info.ico
# End Source File
# Begin Source File

SOURCE=.\res\Splash01.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\clock.avi
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
