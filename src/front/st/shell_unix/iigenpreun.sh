:
#
## Copyright Ingres Corporation 2007, 2010. All Rights Reserved.
##
##
##
##  Name: iigenpreun -- pre removal script for RPM/DEB packaging
##
##  Usage:
##       iigenpreun [ -r release name ] [ -v version ] [ -p package ] [ -u ]
##          -u upgrade mode
##
##  Description:
##      This is a generic pre-removal script, only to be called from
##      DEB or RPM packages
##
##  Exit Status:
##       0       setup procedure completed.
##       1       setup procedure did not complete.
##
##  DEST = utility
### History:
##      12-Jun-2007 (hanje04)
##	    SIR 118420
##          Created from SPEC file templates.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	10-Mar-2010 (hanje04)
##	    SIR 123296
##	    Set upgrade=false as default
##	    Run package removals as one script
##	    Correct rccmd for LSB builds
##	02-Jun-2010 (hanje04)
##	    BUG 123856
##	    Use su if we can't find runuser

(LSBENV)

if [ x"$conf_LSB_BUILD" != x"TRUE" ] ; then
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
fi

rc=0 # initialize return code
upgrade=false

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
            shift ; shift
            ;;
        -s)
            suscr=$2
            shift ; shift
            ;;
        -p)
            pkgname=$2
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
[ -z "$relname" ] || [ -z "$vers" ] || [ -z "$suscr" ] || [ -z "$pkgname" ] && 
{
    echo "Missing argument"
    exit 1
}

# Mesg cmds.
ECHO=true
CAT=true
inst_log=""

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

# Get command for user switching, user runuser if its there
if [ -x /sbin/runuser ] 
then
    runuser=/sbin/runuser
else
    runuser=/bin/su
fi
export runuser
 
    if [ -x $II_SYSTEM/ingres/utility/iisysdep ] ; then
	. iishlib
	. iisysdep
	inst_id=`ingprenv II_INSTALLATION`
	rcfile=$ETCRCFILES/ingres${inst_id}
	if which invoke-rc.d > /dev/null 2>&1 ; then
	    rccmd="invoke-rc.d ingres${inst_id}"
	elif [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
	    rcfile=$ETCRCFILES/ingres
	    rccmd="service ingres"
	else
	    rccmd=$rcfile
	fi
	inst_log="2>&1 | cat >> $ING_LOG/install.log"

	#Check for install userid, groupid
	CONFIG_HOST=`iipmhost`
	II_USERID=`iigetres ii.${CONFIG_HOST}.setup.owner.user`
	II_GROUPID=`iigetres ii.${CONFIG_HOST}.setup.owner.group`
	export II_USERID II_GROUPID

	#Check to see if instance is running and try to shut it down
	#Abort the install is we can't
	[ -x $rcfile ] && [ "$II_USERID" ] &&
	{
	    eval $rccmd stop $inst_log
	    if [ $? != 0 ] ; then
		rc=2
		cat << EOF
${relname} installation $inst_id is running and could not be cleanly shutdown.
EOF
		exit $rc
	    fi
	}


    fi

if $upgrade ; then
    # Script should not be run on upgrade as nothing is actually
    # being removed.
    exit 0;
else
    # Remove config
    rmscript=/tmp/ingrm$$.sh
    rm -f $rmscript
    ifssave=$IFS
    IFS=:
    for script in ${suscr}
    do  
	[ -x "$II_SYSTEM/ingres/utility/${script}" ] &&
	    echo "${script} -rmpkg" >> $rmscript
    done 
    IFS=$ifssave

    $runuser -m -c "sh ${rmscript}" $II_USERID $inst_log
    rm -f $rmscript
    $ECHO "Removing ${relname} ${pkgname}..."
    sleep 5

    exit $rc

fi

