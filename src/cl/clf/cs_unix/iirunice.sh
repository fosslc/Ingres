:
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Name: iirunice - invoke the icesvr
#
# Launches the icesvr into the background, waiting for it to issue the
# sign on its stdout that indicates it is alive and well: the value
# of II_DBMS_SERVER.
#
## History:
##	13-jan-1999 (peeje01)
##	    Created
##	    Based on iirundbms
##	04-sep-2000 (somsa01)
##	    Amended code to print out the server id as well.
##	10-oct-2001 (somsa01)
##	    Make sure we redirect stderr to /dev/null when using grep
##	    to avoid OK error messages from printing out to the user.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	06-Apr-2005 (hanje04)
##	    Replace references to Installation and Operations guide with
##	    Getting Started guide.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
# Pick an executable
# If ii.host.icesvr.flavour.image_name is set uset it ( relative to
# $II_SYSTEM/ingres/bin if unrooted); otherwise use the default icesvr.
(LSBENV)

host=`iipmhost`
arg1=${1-'ice'}
arg2=${2-'*'}

x=`iigetres "ii.$host.$arg1.$arg2.image_name"`

case "$x" in
"")	icesvr=$II_SYSTEM/ingres/bin/icesvr;;
/*)	icesvr=$x;;
*)	icesvr=$II_SYSTEM/ingres/bin/$x;;
esac

# launch the icesvr into the background, waiting for a single line 
# on stdout

tmpfile=`ingprenv II_TEMPORARY`/iirunice.$$
iirun $icesvr $* > $tmpfile 2>&1 &
NOT_DONE=true
NOT_FAILED=true
while $NOT_DONE ; do
    if grep "PASS" $tmpfile > /dev/null 2>&1 ; then
      NOT_DONE=false
    elif grep "FAIL" $tmpfile > /dev/null 2>&1 ; then
      NOT_FAILED=false
      NOT_DONE=false
    fi
done

# If the line is II_DBMS_SERVER = whatever, server is up.  Else,
# bail.  Meaningful reasons should for failure should be found in
# the errlog.log.

grep -v "PASS" $tmpfile
if $NOT_FAILED ; then
    rm -f $tmpfile
    exit 0
else
    if grep "II_DBMS_SERVER" $tmpfile > /dev/null ; then
	echo "iirunice: server would not start."
	echo "        II_SYSTEM must be set in your environment."
	echo "        Has the csinstall program been run?"
	echo "        II_DATABASE, II_CHECKPOINT, II_JOURNAL and II_DUMP"
	echo "            must also be set.  See II_CONFIG/symbol.tbl."
	echo "        Check the file '$II_SYSTEM/ingres/files/errlog.log'"
	echo "            for more details concerning internal errors."
	echo "        See the Installation Guide for more"
	echo "            information concerning server startup."
    fi
    rm -f $tmpfile
    exit 1
fi
