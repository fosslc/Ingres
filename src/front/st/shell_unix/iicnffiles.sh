:
#
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name: iicnffiles -- setup script for Ingres setup files
#
#  Usage:
#	iicnffiles
#
#  Description:
#	This script should be called only by the Ingres installation
#	utility.  It creates the Ingres setup files, symbol.tbl and
#	config.dat, if no other setup takes place during install. 
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	28-jan-95 (wonst02)
##		Created - cloned from iisudbms.sh
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products. Also, removed duplicate code.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG1PRFX)cnffiles
#
#  DEST = utility
#----------------------------------------------------------------------------

(LSBENV)

echo "Creating the (PROD1NAME) setup files..."

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    II_CONFIG=$(PRODLOC)/(PROD2NAME)/files
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
fi
export cfgloc symbloc


(PROG1PRFX)sulock "(PROD1NAME) create setup files" || exit 1

stat=1	# set the default exit status

trap 'rm -f ${cfgloc}/config.lck 1>/dev/null \
   2>/dev/null; exit $stat' 0 1 2 3 15

touch_files # make sure symbol.tbl and config.dat exist

stat=0
exit 0

