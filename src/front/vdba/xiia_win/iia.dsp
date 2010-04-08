# Microsoft Developer Studio Project File - Name="iia" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=iia - Win32 DebugU
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "iia.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "iia.mak" CFG="iia - Win32 DebugU"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "iia - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 Solaris_Release" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 HPUX_Release" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 HPUX_Debug" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 Solaris_Debug" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 Linux_Release" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 AIX_Release" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 DebugU" (based on "Win32 (x86) Application")
!MESSAGE "iia - Win32 ReleaseU" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Xiia", DTHAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "iia - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\iia.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\iia.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iia - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\iia.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\iia.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iia - Win32 Solaris_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iia___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "iia___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "iia___Win32_Solaris_Release"
# PROP Intermediate_Dir "iia___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App/iia.exe" /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib nsl.lib elf.lib dl.lib socket.lib posix4.lib /nologo /subsystem:windows /machine:I386 /out:"..\App/iia.exe" /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "iia - Win32 HPUX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iia___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "iia___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "iia___Win32_HPUX_Release"
# PROP Intermediate_Dir "iia___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App/iia.exe" /libpath:"..\lib"
# ADD LINK32 libguids.lib libingll.lib ws2_32.lib q.1.lib compat.1.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /libpath:"..\lib" /libpath:"$(II_SYSTEM)\ingres\lib"

!ELSEIF  "$(CFG)" == "iia - Win32 HPUX_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iia___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "iia___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "iia___Win32_HPUX_Debug"
# PROP Intermediate_Dir "iia___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App/iia.exe" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 libguids.lib libingll.lib ws2_32.lib q.1.lib compat.1.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"winspool.lib odbccp32" /pdbtype:sept /libpath:"..\lib" /libpath:"$(II_SYSTEM)\ingres\lib"

!ELSEIF  "$(CFG)" == "iia - Win32 Solaris_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iia___Win32_Solaris_Debug1"
# PROP BASE Intermediate_Dir "iia___Win32_Solaris_Debug1"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "iia___Win32_Solaris_Debug1"
# PROP Intermediate_Dir "iia___Win32_Solaris_Debug1"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App/iia.exe" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib nsl.lib elf.lib dl.lib socket.lib posix4.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App/iia.exe" /pdbtype:sept /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "iia - Win32 Linux_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iia___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "iia___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "iia___Win32_Linux_Release"
# PROP Intermediate_Dir "iia___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App/iia.exe" /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App/iia.exe" /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "iia - Win32 AIX_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iia___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "iia___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "iia___Win32_AIX_Release"
# PROP Intermediate_Dir "iia___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "MW_MSCOMPATIBLE_VARIANT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App/iia.exe" /libpath:"..\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App/iia.exe" /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "iia - Win32 DebugU"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "iia___Win32_DebugU"
# PROP BASE Intermediate_Dir "iia___Win32_DebugU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "iia___Win32_DebugU"
# PROP Intermediate_Dir "iia___Win32_DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\hdr" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\iia.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\iia.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\iia.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "iia - Win32 ReleaseU"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "iia___Win32_ReleaseU"
# PROP BASE Intermediate_Dir "iia___Win32_ReleaseU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "iia___Win32_ReleaseU"
# PROP Intermediate_Dir "iia___Win32_ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\hdr" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\iia.exe" /libpath:"$(ING_SRC)\front\lib"
# ADD LINK32 ingres.lib libguids.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\iia.exe" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\iia.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "iia - Win32 Release"
# Name "iia - Win32 Debug"
# Name "iia - Win32 Solaris_Release"
# Name "iia - Win32 HPUX_Release"
# Name "iia - Win32 HPUX_Debug"
# Name "iia - Win32 Solaris_Debug"
# Name "iia - Win32 Linux_Release"
# Name "iia - Win32 AIX_Release"
# Name "iia - Win32 DebugU"
# Name "iia - Win32 ReleaseU"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\iia.cpp
# End Source File
# Begin Source File

SOURCE=.\iia.rc
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

SOURCE=.\view.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\iia.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\parsearg.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\view.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\iia.ico
# End Source File
# Begin Source File

SOURCE=.\res\iia.rc2
# End Source File
# End Group
# End Target
# End Project
