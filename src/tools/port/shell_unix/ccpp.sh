:
#
# Copyright (c) 2004 Ingres Corporation
#
# ccpp.sh - Prepend $ING_SRC/tools/port$noise/conf/CONFIG to $1 and run
#           through cpp.
#
# History:
#	28-jun-1989 (owen)
#		Starting history for ccpp.sh.
#		Added additional matching pattern for non standard config
#		string "i39_vme" for ICL 6.1 Port Kit.
#	27-jul-1989 (owen)
#		Added additional matching pattern for non standard config
#		string "rca_vad" for RCA 6.1 Port Kit.
#	31-jul-1989 (boba)
#		Change conf to tools/port$noise/conf.
#	19-sep-1989 (boba)
#		Allow bii_bos config string for BiiN BiiNOS.
#	02-oct-1989 (boba)
#		Allow -U (undefine) to be passed through to cpp.
#		Necessary now that DISTLIST includes "unix!" in path 
#		names.  Was getting changed to "1!".  Oops.
#	28-nov-89 (swm)
#		/lib/cpp was hard-wired. Introduced a CPP shell variable
#		and set its value in a case statement; /usr/ccs/lib/cpp
#		on dr6_us5, /lib/cpp by default.
#	16-Oct-1990 (jonb)
#		Remove lines that consist of nothing but whitespace, in
#		addition to removing blank lines, from the output.
#	09-jan-91 (sandyd)
#		Added "sqs_ptx" case.
#	04-nov-91 (jlw)
#		added pattern matching for xxx_osf config string for OSF/1
#		ports and derivatives.
#	26-jan-92 (johnr)
#		added patterns for ds3_osf 
#	24-oct-92 (lauraw)
#		Added support for new VERS format: 
#		- readvers is invoked unless "-c config" was given
#		- $optdef is added to "cpp" command
#	09-dec-92 (mikem)
#		su4_us5 6.5 port.  cpp lives in /usr/ccs/lib/cpp on su4_us5.
#	29-jul-93 (swm)
#		Fixed path specification for bzarch.h.
#	07-sep-93 (jab)
#		- $optdef deleted from the "ccpp" command that also included <bzarch.h>.
#		  It's information that's included in <bzarch.h> now....
#	25-jan-94 (arc)
#		Add support for su4_cmw (Sun CMW - B1 Secure O/S)
#	09-feb-94 (ajc)
#		Added hp8_bls options.
#	13-mar-95 (smiba01)
#		Added dr6_ues options.
#       06-jun-1995 (pursch)
#               Add changes by kirke to 6405 18-sep-92
#               pym_us5 cpp is in /usr/ccs/lib.
#	13-nov-1995 (murf)
#		Added sui_us5 option for cpp.
#	06-dec-95 (morayf)
#		Added rmx_us5 options for SNI RMx00 SINIX. Same as pym_us5.
#	11-Mar-1999 (kosma01)
#		Use 32bit mips3 version of cpp for SGI IRIX.
#	10-may-1999 (walro03)
#		Remove obsolete version string dr6_ues.
#       03-jul-1999 (podni01)
#               Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_u
#	06-oct-1999 (toumi01)
#		Change Linux config string from lnx_us5 to int_lnx.
#		Add lnx as a valid suffix.
#	11-oct-1999 (hweho01)
#	        Added support for AIX 64-bit platform (ris_u64), 	
#		list u64 as a valid suffix.
#	16-apr-1999 (hanch04)
#	    Added su9_us5 for CPP.
#	18-jul-2001 (stephenb)
#		Add support for i64_aix
#	02-Nov-2001 (hanje04)
#	    Use $ING_SRC/bin/cpp on Linux.
#	20-Nov-2001 (hanje04)
#	    Use -C on ccp to force C preprocessing and not C++
#	03-Dec-2001 (hanje04)
#	    Define cpp for IA64 Linux (i64_lnx).
#	23-Mar-2003 (hanje04)
#	    Define cpp for AMD x86-64 Linux (a64_lnx)
#	07-Feb-2003 (bonro01)
#	    Add support for i64_hpu.
#	07-Feb-2004 (hanje04)
#	    Add -traditional to CPP definition for Linux as cpp with
#	    gcc version 3.0 and up replaces <TAB> with white space.
#	22-mar-2004 (somsa01)
#	    Do the same as above for Itanium Linux.
#       01-Aug-2004 (hanch04)
#           First line of a shell script needs to be a ":" and
#           then the copyright should be next.
# 	24-Aug-2004 (hanje04)
# 	    bzarch.h is now in cl/hdr/hdr_unix for jam build. Check for it
# 	    there first
#	19-Jan-2005 (hanje04)
#	    Add -I flag to ccpp to allow include path for #include "file.h"
#	    style header in files passed in. This is important when building
#	    under jam as everything is run from ING_SRC and not the
#	    directory where the code resides.
#	07-Mar-2005 (hanje04)
#	    SIR 114034
#	    Add support for reverse hybrid
#	15-Mar-2005 (bonro01)
#	    Add support for Solaris AMD64 a64_sol
#       18-Apr-2005 (hanje04)
#           Add support for Max OS X (mg5_osx).
#           Based on initial changes by utlia01 and monda07.
#	27-Apr-2005 (kodse01)
#	    Add -traditional for a64_lnx (AMD64 Linux).
#        6-Nov-2006 (hanal04) SIR 117044
#           Add int.rpl for Intel Rpath Linux build.
#	23-Mar-2007
#	    SIR 117985
#	    Add support for 64bit PowerPC Linux (ppc_lnx)
#	12-Oct-2007 (kiria01) b118421
#	    Add support for passing through certain text that would have been
#	    stripped out by the preprocessor.
#	08-Feb-2008 (hanje04)
#	    SIR S119978
#	    Add suport for Intel Mac OS X
#	18-Jun-2009 (kschendel) SIR 122138
#	    If bzarch is being included, it will always have the #define config
#	    in it.  Eliminate use of -I- which is a deprecated gcc option.
#	    Compute hybrid config symbol for ccpp -s.
#	17-Jul-2009 (bonro01)
#	    Restore use of -I because we still use compilers that don't
#	    support -iquote
#	17-Jul-2009 (kschendel) SIR 122138
#	    Only do the iquote thing for gcc 4.x.y on linux.  Other gcc users
#	    (e.g. osx) can pick up the iquote fix on an as-needed basis.
#

while [ $# != 0 ]
do case $1 in
	-c)	shift; conf=$1; shift;;
	-s)	shift; sym=$1; shift;;
	-D*)	defs="$defs $1"; shift;;
	-U*)	undefs="$undefs $1"; shift;;
	-I*)    incdir="$1" ; shift ;;
	-*)	echo "usage: ccpp [ -c xxx_uyy ] [ files... ] [-Dsym ...] [-Usym ...]" >&2
		echo "       ccpp [ -c xxx_uyy ] -s symbol [-Dsym ...] [-Usym ...]" >&2
		exit 1;;
	*)	break;;
esac; done

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"
bzarch=
confd=
if [ -z "$conf" ]
then
	. readvers
	conf=$config
	if [ -z "$conf" ] ; then
	    echo "No config set by readvers or by -c option."
	    echo "Make sure that VERS has the correct format."
	    exit 1
	fi
	if [ -z "$sym" ] ; then
	    # (not going to use bzarch.h if -s symbol)
	    # config string is defined in bzarch if it's been made yet
	    if [ -r $ING_SRC/cl/hdr/$ING_VERS/hdr_unix/bzarch.h ]
	    then
		bzarch="$ING_SRC/cl/hdr/$ING_VERS/hdr_unix/bzarch.h"
	    elif [ -r $ING_SRC/cl/hdr/$ING_VERS/hdr/bzarch.h ]
	    then
		bzarch="$ING_SRC/cl/hdr/$ING_VERS/hdr/bzarch.h"
	    else
		confd="-D$conf"
	    fi
	fi
else
	confd="-D$conf"
fi

[ -f $ING_SRC/tools/port$noise/conf/CONFIG ] || exit 1

case $conf in
	dr6_us5)	CPP=/usr/ccs/lib/cpp ;;
	sui_us5)	CPP=/usr/ccs/lib/cpp ;;
	su4_us5)	CPP=/usr/ccs/lib/cpp ;;
	su9_us5)	CPP=/usr/ccs/lib/cpp ;;
	rux_us5)        CPP=/usr/ccs/lib/cpp ;;
        rmx_us5)        CPP=/usr/ccs/lib/cpp ;;
	sgi_us5)	CPP=/usr/lib32/cmplrs/cpp ;;
	int_rpl|\
	*_lnx)	CPP='/lib/cpp -traditional -Ulinux'
		# Only for gcc 4.x, which complain bitterly about -I-.
		# Some 3.x gcc's don't understand -iquote yet.
		v=`cc --version 2>/dev/null | head -1`
		if expr "$v" : '.* 4\.[0-9][0-9]*\.[0-9][0-9]* ' >/dev/null 2>/dev/null ; then
		    incopt='-iquote '
		fi
		;;
	*_osx)	CPP='/usr/bin/cpp'
		# If osx gcc complains about -I-, use incopt code from above.
		;;
	*)	CPP=/lib/cpp ;;
esac

if [ "$sym" ]
then
	# define the conf_BUILD_ARCH_xx_yy for hybrid capables,
	# since -s sym doesn't include bzarch.
	arch=
	if [ -n "$build_arch" ] ; then
	    x=`echo $build_arch | tr '+' '_'`
	    arch="-Dconf_BUILD_ARCH_$x"
	fi
	res=`echo $sym | cat $ING_SRC/tools/port$noise/conf/CONFIG - | \
		$CPP $incdir -DVERS=$conf -D$conf $arch $optdef -P | sed -e '/^[ 	]*$/d' -e '/^#pragma/d'`
	if [ "$res" = "$sym" ]
	then echo "$sym: undefined in CONFIG" >&2; exit 1
	else echo $res
	fi
else
	if [ -n "$incdir" -a -n "$incopt" ] ; then
	    # Change -Idir to -iquote dir for gcc.  It's unclear that
	    # -iquote is really needed, but ...
	    incdir=`expr "$incdir" : '-I\(.*\)'`
	    incdir="${incopt}$incdir"
	elif [ -n "$incdir" ] ; then
	    # if no incopt defined, append -I-
	    incdir="$incdir -I-"
	fi
	cat $bzarch $ING_SRC/tools/port$noise/conf/CONFIG ${*-"-"} | \
		$CPP $incdir -DVERS=$conf $confd $defs $undefs -P | sed -e '/^[ 	]*$/d' -e '/^#pragma/d' -e 's:/%:/*:g' -e 's:%/:*/:g' -e 's:^%::'
fi
