#!/bin/bash
## Copyright (C) 2009  Ingres Corporation
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##
###########################################################################
##
## Filename: buildAndTest.sh
##
## Description:
## This script is intended to be a one step process to build/install/test an
## Ingres work area, using the sep test suites.
## In order to use the sep tests, the installation must be created and owned
## by the ingres user.  This script will chown all files to ingres, and by 
## calling other, task specific scripts, build, install, setup and run the sep
## tests.  When complete it will shutdown the running database, and chown the
## files back to the original owner.
##
## Due to the ownership changes and permission changes for test setup this
## script must be run as root.
##
## Supported platforms:
##	Linux.
##	OSX
##
## History:
## 	21-May-2009 (hanje04)
##	    Added license, fixed up for OSX
##	27-may-2009 (stephenb)
##		Add -noclean and -nobuild parameters and multi-parameter logic; 
##		expand usage function to explain usage
##		fix non-generic basename problem (it's not in /bin on Ubuntu)
##		default II_INSTALLATION to "T1" if not provided
##		add "SELF" so we can set buildtools from it if needed (this saves having to
##			cd to this directory to run the script)
## 	24-Mar-2010 (frima01)
##		Don't copy test directory, create sym link instead.
##		Have noclean depending on nobuild.
##		Removed function keyword - not supported on all systems.
##  08-Jun-2010 (thich01)
##     Added set -- to clear passed in arguments


usage()
{
    echo "Usage: buildAndTest.sh [-noclean] [-nobuild] [-testloc <location>] [<installation_code>]"
    echo "Where:"
    echo "	-nobuild - Do not build the code"
    echo "	-noclean - Do not delete existing build test and installation directories"
    echo "	-testloc <location> - Alternate location for tests (full dir path)"
    echo "	<installation_code> - Two letter installation code for test install (defaults to T1)"
    trap - 0
    exit 1
}

SELF=$0

echo "Checking effective userid...."
echo ""
export WHOAMI=`whoami`
if [ "$WHOAMI" != "root" ]
then
        echo "You must be user 'root' to run this script"
        echo ""
        exit 2
fi
# check parameters
while [ $# != 0 ]
do
	if [ "$1" = "-noclean" ] || [ "$1" = "-nc" ] ; then
		NOCLEAN=true
		shift
	elif [ "$1" = "-testloc" ] ; then
		shift
		if [ "$1" ] ; then
			TST_LOC=$1
			shift
		else
			echo "No location provided for -testloc"
			usage
		fi
	elif [ "$1" = "-nobuild" ] || [ "$1" = "-nb" ] ; then
		NOBUILD=true
		shift
	elif [ "$1" ] ; then
		# must be the installation ID
		case $1 in
    		[A-Z][A-Z,0-9])
        		II_INSTALLATION=$1
        		shift
        		;;
    		*)
        		echo "$1 is an invalid installation ID"
        		usage
        		;;
		esac
	else
		# bad parameter
		echo "Bad Parameter"
		usage
	fi
done


if [ -z "$II_INSTALLATION" ]
then
	II_INSTALLATION=T1
fi

export II_INSTALLATION

# we should set BUILDTOOLS so we know where the other tools are
PWD=`pwd`
if [ -z ${BUILDTOOLS} ]
then
  if [ -d ./buildtools ]
  then
    BUILDTOOLS="${PWD}/buildtools"
  elif [ `basename ${PWD}` == "buildtools" ]
  then
    BUILDTOOLS=${PWD}
  else
	BUILDTOOLS=`dirname ${SELF}`
  fi
fi
export BUILDTOOLS

# Clear $1 argument parms so that they are not passed
# to sourced scripts.
set --

if [ -f ${BUILDTOOLS}/set_env.sh ]
then
  . $BUILDTOOLS/set_env.sh || exit 1
else
  echo "Error, I cannot find the set_env.sh script"
  echo "This is required to use buildAndTest.sh"
  echo "Please run buildAndTest.sh from the root directory of the Ingres source"
  exit 1
fi

if [ -z $TST_LOC ]
then
    TST_LOC=$ING_ROOT/src/tst
fi

if [ ! -d $TST_LOC ]
then
    echo "$TST_LOC not found."
    exit 1
fi

# Verify the existance of the task specific scripts.
echo "Verifying existance of required tools."
echo ""
for fileName in $BUILDTOOLS/cleanbuild.sh $ING_ROOT/runbuild.sh $BUILDTOOLS/createdbms $BUILDTOOLS/root_setup.sh $BUILDTOOLS/runtest.sh $BUILDTOOLS/test_env.sh $BUILDTOOLS/set_env.sh $BUILDTOOLS/clear_env.sh
do
    if [ ! -f $fileName ]
    then
        echo "Could not find $fileName"
        echo ""
        exit 1
    fi
done

# Create users for the test suite
for userName in ingres pvusr1 pvusr2 pvusr3 pvusr4 testenv
do
    /usr/sbin/useradd $userName > /dev/null 2>&1 
    rc=$?
    if [ $rc -ne 0 ] && [ $rc -ne 9 ]
    then
        echo "Error adding user $userName rc=$rc"
        exit 1
    fi
done

# Automatically set testenv's password to ca-testenv

# Trac #187 Debian/Ubuntu doesn't have passwd --stdin
#
# use set_env.sh to set OSVER - that way the test for whether the OS is
# Debian-based remains in one place should it need to change

if [ "$OSVER" == "DEBIAN" ]
then
  echo testenv:ca-testenv | chpasswd
else
  echo ca-testenv |passwd testenv --stdin
fi

# Store the existing user data.
export OLD_USER=`stat -c%U $ING_ROOT/runbuild.sh`
export OLD_PERMS=`stat -c%U:%G $ING_ROOT/runbuild.sh`
export INGRES_USER=ingres:`groups ingres|cut -d " " -f1`

# Clean up all the old artifacts so they aren't used for testing.
if [ "$NOCLEAN" != "true" ] &&  [ "$NOBUILD" != "true" ]
then
	echo "Clean up all old build artifacts."
	echo ""
	su $OLD_USER -c $BUILDTOOLS/cleanbuild.sh
fi

# Change all files to ingres ownership.
echo "Change ownership of files to ingres user to enable testing."
echo ""
chown -R $INGRES_USER $ING_ROOT

# Build the code as ingres user.
if [ "$NOBUILD" != "true" ]
then
	echo "Building code as ingres user."
	echo ""
	su ingres -c $ING_ROOT/runbuild.sh
fi

# Create a mini script to install the database and copy the test suites.
# This mini script is executed as ingres and removed on completion.
echo $BUILDTOOLS/createdbms $II_INSTALLATION > $ING_ROOT/tmpCmd
echo "[ ! -e $ING_TST ] && ln -s $TST_LOC $ING_TST" >> $ING_ROOT/tmpCmd
chmod 777 $ING_ROOT/tmpCmd
su ingres -c $ING_ROOT/tmpCmd
rm $ING_ROOT/tmpCmd

# Complete root only setup for the installation
$BUILDTOOLS/root_setup.sh $II_INSTALLATION

# Create a mini script to run the test suite.  This is executed as testenv
# and removed on completion.
echo $BUILDTOOLS/runtest.sh $II_INSTALLATION > $ING_ROOT/tstTmpCmd
chmod 777 $ING_ROOT/tstTmpCmd
su testenv -c $ING_ROOT/tstTmpCmd
rm $ING_ROOT/tstTmpCmd

# Create a mini script to shutdown the running instance.  This is executed as
# ingres and removed on completion.
echo . $BUILDTOOLS/test_env.sh $II_INSTALLATION > $ING_ROOT/tmpStopCmd
echo ingstop >> $ING_ROOT/tmpStopCmd
chmod 777 $ING_ROOT/tmpStopCmd
su ingres -m -c $ING_ROOT/tmpStopCmd
rm $ING_ROOT/tmpStopCmd

# Return all files to the original user ownership.  This will allow the
# originial user to create another install instance based on what was just
# built, but they will not be able to run the same instance.
chown -R $OLD_PERMS $ING_ROOT
