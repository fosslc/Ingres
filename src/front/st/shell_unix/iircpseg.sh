:
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name: iircpseg -- return the size of the rcp shared memory segment
#
#  Usage:
#	iircpseg [ temp_file ]
#
#  Description:
#	Returns the output from "iishowres" to the configuration rules
#	system.
#
## History:
##	18-jan-93 (tyler)
##		Created.	
##	13-may-93 (vijay)
##		This could be called before iishowres is available. Check.
##	13-may-93 (vijay)
##		Roll back previous change, since we will be shipping iishowres
##		as a link to iimerge. It will be available before iilink is
##		called.
##	28-apr-2000 (somsa01)
##		Enabled multiple product builds.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG4PRFX)rcpseg
#
#  DEST = utility
#----------------------------------------------------------------------------

(LSBENV)

if iishowres > /tmp/(PROG4PRFX)rcpseg.$$
then 
	cat /tmp/(PROG4PRFX)rcpseg.$$ > "$1"
else
	echo 0 > "$1"
fi

rm -f /tmp/(PROG4PRFX)rcpseg.$$

