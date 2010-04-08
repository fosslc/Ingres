:
# Copyright (c) 2005, 2010 Ingres Corporation
#
# Name: iirundbms - invoke the iidbms
#
# Launches the iidbms into the background, waiting for it to issue the
# sign on its stdout that indicates it is alive and well: the value
# of II_DBMS_SERVER.
#
## History:
##	1-Sep-93 (seiwald)
##	    Written to replace iirundbms.c.
##	20-oct-93 (lauraw)
##	    Changed first line to ":" so mkming knows what this is.
##	22-nov-1993 (bryanp) B56521
##	    If the server fails to start, issue the standard messages.
##	29-nov-1993 (peterk) bug 57361
##	    Execute iidbms from $II_SYSTEM/ingres/bin, to avoid inadvertantly
##	    picking up an iidbms from somewhere else in PATH.  This restores
##	    iirundbms to behavior prior to CSoption changes of 10/18/93.
##	28-Jan-1994 (rmuth)  Integrate (seiwald) fix for B59169
##	    Honour the "image_name" parameter in the config.dat.
##	29-Nov-1994 (sweeney) bug 64578
##	    Start the dbms using iirun. See iirun.c in this directory
##	    for why.
##	30-sep-1997 (canor01)
##	    Get the server id from stderr instead of stdout.
##	28-jul-2000 (somsa01)
##	    Amended code so that servers such as GCC and GCB can go through
##	    here as well.
##	26-sep-2001 (somsa01)
##	    Make sure we put out the failure message if we are starting
##	    the DBMS or Recovery servers.
##	10-oct-2001 (somsa01)
##	    Make sure we redirect stderr to /dev/null when using grep
##	    to avoid OK error messages from printing out to the user.
##	26-sep-2002 (devjo01)
##	    Add "-rad=radid" support for NUMA.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##	06-Apr-2005 (hanje04)
##	    Replace references to Installation and Operations guide with
##	    Getting Started guide.
##	12-apr-2005 (devjo01)
##	    Allow iirundbms to spawn commands which will not echo a
##	    PASS or FAIL (I.e. iigcn, rmcmd).  This is part of a
##	    change to assure that detached processes cleanly disassociate
##	    themselves from the starting shell, allowing telnet & rsh/ssh
##	    sessions to exit after starting a server.
##	03-aug-2005 (toumi01)
##	    Call iipthreadtst and bail out if it tells us that we are
##	    running an NPTL Ingres version on a non-NPTL Linux platform.
##	15-Sep-2005 (hanje04)
##	    BUG 115251
##	    Check binary we're trying to start actually exists. If it doesn't
##	    FAIL and return apropriate message.
##      07-Oct-2005 (hweho01)
##          Star 14259755
##          Currently on AIX, the address space for a 64-bit child is 
##          limited to 32-bit parent's if the 32-bit binary is built with    
##          option maxdata. In order to circumvent this situation, need 
##          to start up the 64-bit servers directly.  
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##      06-Sep-2007 (rosan01) integrating contributed update from kschendel (c108)
##        BUG 118144
##        Revise the wait loop after iirun.  It's a waste, but more
##        importantly, grep on a64_lnx appears to be occasionally
##        returning "ok" when run against an empty file, causing a
##        spurious FAIL when a long recovery is running.
##      29-Apr-2008 (coomi01)
##        BUG 120312
##        If the forked child of iirun dies unexpectedly without
##        writing PASS/FAIL to the tmp file, we looped forever.
##        We now add a simple test to see if the process is alive.
##	  (kschendel) which lost 118144 fix, restore empty-file test.
##	16-Feb-2010 (hanje04)
##	  SIR 123296
##	  Add support for LSB builds

##
# Pick an executable
# If ii.host.dbms.flavour.image_name is set uset it ( relative to
# $II_SYSTEM/ingres/bin if unrooted); otherwise use the default iidbms.
(LSBENV)

host=`iipmhost`
radarg=""
[ "$1" ] && [ -z "`echo $1 | sed 's/-rad=[1-9][0-9]*//'`" ] && 
{
    radarg="$1"
    shift
}

arg1=${1-'dbms'}
arg2=${2-'*'}

x=`iigetres "ii.$host.$arg1.$arg2.image_name"`

case "$x" in
"")	if [ "$1" = "recovery" ] ; then
	    iidbms=$II_SYSTEM/ingres/bin/iidbms
	elif [ "$1" = "rmcmd" ] ; then
	    iidbms=$II_SYSTEM/ingres/bin/rmcmd
	else
	    iidbms=$II_SYSTEM/ingres/bin/ii$1
	fi
	;;
/*)	iidbms=$x;;
*)	iidbms=$II_SYSTEM/ingres/bin/$x;;
esac

x=`echo "$iidbms" | sed 's/.*\\/\\([^\\/]*\)$/\\1/'`

unames=`uname -s`
if [ "$unames" = "AIX" ] ; then
   enable_64=`ingprenv II_LP64_ENABLED`
   case "$enable_64" in
    TRUE|true|ON|on) if [ "$x" = "iidbms" ] ; then
                       iidbms=$II_SYSTEM/ingres/bin/lp64/iidbms
                     elif [ "$x" = "iistar" ] ; then
                       iidbms=$II_SYSTEM/ingres/bin/lp64/iistar
                     fi
   esac
fi

# check the binary actually exists and fail out if it doesn't
[ ! -f "$iidbms" ] &&
{
    cat << EOF
iirundbms: could not locate server
The following executable could not be located:

    $iidbms
	
To stop this error, set the startup count for this server
to 0 using the (PROG0PRFX)cbf utility.
EOF

    exit 1
}

case "$x" in
iidbms|iigcb|iigcc|iigcd|iijdbc|iistar)
    # launch the program into the background, waiting for a single line 
    # containing either a PASS or FAIL on stdout

    iipthreadtst
    if [ "$?" = "1" ] ; then
	exit 1
    fi

    # iirun executes in the foreground and will return 
    #    (a) If PASS/FAIL written to poll file
    # or (b) Should the utlimate child deamon exited unexpectedly
    tmpfile=`ingprenv II_TEMPORARY`/iirundbms.$$
    cat /dev/null > $tmpfile
    iirun $radarg $iidbms $* pollFile $tmpfile >> $tmpfile 2>&1


    # Assume the worse
    FAILED=yes
    if [ -s "$tmpfile" ] ; then
	if grep "PASS" $tmpfile > /dev/null 2>&1 ; then
	    # But got it wrong
	    FAILED=no
	fi
    fi

    # If passed, output the II_DBMS_SERVER line that ought to be in
    # the stdout file.  Otherwise, output whatever and bail out.
    # Meaningful reasons should for failure should be found in
    # the errlog.log.

    grep -v "PASS" $tmpfile

    if [ "$FAILED" = 'no' ] ; then
	rm -f $tmpfile
	exit 0
    fi

    if [ "$1" = "recovery" -o "$1" = "dbms" ] ; then
	echo ""
	echo "iirundbms: server would not start."
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
    ;;

*)
    # All other executables (at present: iigcn, rmcmd) do not output
    # a PASS / FAIL.  Confirmation of actual success is the responsibility
    # of the caller.
    iirun $radarg $iidbms $* > /dev/null 2>&1 
    exit 0
    ;;
esac
