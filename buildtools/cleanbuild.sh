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
## Filename: cleanbuild.sh
##
## Description:
## This script will clean up all artifacts from a build and return a work
## area to the state it would be immediately after an svn co, or
## as close as possible.
##
## History:
##	21-May-2009 (hanje04)
##	    Added license, updated for OSX


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
# if we're starting fresh, set up the environment
if [ -f ${BUILDTOOLS}/set_env.sh ]
then
  . $BUILDTOOLS/set_env.sh
else
  echo "Error, I cannot find the set_env.sh script"
  echo "This is required to use cleanbuild.sh"
  echo "Please run cleanbuild.sh from the root directory of the Ingres source"
  exit 1
fi

# Call jam clean to remove compiled objects.
cd $ING_SRC
jam clean
cd $ING_ROOT

# Remove all binaries and other build results at the directory level.
rm -rf build
rm -rf install
rm -rf release
rm -rf tools
