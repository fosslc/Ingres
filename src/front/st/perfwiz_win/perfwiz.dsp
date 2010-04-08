# Microsoft Developer Studio Project File - Name="perfwiz" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=perfwiz - Win32 DebugAMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "perfwiz.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "perfwiz.mak" CFG="perfwiz - Win32 DebugAMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "perfwiz - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "perfwiz - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "perfwiz - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "perfwiz - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "perfwiz - Win32 ReleaseAMD64" (based on "Win32 (x86) Application")
!MESSAGE "perfwiz - Win32 DebugAMD64" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "perfwiz - Win32 Release"

# PROP BASE Use_MFC 5
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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi.lib $(II_SYSTEM)\ingres\lib\ingres.lib  /nologo /subsystem:windows /machine:I386 /libpath:""

!ELSEIF  "$(CFG)" == "perfwiz - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi.lib $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:""

!ELSEIF  "$(CFG)" == "perfwiz - Win32 Debug64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "perfwiz___Win32_Debug64"
# PROP BASE Intermediate_Dir "perfwiz___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi.lib $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi64.lib $(II_SYSTEM)\ingres\lib\ingres64.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"" /machine:IA64
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "perfwiz - Win32 Release64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "perfwiz___Win32_Release64"
# PROP BASE Intermediate_Dir "perfwiz___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi.lib $(II_SYSTEM)\ingres\lib\ingres.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi64.lib $(II_SYSTEM)\ingres\lib\ingres64.lib  /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcmt.lib" /libpath:"" /machine:IA64

!ELSEIF  "$(CFG)" == "perfwiz - Win32 ReleaseAMD64"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "perfwiz___Win32_ReleaseAMD64"
# PROP BASE Intermediate_Dir "perfwiz___Win32_ReleaseAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseAMD64"
# PROP Intermediate_Dir "ReleaseAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I  /D "WIN64" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /D "_AFXDLL" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi64.lib $(II_SYSTEM)\ingres\lib\ingres64.lib  /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcmt.lib" /libpath:"" /machine:IA64
# ADD LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi64.lib $(II_SYSTEM)\ingres\lib\ingres64.lib  /nologo /subsystem:windows /machine:I386 /nodefaultlib:"libcmt.lib" /libpath:"" /machine:AMD64
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "perfwiz - Win32 DebugAMD64"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "perfwiz___Win32_DebugAMD64"
# PROP BASE Intermediate_Dir "perfwiz___Win32_DebugAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugAMD64"
# PROP Intermediate_Dir "DebugAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I  /D "WIN64" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "NT_GENERIC" /Yu"stdafx.h" /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi64.lib $(II_SYSTEM)\ingres\lib\ingres64.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"" /machine:IA64
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 $(ING_SRC)\front\lib\pfctrs.lib $(II_SYSTEM)\ingres\lib\oiapi64.lib $(II_SYSTEM)\ingres\lib\ingres64.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"" /machine:AMD64
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "perfwiz - Win32 Release"
# Name "perfwiz - Win32 Debug"
# Name "perfwiz - Win32 Debug64"
# Name "perfwiz - Win32 Release64"
# Name "perfwiz - Win32 ReleaseAMD64"
# Name "perfwiz - Win32 DebugAMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\256bmp.cpp
# End Source File
# Begin Source File

SOURCE=.\CounterCreation.cpp
# End Source File
# Begin Source File

SOURCE=.\EditCounter.cpp
# End Source File
# Begin Source File

SOURCE=.\EditObject.cpp
# End Source File
# Begin Source File

SOURCE=.\FinalPage.cpp
# End Source File
# Begin Source File

SOURCE=.\gcnquery.c
# ADD CPP /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(II_SYSTEM)\ingres\files"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\gcnquery.sc

!IF  "$(CFG)" == "perfwiz - Win32 Release"

# Begin Custom Build
InputPath=.\gcnquery.sc

"gcnquery.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc.exe gcnquery.sc

# End Custom Build

!ELSEIF  "$(CFG)" == "perfwiz - Win32 Debug"

# Begin Custom Build
InputPath=.\gcnquery.sc

"gcnquery.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%II_SYSTEM%\ingres\bin\esqlc.exe gcnquery.sc

# End Custom Build

!ELSEIF  "$(CFG)" == "perfwiz - Win32 Debug64"

# Begin Custom Build
InputPath=.\gcnquery.sc

"gcnquery.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc.exe gcnquery.sc

# End Custom Build

!ELSEIF  "$(CFG)" == "perfwiz - Win32 Release64"

# Begin Custom Build
InputPath=.\gcnquery.sc

"gcnquery.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc.exe gcnquery.sc

# End Custom Build

!ELSEIF  "$(CFG)" == "perfwiz - Win32 ReleaseAMD64"

# Begin Custom Build
InputPath=.\gcnquery.sc

"gcnquery.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc.exe gcnquery.sc

# End Custom Build

!ELSEIF  "$(CFG)" == "perfwiz - Win32 DebugAMD64"

# Begin Custom Build
InputPath=.\gcnquery.sc

"gcnquery.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc.exe gcnquery.sc

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\InPlaceEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\IntroPage.cpp
# End Source File
# Begin Source File

SOURCE=.\NewListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\perfwiz.cpp
# End Source File
# Begin Source File

SOURCE=.\perfwiz.rc
# End Source File
# Begin Source File

SOURCE=.\PersonalCounter.cpp
# End Source File
# Begin Source File

SOURCE=.\PersonalGroup.cpp
# End Source File
# Begin Source File

SOURCE=.\PersonalHelp.cpp
# End Source File
# Begin Source File

SOURCE=.\pinggcn.c
# ADD CPP /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\common\gcf\hdr" /I "$(ING_SRC)\front\st\hdr"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\PropSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerSelect.cpp
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
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\256Bmp.h
# End Source File
# Begin Source File

SOURCE=.\CounterCreation.h
# End Source File
# Begin Source File

SOURCE=.\EditCounter.h
# End Source File
# Begin Source File

SOURCE=.\EditObject.h
# End Source File
# Begin Source File

SOURCE=.\FinalPage.h
# End Source File
# Begin Source File

SOURCE=.\InPlaceEdit.h
# End Source File
# Begin Source File

SOURCE=.\IntroPage.h
# End Source File
# Begin Source File

SOURCE=.\NewListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\perfchart.h
# End Source File
# Begin Source File

SOURCE=.\perfwiz.h
# End Source File
# Begin Source File

SOURCE=.\PersonalCounter.h
# End Source File
# Begin Source File

SOURCE=.\PersonalGroup.h
# End Source File
# Begin Source File

SOURCE=.\PersonalHelp.h
# End Source File
# Begin Source File

SOURCE=.\PropSheet.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ServerSelect.h
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

SOURCE=.\res\LV_StateImage.bmp
# End Source File
# Begin Source File

SOURCE=.\res\nodetree.bmp
# End Source File
# Begin Source File

SOURCE=.\res\perfwiz.ico
# End Source File
# Begin Source File

SOURCE=.\res\perfwiz.rc2
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
