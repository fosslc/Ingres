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

# Clear $1 argument parms so that they are not passed
# to sourced scripts.
set --

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
cd $ING_ROOT
runbuild.sh -c

# Remove all binaries and other build results at the directory level.
rm -rf build
rm -rf install
rm -rf release
rm -rf tools
rm -rf src/man1
rm -rf src/lib
rm -rf src/bin
rm -rf src/sig/inglogs/Jamdbg
rm -rf src/sig/inglogs/Jamfile
rm -rf src/sig/inglogs/inglogs/Jamdbg
rm -rf src/sig/inglogs/inglogs/Jamfile
rm -rf src/common/odbc/hdr/sql.h
rm -rf src/common/odbc/hdr/sqlext.h
rm -rf src/common/hdr/oldmsg_unix_vmsv2_generic.h
rm -rf src/common/hdr/oldmsg_unix_vmsv2_other.h
rm -rf src/common/hdr/hdr/fe.cat.msg
rm -rf src/front/ice/plays/src/play_NewOrder.h
rm -rf src/ha/Jamdbg
rm -rf src/ha/Jamfile
rm -rf src/ha/sc/Jamdbg
rm -rf src/ha/sc/Jamfile
rm -rf src/ha/sc/specials/Jamdbg
rm -rf src/ha/sc/specials/Jamfile
rm -rf src/ha/sc/shell/Jamdbg
rm -rf src/ha/sc/shell/Jamfile
rm -rf src/ha/sc/util/Jamdbg
rm -rf src/ha/sc/util/Jamfile
rm -rf src/front/st/geo/Jamdbg
rm -rf src/front/st/geo/Jamfile

