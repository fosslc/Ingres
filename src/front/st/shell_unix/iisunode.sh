:
#
#  Copyright (c) 2002, 2008 Ingres Corporation 
# 
# 
#
#  Name: iisunode -- Secondary Cluster Node setup script for Ingres DBMS
#
#  Usage:
#	iisunode
#
#  Description:
#	This script is currently used in lieu of ingbuild when setting
#	secondary nodes in an Ingres cluster.
#
#	Most of the heavy lifting was completed when the first node
#	was set up, so in most cases all that the set up scripts
#	invoked here will do, is copy parameters to the current node.
#
#  Exit Status:
#	0       setup procedure completed.
#	<>0     setup procedure did not complete.
#
#  DEST = utility
## History:
##	12-Feb-2002 (devjo01)
##		Created.	
##	27-mar-2002 (devjo01)
##		Make sure installation is not running on any node.
##	20-Jan-2004 (bonro01)
##		Removed ingbuild calls for PIF installs.
##    30-Jan-2004 (hanje04)
##        Changes for SIR 111617 changed the default behavior of ingprenv
##        such that it now returns an error if the symbol table doesn't
##        exists. Unfortunately most of the iisu setup scripts depend on
##        it failing silently if there isn't a symbol table.
##        Added -f flag to revert to old behavior.
##	03-feb-2004 (somsa01)
##		Backed out last change for now.
##	09-apr-2004 (devjo01)
##		- Rather than run setup scripts when adding nodes, simply
##		clone the source configuration.  Setup scripts were only
##		populating config.dat with default values, with the
##		notable exception of iisudbms which also created the
##		logs.  Advantage is simplicity, and that user can
##		tweak his first node just the way he wants it, then
##		duplicate the settings to the other nodes.
##		- Backup protect.dat also JIC we abort.
##		- Support "-test" parameter.
##	13-may-2004 (devjo01)
##		- Add a reminder after script is run that user must
##		run mkrc if they want automatic startup and shutdown
##		for the installation on this node.
##		- Space some of the prompts for clarity.
##	29-jan-2005 (devjo01/mutma03)
##		- Allow "ingres" not to be 'ingres'.
##		- Lock configuration file
##		- Support removal of nodes.
##		- Make sure quorum & DLM are up.
##		- Add default definition for a NCA server class.
##		- Make sure all base locations are reachable by this node.
##	03-feb-2005 (devjo01)
##		- Prevent running this script as root.
##	14-feb-2005 (devjo01)
##		- Copy environment setup scripts to owners home directory
##		  if this directory is not shared.
##		- Set location for DLM library if it differs from default.
##	06-apr-2005 (devjo01)
##		- Correct logic used to add node affinity classes to 
##		  properly copy the cache definitions.
##		- Shorten default name of NAC configurations from
##		iiaffinityclassX to iinacXX (same as cache name and
##		class name)
##	        - Get "ingres" user from 1st node since this
##		may not be setup yet.
##      07-Apr-2005 (hanje04)
##          Bug 114248
##          When asking users to re-run scripts it's a good idea to tell
##          them which script needs re-running.
##	20-Apr-2005 (mutma03)
##          - Enhancement, Added get_rshmethod to input the type remote server
##            to be used in cluster operations.
##	06-Apr-2005 (mutma03)
##	    - Misc. changes.
##	15-May-2005 (mutma03)
##	    Modified iiaffinityclassX to iicnaXX to reflect the documentation.
##	31-May-2005 (mutma03)
##	    Removed unsupported command line options.
##	12-Jun-2005 (mutma03)
##	    Added version compatibility check with OpenDLM.
##	16-Aug-2005 (mutma03)
##	    Fixed append_class for creating cache entries for affinity servers.
##	29-Aug-2005 (mutma03)
##	    Added code to initialize the default rsh method used by the primary
##	    node.
##	18-Oct-2005 (mutma03)
##	    Miscellaneous changes. Removed a debug statement.
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	01-Dec-2005 (devjo01) b115580
##	    Call syscheck to confirm that shared memory requirements
##	    are supportable.
##	05-Jan-2007 (bonro01)
##	    Change Startup Guide to Installation Guide.
##	07-jan-2008 (joea)
##	    Discontinue use of cluster nicknames.
##	15-Dec-2009 (hanje04)
##	    Bug 123097
##	    Remove references to II_GCNXX_LCL_VNODE as it's obsolete.
#----------------------------------------------------------------------------

#
#   Set up some global variables
#
    . iisysdep

    . iishlib

    self="`basename $0`"
    DATESTAMP="`date +'%Y%m%d_%H%M'`"
    DOIT=""
    DIX=""

#
#   Generate configuration for this node based on that of 1st node.
#
# 	$1 = file name (config.dat | protect.dat)
#
convert_configuration()
{
    grep -i -v "^`grep_search_pattern \"ii.${CONFIG_HOST}.\"`" \
      $II_CONFIG/${1}_$DATESTAMP > $II_CONFIG/${1}
    grep -i "^`grep_search_pattern \"ii.${first_node}.\"`" \
       $II_CONFIG/${1}_$DATESTAMP | \
      sed "s/^ii\.${first_node}\./ii\.${CONFIG_HOST}\./" \
      >> $II_CONFIG/${1}
}

#
#   Utility routine to generate a log name based on a base log name,
#   a segment number, and an optional node extention.  Final name
#   must be 36 chars or less.  Name is echoed to stdout.
#
#       $1 = base name.
#       $2 = segment # (1 .. 16)
#       $3 = $CONFIG_HOST, or "".
#
gen_log_name()
{
    ext=""
    [ -n "$3" ] && ext="_`echo $3 | tr '[a-z]' '[A-Z]'`"
    paddedpart=`echo "0${2}" | sed 's/0\([0-9][0-9]\)/\1/'`
    echo "${1}.l${paddedpart}${ext}" | cut -c 1-36
}

#
#   get remote shell server to be used for remote command execution
#
get_rshmethod()
{
    SERVER="none,rsh,ssh"
    while :
    do

	PRIM_RSH=`iigetres "ii.${first_node}.config.rsh_method"`
	echo $PRIM_RSH | grep -q ssh && PRIM_RSH="ssh"
	echo $PRIM_RSH | grep -q rsh && PRIM_RSH="rsh"
        echo -n Please select the remote shell server {$SERVER} [$PRIM_RSH]:
        read answer junk
        [ -n "$junk"] ||
        {
            cat << !

extra input in response.
!
            continue
        }

        [ -n "$answer" ] || answer=$PRIM_RSH 
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
#
#   Create transaction logs for new node
#
#	$1 = "primary" | "dual"
#	$2 = "" | "-dual"
#	$3 = "-init_log" | "-init_dual"
#
setup_log()
{
    $DOIT ipcclean 

    $DOIT iimklog $2 ||
    {
	error_box << !
Fail to create $1 transaction log for $CONFIG_HOST.
!
	exit 1
    }

    $DOIT csinstall - ||
    {
	cat << !

Unable to install shared memory.  You must correct the problem described above
and re-run $self.

!
	$DOIT ipcclean
	exit 1
    }
    $DOIT rcpconfig $3 >/dev/null ||
    {
	error_box << !
Fail to format $1 transaction log for $CONFIG_HOST.
!
	$DOIT ipcclean
	exit 1
    }
    $DOIT ipcclean
}

#
# Location check to ensure the directory exists
#
#       $1 = Ingres environment variable
#       $2 = Content subdirectory name.
#
location_check()
{
    dir=`ingprenv $1`
    subdir=$2

    [ -d $dir/ingres/$subdir ] ||
    {
       error_box << !
Cannot access content directory for location defined for Ingres environment variable $1 ($dir/ingres/$subdir).

Please check that this directory is accessible from ${CONFIG_HOST}.
!
        exit 1
    }
}

#
# Backup configuration file
#
# 	$1 = file name (config.dat | protect.dat)
#
backup_file()
{
    [ -f $II_CONFIG/${1} ] &&
    {
	cp -p $II_CONFIG/${1} $II_CONFIG/${1}_$DATESTAMP
	cmp $II_CONFIG/${1} $II_CONFIG/${1}_$DATESTAMP ||
	{
	    error_box << !
Could not backup ${1}.  Check disk space.
!
	    exit 11
	}
    }
}

#
# Revert to saved version of configuration file
#
#       $1 = file name (config.dat | protect.dat)
#
restore_file()
{
    [ -f $II_CONFIG/${1}_$DATESTAMP ] &&
     mv -f $II_CONFIG/${1}_$DATESTAMP $II_CONFIG/${1}
}

#
# Add a server class based on default dbms class def in 1st node
#
#   $1 = target file (config.dat | protect.dat )
#   $2 = Dest node for new server class definition
#   $3 = server config name(NAC).
#
append_class()
{
    [ -s $II_CONFIG/${1} ] &&
    {
	pattern="`grep_search_pattern ii.${2}.dbms.*.`"
	grep "${pattern}" $II_CONFIG/${1} | grep -v dmf_connect \
	 | sed "s/\.\*\./\.${3}\./" >> $II_CONFIG/${1}
	if [ "`iigetres ii.${2}.dbms.*.cache_sharing`" = "OFF" ]
	then
	    pattern="`grep_search_pattern ii.${2}.dbms.private.*.`"
	    grep "${pattern}" $II_CONFIG/${1} \
	     | sed "s/\.\*\./\.${3}\./" >> $II_CONFIG/${1}
	fi
    }
}

#
# remove_node - do the actual removal of a node.
#
remove_node()
{
    # Blast log files
    NUM_PARTS=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_parts`
    if [ -z "$NUM_PARTS" -o \
      -n "`echo $NUM_PARTS | sed 's/[1-9][0-9]*//'`" ]
    then
	warning_box << !
Could not determine transaction log configuration.  Transaction logs for node $CONFIG_HOST must be manually removed.
!
    else
	PRIMARY_LOG_NAME=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_name`
	[ -z "$PRIMARY_LOG_NAME" ] && PRIMARY_LOG_NAME='ingres_log'
	DUAL_LOG_NAME=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_name`

	PART=0
	while [ $PART -lt $NUM_PARTS ]
	do
	    PART=`expr $PART + 1`
	    DIR=`iigetres ii.$CONFIG_HOST.rcp.log.log_file_$PART`
	    [ -n "$DIR" ] || continue
	    OLD_NAME=`gen_log_name $PRIMARY_LOG_NAME $PART $CONFIG_HOST`
	    [ -f $DIR/ingres/log/$OLD_NAME ] &&
	     $DOIT rm -f $DIR/ingres/log/$OLD_NAME

	    [ -z "$DUAL_LOG_NAME" ] && continue

	    DIR=`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_$PART`
	    [ -n "$DIR" ] ||
	    {
		[ $PART -eq 1 ] && DUAL_LOG_NAME=""
		continue;
	    }

	    OLD_NAME=`gen_log_name $DUAL_LOG_NAME $PART $CONFIG_HOST`

	    [ -f $DIR/ingres/log/$OLD_NAME ] &&
	     $DOIT rm -f $DIR/ingres/log/$OLD_NAME
	done
    fi

    # Strip out config.dat entries for removed node
    for f in config.dat protect.dat
    do
	grep -i -v "^ii\.${CONFIG_HOST}\." \
	  $II_CONFIG/${f}_$DATESTAMP > $II_CONFIG/${f}
    done

    if [ $first_node = $CONFIG_HOST ]
    then
	# Need to reset server_host
	first_node="`grep \
	 '^ii\..*.config\.cluster.id:' $II_CONFIG/config.dat | \
 sed 's/^ii\.\(.*\)\.config\.cluster\.id:[ 	]*\([0-9]*\).*$/\2 \1/' | \
	 sort -n | tail -1 | sed 's/^[0-9]* //'`"
	$DOIT iisetres 'ii.*.config.server_host' $first_node
    fi

    # Remove miscellaneous unwanted files
    $DOIT rm -f $II_SYSTEM/ingres/admin/$CONFIG_HOST/*
    $DOIT rmdir $II_SYSTEM/ingres/admin/$CONFIG_HOST
    $DOIT rm -f $II_SYSTEM/ingres/files/memory/$CONFIG_HOST/*
    $DOIT rmdir $II_SYSTEM/ingres/files/memory/$CONFIG_HOST
}

#
# cancel_add - backout adding a node
#
cancel_add()
{
    remove_node
    restore_file config.dat
    restore_file protect.dat
    log_config_change_banner "NODE ADD FAILED/CANCELED" 30
    unlock_config_dat
}

#
# Undo changes to config files, saving modified files for post-mort
#
status_quo_ante()
{
    for f in config.dat protect.dat
    do
	[ -f $II_CONFIG/${f} ] || continue
	box << ! >> $DEBUG_LOG_FILE

${f}

!
	cat $II_CONFIG/${f} >> $DEBUG_LOG_FILE
	restore_file ${f}
    done
}

# 
#	Workaround for the issue, "setting startup count messes up the
#	cache_sharing fields of the dbms server" because of special rules.
#	Ideally should be fixed in PMsetCRval.
#	$1=node $2=NAC $3=startup count
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
#   MAIN LINE
#
#   Parse parameters
#
    BATCH=false
    WRITE_RESPONSE=false
    READ_RESPONSE=false
    REMOVE_CONFIG=false

    [ -n "$SHELL_DEBUG" ] && set -x

    while [ -n "$1" ]
    do
	if [ "$1" = "-remove" ] ; then
	    REMOVE_CONFIG=true
	elif [ "$1" = "-test" ] ; then
	    #
	    # This prevents actual execution of most commands, while
	    # config.dat & symbol.tbl values are appended to the trace
	    # file, prior to restoring the existing copies (if any)
	    #
	    DEBUG_LOG_FILE="/tmp/${self}_test_output_$DATESTAMP"
	    DOIT=do_it
	    DIX="\\"
	    case "$2" in
		new)
		    DEBUG_FAKE_NEW=true
		    DEBUG_DRY_RUN=true
		    shift
		    ;;
		dryrun)
		    DEBUG_DRY_RUN=true
		    shift
		    ;;
	    esac

	    cat << !
Logging $self run results to $DEBUG_LOG_FILE

DEBUG_DRY_RUN=$DEBUG_DRY_RUN  DEBUG_FAKE_NEW=$DEBUG_FAKE_NEW
!
	    cat << ! >> $DEBUG_LOG_FILE
Starting $self run on `date`
DEBUG_DRY_RUN=$DEBUG_DRY_RUN  DEBUG_FAKE_NEW=$DEBUG_FAKE_NEW

!
	elif [ "$1" ] ; then
	    cat << !

ERROR: invalid parameter "{$1}".

Usage $self [ -remove ] 
!
	    exit 1
	fi
	shift
    done
    [ -n "$SHELL_DEBUG" ] && set -x

#
#   Validate environment
#
    $REMOVE_CONFIG ||
    {
	$CLUSTERSUPPORT ||
	{
	    error_box << !
Target machine does not appear to be capable of supporting Ingres
in a clustered configuration.
!
	    exit 1
	}
    }
	
    oktogo=false
    while true
    do
	[ -n "$II_SYSTEM" ] || break
	[ -d $II_SYSTEM ] || break
	oktogo=true
	break
    done

    $oktogo ||
    {
	error_box << !
II_SYSTEM must be set to the root of the Ingres directory tree
before you may run $self.  Please see the Installation Guide for
instructions on setting up the Ingres environment.  
!
	exit 1
    }

    # Continue checks
    oktogo=false

    while true
    do
	[ -f $II_CONFIG/config.dat ] || break

	first_node="`iigetres 'ii.*.config.server_host' | tr '[A-Z]' '[a-z]'`"
	[ -n "$first_node" ] || break

	oktogo=true
	break
    done

    $oktogo ||
    {
	error_box << !
Ingres must be installed and set up for the primary node before
$self is run for the secondary nodes.
!
	exit 1
    }

#   Get "ingres" user since user no longer needs to literally be 'ingres'.
    INGUSER="`iigetres ii.$first_node.setup.owner.user 2> /dev/null`"
    [ -n "$INGUSER" ] || INGUSER='ingres'
    
#   Check if caller is "ingres".
    [ "$WHOAMI" != "$INGUSER" ] &&
    {
	error_box << !
You must be '$INGUSER' to run this script.
!
	exit 10
    }

    if $REMOVE_CONFIG
    then
	# Confirm this node is clustered.
	RESVAL=`iigetres ii.$CONFIG_HOST.config.cluster_mode`
	[ "X$RESVAL" = "XON" ] ||
	{
	    error_box << !
Current node ($CONFIG_HOST) does not appear to be part of a cluster.
!
	    exit 1
	}

	# Do not remove the last node.
	[ `grep '^ii\..*\.config\.cluster_mode:[    ]*[Oo][Nn]$' \
           $II_CONFIG/config.dat | wc -l` -lt 2 ] &&
	{
	    warning_box << !
You cannot remove the last node in a cluster using $self.  Please use iiuncluster to convert Ingres instance to a non-cluster configuration.
!
	    exit 1
	}

	# Confirm that all nodes are down
	oktogo=true
	if csreport >& /dev/null
	then
	    freeipc=false
	else
	    csinstall -
	    freeipc=true
	fi
	rcpstat -any_csp_online -silent &&
	{
	    error_box << !
All nodes in cluster must be shutdown prior to changing the static configuration of the cluster.
!
	    oktogo=false
	}

	# Confirm no transactions remain in this node's logs.
	$oktogo && rcpstat -transactions -silent &&
	{
	    error_box << !
There are still transactions in the transaction log for this node.  You may not remove a node from a cluster before these transactions have been resolved.  Please restart this node and then stop it cleanly so these transactions will be removed from the log.
!
	    oktogo=false
	}

	$freeipc && ipcclean >& /dev/null
	
	$oktogo || exit 1

	wrap_lines << ! 
This procedure will remove the current machine ($CONFIG_HOST) from the Ingres cluster.

All the transaction logs for this node have been confirmed empty and will be destroyed as part of this procedure.

!
	prompt "Do you really want to remove $CONFIG_HOST?" n || \
	 exit 1

	lock_config_dat $self || exit 12

	trap "echo 'Unexpected failure' ; unlock_config_dat; exit 11" 0 1 2 3 15

	log_config_change_banner "BEGIN REMOVE NODE" 24

	# Backup config.dat, protect.dat
	backup_file config.dat
	backup_file protect.dat

	remove_node

	trap ":"  0 1 2 3 15
	unlock_config_dat
	wrap_lines << !

Node $CONFIG_HOST has been removed from the cluster.  Old configuration file has been saved as config.dat_${DATESTAMP} for documentation.
!
	[ -n "$DOIT" ] && status_quo_ante
	exit 0
    fi

    # Continue checks for adding a node.

    if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
	VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=dbms` ||
	{
	    cat << !
   
$VERSION
   
!
	    exit 1
	}
    else
	VERSION=`head -1 $II_SYSTEM/ingres/version.rel` ||
	{
	    cat << !
   
Missing file $II_SYSTEM/ingres/version.rel
   
!
	    exit 1
	}
    fi
   
    RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`
   
    oktogo=false

    while true
    do
	RESVAL=`iigetres ii.$first_node.config.dbms.$RELEASE_ID`
	[ "$RESVAL" = "complete" ] || break
    
	oktogo=true
	break
    done

    $oktogo ||
    {
	error_box << !
Ingres must be installed and set up for the primary node before
$self is run for the secondary nodes.
!
	exit 1
    }

    RESVAL=`iigetres ii.$CONFIG_HOST.config.cluster.id`
    [ -n "$RESVAL" ] &&
    {
	error_box << !
$CONFIG_HOST has already been set up for clustering.
!
	exit 1
    }

    oktogo=false

    while true
    do
	II_ADMIN=$II_SYSTEM/ingres/admin/$first_node
	[ -f $II_ADMIN/symbol.tbl ] || II_ADMIN=$II_CONFIG
	export II_ADMIN
	II_INSTALLATION="`ingprenv II_INSTALLATION`"
	[ -n "$II_INSTALLATION" ] || break
	oktogo=true
	break
    done
    
    $oktogo ||
    {
	error_box << !
Could not determine installation identifier.  Check if Ingres is setup correctly for clusters.
!
	exit 1
    }

    RESVAL=`iigetres ii.$first_node.config.cluster.id`
    case "$RESVAL" in
    [1-9]|[1][0-6])		# Setup for cluster
	;;
    *)				# Not setup for cluster, check NUMA cluster
	RESVAL=`iigetres ii.$first_node.config.cluster.numa | \
	 tr '[a-z]' '[A-Z]'`
	[ "$RESVAL" = "ON" ] ||
	{
	    error_box << !
Target Ingres instance ("${II_INSTALLATION}") on $first_node was not set up for Clustered Ingres.  Ingres instance on $first_node must be converted to a cluster configuration before you can run $self.
!
	    exit 1
	}
	;;
    esac

    # Make sure primary locations are accessible as a sanity check
    # Unfortunately without access to iidbdb we can't check ALL locations.
    location_check II_DATABASE data
    location_check II_JOURNAL jnl
    location_check II_CHECKPOINT ckp
    location_check II_DUMP dmp
    location_check II_WORK work

#   Check if quorum daemon is running
    [ -f /etc/init.d/iiquorumd ] ||
    {
	error_box << !
You must install the "iiquorumd" Quorum Manager prior to setting this node up as part Ingres Grid Option cluster.
!
	exit 8
    }

    /etc/init.d/iiquorumd status >& /dev/null ||
    {
	error_box << !
iiquorumd process should be running prior to setting this node up as part Ingres Grid Option cluster.
!
	exit 8
    }

#   Check if DLM process is running.  This check is specific to OpenDLM
#   and will need to be generalized if & when additional DLMs are used.
    [ -f /etc/init.d/haDLM ] ||
    {
	error_box << !
You must install the OpenDLM distributed lock manager before setting this node up as part Ingres Grid Option cluster.
!
	exit 9
    }

    /etc/init.d/haDLM status >& /dev/null ||
    {
	error_box << !
OpenDLM distributed lock manager should be running prior to setting this node up as part Ingres Grid Option cluster.
!
	exit 9
    }

#   Locate OpenDLM application library.
    dfltdlmlib=`iigetres 'ii.*.config.dlmlibrary'`
    if [ -n "$dfltdlmlib" -a -x $dfltdlmlib ]
    then
	opendlmlib="$dfltdlmlib"
    else
	opendlmlib=""
	for f in $(echo "$LD_LIBRARY_PATH" | sed 's/:/ /g') \
	         /usr/lib /usr/local/lib
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
    fi
#   Check for openDLM version compatiblity. Reject if the DLM is older than 0.4
#   The format of DLM version has been changed in later versions and will have
#   Major.Minor.buildlevel.
    DLM_VERSION=`cat /proc/haDLM/version`
    DLM_MAJOR_VER=`echo $DLM_VERSION|awk -F '.' '{ print $1 }'`
    DLM_MINOR_VER=`echo $DLM_VERSION|awk -F '.' '{ print $2 }'`
    if [  $DLM_MAJOR_VER -eq 0 -a $DLM_MINOR_VER -le 3 ]
    then
             error_box <<!
 Installed openDLM version $DLM_VERSION is stale. Please upgrade the openDLM pr
or to converting this Ingres instance into an Ingres Grid Option configuration.
!
             exit 9
    fi
#   Accept if the major number of the DLM version and ingres version are equal

    if [ "`iigetres ii.*.config.dlm_version`" != "" ]
    then
        EXP_MAJOR_VER=`iigetres ii.*.config.dlm_version|awk -F '.' '{ print $1
}
 '`
        if [ $DLM_MAJOR_VER -ne $EXP_MAJOR_VER ]
        then

            error_box << !
openDLM version $DLM_VERSION is not compatible with Ingres. Please update the
correct version of openDLM prior to converting this Ingres instance into an Ingr
sGrid Option configuration.
!
            exit 9
        fi
    else
        wrap_lines << !

Installed OpenDLM version is $DLM_VERSION. Ingres configuration does not specify an explicit DLM version requirement, and this version of Ingres is expected to work well with OpenDLM 0.4/0.9.4.
!
    fi

    wrap_lines << !

$self will set up Ingres on $CONFIG_HOST, as a node in the Clustered Ingres instance "$II_INSTALLATION" using the same configuration as for all $VERSION components already setup on $first_node. 

$opendlmlib will be used as the Distributed Lock Manager shared library.

!
    grep -q "^ii\.$CONFIG_HOST" $II_CONFIG/config.dat &&
    {
	warning_box << !
It appears that $CONFIG_HOST has already been set up non-clustered.  If you run this script, current configuration will be overwritten after being backed up.
!
    }

    $BATCH ||
    {
	prompt "Do you want to continue this setup procedure?" n || exit 1
    }

    warning_box << !

All other Ingres nodes in this Ingres cluster must be shutdown
in order to run this procedure.

!
    $BATCH ||
    {
	prompt "Do you want to continue this setup procedure?" y || exit 1
    }

    lock_config_dat $self || exit 12

    trap "cancel_add ; exit 11" 1 2 3 15
    trap "cancel_add" 0

    backup_file config.dat
    backup_file protect.dat
    
    log_config_change_banner "BEGIN NODE ADD" 24

    #
    #  Get node number.
    #
    gen_nodes_lists
    echo ""
    get_node_number
    echo ""
    get_rshmethod
    wrap_lines << !

$CONFIG_HOST will be set up as node $II_CLUSTER_ID.

!
    $BATCH ||
    {
	prompt "Do you want to continue this setup procedure?" y || exit 1
    }

    wrap_lines << !

Setting up $CONFIG_HOST
!
    #
    #	Replicate symbol.tbl to admin area.
    #
    create_node_dirs $CONFIG_HOST
    $DOIT cp $II_ADMIN/symbol.tbl $II_SYSTEM/ingres/admin/$CONFIG_HOST
    unset II_ADMIN

    # Add new node to config.dat as clone of the old node.
    convert_configuration config.dat
    convert_configuration protect.dat
    $DOIT iisetres "ii.$CONFIG_HOST.config.cluster" "ON"
    $DOIT iisetres "ii.$CONFIG_HOST.config.cluster.id" $II_CLUSTER_ID
    $DOIT iisetres "ii.$CONFIG_HOST.gcn.local_vnode" `uname -n`
    [ "X$dfltdlmlib" = "X$opendlmlib" ] || \
     $DOIT iisetres "ii.$CONFIG_HOST.config.dlmlibrary" "$opendlmlib"
    ingunset \
     "II_GCN${II_INSTALLATION}_`echo $first_node | tr '[a-z]' '[A-Z'`_PORT" 

    # Add new NAC to all nodes.
    NAC=`echo "IICNA0${II_CLUSTER_ID}" | sed 's/[0-9]\([0-9][0-9]\)/\1/'`

    for node in `echo $NODESLST | tr '[A-Z]' '[a-z]'` $CONFIG_HOST
    do
	# Skip if already exists
	grep -iq "^`grep_search_pattern \"ii.${node}.dbms.${NAC}.\"`" \
	    $II_CONFIG/config.dat && continue
	append_class config.dat $node $NAC 
	append_class protect.dat $node $NAC 

	startup_count $node $NAC 0
	$DOIT iisetres "ii.$node.dbms.$NAC.cache_name" "$NAC"
	$DOIT iisetres "ii.$node.dbms.$NAC.cache_sharing" 'OFF'
	$DOIT iisetres "ii.$node.dbms.$NAC.class_node_affinity" 'ON'
	$DOIT iisetres "ii.$node.dbms.$NAC.server_class" "$NAC"
    done

    # Turn off startup count for all NAC classes this node
    grep -i "^`grep_search_pattern \"ii.$CONFIG_HOST.dbms.\"`.*\.class_node_affinity:[	 ]*[Oo][Nn]" $II_CONFIG/config.dat \
     | awk -F '.' '{ print $2, $4 }' | \
    while read node NAC
    do
	startup_count $node $NAC 0
    done

    # Make sure we can support amount of shared memory
    syscheck ||
    {
	error_box << !
Please correct the configuration issues reported above before running $self again.
!
	exit 1
    }

    # Initialize transaction logs for new node.
    setup_log "primary" "" "-init_log"
    [ -n "`iigetres ii.$CONFIG_HOST.rcp.log.dual_log_1`" ] && \
	setup_log "dual" "-dual" "-init_dual"

    # Copy environment setup scripts to home directory unless already present.
    for RESVAL in bash tcsh
    do
	[ -f ~/.ing${II_INSTALLATION}${RESVAL} ] ||
	    cp -p $II_SYSTEM/.ing${II_INSTALLATION}${RESVAL} ~
    done

    cat << !

Node $CONFIG_HOST set up is complete.

!
    [ -x $II_SYSTEM/ingres/utility/mkrc ] && wrap_lines << !
Please note you must run "mkrc -i" as root after sourcing your
Ingres environment if you want the Ingres instance on this node to
automatically start on machine reboot and stop on machine shutdown.

!
    [ -n "$DOIT" ] && status_quo_ante
    trap ":" 0
    unlock_config_dat
    exit 0
