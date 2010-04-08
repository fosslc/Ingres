:
#
# Copyright (c) 2004 Ingres Corporation
#
#  Compare an "original" .sep file with one derived from it, but which
#  contains altcanons.  The first parameter is the name of the original
#  file, the second is the name of the derived file.
#
#  Note: This script depends on the existence of the "tac" program.  That's
#  "cat" spelled backwards, and it does what you might expect, namely writes
#  out its input in line-reversed order.  The source for tac should be
#  distributed with this program.  
#
#  History:
#
#	05-mar-1991 (jonb)
#		Created.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

#
#  Make sure the params are ok...
#
if [ -z "$1" -o ! -z "$3"  ] 
then
    cat << !
Usage: altcompare origfile derivedfile
  ...where: origfile = an original .sep file
     and derivedfile = a file derived from origfile which contains altcanons

or:    altcompare <filename>
  ...in which case <filename>.sep is used for the original file, and
     <filename>.upd is used for the derived file.
!
    exit 1
fi

#
#  Useful things for displays...
#
dashes="---------------------------------------"
dashes=$dashes-$dashes

#
#  Create a directory to keep temporary files in, and make sure it goes
#  away if we exit for any reason.
#
tmpdir=/tmp/altc.$$
mkdir $tmpdir
trap '/bin/rm -rf $tmpdir; exit' 0
trap 'exit' 2

#
#  Some file names in the temp directory...
#

orig=$tmpdir/orig
derived=$tmpdir/derived
alt=$tmpdir/altcanon
main=$tmpdir/maincanon

#
#  Generate full path names for the input files.
#
if [ -z "$2" ]
then
  [ `echo $1 | cut -c1` = / ] && f=$1 || f=`pwd`/$1
  sepfile=$f.sep
  updfile=$f.upd
else
  [ `echo $1 | cut -c1` = / ] && sepfile=$1 || sepfile=`pwd`/$1
  [ `echo $2 | cut -c1` = / ] && updfile=$2 || updfile=`pwd`/$2
fi

#
#  Preprocess the input files.  First we'll get rid of leading pound
#  signs, because they make the C preprocessor unhappy.  Then we use
#  the preprocessor to get rid of comments in the sep files.
#

cd $tmpdir

sed 's/^#/\<pound\>/' $sepfile > temp.c
cc -P temp.c
sed '/^$/d' temp.i > $orig

sed 's/^#/\<pound\>/' $updfile > temp.c
cc -P temp.c
sed '/^$/d' temp.i > $derived

/bin/rm -f temp.*

#
#  Diff the two input files, and grab all lines that indicate that a block
#  of something has been added to the derived file.  We'll just blindly
#  believe that it's an altcanon...
#

diff $orig $derived | grep "^[0-9]*a[0-9]*,[0-9]" |
   while read line
   do
       linenum=`echo $line | cut -f2 -da | cut -f1 -d,`
       linenum1=`expr $linenum - 1`
       [ $linenum1 -le 0 ] && continue
       /bin/rm -f $alt $main
#
#  Get the altcanon...
#
       sed -e "1,${linenum1}d"  -e '/^>>$/,$d' $derived > $alt
       echo '>>' >> $alt
#
#  Get the main canon..
#
       while :
       do
	 sedcmd=$linenum,\$d
         linenum1=`sed "$sedcmd" $derived | tac | 
			sed -e '/^<<$/=' -e 'd' | sed '2,$d'`
	 linenum=`expr $linenum - $linenum1`
	 linenum1=`expr $linenum - 1`
	 command=`sed -e "${linenum1}p" -e 'd' $derived`
	 [ "$command" != ">>" ] && break
       done
       sed -e "1,${linenum1}d" -e '/^>>/,$d' $derived > $main
       echo '>>' >> $main
#
#  Diff them...
#
       echo $dashes
       echo "$command"
       echo $dashes
       diff $main $alt
       echo $dashes
       echo " "
       echo " "
       echo " "
   done 
