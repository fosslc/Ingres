$!	sepcc.com
$!
$!	History
$!	08-Aug-91 (DonJ)
$!		Major rewrite. Setup to preserve previous functionality.
$!		"-COND" switch is conditional processing if result file
$!		"*.obj" exits.
$!	23-Aug-91 (DonJ)
$!		Add sys$library to default include list.
$!      28-jul-1993 (DonJ)
$!		Added include of ing_src:[gl.hdr.hdr]
$!	 7-fab-1994 (donj)
$!		Added in switches and logic for compilation on ALPHA.
$!	25-sep-1995 (dougb)
$!	    Now compiling with /member_alignment on Alpha.
$!	21-dec-1995 (dougb)
$!	    Prevent needless updates to caller's environment.  This
$!	    procedure still modifies the global ALPHA symbol, but that may be
$!	    required elsewhere.  Also, undo the previous change.
$!	28-sep-1998 (kinte01)
$!	    Change VAX compile to use DEC C compiler
$!      96-oct-2000 (horda03)
$!          Nolonger using /NOMEMBER_ALIGNMENT compiler
$!          qualifier
$!      21-oct-2003 (abbjo03)
$!          On Alpha, do not use /STANDARD=VAXC.
$!      13-oct-2008 (stegr01)
$!          Differentiate between ALPHA and Itanium, not ALPHA and VAX
$!
$ def_ver = f$verify(0)
$ if f$type(test).nes."INTEGER" then $ test = 0
$ t = f$verify(test)
$ wro := write sys$output
$!
$ if test.ne.0 then $ set nover
$!
$ ALPHA         == ""
$ ALPHA         =  ""
$ delete/sym/loc ALPHA
$ delete/sym/glo ALPHA
$!
$ if f$edit(f$getsyi("arch_name"), "upcase") .eqs. "ALPHA"
$ then
$       ALPHA          == "TRUE"
$       CPESYM_CCMACH  = "/FLOAT=IEEE_FLOAT"
$       CPESYM_MACMACH = "/MIGRATE"
$ else
$       CPESYM_CCMACH  = "/FLOAT=IEEE_FLOAT"
$       CPESYM_MACMACH = ""
$ endif
$!
$!
$!
$ gosub eval_parameters	!	Eval will set '-' switches and remove them
$			!	from argument list. All unknown switches
$			!	will be dropped.
$ if test.ne.0 then $ set ver
$!
$!**********************!
$!    Main Procedure    !
$!**********************!
$!
$ if nr_of_p.eq.0 then $ goto NO_FILES	! No files specified.
$!
$!************************************************************!
$! Setup vaxc$include to pass include dirs to VAXC compiler.
$!************************************************************!
$!
$ set on
$ on error     then $ goto FILE_LOOP_END
$ on control_y then $ goto FILE_LOOP_END
$!
$ if includes.nes.""
$  then
$	vaxcincl_now = f$trnlnm("VAXC$INCLUDE")
$	!
$	if vaxcincl_now.nes.""
$	   then
$		includes = vaxcincl_now+","+includes
$	endif
$	define/nolog vaxc$include 'includes'
$ endif
$!
$!************************************************************!
$!
$ fil		= ""
$ filspec	= ""
$ NO_COMPILE	= 0
$ VAXC_COMPILE	= 1
$ MACRO_COMPILE	= 2
$ CHECK_FILE	= 3
$!
$!************************************************************!
$!
$! Loop through the file list:
$!
$ i = 0
$ FILE_LOOP_START:
$	i = i + 1
$	if i.gt.nr_of_p then $ goto FILE_LOOP_END
$	!
$	fil     = p'i'
$	do_it   = VAXC_COMPILE
$	filspec = f$search(fil)
$	!
$	!*****************************************************!
$	!
$	if filspec.eqs.""
$	   then
$		if f$parse(fil,".JUNKJUNK",,"TYPE").eqs.".JUNKJUNK"
$		   then
$			filspec = f$search(f$parse(".C",fil))
$			if filspec.eqs.""
$			   then
$				filspec = f$search(f$parse(".MAR",fil))
$				if filspec.nes.""
$				   then
$					do_it = MACRO_COMPILE
$				endif
$			endif
$		endif
$	endif
$	!
$	!*****************************************************!
$	!
$	if filspec.eqs.""
$	   then
$		x = f$parse(fil,,,"NAME")+f$parse(fil,,,"TYPE")
$		x = f$edit(x,"LOWERCASE")
$		wro "File, ''x', does not exist."
$		do_it = NO_COMPILE
$	   else
$		if cond.eq.1
$		   then
$			if f$search(f$parse(".OBJ;*",filspec)).nes.""
$			   then
$				do_it = NO_COMPILE
$			endif
$		endif
$		if do_it.ne.NO_COMPILE
$		   then
$			typ = f$parse(filspec,".JUNKJUNK",,"TYPE")
$			if typ.eqs.".MAR"
$			   then
$				do_it = MACRO_COMPILE
$			else
$			if typ.eqs.".C"
$			   then
$				do_it = VAXC_COMPILE
$			else
$				do_it = CHECK_FILE
$			endif
$			endif
$		endif
$	endif
$	!
$	!*****************************************************!
$	!
$	if do_it.eq.CHECK_FILE then $ gosub CHECKFILE
$	!
$	!*****************************************************!
$	!
$	if do_it.eq.VAXC_COMPILE
$	   then
$		cc/nolist'CPESYM_CCMACH''switches' 'filspec'
$	else
$	if do_it.eq.MACRO_COMPILE
$	   then
$		macro/nolist'CPESYM_MACMACH''switches' 'filspec'
$	endif
$	endif
$	!
$ goto FILE_LOOP_START
$ FILE_LOOP_END:
$ !
$ if includes.nes.""
$  then
$	if vaxcincl_now.eqs.""
$	   then
$		deassign vaxc$include
$	   else
$		define/nolog vaxc$include vaxcincl_now
$	endif
$ endif
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
$ wro "Usage: sepcc <file1> [<file2> ... <filen>]"
$ wro ""
$ wro "        -c[onditional]   Conditional Compilation. Compile only if"
$ wro "                         object file doesn't exist."
$ wro ""
$ wro "        -d[ebug]         Debug code included."
$ wro ""
$ wro "        -I<directory>    Directory to search for include files."
$ wro ""
$ !
$ goto DONE
$ !
$ !*********************************************************************!
$ eval_parameters:
$ !**********************************************************************!
$ !
$ cond = 0
$ switches = ""
$ if f$trnlnm("ii_system").nes.""
$  then
$	includes = "II_SYSTEM:[INGRES.FILES],"
$  else
$	includes = ""
$ endif
$ if f$trnlnm("ing_src").nes.""
$  then
$	includes = includes + "ING_SRC:[GL.HDR.HDR],"
$	includes = includes + "ING_SRC:[CL.HDR.HDR],"
$       includes = includes + "ING_SRC:[CL.CLF.HDR],"
$	includes = includes + "ING_SRC:[FRONT.HDR.HDR],"
$	includes = includes + "ING_SRC:[FRONT.FRONTCL.HDR],"
$	includes = includes + "ING_SRC:[COMMON.HDR.HDR],"
$ endif
$ includes = includes + "SYS$LIBRARY"
$ includes = f$edit(includes,"TRIM,COMPRESS,UPCASE")
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
$			if e0.eqs.f$extract(0,e0l,"DEBUG")
$			   then
$				switches = switches + "/DEBUG"
$			else
$			if e0.eqs.f$extract(0,e0l,"CONDITIONAL")
$			   then
$				cond = 1
$			else
$			if f$extract(0,1,e0).eqs."I".and.e0l.gt.2
$			   then
$				x = f$edit(f$extract(1,e0l-1,e0)-
					  ,"TRIM,COMPRESS,UPCASE")
$				if includes.eqs.""
$				   then
$					includes = x
$				   else
$					if f$locate(x,includes).eq.-
					   f$length(includes)
$					   then
$						includes = includes + "," + x
$					endif
$				endif
$			else
$			endif
$			endif
$			endif
$		endif
$	else
$		if i.ne.j then $ p'j' = p
$		nr_of_p = j
$		j = j + 1
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
$ CHECKFILE:
$ !****************************************************!
$ !
$ open/read fs 'filspec'
$ CHKF_LOOP_START:
$	!
$	if do_it.ne.CHECK_FILE then $ goto CHKF_LOOP_END
$	read/end_of_file=CHKF_LOOP_END fs fline
$	!
$	fline = f$edit(fline,"TRIM,COMPRESS,UPCASE")
$	fel0  = f$element(0," ",fline)
$	fel1  = f$element(1," ",fline)
$	!
$	if (fel0.eqs.".TITLE").or.(fel0.eqs.".PSECT")-
	   .or.(fel0.eqs.".LONG").or.(fel0.eqs.".END")
$	   then
$		do_it = MACRO_COMPILE
$	else
$	if (fel0.eqs."STATIC").or.(fel0.eqs."#INCLUDE")-
	   .or.((fel0.eqs."#").and.(fel1.eqs."INCLUDE"))
$	   then
$		do_it = VAXC_COMPILE
$	endif
$	endif
$	!
$ goto CHKF_LOOP_START
$ CHKF_LOOP_END:
$ close fs
$ !
$ if do_it.eq.CHECK_FILE then $ do_it = NO_COMPILE
$ !
$ return
$ !****************************************************!
$ exit
