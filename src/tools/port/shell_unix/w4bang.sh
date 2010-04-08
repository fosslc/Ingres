:
#
# Copyright (c) 2004 Ingres Corporation
#
# w4bang
#
# Build the w4gl libraries and executables.
#
#   History:
#	02-Oct-90 (andys)
#	    Created.
#	18-oct-90 (brett)
#	    Expanded the bang script to follow the build procedure
#	    and group headers, libraries and executables.  Added the
#	    -F and -n options to pass to ming.
#	24-oct-90 (jab)
#	    Changed the Bourne shell programming constructs to work.
#	    (For the record, 'for w in "$*"' executes at least once,
#	    even if there are no arguments; 'for w in $*' does not
#	    execute if there are no arguments.)
#	07-Nov-90 (andys)
#	    Add files_unix to list of directories containing 'special files'
#	    as it is no longer empty.
#	20-nov-90 (brett)
#	    Add front/utils/fmt to build new w4gl fmt library.
#	20-nov-90 (jab)
#	    Added cl/clf/nm on the list of libraries to build, which
#	    should've been included when the source to that directory
#	    was. (Oh, well.)
#	25-nov-90 (brett)
#	    Add front/embed/libqgca to the list of build directories.
#	15-dec-90 (brett)
#	    Add front!embed!hdr and clf/cl/gv directories.
#	15-dec-90 (brett)
#	    Add woof!hdr to the list of header directories.
#	16-dec-90 (brett)
#	    Update build directories, and procedures.
#	17-dec-90 (jab)
#	    Added cl/clf/ol to put in some 9000/300-specific code
#		without affecting the 6204p code line.
#	17-dec-90 (brett)
#	    Add directory checking code, if directory isn't there
#	    show a messge and continue, except for ercompile.
#	17-dec-90 (brett)
#	    Directory existence checks were wrong, fixed them.
#	20-dec-90 (brett)
#	    Add in code for special 3gl files.  Make the build process
#	    a little more resilient.  If there isn't a MING file then
#	    make one so things don't fail silently.
#	20-dec-90 (brett)
#	    Add the code to build the message files, under specials.
#	    Add a sanity check to make sure all the w4gl libraries
#	    are in place.
#	20-dec-90 (brett)
#	    Fix a couple bugs introduced in the last change.
#	21-dec-90 (brett)
#	    Fixed an echo statement for sph.msg.  Added echo hints to
#	    tell the user what the build script is doing.
#	09-jan-91 (brett)
#	    Add the motif libraries to the build list.
#	16-apr-91 (jab)
#	    Removed the motif libraries from the build list, and in fact,
#	    updated the build list to know about the W4GL04 list of things
#	    to make.
#	    Also, this now insists that you're running as ingres to make
#	    the "mkw4glcat" script work predictably.
#	17-apr-91 (jab)
#	    Corrected a few directory names in the script, namely, clf/gv
#	    should've been "cl/clf/gv" and so on.
#	17-apr-91 (jab)
#	    Checks to make sure that "mkw4gldir" was run before starting.
#	30-apr-91 (jab)
#	    Runs "mkdl3gl" and "w4glprerls" now.
#	3-may-91 (jab)
#	    Added "front/embed" to list of libs to make.
#	13-may-91 (jab)
#	    Added "cl/clf/dl" to list of libs to make.
#	16-may-91 (jab)
#	    Added cl/clf/specials to list of libs to make.
#	8-june-91 (jab)
#	    Added w4gl/specials to list of libs to make.
#	13-jun-91 (kimman)
#	    Moving where cl/clf/specials and w4gl/specials are made from the
#	    list of libs to list of executables, since they are MINGH and don't
#	    do anything under 'ming lib' but does do something under 'ming'.
#	17-jun-91 (jab)
#	    Pulled special processing of 3GL files from end of this script,
#	    because mk3glfiles should do it now.
#	22-jul-91 (jab)
#	    Added cl/clf/ol to CL sparse tree.
#	26-feb-92 (jab)
#	    Substantial changes for W4GLII builds.
#	12-mar-92 (jab)
#	    "mkw4vers" is now run automatically to recreate the version-string
#		file so that "all the right files" get recompiled by w4bang when
#		the version string changes.
#	12-mar-92 (jab)
#	    Just noticed that we weren't running "mk3glfiles" and should've.
#	20-aug-92 (jab)
#	    On the RS/6000, we actually need to create the 3GL support AFTER
#		everything else is done. Unfortunate, but true; on some other platforms,
#		it must be before making "sphmerge".
#	11-jan-1993 (lauraw)
#		Modified for new VERS/CONFIG scheme:
#		This file used to cat tools/port/conf/VERS to set 
#		a variable called $vers which is not used anywhere here,
#		so I removed the outdated reference to the VERS file. 
#	03-aug-1993 (dwilson)
#		Moved to the ingres library, and converted to the w4gl 3.0
#		directory structures...
#	08-sep-1993 (jab)
#		Modified to run "mkw4apped" if asked.
#	10-sep-1993 (jab)
#		Deleted an erroneous debugging statement. (Oops.)
#	08-sep-1993 (www)
#		Modified to use the 6.5 source tree for cl, common,and front.
#	14-sep-1993 (gillnh2o)
#		Added 'mkfecat'.
#	21-sep-1993 (dwilson)
#		Moved some activities around, to better conform to the W4GL 3.0 
#		build checklist.  In addition, commented out the build of 
#		w4gl/woof/proto, as it doesn't work.
#	24-sep-1993 (gillnh2o)
#		Due to the process of submission and integrations
#		I am trying to accomplish getting the same file in
#		main that is in the w4gl30 branch. I also have 
#		re-ordered the comments.
#	28-sep-1993 (jab)
#		Changed the behavior of '-n' so that no header files are
#		created if you ran "w4bang -n". Not w4glvers.h, gv.h, nothing.
#	28-sep-1993 (zaki)
#		Changed the behavior of '-F' so that all the libraries on 
#		which sphmerge depends on are deleted to ensure no old
#		symbols  are used in  building  sphmerge.   Added script
#		w4dellibs.sh to delete libraries.
#       05-jan-1995 (chech02)
#               Added rs4_us5 for AIX 4.1
#       02-Aug-1999 (hweho01)
#               Added ris_u64 for AIX 64-bit platform.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
#

PROGNAME=w4bang
USAGE="Usage: $PROGNAME [-f][-n][-apped]"

. iisysdep
ensure ingres rtingres || exit 1
if [ -z "$ING_SRC" ]
then
	echo $PROGNAME: \$ING_SRC must be set
	exit 1
fi

if [ -z "$ING_BUILD" ]
then
	echo $PROGNAME: \$ING_BUILD must be set
	exit 1
fi

DIRS=" ./files/dictfiles ./files/deffiles ./files/w4gldemo
./files/w4gldemo/Bitmaps ./files/w4gldemo/External ./files/w4gldemo/Strings
./files/english ./w4glapps ./utility
"

for i in $DIRS
do
    if [ ! -d $ING_BUILD/$i ]
    then
	echo ">>>>> $ING_BUILD/$i: does not exist,"
	echo "    run mkw4gldir, then restart the build."
	exit 1
    fi
done

for w in $*
do
	case "$w" in
		-F*)	FORCE=-F ;;
		-f*)	FORCE=-F ;;
		-n*)	FAKEIT=-n ;;
		-apped|-APPED)	DOAPPED=yes;;
		*)
			echo $USAGE
			exit 1
			;;
	esac
done

# Make sure that libraries are deleted if Force mode is set

if [ "$FORCE" = "-F" ]
then
      echo "*** Deleting Libraries - Force mode set"
      w4dellibs
else 
      echo "*** Using Existing Libraries - Force mode not set"
fi

#
# 1.Make sure that the "conf_*" symbols are up-to-date with the
# VERS file (by rebuilding bzarch.h) and...
#
# 2. Make gv.h file for w4gl (always make a new one)
#
if [ "$FAKEIT" = "-n" ]
then
	echo "*** Not building new version of bzarch.h, gv.h, and w4glvers.h - quiet mode set."
else
	echo "*** Build bzarch.h and gv.h"
	if [ -d $ING_SRC/cl/hdr/hdr_unix ]
	then
		cd $ING_SRC/cl/hdr/hdr_unix
		echo "===== $ING_SRC/cl/hdr/hdr_unix/bzarch.h "
		rm -f bzarch.h; mkbzarch
		echo "===== $ING_SRC/cl/hdr/hdr_unix/gv.h "
		mkgv
	else
		echo ">>>>> Can not find $ING_SRC/cl/hdr/hdr_unix, continuing anyway"
	fi

	echo "*** Build w4glvers.h"
	if [ -d $ING_SRC/w4gl/hdr/hdr ]
	then
		echo "===== $ING_SRC/w4gl/hdr/hdr/w4glvers.h"
		cd $ING_SRC/w4gl/hdr/hdr
		mkw4vers
	else
		echo ">>>>> Can not find $ING_SRC/w4gl/hdr/hdr"
	fi
fi

#
# Build er library and ercompile.
#
echo "*** Build GL (if necessary)"
if [ -d $ING_SRC/gl/glf ]
then
	echo "===== $ING_SRC/gl"
	cd $ING_SRC/gl
	if [ ! -f MING -a ! -f MINGH -a ! -f MINGS ]
	then
		echo ">>> $ING_SRC/gl: No ming file, making one"
		mkming ./*/* ./* .
	fi
	mk $FORCE $FAKEIT hdrs
	mk $FORCE $FAKEIT lib
else
	echo ">>>>> Can not find $ING_SRC/gl, exiting"
	exit 1
fi

echo "*** Build CL (if necessary)"
if [ -d $ING_SRC/cl/clf ]
then
	echo "===== $ING_SRC/cl/clf"
	cd $ING_SRC/cl/clf
	if [ ! -f MING -a ! -f MINGH -a ! -f MINGS ]
	then
		echo ">>> $ING_SRC/cl/clf: No ming file, making one"
		mkming
	fi
	mk $FORCE $FAKEIT lib
	(cd specials ; mk $FORCE $FAKEIT )
	(cd er ; mk $FORCE $FAKEIT )
	cd $ING_SRC/common/hdr/hdr ; mk
	cd $ING_SRC/cl/clf ; mk
else
	echo ">>>>> Can not find $ING_SRC/cl/clf/er_unix_win, exiting"
	exit 1
fi

echo "*** Making common libs, if necessary "
(cd $ING_SRC/common ; mk lib)

echo "*** Making front-end bulk files"
echo "++++++ front/frontcl:"
(cd $ING_SRC/front/frontcl ; mk files)
echo "++++++ w4gl:"
(cd $ING_SRC/w4gl; mk files)
echo "Making rest of w4gldemo:"
(cd $ING_SRC/w4gl/w4gldemo ; mk)

echo "*** Creating fe.cat.msg file"
mkfecat

echo "*** Creating message files"
mkw4cat

echo "*** Building common header files"
(cd $ING_SRC/common/hdr/hdr ; mk)

echo "*** Building front-end header files"
(cd $ING_SRC/front ; mk hdrs)

echo "*** Building w4glheader files"
(cd $ING_SRC/w4gl ; mk hdrs)

echo "*** Building front-end libs"
(cd $ING_SRC/front ; mk lib)

echo "*** Building w4gl libs"
(cd $ING_SRC/w4gl ; mk lib)

#
# Build executables
#
if [ $VERS != ris_us5 -a $VERS != rs4_us5 -a $VERS != ris_u64 ]
then
	echo "*** Build 3GL support"
	(cd $ING_SRC/w4gl/fleas/specials ; mk)
fi
echo "*** Build executables"
# Removing w4gl/woof/proto as it is known that it won't work, and 
# we may as well not compile it.

#DIRECTS=' w4gl/woof/proto w4gl/fleas/sphrexes '
DIRECTS=' w4gl/fleas/sphrexes '

for DIR in $DIRECTS
do
	if [ -d $ING_SRC/$DIR ]
	then
		echo "===== $DIR"
		cd $ING_SRC/$DIR
		if [ ! -f MING -a ! -f MINGH -a ! -f MINGS ]
		then
			echo ">>> $ING_SRC/$DIR: No ming file, making one"
			mkming
		fi
		mk $FORCE $FAKEIT
	else
		echo ">>>>> Can not find $ING_SRC/$DIR, continuing anyway"
	fi
done

#
# Regretably, the RS/6000  3GL support includes building a modified "sphmerge". The "ming" in the "fleas" directory
# was redundant.

if [ $VERS = ris_us5 -o $VERS = rs4_us5 -o $VERS = ris_u64 ]
then
	echo "*** Build 3GL support"
	(cd $ING_SRC/w4gl/fleas/specials ; mk)
fi

if [ "$DOAPPED" = "yes" ]
then
	echo "*** Running 'mkw4apped' to create image files."
	if [ -d $ING_BUILD/w4glapps ]
	then
		mkw4apped
	else
		echo ">>>>> Can not find $ING_BUILD/w4glapps, continuing anyway"
	fi
else
	echo "*** Not running 'mkw4apped' - use 'w4bang -apped' to run it."
fi

w4prerls
