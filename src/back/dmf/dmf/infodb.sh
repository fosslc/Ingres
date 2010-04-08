:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
#
# Name: infodb	- Show information about the database from the config file
#
# Description:
#      Show information stored in the database.  Information printed includes:
#       	locations, checkpoints, journal, database status, dump
#	        information, ...
#
#      This shell program provides the INGRES `infodb' command by
#      executing the journaling support program, passing along the
#      command line arguments.
#
## History:
##      06-dec-1989 (mikem)
##          Created.
##      15-mar-1993 (dianeh)
##          Changed first line to a colon (":"); changed the History
##          comment block to double pound-signs; deleted unnecessary
##          apostrophe from Description.
##      20-sep-1993 (jnash)
##          Change $* to "$@" to work with delimited id's.
##      29-may-1998 (wanya01)	
##          Add if-else-fi to prevent infodb without argument returns error
##          E_DM103B  in some Unix system (bug 78793)
##      17-jul-1998 (somsa01)
##          Modified previous change to fix bug #92034; the test
##          condition would fail if the "-s" option was passed in.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##     16-Feb-2010 (hanje04)
##         SIR 123296
##         Add support for LSB build.
#
(LSBENV)

if [ $# -gt 0 ]
then
        exec ${II_SYSTEM?}/ingres/bin/dmfjsp infodb "$@"
else
        exec ${II_SYSTEM?}/ingres/bin/dmfjsp infodb
fi
