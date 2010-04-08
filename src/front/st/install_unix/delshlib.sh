:
# Copyright (c) 2004 Ingres Corporation
#
#  Name: delshlib -- delete shared libraries on install
#
#  Usage:
#	delshlib
#
#  Description:
#	Executed in-line by ingbuild on sharelib installation
#
## History:
##	27-Jan-1995 (canor01)
##		created.
##	26-apr-2000 (somsa01)
##		Added changes for multiple product builds.
##
#  DEST = utility
#----------------------------------------------------------------------------

if [ "$(PRODLOC)" = "" ]
then
	exit 255
fi

rm -f $(PRODLOC)/(PROD2NAME)/lib/libcompat*1.*
rm -f $(PRODLOC)/(PROD2NAME)/lib/libframe*1.*
rm -f $(PRODLOC)/(PROD2NAME)/lib/libinterp*1.*
rm -f $(PRODLOC)/(PROD2NAME)/lib/libq*1.*

exit 0
