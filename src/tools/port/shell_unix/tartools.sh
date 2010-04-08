:
#
# Copyright (c) 2004 Ingres Corporation
#
#
# Create a tar-file of test tools utilities
#
## History
##	09-mar-93 (barbh)
##		Created.
##	10-mar-93 (dianeh)
##		Added error-checking; added argument for specifying output
##		filename; deleted ckpdbmasks, rcompall, sepfiles, and
##		zecho (these don't exist); alphabetized list of files;
##		added History; added colon as first line (required for
##		all shell scripts).
##	06-may-93 (dianeh)
##		Brought over from 6.4. (Note: For now, ckpdb, cts, mts
##		not yet available under 6.5)
##	16-aug-93 (barbh)
##		Removed achilles related files, they will no longer be in
##		tools saveset. Removed rtchkenv, ckpdbmasks, sepfiles, these
##		are not needed in tools saveset.
##	13-dec-93 (dianeh)
##		Added duvetest to list of deliverables.
##	13-dec-93 (dianeh)
##		Oops -- duvetest lives in $ING_BUILD/bin, not $ING_SRC/bin.
##	08-feb-94 (vijay)
##		duvetest is actually called testvdb.
##	24-Oct-94 (GordonW)
##	25-Oct-94 (GordonW)
##		add all the achilles programs.
##	25-Oct-94 (GordonW)
##		missing files. (ac007, knowledge).
##	09-DEc-94 (GordonW)
##		Added more man pages.
##	27-Feb-95 (hanch04)
##		Added audit feature to check to see if the tools exist.
##		Changed tar to tar from list of tools.
##	12-Mar-95 (hanch04)
##		tar should be done with -C, we don't want the whole path
##		new files should be added to both lists.
##	14-dec-95 (hanch04)
##		added makekey, makekey.cnf
##      29-aug-97 (musro02)
##              Various echoed lines are misleading.
##              -C option of tar isn't documented/doesn't work on all platforms.
##              Maintaining 2 nearly identical file name lists is error-prone.
##              Copy everything in tarlist to a temp dir and tar from there.
##	11-jan-1999 (hanch04)
##		removed makekey, makekey.cnf, now using CA licenses.
##		removed msfcclup, msfccrea, msfcsync  
##	03-mar-1999 (rosga02)
##		added missing tmp001, tmp002, tmp003 for Calltrack testing
##	15-mar-1999 (rosga02)
##		correction to the prior change, removed invisible space after \
##	22-mar-1999 (walro03)
##		Removed duplicate bgrp7, bgrp7.1, updperuse entries.
##	15-apr-1999 (walro03)
##		Added msfcdumy, part of the Multi-Server Fast Commit test suite.
##      27-Apr-1999 (merja01)
##              Added qawtl.
##	08-Dec-1999 (bonro01)
##		Added audit return code statements i.e. auditrc=0 auditrc=1
##	19-May-2000 (hanch04)
##		Added resettimezone
##	27-apr-00 (hayke02)
##		We now set and use $noise (/$ING_VERS/src) if $ING_VERS exists
##		to correctly extract the files in testtoool!sep!files for
##		both baroque and non-baroque source trees. This change
##		fixes bug 101347.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	27-Jul-2004 (vande02)
##		Removing some old testing tools.
##	26-Aug-2004 (hanje04)
##		Remove more unwanted tools from archive.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
##	03-Nov-2004 (sheco02)
##	    Fixed the wrong diretory path for tools in unix.
##	28-Oct-2004 (hanje04)
## 	    Update file locations
##	28-Nov-2004 (hanje04)
## 	    Update file locations again
##	17-Dec-2008 (bonro01)
##	    Remove byteswap.1
#

[ $SHELL_DEBUG ] && set -x

: {$ING_BUILD?"must be set"}
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
 $ING_SRC/bin/qasetuser \
 $ING_SRC/bin/qawtl \
 $ING_SRC/bin/tac \
 $ING_SRC/bin/authnetu \
 $ING_SRC/bin/ckpdb1_check \
 $ING_SRC/bin/ckpdb1_load \
 $ING_SRC/bin/ckpdb_abort \
 $ING_SRC/bin/ckpdb_append \
 $ING_SRC/bin/ckpdb_delapp \
 $ING_SRC/bin/ckpdbtst \
 $ING_SRC/bin/cksepenv \
 $ING_SRC/bin/executor \
 $ING_SRC/bin/ferun \
 $ING_SRC/bin/listexec \
 $ING_SRC/bin/peditor \
 $ING_SRC/bin/qasetuserfe \
 $ING_SRC/bin/qasetusertm \
 $ING_SRC/bin/resettimezone \
 $ING_SRC/bin/run \
 $ING_SRC/bin/sep \
 $ING_SRC/bin/sepada \
 $ING_SRC/bin/sepcc \
 $ING_SRC/bin/sepcob \
 $ING_SRC/bin/sepesqlc \
 $ING_SRC/bin/sepfortr \
 $ING_SRC/bin/seplib \
 $ING_SRC/bin/seplnk \
 $ING_SRC/bin/sepperuse \
 $ING_SRC/bin/updperuse \
 $ING_SRC/bin/compress \
 $ING_SRC/man1/VERS.1 \
 $ING_SRC/man1/addrfmt.1 \
 $ING_SRC/man1/align.1 \
 $ING_SRC/man1/altcompare.1 \
 $ING_SRC/man1/arc_lock.1 \
 $ING_SRC/man1/arc_unlock.1 \
 $ING_SRC/man1/argeval.1 \
 $ING_SRC/man1/argloc.1 \
 $ING_SRC/man1/argorder.1 \
 $ING_SRC/man1/bang.1 \
 $ING_SRC/man1/beingres.1 \
 $ING_SRC/man1/bitsin.1 \
 $ING_SRC/man1/buildrel.1 \
 $ING_SRC/man1/ccpp.1 \
 $ING_SRC/man1/chtim.1 \
 $ING_SRC/man1/clningtst.1 \
 $ING_SRC/man1/clntstabf.1 \
 $ING_SRC/man1/clntstdbs.1 \
 $ING_SRC/man1/compress.1 \
 $ING_SRC/man1/context.1 \
 $ING_SRC/man1/copyconv.1 \
 $ING_SRC/man1/copyvec.1 \
 $ING_SRC/man1/dirname.1 \
 $ING_SRC/man1/each.1 \
 $ING_SRC/man1/ensure.1 \
 $ING_SRC/man1/getextra.1 \
 $ING_SRC/man1/grepall.1 \
 $ING_SRC/man1/headers.1 \
 $ING_SRC/man1/includes.1 \
 $ING_SRC/man1/ingman.1 \
 $ING_SRC/man1/louf.1 \
 $ING_SRC/man1/machine.1 \
 $ING_SRC/man1/mantmplt.1 \
 $ING_SRC/man1/mathsigs.1 \
 $ING_SRC/man1/maxfiles.1 \
 $ING_SRC/man1/maxstack.1 \
 $ING_SRC/man1/maxvals.1 \
 $ING_SRC/man1/mergelibs.1 \
 $ING_SRC/man1/mkbzarch.1 \
 $ING_SRC/man1/mkfecat.1 \
 $ING_SRC/man1/mkfill.1 \
 $ING_SRC/man1/mkidir.1 \
 $ING_SRC/man1/mkman.1 \
 $ING_SRC/man1/mksecret.1 \
 $ING_SRC/man1/mksysdep.1 \
 $ING_SRC/man1/mkw4cat.1 \
 $ING_SRC/man1/mkw4dir.1 \
 $ING_SRC/man1/mkw4vers.1 \
 $ING_SRC/man1/needs.1 \
 $ING_SRC/man1/nullderef.1 \
 $ING_SRC/man1/offend.1 \
 $ING_SRC/man1/overwrite.1 \
 $ING_SRC/man1/picall.1 \
 $ING_SRC/man1/project.1 \
 $ING_SRC/man1/rcompall.1 \
 $ING_SRC/man1/rofix.1 \
 $ING_SRC/man1/salign.1 \
 $ING_SRC/man1/sep.1 \
 $ING_SRC/man1/sepperuse.1 \
 $ING_SRC/man1/sizeof.1 \
 $ING_SRC/man1/symlink.1 \
 $ING_SRC/man1/sysint.1 \
 $ING_SRC/man1/tac.1 \
 $ING_SRC/man1/unschar.1 \
 $ING_SRC/man1/updperuse.1 \
 $ING_SRC/man1/w4bang.1 \
 $ING_SRC/man1/whatunix.1 \
 $ING_SRC/man1/which.1 \
 $ING_SRC/man1/whoami.1 \
 $ING_SRC/man1/yyfix.1 \
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
