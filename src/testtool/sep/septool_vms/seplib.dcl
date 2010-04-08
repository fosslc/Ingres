$!	seplib.com.
$!
$!	Accessed by symbol "seplib" in the SEP.
$!
$! HISTORY:
$!
$!	16-Aug-91 (DONJ)
$!		Created.
$!
$ def_ver = f$verify(0)
$ if f$type(test).nes."INTEGER" then $ test == 0
$ t = f$verify(test)
$ !
$ wro :== write sys$output
$ !
$ if test.ne.0 then $ set nover
$ gosub eval_parameters	!	Eval will set '-' switches and remove them
$			!	from argument list.
$			!
$ if test.ne.0 then $ set ver
$!**********************!
$!	Main Procedure
$!**********************!
$!
$ if nr_of_p.eq.0 then $ goto NO_FILES	! No files specified.
$!
$ libname     = p1
$ x = f$search(libname)
$!
$ if x.eqs.""
$  then
$	libname = f$parse(libname,".OLB")
$	x = f$search(libname)
$	if x.nes.""
$	   then
$		libname = x
$	endif
$  else
$	libname = x
$ endif
$!
$ if x.eqs.""
$  then
$	library/create 'libname'
$ endif
$!
$! Loop through the file list:
$!
$ bad_obj = 0
$ fi = 0
$ i  = 2
$ !
$ OBJLOOP:
$	if (i.gt.nr_of_p).or.(bad_obj.ne.0) -
	   then $ goto OBJLOOP_END
$	!
$	x = p'i'
$	!
$	otype = f$parse(x,".JUNKJUNK",,"TYPE")
$	if otype.eqs.".JUNKJUNK"
$	   then
$		x = f$parse(".OBJ",x)
$		if f$search(x).eqs.""
$		   then
$			bad_obj = 1
$		endif
$	endif
$	if bad_obj.eq.0
$	   then
$		obj_name = x
$		x = f$search(obj_name)
$		if x.eqs.""
$		   then
$			bad_obj = 1
$		   else
$			obj_name = x
$			if fi.eq.0
$			   then
$				filnam = obj_name
$			   else
$				filnam = filnam + "," + obj_name
$			endif
$			fi = fi + 1
$		endif
$	endif
$	!
$ i = i + 1
$ goto OBJLOOP
$ OBJLOOP_END:
$ if bad_obj.ne.0      then $ goto OBJ_MISSING
$!
$! Have all arguments, so go ahead and library command:
$!
$!
$ if fi.gt.0
$  then
$	library/replace 'libname' 'filnam'
$ endif
$!
$ DONE:
$ def_ver = f$verify(def_ver)
$ exit
$!
$ NO_FILES:
$	wro ""
$	wro "Usage: seplib <libname> [<ofile1> ... <ofilen>]"
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
