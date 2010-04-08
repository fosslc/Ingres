:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#       ingres_monitor_start -- Stop the monitor program for the Ingres processes.
#
#  Usage:
#       ingres_monitor_start -R <resource name> -T <type> -G <group>
#
#  Description:
#	This program is designed to work in the SUN HA Cluster.
#
#  Exit Value:
#       0       the requested operation succeeded.
#       1       the requested operation failed.
#
## History:
##	01-Jan-2004 (hanch04)
##	    Created for SUN HA cluster support.#
##	03-Oct-2005 (bonro01)
##	    Corrected path for scripts.
##	16-Apr-2008 (bonro01)
##	    Correct name for ingres_probe script.
##
#  DEST = schabin
#----------------------------------------------------------------------------

#Set the language
LANG=C; export LANG

II_SCHA_TRACE=0
II_SCHA_LOG=""
METHOD=monitor_start
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
        set -x
        [ -n "$II_SCHA_LOG" ] &&
            exec 2>> "$II_SCHA_LOG"
    }

    # Obtain the full path for the gettime utility from the
    # RT_basedir property of the resource type.
    RT_BASEDIR=`  get_property RT_BASEDIR`
}

monitor_start()
{
    PMF_TAG=$RESOURCEGROUP_NAME,$RESOURCE_NAME,0.mon

    # Start the probe for the data service under PMF. Use the infinite retries 
    # option to start the probe. Pass the the Resource name, type and group to the 
    # probe method. 

    # -c <nametag>
    # Start a process, with nametag as an identifier.  All arguments that follow the command-
    # line flags are executed as the process of interest.  The current directory, and PATH
    # environment variable, are reinstantiated by the process monitor facility before the com-
    # mand is executed.  No other environment variables are, or should be assumed to be,
    # preserved.

    # -n <retries>
    # Number of retries allowed within the specified time period.  The default value for this
    # field is 0, which means that the process will not be restarted once it exits.  A value of
    # -1 indicates that the number of retries is

    #MONITOR_RETRY_COUNT=1

    # -t <period>
    # Minutes over which to count failures. The default value for this flag is -1, which
    # equates to infinity.  If this parameter is specified, process failures that have
    # occurred outside of the specified period are not counted.

    #MONITOR_RETRY_INTERVAL=5


    pmfadm -E -c $PMF_TAG -n -1 -t -1 -C 1 \
	$RT_BASEDIR/bin/ingres_probe -R $RESOURCE_NAME -G $RESOURCEGROUP_NAME \
	-T $RESOURCETYPE_NAME

    return $?
}

#
#    main() - MAIN
#
main()
{
    parse_arguments "${@:-}" || exit $?

    read_properties

    monitor_start; result=$?

    #log the result
    if [ "$result" -eq 0 ] ; then
	scds_syslog -p info -t $SYSLOG_TAG -m \
	    "Monitor for HA-Ingres successfully started."
    else
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : Monitor for HA-Ingres failed to start."
    fi

    exit $result
}

#
# MAIN
#
main "${@:-}"
