:
#
# Copyright (c) 2004 Ingres Corporation
#
# Run the cvnet module test.
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

[ ! -r cvnettest.can ] && echo "Can't find cvnettest.can!" && exit 1

cvnettest | tr "A-F" "a-f" > /tmp/cvnettest.out
diff cvnettest.can /tmp/cvnettest.out > /tmp/cvnettest.dif
if [ -s /tmp/cvnettest.dif ]
then
	echo "Differences from canonic results:"
	cat /tmp/cvnettest.dif
else
	echo "No differences from canonic results."
fi
rm /tmp/cvnettest.out /tmp/cvnettest.dif
