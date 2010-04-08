$!	seplnk.com.
$!
$!	Accessed by symbol "seplnk" in the SEP. The other
$!	legal flag is -d to specify link with debugger.
$!
$! HISTORY:
$!
$! added -c flag
$!	if -c then define frontmain to "frontmain.obj"
$!	and when you link, link in frontmain.obj before 'modname'.exe
$!
$! replaced equel.opt with esql.opt to include sqlca.obj
$!	by Dave Lo 5/25/88
$!
$! seplnk.com (changed name to avoid conflicts with run/compare tests)
$! removed logicals that redirect output to run/compare directories. WWW
$!
$!	07-jul-91 (DONJ)
$!		Put into piccolo library INGRES63P. Added History stuff.
$!
$!	09-Jul-91 (DONJ)
$!		MAJOR rewrite to allow multiple object files to be specified.
$!		Preserved '-COND' '-C' '-F' and '-D' flags. Will try to make
$!		both UNIX and VMS SEPLNKs compatable and retain prior func-
$!		tionality.
$!	16-Aug-91 (DONJ)
$!		'-C' flag doesn't make any sense. Moved define of frontmain
$!		up to '-Forms' flag. Made '-C' same as '-COND'.
$!	26-feb-1994 (donj)
$!		Added symbols for ALPHA
$!	21-dec-1995 (dougb)
$!		Prevent needless updates to caller's environment.  This
$!		procedure still modifies the global ALPHA symbol, but that
$!		may be required elsewhere.
$!	14-jul-1998 (kinte01)
$!		Added -a flag for linking API applications
$!      13-oct-2008 (stegr01)
$!              Differentiate between ALPHA and Itanium, not Alpha and VAX
$!
$ def_ver = f$verify(0)
$ if f$type(test).nes."INTEGER" then $ test = 0
$ t = f$verify(test)
$ !
$ wro := write sys$output
$ !
$ if test.ne.0 then $ set nover
$!
$ ALPHA         == ""
$ ALPHA         =  ""
$ delete/sym/loc ALPHA
$ delete/sym/glo ALPHA
$!
$ if f$edit(f$getsyi("arch_name"), "upcase") .eqs. "ALPHA" then -
$    ALPHA	== "TRUE"
$!
$ gosub eval_parameters	!	Eval will set '-' switches and remove them
$			!	from argument list.
$			!
$			! Ex:	p1 = "-D" p2 = "FILE1" p3 = "-F" p4 = "FILE2"
$			!	nr_of_p = (undefined)
$			!	gosub eval_parameters
$			!	p1 = "FILE1" p2 = "FILE2" p3 = "" p4 = ""
$			!	nr_of_p = 2
$ if test.ne.0 then $ set ver
$!**********************!
$!	Main Procedure
$!**********************!
$!
$ if nr_of_p.eq.0 then $ goto NO_FILES	! No files specified.
$!
$! Loop through the file list:
$!
$ image_name   = ""
$ nothing_todo = 0
$ bad_obj      = 0
$ fi = 0
$ i  = 1
$ OBJLOOP:
$	if (i.gt.nr_of_p).or.(nothing_todo.ne.0).or.(bad_obj.ne.0) -
	   then $ goto OBJLOOP_END
$	!
$	x = p'i'
$	!
$	if i.eq.1
$	   then
$		image_name = f$parse(".EXE",x)
$		if (cond.eq.1).and.(f$search(image_name).nes."")
$		   then
$			nothing_todo = 1
$		endif
$	   else
$		x = f$element(0,"/",x)
$	endif
$	!
$	if nothing_todo.eq.0
$	   then
$		otype = f$parse(x,".JUNKJUNK",,"TYPE")
$		if otype.eqs.".JUNKJUNK"
$		   then
$			x = f$parse(".OBJ",x)
$			if f$search(x).eqs.""
$			   then
$				x = f$parse(".OLB",x)
$				if f$search(x).eqs.""
$				   then
$					bad_obj = 1
$				endif
$			endif
$		endif
$		if bad_obj.eq.0
$		   then
$			obj_name = x
$			x = f$search(obj_name)
$			if x.eqs.""
$			   then
$				bad_obj = 1
$			   else
$				otype = f$parse(x,,,"TYPE")
$				if otype.eqs.".OBJ"
$				   then
$					obj_name = x
$				else
$				if otype.eqs.".OLB"
$				   then
$					obj_name = x + "/lib"
$				else
$				if otype.eqs.".STB"
$				   then
$					obj_name = x + "/sel"
$				else
$				endif
$				endif
$				endif
$				if fi.eq.0
$				   then
$					filnam = obj_name
$				   else
$					filnam = filnam + "," + obj_name
$				endif
$				fi = fi + 1
$			endif
$		endif
$	endif
$	!
$ i = i + 1
$ goto OBJLOOP
$ OBJLOOP_END:
$ if nothing_todo.ne.0 then $ goto DONE
$ if bad_obj.ne.0      then $ goto OBJ_MISSING
$!
$! Have all arguments, so go ahead and link:
$!  the equel.opt is replaced with esql.opt to include sqlca.obj
$!
$ vaxcrtl_lib = ""
$ if (f$edit(f$getsyi("ARCH_NAME"), "UPCASE") .eqs. "ALPHA")
$ then
$    vaxcrtl_lib = ",sys$library:vaxcrtl/library"
$ else
$    vaxcrtl_lib = "" ! Itanium
$ endif
$!
$ if forms.ne.0
$  then
$	link/nomap'switch'/exe='image_name' 'frontmain',-
	'filnam',-
	'linkopt''vaxcrtl_lib'
$  else
$	link/nomap'switch'/exe='image_name' -
	'filnam',-
	'linkopt''vaxcrtl_lib'
$ endif
$!
$ DONE:
$ def_ver = f$verify(def_ver)
$ exit
$!
$ NO_FILES:
$	wro ""
$	wro "Usage: seplnk <file1> [<file2> ... <filen>]"
$	wro ""
$	wro "        -c[onditional]   Conditional Linking. Only Link if"
$	wro "                         image file does not exist."
$	wro ""
$	wro "        -d[ebug]         debug code included."
$	wro ""
$	wro "        -f[orms]         forms code included."
$	wro ""
$	wro "        -a[api]          Include 
$	wro ""
$	goto DONE
$ OBJ_MISSING:
$	x = f$edit(obj_name,"LOWERCASE")
$	wro "File, ''x', does not exist."
$	goto DONE
$ exit
$ !*********************************************************************!
$ eval_parameters:
$ !**********************************************************************!
$ !
$ switch    = ""
$ frontmain = ""
$ cond  = 0
$ forms = 0
$ linkopt = "ii_system:[ingres.files]esql.opt/opt"
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
$				switch = switch+"/debug"
$			else
$			if e0.eqs.f$extract(0,e0l,"FORMS")
$			   then
$				forms = 1
$				frontmain = "ii_system:[ingres.library]frontmain.obj"
$			else
$			if e0.eqs.f$extract(0,e0l,"CONDITIONAL")
$			   then
$				cond = 1
$			else
$			if e0.eqs.f$extract(0,e0l,"A")
$			  then
$			       linkopt = "ii_system:[ingres.files]iiapi.opt/opt"
$			endif
$			endif
$			endif
$			endif
$		endif
$	   else
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
$ exit
