:
#
#	Copyright (c) 2004, 2010 Ingres Corporation
#	All rights reserved.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds.
#

#
# Name:    catalogdb -	Ingres DBMS Access Control Program.
#
# Description:
#	A UNIX only shell to call IngCntrl in its catalogdb mode.
#
(LSBENV)
umask 1

ingcntrl $*
