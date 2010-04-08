# Microsoft Developer Studio Project File - Name="iilink" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=iilink - Win32 DebugAMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iilink.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iilink.mak" CFG="iilink - Win32 DebugAMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iilink - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "iilink - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "iilink - Win32 DBCS Release" (based on "Win32 (x86) Application")
!MESSAGE "iilink - Win32 DBCS Debug" (based on "Win32 (x86) Application")
!MESSAGE "iilink - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "iilink - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "iilink - Win32 DebugAMD64" (based on "Win32 (x86) Application")
!MESSAGE "iilink - Win32 ReleaseAMD64" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "iilink - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib  /nologo /subsystem:windows /machine:I386 /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\utils"

!ELSEIF  "$(CFG)" == "iilink - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\utils"

!ELSEIF  "$(CFG)" == "iilink - Win32 DBCS Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iilink___Win32_DBCS_Release"
# PROP BASE Intermediate_Dir "iilink___Win32_DBCS_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS_Release"
# PROP Intermediate_Dir "DBCS_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "NDEBUG" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "iilink - Win32 DBCS Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iilink___Win32_DBCS_Debug"
# PROP BASE Intermediate_Dir "iilink___Win32_DBCS_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS_Debug"
# PROP Intermediate_Dir "DBCS_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "_DEBUG" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "iilink - Win32 Release64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iilink___Win32_Release64"
# PROP BASE Intermediate_Dir "iilink___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "NDEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "NT_IA64" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt64.lib $(ING_SRC)\common\lib\iigcfnt64.lib  /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcmt.lib" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\i64_win\utils" /machine:IA64

!ELSEIF  "$(CFG)" == "iilink - Win32 Debug64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iilink___Win32_Debug64"
# PROP BASE Intermediate_Dir "iilink___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "NT_IA64" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt.lib $(ING_SRC)\common\lib\iigcfnt.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt64.lib $(ING_SRC)\common\lib\iigcfnt64.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\i64_win\utils" /machine:IA64

!ELSEIF  "$(CFG)" == "iilink - Win32 DebugAMD64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iilink___Win32_DebugAMD64"
# PROP BASE Intermediate_Dir "iilink___Win32_DebugAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugAMD64"
# PROP Intermediate_Dir "DebugAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "NT_AMD64" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt64.lib $(ING_SRC)\common\lib\iigcfnt64.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\i64_win\utils" /machine:IA64
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt64.lib $(ING_SRC)\common\lib\iigcfnt64.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\a64_win\utils" /machine:AMD64
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "iilink - Win32 ReleaseAMD64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iilink___Win32_ReleaseAMD64"
# PROP BASE Intermediate_Dir "iilink___Win32_ReleaseAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseAMD64"
# PROP Intermediate_Dir "ReleaseAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "NDEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "NDEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "NT_AMD64" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt64.lib $(ING_SRC)\common\lib\iigcfnt64.lib  /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcmt.lib" /libpath:"" /machine:IA64
# ADD LINK32 version.lib $(ING_SRC)\cl\lib\iiclfnt64.lib $(ING_SRC)\common\lib\iigcfnt64.lib  /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcmt.lib" /libpath:"" /machine:AMD64
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "iilink - Win32 Release"
# Name "iilink - Win32 Debug"
# Name "iilink - Win32 DBCS Release"
# Name "iilink - Win32 DBCS Debug"
# Name "iilink - Win32 Release64"
# Name "iilink - Win32 Debug64"
# Name "iilink - Win32 DebugAMD64"
# Name "iilink - Win32 ReleaseAMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\256bmp.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseDir.cpp
# End Source File
# Begin Source File

SOURCE=.\FailDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FinalPage.cpp
# End Source File
# Begin Source File

SOURCE=.\iilink.cpp
# End Source File
# Begin Source File

SOURCE=.\iilink.rc
# End Source File
# Begin Source File

SOURCE=.\MainPage.cpp
# End Source File
# Begin Source File

SOURCE=.\OptionsPage.cpp
# End Source File
# Begin Source File

SOURCE=.\pinggcn.c
# ADD CPP /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\st\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\common\gcf\hdr"
# SUBTRACT CPP /YX /Yc /Yu
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

SOURCE=.\UserUdts.cpp
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

SOURCE=.\ChooseDir.h
# End Source File
# Begin Source File

SOURCE=.\FailDlg.h
# End Source File
# Begin Source File

SOURCE=.\FinalPage.h
# End Source File
# Begin Source File

SOURCE=.\iilink.h
# End Source File
# Begin Source File

SOURCE=.\MainPage.h
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

SOURCE=.\UserUdts.h
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

SOURCE=.\res\Block02.bmp
# End Source File
# Begin Source File

SOURCE=.\res\choosedi.bmp
# End Source File
# Begin Source File

SOURCE=.\res\excl.ico
# End Source File
# Begin Source File

SOURCE=.\res\iilink.ico
# End Source File
# Begin Source File

SOURCE=.\res\iilink.rc2
# End Source File
# Begin Source File

SOURCE=.\res\LV_StateImage.bmp
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
