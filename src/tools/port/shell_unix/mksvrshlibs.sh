:
#
# Copyright (c) 2004 Ingres Corporation
#
# DEST = utility
#
# Name:
#       mksvrshlibs.sh
#
# Usage: mksvrshlibs [becompat] [scf] [psf] [opf] [rdf] [qef]
#		      [qsf] [dbutil] [tpf] [rqf] [gcf] [sxf] [cuf] 
#
#       if no argument given, it will create all thirteen.
#
# Description:
#
#       Creates the shared libraries for the Ingres server. 
#
#       Before running this script, you need to set up various variables such
#       as shlink_cmd and shlink_opts in shlibinfo.sh.
#
#	Server-shared-libraries are generated into
#	$II_SYSTEM/ingres/bin[/lpxx], and not into ingres/lib.  Why?
#	Because iimerge is a set-user-id program, and setuid programs *ignore*
#	LD_LIBRARY_PATH, and *ignore* any -rpath that includes $ORIGIN
#	unless the rpath is *only* $ORIGIN.  (That is true on linux, which
#	is likely to be as restrictive as any unixen.)  So - we define
#	the server with an rpath of just $ORIGIN, and then arrange for
#	the server shared libraries to be in the same directory as the
#	server.
#
# History
#
#	05-Feb-2001 (hanje04)
#	    Created from mkshlibs.sh 85 in ingres!main
#	20-jul-2001 (stephenb)
#	    Add support fir i64_aix
#	03-Dec-2001 (hanje04)
#	    Added support for IA64 Linux (i64_lnx)
#	06-Nov-2002 (hanje04)
#	    Added support for AMD x86-64 Linux and genericise Linux defines
#	    where ever apropriate.
#	26-mar-04 (toumi01)
#	    Clean up do_xxx to allow building of qsf and psf and to avoid
#	    duplicate builds of psf.
#	06-Jul-2004 (hanje04)
#	    Remove unwanted objects from dbutil and opf to stop undefined
#	    references to functions in libq
#	15-Jul-2004 (hanje04)
#	    Define DEST=utility so file is correctly pre-processed
# 	16-Jul-2004 (hanje04)
#	    Add new facility duf!dbutil!dum to list of unwanted dirctories
#	    because it causes undefined references in dbutil.1.so 
#      01-Aug-2004 (hanch04)
#          First line of a shell script needs to be a ":" and
#          then the copyright should be next.
#	07-Mar-2005 (hanje04)
# 	   SIR 114034
#	   Add support for hybrid builds.    
#       04-Aug-2005 (zicdo01)
#          Remove check to build as ingres
#       6-Nov-2006 (hanal04) SIR 117044
#          Add int.rpl for Intel Rpath Linux build.
#	18-Jun-2009 (kschendel) SIR 122138
#	    Hybrid mechanisms changed, fix here.
#	    Revert nov-2007 change since it was caused by calling gcc
#	    instead of ld for linking.  We're calling ld now.
#	    Generate server shared libs into $INGBIN, not $INGLIB.  This
#	    allows $ORIGIN to function even when iimerge is setuid ingres.

if [ -n "$ING_VERS" ] ; then

    # In a baroque environment, we either are building the base area,
    # or a private area; the former can be identified by comparing ING_BUILD
    # with II_SYSTEM (they should be the same); the latter should have a
    # "BASE" file indicating which base area they ppathed against.
    #
    # "BASE" file should have been created on each ppath invocation
    #   Format:  basepath=s00
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
#       $1      : directory (full path), with "tnoise" in string.
#
# Return:
#       ""      : if non-baroque, and $1 doesn't exist
#       $1      : if non-baroque, and $1 does exist
#       base    : if baroque, and ppath dir for $1 doesn't exist, but base does
#       private : if baroque, and ppath dir for $1 does exist
#       ""      : if baroque, and neither base nor ppath dir for $1 exists
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


#  Get info about shared libraries.
. shlibinfo

if  $use_shared_libs & $use_shared_svr_libs 
then
        :
else
        echo    "$0: Build not set up for using shared server libraries"
        exit
fi

: ${ING_BUILD?} ${ING_SRC?}

#
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

#
#       Check arguments.
#

do_scf=false do_psf=false do_opf=false do_rdf=false do_qef=false do_qsf=false do_dbutil=false do_dmf=false do_gwf=false do_adf=false do_ulf=false do_tpf=false do_rqf=false do_gcf=false do_sxf=false do_cuf=false do_malloc=false do_becompat=false
[ $# = 0 ] && do_scf=true do_psf=true do_opf=true do_rdf=true do_qef=true do_qsf=true do_dbutil=true do_tpf=true do_dmf=true do_gwf=true do_adf=true do_ulf=true do_rqf=true do_gcf=true do_sxf=true do_cuf=true do_becompat=true
[ $# = 1 ] && [ $1 = "-lp${HB}" -o $1 = "-lp${RB}" ] &&  do_scf=true do_psf=true do_opf=true do_rdf=true do_qef=true do_qsf=true do_dbutil=true do_tpf=true do_dmf=true do_gwf=true do_adf=true do_ulf=true do_rqf=true do_gcf=true do_sxf=true do_cuf=true do_becompat=true

do_reg=true do_hyb=false

while [ $# -gt 0 ]
do
    case $1 in
 -lp${HB})	do_hyb=true; do_reg=false; shift;;
 -lp${RB})	do_reg=true; shift ;;
 becompat)	do_becompat=true; shift ;;
      scf)	do_scf=true; shift ;;
      psf)	do_psf=true; shift ;;
      opf)	do_opf=true; shift ;;
      rdf)	do_rdf=true; shift ;;
      qef)	do_qef=true; shift ;;
      qsf)	do_qsf=true; shift ;;
   dbutil)	do_dbutil=true; shift ;;
      dmf)	do_dmf=true; shift ;;
      gwf)	do_gwf=true; shift ;;
      adf)	do_adf=true; shift ;;
      ulf)	do_ulf=true; shift ;;
      tpf)	do_tpf=true; shift ;;
      rqf)	do_rqf=true; shift ;;
      gcf)	do_gcf=true; shift ;;
      sxf)	do_sxf=true; shift ;;
      cuf)	do_cuf=true; shift ;;
        *)	echo "Usage: $0 [-lp32|-lp64] [becompat] [scf] [psf] [opf] [rdf] [qef] \n\t[qsf] [dbutil] [tpf] [rqf] [gcf] [sxf] [cuf] "; exit 1 ;;
    esac
done

if ($do_hyb && $do_reg)
then
        echo    "$0: 32 and 64 bit shared libraries can not be build at the same time."
        exit
fi

BITSFX=
if $do_hyb
then
	LPHB=lp${HB}
	INGBIN=$ING_BUILD/bin/${LPHB}
	INGLIB=$ING_BUILD/lib/${LPHB}
	BITS=$HB
else
	LPHB=
	INGBIN=$ING_BUILD/bin
	INGLIB=$ING_BUILD/lib
	BITS=$RB
fi
if [ -n "$build_arch" -a "$BITS" = 'xx' ] ; then
    BITS="$build_arch"
fi
## the upshot of the above is that BITS is set to whatever arch size
## is being built, or xx for non-hybrid-capable platforms.

# shlibinfo returns 32/64-bit variants for hybrid capable, select the
# one wanted this time around.
if [ -n "$build_arch" ] ; then
    eval shlink_cmd=\$"shlink${BITS}_cmd"
    eval shlink_opts=\$"shlink${BITS}_opts"
fi
if [ -z "$shlink_cmd" ] ; then
    echo "Shared library link command shlink_cmd not resolved?"
    [ -n "$build_arch" ] && echo "LPHB is $LBHB, $BITS bits"
    exit 1
fi

# If it is making 64-bit build on AIX platform
# specify the 64 bit mode for ar command.
if [ "$config64" = 'r64_us5' -a $BITS = 64 ] ; then
    OBJECT_MODE="64" ; export OBJECT_MODE
    BITSFX=64
fi


workdir=$INGLIB/shared
mkdir $workdir 2>/dev/null
cd $workdir

trap 'cd $workdir; rm -rf $workdir/*; exit 6' 1 2 3 15

[ `(PROG1PRFX)dsfree` -le 20000 ] && echo "** WARNING !! You may run out of diskspace **"

#
# AIX needs some special things to be done: create an export list, archive the
# shared objects.
#
#
# AIX 4.1 no longer supports the -p option.  The -B option is similar.
#
[ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] &&
mkexplist()
{
    ls -1 *.o | xargs -n50 /usr/bin/nm -B |
      awk ' $2=="D"||$2=="B" { print $3 } ' | sort | uniq > expsym_list
}
#
#
[ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] &&
archive_lib()
{
    libname=$1
    save_dir=`pwd`
    cd $INGBIN
    rm -f $libname.o
    mv $libname.$SLSFX $libname.o
    ar ru $libname.$SLSFX $libname.o
    rm -f $libname.o
    cd $save_dir
}

#
#       check for existence of all the required libraries.
#

echo "Checking static libraries"
missing_libs=
check_libs=
if $do_becompat ; then check_libs=compat ; fi
if $do_scf ; then check_libs="$check_libs scf" ; fi
if $do_psf ; then check_libs="$check_libs psf" ; fi
if $do_opf ; then check_libs="$check_libs opf" ; fi
if $do_rdf ; then check_libs="$check_libs rdf" ; fi
if $do_qef ; then check_libs="$check_libs qef" ; fi
if $do_qsf ; then check_libs="$check_libs qsf" ; fi
if $do_dbutil ; then check_libs="$check_libs dbutil" ; fi
if $do_dmf ; then check_libs="$check_libs dmf" ; fi
if $do_gwf ; then check_libs="$check_libs gwf" ; fi
if $do_adf ; then check_libs="$check_libs adf" ; fi
if $do_ulf ; then check_libs="$check_libs ulf" ; fi
if $do_tpf ; then check_libs="$check_libs tpf" ; fi
if $do_rqf ; then check_libs="$check_libs rqf" ; fi
if $do_gcf ; then check_libs="$check_libs gcf" ; fi
if $do_sxf ; then check_libs="$check_libs sxf" ; fi
if $do_cuf ; then check_libs="$check_libs cuf" ; fi

for lname in $check_libs
do
    if [ ! -r $INGLIB/lib$lname.a ]
    then
        echo Missing library lib$lname.a ...
        missing_libs=yes
    fi
done

[ $missing_libs ] && { echo "Aborting !!" ; exit 2 ;}

# unwanted dirs cs, dy, *50, di, ck, jf, sr for cl and gcc for libq.
unwanted_dirs="back/dmf$tnoise/dmf back/opf$tnoise/opq dbutil/duf$tnoise/dum"

bados=$workdir/bado
echo "" > $bados
for i in $unwanted_dirs
do

    tdir=`translate_dir "$ING_SRC/$i"`

    if [ -n "$tdir" ] ; then

        #  get directory source file list
        #
        ( cd $tdir ; ls -1 *.c *.s *.roc 2>/dev/null ) | \
                sed -e 's/\.c$/\.o/g' -e 's/\.s$/\.o/g' -e 's/\.roc$/\.o/g' \
                >> $bados 2>/dev/null
    fi
done
echo $unwanted_os >> $bados



#
#       Making libbecompat.1
#

if $do_becompat
then
    echo ""
    echo " Building lib(PROG0PRFX)becompat.1.$SLSFX "
    echo ""
    mkdir $workdir/becompat 2> /dev/null ; cd $workdir/becompat
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    for lname in $COMPAT_LIBS
    do
        ar x $INGLIB/lib$lname.a
    done

    # FIXME: objects removed for becompat: should these stay ?
    # pcsspawn.c accesses in_dbestart which is declared in in50/inglobs.o
    # pcfspawn needs pcsspawn
    rm -f pcsspawn.o pcfspawn.o

    # Remove unused objects from shared library otherwise they cause 
    # problems when linking the server

    rm -f cssl.o mebdump.o meberror.o mexdump.o medump.o meconsist.o

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)becompat.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)becompat.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)becompat.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)becompat.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)becompat.1.$SLSFX
        ;;
    *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX *.o \
		$shlink_opts -soname=lib(PROG0PRFX)becompat.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)becompat.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX
        echo Cleaning up ....
	cd $workdir; rm -rf becompat
    fi
fi  # do_becompat


#
# 	Making libscf.1
#

if $do_scf
then
    echo ""
    echo " Building lib(PROG0PRFX)scf.1.$SLSFX "
    echo ""
    mkdir $workdir/scf 2> /dev/null ; cd $workdir/scf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libscf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)scf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)scf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)scf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)scf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)scf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)scf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)scf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)scf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf scf
    fi
fi  # do_scf






#
# 	Making libopf.1
#

if $do_opf
then
    echo ""
    echo " Building lib(PROG0PRFX)opf.1.$SLSFX "
    echo ""
    mkdir $workdir/opf 2> /dev/null ; cd $workdir/opf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libopf.a

    # Remove unwanted objects
    rm -f opqutils.o

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)opf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)opf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)opf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)opf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)opf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)opf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)opf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)opf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf opf
    fi
fi  # do_opf






#
# 	Making librdf.1
#

if $do_rdf
then
    echo ""
    echo " Building lib(PROG0PRFX)rdf.1.$SLSFX "
    echo ""
    mkdir $workdir/rdf 2> /dev/null ; cd $workdir/rdf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/librdf.a
#    echo Removing unwanted objects ...
#    rm -f rdfgetdesc.o rdfgetinfo.o rdfinvalid.o

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)rdf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)rdf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)rdf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)rdf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)rdf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)rdf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)rdf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)rdf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf rdf
    fi
fi  # do_rdf


#
# 	Making libqef.1
#

if $do_qef
then
    echo ""
    echo " Building lib(PROG0PRFX)qef.1.$SLSFX "
    echo ""
    mkdir $workdir/qef 2> /dev/null ; cd $workdir/qef
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libqef.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)qef.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)qef.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)qef.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)qef.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)qef.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)qef.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)qef.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)qef.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf qef
    fi
fi  # do_qef


#
# 	Making libpsf.1
#

if $do_psf
then
    echo ""
    echo " Building lib(PROG0PRFX)psf.1.$SLSFX "
    echo ""
    mkdir $workdir/psf 2> /dev/null ; cd $workdir/psf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libpsf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)psf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)psf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)psf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)psf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)psf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)psf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)psf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)psf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf psf
    fi
fi  # do_psf



#
# 	Making libdbutil.1
#

if $do_dbutil
then
    echo ""
    echo " Building lib(PROG0PRFX)dbutil.1.$SLSFX "
    echo ""
    mkdir $workdir/dbutil 2> /dev/null ; cd $workdir/dbutil
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libdbutil.a

    # Remove unused object from shared lib as they case problems when
    # linking server with shared libraries

    rm -f `cat $bados`
    rm -f duvechk*.o duveutil.o doview.o duvepurg.o duvedbdb.o duvestar.o \
	ducommon.o duicrdb.o duuinits.o dubdsdb.o dusutil.o duveutil.o \
	duvetalk.o duvfiles.o duuerror.o dubcrdb.o duuinpth.o duidsdb.o \
	duvetest.o duifemod.o

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)dbutil.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)dbutil.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)dbutil.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)dbutil.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)dbutil.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)dbutil.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)dbutil.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)dbutil.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf dbutil
    fi
fi  # do_dbutil


#
# 	Making libqsf.1
#

if $do_qsf
then
    echo ""
    echo " Building lib(PROG0PRFX)qsf.1.$SLSFX "
    echo ""
    mkdir $workdir/qsf 2> /dev/null ; cd $workdir/qsf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libqsf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)qsf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)qsf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)qsf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)qsf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)qsf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)qsf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)qsf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)qsf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf qsf
    fi
fi  # do_qsf




#
# 	Making libdmf.1
#

if $do_dmf
then
    echo ""
    echo " Building lib(PROG0PRFX)dmf.1.$SLSFX "
    echo ""
    mkdir $workdir/dmf 2> /dev/null ; cd $workdir/dmf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libdmf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)dmf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)dmf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)dmf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)dmf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)dmf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX *.o \
	$INGBIN/lib(PROG0PRFX)becompat.1.$SLSFX \
        $shlink_opts -soname=lib(PROG0PRFX)dmf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)dmf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)dmf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf dmf
    fi
fi  # do_dmf






#
# 	Making libgwf.1
#

if $do_gwf
then
    echo ""
    echo " Building lib(PROG0PRFX)gwf.1.$SLSFX "
    echo ""
    mkdir $workdir/gwf 2> /dev/null ; cd $workdir/gwf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libgwf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX \*.o $shlink_opts
    echo "Config = $config"
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)gwf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)gwf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)gwf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)gwf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)gwf.1.$SLSFX
        ;;
    *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)gwf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)gwf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)gwf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf gwf
    fi
fi  # do_gwf






#
# 	Making libadf.1
#

if $do_adf
then
    echo ""
    echo " Building lib(PROG0PRFX)adf.1.$SLSFX "
    echo ""
    mkdir $workdir/adf 2> /dev/null ; cd $workdir/adf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libadf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)adf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)adf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)adf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)adf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)adf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)adf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)adf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)adf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf adf
    fi
fi  # do_adf






#
# 	Making libulf.1
#

if $do_ulf
then
    echo ""
    echo " Building lib(PROG0PRFX)ulf.1.$SLSFX "
    echo ""
    mkdir $workdir/ulf 2> /dev/null ; cd $workdir/ulf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libulf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)ulf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)ulf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)ulf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)ulf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)ulf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)ulf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)ulf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)ulf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf ulf
    fi
fi  # do_ulf






#
# 	Making libtpf.1
#

if $do_tpf
then
    echo ""
    echo " Building lib(PROG0PRFX)tpf.1.$SLSFX "
    echo ""
    mkdir $workdir/tpf 2> /dev/null ; cd $workdir/tpf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libtpf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)tpf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)tpf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)tpf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)tpf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)tpf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)tpf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)tpf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)tpf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf tpf
    fi
fi  # do_tpf






#
# 	Making librqf.1
#

if $do_rqf
then
    echo ""
    echo " Building lib(PROG0PRFX)rqf.1.$SLSFX "
    echo ""
    mkdir $workdir/rqf 2> /dev/null ; cd $workdir/rqf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/librqf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)rqf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)rqf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)rqf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)rqf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)rqf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)rqf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)rqf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)rqf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf rqf
    fi
fi  # do_rqf






#
# 	Making libgcf.1
#

if $do_gcf
then
    echo ""
    echo " Building lib(PROG0PRFX)gcf.1.$SLSFX "
    echo ""
    mkdir $workdir/gcf 2> /dev/null ; cd $workdir/gcf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libgcf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)gcf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)gcf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)gcf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)gcf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)gcf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)gcf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)gcf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)gcf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf gcf
    fi
fi  # do_gcf






#
# 	Making libsxf.1
#

if $do_sxf
then
    echo ""
    echo " Building lib(PROG0PRFX)sxf.1.$SLSFX "
    echo ""
    mkdir $workdir/sxf 2> /dev/null ; cd $workdir/sxf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libsxf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)sxf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)sxf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)sxf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)sxf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)sxf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)sxf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)sxf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)sxf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf sxf
    fi
fi  # do_sxf






#
# 	Making libcuf.1
#

if $do_cuf
then
    echo ""
    echo " Building lib(PROG0PRFX)cuf.1.$SLSFX "
    echo ""
    mkdir $workdir/cuf 2> /dev/null ; cd $workdir/cuf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libcuf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)cuf.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)cuf.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)cuf.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)cuf.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)cuf.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)cuf.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)cuf.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)cuf.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf cuf
    fi
fi  # do_cuf






#
# 	Making libmalloc.1
#

if $do_malloc
then
    echo ""
    echo " Building lib(PROG0PRFX)malloc.1.$SLSFX "
    echo ""
    mkdir $workdir/malloc 2> /dev/null ; cd $workdir/malloc
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libmalloc.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    rm -f $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX
    echo $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX \*.o $shlink_opts
    case $config in
    dgi_us5 | dg8_us5 | nc4_us5 | sqs_ptx | sos_us5 | rmx_us5 | \
    rux_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)malloc.1.so
           ;;
    sos_us5)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)malloc.1.$SLSFX
           ;;
    usl_us5 )
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX *.o $shlink_opts \
            -hlib(PROG0PRFX)malloc.1.so
           ;;
    dr6_us5)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX *.o \
        $shlink_opts -hlib(PROG0PRFX)malloc.1.$SLSFX
        ;;
    ts2_us5)
        cp /usr/lib/so_locations .
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX *.o $shlink_opts \
        -no_unresolved -transitive_link -check_registry ./so_locations \
        -soname lib(PROG0PRFX)malloc.1.$SLSFX
        ;;
      *_lnx|int_rpl)
        $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX *.o \
        $shlink_opts -soname=lib(PROG0PRFX)malloc.1.$SLSFX
        ;;
    *)
           $shlink_cmd -o $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX *.o $shlink_opts
           ;;
    esac
    if [ $? != 0 -o ! -r $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX ]
    then
        echo $0 Failed,  aborting !!
        exit 4
    else
        [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] \
	  && archive_lib lib(PROG0PRFX)malloc.1
        # on some systems, libs being non writable may add to performance
        chmod 755 $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX
        ls -l $INGBIN/lib(PROG0PRFX)malloc.1.$SLSFX
        echo Cleaning up ....
        cd $workdir; rm -rf malloc
    fi
fi  # do_malloc
