:
#
# Copyright (c) 2004 Ingres Corporation
#
#***************************************************************************
# Name:
#	iirunschd
#
# Usage:
#	iirunschd
#
# Description:
#	iirunschd starts up the SQL Scheduler, a distributed database 
#	replicator.
#
# Exit status:
#	0 - OK
#	3 - Creation of iialarmdb failed
#	4 - User declined creation of iialarmdb
#	5 - Scheduler init failed
#	6 - Scheduler would not run
#
## History:
##	23-aug-1993 (dianeh)
##		Created (from ingres63p!unix!admin!install!shell!iirunschd.sh);
##		updated for 6.5 (call to chkins deleted, echolog to iiecholg,
##		ask to iiask4it, pointers to where things live, etc.)
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	20-Jan-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds.
##
#***************************************************************************
(LSBENV)

. iisysdep

[ $CMD ] || { CMD=iirunschd ; export CMD ; }

# --------------------------------------------------------------------------

# Start the scheduler
# Check if iialarmdb exists. Prompt to create it if it does not.

infodb -uingres iialarmdb > /dev/null 2>&1
if [ "$?" != "0" ] ; then
    iiecholg "The scheduler requires the database iialarmdb to operate."
    iiecholg "The database iialarmdb does not exist yet."
    if iiask4it "Do you want to create iialarmdb?" ; then
	iiecholg "Creating iialarmdb ..."
	createdb iialarmdb
	if [ "$?" != "0" ] ; then
	    iiecholg << !

 -------------------------------------------------------------------------
|                            ***ERROR***                                  |
| iialarmdb could not be created successfully.  Please check that the     |
| installation is up. You may need to destroy iialarmdb before retrying.  |
| Please rerun `iirunschd' later.                                         |
 -------------------------------------------------------------------------

!
	    exit 3
	else
	    iiecholg "iialarmdb created successfully."
	    break
	fi
    else
	iiecholg << !

 --------------------------------------------------------------------------
|                            ***ERROR***                                   |
| The scheduler cannot operate without "iialarmdb"; scheduler not started. |
 --------------------------------------------------------------------------

!
	exit 4
    fi
fi

# See if iialarmdb needs to be initialized.
# Assume that if i_user_alarmed_dbs exists, then it has been initialized.

output=`echo "help view i_user_alarmed_dbs;\g" | sql -s iialarmdb | \
    grep "No views were found"`

if [ ! -z "$output" ] ; then
    iiecholg "Initializing iialarmdb ..."
    scheduler init master
    rc=$?
    if [ "$rc" != "0" ] ; then
        iiecholg << !

 --------------------------------------------------------------------------
|                            ***ERROR***                                   |
| An error occured while initializing "iialarmdb"; scheduler exited $rc.   |
| Please fix the error and re-initialize "iialarmdb".                      |
 --------------------------------------------------------------------------

!
	exit 5
    fi
fi

ii=`ingprenv II_INSTALLATION`

# Now run the scheduler.
sched=$II_SYSTEM/ingres/sig/star/scheduler
logf=$II_SYSTEM/ingres/files/iischeduler$ii.log

nohup $sched $ii > $logf 2>&1 &

pid=$!
# sleep a bit, and see if it stayed up.
sleep 10
ps $! > /dev/null 2>&1
if [ "$?" != "0" ] ; then
    iiecholg << !

 --------------------------------------------------------------------------
|                            ***ERROR***                                   |
| The scheduler failed to run; please check                                |
| "$logf"
 --------------------------------------------------------------------------

!
    exit 6
fi

exit 0
