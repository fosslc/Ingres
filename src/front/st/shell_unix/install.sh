:
#
#  Copyright (c) 2005, 2008 Ingres Corporation 
#
#  Name: install.sh -- Single point install script for Ingres
#
#  Usage:
#       install.sh 
#
#  Description:
#	Essentially a boot strap wrapper script for running ingbuild.
#	Prompts user for Installation info:
#		II_SYSTEM
#		USER
#		II_DISTRIBUTION
#
#  Exit Status:
#	0	installation competed successfully
#	1	Invalid II_SYSTEM given
#	2	user doesn't exist on system
#	3	Can't find tar archive or binary
#	4	Not run as root
#	5	Failed to create II_SYSTEM
#	6	ingbuild returned an error
#
## History:
##	20-July-2005 (bonro01)
##	    Initial version.
##	30-Sep-2005 (bonro01)
##	    Remove invalid output line.
##	05-Jan-2006 (bonro01)
##	    Change default install location to /opt/Ingres/IngresII
##	7-Apr-2006 (bonro01)
##	    Rename to Ingres 2006
##	27-Jul-2006 (bonro01)
##	    Only start Ingres if the install setup completes
##	24-Aug-2006 (bonro01)
##	    Remove 'which' command, that is not available in the
##	    default UnixWare command path.
##	22-Sep-2006 (hweho01)
##	    Remove "\c" in the line for user input prompts, since it
##          is not interpreted as a format control character in 
##          bash shell which is used for Linux platforms.
##	18-Dec-2006 (bonro01)
##	    Fix filesystem authorization checking to force use of bourne
##	    shell because csh shell does not recognize the test syntax
##	    especially on UnixWare. This change prevents the userid's
##	    default shell from being used because test could fail under csh.
##       2-Mar-2007 (hanal04) Bug 117589
##          The user may not have write priviledges to $II_SYSTEM.
##          Use $II_SYSTEM/ingres for .ingXXcsh and .ingXXsh instead.
##       31-May-2007 (hweho01)
##          Need to modify the platform specific library path for   
##          Tru64, UnixWare, AMD and Itanium Linux platforms in the
##          generated script file. 
##	7-Mar-2008 (bonro01)
##	    Some Linux distributions like Mandriva have TMPDIR set to
##	    ~root/tmp which is inaccessible to the Ingres administrator
##	    userid, so this is set to /tmp
##       23-Jul-2008 (hweho01)
##          Update product name from "Ingres 2006" to "Ingres" for 9.2 release,
##          use (PROD1NAME) which will be substituted by jam during build. 
##	15-Oct-2008 (bonro01)
##	    Install fails on Ubuntu during userid check if user login
##	    script returns a non-zero return code.
##	02-Jun-2009 (hanje04)
##	    Add support for OSX
##	23-Sep-2010 (hweho01)
##	    Add checking for file existences, such as ingsetenv or  
##          generated setup script file before executing or copying. 
##          Avoid "No such file or directory" error. 
#-------------------------------------------------------------------

#
# Multi-platform whoami
#
iiwhoami()
{
if [ -f /usr/ucb/whoami ]; then
	/usr/ucb/whoami
elif [ -f /usr/bin/whoami ]; then
	/usr/bin/whoami
elif [ -f /usr/bin/id ]; then
	IFS="()"
	set - `/usr/bin/id`
	echo $2
elif [ -f /bin/id ]; then
	IFS="()"
	set - `/bin/id`
	echo $2
else
	touch /tmp/who$$
	set - `ls -l /tmp/who$$`
	echo $3
	rm -f /tmp/who$$
fi
}

usage()
{
    cat << EOF
usage:
$self [-d Install dir] [-u userid] [ -s saveset ]

	Install dir	- Full path to location where $prod_name is to be installed
			  (II_SYSTEM)
	userid		- User to install as. Must exist on the system.
	saveset		- path to and name of saveset to be installed.
			  May be absolute or relative
EOF
}

errhldr()
{
    cat << EOF
ERROR:
An error occurred during the installation this product. 

EOF
}

#
# Generate Environment setup scripts
#
genenv()
{
    # Pick up term type from environment
    [ "$TERM_INGRES" ] || TERM_INGRES=vt100fx

    unames=`uname -s`
    unamem=`uname -m`
    case $unames in
	HP-UX)
	    case $unamem in
		ia64)
		    libpathname=SHLIB_PATH
		    libpathdir=$II_SYSTEM/ingres/lib/lp32
		    libpathname64=LD_LIBRARY_PATH
		    libpathdir64=$II_SYSTEM/ingres/lib
		    ;;
		*)
		    libpathname=SHLIB_PATH
		    libpathdir=$II_SYSTEM/ingres/lib
		    libpathname64=LD_LIBRARY_PATH
		    libpathdir64=$II_SYSTEM/ingres/lib/lp64
		    ;;
	    esac
	    ;;
	AIX)
	    libpathname=LIBPATH
	    libpathdir=$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp64
	    ;;
	OSF1|\
	UnixWare)
	    libpathname=LD_LIBRARY_PATH
	    libpathdir=$II_SYSTEM/ingres/lib
	    ;;
        Linux)
	    case $unamem in
                ia64|\
                x86_64)
	          libpathname=LD_LIBRARY_PATH
	          libpathdir=$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp32
                  ;; 
                *)
	          libpathname=LD_LIBRARY_PATH
	          libpathdir=$II_SYSTEM/ingres/lib
                  ;; 
            esac    
            ;;
        Darwin)
	    libpathname=DYLD_LIBRARY_PATH
            libpathdir=$II_SYSTEM/ingres/lib
	    if [ x"$TERM" == x ] ; then
		export TERM=xterm
	    fi
	    export TERM_INGRES=konsolel
	    ;;
	*)
	    libpathname=LD_LIBRARY_PATH
	    libpathdir=$II_SYSTEM/ingres/lib
	    libpathname64=LD_LIBRARY_PATH_64
	    libpathdir64=$II_SYSTEM/ingres/lib/lp64
	    ;;
    esac

    cat << ! > $II_SYSTEM/ingres/.ing${II_INSTALLATION}sh
# Ingres environment for $II_INSTALLATION installation
# Generated at installation time

TERM_INGRES=$TERM_INGRES
II_SYSTEM=$II_SYSTEM
PATH=\$II_SYSTEM/ingres/bin:\$II_SYSTEM/ingres/utility:\$PATH
if [ \$${libpathname} ] ; then
    ${libpathname}=${libpathdir}:\$${libpathname}
else
    ${libpathname}=/lib:/usr/lib:${libpathdir}
fi
export II_SYSTEM TERM_INGRES PATH ${libpathname}
!
    if [ "$libpathname64" ]; then
    cat << ! >> $II_SYSTEM/ingres/.ing${II_INSTALLATION}sh
if [ \$${libpathname64} ] ; then
    ${libpathname64}=${libpathdir64}:\$${libpathname64}
else
    ${libpathname64}=/lib:/usr/lib:${libpathdir64}
fi
export ${libpathname64}
!
    fi

    cat << ! > $II_SYSTEM/ingres/.ing${II_INSTALLATION}csh
# Ingres environment for $II_INSTALLATION installation
# Generated at installation time

setenv II_SYSTEM $II_SYSTEM
set path=(\$II_SYSTEM/ingres/{bin,utility} \$path)
if ( \$?${libpathname} ) then
    setenv ${libpathname} ${libpathdir}:\$${libpathname}
else
    setenv ${libpathname} /lib:/usr/lib:${libpathdir}
endif
setenv TERM_INGRES $TERM_INGRES
!
    if [ "$libpathname64" ]; then
    cat << ! >> $II_SYSTEM/ingres/.ing${II_INSTALLATION}csh
if ( \$?${libpathname64} ) then
    setenv ${libpathname64} ${libpathdir64}:\$${libpathname64}
else
    setenv ${libpathname64} /lib:/usr/lib:${libpathdir64}
endif
!
    fi

} # genenv

#
# Unix tar archive install routine 
#
install_unix()
{
if [ "$doinstall" != "true" ] # First time through just do the setup
then
    II_SYSTEM=$instloc
    export II_SYSTEM
    PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
    export PATH

# Need to make sure II_SYSTEM is readable by $II_USERID. If it isn't we
# abort the install.
    [ -d "$II_SYSTEM" ] &&
    {
	if su $userid -c "sh -c 'test ! -r \"$II_SYSTEM\" ' " ; then
	    rc=7
	    cat << !
Specified location for II_SYSTEM is not readable by user $userid

        II_SYSTEM=$II_SYSTEM

II_SYSTEM must be readable by user $userid for the installation to proceed.
!
	    exit $rc
	fi

	if su $userid -c "sh -c 'test ! -x \"$II_SYSTEM\" ' " ; then
	    rc=7
	    cat << !
User $userid does NOT have execute permission for the location specified
for II_SYSTEM.

        II_SYSTEM=$II_SYSTEM

User $userid must have execute permission in II_SYSTEM for installation
to continue.
!
	    exit $rc
	fi
    }

# Create II_SYSTEM
    echo "Creating $instloc..."
    umask 22
    mkdir -p $instloc/ingres && \
    chown $userid $instloc/ingres ||
    {
	    cat << EOF
Failed to create II_SYSTEM:

	$II_SYSTEM

EOF
	    exit 5
    }
   
# Call script again as ingres to perform the rest of the installation
    doinstall=true
    export instloc ingtar doinstall
    su $userid -c "sh $selfdir/$self" || exit $?

# Build password validation program if we can
    if [ -x $II_SYSTEM/ingres/bin/mkvalidpw ] ; then
	$II_SYSTEM/ingres/bin/mkvalidpw
    elif [ ! -x II_SYSTEM/ingres/bin/ingvalidpw ] ; then
	# Use the distributed version
	[ -f "$II_SYSTEM/ingres/files/iipwd/ingvalidpw.dis" ] && \
	    cp -f $II_SYSTEM/ingres/files/iipwd/ingvalidpw.dis \
		"$II_SYSTEM/ingres/bin/ingvalidpw"

	# Set II_SHADOW_PWD if we need to
	[ -f $II_SYSTEM/ingres/bin/ingsetenv -a \
	 \( -f /etc/shadow -o /etc/security/passwd \) ] && \
	    $II_SYSTEM/ingres/bin/ingsetenv II_SHADOW_PWD $II_SYSTEM/ingres/bin/ingvalidpw
    fi

    # If ingvalidpw exists then it needs to be owned by root
    # and have SUID set.
    [ -x $II_SYSTEM/ingres/bin/ingvalidpw ] && {
	chown root $II_SYSTEM/ingres/bin/ingvalidpw
	chmod 4755 $II_SYSTEM/ingres/bin/ingvalidpw
    }

    homedir=`awk -F: '$1 == "'$userid'" { print $6 }' /etc/passwd`
    [ -d "$homedir" ] && [ -w "$homedir" ] ||
    {
	cat << EOF
The home directory for the user ${userid}:

	$homedir

does not exist or is not writeable. The environment setup script for
this instance of $prod_name $prod_rel will be written to:

	$II_SYSTEM/ingres

EOF
	eval homedir=$II_SYSTEM/ingres
    }
# Copy it to home directory if we can
    [ "$homedir" != "$II_SYSTEM/ingres" ] && \
    {
	if ls $II_SYSTEM/ingres/.ing*sh >/dev/null 2>&1
	then
          su $userid -c "cp -f $II_SYSTEM/ingres/.ing*sh $homedir"
	fi
    }


else # dosetup=true 
# Setup environment
    II_SYSTEM=$instloc
    export II_SYSTEM
    PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
    export PATH
    unames=`uname -s`
    unamem=`uname -m`
    case $unames in
	HP-UX)
	    case $unamem in
		ia64)
		    if [ "$SHLIB_PATH" ] ; then
			SHLIB_PATH=$II_SYSTEM/ingres/lib/lp32:$SHLIB_PATH
		    else
			SHLIB_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib/lp32
		    fi
		    export SHLIB_PATH
		    if [ "$LD_LIBRARY_PATH" ] ; then
			LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib:$LD_LIBRARY_PATH
		    else
			LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib
		    fi
		    export LD_LIBRARY_PATH
		    ;;
		*)
		    if [ "$SHLIB_PATH" ] ; then
			SHLIB_PATH=$II_SYSTEM/ingres/lib:$SHLIB_PATH
		    else
			SHLIB_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
		    fi
		    export SHLIB_PATH
		    if [ "$LD_LIBRARY_PATH" ] ; then
			LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64:$LD_LIBRARY_PATH
		    else
			LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64
		    fi
		    export LD_LIBRARY_PATH
		    ;;
	    esac
	    ;;
	AIX)
	    if [ "$LIBPATH" ] ; then
		LIBPATH=$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp64:$LIBPATH
	    else
		LIBPATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp64
	    fi
	    export LIBPATH
	    ;;
        Darwin)
	    if [ "$DYLD_LIBRARY_PATH" ] ; then
		DYLD_LIBRARY_PATH=$II_SYSTEM/ingres/lib:$DYLD_LIBRARY_PATH
	    else
		DYLD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
	    fi
	    if [ x"$TERM" == x ] ; then
		export TERM=xterm
	    fi
	    export DYLD_LIBRARY_PATH
	    export TERM_INGRES=konsolel
	    ;;
	*)
	    if [ "$LD_LIBRARY_PATH" ] ; then
		LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib:$LD_LIBRARY_PATH
	    else
		LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
	    fi
		export LD_LIBRARY_PATH
	    if [ "$LD_LIBRARY_PATH_64" ] ; then
		LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64:$LD_LIBRARY_PATH_64
	    else
		LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64
	    fi
		export LD_LIBRARY_PATH_64
	    ;;
    esac

    II_DISTRIBUTION=$ingtar
    export II_DISTRIBUTION

# Echo config
    cat << EOF
II_SYSTEM: $instloc
Distribution: $ingtar
Installation owner: `iiwhoami`

EOF

#check to see if instance is running and abort if it is
    [ -x $II_SYSTEM/ingres/utility/csreport ] && csreport >> /dev/null
    if [ $? = 0 ]
    then
	rc=8
	echo "The $prod_name $prod_rel installation under $II_SYSTEM is running"
	echo "Aborting installation..."
	exit $rc
    fi

    # Some Linux distributions like Mandriva have TMPDIR set to ~root/tmp
    # which is inaccessible to the Ingres administrator userid, so this is set to /tmp
    if [ "$TMPDIR" ]; then
        TMPDIR=/tmp
        export TMPDIR
    fi

# Install the save set
    cd $II_SYSTEM/ingres >> /dev/null
    tar xf $ingtar install
    install/ingbuild
    installrc=$?

    if [ $installrc = 0 ] ; then
	[ -x "$II_SYSTEM/ingres/bin/ingprenv" ] && 
	II_INSTALLATION=`$II_SYSTEM/ingres/bin/ingprenv II_INSTALLATION` &&
	genenv

    ## Check for completed installation
	[ -f "$II_SYSTEM/ingres/files/config.dat" ] && 
	{
	    CONFIG_HOST=`iipmhost`
	    VERSION=`head -1 $II_SYSTEM/ingres/version.rel`
	    RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`
	    SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID`
	}

    ## Start up Ingres
	[ "$SETUP" = "complete" -a -x "$II_SYSTEM/ingres/utility/ingstart" ] && 
	{
	    cat << EOF
$prod_name $prod_rel has installed successfully. 
The instance will now be started...

EOF
	    ingstart
	}
    else
	cat << EOF
An error has occurred during the installation of $prod_name $prod_rel.

EOF
	exit 6
    fi
fi
    
} # install_unix

#
# Main body of script starts here
# Script is run as root to start with and then calls its self after
# su-ing to the specified user.
#
self=`basename $0`
selfdir=`dirname $0`
USER=`iiwhoami`
prod_name=(PROD1NAME)
prod_rel=ING_VERSION
definst=/opt/Ingres/IngresII
## Make sure unwanted II_SYSTEM value doesn't affect the installation
unset II_SYSTEM

# First time round
[ "$doinstall" != "true" ] &&
{
inst_id=II
instloc=
userid=
ingtar=

# Check user
if [ "$USER" != "root" ]; then
    cat << EOF
$self must be run as root

EOF
    exit 4
fi

# Check command line
    while [ $# != 0 ]
    do
	case "$1" in
	    # Installation location
	    -d) instloc=`echo $2 | grep '^/'`
		[ -z "$instloc" ] &&
		{
		    cat << EOF
"$2" is not a valid installation location

EOF
		    usage
		    exit 1
		}
	        shift ; shift
	        ;;
	    # Installation owner
	    -u) userid=$2
	        shift ; shift
	        ;;
	    # saveset
	    -s) ingtar=$2
	        shift ; shift	
	        ;;
	    *) usage
		exit 1
	        ;;
        esac
    done # while [ $# != 0 ]
 
# Startup screen
    cat << EOF
$prod_name $prod_rel

EOF
# Prompt user for values if we need them
# Installation location
    while [ -z "$instloc" ]
    do
	cat << EOF
Please choose a location in which to install $prod_name
EOF
        echo "(II_SYSTEM:default $definst): "
	read ans
	[ -z "$ans" ] && ans=$definst
	instloc=`echo $ans | grep '^/'`
	echo

	[ -z "$instloc" ] &&
	{
	    cat << EOF
"$ans" is not a valid installation location

EOF
	    usage
	    exit 1
        }
    done

# Installation owner
    [ -z "$userid" ] && 
    {
	cat << EOF
Please choose a user to install $prod_name as
EOF
	echo "(default ingres): "
	read ans
	[ -z "$ans" ] && ans=ingres
	userid=$ans
	echo
    }

    su "$userid" -c "exit 0" || 
    {
	cat << EOF
System user "$userid" must be created before the installation can proceed

EOF
	usage
	exit 2
    }

# savset
    [ -z "$ingtar" ] &&
    {
	if [ -f "$selfdir/ingres.tar" ]; then
	    ingtar=$selfdir/ingres.tar
	else
	    cat << EOF
Please enter the filename and location of the $prod_name 
saveset you wish to install
EOF
	    echo "(default ./ingres.tar): "
	    read ans
	    [ -z "$ans" ] && ans=ingres.tar
	    ingtar=$ans
	    echo
	fi
    }

    [ -r `pwd`/$ingtar ] && ingtar=`pwd`/$ingtar
    [ -r "$ingtar" ] ||
    {
	cat << EOF
Cannot locate the $prod_name saveset:

	$ingtar

EOF
	usage
	exit 3
    }
# Check tar and archive exists
    tar tf $ingtar > /dev/null 2>&1 ||
    {
	cat << EOF
Cannot locate tar command. Check path and try again

EOF
	exit 3
    }
}

case `uname -s` in
    Linux) reset="- 0"
	   ;;
        *) reset=0
	   ;;
esac

install_unix

trap $reset
exit 0
