:
#
# Copyright (c) 2004 Ingres Corporation
#
#
# Name: relocatedb	- Move journal,dump,checkpoint or default work location 
#                         Make a duplicate copy of a database
#
# Description:
#      This shell program provides the INGRES `relocatedb' command by
#      executing the journaling support program, passing along the
#      command line arguments.
#
## History:
##      04-jan-1995 (alison)
##          Created.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
#
# PROGRAM = relocatedb
#

exec ${II_SYSTEM?}/ingres/bin/dmfjsp relocdb "$@"
