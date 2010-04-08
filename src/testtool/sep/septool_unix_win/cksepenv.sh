:
#
# Copyright (c) 2004 Ingres Corporation
# 
# cksepenv --   checks to see if the environment is OK for SEP tests
#
# 	Flags available:
#		-v (verbose)
#		-s (silent)
#		-c (continue, even if errors found)
#
# Exit status is 0 if environment seems okay.
# 
# Otherwise, error message is echoed and exit is non-zero.
#
#	exit	means
#	----	-----
#	   1	an environment variable not set or
#	           an executable is not found
#	   3	user is not 'testenv'
#	   4	qasetuser is not working
#	   5    user has umask other than 2
#	   6	a test login is not in testenv's group
#	   7    a test login can't create a file
#	   8	wrong number of name servers up
#	   9	no dbms servers up
#         10    ingchkdate found II_AUTHORIZATION error
#	  11	II_INSTALLATION isn't set
#	  12	wrong or missing 'users' file entry
#	  13    bad command line flag
#	  14	missing or uncreatable output directory
#	  15	error running iisysdep
#

## History:
##
## 18-dec-1991	(lauraw)
##	Created.
## 20-dec-1991	(lauraw)
##	Changed "pvusr1" to non-privileged user.
##	Added "output/star" directory.
## 08-jan-1991	(lauraw)
##	Added INGRES/NET test output directories:
##		output/net/local
##		output/net/loopback
##		output/net/remote
## 28-jan-1992	(lauraw)
##	Added output/embed as one of the directories created by this script.
##	Changed the order in which some environment indicators are
##	checked. Put a pathname on iisysdep and exit if the inclusion
##	of it fails.
## 18-mar-1993	(dianeh)
##	Changed call to ingprenv1 (gone for 6.5) to a call to ingprenv.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##

#----------------------------------------------------------------------
# Get command line args
#----------------------------------------------------------------------

Vecho=":"
Eecho="echo"
Cexit="exit"

for arg
do case $arg in

   -v)	Vecho="echo"
      	shift
      	;;

   -s)  Eecho=":"
	shift
	;;

   -c)  Cexit="echo Continuing instead of exiting with code"
	Vecho="echo"
	shift
	;;

   *) 	$Eecho "Unknown flag '$arg'"
      	$Cexit 13
      	;;

   esac
done

#----------------------------------------------------------------------
# Porting environment variables are needed:
#----------------------------------------------------------------------

$Vecho "Checking for porting environment variables..."

: ${ING_SRC?"must be set"}
: ${ING_TST?"must be set"}
: ${ING_BUILD?"must be set"}

#----------------------------------------------------------------------
# Need an INGRES environment:
#----------------------------------------------------------------------

$Vecho "Checking II_SYSTEM ..."

: ${II_SYSTEM?"must be set"}

#----------------------------------------------------------------------
# Get iisysdep vars
#----------------------------------------------------------------------

$Vecho "Running iisysdep..."

if . $II_SYSTEM/ingres/utility/iisysdep
then :
else
	$Eecho "There is a problem with $II_SYSTEM/ingres/utility/iisysdep"
	$Cexit 15
fi

#----------------------------------------------------------------------
# Get installation code
#----------------------------------------------------------------------

$Vecho "Checking for installation code..."

II=`ingprenv II_INSTALLATION`
if [ -z $II ]
then 
	$Eecho "II_INSTALLATION must be set in the INGRES symbol table"
	$Cexit 11
fi

#----------------------------------------------------------------------
# Make sure authorization string is okay
#----------------------------------------------------------------------

$Vecho "Checking II_AUTHORIZATION..."

if ingchkdate 
then :
else
	$Eecho "There is a problem with II_AUTHORIZATION"
	$Cexit 10
fi

#----------------------------------------------------------------------
# User running tests must be 'testenv'
#----------------------------------------------------------------------

$Vecho "Checking effective userid...."

if [ "$WHOAMI" != "testenv" ]
then
	$Eecho "You must be user 'testenv' to run tests"
	$Cexit 3
fi

#----------------------------------------------------------------------
# User running tests must have the correct umask
#----------------------------------------------------------------------

$Vecho "Checking umask...."

if [ `umask` -ne 2 ]
then
	$Eecho "Wrong umask! Your umask must be '2' to run tests"
	$Cexit 5
fi


#----------------------------------------------------------------------
# Test environment variables must be set:
#----------------------------------------------------------------------

$Vecho "Checking for test environment variables...."

: $ {TST_TESTOOLS?"must be set"}
: $ {TST_SEP?"must be set"}
: $ {ING_EDIT?"must be set"}
: $ {PEDITOR?"must be set"}
: $ {TST_TESTENV?"must be set"}
: $ {TST_LISTEXEC?"must be set"}

#----------------------------------------------------------------------
# Are the logins for the test users set up correctly?
# (This also makes sure qasetuser works.)
#----------------------------------------------------------------------

$Vecho "Checking test logins with 'qasetuser'..."

tlogins="testenv qatest ingres pvusr1 pvusr2 pvusr3"

ext="$$"

for t in $tlogins
do
	qasetuser $t touch $t.$ext
	if [ $? -ne 0 ]
	then
		$Eecho "'qasetuser' failed for login $t"
		$Eecho "Did you remember to 'chown root qasetuser'?"
		$Cexit 4
	fi

	if [ -f $t.$ext ]
	then
		rm $t.$ext
		if [ $? -ne 0 ]
		then
			$Eecho "testenv can't remove a file created by $t"
			$Eecho "Maybe testenv and $t are in different groups?"
			$Cexit 6	
		fi
	else
		$Eecho "$t can't create a file in this directory"
		$Cexit 7
	fi

done

#----------------------------------------------------------------------
# Are the output directories there? The main one is assumed to have
# been put there by mkidir, and the subdirs get created now if they're
# not already there.
#----------------------------------------------------------------------

$Vecho "Checking for test output directories..."

if [ -d $ING_TST/output ]
then
	for outdir in be fe embed star init audit net net/local net/loopback net/remote
	do
		outdirl=$ING_TST/output/$outdir
		if [ -d $outdirl ]
		then :
		else
			if mkdir $outdirl
			then 
				$Vecho "Created test output directory $outdirl"
				chmod 770 $outdirl
			else 
				$Eecho "Can't create output directory $outdirl"
				$Cexit 14
			fi
		fi
	done
else
	$Eecho "The \$ING_TST/output directory tree is missing"
	$Cexit 14
fi

#----------------------------------------------------------------------
# Does the installation 'users' file contain the correct entries?
#----------------------------------------------------------------------

$Vecho "Checking the 'users' file in $II_SYSTEM/ingres/files"

while read user
do

	if grep -s $user $II_SYSTEM/ingres/files/users
	then :
	else
		user_is=`echo $user | sed "s/\!.*$//"`

		$Eecho "INGRES user '$user_is' is missing or has wrong privileges"

		$Cexit 12
	fi

done <<\EOF
ingres!0!0!100025
myrtle!0!0!1
pvusr1!0!0!1
pvusr2!0!0!1
pvusr3!0!0!1
pvusr4!0!0!1
qatest!0!0!100025
qatestn!0!0!21
qatesto!0!0!1
qatests!0!0!100025
testenv!0!0!100025
EOF

if [ $? -eq 12 ]
then
	$Cexit 12
fi

#----------------------------------------------------------------------
# Check for only one name server and at least one DBMS server. 
#----------------------------------------------------------------------

$Vecho "Checking for an active name server in installation $II..."

count=`runiinamu iigcn | wc -l`

if [ $count -ne 1 ]
then
	$Eecho "There are `echo $count` name servers running in installation $II"
	$Cexit 8
else
	$Vecho "An active name server was found in installation $II"
fi

$Vecho "Checking for active database server(s) in installation $II..."

count=`runiinamu iidbms | wc -l`
if [ $count -eq 0 ]
then
	$Eecho "There is no database server in installation $II"
	$Cexit 9
else
	$Vecho "Found `echo $count` database server(s) running in installation $II"

fi

#----------------------------------------------------------------------
# Looks like the tests may run okay....
#----------------------------------------------------------------------

if [ "$Cexit" = "exit" ]
then
	$Vecho "No problems were found in the test environment."
else
	$Vecho "Finished checking test environment."
fi

exit 0
