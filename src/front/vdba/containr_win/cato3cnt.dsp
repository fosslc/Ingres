# Microsoft Developer Studio Project File - Name="cato3cnt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cato3cnt - Win32 HPUX_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cato3cnt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cato3cnt.mak" CFG="cato3cnt - Win32 HPUX_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cato3cnt - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cato3cnt - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cato3cnt - Win32 Solaris_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cato3cnt - Win32 HPUX_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cato3cnt - Win32 HPUX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cato3cnt - Win32 Linux_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cato3cnt - Win32 AIX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cato3cnt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 winspool.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /machine:I386 /out:"..\App\cato3cnt.dll" /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\cato3cnt.dll %II_SYSTEM%\ingres\vdba	copy Release\cato3cnt.lib %ING_SRC%\front\lib	copy Release\cato3cnt.lib ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "cato3cnt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winspool.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /debug /machine:I386 /out:"..\App\cato3cnt.dll" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to II_SYSTEM...
PostBuild_Cmds=copy ..\App\cato3cnt.dll %II_SYSTEM%\ingres\vdba	copy Debug\cato3cnt.lib %ING_SRC%\front\lib	copy Debug\cato3cnt.lib ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "cato3cnt - Win32 Solaris_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cato3cnt___Win32_Solaris_Release0"
# PROP BASE Intermediate_Dir "cato3cnt___Win32_Solaris_Release0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "cato3cnt___Win32_Solaris_Release0"
# PROP Intermediate_Dir "cato3cnt___Win32_Solaris_Release0"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /d "NDEBUG"
# ADD RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 cato3tim.lib cato3dat.lib cato3nbr.lib cato3msk.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/cato3cnt.dll" /libpath:"..\lib"
# ADD LINK32 winspool.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /machine:I386 /out:"../app/cato3cnt.dll" /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "cato3cnt - Win32 HPUX_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cato3cnt___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "cato3cnt___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "cato3cnt___Win32_HPUX_Debug"
# PROP Intermediate_Dir "cato3cnt___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /O1 /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "MW_MSCOMPATIBLE_VARIANT" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /d "_DEBUG"
# ADD RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 cato3tim.lib cato3dat.lib cato3nbr.lib cato3msk.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\lib"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "cato3cnt - Win32 HPUX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cato3cnt___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "cato3cnt___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "cato3cnt___Win32_HPUX_Release"
# PROP Intermediate_Dir "cato3cnt___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /D "MW_MSCOMPATIBLE_VARIANT" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /d "NDEBUG"
# ADD RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 cato3tim.lib cato3dat.lib cato3nbr.lib cato3msk.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/cato3cnt.dll" /libpath:"..\lib"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /machine:I386 /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "cato3cnt - Win32 Linux_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cato3cnt___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "cato3cnt___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "cato3cnt___Win32_Linux_Release"
# PROP Intermediate_Dir "cato3cnt___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /d "NDEBUG"
# ADD RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 cato3tim.lib cato3dat.lib cato3nbr.lib cato3msk.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/cato3cnt.dll" /libpath:"..\lib"
# ADD LINK32 winspool.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /machine:I386 /out:"../app/cato3cnt.dll" /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "cato3cnt - Win32 AIX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cato3cnt___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "cato3cnt___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "cato3cnt___Win32_AIX_Release"
# PROP Intermediate_Dir "cato3cnt___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "..\camask" /I "..\canum" /I "..\cadate" /I "..\catime" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CATO3CNT_EXPORTS" /D "REALIZER" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /d "NDEBUG"
# ADD RSC /l 0x40c /i "..\camask" /i "..\canum" /i "..\cadate" /i "..\catime" /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 cato3tim.lib cato3dat.lib cato3nbr.lib cato3msk.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/cato3cnt.dll" /libpath:"..\lib"
# ADD LINK32 winspool.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /machine:I386 /out:"../app/cato3cnt.dll" /libpath:"..\lib"

!ENDIF 

# Begin Target

# Name "cato3cnt - Win32 Release"
# Name "cato3cnt - Win32 Debug"
# Name "cato3cnt - Win32 Solaris_Release"
# Name "cato3cnt - Win32 HPUX_Debug"
# Name "cato3cnt - Win32 HPUX_Release"
# Name "cato3cnt - Win32 Linux_Release"
# Name "cato3cnt - Win32 AIX_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ca_prntf.c
# End Source File
# Begin Source File

SOURCE=.\catocntr.c
# End Source File
# Begin Source File

SOURCE=.\catocntr.rc
# End Source File
# Begin Source File

SOURCE=.\cnt32.def
# End Source File
# Begin Source File

SOURCE=.\cntapi.c
# End Source File
# Begin Source File

SOURCE=.\cntde32.c
# End Source File
# Begin Source File

SOURCE=.\cntdlg.c
# End Source File
# Begin Source File

SOURCE=.\cntdrag.c
# End Source File
# Begin Source File

SOURCE=.\cntinit.c
# End Source File
# Begin Source File

SOURCE=.\cntpaint.c
# End Source File
# Begin Source File

SOURCE=.\cntsplit.c
# End Source File
# Begin Source File

SOURCE=.\cntwep.c
# End Source File
# Begin Source File

SOURCE=.\cvdetail.c
# End Source File
# Begin Source File

SOURCE=.\cvicon.c
# End Source File
# Begin Source File

SOURCE=.\cvname.c
# End Source File
# Begin Source File

SOURCE=.\cvtext.c
# End Source File
# Begin Source File

SOURCE=.\dtlpaint.c
# End Source File
# Begin Source File

SOURCE=.\icnpaint.c
# End Source File
# Begin Source File

SOURCE=.\ll.c
# End Source File
# Begin Source File

SOURCE=.\nampaint.c
# End Source File
# Begin Source File

SOURCE=.\txtpaint.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ca_prntf.h
# End Source File
# Begin Source File

SOURCE=.\cntbrw.h
# End Source File
# Begin Source File

SOURCE=.\cntdlg.h
# End Source File
# Begin Source File

SOURCE=.\cntdll.h
# End Source File
# Begin Source File

SOURCE=.\cntdrag.h
# End Source File
# Begin Source File

SOURCE=.\custctrl.h
# End Source File
# Begin Source File

SOURCE=.\ll.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
