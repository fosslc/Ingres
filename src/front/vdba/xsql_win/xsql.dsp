# Microsoft Developer Studio Project File - Name="xsql" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=xsql - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xsql.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xsql.mak" CFG="xsql - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xsql - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "xsql - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "xsql - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "xsql - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE "xsql - Win32 EDBC_Debug" (based on "Win32 (x86) Application")
!MESSAGE "xsql - Win32 EDBC_Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/COM/Xsql", JPLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xsql - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 ingres.lib libingll.lib libguids.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdbasql.exe" /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vdbasql.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "xsql - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib libingll.lib libguids.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\vdbasql.exe" /pdbtype:sept /libpath:"..\lib" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vdbasql.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "xsql - Win32 Unicode Debug"

# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\vdbasql.exe" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 ingres.lib libingll.lib libguids.lib ws2_32.lib htmlhelp.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /out:"..\App\vdbasql.exe" /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vdbasql.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "xsql - Win32 Unicode Release"

# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libingll.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdbasql.exe" /libpath:"..\lib"
# ADD LINK32 ingres.lib libingll.lib libguids.lib ws2_32.lib htmlhelp.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386 /out:"..\App\vdbasql.exe" /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\app\vdbasql.exe %II_SYSTEM%\ingres\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "xsql - Win32 EDBC_Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xsql___Win32_EDBC_Debug"
# PROP BASE Intermediate_Dir "xsql___Win32_EDBC_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\inc" /I "..\inc4res" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libingll.lib libguids.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\vdbasql.exe" /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 edbc.lib libingll.lib libguids.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\App\edbquery.exe" /pdbtype:sept /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbquery.exe %EDBC_ROOT\edbc\vdba
# End Special Build Tool

!ELSEIF  "$(CFG)" == "xsql - Win32 EDBC_Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "xsql___Win32_EDBC_Release"
# PROP BASE Intermediate_Dir "xsql___Win32_EDBC_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\inc" /I "..\inc4res" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_AFXDLL" /D "EDBC" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "..\inc4res" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "..\inc4res" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ingres.lib libingll.lib libguids.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\vdbasql.exe" /libpath:"..\lib"
# ADD LINK32 edbc.lib libingll.lib libguids.lib ws2_32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:"..\App\edbquery.exe" /libpath:"..\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to EDBC_ROOT...
PostBuild_Cmds=copy ..\app\edbquery.exe %EDBC_ROOT\edbc\vdba
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "xsql - Win32 Release"
# Name "xsql - Win32 Debug"
# Name "xsql - Win32 Unicode Debug"
# Name "xsql - Win32 Unicode Release"
# Name "xsql - Win32 EDBC_Debug"
# Name "xsql - Win32 EDBC_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\axsqledb.cpp
# End Source File
# Begin Source File

SOURCE=.\edbcdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\edbcfram.cpp
# End Source File
# Begin Source File

SOURCE=.\font.cpp
# End Source File
# Begin Source File

SOURCE=.\inputpwd.cpp
# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\sqlquery.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\xsql.cpp
# End Source File
# Begin Source File

SOURCE=.\xsql.rc
# End Source File
# Begin Source File

SOURCE=.\xsqldml.cpp
# End Source File
# Begin Source File

SOURCE=.\xsqldoc.cpp
# End Source File
# Begin Source File

SOURCE=.\xsqlview.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\axsqledb.h
# End Source File
# Begin Source File

SOURCE=.\edbcdoc.h
# End Source File
# Begin Source File

SOURCE=.\edbcfram.h
# End Source File
# Begin Source File

SOURCE=.\font.h
# End Source File
# Begin Source File

SOURCE=.\inputpwd.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\sqlquery.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\xsql.h
# End Source File
# Begin Source File

SOURCE=.\xsqldml.h
# End Source File
# Begin Source File

SOURCE=.\xsqldoc.h
# End Source File
# Begin Source File

SOURCE=.\xsqlview.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\autocom0.ico
# End Source File
# Begin Source File

SOURCE=.\res\autocom1.ico
# End Source File
# Begin Source File

SOURCE=.\res\edbc.ico
# End Source File
# Begin Source File

SOURCE=.\res\hotedbcf.bmp
# End Source File
# Begin Source File

SOURCE=.\res\hotmainf.bmp
# End Source File
# Begin Source File

SOURCE=.\res\mainfram.bmp
# End Source File
# Begin Source File

SOURCE=.\res\readlck0.ico
# End Source File
# Begin Source File

SOURCE=.\res\readlck1.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\xsql.ico
# End Source File
# Begin Source File

SOURCE=.\res\xsql.rc2
# End Source File
# Begin Source File

SOURCE=.\res\xsqldoc.ico
# End Source File
# End Group
# End Target
# End Project
# Section xsql : {634C383D-A069-11D5-8769-00C04F1F754A}
# 	2:21:DefaultSinkHeaderFile:sqlquery.h
# 	2:16:DefaultSinkClass:CuSqlquery
# End Section
# Section xsql : {7A9941A3-9987-4914-AB55-F7F630CF8B38}
# 	2:21:DefaultSinkHeaderFile:axsqledb.h
# 	2:16:DefaultSinkClass:CuSqlqueryEdbc
# End Section
# Section xsql : {BEF6E003-A874-101A-8BBA-00AA00300CAB}
# 	2:5:Class:COleFont
# 	2:10:HeaderFile:font.h
# 	2:8:ImplFile:font.cpp
# End Section
# Section xsql : {634C383B-A069-11D5-8769-00C04F1F754A}
# 	2:5:Class:CuSqlquery
# 	2:10:HeaderFile:sqlquery.h
# 	2:8:ImplFile:sqlquery.cpp
# End Section
# Section xsql : {C026183C-1383-40EB-8A70-DD05B3073091}
# 	2:5:Class:CuSqlqueryEdbc
# 	2:10:HeaderFile:axsqledb.h
# 	2:8:ImplFile:axsqledb.cpp
# End Section
