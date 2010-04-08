$!	sepesqlc.com
$!
$!	History
$!	08-Aug-91 (DonJ)
$!		Major rewrite. Setup to preserve previous functionality.
$!		"-COND" switch is conditional processing if result file
$!		"*.c" exits. Handles both .sc and .qc files. If file type
$!		not included, looks for .sc then .qc.
$!      04-sep-2008 (horda03)
$!              On ODS-5 disks the file names can be lowercase. This means that
$!              the extension may be .sc or .qc  (not .SC or .QC)
$!
$ def_ver = f$verify(0)
$ if f$type(test).nes."INTEGER" then $ test == 0
$ t = f$verify(test)
$ wro :== write sys$output
$ !
$ if test.ne.0 then $ set nover
$ gosub eval_parameters	!	Eval will set '-' switches and remove them
$			!	from argument list. All unknown switches
$			!	will be passed thru to the precompiler in
$			!	the symbol, flgs.
$ if test.ne.0 then $ set ver
$!**********************!
$!	Main Procedure
$!**********************!
$!
$ if nr_of_p.eq.0 then $ goto NO_FILES	! No files specified.
$!
$ fil	  = ""
$ filnam  = ""
$ filspec = ""
$ filtyp  = ""
$ nm = 0
$!
$! Loop through the file list:
$!
$ i = 0
$ FILE_LOOP_START:
$	i = i + 1
$	if i.gt.nr_of_p then $ goto FILE_LOOP_END
$	fil = p'i'
$	x = fil
$	do_it = 1
$!
$	if f$parse(fil,"*.JUNKJUNK",,"TYPE").eqs.".JUNKJUNK"
$	   then
$		fil = f$parse(".SC",fil)
$		if f$search(fil).eqs.""
$		   then
$			fil = f$parse(".QC",fil)
$		endif
$	endif
$!
$	filspec = f$search(fil)
$!
$	if filspec.eqs.""
$	   then
$		x = f$parse(x,,,"NAME")+f$parse(x,,,"TYPE")
$		x = f$edit(x,"LOWERCASE")
$		wro "file, ''x', doesn't exist."
$		do_it = 0
$	   else
$		filtyp = f$edit(f$parse(filspec,,,"TYPE"), "UPCASE")
$		filnam = f$parse(filspec,,,"NAME")
$		if filtyp.eqs.".SC"
$		   then
$			pcccmd  = "ESQLC"
$		   else
$			if filtyp.eqs.".QC"
$			   then
$				pcccmd  = "EQC"
$			   else
$				wro "bad file type for <''filspec'>"
$				do_it = 0
$			endif
$		endif
$	endif
$!
$	if do_it.eq.1.and.cond.eq.1
$	   then
$		if f$search(f$parse(".C;*",filspec)).nes.""
$		   then
$			do_it = 0
$		endif
$	endif
$!
$	if do_it.eq.1
$	   then
$		cmdline = "''pcccmd' ''flgs' ''filspec'"
$		if outfil_flag.eq.1
$		   then
$			cmdline = "''cmdline' >''outfil'"
$		endif
$!
$		'cmdline'
$!
$		x = f$edit(filnam+filtyp,"LOWERCASE")
$		if pcccmd.eqs."ESQLC"
$		   then
$			wro "ESQL ''x':"
$		   else
$			if pcccmd.eqs."EQC"
$			   then
$				wro "EQUEL ''x':"
$			endif
$		endif
$	endif
$!
$ goto FILE_LOOP_START
$ FILE_LOOP_END:
$ !
$ DONE:
$ !
$ def_ver = f$verify(def_ver)
$ !
$ exit
$ !
$ NO_FILES:
$ !
$ wro ""
$ wro "Usage: sepesqlc <file1> [<file2> ... <filen>]"
$ wro ""
$ wro "        -c[onditional]   Conditional Processing. Preprocess only if"
$ wro "                         source file does not exist."
$ wro ""
$ !
$ goto DONE
$ !
$ !*********************************************************************!
$ eval_parameters:
$ !**********************************************************************!
$ !
$ cond		= 0
$ fl		= 0
$ outfil	= ""
$ flgs		= ""
$ need_outfil	= 0
$ outfil_flag	= 0
$ !
$ nr_of_p = 0
$ i = 1
$ j = 1
$ start_eval_loop:
$ if i.eq.9   then $ goto end_eval_loop
$ p = p'i'
$ if p.eqs."" then $ goto cont_eval_loop
$	!
$	c  = f$extract(0,1,p)
$	!
$	if c.eqs."-"
$	   then
$		p  = f$extract(1,f$length(p)-1,p)
$		pl = f$length(p)
$		if pl.gt.0
$		   then
$			e0  = f$element(0,"=",p)
$			e1  = f$element(1,"=",p)
$			e0l = f$length(e0)
$			e1l = f$length(e1)
$			if e0.eqs.f$extract(0,e0l,"CONDITIONAL") .and. e0l.ge.4
$			   then
$				cond = 1
$			   else
$				fl = fl+1
$				flgs = "''flgs' -''p'"
$			endif
$		endif
$		if need_outfil.eq.1 then $ need_outfil = 0
$	else if c.eqs.">"
$	   then
$		p  = f$extract(1,f$length(p)-1,p)
$		pl = f$length(p)
$		if pl.gt.0
$		   then
$			outfil = p
$			need_outfil = 0
$			outfil_flag = 1
$		   else
$			need_outfil = 1
$		endif
$	   else
$		if need_outfil.eq.1
$		   then
$			outfil = p
$			need_outfil = 0
$			outfil_flag = 1
$		   else
$			if i.ne.j then $ p'j' = p
$			nr_of_p = j
$			j = j + 1
$		endif
$	endif
$	endif
$	!
$ cont_eval_loop:
$ i = i + 1
$ goto start_eval_loop
$ end_eval_loop:
$ !
$ i = nr_of_p + 1
$ start_eval_loop2:
$	if i.eq.9 then $ goto end_eval_loop2
$	!
$	p'i' = ""
$	!
$ i = i + 1
$ goto start_eval_loop2
$ end_eval_loop2:
$ !
$ return
$ !****************************************************!
$ exit
