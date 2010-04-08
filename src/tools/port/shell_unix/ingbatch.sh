:
#
# Copyright (c) 2004 Ingres Corporation
#
##	fast install program
##
##	18-apr-95 (hanch04)
##	    created.
##	05-oct-98 (hanch04)
##	    Modified to be generic for TNG
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

#  ingbatch.sh II_DISTRIBUTION PRODUCTS II_INSTALLATION II_TIMEZONE_NAME LOG_BYTES PASSWORD

#  ingbatch.sh /cdrom/ingres.tar "tm,dbms,net,ome" WV NA-EASTERN 49152 netpass


#  $II_SYSTEM must be set.
#  $II_SYSTEM/ingres/bin and $II_SYSTEM/ingres/utility must be in $PATH
#  $II_SYSTEM/ingres/lib must be in $LD_LIBRARY_PATH or equivalent

#  II_DISTRIBUTION would be /cdrom/ingres.tar
#  PRODUCTS for TNG base would be "tm,dbms,net,ome"
#  II_TIMEZONE_NAME for the east coast NA-EASTERN
#  LOG_BYTES in megabytes for log file size

#  Exit values
#	0	Success
#	1-10	Fail

[ -z "$6" ] &&
{
	# All parameters must be passed
	# echo "Usage: ingbatch.sh II_DISTRIBUTION PRODUCTS II_INSTALLATION II_TIMEZONE_NAME LOG_BYTES PASSWORD"
	exit 1
}

[ -z "$II_SYSTEM" ] &&
{
	# II_SYSTEM must be set.
	# echo "II_SYSTEM is not set."
	exit 2
}

cd $II_SYSTEM/ingres

# Full path of the ingres tar file.
II_DISTRIBUTION=$1
export II_DISTRIBUTION

tar xpf $II_DISTRIBUTION install

II_MANIFEST_DIR=$II_SYSTEM/ingres/install
export II_MANIFEST_DIR;

# List of products to install
PRODUCTS=$2

# Pull the files off the distribution without running setup
install/ingbuild -nosetup -install=$PRODUCTS $II_DISTRIBUTION > /dev/null

II_INSTALLATION=$3
export II_INSTALLATION

ingsetenv II_TIMEZONE_NAME $4

# Don't run with dual logs if not using multiple devices.
II_DUAL_LOG=""
export II_DUAL_LOG

# Default log file size may be too small
LOG_KBYTES=$5
export LOG_KBYTES

# Run setup in batch mode
utility/iisudbms -batch >/dev/null	|| exit 5
utility/iisunet -batch	>/dev/null	|| exit 6
utility/iisutm >/dev/null		|| exit 7

# mkvalidpw must be run as root

# Change installation parameters to meet TNG's need.

. iisysdep
CONFIG_HOST=`echo $HOSTNAME | sed "s/[.]/_/g"`

iisetres	ii.$CONFIG_HOST.dbms."*".default_page_size		4096
iisetres	ii.$CONFIG_HOST.dbms."*".max_tuple_length		4016
iisetres	ii.$CONFIG_HOST.dbms."*".qef_qep_mem		25600
iisetres	ii.$CONFIG_HOST.dbms."*".qef_sort_mem		204800
iisetres	ii.$CONFIG_HOST.dbms."*".rule_depth		200
iisetres	ii.$CONFIG_HOST.dbms.private."*".cache.p4k_status	ON

# Manually change log file size, Ingres must be down.

csinstall
rcpstat -online && exit 11
# Do not continue if Ingres is online.
II_LOG_FILE=`ingprenv II_LOG_FILE`
II_LOG_FILE_NAME=`ingprenv II_LOG_FILE_NAME`
rm $II_LOG_FILE/ingres/log/$II_LOG_FILE_NAME
iisetres ii.$CONFIG_HOST.rcp.file.kbytes $LOG_KBYTES
iimklog > /dev/null 2>&1
csinstall
rcpconfig -force_init_log -silent
ipcclean

# Add specials users needed for TNG

ingstart >/dev/null 2>&1 ||
{
	# Ingres must be started to add users
	ingstop >/dev/null 2>&1
	exit 8
}

sql -s -uingres iidbdb  << ! | grep '^E' >/dev/null &&

CREATE USER admin WITH
  NOGROUP,
  PRIVILEGES=(CREATEDB,TRACE,SECURITY,MAINTAIN_LOCATIONS,OPERATOR,AUDITOR,
		MAINTAIN_AUDIT,MAINTAIN_USERS),
  DEFAULT_PRIVILEGES=(CREATEDB,TRACE,SECURITY,MAINTAIN_LOCATIONS,OPERATOR,
		AUDITOR,MAINTAIN_AUDIT,MAINTAIN_USERS),
  NOEXPIRE_DATE,
  NOPROFILE,
  NOSECURITY_AUDIT,
  PASSWORD='secret';
\go

CREATE USER tngsa WITH
  NOGROUP,
  PRIVILEGES=(CREATEDB,TRACE,SECURITY,MAINTAIN_LOCATIONS,OPERATOR,AUDITOR,
		MAINTAIN_AUDIT,MAINTAIN_USERS),
  DEFAULT_PRIVILEGES=(CREATEDB,TRACE,SECURITY,MAINTAIN_LOCATIONS,OPERATOR,
		AUDITOR,MAINTAIN_AUDIT,MAINTAIN_USERS),
  NOEXPIRE_DATE,
  NOPROFILE,
  NOSECURITY_AUDIT,
  PASSWORD='tngadmin';
\go

CREATE USER tng WITH
  NOGROUP,
  NOPRIVILEGES,
  NODEFAULT_PRIVILEGES,
  NOEXPIRE_DATE,
  NOPROFILE,
  NOSECURITY_AUDIT,
  PASSWORD='tnguser';
\go

CREATE USER aworks WITH
  NOGROUP,
  NOPRIVILEGES,
  NODEFAULT_PRIVILEGES,
  NOEXPIRE_DATE,
  NOPROFILE,
  NOSECURITY_AUDIT;
\go

CREATE GROUP tngadmin WITH
  USERS=(tngsa, admin, aworks);
\go

CREATE GROUP tnguser WITH
  USERS=(tng);
\go

\commit 

!
{
	# Users failed to create
	exit 9
}

# Create an installation password

VERSION=`$II_SYSTEM/ingres/install/ingbuild -version=net`
RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`
PASSWORD=$6

stty echo
if netutil -file- << !
C G login $CONFIG_HOST * $PASSWORD
!
    then
	iisetres ii.$CONFIG_HOST.setup.passwd.$RELEASE_ID on
    else
	#Unable to create installation password due to "netutil" failure.
	iisetres ii.$CONFIG_HOST.setup.passwd.$RELEASE_ID off
	exit 10
      fi

ingstop > /dev/null 2>&1

exit 0
