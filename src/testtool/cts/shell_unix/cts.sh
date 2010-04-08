:
#
# Copyright (c) 2004 Ingres Corporation
#
## History:
##	15-mar-1991	New cts.sh created for SEPized CTS (lauraw)
##	24-nov-1995	(wadag01)
##			The output files $coordfile and $slavefile are now
##			defined in terms of $TST_OUTPUT (was $ING_TST)
##			which is set in $TST_SHELL/runcts.sh.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
# This is the "cts" command issued by the CTS SEP tests.
# It starts up the CTS coordinator and slaves and waits till they're done.
#
# $1 is database name
# $2 is test number (from test_num table)
# $3 is number of slaves
# $4 is the SEP test id (passed by SEP but not used in Unix)

# the trace flag should always be 0 for this "cts" command.
#
trace_flag=0

# All result files are suffixed ".res".
# Old .res files are removed before the CTS programs start.
# The .res files produced by this test are left so SEP can diff them.

# The Coordinator outputs a file called ctscoord.res. 
#
coordfile=$TST_OUTPUT/output/$4_c

# The Slave executable outputs files called ctsslv1.res, ctsslv2.res, and
# so forth. 
#
slavefile=$TST_OUTPUT/output/$4_

rm -f $coordfile.res
ctscoord $1 $2 $3 $trace_flag > $coordfile.res &
echo " "
echo "Started CTS coordinator"

count=`expr 1` 
until test $count -gt $3
do
   rm -f $slavefile_$count.res
   ctsslave $1 $2 $count $trace_flag > $slavefile$count.res &
   echo " "
   echo "Started CTS slave number $count"
   count=`expr $count + 1`
done
 
echo " "
echo "All CTS slaves are running"
wait

echo " "
echo "CTS coordinator and slaves are done"
