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
## Filename: runtest.sh
##
## Description:
## This script will execute the automated test suite. 
## It must be run as testenv due to the automation result testing.
##
## History:
##	21-May-2009 (hanje04)
##	    Added license, updated for OSX
##	24-Mar-2010 (frima01)
##	    Removed function keyword - not supported on all systems.


usage()
{
    echo "Usage: runtest.sh installation_code"
    trap - 0
    exit 1
}

echo "Checking effective userid...."
export WHOAMI=`whoami`
if [ "$WHOAMI" != "testenv" ]
then
        echo "You must be user 'testenv' to run this setup script"
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

if [ -f ${BUILDTOOLS}/test_env.sh ]
then
    . $BUILDTOOLS/test_env.sh $II_INSTALLATION
else
    echo "Error, test_env.sh script not found."
    echo "This is required to use root_setup.sh"
    echo "Please source root_setup.sh from the Ingres source root directory"
    exit 1
fi

# Set up installation specific environment variables.
if [ -f "$ING_TST/suites/userenv/tstenv" ]
then
    . $ING_TST/suites/userenv/tstenv --reset --auto
    . ~/.tstenvrc
else
    echo "Error, $ING_TST/suites/userenv/tstenv not found."
    echo "Ensure test suite is installed."
    exit 1
fi

# Complete the test suite setup in the database.
qasetuser ingres sh $TST_SHELL/tstsetup.sh

# Execute the test suite.
cp $ING_TST/suites/userenv/testallunix  $TST_OUTPUT/my_test_run
pushd $TST_OUTPUT
chmod +x my_test_run
echo "Test suite running."
echo "Use tail -f $TST_OUTPUT/my_test_run.log to follow logs."
nohup  ./my_test_run >&  my_test_run.log 
echo "Tests complete, logs produced:"
find . -name \*.log
popd

