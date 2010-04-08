:
# Copyright (c) 2004 Ingres Corporation
#
# Name:
#	iiecholg - echo to stdout and to log file
#
# Usage:
#	iiecholg  [argument ...]
#
# Description
#	echo arguments on standard out and on log file given by file
#	descriptor in environment variable LOG_FD.
#	If no arguments then read standard input.
#
## History
##	8/31/88 (peterk) - created
##      012494 (newca01) - copied from 6.4 echolog and changed name
##	26-apr-2000 (somsa01)
##	    Updated MING hint for program name to account for different
##	    products.
#
#  PROGRAM = (PROG1PRFX)echolg
#
#  DEST = utility
#----------------------------------------------------------------------

if [ $# != 0 ]
then
    echo $*; [ $LOG_FD ] && echo $* >& $LOG_FD
else
    if [ $LOG_FD ]
    then
	cat >/tmp/echolog.$$
	cat /tmp/echolog.$$
	cat /tmp/echolog.$$ >& $LOG_FD
	rm /tmp/echolog.$$
    else
	cat
    fi
fi
