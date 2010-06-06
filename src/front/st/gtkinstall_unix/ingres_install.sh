:
#
# Copyright (c) 2006 Ingres Corporation
#
# Name: ingres_install.sh -- startup script for Ingres GUI installer on
#			     Linux
#
# Usage:
#	ingres_install
#
# Description:
#	This script is run by users to install Ingres. It is a wrapper script
#	for the Ingres GUI installer which needs to be run as root.
#
# Exit Status:
#	0	Success
#	1	Failure
#
## History:
##	26-Sep-2006 (hanje04)
##	    SIR 116877
##	    Created.
##	13-Nov-2006 (hanje04)
##	    Create dummy lib directory with links to RPM libraries before
##	    attempting to start the GUI. This is currently the only way
##	    to get the installer to work against versions of RPM other than
##	    then one it was built against. (Currently 4.1)
##	    Also remove use of GUI su tools has setting LD_LIBRARY_PATH
##	    for them is proving problematic.
##	16-Nov-2006 (hanje04)
##	    SD 111979
##	    If we're root we don't need to SU.
##	01-Dec-2006 (hanje04)
##	    SIR 116877
##	    Echo note to user that we are searching RPM database as this
##	    can take a while.
##	    Make script executable from anywhere.
##	    Remove "inginstgui" from error message.
##	22-Dec-2006 (hanje04)
##	    Fix up library locations to work on x86_64 Linux.
##	01-Mar-2007 (hanje04)
##	    BUG 117812
##	    Only print exit message on error. Clean exit doesn't
##	    always been we've installed.
##	10-Aug-2009 (hanje04)
##	    BUG 122571
##	    RPM saveset and database probing is now done by a separate python
##	    script. Remove RPM library hack, and fix up execution to run 
##	    everything in the right order.
##	10-Dec-2009 (hanje04)
##	    BUG 122944
##	    Make sure license is displayed and accepted before launching
##	    installer.
##	20-May-2010 (hanje04)
##	    SIR 123791
##	    License acceptance now part of GUI, no need to run ingres-LICENSE
##	    Leave in check for LICENSE file though.


# useful variables
unamep=`uname -p`
self=`basename $0`
instloc=`dirname $0`
instexe=${instloc}/bin/inginstgui
inglic=${instloc}/LICENSE
pkgqry=${instloc}/bin/ingpkgqry
pkginfo=/tmp/ingpkg$$.xml
uid=`id -u`

# set umask
umask 022

#
# localsu() - wrapper function for calling /bin/su if one of the GUI
#		equivalents cannot be found.
#
localsu()
{
    ROOTMSG="Installation of Ingres requires 'root' access.
Enter the password for the 'root' account to continue"

    cat << EOF
$ROOTMSG

EOF
    su -c "${1}" root

    return $?
}

#
# noexe() - error handler for missing executables
#
noexe()
{
    exe=$1
    cat << EOF
Error: $self

Cannot locate the Ingres installer executable:

	${instexe}

EOF
    exit 1
}

#
# nopy() - error handler for missing executables
#
checkpy()
{
    # python exits?
    pyvers=`python -V 2>&1`
    [ $? != 0 ] || [ -z "$pyvers" ] &&
    {
	cat << EOF
Error:
Cannot locate 'python' executable which is required by this program.
Please check your \$PATH and try again.

EOF
	 return 1
    }

    # right version ?
    majvers=`echo $pyvers | cut -d' ' -f2 | cut -d. -f1`
    minvers=`echo $pyvers | cut -d' ' -f2 | cut -d. -f2`
    if [ $majvers -lt 2 ] || [ $minvers -lt 3 ] ; then
	cat << EOF
Error:
This program requires "Python" version 2.3 or higher. Only:

    ${pyvers}

was found.

EOF
	return 1
    else
	return 0
    fi
}

#
# clean_exit - Generic exit routine to make sure everything is cleaned up
# 		when we're done
clean_exit()
{
   rc=${1?-1}
   rm -f $pkginfo
   exit $rc
}
#
# Main - body of program
#

# su executable
# FIXME! Only use localsu for now, GUI su tools are causing problems
#
# if [ -x /opt/gnome/bin/gnomesu ]
# then
#     sucmd="exec /opt/gnome/bin/gnomesu"
# elif [ -x /opt/kde3/bin/kdesu ]
# then
#     sucmd="exec /opt/kde3/bin/kdesu"
# else
    sucmd=localsu
# fi
echo "Ingres Installer"
echo

# Sanity checking
checkpy || exit 1
if [ ! -x ${instexe} ]
then
    noexe ${instexe}
elif [ ! -x ${pkgqry} ]
then
    noexe ${pkgqry}
fi

# check license, bail if it's not there
[ -r ${inglic} ] ||
{
    cat << EOF
ERROR: Cannot locate LICENSE file. For further assistance contact
Ingres Corporation Technical Support at http://servicedesk.ingres.com

EOF
exit 3
}

# Gather saveset and install info
${pkgqry} -s ${instloc} -o ${pkginfo}
rc=$?
if [ $rc != 0 ]
then
    cat << EOF
An error occurred querying the installation package information. Please
check and try again. For further assistance contact Ingres Corporation
Technical Support at http://servicedesk.ingres.com

EOF
    exit 2
fi

# Launch installer as root, using sucmd if we need to
if [ $uid != 0 ]
then
    ${sucmd} "${instexe} ${pkginfo}"
    rc=$?
else
    ${instexe} ${pkginfo}
    rc=$?
fi

if [ ${rc} = 0 ]
then
    cat << EOF

Done.
EOF
else
    cat << EOF

Installation failed with error code ${rc}
EOF
fi

clean_exit ${rc}

