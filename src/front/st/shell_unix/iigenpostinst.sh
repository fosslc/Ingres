:
#
## Copyright Ingres Corporation 2007, 2010. All Rights Reserved.
##
##
##
##  Name: iigenpostinst -- post installation setup script for RPM/DEB packaging
##
##  Usage:
##       iigenpostinst [ -r release name ] [ -v version ] [ -p package ] [ -u ]
##          -u upgrade mode
##
##  Description:
##      This is a generic post-install script to only to be called from
##      DEB or RPM packages or the RC service script
##
##  Exit Status:
##       0       setup procedure completed.
##       1       setup procedure did not complete.
##
##  DEST = utility
### History:
##      04-Jun-2007 (hanje04)
##	    SIR 118420
##          Created from SPEC file templates.
##	10-Jan-2010 (hanje04)
##	    Updated for setup to be done from rc script instead of RPM install

## Need II_SYSTEM to be set
[ -z "$II_SYSTEM" ] &&
{
    echo "II_SYSTEM is not set"
    exit 1
}

PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
export II_SYSTEM PATH LD_LIBRARY_PATH
relname=""
vers=""
pkg=""
upgrade=false
rc=0 # initialize return code
. iisysdep
. iishlib

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
[ -z "$relname" ] || [ -z "$vers" ] || [ -z "$suscr" ] && 
{
    echo "Missing argument"
    exit 1
}

# Mesg cmds.
ECHO=echo
CAT=cat
PRINTF=printf

# If we're not installing using a response file we
# won't be doing any setup so skip the re-install checks
[ -x $II_SYSTEM/ingres/utility/iiread_response ] && \
        [ -r "$II_RESPONSE_FILE" ] &&
{   
    silent=`iiread_response SILENT_INSTALL $II_RESPONSE_FILE`
    case "$silent" in
        [Oo][No]|\
        [Yy][Ee][Ss])
            ECHO=true
            CAT=true
	    PRINTF=true
                ;;
    esac
}
export ECHO CAT

# Use response file if one if provided
IISUFLAG="-rpm"
[ "$II_RESPONSE_FILE" ] && IISUFLAG="$IISUFLAG -exresponse $II_RESPONSE_FILE"
export IISUFLAG

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
    echo "Aborting setup..."
    exit 1
fi
[ "${pkgname}" ] && $PRINTF "\t%-65s" "Running setup for $pkgname..."
# Run setup
ifssave=$IFS
IFS=:
for script in ${suscr}
do
    [ -z "${pkgname}" ] && $PRINTF "\t%-65s" "Running ${script}..."
    runuser -m -c "${script} $IISUFLAG" $II_USERID
    rc=$?
    [ $rc != 0 ] && break
done
IFS=$ifssave

# check setup completed OK, iisu scripts don't error
if [ -r "/usr/share/ingres/version.rel" ] ; then
    relid=`head -1 /usr/share/ingres/version.rel | sed "s/[ ().\/]//g"`
else
    relid=`head -1 $II_SYSTEM/ingres/version.rel | sed "s/[ ().\/]//g"`
fi
if [ -z "$relid" ] || [ $rc != 0 ] || [ "${pkgname}" ] && 
    [ "`iigetres ii.${CONFIG_HOST}.config.${pkgname}.${relid}`" != "complete" ]
then
    $ECHO FAIL
    $ECHO "Setup failed for ${pkgname}"
    $ECHO "See $II_LOG/install.log for more info."
    rc=1
else
    $ECHO OK
fi

exit $rc

