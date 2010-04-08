:
#
# Copyright (c) 2004 Ingres Corporation
#
# Copy all deliverable vec files from $ING_SRC to $ING_BUILD.
#
# This script is a temporary cludge until somebody writes the MING file
# to do this correctly.
#
## History:
##       25-apr-89 (boba)
##               Add 6.1 support.
##       31-jul-1989 (boba)
##               Remove 5.0 stuff when added to ingresug.
##		Change conf to tools/port$noise/conf.
##       27-nov-89 (rudyw)
##               Remove line to 'chmod' dot and serial since they
##               are longer part of the RELEASE list.
##	21-dec-1989 (boba)
##		Fix initial echo.  Source now in $ING_SRC/vec31.
##	19-jun-1990 (jonb)
##		Change to used dlist.ccpp rather than RELEASE, as
##		RELEASE no longer exists.  This is based on the,
##		I hope, reasonable assumption that all the vec
##		files will be in dlist and none of them will be
##		in ilist.
##	06-oct-1991 (gillnh2o)
##		Changed dlist.ccpp to Manifest.ccpp since dlist.ccpp
##		no longer exist.
##	24-mar-1993 (dianeh)
##		Modified to work without Manifest.ccpp -- gone for 6.5;
##		besides, there were never any vec files separately #ifdef'd
##		off anyway, so just get all the deliverable files in
##		$ING_SRC/vec31/usr/vec and put them in $ING_BUILD/vec
##		(whether they get delivered on the distrubution will be
##		determined by Manifest.ing in conjunction with buildrel);
##		changed History comment block to double pound-signs;
##		added some error-checking.
##	17-sep-1993 (dianeh)
##		Use iimaninf instead of find to get the vec files.
##	11-Jan-1993 (fredv)
##		Changed graphics to vigraph as this what iimaninf understands
##		now.
##	02-feb-94 (vijay)
##		A bug in iimaninf has been fixed, and it no longer prints the
##		files if install.dat tells it that vigraph is installed.
##		)-: Temporarily move install.dat if it exists in II_SYSTEM.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

CMD=`basename $0`

# Check for required environmentals
: ${ING_SRC?"must be set"}
: ${ING_BUILD?"must be set"}

echo "Copying files from $ING_SRC/vec31/usr/vec to $ING_BUILD/vec ..."

# Make sure that install.dat does not interfere
[ -f $II_SYSTEM/ingres/files/install.dat ] &&
{
	cd $II_SYSTEM/ingres/files
	mv install.dat install.dat.$$
}

# Find files
FILES=`iimaninf -resolve=vigraph | grep ingres/vec | \
       awk '{ print $1,$2 ; }' | sed -e "s:^ingres/::" -e "s: :/:"`

[ -f $II_SYSTEM/ingres/files/install.dat.$$ ] && mv install.dat.$$ install.dat

# Copy files
echo $FILES | tr ' ' '\012' | \
( cd $ING_SRC/vec31/usr; gtar cTfb - - 8 ) | ( cd $ING_BUILD; gtar xpfb - 8 )

# Set permissions for files
cd $ING_BUILD/vec
find . -type f -exec chmod 644 {} ';'

echo "$CMD done."
exit 0
