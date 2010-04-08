:
#  Copyright (c) 2006 Ingres Corporation.  All Rights Reserved.
#
#  Name:
#	iisudb2udb -- set-up script for DB2 UDB Gateway
#
#  Usage:
#	iisudb2udb -batch [inst_code]
#
#  Description:
#	This script should only be called by the Ingres installation
#	utility or by the ingmknfs wrapper.  It sets up the DB2 UDB
#	Gateway on a server.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	29-oct-2002 (mofja02)
##		Created from iisusybase.sh. 
##	20-Jan-2004 (bonro01)
##		Remove ingbuild calls for PIF installs
##    30-Jan-2004 (hanje04)
##        Changes for SIR 111617 changed the default behavior of ingprenv
##        such that it now returns an error if the symbol table doesn't
##        exists. Unfortunately most of the iisu setup scripts depend on
##        it failing silently if there isn't a symbol table.
##        Added -f flag to revert to old behavior.
##	03-feb-2004 (somsa01)
##		Backed out last change for now.
##      09-dec-2004 (clach04)
##              Script was locking the config.dat (.lck) file
##              and not releasing it. Added start of response file
##              code simply to resolve the locking issue.
##              This change does not attempt to deal with db2udb repsonse
##              file configuration.
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##  31-Oct-2005 (clach04)
##      Bug 115482
##      Updated product name to Enterprise Access
##	30-Nov-2005 (hanje04)
##	    Use head -1 instead of cat to read version.rel to
##	    stop patch ID's being printed when present.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##	15-Dec-2009 (hanje04)
##	    Bug 123097
##	    Remove references to II_GCNXX_LCL_VNODE as it's obsolete.
##
#  DEST = utility
#----------------------------------------------------------------------------

GW_PROD_NAME="Enterprise Access for IBM DB2 UDB"
export GW_PROD_NAME

if [ "$1" = "-rmpkg" ] ; then
   II_CONF_DIR=$II_SYSTEM/ingres/files
 
   cp -p $II_CONF_DIR/config.dat $II_CONF_DIR/config.tmp
 
   trap "cp -p $II_CONF_DIR/config.tmp $II_CONF_DIR/config.dat; \
         rm -f $II_CONF_DIR/config.tmp; exit 1" 1 2 3 15
 
   cat $II_CONF_DIR/config.dat | grep -v 'db2udb' >$II_CONF_DIR/config.new
 
   rm -f $II_CONF_DIR/config.dat
 
   mv $II_CONF_DIR/config.new $II_CONF_DIR/config.dat
 
   rm -f $II_CONF_DIR/config.tmp
 
   cat << !
  ${GW_PROD_NAME} has been removed.
 
!
fi

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp

# check for batch flag
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_SYSTEM/ingres/files/install.log"
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
   INSTLOG="2>&1 | tee -a $II_SYSTEM/ingres/files/install.log"
fi

export BATCH
export INSTLOG
export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

trap "rm -f $II_SYSTEM/ingres/files/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15

do_iisudb2udb()
{
echo "Setting up ${GW_PROD_NAME} ..."

. iisysdep

. iishlib

iisulock "${GW_PROD_NAME} setup" || exit 1

touch_files # make sure symbol.tbl and config.dat exist

# override local II_CONFIG symbol.tbl setting
ingsetenv II_CONFIG $II_CONFIG

SERVER_HOST=`iigetres ii."*".db2udb.server_host` || exit 1

# grant owner and root all privileges 
iisetres ii.$CONFIG_HOST.privileges.user.root \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.$CONFIG_HOST.privileges.user.$ING_USER \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED


if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
   VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=db2udb` ||
   {
       cat << !

$VERSION

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

SETUP=`iigetres ii.$CONFIG_HOST.config.db2udb.$RELEASE_ID`
if [ "$SETUP" = "complete" ] ; then
   cat << !

The $VERSION version of ${GW_PROD_NAME}
has already been set up on local host "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
fi
cat << ! 

This procedure will set up the following release of 
${GW_PROD_NAME}:

	$VERSION

to run on local host:
 
	$HOSTNAME

This installation will be owned by:

	$ING_USER
!
#
# get II_INSTALLATION 
#
[ -z "$II_INSTALLATION" ] && II_INSTALLATION=`ingprenv II_INSTALLATION`
if [ "$II_INSTALLATION" ] ; then 
   cat << !

II_INSTALLATION configured as $II_INSTALLATION. 
!
   ingsetenv II_INSTALLATION $II_INSTALLATION
else
   if $BATCH ; then
      if [ "$2" ] ; then
         case "$2" in 
            [a-zA-Z][a-zA-Z0-9])
               II_INSTALLATION="$2"
               ;;
            *)
               cat << !

The installation code you have specified is invalid. The first character
code must be a letter; the second may be a letter or a number.

!
               exit 1
               ;;
         esac
      else
         II_INSTALLATION="II"
      fi
      cat << !

II_INSTALLATION configured as $II_INSTALLATION. 
!
   else
      get_installation_code
   fi
   ingsetenv II_INSTALLATION $II_INSTALLATION
fi

$BATCH || prompt "Do you want to continue this setup procedure?" y \
|| exit 1

   if [ "$SETUP" != "defaults" ] ; then
      # generate default configuration resources
      echo ""
      echo "Generating default configuration..."
      if iigenres $CONFIG_HOST $II_SYSTEM/ingres/files/db2udb.rfm ; then
         iisetres ii.$CONFIG_HOST.config.db2udb.$RELEASE_ID defaults
      else
         cat << !
An error occurred while generating the default configuration.

!
         exit 1
      fi
   else
      cat << !

Default configuration generated.
!
   fi

   # display II_INSTALLATION 
   cat << !
   
II_INSTALLATION configured as $II_INSTALLATION. 

!


iisetres ii.$CONFIG_HOST.config.db2udb.$RELEASE_ID complete
remove_temp_resources
cat << !
${GW_PROD_NAME} has been successfully set up in this installation.

You can now use the "ingstart" command to start the installation.
Refer to the Enterprise Access Installation Guide for more information
about starting and using Enterprise Access.

!
   $BATCH || pause
trap : 0
clean_exit
}

eval do_iisudb2udb $INSTLOG
trap : 0
exit 0
