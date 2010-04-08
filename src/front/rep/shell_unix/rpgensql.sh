:
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Name:	rpgensql - process generated SQL
#
# Description:
#	Calls the SQL terminal monitor to process SQL script.
#
# Parameters:
#	1	- database owner
#	2	- nodename
#	3	- database name
#	4	- script name
#
## History:
##	11-oct-96 (joea)
##		Created based on rpgensql.sh in replicator library.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB build.

(LSBENV)
TMPDIR=`ingprenv II_TEMPORARY`
sql -u$1 $2::$3 < $4 > $TMPDIR/rpgen.log
