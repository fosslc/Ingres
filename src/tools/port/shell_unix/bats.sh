:
#
# Copyright (c) 2004 Ingres Corporation
#
# Build Another Tools Set from source in $ING_TST
#
## History:
##	28-apr-1993 (dianeh)
##		Created.
##	13-may-93 (swm)
##		If a private copy of tools is being built add
##		$ING_SRC/$ING_VERS/bin to path rather than $ING_SRC/bin.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

CMD=`basename $0`

usage='[ -d <dirname>[,<dirname>...n ] ] [ -n ] [ -help ]

      -d     Build only the named directory(s), not the default
             directories ($ING_TST/{be,stress}).
             NOTE: Directories must be in a comma-separated list
             (ex.: be,stress).
      -n     Show what would build, but do not execute.
   -help     Get this usage statement.
'

: ${ING_BUILD?"must be set"} ${ING_SRC?"must be set"} ${ING_TST?"must be set"}

# Determine whether a private copy of the tools is being/has been built
[ -n "$PRIVATE_TOOLS" ] && privtools="/$ING_VERS"

PATH=$ING_SRC$privtools/bin:/usr/local/bin:$PATH

ensure ingres rtingres || exit 1

# Parse arguments
while [ $# -gt 0 ]
do
  case $1 in
    -help) echo "" ; echo "$CMD $usage" ; echo ""
           exit 0
           ;;
       -d) [ $# -gt 1 ] || \
           { echo "" ; echo "$CMD: Illegal syntax." ; \
             echo "" ; echo "$CMD $usage" ; exit 1 ; }
           dir_args="`echo $2 | tr ',' ' '`"
           for dir in $dir_args
           {
             [ -d $ING_TST/$dir ] || \
             { echo "" ; echo "$CMD: Directory $ING_TST/$dir not found." ; \
               echo "Ignoring..." ; continue ; }
             DIRS="$DIRS $dir"
           }
           shift ; shift
           ;;
       -n) MKOPTS="-n"
           shift
           ;;
       -*) echo "" ; echo "$CMD: Illegal flag: $1"
           echo "" ; echo "$CMD $usage" ; echo ""
           exit 1
           ;;
        *) echo "" ; echo "$CMD: Illegal argument: $1"
           echo "" ; echo "$CMD $usage" ; echo ""
           exit 1
           ;;
  esac
done

[ "$DIRS" ] || { DIRS="be stress" ; }

for dir in $DIRS
{
  case $dir in
        be) cd $ING_TST/be/msfc/shell
            echo "==== $ING_TST/be/msfc/shell: mk shell"
            mk $MKOPTS shell
            cd $ING_TST/be/msfc/src
            echo "==== $ING_TST/be/msfc/src: mk lib"
            mk $MKOPTS lib
            echo "==== $ING_TST/be/msfc/src: mk"
            mk $MKOPTS
            ;;
    stress) cd $ING_TST/stress/gcf/esql
            echo "==== $ING_TST/stress/gcf/esql: mk"
            mk $MKOPTS
            ;;
  esac
}

echo "$CMD: done."
exit 0

