:
#
# Copyright (c) 2004 Ingres Corporation
#
# updperuse -- peruse differences between sep canons and altconons
#
# History:
#	06-mar-1991 (boba)
#		Written based on old qaperuse
#	26-nov-1991 (boba)
#		Rewrite of outer file selection loop.  Now compare two
#		entire test trees or a single test tree against a piccolo
#		label.  Keep track of what you've checked in 
#		$ING_TST/UPDPERUSE so you won't have to examine things
#		more than once.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

bflag=false
if more < /dev/null > /dev/null 2>&1
then
	more=more
else
	if pg < /dev/null > /dev/null 2>&1
	then
		more=pg
	else
		echo "Cannot find MORE or PG - using CAT."
		more=cat
	fi
fi

# set defaults
batch=false
new_testenv=$ING_TST

# check the args
while [ $# != 0 ]
do
    case $1 in
	-b) batch=true
	    ;;
	-l) shift
	    echo "Getting label $1 ..."
	    orig_label="/tmp/label$$"
	    label -g $1 > $orig_label 2> /dev/null
	    orig_temp="/tmp/updper$$"
	    ;;
	-n) shift
	    new_testenv="$1"
	    ;;
        -o) shift
            orig_testenv="$1"
            ;;
        *)	
            echov "usage: updperuse { -o <orig testenv> | -l <orig label> } [ -n <new testenv> ]"
	    echov "       [ -b ]"
            exit 255
            ;;
    esac
    shift
done

# set directory & file symbols
updok=$new_testenv/UPDOK
touch $updok

if [ -n "$orig_testenv" -a -n "$orig_label" ]
then
	echo "Must secify original testenv directory or label."
	exit 1
fi

echo "Finding and comparing all *.sep files ..."

for newfile in \
    `find $new_testenv -name "*.sep" -print | egrep -v "SCCS|RCS|ORIG"`
do
    if grep "$newfile" $updok > /dev/null
    then
	redo=false
    else
        if [ -n "$orig_testenv" ]
        then
    	    oldfile=`echo $newfile | sed "s#$new_testenv#$orig_testenv#"`
        else
	    oldfile="$orig_temp"
	    newdir=`dirname $newfile`
	    newbase=`basename $newfile`
	    havefile=`(cd $newdir; have $newbase 2> /dev/null | grep $newbase \
	        | sed "s/[ 	].*//")`
	    lrev=`grep "$havefile" $orig_label | \
	        sed "s/^[^ 	]*[ 	]*\([^ 	]*\).*$/\1/"`
	    (cd $newdir; display -r "$lrev" $newbase > $oldfile 2> /dev/null)
        fi
    	if cmp $oldfile $newfile > /dev/null
	then
	    echo "$newfile unchanged"
	    echo "$newfile" >> $updok
	    redo=false
	else
	    $batch || redo=true
	fi
    fi
    while $redo
    do
        redo=false
        echov "\n--- begin: altcompare $oldfile $newfile ---"
        altcompare $oldfile $newfile | $more
        echov "--- end: altcompare $oldfile $newfile ---"
        prompting=true
        while $prompting
        do
            prompting=false
            echov "\nh)elp n)ext a)ltcompare d)iff v)iew s)hell o)k b)ye: \c"
		$bflag && desire=n || read desire
            case $desire in
                [hH]*)
                    cat << !

h)elp		Print this message.
n)ext		Go to next altcompare (default). 
a)ltcompare	Show this altcompare again.
d)iff		Show difference between old and new script.
v)iew		Run view against both old and new scripts.
s)hell		Launch a \$SHELL, csh default.
o)k		Mark difference as OK in $updok so it won't be seen again.
b)ye		Exit updperuse.
!
                    prompting=true
                    ;;
                [bB]*)
                    exit
		    ;;
                [sS]*)
                    ${SHELL-/bin/csh}
                    echov "peruse: continuing..."
		    redo=true
                    ;;
                [aA]*)
                    redo=true
		    ;;
		[dD]*)
            	    echov "\n*** begin: diff $oldfile $newfile ***"
            	    diff $oldfile $newfile | $more
            	    echov "*** end: diff $oldfile $newfile ***"
		    prompting=true
		    ;;
		[vV]*)
		    view $oldfile $newfile
                    prompting=true
		    ;;
		[oO]*)
		    echo "$newfile" >> $updok
		    ;;
                *)
                    ;;
            esac
        done # prompting
    done # redo
done # sep
[ -n "$orig_label" ] && rm -f $orig_label $orig_temp
