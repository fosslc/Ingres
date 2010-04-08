# Microsoft Developer Studio Project File - Name="libmon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libmon - Win32 EDBC_Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libmon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libmon.mak" CFG="libmon - Win32 EDBC_Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libmon - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmon - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libmon - Win32 EDBC_Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libmon - Win32 EDBC_Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Vdba30/libmon", VKIAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libmon - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\libmon.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libmon.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libmon - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "..\hdr" /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\common\hdr\hdr" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\libmon.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libmon.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libmon___Win32_EDBC_Release"
# PROP BASE Intermediate_Dir "libmon___Win32_EDBC_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "EDBC_Release"
# PROP Intermediate_Dir "EDBC_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\hdr" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "EDBC" /YX /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libmon.lib"
# ADD LIB32 /nologo /out:"..\lib\libmon.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libmon.lib %ING_SRC%\front\lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libmon___Win32_EDBC_Debug"
# PROP BASE Intermediate_Dir "libmon___Win32_EDBC_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "EDBC_Debug"
# PROP Intermediate_Dir "EDBC_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "..\hdr" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "..\hdr" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "DESKTOP" /D "NT_GENERIC" /D "IMPORT_DLL_DATA" /D "INGRESII" /D "EDBC" /YX /FD /GZ /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\lib\libmon.lib"
# ADD LIB32 /nologo /out:"..\lib\libmon.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying result to build tree...
PostBuild_Cmds=copy ..\Lib\libmon.lib %ING_SRC%\front\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libmon - Win32 Release"
# Name "libmon - Win32 Debug"
# Name "libmon - Win32 EDBC_Release"
# Name "libmon - Win32 EDBC_Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\chkdistr.c
# End Source File
# Begin Source File

SOURCE=.\collisio.c
# End Source File
# Begin Source File

SOURCE=.\dbaginfo.c
# End Source File
# Begin Source File

SOURCE=.\dbatime.c
# End Source File
# Begin Source File

SOURCE=.\domdbkrf.c
# End Source File
# Begin Source File

SOURCE=.\domdgetd.c
# End Source File
# Begin Source File

SOURCE=.\domdmisc.c
# End Source File
# Begin Source File

SOURCE=.\domdtls.c
# End Source File
# Begin Source File

SOURCE=.\domdupdt.c
# End Source File
# Begin Source File

SOURCE=.\monitor.c
# End Source File
# Begin Source File

SOURCE=.\rmcmdcli.c
# End Source File
# Begin Source File

SOURCE=.\sqltls.c
# End Source File
# Begin Source File

SOURCE=.\TBLINTEG.c
# End Source File
# Begin Source File

SOURCE=.\tools.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\hdr\COLLISIO.H
# End Source File
# Begin Source File

SOURCE=.\commfunc.h
# End Source File
# Begin Source File

SOURCE=..\hdr\dba.h
# End Source File
# Begin Source File

SOURCE=..\hdr\dbaginfo.h
# End Source File
# Begin Source File

SOURCE=..\hdr\dbaset.h
# End Source File
# Begin Source File

SOURCE=..\hdr\dbatime.h
# End Source File
# Begin Source File

SOURCE=..\hdr\domdata.h
# End Source File
# Begin Source File

SOURCE=..\hdr\domdloca.h
# End Source File
# Begin Source File

SOURCE=..\hdr\error.h
# End Source File
# Begin Source File

SOURCE=..\hdr\esltools.h
# End Source File
# Begin Source File

SOURCE=..\hdr\libmon.h
# End Source File
# Begin Source File

SOURCE=..\hdr\monitor.h
# End Source File
# Begin Source File

SOURCE=..\Hdr\replutil.h
# End Source File
# Begin Source File

SOURCE=..\hdr\reptbdef.h
# End Source File
# Begin Source File

SOURCE=..\hdr\rmcmd.h
# End Source File
# Begin Source File

SOURCE=..\hdr\tools.h
# End Source File
# End Group
# Begin Group "SQL Files"

# PROP Default_Filter ".sc"
# Begin Source File

SOURCE=.\chkdistr.sc

!IF  "$(CFG)" == "libmon - Win32 Release"

# Begin Custom Build
InputPath=.\chkdistr.sc

"chkdistr.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -c  -multi chkdistr

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 Debug"

# Begin Custom Build
InputPath=.\chkdistr.sc

"chkdistr.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -c  -multi chkdistr

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\chkdistr.sc

"chkdistr.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -c  -multi chkdistr

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\chkdistr.sc

"chkdistr.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -c  -multi chkdistr

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\collisio.sc

!IF  "$(CFG)" == "libmon - Win32 Release"

# Begin Custom Build
InputPath=.\collisio.sc

"collisio.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -c  -multi collisio

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 Debug"

# Begin Custom Build
InputPath=.\collisio.sc

"collisio.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -c  -multi collisio

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\collisio.sc

"collisio.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -c  -multi collisio

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\collisio.sc

"collisio.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -c  -multi collisio

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dbaginfo.sc

!IF  "$(CFG)" == "libmon - Win32 Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 Debug"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi dbaginfo

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\dbaginfo.sc

"dbaginfo.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi dbaginfo

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\rmcmdcli.sc

!IF  "$(CFG)" == "libmon - Win32 Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 Debug"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi rmcmdcli

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\rmcmdcli.sc

"rmcmdcli.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi rmcmdcli

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqltls.sc

!IF  "$(CFG)" == "libmon - Win32 Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 Debug"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi sqltls

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\sqltls.sc

"sqltls.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -multi sqltls

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tblinteg.sc

!IF  "$(CFG)" == "libmon - Win32 Release"

# Begin Custom Build
InputPath=.\tblinteg.sc

"tblinteg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -c   -multi TBLINTEG

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 Debug"

# Begin Custom Build
InputPath=.\tblinteg.sc

"tblinteg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	esqlc -c   -multi TBLINTEG

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Release"

# Begin Custom Build
InputPath=.\tblinteg.sc

"tblinteg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -c   -multi TBLINTEG

# End Custom Build

!ELSEIF  "$(CFG)" == "libmon - Win32 EDBC_Debug"

# Begin Custom Build
InputPath=.\tblinteg.sc

"tblinteg.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	%EDBC_ROOT%\edbc\bin\edbcesqlc -c   -multi TBLINTEG

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "doc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libmondoc.doc
# End Source File
# End Group
# End Target
# End Project
