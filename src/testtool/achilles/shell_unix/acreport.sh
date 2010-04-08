:
#
# Copyright (c) 2004 Ingres Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
###############################################################################
## acreport.sh
##      7-Aug-93 (Jeffr) Initial Creationg
##	generate a Report for the USER by sending SIGTERM to Achilles 
##	NOTE 1: PID is acquired from .rpt file
##
###############################################################################

if [ $#  !=  1 ]  
then
	echo "usage: acreport <report file>" 
exit 1
fi

[ -f $1 ] ||
{ 

cat << $

 	file "$1" does not exist  

	please enter valid file or do a KILL -TERM <PID> instead

$
exit 1
}

pid=`cat $1 | grep "ACHILLES" | awk '{print $2}'`  

export pid

[ $pid ] || 
{

cat << $ 

	A valid Pid doesnt exist in "$1" 

	please enter valid file or do a KILL -TERM <PID> instead
$
exit 1
}

kill -TERM $pid  2> /dev/null ||
{ 
cat << !

Invalid Pid from <file> - "$1"  

Please check to see if process "#$pid" is still active !! 
	
!
}
