:
#  Copyright (c) 2004, 2010 Ingres Corporation
#
#  Name:
#	iisustar -- set-up script for Ingres Star Distributed DBMS
#
#  Usage:
#	iisustar
#
#  Description: This script should only be called by the Ingres
#	installation utility.  It sets up Ingres Star Distributed
#	DBMS on a server which already has the Ingres Intelligent
#	DBMS and Ingres Networking set up.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	16-feb-93 (tyler)
##		Created.	
##      15-mar-93 (tyler)
##		Changed if/then and while loop coding style.  Added
##		batch mode.
##	17-mar-93 (tyler)
##		Added code which validates installation code specified 
##		on the command line in batch mode.  Added check for syntax
##		errors in configuration files.
##	21-jun-93 (tyler)
##		Added "Press RETURN to continue:" prompt after errors.
##		Removed confirmation prompt since there are no other 
##		prompts and it's probably correct to proceed.  An
##		interrupts now causes the program to exit with an error
##		status. 
##	23-jul-93 (tyler)
##		Removed extra blank line from generated output.  Removed
##		-installation flag from iimaninf call.
##	26-jul-93 (tyler)
##		Included iishlib and added call to remove_temp_resources()
##		before final (completion) exits to strip temporary resources
##		from config.dat  Added call to iisulock to lock config.dat
##		during setup.  Switched to using pause(). 
##	04-aug-93 (tyler)
##		Replaced calls to pause() before failure exits by calls
##		to pause_message().
##	20-sep-93 (tyler)
##		Make sure II_CONFIG is set before it is referenced.
##	19-oct-93 (tyler)
##		Removed reference to the "Running INGRES" chapter of
##		the Installation Guide.  Removed redundant message
##		which reported syntax error in config.dat or protect.dat. 
##		Stopped clearing the screen at the beginning of the
##		script.
##	22-nov-93 (tyler)
##		Removed multiple instances of code which removed config.lck
##		on exit - now handled by "trap".  Replaced several system
##		calls with calls to shell procedures.  Replaced iimaninf
##		call with ingbuild call.
##	30-nov-93 (tyler)
##		ingbuild must be invoked by its full path name.
##	16-dec-93 (tyler)
##		Use CONFIG_HOST instead of HOSTNAME to compose
##		configuration resource names.
##	03-apr-95 (canor01)
##		Traps cannot be canceled from inside a function in AIX 4.1,
##		so explicitly issue "trap 0" before calling clean_exit()
##      15-jun-95 (popri01)
##              nc4_us5 complained about VERSION sed string. Escape the '/'.
##      17-jul-95 (harpa06)
##              Added feature of removing all references to STAR in config.dat
##              when removing the package through INGBUILD.
##	22-nov-95 (hanch04)
##		added log of install
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##      27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##      26-Sep-2000 (hanje04)
##              Added new flag (-vbatch) to generate verbose output whilst
##              running in batch mode. This is needed by browser based
##              installer on Linux.
##	15-May-2001 (hanje04)
##		II 2.6 needs stack size of 131072. This must be changed 
##		for an upgrade installation.
##       18-Sep-2001 (gupsh01)
##              Added new flag (-mkresponse) to create a response file but
##              do not actually do the setup.
##       23-Oct-2001 (gupsh01)
##              Added code to handle -exresponse flag.
##	5-Dec-2003 (schka24)
##		Just init Star's configuration, don't smash everything.
##	20-Jan-2004 (bonro01)
##		Remove ingbuild calls for PIF installs
##    30-Jan-2004 (hanje04)
##        Changes for SIR 111617 changed the default behavior of ingprenv
##        such that it now returns an error if the symbol table doesn't
##        exists. Unfortunately most of the iisu setup scripts depend on
##        it failing silently if there isn't a symbol table.
##        Added -f flag to revert to old behavior.
##      15-Jan-2004 (hanje04)
##              Added support for running from RPM based install.
##              Invoking with -rpm runs in batch mode and uses
##              version.rel to check version instead of ingbuild.
##	03-feb-2004 (somsa01)
##		backed out symbol.tbl change for now.
##      02-Mar-2004 (hanje04)
##          Duplicate trap inside do_su function so that it is properly
##          honoured on Linux. Bash shell doesn't keep traps across functions.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  DEST = utility
#----------------------------------------------------------------------------

. iisysdep
. iishlib
# override value of II_CONFIG set by ingbuild
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    II_CONFIG=$II_SYSTEM/ingres/files
    export II_CONFIG
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
fi
export cfgloc symbloc

if [ "$1" = "-rmpkg" ] ; then
   cp -p ${cfgloc}/config.dat ${cfgloc}/config.tmp
 
   trap "cp -p ${cfgloc}/config.tmp ${cfgloc}/config.dat; \
         rm -f ${cfgloc}/config.tmp; exit 1" 1 2 3 15
 
   cat ${cfgloc}/config.dat | grep -v '\.star' | \
       grep -v 'iistar_swap' >${cfgloc}/config.new
 
   rm -f ${cfgloc}/config.dat
 
   mv ${cfgloc}/config.new ${cfgloc}/config.dat
 
   rm -f ${cfgloc}/config.tmp
 
   cat << !
  The Ingres Star Distributed DBMS has been removed.
 
!
 
else

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false

# check for batch flag
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
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
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   [ "$1" ] &&
   {
      # reset HOSTNAME for remote NFS client
      HOSTNAME=$1
      CLIENT_ADMIN=$II_SYSTEM/ingres/admin/$HOSTNAME
   }
fi

export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

do_iisustar()
{
echo "Setting up Ingres Star Distributed DBMS..."

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

check_response_file

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## **** Skip everything if we are in WRITE_RESPONSE mode ****


iisulock "Ingres Star Distributed DBMS setup" || exit 1

ACTUAL_HOSTNAME=$HOSTNAME

touch_files # make sure symbol.tbl and config.dat exist

if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
    VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=star` ||
    {
	cat << !

$VERSION

!
	exit 1
    }
elif [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    VERSION=`head -1 /usr/share/ingres/version.rel` ||
    {
	cat << !

Missing file /usr/share/ingres/version.rel

!
	exit 1
    }
else
    VERSION=`head -1 $II_SYSTEM/ingres/version.rel` ||
    {
	cat << !

Missing file $II_SYSTEM/ingres/version.rel

!
	exit 1
    }
fi


RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`iigetres ii.$CONFIG_HOST.config.star.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] &&
{
   cat << !

The $VERSION version of Ingres Star Distributed DBMS has already been
set up on "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
}

DBMS_SETUP=`iigetres ii.$CONFIG_HOST.config.dbms.$RELEASE_ID`
NET_SETUP=`iigetres ii.$CONFIG_HOST.config.net.$RELEASE_ID`

if [ "$DBMS_SETUP" != "complete" ] || [ "$NET_SETUP" != "complete" ]
then
   cat << !

The setup procedures for the following version of Ingres Intelligent
DBMS and Ingres Networking:

	$VERSION

must be completed up before Ingres Star Distributed DBMS can be set
up on host:

	$HOSTNAME

!
   exit 1
fi

II_INSTALLATION=`ingprenv II_INSTALLATION`

cat << ! 

This procedure will set up the following version of the Ingres Star
Distributed DBMS:

	$VERSION

to run on local host:

	$HOSTNAME
!

# generate default configuration resources
## At some point this ought to be made clever like iisudbms, and handle
## new/changed resources more delicately.  There's no great need for it
## at the moment. (schka24)
cat << !

Generating default configuration...
!
iiremres ii.$CONFIG_HOST.syscheck.star_swap

	# make sure upgrade installation gets larger stack size

if iigenres $CONFIG_HOST $II_CONFIG/star.rfm ; then
   iisetres "ii.$CONFIG_HOST.star.*.stack_size" 131072
   # Remove obsolete resources
   iiremres "ii.$CONFIG_HOST.star.*.rdf_cluster_nodes"
   iiremres "ii.$CONFIG_HOST.star.*.rdf_netcosts_rows"
   iiremres "ii.$CONFIG_HOST.star.*.rdf_nodecosts_rows"

else

   cat << !
An error occurred while generating the default configuration.

!
   exit 1
fi

iisetres ii.$CONFIG_HOST.config.star.$RELEASE_ID complete
remove_temp_resources
cat << !

Ingres Star Distributed DBMS has been successfully set up in this
installation.

!
fi #end WRITE_RESPONSE mode
$BATCH || pause
trap : 0
clean_exit
}
eval do_iisustar $INSTLOG
trap : 0
exit 0
fi
