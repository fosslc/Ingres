# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=installer - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to installer - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "installer - Win32 Release" && "$(CFG)" !=\
 "installer - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "installer.mak" CFG="installer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "installer - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "installer - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
MTL=mktyplib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "installer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "WinRel"
OUTDIR=.
INTDIR=.\WinRel

ALL : "$(OUTDIR)\installer.exe" "$(OUTDIR)\installer.bsc"

CLEAN : 
	-@erase "$(INTDIR)\installer.obj"
	-@erase "$(INTDIR)\installer.res"
	-@erase "$(INTDIR)\installer.sbr"
	-@erase "$(OUTDIR)\installer.bsc"
	-@erase "$(OUTDIR)\installer.exe"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/installer.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\WinRel/
CPP_SBRS=.\WinRel/
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/installer.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/installer.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\installer.sbr"

"$(OUTDIR)\installer.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=version.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows\
 /incremental:no /pdb:"$(OUTDIR)/installer.pdb" /machine:I386\
 /out:"$(OUTDIR)/installer.exe" 
LINK32_OBJS= \
	"$(INTDIR)\installer.obj" \
	"$(INTDIR)\installer.res"
#	".\pinggcn.obj" \
#	"$(II_SYSTEM)\ingres\lib\ingres.lib"

"$(OUTDIR)\installer.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "installer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.
INTDIR=.\WinDebug

ALL : "$(OUTDIR)\installer.exe" "$(OUTDIR)\installer.bsc"

CLEAN : 
	-@erase "$(INTDIR)\installer.obj"
	-@erase "$(INTDIR)\installer.res"
	-@erase "$(INTDIR)\installer.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\installer.bsc"
	-@erase "$(OUTDIR)\installer.exe"
	-@erase "$(OUTDIR)\installer.ilk"
	-@erase "$(OUTDIR)\installer.pdb"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

# ADD BASE CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /FR"$(INTDIR)/" /Fp"$(INTDIR)/installer.pch" /YX /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\WinDebug/
CPP_SBRS=.\WinDebug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\..\cl\hdr\hdr" /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/installer.res" /i "..\..\cl\hdr\hdr" /d\
 "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/installer.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\installer.sbr"

"$(OUTDIR)\installer.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 version.lib kernel32.lib user32.lib gdi32.lib winspool.lib comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=version.lib kernel32.lib user32.lib gdi32.lib winspool.lib\
 comctl32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib imagehlp.lib /nologo /subsystem:windows\
 /incremental:yes /pdb:"$(OUTDIR)/installer.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/installer.exe" 
LINK32_OBJS= \
	"$(INTDIR)\installer.obj" \
	"$(INTDIR)\installer.res"
#	".\pinggcn.obj" \
#	"$(II_SYSTEM)\ingres\lib\ingres.lib"

"$(OUTDIR)\installer.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "installer - Win32 Release"
# Name "installer - Win32 Debug"

!IF  "$(CFG)" == "installer - Win32 Release"

!ELSEIF  "$(CFG)" == "installer - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\installer.rc
DEP_RSC_INSTA=\
	".\INSTALLER.ICO"\
	

"$(INTDIR)\installer.res" : $(SOURCE) $(DEP_RSC_INSTA) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\installer.c
DEP_CPP_INSTAL=\
	".\fsizes.roc"\
	

"$(INTDIR)\installer.obj" : $(SOURCE) $(DEP_CPP_INSTAL) "$(INTDIR)"

"$(INTDIR)\installer.sbr" : $(SOURCE) $(DEP_CPP_INSTAL) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
