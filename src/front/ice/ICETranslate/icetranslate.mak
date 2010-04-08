# Microsoft Developer Studio Generated NMAKE File, Based on ICETranslate.dsp
!IF "$(CFG)" == ""
CFG=ICETranslate - Win32 Debug64
!MESSAGE No configuration specified. Defaulting to ICETranslate - Win32 Debug64.
!ENDIF 

!IF "$(CFG)" != "ICETranslate - Win32 Release" && "$(CFG)" != "ICETranslate - Win32 Debug" && "$(CFG)" != "ICETranslate - Win32 Debug64" && "$(CFG)" != "ICETranslate - Win32 Release64"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ICETranslate.mak" CFG="ICETranslate - Win32 Debug64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ICETranslate - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 Debug64" (based on "Win32 (x86) Application")
!MESSAGE "ICETranslate - Win32 Release64" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "ICETranslate - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\ICETranslate.exe"


CLEAN :
	-@erase "$(INTDIR)\ICETranslate.obj"
	-@erase "$(INTDIR)\ICETranslateHandlers.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\ICETranslate.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /Fp"$(INTDIR)\ICETranslate.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ICETranslate.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=xerces-c_1.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ingres.lib oiiceapi.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\ICETranslate.pdb" /machine:I386 /out:"$(OUTDIR)\ICETranslate.exe" /libpath:"$(ING_SRC)\front\lib" 
LINK32_OBJS= \
	"$(INTDIR)\ICETranslate.obj" \
	"$(INTDIR)\ICETranslateHandlers.obj"

"$(OUTDIR)\ICETranslate.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\ICETranslate.exe"


CLEAN :
	-@erase "$(INTDIR)\ICETranslate.obj"
	-@erase "$(INTDIR)\ICETranslateHandlers.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ICETranslate.exe"
	-@erase "$(OUTDIR)\ICETranslate.ilk"
	-@erase "$(OUTDIR)\ICETranslate.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /Fp"$(INTDIR)\ICETranslate.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ICETranslate.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=xerces-c_1D.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ingres.lib oiiceapi.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\ICETranslate.pdb" /debug /machine:I386 /out:"$(OUTDIR)\ICETranslate.exe" /pdbtype:sept /libpath:"$(ING_SRC)\front\lib" 
LINK32_OBJS= \
	"$(INTDIR)\ICETranslate.obj" \
	"$(INTDIR)\ICETranslateHandlers.obj"

"$(OUTDIR)\ICETranslate.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 Debug64"

OUTDIR=.\Debug64
INTDIR=.\Debug64
# Begin Custom Macros
OutDir=.\Debug64
# End Custom Macros

ALL : "$(OUTDIR)\ICETranslate.exe"


CLEAN :
	-@erase "$(INTDIR)\ICETranslate.obj"
	-@erase "$(INTDIR)\ICETranslateHandlers.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\ICETranslate.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "_DEBUG" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /Fp"$(INTDIR)\ICETranslate.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /Wp64 /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ICETranslate.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=xerces-c_1D.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ingres.lib oiiceapi.lib /nologo /subsystem:console /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\ICETranslate.exe" /libpath:"$(ING_SRC)\front\lib" /machine:IA64 
LINK32_OBJS= \
	"$(INTDIR)\ICETranslate.obj" \
	"$(INTDIR)\ICETranslateHandlers.obj"

"$(OUTDIR)\ICETranslate.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ICETranslate - Win32 Release64"

OUTDIR=.\Release64
INTDIR=.\Release64
# Begin Custom Macros
OutDir=.\Release64
# End Custom Macros

ALL : "$(OUTDIR)\ICETranslate.exe"


CLEAN :
	-@erase "$(INTDIR)\ICETranslate.obj"
	-@erase "$(INTDIR)\ICETranslateHandlers.obj"
	-@erase "$(OUTDIR)\ICETranslate.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "$(ING_SRC)\cl\hdr\hdr" /I "$(ING_SRC)\cl\clf\hdr" /I "$(ING_SRC)\gl\hdr\hdr" /I "$(ING_SRC)\back\hdr\hdr" /I "$(ING_SRC)\back\dmf\hdr" /I "$(ING_SRC)\common\hdr\hdr" /I "$(ING_SRC)\front\hdr\hdr" /I "$(ING_SRC)\front\frontcl\hdr" /I "$(ING_SRC)\front\ice\hdr" /I "$(II_SYSTEM)\ingres\files" /I "$(ING_SRC)\front\ice\SAX" /D "WIN64" /D "_WINDOWS" /D "_MBCS" /D "NT_INTEL" /D "IMPORT_DLL_DATA" /D "DESKTOP" /D "DEVL" /D "NT_GENERIC" /D "SEP" /D "SEPDEBUG" /D "xCL_NO_AUTH_STRING_EXISTS" /D "INGRESII" /Fp"$(INTDIR)\ICETranslate.pch" /YX /Fo"$(INTDIR)\\" /Wp64 /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ICETranslate.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=xerces-c_1.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ingres.lib oiiceapi.lib /nologo /subsystem:console /pdb:none /machine:I386 /out:"$(OUTDIR)\ICETranslate.exe" /libpath:"$(ING_SRC)\front\lib" /machine:IA64 
LINK32_OBJS= \
	"$(INTDIR)\ICETranslate.obj" \
	"$(INTDIR)\ICETranslateHandlers.obj"

"$(OUTDIR)\ICETranslate.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("ICETranslate.dep")
!INCLUDE "ICETranslate.dep"
!ELSE 
!MESSAGE Warning: cannot find "ICETranslate.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ICETranslate - Win32 Release" || "$(CFG)" == "ICETranslate - Win32 Debug" || "$(CFG)" == "ICETranslate - Win32 Debug64" || "$(CFG)" == "ICETranslate - Win32 Release64"
SOURCE=.\ICETranslate.cpp

"$(INTDIR)\ICETranslate.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ICETranslateHandlers.cpp

"$(INTDIR)\ICETranslateHandlers.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

