# Microsoft Developer Studio Project File - Name="enterprise" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=enterprise - Win32 DebugAMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "enterprise.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "enterprise.mak" CFG="enterprise - Win32 DebugAMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "enterprise - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "enterprise - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "enterprise - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "enterprise - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "enterprise - Win32 ReleaseAMD64" (based on "Win32 (x86) Application")
!MESSAGE "enterprise - Win32 DebugAMD64" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "enterprise - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 lic98mt.lib msi.lib version.lib /nologo /subsystem:windows /machine:I386 /out:"Release/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\utils"

!ELSEIF  "$(CFG)" == "enterprise - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 lic98mtdbg.lib msi.lib version.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/PatchUpdate.exe" /pdbtype:sept /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\utils"

!ELSEIF  "$(CFG)" == "enterprise - Win32 Debug64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "enterprise___Win32_Debug64"
# PROP BASE Intermediate_Dir "enterprise___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msi.lib lic98mtdbg.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/PatchUpdate.exe" /pdbtype:sept /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\utils"
# ADD LINK32 lic98mtdbg.lib msi.lib version.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug64/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\i64_win\utils" /machine:IA64

!ELSEIF  "$(CFG)" == "enterprise - Win32 Release64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "enterprise___Win32_Release64"
# PROP BASE Intermediate_Dir "enterprise___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 msi.lib lic98mt.lib /nologo /subsystem:windows /machine:I386 /out:"Release/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\utils"
# ADD LINK32 lic98mt.lib msi.lib version.lib /nologo /subsystem:windows /machine:I386 /out:"Release64/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\i64_win\utils" /machine:IA64

!ELSEIF  "$(CFG)" == "enterprise - Win32 ReleaseAMD64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "enterprise___Win32_ReleaseAMD64"
# PROP BASE Intermediate_Dir "enterprise___Win32_ReleaseAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseAMD64"
# PROP Intermediate_Dir "ReleaseAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Wp64 /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98mt.lib msi.lib version.lib /nologo /subsystem:windows /machine:I386 /out:"Release/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\i64_win\utils" /machine:IA64
# ADD LINK32 lic98mt.lib msi.lib version.lib /nologo /subsystem:windows /machine:I386 /out:"ReleaseAMD64/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\a64_win\utils" /machine:AMD64
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "enterprise - Win32 DebugAMD64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "enterprise___Win32_DebugAMD64"
# PROP BASE Intermediate_Dir "enterprise___Win32_DebugAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugAMD64"
# PROP Intermediate_Dir "DebugAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Wp64 /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\treasure\NT\CA_LIC\98\h" /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 lic98mtdbg.lib msi.lib version.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\i64_win\utils" /machine:IA64
# ADD LINK32 lic98mtdbg.lib msi.lib version.lib /nologo /subsystem:windows /debug /machine:I386 /out:"DebugAMD64/PatchUpdate.exe" /libpath:"$(ING_SRC)\treasure\NT\CA_LIC\98\a64_win\utils" /machine:AMD64
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "enterprise - Win32 Release"
# Name "enterprise - Win32 Debug"
# Name "enterprise - Win32 Debug64"
# Name "enterprise - Win32 Release64"
# Name "enterprise - Win32 ReleaseAMD64"
# Name "enterprise - Win32 DebugAMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\256bmp.cpp
# End Source File
# Begin Source File

SOURCE=.\enterprise.cpp
# End Source File
# Begin Source File

SOURCE=.\enterprise.rc
# End Source File
# Begin Source File

SOURCE=.\InstallCode.cpp
# End Source File
# Begin Source File

SOURCE=.\PreInstallation.cpp
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

SOURCE=.\WaitDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Welcome.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\256bmp.h
# End Source File
# Begin Source File

SOURCE=.\enterprise.h
# End Source File
# Begin Source File

SOURCE=.\InstallCode.h
# End Source File
# Begin Source File

SOURCE=.\PreInstallation.h
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

SOURCE=.\WaitDlg.h
# End Source File
# Begin Source File

SOURCE=.\Welcome.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Block01.bmp
# End Source File
# Begin Source File

SOURCE=.\res\enterprise.ico
# End Source File
# Begin Source File

SOURCE=.\res\enterprise.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ingsplash.bmp
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
