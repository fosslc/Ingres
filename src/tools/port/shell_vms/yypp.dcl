$! Copyright (c) 2005 Ingres Corporation
$!
$!
$! yypp.sh - Prepend $ING_SRC/tools/port$noise/conf/{VERS,CONFIG} to $1 and run
$!           through yapp.
$!
$!! History:
$!!    28-jan-2005 (abbjo03)
$!!        Created from yypp.sh.
$!!    16-feb-2005 (abbjo03)
$!!	   Actually implement the logic.
$!!    06-nov-2007 (bolke01) Bug 119420
$!!      Modify the yypp tool to work from current
$!!        directory as well as top level ING_SRC
$!!    06-sep-2010 (horda03)
$!!      Handle multiple -D flags.
$!!
$ echo := write sys$output
$ define /nolog tmp SYS$SCRATCH
$!
$! Save current directory for use when yypp is run from other than
$! the ING_SRC directory.
$ cur_dir = F$ENVIRONMENT( "DEFAULT" )
$!
$ CMD=f$parse(f$env("procedure"),,,"name")
$!
$ usage="[ -c <config> ] [ <files>... ] [ -s <sym> ] [ -D<def> ]"
$!
$ if f$trnl("ING_VERS") .nes. ""
$ then  vers = "."+f$trnl("ING_VERS")
$       noise = vers+".src"
$ endif
$!
$ if f$sear("ING_SRC:[tools.port''noise'.conf]CONFIG") .eqs. "" then -
        exit %x10018290
$!
$! Make sure we have yapp available to us
$!
$ yapp 
$ if $severity .ne. 1 then exit %x2c
$ defs=""
$ i=1
$while:
$	if p'i .eqs. "" then goto endwhile
$	if p'i .eqs. "-C" .or. p'i .eqs. "-c"
$	then	i=i + 1
$		conf=f$edit(p'i, "lowercase")
$	else	if p'i .eqs. "-S" .or. p'i .eqs. "-s"
$	then	i=i + 1
$		sym=p'i
$	else	if f$extr(0, 2, p'i) .eqs. "-D"
$	then	if defs .nes. ""
$		then	defs=F$FAO("!AS ""!AS""", defs, p'i)
$		else	defs=F$FAO("""!AS""", p'i)
$		endif
$	else	if f$extr(0, 2, p'i) .eqs. "-H"
$	then	if "''hist'" .nes. ""
$		then	hist=hist+" "+p'i`
$		else	hist=p'i
$		endif
$	else	if f$extr(0, 1, p'i) .eqs. "-"
$	then	echo "Usage:"
$		echo "''CMD' ''usage'"
$		exit %x10000012
$	else	file=p'i
$!              Run from ING_SRC location or current location
$               if f$extr(0, 2, file) .eqs. "[." 
$               then
$                  file="ING_SRC:["+f$extr(2, 255, file)
$               else 
$                   file=cur_dir+file
$               endif
$		if "''files'" .nes. ""
$		then	files=files+","+file
$		else	files=file
$		endif
$	endif
$	endif
$	endif
$	endif
$	endif
$	i = i + 1
$	if i .le. 8 then goto while
$endwhile:
$!
$ @ING_TOOLS:[bin]readvers
$!
$! If $conf was passed, but readvers reported a different string in
$! VERS, don't use anything from readvers; it doesn't apply to $conf.
$! Print a warning.
$!
$ if "''conf'" .nes. "" .and. "''conf'" .nes. "''IIVERS_config'"
$ then	optdef=""
$	opts=""
$	echo "Warning: ''IIVERS_config defined in VERS but ''conf' will be used."
$!
$! Just use config and everything we got from readvers.
$!
$ else	conf=IIVERS_config
$ endif
$!
$! Build up arguments to yapp
$!
$ dconf="-D''IIVERS_config'"
$ dvers="-DVERS=''IIVERS_config'"
$! dopts="''IIVERS_optdef'"
$!
$ cat := type /nohead
$ pipe cat ING_SRC:[tools.port'noise'.conf]CONFIG., 'files' > tmp:cat-yypp.tmp
$ pipe yapp "''dvers'" "''dconf'" 'dopts' 'defs' 'hist' tmp:cat-yypp.tmp | -
	sed -e "/^[ 	]*$/d"
$ del tmp:cat-yypp.tmp;*
$!
$ exit
