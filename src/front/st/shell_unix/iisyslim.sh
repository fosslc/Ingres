:
#  Copyright (c) 2004, 2010 Ingres Corporation	
#
#  Name: iisyslim	
#
#  Usage:
#	iisyslim -used_segs | -used_sems | -used_sets | -total_swap | 
#		-free_swap  [ outfile ]
#
#  Description:
#	Called by syscheck to get various measures of system resources
#	which are only available only in the form of textual output from
#	system commands.
#
#  Exit Status:
#	0	ok
#	1	unable to read /dev/kmem or bad argument.
#
## History:
##	18-jan-93 (tyler)
##		Created.	
##	18-feb-93 (tyler)
##		Changed message displayed if /dev/kmem isn't readable
##		to use the name of the current user, rather than "ingres",
##		in case the system adminstrator lets users other than
##		"ingres" run syscheck.
##	01-nov-93 (vijay)
##		rs/6000 does not read kmem. Use read_kmem variable.
##	22-nov-93 (tyler)
##		Replaced system calls with shell procedure invocations.
##	15-jan-1996 (toumi01; from 1.1 axp_osf port)
##		Changed use of IPCSCMD to IPCSCMD2.  For bug #60263 on axp_osf,
##		IPCSCMD was changed to post-process its output to hex - this
##		causes problems when used here.	 Hence IPCSCMD2 (without the
##		post-processing) has been created (see mksysdep.sh) for use
##		here.
##	25-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##	08-Mar-2004 (hanje04)
##	 	SIR 107679
##	  	Added support for Intel Linux (int_lnx) to improve effectiveness
##	 	of syscheck.
##	28-Dec-2004 (kodse01)
##	    Added support for other Linux (i64_lnx, a64_lnx, ibm_lnx) also.
##	18-Jan-2005 (hanje04)
##		Add support for other Linuxes.
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##      13-Sep-2007 (hweho01)
##          Turned off library test in iisysdep. 
##      20-Nov-2007 (hweho01)
##          Replaced awk with $AWK which is defined in iisysdep as the 
##          working_awk for all platforms, avoid error on AMD/Solaris.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##
#  PROGRAM = (PROG4PRFX)syslim
#
#  DEST = utility
#--------------------------------------------------------------------------
(LSBENV)

LibTest_in_iisysdep=false
. (PROG1PRFX)sysdep

if [ "$READ_KMEM" = true -a  ! -r /dev/kmem ]
then
   cat << !

 -----------------------------------------------------------------------
|			    ***WARNING***				|
|									|
|  /dev/kmem is not readable by $WHOAMI.
|									|
|  $WHOAMI must have read permission on /dev/kmem in order to check
|  system resources.  To change permissions on /dev/kmem, you must be	|
|  logged on as root.							|
|                                                                       |
 -----------------------------------------------------------------------

!
   exit 1
fi

case "$1" in

   -used_segs)
	case $VERS in
	    *_lnx|int_rpl)
	   	USED_SEGS=`$IPCSCMD2 -m | grep "^0x" | wc -l`
		;;
	        *)
		USED_SEGS=`$IPCSCMD2 -m | grep "^0x" | wc -l`
		;;
	esac
	USED_SEGS=`expr $USED_SEGS \+ 0`
	RESULT=$USED_SEGS
      ;;

   -used_sems)
	case $VERS in
	    *_lnx|int_rpl)
		USED_SEMS=`$IPCSCMD2 -s | grep "^0x" | $AWK '{s += $5} END {print s}'`
		;;
	        *)
      		USED_SEMS=`$IPCSCMD2 -b -s | grep "^s" | $AWK '{s += $7} END {print s}'`
		;;
	esac
	if [ -z "$USED_SEMS" ]
	then
	    USED_SEMS=0
	fi
	RESULT=$USED_SEMS
      ;;

   -used_sets)
	case $VERS in
	    *_lnx|int_rpl)
		USED_SETS=`$IPCSCMD2 -s | grep "^0x" | wc -l`
		;;
	        *)
		USED_SETS=`$IPCSCMD2 -b -s | grep "^s" | wc -l`
		;;
	esac
	USED_SETS=`expr $USED_SETS \+ 0`
	RESULT=$USED_SETS
      ;;

   -total_swap)
      if $CHECK_SWAP_SPACE
      then
         STRIPJUNK="sed 's/[^0-9]//'"
         if [ "$PSTAT_TOTAL_ARG" != "" ]
         then
            TOTAL_SWAP=`eval "$PSTAT | $AWK '{ print \\\$$PSTAT_TOTAL_ARG }' \
               | $STRIPJUNK"`
            TOTAL_SWAP=`expr $TOTAL_SWAP \* $PSTAT_KBYTE_FACTOR`
         else
            FREE_SWAP=`eval "$PSTAT | $AWK '{ print \\\$$PSTAT_REMAIN_ARG }' \
               | $STRIPJUNK"`
            FREE_SWAP=`expr $FREE_SWAP \* $PSTAT_KBYTE_FACTOR`
            USED_SWAP=`eval "$PSTAT | $AWK '{ print \\\$$PSTAT_USED_ARG }' \
               | $STRIPJUNK"`
            USED_SWAP=`expr $USED_SWAP \* $PSTAT_KBYTE_FACTOR`
            TOTAL_SWAP=`expr $USED_SWAP + $FREE_SWAP`
         fi
         RESULT=$TOTAL_SWAP
      else
         RESULT=-1
      fi
      ;;

   -free_swap)
      if $CHECK_SWAP_SPACE
      then :
         STRIPJUNK="sed 's/[^0-9]//'"
         FREE_SWAP=`eval "$PSTAT | $AWK '{ print \\\$$PSTAT_REMAIN_ARG }' \
            | $STRIPJUNK"`
         FREE_SWAP=`expr $FREE_SWAP \* $PSTAT_KBYTE_FACTOR`
         RESULT=$FREE_SWAP
      else
         RESULT=-1
      fi
      ;;
   
   -*)
      echo "(PROG1PRFX)syslim: unrecognized argument: $1"
      exit 1
      ;;
esac
   
# send output to outfile if passed
if [ "$2" ]
then
   echo $RESULT > $2
else
   echo $RESULT
fi

exit 0

