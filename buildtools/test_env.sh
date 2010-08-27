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
## Filename: test_env.sh
##
## Description:
## Sets up environment for running automated tests
##
## History:
##	21-May-2009 (hanje04)
##	    Added license, updated for OSX
##	17-feb-2010 (stephenb)
##		Add -noclear parameter (passed to set_env.sh) to prevent 
##		clearing of pre-existing environment. Use existing ING_TST if
##		it is set; this allows testing with a pre existing test environment
##  19-feb-2010 (stephenb)
##		Set II_DATE_CENTURY_BOUNDARY to 20 to prevent test diffs
##	15-Mar-2010 (hanje04)
##	    Set II_CONFIG and II_ADMIN to prevent conflict with build
##	    environment following LSB changes.
##	24-Mar-2010 (frima01)
##	    Removed function keyword - not supported on all systems.
##  08-Jun-2010 (thich01)
##     Added set -- to clear passed in arguments
##	22-jun-2010 (stephenb)
##	    Add -hoqa option to run handoffqa version of list files (valid
##	    only for Ingres internal testing)

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
    return
  fi
fi
export BUILDTOOLS

usage()
{
	echo "Usage: test_env.sh [-noclear] [-hoqa] installation_code"
	echo ""
	echo "-hoqa implies -noclear and will set both"
	echo "-noclear requires that the environment variable ING_TST is set"
}

set_testenv()
{
	export TERM_INGRES=konsolel
	export II_SYSTEM=${ING_ROOT}/install/${II_INSTALLATION}
	export II_ADMIN=$II_SYSTEM/ingres/files
	export II_CONFIG=$II_ADMIN
	export ING_TOOLS=$II_SYSTEM/tools

	export LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
	xpath1=.:$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility
	xpath1=$xpath1:$ING_TOOLS/bin:$ING_TOOLS/utility
	export PATH=${xpath1}:$PATH

        [ "$ING_TST" ] ||
	    ING_TST=$II_SYSTEM/tst
	export ING_TST
	export TST_OUTPUT=$II_SYSTEM/output
	export TST_ROOT_OUTPUT=$TST_OUTPUT
	export TST_SHELL=$ING_TST/suites/shell
	if [ "$hoqa" ]
	then
	    export TST_CFG=$ING_TST/suites/handoffqa
	    export TST_LISTEXEC=$ING_TST/suites/handoffqa
	else
	    export TST_CFG=$ING_TST/suites/acceptst
	    export TST_LISTEXEC=$ING_TST/suites/acceptst
	fi
	export TST_TOOLS=$ING_TOOLS/bin
	export TOOLS_DIR=$ING_TOOLS

	export TST_INIT=$ING_TST/basis/init
	export TST_TESTOOLS=$ING_TST/testtool
	export TST_DATA=$ING_TST/gcf/gcc/data
	export TST_TESTENV=$ING_TST

	# This should be set to your where 'vi' is installed, other editors might work too
	export ING_EDIT=/usr/bin/vi

	export SEP_TIMEOUT=600
	export II_DATE_CENTURY_BOUNDARY=20
	export SEP_DIFF_SLEEP=10
	export REP_TST=$ING_TST
	export SEPPARAM_JPASSWORD=ca-testenv
	export SEPPARAM_ODBCLIB=$II_SYSTEM/ingres/lib
}
if [ $# -eq 0 ] 
then
    usage
    return 1
fi
while [ $# -gt 0 ]
do
    case $1 in
	-noclear)
	    noclear=true
	    shift
	    ;;
	[A-Z][A-Z,0-9])
	    II_INSTALLATION=$1
            export II_INSTALLATION
	    shift
	    ;; 
	-hoqa)
	    noclear=true
	    hoqa=true
	    shift
	    ;;
	*)
	    usage
	    return 1
	    ;;
    esac
done

# Clear $1 argument parms so that they are not passed
# to sourced scripts.
set --

if [ -f ${BUILDTOOLS}/set_env.sh ]
then
    if [ "$noclear" ] 
    then
	. ${BUILDTOOLS}/set_env.sh -noclear
    else
	. ${BUILDTOOLS}/set_env.sh 
    fi
else
    echo "Error, set_env.sh script not found"
    echo "This is required to use test_env.sh"
    echo "Please source test_env.sh from the Ingres source root directory"
fi
set_testenv
