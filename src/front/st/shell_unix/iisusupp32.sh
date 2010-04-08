:
#  Copyright (c) 2005, 2010 Ingres Corporation
#
# Name: iisusup32.sh
#
# Purpose:
#	Setup program for 32bit support for 64bit Ingres
#
## History
##	09-Mar-2005 (hanje04)
##	    Created.
##	22-Jul-2009 (frima01) SIR 122347
##	    Added do_scriptupdate() to modify load scripts to
##	    contain II_SYSTEM/ingres/lib/lp32 in library path.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
#

# DEST = utility

# Initialize variables

. iishlib
. iisysdep

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false
REMOVE=false

# Process command line args
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   if [ "$2" ] ; then
        RESOUTFILE="$2"
   fi
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   if [ "$2" ] ; then
        RESINFILE="$2"
   fi
elif [ "$1" = "-rpm" ] ; then
   RPM=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-rmpkg" ] ; then
   REMOVE=true
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
fi

export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

#
# do_iisusupp32 - Setup package
#
do_iisu32supp()
{
    echo "Setting up 32bit support for 64bit Ingres"

    check_response_file

    if $WRITE_RESPONSE ; then
   	mkresponse_msg
	rc=$?
    else ## **** Skip everything if we are in WRITE_RESPONSE mode ****
	if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
	    libdir=/usr/lib/ingres
	else
	    libdir=$II_SYSTEM/ingres/lib
	fi
 	## Check files are installed
	if [ -d "$libdir/lp32" ] && \
	   [ -d "$II_CONFIG/collation/lp32" ] && \
	   [ -d "$II_CONFIG/ucharmaps/lp32" ] ; then 
	    echo "Setting II_LP32_ENABLED=TRUE"
            ingsetenv II_LP32_ENABLED TRUE
	    rc=$?
	    do_scriptupdate
	else
	    echo "32bit support for 64bit Ingres is not installed"
	    rc=1
	fi
    fi #end WRITE_RESPONSE mode 

}

#
# do_rmsupp32 - Remove package
#
do_rmsupp32()
{
    echo "Removing 32bit support for 64bit Ingres"
    ingunset II_LP32_ENABLED
}

#
# do_scriptupdate - Update environment scripts with lp32 path
#
do_scriptupdate()
{
  [ "$RPM" = "true" ] || return 0
  TMPFILE=/tmp/iildscript.bak

  #Check for user defined installation ID otherwise default to II
  [ -z "$II_INSTALLATION" ] && II_INSTALLATION=`ingprenv II_INSTALLATION`
  [ -z "$II_INSTALLATION" ] && II_INSTALLATION=II

  eval homedir="~"$II_USERID
  [ -d "$homedir" ] && [ -w "$homedir" ] && 
    homebash=$homedir/.ing${II_INSTALLATION}bash &&
    hometcsh=$homedir/.ing${II_INSTALLATION}tcsh
  bashld=$II_SYSTEM/.ing${II_INSTALLATION}bash
  tcshld=$II_SYSTEM/.ing${II_INSTALLATION}tcsh

  for ldscript in $bashld $tcshld $homebash $hometcsh
  do
    if [ -f "$ldscript" ]
    then
      cp $ldscript $TMPFILE
      sed -e '/LD_LIBRARY_PATH/s!\$II_SYSTEM/ingres/lib:!\$II_SYSTEM/ingres/lib:\$II_SYSTEM/ingres/lib/lp32:!' \
      -e '/LD_LIBRARY_PATH/s!\$II_SYSTEM/ingres/lib[ ]*$!\$II_SYSTEM/ingres/lib:\$II_SYSTEM/ingres/lib/lp32!' \
      -e '/LD_LIBRARY_PATH/s!:\$II_SYSTEM/ingres/lib/lp32!!2' $TMPFILE > $ldscript 2> /dev/null ||
      mv $TMPFILE $ldscript
      [ -f "$TMPFILE" ] &&  rm $TMPFILE
    fi
  done
}

if $REMOVE ; then
    eval do_rmsupp32 $INSTLOG
    rc=$?
else
    eval do_iisu32supp $INSTLOG
    rc=$?
fi

exit $rc
