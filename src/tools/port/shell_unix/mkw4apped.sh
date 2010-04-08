:
#
# Copyright (c) 2004 Ingres Corporation
#
# mkw4apped.sh -- build the apped.img file.
#
# History: 
#	18-oct-90 (brett)
#	    Created the script.
#	16-dec-90 (brett)
#	    Add -t flag to importapp.
#	14-jan-91 (brett)
#	    Add chmod to set the correct mode on the apped.img file.
#	6-may-91 (jab)
#	    Changed dbname to 'apped_db'. Looks less, umm, silly.
#	16-may-91 (jab)
#	    This script really should be run as ingres. Now it checks.
#	19-feb-92 (jab)
#	    The pathnames changed, and in fact the apped is now six
#		applications, not one. Changed to account for all this.
#	26-feb-92 (jab)
#	    Small bug fix - the export files live in $ING_SRC/w4gl/apped
#		now.
#	30-apr-92 (szeng)
#	    Added ";" to seprate set environment variables and export
#	    them since ds3_ulx 'sh' requires so.
#	09-aug-93 (dwilson)
#		Made changes to accommodate W4GL 3.0.  Primary changes 
#		was in the directory structure, from front/apped/... to 
#		w4gl/apped/...
#	18-aug-93 (gillnh2o)
#		Added new importapp and makeimage directives for
#		iisyslib and iisystem. Note, the order of these two
#		are important. iisyslib.lbr must be created before
#		iisystem.img. 
#	19-aug-93 (www)
#		Just adding a comment so I can submit a change. Changes to this file
#		don't seem to be showing up on the ineed list of the linked copy in
#		main!
#	02-sep-93 (gillnh2o)
#		Adding importapp and makeimage directives for amethyst. I
#		know the application name is spelled with an "x" instead of
#		of a "y" but this is correct.
#	03-sep-93 (jab)
#		1. Adding support for the '-e' flag on compileapp/makeimage, so
#		that if it hits a compile error it'll dump out the 4GL that's
#		erroneous.
#		2. Also, changing this script so that it can use the
#		sphmerge in $ING_BUILD/bin (note that we change $II_CONFIG to
#		find the 'deffiles' and message files that might've changed).
#		This should allow this script to be run even if W4GL isn't
#		installed into the $II_SYSTEM area.
#		3. Also, deleted the 'versionapp' references - they were commented
#		out anyhow.
#	10-sep-93 (jab)
#		The $II_CONFIG was a bad idea: things like 'charset' are not
#		necessarily in the $ING_BUILD/files. Oops.
#	15-sep-93 (dwilson)
#		Added '-t' flag to all importapp commands that don't have
#		them so that no importapp will fill the log file.  
#	15-sep-93 (gillnh2o)
#		This was missy.  Integrated dwilson's change above. But
#		it turned out that jab had already made the change. I
#		kept dwilson's comment but deleted his version of the
#		integration keeping only jab's previous change. The
#		problem was that dwilson had not integrated jab's change
#		into main before making his change into main.
#	15-sep-93 (gillnh2o)
#		The previous integration had put dwilson's 15-sep 
#		comments before 03-sep comments. I removed that comment
#		placing his comment in correct order.
#	15-oct-93 (jab)
#		Adding ruby/rptsys.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

PROG_NAME=mkw4apped
DBNAME=apped_db

ensure ingres rtingres || exit 1

#
# Sanity checks
#
if [ -z "$ING_SRC" ]
then
    echo $PROG_NAME: \$ING_SRC not set.
    exit 1
fi

if [ ! -d $ING_SRC ]
then
    echo "$PROG_NAME: Can not find the \$ING_SRC ($ING_SRC) directory."
    exit 1
fi

APPED_SRC=$ING_SRC/w4gl/apped
DESTDIR=$ING_BUILD/w4glapps
II_W4GLAPPS_SYS=$DESTDIR; export II_W4GLAPPS_SYS

if [ ! -d $APPED_SRC ]
then
    echo "$PROG_NAME: Can not find the $APPED_SRC directory."
    exit 1
fi

if [ ! -d $ING_BUILD/w4glapps ]
then
    mkdir $ING_BUILD/w4glapps > /dev/null 2>&1
    if [ $? = 1 ]
    then
	echo "$PROG_NAME: Can not create $ING_BUILD/w4glapps directory."
	exit 1
    fi
    chmod 755 $ING_BUILD/w4glapps
else
    touch $ING_BUILD/w4glapps/$DBNAME > /dev/null 2>&1
    if [ $? = 1 ]
    then
	echo "$PROG_NAME: Can not write in $ING_BUILD/w4glapps directory."
	rm -f $ING_BUILD/w4glapps/$DBNAME > /dev/null 2>&1
	exit 1
    fi
    rm -f $ING_BUILD/w4glapps/$DBNAME > /dev/null 2>&1
fi

#
# Create temporary database.
#
echo "Creating temporary database . . ."
echo "createdb $DBNAME"
createdb $DBNAME
if [ $? = 1 ]
then
    echo "$PROG_NAME: Createdb failed, exiting..."
    exit 1
fi

II_BINARY=$ING_BUILD/bin
export II_BINARY
#
# Build Application Editor Image file.
#
echo "Import/Build Application Editor Image file . . ."

importapp -t $DBNAME syslib $APPED_SRC/syslib/syslib.exp
makeimage $DBNAME syslib $DESTDIR/iisyslib.lbr -asystem_library -l -e

importapp -t $DBNAME _system $APPED_SRC/iisystem/system_.exp
makeimage $DBNAME _system $DESTDIR/iisystem.img -aiisystem -e

importapp -t $DBNAME utility $APPED_SRC/utility/utility.exp
makeimage $DBNAME utility $DESTDIR/iiutil.img -e

importapp -t $DBNAME scripted $APPED_SRC/scripted/scripted.exp
makeimage $DBNAME scripted $DESTDIR/iiscred.img -e

importapp -t $DBNAME forest $APPED_SRC/forest/forest.exp
makeimage $DBNAME forest $DESTDIR/iiforst.img -e

importapp -t $DBNAME outline $APPED_SRC/outline/outline.exp
makeimage $DBNAME outline $DESTDIR/iioutln.img -e

importapp -t $DBNAME debugger $APPED_SRC/debugger/debugger.exp
makeimage $DBNAME debugger $DESTDIR/iidebug.img -e

importapp -t $DBNAME rptsys $APPED_SRC/rptsys/rptsys.exp
makeimage $DBNAME rptsys $DESTDIR/rptsys.lbr -l -e

importapp -t $DBNAME ruby $APPED_SRC/ruby/ruby.exp
makeimage $DBNAME -arby ruby $DESTDIR/rby.img -e

importapp -t $DBNAME amethxst $APPED_SRC/amethyst/amethyst.exp
makeimage $DBNAME amethxst $DESTDIR/amethxst.img -e

importapp -t $DBNAME  apped $APPED_SRC/apped/apped.exp
makeimage $DBNAME apped $DESTDIR/apped.img -e
if [ $? = 1 ]
then
   echo "$PROG_NAME: Makeimage failed, exiting..."
   exit 1
else
   chmod 644 $DESTDIR/*.img
fi


echo Don\'t forget to \"destroydb $DBNAME\" when you\'ve confirmed
echo that this worked!
exit 0
