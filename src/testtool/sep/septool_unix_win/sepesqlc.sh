:
#
# Copyright (c) 2004 Ingres Corporation
#
# sepesqlc -- Preprocesses an Esql/C or Equel/C script.
#
## History:
##	05-Jul-91 (DonJ)
##		Added this file to Ingres63p.
##	06-Aug-91 (DonJ)
##		Added capability to do quel as well as sql.
##		Cleaned up a bit, taking out variable,
##		ofilnam. Wasn't used for anything. Handled
##		case where file extension is left off in a
##		better manner, looking for a *.sc then a
##		*.qc file. I still don't know what this
##		"ifiltext" is, leaving it in until I'm more
##		sure its' not needed. Check for existence
##		of file before trying to precompile, error
##		if not found.
##	07-Aug-91 (DonJ)
##		Removing ifilext, no reasonable function.
##      20-Aug-91 (DonJ)
##              Simplified conditional statements. Made other changes
##              suggested by Roger Taranto.
##      27-Aug-91 (DonJ) 
##              More improvements.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
## 
#

if [ $# -eq 0 ]
then
    echo ""
    echo "Usage: sepesqlc <file1> [<file2> ... <filen>]"
    echo ""
    echo "        -c[onditional]   Conditional Processing. Preprocess only if"
    echo "                         source file does not exist."
    echo ""
    exit 0
fi

cflag=false

for i
do
    case $i in
	-conditional | -conditiona | -condition | -conditio |	\
	-conditi | -condit | -condi | -cond | -con | -co | -c )	\
		cflag=true;;

	-* )	echo "* bad flag $i";;

	*.* | * ) ;;
    esac
done

for i
do
    do_it=false
    pcccmd='esqlc'
    case $i in
	-* )    ;;

	*.sc )  esfilnam="$i"
		filnam=`basename $i .sc`
		cfilnam="$filnam.c"
		do_it=true
		pcccmd='esqlc'
		;;

	*.qc )  esfilnam=$i
		filnam=`basename $i .qc`
		cfilnam="$filnam.c"
		do_it=true
		pcccmd='eqc'
		;;

	* )     filnam="$i"
		esfilnam=`basename $i.sc`
		if [ -f `basename $i.sc` ]
		then
		    esfilnam=`basename $i.sc`
		    do_it=true
		    cfilnam="$i.c"
		    pcccmd='esqlc'
		elif [ -f `basename $i.qc` ]
		then
		    esfilnam=`basename $i.qc`
		    do_it=true
		    cfilnam="$i.c"
		    pcccmd='eqc'
		else
		    esfilnam=`basename $i`
		    do_it=true
		    pcccmd='esqlc'
		fi
		;;
    esac

    if [ $do_it -a ! -f $esfilnam ]
    then
	echo " file $esfilnam not found..."
	do_it=false
    fi

    $do_it && $cflag && [ -f $cfilnam ] && do_it=false

    if $do_it
    then
	$pcccmd -f$filnam.c $esfilnam
    fi
done

