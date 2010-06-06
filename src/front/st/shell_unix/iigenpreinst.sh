:
#
## Copyright Ingres Corporation 2007, 2010. All Rights Reserved.
##
##
##
##  Name: iigenpreinst -- pre installation setup script for RPM/DEB packaging
##
##  Usage:
##       iigenpreinst [ -r release name ] [ -v version ] [ -u ]
##	    -u upgrade mode
##
##  Description:
##      This is a generic pre-install script to only to be called from
##	DEB for RPM packages
##
##  Exit Status:
##       0       setup procedure completed.
##       1       setup procedure did not complete.
##
##  DEST = utility
## History:
##	01-Jun-2007 (hanje04)
##	    SIR 118420
##	    Created from SPEC file templates.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	02-Jun-2010 (hanje04)
##	    BUG 123856
##	    Use su if we can't find runuser

(LSBENV)

# override value of II_CONFIG set by ingbuild
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    ## Need II_SYSTEM to be set
    [ -z "$II_SYSTEM" ] &&
    {
        echo "II_SYSTEM is not set"
        exit 1
    }

    PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
    LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
    export II_SYSTEM PATH LD_LIBRARY_PATH
    II_CONFIG=$II_SYSTEM/ingres/files
    export II_CONFIG
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
    unset BASH_ENV
fi
export cfgloc symbloc

relname=""
vers=""
upgrade=false
. iisysdep
. iishlib


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

# Get command for user switching, user runuser if its there
if [ -x /sbin/runuser ] 
then
    runuser=/sbin/runuser
else
    runuser=/bin/su
fi
export runuser
 
#Check for install userid, groupid
CONFIG_HOST=`iipmhost`
[ "$CONFIG_HOST" ] && 
{
    II_USERID=`iigetres ii.${CONFIG_HOST}.setup.owner.user`
    II_GROUPID=`iigetres ii.${CONFIG_HOST}.setup.owner.group`
    export II_USERID II_GROUPID
}
if [ ! "$CONFIG_HOST" -o ! "$II_USERID" -o ! "$II_GROUPID" ] ; then
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
[ -x $rcfile ] && [ -f $cfglocconfig.dat ] && 
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

# Abort if response file is set but invalid
[ "$II_RESPONSE_FILE" ] && [ ! -f "$II_RESPONSE_FILE" ] && {
rc=3
cat << !
Cannot locate response file.

	II_RESPONSE_FILE=$II_RESPONSE_FILE
!
exit $rc
}

[ "$II_RESPONSE_FILE" ] && {

# Check response file is readable by specified installation owner.
# If not abort the install before it starts.
    if $runuser -m -c "test ! -r $II_RESPONSE_FILE" $II_USERID ; then
	rc=4
        cat << !
Response file is not readable by user $II_USERID

        II_RESPONSE_FILE=$II_RESPONSE_FILE

If user $II_USERID does not exists, the response file should be
globally readable.
!
        exit $rc
    fi
}

exit $rc

