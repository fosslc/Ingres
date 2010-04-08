#
#   NMAKE makefile
#
#   History:
#	06-jun-96 (somsa01)
#	    Created.  The latest ingresnt.mas should be placed in this
#	    directory.  Then an nmake -a should bring everything up to
#	    date.  This means making a new fsizes.roc, and a new copycut.bat.
#	    A new fsizes.roc means that installer.exe must be rebuilt.
#
#	    To summarize:
#
#	    inst.hpj	- The project file for the installation's help.
#	    space.exe	- Builds fsizes.roc, a structure need by the installer
#			  to compute product sizes.
#	    pinggcn.obj - Used to check if Ingres is running.
#	    installer.exe - The "pre" installer.  Kicks off CA-Installer.
#	    windo_pt.exe - The "post" installer. Just gives the installer a
#			   successful message.
#	12-jun-96 (somsa01)
#		Modified compiling of pinggcn.obj so that it would work. Keep
#		in mind that the installer needs to be linked with the 
#		appropriate ingres.lib for the specific release (and copy it
#		into this directory)!
#	14-jun-96 (somsa01)
#		Added making of geningnt for patch.
#	22-jan-97 (somsa01)
#		Added /MD to compiling of pinggcn.obj .
#	23-nov-97 (mcgem01)
#		Inclusion of the include path causes problems when
#		the compiler is installed to Program Files.
#	13-oct-1998 (somsa01)
#		Differentiate between an NT patch and a W95 patch.
#	16-oct-1998 (somsa01)
#	    Minor correction to last change.
#	28-jan-1999 (somsa01)
#	    Made changes for new patch installation process.
#	04-feb-1999 (somsa01)
#	    Corrected compile options for pinggcn.exe . On some machines,
#	    NTVDM gave us errors or chewed up CPU when pinggcn was
#	    executed by the patch installer (even though we should not
#	    have been running with NTVDM).
#	12-Mar-1999 (andbr08)
#		Added compilation of redirdos.exe.
#       12-Mar-2001 (rigka01) bug 104173
#               Fix for doublebyte (dbl) patches
#	15-May-2002 (inifa01)
#		Modified for 2.6		
!INCLUDE "..\..\..\tools\port\make\make.ini"

!IF "$(DOUBLEBYTE)" != ""
!MESSAGE DOUBLEBYTE : ON
DBLFLAG=-DDOUBLEBYTE
!ELSE
!MESSAGE DOUBLEBYTE : OFF
!ENDIF

all: space.exe fsizes pinggcn.exe redirdos.exe installer.exe

space.exe: space.c
!IF "$(DEBUG)" == "ON"
	cl /Zi $(DBLFLAG) space.c kernel32.lib user32.lib imagehlp.lib
!ELSE
	cl $(DBLFLAG) space.c kernel32.lib user32.lib imagehlp.lib
!ENDIF

fsizes: filelist
	space.exe

pinggcn.exe: pinggcn.c
	cl /MD /G5 -Di386=1 -D_X86_ /Od -W2 -DNT_INTEL -DDESKTOP \
		-DDEVL -DWIN32 -DCL_HAS_PROTOS -DINCLUDE_ALL_CL_PROTOS \
		-DNT_GENERIC -I$(ING_SRC)\front\st\hdr \
		-I$(ING_SRC)\cl\hdr\hdr -I$(ING_SRC)\cl\clf\hdr \
		-I$(ING_SRC)\gl\hdr\hdr -I$(ING_SRC)\back\hdr\hdr \
		-I$(ING_SRC)\back\dmf\hdr -I$(ING_SRC)\common\hdr\hdr \
		-I$(ING_SRC)\front\hdr\hdr -I$(ING_SRC)\front\frontcl\hdr \
		-I$(ING_SRC)\common\gcf\hdr -Gd -Ge pinggcn.c \
		$(II_SYSTEM)\ingres\lib\ingres.lib msvcrt.lib kernel32.lib \
		user32.lib advapi32.lib netapi32.lib

redirdos.exe: redirdos.c
	nmake -f redirdos.mak CFG="redirdos - Win32 Release"

installer.exe: fsizes.roc installer.rc pinggcn.c
	nmake -f installer.mak  CFG="installer - Win32 Debug"
