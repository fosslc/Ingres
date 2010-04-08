:
#  Copyright (c) 2004, 2010 Ingres Corporation	
#  Name: iisutm	
#
#  Usage:
#	iisutm
#
#  Description:
#	Setup program for terminal monitors called by the installation
#	utility.
#
#  Exit Status:
#	0	setup successful	
#	1	setup failed.	
#
## History:
##	xx-xxx-92 (jonb)	
##		Created.	
##	22-nov-93 (tyler)
##		Replaced system calls with shell procedure invocations.
##	05-jan-93 (tyler)
##		Removed extra newline in displayed output.
##	22-nov-95 (hanch04)
##		added log of install
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##      26-Sep-2000 (hanje04)
##              Added new flag (-vbatch) to generate verbose output whilst
##              running in batch mode. This is needed by browser based
##              installer on Linux.
##	18-Sep-2001 (gupsh01)
##		Added new flag -mkresponse for generating response file, without
##		actually doing the installation.
##	23-Oct-2001 (gupsh01)
##		Added code to handle -exresponse flag.
##	7-Mar-2004 (bonro01)
##		Added -rmpkg flag for PIF un-installs
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG1PRFX)sutm
#
#  DEST = utility
#--------------------------------------------------------------------------

if [ "$1" = "-rmpkg" ] ; then
   cat << !
  Terminal Monitor utility files have been removed

!
exit 0
fi

. (PROG1PRFX)sysdep
. (PROG1PRFX)shlib

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp

# check for batch flag
if [ "$1" = "-batch"  -o "$1" = "-rpm" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> ${II_LOG}/install.log"
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a ${II_LOG}/install.log"
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a ${II_LOG}/install.log"
   if [ "$2" ] ; then
        RESOUTFILE="$2"
   fi
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a ${II_LOG}/install.log"
   if [ "$2" ] ; then
        RESINFILE="$2"
   fi
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a ${II_LOG}/install.log"
fi

export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE

do_iisutm()
{
check_response_file

#
# Install utility files: dayfile, startup.
#

echo "Installing Terminal Monitor utility files..."

exitstat=0

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## **** Skip everything if we are in WRITE_RESPONSE mode ****
 
if [ "$conf_LSB_BUILD" = "TRUE" ] ; then
    cd ${II_ADMIN}
else
    cd $(PRODLOC)/(PROD2NAME)/files
fi
 
#
# Save the old dayfile, and install the new one.
#
 
[ -r dayfile ] && {
    mv dayfile dayfile.old
    [ $? -eq 0 ] || exitstat=$?
}

cp dayfile.dst dayfile
[ $? -eq 0 ] || exitstat=$?

chmod 644 dayfile
[ $? -eq 0 ] || exitstat=$?
 
#
# quel and sql startup files
#

for file in startup startsql
do
    [ -f ${file}.dst -a ! -r $file ] && {
	cp ${file}.dst $file
	[ $? -eq 0 ] || exitstat=$?
	chmod 644 $file
	[ $? -eq 0 ] || exitstat=$?
    }
done
fi #end WRITE_RESPONSE mode

exit $exitstat
}
eval do_iisutm $INSTLOG
