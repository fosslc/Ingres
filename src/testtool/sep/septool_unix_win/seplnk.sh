:
#
# Copyright (c) 2004 Ingres Corporation
#
# seplnk : Link EQUEL/ESQL object file with various INGRES libraries
#
## History:
##	15-Jan-91 (kenl)
##		removed reference to qalib.a for your basic link,
##		that is, if both fflag and cflag are false.
##	21-Mar-91 (sgp)
##		added support for multiple object files.  put in
##		that "wow" stuff and caused the cfilnam assignments
##		to concatenate themselves.
##	05-Jul-91 (DonJ)
##		Added this file into Ingres63p library.
##      20-Aug-91 (DonJ)
##		Simplified conditional statements. Made other changes
##		suggested by Roger Taranto.
##      27-Aug-91 (DonJ) 
##              More improvements.
##      08-feb-93 (bconnell)
##              Some machines have split libingres into two libraries.
##              Added conditional code for this.
##	17-mar-93 (DonJ)
##              Added Ingres environment check and call to iisysdep. Will
##              make compiler calls more compatible to porting.
##    08-apr-1993 (donj)
##            Allow the use of the $CC Env Var. to pick which compiler.
##    01-dec-93 (vijay)
##	      Use shared libs if available.
##   28-Aug-1997 (merja01)
##        added libingres.a to libingfiles for raat tests.
##    15-apr-1998 (popri01)
##		Add a platform case statement to allow ld flags.
##		usl_us5 needs -K thread for proper inclusion of thread lib.
##	09-dec-1998 (walro03)
##		For ICL (dr6_us5), grep out linker warnings.
##	02-apr-1999 (muhpa01)
##		Moved libingres.a to end of library list for linking with
##		shared libraries.
##	05-Apr-1999 (kosma01)
##		remove some common and useless warnings on sgi_us5.
##      23-Jun-1999 (merja01)
##              Added chmod to axp_osf links to insure linked file is left
##              fully executable.  This resolves a multiple definition 
##              problem using seplnk during raat testing.
##    03-jul-99 (podni01)
##              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
##    07-Jul-1999 (podni01)
##              rmx_us5 needs -K thread for proper inclusion of thread lib.
##    03-Aug-1999 (hweho01)
##              Added support for AIX 64-bit platform (ris_u64).
##    26-Jan-2001 (bonro01)
##		Remove rmx_us5 entry which was added incorrectly.
##		rmx_us5 does not support the -K thread flag.
##    21-May-2003 (bonro01)
##		Add support for HP Itanium (i64_hpu)
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##      03-Nov-2004 (sheco02)
##	    Change the shared library linking flags due to the merge of windows 
##	    jam build rules.
##	15-Jul-2005 (hanje04)
##	    Mac OSX has a noisy linker. Redirect output to a temporary file and
##	    only display it on error.
##	28-Mar-2007 (hanje04)
##	    SIR 117985
##	    Need it include CCLDMACH in link line for PPC Linux. Not sure
##	    why we don't on other platforms.
##	12-Feb-2008 (hanje04)
##	    SIR S119978
##	    Replace mg5_osx with generic OSX.
##	28-Oct-2008 (wanfr01)
##	    SIR 121146
##	    Add -a option to sync sepcc behavior across platforms
##	10-Apr-2009 (frima01)
##	    Add check for type of object file and create a 32- respectively
##	    64-bit executable accordingly. Therefore removed separate
##	    link command for r64.us5 and usl.us5.
##	23-Jun-2009 (kschendel) SIR 122138
##	    New hybrid scheme always has 32+64 bit strings in iisysdep.
##	    Revise the hybrid test to look at conf_BUILD_ARCH_xx_yy.
##	12-Mar-2010 (hanje04)
##	   SIR 123296
##	   Add support for LSB builds
##	29-Mar-2010 (hanje04)
##	   SIR 123296
##	   Define inglibdir for non-LSB builds
##
#

CMD=seplnk
export CMD

if [ $# -eq 0 ]
then
     cat << !

Usage: seplnk <file1> [<file2> ... <filen>]

	-a		 VMS only:  Include API option files

        -c[onditional]   Conditional Linking. Only link if
                         image file does not exist.

!
exit 0
fi
 
###############################################################################
#
# Check local Ingres environment
#
###############################################################################

if [ ! -n "${II_SYSTEM}" ]
then
     cat << !

|------------------------------------------------------------|
|                      E  R  R  O  R                         |
|------------------------------------------------------------|
|     The local Ingres environment must be setup and the     |
|     the installation running before continuing             |
|------------------------------------------------------------|

!
   exit 1
fi

. $II_SYSTEM/ingres/utility/iisysdep

need_appended=true
tmp=`echo $PATH | sed -e 's/:/ /g'`
for i in $tmp
do
        if [ $i = "${II_SYSTEM}/ingres/utility" ]
           then
                need_appended=false
        fi
done

if $need_appended
   then
        PATH=$II_SYSTEM/ingres/utility:$PATH
        export PATH
fi

###############################################################################

if [ ! -n "${CC}" ]
then
      CC=cc
fi

imageflg=true

cflag=false    # conditional

for i
do	case $i in
		-conditional | -conditiona | -condition |	\
		-conditio | -conditi | -condit | -condi |	\
		-cond | -con | -co | -c ) cflag=true;;

		-a )	;;

		-* )	echo "bad flag $i"
			;;

		*.* | * ) ;;
	esac
done

do_it=false

for i
do
	case $i in
		-* )	continue
                        ;;

		*.o )   cfilnam="$cfilnam $i"
			tfilnam=`basename $i .o`
			;;

		*.a )	cfilnam="$cfilnam $i"
			tfilnam=`basename $i .a`
			;;

                * )	if [ -f `basename $i.a` ]
			then
			    ofilnam=`basename $i.a`
			    cfilnam="$cfilnam $ofilnam"
			    tfilnam=`basename $ofilnam .a`
			elif [ -f `basename $i.o` ]
			then
			    ofilnam=`basename $i.o`
			    cfilnam="$cfilnam $ofilnam"
			    tfilnam=`basename $ofilnam .o`
			else
			    cfilnam="$cfilnam $i"
			    tfilnam=`basename $i`
			fi
			;;
	esac

	if $imageflg
	then
	    filnam=$tfilnam
	    imageflg=false
	    do_it=true
	fi
done

if $do_it && $cflag && [ -f $filnam.exe ]
then
	do_it=false
fi

# Hybrid builds might need linking against an add-on lib directory.
# determine object code type from first given file

lpdir=
cc_ld="$CCLDMACH"
ld_lib="$LDLIBMACH"

if [ -n "$conf_BUILD_ARCH_32_64" -o -n "$conf_BUILD_ARCH_64_32" ] ; then
    firstfile=`echo $cfilnam | cut -d" " -f1`
    if `file $firstfile | sed  "s/$firstfile//g"  | sed "s/AMD64//g" | sed "s/IA64//g" | sed "s/x86_64//g" | grep 64 1> /dev/null 2> /dev/null`
    then 
	if [ -n "$conf_BUILD_ARCH_32_64" ] ; then
	    cc_ld="$CCLDMACH64"
	    ld_lib="$LDLIBMACH64"
	    lpdir=/lp64
	fi
    else
	if [ -n "$conf_BUILD_ARCH_64_32" ] ; then
	    cc_ld="$CCLDMACH32"
	    ld_lib="$LDLIBMACH32"
	    lpdir=/lp32
	fi
    fi
fi

if [ x"$conf_LSB_BUILD" = x"TRUE" ]
then
    libingdir="" # Should pick up from ldconfig
else
    libingdir="-L$II_SYSTEM/ingres/lib${lpdir}"
fi

if $do_it
then
  if $USE_SHARED_LIBS
  then
    libingfiles="$libingdir -lframe.1"
    libingfiles="$libingfiles -lq.1"
    libingfiles="$libingfiles -lcompat.1"
    libingfiles="$libingfiles -lingres"
  else
    libingfiles=`ls $libingdir/libingres*.a`
  fi
  case $VERS in
	dr6_us5)
		$CC $OPTIM -o $filnam.exe $cfilnam $libingfiles $LDLIBMACH 2>&1 | grep -v warning
		;;
	rux_us5)
		$CC $OPTIM -K thread -o $filnam.exe $cfilnam $libingfiles $LDLIBMACH
		;;
	sgi_us5)
		ldir="-L $II_SYSTEM/ingres/lib"
		llibs="-lingres -lframe.1 -lq.1 -lcompat.1 "
		$CC $OPTIM -Wl,-woff,84,-woff,85,-woff,34 -o $filnam.exe $cfilnam $ldir $llibs $LDLIBMACH
		;;
        axp_osf)
                $CC $OPTIM -o $filnam.exe $cfilnam $libingfiles $LDLIBMACH
                chmod 777 $filnam.exe
                ;;
	  *_osx)
		ldout=/tmp/ldout.$$
		$CC $OPTIM -o $filnam.exe $cfilnam $libingfiles $LDLIBMACH  > \
							${ldout} 2>&1
		[ $? != 0 ] && cat ${ldout}
		rm -f ${ldout}
		;;
		
	ppc_lnx)
		$CC $OPTIM $CCLDMACH -o $filnam.exe $cfilnam $libingfiles\
			 $LDLIBMACH
                ;;
	* )	$CC $OPTIM $cc_ld -o $filnam.exe $cfilnam $libingfiles $ld_lib
		;;
  esac

fi
