:
#
# Copyright (c) 2004 Ingres Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.

# $Header: /cmlib1/ingres63p.lib/unix/tools/port/shell/yyfix.sh,v 1.1 90/03/09 09:18:35 source Exp $

tmp=/tmp/ro$$.c
script=/tmp/yy$$.sed

# Get any "typedef"s of the structures
#	(System V only; null op on BSD)

grep "typedef[ 	].*yy.*;" y.tab.c > rodata.c

cat > $script << EOF
/@(#)/d
/yy.*\[][ 	]*=/,/}/{
	w $tmp
	s/\(.*yy.*\[]\)[ 	]*=.*/extern \1;/
	/^extern/!d
}
EOF

# Overwrite "y.tab.c" with the `sed' output.

overwrite y.tab.c sed -f $script y.tab.c
cat $tmp >> rodata.c
rm -f $tmp $script
