:
#  Copyright (c) 2006 Ingres Corporation.  All Rights Reserved.
#
#  Name:
#	iisuoracle -- set-up script for Oracle Gateway
#
#  Usage:
#	iisuoracle -batch [inst_code] | [ inst_code [oracle_version] ]
#
#  Description:
#	This script should only be called by the Ingres installation
#	utility or by the ingmknfs wrapper.  It sets up the Oracle
#	Gateway on a server. Oracle version may also be specified via
#	OS variable ORACLE_VER.
#
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	25-mar-1998 (canor01)
##		Created. 
##	20-apr-98 (mcgem01)
##		Product name change to Ingres.
##	05-nov-98 (toumi01)
##		Modify INSTLOG for -batch to an equivalent that avoids a
##		nasty bug in the Linux GNU bash shell version 1.
##	25-oct-00 (clach04)
##		Removed check for II_AUTH_STRING this is no longer needed.
##		LicenseIt now deals with all licensing checks.
##	26-Feb-01 (clach04)
##		Added call to iigwuseora to select Oracle version.
##		Added support to pass Oracle version as parameter
##	12-Jun-02 (clach04)
##		Added support for using latest version of Oracle without
##		prompting user, this is used for Express installs where no
##		user input is possible. Can still specify version explictly
##		by specifying on the command line (see usage) and also by
##		pre-setting os variable ORACLE_VER.
##		ORACLE_VER can only be used if the -batch flag is used.
##      27-apr-2000 (hanch04)
##            Moved trap out of do_setup so that the correct return code
##            would be passed back to inbuild.
##	18-sep-2001 (gupsh01)
##		Added -mkresponse option.
##	23-oct-2001 (gupsh01)
##		Added code to handle -exresponse option.
##      18-Jul-2002 (hanch04)
##              Changed Installation Guide to Getting Started Guide.
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
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##  31-Oct-2005 (clach04)
##      Bug 115482
##      Updated product name to Enterprise Access
##  31-Oct-2005 (clach04)
##      Bug 115473
##      Added code to handle ExpressInstalls (which are essentially
##      batch mode) by defaulting the Oracle version to the latest.
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

GW_PROD_NAME="Enterprise Access for Oracle"
export GW_PROD_NAME

if [ "$1" = "-rmpkg" ] ; then
   II_CONF_DIR=$II_SYSTEM/ingres/files
 
   cp -p $II_CONF_DIR/config.dat $II_CONF_DIR/config.tmp
 
   trap "cp -p $II_CONF_DIR/config.tmp $II_CONF_DIR/config.dat; \
         rm -f $II_CONF_DIR/config.tmp; exit 1" 1 2 3 15
 
   cat $II_CONF_DIR/config.dat | grep -v 'oracle' >$II_CONF_DIR/config.new
 
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
   # set oracle version, command line parameter takes precedent
   # if non specified, use OS variable ORACLE_VER
   # if that's not set use the latest
   ORACLE_VER="${3:-${ORACLE_VER:-latest}}"
   export ORACLE_VER
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

if [ "$3" ] ; then
   ORACLE_VER="$3"
else
    if [ $BATCH = "true" ] ; then
        ORACLE_VER="latest"
    fi
fi
export ORACLE_VER

if [ "$WRITE_RESPONSE" = "false" ] ; then
trap "rm -f $II_SYSTEM/ingres/files/config.lck /tmp/*.$$ 1>/dev/null \
   2>/dev/null; exit 1" 0 1 2 3 15
fi

do_iisuoracle()
{
echo "Setting up ${GW_PROD_NAME} ..."

. iisysdep

. iishlib

check_response_file

if [ "$WRITE_RESPONSE" = "true" ] ; then
        mkresponse_msg
else ## **** Skip everything if we are in WRITE_RESPONSE mode ****
 
iisulock "${GW_PROD_NAME} setup" || exit 1

touch_files # make sure symbol.tbl and config.dat exist

# override local II_CONFIG symbol.tbl setting
ingsetenv II_CONFIG $II_CONFIG

SERVER_HOST=`iigetres ii."*".oracle.server_host` || exit 1

# grant owner and root all privileges 
iisetres ii.$CONFIG_HOST.privileges.user.root \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.$CONFIG_HOST.privileges.user.$ING_USER \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED

if [ -f $II_SYSTEM/ingres/install/release.dat ]; then
    VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=oracle` ||
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

SETUP=`iigetres ii.$CONFIG_HOST.config.oracle.$RELEASE_ID`
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
fi #end WRITE_RESPONSE mode
#
# get II_INSTALLATION 
#
[ -z "$II_INSTALLATION" ] && II_INSTALLATION=`ingprenv II_INSTALLATION`
if [ "$II_INSTALLATION" ] ; then 
   cat << !

II_INSTALLATION configured as $II_INSTALLATION. 
!
   if [ "$WRITE_RESPONSE" = "true" ] ; then
     eval check_response_write II_INSTALLATION $II_INSTALLATION
   else
     ingsetenv II_INSTALLATION $II_INSTALLATION
   fi
else
   if $BATCH ; then
      if [ "$READ_RESPONSE" = "true" ] ; then
         II_INSTALLATION=`iiread_response II_INSTALLATION $RESINFILE`
         [ -z "$II_INSTALLATION" ] && II_INSTALLATION="II"
      elif [ "$2" ] ; then
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
      if [ "$WRITE_RESPONSE" = "true" ] ; then
        eval check_response_write II_INSTALLATION $II_INSTALLATION replace
      else
        ingsetenv II_INSTALLATION $II_INSTALLATION
      fi
fi

$BATCH || prompt "Do you want to continue this setup procedure?" y \
|| exit 1

if  [ "$WRITE_RESPONSE" = "false" ] ; then # skip the rest for true
   if [ "$SETUP" != "defaults" ] ; then
      # generate default configuration resources
      echo ""
      echo "Generating default configuration..."
      if iigenres $CONFIG_HOST $II_SYSTEM/ingres/files/oracle.rfm ; then
         iisetres ii.$CONFIG_HOST.config.oracle.$RELEASE_ID defaults
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

$II_SYSTEM/ingres/utility/iigwuseora ${ORACLE_VER}

iisetres ii.$CONFIG_HOST.config.oracle.$RELEASE_ID complete
remove_temp_resources
cat << !
${GW_PROD_NAME} has been successfully set up in this installation.

You can now use the "ingstart" command to start the installation.
Refer to the Enterprise Access Installation Guide for more information 
about starting and using Enterprise Access.

!

fi #end WRITE_RESPONSE mode
   $BATCH || pause
trap : 0
clean_exit
}

eval do_iisuoracle $INSTLOG
trap : 0
exit 0
