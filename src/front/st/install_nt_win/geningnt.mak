# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Win32 Release
!MESSAGE No configuration specified.  Defaulting to Win32 Release.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "GENINGNT.MAK" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
OUTDIR=.
INTDIR=.

ALL : $(OUTDIR)/GENINGNT.exe $(OUTDIR)/GENINGNT.bsc

# ADD BASE CPP /nologo /ML /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /ML /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR /c
CPP_PROJ=/nologo /ML /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /FR$(INTDIR)/ /Fp$(OUTDIR)/"GENINGNT.pch" /Fo$(INTDIR)/ /c 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
BSC32_SBRS= \
	$(INTDIR)/GENINGNT.SBR
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"GENINGNT.bsc" 

$(OUTDIR)/GENINGNT.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/GENINGNT.OBJ
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:console /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"GENINGNT.pdb" /MACHINE:I386 /OUT:$(OUTDIR)/"GENINGNT.exe" 

$(OUTDIR)/GENINGNT.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
OUTDIR=.
INTDIR=.

ALL : $(OUTDIR)/GENINGNT.exe $(OUTDIR)/GENINGNT.bsc

# ADD BASE CPP /nologo /ML /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /c
# ADD CPP /nologo /ML /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /c
CPP_PROJ=/nologo /ML /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /FR$(INTDIR)/ /Fp$(OUTDIR)/"GENINGNT.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"GENINGNT.pdb" /c 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
BSC32_SBRS= \
	$(INTDIR)/GENINGNT.SBR
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"GENINGNT.bsc" 

$(OUTDIR)/GENINGNT.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/GENINGNT.OBJ
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:console /DEBUG /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:console /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"GENINGNT.pdb" /DEBUG /MACHINE:I386\
 /OUT:$(OUTDIR)/"GENINGNT.exe" 

$(OUTDIR)/GENINGNT.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
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

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\GENINGNT.CPP

$(INTDIR)/GENINGNT.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
