:
#
# Copyright (c) 2004 Ingres Corporation
#
#	Remove all SEP and LISTEXEC temporary files from $ING_TST 
#	and all log output files other than in $ING_TST/output.
#
# History:
#	26-sep-1989 (boba)
#		Created
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

echo "Removing all SEP and LISTEXEC temporary files from $ING_TST ..."
for tfm in '*.stf' '*.syn' 'in_*.sep' 'out_*.sep' '*.dif' 
do
	find $ING_TST -type f -name $tfm -print
done | while read tf
do
	echo Removing $tf
	rm -f $tf
done

echo "Removing all SEP and LISTEXEC log output files from $ING_TST"
echo "	except in $ING_TST/output ..."
for outm in '*.log' '*.out' '*.rpt' 
do
	find $ING_TST -type f -name $outm -print
done | grep -v output | while read out
do
	echo Removing $out
	rm -f $out
done
