# Microsoft Developer Studio Project File - Name="vcda" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vcda - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vcda.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vcda.mak" CFG="vcda - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vcda - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vcda - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Xvcda", BEMAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vcda - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"../app/vcda.exe" /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vcda.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcda - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../app/vcda.exe" /pdbtype:sept /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vcda.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "vcda - Win32 Release"
# Name "vcda - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\udlgmain.cpp
# End Source File
# Begin Source File

SOURCE=.\vcda.cpp
# End Source File
# Begin Source File

SOURCE=.\vcda.rc
# End Source File
# Begin Source File

SOURCE=.\vcdactrl.cpp
# End Source File
# Begin Source File

SOURCE=.\vcdadoc.cpp
# End Source File
# Begin Source File

SOURCE=.\vcdaview.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\udlgmain.h
# End Source File
# Begin Source File

SOURCE=.\vcda.h
# End Source File
# Begin Source File

SOURCE=.\vcdactrl.h
# End Source File
# Begin Source File

SOURCE=.\vcdadoc.h
# End Source File
# Begin Source File

SOURCE=.\vcdaview.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\about.bmp
# End Source File
# Begin Source File

SOURCE=.\res\vcda.ico
# End Source File
# Begin Source File

SOURCE=.\res\vcda.rc2
# End Source File
# Begin Source File

SOURCE=.\res\vcdadoc.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Target
# End Project
# Section vcda : {EAF345E8-D514-11D6-87EA-00C04F1F754A}
# 	2:5:Class:CuVcda
# 	2:10:HeaderFile:vcdactrl.h
# 	2:8:ImplFile:vcdactrl.cpp
# End Section
# Section vcda : {EAF345EA-D514-11D6-87EA-00C04F1F754A}
# 	2:21:DefaultSinkHeaderFile:vcdactrl.h
# 	2:16:DefaultSinkClass:CuVcda
# End Section
