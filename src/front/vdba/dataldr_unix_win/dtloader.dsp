# Microsoft Developer Studio Project File - Name="dtloader" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=dtloader - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dtloader.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dtloader.mak" CFG="dtloader - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dtloader - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "dtloader - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/DtLoader", VJMAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dtloader - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX"stdafx.h" /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 ingres.lib /nologo /subsystem:console /machine:I386 /out:"..\app\dataldr.exe" /libpath:"$(ING_SRC)\front\lib"

!ELSEIF  "$(CFG)" == "dtloader - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ingres.lib /nologo /subsystem:console /debug /machine:I386 /out:"..\app\dataldr.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib"

!ENDIF 

# Begin Target

# Name "dtloader - Win32 Release"
# Name "dtloader - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dtloader.c
# End Source File
# Begin Source File

SOURCE=.\dtstruct.c
# End Source File
# Begin Source File

SOURCE=.\listchar.c
# End Source File
# Begin Source File

SOURCE=.\parse.c
# End Source File
# Begin Source File

SOURCE=.\sqlcopy.c
# End Source File
# Begin Source File

SOURCE=.\stdafx.c
# End Source File
# Begin Source File

SOURCE=.\tsynchro.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dtstruct.h
# End Source File
# Begin Source File

SOURCE=.\listchar.h
# End Source File
# Begin Source File

SOURCE=.\parse.h
# End Source File
# Begin Source File

SOURCE=.\sqlcopy.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\tsynchro.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Sql Files"

# PROP Default_Filter "*.sc"
# Begin Source File

SOURCE=.\sqlcopy.sc

!IF  "$(CFG)" == "dtloader - Win32 Release"

# Begin Custom Build - Performing ESQLC sqlcopy.sc...
InputPath=.\sqlcopy.sc

"sqlcopy.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi -fsqlcopy.inc sqlcopy.sc

# End Custom Build

!ELSEIF  "$(CFG)" == "dtloader - Win32 Debug"

# Begin Custom Build - Performing ESQLC sqlcopy.sc...
InputPath=.\sqlcopy.sc

"sqlcopy.inc" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi -fsqlcopy.inc sqlcopy.sc

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
