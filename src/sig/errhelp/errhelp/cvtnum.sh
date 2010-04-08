:
# Copyright (c) 2004 Ingres Corporation 
# This script requires two arguments.  The first argument is either -h or -d.
# -h tells the script to convert the second argument from base 10 to base 16.
# -d tells the script to convert the second argument from base 16 to base 10.
#
# History:
# 	08-Mar-1990 (rog)
#		Written.

[ $# != 2 ] && {
	echo "Usage: $0 -[hd] number"
	exit 1
}

number=`echo $2 | tr '[a-z]' '[A-Z]'`

case $1 in
	-d) ibase="ibase=16"
	    obase=""
	    echo "$number" | grep '[0-9A-F]' > /dev/null 2>&1 || {
		echo "$2 is not a hexidecimal number.  Please try again."
		exit 1
	    }
	    ;;
	-h) ibase=""
	    obase="obase=16"
	    echo "$number" | grep '[0-9]' > /dev/null 2>&1 || {
		echo "$2 is not a decimal number.  Please try again."
		exit 1
	    }
	    ;;
	*)
	    echo "Usage: $0 -[hd] number"
	    exit 1
	    ;;
esac

result=`bc << _EOF_
$ibase
$obase
$number
_EOF_`

echo $result
exit 0
