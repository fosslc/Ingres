:
#  Copyright (c) 2004 Ingres Corporation 
#  Name: pfinstall
#
#  Usage:
#		wrapper for 'ingbuild' for patch installation
#
##  Description:
##	Executed in-line by ingbuild on patch installation
#
## History:
##   15-March-1996 (angusm)	
##		created.
##   28-apr-2000 (somsa01)
##		Enabled for different product builds.
##
#  DEST = utility
#----------------------------------------------------------------------------
usage () {
echo "Usage $0 : <patchid> <device>"
}
GTAR=$(PRODLOC)/(PROD2NAME)/install/gtar

if [ $# -lt 2 ]
then
	usage
	exit 255
fi

proot=$(PRODLOC)/(PROD2NAME)/install/$1
pdev=$2

if [ ! -d $proot ]
then
	mkdir $proot
fi

cd $proot
$GTAR -xpf $pdev install || 
{
	cat << !
	Error unloading patch manifest from patch device
!
}

(PROG3PRFX)_MANIFEST_DIR=$proot/install
(PROG3PRFX)_DISTRIBUTION=$pdev
export (PROG3PRFX)_MANIFEST_DIR (PROG3PRFX)_DISTRIBUTION
$(PRODLOC)/(PROD2NAME)/install/(PROG2PRFX)build -patch || exit 255

exit 0
