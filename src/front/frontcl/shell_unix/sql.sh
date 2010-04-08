:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Name:
#	sql- wrapper to Terminal Monitor.
#
## History:
##	20-apr-2000 (somsa01)
##	    Updated MING hint for program name to account for different
##	    products.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG0PRFX)sql
# -----------------------------------------------------------------
(LSBENV)

exec (PROG0PRFX)tm -qSQL "$@"
