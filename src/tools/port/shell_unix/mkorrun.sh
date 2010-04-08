:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkorrun:
#
#	Extract modules out of libcompat.a, create the liborrun archive
#	and populate with libcompat's modules minus the si layer.
#	May be run more than once.
#
## History:
#	17-jan-2001 (crido01)
#	13-feb-2001 (crido01)
#	    Remove extracted .o files
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

: ${ING_SRC?"must be set"}

CMD=`basename $0`

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

CONFDIR=$ING_SRC/tools/port$noise/conf

[ -d ${ING_BUILD?"must be set"} ] ||
{ echo "Creating $ING_BUILD..." ; mkdir -p $ING_BUILD 2>/dev/null ||
  { echo "$CMD: Cannot create $ING_BUILD. Check permissions." ; exit 1 ; } ; }

rm $ING_BUILD/lib/liborrun.a

cd $ING_SRC/w4gl/w4glcl/spec_unix/orrun

ar -t $ING_BUILD/lib/libcompat.a > ar.ccpp
ar -x $ING_BUILD/lib/libcompat.a
rm si*

ccpp -Uunix ar.ccpp | while read filenm
do
case $filenm in
si*)
  { echo "Skipping module $filenm..." ; } ;;  
*)
  { echo "Adding module $filenm..." ; ar -r $ING_BUILD/lib/liborrun.a $filenm ; } ;;
esac
done

rm $ING_SRC/w4gl/w4glcl/spec_unix/orrun/*.o
exit 0
