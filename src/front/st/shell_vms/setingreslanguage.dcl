$!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$!
$! Name: setingreslanguage.com
$!
$! Purpose: Set language for Ingres messages
$!
$!! History:
$!!	03-mar-2005 (abbjo03)
$!!	    Created from setingreslanguage.sh.
$!!
$ echo="write sys$output"
$ self=f$parse(f$env("procedure"),,,"name")
$!
$! Check command args
$!
$ langs = "|deu|english|esn|fra|ita|jpn|ptb|sch|"
$ arg = f$edit(p1, "lowercase")
$ if f$locate("|"+arg+"|", langs) .eq. f$length(langs)
$ then	echo "Usage:"
$	echo "	''self' [fra|deu|esn|ita|ptb|sch or jpn]"
$	exit %X10000010		! BADPARAM
$ endif
$!
$! Check environment
$!
$ if f$trnlnm("II_SYSTEM") .eqs. ""
$ then	echo "Error:"
$	echo "	II_SYSTEM must be set before running this script."
$	exit %X10000150		! IVLOGNAM
$ endif
$!
$! Check that the language dir exists
$!
$ if f$search("II_SYSTEM:[ingres.files]''arg'.dir") .eqs. ""
$ then	echo "Error:"
$	echo "	Language files for ''arg' have not been installed"
$	exit %X10018292		! FNF
$ endif
$ echo "Setting the Ingres Language Environment to ''arg'"
$ echo ""
$ iisetres ii.'f$getsyi("NODENAME")'.lnm.II_LANGUAGE 'arg'
$ define /job II_LANGUAGE 'arg'
