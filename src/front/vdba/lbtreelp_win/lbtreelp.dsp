# Microsoft Developer Studio Project File - Name="lbtreelp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=lbtreelp - Win32 HPUX_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lbtreelp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lbtreelp.mak" CFG="lbtreelp - Win32 HPUX_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lbtreelp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 DBCS Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 DBCS Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 Solaris_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 HPUX_Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 HPUX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 Linux_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "lbtreelp - Win32 AIX_Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "lbtreelp - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "NDEBUG" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\App\lbtreelp.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\lbtreelp.dll %II_SYSTEM%\ingres\vdba	copy Release\lbtreelp.lib %ING_SRC%\front\lib	copy Release\lbtreelp.lib ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "_DEBUG" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\App\lbtreelp.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\lbtreelp.dll %II_SYSTEM%\ingres\vdba	copy Debug\lbtreelp.lib %ING_SRC%\front\lib	copy Debug\lbtreelp.lib ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 DBCS Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DBCS Debug"
# PROP BASE Intermediate_Dir "DBCS Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DBCS Debug"
# PROP Intermediate_Dir "DBCS Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "_DEBUG" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\App\lbtreelp.dll" /pdbtype:sept /libpath:"$(II_SYSTEM)\ingres\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\lbtreelp.dll %II_SYSTEM%\ingres\vdba	copy "DBCS Debug\lbtreelp.lib" %ING_SRC%\front\lib	copy "DBCS Debug\lbtreelp.lib" ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 DBCS Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "DBCS Release"
# PROP BASE Intermediate_Dir "DBCS Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "DBCS Release"
# PROP Intermediate_Dir "DBCS Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "NDEBUG" /D "DOUBLEBYTE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ingres.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\App\lbtreelp.dll" /libpath:"$(II_SYSTEM)\ingres\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying results to II_SYSTEM...
PostBuild_Cmds=copy ..\App\lbtreelp.dll %II_SYSTEM%\ingres\vdba	copy "DBCS Release\lbtreelp.lib" %ING_SRC%\front\lib	copy "DBCS Release\lbtreelp.lib" ..\Lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 Solaris_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lbtreelp___Win32_Solaris_Release"
# PROP BASE Intermediate_Dir "lbtreelp___Win32_Solaris_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "lbtreelp___Win32_Solaris_Release"
# PROP Intermediate_Dir "lbtreelp___Win32_Solaris_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "NDEBUG" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/lbtreelp.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/lbtreelp.dll"

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 HPUX_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "lbtreelp___Win32_HPUX_Debug"
# PROP BASE Intermediate_Dir "lbtreelp___Win32_HPUX_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "lbtreelp___Win32_HPUX_Debug"
# PROP Intermediate_Dir "lbtreelp___Win32_HPUX_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "_DEBUG" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /D "MW_MSCOMPATIBLE_VARIANT" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "_DEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 HPUX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lbtreelp___Win32_HPUX_Release"
# PROP BASE Intermediate_Dir "lbtreelp___Win32_HPUX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "lbtreelp___Win32_HPUX_Release"
# PROP Intermediate_Dir "lbtreelp___Win32_HPUX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "NDEBUG" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /D "MW_MSCOMPATIBLE_VARIANT" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/lbtreelp.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 Linux_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lbtreelp___Win32_Linux_Release"
# PROP BASE Intermediate_Dir "lbtreelp___Win32_Linux_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "lbtreelp___Win32_Linux_Release"
# PROP Intermediate_Dir "lbtreelp___Win32_Linux_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "NDEBUG" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/lbtreelp.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/lbtreelp.dll"

!ELSEIF  "$(CFG)" == "lbtreelp - Win32 AIX_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lbtreelp___Win32_AIX_Release"
# PROP BASE Intermediate_Dir "lbtreelp___Win32_AIX_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "lbtreelp___Win32_AIX_Release"
# PROP Intermediate_Dir "lbtreelp___Win32_AIX_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "NDEBUG" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LBTREELP_EXPORTS" /D "MW_MSCOMPATIBLE_VARIANT" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /i "$(ING_SRC)\cl\clf\gv" /d "NDEBUG" /d "MAINWIN"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/lbtreelp.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../app/lbtreelp.dll"

!ENDIF 

# Begin Target

# Name "lbtreelp - Win32 Release"
# Name "lbtreelp - Win32 Debug"
# Name "lbtreelp - Win32 DBCS Debug"
# Name "lbtreelp - Win32 DBCS Release"
# Name "lbtreelp - Win32 Solaris_Release"
# Name "lbtreelp - Win32 HPUX_Debug"
# Name "lbtreelp - Win32 HPUX_Release"
# Name "lbtreelp - Win32 Linux_Release"
# Name "lbtreelp - Win32 AIX_Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\lbtree.cpp
# End Source File
# Begin Source File

SOURCE=.\lbtree.def
# End Source File
# Begin Source File

SOURCE=.\lbtree.h
# End Source File
# Begin Source File

SOURCE=.\libmain.cpp
# End Source File
# Begin Source File

SOURCE=.\message.cpp
# End Source File
# Begin Source File

SOURCE=.\winlink.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\icons.h
# End Source File
# Begin Source File

SOURCE=.\winlink.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\lbtree.rc
# End Source File
# End Group
# End Target
# End Project
