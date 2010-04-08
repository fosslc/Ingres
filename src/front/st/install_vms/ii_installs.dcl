$ GOTO beginning !		II_INSTALLS.COM


		This routine helps the Ingres System Administrator
		keep track of Ingres installations on a given
		system by helping maintain the Ingres Installation
		history logfile:

		SYS$COMMON:[SYSEXE]INGRES_INSTALLATIONS.DAT

	Copyright (c) 1993, 2009  Ingres Corporation


	HIST_FILE is the current "history file"
	SCRATCH_FILE is where all updates occur
	FILE_IN_USE is the file from which displays happen.

$!!  History:
$!!	23-mar-93 (troy)
$!!	    Created.
$!!	07-feb-97 (boama01)
$!!	    * Bug 80613: REMOVE logic tries to collect split-up history by
$!!	      looking for 1 leading space; but single-digit days are stored
$!!	      w. a leading space (eg., " 1-jan-1997"). This caused BUFOVF
$!!	      and INVIFNEST errors.  Chgd logic to search for 2 spaces.
$!!	    * Allow case-insensitive matching of strings in REMOVE logic
$!!	      (with verification from user).
$!!	    * Add "?" help for REMOVE verb (after all, menu says its there)
$!!	    * ADD improperly formats date/time values; fixed parsing of
$!!	      F$TIME() value.  Also, release string was improperly handled
$!!	      in ADD logic, and not even written to new history rec!
$!!	    * Lookup release string in II_SYSTEM:[INGRES.INSTALL]KITINSTAL.
$!!	      DATA, instead of hard-coded obsolete "6.5/00".  New logic in
$!!	      "init" subroutine.
$!!	    * Would it hurt to have a few comments here and there?!  Added
$!!	      some as the need arose.
$!!	    * Corrected several messages.
$!!	23-mar-98 (kinte01)
$!!	    Update default version string
$!!	06-feb-2009 (joea)
$!!	    Update copyright. Remove unimplemented menu options. Change history
$!!	    comments so that they can be removed by preprocessing.
$ beginning:
$ GOSUB init
$ rec=p1
$ GOTO may_have_p1

$ prompt_loop:
$ rec=""
$ READ/END=user_done/PROMPT="II_INSTALLS> " sys$command rec
$ may_have_p1:
$ sub=""
$ IF "''rec'".EQS."" THEN GOTO prompt_loop	!CMD = up to first space
$ cmd=F$ELEMENT(0," ",F$EDIT(rec,"UPCASE,COMPRESS,UNCOMMENT,TRIM"))
$ IF F$LOCATE(cmd,cmd_list).EQ.F$LENGTH(cmd_list) THEN sub="help_user"
$ IF F$LOCATE(cmd,"ADD").EQ.0 THEN sub="add"
$ IF F$LOCATE(cmd,"DISCARD").EQ.0 THEN sub="discard"
$ IF F$LOCATE(cmd,"EXIT").EQ.0 THEN GOTO user_done
$ IF F$LOCATE(cmd,"HELP").EQ.0 THEN sub="help_user"
$ IF F$LOCATE(cmd,"QUIT").EQ.0 THEN GOTO user_done
$ IF F$LOCATE(cmd,"REMOVE").EQ.0 THEN sub="remove"
$ IF F$LOCATE(cmd,"SAVE").EQ.0 THEN sub="save"
$ IF F$LOCATE(cmd,"SHOW").EQ.0 THEN sub="display"
$ IF F$LOCATE(cmd,"USE").EQ.0 THEN sub="use_file"
$ IF "''sub'".NES."" THEN GOSUB 'sub
$ GOTO prompt_loop

$ help_user:
$ TYPE SYS$INPUT
			HELP for II_INSTALLS.COM

	II_INSTALLS.COM is used to facilitate maintenance of the INGRES
	Installation History Log file, INGRES_INSTALLATIONS.DAT, which is
	located in SYS$COMMON:[SYSEXE]. It has no bearing on the functionality
	of INGRES; it is provided for your convenience only.

Verbs:		ADD		Create a new history entry. Help available.
		DISCARD		Discard any changes.
		EXIT, QUIT	Changes will be lost if not SAVED!
		HELP		Displays this screen
		REMOVE		Delete an existing entry. Help available.
		SAVE		Keep any changes made
		SHOW		Displays all recorded history
		USE		Select an alternate history file

To obtain additional help, when available, enter ? as the first argument.

$ RETURN

$ remove:
$ IF "''F$EXTRACT(0,1,F$ELEMENT(1," ",rec))'".EQS."?"
$	THEN
$	TYPE sys$input
Usage:

   REMOVE match_string

	History records containing a match to the match_string will be removed.
	Match is attempted in exact case, then without case sensitivity.
	Confirmation is mandatory.

$	RETURN
$	ENDIF
$ IF .NOT.scratch
$	THEN
$	say "%INGRES-E-NOSCRATCH, No scratch file available. Updates not possible."
$	RETURN
$	ENDIF
$ IF dirty
$ THEN
$       say "%INGRES-W-NOSAVE, You must save any additions/changes before remove."
$       RETURN
$ ENDIF
$ rec=F$EXTRACT(F$LOCATE(" ",rec)+1,F$LENGTH(rec),rec) !remove remove cmd!
$ IF "''rec'".EQS.""
$	THEN
$	param_prompt="Match string : "
$	GOSUB get_param
$	IF "''param'".EQS.""
$		THEN
$		say "%INGRES-I-CANCEL, Remove operation canceled."
$		RETURN
$		ENDIF
$	rec=param
$	ENDIF
$! (We know the scratch_file is not "dirty" after checking above...)
$ DELETE/NOLOG 'scratch_file
$ scratch_two=scratch_file
$ removed=0
$ efile=file_in_use
$ err_msg="%INGRES-E-OPENERR, Error opening file for read."
$ OPEN/READ/ERROR=file_error h_file 'file_in_use
$ h_open=1
$ efile=scratch_two
$ err_msg="%INGRES-E-OPENERR, Error opening file for write."
$ OPEN/WRITE/ERROR=file_error s_file 'scratch_two
$ s_open=1
$! (Read and rewrite history header lines, up to & including underscore line)
$ r_find_underscores:
$ err_msg="%INGRES-E-READERR, File read error."
$ efile=file_in_use
$ READ/END=remove_done/ERROR=file_error h_file tstr
$ err_msg="%INGRES-E-READERR, File write error."
$ efile=scratch_two
$ WRITE/ERROR=file_error s_file tstr
$ IF F$LOCATE("___",tstr).EQ.F$LENGTH(tstr) THEN GOTO r_find_underscores
$! (The next history line should be simple CR separator line...)
$ err_msg="%INGRES-E-READERR, File read error."
$ efile=file_in_use
$ READ/END=r_last_set/ERROR=file_error h_file out_rec
$! (Branch here for each "set" of "split" history records (comprising 1
$!  history entry...))
$ r_next_set:
$ err_msg="%INGRES-E-READERR, File read error."
$ efile=file_in_use
$! (Branch here for each physical record in history file...)
$ r_next_rec:
$ READ/END=r_last_set/ERROR=file_error h_file in_rec
$! (Eat all "simple CR" records after the first...)
$ IF F$LENGTH(in_rec).EQ.0 THEN GOTO r_next_rec
$! (If in_rec begins w. at least 2 spaces, it must be a "split" within current
$!  set; collect all pieces in the "set"...)
$ IF "''F$EXTRACT(0,2,in_rec)'".EQS."  "
$	THEN
$	out_rec=out_rec+F$EDIT(in_rec,"TRIM") !remove leading spaces
$	GOTO r_next_rec
$	ENDIF
$ IF F$LENGTH(out_rec).EQ.0  !(should only occur first time, on simple CR rec)
$	THEN
$	out_rec=in_rec
$	GOTO r_next_rec
$	ENDIF
$! (Try to match string within out_rec exactly...)
$ IF F$LOCATE(rec,out_rec).EQ.F$LENGTH(out_rec)
$	THEN
$! (Try to match string without regard to case...)
$	IF F$LOCATE(F$EDIT(rec,"UPCASE"),F$EDIT(out_rec,"UPCASE")).EQ.F$LENGTH(out_rec)
$		THEN
$		GOSUB write_rec
$		out_rec=in_rec
$		GOTO r_next_set
$		ENDIF
$	say "%INGRES-W-CASEMATCH, match found with different cases."
$	ENDIF
$ GOSUB rem_match_found
$ GOTO r_next_set

$! (Try to match string within the last "set", in same or different case...)
$ r_last_set:
$ IF F$LOCATE(rec,out_rec).EQ.F$LENGTH(out_rec)
$	THEN
$	IF F$LOCATE(F$EDIT(rec,"UPCASE"),F$EDIT(out_rec,"UPCASE")).EQ.F$LENGTH(out_rec)
$		THEN
$		GOSUB write_rec
$		ELSE
$		say "%INGRES-W-CASEMATCH, match found with different cases."
$		GOSUB rem_match_found
$		ENDIF
$	ELSE
$	GOSUB rem_match_found
$	ENDIF
$ CLOSE/ERROR=fatal_file_error h_file
$ h_open=0
$ CLOSE/ERROR=fatal_file_error s_file
$ s_open=0
$ IF removed
$	THEN
$	IF dirty	! means 3 files exist..., but "file in use" was scratch
$		THEN
$		DELETE/NOLOG 'file_in_use
$		COPY/NOLOG 'scratch_two 'file_in_use
$		DELETE/NOLOG 'scratch_two
$		ELSE 				! only 2 files...
$		dirty=1 ! we're dirty now!
$		file_in_use=scratch_file
$		ENDIF
$	ELSE
$	say "%INGRES-I-NOCHANGE, Nothing was removed."
$	DELETE 'scratch_two
$	IF .NOT.dirty THEN COPY/NOLOG 'file_in_use 'scratch_file
$	ENDIF
$ scratch_file=F$SEARCH(scratch_file)
$ file_in_use=F$SEARCH(file_in_use)
$ RETURN

$ rem_match_found:
$ say "The following match was found: "
$! (Cut a long out-rec back into "pieces" just for display...)
$ rem_cut_rec:
$ IF F$LENGTH(out_rec).GT.80
$	THEN
$	tstr=F$EXTRACT(79,F$LENGTH(out_rec),out_rec)
$	out_rec=F$EXTRACT(0,79,out_rec)
$	WRITE sys$output out_rec
$	out_rec="                        "+tstr
$	GOTO rem_cut_rec
$	ELSE
$	WRITE sys$output out_rec
$	ENDIF
$ READ/END=not_removed/PROMPT="Please confirm removal (Yes/No) [NO] :" sys$command tstr
$ IF tstr
$	THEN
$	say "%INGRES-S-REMOVED, Entry removed."
$	removed=1
$	ELSE
$	say "%INGRES-I-NOTREMOVED, Entry will not be removed."
$	GOSUB write_rec
$	ENDIF
$ out_rec=in_rec
$ RETURN

$ write_rec:
$ err_msg="%INGRES-E-READERR, File write error."
$ efile=scratch_two
$! (Cut a long out-rec into a set of "pieces" so it fits on screens...)
$ cut_out_rec:
$ IF F$LENGTH(out_rec).GT.80
$	THEN
$	tstr=F$EXTRACT(79,F$LENGTH(out_rec),out_rec)
$	out_rec=F$EXTRACT(0,79,out_rec)
$	WRITE/ERROR=file_error s_file out_rec
$	out_rec="                        "+tstr
$	GOTO cut_out_rec
$	ELSE
$	WRITE/ERROR=file_error s_file out_rec
$	ENDIF
$ RETURN

$ discard:
$ IF dirty
$	THEN
$	tstr=""
$	READ/END=no_discard/PROMPT="Please confirm discard of changes (Yes/No) [NO] :" sys$command tstr
$	no_discard:
$	IF (.NOT.tstr).OR.("''tstr'".EQS."")
$		THEN
$		say "%INGRES-I-NOTDISCARDED, Changes were not discarded."
$		RETURN
$		ENDIF
$	file_in_use=hist_file
$	DELETE/NOLOG 'scratch_file
$	say "%INGRES-S-DISCARDED, Changes discarded."
$	dirty=0
$	ELSE
$	say "%INGRES-I-NOCHANGES, There were no changes to discard."
$	IF .NOT.scratch THEN RETURN
$	say "%INGRES-I-REFRESH, Refreshing scratch file from history file."
$	ENDIF
$ scratch_file="sys$login:scratch_file.''F$GETJPI("","PID")'"
$ IF "''F$SEARCH(scratch_file)'".NES."" THEN DELETE/NOLOG 'F$SEARCH(scratch_file)
$ IF "''F$SEARCH(scratch_file)'".EQS."" THEN COPY/NOLOG 'hist_file 'scratch_file
$ IF "''F$SEARCH(scratch_file)'".EQS.""
$	THEN
$	err_msg="%INGRES-W-NOSCRATCH, Unable to create scratch file."
$	efile=""
$	scratch=0
$	GOSUB rpt_file_error
$	say "%INGRES-I-CONTINUE, Attempting to continue anyway. . ."
$	ELSE
$	scratch=1
$	scratch_file=F$SEARCH(scratch_file)
$	ENDIF
$ RETURN

$ save:
$ IF .NOT.dirty
$	THEN
$	say "%INGRES-I-NOTDIRTY, You have no changes to save."
$	RETURN
$	ENDIF
$ scratch_file=F$EXTRACT(0,F$LOCATE(";",scratch_file),scratch_file)
$ hist_file=F$EXTRACT(0,F$LOCATE(";",hist_file),hist_file)
$ COPY/NOLOG 'scratch_file 'hist_file
$ s=$status
$ scratch_file=F$SEARCH(scratch_file)
$ hist_file=F$SEARCH(hist_file)
$ file_in_use=hist_file
$ IF ("''s'".EQS."%X00000001").OR.("''s'".EQS."%X10030001")
$	THEN
$	say "%INGRES-S-SAVED, Changes saved."
$	dirty=0
$	ELSE
$	say "%INGRES-F-NOTSAVED, Changes were not saved."
$	say "%INGRES-I-STATUS, Status from COPY was : ''s'"
$	ENDIF
$ RETURN

$ add:
$ IF "''F$EXTRACT(0,1,F$ELEMENT(1," ",rec))'".EQS."?"
$	THEN
$	TYPE sys$input
Usage:

   ADD date time major_release location uic packages [comments]

	All arguments are delimited by space[s] or tabs.
	Lines may wrap.
	ADD prompts for missing arguments.
	ADD does NOT validate arguments.
	Confirmation is mandatory.

$	RETURN
$	ENDIF
$ IF .NOT.scratch
$	THEN
$	say "%INGRES-E-NOSCRATCH, No scratch file available. Updates not possible."
$	RETURN
$	ENDIF
$ err_msg="%INGRES-E-APPENDERR, Error opening for append."
$ efile=scratch_file
$ rec=F$EDIT(rec,"COMPRESS")
$ OPEN/ERROR=file_error/APPEND s_file 'scratch_file
$ s_open=1
$ date=F$ELEMENT(1," ",rec)
$ time=F$ELEMENT(2," ",rec)
$ new_rel=F$ELEMENT(3," ",rec)+" "+F$ELEMENT(4," ",rec)
$ loc=F$ELEMENT(5," ",rec)
$ uic=F$ELEMENT(6," ",rec)
$ pkgs=F$ELEMENT(7," ",rec)
$ comments=" "
$ IF "''pkgs'".NES." " THEN comments=F$EXTRACT(F$LOCATE(pkgs,rec)+F$LENGTH(pkgs),F$LENGTH(rec),rec)
$ say ""
$ IF "''date'".EQS." "
$	THEN
$	param_prompt="Date [''F$ELEMENT(0," ",F$EDIT(F$TIME(),"TRIM"))']: "
$	GOSUB get_param
$	IF "''param'".eqs."" then param=F$ELEMENT(0," ",F$EDIT(F$TIME(),"TRIM"))
$	date=param
$! (A neatness check to line up 1-digit and 2-digit dates...)
$	IF F$EXTRACT(0,1,date).NES." ".AND.F$EXTRACT(1,1,date).EQS."-"-
	  THEN date=" "+date
$	ENDIF
$ IF "''time'".EQS." "
$	THEN
$	param_prompt="Time [''F$ELEMENT(1," ",F$EDIT(F$TIME(),"TRIM"))']: "
$	GOSUB get_param
$	IF "''param'".eqs."" then param=F$ELEMENT(1," ",F$EDIT(F$TIME(),"TRIM"))
$	time=param
$	ENDIF
$ IF "''F$EDIT(new_rel,"COMPRESS")'".EQS." "
$	THEN
$	param_prompt="Release [''release'] "
$	GOSUB get_param
$	IF "''param'".eqs."" then param=release
$	new_rel=param
$	ENDIF
$ IF "''loc'".EQS." "
$	THEN
$	param_prompt="Location [''F$TRNLNM("II_SYSTEM")']: "
$	GOSUB get_param
$	IF "''param'".eqs.""  then param=F$TRNLNM("II_SYSTEM")
$	loc=param
$	ENDIF
$ IF "''uic'".EQS." "
$	THEN
$	param_prompt="UIC [ ''F$GETJPI("","UIC")' ]: "
$	GOSUB get_param
$	IF "''param'".eqs.""  then param=F$GETJPI("","UIC")
$	uic=param
$	ENDIF
$ IF "''pkgs'".EQS." "
$	THEN
$	param_prompt="Packages [All]: "
$	GOSUB get_param
$	IF "''param'".eqs."" then param="All packages"
$	pkgs=param
$	ENDIF
$ IF "''comments'".EQS." "
$	THEN
$	param_prompt="Comments : "
$	GOSUB get_param
$	comments=param
$	IF ("''comments'".NES."").AND.(F$EXTRACT(0,1,F$EDIT(comments,"COLLAPSE")).NES."!") THEN comments="! "+comments
$	ENDIF
$ rec=date+" "+time+" "+new_rel+"  "+uic+"  "+loc+" "+pkgs+" "+comments
$ err_msg=""
$ say cr+lf+"ADD record : ''rec'"
$ tstr="Yes"
$ READ/END=add_done/ERROR=fatal_file_error/PROMPT="Please confirm (Y/N) [''tstr'] " sys$command tstr
$ IF "''tstr'".EQS."" THEN tstr="TRUE"
$ err_msg="%INGRES-E-APPENDERR, Error appending to file."
$ IF tstr
$	THEN
$	dirty=1
$	cut_hist_rec:
$	IF F$LENGTH(rec).GT.80
$		THEN
$		tstr=F$EXTRACT(79,F$LENGTH(rec),rec)
$		rec=F$EXTRACT(0,79,rec)
$		WRITE/ERROR=file_error s_file rec
$		rec="                        "+tstr
$		GOTO cut_hist_rec
$		ELSE
$		WRITE/ERROR=file_error s_file rec
$		ENDIF
$	file_in_use=scratch_file
$	say "%INGRES-S-ADDED, History record added."
$	ENDIF
$ add_done:
$ err_msg="%INGRES-E-CLOSERR, Error closing file."
$ CLOSE/ERROR=file_error s_file
$ s_open=0
$ RETURN

$ display:
$ efile=file_in_use
$ err_msg="%INGRES-E-OPENERR, File open error."
$ OPEN/READ/ERROR=file_error h_file 'file_in_use
$ h_open=1
$ err_msg="%INGRES-E-READERR, File read error."
$! (Read history header lines, up to & including underscore line)
$ d_find_underscores:
$ READ/END=display_done/ERROR=file_error h_file tstr
$ IF F$LOCATE("___",tstr).EQ.F$LENGTH(tstr) THEN GOTO d_find_underscores
$ say "%INGRES-I-HISTORY, Using file: ''file_in_use'"
$ TYPE SYS$INPUT
	Date			Release		Owner		Location	Package list
________________________________________________________________________________
$ t=2
$! (Display up to 21 lines per screen)
$ disp_hist_file:
$ READ/END=display_done/ERROR=file_error h_file tstr
$ t=t+1
$ say "''tstr'"
$ IF t.EQ.21
$	THEN
$	GOSUB press_return
$	t=1  !(Allows last rec from last screen to carry over to new screen)
$	ENDIF
$ GOTO disp_hist_file

$ display_done:
$ say "________________________________________________________________________________"+cr+lf
$ err_msg="%INGRES-E-CLOSERR, File close error."
$ CLOSE/ERROR=file_error h_file
$ h_open=0
$ efile=""
$ RETURN

________________________________________________________________________________

$ init:
$ ON ERROR THEN GOTO forced_exit
$ ON control_y THEN GOTO ctl_y
$ tried_save=0	! prevents looping of error exits
$ dirty=0 ! T = delta of scratch_file to hist_file, file_in_use=scratch_file
$ s_open=0	! scratch_file is open, needs close (aka sfile)
$ h_open=0	! hist_file is open, needs close (aka hfile)
$ scratch=0	! no scratch file available - no updates possible
$ cr[0,8] = %x0D
$ lf[0,8] = %x0A
$ tstr=""	! temp string
$ efile="" !set before access to acurately report errors "error file"
$ s="" ! captures $STATUS from calls to utilities like COPY
$ rec=""	!user "record" (command line), contains all comments, etc...
$ !in_rec	used for "REMOVE" operation
$ !out_rec	as above
$ say = "write sys$output "
$ cmd_list="DISPLAY,ADD,REMOVE,USE,SAVE,HELP,EXIT,QUIT"
$ hist_file="SYS$COMMON:[SYSEXE]INGRES_INSTALLATIONS.DAT"
$ scratch_file=""
$ file_in_use=hist_file ! if DIRTY then file_in_use = scratch_file
$! (Retrieve current release ID from KITINSTAL.DATA data file)
$ param=F$SEARCH("II_SYSTEM:[INGRES.INSTALL]KITINSTAL.DATA")
$ init_default_release:
$ IF "''param'".EQS.""
$	THEN IF F$GETSYI("HW_MODEL").LT.1024
$		THEN release="2.0/0106 (vax.vms/00)"
$		ELSE release="2.0/0011 (axp.vms/00)"
$		ENDIF
$	say "%INGRES-S-DFLTRLSE, Default release set to: ''release'"+cr+lf
$	GOSUB use_file
$	RETURN
$	ENDIF
$ efile=param
$ err_msg="%INGRES-E-OPENERR, File open error."
$ OPEN/READ/ERROR=file_error d_file 'param
$ release=""
$ err_msg="%INGRES-E-READERR, File read error."
$! (Read KITINSTAL.DATA until we find the RELEASE string)
$ init_find_release:
$ READ/END=init_done/ERROR=file_error d_file tstr
$ tstr=F$EDIT(tstr,"COMPRESS,UNCOMMENT")
$ IF F$LOCATE("RELEASE",tstr).LT.F$LENGTH(tstr)
$	THEN release=F$EXTRACT(F$LOCATE(":",tstr)+2,F$LENGTH(tstr),tstr)
$	ELSE GOTO init_find_release
$	ENDIF
$ init_done:
$ err_msg="%INGRES-E-CLOSERR, File close error."
$ CLOSE/ERROR=file_error d_file
$ IF "''release'".EQS.""
$	THEN 
$	param=""
$	GOTO init_default_release
$	ENDIF
$ say "%INGRES-S-RELEASE, Release determined as: ''release'"+cr+lf
$ GOSUB use_file
$ RETURN

$ use_file:
$ IF "''rec'".EQS."" THEN rec="use sys$common:[sysexe]ingres_installations.dat"
$ tstr=F$ELEMENT(1," ",F$EDIT(rec,"COMPRESS,UNCOMMENT"))
$ IF "''tstr'".EQS." "
$	THEN
$	param_prompt="File specification [SYS$COMMON:[SYSEXE]INGRES_INSTALLATIONS.DAT] : "
$	GOSUB get_param
$	IF .NOT.ok THEN RETURN
$	IF "''param'".EQS."" THEN param="SYS$COMMON:[SYSEXE]INGRES_INSTALLATIONS.DAT"
$	tstr=param
$	ENDIF
$ IF "''F$SEARCH(tstr)'".EQS.""
$	THEN
$	say cr+lf+"%INGRES-W-FNF, Installation History file not found."
$	say "%INGRES-W-FILESPEC, Searching for ''tstr'"+cr+lf
$	IF "''F$SEARCH(hist_file)'".NES."" THEN say "%INGRES-I-HISTFILE, Still using History file ''hist_file'"
$	ELSE
$	hist_file=F$SEARCH(tstr)
$	IF .NOT.dirty THEN file_in_use=F$SEARCH(hist_file)
$	efile=hist_file
$	err_msg="%INGRES-E-OPENERR, File open error."+cr+lf+"%INGRES-I-CONTINUE, To continue anyway, 'USE' an accessable file."
$	OPEN/APPEND/ERROR=file_error h_file 'file_in_use
$	CLOSE/ERROR=fatal_file_error h_file
$	IF .NOT.dirty
$		THEN
$		IF scratch THEN DELETE/NOLOG 'scratch_file'
$		scratch_file="sys$login:scratch_file.''F$GETJPI("","PID")'"
$		IF "''F$SEARCH(scratch_file)'".NES."" THEN DELETE/NOLOG 'F$SEARCH(scratch_file)
$		COPY/NOLOG 'hist_file 'scratch_file
$		IF "''F$SEARCH(scratch_file)'".EQS.""
$			THEN
$			say "%INGRES-I-FOUNDFILE, Installation History file ''hist_file' found."
$			err_msg="%INGRES-W-NOSCRATCH, Unable to create scratch file."
$			efile=""
$			GOSUB rpt_file_error
$			say "%INGRES-I-CONTINUE, Attempting to continue anyway. . ."
$			ELSE
$			scratch=1
$			scratch_file=F$SEARCH(scratch_file)
$			say "%INGRES-S-FILESPEC, Using Installation History file ''hist_file'"
$			ENDIF
$		ELSE
$		say "%INGRES-S-FILESPEC, Using Installation History file ''hist_file'"
$		say "%INGRES-W-NOTSAVED, Unsaved changes prevent refreshing scratch file."
$		say "%INGRES-I-NEWSAVES, A SAVE now will overwrite the newly selected history file."
$		ENDIF
$	ENDIF
$ RETURN

$ get_param:
$ ok=0
$ param=""
$ READ/END=no_param/PROMPT="''param_prompt'" sys$command param
$ ok=1
$ no_param:
$ RETURN

$ press_return:
$ READ/END=user_done/PROMPT="Press return to continue. " sys$command tstr 
$ RETURN

$ ask_save:
$ if tried_save THEN GOTO just_exit
$ tried_save=1
$ say "%INGRES-W-NOTSAVED, You have unsaved updates."
$ tstr=""
$ READ/END=no_save_exit/PROMPT="Do you want to SAVE before exiting [Yes]? " sys$command tstr 
$ IF (tstr).OR.("''tstr'".EQS."") THEN GOSUB save
$ no_save_exit:
$ RETURN

$ file_error:
$ s=$status
$ GOSUB rpt_file_error
$ RETURN

$ rpt_file_error:
$ ok=0
$ say ""
$ IF "''err_msg'".NES."" THEN say "''err_msg'"
$ IF "''efile'".NES."" THEN say "%INGRES-E-FILACCERR, File error on ''efile'"
$ IF "''s'".EQS."%X1001829A"
$	THEN
$	ok=1
$	say "%INGRES-E-PRV, Insufficient privilege or file protection violation.
$	ENDIF
$ IF "''s'".EQS."%X1067109A"
$	THEN
$	ok=1
$	say "%INGRES-E-OPENIN, Error opening file as input."
$	ENDIF
$ IF .NOT.ok THEN say "%INGRES-I-STATUS, Returned status: ''s'"
$! IF "''s'".EQS."%X
$ err_msg=""
$ s=""
$ RETURN

$ fatal_file_error:
$ s=$status
$ GOSUB rpt_file_error
$ say cr+lf+"%INGRES-F-FATAL, File access error was fatal."+cr+lf
$ GOTO done

$ ctl_y:
$ say "%INGRES-F-CTLY, Exit forced by CONTROL-Y"
$ s=0
$ GOTO done

$ forced_exit:
$ say "%INGRES-F-FORCEXIT, Exit forced by error."
$ s=0
$ GOTO done

$ user_done:
$ s=1
$ done:
$ IF s_open THEN CLOSE s_file
$ IF h_open THEN CLOSE h_file
$ IF dirty THEN GOSUB ask_save
$ IF "''F$SEARCH(scratch_file)'".NES."" THEN $ DELETE/NOLOG 'scratch_file
$ just_exit:
$ exit 's
