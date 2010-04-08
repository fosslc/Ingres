:
#  Copyright (c) 2002, 2005 Ingres Corporation
# 
# 
#  $Header $
#
#  Name: iiuncluster - Convert Ingres from Clustered to non-Clustered.
#
#  Usage:
#	iiuncluster [-force [<datestamp>]
#
#  Description:
#	Converts the Ingres instance running on the current host machine
#	to a non-clustered Ingres instance.  Ingres will be disabled
#	on all other nodes for this instance.
#
#	This script must be run as ingres, and while installation is
#	down.
#
# Parameters:
#	-force	: if present, makes iiuncluster run without prompts.
#		  this parameter should only be used by iimkcluster
#		  to undo a partial conversion.
#
# Exit status:
#	0	OK - conversion to non-clustered Ingres successful.
#	1	User decided not to procede.
#	3	Ingres instance fails basic setup sanity checks.
#	4	Ingres is apparently not set up.
#	5	Ingres is not set up for clusters.
#	6	Ingres must be shutdown before conversion.
#	7	Recoverable transactions must be scavanged before convert.
#	10	Must be "ingres" to run script.
#	11	Process failed or was interrupted during convert.
#	12	Configuration file is locked.
#
## History
##	19-jun-2002 (devjo01)
##		Initial version.
##	27-feb-2004 (devjo01)
##		Remove dependency on 'ingbuild'.
##	12-mar-2004 (devjo01)
##		Remove redundant "cat"s.
##	09-apr-2004 (devjo01)
##		Correctly handle admin directories.
##	28-jan-2005 (devjo01)
##		CATOSL banner.  Handle "-force" correctly.  Allow
##		"ingres" not to be 'ingres'.  Lock config file
##	03-feb-2005 (devjo01)
##		Don't allow this to be run as root.
##	15-mar-2005 (mutma03)
##	  	Restore the document link which was deleted 
##		during iimkcluster.
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
#   mv wrapper to allow suppression of rename.
#
#	$1 = target.
#	$2 = new name
#
rename_file()
{
    if $FAKEIT
    then
	echo "Skipped mv of $1 to $2"
    else
	mv -f $1 $2
    fi
}

#
#   rm wrapper to allow suppression of file deletion.
#
#	$1 = target.
#
remove_file()
{
    [ -f $1 -o -d $1 ] &&
    {
	if $FAKEIT
	then
	    echo "Skipped rm of $1"
	else
	    /bin/rm -rf $1
	fi
    }
}

#
#   Convert a configuration file
#
#	$1 = config.dat | protect.dat
#
convertconfig()
{
    [ -f $II_CONFIG/$1_$DATESTAMP ] ||
    {
#	Only backup files if not '-force'
	cp -p $II_CONFIG/$1 $II_CONFIG/$1_$DATESTAMP
	cmp -s $II_CONFIG/$1_$DATESTAMP $II_CONFIG/$1 2>/dev/null ||
	{
	    error_box << !
Could not backup $1, check disk space, permissions.
!
	    /bin/rm -f $II_CONFIG/$1_$DATESTAMP 2>/dev/null
	    exit 11
	}
	#   Recreate config.dat without entries for other nodes
	awk -F '.' -f /tmp/$SELF.awk.$$ $II_CONFIG/$1_$DATESTAMP \
	 > $II_CONFIG/$1
	return 0
    }
    return 1
}

#
#   Backup a configuration file if not aleady backed up.
#
#	$1 = config.dat | protect.dat
#
backup_file()
{
    [ -f $II_CONFIG/${1} ] && [ -f $II_CONFIG/${1}_$DATESTAMP ] ||
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
#
#   Restore a configuration file from backup
#
#	$1 = config.dat | protect.dat
#
restoreconfig()
{
    [ -f $II_CONFIG/$1.test ] && rm -f $II_CONFIG/$1.test
    mv $II_CONFIG/$1 $II_CONFIG/$1.test
    [ -f $II_CONFIG/$1_$DATESTAMP ] && \
     mv $II_CONFIG/$1_$DATESTAMP $II_CONFIG/$1
}

#
#   Cleanup routine
#
cleanup()
{
    $CSINSTALL && ipcclean
    remove_file /tmp/$SELF.files.$$
    remove_file /tmp/$SELF.awk.$$
    $FAKEIT && 
    {
	cat << !

== Resultant symbol table ==
!
	ingprenv
	wrap_lines << !

Restoring original symbol.tbl & config.dat.  Modified config.dat saved as config.dat.test

!
	[ -f $II_CONFIG/symbol.tbl ] && rm $II_CONFIG/symbol.tbl
	mv $II_CONFIG/symbol.tbl.$$ $II_SYSTEM/ingres/admin/$CONFIG_HOST/symbol.tbl
	restoreconfig config.dat
	restoreconfig protect.dat
    }
    unlock_config_dat
}

#
#   Start of MAINLINE
#

#   Set up defaults and useful constants.
    SELF="`basename $0`"
    CONFIRM=true
    CSINSTALL=false
    FAKEIT=false
    DATESTAMP="`date +'%Y%m%d_%H%M'`"

    while [ -n "$1" ]
    do
	case "$1" in
	-force)	# Convert without prompting user.
	    CONFIRM=false
            [ -n "$2" ] &&
            {
                [ -f $II_SYSTEM/ingres/config.dat_$2 ] &&
                {
                    DATESTAMP=$2
                    shift
                }
            }
	    ;;
	-test)	# For testing without actually doing anything.
	    FAKEIT=true
	    warning_box << !
Running $SELF in '-test' mode.  Conversion will not actually be perfomed.
!
	    ;;
	*)	# Error.
	    echo "Bad parameter! ($1)."
	    exit 4
	    ;;
	esac
	shift
    done

    [ -n "$SHELL_DEBUG" ] && set -x

#   Get "ingres" user since user no longer needs to literally be 'ingres'.
    INGUSER="`iigetres ii.$CONFIG_HOST.setup.owner.user 2> /dev/null`"
    [ -n "$INGUSER" ] || INGUSER='ingres'
    
#   Check if caller is "ingres".
    [ "$WHOAMI" != "$INGUSER" ] &&
    {
	box << !
You must be '$INGUSER' to run this script.
!
	exit 10
    }

#   Perform some basic installation set-up checks.
    [ -n "$II_SYSTEM" ] ||
    {
	error_box << !
Environment variable II_SYSTEM is not set. 
!
	exit 3
    }

    check_path bin || exit 3
    check_path utility || exit 3

    lock_config_dat $SELF || exit 12
    trap "unlock_config_dat" 0

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
	    wrap_lines << !

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

#   Make a list of other nodes.
    ALLNODES="`grep '^ii\..*\.config\.cluster_mode:[ 	]*[Oo][Nn]$' \
	   $II_CONFIG/config.dat | awk -F . '{ print $2 }'`"

#   Check if clustered.
    $CONFIRM &&
    {
	VALUE=`iigetres ii.$CONFIG_HOST.config.cluster_mode`
	[ "X${VALUE}" = "XON" ] ||
	{
	    error_box << !
Ingres is not set up in a cluster configuration.
!
	    exit 5
	}
    }

#   Establish trap to perform cleanup.
    trap "cleanup" 0
    trap "cleanup ; exit 11" 1 2 3 15

#   Check if installation is up.
    rcpstat -online -silent > /dev/null || \
     csinstall > /dev/null && CSINSTALL=true
    rcpstat -any_csp_online -silent > /dev/null &&
    {
	warning_box << !
Ingres must be shutdown on ALL nodes before converting to a non-cluster configuration.

Please 'ingstop -cluster' your installation before running $SELF again.
!
	exit 6
    }

#   Check if recoverable transactions exist.
    OTHERNODES=""
    PLURAL=""
    for node in $ALLNODES
    do
	rcpstat -transactions -silent -node $node > /dev/null && 
	{
	    warning_box << !
There are recoverable transactions for one or more nodes.  Please start and stop the Ingres cluster to make sure all transactions are recovered before converting to a non-cluster configuration.
!
	    exit 7
	}
	[ $node = $CONFIG_HOST ] || 
	{
	    if [ -z "$OTHERNODES" ]
	    then
		OTHERNODES="$node"
	    else
		OTHERNODES="$OTHERNODES $node"
		PLURAL='s'
	    fi
	    echo "\$2==\"$node\" { next }" >> /tmp/$SELF.awk.$$
	}
    done
    echo '$4=="cluster" && $5~"^id:" { next }' >> /tmp/$SELF.awk.$$
    echo '{print}' >> /tmp/$SELF.awk.$$
    
#   Prompt for user confirmation.
    II_INSTALLATION="`ingprenv II_INSTALLATION`"
    $CONFIRM &&
    {
	wrap_lines << !

$SELF will convert Ingres $VERSION instance "$II_INSTALLATION" on host $CONFIG_HOST to a non-clustered Ingres configuration.

!
	[ -n "$OTHERNODES" ] &&
	{
	    wrap_lines << !
Ingres instance "$II_INSTALLATION" will be disabled from running on the following node$PLURAL: $OTHERNODES.

Empty transaction logs for the disabled node$PLURAL will be destroyed.

!
	}

	prompt "Are you sure you want to continue this procedure?" n || exit 1

	wrap_lines << !

Proceeding with conversion.  Existing config.dat saved as "config.dat_$DATESTAMP".
!
    }

#
#   The Rubicon is crossed.
#
    # Clean shared memory segments now, before memory directories trashed.
    $CSINSTALL && ipcclean
    CSINSTALL=false

    log_config_change_banner "BEGIN IIUNCLUSTER" 24

#   Mark cluster specific directories for death.
    echo "$II_SYSTEM/ingres/files/memory" > /tmp/$SELF.files.$$
    echo "$II_SYSTEM/ingres/admin" >> /tmp/$SELF.files.$$

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
	    $CONFIRM && warning_box << !
Cannot find configuration information for "iigetres ii.$CONFIG_HOST.rcp.log.log_file_$PART".
!
	    WARNINGS=1
	else
	    OLD_NAME=`gen_log_name $PRIMARY_LOG_NAME $PART $CONFIG_HOST`

	    if [ -f $DIR/ingres/log/$OLD_NAME ]
	    then
		rename_file $DIR/ingres/log/$OLD_NAME \
 $DIR/ingres/log/`gen_log_name $PRIMARY_LOG_NAME $PART`
	    else
		$CONFIRM && warning_box << !
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
		$CONFIRM && warning_box << !
Cannot find configuration information for "iigetres ii.$CONFIG_HOST.rcp.log.dual_log_$PART".
!
		WARNINGS=1
	    fi
	    continue;
	fi

	OLD_NAME=`gen_log_name $DUAL_LOG_NAME $PART $CONFIG_HOST`

	if [ -f $DIR/ingres/log/$OLD_NAME ]
	then
	    rename_file $DIR/ingres/log/$OLD_NAME \
     $DIR/ingres/log/`gen_log_name $DUAL_LOG_NAME $PART`
	elif [ $PART -eq 1 ]
	then
	    # If 1st dual log missing, assume no dual logging.
	    DUAL_LOG_NAME=""
	else
	    $CONFIRM && warning_box << !
Cannot find "$DIR/ingres/log/$OLD_NAME". Skipping file.
!
	    WARNINGS=1
	fi
    done

#   Create list of files from other nodes to destroy
    if [ -n "$OTHERNODES" ]
    then
	for node in $OTHERNODES
	do
	    NUM_PARTS=`iigetres ii.$node.rcp.log.log_file_parts`
	    [ -z "$NUM_PARTS" -o \
	      -n "`echo $NUM_PARTS | sed 's/[1-9][0-9]*//'`" ] &&
		continue

	    PRIMARY_LOG_NAME=`iigetres ii.$node.rcp.log.log_file_name`
	    [ -z "$PRIMARY_LOG_NAME" ] && PRIMARY_LOG_NAME='ingres_log'
	    DUAL_LOG_NAME=`iigetres ii.$node.rcp.log.dual_log_name`
	    
	    PART=0
	    while [ $PART -lt $NUM_PARTS ]
	    do
		PART=`expr $PART + 1`
		DIR=`iigetres ii.$node.rcp.log.log_file_$PART`
		[ -n "$DIR" ] || continue
		OLD_NAME=`gen_log_name $PRIMARY_LOG_NAME $PART $node`
		[ -f $DIR/ingres/log/$OLD_NAME ] &&
		    echo "$DIR/ingres/log/$OLD_NAME" >> \
		     /tmp/$SELF.files.$$
		
		[ -z "$DUAL_LOG_NAME" ] && continue

		DIR=`iigetres ii.$node.rcp.log.dual_log_$PART`
		[ -n "$DIR" ] ||
		{
		    [ $PART -eq 1 ] && DUAL_LOG_NAME=""
		    continue;
		}

		OLD_NAME=`gen_log_name $DUAL_LOG_NAME $PART $node`

		[ -f $DIR/ingres/log/$OLD_NAME ] &&
		    echo "$DIR/ingres/log/$OLD_NAME" >> \
		     /tmp/$SELF.files.$$
	    done
	done
    fi

#   Strip out all reference to other nodes from configuration.
    convertconfig config.dat
    convertconfig protect.dat

#   Modify symbol.tbl
    $FAKEIT || ingunset II_CLUSTER
    [ -n "$OTHERNODES" ] &&
    {
	for node in $OTHERNODES
	do
	    ucasenode="`echo $node | tr '[a-z]' '[A-Z]'`"
	    $FAKEIT || ingunset "II_GCN${II_INSTALLATION}_${ucasenode}_PORT"
	done
    }
    $FAKEIT || ingsetenv "II_GCN${II_INSTALLATION}_LCL_VNODE" `uname -n`
    $FAKEIT || ingunset \
     "II_GCN${II_INSTALLATION}_`echo $CONFIG_HOST | tr '[a-z]' '[A-Z'`_PORT" 
    rename_file $II_SYSTEM/ingres/admin/$CONFIG_HOST/symbol.tbl \
	 $II_CONFIG/symbol.tbl

#   Reset certain entries for surviving node.
    $FAKEIT || iisetres "ii.$CONFIG_HOST.config.cluster_mode" 'OFF'
    $FAKEIT || iisetres 'ii.*.config.server_host' $CONFIG_HOST

#   Force DMCM off for all classes, JIC
    grep -i "\.dmcm:" $II_CONFIG/config.dat | awk -F ':' '{ print $1,$2 }' | \
    while read PARAM VALUE
    do
	[ "X$VALUE" = 'XOFF' ] || iisetres "$PARAM" 'OFF'
    done

#   Restore the document link if the document was installed
    VERSNUM="`cat $II_SYSTEM/ingres/version.rel | awk '{print $2}'`"
    if [ -d /usr/share/doc/ca-ingres-$VERSNUM ]; then
        ln -s /usr/share/doc/ca-ingres-$VERSNUM $II_SYSTEM/ingres/doc
    fi

#   Ingres configuration change is now complete.  Any failure
#   from here on will NOT abort the conversion.
    trap ":" 0 1 2 3 15

#   Destroy files marked for death.
    [ -f /tmp/$SELF.files.$$ ] && cat /tmp/$SELF.files.$$ | while read filename
    do
	remove_file $filename
    done

    $FAKEIT || 
    {
	# Put back an empty "memory" directory.
    	mkdir $II_SYSTEM/ingres/files/memory
	chmod 755 $II_SYSTEM/ingres/files/memory
    }

    cleanup

    $CONFIRM &&
    {
	if [ -n "$WARNINGS" ]
	then
	    warning_box << !
During conversion, $SELF detected one or more configuration inconsistencies.  Please resolve the reported problems before using Ingres.
!
	else
	    wrap_lines << !

Ingres has now been converted to a non-cluster configuration.

!
	fi
    }

    unlock_config_dat
    exit 0
