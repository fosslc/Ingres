:
#
# Copyright (c) 2004 Ingres Corporation
#
#
# Create a tar-file of test tools utilities
#
## History
##    22-Sep-2004 (hanje04)
##	 Created from tartools.sh v26 as opensource tools are only a subset
##	 of those needed by QA
##    19-Nov-2004 (sheco02)
##       Modify the directory path after Windows Jam files merge.
##    16-Dec-2004 (sheco02)
##       Update the directory path to get ctsslave and ctscoord.
##    10-Feb-2005 (bonro01)
##       Remove obsolete files.
##    29-Jul-2008 (bonro01)
##       Add new stress test executables.  Remove obsolete man files.

[ $SHELL_DEBUG ] && set -x

: {$ING_BUILD?"must be set"}
: {$ING_TOOLS?"must be set"}
: {$ING_SRC?"must be set"}
[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

tarfile=""
tarlist=""
compress_it=false
audit=false
while [ ! "$1" = "" ]
do
	if [ "$1" = "-compress" ]
	then
		compress_it=true
	elif [ "$1" = "-nocompress" ]
	then
		compress_it=false
	elif [ "$1" = "-a" ]
	then
		audit=true
	else
		tarfile=$1
	fi
	[ "$2" = "" ] && break
	shift
done

for file in \
 $ING_BUILD/bin/deleter \
 $ING_BUILD/bin/testvdb \
 $ING_TOOLS/bin/authnetu \
 $ING_TOOLS/bin/ckpdb1_check \
 $ING_TOOLS/bin/ckpdb1_load \
 $ING_TOOLS/bin/ckpdb_abort \
 $ING_TOOLS/bin/ckpdb_append \
 $ING_TOOLS/bin/ckpdb_delapp \
 $ING_TOOLS/bin/ckpdbtst \
 $ING_TOOLS/bin/cksepenv \
 $ING_TOOLS/bin/cts \
 $ING_TOOLS/bin/ctscoord \
 $ING_TOOLS/bin/ctsslave \
 $ING_TOOLS/bin/executor \
 $ING_TOOLS/bin/ferun \
 $ING_TOOLS/bin/getseed \
 $ING_TOOLS/bin/listexec \
 $ING_TOOLS/bin/msfcchk \
 $ING_TOOLS/bin/msfccldb \
 $ING_TOOLS/bin/msfcdrv \
 $ING_TOOLS/bin/msfcdumy \
 $ING_TOOLS/bin/msfcrun \
 $ING_TOOLS/bin/msfcset \
 $ING_TOOLS/bin/msfcslv \
 $ING_TOOLS/bin/msfctest \
 $ING_TOOLS/bin/mts \
 $ING_TOOLS/bin/mtscoord \
 $ING_TOOLS/bin/mtsslave \
 $ING_TOOLS/bin/peditor \
 $ING_TOOLS/bin/qasetuser \
 $ING_TOOLS/bin/qasetuserfe \
 $ING_TOOLS/bin/qasetusertm \
 $ING_TOOLS/bin/qawtl \
 $ING_TOOLS/bin/readlog \
 $ING_TOOLS/bin/resettimezone \
 $ING_TOOLS/bin/run \
 $ING_TOOLS/bin/sep \
 $ING_TOOLS/bin/sepada \
 $ING_TOOLS/bin/sepcc \
 $ING_TOOLS/bin/sepcob \
 $ING_TOOLS/bin/sepesqlc \
 $ING_TOOLS/bin/sepfortr \
 $ING_TOOLS/bin/seplib \
 $ING_TOOLS/bin/seplnk \
 $ING_TOOLS/bin/sepperuse \
 $ING_TOOLS/bin/strsvr \
 $ING_TOOLS/bin/tac \
 $ING_TOOLS/bin/updperuse \
 $ING_TOOLS/bin/achilles \
 $ING_TOOLS/bin/acconfig \
 $ING_TOOLS/bin/acstart \
 $ING_TOOLS/bin/acstop \
 $ING_TOOLS/bin/acreport \
 $ING_TOOLS/bin/ACerror \
 $ING_TOOLS/bin/ACmsg \
 $ING_TOOLS/bin/ACsleep \
 $ING_TOOLS/bin/droptables \
 $ING_TOOLS/bin/compress \
 $ING_TOOLS/bin/ach_stress \
 $ING_TOOLS/bin/run_stress \
 $ING_TOOLS/bin/setup_stress \
 $ING_TOOLS/bin/ac001 \
 $ING_TOOLS/bin/ac002 \
 $ING_TOOLS/bin/ac003 \
 $ING_TOOLS/bin/ac004 \
 $ING_TOOLS/bin/ac005 \
 $ING_TOOLS/bin/ac006 \
 $ING_TOOLS/bin/ac007 \
 $ING_TOOLS/bin/ac008 \
 $ING_TOOLS/bin/ac011 \
 $ING_TOOLS/bin/tp1load \
 $ING_TOOLS/bin/ach_actests \
 $ING_TOOLS/bin/run_actests \
 $ING_TOOLS/bin/setup_actest \
 $ING_TOOLS/bin/bgrp1 \
 $ING_TOOLS/bin/bgrp2 \
 $ING_TOOLS/bin/bgrp3 \
 $ING_TOOLS/bin/bgrp4 \
 $ING_TOOLS/bin/bgrp5 \
 $ING_TOOLS/bin/bgrp6 \
 $ING_TOOLS/bin/bgrp7 \
 $ING_TOOLS/bin/bgrp8 \
 $ING_TOOLS/bin/bgrp9 \
 $ING_TOOLS/bin/b_buginfo \
 $ING_TOOLS/bin/b_versinfo \
 $ING_TOOLS/bin/p_patch \
 $ING_TOOLS/bin/c_tsinfo \
 $ING_TOOLS/bin/ach_bugs \
 $ING_TOOLS/bin/run_bugs \
 $ING_TOOLS/bin/setup_bugs \
 $ING_TOOLS/bin/ctrp1 \
 $ING_TOOLS/bin/ctrp2 \
 $ING_TOOLS/bin/ctrp3 \
 $ING_TOOLS/bin/ctrp4 \
 $ING_TOOLS/bin/ctrp5 \
 $ING_TOOLS/bin/callmix \
 $ING_TOOLS/bin/grab \
 $ING_TOOLS/bin/handle \
 $ING_TOOLS/bin/knowledge \
 $ING_TOOLS/bin/yourcall \
 $ING_TOOLS/bin/newcall \
 $ING_TOOLS/bin/tmp001 \
 $ING_TOOLS/bin/tmp002 \
 $ING_TOOLS/bin/tmp003 \
 $ING_TOOLS/bin/ach_callt \
 $ING_TOOLS/bin/run_callt \
 $ING_TOOLS/bin/setup_callt \
 $ING_TOOLS/bin/activities \
 $ING_TOOLS/bin/con_fido \
 $ING_TOOLS/bin/empty_fido \
 $ING_TOOLS/bin/load_fido \
 $ING_TOOLS/bin/make_fido \
 $ING_TOOLS/bin/modify_fido \
 $ING_TOOLS/bin/rules_fido \
 $ING_TOOLS/bin/fido \
 $ING_TOOLS/bin/ach_fido \
 $ING_TOOLS/bin/run_fido \
 $ING_TOOLS/bin/setup_fido \
 $ING_TOOLS/bin/itbload \
 $ING_TOOLS/bin/itbmaster \
 $ING_TOOLS/bin/itbdriver \
 $ING_TOOLS/bin/itb \
 $ING_TOOLS/bin/itbrun \
 $ING_TOOLS/bin/ach_itb \
 $ING_TOOLS/bin/run_itb \
 $ING_TOOLS/bin/setup_itb \
 $ING_TOOLS/bin/mayo \
 $ING_TOOLS/bin/ach_mayo \
 $ING_TOOLS/bin/run_mayo \
 $ING_TOOLS/bin/setup_mayo \
 $ING_TOOLS/bin/pooltest \
 $ING_TOOLS/bin/poolbuild \
 $ING_TOOLS/bin/ach_pool \
 $ING_TOOLS/bin/run_pool \
 $ING_TOOLS/bin/setup_pool \
 $ING_TOOLS/bin/access \
 $ING_TOOLS/bin/accntl \
 $ING_TOOLS/bin/datatypes \
 $ING_TOOLS/bin/nettest \
 $ING_TOOLS/bin/qryproc \
 $ING_TOOLS/bin/ach_access \
 $ING_TOOLS/bin/ach_accntl \
 $ING_TOOLS/bin/ach_dtypes \
 $ING_TOOLS/bin/ach_nettest \
 $ING_TOOLS/bin/ach_qryproc \
 $ING_TOOLS/bin/run_access \
 $ING_TOOLS/bin/run_accntl \
 $ING_TOOLS/bin/run_dtypes \
 $ING_TOOLS/bin/run_nettest \
 $ING_TOOLS/bin/run_qryproc \
 $ING_TOOLS/bin/setup_access \
 $ING_TOOLS/bin/setup_accntl \
 $ING_TOOLS/bin/setup_dtypes \
 $ING_TOOLS/bin/setup_nettest \
 $ING_TOOLS/bin/setup_qryproc \
 $ING_TOOLS/bin/noabfvis \
 $ING_TOOLS/bin/runbe \
 $ING_TOOLS/bin/runcts \
 $ING_TOOLS/bin/runfe \
 $ING_TOOLS/bin/runfe3gl \
 $ING_TOOLS/bin/runfehet \
 $ING_TOOLS/bin/runfestr \
 $ING_TOOLS/bin/runjob \
 $ING_TOOLS/bin/runlbnet \
 $ING_TOOLS/bin/runmts \
 $ING_TOOLS/bin/runstar \
 $ING_TOOLS/bin/tstsetup \
 $ING_TOOLS/bin/ddlv1 \
 $ING_TOOLS/bin/insdel \
 $ING_TOOLS/bin/ordent \
 $ING_TOOLS/bin/qp1 \
 $ING_TOOLS/bin/qp3 \
 $ING_TOOLS/bin/selv1 \
 $ING_TOOLS/bin/updv1 \
 $ING_TOOLS/man1/achilles.1 \
 $ING_TOOLS/man1/ach_stress.1 \
 $ING_TOOLS/man1/run_stress.1 \
 $ING_TOOLS/man1/setup_stress.1 \
 $ING_TOOLS/man1/ac001.1 \
 $ING_TOOLS/man1/ac002.1 \
 $ING_TOOLS/man1/ac003.1 \
 $ING_TOOLS/man1/ac004.1 \
 $ING_TOOLS/man1/ac005.1 \
 $ING_TOOLS/man1/ac006.1 \
 $ING_TOOLS/man1/ac007.1 \
 $ING_TOOLS/man1/ac008.1 \
 $ING_TOOLS/man1/ac011.1 \
 $ING_TOOLS/man1/ach_actests.1 \
 $ING_TOOLS/man1/run_actests.1 \
 $ING_TOOLS/man1/setup_actest.1 \
 $ING_TOOLS/man1/bgrp1.1 \
 $ING_TOOLS/man1/bgrp2.1 \
 $ING_TOOLS/man1/bgrp3.1 \
 $ING_TOOLS/man1/bgrp4.1 \
 $ING_TOOLS/man1/bgrp5.1 \
 $ING_TOOLS/man1/bgrp6.1 \
 $ING_TOOLS/man1/bgrp7.1 \
 $ING_TOOLS/man1/bgrp8.1 \
 $ING_TOOLS/man1/bgrp9.1 \
 $ING_TOOLS/man1/b_buginfo.1 \
 $ING_TOOLS/man1/b_versinfo.1 \
 $ING_TOOLS/man1/p_patch.1 \
 $ING_TOOLS/man1/c_tsinfo.1 \
 $ING_TOOLS/man1/ach_bugs.1 \
 $ING_TOOLS/man1/run_bugs.1 \
 $ING_TOOLS/man1/setup_bugs.1 \
 $ING_TOOLS/man1/ctrp1.1 \
 $ING_TOOLS/man1/ctrp2.1 \
 $ING_TOOLS/man1/ctrp3.1 \
 $ING_TOOLS/man1/ctrp4.1 \
 $ING_TOOLS/man1/ctrp5.1 \
 $ING_TOOLS/man1/callmix.1 \
 $ING_TOOLS/man1/grab.1 \
 $ING_TOOLS/man1/handle.1 \
 $ING_TOOLS/man1/knowledge.1 \
 $ING_TOOLS/man1/yourcall.1 \
 $ING_TOOLS/man1/newcall.1 \
 $ING_TOOLS/man1/ach_callt.1 \
 $ING_TOOLS/man1/run_callt.1 \
 $ING_TOOLS/man1/setup_callt.1 \
 $ING_TOOLS/man1/make_fido.1 \
 $ING_TOOLS/man1/load_fido.1 \
 $ING_TOOLS/man1/modify_fido.1 \
 $ING_TOOLS/man1/fido.1 \
 $ING_TOOLS/man1/activities.1 \
 $ING_TOOLS/man1/ach_fido.1 \
 $ING_TOOLS/man1/run_fido.1 \
 $ING_TOOLS/man1/setup_fido.1 \
 $ING_TOOLS/man1/itb.1 \
 $ING_TOOLS/man1/itbrun.1 \
 $ING_TOOLS/man1/itbload.1 \
 $ING_TOOLS/man1/ach_itb.1 \
 $ING_TOOLS/man1/run_itb.1 \
 $ING_TOOLS/man1/setup_itb.1 \
 $ING_TOOLS/man1/pooltest.1 \
 $ING_TOOLS/man1/poolbuild.1 \
 $ING_TOOLS/man1/ach_pool.1 \
 $ING_TOOLS/man1/run_pool.1 \
 $ING_TOOLS/man1/setup_pool.1 \
 $ING_TOOLS/man1/VERS.1 \
 $ING_TOOLS/man1/compress.1 \
 $ING_TOOLS/man1/sep.1 \
 $ING_TOOLS/man1/sepperuse.1 \
 $ING_TOOLS/man1/tac.1 \
 $ING_TOOLS/man1/updperuse.1 \
 $ING_SRC/testtool/sep$noise/files_unix_win/basickeys.sep \
 $ING_SRC/testtool/sep$noise/files_unix_win/commands.sep \
 $ING_SRC/testtool/sep$noise/files_unix_win/header.txt \
 $ING_SRC/testtool/sep$noise/files_unix_win/makelocs.key \
 $ING_SRC/testtool/sep$noise/files_unix_win/septerm.sep \
 $ING_SRC/testtool/sep$noise/files_unix/termcap.sep \
 $ING_SRC/testtool/sep$noise/files_unix_win/vt100.sep \
 $ING_SRC/testtool/sep$noise/files_unix_win/vt220.sep
do
  tarlist="$tarlist $file"
done

if $audit 
then
auditrc=0
echo "Checking tools and utilities for $1"
for file in $tarlist
  do
	[ ! -r $file ] &&
	{
	    echo "$file does not exist."
	    auditrc=1
	}
  done
exit $auditrc
fi

[ "$tarfile" = "" ] &&
{
	echo "usage: tartools [-a] tarfile..."
	exit 1
}

fname=`basename $tarfile`
fdir=`dirname $tarfile`
[ "$fdir" = "." ] && fdir=`pwd`
tarfile=$fdir/$fname


V=
VS=
[ -n "$ING_VERS" ] && {
    VS="/$ING_VERS/src"
    V="/$ING_VERS"
}

echo "Populating temp dir in `pwd`"
rm -f $tarfile
mkdir ./temp
cd ./temp
for file in $tarlist
   do
      bigname=`dirname $file`
      subdir=`basename $bigname`
      if [ $subdir = "bin" ] || [ $subdir = "man1" ]
      then      [ ! -d $subdir ] &&
         mkdir $subdir
         [ -r $file ] &&
         cp $file ./$subdir
      else
      [ ! -d files ] &&
         mkdir files
      [ -r $file ] &&
         cp $file ./files
      fi
   done
echo "Creating $1"
tar cvf $tarfile ./*
cd ..
rm -fr ./temp/*
rmdir ./temp

if $compress_it
then
	echo "Compressing $tarfile..."
	compress $tarfile
fi

echo "Done creating $1"
exit 0
