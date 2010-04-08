:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
#
# Name: fastload - fastload a table
#
# Description:
#      This shell program provides the INGRES 'fastload' command by
#      executing the journaling support program, passing along the
#      command line arguments.
#
## History:
##      09-aug-1995 (sarjo01)
##          Created.
##	26-oct-95 (stephenb)
##	    add ":" to start of file so that ming recognises a shell
##	    script instaed of an sql header (also .sh)
##	08-mar-99 (schte01) 
##          Change to only use "$@" when there are parameters being passed.
##          Without parameters, some unix platforms will receive an E_DM103B
##          error.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##     16-Feb-2010 (hanje04)
##         SIR 123296
##         Add support for LSB build.

(LSBENV)

#
# PROGRAM = fastload
#

if [ $# -gt 0 ]
then
exec ${II_SYSTEM?}/ingres/bin/dmfjsp fastload "$@"
else
exec ${II_SYSTEM?}/ingres/bin/dmfjsp fastload
fi
