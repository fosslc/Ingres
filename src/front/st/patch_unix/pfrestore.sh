:
#  Copyright (c) 2004 Ingres Corporation
#  Name: pfrestore
#
#  Usage:
#   wrapper for pffiles	
#
##  Description:
##	Executed in-line by ingbuild on patch installation
##
## History:
##   15-March-1996 (angusm)	
##		created.
##   28-apr-2000 (somsa01)
##		Enabled for different product builds.
##
#  DEST = utility
#----------------------------------------------------------------------------

if [ "$1" = "" ]
then
	exit 255
fi

$(PRODLOC)/(PROD2NAME)/install/pffiles $1 out || exit 255

exit 0
