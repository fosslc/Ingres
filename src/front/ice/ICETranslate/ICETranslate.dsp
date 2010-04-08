# Microsoft Developer Studio Project File - Name="ICETranslate" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ICETranslate - Win32 DebugAMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ICETranslate.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ICETranslate.mak" CFG="ICETranslate - Win32 DebugAMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ICETranslate - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 ReleaseAMD64" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 DebugAMD64" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ICETranslate - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 xerces-c_1.lib ingres.lib oiiceapi.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 xerces-c_1D.lib ingres.lib oiiceapi.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 Debug64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ICETranslate___Win32_Debug64"
# PROP BASE Intermediate_Dir "ICETranslate___Win32_Debug64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib xerces-c_1D.lib oiiceapi.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 xerces-c_1D.lib ingres64.lib oiiceapi64.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:IA64

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 Release64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ICETranslate___Win32_Release64"
# PROP BASE Intermediate_Dir "ICETranslate___Win32_Release64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib xerces-c_1.lib oiiceapi.lib /nologo /subsystem:console /machine:I386 /libpath:"$(ING_SRC)\front\lib"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 xerces-c_1.lib ingres64.lib oiiceapi64.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:IA64

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 ReleaseAMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ICETranslate___Win32_ReleaseAMD64"
# PROP BASE Intermediate_Dir "ICETranslate___Win32_ReleaseAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseAMD64"
# PROP Intermediate_Dir "ReleaseAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /Wp64 /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /Wp64 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 xerces-c_1.lib ingres64.lib oiiceapi64.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:IA64
# ADD LINK32 xerces-c_1.lib ingres64.lib oiiceapi64.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:AMD64

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 DebugAMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ICETranslate___Win32_DebugAMD64"
# PROP BASE Intermediate_Dir "ICETranslate___Win32_DebugAMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugAMD64"
# PROP Intermediate_Dir "DebugAMD64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /Wp64 /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /YX /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 xerces-c_1D.lib ingres64.lib oiiceapi64.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:IA64
# ADD LINK32 xerces-c_1D.lib ingres64.lib oiiceapi64.lib $(ING_SRC)\cl\clf\gv\gv.res kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /libpath:"$(ING_SRC)\front\lib" /machine:AMD64

!ENDIF 

# Begin Target

# Name "ICETranslate - Win32 Release"
# Name "ICETranslate - Win32 Debug"
# Name "ICETranslate - Win32 Debug64"
# Name "ICETranslate - Win32 Release64"
# Name "ICETranslate - Win32 ReleaseAMD64"
# Name "ICETranslate - Win32 DebugAMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ICETranslate.cpp
# End Source File
# Begin Source File

SOURCE=.\ICETranslateHandlers.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ICETranslate.hpp
# End Source File
# Begin Source File

SOURCE=.\ICETranslateHandlers.hpp
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
