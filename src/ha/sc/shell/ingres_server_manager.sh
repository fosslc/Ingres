:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#       ingres_server_manager -- Manager the Ingres processes
#
#  Usage:
#	ingres_server_manager [ -start | -stop | -kill | -status ]
#
#  Description:
#       This program is designed to work in the SUN HA Cluster.
#       It starts an entire Ingres installation.  If Ingres is
#       already running, it shuts it down and restarts it.
#
#	It is assumed that the ingres environment is setup.
#
#  Exit Value:
#       0       the requested operation succeeded.
#       1       the requested operation failed.
# 	100     indicates no servers are running
# 	101-109 indicates the percentage of servers that are running
# 	110     indicates online and all severs are running
#
## History:
##	01-Jan-2004 (hanch04)
##	    Created for SUN HA cluster support.
##	16-Apr-2008 (bonro01)
##	    Fix spelling errors.
##	20-May-2008 (bonro01)
##	    Fix more spelling errors.
##
#  DEST = schabin
#----------------------------------------------------------------------------

#Set the language
LANG=C; export LANG

SELF=`/usr/bin/basename $0`
ECHO=/usr/bin/echo

# set trap on exit to clean up temporary files
trap "cleanup ; exit 1" 1 2 3 15

#
#    usage() - Display the program's usage.
#
usage()
{

    $ECHO "Usage: $SELF [ -start | -stop | -kill | -status ]"
}

#
#    cleanup() - Remove temporary work files on exit
#
cleanup()
{
    [ -z "$II_SCHA_TRACE" ] && rm -f /tmp/*.$$ 2>/dev/null
}

#
#    start_ingres() - Start the ingres installation.
#
start_ingres()
{
    [ ! -x $II_SYSTEM/ingres/utility/ingstart ] && exit 1

    $II_SYSTEM/ingres/utility/ingstart

    return $?
}

#
#    stop_ingres() - Stop the ingres installation.
#
stop_ingres()
{
    [ ! -x $II_SYSTEM/ingres/utility/ingstop ] && exit 1


    $II_SYSTEM/ingres/utility/ingstop

    return $?
}

#
#    kill_ingres() - kill the ingres installation.
#
kill_ingres()
{
    [ ! -x $II_SYSTEM/ingres/utility/ingstop ] && exit 1

    $II_SYSTEM/ingres/utility/ingstop -kill

    return $?
}

#
#    gcc_status() - check the status of the gcc server
#
gcc_status()
{
    # Based on the ingstop method of determining the gcc status.

    . iisysdep

    gcc_count=0    # number of comm server processes

    IIGCFID_FILTER="sed -e 's/ *(.*)//' -e '$SEDTRIM' | sort -u 2> /dev/null"

    # get list of Net servers, report count
    #
    eval "iigcfid iigcc | $IIGCFID_FILTER" > /tmp/iigcc.$$
    gcc_count=`wc -l < /tmp/iigcc.$$ | tr -d ' '`

    echo gcc_count=$gcc_count
}

#
#    dbms_status() - check the status of the dbms server
#
dbms_status()
{
    # Based on the ingstop method of determining the dbms status.

    . iisysdep

    dbms_count=0   # number of dbms server processes
    pmhost=`iipmhost`
    config_dat="$II_SYSTEM/ingres/files/config.dat"

    # Initialize temp files used to record current active objects
    #
    for f in psinst csreport iidbms 
    do
        cat /dev/null > /tmp/$f.$$
    done

    $PSCMD > /tmp/ps.$$

    # get status of Ingres processes from csreport
    #
    for pid in `csreport | grep "inuse 1" | tr , \  | tee /tmp/csreport.$$ | \
       awk '{ print $4 }'`
    do
       grep $pid /tmp/ps.$$ >> /tmp/psinst.$$
    done

    # get list of DBMS servers, report count
    #
    for pid in `grep iidbms /tmp/psinst.$$ | grep -v "/iidbms recovery" | \
       grep -v "/iistar star" | awk '{ print $2 }'`
    do
       grep "pid $pid" /tmp/csreport.$$ | awk '{ print $7 }' >> /tmp/iidbms.$$
    done

    # sort list of DBMS servers
    #
    sort /tmp/iidbms.$$ | sed "$SEDTRIM" > /tmp/siidbms.$$
    mv /tmp/siidbms.$$ /tmp/iidbms.$$

    if grep "$pmhost.*ingstart.*dbms" $config_dat >/dev/null ; then
        dbms_installed=1
    else
        dbms_installed=0
    fi

    total_startup_count=0

    for startup_count in `grep "$pmhost.*ingstart.*dbms" $config_dat | awk '{ print $2 }'`
    do
 	total_startup_count=`expr $total_startup_count + $startup_count` 
    done

    dbms_count=`wc -l < /tmp/iidbms.$$ | tr -d ' '`

    if [ $total_startup_count -ge 1 ] ; then
	confidence_level=`expr 10 \* $dbms_count / $total_startup_count + 100`
    else
	confidence_level=0
    fi

    # 100     indicates no servers are running
    # 101-109 indicates the percentage of servers that are running
    # 110     indicates online and all severs are running

   # rm -f /tmp/*.$$ 2>/dev/null

    return $confidence_level

}

#
#    main() - MAIN
#
main()
{
    #
    # Do a basic sanity check (II_SYSTEM set to a directory).
    #

    [ -z "$II_SYSTEM" -o ! -d "$II_SYSTEM" ] &&
    {
        echo \$II_SYSTEM does not exist.
        exit 1
    }

    case "$1" in

	-dbms_status) dbms_status ; rc=$? ;;

	 -gcc_status) gcc_status ; rc=$? ;;

 	      -start) start_ingres ; rc=$? ;;

	       -stop) stop_ingres ; rc=$? ;;

	       -kill) kill_ingres ; rc=$? ;;

	           *) usage ; rc = 1 ;;
    esac

    exit $rc
}

#
#
#
main "${@:-}"
