:
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Name:	rpserver.sh - run Replicator Server
#
# Description:
#	Starts Replicator Server after switching to the server directory.
#	$1 is the server number.
#
## History:
##	15-oct-96 (joea)
##		Created based dd_server1.sh in replicator library.
##	11-dec-97 (joea)
##		Replace RepServer executable name and location.
##	26-jun-98 (abbjo03)
##		If DD_RSERVERS is not defined, use default.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB build.
##
(LSBENV)

rservers=`ingprenv DD_RSERVERS`
if [ -z "$DD_RSERVERS" ]
then
    if [ -z "$rservers" ]
	then DD_RSERVERS=$II_SYSTEM/ingres/rep
    else
	DD_RSERVERS=$rservers
    fi
fi
cd $DD_RSERVERS/servers/server$1
$II_SYSTEM/ingres/bin/repserv > print.log &
