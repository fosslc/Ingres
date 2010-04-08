$ set noon
$ create/name_table/parent=lnm$process_directory/prot=(w:rwed)/nolog sep_table
$ define/nolog/table=lnm$process_directory lnm$file_dev sep_table,-
  lnm$process,lnm$job,lnm$group,lnm$system
$ !
$ cp :== copy
$ !
$ if (f$type(sleep).nes."STRING").and.-
     (f$search("ing_src:[testtool.sep.''llbase_bin']sleep.exe").nes."")
$  then sleep := $ing_src:[testtool.sep.'llbase_bin']sleep.exe
$ endif
$ !
$ if (f$type(echo).nes."STRING").and.-
     (f$search("ing_src:[testtool.sep.''llbase_bin']echo.exe").nes."")
$  then echo := $ing_src:[testtool.sep.'llbase_bin']echo.exe
$ endif
$ !
$ set proc/priv=sysnam
$ if f$type(jpt_llbase).nes."STRING"
$  then llbase_src = ""
$	llbase_bin = ""
$  else llbase_src = jpt_llbase+".src."
$	llbase_bin = jpt_llbase+".bin"
$ endif
$ run   :== @ing_src:[testtool.sep.'llbase_src'septool]run.com
$ ferun :== @ing_src:[testtool.sep.'llbase_src'septool]run.com
$ tmrun :== @ing_src:[testtool.sep.'llbase_src'septool]run.com
$ old_qasetuser   = qasetuser
$ old_qasetusertm = qasetusertm
$ old_qasetuserfe = qasetuserfe
$ if f$search("ing_src:[testtool.sep.''llbase_bin']setuser.exe").nes.""
$  then qasetuser   :== $ing_src:[testtool.sep.'llbase_bin']setuser.exe
$	qasetuserfe :== $ing_src:[testtool.sep.'llbase_bin']setuser.exe
$	qasetusertm :== $ing_src:[testtool.sep.'llbase_bin']setuser.exe
$  else
$	if f$search("ing_src:[bin]setuser.exe").nes.""
$	   then qasetuser   :== $ing_src:[bin]setuser.exe
$		qasetuserfe :== $ing_src:[bin]setuser.exe
$		qasetusertm :== $ing_src:[bin]setuser.exe
$	endif
$ endif
$ if f$search("ing_src:[testtool.sep.''llbase_bin']septool.exe").nes.""
$  then sep31 := $ing_src:[testtool.sep.'llbase_bin']septool.exe
$  else if f$search("ing_src:[bin]septool.exe").nes.""
$	   then sep31 := $ing_src:[bin]septool.exe
$	endif
$ endif
$ sep31 'p1' 'p2' 'p3' 'p4' 'p5' 'p6' 'p7' 'p8'
$ set proc/priv=nosysnam
$ deassign/table=lnm$process_directory lnm$file_dev
$ qasetuser   == old_qasetuser
$ qasetusertm == old_qasetusertm
$ qasetuserfe == old_qasetuserfe
$ delete/sym/glo run
$ delete/sym/glo ferun
$ delete/sym/glo tmrun
$ exit
