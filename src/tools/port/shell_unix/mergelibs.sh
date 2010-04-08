:
#
# Copyright (c) 2004 Ingres Corporation
#
# merge INGRES libraries into libingres.a
#
# DEST = utility
## History:
##	24-apr-1980 (boba)
##		First created.  Based on mergelibs used for Sun 3 6.1 FCS.
##	01-jun-1989 (boba)
##		Remove obsolite csstub as suggested by Jeff Anton.
##	17-jul-1989 (kimman)
##		Added shell commands to obtain the machine config name and to
##		use this config name to determine the machine specific 
##		archive header file name.  On the DECstation 3100 (ds3_ulx)
##		the archive contains a file named '__________ELEL_', not
## 		'__.SYMDEF' as on most other systems.
##	31-Jul-89 (GordonW)
##		Added special case for Sequent's libraries.
##	31-jul-1989 (boba)
##		Change conf to tools/port$noise/conf.
##	30-aug-1989 (kimman)
##		Added interp to list of libraries 
##	30-aug-1989 (kimman)
##		Added iaom, ilrf, ioi to list of libraries 
##	4-dec-89 (daveb)
##		Run in $INGLIB/tmp instead of /tmp.  /tmp can run
##		out of room.
##	15-jan-1990 (boba)
##		Ensure now accepts multiple names so it works correctly.
##	07-jun-1990 (achiu)
##		add libatt.a for pyr_u42
##		add libbsd.a for pyr_us5
##	31-Jul-90 (GordonW)
##		Redid the logic for "unwanted" object list. Under the older
##		scheme this list was a static list, which meant anytime
##		new objects were added to libraries that weren't meant for
##		libingres.a, this script should have been edited, whic it
##		never was. Now the "unwanted" object list is generated from
##		unwanted object directories, such as "LG, LK, CS, and LGK.
##	09-Aug-90 (GordonW)
##		Take care of removing "crypt" calls from front-ends by
##		recompiling gcaconn.c with NO_CRYPT set.
##	10-Sep-90 (Owen/Shelby/Kimman)
##		Adding abf/iaom/iamiltab.roc to the unwanted list. It is
##		a duplicate of IIILtab and can cause confusion in
##		abf on systems that do odd library sorting.
##	30-Oct-90 (pholman)
##		More extensive use of 'noise' variable, to make compatible
##		with DBMS/PE source code structures.
##	10-jan-91 (rog)
##		We still need to use xargs to run ar even on systems that
##		don't use lorder; therefore, I removed the test and made
##		mergelibs always use xargs.
##	25-mar-91 (mikem)
##		Strip the DBE special objects out of the library provided
##		to users.
##	23-apr-91 (rudyw)                                
##		Set up case control for avoiding ranlib as done in mkdefault.
##	26-jun-91 (jonb)
##		Change quoting on trap command.  Using double quotes caused
##		evaluation of the variables at the wrong time; they shouldn't
##		be eval'd until the signal actually occurs.
##	26-jun-91 (jonb)
##		Integrate splitlibs processing from ingres6302p line.  Also,
##		use the HASRANLIB definition from default.h via iisysdep
##		instead of a locally generated one.
##	05-jul-91 (rudyw)
##		Take odt_us5 and sco_us5 out of recently reintegrated splitlibs
##		processing since they no longer split the libingres.a library.
##	29-oct-91 (gautam)
##		Change the NO_CRYPT processing to operate on gcapwd.c
##       27-dec-91 (rudyw)
##		Add odt_us5 and sco_us5 to list of platforms using splitlibs
##		since SCO has not yet incorporated large library support into
##		its development package as expected.
##	26-jan-92 (johnr)
##		Added ds3_osf entry to propagate the ds3_ulx entry.
##       04-mar-92 (bonobo)
##               Fixed the trap so that mergelibs exits properly. 
##	09-oct-92 (bonobo)
##		Updated the location of the lg, lgk and lk directories.
##	16-oct-92 (lauraw)
##		Uses new readvers to get config string.
##	19-nov-92 (dianeh)
##		Added cuf to the list of libraries wanted (lnames).
##	20-nov-92 (dianeh)
##		Added gcnvalid.o to wanted object list (wanted), to correct
##		problem with gcn_usr_validate. Also corrected the user id on
##		the history comment.
##	20-nov-92 (dianeh)
##		Corrected previous history comments spacing.
##	18-jan-93 (geri)
##		Removed obsolete ch50, in50, nt50 libraries from dlist
##	09-mar-93 (sandyd)
##		Added libqxa to the libingres.a list.
##	23-mar-93 (lauraw)
##		Used the AR_L_OPTS value from iisysdep instead of hardcoded
##		"l" on "ar" command. (Some machines don't have "l".)
##	03-may-93 (dianeh)
##		Remove reference to gcnvalid.o -- gone for 6.5; change History
##		comment-block to double pound-signs.
##	23-jul-93 (dianeh)
##		Remove -n300 flag from call to xargs -- latest version of xargs
##		in tools/port/others doesn't have any flags.
##	15-Sep-1993 (fredv)
##		Update the unwanted object list and unwanted dirs list.
##	13-apr-94 (swm)
##		Added SYMDEFNAME for axp_osf to avoid spurious repeated
##		symbols error.
##	10-may-95 (morayf)
##		Removed sco_us5 and odt_us5 from splitlibs list as SCO
##		has large library support now (i.e ar works for libingres.a)
##	11-May-95 (hanch04)
##		Added raat to the libingres.a list.
##	14-Feb-96 (morayf)
##		Added iiufutil.o to unwanted list for rmx_us5.
##		Mixed-case Fortran systems should probably all follow suit.
##		The problem is with multiply defined transaction functions.
##	22-Apr-96 (rajus01)
##		Removed gcn dir from unwanted dirs list for object removals.
##	14-Jun-96 (gordy)
##		Include OpenAPI routines.
##	27-jul-1996 (canor01)
##		Use csmt directory if using operating-system threads.
##      09-jan-1997 (hanch04)
##              Use cs or csmt if either exists
##	18-feb-1997 (hanch04)
##		OS threads now use both cs and csmt
##	13-feb-1998 (fucch01)
##		Added sgi_us5 to HASRANLIB, so sgi uses sort instead of
##		lorder and tsort.  Also had to add special case for sgi_us5
##		for "polishing libingres.a" so it uses "ar ts libingres.a",
##		since ranlib doesn't exist on sgi.  Had to alter the if
##		statement for HASRANLIB (and $vers = sgi) to:
##		    if [ "$HASRANLIB" != "" -o "$vers" = "sgi_us5" ]
##		May create sgi's own code to separate the two, in case the
##		change to the HASRAN condition is not properly portable???
##	15-dec-1998 (marro11)
##		Tweaked a bit further to work properly in baroque envs.
##		Introduced notion of:
##		    pnoise  - private path noise; one's ppath version
##		    bnoise  - base path noise; where the real potatoes are
##		    tnoise  - transient noise; to be filled in later
##	31-Mar-98 (gordy)
##		Removed gcnutil.o from wanted list.
##	27-Jul-1998 (hanch04)
##		Fix case for ranlib
##	10-may-1999 (walro03)
##		Remove obsolete version string bul_us5, dr4_us5, ds3_osf,
##		ix3_us5, nec_us5, ps2_us5, pyr_u42, sqs_us5.
##      03-jul-1999 (podni01)
##              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
##      02-Aug-1999 (linke01)
##              SGI (sgi_us5) doesn't have lorder file, change of 
##              hanch04 on 27-Jul-1998 dominates fucch01's change on
##              13-feb-1998. Keep hanch04's change and put fucch01's change 
##              back for sgi_us5.
##	24-mar-2000 (stephenb)
##		Added ddf and iceclient.  Need for building ice apps.
##	25-apr-2000 (somsa01)
##		Added capability for building other products.
##	05-mar-1999 (hanch04)
##		Fix case for ranlib
##	19-mar-2001 (somsa01)
##		For other products, we make lib<product>.a .
##	21-jun-2001 (devjo01)
##		Exclude all of CX, except cxapi.o.  If LK is included,
##		then sadly CX, plus CS must also be included.
##
##      11-jun-2002 (hanch04)
##              Changes for 32/64 bit build.
##              Use one version of mergelibs for 32 or 64 bit builds.
##      24-Sep-2002 (hweho01)
##              Changes for hybrid build on AIX.
##	14-Mar-03 (gordy)
##		Removed server specific objects from GCF libarary,
##		so don't need to handle has special cases here.
##	21-jun-2001 (devjo01)
##		Exclude all of CX, except cxapi.o.  If LK is included,
##		then sadly CX, plus CS must also be included.
##	04-oct-2002 (devjo01
##            Add cxdata.o cxnuma.o to wanted objects.
##	11-Jun-2004 (somsa01)
##		Cleaned up code for Open Source.
##	14-Jul-2004 (hanje04)
##	      Remove dependency on ming
##	15-Jul-2004 (hanje04)
##		Define DEST=utility so file is correctly pre-processed
##	16-Jul-2004 (hanje04)
##		 Move gcapwd.o to $INGLIB after creation.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	2-aug-2004 (stephenb)
##		Fix for platform extensions on directory names
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
##	23-Feb-2005 (srisu02)
##		Changed OPTIM to IIOPTIM as the flag -DNO_CRYPT was not 
##              being picked up by jam. This flag is used to generate 
##              gcapwd.o without the crypt() symbol
##       02-Mar-2005 (hanje04)
## 	    SIR 114034
##          Add support for reverse hybrid builds. If conf_ADD_ON64 is
##          defined then script will behave as it did prior to change.
##          However, if conf_ADD_ON32 is set -lp32 flag will work like
##          the -${LPHB} flag does for the conf_ADD_ON64 case except that
##          a 32bit libraries will be built under $ING_BUILD/lib/lp32.
##	04-Aug-2005 (zicdo01)
##	    Remove check to build as ingres
##      11-Apr-2007 (hanal04) Bug 118045
##          Correct syntax errors reported on r64.us5 which checking for
##          `which jam`
##	17-Jun-2009 (kschendel) SIR 122138
##	    New scheme for hybrid building.

[ $SHELL_DEBUG ] && set -x

if [ -n "$ING_VERS" ] ; then

    # In a baroque environment, we either are building the base area,
    # or a private area; the former can be identified by comparing ING_BUILD
    # with II_SYSTEM (they should be the same); the latter should have a 
    # "BASE" file indicating which base area they ppathed against.
    #
    # "BASE" file should have been created on each ppath invocation
    #	Format:  basepath=s00
    #

    # First get physical dirnames using csh (sh may not translate symlinks)
    #
    bdir=`csh -fc "cd $ING_BUILD ; pwd"`
    idir=`csh -fc "cd $(PRODLOC)/(PROD2NAME) ; pwd"`

    if [ "$bdir" = "$idir" ] ; then
	bnoise="$ING_VERS"

    else
	basefile="$ING_SRC/clients/$ING_VERS/BASE"
	if [ ! -r "$basefile" ] ; then
	    echo "Unable to determine base path used; missing file."
	    echo "File:  $basefile"
	    exit 1
	fi

	# Extract base string; add "src" below
	#
	bnoise=`grep "^basepath=" $basefile | head -1 | cut -f2 -d=`
	if [ -z "$bnoise" ] ; then
	    echo "Unable to determine base path used; odd format."
	    echo "File:  $basefile"
	    exit 1
	fi
    fi

    tnoise="__noise__"
    bnoise="/$bnoise/src"
    pnoise="/$ING_VERS/src"

else
    # Non-baroque; initialize these to null
    #
    tnoise=""
    bnoise=""
    pnoise=""
fi

#
# Translates dirname $1 to physical location if it exists.
#
# Supply:
#	$1	: directory (full path), with "tnoise" in string.
#
# Return:
#	""	: if non-baroque, and $1 doesn't exist
#	$1	: if non-baroque, and $1 does exist
#	base    : if baroque, and ppath dir for $1 doesn't exist, but base does
#	private	: if baroque, and ppath dir for $1 does exist
#	""	: if baroque, and neither base nor ppath dir for $1 exists
#
translate_dir()
{

    tdir=""
    if [ -n "$ING_VERS" ] ; then
	# baroque; use base, but override with private if there
	#
	xdir=`echo "$1" | sed "s@$tnoise@$bnoise@"`
	[ -d "$xdir" ] && tdir="$xdir"
	xdir=`echo "$1" | sed "s@$tnoise@$pnoise@"`
	[ -d "$xdir" ] && tdir="$xdir"

    else
	[ -d "$1" ] && tdir="$1" 
    fi

    echo "$tdir"
    return 0
}

. readvers
vers=$config
# Set hybrid variables based on build_arch variable
#
RB=xx	## regular / primary build
HB=xx	## hybrid (add-on) build
if [ "$build_arch" = '32+64' ] ; then
    RB=32
    HB=64
elif [ "$build_arch" = '64+32' ] ; then
    RB=64
    HB=32
fi

do_reg=true do_hyb=false

while [ $# -gt 0 ]
do
    case $1 in
        -lp${HB})          do_hyb=true; do_reg=false; shift ;;
        -lp${RB})          do_reg=true; shift ;;
    esac
done

if ($do_hyb && $do_reg)
then
        echo    "$0: 32 and 64 bit libraries can not be build at the same time."
        exit
fi

if $do_hyb
then
    LPHB_DIR=lp${HB}/
    INGLIB=$ING_BUILD/lib/$LPHB_DIR
    BITS=$HB
else
    INGLIB=$ING_BUILD/lib
    BITS=$RB
fi
if [ -n "$build_arch" -a "$BITS" = 'xx' ] ; then
    BITS="$build_arch"
fi
## the upshot of the above is that BITS is set to whatever arch size
## is being built, or xx for non-hybrid-capable platforms.

# If it is making 64-bit build on AIX platform
# specify the 64 bit mode for ar command.
if [ "$config64" = 'r64_us5' -a $BITS = 64 ] ; then
    OBJECT_MODE="64" ; export OBJECT_MODE
fi

cd $INGLIB

case $vers in
        ncr_us5|sx1_us5|mx3_us5|386_us5)
                splitlibs $*
                exit $?
                ;;
        *) ;;
esac

. (PROG1PRFX)sysdep

# see if they want stripped
strip=false
for a in $*
do
	case $a in
	-s) strip=true;;
	*) echo "unknown option, $a" exit 1;;
	esac
done

case $vers in
    	ds3_ulx)	SYMDEFNAME='__________ELEL_' ;;
    	axp_osf)	SYMDEFNAME='________64ELEL_' ;;
    	*)		SYMDEFNAME='__.SYMDEF' ;;
esac


if [ -f lib(PROD2NAME).a ]
then
	echo "replacing $INGLIB/lib(PROD2NAME).a ..."
	rm -f $INGLIB/lib(PROD2NAME).a
else
	echo "creating $INGLIB/lib(PROD2NAME).a ..."
fi

# setup trap(s)
trap 'rm -rf $objs $allobjs $objdir; exit 1' 1 2 3 15

# setup temp directories
TMP=$INGLIB/tmp
(mkdir $TMP) 2>/dev/null
objdir=$TMP/mgd$$
allobjs=$TMP/mga$$
objs=$TMP/mgb$$

# libraries wanted
lnames="abfrt adf afe (PROG0PRFX)compat cuf fd feds fmt ft gcf generate iaom (PROG1PRFX)api ilrf"
lnames="$lnames raat mt ioi (PROG0PRFX)interp (PROG0PRFX)q qgca qsys qxa runsys runtime sqlca uf ug"
lnames="$lnames ui oo"
if [ -n "$conf_BUILD_ICE" ] ; then
    lnames="$lnames ddf iceclient"
fi

# directory list for object removals (unwanted)
dlist="cl/clf$tnoise/di_unix cl/clf$tnoise/ck_unix_win cl/clf$tnoise/jf_unix_win cl/clf$tnoise/sr_unix_win"
dlist="$dlist back/dmf$tnoise/lg back/dmf$tnoise/lgk \
	back/dmf$tnoise/lk gl/glf$tnoise/cx"

# unwanted object list within wanted directories (libcompat.a)
unwanted="mucs.o gcccl.o gcapsub.o gcctcp.o gccdecnet.o gccptbl.o \
	bsdnet.o bsnetw.o cvnet.o iamiltab.o clnt_tcp.o \
	clnt_udp.o pmap_rmt.o svc.o svc_run.o svc_tcp.o meuse.o \
	meconsist.o medump.o mebdump.o meberror.o mexdump.o fegeterr.o \
	pcsspawn.o pcfspawn.o "

[ "$vers" = "rmx_us5" -o "$vers" = "rux_us5" ] &&
unwanted="$unwanted iiufutil.o "

# wanted object list within unwanted directories (libcompat.a)
wanted="cxapi.o cxdata.o cxnuma.o"

# build "unwanted" list from directories
echo "building unwanted object list ..."
here=`pwd`
echo "" >$allobjs
echo "" >$objs
for i in $dlist
do
	tdir=`translate_dir "$ING_SRC/$i"`

	# legal directory?
	if [ -z "$tdir" ]
	then
		tdir=`echo "$ING_SRC/$i" | sed "s@$tnoise@@"`
		echo "Can't find:  \"$tdir\""
		echo "...aborting..."
		exit 1
	fi

	#  get directory source file list
	#
	( cd $tdir ; ls -1 *.c *.s *.roc 2>/dev/null ) | \
		sed -e 's/\.c$/\.o/g' -e 's/\.s$/\.o/g' -e 's/\.roc$/\.o/g' \
		>> $allobjs 2>/dev/null 

	# get source directory (main.c) file list
	( cd $tdir ; ls -1 *.o ) >> $objs 2>/dev/null
done

unwanted="$unwanted `cat $allobjs | tr '\012' ' '`"
wanted="$wanted `cat $objs | tr '\012' ' '`"

# now remove any "wanted" objects from unwanted list
echo "building wanted object list ..."
ufiles=""
echo "$wanted" >$allobjs
for i in $unwanted
do
	if grep -s $i $allobjs >/dev/null 2>/dev/null
	then
		continue
	else
		ufiles="$ufiles $i"
	fi
done
unwanted="$ufiles"

echo "checking for missing libraries ..."
missing=false
for i in $lnames
do
	if [ -f lib$i.a ]
	then
		libs="$libs lib$i.a"
	else
		if [ -f $i.a ]
		then
			libs="$libs $i.a"
		else
			echo "$i.a not found" >&2
			missing=true
		fi
	fi
done

if $missing
then
	echo "aborting" >&2
	exit 2
fi

echo "fetching library object list ..."
echo "" >$objs
for lib in $libs
do
	ar t $lib >>$objs
done
sed -e "/$SYMDEFNAME/d" $objs >$allobjs

echo "checking for duplicates ..."
sort $allobjs | uniq -d > $objs
if [ -s $objs ]
then
	echo "repeated objects - aborting" >&2
	objlist=`cat $objs`
	for obj in $objlist
	do
		for lib in $libs
		do
			if ar t $lib $obj 1>/dev/null 2>&1
			then
				echo "$lib: $obj" >&2
			fi
		done
	done
	exit 3
fi

echo "OK for merge ..."

do_lorder=true
if [ "$HASRANLIB" != "" -o "$vers" = "sgi_us5" ] 
then
	do_lorder=false
fi

# now do slow or fast object list generation...
if $do_lorder
then
	echo "lorder ..."
	# Sun OS 4.0 lorder is buggy; it lists archive names along with objects
	case $vers in
    	ds3_ulx)
	   lorder $libs > $allobjs 2>/dev/null
	   ;;
    	*)
	   lorder $libs | grep -v '\.a' > $allobjs 2>/dev/null
	   ;;
	esac

	echo "tsort ..."
	# clean up output to screen of tsort errors for mx3_us5
	tsort $allobjs 2> /dev/null | grep -v tsort > $objs
else
	sort $allobjs >$objs 2>/dev/null
fi

echo "extracting objects from libraries ..."
rm -rf $objdir
mkdir $objdir
cd $objdir
for lib in $libs
do
	echo "	$lib"
	if [ -f $INGLIB/$lib ]
	then
		ar x$AR_L_OPT $INGLIB/$lib
	else
		ar x$AR_L_OPT $lib
	fi
	rm -f $SYMDEFNAME
done

ar x$AR_L_OPT $INGLIB/libulf.a ultrace.o

# do stripping
if $strip
then
	echo "stripping symbols ..."
	olist=`cat $objs`
	for i in $olist
	do
		strip $i
	done
fi

echo "building lib(PROD2NAME).a ..."
xargs ar cq$AR_L_OPT $INGLIB/lib(PROD2NAME).a < $objs
ar r $INGLIB/lib(PROD2NAME).a ultrace.o

echo "removing crypt from gcapwd.c"

tdir=`translate_dir "$ING_SRC/cl/clf$tnoise/gc_unix"`
cd $tdir

if [ -x $ING_SRC/bin/ming ] ; then
    if [ `grep NO_CRYPT gcapwd.c | wc -l` -ge 1 ]
    then
	if $do_reg
	then
	    ming OPTIM="-DNO_CRYPT" gcapwd.o
	else
  	    ming64 OPTIM="-DNO_CRYPT" gcapwd.o
	fi

	if [ ! -f ${LPHB_DIR}gcapwd.o ]
	then
		echo "gcapwd.o didn't compile!"
	else
		ar r $INGLIB/lib(PROD2NAME).a ${LPHB_DIR}gcapwd.o
		rm -f ${LPHB_DIR}gcapwd.o
	fi
    else
	echo "gcapwd.c didn't have NO_CRYPT symbol!"
    fi
elif [ -x `which jam` ] ; then
	IIOPTIM=-DNO_CRYPT
	if $do_reg
        then
	    jam '<cl!clf!gc_unix>'gcapwd.o
	    mv gcapwd.o $INGLIB
	else
	    jam '<cl!clf!gc_unix>'${LPHB_DIR}gcapwd.o
	    mv ${LPHB_DIR}gcapwd.o $INGLIB
	fi
else
     echo "couldn't compile gcapwd.o"
fi

echo "concurrent cleanup ..."
cd $INGLIB
rm -rf $objs $allobjs $objdir &

echo "removing unwanted objects from lib(PROD2NAME).a ..."
ar d$AR_L_OPT lib(PROD2NAME).a $unwanted 2>/dev/null

if [ "$HASRANLIB" != "" -o "$vers" = "sgi_us5" ]
then
	echo "polishing lib(PROD2NAME).a ..."
	case $vers in
		sgi_us5)	ar ts$AR_L_OPT lib(PROD2NAME).a;;
		*)		ranlib lib(PROD2NAME).a;;
	esac
          if [ "$vers" != "sgi_us5" ]
          then 
	       ranlib lib(PROD2NAME).a
          fi
fi
chmod 644 lib(PROD2NAME).a

wait
echo "done"
exit 0
