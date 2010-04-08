:
#
# Copyright (c) 2004 Ingres Corporation
#
# This script makes an executable file from an Ada source file.
# It is called as an OS/TCL command from the SEP Ada tests.
#
# The instructions in the INGRES SQL Verdix Ada Companion Guide
# were used as reference in writing this script. If you are testing with
# any other compiler than Verdix Ada, you may have to change this script 
# (and you may have to change the Ada source code in the SEP scripts).
#
# Usage is:
#	
#	sepada <querylanguage> <programname> <libnames...>
#
# where <querylanguage> is either 'sql' or 'quel'
#	<programname> is the root part of a *.sa or *.qa file
#	<libnames...> are a list of libraries for this test (i.e.,
#		the outputs of the "seplib" command, without the ".a"
#		extension.) 
#
# Examples:
#	
#   To make an executable called "thing.exe" from an Ada/QUEL
#   file called "thing.qa": 
#
#	sepada quel thing
#
#   To make an executable called "stuff.exe" from and Ada/SQL source
#   file called "stuff.sa" and a library named "myforms.a":
#	
#	sepada sql stuff myform
#
# About libraries:
#
#   The library names passed on the 'sepada' command line must be portable
#   (i.e., no path names, etc.).  Ideally, the same 'sepada' command 
#   should work on any platform (Unix, VMS... even MPE XL!) and in any
#   test environment (PT, OSG, PKS...).
#
#   Use the environment variable SEP_ADA_LD to pass libraries to 'sepada'
#   that are not specific to an individual test. Also, use SEP_ADA_LD to 
#   pass names of libraries that are platform dependent. For example,
#
#    setenv SEP_ADA_LD "$II_SYSTEM/ingres/lib/libingres.a -lm /usr/lib/libc.a"
#
#   But DON'T set SEP_ADA_LD from within a SEP script! Always set it 
#   BEFORE you invoke SEP or Listexec!

#
# NOTE: If this script seems a little heavy on the error checking, it's
# because I've tried to set it up in such a way that it is silent when
# it works, but helpful when something goes wrong. That way, we don't
# have platform-dependent output in the SEP script canon.
#
# Also, the temporary directory we create is only removed if everything
# turns out okay. Otherwise, we exit without removing it so the tester
# can diagnose the problem.
#
## History:
##
##	27-jan-1992 (lauraw)
##		Created.
##
##	1-SEP-1993  (jbryson)	Turned off ADA optimization (-O0) as a
##				workaround for bug #40718.
##	10-may-96 (muhpa01)
##		add support for DEC Ada for axp_osf
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	04-May-2007 (toumi01)
##	    For -multi support add eqsqlcag.a[da].
##	11-Oct-2007 (toumi01)
##	    For -multi support add eqsqlcam.a[da] and delete eqsqlcag.a[da].
##

# First, check args and determine $qsuffix, $preproc, and $sfile:

. iisysdep

if [ $# -lt 2 ]
then
	echo "ERROR: usage is 'sepada <querylanguage> <programname> <libraries>'"
	exit 1
fi

case $1 in 
	quel)	qsuffix=qa
		preproc=eqa
		;;
	sql)	qsuffix=sa
		preproc=esqla
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

# The tester is required to set environment-specific $SEP_ADA_LD variable:

if [ -z "$SEP_ADA_LD" ]
then
	echo "ERROR: \$SEP_ADA_LD is not set. Use \$SEP_ADA_LD to identify"
	echo "       environment-specific load libraries (including"
	echo "       \$II_SYSTEM/ingres/lib/libingres.a) and environment-specific".
	echo "       flags to 'ld'."
	exit 1
else
	ldflags="$libs $SEP_ADA_LD"
fi

# Verdix Ada requires a library directory of its own to compile your program.
# In order to keep the SEP tests self-contained, this script recreates
# the Ada library directory each time it compiles a program.
 
# We make a scratch directory to house the Ada library:
#

scratch=$TMPDIR/sepada$$

if [ $VERS = axp_osf ]
then 
    mkdir $scratch
else
if 
	mkdir $scratch &&
	cp $II_SYSTEM/ingres/files/eqdef.a $scratch &&
	cp $II_SYSTEM/ingres/files/eqsqlca.a $scratch &&
	cp $II_SYSTEM/ingres/files/eqsqlcam.a $scratch &&
	cp $II_SYSTEM/ingres/files/eqsqlda.a $scratch 
then :
else
	echo "ERROR: couldn't copy INGRES files to $scratch for Ada library"
	exit 1
fi
fi

if [ $VERS = axp_osf ]
then :
else
if cp $sfile.$qsuffix $scratch
then :
else
	echo "ERROR: couldn't copy source file $sfile.$qsuffix to $scratch"
	exit 1
fi
fi

if [ $VERS = axp_osf ]
then :
else
for f in $libs
do
	if cp $f $scratch
	then :
	else
		echo "ERROR: couldn't copy $f to Ada directory $scratch"
		exit 1
	fi
done
fi

#
# Make a library and compile the INGRES Ada defs into it:
#

if [ $VERS = axp_osf ]
then
    amklib $scratch/adalib
    ADALIB=@$scratch/adalib
    export ADALIB
    if
    	ada $II_SYSTEM/ingres/files/eqdef.a &&
    	ada $II_SYSTEM/ingres/files/eqsqlca.a &&
    	ada $II_SYSTEM/ingres/files/eqsqlcam.a &&
    	ada $II_SYSTEM/ingres/files/eqsqlda.a
    then :
    else
        echo "ERROR: 'ada' could not compile eqdef.a, eqsqlca.a, eqsqlcam.a, or eqsqlda.a"
        exit 1
    fi
else
if (
	cd $scratch

	if a.mklib 
	then :
	else
		echo "ERROR: 'a.mklib' failed -- is Verdix Ada in your path?"
		exit 1
	fi

	if
		ada eqdef.a &&
		ada eqsqlca.a &&
		ada eqsqlcam.a &&
		ada eqsqlda.a 
	then :
	else 
		echo "ERROR: 'ada' could not compile eqdef.a, eqsqlca.a, eqsqlcam.a, or eqsqlda.a"
		exit 1
	fi

   )
then :
else 
	echo "ERROR: unable to make Ada library in directory $scratch"
	exit 1
fi
fi

# 
# Okay, if we get here, the setup stuff is okay and we just need to compile
# the test program:
#

if (
	if [ $VERS != axp_osf ]
	then
		cd $scratch
	fi
	
	if $preproc $sfile
	then :
	else
		echo "ERROR: \"$preproc\" failed on $sfile"
		exit 1
	fi

	if ada -O0 $sfile.a
	then :
	else
		echo "ERROR: 'ada' failed on $sfile.a"
		exit 1
	fi

	if [ $VERS = axp_osf ]
	then
		if ald -o $sfile.exe $sfile -a "$ldflags"
		then :
		else
			echo "ECHO: 'ald' failed -- command was:"
			echo " ald -o $sfile.exe $sfile -a \"$ldflags\""
			exit 1
		fi
	else
		if a.ld -o $sfile.exe $sfile $ldflags
		then :
		else
			echo "ERROR: 'a.ld' failed -- command was:"
			echo "	a.ld -o $sfile.exe $sfile $ldflags"
			exit 1
		fi
	fi
   )
then :
else
	echo "ERROR: unable to compile and link $sfile -- see $scratch directory"
	exit 1
fi

#
# Okay, we have an executable object -- let's move it to the currect
# directory and remove the scratch directory: 

if [ $VERS = axp_osf ]
then
    rm -rf $scratch
else
if mv $scratch/$sfile.exe .
then
	rm -rf $scratch
else
	echo "ERROR: can't move $scratch/$sfile.exe to current directory"
	exit 1
fi
fi

exit

