:
#  Copyright (c) 2004 Ingres Corporation
#
#  Name:
#       install.sh -- Install script for Ingres data service
#		       for Sun Cluster HA
#
#  Usage:
#       install.sh 
#
#  Description:
#       This program is designed to work in the SUN HA Cluster.
#
#       It is assumed that the ingres environment is setup.
#
#  Exit Value:
#       0       the requested operation succeeded.
#       1       the requested operation failed.
#
## History:
##      15-Sep-2005 (bonro01)
##          Created for SUN HA cluster support.
##	27-Jan-2006 (bonro01)
##	    Rename for Ingres Corporation
##	24-Jan-2008 (bonro01)
##	    Update install script to add additional instructions and
##	    also make the prompts more intuitive.
##	16-Apr-2008 (bonro01)
##	    Allow overlay upgrade of failover scripts.
##	20-May-2008 (bonro01)
##	    Install files with users uid/gid.  Install is normally done
##	    as root so the files should be owned by root.
##
#  DEST = utility
#----------------------------------------------------------------------------

#
# Step 1
#
install_ha_option()
{
cat << !

Installing the Ingres High Availability Option package.

The Ingres High Availability Option files need to be accessible
from all nodes in the cluster. You may choose to install these files
once on a cluster filesystem or you may install them in a
local filesystem on each of the cluster nodes.
The default is to install HA option scripts on a local filesystem which means
that these files will need to be installed on each failover node.

These files should be installed on each node in the cluster in the same
directory path before proceeding to Step 2.

The default installation directory is:
$INSTALL_DIRECTORY

!

prompt "Do you want to change the default installation directory?" n &&
{
    echo "Enter the installation directory"
    read INSTALL_DIRECTORY junk
}

if [ ! -d $INSTALL_DIRECTORY ]; then
    mkdir -p $INSTALL_DIRECTORY ||
    {
	echo "ERROR: Unable to create directory $INSTALL_DIRECTORY"
	return
    }
fi

(cd $INSTALL_DIRECTORY; zcat $BASEDIR/SCAgent.tar.Z | tar xovf - )

sed -e 's:(INSTALL_DIRECTORY):'$INSTALL_DIRECTORY':g' \
    $INSTALL_DIRECTORY/etc/$INGRES_RESTYPE > \
    $RES_TYPE_DIR/$INGRES_RESTYPE

cat << !

The Ingres resource type $INGRES_RESTYPE has been copied to
$RES_TYPE_DIR/$INGRES_RESTYPE

The Ingres High Availability Option files have been installed.

!
}

#
# Step 2
#
install_resource_type()
{
cat << !

Installing the $INGRES_RESTYPE resource type.

The $INGRES_RESTYPE resource type will be installed on the cluster
in order to define Ingres High Availability failover resource groups.

The $INGRES_RESTYPE resource type can only be installed once 
per cluster.

!

prompt "Do you want to continue this setup procedure?" y || return

resourcetype=$RES_TYPE_DIR/$INGRES_RESTYPE
while [ ! -f "$resourcetype" ]
do
    echo "The file $resourcetype does not exist."
    echo
    echo "Enter the path for $INGRES_RESTYPE"
    read resourcetype junk
done

scrgadm -a -t $INGRES_RESTYPE -f $resourcetype
result=$?

if [ $result -eq 13 ]; then
    cat << !
The Ingres data service is already registered.
!
    return
elif [ $result -ne 0 ]; then
    cat << !
The Ingres data service could not be registered.
!
    return
fi

cat << !

The Ingres data service has been registered.

!
}

#
# Step 3
#
configure_ingres()
{
cat << !

Configuring the Ingres installation.

The Ingres installation at $II_SYSTEM
will be configured to allow it to be executed from the failover
nodes in the cluster. This involves setting the CBF parameters that are
stored in config.dat to be cluster wide options by creating an Ingres logical
hostname to reference the Ingres installation and renaming all the config.dat
options to the logical hostname.  The Ingres environment variable II_HOSTNAME
will then be set to the Ingres logical hostname.

This Ingres installation must be shutdown on all nodes in the cluster
in order to run this procedure.

!

prompt "Do you want to continue this setup procedure?" y || return

SERVER_HOST=`ingprenv II_HOSTNAME`
if [ -n "$SERVER_HOST" ] ; then
    cat << !

The Ingres Intelligent DBMS installation located at:

        $II_SYSTEM
 
is already set up to run as logical host:

        $SERVER_HOST
!
    pause
    return
fi

cat << !

This Ingres installation must be shutdown on all nodes in the cluster
in order to run this procedure.

!

prompt "Is this Ingres installation shutdown?" y || return

SERVER_HOST=`iigetres ii."*".config.server_host` || 
{
    echo "ERROR: unable to query config.server_host"
    exit 1
}

iisulock "Ingres Sun Cluster HA setup" || exit 1

(PROG1PRFX)echonn "Please enter a logical hostname: "
read II_HOSTNAME junk
cp $(PROG3PRFX)_CONFIG/config.dat $(PROG3PRFX)_CONFIG/config.bck
sed "s/$SERVER_HOST/$II_HOSTNAME/g" \
    $(PROG3PRFX)_CONFIG/config.dat \
    > /tmp/config.$$
cp /tmp/config.$$  $(PROG3PRFX)_CONFIG/config.dat
rm -f $II_SYSTEM/ingres/files/config.lck /tmp/config.$$
(PROG2PRFX)setenv II_HOSTNAME $II_HOSTNAME
iisetres ii."*".config.server_host $II_HOSTNAME
iisetres ii.$II_HOSTNAME.gcn.local_vnode $II_HOSTNAME
cat << !

Ingres logical hostname setup complete.
!

}

#
# Step 4
#
create_resource_group()
{
cat << !

Creating failover resource group and resource name.

A user defined resource group and resource name will be defined.  A resource
group contains all the resources that will failover from one node to another.
This install only defines one resource for the Ingres installation and
optionally a logical IP or hostname for the Ingres installation.
Any other resources that need to failover for this Ingres installation will
need to be defined manually.

As part of the resource name definition, you will be given the option of
setting the parameters to create a database probe which will periodically
query the database to determine if it is still accessible.  This setup is
optional, and may be defined later manually.

!

prompt "Do you want to continue this setup procedure?" y || return

II_HOSTNAME=`(PROG2PRFX)prenv II_HOSTNAME`

[ -z "$II_OWNER" ] &&
{
    if [ -x ${II_SYSTEM}/ingres/utility/ingstart ] ; then
	II_OWNER=`/bin/ls -ld ${II_SYSTEM}/ingres/utility/ingstart |  $NAWK '{print $3}'`
    fi
}

II_INSTALLATION=`(PROG2PRFX)prenv II_INSTALLATION`

RESOURCEGROUP_NAME_DEF="Ingres_${II_INSTALLATION}_group"
cat << !

The default Ingres resource group name is $RESOURCEGROUP_NAME_DEF
!
(PROG1PRFX)echonn "Enter the name of the Ingres resource group: [$RESOURCEGROUP_NAME_DEF] "
read RESOURCEGROUP_NAME junk
if [ -z "$RESOURCEGROUP_NAME" ]; then
    RESOURCEGROUP_NAME=$RESOURCEGROUP_NAME_DEF
fi
export RESOURCEGROUP_NAME

RESOURCE_NAME_DEF=Ingres_$II_INSTALLATION
cat << !

The default Ingres resource name is $RESOURCE_NAME_DEF
!
(PROG1PRFX)echonn "Enter a resource name for the Ingres Installation: [$RESOURCE_NAME_DEF] "
read RESOURCE_NAME junk
if [ -z "$RESOURCE_NAME" ]; then
    RESOURCE_NAME=$RESOURCE_NAME_DEF
fi
export RESOURCE_NAME

cat << !

A database status probe can be defined to periodically connect to a specified
database to determine if the database continues to be accessible.

To define a database status probe you must specify a Database and Table to
probe and a Userid to connect with.

!

prompt "Do you want to define a database status probe now?" y &&
{

    Database=
    Table=
    User=

    while [ "$Database" = "" ]
    do
	(PROG1PRFX)echonn "Please enter the name of the database to be probed: "
	read Database junk
    done

    while [ "$Table" = "" ]
    do
	(PROG1PRFX)echonn "Please enter the name of the table to be probed: "
	read Table junk
    done

    while [ "$User" = "" ]
    do
	(PROG1PRFX)echonn "Please enter the userid of the probe database owner: "
	read User junk
    done

}

scrgadm -a -g $RESOURCEGROUP_NAME 
if [ $? -ne 0 ]; then
    cat << !
Creating the resource group failed.  
The Ingres data service will not be registered.
!
    return
else
    cat << !
The Ingres resource group $RESOURCEGROUP_NAME was created.
!
fi

if [ -z "$Database" -o -z "$Table" -o -z "$User" ] ; then

    if [ -n "$II_HOSTNAME" ]; then
	scrgadm -a -j $RESOURCE_NAME -g $RESOURCEGROUP_NAME \
	    -t $INGRES_RESTYPE \
	    -x II_SYSTEM=$II_SYSTEM \
	    -x II_OWNER=$II_OWNER \
	    -x II_HOSTNAME=$II_HOSTNAME 
    else
	scrgadm -a -j $RESOURCE_NAME -g $RESOURCEGROUP_NAME \
	    -t $INGRES_RESTYPE \
	    -x II_SYSTEM=$II_SYSTEM \
	    -x II_OWNER=$II_OWNER
    fi
else
    if [ -n "$II_HOSTNAME" ]; then
    scrgadm -a -j $RESOURCE_NAME -g $RESOURCEGROUP_NAME \
	-t $INGRES_RESTYPE \
	-x II_SYSTEM=$II_SYSTEM \
	-x II_OWNER=$II_OWNER \
	-x User=$User \
	-x Table=$Table \
	-x Database=$Database \
	-x II_HOSTNAME=$II_HOSTNAME 
    else
    scrgadm -a -j $RESOURCE_NAME -g $RESOURCEGROUP_NAME \
	-t $INGRES_RESTYPE \
	-x II_SYSTEM=$II_SYSTEM \
	-x II_OWNER=$II_OWNER \
	-x User=$User \
	-x Table=$Table \
	-x Database=$Database
    fi
fi
if [ $? -ne 0 ]; then
    cat << !
Creating the Ingres resource failed.  
The Ingres data service will not be registered.
!
    return
else
    cat << !
The Ingres resource $RESOURCE_NAME was created.
!
fi


cat << !

The Ingres failover resource group has been registered.

!
}

#
# Step 5
#
add_logical_hostname()
{
cat << !

Adding logical hostname to Ingres resource group.

The failover resource group should contain a logical hostname
resource for the Ingres resource group so that Ingres clients
can access the Ingres data service without knowing the physical
hostname that Ingres is running on.

!

prompt "Do you want to continue this setup procedure?" y || return

while [ "$RESOURCEGROUP_NAME" = "" ]
do
    (PROG1PRFX)echonn "Enter the name of the Ingres resource group : "
    read RESOURCEGROUP_NAME junk
done

while [ "$LOGICAL_HOST_IP" = "" ]
do
    echo "A logical hostname resource will be added to $RESOURCEGROUP_NAME"
    (PROG1PRFX)echonn "Enter the logical hostname or IP address : "
    read LOGICAL_HOST_IP junk
done

II_INSTALLATION=`(PROG2PRFX)prenv II_INSTALLATION`
II_HOSTNAME=`(PROG2PRFX)prenv II_HOSTNAME`

LOGICAL_HOSTNAME=Ingres_${II_INSTALLATION}_${II_HOSTNAME}
scrgadm -a -L -j $LOGICAL_HOSTNAME -g $RESOURCEGROUP_NAME -l $LOGICAL_HOST_IP
if [ $? -ne 0 ]; then
    cat << !

Adding the Ingres logical hostname resource failed.  
!
    return
else
    cat << !

The Ingres logical hostname resource $LOGICAL_HOSTNAME
was added to group $RESOURCEGROUP_NAME.
!
fi

}

#=[ MAIN ]========================================================
#
SELF=`basename $0`
BASEDIR=`dirname $0`
[ "$BASEDIR" = "." ] && BASEDIR=`pwd`
NAWK=/usr/bin/nawk
INSTALL_DIRECTORY="/opt/Ingres/IngresSCAgent"
INGRES_RESTYPE="Ingres.ingres_server"
RES_TYPE_DIR=/usr/cluster/lib/rgm/rtreg
install_dir=false

# Ensure cluster executables are in PATH
if [ -x /usr/cluster/bin/scinstall ]; then
    PATH=$PATH:/usr/cluster/bin
fi

#
# Determine if Sun Cluster Software is installed.
#
scinstall -p > /dev/null
if [ $? -ne 0 ]; then
    cat << !
The Sun Cluster environment does not appear to be setup. 
The Ingres data service will not be registered.
!
    exit 1
fi

#
# Do a basic sanity check (II_SYSTEM set to a directory).
#
[ -z $II_SYSTEM -o ! -d $II_SYSTEM ] &&
{
    cat << !

II_SYSTEM must be set to the Ingres root directory before running $SELF.

Please correct your environment before running $SELF again.

!
    exit 1
}

. iisysdep

. iishlib


while true
do
    cat << !

Installation Utility for Ingres High Availability Option for Sun

1. Install Ingres High Availability Option package files.
2. Install $INGRES_RESTYPE resource type.

3. Configure Ingres installation.
4. Create resource group containing an Ingres installation
5. Add the cluster logical hostname to the Ingres resource group.

6. Perform ALL installation tasks.

X. Exit install.

!

    (PROG1PRFX)echonn "Enter the required task number: "
    read choice junk

[ "$choice" = 1 -o "$choice" = 6 ] &&
{
    install_ha_option
    [ $choice = 1 ] && continue
}
[ "$choice" = 2 -o "$choice" = 6 ] &&
{
    install_resource_type
    [ $choice = 2 ] && continue
}
[ "$choice" = 3 -o "$choice" = 6 ] &&
{
    configure_ingres
    [ $choice = 3 ] && continue
}
[ "$choice" = 4 -o "$choice" = 6 ] &&
{
    create_resource_group
    [ $choice = 4 ] && continue
}
[ "$choice" = 5 -o "$choice" = 6 ] &&
{
    add_logical_hostname
    continue
}
[ "$choice" = X -o "$choice" = x ] && break

cat << !
ERROR: Unknown selection $choice
!

done


