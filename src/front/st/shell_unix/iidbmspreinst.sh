:
#
## Copyright Ingres Corporation 2007, 2010. All Rights Reserved.
##
##
##
##  Name: iidbmspreinst -- pre installation setup script for RPM/DEB DBMS
##			   package
##  Usage:
##       iidbmspreinst [ -r release name ] [ -v version ] [ -u ]
##	    -u upgrade mode
##
##  Description:
##      This is a pre-install script to only to be called from the DBMS
##	DEB or RPM package
##
##  Exit Status:
##       0       setup procedure completed.
##       1       setup procedure did not complete.
##
##  DEST = utility
### History:
##	12-Jun-2007 (hanje04)
##	    SIR 118420
##	    Created.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds

(LSBENV)

. iishlib
. iisysdep

parse_response()
{

# Read response file and set environment variables accordingly
    [ "$II_RESPONSE_FILE" ] && [ ! -f "$II_RESPONSE_FILE" ] &&
    {
	rc=1
	cat << !
Cannot locate response file.

        II_RESPONSE_FILE=$II_RESPONSE_FILE
!
	exit $rc
    }

    [ "$II_RESPONSE_FILE" ] &&
    {

	# Check response file is readable by specified installation owner.
	# If not abort the install before it starts.
	if runuser -m -c "! test -r $II_RESPONSE_FILE" $II_USERID ; then
            rc=2
            cat << !
Response file is not readable by user $II_USERID

        II_RESPONSE_FILE=$II_RESPONSE_FILE

If user $II_USERID does not exists, the response file should be
globally readable.
!
            exit $rc
	fi

	for var in `cut -s -d= -f1 $II_RESPONSE_FILE | grep -v ^#`
	do
            export ${var}=`iiread_response ${var} $II_RESPONSE_FILE`
	done
    }
}

check_location()
{
    # $1 is location path i.e /path/to location
    # $2 is location type i.e II_DATABASE, II_CHECKPOINT, etc

    # Check we were actuall passed a location, check it exitst and
    # if we can write to it
    [ -z "$1" ] && exit 3
    if [ ! -d "$1"  ] ; then
        rc=4
        cat << EOF
The $2 location:

        $1

does not exist please create and try again.
EOF
        exit $rc
    else
# If location is not the same as $II_SYSTEM, check it's writable,
# by Ingres install userid. If not, return an error
        [ ! "`dirname $1/.`" = "`dirname $II_SYSTEM/.`" ] && \
            runuser -m -c "! test -w $1/." $II_USERID && \
                return 1
    fi

    return 0
}

check_diskspace()
{
rc=0
[ "$II_LOG_FILE_SIZE_MB" ] && \
        LOG_KBYTES=`eval expr "$II_LOG_FILE_SIZE_MB \* 1024"`
[ "$LOG_KBYTES" ] || LOG_KBYTES=262144
log_need=$LOG_KBYTES
iisys_dev=`df -k $II_SYSTEM | tail -1 | awk '{print $NF}'`
[ "$II_DATABASE" ] && data_dev=`df -k $II_DATABASE  |\
                        tail -1 | awk '{print $NF}'`
[ "$II_CHECKPOINT" ] && ckp_dev=`df -k $II_CHECKPOINT |\
                        tail -1 | awk '{print $NF}'`
[ "$II_LOG_FILE" ] && log_dev=`df -k $II_LOG_FILE |\
                        tail -1 | awk '{print $NF}'`
[ "$II_DUAL_LOG" ] && dual_dev=`df -k $II_DUAL_LOG |\
                        tail -1 | awk '{print $NF}'`




# Disk space need in each location in KB
    iisys_need=0
    data_need=30000
    ckp_need=30000
    [ "$II_DUAL_LOG" ] && dual_need=$log_need

# Check each location, and set disk space needs for each device
# accordingly

# II_DATABASE
    if [ ! "$data_dev" ] || [ "$data_dev" = "$iisys_dev" ] ; then
        iisys_need=`eval expr $iisys_need + $data_need`
        data_loc=iisys
        data_dev=''
    else
        data_loc=data
    fi

# II_CHECKPOINT
    if [ ! "$ckp_dev" ] || [ "$ckp_dev" = "$iisys_dev" ] ; then
        iisys_need=`eval expr $iisys_need + $ckp_need`
        ckp_loc=iisys
        ckp_dev=''
    else
        case "$ckp_dev" in
            "$data_dev") ckp_loc=data
                         ckp_dev=''
                         data_need=`eval expr $data_need + $ckp_need`
                         ;;
                      *) ckp_loc=ckp
                         ;;
        esac
    fi

# II_LOG_FILE
    if [ ! "$log_dev" ] || [ "$log_dev" = "$iisys_dev" ] ; then
        iisys_need=`eval expr $iisys_need + $log_need`
        log_loc=iisys
        log_dev=''
    else
        case "$log_dev" in
               "$data_dev") log_loc=data
                            log_dev=''
                            data_need=`eval expr $data_need + $log_need`
                            ;;
                "$ckp_dev") log_loc=ckp
                            log_dev=''
                            ckp_need=`eval expr $ckp_need + $log_need`
                            ;;
                         *) log_loc=log
                            ;;
        esac
    fi

# II_DUAL_LOG
    if [ "$dual_dev" = "$iisys_dev" ] ; then
        iisys_need=`eval expr $iisys_need + $dual_need`
        dual_loc=iisys
        dual_dev=''
    elif [ "$dual_dev" ] ; then
        case "$dual_dev" in
               "$data_dev") dual_loc=data
                            dual_dev=''
                            data_need=`eval expr $data_need + $dual_need`
                            ;;
                "$ckp_dev") dual_loc=ckp
                            dual_dev=''
                            ckp_need=`eval expr $ckp_need + $dual_need`
                            ;;
                "$log_dev") dual_log=log
                            log_need=`eval expr $log_need + $dual_need`
                            dual_dev=''
                            ;;
                         *) dual_log=dual
                    ;;
        esac

    fi

# Check the disk space on each of the devices
    [ "$iisys_dev" ] &&
    {
        free=`iidsfree $II_SYSTEM`
        [ $free -lt $iisys_need ] &&
        {
            cat << EOF
ERROR:
II_SYSTEM location:

        $II_SYSTEM

has only ${free}KB of free space
${iisys_need}KB is required to install this package
EOF

            rc=5
        }
    }

    [ "$data_dev" ] &&
    {
        free=`iidsfree $II_DATABASE`
        [ $free -lt $data_need ] &&
        {
            cat << EOF
ERROR:
II_DATABASE location:

        $II_DATABASE

has only ${free}KB of free space
${data_need}KB is required to install this package
EOF

            rc=6
        }
    }

    [ "$ckp_dev" ] &&
    {
        free=`iidsfree $II_CHECKPOINT`
        [ $free -lt $ckp_need ] &&
        {
            cat << EOF
ERROR:
II_CHECKPOINT location:

        $II_CHECKPOINT

has only ${free}KB of free space
${ckp_need}KB is required to install this package
EOF

            rc=7
        }
    }

    [ "$log_dev" ] &&
    {
        free=`iidsfree $II_LOG_FILE`
        [ $free -lt $log_need ] &&
        {
            cat << EOF
ERROR:
II_LOG_FILE location:

        $II_LOG_FILE

has only ${free}KB of free space
${log_need}KB is required to install this package
EOF

            rc=8
        }
    }

    [ "$dual_dev" ] &&
    {
        free=`iidsfree $II_DUAL_LOG`
        [ $free -lt $dual_need ] &&
        {
            cat << EOF
ERROR:
II_DUAL_LOG location:

        $II_DUAL_LOG

has only ${free}KB of free space
${dual_need}KB is required to install this package
EOF

            rc=9
        }
    }

    [ $rc = 0 ] || exit $rc
}

# === main body of script starts here
## Need II_SYSTEM to be set
[ -z "$II_SYSTEM" ] &&
{
    echo "II_SYSTEM is not set"
    exit 1
}

PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
export II_SYSTEM PATH LD_LIBRARY_PATH
unset BASH_ENV
relname=""
vers=""
upgrade=false
rc=0 # initialize return code

# Parse arguments
while [ "$1" ]
do
    case "$1" in
	-r)
	    relname=$2
	    shift ; shift
	    ;;
	-v)
	    vers=$2
	    majvers=`echo $vers | cut -d- -f1`
	    minvers=`echo $vers | cut -d- -f2`
	    shift ; shift
	    ;;
	-u)
	    upgrade=true
	    shift
	    ;;
	 *)
	    exit 1
	    ;;
    esac
done

# need version and release name
[ -z "$relname" ] || [ -z "$vers" ] && exit 1

# Mesg cmds.
ECHO=true
CAT=true

# Keep silent unless explicitly told not to
[ -x $II_SYSTEM/ingres/utility/iiread_response ] && \
	[ -r "$II_RESPONSE_FILE" ] &&
{
    silent=`iiread_response SILENT_INSTALL $II_RESPONSE_FILE`
    case "$silent" in
	[Oo][No]|\
	[Yy][Ee][Ss])
	    ECHO=echo
	    CAT=cat
		;;
    esac
}
export ECHO CAT

if [ ! -x $II_SYSTEM/ingres/utility/iipmhost -o \
     ! -x $II_SYSTEM/ingres/utility/iigetres ] ; then
    echo "${relname} base package files are missing"
    echo "Aborting installation..."
    exit 1
fi

#Check for install userid, groupid
CONFIG_HOST=`iipmhost`
[ "$CONFIG_HOST" ] && 
{
    II_USERID=`iigetres ii.${CONFIG_HOST}.setup.owner.user`
    II_GROUPID=`iigetres ii.${CONFIG_HOST}.setup.owner.group`
    export II_USERID II_GROUPID
}

if [ ! "$II_USERID" -o ! "$II_GROUPID" ] ; then
    echo "${relname} base package install was invalid"
    echo "Aborting installation..."
    exit 1
fi

inst_id=`ingprenv II_INSTALLATION`
rcfile=$ETCRCFILES/ingres${inst_id}
if which invoke-rc.d > /dev/null 2>&1 ; then
    rccmd="invoke-rc.d ingres${inst_id}"
else
    rccmd=$rcfile
fi
inst_log="2>&1 | cat >> $II_LOG/install.log"

#Check to see if instance is running and try to shut it down
#Abort the install is we can't
[ -x $rcfile ] && [ -f $II_SYSTEM/ingres/files/config.dat ] && 
{
    eval ${rccmd} status >> /dev/null 2>&1
    if [ $? = 0 ]
    then
	eval ${rccmd} stop $inst_log
	if [ $? != 0 ] ; then
	    rc=2
	    cat << EOF 
${relname} installation $inst_id is running and could not be cleanly shutdown.
Aborting installation...
EOF
	    exit $rc
	fi
    fi
}

# If we're upgrading we don't need to check anything else so exit here
# passed this point, we're dealing with a new installation
$upgrade && exit 0

# Parse response file and check Data (etc.) locations exist
# and have correct permissions
parse_response
[ "$II_DATABASE" ] &&
{
    check_location "$II_DATABASE" II_DATABASE
     if [ $? != 0 ] ; then
            rc=3
            cat << EOF
The following location:

        II_DATABASE=$II_DATABASE

is not writable by user '$II_USERID'
Aborting installation...
EOF
            exit $rc
        fi
}

# Checkpoint (backup) location
[ "$II_CHECKPOINT" ] &&
{
    check_location "$II_CHECKPOINT" II_CHECKPOINT
    if [ $? != 0 ] ; then
            rc=3
            cat << EOF
The following location:

        II_CHECKPOINT=$II_CHECKPOINT

is not writable by user '$II_USERID'
Aborting installation...
EOF
            exit $rc
        fi
}

# Journal location
[ "$II_JOURNAL" ] &&
{
    check_location "$II_JOURNAL" II_JOURNAL
    if [ $? != 0 ] ; then
            rc=3
            cat << EOF
The following location:

        II_JOURNAL=$II_JOURNAL

is not writable by user '$II_USERID'
Aborting installation...
EOF
            exit $rc
        fi
}

# Temporary work location
[ "$II_WORK" ] &&
{
    check_location "$II_WORK" II_WORK
    if [ $? != 0 ] ; then
            rc=3
            cat << EOF
The following location:

        II_WORK=$II_WORK

is not writable by user '$II_USERID'
Aborting installation...
EOF
            exit $rc
        fi
}

# On-line checkpoint dump location
[ "$II_DUMP" ] &&
{
    check_location "$II_DUMP" II_DUMP
    if [ $? != 0 ] ; then
            rc=3
            cat << EOF
The following location:

        II_DUMP=$II_DUMP

is not writable by user '$II_USERID'
Aborting installation...
EOF
            exit $rc
        fi
}

# Transaction log
[ "$II_LOG_FILE" ] &&
{
    check_location "$II_LOG_FILE" II_LOG_FILE
    if [ $? != 0 ] ; then
            rc=3
            cat << EOF
The following location:

        II_LOG_FILE=$II_LOG_FILE

is not writable by user '$II_USERID'
Aborting installation...
EOF
            exit $rc
        fi
}

# Dual Tx log
[ "$II_DUAL_LOG" ] &&
{
    check_location "$II_DUAL_LOG" II_DUAL_LOG
    if [ $? != 0 ] ; then
            rc=3
            cat << EOF
The following location:

        II_DUAL_LOG=$II_DUAL_LOG

is not writable by user '$II_USERID'
Aborting installation...
EOF
            exit $rc
        fi
}

# Check we have enough space for install
# df can fail in chroot only check if df is working
if (df >& /dev/null) ; then
    check_diskspace
    rc=$?
else
    rc=0
fi


exit $rc

