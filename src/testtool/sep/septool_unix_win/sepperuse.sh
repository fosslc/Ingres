:
#
# Copyright (c) 2004 Ingres Corporation
#
# sepperuse -- peruse sep tests with differences
#
# History:
#	17-jan-1990 (rog)
#		Written.
#	24-jan-1990 (boba)
#		Allow for override of default output directory with -o
#		option or by setting $tst_output.  Peruse all .log
#		files regardless of the .cfg or .out files with the -a
#		option.  Add usage message.  Allow for multiple .cfg
#		files.  Make more & pg search for "DIFF" if -d is 
#		specified.  Misc. fixes.
#	25-jan-1990 (boba)
#		Fix bug in creation of list for -a option (don't include
#		full path).  The Delete option wasn't deleting files.
#		Also, remove "-f" for delete option so it TELLS you if
#		something is wrong.
#	02-dec-1991 (boba)
#		Remove "delete" option and, instead, add "ok" option to
#		keep track of valid differences during audit.  Ok list
#		is kept in $ING_TST/output/SEPOK.  Also, clean up help
#		message still based on qaperuse diffs, not sep log files.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

: must be set ${ING_TST?} ${TST_LISTEXEC?}

if more < /dev/null > /dev/null 2>&1
then
	more=more
	diffscan="+/DIFF"
else
	if pg < /dev/null > /dev/null 2>&1
	then
		more=pg
		diffscan="+/DIFF/"
	else
		echo "Cannot find MORE or PG - using CAT."
		more=cat
	fi
fi

# check the args
alllog=false
moreparam=""
while [ $# != 0 ]
do
    case $1 in
	-a) alllog=true
	    ;;
	-d) moreparam="$diffscan"
	    ;;
	-o) shift
	    ( cd $1 ) || exit 1
	    tst_output=`cd $1; pwd`
	    ;;
	-*) echov "usage: sepperuse [ -a ] [ -o <outdir> ] [ <cfg-file-name> ... ]"
	    exit 255
	    ;;
	*)  files="$files $*"; break
	    ;;
    esac
    shift
done

[ "$files" = "" ] && nocfg=true || nocfg=false


# set directory & file symbols
tst_output=${tst_output-$ING_TST/output}
sepok=$tst_output/SEPOK
touch $sepok

cd $tst_output

if $alllog
then
    list="`ls *.log`"
else
    allout=/tmp/peruse.$$
    if $nocfg
    then
	cat $tst_output/*.out > $allout
    else
        for file in $files
        do
	    [ -r $TST_LISTEXEC/$file ] || {
	        echov "$TST_LISTEXEC/$file does not exist!"
	        exit 1
	    }

	    out=`echov $file | sed 's/.cfg/.out/'`

	    if [ -r $out ]
	    then
		cat $out >> $allout
	    else
	        echov "Cannot find $out SEP output file.  Have you run the $file tests?"
	        exit 1
	    fi
        done
    fi

    list=`awk '
        /\=\>/ {
	    while (index($4,"/") != 0) {
	        $4 = substr($4,index($4,"/")+1)
	    }
	    $4 = substr($4,1,index($4,"."))
	    save = sprintf("%slog",$4)
        }

        /Number of diff/ {
	    if ($6 != 0)
	        print save
        }
    ' < $allout`

    rm -f $allout
fi

for i in $list
do
    if grep "$i" $sepok > /dev/null
    then
	redo=false
    else
	redo=true
    fi
    while $redo
    do
	redo=false
	echov "\n--- begin ($tst_output/$i) ---"
	$more $moreparam $i
	echov "--- end ($tst_output/$i) ---"
	prompting=true
	while $prompting
	do
	    prompting=false
	    echov "\nh)elp n)ext r)epeat p)rint o)k s)hell b)ye: \c"
	    read desire
	    case $desire in
		[hH]*)
		    cat << !

h)elp		Print this message.
n)ext		Go to next log file, leaving this for later perusal.
r)epeat		Show this log file again.
p)rint		Print the log file with lpr -p.
o)k		Mark log file as OK in $sepok so it won't be seen again.
s)hell		Launch a $SHELL, csh default.
b)ye		Exit sepperuse.
!
		    prompting=true
		    ;;
		[bB]*)
		    exit
		    ;;
		[sS]*)
		    ${SHELL-/bin/csh}
		    echov "sepperuse: continuing..."
		    redo=true
		    ;;
		[oO]*)
		    echo "$i" >> $sepok
		    ;;
		[pP]*)
		    (lpr -p $tst_output/$i &)
		    redo=true
		    ;;
		[rR]*)
		    redo=true
		    ;;
		*)
		    ;;
	    esac
	done # prompting
    done # redo
done # file
