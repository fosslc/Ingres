:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#   	ingres_probe -- Probe the Ingres processes.
#
#  Usage:
#       ingres_probe -R <resource name> -T <type> -G <group>
#
#  Description:
#	This program is designed to work in the SUN HA Cluster.
#
#  Exit Value:
#
## History:
##	01-Jan-2004 (hanch04)
##	    Created for SUN HA cluster support.
##      03-Oct-2005 (bonro01)
##          Corrected path for scripts.
##	16-Apr-2008 (bonro01)
##	    Fix spelling errors.
##	20-May-2008 (bonro01)
##	    Certain shell syntax containing multiple ;; without an intervening
##	    command fails to execute under bash, sh and ksh shells
##
#  DEST = schabin
#----------------------------------------------------------------------------

#Set the language
LANG=C; export LANG

II_SCHA_TRACE=0
II_SCHA_LOG=""
METHOD=probe
SYSLOG_TAG="SC[,,,$METHOD]"
LOGGER=/usr/cluster/lib/sc/scds_syslog

SELF=`/usr/bin/basename $0`
SU=/usr/bin/su
GREP=/usr/bin/grep
NAWK=/usr/bin/nawk
ECHO=/usr/bin/echo

#
#    usage() - Display the program's usage.
#
usage()
{
    $ECHO "usage : $SELF -R <resource name> -T <type> -G <group>"

    
    if [ "$illegal_option" = "YES" ] ; then
        $ECHO "$SELF : Unknown option ($argument_passed) passed."
    fi
}

#
#    scds_syslog() - write info out to syslog and trace log
#
#	scds_syslog [-p priority] [-t tag] -m <message_format> [arg [...]]
#
scds_syslog()
{
    # Output to the Sun cluster system log file.
    $LOGGER "$@" &

    while [ -n "$1" ]
    do
	case "$1" in
	    -p) shift; priority=$1 ;;
	    -t) shift; tag=$1 ;;
	    -m) shift; message_format=$1 ; shift; variables=$@ ;;
	    *)  shift ;;
	esac
	[ -n "$1" ] && shift
    done
    message=`/usr/bin/printf "$message_format" $variables`

    [ -n "$II_SCHA_LOG" ] &&
        $ECHO `date '+%b %d %T'` $SYSLOG_TAG: $message >> $II_SCHA_LOG
}

#
#    trace_log() - write info out to trace log, but not to syslog
#
#	trace_log <message_format> [arg [...]]
#
trace_log()
{
    message_format="$1"
    shift
    if [ -n "$1" ] ; then
        message=`/usr/bin/printf "$message_format" $@`
    else
        message=`/usr/bin/printf "$message_format" `
    fi

    [ -n "$II_SCHA_LOG" ] &&
        $ECHO `date '+%b %d %T'` $SYSLOG_TAG: $message >> $II_SCHA_LOG
}

#
#    parse_arguments() - Parse the program arguments.
#
parse_arguments()
{
    RESOURCE_NAME=""
    RESOURCEGROUP_NAME=""
    RESOURCETYPE_NAME=""
    illegal_option=""
    argument_passed=""

    while [ -n "$1" ]
    do
        case "$1" in
            # Name of the ingres resource.
            -R)  shift; RESOURCE_NAME="$1" ;;

            # Name of the resource group in which the resource is
            # configured.
            -G)  shift; RESOURCEGROUP_NAME="$1" ;;

            # Name of the resource type.
            -T)  shift; RESOURCETYPE_NAME="$1" ;;

            *)  illegal_option="YES"
                argument_passed="$1"
	        usage ;;
        esac
        shift
    done

    SYSLOG_TAG="SC[$RESOURCETYPE_NAME,$RESOURCEGROUP_NAME,$RESOURCE_NAME,$METHOD]"

    if [ -z "$RESOURCE_NAME" ] ; then
	scds_syslog -p error -t $SYSLOG_TAG -m "RESOURCE NAME is not defined."
	return 1
    fi
    if [ -z "$RESOURCETYPE_NAME" ] ; then
	scds_syslog -p error -t $SYSLOG_TAG -m "RESOURCE TYPE is not defined."
	return 1
    fi
    if [ -z "$RESOURCEGROUP_NAME" ] ; then
	scds_syslog -p error -t $SYSLOG_TAG -m "RESOURCE GROUP is not defined."
	return 1
    fi
    if [ -n "$illegal_option" ] ; then
	scds_syslog -p error -t $SYSLOG_TAG -m "Error: Unknown option (%s) passed." \
	    $argument_passed
	return 1
    fi

    [ "$II_SCHA_TRACE" -ge 1 ] &&
	trace_log "RESOURCE_NAME=%s RESOURCEGROUP_NAME=%s RESOURCETYPE_NAME=%s METHOD=%s" \
	    $RESOURCE_NAME $RESOURCETYPE_NAME $RESOURCEGROUP_NAME $METHOD

    return 0
}

#
#    get_extension_property() - access  information about a resource that is under the 
#	control of the Resource Group Manager (RGM) cluster facility
#
get_extension_property()
{
    result=0

    value_info=`scha_resource_get -O EXTENSION \
	-R "$RESOURCE_NAME" -G "$RESOURCEGROUP_NAME" $1`
    result=$?

    if [ "$result" -ne 0 ]; then
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR (%s) when accessing property %s." "$result" "$1"
    fi

    value=`$ECHO $value_info | $NAWK '{ print $2 }'`
    $ECHO $value

    [ "$II_SCHA_TRACE" -ge 1 ] &&
         trace_log "Property : %s = %s" "$1" "$value"

    return $result
}

#
#    get_property() - access  information about a resource that is under the 
#	control of the Resource Group Manager (RGM) cluster facility
#
get_property()
{
    result=0

    value=`scha_resource_get -O $1 -R "$RESOURCE_NAME" -G "$RESOURCEGROUP_NAME"`
    result=$?

    if [ "$result" -ne 0 ]; then
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR (%s) when accessing property %s." "$result" "$1"
    fi

    $ECHO $value

    [ "$II_SCHA_TRACE" -ge 1 ] &&
        trace_log "Property : %s = %s" "$1" "$value"

    return $result
}

#
#    set_status() - set resource status
#  
#        set_status [OK | DEGRADED | FAULTED | UNKNOWN | OFFLINE] <message> 
#
set_status()
{
    resource_status="UNKNOWN"
    message=""

    while [ -n "$1" ]
    do
	case "$1" in
	    -s) shift; resource_status=$1 ;;
	    -m) shift; message=$1 ;;
	     *) shift ;;
	esac
	[ -n "$1" ] && shift
    done

    current_status_info=`get_property Status`
    current_status=`$ECHO $current_status_info | $NAWK '{ print $1 }'`

    if [ "$resource_status" = "$current_status" ]; then
        return 0
    fi

    if [ -z "$message" ]; then
            scha_resource_setstatus -s $resource_status \
                -R $RESOURCE_NAME -G $RESOURCEGROUP_NAME
        else
            scha_resource_setstatus -s $resource_status -m "$message" \
                -R $RESOURCE_NAME -G $RESOURCEGROUP_NAME
        fi

    [ "$II_SCHA_TRACE" -ge 1 ] &&
        trace_log "Set resource status : %s %s" "$resource_status" "$message"

    return 0
}

#
#    read_properties() - Access the information relative to the resource group.
#
read_properties()
{
    #
    # Set trace output
    #
    II_SCHA_TRACE=`get_extension_property II_SCHA_TRACE`
    II_SCHA_LOG=`  get_extension_property II_SCHA_LOG`
    [ "$II_SCHA_TRACE" -ge 9 ] &&
    {
        [ -n "$II_SCHA_LOG" ] &&
            exec 2>> "$II_SCHA_LOG"
        set -x
    }

    II_OWNER=`    get_extension_property II_OWNER`
    II_SYSTEM=`   get_extension_property II_SYSTEM`
    II_HOSTNAME=` get_extension_property II_HOSTNAME`
    User=`        get_extension_property User`
    Pword=`       get_extension_property Pword`
    Database=`    get_extension_property Database`
    Table=`       get_extension_property Table`
    EnvFile=`     get_extension_property EnvFile`

    # Obtain the full path for the gettime utility from the
    # RT_basedir property of the resource type.
    RT_BASEDIR=`get_property RT_BASEDIR`

    # Obtain the timeout value allowed for the probe, which is set in the
    # PROBE_TIMEOUT extension property in the RTR file. 
    PROBE_TIMEOUT=`get_extension_property Probe_timeout`

    # Get the retry count value from the system defined property Retry_count
    RETRY_COUNT=`get_property RETRY_COUNT`

    # Get the retry interval value from the system defined property Retry_interval
    RETRY_INTERVAL=`get_property RETRY_INTERVAL`
}

#
#    set_environment() - Set up the user's environment with the required variables.
#
set_environment()
{
    # Set the environment variables based on the owners shell

    if $GREP "^$II_OWNER:.*/bin/.*csh\$" /etc/passwd  >/dev/null ; then
	SET_ENVS=" setenv II_SYSTEM $II_SYSTEM; \
            setenv PATH ${II_SYSTEM}/ingres/bin:${II_SYSTEM}/ingres/utility:$PATH; \
            setenv LD_LIBRARY_PATH /usr/lib:${II_SYSTEM}/ingres/lib:$LD_LIBRARY_PATH; \
            setenv LD_LIBRARY_PATH_64 ${II_SYSTEM}/ingres/lib/lp64:$LD_LIBRARY_PATH_64 "

        # Set virtual hostname if defined.
        if [ ! -z "$II_HOSTNAME" ] ; then
            SET_ENVS=`$ECHO "$SET_ENVS; setenv II_HOSTNAME $II_HOSTNAME "`
        fi

        # Set additional environment variables if the EnvFile exists
        if [ ! -z "$EnvFile" -a -f "$EnvFile" ] ; then
            SET_ENVS=`$ECHO "$SET_ENVS; source $EnvFile "`
        fi

    else
	SET_ENVS="II_SYSTEM=$II_SYSTEM; export II_SYSTEM; \
	    PATH=${II_SYSTEM}/ingres/bin:${II_SYSTEM}/ingres/utility:$PATH; \
	    export PATH; \
	    LD_LIBRARY_PATH=/usr/lib:${II_SYSTEM}/ingres/lib:$LD_LIBRARY_PATH; \
	    export LD_LIBRARY_PATH; \
	    LD_LIBRARY_PATH_64=${II_SYSTEM}/ingres/lib/lp64:$LD_LIBRARY_PATH_64; \
	    export LD_LIBRARY_PATH_64"

        # Set virtual hostname if defined.
        if [ ! -z "$II_HOSTNAME" ] ; then
            SET_ENVS=`$ECHO "$SET_ENVS; \
		II_HOSTNAME=$II_HOSTNAME; export II_HOSTNAME "`
        fi

        # Set additional environment variables if the EnvFile exists
        if [ ! -z "$EnvFile" -a -f "$EnvFile" ] ; then
            SET_ENVS=`$ECHO "$SET_ENVS; . $EnvFile "`
        fi
    fi

    [ "$II_SCHA_TRACE" -ge 1 ] &&
         trace_log "Environment : %s" "$SET_ENVS"
}

restart_service()
{
    # Obtain the Stop method name and the STOP_TIMEOUT value for
    # this resource.

    STOP_TIMEOUT=`get_property  STOP_TIMEOUT` 

    STOP_METHOD=`get_property STOP`

    hatimerun -t $STOP_TIMEOUT $RT_BASEDIR/$STOP_METHOD \
        -R $RESOURCE_NAME -G $RESOURCEGROUP_NAME -T $RESOURCETYPE_NAME

    if [ $? -ne 0 ]; then
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : Stop method failed for the HA-Ingres data service."
        return 1
    fi

    set_status -s OFFLINE -m "Ingres DBMS server offline."

    # Obtain the Start method name and the START_TIMEOUT value for
    # this resource.

    START_TIMEOUT=`get_property START_TIMEOUT` 

    START_METHOD=`get_property START`

    hatimerun -t $START_TIMEOUT $RT_BASEDIR/$START_METHOD \
	-R $RESOURCE_NAME -G $RESOURCEGROUP_NAME -T $RESOURCETYPE_NAME

    if [ $? -ne 0 ]; then
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : Start method failed for the HA-Ingres data service."
	return 1
    fi

    set_status -s OK -m "Ingres DBMS server online."

    return 0
}

#
# This function decides the action to be taken upon the failure of a probe:
# restart the data service locally or fail over to another node in the cluster.
#
decide_restart_or_failover()
{
    # Check if this is the first restart attempt.
    if [ $retries -eq 0 ]; then
        # This is the first failure. Note the time of
        # this first attempt.
        start_time=`$RT_BASEDIR/bin/gettime`
        retries=`expr $retries + 1`
    
        # Because this is the first failure, attempt to restart
        # the data service.
        restart_service
        if [ $? -ne 0 ]; then
	    scds_syslog -p error -t $SYSLOG_TAG -m \
        	"ERROR : Failed to restart the HA-Ingres data service."
            exit 1
        fi
    else
	# This is not the first failure
	current_time=`$RT_BASEDIR/bin/gettime`
	time_diff=`expr $current_time - $start_time`
	if [ $time_diff -ge $RETRY_INTERVAL ]; then

	    # This failure happened after the time window
	    # elapsed, so reset the retries counter,
	    # slide the window, and do a retry.
	    retries=1
	    start_time=$current_time

	    # Because the previous failure occurred more than
	    # Retry_interval ago, attempt to restart the data service.
	    restart_service
	    if [ $? -ne 0 ]; then
		scds_syslog -p error -t $SYSLOG_TAG -m \
		    "ERROR : Failover to restart the HA-Ingres data service."
		exit 1
	    fi
	elif [ $retries -ge $RETRY_COUNT ]; then

	    # Still within the time window,
	    # and the retry counter expired, so fail over.
	    retries=0
	    scha_control -O GIVEOVER -G $RESOURCEGROUP_NAME -R $RESOURCE_NAME

	    if [ $? -ne 0 ]; then
		scds_syslog -p error -t $SYSLOG_TAG -m \
		    "ERROR : Failover attempt failed for the HA-Ingres data service."
	        exit 1
	    fi
	else
	    # Still within the time window,
	    # and the retry counter has not expired,
	    # so do another retry.
	    retries=`expr $retries + 1`
	    restart_service
	    if [ $? -ne 0 ]; then
	        scds_syslog -p error -t $SYSLOG_TAG -m \
        	    "ERROR : Failed to restart the HA-Ingres data service."
	        exit 1
	    fi
	fi
    fi
}

#
#    probe_ingres() - Check the status of the DBMS server.
#
probe_ingres()
{
    #
    # Do a basic sanity check (II_SYSTEM set to a directory).
    #
    [ -z "$II_SYSTEM" ] &&
    {
        # home for online is a required parameter:
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : II_SYSTEM is not set.  Resource will not go online."
	return 1
    }

    [ ! -d "$II_SYSTEM" ] &&
    {
        # home for online is a required parameter:
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : II_SYSTEM does not exist.  Resource will not go online."
	return 1
    }

    [ -z "$II_OWNER" ] &&
    {
        if [ -x ${II_SYSTEM}/ingres/utility/ingstart ] ; then
            II_OWNER=`/bin/ls -ld ${II_SYSTEM}/ingres/utility/ingstart |  $NAWK '{print $3}'`
        fi
    }

    [ -z "$II_OWNER" ] &&
    {
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : The Ingres owner must be configured.  Resource will not go online."
	return 1
    }

    probefail=0
    retries=0

    while :
    do

        # The interval at which probing is to be done is set in the system defined
        # property THOROUGH_PROBE_INTERVAL. Obtain the value of this property with
        # scha_resource_get
        PROBE_INTERVAL=`get_property THOROUGH_PROBE_INTERVAL`

        # The interval at which the probe needs to run is specified in the
        # property THOROUGH_PROBE_INTERVAL. Therefore, set the probe to sleep for a
        # duration of <THOROUGH_PROBE_INTERVAL> 

        sleep $PROBE_INTERVAL

        # if u are here, it means that the user has provided
        # all parameters. we therefore issue the command:

        #       1       the requested operation failed.
        #       99      the requested operation failed.
        #       100     indicates no servers are running
        #       101-109 indicates the percentage of servers that are running
        #       110     indicates online and all severs are running
        #       n       unknown error

        if [ -z "$Database" -o -z "$User" -o -z "$Table" ] ; then 

            hatimerun -t $PROBE_TIMEOUT $SU $II_OWNER -c \
		"$SET_ENVS; $RT_BASEDIR/bin/ingres_server_manager -dbms_status" \
		> /dev/null 2>&1

        probe_status=$?

    else

    #
    # The following test updates a row in "cluster_monitor" with the latest value
    # of the Ingres internal function date('now')
    #
    # A prerequisite for this test is that a user/password/table has been
    # created before enabling the script by defining the attributes
    # User/Database/Table for the Ingres resource.
    #
    # This task can be accomplished by the following SQL statements as the dba:
    #
    # sql iidbdb
    # * CREATE USER <User> \g
    # * COMMIT \g \q
    #
    # sql <Database> -u<User>
    # * CREATE TABLE <User>.<Table> ( cluster_monitor DATE ); \g
    # * INSERT INTO <User>.<Table> ( cluster_monitor ) VALUES ( DATE('NOW')); \g
    # * COMMIT \g \q
    #
    # To test the database:
    #
    # sql <Database> -u<User>
    # * UPDATE <User>.<Table> SET cluster_monitor = DATE('NOW') ; \g
    # * SELECT cluster_monitor FROM <User>.<Table>; \g
    # * COMMIT \g \q
    #
    # If you received the correct timestamp, the In-depth agent can be enabled
    # by using the following Sun Cluster commands:
    #
    # scrgadm -c -j <IngresResource> -x User=<User>
    # scrgadm -c -j <IngresResource> -x Database=<Database>
    # scrgadm -c -j <IngresResource> -x Table=<Table>
    # scrgadm -c -j <IngresResource> -x Probe_timeout=<Seconds>
    #

    # if u are here, it means that the user has provided
    # all parameters. we therefore issue the command:

        out=`hatimerun -t $PROBE_TIMEOUT $SU $II_OWNER -c \
	    "$SET_ENVS; sql -s $Database -u$User" 2> /dev/null << END
	    UPDATE $User.$Table SET cluster_monitor = DATE('NOW') ; \g
END`;

        #       1       the requested operation failed.
        #       99      the requested operation failed.
        #       100     indicates no servers are running
        #       101-109 indicates the percentage of servers that are running
        #       110     indicates online and all severs are running
        #       n       unknown error

        if $ECHO $out | $GREP "1 row" > /dev/null ; then
            probe_status=110
        else
            probe_status=100
        fi

    fi

    if [ $probe_status -eq 110 ]; then
	trace_log "Probe for resource HA-Ingres successful.  OK"

	set_status -s OK -m "Ingres DBMS server online."
    else
    if [ $probe_status -lt 110 -a $probe_status -gt 100 ]; then
	trace_log "Probe for resource HA-Ingres successful.  Degraded (%s)." $probe_status

	set_status -s DEGRADED -m "Ingres DBMS server degraded."
    else
    if [ $probe_status -eq 100 ]; then
	trace_log "Probe for resource HA-Ingres failed. Offline."
 
	set_status -s OFFLINE -m "Ingres DBMS server offline."
        decide_restart_or_failover
    else
	trace_log "Probe for resource HA-Ingres failed. Unknown (%s)." $probe_status
 
	set_status -s UNKNOWN -m "Ingres DBMS server unknown."
        decide_restart_or_failover
    fi
    fi
    fi

    done
}

#
#    main() - MAIN
#
main()
{

    parse_arguments "${@:-}" || exit $?

    read_properties

    set_environment

    probe_ingres; result=$?

    #log the result
    if [ "$result" -eq 0 ] ; then
	scds_syslog -p info -t $SYSLOG_TAG -m \
	    "Probe for resource HA-Ingres successful."
    else
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : Probe for resource HA-Ingres failed."
    fi

    exit $result
}

#
# MAIN
#
main "${@:-}"
