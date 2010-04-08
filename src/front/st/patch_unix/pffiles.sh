:
#  Copyright (c) 2004 Ingres Corporation 
#  Name: pffiles
#
## Usage:
##	pffiles	<patch package id> in | out
##
##  Description:
##	Executed in-line by ingbuild on patch installation
##		Modes:
##			'in'	- create directory $II_SYSTEM/ingres/install/<patchid>
##					- make list of files
##					- make 'before-image' (BI) copy of files in patch
##			'out'	- check existence of BI
##					- apply BI to current installation.
## History:
##   15-March-1996 (angusm)	
##		created.
##   28-apr-2000 (somsa01)
##		Enabled for different product builds.
##
#  DEST = utility
#----------------------------------------------------------------------------
GTAR=$(PRODLOC)/(PROD2NAME)/install/gtar

if [ "$(PRODLOC)" = "" ]
then
	exit 255
fi

if [ $# -lt 2 ]
then
	exit 255
fi

in=0
out=0

case $2 in
	in)		in=1;	break;;	
	out)	out=1;	break;;
	*)				exit 255;;
esac

list=$(PRODLOC)/(PROD2NAME)/install/$1/patch.list
archive=$(PRODLOC)/(PROD2NAME)/install/tmp/$1/install.tar.Z
save=$(PRODLOC)/(PROD2NAME)/install/$1/pre$1.tar
dummy=$(PRODLOC)/(PROD2NAME)/install/$1/dummy

if [ $in -eq 1 ]
then
	if [ ! -d $(PRODLOC)/(PROD2NAME)/install/$1 ]
	then
		mkdir $(PRODLOC)/(PROD2NAME)/install/$1
	fi
	{ 
		touch $list 
	} || 
	{
		cat << !

	Cannot create file list for patch 

!
		exit 255
	}
	if [ ! -f $archive ]
	then
	{
		cat << !
Cannot find patch archive for preload

!
	}
	exit 255
	fi
	uncompress < $archive | $GTAR -tf - | sed '
/\/$/d
/\-\>/d' > /tmp/1.$$
	cat /tmp/1.$$ | awk ' {print SY "/" $0 } ' SY=$(PRODLOC) > /tmp/2.$$
	for i in `cat /tmp/2.$$`
	do
		if [ -f $i ] 
		then 
			echo $i >> $list
		fi
	done
	{
		$GTAR -B -cvf $dummy -T $list >/dev/null 2>&1
	} ||
	{
		cat << !

		Error saving files before loading patch
!
	}
	tar tvf $dummy 
	compress $dummy
	mv $dummy.Z $save.Z
	\rm /tmp/1.$$ /tmp/2.$$
fi

if [ $out -eq 1 ]
then
	if [ ! -f $save.Z ]
	then
	{
		cat << !
	
	Cannot back patch out - no copy of saved files available 
!
	}
		exit 255
	fi
	cd $(PRODLOC)
	{
		uncompress < $save.Z | $GTAR -xpf -
	} ||
	{
		cat << !

		Error restoring files 

!
		exit 255
	}
fi


exit 0
