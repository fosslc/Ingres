:
#
# Copyright (c) 2004 Ingres Corporation
#
#
# shar - make a "shell archive" for net.sources, etc.
#
## History:
##
##	16-feb-1993 (dianeh)
##		Change first line to just a colon (:); added History
##		comment-block.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#

FILE=/tmp/shar$$
TFILE=/tmp/sharx$$
head=shar
size=0
verbose=1

cat > "$TFILE" << '________This_Is_The_End________'
#--------------------------------CUT HERE-------------------------------------
#! /bin/sh
#
# This is a shell archive.  Save this into a file, edit it
# and delete all lines above this comment.  Then give this
# file to sh by executing the command "sh file".  The files
# will be extracted into the current directory owned by
# you with default permissions.
#
# The files contained herein are:
#
________This_Is_The_End________
echo "#" > "$FILE"
while [ "$#" -gt 0 ]; do
	case "$1" in
	-v)	verbose=0
		;;
	-s)	size="$2"
		shift
		;;
	-h)	head="$2"
		shift
		;;
	*)	if [ ! -r "$1" ]; then
			echo "shar: can't read $1" >&2
		else
			files="$files $1"
		fi
	esac
	shift
done
if [ "$files" = "" ]; then
	echo "usage: shar [-v] [-s size] [-h header] files..." >&2
	exit 1
fi
if [ "$size" -ne 0 ]; then
	count=1
else
	count=
fi
if [ "$verbose" = 1 ]; then
	echo "o - $head$count"
fi
for file in $files; do
	f=`basename $file`
	case "$verbose" in
	1)	echo "a - $f"
	esac
	if [ "$size" -ne 0 ]; then
		set -- `ls -l "$FILE"`
		csize=$5
		set -- `ls -l "$file"`
		if [ "$size" -lt `expr "$csize" + $5` ]; then
			echo "exit 0" >> "$FILE"
			cat "$TFILE" "$FILE" > "$head$count"
			cat > "$TFILE" << '________This_Is_The_End________'
#--------------------------------CUT HERE-------------------------------------
#! /bin/sh
#
# This is a shell archive.  Save this into a file, edit it
# and delete all lines above this comment.  Then give this
# file to sh by executing the command "sh file".  The files
# will be extracted into the current directory owned by
# you with default permissions.
#
# The files contained herein are:
#
________This_Is_The_End________
			echo "#" > "$FILE"
			count=`expr $count + 1`
			if [ "$verbose" = 1 ]; then
				echo "o - $head$count"
			fi
		fi
	fi
	echo '# \c' >> "$TFILE"
	ls -l "$file" >> "$TFILE"
	wcl=`wc -l < $f | sed 's/^  *//'`
	echo "echo 'x - $f'" >> "$FILE"
	echo "if test -f $f; then echo 'shar: not overwriting $f'; else" >> "$FILE"
	echo "sed 's/^X//' << '________This_Is_The_END________' > $f" >> "$FILE"
	sed 's/^/X/' < "$file" >> "$FILE"
	echo '________This_Is_The_END________' >> "$FILE"
	echo 'if test `wc -l < '"$f"'` -ne '"$wcl"'; then' >> "$FILE"
	echo "	echo 'shar: $f was damaged during transit (should have been $wcl bytes)'" >> "$FILE"
	echo "fi" >> "$FILE"
	echo "fi		; : end of overwriting check" >> "$FILE"
done
echo "exit 0" >> "$FILE"
cat "$TFILE" "$FILE" > "$head$count"
rm "$FILE" "$TFILE"
