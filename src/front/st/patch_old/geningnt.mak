# Microsoft Developer Studio Generated NMAKE File, Format Version 4.10
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=geningnt - Win32 Release
!MESSAGE No configuration specified.  Defaulting to geningnt - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "geningnt - Win32 Release" && "$(CFG)" !=\
 "geningnt - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "geningnt.mak" CFG="geningnt - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "geningnt - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "geningnt - Win32 Debug" (based on "Win32 (x86) Console Application")
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
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "geningnt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
OUTDIR=.
INTDIR=.

ALL : "$(OUTDIR)\geningnt.exe" "$(OUTDIR)\geningnt.bsc"

CLEAN : 
	-@erase "$(INTDIR)\GENINGNT.OBJ"
	-@erase "$(INTDIR)\GENINGNT.SBR"
	-@erase "$(OUTDIR)\geningnt.bsc"
	-@erase "$(OUTDIR)\geningnt.exe"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR\
 /Fp"geningnt.pch" /YX /c 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/geningnt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\GENINGNT.SBR"

"$(OUTDIR)\geningnt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/geningnt.pdb" /machine:I386 /out:"$(OUTDIR)/geningnt.exe" 
LINK32_OBJS= \
	"$(INTDIR)\GENINGNT.OBJ"

"$(OUTDIR)\geningnt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "geningnt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
OUTDIR=.
INTDIR=.

ALL : "$(OUTDIR)\geningnt.exe" "$(OUTDIR)\geningnt.bsc"

CLEAN : 
	-@erase "$(INTDIR)\GENINGNT.OBJ"
	-@erase "$(INTDIR)\GENINGNT.SBR"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\geningnt.bsc"
	-@erase "$(OUTDIR)\geningnt.exe"
	-@erase "$(OUTDIR)\geningnt.ilk"
	-@erase "$(OUTDIR)\geningnt.pdb"

# ADD BASE CPP /nologo /MD /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /c
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /FR /Fp"geningnt.pch" /YX /c 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/geningnt.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\GENINGNT.SBR"

"$(OUTDIR)\geningnt.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/geningnt.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/geningnt.exe" 
LINK32_OBJS= \
	"$(INTDIR)\GENINGNT.OBJ"

"$(OUTDIR)\geningnt.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx.obj:
   $(CPP) $(CPP_PROJ) $<  

.c.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "geningnt - Win32 Release"
# Name "geningnt - Win32 Debug"

!IF  "$(CFG)" == "geningnt - Win32 Release"

!ELSEIF  "$(CFG)" == "geningnt - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\GENINGNT.CPP
DEP_CPP_GENIN=\
	{$(INCLUDE)}"\sys\stat.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\GENINGNT.OBJ" : $(SOURCE) $(DEP_CPP_GENIN) "$(INTDIR)"

"$(INTDIR)\GENINGNT.SBR" : $(SOURCE) $(DEP_CPP_GENIN) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
