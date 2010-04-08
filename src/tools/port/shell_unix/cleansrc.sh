:
#
# Copyright (c) 2004 Ingres Corporation
#
# Clean out ING_SRC -- remove everything in the subscribed-to directories
# but the piccolo'd source files, and (optionally) remove everything in
# ING_SRC/{bin,lib,man1}.
#
# Flags:
#     -f            -- specifies that a full clean should be done
#                      on ING_SRC/{bin,lib,man1}.
#
#     -l <listfile> -- specify a file instead of using `subscribed'
#                      output; listfile must be in the format of
#                      output from `subscribed'.
#
#     -noming          specifies that generated MING files should
#                      NOT be removed.
#
## History
##	02-aug-1993 (dianeh)
##		Created.
##	13-oct-1993 (dianeh)
##		Bug fixes: check for noming=yes; add -noming to usage;
##		get rid of parens around the first cd to $ING_SRC; add
##		an echo for $ING_SRC/{bin,lib,man1} removed.
##	10-mar-1998 (walro03)
##		Use "p subscribed" instead of "subscribed" to accomodate users
##		who don't alias their Piccolo commands.
##	10-feb-1999 (musro02)
##		Ditto for "p sfiles".  Horrendously bad things happened to those
##		who use this script without "sfiles" being aliased to "p sfiles"
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

CMD=`basename $0`

usage='[ -f ] [ -l <listfile>] [ -noming ]'

cut_cmd=/usr/bin/cut
TMP_FILE=/tmp/sub.out$$

#trap 'rm -f $TMP_FILE' 0 1 2 15

# Parse arguments
while [ $# -gt 0 ]
do
  case "$1" in
         -f) full=yes ; shift
             ;;
        -l*) if [ `expr $1 : '.*'` -gt 2 ] ; then
               SUB_FILE="`expr $1 : '-l\(.*\)'`" ; shift
             else
               [ $# -gt 1 ] ||
               { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
               SUB_FILE="$2" ; limited=yes ; shift ; shift
             fi
             ;;
    -noming) noming=yes ; shift
             ;;
         -*) echo "Illegal flag: $1" ; echo
             echo "Usage:" ; echo "$CMD $usage" ; exit 1
             ;;
          *) echo "Illegal argument: $1" ; echo
             echo "Usage:" ; echo "$CMD $usage" ; exit 1
             ;;
  esac
done

# Check for user root
[ `IFS="()"; set - \`id\`; echo $2` = root ] ||
{ echo "$CMD: You must be root to run this command." ; exit 1 ; }

# Remove files built into the $ING_SRC subsystem directories
if [ "$SUB_FILE" ] ; then
  [ -r $SUB_FILE ] || { echo "$CMD: Cannot read $SUB_FILE." ; exit 1 ; }
else
  cd $ING_SRC ; SUB_FILE=$TMP_FILE ; p subscribed > $SUB_FILE
fi
for dir in `cat $SUB_FILE`
{
  cd $ING_SRC
  [ `echo $dir | tr '!' ' ' | wc -w` -lt 4 ] && continue
  fields=4
  [ "`expr $dir : '\(...\)'`" = "vec" ] && { fields=3 ; prefix="vec31/" ; }
  [ "`expr $dir : '\(...\)'`" = "sep" ] && { prefix="testtool/sep/" ; }
  dirname=`echo $dir | $cut_cmd -d'!' -f${fields}- | sed -e "s/!/\//g"`
  [ -d ${prefix}$dirname ] || \
  { echo "${prefix}$dirname not found" ; continue ; }
  cd ${prefix}$dirname && echo ${prefix}$dirname      # to avoid silent running
  srcfiles="`p sfiles | $cut_cmd -d' ' -f2`"
  lsfiles="`ls`"
  for ii in $lsfiles
  {
    [ "$ii" = "$SUB_FILE" ] && continue
    [ "$noming" ] && { [ "$ii" = "MING" ] && continue ; }
    echo $srcfiles | tr ' ' '\012' | grep -s $ii ||
    { [ ! -d $ii ] && { rm -f $ii && { echo $ii removed ; } ; } ; }
  }
  
  cd $ING_SRC
}

# Remove files built into $ING_SRC/{bin,lib,man1}
[ "$full" ] &&
{
  for dir in bin lib man1
  {
    cd $ING_SRC/$dir && { rm -f * && echo "$dir files removed" ; }
  }
}

echo "$CMD: done."
exit 0
