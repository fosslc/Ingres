:
#
# Copyright (c) 1994, 2010 Ingres Corporation
#
# Name:
#	esqlcc - wrapper to esqlc for preprocessing C++ files.
#
## History:
##	19-mar-2001 (somsa01)
##	    Updated MING hint for program name to account for different
##	    product builds.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG0PRFX)esqlcc
# -----------------------------------------------------------------
(LSBENV)

exec (PROG0PRFX)esqlc -cplusplus "$@"
