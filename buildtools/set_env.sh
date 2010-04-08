#!/bin/sh
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
## Filename: set_env.sh
##
## Description:
##	This script eliminates a few manual steps when building from Ingres's
##	subversion repository. It does this by setting up the build environment
##	variables automatically.
##
##	If the user calls set_env.sh direct rather than through runbuild,
##	we should set BUILDTOOLS so we know where the other tools are
## History:
##	Apr-2008 (thich01)
##	    Added.
##	22-Sep-2008 (hanje04)
##	    Added licence, comments and cleaned up for OSX
##	17-feb-2010 (stephenb)
##		Add -noclear parameter to prevent clearing existing environment.
##		This allows a pre-existing test environment to be used.
##	3-mar-2010 (stephenb)
##		Save ING_TST before calling bldenv because it is unconditionally
##		set in that script.
##	24-Mar-2010 (frima01)
##		Removed function keyword - not supported on all systems.

usage()
{
        echo "Usage: set_env.sh [-noclear]"
}

PWD=`pwd`
if [ `basename ${PWD}` = "buildtools" ]
then
  cd ..
fi

if [ -z ${BUILDTOOLS} ]
then
  if [ -d ./buildtools ]
  then
    export BUILDTOOLS="${PWD}/buildtools"
    export PATH=${BUILDTOOLS}:$PATH
  else
    echo "Error: I'm sorry, I can't find the buildtools directory"
    echo "There are tools that I need from there."
    echo "Please re-source set_env.sh from the root of the source tree."
    echo "This is where README.txt and runbuild.sh are stored."
    echo "I cannot continue."
    return
  fi
fi

cd $BUILDTOOLS/..
while [ $# -gt 0 ]
do
    case $1 in
	-noclear)
	    noclear=true
	    shift
	    ;;
    	*)
	    usage
	    return 1
	    ;;
    esac
done
# Check for the presence of things that likely mean the src directory
# is in front of us so we know where we are
if [ -f src/INSTALL ] && [ -d src/back ]
then
  # First, clear the known environment variables used for Ingres builds
  [ "$noclear" ] || . ${BUILDTOOLS}/clear_env.sh
  export ING_ROOT=`pwd`

  # Then, set up the environment using bldenv
  if [ -d "$ING_TST" ]
  then
    savetest=$ING_TST
  fi
  . $ING_ROOT/src/tools/port/jam/bldenv
  if [ $savetest ]
  then
    ING_TST=$savetest
    export ING_TST
  fi
else
  # Otherwise give up
  echo " "
  echo "This file depends on releative locations of other source files."
  echo "Please source this file from echo the root of the Ingres server source tree."
  echo "This is where README.txt and runbuild.sh are stored."
  echo " "
fi
