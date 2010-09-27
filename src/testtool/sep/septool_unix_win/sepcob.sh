:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
# This script makes a code that can be run using the Ingres-linked RTS.
# It also builds the executable ingrts for you also.
# It is called as an OS/TCL command from the SEP COBOL tests.
#
# Usage is:
#	
#	sepcob <querylanguage> <programname> <libnames...>
#
# where <querylanguage> is either 'sql' or 'quel'
#	<programname> is the root part of a *.scb or *.qcb file
#	<libnames...> are a list of libraries for this test (i.e.,
#		the outputs of the "seplib" command, without the ".a"
#		extension.) 
#
# Examples:
#	
#   To make a runnable code from a COBOL/QUEL file called "thing.qcb": 
#
#	sepcob quel thing
#
#   To run the file using ingrts:
#
#	ingrts thing
#
#   To make a runnable code from a COBOL/SQL source file called "stuff.scb" 
#   and a library named "myforms.a":
#	
#	sepcob sql stuff myform
#
#   
# About libraries:
#
#   The library names passed on the 'sepcob' command line must be portable
#   (i.e., no path names, etc.).  Ideally, the same 'sepcob' command 
#   should work on any platform (Unix, VMS... even MPE XL!) and in any
#   test environment (PT, OSG, PKS...).
#   Object files can also be passed in.
#
# History:
#	15-JAN-93	written (jpark)
#
#	3-FEB-93	modified (jpark)
#		can pass in libraries and object files for building ingrts
#	06-dec-93 (vijay)
#		use shared libraries if available.
#	10-may-96 (muhpa01)
#		add support for DEC Cobol for axp_osf
#       26-Sep-2003 (bonro01)
#		Added Chris Rogers support for new COBOL tests.
#		Support for sql-sqlcode parameter.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##      25-Aug-2010 (hweho01)
##          For su4.us5 platform, remove "-mt" from the LDLIBMACH list.


# First, check args and determine $qsuffix, $preproc, and $sfile:

if [ $# -lt 2 ]
then
	echo "ERROR: usage is 'sepcob' <querylanguage> <programname> <libraries>'"
	exit 1
fi

USE_SHARED_LIBS=false

. iisysdep

case $1 in 
	quel)	qsuffix=qcb
		preproc=eqcbl
		;;
	sql)	qsuffix=scb
		preproc=esqlcbl
		;;
	sql-sqlcode)	qsuffix=scb
		preproc="esqlcbl -sqlcode"
		;;
	*)	echo "ERROR: invalid query language: $1. Must be 'quel', 'sql' or 'sql-sqlcode'"
		exit 1
	 	;;
esac

if [ -r $2.$qsuffix -a -s $2.$qsuffix ]
then
	sfile=$2
else
	echo "ERROR: $2.$qsuffix is not a valid input source file"
	exit 1
fi

# The remaining args (3 thru N) must be libraries.
# Check to make sure they're in the current directory and
# add them to the $libs list:

libs=""
shift; shift
while [ $1 ]
do
	if [ -r $1.a -a -s $1.a ]
	then
		libs="$libs $1.a"
	elif [ -r $1.o -a -s $1.o ]
	then 
		libs="$libs $1.o"
	else
		echo "ERROR: $1.a is not a valid load library file"
		exit 1
	fi
	
	shift
done

if $preproc $sfile.$qsuffix
then :
else
	echo "ERROR: \"$preproc\" failed on $sfile"
	exit 1
fi

# work around for AIX COBOL bug.
[ $VERS = ris_us5 ] && unset LIBPATH

# if environment is axp_osf, then set up compile & links for
# DEC Cobol

if [ $VERS = axp_osf ]
then
if $USE_SHARED_LIBS
then
   libingfiles="$II_SYSTEM/ingres/lib/libframe.1.$SLSFX"
   libingfiles="$libingfiles $II_SYSTEM/ingres/lib/libq.1.$SLSFX"
   libingfiles="$libingfiles $II_SYSTEM/ingres/lib/libcompat.1.$SLSFX"
   libingfiles="$libingfiles $II_SYSTEM/ingres/lib/libingres.a"
else
   libingfiles=`ls $II_SYSTEM/ingres/lib/libingres*.a`
fi

if cobol -ansi -names as_is -o $sfile.exe $sfile.cbl $libingfiles $libs -lc -lm
then :
else
    echo "ERROR: "cobol" failed on $sfile.cbl"
    echo "       Command was: cobol -ansi -names as_is -o $sfile.exe $sfile.cbl $libingfiles $libs -lc -lm"
    echo "       Check to make sure "cobol" is in your path"
    exit 1
fi
exit

else
# Micro Focus 
# the -C warning=1 flag is to suppress the COMP-5 information message

if cob -C warning=1 $sfile.cbl 
then :
else
	echo "ERROR: "cob" failed on $sfile.cbl"
	echo "       Command was: cob $sfile.cbl $libs"
	echo "       Check to make sure "cob" is in your path" 
	exit 1
fi

# prepares Ingres Micro Focus Run-Time System

# Remove '-mt' from the LDLIBMACH list for Solaris/Sparc.
[ "$VERS" = "su4_us5" ] && LDLIBMACH=`echo $LDLIBMACH | sed 's/\-mt//g'`

# extract 3 Ingres Micro Focus COBOL support modules

ar xv $II_SYSTEM/ingres/lib/libingres.a iimfdata.o
ar xv $II_SYSTEM/ingres/lib/libingres.a iimflibq.o
ar xv $II_SYSTEM/ingres/lib/libingres.a  iimffrs.o

# link new Ingres COBOL RTS

if $USE_SHARED_LIBS
then
   libingfiles="$II_SYSTEM/ingres/lib/libframe.1.$SLSFX"
   libingfiles="$libingfiles $II_SYSTEM/ingres/lib/libq.1.$SLSFX"
   libingfiles="$libingfiles $II_SYSTEM/ingres/lib/libcompat.1.$SLSFX"
else
   libingfiles=`ls $II_SYSTEM/ingres/lib/libingres*.a`
fi

cob -x -o $sfile.exe $sfile.cbl $libingfiles $libs $LDLIBMACH
echo cob -x -o $sfile.exe $sfile.cbl $libingfiles $libs $LDLIBMACH
exit
fi

