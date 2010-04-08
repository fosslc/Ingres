:
#
# Copyright (c) 2004, 2010 Ingres Corporation
#
#############################################################################
##
## Name: setingreslanguage.sh
##
## Purpose: Set language for Ingres messages
##
## History:
##	03-Nov-2004 (hanje04)
##	    Created.
##	17-Dec-2004 (hanch04)
##	    Added [fra|deu|esn|ita|ptb or jpn]
##	05-Jan-2005 (gupsh01)
##	    Added Simplified Chinese.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds

(LSBENV)

self=`basename $0`

usage()
{
    cat << !
Usage:
	$self [fra|deu|esn|ita|ptb|sch or jpn]

!
    exit 1
}

# Check command args
case "$1" in
    fra|\
    deu|\
    esn|\
    ita|\
    ptb|\
    sch|\
    jpn)
		;;
	   *)
	   	usage
		;;
esac

# Check environment
[ "$II_SYSTEM" ] || 
{
    cat << !
Error:
	II_SYSTEM must be set before running this script.

!
exit 2
}

cat << !
Setting the Ingres Language Environment to $1

!

# Create language dir and copy in files
[ ! -d "$II_SYSTEM/ingres/files/$1" ] && mkdir -p $II_SYSTEM/ingres/files/$1
cp $1/* $II_SYSTEM/ingres/files/$1
ingsetenv II_LANGUAGE $1
