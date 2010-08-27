:
#
## Copyright Ingres Corporation 2007, 2010. All Rights Reserved.
##
##
##
##  Name: iicorepostinst -- post installation setup script for RPM/DEB
##	 core package
##
##  Usage:
##       iicorepostinst [ -r release name ] [ -v version ]
##
##  Description:
##      This is a generic post-install script to only to be called from
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
##          Created.
##      22-Jul-2009 (frima01) SIR 122347
##          Remove lp32 path from .ingIIbash - now in
##          32bit support package.
##	20-Dec-2009 (hanje04)
##	    Replace su with sudo to allow installation into chroot
##	08-Mar-2010 (hanje04)
##	   SIR 123296
##	   Remove references to iisudo as it no longer exists. Note,
##	   this script is no longer used by the RPM install process
##	02-Jun-2010 (hanje04)
##	    BUG 123856
##	    Use su if we can't find runuser
##	16-Jun-2010 (hanje04)
##	    BUG 123929
##	    Run iisetres as $II_USERID not $II_GROUPID
##	25-Jun-2010 (hanje04)
##	    BUG 124005
##	    Run mkrc as $II_USERID so that the generated script is owned by
##	    $II_USERID and not root

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
upgrade=false
rc=0 # initialize return code
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
[ -z "$relname" ] || [ -z "$vers" ] && 
{
    echo "Missing argument"
    exit 1
}

#Silent install?
[ -r "$II_RESPONSE_FILE" ] && \
        silent=`iiread_response SILENT_INSTALL $II_RESPONSE_FILE`
[ "$silent" ] &&
{
    ECHO=true
    CAT=true
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
 
parse_response()
{

# Read response file and set environment variables accordingly
    [ "$II_RESPONSE_FILE" ] && [ ! -f "$II_RESPONSE_FILE" ] && {
    rc=1
    $CAT << !
Cannot locate response file.

        II_RESPONSE_FILE=$II_RESPONSE_FILE
!
    exit $rc
    }

    for var in `cut -s -d= -f1 $II_RESPONSE_FILE | grep -v ^#`
    do
        export ${var}=`iiread_response ${var} $II_RESPONSE_FILE`
    done
}

fix_owner()
{
    if $upgrade
    then
	CONFIG_HOST=`iipmhost`
	II_USERID=`iigetres "ii.${CONFIG_HOST}.setup.owner.user"`
	II_GROUPID=`iigetres "ii.${CONFIG_HOST}.setup.owner.group"`
    else
	#Check for install userid, groupid; default to ingres
	export II_USERID=${II_USERID:-ingres}
	export II_GROUPID=${II_GROUPID:-ingres}
    fi
    # correct dir ownership
    chown $II_USERID:$II_GROUPID ${II_SYSTEM}/ingres/.
    for dir in bin demo files ice install lib sig utility vdba
    do
        [ -d "${II_SYSTEM}/ingres/${dir}/." ] && \
            chown -R $II_USERID:$II_GROUPID ${II_SYSTEM}/ingres/${dir}/.
    done

    # and version.rel
    chown $II_USERID:$II_GROUPID ${II_SYSTEM}/ingres/version.rel
}

check_env()
{
    # Use response file if one if provided
    [ "$II_RESPONSE_FILE" ] && parse_response
    fix_owner
    IISUFLAG=-batch
    [ "$II_RESPONSE_FILE" ] && IISUFLAG="$IISUFLAG -exresponse $II_RESPONSE_FILE"
    export II_SUFLAG

    if ! $upgrade
    then
        $runuser -m -c "touch $II_SYSTEM/ingres/files/config.dat" $II_USERID

	#Check for user defined installation ID otherwise default to II
	[ -z "$II_INSTALLATION" ] && II_INSTALLATION=`ingprenv II_INSTALLATION`
	[ -z "$II_INSTALLATION" ] && II_INSTALLATION=II
	$runuser -m -c "$II_SYSTEM/ingres/bin/ingsetenv II_INSTALLATION $II_INSTALLATION" $II_USERID

	$CAT << !
II_INSTALLATION configured as $II_INSTALLATION.
!
        # Same for hostname, default to localhost
	[ -z "$II_HOSTNAME" ] && II_HOSTNAME=`ingprenv II_HOSTNAME`
	[ -z "$II_HOSTNAME" ] && II_HOSTNAME=localhost

	$runuser -m -c "$II_SYSTEM/ingres/bin/ingsetenv II_HOSTNAME $II_HOSTNAME" $II_USERID

        CONFIG_HOST=`iipmhost`
	$runuser -m -c "$II_SYSTEM/ingres/utility/iisetres \"ii.${CONFIG_HOST}.setup.owner.user\" $II_USERID" $II_USERID
	$runuser -m -c "$II_SYSTEM/ingres/utility/iisetres \"ii.${CONFIG_HOST}.setup.owner.group\" $II_GROUPID" $II_USERID
    fi

}

genenv()
{
    # Pick up term type from response file
    [ "$II_TERMINAL" ] && export TERM_INGRES=$II_TERMINAL
    [ "$TERM_INGRES" ] || TERM_INGRES=konsolel

    cat << ! > $II_SYSTEM/.ing${II_INSTALLATION}bash
# ${relname} environment for $II_INSTALLATION installation
# Generated at installation time, any changes made will be lost

export II_SYSTEM=$II_SYSTEM
export PATH=\$II_SYSTEM/ingres/bin:\$II_SYSTEM/ingres/utility:\$PATH
if [ "\$LD_LIBRARY_PATH" ] ; then
    LD_LIBRARY_PATH=/usr/local/lib:\$II_SYSTEM/ingres/lib:\$LD_LIBRARY_PATH
else
    LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib:\$II_SYSTEM/ingres/lib
fi
export LD_LIBRARY_PATH
export TERM_INGRES=$TERM_INGRES
!

    cat << ! > $II_SYSTEM/.ing${II_INSTALLATION}tcsh
# ${relname} environment for $II_INSTALLATION installation
# Generated at installation time, any changes made will be lost

setenv II_SYSTEM $II_SYSTEM
set path=(\$II_SYSTEM/ingres/{bin,utility} \$path)
if ( \$?LD_LIBRARY_PATH ) then
    setenv LD_LIBRARY_PATH /usr/local/lib:\$II_SYSTEM/ingres/lib:\$LD_LIBRARY_PATH
else
    setenv LD_LIBRARY_PATH /lib:/usr/lib:/usr/local/lib:\$II_SYSTEM/ingres/lib
endif
setenv TERM_INGRES $TERM_INGRES
!
    chown $II_USERID:$II_GROUPID $II_SYSTEM/.ing${II_INSTALLATION}*sh

} # genenv

do_setup()
{

    # If ingvalidpw exists then it needs to be owned by root
    # and have SUID set.
    [ -x $II_SYSTEM/ingres/bin/ingvalidpw ] && {
        chown root $II_SYSTEM/ingres/bin/ingvalidpw
        chmod 4755 $II_SYSTEM/ingres/bin/ingvalidpw
    }


    # Remove release.dat if it exists
    [ -f $II_SYSTEM/ingres/install/release.dat ] && \
        rm -f $II_SYSTEM/ingres/install/release.dat

    # Generate environment script
    genenv

    # Run setup
    $runuser -m -c "$II_SYSTEM/ingres/utility/iisutm $IISUFLAG || ( echo 'Setup of ${relname} Base Package failed.' && echo 'See $II_LOG/install.log for more info.' )" $II_USERID || rc=2

    eval homedir="~"$II_USERID
    [ -d "$homedir" ] && [ -w "$homedir" ] || eval homedir=$II_SYSTEM

    # Copy it to home directory if we can
    [ "$homedir" != "$II_SYSTEM" ] && \
	$runuser -m -c "cp -f $II_SYSTEM/.ing*sh $homedir" $II_USERID >> /dev/null 2>&1

    # Install startup scripts under /etc/rc.d
    [ -x $II_SYSTEM/ingres/utility/mkrc ] &&
    {
	inst_id=`ingprenv II_INSTALLATION`
	mkrc -r
	[ -f "$II_SYSTEM/ingres/files/rcfiles/ingres${inst_id}" ] &&
	    rm -f "$II_SYSTEM/ingres/files/rcfiles/ingres${inst_id}"
	( $runuser -m -c "mkrc" $II_USERID ) && mkrc -i
	( [ "$II_START_ON_BOOT" = "NO" ] || [ "$START_ON_BOOT" = "NO" ] ) &&
	{
	    if [ -x /usr/sbin/update-rc.d ] ; then
		/usr/sbin/update-rc.d -f ingres${inst_id} remove > \
							/dev/null 2>&1
	    else
		/sbin/chkconfig --del ingres${inst_id} > /dev/null 2>&1
	    fi
	}
	# create state dir
	mkdir -p /var/lib/ingres/${inst_id}
	chown $II_USERID:$II_GROUPID /var/lib/ingres/${inst_id}
	
    }
} # do_setup

check_env
do_setup

exit $rc

