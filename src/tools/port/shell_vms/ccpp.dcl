$! Copyright (c) 2004 Ingres Corporation
$!
$!
$! ccpp.sh - Prepend $ING_SRC/tools/port$noise/conf/CONFIG to $1 and run
$!           through cpp.
$!
$!! History:
$!!	20-sep-2004 (abbjo03)
$!!		Created from ccpp.sh.
$!!	25-may-2005 (abbjo03)
$!!	    Add /noobj to preprocessor in order not to create stray object
$!!	    files.
$!!    11-Dec-2007 (bolke01) Bug b118421 
$!!         Original change was put into UNIX and WIN areas only.
$!!         Add support for passing through certain text that would have been
$!!         stripped out by the preprocessor.
$!!    11-Dec-2007 (horda03/bolke01) Bug b119592
$!!         The current directory is not included correctly by the pre-processor
$!!         Added /include="" to this script's cc parms  so that any files that
$!!         include files ( #include ) will function correctly.
$!!         Changed the usage of the pipe comand as this was causing an out of
$!!         virtual memory error and hanging the build when a ccpp file included
$!!         additional files.
$!!         As part of this change the temporary files need to be co-located with
$!!         the original file.  A pid identifier has been used to name the temp
$!!         files.
$!!         Housekeeping:  changes file[s] to ccpp_file[s]
$!!
$ echo := write sys$output
$ pid = F$GETJPI(0, "PID")
$ ccpp_files=""
$
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
$	then	arg=f$extr(2, 128, p'i)
$		if "''defs'" .nes. "" 
$		then	defs=defs+","+arg
$		else	defs=arg
$		endif
$	else	if f$extr(0, 2, p'i) .eqs. "-U"
$	then	arg=f$extr(2, 128, p'i)
$		if "''undefs'" .nes. "" 
$		then	undefs=undefs+","+arg
$		else	undefs=arg
$		endif
$	else	if f$extr(0, 1, p'i) .eqs. "-"
$	then	echo "usage: ccpp [ -c xxx_uyy ] [ files... ] [-Dsym ...] [-Usym ...]"
$		echo "       ccpp [ -c xxx_uyy ] -s symbol [-Dsym ...] [-Usym ...]"
$		exit %x10000012
$	else	ccpp_file=p'i
$		ccpp_files=ccpp_files+","+ccpp_file
$	endif
$	endif
$	endif
$	endif
$	endif
$	i = i + 1
$	if i .le. 8 then goto while
$endwhile:
$!
$ if f$trnl("ING_VERS") .nes. ""
$ then	ing_vers = "."+f$trnl("ING_VERS")
$	noise = ing_vers+".src"
$ endif
$ if "''conf'" .eqs. ""
$ then	@ING_TOOLS:[bin]readvers
$	conf=IIVERS_config
$	if f$sear("ING_SRC:[cl.hdr''ing_vers'.hdr_vms]bzarch.h") .nes. ""
$	then	bzarch="ING_SRC:[cl.hdr''ing_vers'.hdr_vms]bzarch.h"
$	else if f$sear("ING_SRC:[cl.hdr''ing_vers'.hdr]bzarch.h") .nes. "" 
$	then	bzarch="ING_SRC:[cl.hdr''ing_vers'.hdr]bzarch.h"
$	else	confd="/DEFINE="+conf
$	endif
$	endif
$ else	confd="/DEFINE="+conf
$ endif
$!
$ if "''conf'" .eqs. "" then exit %x10000012
$ if f$sear("ING_SRC:[tools.port''noise'.conf]CONFIG") .eqs. "" then -
	exit %x10018290
$ if f$extr(3, 4, conf) .eqs. "_vms"
$! Need to use /STANDARD=COMMON otherwise the Ingres version is treated
$! as an expression and it ends up with spaces around the "/".
$ then	CPP="cc /prep=sys$output /noobj /noline /nowarn /include=("""") /stan=common"
$ endif
$!
$ digits="0123456789"
$ alnum=digits+"abcdefghijklmnopqrstuvwxyz"
$ l=f$len(alnum)
$ isalnum = f$locate(f$extr(0, 1, conf), alnum) .ne. l .and. -
	f$locate(f$extr(1, 1, conf), alnum) .ne. l .and. -
	f$locate(f$extr(2, 1, conf), alnum) .ne. l
$ if f$len(conf) .ne. 7 .or. f$extr(3, 4, conf) .nes. "_vms" .or. -
	.not. isalnum
$ then	arg0=f$parse(f$env("procedure"),,,"name")
$	echo "''arg0': bad config, should be xxx_yyy"
$	exit %x10000012
$ endif
$ if f$locate(f$extr(0, 1, conf), digits) .ne. f$len(digits) then -
	conf="x"+conf
$!
$ if "''confd'" .nes. "" then -
	confd="/DEFINE=("+conf+")"
$!
$ cat := type /nohead
$ if "''sym'" .nes. ""
$ then
$	pipe echo "''sym'" | cat ING_SRC:[tools.port'noise'.conf]CONFIG., -
	sys$pipe | 'CPP' /DEF="VERS=''conf'" /DEF="''conf'" 'optdef' sys$pipe -
	| sed "/^[      ]*$/d" >readvar-ccpp.tmp_'pid'
$	gosub readvar
$	res=varvalue
$	if res .eqs. sym
$	then	echo "''sym': undefined in CONFIG"
$		exit %x10000012
$	else	echo res
$	endif
$ else	if "''defs'" .nes. "" then defs="/DEF=(''defs')"
$	if "''undefs'" .nes. "" then undefs="/UNDEF=(''undefs')"
$	pipe cat 'bzarch', ING_SRC:[tools.port'noise'.conf]CONFIG. | -
	     cat sys$pipe 'ccpp_files' > cpp-ccpp.tmp_'pid'
$	pipe 'cpp' /DEF="VERS=''conf'" 'defs' 'undefs' cpp-ccpp.tmp_'pid' | -
             sed -e "/^[ 	]*$/d" -e "s:/%:/*:g"  -e "s:%/:*/:g"  -e "s:^%::" sys$pipe
$       delete/nolog cpp-ccpp.tmp*_'pid'.*
$ endif
$ exit
$!
$readvar:
$ open /read varfile readvar-ccpp.tmp_'pid'
$nextline:
$ read /end=closevar varfile varvalue
$ if varvalue .eqs. "" then goto nextline
$closevar:
$ close varfile
$ del readvar-ccpp.tmp_'pid';*
$ return
