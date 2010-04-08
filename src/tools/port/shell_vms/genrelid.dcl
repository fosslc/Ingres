$ cur_vfy='f$verify(0)'
$ goto _start_main
$! genrelid.sh  - generates a release id and echoes it on stdout
$!
$!
$! This is the one and only place release IDs are generated from VERS
$! and CONFIG (using readvers and ccpp, that is). If it all happens here, 
$! we can keep it consistent and only have to update the algorithm in one place.
$!
$!! History
$!!	07-sep-2004 (abbjo03)
$!!		Created from genrelid.sh.
$!!     12-mar-2008 (bolke01) 120100
$!!             Added protection from being run with 'set verify' turned on
$!!             as this corrupted the version string.
$!!
$readvar:
$ open /read varfile tmp:readvar-genrelid.tmp_'pid'
$nextline:
$ read /end=closevar varfile varvalue
$ if varvalue .eqs. "" 
$ then 
$     goto nextline
$ endif
$closevar:
$ close varfile
$ delete /nolog /noconf tmp:readvar-genrelid.tmp_'pid';*
$ return
$!

$_start_main:
$ echo := write sys$output
$ pid=F$GETJPI("","PID")
$ define /nolog tmp SYS$SCRATCH
$ CMD=f$parse(f$env("procedure"),,,"name")
$!
$ @ING_TOOLS:[bin]readvers
$!
$! Check for product.
$!
$ do_64bit="false"
$ prod="INGRES"
$ i=1
$while:
$	if p'i .eqs. "" then goto endwhile
$!
$	if p'i .eqs. "-LP64"
$	then	
$           do_64bit="true"
$	     ! (Right now I only know of 3 products. Others can be added.
$	     ! If you add a product, add it here and in the case statement that
$	     ! outputs the release id.)
$	else 
$           if p'i .eqs. "INGRES" .or. p'i .eqs. "W4GL"
$	    then
$ 	        prod=p'i
$		inc=""
$	    else	
$               arg=p'i
$		echo "''CMD': Invalid product type ""''arg'"""
$               if cur_vfy .eq. 1 then set verify
$		exit %x10000012
$	    endif
$	endif
$	i = i + 1
$	if i .le. 8 then goto while
$endwhile:
$!
$! Set up the suffix:
$!
$! (Right now, there are only 2 known suffixes, OL and DBL.
$! Others can be added.)
$!
$! The order in which suffixes are concatenated is significant.
$! Check with Release Management before adding new suffixes into the mix.
$!
$ suffix=""
$ if "''IIVERS_conf_ES'" .nes. "" then suffix="ES"
$ if "''IIVERS_conf_OL'" .nes. "" then suffix="''suffix'OL"
$!
$! Output the release id:
$!
$ if do_64bit
$ then	
$    config="''IIVERS_config64'"
$ else	
$    config=IIVERS_config
$ endif
$!
$ locunder=f$loc("_", config)
$ dotvers=f$extr(0, locunder, config) + "." + -
	f$extr(locunder + 1, f$len(config), config)
$ if prod .eqs. "INGRES"
$ then	
$       pipe ccpp -s ING_VER >tmp:readvar-genrelid.tmp_'pid'
$	if $severity .ne. 1 
$       then 
$           stt = $status
$           if cur_vfy .eq. 1 then set verify
$           exit stt$
$       endif
$	gosub readvar
$	VER=varvalue
$	echo "''VER' (''dotvers'/''IIVERS_build')''suffix'"
$	if "''IIVERS_inc'" .nes. "" 
$       then 
$          echo "Build_''IIVERS_inc'"
$       endif
$ else 
$    if prod .eqs. "W4GL"
$    then	
$       pipe ccpp -s ING_W4GL_VER >tmp:readvar-genrelid.tmp_'pid'
$	if $severity .ne. 1 
$       then 
$           stt = $status
$           if cur_vfy .eq. 1 then set verify
$           exit stt$
$       endif
$	gosub readvar
$	VER=varvalue
$	echo "W4GL ''VER' (''dotvers'/''IIVERS_build')''suffix'"
$    endif
$ endif
$ if cur_vfy .eq. 1 then set verify
