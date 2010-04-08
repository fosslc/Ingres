$! mkfecat - Create the file fe.cat.msg.
$!
$!
$! This file is the concatenation of all the *.msg files from the
$! "front" and "gateway" directories of a source tree.
$!
$!! History:
$!!	07-oct-2004 (abbjo03)
$!!	    Created from mkfecat.sh.
$!!
$ echo := write sys$output
$ if f$trnl("ING_SRC") .eqs. ""
$ then	echo "ING_SRC: must be set"
$	exit %x10000012
$ endif
$!
$ CMD=f$parse(f$env("procedure"),,,"name")
$!
$ if f$trnl("ING_VERS") .nes. "" then V=".''f$trnl("ING_VERS")'"
$!
$ catfile="ING_SRC:[common.hdr''V'.hdr]fe_cat.msg"
$!
$! Find the front-end and gateway .msg files, sort and unique them
$!
$ if f$parse("ING_SRC:[gateway]") .nes. ""
$ then	gwdirs="ING_SRC:[gateway]"
$	msgclause="and gateway "
$ else
$	gwdirs=""
$	msgclause=" "
$ endif
$!
$ echo "Finding front-end ''msgclause'msg files..."
$!
$! Build $catfile
$ echo "Building fe.cat.msg..."
$ if f$sear("''catfile'") .nes. "" then del 'catfile';*
$!
$! NOTE: This is not as generic as the Unix version because, at least for
$!	 now, all *.msg files in front are two levels down.
$!
$ f="ING_SRC:[front.*.*]*.msg"
$ first=1
$findloop:
$	msgfile=f$sear(f, 1)
$	if msgfile .eqs. "" then goto endfind
$	if f$loc("vifred.vifred", f$edit(msgfile, "lowercase")) .ne. -
		f$len(msgfile) then goto findloop
$	if first
$	then	copy 'msgfile' 'catfile'
$		first=0
$	else	append 'msgfile' 'catfile'
$	endif
$	goto findloop
$endfind:
$!
$ echo "''CMD': done: ''f$time()'"
$ exit
