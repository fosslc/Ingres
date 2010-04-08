:
#
#  Copyright (c) 2004, 2010 Ingres Corporation
#  Run the UDT demo, and compare the results to the canonical output (*.log).
#
##  History:
##	06-sep-1990 (jonb)
##		Created
##	29-aug-1991 (jonb)
##		Added new datatypes zchar and cpx.  Added a feature to allow
##		running without creating a new database.
##	27-feb-1992 (lan)
##		Added -s to run the demo in silent mode.
##	07-jun-1993 (dianeh)
##		Updated to reflect 6.5 commands; added a -d and -o flag, to
##		allow the database name and output directory to be specified
##		on the command line; added a -help flag; added error-checking;
##		general clean-up.
##      18-aug-1993 (stevet)
##              Added PNUM test and add -kf SQL startup flag.  UDT cannot
##              work with DECIMAL constant so -kf will force numeric 
##              constant to FLOAT.
##     16-Feb-2010 (hanje04)
##         SIR 123296
##         Add support for LSB build.
#

(LSBENV)

. $II_SYSTEM/ingres/utility/iisysdep

DEMO=$II_SYSTEM/ingres/demo/udadts

CMD=`basename $0`

usage='[ -d <database> ]  [ -o <out_dir> ]'

TESTLIST="intlist op_load op_test cpx zchar pnum_test"

trap 'rm -f $outdir/$test.out $outdir/$test.dif ; exit' 1 2 3 15

while [ $# -gt 0 ]
do
  case "$1" in
    -d*) if [ `expr $1 : '.*'` -gt 2 ] ; then
           udtdb="`expr $1 : '-d\(.*\)'`"
           shift
         else
           [ $# -gt 1 ] ||
           { echo "Usage: " ; echo "$CMD $usage" ; exit 1 ; }
           udtdb=$2 ; shift ; shift
         fi
         ;;
    -o*) if [ `expr $1 : '.*'` -gt 2 ] ; then
           outdir="`expr $1 : '-o\(.*\)'`"
           shift
         else
           [ $# -gt 1 ] ||
           { echo "Usage: " ; echo "$CMD $usage" ; exit 1 ; }
           outdir=$2 ; shift ; shift
         fi
         ;;
  -help) echo "" ; echo "Usage:" ; echo "  $CMD $usage" ; exit 0
         ;;
     -*) echo "$CMD: Illegal flag: $1" ; echo ""
         echo "Usage:" ; echo "  $CMD $usage" ; exit 1
         ;;
      *) echo "$CMD: Illegal argument: $1" ; echo ""
         echo "Usage:" ; echo "  $CMD $usage" ; exit 1
         ;;
  esac
done

# Check that specified out_dir exists
[ "$outdir" ] &&
{
  [ -d $outdir ] ||
  {
    echo "$CMD: No such directory: $outdir. Exiting..." ; exit 1
  }
}

cat << !

This program will execute the sample queries that use the User Defined
Datatypes (UDTs).  These queries are provided in the UDT demonstration
directory ($DEMO).

NOTE: The sample queries can only be run if the INGRES DBMS has been
linked with the UDT object code provided in the UDT demonstration directory.
Before continuing, be sure you have run the \`iilink' program, specifying:

    $DEMO/*.o

as input when prompted for object filenames.

!

iiask4it "Do you want to continue" ||
{
  cat << !

To link the DBMS with the UDT demo code, you must use these commands:

    ingstop     # to bring down the INGRES installation
    iilink      # to relink the DBMS executables
    ingstart    # to bring up the INGRES installation

!
  exit 7
}

[ ! -d $DEMO -o ! -r $DEMO/intlist.qry ] &&
{
  cat << !

The UDT demonstration directory does not exist or is not readable.
Check for the existence of:   

    $DEMO

and reissue \`$CMD' when the problem has been corrected.

!
  exit 5
}

echo "show ingres" | iinamu >/dev/null 2>&1 ||
{
  cat << !

There is no INGRES DBMS running in the current installation.
Use \`ingstart' to start INGRES, and then reissue \`$CMD'.

!
  exit 6
}

cat << !

This program will capture output from running the queries. 

!

[ "$outdir" ] ||
{
  while [ -z "$outdir" ]
  do
    echo "Enter the directory for the output files, or <RETURN> for default."
    iiechonn "[`pwd`]: "
    read outdir
    [ "$outdir" ] || outdir=`pwd`
    echo "" ; echo "Output directory is: $outdir"
    iiask4it "Okay" || outdir=
    [ -d $outdir ] || { echo "No such directory: $outdir" ; outdir= ; }
  done
}

[ "$udtdb" ] ||
{
  cat << !

You must supply the name of a database to run the queries against.

!
}
db=$udtdb
[ "$db" ] || db=database
if iiask4it "Do you want to create a new $db" ; then
  while [ -z "$udtdb" ]
  do
    iiechonn "Enter a name for this database: "
    read udtdb
    [ "$udtdb" ] &&
    {
      echo "" ; echo "Database to use is: $udtdb" ; iiask4it "Okay" || udtdb=
    }
  done
  echo "select dbmsinfo('_version')\g" | sql -s $udtdb >/dev/null 2>&1 &&
  {
    echo "Removing previous version of $udtdb . . ."
    destroydb $udtdb ||
    {
      cat <<!

Unable to destroy $udtdb. Please resolve the problem,
then reissue \`$CMD'.
!
      exit 1
    }
  }
  createdb $udtdb ||
  {
    cat <<!

Unable to create $udtdb. Please resolve the problem,
then reissue \`$CMD'.
!
    exit 1
  }
  iiask4it "Do you want to continue" || exit 10
else
  while [ -z "$udtdb" ]
  do
    iiechonn "Enter the name of the database to use for the queries: "
    read udtdb
    [ "$udtdb" ] &&
    {
      echo "" ; echo "Database to use is: $udtdb" ; iiask4it "Okay" || udtdb=
    }
  done
  echo "select dbmsinfo('_version')\g" | sql -s $udtdb >/dev/null 2>&1 ||
  {
    cat <<!

Unable to access $udtdb. Please resolve the problem,
then reissue \`$CMD'.
!
    exit 1
  }
fi

for test in $TESTLIST
{
  cat << !

Running $DEMO/$test.qry . . .
Output file: $outdir/$test.out
!
  touch $outdir/$test.out >/dev/null 2>&1 ||
  {
    cat <<!

Unable to create output file $outdir/$test.out.
!
    iiask4it "Do you want to continue" || exit 10
  }
  sql -s -kf $udtdb < $DEMO/$test.qry > $outdir/$test.out
  diff -w $outdir/$test.out $DEMO/$test.log > $outdir/$test.dif 2>&1 ||
  {
    echo "Differences between the output (<) and the expected results (>):"
    cat $outdir/$test.dif
  }
  [ "$test" != pnum_test ] && { iiask4it "Do you want to continue" || exit 10 ; }
}

exit 0
