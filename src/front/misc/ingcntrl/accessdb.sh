:
#
#	Copyright (c) 2004, 2010 Ingres Corporation
#	All rights reserved.
#

#
# Name:    accessdb -	Ingres DBMS Access Control Program.
#
# Description:
#	A UNIX only shell to call IngCntrl in its AccessDB mode.
#
## History:
##	04-jul-1998 (somsa01)
##	    Additional fix for bug #81048; pass the arguments from the command
##	    line "as is" to ingcntrl. The "$*" was stripping out quotes.
##	24-Aug-1998 (schte01) 
##	    Change to only use "$@" when there are parameters being passed.
##	    Without parameters, some unix platforms will receive an E_IC0033
##	    error.
##	23-sep-1998 (bobmart)
##	    Adding "##" for history section so this stuff isn't shipped.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds.
##
(LSBENV)

umask 1

if [ $# -gt 0 ]
then
   ingcntrl "$@" -m
else
   ingcntrl -m
fi
