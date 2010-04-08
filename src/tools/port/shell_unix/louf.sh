:
#
# Copyright (c) 2004 Ingres Corporation
#
# louf - lint output filter
#
# reduces messages of questionable relevance.
# daveb Thu Aug 27 10:50:28 PDT 1987
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.


cat $* | eatjunk \
	'possible pointer alignment problem' \
	'name declared but never used or defined' \
	'name defined but never used' \
	'name multiply declared' | \
sed '
	/No such/d
	/Making.*ln.../d
	/No source/d
	/ln done./d
	//d
	/\.c:$/d
	/main program/d
	/^==============$/d
	/No files to lint/d
	/warning:.*union.*never defined/d
	/set but not used in function/d
	/unused in function /d
	/ *read( arg 3/d
	/ *write( arg 3/d
	/^$/d
'

