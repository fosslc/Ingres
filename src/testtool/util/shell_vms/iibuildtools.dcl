$ def_ver = f$verify(0)
$ if f$type(test).nes."INTEGER" then $ test == 0
$ t = f$verify(test)
$ !
$ !*********************************************************************!
$ !
$ !	IIBuildTools.Com
$ !
$ !	History:
$ !	10-sep-91 (donj)
$ !		Created.
$ !	20-sep-91 (donj)
$ !		Add in symbol for acstart.
$ !	30-sep-91 (DonJ)
$ !		Add in symbol for looking at the que, "ac_q"
$ !	14-jan-92 (DonJ)
$ !		Add subroutine, fix_runsv_com to add call
$ !		to runsv.com to execute iitoolsetup.com.
$ !	20-jan-94 (DonJ)
$ !		Add create/name_table for and echo symbol for SEP.
$ !	21-jun-95 (albany)
$ !		Removed references to runsv.com.  We no longer use
$ !		runsv.com in OpenIngres 1.1 and not finding it
$ !		caused this procedure to abort.
$ !	10-jul-98 (kinte01)
$ !		define definitions of sed, lex, and yacc
$ !	22-jan-99 (kinte01)
$ !		add the call for the setup of achilles standard environment
$ !	27-apr-01 (kinte01)
$ !		remove deletion of qasetuser symbols per chrisr
$ !	24-jul-08 (upake01) b97260
$ !		add symbol qawtl
$ !     08-sep-2008 (horda03)
$ !             Correct location of utilities built by Jam.
$ !     13-jan-2009 (upake01)
$ !             Correct location of utilities built by Jam.
$ !     21-May-2009 (horda03)
$ !             The location of RUN.COM depends on whether SEP is being
$ !             executed in a BUILD environment or an installed
$ !             installation.
$ !     10-Sep-2009 (horda03)
$ !             Move the definition of run. ferun, tmrun, qasetuser* from
$ !             sep.com and dbgsep.com to iitoolsymdef.com, qasetuser now
$ !             needed in RUNBE.COM
$ !             Only run ach_stdenv if the file exists.
$ !
$ !
$ !*********************************************************************!
$ !
$ gosub initialize
$ !
$ !*********************************************************************!
$ !
$ call say " Creating Sep command file."
$ gosub create_sep_com
$ if abort_build then $ goto abort_main
$ !
$ !*********************************************************************!
$ !
$ call say " Creating Tools job definitions file."
$ gosub create_jobdef
$ if abort_build then $ goto abort_main
$ !
$ !*********************************************************************!
$ !
$ call say " Creating Tools symbol definition file."
$ gosub create_symdef
$ if abort_build then $ goto abort_main
$ !
$ !*********************************************************************!
$ !
$ call say " Creating Tools setup file."
$ gosub create_setup
$ if abort_build then $ goto abort_main
$ !
$ !*********************************************************************!
$ !
$ @ii_system:[tools]iitoolsetup.com
$ !
$ !*********************************************************************!
$ end_it:
$ !
$ call say " Clean up."
$ gosub clean_up
$ call say " Exiting..."
$ def_ver = f$verify(def_ver)
$ set def 'def_dir'
$ exit
$ !*********************************************************************!
$ abort_main:
$ !
$ call say " Aborting..."
$ def_ver = f$verify(def_ver)
$ set def 'def_dir'
$ exit

$ !*********************************************************************!
$ create_sep_com:							!
$ !*********************************************************************!
$ !
$ set on
$ wrs := write sepcom
$ !
$ open/write sepcom ii_system:[tools.utility]sep.com
$ on error     then $ goto create_sep_com_abort1
$ on control_y then $ goto create_sep_com_abort1
$ !
$ gosub write_sep_com1
$ wrs "$ sep31 := $ii_system:[tools.bin]septool.exe"
$ gosub write_sep_com2
$ !
$ close sepcom
$ purge/keep=2 ii_system:[tools.utility]sep.com
$ !
$ open/write sepcom ii_system:[tools.utility]dbgsep.com
$ on error     then $ goto create_sep_com_abort2
$ on control_y then $ goto create_sep_com_abort2
$ !
$ gosub write_sep_com1
$ wrs "$ sep31 := $ii_system:[tools.bin]septool_dbg.exe"
$ gosub write_sep_com2
$ !
$ close sepcom
$ purge/keep=2 ii_system:[tools.utility]dbgsep.com
$ set noon
$ return
$ !
$ write_sep_com1:
$ !
$ wrs "$ set noon"
$ wrs "$ define/nolog/table=lnm$process_directory LNM$FILE_DEV SEP_TABLE,-"
$ wrs "  LNM$PROCESS,LNM$JOB,LNM$GROUP,LNM$SYSTEM"
$ wrs "$ set proc/priv=sysnam"
$
$ !
$ return
$ !
$ write_sep_com2:
$ !
$ wrs "$ sep31 'p1' 'p2' 'p3' 'p4' 'p5' 'p6' 'p7' 'p8'"
$ wrs "$ !"
$ wrs "$ set proc/priv=nosysnam"
$ wrs "$ deassign/table=lnm$process_directory lnm$file_dev"
$ wrs "$ !"
$ wrs "$ exit"
$ !
$ return
$ !
$ create_sep_com_abort1:
$ !
$ close sepcom
$ call clean_up_file "ii_system:[tools.utility]sep.com"
$ abort_build = 1
$ set noon
$ return
$ !
$ create_sep_com_abort2:
$ !
$ close sepcom
$ call clean_up_file "ii_system:[tools.utility]dbgsep.com"
$ abort_build = 1
$ set noon
$ return

$ !*********************************************************************!
$ create_symdef:							!
$ !*********************************************************************!
$ !
$ run_com = "ii_system:[tools.bin]run.com"
$
$ if f$search( "''run_com'" ) .eqs. ""
$ then
$    run_com = "ii_system:[tools.utility]run.com"
$ endif
$    
$ set on
$ wrtl := write tlsm
$ !
$ open/write tlsm ii_system:[tools.utility]iitoolsymdef.com
$ on error     then $ goto create_symdef_abort
$ on control_y then $ goto create_symdef_abort
$ !
$ wrtl "$ !"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !		SEP Symbols"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !"
$ wrtl "$ dbgsep	:== @ii_system:[tools.utility]dbgsep.com"
$ wrtl "$ echo		:== $ii_system:[tools.bin]echo.exe"
$ wrtl "$ executor	:== $ii_system:[tools.bin]executor.exe"
$ wrtl "$ executor_dbg	:== $ii_system:[tools.bin]executor_dbg.exe"
$ wrtl "$ listexec	:== $ii_system:[tools.bin]listexec.exe"
$ wrtl "$ listexec_dbg	:== $ii_system:[tools.bin]listexec_dbg.exe"
$ wrtl "$ peditor	:== $ii_system:[tools.bin]peditor.exe"
$ wrtl "$ peditor_dbg	:== $ii_system:[tools.bin]peditor_dbg.exe"
$ wrtl "$ sep		:== @ii_system:[tools.utility]sep.com"
$ wrtl "$ sepcc		:== @ii_system:[tools.utility]sepcc.com"
$ wrtl "$ sepesqlc	:== @ii_system:[tools.utility]sepesqlc.com"
$ wrtl "$ seplnk	:== @ii_system:[tools.utility]seplnk.com"
$ wrtl "$ seplib	:== @ii_system:[tools.utility]seplib.com"
$ wrtl "$ deleter	:== $ii_system:[tools.bin]deleter.exe"
$ wrtl "$ source	:== @"
$ wrtl "$ !"
$ wrtl "$ run		:== @''run_com'"
$ wrtl "$ ferun		:== @''run_com'"
$ wrtl "$ tmrun		:== @''run_com'"
$ wrtl "$ qasetuser	:== $ii_system:[tools.bin]setuser.exe"
$ wrtl "$ qasetuserfe	:== $ii_system:[tools.bin]setuser.exe"
$ wrtl "$ qasetusertm	:== $ii_system:[tools.bin]setuser.exe"
$ wrtl "$ !"
$ wrtl "$ !"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !		Achilles Symbols"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !"
$ wrtl "$ achilles	:== $ii_system:[tools.bin]achilles.exe"
$ wrtl "$ accopier	:== $ii_system:[tools.bin]accopier.exe"
$ wrtl "$ readlog	:== $ii_system:[tools.bin]readlog.exe"
$ wrtl "$ !"
$ wrtl "$ acstart	:== @ii_system:[tools.utility]acstart.com"
$ wrtl "$ ac_q*ue	:== show que/bat/all achilles$batch"
$ wrtl "$ !"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !		Achilles Symbols"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !"
$ wrtl "$ iiversion     :== @ii_system:[tools.utility]version.com"
$ wrtl "$ !"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !             SHELL Symbols"
$ wrtl "$ !*******************************************!"
$ wrtl "$ sed		:== $ii_system:[tools.bin]sed.exe"
$ wrtl "$ lex		:== $ii_system:[tools.bin]lex.exe"
$ wrtl "$ yacc		:== $ii_system:[tools.bin]yacc.exe"
$ wrtl "$ !"
$ wrtl "$ !*******************************************!"
$ wrtl "$ !             QAWTL Symbols"
$ wrtl "$ !*******************************************!"
$ wrtl "$ qawtl		:== $ii_system:[tools.bin]qawtl.exe"
$ wrtl "$ !"
$ wrtl "$ if f$search( ""ii_system:[tools.utility]ach_stdenv.com"") .nes. """""
$ wrtl "$ then"
$ wrtl "$    @ii_system:[tools.utility]ach_stdenv.com
$ wrtl "$ endif"
$ wrtl "$ !"
$ if f$search("sys$common:[shellexe]*.*;*").nes.""
$  then
$	wrtl "$ !*******************************************!"
$	wrtl "$ !		SHELL Symbols"
$	wrtl "$ !*******************************************!"
$	wrtl "$ !"
$	if f$search("sys$common:[shellexe]awk.exe").nes."" -
	   then $ wrtl "$ awk      :== $sys$common:[shellexe]awk.exe"
$	if f$search("sys$common:[shellexe]basename.exe").nes."" -
	   then $ wrtl "$ basename :== $sys$common:[shellexe]basename.exe"
$	if f$search("sys$common:[shellexe]cat.exe").nes."" -
	   then $ wrtl "$ cat      :== $sys$common:[shellexe]cat.exe"
$	if f$search("sys$common:[shellexe]chmod.exe").nes."" -
	   then $ wrtl "$ chmod    :== $sys$common:[shellexe]chmod.exe"
$	if f$search("sys$common:[shellexe]chown.exe").nes."" -
	   then $ wrtl "$ chown    :== $sys$common:[shellexe]chown.exe"
$	if f$search("sys$common:[shellexe]echo.exe").nes."" -
	   then $ wrtl "$ echo     :== $sys$common:[shellexe]echo.exe"
$	if f$search("sys$common:[shellexe]grep.exe").nes."" -
	   then $ wrtl "$ grep     :== $sys$common:[shellexe]grep.exe"
$	if f$search("sys$common:[shellexe]head.exe").nes."" -
	   then $ wrtl "$ head     :== $sys$common:[shellexe]head.exe"
$	if f$search("sys$common:[shellexe]mkdir.exe").nes."" -
	   then $ wrtl "$ mkdir    :== $sys$common:[shellexe]mkdir.exe"
$	if f$search("sys$common:[shellexe]nroff.exe").nes."" -
	   then $ wrtl "$ nroff    :== $sys$common:[shellexe]nroff.exe"
$	if f$search("sys$common:[shellexe]od.exe").nes."" -
	   then $ wrtl "$ od       :== $sys$common:[shellexe]od.exe"
$	if f$search("sys$common:[shellexe]pwd.exe").nes."" -
	   then $ wrtl "$ pwd      :== $sys$common:[shellexe]pwd.exe"
$	if f$search("sys$common:[shellexe]rm.exe").nes."" -
	   then $ wrtl "$ rm       :== $sys$common:[shellexe]rm.exe"
$	if f$search("sys$common:[shellexe]rmdir.exe").nes."" -
	   then $ wrtl "$ rmdir    :== $sys$common:[shellexe]rmdir.exe"
$	if f$search("sys$common:[shellexe]sleep.exe").nes."" -
	   then $ wrtl "$ sleep    :== $sys$common:[shellexe]sleep.exe"
$	if f$search("sys$common:[shellexe]tail.exe").nes."" -
	   then $ wrtl "$ tail     :== $sys$common:[shellexe]tail.exe"
$	if f$search("sys$common:[shellexe]touch.exe").nes."" -
	   then $ wrtl "$ touch    :== $sys$common:[shellexe]touch.exe"
$	if f$search("sys$common:[shellexe]wc.exe").nes."" -
	   then $ wrtl "$ wc       :== $sys$common:[shellexe]wc.exe"
$	if f$search("sys$common:[shellexe]who.exe").nes."" -
	   then $ wrtl "$ who      :== $sys$common:[shellexe]who.exe"
$	wrtl "$ !"
$ endif
$ wrtl "$ exit"
$ !
$ close tlsm
$ purge/keep=2 ii_system:[tools.utility]iitoolsymdef.com
$ set noon
$ return
$ !
$ create_symdef_abort:
$ !
$ close tlsm
$ call clean_up_file "ii_system:[tools.utility]iitoolsymdef.com"
$ abort_build = 1
$ set noon
$ return

$ !*********************************************************************!
$ create_jobdef:							!
$ !*********************************************************************!
$ !
$ set on
$ wrtl := write tlsm
$ !
$ open/write tlsm ii_system:[tools.utility]iitooljobdef.com
$ on error     then $ goto create_jobdef_abort
$ on control_y then $ goto create_jobdef_abort
$ !
$ wrtl "$ !"
$ wrtl "$ define/job/exec/nolog	tst_sep		ii_system:[tools.files]"
$ wrtl "$ !"
$ wrtl "$ exit"
$ !
$ close tlsm
$ purge/keep=2 ii_system:[tools.utility]iitooljobdef.com
$ set noon
$ return
$ !
$ create_jobdef_abort:
$ !
$ close tlsm
$ call clean_up_file "ii_system:[tools.utility]iitooljobdef.com"
$ abort_build = 1
$ set noon
$ return

$ !*********************************************************************!
$ create_setup:								!
$ !*********************************************************************!
$ !
$ set on
$ wrsu := write tlsu
$ !
$ open/write tlsu ii_system:[tools]iitoolsetup.com
$ on error     then $ goto create_setup_abort
$ on control_y then $ goto create_setup_abort
$ !
$ wrsu "$ set noon"
$ wrsu "$ !"
$ wrsu "$ create/name_table/parent=lnm$process_directory/prot=(w:rwed)/nolog sep_table"
$ wrsu "$ !"
$ wrsu "$ @ii_system:[tools.utility]iitooljobdef.com/out=nl:"
$ wrsu "$ @ii_system:[tools.utility]iitoolsymdef.com/out=nl:"
$ wrsu "$ !"
$ wrsu "$ set on"
$ wrsu "$ !"
$ wrsu "$ exit"
$ !
$ close tlsu
$ purge/keep=2 ii_system:[tools]iitoolsetup.com
$ set noon
$ return
$ !
$ create_setup_abort:
$ !
$ close tlsm
$ call clean_up_file "ii_system:[tools]iitoolsetup.com"
$ abort_build = 1
$ set noon
$ return

$ !*********************************************************************!
$ say: subroutine							!
$ !*********************************************************************!
$ !
$ write sys$error ""
$ write sys$error p1
$ write sys$error ""
$ !
$ exit
$ endsubroutine
$ !*********************************************************************!
$ clean_up_file: subroutine
$ !*********************************************************************!
$ !
$ s = f$search(p1)
$ if s.nes.""
$  then
$	x = " Aborting "+f$parse(s,,,"NAME")+f$parse(s,,,"TYPE")+" creation..."
$	call say "''x'"
$	call say "	Deleting ''s'"
$	delete 's'
$ endif
$ !
$ exit
$ endsubroutine
$ !*********************************************************************!
$ clean_up:
$ !*********************************************************************!
$ !
$ purge ii_system:[tools...]
$ delete/sym/glo condns_dir
$ !
$ return
$ !*********************************************************************!
$ !*********************************************************************!
$ initialize:
$ !*********************************************************************!
$ !
$ abort_build  = 0
$ condns_dir :== call condns_dir
$ nodename    == f$getsyi("nodename")
$ wro	     :== write sys$output
$ def_dir      = f$environment("DEFAULT")
$ !
$ return
$ !*********************************************************************!
$ exit
