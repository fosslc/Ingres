:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#       ingres_service_stop -- Start the Ingres processes.
#
#  Usage:
#       ingres_service_stop -R <resource name> -T <type> -G <group>
#
#  Description:
#	This program is designed to work in the SUN HA Cluster.
#       It stops an entire Ingres installation.
#
#  Exit Status:
#       0       the requested operation succeeded.
#       1       the requested operation failed.
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
##	    comand fails to execute under bash, sh and ksh shells
##
#  DEST = schabin
#----------------------------------------------------------------------------

#Set the language
LANG=C; export LANG

II_SCHA_TRACE=0
II_SCHA_LOG=""
METHOD=stop
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
        [ "$II_SCHA_TRACE" -ge 1 ] &&
            trace_log "Set resource same status : %s %s" "$resource_status" "$message"
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
    RT_BASEDIR=`  get_property RT_BASEDIR`

    STOP_TIMEOUT=`get_property STOP_TIMEOUT`
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
	SET_ENVS="II_SYSTEM=$II_SYSTEM; export II_SYSTEM \
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

#
#    stop_ingres() - Start the ingres installation.
#
stop_ingres()
{
    #
    # Do a basic sanity check (II_SYSTEM set to a directory).
    #
    [ -z "$II_SYSTEM" ] &&
    {
        # home for online is a required parameter:
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : II_SYSTEM is not set.  Resource will not go offline."
	exit 1
    }

    [ ! -d "$II_SYSTEM" ] &&
    {
        # home for online is a required parameter:
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : II_SYSTEM does not exist..  Resource will not go offline."
	exit 1
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
	    "ERROR : The Ingres owner must be configured.  Resource will not go offline."
	exit 1
    }

    # Try a gentle shutdown
    hatimerun -t $STOP_TIMEOUT $SU $II_OWNER -c "$SET_ENVS; $RT_BASEDIR/bin/ingres_server_manager -stop" \
	> /dev/null 2>&1

    result=$?

    if [ "$result" -ne 0 ] ; then
        # Make sure it's really down
         hatimerun -t $STOP_TIMEOUT $SU $II_OWNER -c "$SET_ENVS; $RT_BASEDIR/bin/ingres_server_manager -kill" \
	    > /dev/null 2>&1
    fi

    result=$?

    #log the result
    if [ "$result" -eq 0 ] ; then
	scds_syslog -p info -t $SYSLOG_TAG -m \
	    "HA-Ingres successfully stopped"

        set_status -s OFFLINE -m "Ingres DBMS server offline."
    else
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : HA-Ingres failed to stop."

        set_status -s FAULTED -m "Ingres DBMS server faulted."
    fi

    return $result
}

#
#    main() - MAIN
#
main()
{

    parse_arguments "${@:-}" || exit $?

    set_status -s UNKNOWN -m "Bringing Ingres DBMS server offline."

    read_properties

    set_environment

    stop_ingres; result=$?

    exit $result
}

#
# MAIN
#
main "${@:-}"
