:
#
# Copyright (c) 2004 Ingres Corporation
#
#	This script builds `gv.h'
#
# History:
#	31-jul-1989 (boba)
#		Change conf to tools/port$noise/conf.
#	24-oct-1992 (lauraw)
#		Added support for readvers.sh and new VERS format.
#	26-oct-1992 (lauraw)
#		Modified to build version strings with genrelid.
#		Removed ING_VERS check since $noise is never referenced.
#		Took out "2> /dev/null" so if anything goes wrong user has a 
#		clue.
#	30-oct-1992 (lauraw)
#		Changed the "trap '' 0" at the end of this script to "trap 0"
#		because on some platforms the former has no effect. In any
#		case, the latter is correct -- we are *unsetting* the trap
#		for 0, not setting it to be ignored.
#	25-feb-97 (funka01)
#		Added option for creating gv.h for jasmine.
#		Use mkgv JASMINE to create jasmine's gv.h
#	20-apr-2000 (somsa01)
#		Added other products as alternate products.
#	20-aug-2002 (hanch04)
#		Changes for 32/64 bit dual build.
#	21-aug-2002 (hanch04)
#		Changes for 32/64 bit dual build.
#		The release id for every exe (32 or 64) should have the
#		new 64 bit identifier.
#       11-Feb-2004 (hweho01)
#               SIR 111718
#               Add initialization of the version structure to the automatic 
#               header generation on Unix platforms.
#       12-Feb-2004 (hweho01)
#               Add define statements for GV_HW and GV_OS.
#       08-Mar-2004 (hweho01)
#               Modified the processing of GV_HW and GV_OS definitions,   
#               so it is compatible on int_lnx platform.
#	11-Jun-2004 (somsa01)
#		Cleaned up code for Open Source.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	07-Mar-2005 (hanje04)
## 	    SIR 114034
##	    Add support for reverse hybrid, don't define vers for Linux.
##	12-Apr-2005 (sheco02)
## 	    Fix conf_ADD_ON32 test condition for 64 bit platforms.
##	21-Apr-2005 (hanje04)
##	    Add support for MAx OS X.
##  01-Jun-2005 (fanra01)
##      SIR 114614
##      Add definition of GV_BLDLEVEL to gv.h definitions.
##	9-Sep-2006 (bonro01)
##	    Simplify genrelid so that is does not require -lpHB parm.
##	11-Oct-2007 (hanje04)
##	    SIR 114907
##	    awk on Mac OSX 10.4 needs a space between '-v' & 'var=val'
##	    (bug ?)
##	19-Nov-2007 (bonro01)
##	    Fix problem caused by last change for Mac OSX which breaks awk
##	    on Solaris.
##	14-Feb-2008 (hanje04)
##	    SIR S119978
##	    Wrap "# define $vers" in "#ifndef" to prevent redefinition warnings
##	    on OS X
##	18-Jun-2009 (kschendel) SIR 122138
##	    Don't ever define vers here.  It should come from bzarch.h which
##	    is included by compat.h, which pretty much everything includes.

CMD=`basename $0`

header=gv.h
date=`date`

# stdout into newly created header

exec > $header

# don't leave it around if we have trouble

trap 'rm $header' 0

# create a new target:

cat << !
/*
** $header created on $date
** by the $0 shell script
*/
!

. readvers
. iisysdep

echo "# include <gvcl.h>"

gvenv=`ccpp -s GV_ENV`
echo "# define	GV_ENV $gvenv"

# Check for product.
 
case $# in
   0) prod=INGRES
      ;;
   1) case $1 in
 
         INGRES) prod=$1
                           ;;
         *) echo "$CMD: Invalid product type \"$1\"" >&2
            exit 1
            ;;
      esac
      ;;
   *) echo "$CMD: Too many arguments" >&2
      exit 1
      ;;
esac


gvver=`genrelid $prod` && 
   echo "# define	GV_VER \"$gvver\"" ||
   {
      echo "$CMD: Unable to generate release id. Check CONFIG, VERS, etc." >&2
      exit 1
   }

echo ""

echo ""
echo "/*"
echo "**"
echo "** Value definitions for components of the version string."
echo "*/"
ccpp -s ING_VER | sed -e 's/.* //g' -e 's,/,.,g' | $AWK -v build=$build -F. '{printf "# define GV_MAJOR %d\n# define GV_MINOR %d\n# define GV_GENLEVEL %d\n# define GV_BLDLEVEL %d\n", $1, $2, $3, build}'

if [ "$conf_DBL" ]
then
    echo "# define GV_BYTE_TYPE 2"
else
    echo "# define GV_BYTE_TYPE 1"
fi

ihw=`echo $config | sed "s/_...//g"`
ios=`echo $config | sed "s/..._//g"`

# Note: /usr/bin/echo should be used if it's there
if [ -x /usr/bin/echo ] ; then
   ECHO_CMD="/usr/bin/echo"
else
   ECHO_CMD="echo"
fi

# od has been depricated on Mac OSX, use hexdump instead
if [ "$ios" = "osx" ] ; then
    # Example: if $ihw is "su4", then the generated statement should be:
    #          "# define GV_HW 0x737534"
    $ECHO_CMD $ihw\\c |  hexdump -C | head -1 | $AWK '{
                     for( i=2; i<=4; ++i )
                     {
                      if ( ( len=length($i) ) > 2 )
                        $i=substr( $i, len-2+1 , 2)
                     }
                     printf "# define GV_HW 0x%s%s%s\n", $2,$3, $4 }'

    # Example: if $ios is "us5", then the generated statement should be:
    #          "# define GV_OS 0x757335"
    $ECHO_CMD $ios\\c | hexdump -C | head -1 | $AWK '{
                     for( i=2; i<=4; ++i )
                     {
                      if ( ( len=length($i) ) > 2 )
                        $i=substr( $i, len-2+1 , 2)
                     }
                     printf "# define GV_OS 0x%s%s%s\n", $2, $3, $4 }'
else
    # Example: if $ihw is "su4", then the generated statement should be:
    #          "# define GV_HW 0x737534"
    $ECHO_CMD $ihw\\c | od -An -t x1 | head -1 | $AWK '{
                     for( i=1; i<=3; ++i )
                     {
                      if ( ( len=length($i) ) > 2 )
                        $i=substr( $i, len-2+1 , 2)
                     }
                     printf "# define GV_HW 0x%s%s%s\n", $1,$2, $3 }'
    # Example: if $ios is "us5", then the generated statement should be:
    #          "# define GV_OS 0x757335"
    $ECHO_CMD $ios\\c | od -An -t x1 | head -1 | $AWK '{
                     for( i=1; i<=3; ++i )
                     {
                      if ( ( len=length($i) ) > 2 )
                        $i=substr( $i, len-2+1 , 2)
                     }
                     printf "# define GV_OS 0x%s%s%s\n", $1, $2, $3 }'
fi # Mac OSX


if [ "$inc" != "" ]
then
    echo "# define GV_BLDINC  $inc"
else
    echo "# define GV_BLDINC  00"
fi

echo ""
echo ""
echo "GLOBALREF ING_VERSION  ii_ver;"
echo ""
echo ""
echo "/* End of $header */"

trap : 0
