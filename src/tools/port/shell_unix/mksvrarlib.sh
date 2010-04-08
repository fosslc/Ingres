:
#
# Copyright (c) 2004 Ingres Corporation
#
# DEST = utility
#
# Name:
#       mksvrarlib.sh
#
# Usage: mksvrarlib [becompat] [scf] [psf] [opf] [rdf] [qef]
#		      [qsf] [dbutil] [tpf] [rqf] [gcf] [sxf] [cuf] 
#
#       if no argument given, it will add all thirteen facilities to
#       iimerge.a.
#
# Description:
#
#       Creates the archive iimerge.a for the Ingres server. 
#
#	Objects are replaced (ar ru) in iimerge.a, so if there are any
#	deleted objects, they will stay in iimerge.a.  The simplest thing
#	to do if there is any question is to just delete the old iimerge.a
#	(by hand) before running this.
#
#       Before running this script, you need to set up various variables such
#       as shlink_cmd and shlink_opts in shlibinfo.sh.
#
#
# History
#
#	 8-Oct-2007 (hanal04) Bug 119262
#	    Created from mkvrshlibs.sh and mergelibs.sh
#       23-Oct-2007 (hanal04) Bug 119262
#           Updated options on ar to ensure we update only newer objects
#           instead of appending object and getting multiple definitions.
#       05-Jun-2009 (frima01) SIR 121775
#           In order to create the server from iimerge.a don't remove
#           cssl.o from libcompat objects for AIX.
#	18-Jun-2009 (kschendel) SIR 122138
#	    Hybrid mechanisms changed, fix here.
#	    Clean out work directories after doing each phase.

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
	INGLIB=$ING_BUILD/lib/${LPHB}
	BITS=$HB
else
	LPHB=
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
if [ "$config64" = 'r64_us5' -a "$BITS" = 64 ] ; then
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
[ "$config" = "ris_us5" ] &&
mkexplist()
{
    ls -1 *.o | xargs -n50 /usr/ucb/nm -p |
      awk ' $2=="D"||$2=="B" { print $3 } ' > expsym_list
}
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
    cd $INGLIB
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
#       Adding becompat to iimerge.a
#

if $do_becompat
then
    echo ""
    echo " Adding becompat to iimerge.a "
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
    if [ "$config" = "rs4_us5" ] ; then
       rm -f  mebdump.o meberror.o mexdump.o medump.o meconsist.o
    else
       rm -f  cssl.o mebdump.o meberror.o mexdump.o medump.o meconsist.o
    fi

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_becompat



#
# 	Adding scf to iimerge.a
#

if $do_scf
then
    echo ""
    echo " Adding scf to iimerge.a "
    echo ""
    mkdir $workdir/scf 2> /dev/null ; cd $workdir/scf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libscf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_scf



#
# 	Adding opf to iimerge.a
#

if $do_opf
then
    echo ""
    echo " Adding opf to iimerge.a "
    echo ""
    mkdir $workdir/opf 2> /dev/null ; cd $workdir/opf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libopf.a

    # Remove unwanted objects
    rm -f opqutils.o

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_opf



#
# 	Adding rdf to iimerge.a
#

if $do_rdf
then
    echo ""
    echo " Adding rdf to iimerge.a "
    echo ""
    mkdir $workdir/rdf 2> /dev/null ; cd $workdir/rdf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/librdf.a
#    echo Removing unwanted objects ...
#    rm -f rdfgetdesc.o rdfgetinfo.o rdfinvalid.o

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_rdf



#
# 	Adding qef to iimerge.a
#

if $do_qef
then
    echo ""
    echo " Adding qef to iimerge.a "
    echo ""
    mkdir $workdir/qef 2> /dev/null ; cd $workdir/qef
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libqef.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_qef



#
# 	Adding psf to iimerge.a
#

if $do_psf
then
    echo ""
    echo " Adding psf to iimerge.a "
    echo ""
    mkdir $workdir/psf 2> /dev/null ; cd $workdir/psf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libpsf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_psf



#
# 	Adding dbutil to iimerge.a
#

if $do_dbutil
then
    echo ""
    echo " Adding dbutil to iimerge.a "
    echo ""
    mkdir $workdir/dbutil 2> /dev/null ; cd $workdir/dbutil
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libdbutil.a

    # Remove unused objects

    rm -f `cat $bados`
    rm -f duvechk*.o duveutil.o doview.o duvepurg.o duvedbdb.o duvestar.o \
	ducommon.o duicrdb.o duuinits.o dubdsdb.o dusutil.o duveutil.o \
	duvetalk.o duvfiles.o duuerror.o dubcrdb.o duuinpth.o duidsdb.o \
	duvetest.o duifemod.o

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_dbutil



#
# 	Adding qsf to iimerge.a
#

if $do_qsf
then
    echo ""
    echo " Adding qsf to iimerge.a "
    echo ""
    mkdir $workdir/qsf 2> /dev/null ; cd $workdir/qsf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libqsf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_qsf



#
# 	Adding dmf to iimerge.a
#

if $do_dmf
then
    echo ""
    echo " Adding dmf to iimerge.a "
    echo ""
    mkdir $workdir/dmf 2> /dev/null ; cd $workdir/dmf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libdmf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_dmf



#
# 	Adding gwf to iimerge.a
#

if $do_gwf
then
    echo ""
    echo " Adding gwf to iimerge.a "
    echo ""
    mkdir $workdir/gwf 2> /dev/null ; cd $workdir/gwf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libgwf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_gwf



#
# 	Adding adf to iimerge.a
#

if $do_adf
then
    echo ""
    echo " Adding adf to iimerge.a "
    echo ""
    mkdir $workdir/adf 2> /dev/null ; cd $workdir/adf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libadf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_adf



#
# 	Adding ulf to iimerge.a
#

if $do_ulf
then
    echo ""
    echo " Adding ulf to iimerge.a "
    echo ""
    mkdir $workdir/ulf 2> /dev/null ; cd $workdir/ulf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libulf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_ulf



#
# 	Adding tpf to iimerge.a
#

if $do_tpf
then
    echo ""
    echo " Adding tpf to iimerge.a "
    echo ""
    mkdir $workdir/tpf 2> /dev/null ; cd $workdir/tpf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libtpf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_tpf



#
# 	Adding rqf to iimerge.a
#

if $do_rqf
then
    echo ""
    echo " Adding rqf to iimerge.a "
    echo ""
    mkdir $workdir/rqf 2> /dev/null ; cd $workdir/rqf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/librqf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_rqf



#
# 	Adding gcf to iimerge.a
#

if $do_gcf
then
    echo ""
    echo " Adding gcf to iimerge.a "
    echo ""
    mkdir $workdir/gcf 2> /dev/null ; cd $workdir/gcf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libgcf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_gcf



#
# 	Adding sxf to iimerge.a
#

if $do_sxf
then
    echo ""
    echo " Adding sxf to iimerge.a "
    echo ""
    mkdir $workdir/sxf 2> /dev/null ; cd $workdir/sxf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libsxf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_sxf



#
# 	Adding cuf to iimerge.a
#

if $do_cuf
then
    echo ""
    echo " Adding cuf to iimerge.a "
    echo ""
    mkdir $workdir/cuf 2> /dev/null ; cd $workdir/cuf
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libcuf.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_cuf



#
# 	Adding malloc to iimerge.a
#

if $do_malloc
then
    echo ""
    echo " Adding malloc to iimerge.a "
    echo ""
    mkdir $workdir/malloc 2> /dev/null ; cd $workdir/malloc
    [ $? != 0 ] &&  { echo Aborting ... ; exit 3 ;}

    rm -f *
    echo Extracting objects ...
    ar x $INGLIB/libmalloc.a

    [ "$config" = "rs4_us5" -o "$config" = "r64_us5" ] && mkexplist
    echo ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
    case $config in
    *)
        ar ru$AR_L_OPT $INGLIB/iimerge.a *.o
        ;;
    esac
    rm -f *.o
fi  # do_malloc
