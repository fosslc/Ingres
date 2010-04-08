:
#
# Copyright (c) 2004 Ingres Corporation
#
# Do a forced get of all the source files in all the subscribed-to
# source directories.  Or do a subset of the directories, based on
# a list-file specifed with the -l flag (note: the list-file must
# be in the format of output from `subscribed').
#
## History
##	28-apr-1993 (dianeh)
##		Created.
##	09-aug-1993 (dianeh)
##		Added the option of specifying a list-file instead of having
##		to refresh the whole source tree; fixed the vec31 stuff.
##	23-aug-1993 (dianeh)
##		Oops -- left my debugging "echo" in instead of changing it
##		to the "get -f" it's supposed to be.
##	08-sep-1993 (dianeh)
##		Since `sfiles' doesn't work for deleted files, just use
##		`get -f *' instead.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

CMD=`basename $0`

cut_cmd=/usr/bin/cut
TMP_FILE=/tmp/sub.out$$

trap 'rm -f $TMP_FILE' 0 1 2 15

usage='[ -l <list-file> ]'

: ${ING_SRC?"must be set"}

while [ $# -gt 0 ]
do
  case "$1" in
    -l*) if [ `expr $1 : '.*'` -gt 2 ] ; then
           SUB_FILE="`expr $1 : '-l\(.*\)'`" ; shift
         else
           [ $# -gt 1 ] ||
           { echo "Usage:" ; echo "$CMD $usage" ; exit 1 ; }
           SUB_FILE="$2" ; shift ; shift
         fi
         ;;
     -*) echo "Usage:" ; echo "$CMD $usage" ; exit 1
         ;;
      *) echo "Usage:" ; echo "$CMD $usage" ; exit 1
         ;;
  esac
done

[ "$SUB_FILE" ] &&
{
  [ -r "$SUB_FILE" ] || { echo "$CMD: Cannot read $SUB_FILE" ; exit 1 ; }
}

cd $ING_SRC
[ "$SUB_FILE" ] || { SUB_FILE=$TMP_FILE ; subscribed >$SUB_FILE ; }
for dir in `cat $SUB_FILE`
{
  echo $dir | grep -s '!' || continue
  fields=4
  [ "`expr $dir : '\(...\)'`" = "vec" ] && { fields=3 ; vecdir="vec31/" ; }
  dirname=`echo $dir | $cut_cmd -d'!' -f${fields}- | sed -e "s/!/\//g"`
  [ -d $vecdir$dirname ] || \
  { echo "$vecdir$dirname not found" ; continue ; }
  cd $vecdir$dirname && echo $vecdir$dirname        # to avoid silent running
  get -f * 2>&1 | grep -v "no such file"
  cd $ING_SRC
}

echo "" ; echo "$CMD: Done (`date`)."
exit 0
