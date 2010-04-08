:
#  Copyright (c) 2002, 2008 Ingres Corporation
# 
# 
#  $Header $
#
#  Name: iimkcluster - Convert a non-cluster Ingres instance to 1st node
#		       of a Clustered Ingres instance.
#
#  Usage:
#	iimkcluster
#
#  Description:
#	Converts where supported a non-clustered Ingres instance
#	to the first node of an Ingres cluster.
#
#	This basically consists of "decorating" the log file names,
#	Modifying certain config.dat and symbol.tbl values, and
#	creating a number of node specific subdirectories.
#
#	This script must be run as 'ingres', and while instance is
#	down.
#
# Exit status:
#	0	OK - conversion to Clustered Ingres successful.
#	1	User decided not to proceed.
#	2	Environment does not support clustering.
#	3	Instance fails basic setup sanity checks.
#	4	Ingres is apparently not set up.
#	5	Ingres is already set up for clusters.
#	6	II_CLUSTER indicates instance is already
#		part of a cluster.
#	7	Ingres must be shutdown before conversion.
#	8	Quorum manager was not up.
#	9	Distributed Lock Manager was not up.
#	10	Must be "ingres" to run script.
#	11	Process failed or was interrupted during convert.
#	12	Configuration file is locked.
#	14	Unable to backup an existing file.
#
## History
##	19-jun-2002 (devjo01)
##		Initial version.
##	27-feb-2004 (devjo01)
##		Remove dependency on 'ingbuild'.
##	12-mar-2004 (devjo01)
##		Allow -nodenumber as a parameter, and prompt for a node
##		if one is not specified.  (Possible future enhancement
##		would move code common to iimkcluster/iisudbms to iishlib).
##	26-mar-2004 (devjo01)
##		- Correct name of e.v. CLUSTERSUPPORT to match iisysdep.
##		- Don't create admin subdirectories.
##	09-apr-2004 (devjo01)
##		- Backout part of previous change, and do create admin
##		directory, and copy symbol.tbl to it.  symbol.tbl gets
##		frequently truncated otherwise.  Downside, is that you'll
##		need to set Ingres environment variables in each host,
##		Upside, is you gain flexibility.
##	20-Jan-2005
##          	- Do not assume "ingres" user is literally 'ingres'.
##		- Fix cleanup performed if script cannot complete.
##		- Add default NAC server class.
##		- Check that iiquorumd & DLM are up before proceeding.
##		- "Lock" config.dat file, log operations to config.log
##		- Change "trap 0" to trap ":" to avoid Suse 9.2 syntax err.
##	03-feb-2005 (devjo01)
##		Don't allow this to be run as root.
##	09-feb-2005 (devjo01)
##		"protect" lock_limit setting by adding +p to iisetres.
##	14-feb-2005 (devjo01)
##		Set dlmlibrary configuration parameter.
##	15-mar-2005 (mutma03)
##		Remove the document link as it was decided best to remove
##		on clusters since it is installed locally in /usr/share/loc
##		and the secondary nodes wont see it. 
##	06-apr-2005 (devjo01)
##		- Correct "append_class" to duplicate the private cache
##		definitions, and handle the case where "template" DBMS
##		definition uses a shared cache.
##		- Shorten default name of NAC configurations from
##		iiaffinityclassX to iinacXX (same as cache name and
##		class name)
##      18-apr-2005 (mutma03)
##              - Enhancements, Added get_rshmethod to input the type remote
##		 server to be used in cluster operations.
##      15-May-2005 (mutma03)
##          modified iiaffinityclassX to iicnaXX to reflect the documentation.
##      12-Jun-2005 (mutma03)
##          Added version compatibility check with OpenDLM.
##	16-Aug-2005 (mutma03)
##        Fixed append_class for creating cache entires for CNA servers.
##	01-Dec-2005 (devjo01) b115580
##	    Call syscheck to confirm that shared memory requirements
##	    are supportable.
##	07-jan-2008 (joea)
##	    Discontinue use of cluster nicknames.
##	28-feb-2008 (upake01)
##	    SIR 120019 - Remove lines that set "100,000" to "rcp.lock.lock_limit" 
##          allowing the value to get calculated using the formula in DBMS.CRS.
##
#  DEST = utility
# ------------------------------------------------------------------------

. iisysdep
. iishlib

#
#   Utility routine to check precence of a directory in PATH
#
#   $1 = last segment of directory path, e.g. bin or utility.
#
check_path()
{
    echo "$PATH" | grep -q "$II_SYSTEM/ingres/$1" && return 0
    error_box << !
$II_SYSTEM/ingres/$1 is not in the PATH.
!
    return 1
}

#
#   Utility routine to generate a log name based on a base log name,
#   a segment number, and an optional node extention.  Final name
#   must be 36 chars or less.  Name is echoed to stdout.
#
#	$1 = base name.
#	$2 = segment # (1 .. 16)
#	$3 = $CONFIG_HOST, or "".
#
gen_log_name()
{
    ext=""
    [ -n "$3" ] && ext="_`echo $3 | tr '[a-z]' '[A-Z]'`"
    paddedpart=`echo "0${2}" | sed 's/0\([0-9][0-9]\)/\1/'`
    echo "${1}.l${paddedpart}${ext}" | cut -c 1-36
}


#
#   Complain about bad input.
#
#	$1 = bad parameter 
#
usage()
{
    error_box << !
     
invalid parameter "${1}".

Usage $SELF [ -nodenumber <node_number> ] 
!
    exit 1
}

#
#   Verify legit value for node number
#
#	$1 = candidate node number.
#	$2 = if not blank, surpress warning message.
#
validate_node_number()
{
    [ -z "`echo ${1} | sed 's/[0-9]//g'`" ] && \
     [ ${1} -ge 1 ] && \
     [ ${1} -le $CLUSTER_MAX_NODE ] && return 0
    [ -n "${2}" ] || wrap_lines << !

Invalid node number value.  Node number must be an integer in the range 1 through ${CLUSTER_MAX_NODE} inclusive!

!
    return 1
}

confirm_intent()
{
    prompt "Do you want to continue this procedure?" n || exit 1
}

create_dir()
{
    if [ ! -d $1 ]; then
	mkdir $1
	chmod $2 $1
    fi
}

backup_file()
{
    [ -f $II_CONFIG/${1} ] && 
    {
	cp -p $II_CONFIG/${1} $II_CONFIG/${1}_$DATESTAMP
	cmp $II_CONFIG/${1} $II_CONFIG/${1}_$DATESTAMP >& /dev/null ||
	{
	    error_box << !
Failed to backup ${1}
!
	    rm -f $II_CONFIG/${1}_$DATESTAMP >& /dev/null
	    exit 14
	}
    }
}

restore_file()
{
    [ -f $II_CONFIG/${1}_$DATESTAMP ] &&
    {
	mv -f $II_CONFIG/${1}_$DATESTAMP $II_CONFIG/${1}
    }
}
#
# Add a server class based on default dbms class def in 1st node
#
#   $1 = target file (config.dat | protect.dat )
#   $2 = server config name (NAC).
#
append_class()
{
    [ -s $II_CONFIG/${1}_${DATESTAMP} ] &&
    {
	pattern="`grep_search_pattern ii.${CONFIG_HOST}.dbms.*.`"
	grep "${pattern}" $II_CONFIG/${1}_${DATESTAMP} | grep -v dmf_connect \
	 | sed "s/\.\*\./\.${2}\./" >> $II_CONFIG/${1}
	if [ "`iigetres ii.${CONFIG_HOST}.dbms.*.cache_sharing`" = "OFF" ]
	then
	    pattern="`grep_search_pattern ii.${CONFIG_HOST}.dbms.private.*.`"
	    grep "${pattern}" $II_CONFIG/${1}_${DATESTAMP} \
	     | sed "s/\.\*\./\.${2}\./" >> $II_CONFIG/${1}
	fi
    }
}
#
#   get remote shell server to be used for remote command execution
#
get_rshmethod()
{
	SERVER="none,rsh,ssh"
        wrap_lines << !

 To stop and start components of the Ingres cluster from any one node requires that a remote shell facility be selected and set up.   This is a convenience feature, and is not mandatory for proper operation of the Ingres cluster.   If you enter none and elect not to enable this feature, you will need to log on to each machine to start or stop a component on that node.    

If you do elect to set up cluster wide administration you should enter the name of the utility you wish to use (typically rsh or ssh). You may either specify a full path or let Ingres find the specified utility in your PATH.   Each node in the Ingres cluster should be setup to allow remote shell execution by the <ingres_user> from each other node in the Ingres cluster without prompting for a password.

!
    while :
    do

        echo -n Please select the remote shell server {$SERVER} [none]:
        read answer junk
        [ -n "$junk"] ||
        {
            cat << !

extra input in response.
!
            continue
        }

        [ -n "$answer" ] || answer="none" 
        [ "$answer" = "none" ] && 
	{
	    myrsh="none"
	    break 
        }

	if  echo $answer | grep -q '/'
        then
            echo $answer | grep -q '^/' ||
            {
	        error_box << !
Bad input!
If you provide a path as part of your input, path must the complete path and filename of the remote shell utility.
!
                continue
            } 
	    myrsh="$answer"
        else
            myrsh=`which $answer 2>/dev/null`
            [ -n "$myrsh" ] ||
             {
                error_box << !
Specified remote shell utility ($answer) is not in PATH.   Please check utility name, or specify the utility as a fully qualified file specification.
!
                continue	    
             }           
         fi
         [ -x $myrsh ] && break
         error_box << ! 
Specified remote shell utility is not an executable.   Please check input and try again.
!
# end inner while
    done

   if [ "$myrsh" != "none" ] 
   then 
       echo ""
       echo "$myrsh will be used as the remote shell utility"
   fi

# Save config info obtained
    iisetres "ii.$CONFIG_HOST.config.rsh_method" $myrsh
}

#
#       Workaround for the issue, "setting startup count messes up the
#       cache_sharing fields of the dbms server" because of special rules.
#       Ideally should be fixed in PMsetCRval.
#       $1=node $2=NAC $3=startup count
#
startup_count()
{
    CACHE_SHARING=`iigetres ii.$1.dbms.*.cache_sharing`

    if [ "$CACHE_SHARING" = "ON" ] ; then
	$DOIT iisetres "ii.$1.dbms.*.cache_sharing" 'OFF'
	$DOIT iisetres "ii.$1.ingstart.$2.dbms" $3
	$DOIT iisetres "ii.$1.dbms.*.cache_sharing" 'ON'
    else
	$DOIT iisetres "ii.$1.ingstart.$2.dbms" $3
    fi

}
#
#   Start of MAINLINE
#

#   Set up defaults and useful constants.
    SELF="`basename $0`"

#   Clear any unwanted environment variables.
    [ -n "$II_CLUSTER_ID" ] && \
        validate_node_number "$II_CLUSTER_ID" "quiet" || \
	II_CLUSTER_ID=""

#   Check parameters
    while [ $# != 0 ]
    do
	if [ "$1" = "-nodenumber" ] ; then
	    [ -z "$2" ] && usage $1
	    validate_node_number "$2" || usage "$1 $2"
	    shift
	    II_CLUSTER_ID=$1
	elif [ "$1" ] ; then
	    usage $1
	fi
	shift
    done

    [ -n "$SHELL_DEBUG" ] && set -x

#   Check if Cluster is supported.
    $CLUSTERSUPPORT ||
    {
	error_box << !
Your environment cannot support the Ingres Grid option (clustered Ingres).
!
	exit 2
    }

#   Perform some basic instance set-up checks.
    [ -n "$II_SYSTEM" ] ||
    {
	error_box << !
Environment variable II_SYSTEM is not set. 
!
	exit 3
    }

    check_path bin || exit 3
    check_path utility || exit 3

#   Get "ingres" user since user no longer needs to literally be 'ingres'.
    INGUSER="`iigetres ii.$CONFIG_HOST.setup.owner.user 2> /dev/null`"
    [ -n "$INGUSER" ] || INGUSER='ingres'
    INGGRP="`iigetres ii.$CONFIG_HOST.setup.owner.group 2> /dev/null`"
    
#   Check if caller is "ingres".
    [ "$WHOAMI" != "$INGUSER" ] &&
    {
	box << !
You must be '$INGUSER' to run this script.
!
	exit 10
    }

    if [ -f $II_SYSTEM/ingres/install/release.dat ]
    then
	[ -x $II_SYSTEM/ingres/install/ingbuild ] ||
	{
	    error_box << !
'ingbuild' is not in $II_SYSTEM/ingres/install.
!
	    exit 3
	}

	VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms` ||
	{
	    cat << !

$VERSION

!
	    exit 1
	}
    else
	VERSION=`cat $II_SYSTEM/ingres/version.rel | head -1` ||
	{
	    error_box << !

Missing file $II_SYSTEM/ingres/version.rel

!
	    exit 1
	}
    fi

    NUM_PARTS=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_parts`
    [ -z "$NUM_PARTS" -o -n "`echo $NUM_PARTS | sed 's/[1-9][0-9]*//'`" ] &&
    {
	error_box << !
Ingres transaction log improperly configured.  Cannot determine the number of log segments from "ii.$CONFIG_HOST.rcp.log.log_file_parts".
!
	exit 3
    }

    RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

    VALUE=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID`
    [ "X$VALUE" = "Xcomplete" ] ||
    {
	error_box << !
$RELEASE_ID is not setup.   If this is a new installation, you must complete install of Ingres in stand-alone mode.   If this is an upgrade from a previous version of Ingres, please complete the upgrade, then run $SELF once the upgrade is complete.
!
	exit 4
    }

#   Check if already clustered.
    VALUE=`iigetres ii.$CONFIG_HOST.config.cluster_mode`
    [ "X${VALUE}" = "XON" ] &&
    {
	error_box << !
Ingres is already set up in a cluster configuration.
!
	exit 5
    }

    VALUE="`ingprenv II_CLUSTER`"
    [ "X${VALUE}" = "X1" ] &&
    {
	warning_box << !
Environment variable II_CLUSTER is set to "1", indicating that this Ingres instance is already set up for Clustered Ingres, but configuration information for "ii.$CONFIG_HOST.config.cluster_mode" indicates that this is not a Clustered Ingres instance.  If it is known that instance is NOT part of an Ingres cluster, you may run 'iiuncluster' to reset these values to consistent non-clustered settings, before running $SELF again.
!
	exit 6
    }

#   Check if instance is up.
    rcpstat -online -silent >/dev/null &&
    {
	warning_box << !
Ingres must be shutdown before converting to a cluster configuration.

Please 'ingstop' your installation before running $SELF again.
!
	exit 7
    }

#   Check if quorum daemon is running
    [ -f /etc/init.d/iiquorumd ] ||
    {
	error_box << !
You must install the "iiquorumd" Quorum Manager prior to converting this Ingres instance into an Ingres Grid Option configuration.
!
	exit 8
    }

    /etc/init.d/iiquorumd status >& /dev/null ||
    {
	error_box << !
iiquorumd process should be running prior to converting this Ingres instance into an Ingres Grid Option configuration.
!
	exit 8
    }

#   Check if DLM process is running.  This check is specific to OpenDLM
#   and will need to be generalized if & when additional DLMs are used.
    [ -f /etc/init.d/haDLM ] ||
    {
	error_box << !
You must install the OpenDLM distributed lock manager before converting this Ingres instance into an Ingres Grid Option configuration.
!
	exit 9
    }

    /etc/init.d/haDLM status >& /dev/null ||
    {
	error_box << !
OpenDLM distributed lock manager should be running prior to converting this Ingres instance into an Ingres Grid Option configuration.
!
	exit 9
    }

#   Check for openDLM version compatiblity. Reject if the DLM is older than 0.4
#   The format of DLM version has been changed in later versions and will have
#   Major.Minor.buildlevel.
    DLM_VERSION=`cat /proc/haDLM/version`
    DLM_MAJOR_VER=`echo $DLM_VERSION|awk -F '.' '{ print $1 }'`
    DLM_MINOR_VER=`echo $DLM_VERSION|awk -F '.' '{ print $2 }'`
    if [  $DLM_MAJOR_VER -eq 0 -a $DLM_MINOR_VER -le 3 ]
    then
            error_box <<!
Installed openDLM version $DLM_VERSION is stale. Please upgrade the openDLM prior to converting this Ingres instance into an Ingres Grid Option
configuration.
!
            exit 9
    fi
#   Accept if the major number of the DLM version and ingres version are equal.

    if [ "`iigetres ii.*.config.dlm_version`" != "" ]
    then
        EXP_MAJOR_VER=`iigetres ii.*.config.dlm_version|awk -F '.' '{ print $1 }
'`
        if [ $DLM_MAJOR_VER -ne $EXP_MAJOR_VER ]
        then

            error_box << !
openDLM version $DLM_VERSION is not compatible with Ingres. Please update the correct version of openDLM prior to converting this Ingres instance into an IngresGrid Option configuration.
!
            exit 9
        fi
    else
        wrap_lines << !

Installed OpenDLM version is $DLM_VERSION. Ingres configuration does not specify an explicit DLM version requirement, and this version of Ingres is expected to work well with OpenDLM 0.4/0.9.4.
!
    fi 

#   Prompt for user confirmation.
    II_INSTALLATION="`ingprenv II_INSTALLATION`"
    wrap_lines << !

$SELF will convert Ingres instance "$II_INSTALLATION" on $CONFIG_HOST into the first node of an Clustered Ingres instance.

!
    confirm_intent

#   Check if STAR is configured.
    VALUE="`iigetres ii.$CONFIG_HOST.ingstart.*.star`"
    [ -z "$VALUE" ] || [ "$VALUE" = "0" ] || \
    {
	warning_box << !
The Ingres STAR distributed database server uses two phase commit (2PC), and therefore is not currently suppported by Clustered Ingres.

Your installation is currently configured for one or more STAR servers.

If you decide to continue, the startup count will be set to zero, and no STAR servers will be configured to start.
!
	confirm_intent
    }

#   Lock config.dat and backup files.  We do this before getting the nodenumber
#   because sadly these routines update the config.dat
    lock_config_dat $SELF || exit 12
    DATESTAMP="`date +'%Y%m%d_%H%M'`"
    trap "restore_file config.dat ; restore_file protect.dat ; \
	 unlock_config_dat; exit 11" 1 2 3 15
    trap "restore_file config.dat ; restore_file protect.dat ; \
     unlock_config_dat" 0
     
    backup_file config.dat
    backup_file protect.dat

    if [ -z "$II_CLUSTER_ID" ]
    then
#       Prompt for node number
	wrap_lines << !

Please specify a unique integer node number in the range 1 through ${CLUSTER_MAX_NODE}.  Since the surviving node with the lowest node number in a partially failed cluster is responsible for recovery, the lowest number to be assigned should be assigned to the most powerful machine planned for inclusion in the Ingres cluster.

!
	II_CLUSTER_ID="1"
	while :
	do
	    iiechonn "[$II_CLUSTER_ID] "
	    read VALUE junk
	    [ -z "$VALUE" ] && break
	    validate_node_number $VALUE || continue
	    II_CLUSTER_ID="$VALUE"
	    break
	done
    fi

    echo ""
    get_rshmethod

#   Locate OpenDLM application library.
    opendlmlib=""
    for f in $(echo "$LD_LIBRARY_PATH" | sed 's/:/ /g') /usr/lib /usr/local/lib
    do
	[ -x $f/libdlm.so ] &&
	{
	    opendlmlib=$f/libdlm.so
	    break
	}
    done

    [ -n "$opendlmlib" ] ||
    {
	warning_box << !
OpenDLM application library (libdlm.so) cannot be found in the LD_LIBRARY_PATH environment variable, or standard shared library locations.

Please enter fully qualified directory path for libdlm.so, or enter an empty path to exit. 
!
	while :
	do
	    iiechonn "libdlm.so directory path: "
	    read VALUE
	    [ -n "$VALUE" ] ||
	    {
		wrap_lines << !

Exitting $SELF because libdlm.so directory path was not specified.
!
		exit 9
	    }
	    [ -x $VALUE/libdlm.so ] &&
	    {
		opendlmlib=$VALUE/libdlm.so
		break
	    }
	    [ -f $VALUE/libdlm.so ] &&
	    {
		wrap_lines << !

$VALUE/libdlm.so exists but has incorrect permissions.

!
		continue
	    }
	    wrap_lines << !

Cannot find $VALUE/libdlm.so.

!
	done
    }

    wrap_lines << !

Existing $VERSION Ingres instance "$II_INSTALLATION" on host $CONFIG_HOST will be converted to the initial node of an Ingres Cluster using node number $II_CLUSTER_ID.

$opendlmlib will be used as the Distributed Lock Manager shared library.

!
    confirm_intent

#
#   The Rubicon is crossed.
#
    log_config_change_banner "BEGIN IIMKCLUSTER" 24

#   Establish trap to undo a partial conversion   
    trap "$II_SYSTEM/ingres/utility/iiuncluster -force $DATESTAMP ; unlock_config_dat ; exit 11" \
     0 1 2 3 15

#   Create new sub-directories.
    create_dir $II_CONFIG/memory 755
    create_dir $II_CONFIG/memory/$CONFIG_HOST 755
    create_dir $II_SYSTEM/ingres/admin 755
    create_dir $II_SYSTEM/ingres/admin/$CONFIG_HOST 755

    mv $II_CONFIG/symbol.tbl $II_SYSTEM/ingres/admin/$CONFIG_HOST/symbol.tbl

#   Rename transaction log files.
    PRIMARY_LOG_NAME=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_name`
    [ -z "$PRIMARY_LOG_NAME" ] && PRIMARY_LOG_NAME='ingres_log'
    DUAL_LOG_NAME=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_name`
    
    PART=0
    WARNINGS=""
    while [ $PART -lt $NUM_PARTS ]
    do
	PART=`expr $PART + 1`
	DIR=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_$PART`
	if [ -z "$DIR" ]
	then
	    warning_box << !
Cannot find configuration information for "iigetres ii.$CONFIG_HOST.rcp.log.log_file_$PART".
!
	    WARNINGS=1
	else
	    OLD_NAME=`gen_log_name $PRIMARY_LOG_NAME $PART`

	    if [ -f $DIR/ingres/log/$OLD_NAME ]
	    then
		mv $DIR/ingres/log/$OLD_NAME \
 $DIR/ingres/log/`gen_log_name $PRIMARY_LOG_NAME $PART $CONFIG_HOST`
	    else
		warning_box << !
Cannot find "$DIR/ingres/log/$OLD_NAME". Skipping file.
!
		WARNINGS=1
	    fi
	fi
	
	[ -z "$DUAL_LOG_NAME" ] && continue

	DIR=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_$PART`
	if [ -z "$DIR" ]
	then
	    if [ $PART -eq 1 ] 
	    then
		# If 1st dual log missing, assume no dual logging.
		DUAL_LOG_NAME=""
	    else
		warning_box << !
Cannot find configuration information for "iigetres ii.$CONFIG_HOST.rcp.log.dual_log_$PART".
!
		WARNINGS=1
	    fi
	    continue;
	fi

	OLD_NAME=`gen_log_name $DUAL_LOG_NAME $PART`

	if [ -f $DIR/ingres/log/$OLD_NAME ]
	then
	    mv $DIR/ingres/log/$OLD_NAME \
     $DIR/ingres/log/`gen_log_name $DUAL_LOG_NAME $PART $CONFIG_HOST`
	elif [ $PART -eq 1 ]
	then
	    # If 1st dual log missing, assume no dual logging.
	    DUAL_LOG_NAME=""
	else
	    warning_box << !
Cannot find "$DIR/ingres/log/$OLD_NAME". Skipping file.
!
	    WARNINGS=1
	fi
    done

#   Modify config.dat
    iisetres "ii.$CONFIG_HOST.config.cluster_mode" 'ON'
    iisetres "ii.$CONFIG_HOST.config.cluster.id" "$II_CLUSTER_ID"
    iisetres "ii.$CONFIG_HOST.ingstart.*.star" 0
    iisetres "ii.*.config.dlmlibrary" "$opendlmlib"
   
#   Add default Node affinity class
    NAC=`echo "IICNA0${II_CLUSTER_ID}" | sed 's/[0-9]\([0-9][0-9]\)/\1/'`

    append_class config.dat $NAC 
    append_class protect.dat $NAC 

#   update ingstart.$NAC.dbms after saving cache_sharing info
    startup_count $CONFIG_HOST $NAC 0

    iisetres "ii.$CONFIG_HOST.dbms.$NAC.server_class" "$NAC"
    iisetres "ii.$CONFIG_HOST.dbms.$NAC.cache_name" "$NAC"
    iisetres "ii.$CONFIG_HOST.dbms.$NAC.cache_sharing" 'OFF'
    iisetres "ii.$CONFIG_HOST.dbms.$NAC.class_node_affinity" 'ON'

#   Force DMCM on for all classes, JIC
    grep -i "\.dmcm:" $II_CONFIG/config.dat | awk -F ':' '{ print $1,$2 }' | \
    while read PARAM VALUE
    do
	[ "X$VALUE" = 'XON' ] || iisetres "$PARAM" 'ON'
    done

#   Modify symbol.tbl
    ingsetenv II_CLUSTER 1
    [ "$VERS" = 'axp_osf' ] && ingsetenv II_THREAD_TYPE INTERNAL

#   Remove the document link as it is on local machine and would be broken in
#   secondary nodes.

    if [ -d $II_SYSTEM/ingres/doc ]; then
        rm -f $II_SYSTEM/ingres/doc
    fi
#   We're done, clean up.
    unlock_config_dat
    trap ":" 0 1 2 3 15

    if [ -n "$WARNINGS" ]
    then
	warning_box << !
Caution, during conversion, $SELF detected one or more configuration inconsistencies.  Please resolve the detected problems before using Ingres in the Cluster configuration.
!
    else
	wrap_lines << !

Ingres has now been converted to an Ingres Cluster configuration.  You may now use 'iisunode' to set up additional Ingres Cluster nodes.

Note: Certain text log files found in $II_CONFIG, such as iiacp.log, and iircp.log will remain in place, but all new information will be written to log files with names "decorated" with the associated Ingres Cluster node (e.g. iiacp.log_${CONFIG_HOST}).

!
    fi
    exit 0

