:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
# This script makes an executable file from a FORTRAN source file.
# It is called as an OS/TCL command from the SEP FORTRAN tests.
#
# Usage is:
#	
#	sepfortr <querylanguage> <programname> <libnames...>
#
# where <querylanguage> is either 'sql' or 'quel'
#	<programname> is the root part of a *.sa or *.qa file
#	<libnames...> are a list of libraries for this test (i.e.,
#		the outputs of the "seplib" command, without the ".a"
#		extension.) 
#
# Examples:
#	
#   To make an executable called "thing.exe" from an FORTRAN/QUEL
#   file called "thing.qa": 
#
#	sepfortr quel thing
#
#   To make an executable called "stuff.exe" from and FORTRAN/SQL source
#   file called "stuff.sa" and a library named "myforms.a":
#	
#	sepfortr sql stuff myform
#
# About libraries:
#
#   The library names passed on the 'sepfortr' command line must be portable
#   (i.e., no path names, etc.).  Ideally, the same 'sepfortr' command 
#   should work on any platform (Unix, VMS... even MPE XL!) and in any
#   test environment (PT, OSG, PKS...).
#
#   Use the environment variable SEP_FORTRAN_LD to pass libraries to 'sepfortr'
#   that are not specific to an individual test. Also, use SEP_FORTRAN_LD to 
#   pass names of libraries that are platform dependent. For example,
#
#    setenv SEP_FORTRAN_LD "$II_SYSTEM/ingres/lib/libingres.a -lm /usr/lib/libc.a"
#
#   But DON'T set SEP_FORTRAN_LD from within a SEP script! Always set it 
#   BEFORE you invoke SEP or Listexec!
#
# About the 64-bit tests on Unix hybrid platforms:
#
#   For the hybrid platforms (su9_us5, a64_sol, hp2_us5 and r64_us5), 
#   if the environment variable "FORTRAN_64" is set, the 64-bit test program
#   will be built.

#
## History:
##
##	27-jan-1992 (lauraw)
##		Created.
##	26-may-94 (vijay)
##		allow for other names for fortran compiler.
##  28-Aug-97 (merja01)
##      To resolve link problems using 2.0 POSIX threads, added call to
##      iisysdep and added LDLIBMACH.
##	27-Aug-2003 (bonro01)
##		Add sql-code option to support new tests.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##   2-Nov-2006 (hanal04) Bug 117020
##      Added $CCLDMACH to compile line to ensure compiler flags are included.
##   31-Aug-2010 (hweho01)
##      Include the 64-bit support for hybrid platforms, add a new switch
##      "FORTRAN_64". 
##

. $II_SYSTEM/ingres/utility/iisysdep
# We make an assumption about the compiler:

compiler=$F77
[ "$compiler" = "" ] && compiler=f77

# First, check args and determine $qsuffix, $preproc, and $sfile:

if [ $# -lt 2 ]
then
	echo "ERROR: usage is 'sepfortr' <querylanguage> <programname> <libraries>'"
	exit 1
fi

case $1 in 
	quel)	qsuffix=qf
		preproc=eqf
		;;
	sql)	qsuffix=sf
		preproc=esqlf
		;;
	sql-sqlcode)	qsuffix=sf
		preproc="esqlf -sqlcode"
		;;

	*)	echo "ERROR: invalid query language: $1. Must be 'quel' or 'sql'"
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
	else
		echo "ERROR: $1.a is not a valid load library file"
		exit 1
	fi
	
	shift
done

# The tester is required to set environment-specific $SEP_FORTRAN_LD variable:

if [ -z "$SEP_FORTRAN_LD" ]
then
	echo "ERROR: \$SEP_FORTRAN_LD is not set. Use \$SEP_FORTRAN_LD to identify"
	echo "       environment-specific load libraries (including"
	echo "       \$II_SYSTEM/ingres/lib/libingres.a) and environment-specific".
	echo "       flags to 'ld'."
	exit 1
else
	ldflags="$libs $SEP_FORTRAN_LD"
fi

preproc_opt=""

test_64bit=false
[ ! -z "$FORTRAN_64" ] && test_64bit=true

/* setup the 64 bit options for preprocessor, library  and compiler */
if [ "$test_64bit" = "true" ] ; then
preproc_opt="-g64"
SEP_FORTRAN_LD="$II_SYSTEM/ingres/lib/lp64/libingres.a"
ldflags="$libs $SEP_FORTRAN_LD"
CCLDMACH="$CCLDMACH64"
LDLIBMACH="$LDLIBMACH64"
fi
	
if $preproc $preproc_opt $sfile
then :
else
	echo "ERROR: \"$preproc\" failed on $sfile"
	exit 1
fi

echo $compiler  -o $sfile.exe $sfile.f $CCLDMACH $ldflags $LDLIBMACH
if $compiler  -o $sfile.exe $sfile.f $CCLDMACH $ldflags $LDLIBMACH
then :
else
	echo "ERROR: \"$compiler\" failed on $sfile.f"
	echo "       Command was: $compiler -o $sfile.exe $sfile.f $CCLDMACH $ldflags $LDLIBMACH"
	echo "       Check to make sure \"$compiler\" is in your path" 
	exit 1
fi

exit

