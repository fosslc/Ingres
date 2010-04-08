:
#
# Copyright (c) 2004 Ingres Corporation
#
# $Header:
#
# This script builds UNIX versions of `xbzarch.h,' a file containing
# all configuration dependent definitions and included by <Xos.h>.
#
# This is stolen, wholesale, from "mkbzarch.h" on the Ingres code line.
#
#  _NO_PROTO       defined if prototypes aren't allowed in source code
#  SYSV            defined if you're on one of T-H-O-S-E machines
#  BSD             defined if you're on one of T-H-O-S-E O-T-H-E-R machines
#
# History:
#
# 6-may-91 (jab)
#	Added a little more checking for prototype determination code.
# 8-may-91 (kimman)
#     Adding DECstation ULTRIX (ds3_ulx) specific information
# 29-apr-91 (vijay)
#	Corrected MALLOC_0_RETURNS_NULL logic. It was exactly reversed.
#	Added ris_us5 specific stuff.
#	Put in a check to see if regcmp is available on the box.
#	Changed the order of rules for NDIR, SYS_DIR stuff.
# 10-jun-91 (kimman)
#	Moving #includes so that the test programs will compile on 
#	the DecStation ULTRIX platforms.  The 'cc' compiler needs
#	the #includes starting in column one of each line.
# 02-dec-91 (kimman)
#     Adding DNETCONN to DECstation ULTRIX (ds3_ulx) specific information
# 29-jan-92 (jab)
#     Adding Sun support, and recognition that alloca.h might be needed
#	  on some platforms.
# 14-feb-92 (vijay)
#	Added odt_us5 for TCPCONN. Correct stdarg logic. 
# 19-feb-92 (vijay)
#	Add NEED_FFS for odt_us5. If a port needs some bsd library calls from
#	x11-src!x11r4!X!sysV!Berklib.c, it should define the proper NEED's here.
#	Add R_OK for odt_us5. This is the flag sent to access() call, not
#	defined in the system include files.
# 19-aug-92 (www)
#       Added TCPCONN and UNIXCONN for su4_us5 by removing the su4_u42
#       entry that was or'd with hp and ris, and making a su4_* entry.
# 18-sep-92 (seng)
#	On the hp3_us5 port, regex does exist but is in the -lPW library.
#	Include this library in the test for regex for the hp3_us5.
# 27-sep-92 (vijay)
#	changes for motorola. BSD_COMP should be defined to get some of the ioctl
#	stuff. see ioctl.h. need bcopy, bzero and bcmp. dont define re_comp
#	since regex is in libgen.a.
# 02-mar-93 (jab)
#	Added a couple of X11R5'isms. Mainly, added a little bit for recognizing
#	whether <regex.h> is there or not, and hard-coded NO_CONST into the
#	resultant file. (Life is short, and "const" in C compilers makes it
#	shorter.)
# 04-mar-93 (jab)
#	Corrected a typo, and added a little to check for 'bzero' being there.
# 07-apr-93 (jab & dwilson)
#	A few changes:  
#		1) Defined NO_MEMMOVE for su4_u42, since it doesn't have it.
#		2) Actually perpetrate a function call to see if regex() is 
#		   available.
#		3) Did a brute force definition of NO_ALLOCA
# 07-apr-93 (dwilson)
#	Added a series of defines for hp3_us5 (to summarize:  we don't need
#		steenkeeng prototypes!) and moved the NO_MEMMOVE definition
#		for su4_u42 to the platform specific strangeness case.
# 09-may-93 (jab)
#	Adding a '#define NO_SHAPE' for the times we might need to compile
#	a 'mwm'. Should have no effect on other platforms.
# 22-jul-93 (gillnh2o)
#	Adding file under tools!port!shell. This file use to be in
#	x11tools.
#  17-dec-93 (robf)
#	 Check ING_VERS when looking up VERS location so we can find it
#	 in a BAROQUE environment (same change as lower down applied)
#  05-jan-95 (chech02)
#        Added rs4_us5 for AIX 4.1.  
#  18-feb-98 (toumi01)
#	Added Linux (lnx_us5) LIB definition to set LIBC= correctly.
#  10-may-1999 (walro03)
#       Remove obsolete version string odt_us5.
#  06-oct-1999 (toumi01)
#	Change Linux config string from lnx_us5 to int_lnx.
#  18-Oct-1999 (hweho01)
#        Added ris_u64 for AIX 64-bit platform.  
#  15-Jun-2000 (toumi01)
#	Added ibm_lnx to Linux Lib definition.
#  11-Sep-2000 (hanje04)
#	 Added suport for Alpha Linux (axp_lnx).
#  20-jul-2001 (stephenb)
#	Add support for i64_aix
#  03-Dec-2001 (hanje04)
#	Added support for AI64 Linux (i64_lnx)
#  06-Nov-2002 (hanje04)
#	Added support for AMD x86-64 Linux and genericise Linux defines
#	where apropriate.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
#	23-Mar-2007
#	    SIR 117985
#	    Add support for 64bit PowerPC Linux (ppc_lnx)
#

header=xbzarch.h
date=`date`
whatunix=`whatunix`
[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"
vers=`grep config= $ING_SRC/tools/port$noise/conf/VERS | sed s/config=//`

USRINCLUDE=/usr/include
LIB=/lib
if [ "$vers" = "m88_us5" ]
then
	LIB=/usr/ccs/lib
fi
if [ "$vers" = "int_lnx" ] || [ "$vers" = "ibm_lnx" ] || \
   [ "$vers" = "ibm_lnx" ] || [ "$vers" = "i64_lnx" ] || \
   [ "$vers" = "int_rpl" ]
then
	LIB=/usr/lib
elif [ "$vers" = "a64_lnx" ] || [ "$vers" = "ppc_lnx" ]
then
	LIB=/usr/lib64
fi
LIBC=$LIB/libc.a
if [ $vers = hp*_us* ]
then
	CC=c89
else
	CC=cc
fi

# stdout into newly created header

exec > $header
echo "#ifndef xbzarch_h"
echo "# define xbzarch_h"

# don't leave it around if we have trouble

trap 'rm $header' 0

# create a new target:

cat << !
/*
** X11 $header created on $date
** by the $0 shell script
*/

!

# version file generated by running the source transfer script
# generated by mkreq.sh.  GV_VER and GV_ENV used in gver.roc.

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"
< $ING_SRC/tools/port$noise/conf/VERS ||
{
	echo "put config name there (like cci_u42) and try again." >&2
	exit 1
}


case $vers in
[0-9]*)	echo "# define x$vers";;
*)	echo "# define $vers";;
esac

case $vers in
*_us5)	echo "# define SYSV" ;;
*_bsd)	echo "# define BSD" ;;
*_ulx)	echo "# define BSD" ;;
esac

case $vers in
ris_us5|hp*_us*|m88_us5|rs4_us5|ris_u64|i64_aix)	echo "#define TCPCONN"
		echo "#define UNIXCONN"
		;;
su4_*)          echo "#define TCPCONN"
                echo "#define UNIXCONN"
                ;;
*_ulx)          echo "#define TCPCONN"
	        echo "#define UNIXCONN"
	        echo "#define DNETCONN"
	  	;;
esac


[ -r $USRINCLUDE/stdarg.h ] || echo "# define  MissingStdargH"
# There's a scary part of Xtos.h that looks like if you use
# alloca it never frees stuff (that might be part of the 
# way that alloca works, but it's still vile), so let's be a little
# careful....
echo "# define NO_ALLOCA"
echo "# define NO_CONST"
echo "# define	NO_SHAPE"


if (
	cd /tmp
	echo '
		int setuid(int);
		int george(int p);
		main(){}
		int herman(int p){ return(p);}
	' > m.c
	$CC  -c m.c > /dev/null 2>&1
)
then
	echo "/* Function prototypes are supported. */"
else
	echo "/* No function prototypes allowed: */"
	echo "# define	_NO_PROTO"
fi
rm -f /tmp/m.[co]

echo "/* Ignoring SUPPORT_GET_VALUES_EXT for now */"
echo "/* Ignoring BB_HAS_LOSING_FOCUS_CB for now */"
echo "/* Ignoring DBUTTON_FOCUS_BOGOUSITY for now */"
echo "/* Ignoring FULL_EXT for now */"
echo "/* Ignoring CALLBACKS_USE_RES_LIST for now */"

if [ -r $USRINCLUDE/dirent.h ]
then
	:
elif [ -r $USRINCLUDE/sys/dir.h ]
then
	echo "# define  SYS_DIR"
elif [ -r $USRINCLUDE/ndir.h ]
then
	echo "# define  NDIR"
else
	echo "Default of dirent.h isn't there" >&2
	echo "error will occur compiling Xm/FileSB.c" >&2
	exit 1
fi

cnm=/tmp/libc.$$
nm $LIBC > $cnm
grep getcwd $cnm > /dev/null && echo "# define  USE_GETCWD"
grep vfork $cnm > /dev/null || echo "# define  NO_VFORK"
rm $cnm

echo "/* Ignoring TEXT_IS_GADGET for now */"
echo "/* Ignoring _TRACE_HEAP */"
echo "/* Ignoring FUTURES for now */"
echo "/* Ignoring OLD_FOCUS_MOVED for now */"
echo "/* Ignoring FOLLOW_SERVER for now */"
echo "/* Ignoring  for now */"
if (
	cd /tmp
	echo '
#include <signal.h>
		int one = 1;
		short two = 2;
		done() { exit(1); }
		main()
		{
			char arr[20], *parr = arr;
			register int i;
			for (i = SIGHUP; i < NSIG; i++)
				signal(i, done);
			parr = &arr[1];
			*(int *)parr = one;
			if ((*(int *)parr) != 1)
				exit(1);
			parr = &arr[1];
			*(short *)parr = two;
			if ((*(short *)parr) != 1)
				exit(1);
			else
				exit(0);
		}
	' > m.c
	($CC  m.c && ./a.out) > /dev/null 2>&1
)
then
	echo "# define UNALIGNED"
else
	echo "/* No unaligned assignments */"
	echo "# undef UNALIGNED"
fi
rm -f /tmp/m.[co] core
echo "/* Ignoring HP_MOTIF for now */"
echo "/* Defaulting to paranoid STRINGS_ALIGNED for now */"
echo "#define	STRINGS_ALIGNED"
echo "/* Ignoring DXM_V11 for now */"
[ -r $USRINCLUDE/netdnet/dn.h ] || echo "/* Need to include <dn.h>? */"
if (
	cd /tmp
	echo '
#include <stdio.h>
#include <malloc.h>
		main()
		{
			if (malloc((unsigned)0) == NULL)
				exit(1);
			else
				exit(0);
		}
	' > m.c
	$CC  m.c > /dev/null 2>&1 && ./a.out
)
then
	echo "# undef MALLOC_0_RETURNS_NULL"
else
	echo "# define MALLOC_0_RETURNS_NULL"
fi

# see if regex is available.
if (
        cd /tmp
        echo '
                main()
                {
			regex();
                }
        ' > r.c
		cc r.c > /dev/null 2>&1 
)
then
        echo "# undef NO_REGEX"
else
        echo "# define NO_REGEX"
fi
# see if regcmp is available.
if (
        cd /tmp
        echo '
                main()
                {
			regexec();
                }
        ' > r.c
		cc r.c > /dev/null 2>&1 
)
then
        echo "# undef NO_REGCOMP"
else
        echo "# define NO_REGCOMP"
fi

# see if bzero is available.
if (
        cd /tmp
        echo '
                main()
                {
                        char *x,*y,*z;

						bzero(x,y,z);
                }
        ' > r.c
		cc r.c > /dev/null 2>&1
)
then
        echo "# define X_USEBFUNCS"
else
        echo "# undef X_USEBFUNCS"
fi

# platform specific strangeness.
case $vers in
su4_u42)	echo "# define NO_MEMMOVE" ;;
hp3_us5)	
		echo "# define NeedFunctionPrototypes 0"
		echo "# define NeedNestedPrototypes 0"
		echo "# define NeedWidePrototypes 0"
		echo "# define NeedVarargsPrototypes 0"
		;;
ris_u64|\
i64_aix|\
rs4_us5|\
ris_us5)
                # RS/6000 has NULL defined as ((void *)0) and the compiler
                # chokes when NULL is used in place of a zero in stmts
                # containing the ? operator.

                echo "# ifdef NULL"
                echo "# undef NULL"
                echo "# define NULL 0"
                echo "# endif"
                ;;
m88_us5)	
		# need to define BSD_COMP to get some ioctl flags in.
		echo "# define BSD_COMP"
		# regex is in libgen.a
		echo "# undef NO_REGEX"
		echo "# define NEED_BCMP"
		echo "# define NEED_BCOPY"
		echo "# define NEED_BZERO"
		;;
esac

rm -f /tmp/m.[co] core
echo "/* End of $header */"
echo "#endif /* xbzarch_h */"

trap '' 0
