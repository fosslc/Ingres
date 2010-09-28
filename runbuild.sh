#!/bin/sh
##  08-Jun-2010 (thich01)
##     Added set -- to clear passed in arguments
##  17-Aug-2010 (thich01)
##     Added quit op to allow jam to stop building on error by default. Changed
##     response text to make it more clear.

# This is a simple script to help you run the build with less overhead.
# It will automatically set up your environment and run the build.
# It will log all logs to a timestamped log file.

#TODO
# - add online help
# - add automatic execution of sep tests
# - add automatic packaging 
# - add cleanup and forced rebuild options (jam clean with -c) (jam force with -a)
# - add some additional sanity checks as issues are uncovered

NOW=`date +"%m_%d_%y_%H%M"`
PWD=`pwd`
LOGDIR="${PWD}/logs"
LOGFILE="${LOGDIR}/jam_${NOW}.log"
DEBUGOPT=""
REBUILDOPT=""
CLEANOPT=""

usage()
{
  cat <<STOP
Usage: -a to force a rebuild (i.e. jam -a)
       -c to clean a build (i.e. jam clean)
       -g to compile with extra debugging information
       -q to turn off the jam -q option, which will make jam complete the whole process, even when encountering an error.
       -h for this help text.
STOP
}

QUITOPT="-q"

while getopts "acghq" options; do
  case $options in
     a ) REBUILDOPT="-a";;
     c ) CLEANOPT="clean";;
     g ) DEBUGOPT="-sIIOPTIM=-g";;
     q ) QUITOPT="";;
     h ) usage
       exit 1;;
    \? ) usage
       exit 1;;
     * ) usage
       exit 1;;
  esac
done

# Check to see where the rest of the build tools are
if [ -d ./buildtools ]
then
  BUILDTOOLS="${PWD}/buildtools"
  export BUILDTOOLS
fi

# Set a return code to success, we'll toggle it if anything fails
RETURN=0

if [ ! -d ${LOGDIR} ]
then
  mkdir -p ${LOGDIR}
fi

# Clear $1 argument parms so that they are not passed
# to sourced scripts.
set --

# if we're starting fresh, set up the environment
if [ -f ${BUILDTOOLS}/set_env.sh ]
then
  . ${BUILDTOOLS}/set_env.sh || exit 1
else
  echo "Error, I cannot find the set_env.sh script"
  echo "This is required to use runbuild.sh"
  echo "Please run runbuild.sh from the root directory of the Ingres source"
  exit 1
fi

if [ $CLEANOPT ]
then
  cd $ING_SRC
  date > ${LOGFILE}
  jam $CLEANOPT >> ${LOGFILE} 2>&1
  echo "Clean complete."
  echo "Logs are in: ${LOGFILE}"
  exit 0
fi

date > ${LOGFILE}
echo " "
echo "Starting Ingres build."
echo "This will take roughly 20 minutes or more."
echo "(depending on how fast your computer is)"
echo " "
echo "Use \"tail -f ${LOGFILE}\""
echo "in another terminal window to follow progress."
echo " "
echo "RESULTS:"
cd $ING_ROOT/src/tools/port/jam/; jam >> ${LOGFILE} 2>&1
cd $ING_SRC; mkjams >> ${LOGFILE} 2>&1 
jam $QUITOPT $REBUILDOPT $DEBUGOPT >> ${LOGFILE} 2>&1

# Check if the build succeeded or failed by the presence of
# skipped or failed targets
grep "failed updating" ${LOGFILE} > /dev/null 2>&1
if [ $? -eq 0 ]
then
  echo "Some compile targets failed."
  RETURN=1
else
  echo "All compile targets were successful."
fi
grep "skipped" ${LOGFILE} > /dev/null 2>&1
if [ $? -eq 0 ]
then
  echo "Some compile targets were skipped."
  RETURN=1
else
  echo "All compile targets were executed."
fi

buildrel -a >> ${LOGFILE} 2>&1
if [ $? -eq 0 ]
then
  echo "buildrel -a succeeded."
else
  echo "buildrel -a failed."
  RETURN=1
fi

echo " "
date >> ${LOGFILE}

if [ $RETURN -eq 0 ]
then
  echo "The build succeeded."
else
  echo "The build failed. Check the logs to determine why."
fi

echo "Logs are in: ${LOGFILE}"

exit ${RETURN}
