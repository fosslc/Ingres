$ !*********************************************************************!
$ !									!
$ !    Version.Com - Determine the version level of an Installation.	!
$ !									!
$ !*********************************************************************!
$ !
$ !	History:
$ !	26-sep-1991 (DonJ)
$ !		Rearrange to optimize. If version*.rel files exist, reset
$ !		ii_version as long as not in batch. Don't get fooled by
$ !		old version_tls.rel hanging out in a 6.4 installation,
$ !		read both files and compare versions.
$ !
$ !	27-sep-1991 (DonJ)
$ !		Small problem in batch mode, 1 shud've been a 0.
$ !
$ !	27-aug-1992 (DonJ)
$ !		Problem with running terminal monitor to find version
$ !		if version*.rel files don't exist. Procedure was trying
$ !		to open tmp file for read twice rather than for write to
$ !		create it then read to read tm's results.
$ !
$ !*********************************************************************!
$ !	The following symbols are used by other procedures that call    !
$ !	this one:							!
$ !									!
$ !	sys_defined	- Set to 1 if Ingres Installation is correctly	!
$ !			  set up. Otherwise set to 0.			!
$ !	test_install	- Set to 1 if Ingres is a Test Installation.	!
$ !			  Otherwise set to 0.				!
$ !	prod_install	- Set to 1 if Ingres is a Production Instal-	!
$ !			  lation. Otherwise set to 0.			!
$ !	ii_version	- Set to the Ingres Version string,		!
$ !			  #.# (???.???/##). symbol undefined if Ingres	!
$ !			  Installation is not correctly set up.		!
$ !	iiv		- Set to the Major Ingres Version string, #.#.	!
$ !			  symbol undefined if Ingres Installation is	!
$ !			  not correctly set up.				!
$ !									!
$ !*********************************************************************!
$ gosub init_com
$ !
$ if p1.nes.""
$  then if test.eq.0
$	   then define sys$error  NL:	! not interested in setup error msgs.
$		define sys$output NL:
$	endif
$	'p1'
$	if test.eq.0
$	   then if f$trnlnm("sys$error").eqs."NL:" -
		   then $ deassign sys$error
$		if f$trnlnm("sys$output").eqs."NL:" -
		   then $ deassign sys$output
$	endif
$	if f$type(ii_version).nes."".and.mode_.nes."BATCH" -
	   then $ delete/symbol/global ii_version
$ endif
$ !
$ !********************************************************************!
$ !	Check if there's a valid installation.
$ !********************************************************************!
$ !
$ if f$trnlnm("ii_system").eqs."".or.f$trnlnm("ii_authorization").eqs.""
$  then 
$	test_install == 0	! Other procedures depend on this being set
$	prod_install == 0	! Other procedures depend on this being set
$	sys_defined  == 0	! Other procedures depend on this being set
$	wro ""
$	wro " Installation not defined..."
$	wro ""
$  else 
$	sys_defined  == 1	! Other procedures depend on this being set
$	if f$trnlnm("ii_installation" ).nes.""
$	   then test_install == 1
$		prod_install == 0
$	   else test_install == 0
$		prod_install == 1
$	endif
$ endif
$ if sys_defined.eq.0 then $ goto end_it
$ !
$ !********************************************************************!
$ !	Get rid of ii_version symbol if defined to blank.
$ !********************************************************************!
$ !
$ if f$type(ii_version).eqs."STRING" then $ if ii_version.eqs."" -
     then $ delete/symbol/global ii_version
$ !
$ !********************************************************************!
$ !
$ version_tls = f$search("ii_system:[ingres]version_tls.rel")
$ version_rel = f$search("ii_system:[ingres]version.rel")
$ !
$ if (f$type(ii_version).eqs."")
$  then ii_vers_defined = 0
$  else ii_vers_defined = 1
$	old_ii_vers = ii_version
$ endif
$ !
$ if (ii_vers_defined.eq.0).or.-
     (mode_.eqs."INTERACTIVE".and.(version_tls.nes."".or.version_rel.nes.""))
$  then if version_rel.eqs.""
$	   then if version_tls.eqs.""
$		   then call run_sql_for_vers
$		   else call read_rel_for_vers "''version_tls'"
$		endif
$	   else if version_tls.eqs.""
$		   then call read_rel_for_vers "''version_rel'"
$		   else call read_rel_for_vers "''version_rel'" "xxx"
$			x = f$integer(maint_rel)+(f$integer(minor_rel)*100)-
						+(f$integer(major_rel)*1000)
$			call read_rel_for_vers "''version_tls'" "yyy"
$			y = f$integer(maint_rel)+(f$integer(minor_rel)*100)-
						+(f$integer(major_rel)*1000)
$			if x.gt.y
$			   then ii_version == xxx
$			   else ii_version == yyy
$			endif
$			delete/sym/glo xxx
$			delete/sym/glo yyy
$		endif
$	endif
$	if ii_vers_defined.eq.1 -
	   then $ if old_ii_vers.eqs.ii_version -
		     then $ new_sym == 0
$	if new_sym
$	   then wel_file = f$search("ii_system:[ingres]welcome.com")
$		if wel_file.eqs."" then $ no_wel_file = 1
$	endif
$ endif
$ !
$ iiv == f$element(0,"/",ii_version)
$ !
$ if new_sym
$  then if no_wel_file
$	   then gosub create_welcome_file
$	   else gosub update_welcome_file
$	endif
$ endif
$ !
$ wro ""
$ wro "Ingres Version is ''ii_version'."
$ wro ""
$ if mode_.eqs."INTERACTIVE" then $ wait 00:00:02
$ !
$ end_it:
$ !
$ def_ver = f$verify(def_ver)
$ set def 'def_dir'
$ set process/priv=(noall,'def_priv')
$ if f$type(new_sym  ).nes."" then $ delete/sym/glo new_sym
$ if f$type(pid      ).nes."" then $ delete/sym/glo pid
$ if f$type(major_rel).nes."" then $ delete/sym/glo major_rel
$ if f$type(minor_rel).nes."" then $ delete/sym/glo minor_rel
$ if f$type(maint_rel).nes."" then $ delete/sym/glo maint_rel
$ !
$ exit
$ !
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ read_rel_for_vers: subroutine
$ !
$ if p1.eqs.""
$  then rel_fil = version_rel
$  else rel_fil = p1
$ endif
$ if p2.eqs.""
$  then ret_sym = "ii_version"
$  else ret_sym = p2
$ endif
$ !
$ open/read tmp 'rel_fil'
$ read tmp l1
$ !
$ l1 = f$edit( l1,"COMPRESS,TRIM")
$ !
$ major_rel	== f$element(0,".",l1)
$ r1		 = f$element(1,".",l1)
$ r2		 = f$element(2,".",l1)
$ r		 = r1+"."+r2
$ !
$ major_rel	== f$extract(f$length(major_rel)-1,1,major_rel)
$ !
$ minor_rel	== f$element(0,"/",r)
$ r1		 = f$element(1,"/",r)
$ r2		 = f$element(2,"/",r)
$ !
$ maint_rel	== f$edit(f$element(0,"(",r1),"TRIM")
$ r1		 = f$element(1,"(",r1)
$ bld_lev	 = f$element(0,")",r2)
$ spc_dist	 = f$edit(f$element(1,")",r2),"TRIM")
$ host		 = f$element(0,".",r1)
$ opsys		 = f$element(1,".",r1)
$ !
$ 'ret_sym' == major_rel+"."+minor_rel+"/"+maint_rel-
	       +" ("+host+"."+opsys+"/"+bld_lev+")"+spc_dist
$ !
$ close tmp
$ new_sym == 1
$ !
$ exit
$ endsubroutine
$ !
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ run_sql_for_vers: subroutine
$ !
$ tmp_file = "sys$login:tmp''pid'.tmp"
$ !
$ if p1.eqs.""
$  then ret_sym = "ii_version"
$  else ret_sym = p1
$ endif
$ !
$ open/write tmp 'tmp_file'
$ write tmp "\q"
$ close tmp
$ sql iidbdb <'tmp_file' >'tmp_file'
$ purge 'tmp_file'
$ !
$ open/read tmp 'tmp_file'
$ read tmp l1
$ read tmp l1
$ vpos = f$locate("Version ",l1) + 8
$ 'ret_sym' == f$edit( f$extract( vpos,(f$locate(")",l1)+2)-vpos,l1 ),"TRIM" )
$ close tmp
$ delete 'tmp_file';*
$ new_sym == 1
$ !
$ exit
$ endsubroutine
$ !
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ create_welcome_file:
$ !
$ if ii_version.eqs."" then $ return
$ !
$ set on
$ !
$ on error then $ goto err_in_open_write0
$ open/write wl1 ii_system:[ingres]welcome.com
$ !
$ line_ = "$ ii_version == ""''ii_version'"""
$ write wl1 line_
$ close wl1
$ new_sym    == 0
$ no_wel_file = 0
$ !
$ set noon
$ return
$ !
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ update_welcome_file:
$ !
$ if ii_version.eqs."" then $ return
$ !
$ wel_file_uic = f$file_attribute(wel_file,"uic")
$ no_vers_dif = 0
$ !
$ set on
$ on error then $ goto err_in_open_read0
$ open/read  wl1 'wel_file'
$ !
$ start_wl1_read0:
$	!
$	read/end_of_file=end_wl1_read0/error=end_wl1_read0 wl1 tmp
$	!
$	tmp2   = f$edit(tmp,"COLLAPSE,UPCASE")
$	tmplen = f$length(tmp2)
$	tloc   = f$locate("II_VERSION==""''ii_version'""",TMP2)
$	if tloc.eq.tmplen -
	   then $ tloc = f$locate("II_VERSION:==''ii_version'",TMP2)
$	if tloc.eq.tmplen then $ goto start_wl1_read0
$	no_vers_dif = 1
$	!
$ end_wl1_read0:
$ close wl1
$ !
$ !**********************************************************!
$ !
$ if no_vers_dif then $ return
$ !
$ !**********************************************************!
$ !
$ set on
$ on error then $ goto err_in_open_read0
$ open/read  wl1 'wel_file'
$ !
$ on error then $ goto err_in_open_write0
$ open/write wl2 ii_system:[ingres]welcome.com
$ !
$ on error then $ goto end_wl1_read
$ line_ = "$ ii_version == ""''ii_version'"""
$ write wl2 line_
$ !
$ start_wl1_read:
$	!
$	read/end_of_file=end_wl1_read/error=end_wl1_read wl1 tmp
$	!
$	tmp2   = f$edit(tmp,"COLLAPSE,UPCASE")
$	tmplen = f$length(tmp2)
$	tloc   = f$locate("II_VERSION==",TMP2)
$	if tloc.eq.tmplen then $ tloc = f$locate("II_VERSION:==",TMP2)
$	if tloc.eq.tmplen then $ write wl2 tmp
$	!
$	goto start_wl1_read
$ end_wl1_read:
$ !
$ close wl1
$ close wl2
$ !
$ new_sym == 0
$ !
$ on error then $ goto err_in_purge0
$ assign/user nl: sys$output
$ assign/user nl: sys$error
$ purge/keep=2 ii_system:[ingres]welcome.com
$ !
$ set noon
$ return
$ !
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ err_in__purge0:
$ !
$ wro " Can't purge file."
$ !
$ set noon
$ return
$ !
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ err_in_open_read0:
$ !
$ wro " Can't open file to read."
$ !
$ set noon
$ return
$ !
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ err_in_open_write0:
$ !
$ wro " Can't open file to write."
$ !
$ set noon
$ return
$ !********************************************************************!
$ !
$ !********************************************************************!
$ !
$ init_com:
$ !
$ if f$type(test).eqs."" then $ test == 0
$ def_ver      = f$verify(test)
$ set noon
$ def_dir      = f$environment("DEFAULT")
$ if f$type(nodename).nes."STRING" then $ nodename == f$getsyi("nodename")
$ def_priv     = f$getjpi("","procpriv")
$ def_uic      = f$getjpi("","uic")
$ wro	      := write sys$output
$ pid	      == f$getjpi(0,"PID")
$ new_sym     == 0
$ no_wel_file  = 0
$ mode_        = f$mode()
$ sys_defined == 0
$ !
$ return
