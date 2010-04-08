:
#  Copyright (c) 2004 Ingres Corporation
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
##	20-aug-2003 (somsa01)
##	    Created.
##	20-aug-2003 (hweho01)
##	    Added chagnes for AIX 64-bit build.
##
#  DEST = utility
#----------------------------------------------------------------------------


if [ "$(PRODLOC)" = "" ]
then
	exit 255
fi

os_name=`uname -s`
if [ "$os_name" = "AIX" ] ; then
  rm -f $(PRODLOC)/(PROD2NAME)/lib/libcompat64*1.*
  rm -f $(PRODLOC)/(PROD2NAME)/lib/libframe64*1.*
  rm -f $(PRODLOC)/(PROD2NAME)/lib/libinterp64*1.*
  rm -f $(PRODLOC)/(PROD2NAME)/lib/libq64*1.*
else
  rm -f $(PRODLOC)/(PROD2NAME)/lib/lp64/libcompat*1.*
  rm -f $(PRODLOC)/(PROD2NAME)/lib/lp64/libframe*1.*
  rm -f $(PRODLOC)/(PROD2NAME)/lib/lp64/libinterp*1.*
  rm -f $(PRODLOC)/(PROD2NAME)/lib/lp64/libq*1.*
fi 

exit 0
