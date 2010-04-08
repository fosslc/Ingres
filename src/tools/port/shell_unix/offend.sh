:
#
# Copyright (c) 2004 Ingres Corporation
#
# offend.sh - match compiler warnings/errors with offending source lines.
#
# History:
#	03-apr-1992 (bonobo)
#		Starting history for offend.sh.
#	03-apr-1992 (bonobo)
#		Added filter for additional double-quotes which might
#		be encountered in warnings/errors.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

cat $1 |
sed -n '/^"/s/^"\([^"]*\)", line \([0-9][0-9]*\): \(.*\)/\1 \2 \3/p'|
sed -e 's/"//g' |
awk '
NF > 1 {
	if ((message = substr($0, index($0, $3))) ~ /^warning:.*/)
		message = sprintf("\"%s\"", message);
	else
		message = sprintf("\"error: %s\"", message);
	printf("file=%s; line=%s; message=%s\n", $1, $2, message);
}
' |
while read bindings
do
	eval $bindings
	{
		echo "file: $file  line: $line"
		echo "$message"
		sed -n "$line{
			p
			q
		}" $file
		echo "---"
	} >&2
done
