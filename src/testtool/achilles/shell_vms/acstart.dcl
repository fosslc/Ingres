$ set noon
$ if f$type(test).nes."INTEGER" then $ test == 0
$ def_ver = f$verify(test)
$ !*********************************************************************!
$ !
$ !	ACStart.Com
$ !
$ !		Front for the achilles test tool.
$ !
$ !	History:
$ !	20-sep-91 (DonJ)
$ !		Created.
$ !	30-sep-91 (DonJ)
$ !		Add logic to check batch que, he if achilles$batch is
$ !		defined or setup straight as a batch queue. Either way,
$ !		check that it runs on the same node that we are executing
$ !		on. If achilles$batch isn't setup or its on a different
$ !		node of the cluster, ask for the proper queue, then recheck.
$ !
$ !	7-aug-93 (Jeffr)
$ !		Added changes for Achilles Version 2. (i.e) the
$ !		ability to handle -a flag for actual time to run
$ !   
$ !     21-oct-93 (Jeffr)
$ !		Myriad changes to upgrade this to 6.5 including adding
$ !		new symbol definitons  for ingsysdef, iisymboldef
$ !		also changed valid_user.c to angle brackets for .h's
$ !		 - this gave a problem under Alpha VMS
$ !*********************************************************************!
$ !
$ set on
$ gosub initialize
$ gosub error_control
$ call wr_message ""
$ gosub eval_parameters
$ gosub error_control
$ if help_str then $ goto print_help
$ !
$ gosub confirm_output_dir
$ gosub error_control
$ !
$ !*********************************************************************!
$ !	Look for Test Case.
$ !*********************************************************************!
$ !
$ call find_test_case "''p1'" "test_case_spec"
$ if test_case_spec.eqs.""
$  then call wr_message "Test Case, ''p1', cannot be found..." "get_out"
$  else call wr_message "Found ''test_case_spec'"
$ endif
$ gosub error_control
$ !
$ !*********************************************************************!
$ !	Check Out Front End Installation
$ !*********************************************************************!
$ !
$ call wr_message "Checking Ingres Installation"
$ gosub check_db_spec
$ gosub error_control
$ !
$ !*********************************************************************!
$ !	If virtual node specified, check it in NETU.
$ !*********************************************************************!
$ !
$ if vn_is
$  then call wr_message "Checking ''vns':: in Netu."
$	gosub look_for_vnode
$	gosub error_control
$ endif
$ !
$ !*********************************************************************!
$ !	Try accessing database.
$ !*********************************************************************!
$ !
$ call wr_message "Try connecting to ''dbspc'"
$ gosub try_connecting
$ gosub error_control
$ !
$ !*********************************************************************!
$ !	Read Test Case and Create this specific config file.
$ !*********************************************************************!
$ !
$ build_spec_cfg:
$ specific_cfg = res_opt+hostname+"''itera'"+pid+".config"
$ log_fil = res_opt+hostname+"''itera'"+pid+".log"
$ if f$search(specific_cfg).nes.""
$  then itera = itera + 1
$	goto build_spec_cfg
$ endif
$ call wr_message "Creating ''specific_cfg'"
$ call wr_message "Creating ''log_fil'"
$ !
$ open/write nlog 'log_fil'
$ on control_y then $ goto end_sini
$ on error     then $ goto end_sini
$ !
$ !
$ gosub create_spec_cfg
$ gosub error_control
$ !
$ !*********************************************************************!
$ !	Build Children's Initialization File.
$ !*********************************************************************!
$ !
$ build_spec_ini:
$ specific_ini = res_opt+hostname+"''itera'"+pid+".ini"
$ if f$search(specific_ini).nes.""
$  then itera = itera + 1
$	goto build_spec_ini
$ endif
$ call wr_message "Creating ''specific_ini'"
$ gosub create_spec_ini
$ gosub error_control
$ !
$ call wr_message "Looking for batch que"
$ gosub look_for_batch_que
$ gosub error_control
$ !
$ !*********************************************************************!
$ !	Go Ahead and do it.
$ !*********************************************************************!
$ !
$ if run_interactive
$  then if debug_on
$	   then call wr_message "Here's the command we would've used."
$		wro ""
$		wro "$ achilles -e ''specific_ini' -"
$		wro "           -d ''dbspc'-"
$               wro "           -f ''specific_cfg'-"
$               wro "           -o ''res_opt'-"
$		wro "		-l -"
$		wro "		-a ''act_opt'"
$		wro ""
$	   else if test.eq.0 then $ set ver
$		achilles 	-e 'specific_ini' -
				-d 'dbspc'-
				-f 'specific_cfg'-
				-o 'res_opt'-
				-l -
				-a 'act_opt'
$		x = f$verifty(test)
$	endif
$  else !***************************************************************!
$	!	Build Batch file.
$	!***************************************************************!
$	!
$	build_spec_submit:
$	specific_submit = res_opt+"submit_"+hostname+"''itera'"+pid+".com"
$	specific_log    = res_opt+"submit_"+hostname+"''itera'"+pid+".log"
$	job_name	= hostname+"''itera'"+pid
$	if f$search(specific_submit).nes."".or.f$search(specific_log).nes.""
$	   then itera = itera + 1
$		goto build_spec_submit
$	endif
$	call wr_message "Creating ''specific_submit'"
$	gosub create_spec_submit
$	gosub error_control
$	!
$	!***************************************************************!
$	!	Do the submission to batch
$	!***************************************************************!
$	!
$	if debug_on
$	   then call wr_message "Here's the command we would've used."
$		tmp = "submit/que=achilles$batch/log=" + specific_log -
	            + "/name=" + job_name + "/noprint/notify  " -
	            + specific_submit
$		wro ""
$		wro "$ "+tmp
$		wro ""
$		call wr_message ""
$	   else call wr_message "Submitting Batch Job..."
$		submit/que=achilles$batch	-
			/log='specific_log'	-
			/name='job_name'	-
			/noprint/notify         'specific_submit'
$	endif
$	!***************************************************************!
$ endif
$ !
$ !*********************************************************************!
$ end_it:
$ !*********************************************************************!
$ !
$ call wr_message "[Exiting...]"
$ wro ""
$ set def 'def_dir'
$ def_ver = f$verify(def_ver)
$ gosub clean_up
$ !
$ exit
$ !*********************************************************************!

$ !*********************************************************************!
$ eval_parameters:
$ !*********************************************************************!
$ nr_of_p = 0
$ i = 1
$ j = 1
$ eval1_loop_start:
$	if i.eq.9 then $ goto eval1_loop_end
$	if p'i'.nes.""
$	   then if i.ne.j then $ p'j' = p'i'
$		j = j + 1
$	endif
$	i = i + 1
$ goto eval1_loop_start
$ eval1_loop_end:
$ nr_of_p = j - 1
$ i = 1
$ j = 1
$ eval2_loop_start:
$	if i.gt.nr_of_p then $ goto eval2_loop_end
$	p  = p'i'
$	pl = f$length(p)
$	s  = f$element(0,"=","''p'")
$	sl = f$length(s)
$	if sl.lt.pl
$	   then ol = pl - (sl + 1)
$		o  = f$extract(sl+1,ol,p)
$	   else ol = 0
$		o  = ""
$	endif
$	if f$extract(0,1,s).eqs."-"
$	   then sl = sl - 1
$		s  = f$extract(1,sl,s)
$		!***********************************!
$		!	   Process Switch
$		!***********************************!
$		!
$		if s.eqs.f$extract(0,sl,"OUTPUT")
$		   then if ol.ne.0
$			   then res_opt == f$parse(o,,,"DEVICE",   "SYNTAX_ONLY")-
					 + f$parse(o,,,"DIRECTORY","SYNTAX_ONLY")
$				have_res_opt == 1
$			   else msg_str = "bad -o flag, no pathname."
$				gosub eval_sw2
$			endif
$		else if s.eqs.f$extract(0,sl,"CONFIG")
$		   then if ol.ne.0
$			   then cfg_opt == f$parse(o,,,"DEVICE"   ,"SYNTAX_ONLY")-
					 - f$parse(o,,,"DIRECTORY","SYNTAX_ONLY")
$				have_cfg_opt == 1
$			   else msg_str = "bad -c flag, no pathname."
$				gosub eval_sw2
$			endif
$               else if s.eqs.f$extract(0,sl,"ACTUAL")
$                  then if ol.ne.0
$                          then act_opt == f$integer(o)
$                               have_act_opt == 1
$                          else k = i + 1
$                               if k.le.nr_of_p
$                                  then pk = p'k'
$                                       kc = f$extract(0,1,pk)
$                                       if kc.eqs."-"
$                                          then act_opt == 0
$                                               have_act_opt == 1
$                                          else gosub eval_sw1
$                                       endif
$                                  else act_opt == 0
$                                       have_act_opt == 1
$                               endif
$                       endif
$		else if s.eqs.f$extract(0,sl,"REPEAT")
$		   then if ol.ne.0
$			   then rep_opt == f$integer(o)
$				have_rep_opt == 1
$			   else k = i + 1
$				if k.le.nr_of_p
$				   then pk = p'k'
$					kc = f$extract(0,1,pk)
$					if kc.eqs."-"
$					   then rep_opt == 0
$						have_rep_opt == 1
$					   else gosub eval_sw1
$					endif
$				   else rep_opt == 0
$					have_rep_opt == 1
$				endif
$			endif
$		else if s.eqs.f$extract(0,sl,"USERNAME")
$		   then if ol.ne.0
$			   then call check_user "''o'" "get_out"
$				if get_out.ne.1
$				   then call wr_message "Bad Username, ''o'" "help_str"
$				   else achilles_user == o
$					have_usr_opt  == 1
$					get_out       == 0
$				endif
$			   else msg_str = "bad -u flag, no username."
$				gosub eval_sw2
$			endif
$		else if s.eqs.f$extract(0,sl,"ENVIRONMENT")
$		   then if ol.ne.0
$			   then env_opt       == o
$				have_env_opt  == 1
$			   else msg_str = "bad -e flag, no filespec."
$				gosub eval_sw2
$			endif
$		else if s.eqs.f$extract(0,sl,"INTERACTIVE")
$		   then run_interactive == 1
$		else if s.eqs.f$extract(0,sl,"BATCH")
$		   then run_interactive == 0
$		else if s.eqs.f$extract(0,sl,"DEBUG")
$		   then debug_on == 1
$		else call wr_message "Illegal Switch, <''p'>" "help_str"
$		endif
$		endif
$		endif
$		endif
$		endif
$		endif
$		endif
$		endif
$		endif
$		!***********************************!
$	   else if i.ne.j then $ p'j' = p'i'
$		j = j + 1
$	endif
$	i = i + 1
$ goto eval2_loop_start
$ eval2_loop_end:
$ nr_of_p = j - 1
$ if help_str.eq.0
$  then if nr_of_p.eq.0
$	   then help_str == 1
$	   else if nr_of_p.eq.1
$		   then if p1.eqs."?"
$			   then help_str == 1
$			   else if f$edit(p1,"UPCASE")-
				   .eqs.f$extract(0,f$length(p1),"HELP")-
				   then $ help_str == 1
$			endif
$		endif
$	endif
$ endif
$ return
$ !
$ !********!
$ eval_sw1:
$ !********!
$ if kc.eqs."="
$  then p'k' = "-"+s+p'k'
$  else p'k' = "-"+s+"="+p'k'
$ endif
$ return
$ !********!
$ eval_sw2:
$ !********!
$ k = i + 1
$ if k.le.nr_of_p
$  then pk = p'k'
$	kc = f$extract(0,1,pk)
$	if kc.eqs."-"
$	   then get_out == 1
$	   else gosub eval_sw1
$	endif
$  else get_out == 1
$ endif
$ if get_out.ne.0 then $ call wr_message "''msg_str'" "get_out"
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ print_help:
$ !*********************************************************************!
$ wro ""
$ wro "Usage: acstart <TEST_CASE> <DATA_BASE_SPEC>"
$ wro ""
$ wro "Switches:"
$ wro ""
$ wro "    -o[utput]=path    Change output directory where all achilles"
$ wro "                      files are written. Where path is dev:[dir]"
$ wro "                      or logical_name:"
$ wro ""
$ wro "    -r[epeat]=n       Change repeat tests parameter in config or"
$ wro "                      template file."
$ wro ""
$ wro "    -u[sername]=user  Change effective user parameter in config"
$ wro "                      or template file."
$ wro ""
$ wro "    -d[ebug]          Do all sanity checking and file creation"
$ wro "                      without actually starting up achilles."
$ wro ""
$ wro "    -i[nteractive]    Start up achilles in this process rather"
$ wro "                      than as a batch process."
$ wro "
$ wro "    -a[ctual]	     actual time to run Achilles - Note:
$ wro "			     this will override -r[epeat] 	     
$ wro ""
$ wro " There's more help by typing: HELP ACHILLES"
$ wro ""
$ exit

$ !*********************************************************************!
$ look_for_dbms:
$ !*********************************************************************!
$ !
$ assign/user ac'pid'.tmp sys$output
$ iinamu
show ingres
quit
$ open/read actmp ac'pid'.tmp
$ read actmp acl
$ read actmp acl
$ read actmp acl
$ if f$element(0," ",acl).eqs."IINAMU>".and.f$element(1," ",acl).eqs."INGRES"
$  then dbms_svr = 1
$	name_svr = 1
$  else dbms_svr = 0
$ endif
$ close actmp
$ dele ac'pid'.tmp;*
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ look_for_gcc:
$ !*********************************************************************!
$ !
$ assign/user ac'pid'.tmp sys$output
$ iinamu
show comsvr
quit
$ open/read actmp ac'pid'.tmp
$ read actmp acl
$ read actmp acl
$ read actmp acl
$ if f$element(0," ",acl).eqs."IINAMU>".and.f$element(1," ",acl).eqs."COMSVR"
$  then gcc_svr = 1
$  else gcc_svr = 0
$ endif
$ close actmp
$ dele ac'pid'.tmp;*
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ look_for_star:
$ !*********************************************************************!
$ !
$ assign/user ac'pid'.tmp sys$output
$ iinamu
show star
quit
$ open/read actmp ac'pid'.tmp
$ read actmp acl
$ read actmp acl
$ read actmp acl
$ if f$element(0," ",acl).eqs."IINAMU>".and.f$element(1," ",acl).eqs."STAR"
$  then star_svr = 1
$  else star_svr = 0
$ endif
$ close actmp
$ dele ac'pid'.tmp;*
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ look_for_rms:
$ !*********************************************************************!
$ !
$ assign/user ac'pid'.tmp sys$output
$ iinamu
show rms
quit
$ open/read actmp ac'pid'.tmp
$ read actmp acl
$ read actmp acl
$ read actmp acl
$ if f$element(0," ",acl).eqs."IINAMU>".and.f$element(1," ",acl).eqs."RMS"
$  then rms_svr = 1
$  else rms_svr = 0
$ endif
$ close actmp
$ dele ac'pid'.tmp;*
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ look_for_rdb:
$ !*********************************************************************!
$ !
$ assign/user ac'pid'.tmp sys$output
$ iinamu
show rdb
quit
$ open/read actmp ac'pid'.tmp
$ read actmp acl
$ read actmp acl
$ read actmp acl
$ if f$element(0," ",acl).eqs."IINAMU>".and.f$element(1," ",acl).eqs."RDB"
$  then rdb_svr = 1
$  else rdb_svr = 0
$ endif
$ close actmp
$ dele ac'pid'.tmp;*
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ create_spec_cfg:
$ !*********************************************************************!
$ !
$ !--------------------------------------------------------------!
$ ! Typical template file:
$ !--------------------------------------------------------------!
$ !
$ ! #
$ ! # Twelve users LOW interupt rate
$ ! #
$ ! #Name Iter  I-sleep C/D Slp D/C Slp Childs  IntCnt  KillCnt IntInt  KillInt
$ !
$ ! ac001 200   0       0       0       1       0       0       0       0
$ ! ac002 100   45      0       0       3       0       0       0       0
$ ! ac007 100   45      0       0       4       0       0       0       0
$ ! ac011 1000  0       0       0       4       1       0       3600    0
$ !
$ !--------------------------------------------------------------!
$ ! This is the resultant configuration file, either can be used.
$ !--------------------------------------------------------------!
$ !
$ ! #
$ ! # USR_SWIFT:[DONJ.TMP]SWIFT00309.config 
$ ! #     contructed from 
$ ! #       USR_SWIFT:[DONJ.TMP]D.;1
$ ! #
$ ! ac001 <DBNAME> 200 0 <CHILDNUM> 0 0 ; 1 0 0 0 0 0 <NONE> SWIFT00309 DONJ
$ ! ac002 <DBNAME> 100 45 <CHILDNUM> 0 0 ; 3 0 0 0 0 0 <NONE> SWIFT00309 DONJ
$ ! ac007 <DBNAME> 100 45 <CHILDNUM> 0 0 ; 4 0 0 0 0 0 <NONE> SWIFT00309 DONJ
$ ! ac011 <DBNAME> 1000 0 <CHILDNUM> 0 0 ; 4 0 1 0 3600 0 <NONE> SWIFT00309 DONJ
$ !
$ !*********************************************************************!
$ !
$ open/read cfg 'test_case_spec'
$ open/write scfg 'specific_cfg'
$ on error     then $ goto cfg_loop_end
$ on control_y then $ goto cfg_loop_end
$ !
$ write scfg "#"
$ write scfg "# ''specific_cfg' "
$ write scfg "#     contructed from "
$ write scfg "#       ''test_case_spec'"
$ write scfg "#"
$ !
$ cfg_loop_start:
$	read/end_of_file=cfg_loop_end cfg cline
$	cline = f$edit(cline,"TRIM,COMPRESS")
$	cll   = f$length(cline)
$	if (cll.eq.0).or.f$extract(0,1,cline).eqs."#" then $ goto cfg_loop_start
$	c   = f$edit(f$element(1,";",cline),"TRIM")
$	if c.eqs.";"
$	   then cl00 = f$element( 0," ",cline)
$		cl01 = f$element( 1," ",cline)
$		cl02 = f$element( 2," ",cline)
$		cl03 = f$element( 3," ",cline)
$		cl04 = f$element( 4," ",cline)
$		cl05 = f$element( 5," ",cline)
$		cl06 = f$element( 6," ",cline)
$		cl07 = f$element( 7," ",cline)
$		cl08 = f$element( 8," ",cline)
$		cl09 = f$element( 9," ",cline)
$	   else cl00 = f$element( 0,";",cline)
$		cl01 = f$element( 0," ",c)
$		cl02 = f$element( 1," ",c)
$		cl03 = f$element( 2," ",c)
$		cl04 = f$element( 3," ",c)
$		cl05 = f$element( 4," ",c)
$		cl06 = f$element( 5," ",c)
$		cl07 = f$element( 6," ",c)
$		cl08 = f$element( 7," ",c)
$		cl09 = f$element( 8," ",c)
$	endif
$	if is_cfg
$	   then if have_rep_opt then $ cl02 = "''rep_opt'"
$		if have_usr_opt
$		   then cl09 = achilles_user
$		   else call check_user "''cl09'" "get_out"
$			if get_out.ne.1
$			   then call wr_message -
				  "Bad user <''cl09'> in cfg file" "get_out"
$			   else get_out == 0
$			endif
$		endif
$		new_cline = cl00 + " ; " + cl01 + " " + cl02 + " " -
			  + cl03 + " "   + cl04 + " " + cl05 + " " -
			  + cl06 + " "   + cl07 + " " + cl08 + " " -
			  + cl09
$	   else new_cline = cl00 + " <DBNAME> " + cl01 + " " -
			  + cl02 + " <CHILDNUM> " + cl03 + " " -
			  + cl04 + " ; " + cl05 + " ''rep_opt' " -
			  + cl06 + " "
$		new_cline = new_cline + cl07 + " " + cl08 + " " -
			  + cl09 + " <NONE> " +  hostname -
			  + "''itera'" + pid +  " " + achilles_user
$	endif
$	write scfg f$edit(new_cline,"TRIM,COMPRESS")
$	!
$ goto cfg_loop_start
$ cfg_loop_end:
$ close cfg
$ close scfg
$ !
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ check_db_spec:
$ !*********************************************************************!
$ !
$ vn_is = 0
$ db_is = 0
$ sv_is = 0
$ vns   = ""
$ dbs   = ""
$ svs   = ""
$ dbspc = ""
$ if nr_of_p.eq.1 then $ goto no_db_spec
$ dbs = f$edit(p2,"UPCASE")
$ dbspc = dbs
$ dbl = f$length(dbs)
$ vn  = f$locate("::",dbs)
$ if vn.lt.dbl
$  then vns = f$extract(0,vn,dbs)
$	dbs = f$extract(vn+2,dbl-(vn+2),dbs)
$ endif
$ svs = f$element(1,"/",dbs)
$ if svs.eqs."/" then $ svs = ""
$ dbs = f$element(0,"/",dbs)
$ !
$ if vns.nes."" then $ vn_is = 1
$ if dbs.nes."" then $ db_is = 1
$ if svs.nes."" then $ sv_is = 1
$ !
$ no_db_spec:
$ !
$ if db_is.eq.0
$  then call wr_message "No Database specified..." "get_out"
$ else if vn_is
$  then gosub look_for_gcc
$	if gcc_svr.eq.0
$	   then call wr_message "Comm Server not running..." "get_out"
$	endif
$ else if sv_is.eq.0
$  then gosub look_for_dbms
$	if dbms_svr.eq.0
$	   then call wr_message "Ingres Server not running..." "get_out"
$	endif
$ else if svs.eqs."STAR"
$  then gosub look_for_star
$	if star_svr.eq.0
$	   then call wr_message "Ingres Star Server not running..." "get_out"
$	endif
$ else if svs.eqs."RMS"
$  then gosub look_for_rms
$	if rms_svr.eq.0
$	   then call wr_message "Ingres RMS Server not running..." "get_out"
$	endif
$ else if svs.eqs."RDB"
$  then gosub look_for_rdb
$	if rdb_svr.eq.0
$	   then call wr_message "Ingres RDB Server not running..." "get_out"
$	endif
$  else
$	call wr_message "Not setup for ''svs' yet." "get_out"
$ endif
$ endif
$ endif
$ endif
$ endif
$ endif
$ !
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ confirm_output_dir:
$ !*********************************************************************!
$ y = 1
$ !
$ if have_res_opt.eq.0.and.f$trnlnm("achilles_result").nes.""
$  then res_opt == f$trnlnm("achilles_result")
$	have_res_opt == 1
$ endif
$ !
$ ask_if_we_need_to:
$ !
$ if have_res_opt.eq.0
$  then if y.eq.1 then $ wro "!  You have not specified an output directory."
$	call wr_message "By default I'll use ''def_dir' for output."
$	inquire x "!  Output Directory  <''def_dir'> "
$	call wr_message ""
$	if x.eqs.""
$	   then res_opt == def_dir
$	   else res_opt == f$parse(x,,,"DEVICE",   "SYNTAX_ONLY")-
			 + f$parse(x,,,"DIRECTORY","SYNTAX_ONLY")
$	endif
$	have_res_opt == 1
$ endif
$ !
$ see_if_it_exists:
$ !
$ r1 = f$parse(res_opt,,,"DEVICE",   "SYNTAX_ONLY,NO_CONCEAL")
$ r2 = f$parse(res_opt,,,"DIRECTORY","SYNTAX_ONLY,NO_CONCEAL")
$ !
$ call condns_dir "''r1'''r2'" "res_opt"
$ !
$ if f$parse("''res_opt'*.*;*").eqs.""
$  then
$	wro "!  ''res_opt', Does not exist."
$	inquire/local xx "!  Create it? <Y> "
$	xx = f$extract(0,1,f$edit(xx,"UPCASE,TRIM,COMPRESS"))
$	if xx.eqs."Y".or.xx.eqs.""
$	   then call wr_message "Creating ''res_opt'"
$		create/dir 'res_opt'
$	else if xx.eqs."N"
$	   then y = 0
$		have_res_opt == 0
$		res_opt == ""
$		goto ask_if_we_need_to
$	   else call wr_message " Bad Answer. YES or NO (<CR>=Y) " "get_out"
$		get_out == 0
$		goto see_if_it_exists
$	endif
$	endif
$ endif
$ !
$ call wr_message "Output Directory  = ''res_opt'"
$ !
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ find_test_case: subroutine
$ !
$ nx = p1
$ gosub look_for_it
$ if nx.nes."" then $ goto end_find_test_case
$ nx = p1
$ !
$ x  = f$parse(nx,junk_path,,,"SYNTAX_ONLY")
$ if f$locate("JUNK",x).eq.f$length(x) then $ goto end_find_test_case
$ !
$ gosub set_junk
$ !
$ if xty_is then $ xty = ".*"
$ !
$ if xna_is then $ goto end_find_test_case
$ !
$ if xde_is.and.xdi_is
$  then
$	if have_cfg_opt
$	   then xde = cfg_opt
$	   else xde = "ACHILLES_CONFIG:"
$	endif
$	xdi = ""
$	xdi_is = 0
$	nx = xde + xdi + xna + xty + xve
$	gosub look_for_it
$	if nx.eqs.""
$	   then
$		xde = ""
$		xde_is = 0
$	endif
$  else
$	if xde_is
$	   then
$		xde = ""
$		xde_is = 0
$	endif
$ endif
$ !
$ nx = xde + xdi + xna + xty + xve
$ gosub look_for_it
$ if nx.nes."" then $ goto end_find_test_case
$ !
$ if xty_is then $ xty = ".CONFIG"
$ nx = xde + xdi + xna + xty + xve
$ gosub look_for_it
$ if nx.nes."" then $ goto end_find_test_case
$ !
$ if xty_is then $ xty = "."
$ nx = xde + xdi + xna + xty + xve
$ gosub look_for_it
$ !
$ !*********end**********!
$ goto end_find_test_case
$ !*********end**********!
$ look_for_it:
$ ! if f$type(ii).eqs.""
$ !  then ii = 1
$ !  else ii = ii + 1
$ ! endif
$ nx = f$search(nx)
$ ! wro "''ii') nx = ''nx'"
$ return
$ !
$ set_junk:
$ xde_is = 0
$ xdi_is = 0
$ xna_is = 0
$ xty_is = 0
$ xve_is = 0
$ xde    = f$parse(nx,junk_path,,"DEVICE",   "SYNTAX_ONLY")
$ xdi    = f$parse(nx,junk_path,,"DIRECTORY","SYNTAX_ONLY")
$ xna    = f$parse(nx,junk_path,,"NAME",     "SYNTAX_ONLY")
$ xty    = f$parse(nx,junk_path,,"TYPE",     "SYNTAX_ONLY")
$ xve    = f$parse(nx,junk_path,,"VERSION",  "SYNTAX_ONLY")
$ if xde.eqs.f$parse(junk_path,,,"DEVICE",   "SYNTAX_ONLY") then $ xde_is = 1
$ if xdi.eqs.f$parse(junk_path,,,"DIRECTORY","SYNTAX_ONLY") then $ xdi_is = 1
$ if xna.eqs.f$parse(junk_path,,,"NAME",     "SYNTAX_ONLY") then $ xna_is = 1
$ if xty.eqs.f$parse(junk_path,,,"TYPE",     "SYNTAX_ONLY") then $ xty_is = 1
$ if xve.eqs.f$parse(junk_path,,,"VERSION",  "SYNTAX_ONLY") then $ xve_is = 1
$ return
$ !*********end**********!
$ end_find_test_case:
$	if f$parse(nx,,,"TYPE","SYNTAX_ONLY").eqs.".CONFIG"
$	   then is_cfg == 1
$	   else is_cfg == 0
$	endif
$	'p2' == nx
$ exit
$ !*********end**********!
$ endsubroutine
$ !*********************************************************************!

$ !**********************************************************************!
$ condns_dir: subroutine
$ c_ver     = f$verify(0)
$ !**********************************************************************!
$ !
$ !	This procedure takes a directory specification and checks it
$ !	for and removes various constructs:
$ !
$ !	1)	".]["		as in		dev:[root.][dir]
$ !	2)	"000000."	as in		dev:[000000.dir]
$ !				   or		dev:[root.][000000.dir]
$ !
$ !	It will leave the construct:		dev:[000000]
$ !
$ !********************!
$ !	Parameters     !
$ !********************!
$ !
$ !	P1	-	The Directory specification.
$ !
$ !			If P1 is blank, procedure uses default dir.
$ !
$ !	P2	-	The name to use for a global symbol to return
$ !			condensed structure to calling procedure
$ !		Ex:
$ !			$ @ing_tool:condns_dir dev:[root.][dir1.dir2] tmp_sym
$ !			$ type 'tmp_sym'file.ext
$ !			$ delete/sym/glo tmp_sym
$ !
$ !			If P2 is blank, procedure outputs condensed structure
$ !			to sys$output.
$ !
$ !**********************************************************************!
$ c_spec    = "''p1'"
$ c_spec_l  = f$length(c_spec)
$ !
$ if f$extract(c_spec_l-2,2,c_spec).eqs.".]"
$  then is_root = 1
$	c_spec  = f$extract(0,c_spec_l-2,c_spec)+"]"
$  else is_root = 0
$ endif
$ !
$ return_ = p2
$ x = "junk::junk:[junk]junk.junk;32767"
$ c_node = f$parse(c_spec,x,,"NODE","SYNTAX_ONLY")
$ c_name = f$parse(c_spec,x,,"NAME","SYNTAX_ONLY")
$ c_type = f$parse(c_spec,x,,"TYPE","SYNTAX_ONLY")
$ c_vers = f$parse(c_spec,x,,"VERSION","SYNTAX_ONLY")
$ !
$ if c_node.eqs."JUNK::" then $ c_node = ""
$ if c_name.eqs."JUNK"   then $ c_name = ""
$ if c_type.eqs.".JUNK"  then $ c_type = ""
$ if c_vers.eqs.";32767" then $ c_vers = ""
$ !
$ c_dev     = f$parse(c_spec,          ,,"DEVICE",   "SYNTAX_ONLY")
$ c_dir     = f$parse(c_spec,"''c_dev'",,"DIRECTORY","SYNTAX_ONLY")
$ c_l       = f$length(c_dir)
$ c_dir     = f$extract(1,c_l-2,c_dir)
$ c_tmp     = ""
$ i         = 0
$ found_mfd = 0
$ start_cond_loop:
$	!
$	el_i = f$element(i,".",c_dir)
$	if "''el_i'".eqs."." then $ goto end_cond_loop
$	!
$	if f$extract(0,2,el_i).eqs."][" -
	   then $ el_i = f$extract(2,f$length(el_i)-2,el_i)
$	!
$	i = i + 1
$	if el_i.eqs."000000"
$	   then found_mfd = 1
$		goto start_cond_loop
$	endif
$	!
$	if f$length(c_tmp).gt.0 then $ c_tmp = c_tmp + "."
$	!
$	c_tmp = c_tmp + el_i
$	!
$ goto start_cond_loop
$ end_cond_loop:
$ !
$ if f$length(c_tmp).ne.0
$  then c_dir = c_tmp
$  else if found_mfd then $ c_dir = "000000"
$ endif
$ !
$ if c_dir.nes.""
$  then c_dir = "["+c_dir
$	if is_root.eq.1 then $ c_dir = c_dir+"."
$	c_dir = c_dir+"]"
$ endif
$ !
$ r_spec = c_node+c_dev+c_dir+c_name+c_type+c_vers
$ !
$ if f$type(return_).nes."STRING"
$  then wro r_spec
$  else if return_.eqs.""
$	   then wro r_spec
$	   else 'return_' == r_spec
$	endif
$ endif
$ !
$ c_ver = f$verify(c_ver)
$ exit
$ endsubroutine
$ !*********************************************************************!

$ !*********************************************************************!
$ check_user: subroutine
$ !*********************************************************************!
$ !
$ set noon
$ x = f$edit(p1,"UPCASE")
$ assign/user nl: sys$error
$ assign/user nl: sys$output
$ valid_user "''x'"
$ 'p2' == $status
$ set on
$ !
$ exit
$ endsubroutine
$ !*********************************************************************!

$ !*********************************************************************!
$ wr_message: subroutine
$ !*********************************************************************!
$ if p2.nes.""
$  then 'p2' == 1
$	err_or = 1
$	p1 = f$edit(p1,"UPCASE")
$  else err_or = 0
$ endif
$ if err_or
$  then if p1.nes.""
$	   then wro ""
$		wro "!!!!!!!!!!!! ERROR !!!!!!!!!!! "
$		wro "!"
$		wro "!  ''p1'"
$		wro "!"
$		wro "!!!!!!!!!!!! ERROR !!!!!!!!!!! "
$		wro ""
$	endif
$  else if p1.nes.""
$	   then p1 = "!  ''p1'"
$		wr_m_do_it:
$		if f$length(p1).gt.77
$		   then err_or = f$extract(0,77,p1)
$			wro err_or
$			p1 = "!  "+f$extract(77,f$length(p1)-77,p1)
$			goto wr_m_do_it
$		   else wro p1
$		endif
$	endif
$	wro "!-----------------------------------------------------------------------------"
$ endif
$ !
$ exit
$ endsubroutine
$ !*********************************************************************!

$ !*********************************************************************!
$ look_for_batch_que:
$ !*********************************************************************!
$ !set ver
$ recheck_que  = 1
$ start_looking_for_que:
$ !
$ ach_bat = f$trnlnm("ACHILLES$BATCH")
$ !
$ if ach_bat.eqs.""
$  then diff_ach_bat = 0
$	ach_bat = "ACHILLES$BATCH"
$	call wr_message "ACHILLES$BATCH not defined. Will look for que by that name"
$  else call wr_message "ACHILLES$BATCH defined to be ''ach_bat'..."
$	diff_ach_bat = 1
$ endif
$ !
$ found_ach_bat = 0
$ temp = f$getqui("")
$ qloop:
$	qname = f$getqui("DISPLAY_QUEUE","QUEUE_NAME","''ach_bat'*","BATCH")
$	if qname.eqs."" then $ goto qloop_end
$	if qname.nes.ach_bat then $ goto qloop
$	found_ach_bat = 1
$	scsnode = f$getqui("DISPLAY_QUEUE","SCSNODE_NAME",qname,"FREEZE_CONTEXT")
$	!
$ qloop_end:
$ !
$ if found_ach_bat
$  then if diff_ach_bat
$	   then wro "!  found achilles$batch (''qname') on ''scsnode'."
$	   else wro "!  found ''qname' on ''scsnode'."
$	endif
$	if hostname.nes.scsnode
$	   then wro "!  this que is not running on this node... "
$		not_this_node = 1
$		ach_bat = ""
$	   else not_this_node = 0
$	endif
$  else if diff_ach_bat
$	   then wro "!  couldn't find achilles$batch (''ach_bat')"
$	   else wro "!  couldn't find ''ach_bat'"
$	endif
$	ach_bat = ""
$ endif
$ call wr_message ""
$ !
$ ask_about_batch:
$ tmp1 = ""
$ !
$ if .not.((found_ach_bat.ne.0).and.(not_this_node.eq.0))
$  then if ach_bat.eqs.""
$	   then call wr_message "Please enter the correct batch que."
$		inquire tmp1 "!                >"
$	   else if recheck_que.eq.0
$		   then call wr_message "Please enter the correct batch que or take the default if correct."
$			inquire tmp1 "!     <''ach_bat'>"
$		endif
$	endif
$	if tmp1.nes.""
$	   then tmp1 = f$edit(tmp1,"TRIM,COMPRESS,UPCASE")
$		if tmp1.nes.ach_bat
$		   then define_ach_str == "define/job/exec achilles$batch ''tmp1'"
$			call wr_message "''define_ach_str'"
$			'define_ach_str'
$		endif
$		if recheck_que
$		   then recheck_que  = 0
$			goto start_looking_for_que
$		endif
$	   else if ach_bat.eqs.""
$		   then call wr_message "No Batch que given" "get_out"
$		   else call wr_message "Here we Go!!!!!"
$		endif
$	endif
$ endif
$ !
$ x = f$verify(test)
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ create_spec_submit:
$ !*********************************************************************!
$ !
$ open/write ssub 'specific_submit'
$ on control_y then $ goto end_ssub
$ on error     then $ goto end_ssub
$ !
$ write ssub "$ !"
$ write ssub "$ set verify"
$ write ssub "$ !"
$ write ssub "$ ! ''specific_submit'"
$ write ssub "$ !"
$ write ssub "$ set proc/priv=(all,nobypass)"
$ write ssub "$ !"
$ write ssub "$ define /job II_SYSTEM ''ii_sys_def'"
$ if stdenv_spec.nes.""
$  then write ssub "$ @''stdenv_spec' -v"
$	write ssub "$ !"
$ endif
$ ! changed below to insysdef for 6.5 - jeffr
$ ! SYMBOLS FOR ACHILLES ARE HERE
$ write ssub "$ set proc
$ write ssub "$ !"
$ write ssub "$ ''define_ach_str'"
$ write ssub "$ @''ii_sys_def'[ingres]ingsysdef.com/out=nl:"
$ write ssub "$ @''ii_sys_def'[ingres.utility]iisymboldef.com/out=nl:"
$ write ssub "$ iiversion"
$ write ssub "$ if sys_defined.eq.0 then $ exit"
$ write ssub "$ !"
$ write ssub "$ if f$mode().eqs.""BATCH"" then $ set verify"
$ write ssub "$ !"
$ write ssub "$ achilles -e ''specific_ini' -"
$ write ssub "           -d ''dbspc' -"
$ write ssub "           -f ''specific_cfg' -"
$ write ssub "           -l  -"
$ write ssub "           -o ''res_opt' -"
$ write ssub "           -a ''act_opt'"
$ write ssub "$ exit"
$ !
$ end_ssub:
$ !
$ close ssub
$ !
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ create_spec_ini:
$ !*********************************************************************!
$ !
$ open/write sini 'specific_ini'
$ on control_y then $ goto end_sini
$ on error     then $ goto end_sini
$ !
$ !
$ write sini "$ set noon"
$ write sini "$ set verify "
$ write sini "$ !"
$ write sini "$ ! ''specific_ini'  "
$ write sini "$ !"
$ write sini "$ ! Define the Achilles environment before starting up the tests"
$ write sini "$ !"
$ write sini "$ set noverify"
$ write sini "$ !"
$ write sini "$ set proc/priv=(all,nobypass)"
$ write sini "$ define /job II_SYSTEM ''ii_sys_def'" 
$ write sini "$ !"
$ if f$type(stdenv).eqs."STRING"
$  then x = f$trnlnm("ing_tool")
$	x = f$parse(x,,,"DEVICE",   "NO_CONCEAL")-
	  + f$parse(x,,,"DIRECTORY","NO_CONCEAL")
$	call condns_dir "''x'" "stdenv_spec"
$	stdenv_spec == stdenv_spec + "stdenv.com"
$	if f$search(stdenv_spec).nes.""
$	   then write sini "$ @''stdenv_spec' -v"
$		write sini "$ !"
$	   else stdenv_spec == ""
$	endif
$ endif
$ if have_env_opt
$  then if f$extract(0,1,env_opt).eqs."@"
$	   then write sini "$ ''env_opt'"
$	   else open/read  senv 'env_opt'
$		env_loop_start:
$			read/end_of_file=env_loop_end senv sline
$			x = f$edit(sline,"TRIM,COMPRESS,UPCASE")
$			if x.eqs."$ EXIT" then $ goto env_loop_end
$			write sini sline
$		goto env_loop_start
$		env_loop_end:
$		close senv
$	endif
$	write sini "$ !"
$ endif
$ write sini "$ @''ii_sys_def'[ingres]ingsysdef.com/out=nl:"
$ write sini "$ @''ii_sys_def'[ingres.utility]iisymboldef.com/out=nl:"
$ write sini "$ !"
$ write sini "$ !"
$ write sini "$ iiversion"
$ write sini "$ if sys_defined.eq.0 then $ exit"
$ !*********************************************!
$ !   Uncomment this line for pvlib v6.3 code
$ !*********************************************!
$ write sini "$ ! set_ach_log ts_bin"
$ !*********************************************!
$ write sini "$ !"
$ write sini "$ if f$mode().eqs.""BATCH"" then $ set verify"
$ write sini "$ show time"
$ write sini "$ !"
$ !
$ end_sini:
$ !
$ close sini
$ if f$trnlnm("senv"  ) then $ close senv
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ create_valid_user:
$ !*********************************************************************!
$ !
$ open/write vusr valid_user.c
$ on control_y then $ goto end_vusr
$ on error     then $ goto end_vusr
$ ! changed to angle brackets for ALPHA - jeffr
$ write vusr "#include <stdio.h>"
$ write vusr "#include <ssdef.h>"
$ write vusr "#include <descrip.h>"
$ write vusr "#include <uaidef.h>"
$ write vusr "main (argc, argv)"
$ write vusr "int argc;"
$ write vusr "char *argv[];"
$ write vusr "{"
$ write vusr "    struct ITEM_LIST { unsigned short  len, code;"
$ write vusr "                       long            *buf,*rtn;"
$ write vusr "                     };"
$ write vusr "    char owner_buf [33], uname [13];"
$ write vusr "    int i, ret_val, owner_len;"
$ write vusr "    struct ITEM_LIST itm_1[] = { "
$ write vusr "                  32, UAI$_OWNER, owner_buf, &owner_len, 0, 0 };"
$ write vusr "    struct dsc$descriptor_s"
$ write vusr "           usr_name = { 12,DSC$K_DTYPE_T,DSC$K_CLASS_S, uname };"
$ write vusr "    if (argc != 2) exit(0);"
$ write vusr "    for ( i=0; argv[1][i]; i++ ) uname[i] = argv[1][i];"
$ write vusr "    for ( ; i<12; i++ ) uname[i] = ' ';"
$ write vusr "    uname[i] = '\0';"
$ write vusr "    exit(sys$getuai(0, 0, &usr_name, &itm_1, 0, 0, 0));"
$ write vusr "}"
$ !
$ end_vusr:
$ !
$ close vusr
$ !
$ set verify
$ cc valid_user
$ link valid_user,sys$library:vaxcrtl/lib
$ copy valid_user.exe ii_system:[tools.bin]valid_user.exe
$ delete valid_user.*;*
$ x = f$verify(test)
$ !
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ look_for_vnode:
$ !*********************************************************************!
$ !
$ netu >ac'pid'.tmp
N
S
P
*
*
*
*
S
G
*
*
*
*
E
E
$ in_priv = 0
$ in_glob = 0
$ open/read tmp ac'pid'.tmp
$ read_netu_start:
$	read/end_of_file=read_netu_end tmp nline
$	nline = f$edit(nline,"TRIM,COMPRESS,UPCASE")
$	nl0 = f$element(0," ",nline)
$	nl1 = f$element(1," ",nline)
$	if (in_priv.or.in_glob).eq.0
$	   then if nl1.eqs."V_NODE"
$		   then if nl0.eqs."PRIVATE:" then $ in_priv = 1
$			if nl0.eqs."GLOBAL:"  then $ in_glob = 1
$		endif
$	   else if nline.eqs.""
$		   then in_priv = 0
$			in_glob = 0
$		   else if nl0.eqs.vns
$			   then if in_priv then $ x = "Private"
$				if in_glob then $ x = "Global"
$				nl2 = f$element(2," ",nline)
$				nl3 = f$element(3," ",nline)
$				goto read_netu_end
$			endif
$		endif
$	endif
$ goto read_netu_start
$ read_netu_end:
$ if (in_priv.or.in_glob).ne.0
$  then net_prot == nl1
$	rem_node == nl2
$	lis_addr == nl3
$	vn_type  == x
$  else get_out  == 1
$ endif
$ close tmp
$ delete ac'pid'.tmp;*
$ !
$ if get_out
$  then call wr_message "Can't find ''vns':: in Netu." "get_out"
$  else x = "Found ''vn_type' entry, ''vns'::, ''net_prot' to "-
	  + "''rem_node', ''lis_addr'"
$	call wr_message "''x'"
$ endif
$ !
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ try_connecting:
$ !*********************************************************************!
$ !
$ set noon
$ assign/user nl: sys$error  ! redirect errors to null for search.
$ assign/user nl: sys$output ! redirect tty output to null for search.
$ sql 'dbspc' >nl:
\q
$ if $status.ne.%X00000001 -
   then call wr_message "Could not access <''dbspc'> -- tests not submitted" "get_out"
$ !
$ set on
$ return
$ !*********************************************************************!
$ error_control:
$ !*********************************************************************!
$ !
$ on error        then $ goto end_it
$ on control_y    then $ goto end_it
$ if get_out.ne.0 then $ goto end_it
$ !
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ clean_up:
$ !*********************************************************************!
$ !
$ if f$type(valid_user     ).nes."" then $ delete/sym/glo valid_user
$ if f$type(junk_path      ).nes."" then $ delete/sym/glo junk_path
$ if f$type(test_case_spec ).nes."" then $ delete/sym/glo test_case_spec
$ if f$type(is_cfg         ).nes."" then $ delete/sym/glo is_cfg
$ if f$type(env_opt        ).nes."" then $ delete/sym/glo env_opt
$ if f$type(rep_opt        ).nes."" then $ delete/sym/glo rep_opt
$ if f$type(res_opt        ).nes."" then $ delete/sym/glo res_opt
$ if f$type(cfg_opt        ).nes."" then $ delete/sym/glo cfg_opt
$ if f$type(have_usr_opt   ).nes."" then $ delete/sym/glo have_usr_opt
$ if f$type(have_env_opt   ).nes."" then $ delete/sym/glo have_env_opt
$ if f$type(have_rep_opt   ).nes."" then $ delete/sym/glo have_rep_opt
$ if f$type(have_res_opt   ).nes."" then $ delete/sym/glo have_res_opt
$ if f$type(have_cfg_opt   ).nes."" then $ delete/sym/glo have_cfg_opt
$ if f$type(achilles_user  ).nes."" then $ delete/sym/glo achilles_user
$ if f$type(hostname       ).nes."" then $ delete/sym/glo hostname
$ if f$type(stdenv_spec    ).nes."" then $ delete/sym/glo stdenv_spec
$ if f$type(run_interactive).nes."" then $ delete/sym/glo run_interactive
$ if f$type(debug_on       ).nes."" then $ delete/sym/glo debug_on
$ if f$type(help_str       ).nes."" then $ delete/sym/glo help_str
$ if f$type(get_out        ).nes."" then $ delete/sym/glo get_out
$ if f$type(net_prot       ).nes."" then $ delete/sym/glo net_prot
$ if f$type(rem_node       ).nes."" then $ delete/sym/glo rem_node
$ if f$type(lis_addr       ).nes."" then $ delete/sym/glo lis_addr
$ if f$type(vn_type        ).nes."" then $ delete/sym/glo vn_type
$ if f$type(define_ach_str ).nes."" then $ delete/sym/glo define_ach_str
$ !
$ if f$trnlnm("cfg"  ) then $ close cfg
$ if f$trnlnm("scfg" ) then $ close scfg
$ if f$trnlnm("actmp") then $ close actmp
$ if f$trnlnm("sini" ) then $ close sini
$ if f$trnlnm("vusr" ) then $ close vusr
$ if f$trnlnm("senv" ) then $ close senv
$ if f$trnlnm("ssub" ) then $ close senv
$ !
$ if f$search("ac''pid'.tmp;*").nes."" then $ dele ac'pid'.tmp;*
$ return
$ !*********************************************************************!

$ !*********************************************************************!
$ initialize:
$ !*********************************************************************!
$ !
$ def_dir          = f$environment("DEFAULT")
$ get_out         == 0
$ if f$type(wro).nes."STRING" then $ wro := write sys$output
$ !
$ if f$search("ii_system:[tools.bin]valid_user.exe;*").eqs.""
$  then call wr_message ""
$	call wr_message "Need to Create Valid User Tool."
$	gosub create_valid_user
$ endif
$ !
$ valid_user	 :== $ii_system:[tools.bin]valid_user.exe
$ if f$type(wre).nes."STRING" then $ wre := write sys$error
$ help_str        == 0
$ junk_path       == "_JUNK:[JUNK]JUNK.JUNK"
$ pid              = f$extract(4,4,f$getjpi("","PID"))
$ itera		   = 0
$ rep_opt         == 0
$ have_rep_opt    == 0
$ act_opt	  == 0 
$ have_act_opt    == 0
$ env_opt         == ""
$ have_env_opt    == 0
$ res_opt         == ""
$ log_fil         = ""
$ have_res_opt    == 0
$ cfg_opt         == ""
$ have_cfg_opt    == 0
$ is_cfg          == 0
$ hostname        == f$getsyi("nodename")
$ have_usr_opt    == 0
$ achilles_user   == f$edit(f$getjpi("","USERNAME"),"TRIM")
$ specific_cfg     = ""
$ ii_sys_def      = f$trnlnm("II_SYSTEM")
$ stdenv_spec	  == ""
$ run_interactive == 0
$ debug_on	  == 0
$ net_prot        == ""
$ rem_node        == ""
$ lis_addr        == ""
$ vn_type         == ""
$ define_ach_str  == ""
$ !
$ return
$ !*********************************************************************!
$ exit
