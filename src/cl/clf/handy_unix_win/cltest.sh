:
#
#Copyright (c) 2004 Ingres Corporation
#
# Special CL test suite
#
## History:
##	15-Mar-89 (GordonW)
##		created
##	17-Mar-89 (GordonW)
##		added lktest
##
##	20-Mark-89 (markd)
##		added csspintest
##	08-may-89 (daveb)
##		add mecopytest
##	10-may-1989 (boba)
##		Moved to clf/handy so it will be built automatically.
##	07-jan-1992 (bonobo)
##		Added the following changes from ingres63p cl: 09-nov-89 (rog),
##		23-jan-1991 (jonb).
##	09-nov-89 (rog)
##		Added a flag, -f, to force all tests to run instead of
##		exiting at the first failure.
##	23-jan-1991 (jonb)
##		Remove tests that don't exist any more.  Change the
##		numbering scheme to make it easy to add and remove stuff.
##	03-feb-93 (swm)
##		Make cd command go to the right place in the BAROQUE
##		environment.
##	10-jun-93 (dianeh)
##		Removed call to lkmemtest and lktest -- gone for 6.5;
##		changed History comment-block to double pound-signs.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	4-Oct-2004 (drivi01)
##		Due to merge of jam build with windows, some directories
##		needed to be updated to reflect most recent map.
#

[ -n "$ING_VERS" ] && noise="/$ING_VERS/src"

FORCE_ALL_TESTS=false

while [ $# != 0 ]
do
	case $1 in
		-f) FORCE_ALL_TESTS=true ;;
	esac
	shift
done

tmpfile=/tmp/cltest.$$
testnum=0
incrtest='testnum=`expr $testnum + 1`'

# test the CS threading
eval $incrtest
echo "$testnum) Testing CS threading"
csphil >$tmpfile 2>&1
return=$?
# On normal server shutdown csphil returns 1
if [ "$return" != "0" ]
then
	echo "CSPHIL failed: returned: $return"
	cat $tmpfile
	rm -f $tmpfile
	$FORCE_ALL_TESTS || exit 1
fi

# test the shared memory
eval $incrtest
echo "$testnum) Testing Shared Memory"
csshmemtest >$tmpfile 2>&1
return=$?
if [ "$return" != "0" ]
then
	echo "CSSHMEMRTEST failed: returned: $return"
	cat $tmpfile
	rm -f $tmpfile
	$FORCE_ALL_TESTS || exit 1
fi

# Test the CV code
eval $incrtest
echo "$testnum) Testing CV->net"
cd $ING_SRC/cl/clf$noise/cv_unix_win
testcvnet >$tmpfile 2>&1
return=$?
if [ "$return" != "0" ]
then
	echo "CVNET failed: returned: $return"
	cat $tmpfile
	rm -f $tmpfile
	$FORCE_ALL_TESTS || exit 1
fi

# Test of ERrport
eval $incrtest
echo "$testnum) Testing ERreport facility"
ercodeexam s >$tmpfile 2>&1
return=$?
if [ "$return" = "0" ]
then
	ercodeexam c >$tmpfile 2>&1
	return=$?
fi
if [ "$return" != "0" ]
then
	echo "ERCODEEXAM failed: returned: $return"
	cat $tmpfile
	rm -f $tmpfile
	$FORCE_ALL_TESTS || exit 1
fi
ercallexam >$tmpfile 2>&1
return=$?
if [ "$return" != "0" ]
then
	echo "ERCALLEXAM failed: returned: $return"
	cat $tmpfile
	rm -f $tmpfile
	$FORCE_ALL_TESTS || exit 1
fi

# Test CS TAS and spinlock code
eval $incrtest
echo "$testnum) Testing CS_getspin and CS_tas"
for i in 1 2 5 10 20
do
	csspintest 500 $i > $tmpfile 2>&1
	return=$?
	if [ "$return" != "0" ]
	then
		echo "CSSPINTEST using CS_getspin failed: returned: $return"
		cat $tmpfile
		rm -f $tmpfile
		$FORCE_ALL_TESTS || exit 1
	fi
done
rm -f $tmpfile
for i in 1 2 5 10 20
do
	csspintest -t 500 $i > $tmpfile 2>&1
	return=$?
	if [ "$return" != "0" ]
	then
		echo "CSSPINTEST using CS_tas failed: returned: $return"
		cat $tmpfile
		rm -f $tmpfile
		$FORCE_ALL_TESTS || exit 1
	fi
done

rm -f $tmpfile

# Test the MECOPY macros
eval $incrtest
echo "$testnum) Testing MECOPY macros"
mecopytest >$tmpfile 2>&1
return=$?
if [ "$return" != "0" ]
then
	echo "mecopytest failed: returned: $return"
	cat $tmpfile
	rm -f $tmpfile
	$FORCE_ALL_TESTS || exit 1
fi

exit 0
