$ goto _START_IISHLIB
$!  Copyright (c) 2002, 2008 Ingres Corporation
$! 
$! 
$!  Name: iishlib
$!
$!  Usage:
$!      iishlib
$!
$!  Description:
$!      Stand-alone procedures that can be called by any internal or external
$!	procedure. Primarily utility routines.
$!
$! Exit status
$!
$!	iishlib_err
$!		ii_status_ok
$!		ii_status_warning
$!		ii_status_error
$!		ii_status_fatal
$!
$!	 iishlib_rv1 user dats or null value 
$!	 iishlib_rv2 is retuirned to the calling
$!	 iishlib_rv3 program in the following 
$!	 iishlib_rv4 global variables
$!	 iishlib_rv5  
$!	 iishlib_rv6 the application should copy
$!	 iishlib_rv7 the data to a local variable 
$!	 iishlib_rv8 upon return after checking
$!		     the error status
$!
$! Side Effects
$!	Not all of the functions return the error status 
$!	Some functions ONLY return the error status
$!	A true / false response should is returned using iishlib_err
$!	A yes / no response is returned in iishlib_rv1
$!	On entry all of the iishlib_rv* values are set to ""
$!
$!	The iishlib_err is only set in thee iishlib_init procedure
$!	
$!  DEST = utility
$!! History
$!!     13-may-2005 (bolke01)
$!!             Initial version.
$!!             Based on the UNIX version iishlib.sh
$!!     15-may-2005 (bolke01)
$!!		Modifiled incorrect labeling of some subroutines, colon not 
$!!		against label.
$!!	07-Jun-2005 (bolke01)
$!!		Correction to _create_dirs. 
$!!	08-jun-2005 (abbjo03)
$!!	    Remove unneeded checks from check_sanity.
$!!	09-Jun-2005 (bolke01/devjo01)
$!!		- Changed WARN to WARNING
$!!		- Over-ride border_side default to be " " in message_ box
$!!		- Changed del/sym/glob to delete/symbol/global
$!!	 	- Protected system from errors if echo is not defined
$!!		- Added symbol definitions for ingres utility calls
$!!		- used in the routines in this library and iisunode.
$!!	        - Combine iiappend_first_class & iiappend_class.
$!!		- Various minor tweaks to silence DCL checker script.
$!!	14-Jun-2005 (bolke01)
$!!	    	- Unconditionally set iisunode in iishlib_init
$!!	14-jul-2005 (devjo01)
$!!		- Cleanup setting of iishlib_err
$!!		- Add add_install_history
$!!		- Add expand_logical
$!!		- Add set_ingres_symbols
$!!	        - Add copyright_banner
$!!     19-jul-2005 (devjo01)
$!!	        - Add gen_log_spec
$!!	27-sep-2005 (devjo01)
$!!		In get_node_number do not make an empty exclusion set
$!!		a special case.
$!!	15-mar-2007 (upake01)
$!!		In "_create_dir: SUBROUTINE" added F$EDIT with "upcase"
$!!             to convert the value of logical captured in ii_root_dir
$!!             to uppercase.  This was needed as file and directory names
$!!             on ODS-5 disks with "parse_style=extended" is case-sensitive.
$!!	16-jul-2007 (upake01)
$!!         SIR 118402 - Added subroutine "_selective_purge_on_config" to
$!!             do a "selective" purge on CONFIG.DAT.  This will becalled
$!!             from IiMkCluster, IiSuNode, IiUnCluster and other procedures.
$!!	07-jan-2008 (joea)
$!!	    Discontinue use of cluster nicknames.
$!!	22-jan-2008 (upake01)
$!!         BUG 119799 - In "Selective Purge" - after doing PURGE and RENAME
$!!             and before proceeding do to a DIFF of last two versions of
$!!             the file, check to make sure that there are at least two
$!!             versions of the file.
$!!	12-feb-2008 (upake01)
$!!         SIR 118402 - Cluster Support - Add missing "goto _IIREAD_RESPONSE_EXIT"
$!!             in "_iiread_response" subroutine that was giving "File not opened
$!!             by DCL" error as it was executing the next line that was trying
$!!             to close the file that was not opened.  Also modifeid to call
$!!             "check_response_write" with correct parameters.
$!!     14-Jan-2010 (horda03) Bug 123153
$!!             Add pan_cluster_cmd, pan_cluster_set_notdefined, 
$!!             pan_cluster_min_set, pan_cluster_iigetres, get_nodes,
$!!             display_nodes.
$!!
$! ------------------------------------------------------------------------
$!
$! Shell verify handling - turn off shell verify on entry, reset to previous
$! saved value on exit.
$!
$_START_IISHLIB:
$!
$ btz_ver = "''IISHLIB_VERIFY'"
$ btz_ver = F$INTEGER(btz_ver)
$ btz_ver_on = F$VERIFY(btz_ver)
$ set noon
$ border_side   = "''border_side'"
$ iishlib_rv1 :== ""
$ iishlib_rv2 :== ""
$ iishlib_rv3 :== ""
$ iishlib_rv4 :== ""
$ iishlib_rv5 :== ""
$ iishlib_rv6 :== ""
$ iishlib_rv7 :== ""
$ iishlib_rv8 :== ""
$ ii_status_ok :== 1
$ ii_status_warning :== 2
$ ii_status_error :== -1
$ ii_status_fatal :== -2
$ STS$K_WARNING = 0
$ STS$K_SUCCESS = 1
$ STS$K_ERROR = 2
$ STS$K_INFO = 3
$ STS$K_SEVERE = 4
$!
$ STS_DIFF_OK=%X006C8009
$! 
$ if 0'IISHLIB_DEBUG' .GT. 02 then call _box "''p1'" "''p2'" "''p3'" "''p4'" "''p5'" "''p6'" "''p7'" "''p8'"
$ if P1 - "_x"  .nes. P1 
$ then 
$    if P1 - "_xbox"  .nes. P1 
$    then 
$       if 0'SHELL_DEBUG' .lt. 01 then P1 = ""
$    else
$       P1 = "_" + P1 - "_x"
$    endif
$ endif
$ if "''p1'" .nes. "" 
$ then 
$    gosub check_for_quotes
$
$    call "''p1'" "''p2'" "''p3'" "''p4'" "''p5'" "''p6'" "''p7'" "''p8'"
$    if 0'IISHLIB_DEBUG' .GT. 03 then call _box "err=''iishlib_err'" "1=''iishlib_rv1'"   -
	"2=''iishlib_rv2'" "3=''iishlib_rv3'" "4=''iishlib_rv14'" "5=''iishlib_rv5'" -
	"6=''iishlib_rv6'" "7=''iishlib_rv7'"
$ endif
$!
$!
$ if btz_ver_on then btz_ver_off = F$VERIFY(btz_ver_on)
$! WRITE SYS$OUTPUT "btz_ver_on=''btz_ver_on'"
$!
$ exit 

$check_for_quotes:
$ param = 2
$
$check_fq_loop:
$ if param .le. 8
$ then
$    val = P'param'
$    par = ""
$
$val_loop:
$    len = f$length( val )
$    quote = f$locate( """", val)
$
$    if len .ne. quote
$    then
$       par = par + f$extract(0, quote+1, val) + """"
$
$       val = f$extract(quote+1, len, val)
$
$       goto val_loop
$    endif
$
$    if par .nes. ""
$    then
$       P'param' = par + val
$    endif
$
$    param = param + 1
$    goto check_fq_loop
$ endif
$
$ return
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$! CALLING USAGE:
$!    Calling of routines in this library of common subroutines should be
$!
$!    btz_ver_on          == 0
$!    if F$SEARCH("II_SYSTEM:[ingres]IISHLIB.COM") .NES ""
$!    then 
$!	 DEFINE/JOB IISHLIB "PIPE @II_SYSTEM:[ingres.utility]iishlib"
$!       callsh       :== @II_SYSTEM:[ingres.utility]iishlib
$!       iishlib_init := 'callsh' _iishlib_init
$!       iishlib_init "''self'"
$!       cleanup
$!     else
$!        callsh := CALL
$!    endif
$!-----------------------------------------------------------------------
$!
$! Begin subroutine defs.
$!
$!--------------------------------------------------------------------
$! UTILITY SUBROUTINES
$! ------------------- 
$!--------------------------------------------------------------------
$! Utility subroutine to output upto eight lines of text with top and 
$! tail as required
$!
$!--------------------------------------------------------------------
$! usage :
$! box :== 'callsh' _box 
$!
$! VMS has a restriction of 256 characters for a single command
$! therefore the box routine is restricted to short output messges
$!
$! blank lines can be output by using " " ( single space ).  output is
$! terminated when the first NULL line is parsed
$!--------------------------------------------------------------------
$ _box: SUBROUTINES
$ set nover
$ line_no=1
$!
$_NEXT_LINE:
$ parm = "p''line_no'"
$!
$ if 'parm' .NES. "" 
$ then
$	spos = 0
$	len = 0
$	lim = F$length('parm')
$ _bloop_lab:
$	inc = F$locate(" ", F$extract(spos + len,lim - spos - len,'parm') )
$	if len + inc .ge. 70
$	then
$	    if len .eq. 0
$	    then
$		len = 70
$	    endif
$	else
$	    len = len + inc + 1
$	    goto _bloop_lab
$	endif
$	write sys$output border_side + " " + -
          F$FAO("!71AS",F$extract(spos, len, 'parm')) + -
	  border_side
$	spos = spos + len
$	len = 0
$	if spos .lt. lim then goto _bloop_lab
$	line_no=line_no+1
$	if line_no .le. 8 then GOTO _NEXT_LINE
$ endif
$!
$ ENDSUBROUTINE
$!
$!
$! - A subroutine to basically turn off output when box is called directly
$!
$_xbox: SUBROUTINE
$    call _box "''p2'" "''p3'" "''p4'" "''p5'" "''p6'" "''p7'" "''p8'"
$!
$ return
$!
$ ENDSUBROUTINE
$!
$!
$!--------------------------------------------------------------------
$! usage : nobox "paragraph1" [ "paragraph2" ... ]
$!
$! Outputs each "paragraph" breaking output < 72 characters.
$!
$!--------------------------------------------------------------------
$ _nobox: SUBROUTINE
$!
$ border_side = ""
$ CALL _box "''P1'" "''P2'" "''P3'" "''P4'" "''P5'" "''P6'" "''P7'" "''P8'" 
$ border_side = "|"
$!
$ EXIT
$!
$ ENDSUBROUTINE
$!
$!
$!--------------------------------------------------------------------
$! usage : copyright_banner
$!
$! Assumes II_SYSTEM logical is correct, 'self' set.
$!
$! Prints standard copyright banner, and sets global variables
$! 'PID', 'VERSION', and 'RELEASE_ID'.
$! 
$! Sets iishlib_err to ii_status_error if RELEASE_ID can't be set.
$!--------------------------------------------------------------------
$ _copyright_banner: SUBROUTINE
$!
$!  # extract the version string from the manifest file and remove
$!  # the following symbols ( ) . / from the string.
$!   
$  PID == F$GETJPI("","PID")
$  runtime = F$TIME()
$  lversion = ""
$  tmpfile = "II_SYSTEM:[ingres]''self'.''pid'" 
$  iimaninf := $II_SYSTEM:[INGRES.UTILITY]iimaninf
$  iimaninf -version=basic -output='tmpfile'
$  OPEN iioutput 'tmpfile'
$  READ iioutput lversion
$  CLOSE iioutput
$  DELETE/NOCONFIRM/NOLOG 'tmpfile';0
$!
$  VERSION == "''lversion'"
$  RELEASE_ID == f$edit( VERSION, "lowercase,collapse, trim" ) - "(" - ")" - "." - "." - "." - " " - "/"
$  if RELEASE_ID .eqs. ""
$  then 
$     error_box "Unable to determine the release of Ingres"   
$     iishlib_err == ii_status_error
$  else
$     aname = f$getsyi("arch_name")
$     nobox " " -
	"Copyright (c) 2002, 2005 Ingres Corporation" -
	"Ingres ''self' ''aname' OpenVMS version ''VERSION'" -
	"''runtime'" -
	" "
$     iishlib_err == ii_status_ok
$  endif
$!
$ ENDSUBROUTINE
$!
$!
$!--------------------------------------------------------------------
$! usage : box_banner "Title for box"
$!
$! Centers passed title in 1st line of box, then outputs an empty box line.
$!--------------------------------------------------------------------
$ _box_banner: SUBROUTINE
$!
$ pad = 35 - F$length(p1) / 2
$ pad = "!"+F$FAO("!UL",pad)+"* "
$ title = F$FAO(pad)+"''P1'"
$ CALL _box "''title'" " " 
$!
$ ENDSUBROUTINE
$!--------------------------------------------------------------------
$! usage :
$! error_box :== 'callsh' _error_box 
$!
$!--------------------------------------------------------------------
$ _error_box: SUBROUTINE
$!
$ box_division
$ CALL _box_banner "*** ERROR ***"
$ CALL _box "''P1'" "''P2'" "''P3'" "''P4'" "''P5'" "''P6'" "''P7'" "''P8'" 
$ box_division
$!
$ ENDSUBROUTINE
$!
$!--------------------------------------------------------------------
$! usage :
$! warn_box :== 'callsh' _warn_box 
$!
$!--------------------------------------------------------------------
$ _warn_box: SUBROUTINE
$!
$ box_division
$ CALL _box_banner "*** WARNING ***"
$ CALL _box "''P1'" "''P2'" "''P3'" "''P4'" "''P5'" "''P6'" "''P7'" "''P8'" 
$ box_division
$!
$ ENDSUBROUTINE
$!
$!--------------------------------------------------------------------
$! usage :
$! message_box :== 'callsh' _message_box 
$!
$!--------------------------------------------------------------------
$ _message_box: SUBROUTINE
$!
$ border_side         == ""
$ box_division
$ CALL _box_banner "*** INFORMATION ***"
$ CALL _box "''P1'" "''P2'" "''P3'" "''P4'" "''P5'" "''P6'" "''P7'" "''P8'" 
$ box_division
$ border_side         == "|"
$!
$ ENDSUBROUTINE
$!
$!--------------------------------------------------------------------
$! usage :
$! info_box :== 'callsh' _message_box 
$!
$!--------------------------------------------------------------------
$ _info_box: SUBROUTINE
$!
$ if "''iishlib_info'" .nes. ""
$ then
$    border_side         == "*"
$    write sys$output F$FAO("*!72***")
$    CALL _box_banner "*** DIAGNOSTIC ***"
$    CALL _box "''P1'" "''P2'" "''P3'" "''P4'" "''P5'" "''P6'" "''P7'" "''P8'" 
$    write sys$output F$FAO("+!72**+")
$    border_side         == "|"
$ endif
$!
$ ENDSUBROUTINE
$!
$_iishlib_init: SUBROUTINE
$!
$! The error status must only be initialsed in the iishlib_init routine
$! since any subsequent call to the routines in iishlib must not over
$! write its value.
$!
$ iishlib_err == 0
$!
$   self = "''P1'"
$   if self .eqs. ""
$   then
$      call _box "The caller prgram must identify its-self"
$      iishlib_err == ii_status_fatal
$      exit
$   endif
$!
$   iishlib_ltime == F$ELEMENT(0,".",F$TIME())   
$!
$   callsh 	        :== @II_SYSTEM:[ingres.utility]iishlib
$!
$   border_side         == "|"
$   box_division        :== "write sys$output F$FAO(""+!72*-+"")"
$   box                 :== 'callsh' _box 
$   chop_and_count      :== 'callsh' _chop_and_count
$   cleanup      	:== 'callsh' _cleanup
$   confirm_intent	:== 'callsh' _confirm_intent
$   cut_and_append	:== 'callsh' _cut_and_append
$   element_index 	:== 'callsh' _element_index
$   error_box           :== 'callsh' _error_box 
$   error_condition     :== 'callsh' _error_condition
$   in_set              :== 'callsh' _in_set
$   iiappend_class      :== 'callsh' _append_class 
$   info_box            :== 'callsh' _info_box 
$   message_box         :== 'callsh' _message_box 
$   nobox               :== 'callsh' _nobox 
$   nth_element 	:== 'callsh' _nth_element
$   substitute_string   :== 'callsh' _substitute_string
$   warn_box            :== 'callsh' _warn_box 
$   xbox                :== 'callsh' _xbox 
$   copyright_banner	:== 'callsh' _copyright_banner
$!
$   backup_file		:== 'callsh' _backup_file
$   check_path 		:== 'callsh' _check_path
$   check_response_write :== 'callsh' _check_response_write
$   check_user          :== 'callsh' _check_user
$   iicreate_dir        :== 'callsh' _create_dir 
$   gen_log_name        :== 'callsh' _gen_log_name
$   gen_log_spec        :== 'callsh' _gen_log_spec
$   gen_ok_set          :== 'callsh' _gen_ok_set
$   get_node_number     :== 'callsh' _get_node_number
$   gen_nodes_lists     :== 'callsh' _gen_nodes_lists
$   getressym           :== 'callsh' _getressym
$   iiread_response     :== 'callsh' _iiread_response
$   lock_config_dat     :== 'callsh' _lock_config_dat
$   log_config_change_banner :== 'callsh' _log_config_change_banner
$   add_install_history :== 'callsh' _add_install_history
$   expand_logical	:== 'callsh' _expand_logical
$   set_ingres_symbols	:== 'callsh' _set_ingres_symbols
$   node_name           :== 'callsh' _node_name
$   remove_file         :== 'callsh' _remove_file
$   rename_file         :== 'callsh' _rename_file
$   restore_file	:== 'callsh' _restore_file
$   check_sanity        :== 'callsh' _check_sanity
$   unlock_config_dat   :== 'callsh' _unlock_config_dat
$   usage               :== 'callsh' _usage 
$   validate_node_number  :== 'callsh' _validate_node_number
$   selective_purge       :== 'callsh' _selective_purge_on_config
$   pan_cluster_cmd       :== 'callsh' _pan_cluster_cmd
$   pan_cluster_set_notdefined :== 'callsh' _pan_cluster_set_notdefined
$   pan_cluster_min_set   :== 'callsh' _pan_cluster_min_set
$   pan_cluster_iigetres  :== 'callsh' _pan_cluster_iigetres
$   get_nodes             :== 'callsh' _get_nodes
$   display_nodes         :== 'callsh' _display_nodes
$!
$   iishlib_'self'_ltime == ""
$!
$   iishlib_messages_off :== SET MESSAGE/NOFAC/NOID/NOSEV/NOTEXT
$   iishlib_messages_on  :== SET MESSAGE/FAC/ID/SEV/TEXT
$!
$!  Primary definitions as defined in sysdef.com
$!  assume if iisetres is defined, all required symbols are defined.
$!  defien a flag to indicate that the clean opperation should remove these 
$!  definitions
$!
$   if "''iisetres'" .EQS. ""
$   then
$      iigetres  		:== $II_SYSTEM:[INGRES.UTILITY]IIGETRES.EXE
$      iiinitres  		:== $II_SYSTEM:[INGRES.UTILITY]IIINITRES.EXE
$      iisetres  		:== $II_SYSTEM:[INGRES.UTILITY]IISETRES.EXE
$      rcpconfig 		:== $II_SYSTEM:[INGRES.BIN]RCPCONFIG.EXE
$      rcpstat			:== $II_SYSTEM:[INGRES.BIN]RCPSTAT.EXE
$      iishlib_iisysdef == 1
$   else
$      iishlib_iisysdef == 0
$   endif
$   iisulock 		:== $II_SYSTEM:[INGRES.UTILITY]IISULOCK.EXE
$
$   'P1'_gbl_syms == ""
$!
$ exit 
$!
$ endsubroutine
$!
$!
$ _error_condition: SUBROUTINE
$ if p1 .eq. STS$K_ERROR .or. p1 .eq. STS$K_SEVERE
$ then
$     iishlib_err == ii_status_error
$ else
$     iishlib_err == ii_status_ok
$ endif
$ exit 
$ endsubroutine
$!
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _cleanup: SUBROUTINE
$!
$ delete/symbol/global iishlib_rv1
$ delete/symbol/global iishlib_rv2
$ delete/symbol/global iishlib_rv3
$ delete/symbol/global iishlib_rv4
$ delete/symbol/global iishlib_rv5
$ delete/symbol/global iishlib_rv6
$ delete/symbol/global iishlib_rv7
$ delete/symbol/global iishlib_rv8
$ delete/symbol/global iishlib_err
$ delete/symbol/global ii_status_ok 
$ delete/symbol/global ii_status_warning 
$ delete/symbol/global ii_status_error 
$ delete/symbol/global ii_status_fatal
$ if P1 .nes. ""
$ then 
$    if iishlib_'P1'_ltime .nes. "" 
$    then
$       iishlib_'P1'_ltime = ""
$	delete/symbol/global iishlib_ltime 
$       delete/symbol/global iishlib_'P1'_ltime
$    endif
$    if  iishlib_iisysdef .eq. 1
$    then
$        delete/symbol/global iigetres
$        delete/symbol/global iiinitres
$        delete/symbol/global iisetres
$        delete/symbol/global rcpconfig
$        delete/symbol/global rcpstat
$    endif
$    delete/symbol/global iisulock
$
$    if f$type('P1'_gbl_syms) .nes. ""
$    then
$       sym = 0
$
$       mess = f$environment( "message" )
$       set message/notext/nosev/noid/nofac
$
$DEL_SYM:
$       del_sym = f$element( sym, ",", 'P1'_gbl_syms)
$
$       if del_sym .nes. ","
$       then
$          sym = sym + 1
$
$          if del_sym .nes. ""
$          then
$             delete/sym/glob 'del_sym
$          endif
$
$          goto DEL_SYM
$       endif
$
$       delete/sym/glob 'P1'_gbl_syms
$       set message 'mess'
$    endif
$!
$ endif
$!
$ exit
$!
$ endsubroutine
$!
$!
$!   Complain about bad input.
$!
$!      $1 = bad parameter 
$!
$ _usage: SUBROUTINE
$!
$ call _warn_box "All Usage descriptions should be handled in the local module"
$ exit
$ endsubroutine
$!
$!
$!   Verify legit value for node number
$!
$!	$1 = candidate node number.
$!	$2 = if not blank, surpress warning message.
$!
$ _validate_node_number: SUBROUTINE
$!
$ nodeId = F$INTEGER(P1)
$ verbose = "''P2'"
$ if nodeId .lt. 1 .or. nodeId .gt. cluster_max_node
$ then
$    if verbose .nes. ""
$    then
$       warn_box  "Invalid node number value [''nodeId'].  Node number must be an integer in" -
		 "the range 1 through ''cluster_max_node' inclusive!"
$    endif
$    iishlib_err == ii_status_error
$    exit
$ endif
$ iishlib_err == ii_status_ok
$!
$ exit
$!
$ endsubroutine
$!
$!
$!------------------------------------------------------------------------
$!   Utility routine to check precence of a directory in PATH
$! Usage:
$!
$! Input:
$!   $1 = last segment of directory path, e.g. bin or utility.
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _check_path: SUBROUTINE
$!
$ ingDir = "''P1'"
$!
$ ii_system = f$trnlnm("II_SYSTEM")
$ if f$search("II_SYSTEM:[ingres]''ingDir'.dir") .eqs. ""
$ then
$     error_box "''ii_system'[ingres.''ingDir'] does  not exist."
$     iishlib_err == ii_status_error
$ else
$     iishlib_err == ii_status_ok
$ endif
$ exit
$ endsubroutine
$!
$!
$!------------------------------------------------------------------------
$! Generic prompt for user input of y or n
$! Usage:
$!
$! Input:
$!
$! Output
$!	iishlib_rv1 == [y | n]
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _confirm_intent: SUBROUTINE
$!
$_CONFIRM_INTENT_RESP:
$ WRITE SYS$OUTPUT " "
$ inquire resp " Are you sure you want to continue this procedure? (y/n) " 
$ resp = F$EDIT(F$EXTRACT(0,1,resp),"LOWERCASE, COMPRESS, TRIM")
$ WRITE SYS$OUTPUT " "
$!
$ if resp -"y" .nes. resp .OR. resp -"n" .nes. resp  
$ then
$   iishlib_rv1 == resp
$ else
$   goto _CONFIRM_INTENT_RESP
$ endif
$!
$ exit
$!
$ endsubrouitne
$!
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$! 	$1	target 		file to backup
$! 	$2	unqext 		Unique extension ( datestamp)
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _backup_file: SUBROUTINE
$!
$ target  = "''P1'"
$ unqext  = "''P2'"
$!
$ if unqext .EQS. "" 
$ then
$   error_box "Invalid unique reference passed as parm2 - blank is not allowed"
$   iishlib_err == ii_status_error
$   goto _backup_file_exit
$ endif
$!
$  bckfile = "''target'" + "''unqext'"
$ if f$search("II_CONFIG:''bckfile'") .nes. ""
$ then
$    error_box "Failed to backup ''target' , ''bckfile' exists" 
$    iishlib_err == ii_status_error
$    goto _backup_file_exit
$ endif
$!
$ if f$search("II_CONFIG:''target'") .nes. ""
$ then
$     copy II_CONFIG:'target' II_CONFIG:'bckfile'
$     PIPE DIFF/OUT=NL: II_CONFIG:'target' II_CONFIG:'bckfile' ; svs = $SEVERITY > NL:
$     PIPE DIFF/OUT=NL: II_CONFIG:'target' II_CONFIG:'bckfile' ; sts = $STATUS > NL:
$     if 'svs' .ne. STS$K_SUCCESS .and. 'svs' .ne. STS$K_INFO .and. sts .ne. STS_DIFF_OK
$     then 
$         error_box "Failed to backup ''target'" 
$         iishlib_err == ii_status_error
$     else
$	  info_box "Backup of ''target'=''bckfile'"
$         iishlib_err == ii_status_ok
$     endif
$ endif
$!
$_backup_file_exit:
$ exit
$ endsubroutine
$!
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$! 	$1	target 		file to backup
$! 	$2	unqext 		Unique extension ( datestamp)
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _restore_file: SUBROUTINE
$!
$ target  = "''P1'"
$ unqext  = "''P2'"
$!
$ if unqext .EQS. "" 
$ then
$   error_box "Invalid unique reference passed as parm2 - blank is not allowed"
$   iishlib_err == ii_status_error
$   goto _restore_file_exit
$ endif
$!
$ bckfile = "''target'" + "''unqext'"
$ if f$search("II_CONFIG:''bckfile'") .nes. ""
$ then
$    if f$search("II_CONFIG:''bckfile'") .nes. ""
$    then
$        purge II_CONFIG:'bckfile'
$        rename II_CONFIG:'bckfile II_CONFIG:'target'
$        if '$severity' .ne. STS$K_SUCCESS .and. '$severity' .ne. STS$K_INFO
$        then 
$            error_box "Failed to restore ''target'" 
$            iishlib_err == ii_status_error
$        else
$ 	     message_box "Restored ''bckfile' to ''target'"
$            iishlib_err == ii_status_ok
$            if f$search("II_CONFIG:''target'") .nes. ""
$            then
$               purge II_CONFIG:'target'
$	     else
$               error_box "Failed to restore ''target' correctly" 
$               iishlib_err == ii_status_fatal
$            endif
$        endif
$    endif
$ endif
$_restore_file_exit:
$ exit
$ endsubroutine
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$!*********************
$!*** BEGIN SESSION ***
$!*********************
$!
$!host:          HASTY
$!user:          auserid
$!terminal:      tna1180
$!time:           6-MAY-2005 11:36:36
$!------------------------------------------------------------------------
$ _log_config_change_banner: SUBROUTINE
$ self = F$EDIT(P3,"LOWERCASE")
$ host = F$EDIT(P2,"UPCASE")
$ user = F$EDIT(F$USER(),"LOWERCASE") -"[" - "]"
$ terminal = F$EDIT(F$PROCESS(),"LOWERCASE") - "_" 
$!
$ if self .eqs. "" 
$ then
$    error_box "Missing or invalid value provided for P3 [''self']"
$    iishlib_err == ii_status_error
$ endif
$!
$ if iishlib_'self'_ltime .eqs. ""
$ then
$!
$    iishlib_'self'_ltime == iishlib_ltime
$    $DEBUGIT copy II_CONFIG:config.log II_CONFIG:config.logx
$    open/append cfglog II_CONFIG:config.log
$    write cfglog " "
$    write cfglog "*********************"
$    write cfglog "*** BEGIN SESSION ***"
$    write cfglog "*********************"
$    write cfglog "host:          ''host'"
$    write cfglog "user:          ''user'"
$    write cfglog "terminal:      ''terminal'"
$    write cfglog "time:          ''iishlib_ltime'"
$ endif
$ write cfglog " " 
$ write cfglog "------------------------------------------------------------------------"
$ write cfglog "''p1' ''p2' ''p3' ''p5' ''P6' ''P4'" 
$ write cfglog "------------------------------------------------------------------------"
$ write cfglog " " 
$ close cfglog
$ $DEBUGIT diff II_CONFIG:config.log II_CONFIG:config.logx
$!
$ exit
$!
$ endsubroutine
$!
$!------------------------------------------------------------------------
$! Usage: add_install_history "version" "text"
$!
$!	This routine appends an entry to the Ingres Installation History
$!	file, which tracks major changes to all Ingres installations on
$!	a machine.
$!
$! Input: p1 - Version String
$!        p2 - text string to be added for "package" & "comment"
$!
$! Output
$!	  iishlib_err -> ii_status_warning if can't update log.
$!
$! Side effects
$!	  New entry to sys$common:[sysexe]ingres_installations.dat
$!
$!------------------------------------------------------------------------
$ _add_install_history: SUBROUTINE
$!
$	lim_log="sys$common:[sysexe]ingres_installations.dat"
$	if F$search(lim_log) .EQS. ""
$	then
$!	    Start the new file
$	    lim_seedlog="II_SYSTEM:[INGRES.INSTALL]INGRES_INSTALLATIONS.DAT"
$	    if F$search(lim_seedlog) .EQS. "" then goto lim_fail_lab
$	    COPY 'lim_seedlog' 'lim_log'
$	endif
$	OPEN /APPEND/ERROR=lim_fail_lab lim_file 'lim_log'
$	expand_logical II_SYSTEM
$	lim_rec = "''F$Time()' ''p1'  " + -
  "''f$fao("!%U",f$identifier(f$getjpi("","username"),"NAME_TO_NUMBER"))'" + -
  "   ''iishlib_rv1'  ''p2'"
$	lim_buf = F$Extract(0,79,lim_rec)
$	lim_off = 79
$ lim_wloop_lab:
$	WRITE /ERROR=lim_close_lab lim_file lim_buf
$	lim_buf = F$Extract(lim_off,56,lim_rec)
$	if "''lim_buf'" .EQS. "" then goto lim_close_lab
$	lim_buf = F$FAO("!23* ") + lim_buf
$	lim_off = lim_off + 56
$	goto lim_wloop_lab
$ lim_close_lab:
$	CLOSE lim_file
$	iishlib_err == ii_status_ok
$	EXIT
$ lim_fail_lab:
$	warn_box "Unable to update Ingres Installation History file" -
		 "''lim_log'"
$	iishlib_err == ii_status_warning
$!
$ endsubroutine
$!
$!------------------------------------------------------------------------
$! Usage: expand_logical logical
$!
$!	Iteratively expands a concealed logical until it can be expanded
$!	no more.
$!
$! Input: p1 - Name of logical to expand.
$!
$! Output
$!	  iishlib_rv1 will hold expanded logical.
$!
$! Side effects
$!	  none
$!
$!------------------------------------------------------------------------
$ _expand_logical: SUBROUTINE
$!
$	exl_logical="''p1'"
$	exl_dross=""
$	exl_next=""
$ exl_loop_label:
$	exl_next=f$trnlnm(exl_logical)
$	if "''exl_next'" .NES. "''exl_logical'" .AND. -
           "FALSE" .EQS. f$trnlnm(exl_logical,,,,,"TERMINAL")
$	then
$	    exl_split = F$Locate(":",exl_next)
$	    if F$length(exl_next) - 1 .GT. 'exl_split' 
$	    then
$		exl_dross=F$extract(exl_split + 2, -
		            F$locate("]",exl_next) - (exl_split + 2), -
			    exl_next) + "''exl_dross'"
$	    endif
$	    exl_logical = F$extract(0, exl_split, exl_next)
$	    goto exl_loop_label
$	endif
$	if "''exl_next'" .eqs. "" then exl_next = "''exl_logical':"
$	if "''exl_dross'" .EQS. ""
$	then
$	    iishlib_rv1 == "''exl_next'"
$	else
$	    iishlib_rv1 == "''exl_next'[''exl_dross']"
$	endif
$ endsubroutine
$!
$!------------------------------------------------------------------------
$! Usage: set_ingres_symbols [ quiet ]
$!
$!	Call INGSYSDEF to establish ALL the Ingres symbols & logicals.
$!
$! Input: p1 - If present, suppress display.
$!
$! Output
$!	  none
$!
$! Side effects
$!	  none
$!
$!------------------------------------------------------------------------
$ _set_ingres_symbols: SUBROUTINE
$!
$   if F$SEARCH("II_SYSTEM:[ingres]ingsysdef.com") .NES. ""
$   then
$	if "''p1'"
$	then
$           PIPE @II_SYSTEM:[ingres]ingsysdef > NL:
$	else
$           @II_SYSTEM:[ingres]ingsysdef
$       endif
$!	Prevent cleanup from removing symbols.
$       iishlib_iisysdef = 0
$   endif
$ endsubroutine
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$! 	$1	target_list 	list of items to search
$!	$2 	search_subst	item to search for
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _in_set: SUBROUTINE
$!
$ target_list = " ''P1' "
$ search_subst = "''P2'"
$!
$ target_eos = F$LENGTH (target_list)
$ found = F$LOCATE (" ''search_subst' ", target_list)
$!
$ if found .EQ. target_eos 
$ then
$    iishlib_rv1 == 0
$    iishlib_err == ii_status_warning
$ else
$    iishlib_rv1 == 1
$ endif
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$! 	$1	self		calling program
$!	$2 	node_name	node to obtain value from
$! 	$3	parm		parameter 
$!	$4	<reserved> 	
$!	$5 	verbose		diagnostic define in calling program
$!				typically using 'SHELL_DEBUG'
$! Output
$!
$! Side effects
$!	All values are returned as lowercase
$!##
$!##	A logical may get left defined depending on the version of
$!##	VMS that is installed as iigetres defines a lowercase 
$!##	logical name and DEASIGN is unable to recognise this.
$!
$!-----------------------------------------------------------------
$ _getressym: SUBROUTINE
$!
$ self          = F$EDIT(P1,"LOWERCASE") 
$ node_nm       = F$EDIT(P2,"LOWERCASE") 
$ parm          = F$EDIT(P3,"LOWERCASE") 
$ verbose       = "''p5'"
$!
$ local_result = f$trnlnm( "''self'_shlib_rv1","LNM$PROCESS_TABLE" )
$!
$ if local_result .nes. ""
$ then
$    if  verbose .nes. ""
$    then
$       WRITE SYS$OUTPUT ""
$       show log/full 'self'_l_shlib_rv1
$       WRITE SYS$OUTPUT ""
$    endif
$    DEFINE/PROCESS 'self'_l_shlib_rv1 "Undefined"
$    DEASSIGN/TABLE=LNM$PROCESS_TABLE 'self'_l_shlib_rv1
$ endif
$!
$ iigetres ii.'node_nm'.'parm' 'self'_l_shlib_rv1
$ sts = $severity
$ local_result = f$trnlnm( "''self'_l_shlib_rv1" )
$ if local_result .eqs. ""
$ then
$    if  verbose .nes. ""
$    then
$       WRITE SYS$OUTPUT ""
$       WRITE SYS$OUTPUT "ii.''node_nm'.''parm' not found in CONFIG.DAT."
$       WRITE SYS$OUTPUT ""
$    endif
$    if sts .gt. 1
$    then
$       box " " "Value of ii.''node_nm'.''parm' was blank." " "
$    endif
$    iishlib_rv1 == ""
$ else
$    iishlib_rv1 == F$EDIT(local_result,"LOWERCASE") 
$    if  verbose .nes. ""
$    then
$       box " " "value of ii.''node_nm'.''parm' = ''iishlib_rv1'" " "
$       iishlib_rv1 == F$EDIT(local_result,"LOWERCASE") 
$    endif
$ endif
$!
$ DEFINE/PROCESS 'self'_l_shlib_rv1 "Undefined"
$ DEASSIGN/TABLE=LNM$PROCESS_TABLE 'self'_l_shlib_rv1
$!
$ exit 
$!
$ ENDSUBROUTINE 
$!
$!------------------------------------------------------------------------
$! Usage:
$!#
$!#   Utility routine to generate a log name based on a base log name,
$!#   a segment number, and an optional node extention.  Final name
$!#   must be 36 chars or less.  Name is echoed to stdout.
$!
$! Input:
$!#       $1 	 base name.
$!#       $2 	 segment # (1 .. 16)    ERROR if out of range or string
$!#       $3 	 CONFIG_HOST, or "".
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _gen_log_name: SUBROUTINE 
$!
$ base_name     = F$EDIT(P1,"LOWERCASE") 
$ segment       = F$INTEGER(P2) 
$ host          = F$EDIT(P3,"COLLAPSE,LOWERCASE") 
$ verbose       = "''p4'"
$!
$ if segment .EQ. 0
$ then 
$    error_box "Bad segment specified ''P2'" "BASE_NMAE=''base_name'" -
 		"''host'" 
$ endif
$    ext=""
$    if "''host'" .NES. "" 
$    then
$       ext="_''host'" 
$    endif
$    paddedpart=F$FAO("!2ZW",segment) 
$    if verbose .nes. "" then WRITE SYS$OUTPUT "''base_name'.l''paddedpart'''ext'"
$!
$    iishlib_rv1 == "''base_name'.l''paddedpart'''ext'"
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!#
$!#   Utility routine to generate a transaction log file specification
$!#   from a base location which can be either a concealed logical or
$!#   a device plus dir spec, and the file name.
$!
$! Input:
$!#       $1 	 base location
$!#       $2 	 file name
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _gen_log_spec: SUBROUTINE 
$!
$ base_loc      = "''P1'" 
$ file_name     = "''P2'"
$!
$ if F$Extract(F$Length(base_loc) - 1, 1, base_loc ) .NES. "]"
$ then
$    iishlib_rv1 == "''base_loc'[ingres.log]''file_name'"
$ else
$    iishlib_rv1 == F$Extract(0, F$Length(base_loc) - 1, base_loc) + -
       ".ingres.log]''file_name'"
$ endif 
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Unlock the config.dat file only if we own the config.lck file still
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$!
$ _unlock_config_dat: SUBROUTINE
$!
$ svs = STS$K_SUCCESS
$ self = "''P1'"
$ if self .eqs. ""
$ then 
$    iishlib_err == ii_status_warning
$    warn_box "Unable to remove lock file - No owner program specified as the creator"
$    exit 
$ endif 
$ iisulocked = F$INTEGER(P2)
$ if iisulocked .AND. F$SEARCH("II_CONFIG:config.lck;*") .NES. "" 
$ then
$    PIPE SEARCH/NOWARN/NOLOG/NOOUT II_CONFIG:config.lck.0 "''self'" ; svs = $SEVERITY > NL:
$    if svs .EQ. 1
$    then
$       DELETE/NOCONFIRM/NOLOG II_CONFIG:config.lck.0
$       iishlib_err == ii_status_ok
$    else
$       iishlib_err == ii_status_warning
$       warn_box "Unable to remove lock file - ''self' is not the creator"
$    endif
$ endif
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! lock the config.dat file only if we config.lck does not exist
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$!
$ _lock_config_dat: SUBROUTINE
$!
$ self = "''P1'"
$ if self .eqs. ""
$ then 
$    iishlib_err == ii_status_warning
$    warn_box "Unable to lock file - No owner program specified as the creator"
$    exit 
$ endif 
$ iisulocked = F$INTEGER(P2)
$ val =  F$SEARCH("II_CONFIG:config.lck;*")
$ if 'iisulocked' .EQ. 0 .AND. "''val'" .EQS. "" 
$ then
$    PIPE iisulock "''self'" ;  ss = $SEVERITY > NL:
$    val = F$SEARCH("II_CONFIG:config.lck;*")
$    if val .EQS. ""
$    then
$       error_box "Unable to lock config.dat file for ''self'"
$       iishlib_rv1 == 0
$       iishlib_err == ii_status_error
$    else
$       info_box "Locked config.dat file for ''self'"
$       iishlib_rv1 == 1
$       iishlib_err == ii_status_ok
$    endif
$ else
$    error_box "''self' shows config.dat locked or config.lck exists"
$    iishlib_rv1 == 0
$    iishlib_err == ii_status_error
$ endif
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!# gen_ok_set() - Create a numeric range string
$!#   
$!#       Generate a string containing a valid set of integers
$!#       between 1 and a max value, excluding those numbers
$!#       which appear in an exclusion set.  String is formatted
$!#       as a comma separated list, with consecutive integers
$!#       replaced with the low and high numbers in the sequence
$!#       separated by a hyphen.
$!#       
$!#       In passing, a default input value of the the lowest
$!#       available number is calculated.
$!#       
$!#       Params: $1 - exclusion set (double quote this)
$!#               $2 - max legitimate value.
$!#           
$!#       Outputs: okset  - string containing valid values.
$!#               okdefault       - default value or "" is no
$!#                                 numbers are available to select.
$!#
$!------------------------------------------------------------------------
$ _gen_ok_set: SUBROUTINE
$!
$!    # Set default as lowest unused #.
$    exclst 	= "''P1'"
$    maxval 	= F$INTEGER(P2)
$    pad_zero 	= "''P3'"
$!
$    if maxval .eq. 0
$    then 
$       error_box "Invalid parameter P2=[''P2']"
$       exit 
$    endif
$    okdefault=""
$    okset=""
$    i=0
$    j=1
$    k=0
$_GEN_OK_SET_NEXT:
$ if i .lt. maxval
$ then
$    i = i + 1
$    if pad_zero .nes. ""
$    then
$       iz = F$FAO("!2ZL",i)
$    else
$	iz = i
$    endif
$!
$    in_set "''exclst'" "''iz'" 
$    if iishlib_rv1 .eq. 0
$    then
$       if okdefault .eqs. "" then  okdefault="''i'"
$       if i .eq. j + k
$       then
$           k = k + 1
$           goto _GEN_OK_SET_NEXT
$       endif
$       if okset .nes. "" then  okset="''okset',"
$       if k .gt. 1 
$       then
$            tmp = j + k - 1
$            okset="''okset'''j'-''tmp'"
$       else 
$          if  k .eq. 1 
$          then
$             okset="''okset'''j'"
$          endif
$       endif
$       j=i
$       k=1
$    endif
$    goto _GEN_OK_SET_NEXT
$ endif
$!
$!    # Finish off last subset of "okset".
$     if okset .nes. "" then  okset="''okset',"
$     if k .gt. 1
$     then
$        tmp = j + k - 1
$        okset="''okset'''j'-''tmp'"
$     else
$        okset="''okset'''j'"
$    endif
$!
$ iishlib_rv1 == okdefault
$ iishlib_rv2 == okset
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _iiread_response: SUBROUTINE
$ response = F$EDIT(P1,"COLLAPSE, TRIM, UPCASE")
$ respfile = "''P2'"
$!
$ if F$SEARCH(respfile) .NES. ""
$ then 
$    OPEN/READ/ERROR=_IIREAD_RESPONSE_O_ERR infile 'respfile' 
$_IIREAD_RESPONSE_NEXT:
$    READ/ERROR=_IIREAD_RESPONSE_R_ERR/END_OF_FILE=_IIREAD_RESPONSE_EOF infile irec
$    if irec - "''response' " .nes. irec
$    then
$       iishlib_rv1 ==  F$EDIT(irec - response, "COMPRESS, TRIM" )
$       info_box "''response' = ''iishlib_rv1'"
$       iishlib_err == ii_status_ok
$       goto  _IIREAD_RESPONSE_EOF
$    else
$       goto  _IIREAD_RESPONSE_NEXT
$    endif
$ else
$       iishlib_rv1 == ""
$       iishlib_err == ii_status_error 
$       goto _IIREAD_RESPONSE_EXIT
$ endif
$!
$_IIREAD_RESPONSE_EOF:
$ close infile
$!
$ goto _IIREAD_RESPONSE_EXIT
$!
$_IIREAD_RESPONSE_R_ERR:
$ close infile
$!
$_IIREAD_RESPONSE_O_ERR:
$!
$_IIREAD_RESPONSE_EXIT:
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$!# element_index() - Echo the index of an element of a space delimited
$!#                   set or "0".
$!
$! Input:
$!#       Params: $1 = set (double quote it!)
$!#               $2 = element to search for.
$!#
$! Output
$!  	iishlib_err == ii_status_warning when no element is found
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _element_index: SUBROUTINE
$!
$ ele_set = F$EDIT("''P1'","LOWERCASE, COMPRESS")
$ element = F$EDIT("''P2'","LOWERCASE, COMPRESS")
$ iishlib_err == ii_status_error
$!
$ num = 0
$ label = ""
$!
$_ELEMENT_INDEX_NEXT:
$  label = F$ELEMENT(num," ","''ele_set'")
$  if label .eqs. " " then goto _ELEMENT_INDEX_EOS
$  if element .eqs. label then goto _ELEMENT_INDEX_FOUND
$!
$ num = num + 1
$ goto _ELEMENT_INDEX_NEXT
$!
$_ELEMENT_INDEX_FOUND:
$ iishlib_rv1 == num
$ iishlib_err == ii_status_ok
$ goto _ELEMENT_INDEX_EXIT
$! 
$_ELEMENT_INDEX_EOS:
$ iishlib_rv1 == 0
$ iishlib_err == ii_status_warning
$ goto _ELEMENT_INDEX_EXIT
$!
$_ELEMENT_INDEX_EXIT:
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$!# nth_element() - Echo n'th element of a space delimited set.
$!
$! Input:
$!#       Params: $1 = set (double quote it!)
$!#               $2 = element to echo { n = 1 .. <n> }.
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _nth_element: SUBROUTINE
$!
$ target_set = F$EDIT("''P1'","LOWERCASE, COMPRESS")
$ element = F$INTEGER(P2)
$ iishlib_rv1 == ""
$!
$  label_value = F$ELEMENT(element," ",target_set)
$  if label_value .EQS. " " then goto _NTH_ELEMENT_EOS
$!
$ iishlib_rv1 == label_value
$! 
$_NTH_ELEMENT_EOS:
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _check_response_write: SUBROUTINE
$!
$ response = F$EDIT(P1,"COLLAPSE, TRIM, UPCASE")
$ respfile = "''P2'"
$ replace  = F$EDIT("''P3'","LOWERCASE")
$!
$ if F$SEARCH(respfile) .NES. ""
$ then 
$    if replace .eqs. "replace" .or. replace .eqs. "delete"
$    then
$       PIPE SEARCH/NOLOG/MATCH=NOR/EXACT/OUT='respfile'_tmp 'respfile'  "''response'" > NL:
$       rename 'respfile'_tmp 'respfile'
$       PURGE 'respfile'
$       if replace .eqs. "delete"
$       then 
$          goto _CHECK_RESP_WRITE_END
$       endif
$    endif
$! 
$    OPEN/WRITE outfile 'respfile' 
$!
$!# write to reponse file
$    WRITE/ERROR=_CHECK_RESP_WRITE_ERROR outfile "''response' ''value'"
$!
$    iiread_response "''response'"
$    if iishlib_rv1 .nes. value
$    then
$       error_box "The value written to response file was differnt to the value" -
		  "that was provided" -
		  " " "response ''response' was given as [''value']" -
		  "           written as [''iishlib_rv1']" 
$       goto _CHECK_RESP_WRITE_ERROR
$    endif
$ endif
$ goto _CHECK_RESP_WRITE_END
$!    
$_CHECK_RESP_WRITE_ERROR:
$ error_box "Unable to write to response file ''respfile'"
$ iishlib_err == ii_status_error
$ goto _CHECK_RESP_WRITE_END
$!
$_CHECK_RESP_WRITE_END:
$ close outfile
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$!# node_name() - Modify CONFIG_HOST value if using NUMA clusters
$!#
$!#       Utility routine which echoes the "NUMA" host name.  If $1 empty
$!#       $CONFIG_HOST is returned, otherwise $CONFIG_HOST is prefixed
$!#       with 'r${1}_'.   This "decorated" CONFIG_HOST value is used
$!#       where individual virtual NUMA nodes must save parameters
$!#       specific to their RAD.
$!
$! Input:
$!#       Params: $1 = RAD number or blank if not NUMA.
$!#               $2 = Optional CONFIG_HOST 
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _node_name: SUBROUTINE
$!
$    self     = "''P1'"
$    numa_rad = "''P2'"
$    old_node = "''P3'"
$    if "''old_node'" .NES. "" 
$    then
$       config = F$EDIT(old_node,"LOWERCASE")
$    else
$       config = F$EDIT("''CONFIG_HOST'","LOWERCASE")
$    endif
$    if  "''config'" .EQS. "" 
$    then
$       error_box "Missing CONFIG_HOST value - aborting"
$       iishlib_err == ii_status_error
$       goto _NODE_NAME_EXIT
$    endif
$    if "''numa_rad'" - " " .nes. ""
$    then
$        iishlib_rv1 == "''numa_rad'_''config'"
$    else
$        iishlib_rv1 == "''config'"
$    endif
$!
$_NODE_NAME_EXIT:
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! cut_and_append :== 'callsh' _cut_and_append
$! Usage:
$!
$! Input:
$! 	P1 	delim		value to use as a field delimeter
$!	P2	fld		Field that is to be selected
$!	P3	src		input file with records from which to cut
$!				field fld from
$!	P4	dst		resultant string of space separated values 
$!				if blank uses iishlib_rv1
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _cut_and_append: SUBROUTINE
$!
$ delim = "''P1'" - "-d"
$ fld = "''P2'" - "-f"
$ src = "''P3'"
$ dst = "''P4'"
$!
$ if dst .eqs. "" 
$ then
$    dst = "iishlib_rv1"
$ endif
$ 'dst'1 = ""
$!
$ OPEN/READ/ERROR=_CUT_O_ERROR srcfile 'src'
$_CUT_NEXT:
$ READ/ERROR=_CUT_R_ERROR/END_OF_FILE=_CUT_LAST srcfile irec
$ 'dst'1 = 'dst'1 + " " + F$ELEMENT("''fld'","''delim'","''irec'")
$ GOTO _CUT_NEXT
$!
$_CUT_LAST:
$ CLOSE srcfile 
$!
$ 'dst' == F$EDIT('dst'1, "COMPRESS, TRIM")
$!
$ goto _CUT_EXIT
$!
$_CUT_R_ERROR:
$ CLOSE srcfile 
$!
$_CUT_O_ERROR:
$ iishlib_err == ii_status_error
$!
$_CUT_EXIT:
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$!#=[ gen_nodes_lists ]============================================
$!#
$!#       Utility routine to generate list of existing cluster
$!#       nodes (NODESLST), and a list (same order) of node
$!#       number assignments (IDSLST).
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _gen_nodes_lists: SUBROUTINE
$!
$   self = "''self'"
$   fileext = f$cvtime() - "-" - "-" - " " - ":" - ":" - "."
$   tmp_file = "II_CONFIG:tmp_config.dat_''fileext'"
$!  # Get list of existing nodes and node numbers.
$!
$!  Initialize global variables
$!
$   'self'NODESLST      == ""
$   'self'IDSLST        == ""
$!
$   PIPE SEARCH/NOWARN/NOLOG/OUTPUT='tmp_file' II_CONFIG:config.dat "config.cluster.id" > NL:
$   cut_and_append "-d." "-f1" "''TMP_FILE'" "''self'NODESLST" 
$   cut_and_append "-d:" "-f1" "''TMP_FILE'" "''self'IDSLST"
$   if 'self'NODESLST .eqs. "" 
$   then 
$      NODESLST_dsp = "No nodes defined"
$   else
$      NODESLST_dsp = 'self'NODESLST
$   endif
$   if 'self'IDSLST .eqs. "" 
$   then 
$      IDSLST_dsp   = "No nodes defined"
$   else
$      IDSLST_dsp = 'self'IDSLST
$   endif
$   info_box "Current list of nodes"  -
	"NODESLST   =''NODESLST_dsp'"   -
	"IDSLST     =''IDSLST_dsp'" 
$!
$   DELETE/NOLOG 'tmp_file';*
$   iishlib_rv1 == 'self'NODESLST
$   iishlib_rv2 == 'self'IDSLST
$   iishlib_err == ii_status_ok
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$!#=[ get_node_number ]=============================================
$!#
$!#       Utility routine to prompt for a node number
$!#
$!#       Params: $1 = RAD number or blank if regular cluster.
$!#
$!#       Data:   IDSLST - space delimited list of taken node id's.
$!#               NODESLST - space delimited list of nodes for
$!#                          each ID in IDSLST, in same order.
$!#               CLUSTER_MAX_NODE - Max valid Node ID
$!#
$!#       Outputs: II_CLUSTER_ID set to valid node ID, or cleared if no input.
$!#                NODESLST      updates to include new node
$!#               IDSLST         updates to include new node ID
$!#
$! Side effects
$!         allowdefaults is not implemented on VMS
$!------------------------------------------------------------------------
$ _get_node_number: SUBROUTINE
$!
$    self 	= "''P1'"
$    numa_rad 	= F$EDIT("''P2'","UPCASE")
$    host     	= F$EDIT("''P3'","LOWERCASE")
$!
$    local_err == ii_status_ok
$    iishlib_err == ii_status_ok
$!
$!   Clear any existing logical for II_CLUSTR_ID
$!
$_GET_NODE_CLEAR_LOG:
$!
$    rm_log = F$TRNLNM("II_CLUSTER_ID",,,,,"TABLE_NAME") 
$    if rm_log .nes. ""
$    then 
$       value = F$TRNLNM("II_CLUSTER_ID") 
$	$DEBUGIT box " Found II_CLUSTER_ID=''value' in table ''rm_log' - deleting."
$       set noon
$       iishlib_messages_off
$       value = F$TRNLNM("II_CLUSTER_ID","''rm_log'",,"SUPERVISOR") 
$	if value .nes. "" then DEASIGN/SUPER/TABLE='rm_log'/SUPER II_CLUSTER_ID
$       value = F$TRNLNM("II_CLUSTER_ID","''rm_log'",,"EXECUTIVE") 
$	if value .nes. "" then DEASIGN/SUPER/TABLE='rm_log'/EXEC II_CLUSTER_ID
$       value = F$TRNLNM("II_CLUSTER_ID","''rm_log'",,"USER") 
$	if value .nes. "" then DEASIGN/SUPER/TABLE='rm_log'/USER II_CLUSTER_ID
$       iishlib_messages_on
$       goto _GET_NODE_CLEAR_LOG
$    endif
$!
$    II_CLUSTER_ID==""
$    ii_clu_id = 0
$!
$    IDSLST = 'self'IDSLST
$    gen_ok_set "''IDSLST'" "''CLUSTER_MAX_NODE'"
$    okdefault = F$INTEGER(iishlib_rv1)
$    okset     = iishlib_rv2
$!
$    if okdefault .EQ. 0
$    then
$        error_box "Maximum number of nodes (''CLUSTER_MAX_NODE') for an " -
                  "Ingres cluster have already been configured." -
                  " " "Aborting setup."
$        iishlib_err == ii_status_error
$        goto _GEN_NODE_NUM_EXIT
$    endif
$!
$_GEN_NODE_NUM_INIT:
$ if ii_clu_id .EQ. 0 .OR. ii_clu_id .EQS. ""
$ then
$    resfile = 'self'RESINFILE
$    if 'self'READ_RESPONSE .eq. true
$    then  !  # Get CLUSTER ID from response file.
$       iiread_response """CLUSTER_NODE_ID${1}""" "''resfile'"
$       ii_clu_id = F$INTEGER(iishlib_rv1)
$       iishlib_err == ii_status_ok
$    else 
$       if 'self'BATCH .eq. true
$       then  !  # Regular batch must use default
$          ii_clu_id = okdefault
$          iishlib_err == ii_status_ok
$       else !   # Get CLUSTER ID interactively
$_GEN_NODE_NUM_USER_NEXT:
$          resp = okdefault
$          inquire resp " Please enter an available node number from the set (''okset'). [''okdefault'] "
$	   if resp - " " .eqs. "" 
$	   then
$	      resp = okdefault
$	   endif
$          GETRES = F$INTEGER(resp)
$	   info_box "USER entered [''resp'] using ''GETRES'" 
$          if GETRES .LT. 1 .or. GETRES .GT. CLUSTER_MAX_NODE  
$          then 
$             goto _GEN_NODE_NUM_USER_NEXT
$          endif
$!
$          ii_clu_id = "''GETRES'"
$          ii_clu_id = F$INTEGER(ii_clu_id)
$!
$          if ii_clu_id .EQ. 0 
$          then 
$             ii_clu_id = "''okdefault'"
$             ii_clu_id = F$INTEGER(ii_clu_id)
$          endif
$          iishlib_err == ii_status_ok
$!
$       endif
$    endif
$!
$    ii_clu_id = F$INTEGER(ii_clu_id)
$    ii_clu_id2 = F$FAO("!2ZL",ii_clu_id) 
$! 
$    if iishlib_err .eq. ii_status_ok 
$    then
$       IDSLST = 'self'IDSLST
$       element_index "''IDSLST'" "''ii_clu_id'" "''self'"
$       if iishlib_err .eqs. ii_status_warning
$       then
$	   iishlib_err == ii_status_ok
$          GOTO _GEN_NODE_NUM_VALID
$	endif
$       GETRES = iishlib_rv1
$       label = F$ELEMENT(GETRES," ",'self'NODESLST)
$       warn_box "Node number """"''ii_clu_id'"""" is already in use by node ''label'."
$!
$       ii_clu_id=""
$       iishlib_err == ii_status_warning
$       local_err == ii_status_warning
$       goto _GEN_NODE_NUM_INIT
$    else
$       iishlib_err == ii_status_ok
$    endif
$!
$    if 'self'READ_RESPONSE .EQ. true .and. "''ii_clu_id'" .eqs. ""
$    then
$       error_box "Response file " - 
            "  ''resfile'" - 
            "specified a CLUSTER_NODE_ID which conflicted with an existing node."
$       iishlib_err == ii_status_error
$    endif
$!
$    if iishlib_err .NE. ii_status_ok
$    then
$        message_box "Unable to continue due to previous errors"
$        GOTO _GEN_NODE_NUM_EXIT
$    endif
$!    
$_GEN_NODE_NUM_VALID:
$!
$!    We have valid II_CLUSTER_ID now.
$!
$    check_type = F$TYPE(ii_clu_id)
$    if check_type .eqs. "INTEGER"
$    then
$       ii_clu_id = F$INTEGER(ii_clu_id)
$       II_CLUSTER_ID == ii_clu_id
$    else
$       message_box "Error processing """"II_CLUSTER_ID""""" "Aborting processing"
$       iishlib_err == ii_status_error
$    endif
$!
$!   Augment node if a niuma cluster.
$!
$    node_name "''self'" "''numa_rad'" "''CONFIG_HOST'"
$    node = "''iishlib_rv1'"
$!
$    if 'self'WRITE_RESPONSE .eq. true
$    then
$!       # configured than Ingres instance used to generate
$!       # the response file.
$        resfile = 'self'RESINFILE
$        wresponse = 'self'WRITE_RESPONSE
$        check_response_write   "''wresponse'"   "''resfile'"   "replace"
$    endif
$!
$!   Add the current node to the nodes list
$    nlst   	= 'self'NODESLST
$    ilst   	= 'self'IDSLST
$!
$    'self'NODESLST == F$EDIT("''nlst' ''node'","LOWERCASE")
$    'self'IDSLST   == F$EDIT("''ilst' ''II_CLUSTER_ID'","LOWERCASE")
$!
$!   Prove that the values have bee passed OK
$!
$    nlst	= 'self'NODESLST
$    ilst	=  'self'IDSLST 
$    info_box "Nodes Currently assigned:-" -
	"NODESLST =''nlst'" -
	"IDSLST   =''ilst'"
$!
$!   return values to the caller.
$!
$    iishlib_rv1 == II_CLUSTER_ID
$    iishlib_rv2 == node
$    iishlib_rv3 == local_err
$    iishlib_rv4 == nlst
$    iishlib_rv5 == ilst
$    iishlib_err == ii_status_ok
$ else
$!
$!   Return Error to caller.
$!
$    iishlib_rv1 == ""
$    iishlib_err == ii_status_error
$ endif
$!
$_GEN_NODE_NUM_EXIT:
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$!	P1	self		caller program
$!	P2	sub1		string to be replaced
$!	P3	sub2		replacement string
$!	P4 	usefile		file or string to act upon
$!	
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _substitute_string: SUBROUTINE
$!
$ self          = F$EDIT(P1,"LOWERCASE") 
$ sub1          = F$EDIT(P2,"LOWERCASE") 
$ sub2          = F$EDIT(P3,"LOWERCASE") 
$ usefile       = F$EDIT(P4,"LOWERCASE") 
$!
$ iishlib_err == ii_status_ok
$!
$ if F$SEARCH("''usefile'") .NES. ""
$ then
$   OPEN/READ/ERROR=_SUBSTITUTE_OPEN_R_ERR infile 'usefile'
$   OPEN/WRITE/ERROR=_SUBSTITUTE_OPEN_W_ERR outfile 'usefile'_tmp
$!
$_SUBSTITUTE_NEXT_IREC:
$   READ/END_OF_FILE=_SUBSTITUTE_EOF infile irec
$   l_irec = F$LENGTH(irec)
$   p_part1 = F$LOCATE(sub1,irec)
$   if p_part1 .ne. l_irec
$   then
$      if p_part1 .ne. 0
$      then
$         v_part1 = F$EXTRACT(0,'p_part1',"''irec'")
$      else
$         v_part1 = ""
$      endif
$      v_part2 = irec - "''v_part1'" - "''sub1'"
$      orec    = v_part1 + sub2 + "''v_part2'"
$      WRITE outfile "''orec'"
$   else
$      iishlib_err == ii_status_warning
$   endif
$   GOTO _SUBSTITUTE_NEXT_IREC
$ else
$   irec = usefile
$   l_irec = F$LENGTH(irec)
$   p_part1 = F$LOCATE(sub1,irec)
$   if p_part1 .ne. l_irec
$   then
$      v_part1 = F$EXTRACT(0,p_part1-1,irec)
$      v_part2 = irec - v_part1 - sub1
$      iishlib_rv1 == v_part1 + sub2 + v_part2
$   else
$      iishlib_err == ii_status_warning
$      iishlib_rv1 == usefile
$   endif
$   GOTO _SUBSTITUTE_EXIT
$ endif
$!
$_SUBSTITUTE_EOF:
$ CLOSE infile
$ CLOSE outfile
$ RENAME 'usefile'_tmp 'usefile'
$ PURGE 'usefile'
$ $DEBUGIT TYPE 'usefile'
$!
$ GOTO _SUBSTITUTE_EXIT
$!
$_SUBSTITUTE_OPEN_W_ERR:
$ iishlib_err == ii_status_warning
$ close infile
$ GOTO _SUBSTITUTE_EXIT
$!
$_SUBSTITUTE_OPEN_R_ERR:
$ iishlib_err == ii_status_warning
$ error_box "Problems occured accessing the work files ''usefile'"
$ GOTO _SUBSTITUTE_EXIT
$!
$_SUBSTITUTE_EXIT:
$ EXIT
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _check_user: SUBROUTINE 
$!
$!   Get "ingres" user since user no longer needs to literally be 'ingres'.
$!
$ iishlib_err == ii_status_ok
$!
$ user_name     = f$trnlnm( "II_SUINGRES_USER","LNM$JOB" )
$!
$ if "''user_name'".EQS.""
$ then
$    isa_uic = f$file_attributes( "II_SYSTEM:[000000]INGRES.DIR", "UIC")
$    if isa_uic .NES. user_uic
$    then
$       error_box "This setup procedure must be run from the Ingres System " - 
                  "Administrator account."
$       iishlib_err == ii_status_error
$    else 
$       iishlib_err == ii_status_ok
$    endif
$ endif
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$! Single routine to get the first 1 through 5 elements of a string
$! The string may be the first record in a file [ P1 ] or string [ P1 ]
$! The default delimeter is a space [ " " ] , the first character of P2
$! is used as the delimeter.
$!
$! If a filename is given and the file exists and can be accessed
$! all records are read and the total number returned.
$!
$! Input:
$!
$!	P1	filename or string to process
$!	P2	delimeter - default = space
$!
$! Output
$!
$!	iishlib_rv1 - count of records in a file
$!		      0 if a string was detected
$!		      ii_status_error (negative) on error
$!	iishlib_rv2 - uncompressed version of first record
$!	iishlib_rv3 - compressed version of first record
$!		      if input string or first record of file
$!		      are null then the value of
$!		      iishlib_rv2 and iishlib_rv3 will be null
$!		      if the string or first record of file
$!		      contains no white space then the value
$!		      of iishlib_rv2 and iishlib_rv3 will be 
$!		      the same 
$!	iishlib_rv4 - element 0 or null string
$!	iishlib_rv5 - element 1 or null string
$!	iishlib_rv6 - element 2 or null string
$!	iishlib_rv7 - element 3 or null string
$!	iishlib_rv8 - element 4 or null string
$!		      all values returned are the compressed and trimmed
$!		      versions of the input.
$!
$!	iishlib_err - not used, error status returned as iishlib_rv1
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _chop_and_count: SUBROUTINE
$!
$ irec = "''P1'"
$ cnt  = 0
$ if F$SEARCH(P1) .NES. ""
$ then
$    open/read/error=_CHOP_AND_COUNT_ERROR infile 'P1' 
$    read/error=_CHOP_AND_COUNT_ERROR_EXIT/end=_CHOP_AND_COUNT_EOF infile irec
$    cnt = cnt + 1
$ endif
$!
$_CHOP_AND_COUNT_ERROR:
$!
$ dlm = F$EXTRACT(0,1,P2)
$ if dlm .eqs. "" then dlm = " "
$ iishlib_rv1 == cnt
$ iishlib_rv2 == irec
$ irec = F$EDIT(irec,"COMPRESS,TRIM")
$ iishlib_rv3 == irec
$ iishlib_rv4 == F$ELEMENT(0,dlm,irec) - dlm
$ iishlib_rv5 == F$ELEMENT(1,dlm,irec) - dlm
$ iishlib_rv6 == F$ELEMENT(2,dlm,irec) - dlm
$ iishlib_rv7 == F$ELEMENT(3,dlm,irec) - dlm
$ iishlib_rv8 == F$ELEMENT(4,dlm,irec) - dlm
$ if cnt .eq. 0 then goto _CHOP_AND_COUNT_EXIT
$!
$_CHOP_AND_COUNT_NEXT:
$!
$ read/error=_CHOP_AND_COUNT_ERROR_EXIT/end=_CHOP_AND_COUNT_EOF infile irec
$ cnt = cnt + 1
$!
$ goto _CHOP_AND_COUNT_NEXT
$!
$_CHOP_AND_COUNT_EOF:
$ iishlib_rv1 == cnt
$ goto _CHOP_AND_COUNT_EXIT
$!
$_CHOP_AND_COUNT_ERROR_EXIT:
$ iishlib_rv1 == ii_status_error
$ goto _CHOP_AND_COUNT_EXIT
$!
$_CHOP_AND_COUNT_EXIT:
$ close infile
$ exit 
$!
$ ENDSUBROUTINE
$!
$!
$!
$!------------------------------------------------------------------------
$!
$!
$!
$!------------------------------------------------------------------------
$ _check_sanity: SUBROUTINE 
$!
$!   Check if Cluster is supported.
$ if test .eq. 0
$ then
$ if .NOT. f$getsyi("cluster_member") 
$ then
$     message_box "Your environment cannot support the Ingres Grid " -
                  "option (clustered Ingres)."
$       iishlib_err == ii_status_error
$ else 
$       info_box "Your environment can support the Ingres Grid " -
                  "option (clustered Ingres)."
$       iishlib_err == ii_status_ok
$ endif
$ endif
$!
$ ii_system = f$trnlnm("II_SYSTEM") 
$ if ii_system .EQS. ""
$ then
$    message_box "The Ingres logical """"II_SYSTEM"""" is not defined."
$       iishlib_err == ii_status_error
$ else 
$       iishlib_err == ii_status_ok
$ endif
$!
$ exit
$!
$ ENDSUBROUTINE
$!
$!#
$!# Add a server class based on default dbms class def in 1st node
$!#
$!#   $1 = calling program - passed on to further routines
$!#   $2 = target file (config.dat | protect.dat )
$!#   $3 = First Node 
$!#   $4 = Dest node for new server class definition
$!#   $5 = sever class name.
$!#   $6 = optional file extension.
$!#
$ _append_class: SUBROUTINE
$!
$    self     = F$EDIT("''p1'","LOWERCASE") 
$    fstnode  = F$EDIT("''p2'","LOWERCASE")
$    cf_file  = F$EDIT("''p3'","LOWERCASE") 
$    newnode  = F$EDIT("''p4'","LOWERCASE")
$    svrclass = F$EDIT("''p5'","TRIM,LOWERCASE")
$    ext      = F$EDIT("''p6'","TRIM,LOWERCASE")
$!
$    if svrclass - " " .EQS. "" 
$    then
$       error_box "Mandatory value """"serverclass"""" is blank " 
$       iishlib_err == ii_status_error
$    endif 
$!
$    if fstnode .EQS. "" 
$    then
$       error_box "Mandatory value - first node is requied"
$       iishlib_err == ii_status_error
$    endif 
$!
$    PIPE SPEC = F$SEARCH("II_CONFIG:''cf_file'''ext'") > NL:
$    if SPEC .EQS. ""
$    then
$       error_box "Cannot access II_CONFIG:''cf_file'''ext' - No such file" -
                  "Please check that this directory is accessible from ''newnode'."
$       iishlib_err == ii_status_error
$    else 
$       $DEBUGIT box "''cf_file' = ''SPEC'"
$       pattern="ii.''fstnode'.dbms.*."
$       PIPE SEAR/NOWARN/NOLOG		  		-
		/OUTPUT=II_CONFIG:tmp_'cf_file''ext' 	-
	    	        II_CONFIG:'cf_file''ext' 	-
		"''pattern'" > NL:
$       $DEBUGIT info_box "pattern    = ''pattern'"
$       $DEBUGIT TYPE/NOHEAD 	      II_CONFIG:tmp_'cf_file''ext'
$!
$       PIPE SEAR/NOWARN/MATCH=NOR/NOLOG  		-
		/OUTPUT=II_CONFIG:tmp2_'cf_file''ext' 	-
	    		II_CONFIG:tmp_'cf_file''ext' 	-
		"dmf_connect" > NL: 
$       $DEBUGIT info_box "pattern    = .''svrclass'. - EXCLUDE"
$       $DEBUGIT TYPE/NOHEAD II_CONFIG:tmp2_'cf_file''ext'
$!
$	substitute_string "''self'"   ".*."	".''svrclass'." 	-
		II_CONFIG:tmp2_'cf_file''ext' 
$       $DEBUGIT TYPE/NOHEAD II_CONFIG:tmp2_'cf_file''ext'
$!
$	$DEBUGIT chop_and_count II_CONFIG:'cf_file'
$       $DEBUGIT info_box "II_CONFIG:''cf_file'" "wc = ''iishlib_rv1'"
$!
$       TYPE/NOHEAD/OUTPUT=II_CONFIG:new_'cf_file''ext' -
			   II_CONFIG:'cf_file', 	-
		           II_CONFIG:tmp2_'cf_file''ext'
$!
$	$DEBUGIT chop_and_count II_CONFIG:tmp_'cf_file''ext'
$       $DEBUGIT info_box "II_CONFIG:tmp_''cf_file'''ext'" "wc = ''iishlib_rv1'"
$	$DEBUGIT chop_and_count II_CONFIG:tmp2_'cf_file''ext'
$       $DEBUGIT info_box "II_CONFIG:tmp2_''cf_file'''ext'" "wc = ''iishlib_rv1'"
$	$DEBUGIT chop_and_count II_CONFIG:new_'cf_file''ext'
$       $DEBUGIT info_box "II_CONFIG:new_''cf_file'''ext'" "wc = ''iishlib_rv1'"
$!
$       PURGE/NOLOG/KEEP=1  	II_CONFIG:*'cf_file'* 
$       RENAME/NOLOG  		II_CONFIG:new_'cf_file''ext'  II_CONFIG:'cf_file'
$       PURGE/NOLOG/KEEP=20     II_CONFIG:'cf_file' 
$       DELETE/NOLOG        	II_CONFIG:tmp_'cf_file''ext';0
$       DELETE/NOLOG        	II_CONFIG:tmp2_'cf_file''ext';0
$!
$	$DEBUGIT chop_and_count II_CONFIG:'cf_file'
$       $DEBUGIT info_box "II_CONFIG:''cf_file'" "wc = ''iishlib_rv1'"
$!
$	getressym "''pattern'" "cache_sharing"
$	VALUE = iishlib_rv1
$	if VALUE .eqs. "on" .OR. VALUE .eqs. "ON" 
$       then
$          getressym "''pattern'" "cache_name"
$	   VALUE = iishlib_rv1
$!
$          pattern="ii.''fstnode'.dbms.shared.''VALUE'."
$          PIPE SEARCH/NOWARN/NOLOG			-
		/OUTPUT=II_CONFIG:tmp2_'cf_file''ext' 	-
 		 	II_CONFIG:'cf_file''ext' 	-
		"''pattern'" > NL: 
$          $DEBUGIT info_box "pattern    = ''pattern' - EXCLUDE"
$          $DEBUGIT TYPE/NOHEAD II_CONFIG:tmp2_'cf_file''ext'
$!
$	   substitute_string "''self'" ".''VALUE'." ".''svrclass'." II_CONFIG:tmp2_'cf_file''ext' 
$       else
$          pattern="ii.''fstnode'.dbms.private.*."
$          PIPE SEAR/NOWARN/NOLOG			-
		/OUTPUT=II_CONFIG:tmp2_'cf_file''ext' 	-
 		 	II_CONFIG:'cf_file''ext' 	-
		"''pattern'" > NL: 
$          $DEBUGIT info_box "pattern    = ''pattern' - EXCLUDE"
$          $DEBUGIT TYPE/NOHEAD II_CONFIG:tmp2_'cf_file''ext'
$!
$	   substitute_string "''self'" ".*." ".''svrclass'." II_CONFIG:tmp2_'cf_file''ext' 
$          $DEBUGIT TYPE/NOHEAD II_CONFIG:tmp2_'cf_file''ext'
$!
$       endif
$	$DEBUGIT chop_and_count II_CONFIG:'cf_file'
$       $DEBUGIT info_box "II_CONFIG:''cf_file'" "wc = ''iishlib_rv1'"
$!
$       $DEBUGIT TYPE/NOHEAD II_CONFIG:tmp2_'cf_file''ext'
$       TYPE/NOHEAD/OUTPUT=II_CONFIG:new_'cf_file''ext' -
			  II_CONFIG:'cf_file', 		-
			  II_CONFIG:tmp2_'cf_file''ext'
$	$DEBUGIT chop_and_count II_CONFIG:tmp2_'cf_file''ext'
$       $DEBUGIT info_box "II_CONFIG:tmp2_''cf_file'''ext'" "wc = ''iishlib_rv1'"
$	$DEBUGIT chop_and_count II_CONFIG:new_'cf_file''ext'
$       $DEBUGIT info_box "II_CONFIG:new_''cf_file'''ext'" "wc = ''iishlib_rv1'"
$!
$	$DEBUGIT chop_and_count II_CONFIG:'cf_file'
$       $DEBUGIT info_box "II_CONFIG:''cf_file'" "wc = ''iishlib_rv1'"
$!
$    endif
$!
$    PURGE/NOLOG/KEEP=1  	II_CONFIG:*'cf_file'* 
$    RENAME/NOLOG  		II_CONFIG:new_'cf_file''ext'  II_CONFIG:'cf_file'
$    PURGE/NOLOG/KEEP=20        II_CONFIG:'cf_file' 
$    DELETE/NOLOG        	II_CONFIG:tmp2_'cf_file''ext';0
$    iishlib_err == ii_status_ok
$!
$ EXIT
$!
$ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$! Usage:
$!
$! Input:
$!
$! Output
$!
$! Side effects
$!
$!------------------------------------------------------------------------
$ _create_dir: SUBROUTINE
$!
$ svs = STS$K_SUCCESS
$ iishlib_err == ii_status_warning
$!
$ ii_root_dir = F$EDIT(F$TRNLNM("II_CONFIG"),"UPCASE") - ".FILES]" + "]"
$ ii_root_dir = ii_root_dir - "INGRES]" -"]" 
$ if p1 .eqs. "" .or. p2 .eqs. "" 
$ then 
$    warn_box "Failed to create directory: No directory or protection" -
	     "parameter passed in."
$    exit
$ endif
$!
$ if p3 .eqs. "" .and. f$search("''ii_root_dir'ingres]''p2'.dir") .eqs. ""
$ then
$     create/dir/nol 'ii_root_dir'ingres.'p2']
$     svs = $SEVERITY
$     set prot=('p1) 'ii_root_dir'ingres]'p2'.dir
$ else
$     if p4 .eqs. "" .and. p3 .nes. "" .and. -
   f$search("''ii_root_dir'ingres.''p2']''p3'.dir") .eqs. ""
$     then
$         create/dir/nol 'ii_root_dir'ingres.'p2'.'p3']
$	  svs = $SEVERITY
$         set prot=('p1) 'ii_root_dir'ingres.'p2']'p3'.dir
$     else
$         if p5 .eqs. "" .and. p4 .nes. "" .and. -
   f$search("''ii_root_dir'ingres.''p2'.''p3']''p4'.dir") .eqs. ""
$         then
$             create/dir/nol 'ii_root_dir'ingres.'p2'.'p3'.'p4']
$	      svs = $SEVERITY
$             set prot=('p1) 'ii_root_dir'ingres.'p2'.'p3']'p4'.dir
$         endif
$     endif
$ endif
$!
$! Handle errors
$!
$ if svs .ne. STS$K_SUCCESS .and. svs .ne. STS$K_INFO 
$ then 
$    error_box "Failed to create directory ''P2' ''p3' ''p4' "
$    iishlib_err == ii_status_error
$ else
$    info_box "Node specific directories created OK :"
$    iishlib_err == ii_status_ok
$ endif
$!
$ exit
$!
$ endsubroutine
$!
$!
$!
$!#
$!#   rename wrapper to allow suppression of rename.
$!#
$!#     $1 = target.
$!#     $2 = new name
$!#
$ _rename_file: SUBROUTINE
$ iishlib_err == ii_status_ok
$ src_file =  F$SEARCH("''P1'")
$ tgt_file =  F$SEARCH("''P2'")
$ if src_file .eqs. ""
$ then
$   warn_box "Source file missing" -
            "''P2'"
$ endif
$ if tgt_file .nes. ""
$ then
$   info_box "Target file exists" -
            "''P2'"
$ endif
$ if src_file .nes. ""
$ then
$    if FAKEIT .ne. 0
$    then    
$       box  "Skipped mv of: ''P1'" -
             "           to: ''P2'"
$     else
$!
$        ren_file = F$ELEMENT(0,";",src_file)
$        dst_file = F$ELEMENT(0,";",tgt_file)
$        PURGE 'ren_file'
$        RENAME/NOCONF  'ren_file';0 'dst_file'
$	 error_condition $SEVERITY
$        PURGE 'dst_file'
$     endif
$ else
$     box "NOfile: ''P1'"
$     iishlib_err == ii_status_warning
$ endif
$ EXIT
$!
$ ENDSUBROUTIMNE
$!
$!#
$!#   delete wrapper to allow suppression of file deletion.
$!#
$!#     $1 = target.
$!#
$ _remove_file: SUBROUTINE
$ if F$SEARCH("''P1'") .nes. ""
$ then
$    if FAKEIT .ne. 0
$    then    
$       box  "Skipped mv of: ''P1' " -
             "           to: ''P2'"
$    else
$          del_file = F$ELEMENT(0,";",P1)
$          PURGE 'del_file'
$          DELETE/NOCONFIRM/NOLOG  'del_file';* 
$	   error_condition $SEVERITY
$    endif
$ else
$    xbox "NOfile: ''P1'"
$    iishlib_err == ii_status_warning
$ endif
$!
$ EXIT
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!
$! Subroutine:  _selective_purge_on_config
$!
$! Usage:
$! To do a selective purge on CONFIG.DAT.  Parameter P1 contains time
$! or a time stamp (i.e. P1 could be "13:55" or "23-MAY-2007:15:30:45.90")
$! The subroutine does the following:
$!     (a) PURGE file CONFIG.DAT since the time specified in P1
$!     (b) Rename the latest version to previous version plus 1
$!     (c) Compare the last two versions of CONFIG.DAT and if they are
$!         same, delete the latest version
$!
$!
$! Input:
$!	P1	time or a complete time-stamp
$!
$!
$! Output:
$!      None
$!
$!
$! Side effects
$!
$!
$! Future Considerations:
$!      Could be modified to do a selective purge on any file by passing
$!      file location (logical or disk/directory name) as parameter P2
$!      and file name as parameter P3.
$!      In our example, we could pass II_CONFIG as P2 and CONFIG.DAT as P3.
$!
$!------------------------------------------------------------------------
$!
$ _selective_purge_on_config: SUBROUTINE
$!
$ iishlib_err == ii_status_ok
$! Check to see there exists at least two versions of the file.  Otherwise, exit.
$ IF (F$SEARCH("II_CONFIG:CONFIG.DAT;-1") .EQS. "") THEN goto _SELECTIVE_PURGE_ON_CONFIG_EXIT
$ job_start_time = "''P1'"
$ if (job_start_time .eqs. "") then  goto  _SELECTIVE_PURGE_ON_CONFIG_ERROR_EXIT
$!
$ on error then goto _SELECTIVE_PURGE_ON_CONFIG_ERROR_EXIT
$!
$! purge since the time the job started - if the job created
$! ten new config.dat files, keep only the latest
$ purge/nolog/since='job_start_time'    II_CONFIG:CONFIG.DAT
$!
$! rename the latest version to previous version plus 1
$ rename/nolog   II_CONFIG:CONFIG.DAT;    II_CONFIG:CONFIG.DAT;0
$!
$! Check to see there exists at least two versions of the file.  Otherwise, exit.
$ IF (F$SEARCH("II_CONFIG:CONFIG.DAT;-1") .EQS. "") THEN goto _SELECTIVE_PURGE_ON_CONFIG_EXIT
$!
$! Now, check to see if the last two versions are same - if they are,
$! delete the most recent version
$! Compare the file with itself to get the status code for a match
$ diff/output=nla0:  II_CONFIG:CONFIG.DAT   II_CONFIG:CONFIG.DAT
$ exact_match = $STATUS    ! store the status code for exact match to use later
$!
$! Compare the last two versions.  If they are same (by checking the status code
$! of DIFF with exact_match from previous DIFF)
$ PIPE  diff/output=nla0:   II_CONFIG:CONFIG.DAT   >NLA0:   2>&1
$ if $STATUS .EQ. exact_match  then  delete/noconfirm/nolog  II_CONFIG:CONFIG.DAT;
$ goto _SELECTIVE_PURGE_ON_CONFIG_EXIT
$!
$!
$_SELECTIVE_PURGE_ON_CONFIG_ERROR_EXIT:
$ error_box "Error running Selective Purge in IISHLIB"
$ iishlib_err == ii_status_error
$ goto _SELECTIVE_PURGE_ON_CONFIG_EXIT
$!
$_SELECTIVE_PURGE_ON_CONFIG_EXIT:
$ exit 
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!
$!------------------------------------------------------------------------
$!
$! Subroutine:  _pan_cluster_cmd
$!
$! Usage:
$! Invoke the specified command against all the nodes in the installation.
$! The "node" is passed to the command. Here's some example
$!
$!      pan_cluster_cmd aaa,bbb A_CMD "first." ".second"
$!            A_CMD first.aaa.second
$!            A_CMD first.bbb.second
$!
$!      pan_cluster_cmd aaa,bbb A_CMD "first." ".second" 1
$!            A_CMD first.aaa.second aaa
$             A_CMD first.bbb.second bbb
$!
$! Input:
$!	P1	Node list (comma separated)
$!      P2      Command
$!      P3      Pre node parameter info
$!      P4      Post node parameter info
$!      P5      Add node as parameter at end of cmd line
$!
$! Output:
$!      None
$!
$!
$! Side effects
$!
$!      None
$!
$!------------------------------------------------------------------------
$!
$ _pan_cluster_cmd: SUBROUTINE
$!
$ nodes     = P1
$ cmd       = P2
$ pre_node  = P3
$ post_node = P4
$ add_node  = P5
$
$ end_param = ""
$
$ entry = 0
$
$CMD_LOOP:
$ node = f$element(entry, ",", nodes)
$
$ if (node .nes. ",")
$ then
$    if (add_node .nes. "")
$    then
$       end_param = node
$    endif
$
$    'cmd' 'pre_node''node''post_node' 'end_param'
$
$    entry = entry + 1
$
$    goto CMD_LOOP
$ endif
$ exit 
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!
$! Subroutine:  _pan_cluster_set_notdefined
$!
$! Usage:
$! Set the config parameter on all nodes where not previously defined.
$!
$! Input:
$!	P1	Node list (comma separated)
$!      P2      Pre node parameter info
$!      P3      Post node parameter info
$!      P4      value to set
$!
$! Output:
$!      None
$!
$!
$! Side effects
$!
$!      Updates config.dat
$!
$!------------------------------------------------------------------------
$!
$ _pan_cluster_set_notdefined: SUBROUTINE
$!
$ nodes     = P1
$ pre_node  = P2
$ post_node = P3
$ value     = P4
$
$ entry = 0
$
$CMD_LOOP:
$ node = f$element(entry, ",", nodes)
$
$ if (node .nes. ",")
$ then
$    PIPE iigetres 'pre_node''node''post_node' res >NLA0: 2>NLA0:
$    res = f$trnlnm( "res" )
$    if res .eqs. ""
$    then
$       iisetres 'pre_node''node''post_node' "''value'"
$    else
$       deassign "res"
$    endif
$
$    entry = entry + 1
$
$    goto CMD_LOOP
$ endif
$ exit 
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!
$! Subroutine:  _pan_cluster_min_set
$!
$! Usage:
$! Set the config parameter on all nodes where the value is less than
$! the value specified.
$!
$! Input:
$!	P1	Node list (comma separated)
$!      P2      Pre node parameter info
$!      P3      Post node parameter info
$!      P4      value to set
$!
$! Output:
$!      None
$!
$!
$! Side effects
$!
$!      Updates config.dat
$!
$!------------------------------------------------------------------------
$!
$ _pan_cluster_min_set: SUBROUTINE
$!
$ nodes     = P1
$ pre_node  = P2
$ post_node = P3
$ value     = P4
$
$ entry = 0
$
$CMD_LOOP:
$ node = f$element(entry, ",", nodes)
$
$ if (node .nes. ",")
$ then
$    def_val = 0
$    PIPE iigetres 'pre_node''node''post_node' res >NLA0: 2>NLA0:
$    res = f$trnlnm( "res" )
$    if res .eqs. ""
$    then
$       def_val = 1
$    else
$       deassign "res"
$
$       if res .lt. value
$       then
$          def_val = 1
$       endif
$    endif
$
$    if def_val
$    then
$       iisetres 'pre_node''node''post_node' 'value'
$    endif
$
$    entry = entry + 1
$
$    goto CMD_LOOP
$ endif
$ exit 
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!
$! Subroutine:  _pan_cluster_iigetres
$!
$! Usage:
$! Get the config.dat values for each node. The varname
$! provided is the name of the global variables to define.
$! For example, if the nodes are "aaa,bbb" and the varname
$! is my_var, then the symbols
$!        my_var_aaa  and my_var_bbb will be defined.
$!
$! Input:
$!      P1      Identity of the utility invoking script
$!	P2	Node list (comma separated)
$!      P3      Pre node parameter info
$!      P4      Post node parameter info
$!      P5      varname
$!
$! Output:
$!      None
$!
$! Side effects
$!
$!      Defined global symbols
$!
$!------------------------------------------------------------------------
$!
$ _pan_cluster_iigetres: SUBROUTINE
$!
$ script_id = P1
$ nodes     = P2
$ pre_node  = P3
$ post_node = P4
$ varname   = P5
$
$ entry = 0
$
$CMD_LOOP:
$ node = f$element(entry, ",", nodes)
$
$ if (node .nes. ",")
$ then
$    PIPE iigetres 'pre_node''node''post_node' res >NLA0: 2>NLA0:
$    res = f$trnlnm( "res" )
$    if res .nes. ""
$    then
$       deassign "res"
$    endif
$
$    'varname'_'node' == res
$
$    'script_id'_gbl_syms == 'script_id'_gbl_syms + ",''varname'_''node'"
$
$    entry = entry + 1
$
$    goto CMD_LOOP
$ endif
$
$ exit 
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!
$! Subroutine:  _get_nodes
$!
$! Usage:
$! Returns a list of nodes for the installation.
$!
$! Input:
$!      P1 - Current node.
$!
$! Output:
$!      iishlib_rv1 = list of nodes (comma separated)
$!      iishlib_rv2 = cluster mode
$!
$!
$! Side effects
$!
$!      None
$!
$!------------------------------------------------------------------------
$!
$ _get_nodes: SUBROUTINE
$!
$ iishlib_err == ii_status_ok
$ cluster_mode = "OFF"
$
$ node = f$edit(P1, "lowercase")
$
$ iishlib_rv1 == node
$
$ pid = F$GETJPI("","PID")
$
$ tmpfile = "II_SYSTEM:[ingres]get_nodes.''pid'"
$
$ on control_y then goto abort
$ on error     then goto abort
$
$ ! Get the installation node members
$ SEARCH/NOWARN/OUT='tmpfile ii_system:[ingres.files]config.dat config.cluster_mode
$
$ open tmp 'tmpfile
$CLUSTER_NODES:
$ read/end_of_file=CLUSTER_NODES_DONE tmp record
$ mode = f$edit(f$element( 1, ":", record), "upcase,collapse")
$ c_node =  f$edit(f$element(1, ".", record), "lowercase")
$ if mode .eqs. "ON"
$ then
$    if c_node .nes. node
$    then
$       iishlib_rv1 == iishlib_rv1 + ",''c_node'"
$    endif
$
$    cluster_mode = mode
$ endif
$
$ goto CLUSTER_NODES

$abort:
$ iishlib_err == ii_status_error
$
$CLUSTER_NODES_DONE:
$ if f$trnlnm( "tmp" ) .nes. "" then close tmp
$ if f$search( "''tmpfile'") .nes. "" then delete 'tmpfile;*
$
$ iishlib_rv2 == cluster_mode
$
$ exit 
$!
$ ENDSUBROUTINE
$!
$!------------------------------------------------------------------------
$!
$! Subroutine:  _display_nodes
$!
$! Usage:
$! Outputs the node list in a tabulated form
$!
$! Input:
$!      P1 - Node list
$!
$! Output:
$!
$!
$! Side effects
$!
$!      None
$!
$!------------------------------------------------------------------------
$!
$ _display_nodes: SUBROUTINE
$!
$ nodes = P1
$
$ entry = 0
$ sep = ""
$ last_node = ""
$ node_list = ""
$NODE_LIST:
$ node = f$edit(f$element(entry, ",", nodes), "UPCASE")
$ if node .nes. ","
$ then
$    entry = entry + 1
$
$    node_list = node_list + sep
$
$    if f$length( node_list ) .gt. 40
$    then
$       echo "	''node_list'"
$       node_list = ""
$    endif
$
$    if (last_node .nes. "")
$    then
$       node_list = node_list + last_node
$
$       sep = ", "
$    endif
$
$    last_node = node
$
$    goto NODE_LIST
$ endif
$
$ node_list = node_list + " and "
$
$ if f$length( node_list ) .gt. 40
$ then
$    echo "	''node_list'"
$    node_list = ""
$ endif   
$
$ node_list = node_list + last_node
$
$ echo "	''node_list'"
$
$ exit 
$!
$ ENDSUBROUTINE
$!
