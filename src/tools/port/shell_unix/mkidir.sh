:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkidir:
#
#	Set permissions on build directories; create them if they don't	exist.
#	This must be run as ingres; on sysV, this means ruid and euid.
#	May be run more than once.
#
## History:
##	24-apr-1989 (boba)
##		Add abf/DUMMY.
##	11-may-1989 (boba)
##		Add abf/.  How did I miss this?
##	31-jul-1989 (boba)
##		Move from mkidir61.sh to mkidir.sh when added to ingresug.
##	18-dec-1989 (boba)
##		Add sig/ipm to directories and remove log/DUMMY as log
##		directory is not delivered on the release tape.
##	15-jan-1990 (boba)
##		Ensure user is either ingres or rtingres.
##	14-feb-1990 (swm)
##		Added FIFODIR for BS fifos.
##	23-feb-1990 (kerry)
##		Added utility/iistream for SYS V.3 Streams Driver
##	27-mar-1990 (boba)
##		Added files/collation
##	05-apr-1990 (boba)
##		Add files/memory (new for 6.3).
##	22-may-1990 (achiu)
##		Add dmp (new for 6.3).
##		Add dmp/defaults (new for 6.3).
##	02-aug-1990 (boba)
##		Add sig/errhelp.
##	31-aug-1990 (jonb)
##		Add demo/udadts
##	23-oct-1990 (sandyd)
##		Added files/tutorial and files/dictfiles (new for 6.3/03).
##	15-feb-1991 (jonb)
##		Make demo mode 755, to stay in sync with iibuild/chklocation.
##	06-jun-1991 (sandyd)
##		Add files/chxfiles for read-in character set support.
##	24-jun-1991 (sandyd)
##		Brought sig directory set up to date.
##	16-aug-1991 (sandyd)
##		The plan for chxfiles was changed.  Now we have to make a
##		separate directory for each supported character set.
##	19-aug-1991 (sandyd)
##		A new sig directory (star) from Teresa S.
##	21-aug-91 (kirke)
##		We need to create ./files/charsets before we can create
##		any of its subdirectories.
##	22-aug-91 (sandyd)
##		The read-in charset names International gave us were illegal.
##		We now have a new list that conforms to length restrictions.
##	13-sep-91 (sandyd)
##		New sig directory (w4v3cats).
##	16-sep-91 (sandyd)
##		Yet another sig directory (iievutil).
##	23-oct-91 (sandyd)
##		And another new sig directory (w4glipc).  A demo of dbevent 
##		usage in w4gl.
##	28-oct-91 (fredv)
##		Add new charsets subdirectory for Roman8 character set.
##	27-feb-92 (sandyd)
##		Replaced old "notes" directory with new "advisor" directory,
##		at the request of Tech Support.
##	03-apr-92 (philiph)
##		Add new charsets subdirectories for iso88592, slav_852,
##		& elot_437
##		character sets.
##	14-apr-92 (philiph)
##		Removed illegal "_" from character set names.
##	18-may-92 (sandyd)
##		Added cachelock sig directory.
##	05-oct-92 (stevet)
##		Added files/zoneinfo directory.
##	18-nov-92 (sandyd for kirke)
##		Carried over new Asian language charset directories from 6.4.
##	19-nov-92 (dianeh)
##		Added vec directories; removed convto60 (gone for 6.5); added
##		work and work/default (new for 6.5).
##	02-dec-92 (lauraw)
##		Moved directory list into tools/port/conf/idirs.ccpp.
##		Also makes extra ING_SRC subdirectories now.
##		(Note that we don't create the DUMMY files anymore either).
##	10-mar-93 (vijay)
##		Create ING_BUILD directory if it does not exist.
##	23-sep-93 (dianeh)
##		Don't call ensure; just find out the user directly.
##	23-sep-93 (dianeh)
##		Oops, it's not rtiingres, it's rtingres (beats me either way).
##	27-sep-93 (www)
##		Only set permissions on directories if you just created them.
##		This is for backward compatibility to existing clients, since
##		mkidir is now called by BOOT.
##	22-oct-93 (seiwald)
##		Remove user name check: it didn't work when run setuid, and
##		the whole build has to be done as ingres - why check this 
##		one command?
##	29-nov-93 (dianeh)
##		Put in meaningful exit statuses (stati?); little cleanups.
##      29-aug-97 (musro02)
##              Add -p to "mkdir $ING_BUILD"
##	14-Jul-2004 (hanje04)
##		Add check for $II_SYSTEM/ingres->$ING_BUILD link and add it if
##		it's missing.
##	14-Jul-2004 (hanje04)
## 	 	Replace : with ? when checking II_SYSTEM (DOH!)
##	30-Jul-2004 (hanje04)
##		Create ING_SRC targets under ING_TOOLS
##	01-Aug-2004 (hanje04)
##		Don't create links if they're already there (of course).
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	11-sep-2004 (stephenb)
##	    make lp64 directories
##	18-Feb-2005 (bonro01)
##	    Create link $ING_SRC/tst->$ING_TST to build ingtest tools.
##	    Add messages for other symbolic links.
##	09-Mar-2005 (hanje04)
## 	    SIR 114034
##	    Add support for reverse hybrid builds
##	03-Mar-2006 (hanje04)
##	    BUG 115796
##	    Don't create ING_TST links here as they need to exist
##	    when mkjams is run. Now created at the start of mkjams
##	02-Nov-2006 (clach04)
##		Updated (link) -L checks to be standard posix -h checks
##		-L checks fail with built-in test in bourne shell
##		-L works with /bin/test
##	18-Jun-2009 (kschendel) SIR 122138
##	    Some changes to how hybrids work, fix here.
##	    Unconditionally recreate the bin lib man1 links in ING_SRC,
##	    it's too easy to tar source trees around and pick up an
##	    out-of-date symlink.  (Ideally they would be relative symlinks,
##	    but ING_TOOLS isn't necessarily ../tools.)
#

: ${ING_SRC?"must be set"}

CMD=`basename $0`

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

CONFDIR=$ING_SRC/tools/port$noise/conf
. readvers

[ -d ${ING_BUILD?"must be set"} ] ||
{ echo "Creating $ING_BUILD..." ; mkdir -p $ING_BUILD 2>/dev/null ||
  { echo "$CMD: Cannot create $ING_BUILD. Check permissions." ; exit 1 ; } ; }

cd $ING_BUILD
echo "Checking ING_BUILD directories..."
ccpp -Uunix -DDIR_INSTALL $CONFDIR/idirs.ccpp | while read perm dir
do
  [ -d $dir ] ||
  { echo "Creating $dir..." ; mkdir $dir 2>/dev/null ||
    { echo "$CMD: Cannot create $dir. Check permissions." ; exit 1 ; }
    chmod $perm $dir ; }
done

# Check for II_SYSTEM, create it if we need to
[ -d ${II_SYSTEM?"must be set"} ] ||
{ echo "Creating $II_SYSTEM..." ; mkdir -p $II_SYSTEM 2>/dev/null ||
  { echo "$CMD: Cannot create $II_SYSTEM. Check permissions." ; exit 1 ; } ; }

# Check for link back to $ING_BUILD, create it if we need to
[ -h ${II_SYSTEM}/ingres ] ||
{ echo 'Creating $II_SYSTEM/ingres->$ING_BUILD  link...'
  ln -s $ING_BUILD $II_SYSTEM/ingres 2>/dev/null ||
    { echo "$CMD: Cannot create link $II_SYSTEM/ingres. Check permissions." 
      exit 1 
    }
}

# make lp64 directories
if [ "$build_arch" = '32+64' ]
then
   HB=64
elif [ "$build_arch" = '64+32' ]
then
   HB=32
fi

if [ "$HB" ]
then
  echo "Checking lp${HB} directories"
  cd ${ING_SRC}
  [ -r "${ING_SRC}/tools/port/conf/hbdirs.ccpp" ] ||
  {
	echo "Cannot read ${ING_SRC}/tools/port/conf/hbdirs.ccpp"
	echo "Aborting..."
	exit 1
  }

  ccpp -Uunix $CONFDIR/hbdirs.ccpp |  while read dir
  do
    if [ -d $dir ]
    then
      [ -d $dir/lp${HB} ] ||
      {
	echo "Making directory $dir/lp${HB}"
	mkdir $dir/lp${HB}
      }
    fi
  done
fi

[ $? -eq 1 ] && exit 1

#Create $ING_SRC target directories: bin, lib, man1.
#Create then under ING_TOOLS and link them back to $ING_SRC
echo "Checking ING_TOOLS directories..."
cd $ING_TOOLS
ccpp -Uunix -DDIR_SRC $CONFDIR/idirs.ccpp |  while read perm dir
do
  [ -d $dir ] ||
  { echo "Creating $dir..." ; mkdir $dir 2>/dev/null ||
    { echo "$CMD: Cannot create $dir. Check permissions." ; exit 1 ; }
  chmod $perm $dir ; }
done

# Unconditionally recreate links from ing-src to ing-tools;  if
# the source tree has been copied, the links might be wrong.
cd $ING_SRC
rm -f bin lib man1
for d in bin lib man1
do
    { echo 'Creating $ING_SRC/'$d'->$ING_TOOLS/'$d'  link...'
	ln -s $ING_TOOLS/$d $ING_SRC ||
	{ echo "$CMD: Cannot create link $ING_SRC/${d}. Check permissions."
	  exit 1 
	}
    }
done

[ $? -eq 1 ] && exit 1
exit 0
