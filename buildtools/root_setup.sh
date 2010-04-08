#!/bin/bash
## Copyright (C) 2008  Ingres Corporation
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
## Filename: root_setup.sh
##
## Description:
## This script will setup users and file permissions used for running the
## test suite.  These are operations specifically requiring root access.
##
## History:
##	21-May-2009 (hanje04)
##	    Added licence, comments and cleaned up for OSX
##  17-feb-2010 (stephenb)
##		qawtl belongs to "ingres" not "root"
##	24-Mar-2010 (frima01)
##	    Removed function keyword - not supported on all systems.

unames=`uname -s`
usage()
{
    echo "Usage: root_setup.sh installation_code"
    trap - 0
    exit 1
}

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
    echo "Error: I'm sorry, I can't find the buildtools directory"
    echo "There are tools that I need from there."
    echo "Please re-source set_env.sh from the root of the source tree."
    echo "This is where README.txt and runbuild.sh are stored."
    echo "I cannot continue."
    exit 1
  fi
fi
export BUILDTOOLS

echo "Checking effective userid...."
export WHOAMI=`whoami`
if [ "$WHOAMI" != "root" ]
then
        echo "You must be user 'root' to run this setup script"
        echo ""
        exit 2
fi

case $1 in
    [A-Z][A-Z,0-9])
        II_INSTALLATION=$1
        ;;
    *)
        echo "$1 is an invalid installation ID"
        usage
        ;;
esac

if [ -z "$II_INSTALLATION" ]
then
    echo "installation_code not specified"
    usage
fi

export II_INSTALLATION

if [ -f ${BUILDTOOLS}/test_env.sh ]
then
    . ${BUILDTOOLS}/test_env.sh $II_INSTALLATION
else
    echo "Error, test_env.sh script not found."
    echo "This is required to use root_setup.sh"
    echo "Please execute root_setup.sh from the Ingres source root directory"
    exit 1
fi

# Execute the shadow utils for Ingres.
if [ -x "$II_SYSTEM/ingres/bin/mkvalidpw" ]
then
     $II_SYSTEM/ingres/bin/mkvalidpw
elif [ "$unames" = "Darwin" ]
then
     # Do nothing, no mkvalidpw on OSX
     true
else
     echo "$II_SYSTEM/ingres/bin/mkvalidpw not found"
     exit 1
fi

# Set permissions on QA tools
if [ ! -f "$ING_TOOLS/bin/qasetuser" ]
then
    echo "$ING_TOOLS/bin/qasetuser not found"
    exit 1
else
    chown root $ING_TOOLS/bin/qasetuser
    chmod 4755 $ING_TOOLS/bin/qasetuser
fi

if [ ! -f "$ING_TOOLS/bin/qawtl" ]
then
    echo "$ING_TOOLS/bin/qawtl not found"
    exit 1
else
    chown ingres $ING_TOOLS/bin/qawtl
    chmod 4755 $ING_TOOLS/bin/qawtl
fi

# Increase SHMMAX (shared memory size) using sysctl if we have it
if [ -x /sbin/sysctl ] ; then
    sysctlcmd=/sbin/sysctl
elif [ -x /usr/sbin/sysctl ] ; then
    sysctlcmd=/usr/sbin/sysctl
else
    sysctlcmd=''
fi
if [ "$sysctlcmd" ] ; then
    $sysctlcmd -w kernel.shmmax=256000000
fi
