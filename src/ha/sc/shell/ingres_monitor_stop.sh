:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#       ingres_monitor_stop -- Stop the monitoring Ingres processes.
#
#  Usage:
#       ingres_monitor_stop -R <resource name> -T <type> -G <group>
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
##	    Created for SUN HA cluster support.
##	16-Apr-2008 (bonro01)
##	    Fix spelling errors.
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

#
#    monitor_stop() -- Stop the monitor that is running using PMF
#
monitor_stop()
{
    PMF_TAG=$RESOURCEGROUP_NAME,$RESOURCE_NAME,0.mon

    if pmfadm -q $PMF_TAG; then

        pmfadm -s $PMF_TAG KILL

	return $?
    fi

    return 0
}

#
#    main() - MAIN
#
main()
{
    parse_arguments "${@:-}" || exit $?

    read_properties

    monitor_stop; result=$?

    #log the result
    if [ "$result" -eq 0 ] ; then
	scds_syslog -p info -t $SYSLOG_TAG -m \
	    "Monitor for HA-Ingres successfully stopped."
    else
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : Monitor for HA-Ingres failed to stop."
    fi

    exit $result
}

#
# MAIN
#
main "${@:-}"
