:
#
# Copyright (c) 2004 Ingres Corporation
#
# genrelid.sh  - generates a release id and echoes it on stdout
#
# This is the one and only place release IDs are generated from VERS
# and CONFIG (using readvers and ccpp, that is). If it all happens here, 
# we can keep it consistent and only have to update the algorithm in one place.
#
## History
##	27-0ct-1992 (lauraw)
##		Created.
##	11-nov-1992 (lauraw)
##		Added OL (OpenLook).
##	25-feb-1993 (jab)
##		Corrected slight coding error: there's a space after the version
##		for all release ID's, as in 'W4GL 2.0/02 (hp8.us5/00)'. The W4GL
##		one was wrong.
##	05-mar-94 (arc)
##		Added ES (Enhanced Security) suffix.
##	01-may-95 (wadag01)
##		Added odt_us5 -> sco_us5 to change from internal to external
##		config string as per 6.4/05.
##	25-feb-97 (funka01)
##		Added product JASMINE to list.
##	10-oct-97 (funka01)
##		Modified output for version string for JASMINE
##	06-Nov-1997 (hanch04)
##	    Added B64 for 64 bit features in OS
##	01-sep-98 (toumi01)
##		Add libc5 and libc6 as suffixes for Linux versions.
##	13-Apr-1999 (hanch04)
##	    Remove tag B64 for 64 bit features in OS
##	10-may-1999 (walro03)
##	    Remove obsolete version string odt_us5.
##	18-jun-99 (toumi01)
##		Remove libc5 and libc6 as suffixes for Linux versions (since
##		we now support only libc6; libc5 is obsolete).
##	20-apr-2000 (somsa01)
##	    Added other products to the product list.
##	22-Feb-2002 (hanje04)
##	    If build incrementis defined in VERS file then display it in
##	    version.rel file.
##	04-mar-2002 (somsa01)
##	    For 2.6 ONLY, if conf_ADD_ON64 is on, then for Solaris the
##	    dotvers is "su9.us5" and for HP-UX 11i it is "hp2.us5".
##	02-Apr-2002 (hanje04)
##	    For Linux SDK builds add SDK to version string and when genrelid is
##	    envoked with a product, don't print inc. 
##	15-Aug-2002 (hanch04)
##	    Added -lp64 to generate a relid in the 32/64 bit build.
##	10-Apr-2003 (hanje04)
##	    Ammend fix for bug 107192, when inc is defined in VERS, print
##	    it as the build number on a separate line.
##	09-may-2003 (abbjo03)
##	    Remove ING_STAR_VER.
##	03-Dev-2003 (hanje04)
##	    BUG 110995
##	    Suffix vers string with LFS for large file support builds on Linux.
##	07-Jan-2003 (hanch04)
##	    Fixed missing $ for conf_LinuxCOMMERCIAL and LFS.
##	29-jan-2004 (stephenb)
##	    Remove DBL suffix for double-byte builds, this will now be
##	    the default build.
##	30-Jan-2004 (hanje04)
##	    Remove LFS suffix for Linux because it is now the default build.
##	11-Jun-2004 (somsa01)
##	    Cleaned up code for Open Source.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	01-Sep-2004 (sheco02)
##	    X-integrate change 467034 and leave out LFS suffix.
##	05-Jam-2005 (hanje04)
##	    Add suffix for NPTL threading build for Linux.
##      06-Jan-2005 (clach04)
##          Added Enterprise Access as a valid product; copy/pasted Ingres
##          version code for the product code "EA".
##	12-Jan-2005 (hanje04)
##	    Fix up NPTL suffix generation.
##	01-Mar-2005 (hanje04)
##	    SIR 114034
##	    Add support for reverse hybrid builds
##	12-Apr-2005 (sheco02)
##	    Fixed conf_ADD_ON32 test condition that caused wrong version.rel.
##	13-Apr-2005 (kodse01)
##	    BUG 114285
##	    Corrected the version.rel generation behavior for hybrid and
##	    reverse hybrid builds.
##	20-Apr-2005 (bonro01)
##	    Back-out previous incorrect change.  The contents of version.rel
##	    is controlled by the Jamrules file and not by the output of
##	    this genrelid script.  This script should continue to default
##	    to output the $config string from VERS and only output the
##	    $config32 or $config64 string when the -lp32 or -lp64 parms are
##	    passed.
##	    Corrected the version.rel generation behavior for hybrid and
##	    reverse hybrid builds.
##	9-Oct-2006 (bonro01)
##	    Simplify genrelid by removing -lpHB parm. genrelid only needs to
##	    output one identifier for a release.  For standard and Reverse
##	    Hybrid builds the release is $config, and for Hybrid builds the
##	    release is $config64
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##      20-Nov-2006 (hanal04) SIR 117044
##          Not using NPTL suffix for int.rpl
##	03-Apr-2007 (hanje04)
##	    SIR 117985
##	    Allow PowerPC Linux (ppc.lnx) to be built for the PS3 as ps3.lnx
##	17-Jun-2009 (kschendel) SIR 122138
##	    Use RELID param from VERS before using config.  Remove
##	    any other hybrid guessing, let readvers do it.

CMD=`basename $0`

. readvers

# Check for product.

prod=INGRES



while [ $# -gt 0 ]
do
    case $1 in
         INGRES|EA|W4GL) prod=$1; inc=""; shift;;
         *) echo "$CMD: Invalid product type \"$1\"" >&2
            exit 1
            ;;
    esac
done

# Set up the suffix:

# (Right now, there are only 2 known suffixes, OL and DBL.
# Others can be added.)

# The order in which suffixes are concatenated is significant.
# Check with Release Management before adding new suffixes into the mix.

suffix=""
[ "$conf_ES" ] && suffix="ES"
[ "$conf_OL" ] && suffix="${suffix}OL"
[ "$conf_LinuxSDK" ] && suffix="SDK"

# On linux set "NPTL" suffix if we're not using SIMULATE_PROCESS_SHARED
case "$config" in
	*_lnx)	cat << EOF > /tmp/nptl.$$ 
# ifndef SIMULATE_PROCESS_SHARED
NPTL
# endif
EOF
		suffix=`ccpp /tmp/nptl.$$`
		rm /tmp/nptl.$$
		;;
esac

# Output the release id:
# Use option RELID from vers if given;
# Use 64-bit variant if 32+64 bit hybrid;
# Use special id if demo PS3 build;
# else (non-hybrid, or non-add-on, or 64+32 reverse hybrid) use primary config.
 
if [ -n "$conf_RELID" ]
then
    dotvers=`echo "$conf_RELID" | tr "_" "."`
elif [ "$build_arch" = '32+64' ] ; then
    dotvers=`echo "$config64" | tr "_" "."`
elif [ "$config" = "ppc_lnx" ] && [ "$conf_PS3_BUILD" ] ; then
    dotvers=ps3.lnx # special case for demo build
else
    dotvers=`echo "$config" | tr "_" "."`
fi

case $prod in

INGRES)
   VER=`ccpp -s ING_VER` &&
      echo "${VER} (${dotvers}/${build})${suffix}"  || exit 1
    	 [ "${inc}" ] && echo "Build_${inc}"
	 exit 0

   ;;

EA)
   VER=`ccpp -s EA_VER` &&
      echo "${VER} (${dotvers}/${build})${suffix}"  || exit 1
         [ "${inc}" ] && echo "Build_${inc}"
         exit 0
   ;;

W4GL)
   VER=`ccpp -s ING_W4GL_VER` &&
      echo "W4GL ${VER} (${dotvers}/${build})${suffix}" || exit 1
   ;;

esac

