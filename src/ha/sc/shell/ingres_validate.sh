:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#       validate -- Stop the Ingres processes.
#
#  Usage:
#       validate -R <resource name> -T <type> -G <group>
#
#  Description:
#	This program is designed to work in the SUN HA Cluster.
#       It starts an entire Ingres installation.  If Ingres is
#	already running, it shuts it down and restarts it.
#
#  Exit Value:
#       0       the requested operation succeeded.
#       1       the requested operation failed.
#
## History:
##	01-Jan-2004 (hanch04)
##	    Created for SUN HA cluster support.
##
#  DEST = schabin
#----------------------------------------------------------------------------

#Set the language
LANG=C; export LANG

SELF=`basename $0`
BINDIR=`/bin/dirname $0`
SU=/usr/bin/su
GREP=/usr/bin/grep
NAWK=/usr/bin/nawk
ECHO=/usr/bin/echo

SYSLOG_FACILITY=`scha_cluster_get -O SYSLOG_FACILITY`

#Set the language
LANG=C; export LANG

II_SCHA_TRACE=0
II_SCHA_LOG=""
METHOD=validate
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
set_property()
{
    param=`echo $1 | awk -F= '{print $1}'`
    val=`echo $1 | awk -F= '{print $2}'`
    eval ${param}=\"${val}\"
}

#
#    parse_arguments() - Parse the program arguments.
#
parse_arguments()
{
    RESOURCE_NAME=""
    RESOURCEGROUP_NAME=""
    RESOURCETYPE_NAME=""
    CREATE_RESOURCE=""
    UPDATE_PROPERTY=""
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

 	    #The method is not accessing any system defined
 	    #properties, so this is a no-op.
 	    -r)  shift; set_property $1 ;;

 	    # The method is not accessing any resource group
 	    # properties, so this is a no-op.
 	    -g)  shift; set_property $1 ;;

 	    # Indicates the Validate method is being called while
 	    # creating the resource, so this flag is a no-op.
 	    -c)  CREATE_RESOURCE=1 ;;

 	    # Indicates the updating of a property when the
 	    # resource already exists. If the update is to the
 	    # Confdir property then Confdir should appear in the
 	    # command-line arguments. If it does not, the method must
 	    # look for it specifically using scha_resource_get.
 	    -u)  UPDATE_PROPERTY=1 ;;

 	    # Extension property list. Separate the property and
 	    # value pairs using = as the separator.
   	    -x)  shift; set_property $1 ;;

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
    [ "$II_SCHA_TRACE" -ge 9 ] &&
    {
        set -x
        [ -n "$II_SCHA_LOG" ] &&
            exec 2>> "$II_SCHA_LOG"
    }
    if [ -z "$CREATE_RESOURCE" ] ; then
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
    fi
}

#
#    validate_ingres() -
#
validate_ingres()
{
    #
    # Do a basic sanity check (II_SYSTEM set to a directory).
    #
    [ -z "$II_SYSTEM" ] &&
    {
        # home for online is a required parameter:
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : II_SYSTEM is not set.  Validate method failed."
	exit 1
    }

    [ ! -d "$II_SYSTEM" ] &&
    {
        # home for online is a required parameter:
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : II_SYSTEM does not exist.  Validate method failed." 
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
	    "ERROR : The Ingres owner must be configured.  Validate method failed."
	exit 1
    }
    return 0
}

#
#    main() - MAIN
#
main()
{

    parse_arguments "${@:-}" || exit $?

    read_properties

    validate_ingres; result=$?

    #log the result
    if [ "$result" -eq 0 ] ; then
	scds_syslog -p info -t $SYSLOG_TAG -m \
	    "Validate method completed successfully."
    else
	scds_syslog -p error -t $SYSLOG_TAG -m \
	    "ERROR : Validate method failed."
    fi

    exit $result
}

#
# MAIN
#
main "${@:-}"
