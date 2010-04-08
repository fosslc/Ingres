:
#
# Copyright (c) 2004 Ingres Corporation
#
# MTS - submit the mtscoord process and
#	the requested number of mtslave process
#
#
# $1 is database name
# $2 is test id
# $3 is number of slaves
# $4 i traceflag
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

rm -f mts$2_cord.res
mtscoord $1 $2 $3 $4 > mts$2_cord.res &
echo " "
echo "Started MTS coordinator"

#
# add this to help in multi-STAR configurations -
# the START table is destroyed by the setup
# sql, then re-created by the coordinator.
# It is possible for the slaves to run, and
# exhaust their sleep time before the coordinator
# can read all the slaves are up and create the table
# again.  This lets the coordinator get a head start
# on the slaves.
#	jds	2/14/92
#

sleep 60

count=`expr 1`
until test $count -gt $3
do
   rm -f mts$2_$count.res
   mtsslave $1 $2 $count $4 > mts$2_$count.res &
   echo " "
   echo "Started MTS slave number $count"
   count=`expr $count + 1`
done

echo " "
echo "All MTS slaves are running"
wait

echo " "
echo "MTS coordinator and slaves are done"
