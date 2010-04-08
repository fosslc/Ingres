:
#
# Copyright (c) 2004 Ingres Corporation
#
#	This script builds Windows4GL version of `gv.h'
#
# History:
#	31-jul-1989 (boba)
#		Change conf to tools/port$noise/conf.
#	26-nov-1990 (seng)
#		Modified script for use with Windows4GL.
#	14-dec-90 (brett)
#		Added code to change directories into cl/clf/gv.
#	14-feb-92 (jab)
#		Changed to generate the LIRE-version of this file, which
#		is "w4glvers.h".
#	11-jan-93 (lauraw)
#		Modified for new VERS/CONFIG scheme:
#		Now uses readvers to get config string. No longer sets
#		$noise, since that was only used to find VERS file.
#		Uses genrelid to construct release id instead of looking
#		in CONFIG.
#		Took out check for config strings starting with numerics
#		since this script doesn't use it. (That was a hangover from
#		mkgv.)
#	16-feb-93 (gillnh2o)
#		The previous change accidently echoed the #define
#		as GV_W4GL_VER. I changed this to be a #define of
#		W4GL_VERSION.  front!fleas!exeobj!sesobj.sc expects
#		W4GL_VERSION to be defined.
#	30-jul-93 (gillnh2o)
#		Changed the HDRDIR pathname to reflect that w4gl is
#		now a part and the hdrs are in w4gl!hdr!hdr.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

header=w4glvers.h
date=`date`

HDRDIR=$ING_SRC/w4gl/hdr/hdr

if [ ! -d "$HDRDIR" ]
then
	echo "$0: $HDRDIR no directory."
	exit 1
fi

cd $HDRDIR

# stdout into newly created header

if [ -r $header ]
then
	rm -f $header
fi
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

vers=$config

gvw4glver=`genrelid W4GL` &&
   echo "# define	W4GL_VERSION \"$gvw4glver\"" ||
   {
      echo "$CMD: Unable to generate W4GL release id. Check CONFIG, VERS, etc." >&2
      exit 1
   }

echo ""

echo "/* End of $header */"

trap '' 0
