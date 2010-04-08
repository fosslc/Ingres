:
#
#  Copyright (c) 2004, 2010 Ingres Corporation 
#  Setup program for the Ingres library
#
## History
##	11-feb-93 (?)
##		Created.
##	16-mar-93 (dianeh)
##		Added ":" as first line; added History comment block.
##	13-may-93 (vijay)
##		Don't run ranlib libingres.a if using shared libs.
##	23-Jan-1995 (canor01)
##		Run "ar -ts" if ranlib is not available (B65025)
##	26-Feb-1995 (canor01)
##		Check path and usual places for ranlib and ar
##	25-Apr-1995 (holla02)
##		Was setting RANLIB_CMD to "ar -ts" even if ranlib was found
##		(incorrect ar syntax on su4_u42).  Skip ar if we've got ranlib.
##	16-jun-1995 (murf)
##		dr6_us5 returns "Not found" when searching for ranlib, which 
##		is the correct functioning. The problem is due to a case
##		sensitive grep which searches for "not found". Changed this
##		to "grep -i" to find all cases.
##	22-nov-95 (hanch04)
##		added batchmode and log of install
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
##		Added new flag (-mkresponse) to generate a response file, 
##		instead of carrying out the installation.
##	23-Oct-2001 (gupsh01)
##		Added code to handle exresponse flag. 
##	15-Nov-2001 (hanje04)
##	16-Nov-2001 (hanje04) - BACKED OUT
##		BUG:106401
##		Create link from $II_SYSTEM/ingres/lib/liblic98.so to
##		/ca_lic/lic98.so (or equiv) to resolve runtime link errors
##		with VDBA
##	7-Mar-2004 (bonro01)
##		Add -rmpkg flag for PIF un-installs
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
#


if [ "$1" = "-rmpkg" ] ; then
   cat << !
  The (PROD1NAME) library has been removed

!
exit 0
fi

# PROGRAM = (PROG1PRFX)sulib
#
# DEST = utility

# check for response file modes
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp

if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $(PRODLOC)/(PROD2NAME)/files/install.log"
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $(PRODLOC)/(PROD2NAME)/files/install.log"
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
   if [ "$2" ] ; then
        RESOUTFILE="$2"
   fi
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
   if [ "$2" ] ; then
        RESINFILE="$2"
   fi
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $(PRODLOC)/(PROD2NAME)/files/install.log"
fi

export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

do_iisulib()
{
echo "Setting up the (PROD1NAME) library..."

. (PROG1PRFX)sysdep

if [ "$WRITE_RESPONSE" = "true" ] ; then
   exit 0 # do not do anything here
fi

if $USE_SHARED_LIBS 
then
    exit 0
fi

RANLIB_CMD=""
# check PATH
sh -c ranlib 2>&1 | grep -i "not found"  >/dev/null 2>&1 || RANLIB_CMD=ranlib
if [ "$RANLIB_CMD" = "" ]
then
sh -c ar 2>&1     | grep -i "not found"  >/dev/null 2>&1 || RANLIB_CMD="ar -ts"
fi
if [ "$RANLIB_CMD" = "" ]
then
    # not in PATH, check usual places:
    for DIRS in /bin /usr/bin /usr/ucb /usr/ccs/bin
    do
	if [ -f $DIRS/ranlib ]
	then
		RANLIB_CMD=$DIRS/ranlib
		break
	elif [ -f $DIRS/ar ]
	then
		RANLIB_CMD="$DIRS/ar -ts"
		break
	fi
    done
fi
if [ "$RANLIB_CMD" = "" ]
then
    echo "Unable to run either "ranlib" or "ar" on "
    echo "\$$(PRODLOC)/(PROD2NAME)/lib/lib(PROD2NAME).a..."
    echo "You will need to be able to run this command to properly"
    echo "set up the libaries for use with embedded SQL"
    exit 1
fi

echo Running $RANLIB_CMD on \$(PRODLOC)/(PROD2NAME)/lib/lib(PROD2NAME).a...

$RANLIB_CMD $(PRODLOC)/(PROD2NAME)/lib/lib(PROD2NAME).a > /tmp/(PROG1PRFX)sulib.$$ 2>&1 

if [ $? -eq 0 ]
then
    rm -f /tmp/(PROG1PRFX)sulib.$$
    exit 0
fi

echo "\"$RANLIB_CMD\" failed.  The following output was produced:"
echo
cat /tmp/(PROG1PRFX)sulib.$$
rm -f /tmp/(PROG1PRFX)sulib.$$

exit 1
}
eval do_iisulib $INSTLOG
