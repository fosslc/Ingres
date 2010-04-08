:
# Copyright (c) 2004, 2010 Ingres Corporation
#
# Name:
#	ingprsym	- print single INGRES symbol or environment variable
#
# Usage:
#	ingprsym symbol
#
# Description:
#	Print out the value of an INGRES "symbol" as it would be evaluated
#	in an INGRES executable, i.e. if an environment variable named
#	"symbol" exists, print its value, otherwise print the value obtained
#	from the INGRES symbol table, otherwise exit.
#	
# Exit status:
#	0	OK
#	1	symbol argument not present
#
## History:
##	9-nov-88 (peterk) - created
##
##	24-Jul-90 (achiu) 
##		get rid of "DEST = bin" to change ming hint to put ingprsym in
##		in utility instead of bin as specified by dlist
##	22-jan-1991 (jonb)
##		Restore "DEST = bin", because that's what dlist says now.
##	15-mar-1993 (dianeh)
##		Changed call to ingprenv1 to ingprenv (ingprenv1 gone for 6.5);
##		cleaned up Description text. 
##	14-Apr-1999 (peeje01)
##		Add evaluation of sub variables to support 
##              e.g. II_GCN${II_INSTALLATION}_LCL_VNODE
##              Usage Note: Protect the ${..} construct from the shell
##                    with single quotes.
##	21-apr-2000 (somsa01)
##		Updated MING hint for program name to account for different
##		products.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB build.
#
#  PROGRAM = (PROG2PRFX)prsym

(LSBENV)

# if being called from iigenres, redirect to  output file (arg 2)
if test $# -eq 3
then
    OUT="> $2"
fi

[ $1 ] || exit 1
if ( echo $1 | egrep '\$\{.+\}' > /dev/null 2>&1 )
then
    # extract the sub Symbol name, between '${' and '}'
    # eg '$II_GCN${II_INSTALLATION}_LCL_VNODE' will result
    # in subSym being set to II_INSTALLATION
    # Be sure to protect the whole with single quotes "'"
    # from accidental evaluation by the shell
    #
    # Reminder: '.*' like all REs matches the longest string
    subSym=`echo $1 | sed 's/.*\${\(.*\)}.*/\1/'`
    subSymVal=`(PROG2PRFX)prenv $subSym`
    # replace the sub Symbol name with its value
    fullSym=`echo $1|sed 's/\(.*\)\${\('$subSym'\)}\(.*\)/\1'$subSymVal'\3/'`
    eval (PROG2PRFX)prenv $fullSym $OUT
else
    val=`eval echo \\$$1`
    if [ $val ]
    then
	eval echo $val $OUT
    else
	eval (PROG2PRFX)prenv $1 $OUT
    fi
fi
exit 0
