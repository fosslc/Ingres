
/*
* * Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	errw.msg -	Message file for class RW
**
** Description:
**	This file defines the messages for class RW.
**
** History:
**	17-aug-1987 (peterk)
**		Created.
**	9/20/88 (elein)
**		B3376 Added error message E_RW13E9 for rbf default reports.
**      12-dec-88 (sylviap)
**              Added error E_RW1084_.  In 6.1, adf does not support sum or avg
**                 of a date datatype.  Called from r_p_agg.
**              Changed msg 13E9 to say 'characters wide', not 'columns wide'.
**      29-jun-89 (sylviap)
**		Added -o (output) and -n (number of copies) to all the REPORT
**		   command line flags errors.
**		Created E_RW13EC and E_RW13ED for direct printing.
**      26-jul-89 (sylviap)
**		Added E_RW13F0 - for bug 7011.
**      14-aug-89 (sylviap)
**		Added E_RW13F1 for scrollable output help file title.
**	08/16/89 (elein) garnet
**		* Changed wording on 13CB to be more general.  This message
**		can now occur on commands other than printing.
**		* E_RW13F2/4/5 message added for general error of 
**		invalid parameters to commands.
**		* Changed wording on E_RW13F0 to mention that values
**		may be passed on the command line
**		* Added E_RW13F3 for missing argument to right/left
**		margin commands.
**		* Added E_RW13F6 for ending a runtime session due to
**		errors.
**		* Changed title of E_RW13EF because it was too long.
**      28-aug-89 (sylviap)
**		Added E_RW13F7 - for bug 7584 (DG bug).
**      11-sep-89 (sylviap)
**		Added -6 flag to all the syntax error messages.
**      06-oct-89 (sylviap)
**		Fixed E_RW0036_r_nxt_set__Bad_break_ to use %c rather than %s.
**		Was getting 'Internal error' msg.  Added E_RW13F9_Bad_Break
**		to be called rather than E_RW0036.
**      13-oct-89 (sylviap)
**              Added E_RW13FA because RW is no longer supporting the -c flag.
**		Took out the -c in RW's syntax error msgs.
**    10/27/89 (elein)
**            Added E_RW13FB_Bad_Sort_Direction
**	11/1/89 (elein) (jb4642)
**		Removed word "non-fatal" from E_RW100A since any number
**		of errors causes RW to stop.
**	11/3/89 (elein) (garnet)
**		Added -d flag to syntax error message
**      01-dec-89 (sylviap)
**              Added E_RW13FC - 13FE for batch reporting.
**		Added the -d (setup) flag to the REPORT syntax errors. 
**	12/8/89 (martym)  (garnet)
**      	Added F_RW0007_Report_on_JoinDef_   "Report on JoinDef: ".
**	12/21/89 (elein)
**		Corrected msgs which are missing param number.  Note that
**		the messages used by IIUG routines require the param number.
**		The DOS group found some of these.
**		E_RW0041, E_RW0004, E_RW000C, E_RW000E, E_RW0018, E_RW0019,
**		E_RW0021, E_RW0024, E_RW0033, E_RW0034, E_RW0035, E_RW0040
**		E_RW0043, E_RW004C
**      02-jan-90 (sylviap)
**		Added E_RW13FF to make E_RW1079 obsolete.  All the parameters
**		 for 1079 were not being filled in when it was called.
**		Added S_RW1400 for the title of the output browser.
**	30-jan-90 (sylviap)
**		Added -6 to RW's syntax error message. (US #423)
**		Took out the -i to COPYREP's syntax error message. (US #344)
**		Took out extra parenthesis in E_RW139F error message. (US #426)
**		Added tabular, indented and labels to E_RW12C5 as valid modes.
**	06-feb-90 (sylviap)
**		Added more information to E_RW1389. (US #409)
**		Added E_RW1401. (US #501)
**	2/15/90 (elein)
**		forgot history for last change: change ... to . . .
**		except in syntax msgs
**	02-mar-90 (sylviap)
**		Added E_RW1402 for jupbug #9629 and US #157.
**	26-mar-90 (sylviap)
**		Changed E_RW10D1 for US #724.
**	3/26/90 (martym)
**		Added "[-p] [-GgroupID]" to REPORT, RBF, COPYREP, and 
**		SREPORT command line flag messages.
**	10-apr-90 (sylviap)
**		Changed wording for 106B.
**		Added E_RW1403 - E_RW1406.
**      23-apr-90 (sylviap)
**              Added E_RW1407_ for jupbug #21354, #US #347.
**	01-may-90 (elein)
**		Added E_RW140A for bad break names found at runtime.
**      01-may-90 (sylviap)
**		Added S_RW1408 - S_RW1409 for scrollable output popup titles.
**      17-may-90 (sylviap)
**		Added E_RW140B. (#957)
**	5/18/90 (elein)
**		Changed \n & spacing on E_RW13BE
**      08-jun-90 (sylviap)
**		Added E_RW140C. (#969)
**	7/18/90 (elein)
**		Changed syntax message for .DECLARE to include
**		WITH VALUE "value string"-- new option
**	8/6/90 (elein) b32098
**		New messages for invalid .LET and parameters
**		and overflowing integers
**	8/30/90 (elein) b32850
**		Add exception messages for invalid parameter assignments
**	23-oct-1990 (mgw)
**		Corrected some misspellings.
**      06-nov-90 (sylviap)
**		Added E_RW1413_. (#33894) 
**	3/15/91 (elein) 35655
**		Improve error RW13F0 so that users know that they cant
**		use .LET to initialize variables for some commands.
**	8/12/91 (steveh) 38434
**		Clarified error message E_RW13ED_Error_printing.
**	12-jun-92 (rdrane)
**		Add messages used by the DELOBJ utility.
**	7-oct-1992 (rdrane)
**		Fix-up REPORT command line messages to include new
**		-ansi92 flag.
**	26-oct-92 (rdrane)
**		Remove DELOBJ messages.  Add "attribute is unsupported
**		datatype" message (E_RW1414_).
**	4-nov-1992 (rdrane)
**		Back-out the command line message change to include
**		-ansi92 flag.  There's really no practical reason to
**		have it be command-line enabled.
**	9-dec-1992 (rdrane)
**		Add "Decimal overflow in expr ..." message (E_RW1415_).
**	21-dec-1992 (rdrane)
**		Add r_m_dsort() failure message (E_RW_1416_).
**	2-feb-1993 (rdrane)
**		Correct E_RW1414_.  Used w/ FEmsg(), so needs to have
**		parameters specified in printf() format.
**	17-mar-1993 (rdrane)
**		Clarified msg E_RW112D_ and made it specific to .BLOCK
**		mode.  Added msg E_RW112E_ for the non-.BLOCK mode.
**		These changes address bug 49749.
**	1-jun-1993 (rdrane)
**		Add msg E_RW10A7_No_DelimID.
**	22-jun-1993 (rdrane)
**		Fix-up REPORT command line messages to include new
**		-nofirstff/-firstff flags.
**	7-jul-1993
**		Modify "attribute is unsupported datatype" message (E_RW1414_)
**		to function with IIUGerr() rather than FEmsg().  Remove "or
**		have been pre-deleted" from E_RW1416_, since RBF ChooseColumns
**		prohibits continuing if all column are pre-deleted, and ignores
**		unsupported datatypes.
**	12-jul-1993
**		Modify "attribute is unsupported datatype" messages (E_RW1414_
**		and E_RW1416_) wording so as not to be quite so brusque.
**	21-jul-1993
**		Modify "attribute is unsupported datatype" messages (E_RW1414_
**		and E_RW1416_) so that the line breaks are such that no
**		"extra" wrapping occurs.
**	17-dec-1996 (angusm)
**		Add new message E_RW1417 for use of II_APP_DECIMAL with RW
**		(bug 75576).
**      28-apr-2004 (srisu02)
**             Changes for addition of new flag "numeric_overflow=ignore|fail|warn"
**	25-Jul-2007 (drivi01)
**	       Updated error E_RW1323_ error to provide clearer message 
**	       on Vista.
**/

# define	_RW_CLASS	94
/*
**	raacc.c
*/
E_RW0001_r_a_acc_Null_ACC	"r_a_acc: Null ACC pointer.\n"
/*
**	racaccs.c
*/
E_RW0002_r_a_caccs_Null_ACC	"r_a_caccs: Null ACC pointer.\n"
/*
**	raccget.c
*/
E_RW0003_r_acc_get_Bad_break	"r_acc_get: Bad break ordinal.\n"
/*
**	ractset.c
*/
E_RW0004_r_act_set_bad_code	"r_act_set: bad code %0d\n"
/*
**	racum.c
*/
E_RW0005_r_a_cum_Null_CUM	"r_a_cum: Null CUM pointer.\n"
E_RW0006_r_a_cum_Null_ACC	"r_a_cum: Null ACC pointer.\n"
/*
**	radvance.c
*/
E_RW0007_r_advance_unexp_end	"r_advance: unexpected end of report.\n"
/*
**	raops.c
*/
E_RW0008_r_a_ops_Null_ACC	"r_a_ops: Null ACC pointer.\n"
/*
**	rattdflt.qc
*/
E_RW0009_r_att_dflt_No_names	"r_att_dflt: row descriptor doesn't contain names\n"
/*
**	WARNING message
*/
S_RW000A_Column_name_keyword	"WARNING: Column name '%s' is also a Report Writer keyword."
/*
**	rbrkfind.c
*/
E_RW000B_r_brk_find_Null_ACC	"r_brk_find: Null ACC pointer.\n"
E_RW000C_r_brk_find_Bad_attrib	"r_brk_find:Bad attribute ordinal %0d\n"
E_RW000D_r_brk_find_Bad_break	"r_brk_find:Bad break ordinal.\n"
/*
**	rbrkset.c
*/
E_RW000E_r_brk_set_Bad_sort	"r_brk_set:bad sort att ordinal %0d\n"
/*
**	rcrack.c
*/
S_RW000F_Report_or_Table	"Report or Table"
/*
**	rdoprompt.c
*/
S_RW0010_ENTER_C_S_HELP_RETURN	"ENTER C,S,HELP OR <RETURN>: "
S_RW0011_Enter_S_HELP_ENTER	"Enter 'S', 'HELP' or <ENTER>:\n"
E_RW0012_r_do_prompt_Cant_read	"r_do_prompt:Can't read input file.\n"
S_RW0013_Enter_code	"Enter one of the following codes:\n"
S_RW0014_c_continue_printing	"  c - continue printing report, but without prompts.\n"
S_RW0015_s_stop_the_report	"  s - stop the report and exit.\n"
S_RW0016_h_print_this_help	"  h - print this help message.\n"
S_RW0017_Anything_else	"Anything else (like a carriage return) continues, with prompts.\n"
/*
**	rdtplset.c
*/
E_RW0018_NoMatch_syserr	"r_dtpl_set: target mismatch, %0c ==> %1c\n"
E_RW0019_BadExpr_syserr	"r_dtpl_set: bad target expression, %0c\n"
/*
**	reblexpr.c
*/
E_RW001A_And_must_have_boolean	"r_e_blexpr: AND must have boolean expressions as operands\n"
E_RW001B_Or_must_have_boolean	"r_e_blexpr: OR must have boolean expressions as operands\n"
E_RW001C_Not_must_have_boolean	"r_e_blexpr: NOT must have a boolean expressions as an operand\n"
E_RW001D_Must_be_break_column	"r_e_blexpr: argument of break must be a break column or 'report'.\n"
/*
**	reitem.c
*/
E_RW001E_r_e_item_No_item	"r_e_item: no item\n"
E_RW001F_r_e_item_Unknown_item	"r_e_item: unknown item\n"
/*
**	repar.c
*/
E_RW0020_r_e_par_Null_PAR	"r_e_par:NULL PAR pointer.\n"
/*
**	rerror.c
*/
E_RW0021_r_error_Too_many_args	"r_error('%0d'): too many arguments.\n"
/*
**	restack.c
*/
E_RW0022_r_pop_env_No_environm	"r_pop_env:No environments in stack.\n"
/*
**	revalpos.c
*/
E_RW0023_r_eval_pos_Null_tcmd_	"r_eval_pos:Null tcmd address.\n"
E_RW0024_r_eval_pos_Bad_tcmd	"r_eval_pos:Bad tcmd code %0d\n"
/*
**	rexit.qc
*/
S_RW0025_Hit_RETURN_when_done	"Press the <RETURN> key when done"
/*
**	rfixparl.c
*/
E_RW0026_Illegal_value_specifi	"    *** Illegal value specified for datatype of parameter. Re-enter it.\n"
/*
**	rges.c
*/
E_RW0027_size_too_large	"r_g_espace: size too large\n"
/*
**	rgeskip.c
*/
E_RW0028_r_g_eskip_Pointer_err	"r_g_eskip: pointer error"
/*
**	rgexpr.c
*/
E_RW0029_Expected_boolean_expr	"r_g_expr: expected boolean expression.\n"
E_RW002A_expected_operand	"r_g_expr: expected operand.\n"
E_RW002B_expected_IS_NULL	"r_g_expr: expected IS [NOT] NULL\n"
E_RW002C_expected_parenthesis	"r_g_expr: expected closing parenthesis.\n"
E_RW002D_expected_legal_number	"r_g_expr: expected legal number.\n"
E_RW002E_expected_alphanumeric	"r_g_expr: expected alphanumeric parameter.\n"
/*
**	rmaction.c
*/
E_RW002F_r_m_action_Null_cmd	"r_m_action: Null ptr to command.\n"
E_RW0030_r_m_action_Too_many	"r_m_action: too many arguments.\n"
/*
**	rmakagg.c
*/
E_RW0031_Bad_attribute	"r_mk_agg:Bad attribute ordinal.\n"
/*
**	rmakcum.c
*/
E_RW0032_r_mk_cum_Bad_attribut	"r_mk_cum:Bad attribute ordinal:%d.\n"
/*
**	rmptop.c
*/
F_RW0001_Report_	"Report: "
/*
**	rmrprt.c
*/
E_RW0033_r_m_rprt_Bad_style	"r_m_rprt: Bad style '%0d'.\n"
/*
**	rmrtitle.c
*/
F_RW0002_Report_on_Table_	"Report on Table: "
/*
**	rmsort.c
*/
E_RW0034_Bad_sort_order	"r_m_sort: Bad sort order %0d.\n"
E_RW0035_r_m_sort_Bad_attribut	"r_m_sort: Bad attribute ordinal %0d\n"
/*
**	rnxtset.c
*/
E_RW0036_r_nxt_set__Bad_break_	"r_nxt_set: Bad break '%0c' in '%1c'.\n"
/*
**	ropadd.c
*/
E_RW0037_r_op_add_Bad_brk_or_a	"r_op_add:Bad brk or acc ptr.\n"
/*
**	rparreq.c
*/
S_RW0038_Enter	"Enter '%s' "
/*
**	rpartype.c
*/
E_RW0039_r_par_type__parameter	"r_par_type: parameter not in list\n"
/*
**	rpif.c
*/
E_RW003A_expected_boolean_expr	"r_p_if: expected boolean expression\n"
E_RW003B_expected_ENDIF	"r_p_if: expected ENDIF.\n"
E_RW003C_expected_THEN	"r_p_if: expected THEN\n"
/*
**	rplet.c
*/
E_RW003D_r_p_let_No_boolean	"r_p_let: boolean expression not allowed in .LET assignment command.\n"
/*
**	rptext.c
*/
E_RW003E_r_p_tcmd_Cannot_print	"r_p_tcmd: cannot print boolean expression.\n"
S_RW003F_expression	"expression"
/*
**	rqurset.c
*/
E_RW0040_r_qur_set_Bad_section	"r_qur_set:Bad section name:%0c\n"
/*
**	rrcoset.qc
*/
E_RW0041_Error_in_RCOMMANDS	"r_rco_set: Error in RCOMMANDS table. Code is %0d\n"
E_RW0042_unexpected_call	"r_gsv_act: unexpected call"
E_RW0043_bad_file_read	"r_gsv_act: bad file read, seq %0d: read %1d, got %2d"
/*
**	rreadln.c
*/
E_RW0044_read_returns_error	"read returns error\n"
/*
**	rrepdo.qc
*/
F_RW0003_Retrieving_data	"Retrieving data . . .\n"
F_RW0004_Retrieving_and_sortin	"Retrieving and sorting data . . .\n"
S_RW0045_No_data_in_table	"No rows found.  No report will be run.\n"
/*
**	rrepld.c
*/
F_RW0005_Reading_report_spec	"Reading report specifications and preparing query . . .\n"
F_RW0006_Setting_up_default	"Setting up default report . . .\n"
/*
**  rmrtitle.c
*/
F_RW0007_Report_on_JoinDef_ "Report on JoinDef: "
/*
**	rreset.c
*/
E_RW0046_r_reset_Too_many_arg	"r_reset: too many arguments.\n"
E_RW0047_level_4_memory_alloca	"level 4 memory allocation tag not set"
E_RW0048_level_5_memory_alloca	"level 5 memory allocation tag not set"
/*
**	rrunerr.c
*/
E_RW0049_not_legal_sequence_no	"r_runerr: not a legal sequence number"
/*
**	rwibegin.c
*/
E_RW004A_r_wi_begin_Bad_WITHIN	"r_wi_begin: Bad nesting of WITHIN.\n"
/*
**	rwiend.c
*/
E_RW004B_r_wi_end_Bad_WITHIN	"r_wi_end: Bad nesting of WITHIN.\n"
/*
**	rxadjust.c
*/
E_RW004C_r_x_adjust_Field_widt	"r_x_adjust:Width of field too small:%0d\n"
/*
**	rxnewline.c
*/
E_RW004D_r_x_newline_Write_err	"r_x_newline:error in write to file.\n"
/*
**	rxtcmd.c
*/
E_RW004E_r_x_tcmd_Bad_tcmd_cod	"r_x_tcmd:Bad tcmd code.\n"
/*
**	sqlscan.c
*/
E_RW004F_Statement_in_error_is	"Statement in error is:\n"

/* The error messages follow: */

E_RW0999_
"Line with error: '%1c %2c'.\n
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1000_
"Bad flag '%0c' specified in REPORT command.\n
Correct usage of REPORT command is:\n
        REPORT dbname [[-m..] table | [-r] report | -i..] [-d]\n
	       [-f..] [-qn] [-ln] [-wn] [-vn]\n
	       [-t|+t] [-b|+b] [-5] [-6]\n
	       [-nofirstff|-firstff]\n
	       [-h] [-s] [-uuser] [-oprinter] [-nn] [({param=val})]\n
	       [-P] [-GgroupID] [-numeric_overflow=ignore|warn|fail]\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1001_
"Bad parameters specified at or near '%0c'\n
Correct usage of REPORT command is:\n
        REPORT dbname [[-m..] table | [-r] report | -i..] [-d]\n
	       [-f..] [-qn] [-ln] [-wn] [-vn]\n
	       [-t|+t] [-b|+b] [-5] [-6]\n
	       [-nofirstff|-firstff]\n
	       [-h] [-s] [-uuser] [-oprinter] [-nn] [({param=val})]\n
	       [-P] [-GgroupID]\n
Processing stops."

/*
** Explanation:
**	===> This message should be obsolete <===
**
** System Status:
**
** User Action:
*/

E_RW1002_
"Not enough information specified on the REPORT command.\n
You must specify database name, report or table name and any parameters
used in your query, unless you specify the '-p' flag
on the REPORT command, which will prompt you for any
missing information.  Correct usage of REPORT is:\n
        REPORT dbname [[-m..] table | [-r] report | -i..] [-d]\n
	       [-f..] [-qn] [-ln] [-wn] [-vn]\n
	       [-t|+t] [-b|+b] [-5] [-6]\n
	       [-nofirstff|-firstff]\n
	       [-h] [-s] [-uuser] [-oprinter] [-nn] [({param=val})]\n
	       [-P] [-GgroupID]\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1004_
"Report or table '%0c' does not exist or is not owned by you."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1005_
"Data table '%0c' in Report '%1c' does not exist or is not owned by you."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1006_
"Sort column '%0c' does not exist."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1007_
"Database '%0c' does not exist."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1008_
"Bad numeric value specified for flag '%0c' on REPORT command.\n
Only positive values allowed.  Value set to default.
Correct syntax of REPORT is:\n
        REPORT dbname [[-m..] table | [-r] report | -i..] [-d]\n
	       [-f..] [-qn] [-ln] [-wn] [-vn]\n
	       [-t|+t] [-b|+b] [-5] [-6]\n
	       [-nofirstff|-firstff]\n
	       [-h] [-s] [-uuser] [-oprinter] [-nn] [({param=val})]\n
	       [-P] [-GgroupID]\n
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1009_
"Bad value '%0c' specified for the -m flag on REPORT command.\n
Legal values are '-mdefault' (or '-m'), '-mcolumn', '-mwrap'
or '-mblock'.  Value set to default.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW100A_
"%0c error(s) have occurred in setting up the report.
No report will be written.  Correct errors and rerun."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW100B_
"Error opening report file '%0c'.  No report will be written."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW100C_
"Because of errors found in the .SORT list, the report writer stops.\n
Fix .SORT list and rerun SREPORT.  Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW100D_
"Bad numeric value specified for flag '%0c' on REPORT command.\n
Only non-negative values allowed.  Value set to default.
Correct syntax of REPORT is:\n
        REPORT dbname [[-m..] table | [-r] report | -i..] [-d]\n
	       [-f..] [-qn] [-ln] [-wn] [-vn]\n
	       [-t|+t] [-b|+b] [-5] [-6]\n
	       [-nofirstff|-firstff]\n
	       [-h] [-s] [-uuser] [-oprinter] [-nn] [({param=val})]\n
	       [-P] [-GgroupID]\n
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW100E_
"Cannot specify -c flag with -i flag. -c flag will be ignored.\n
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW100F_
"Cannot specify report or table name and -i flag together."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1014_
"Query specified in .QUERY command is too long.\n
Maximum allowable size is %0c characters.
You can use the '-q' flag on the REPORT command to extend the max."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1015_
"Error in .QUERY command RANGE statement.\n
Problem at or near RANGE of %0c is %1c.
Error check continues . . ."

/*
** Explanation:
**	==> This message should be obsolete <==
**
** System Status:
**
** User Action:
*/

E_RW1016_
"Parameter '%0c' not specified on command line.\n
You can have the report writer prompt you for values
of parameters by specifying the -p flag on the 
REPORT command.  Correct syntax is:\n
        REPORT dbname [[-m..] table | [-r] report | -i..] [-d]\n
	       [-f..] [-qn] [-ln] [-wn] [-vn]\n
	       [-t|+t] [-b|+b] [-5] [-6]\n
	       [-nofirstff|-firstff]\n
	       [-h] [-s] [-uuser] [-oprinter] [-nn] [({param=val})]\n
	       [-P] [-GgroupID]\n
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1017_
"Bad parameter name in .QUERY at or near '%0c'.\n
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1019_
"Either you had errors in the .QUERY command, or you didn't
specify either a .QUERY or .DATA command.  Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW101A_
"Because of errors in running report, the report writer stops.\n
Fix errors in report or data table and rerun."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW101B_
"Because of errors in running report, the report writer stops.\n
Fix errors in report or data table and rerun, verifying that
all parameters were given valid entries."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1032_
"Bad declaration specified for variable '%0c'.  Correct syntax is:\n
        .DECLARE variablename = datatype [WITH NULL | NOT NULL]\n
                                         [WITH PROMPT \"prompt string\"]\n
                                         [WITH VALUE \"value string\"]\n
             { , variablename = datatype ... }"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1034_
"Bad datatype name specified for variable '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1037_
"Variable '%0c' was previously declared."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW103C_
"Bad syntax for .LET assignment command in %0c text for '%1c'.\n
The correct syntax is:\n
        .LET declared_variable_name [:]= expression\n
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW103E_
"Expression expected on right side of .LET assignment command
in %0c text for '%1c'.
Problem occurs at or near '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1041_
"NULL cannot be assigned to a non-nullable variable in %0c text for '%1c'.\n
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1043_
"NULL cannot be operand of comparison operator in %0c text for '%1c'.
Use IS NULL or IS NOT NULL instead.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1044_
"NULL cannot be used in an arithmetic expression in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1064_
"Unrecognizable command in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1065_
"Cannot interpret printing command in %0c text for '%1c'.
Problem occurs at or near '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1066_
"Unknown name '%0c' in %1c text for '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1067_
"Unrecognized numeric value in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1068_
"Extra characters ignored in %0c text for '%1c'.
Did you forget to specify the .PRINT command?
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1069_
"Bad format for %0c in %1c text for '%2c'.  
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW106A_
"Format not compatible with datatype of '%0c' in %1c text for '%2c'.  
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW106B_
"The data type of variable $%0c is incompatible with its usage
in %1c text for '%2c'.  
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW106C_
"No column specified for aggregate '%0c' in %1c text for '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW106D_
"Bad column '%0c' specified for aggregate '%1c'.
Error occurred in %2c text for '%3c'.
Line with error: '%4c %5c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW106E_
"Aggregate '%0c' in %1c text for '%2c' not in footer text.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW106F_
"Cannot aggregate a column with a higher sort order.
Aggregate '%0c' in '%1c' caused the error.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1071_
"Expected closing parenthesis in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1072_
"Column '%0c' not a break column in %1c text for '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1073_
"Bad name '%0c' found in cumulative in %1c text for '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1074_
"Bad name found in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1075_
"Bad or no primitive aggregate found for cumulative in
%0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1076_
"Bad preset value found in '%0c' in %1c text for '%2c'.
Preset can be a numeric, date constant, or a column name
and must match the datatype of the aggregated column.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1077_
"Cannot preset average aggregate in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1078_
"Bad parameter name at or near '%0c' in %1c text for '%2c'.
Rest of print command skipped.  
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1079_
"Parameter '%0c' in %1c text for '%2c' not on command line.
Use '-p' option on REPORT command to have the report writer
prompt you for values of parameters.  Rest of command skipped.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW107B_
"You can only specify a unique aggregate on a column which is in the
sort list.
Aggregate '%0c' in '%1c' caused the error.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW107C_
"Expected comma or closing parenthesis for function call
in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW107D_
"
Expected argument for function call between comma and closing
parenthesis in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW107E_
"Too many arguments for function call or no closing parenthesis
in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW107F_
"Expected open parenthesis preceding argument(s) for function call
in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1080_
"Preset value for count aggregate must be an integer datatype
in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1081_
"Error found in aggregate in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1082_
"Bad column name '%0c' found in %1c text for '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1083_
"Bad format of FORMAT or TFORMAT command at or near '%0c'.
Error found in %1c text for '%2c'.
Correct format is:\n
        .FORMAT colname {,colname} (format) {,colname ...}\n
Line with error: '%3c %4c'.
Rest of command skipped.  Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/


E_RW1084_
"Aggregate function '%0c' is currently not available with the DATE datatype.
Line with error: '%1c %2c'.
Error check continues . . ."

/*
** Explanation: In release 6.1, adf does not support the aggregate functions
**              sum, avg on the date datatype.  These functions will be
**              supported in later releases.  At that point, this error message
**              should be deleted.
**
** System Status:
**
** User Action: User should change the report so it no longer uses the
**              function sum or avg on a date datatype.
*/



E_RW1085_
"A .PRINT command does not follow justification in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW108C_
"Bad format of .POSITION command at or near '%0c'.
Error found in %1c text for '%2c'.
Correct format is:\n
        .POSITION colname {,colname} (position [,width]) {,colname ...}\n
Line with error: '%3c %4c'.
Rest of command skipped.  Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW108D_
"Value given for .POSITION command is too large or too small.
Error found in %0c text for '%1c'.
Maximum value is %2c, minimum is 0.
Line with error: '%3c %4c'.
If you wish to reset the maximum value, use the -l flag.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW108F_
"Bad format of .WIDTH command at or near '%0c'.
Error found in %1c text for '%2c'.
Correct format is:\n
        .WIDTH colname {,colname} (width) {,colname ...}\n
Line with error: '%3c %4c'.
Rest of command skipped.  Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1090_
"Value given for column width is too large or too small.
Error found in %0c text for '%1c'.  Maximum value is %2c, 
minimum is 1.  Line with error: '%3c %4c'.
If you wish to reset the maximum value, use the -l flag.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A0_
"Expected quoted character for .ULCHAR command at or near '%0c'.
Error found in %1c text for '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A1_
"Expected quoted string for .NULLSTRING command at or near '%0c'.
Error found in %1c text for '%2c'.
Line with error: '%3c %4c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A2_
"No column name specified for .WITHIN command.
Error occurred in %0c text for '%1c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A3_
"Bad column name '%0c' found in .WITHIN command.
Error occurred in %1c text for '%2c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A4_
"Bad format of .WITHIN command at or near '%0c'.
Error occurred in %1c text for '%2c'.
Correct format of command is:\n
        .WITHIN colname {,colname}\n
Line with error: '%3c %4c'.
Rest of command skipped.  Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A5_
"Nested .WITHIN blocks in %0c text for '%1c'.
Make sure you .ENDWITHIN the previous block first.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A6_
"Tried to .END a .WITHIN command in %0c text for '%1c'.
No .WITHIN command precedes the .END.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10A7_No_DelimID
"Delimited identifier '%2c' is invalid, or no preceeding .DELIMID statement
was found."

/*
** Explanation: s_g_ident() returned UI_DELIM_ID when St_xns_given == FALSE.
**
** System Status:
**
** User Action: Correct the invalid identifier and/or add a .DELIMID
**		statement.
*/

E_RW10AA_
"Bad name '%0c' found after .END command.
Error occurred in %1c text for '%2c'.
Only .END WITHIN or .END BLOCK or .END allowed.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10AF_
"Bad name '%0c' found after .BEGIN command.
Error occurred in %1c text for '%2c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10B5_
"Command out of place in %0c text for '%1c'.
Command only allowed in a .WITHIN block of commands.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10B6_
"Command out of place in %0c text for '%1c'.
Command not allowed in a .WITHIN block of commands.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10B7_
"Column name specified in positioning command in %0c text for '%1c'.
Because you are currently in a .WITHIN block, you cannot
specify a column name on the positioning command.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10B8_
"Specified W_COLUMN constant outside of .WITHIN block.
Error occurred in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10B9_
"Cannot specify W_COLUMN as the CUM column or the PRESET column.
Error occurred in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10C9_
"Left margin set to too small a value in %0c text for '%1c'.
Value set to minimum allowable value.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10CA_
"Left margin set to too big a value in %0c text for '%1c'.
Value set to default.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10CB_
"Page length set to too small a value in %0c text for '%1c'.
Must be greater than the combined size of the page header and
page footer.  Set to minimum allowable value.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10CC_
"Non-character format found in %0c text for '%1c'.
Field skipped.  Error check continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10CE_
"Right margin set to too small a value in %0c text for '%1c'.
Value set to default.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10CF_
"Right margin set to too large a value in %0c text for '%1c'.
Value set to maximum allowable value.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10D0_
"Tab set to negative column number in %0c text for '%1c'.
Value set to zero.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10D1_
"Tab set to position beyond end of line in %0c text for '%1c'.\n
You can change this maximum report width by using the '-l' flag on the 
report command.  .TAB command ignored.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10D2_
"Left margin set to value greater than right margin 
in %0c text for '%1c'.
Right margin set greater than this value. Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10D3_
"Right margin set to value less than left margin
in %0c text for '%1c'.
Value set greater than left margin. Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10DC_
"Positioning for .CENTER, .LEFT or .RIGHT too small.
Error found in %0c text for '%1c'.  Position set to default.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10DD_
"Positioning for .CENTER, .LEFT or .RIGHT too big.
Error found in %0c text for '%1c'.  Position set to default.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10E6_
"Value for .POSITION in %0c text for '%1c' ignored.
Value is below minimum(0) or above maximum(255).
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW10E7_
"Value for .WIDTH in %0c text for '%1c' ignored.
Value is below minimum(1) or above maximum(255).
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW112D_
"Exceeded maximum number of lines which can be written in one
.BLOCK (using the wraparound, filling, or adjusting formats).  Maximum
value allowed is %0c.  You can change this maximum by using the
'-w' flag on the REPORT command.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW112E_
"Exceeded maximum number of lines which can be written
(using the wraparound, filling, or adjusting formats)
without an explicit .NEWLINE or .NEWPAGE.  Maximum value
allowed is %0c.  You can change this maximum by using the
'-w' flag on the REPORT command.  Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1131_
"Tried to write beyond end of line while centering, or
justifying.  The offending line has been truncated.
Maximum line length is %0c.  You can change this maximum
by using the '-l' flag on the report command.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW11F7_
"Arithmetic operands not the same datatype and each cannot be coerced
to the datatype of the other in %0c text for '%1c'.
Line with error: '%2c %3c'.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW11F8_
"Wrong datatype for argument of function in %0c text for '%1c'.
Line with error: '%2c %3c'.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW11F9_
"Error calling function in %0c text for '%1c'.
Line with error: '%2c %3c'.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1203_
"Divided by zero in expression in %0c text for '%1c'.
Line with error: '%2c %3c'.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1204_
"Result overflowed while evaluating expression 
in %0c text for '%1c'.
Line with error: '%2c %3c'.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1205_
"Result underflowed while evaluating expression 
in %0c text for '%1c'.
Line with error: '%2c %3c'.
Processing continues . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1209_
"Fatal error occurred in %0c text for '%1c'.
Line with error: '%2c %3c'.
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW12BC_
"Bad flag '%0c' specified on RBF command.\n
Correct usage of RBF command is:\n
        RBF dbname [[-m..] table | [-r] report] [-ln] [-s] [-uuser] [-e]\n
	[-P] [-GgroupID]\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW12BE_
"Not enough information specified on the RBF command.\n
You must specify database name and report name, unless you specify the 
'-p' flag on the RBF command, which will prompt you for any
missing information.  Correct usage of RBF is:\n
        RBF dbname [[-m..] table | [-r] report] [-ln] [-s] [-uuser] [-e]\n
	[-P] [-GgroupID]\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW12C4_
"Bad numeric value specified for flag '%0c' on RBF command.\n
Only positive values allowed.  Value set to default.
Correct usage of RBF command is:\n
        RBF dbname [[-m..] table | [-r] report] [-ln] [-s] [-uuser] [-e]\n
	[-P] [-GgroupID]\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW12C5_
"Bad value '%0c' specified for the -m flag on RBF command.\n
Legal values are '-mdefault' (or '-m'), '-mtabular', '-mindented', '-mlabels',
'-mcolumn', '-mwrap' or '-mblock'.\n
Correct usage of RBF command is:\n
        RBF dbname [[-m..] table | [-r] report] [-ln] [-s] [-uuser]\n
        [-e] [-P] [-GgroupID]\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW12C6_
"Report '%0c' created with SREPORT rather than RBF.\n
You can only use RBF to edit reports created by default
or in a previous RBF session."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW12C7_
"Report '%0c' is too wide for your terminal."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW12C8_
"Duplicate attribute name '%0c' found.  Cannot initialize RBF layout.\n
Rename attribute to run RBF.  Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1320_
"Bad flag specified on COPYREP command.  Legal syntax is:\n
        COPYREP [-f] [-s] [-uuser] [-P] [-GgroupID] dbname file {report(s)}\n
Use SREPORT to load reports into a database.
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1321_
"No reports specified for COPYREP command.  Legal syntax is:\n
        COPYREP [-f] [-s] [-uuser] [-P] [-GgroupID] dbname file {report(s)}\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1323_
"Error opening or writing file '%0c' in COPYREP.
\nVerify that you have sufficient privileges to write to this location.\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1384_
"Bad flag '%0c' specified on SREPORT command.
Correct format is:\n
        SREPORT [-s] [-uuser] [-P] [-GgroupID] dbname filename\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1385_
"Not enough information specified on the SREPORT command.
You must specify a database name and a file name, unless
you specify the '-p' flag, which will prompt you for any
missing information.   Correct format is:\n
        SREPORT [-s] [-uuser] [-P] [-GgroupID] dbname filename\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1386_
"Text file '%0c' does not exist, is not readable or is a bad name.
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1387_
"File '%0c' Line %1c: Expected command at '%2c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1388_
"File '%0c' Line %1c: Bad command name '%2c' found."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1389_
"File '%0c' Line %1c:\nBad name '%2c' specified for report.\nValid characters 
are alphanumeric characters and the '_', '#', '$', '@'.  A report name must
begin with either a letter or an underscore.\n
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW138A_
"File '%0c' Line %1c: No .NAME command encountered.
It must be the first command in the file.  Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW138B_
"File '%0c' Line %1c: Parameters starting at '%2c' in command '%3c'
won't fit.  Break up the command into 2 or more commands
so that it will fit."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW138C_
"File '%0c' Line %1c: .HEADER or .FOOTER specified for '%2c',
which is not a break column specified in .SORT command,
or 'report' or 'page'.  Command ignored."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW138D_
"File '%0c' Line %1c: Bad format of .HEADER or .FOOTER command.
Correct format is:\n
        .HEADER column|REPORT|PAGE\n
or      .FOOTER column|REPORT|PAGE\n
Command ignored."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW138E_
"%0c errors encountered in report specification.  
The report(s) will not be added to your database.
Rerun SREPORT with errors corrected.  Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW138F_
"File '%0c' Line %1c: Bad format of .DATA command.
Correct format is:\n
        .DATA tablename\n
Command ignored."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1390_
"File '%0c' Line %1c: Bad format of .OUTPUT command.
Correct format is:\n
        .OUTPUT filename\n
Command ignored."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1391_
"File '%0c' Line %1c: Bad format of .SORT command.
Correct format is:\n
        .SORT { columnname [: ASCENDING|DESCENDING] [,]}"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1392_
"File '%0c' Line %1c: '%2c' is a reserved name.
It cannot be used as a column name in your data table."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1393_
"File '%0c' Line %1c: Bad sort direction '%2c' specified.
Correct values are 'ascending' or 'descending' ('a', 'asc', 'd' or 'desc')."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1394_
"File '%0c' Line %1c: Column name '%2c' already specified in .SORT.
You can only specify a column name once."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1395_
"File '%0c' Line %1c: Bad format of cumulative or aggregate.
Correct format is:\n
        [CUM [(break)]] aggname (colname [,preset])"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1396_
"Cannot have .NEWPAGE command in the page header."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1398_
"Cannot create file '%0c' for writing.  Processing stops.
This file is used internally by the SREPORT program.
If you don't know why this error occurs, see the INGRES
system manager."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1399_
"File '%0c' Line %1c: Header or Footer already specified for %2c.
You can only specify one header and one footer for a break."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW139A_
"File '%0c' Line %1c: .OUTPUT already specified for report.
You can only specify one output file for a report."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW139B_
"File '%0c' Line %1c: .HEADER or .FOOTER specified for '%2c' but
it must be specified as a column in a .BREAK or .SORT command.
Command ignored."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW139C_
"File '%0c' Line %1c: .HEADER or .FOOTER specified for '%2c',
which is not a break column specified in .BREAK command,
or 'report' or 'page'.  Command ignored."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW139D_
"File '%0c' Line %1c: Bad format of .BREAK command.
Correct format is:\n
        .BREAK {columnname [,]}"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW139E_
"File '%0c' Line %1c: Column name '%2c' already specified in .BREAK.
You can only specify a column name once."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW139F_
"File '%0c' Line %1c: More than one .SORT command specified or
a .SORT command was specified and the retrieval is already sorted
via a SORT BY or ORDER BY clause in the .QUERY command."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A0_
"File '%0c' Line %1c: Bad format of .DECLARE command.
Correct format is:\n
        .DECLARE variablename = datatype [WITH NULL | NOT NULL]\n
                                         [WITH PROMPT \"prompt string\"]\n
                                         [WITH VALUE \"value string\"]\n
             { , variablename = datatype ... }"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A1_
"File '%0c' Line %1c: Length of variable name exceeds %2c characters."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A2_
"Premature end of file found.  This is probably caused by
unmatched quotes (\"), parentheses, or .IF/.ENDIF.
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A3_
"Premature end of file found during a LONGREMARK. This is caused by
not ending the long remark with an .ENDREMARK command.
Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A6_
"File '%0c' Line %1c: .BREAK already specified for report.
You can only declare break columns in one command in a report."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A7_
"File '%0c' Line %1c: .DECLARE already specified for report.
You can only declare variables in one command in a report."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A8_
"File '%0c' Line %1c: .SHORTREMARK already specified for report.
You can only specify one short remark for a report."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13A9_
"File '%0c' Line %1c: .LONGREMARK already specified for report.
You can only specify one long remark for a report."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13AC_
"File '%0c' Line %1c: Single-quoted strings are not allowed in 
the .QUERY command when	the query language being used is QUEL."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13AD_
"File '%0c' Line %1c: Only one retrieval command allowed per report."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13B6_
"File '%0c' Line %1c: Error in RANGE statement in .query command.
Correct format of RANGE is:\n
        RANGE OF range_var IS tablename\n
Rest of query skipped."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13B7_
"File '%0c' Line %1c: Unrecognized QUEL command in .query.
Problem at or near '%2c'.
You can only specify RANGE and RETRIEVE commands in query.
Rest of query skipped."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13B8_
"File '%0c' Line %1c: Cannot use RETRIEVE INTO in .query.
Only a simple RETRIEVE statement is allowed in query.
Rest of query skipped."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13B9_
"File '%0c' Line %1c: Error in RETRIEVE target list in .query.
You probably have a problem with nesting of parentheses.
Rest of query skipped."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13BA_
"File '%0c' Line %1c: Error in RETRIEVE 'where' clause in .query.
You probably have a problem with nesting of parentheses.
Rest of query skipped."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13BB_
"File '%0c' Line %1c: Premature end of query found.
Check the syntax of your query.  Correct syntax is:\n
        .QUERY\n
                RANGE OF x IS y\n
                ..\n
                RETRIEVE [UNIQUE] (target_list) [WHERE ... ]\n
                                                [SORT [BY] ... | ORDER BY ...]"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13BC_
"File '%0c' Line %1c: You have already specified .query or .data.
You must specify .query or .data for each report, but you
can't specify both, or one of them twice."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13BD_
"File '%0c' Line %1c: Premature end of query found.
Check the syntax of your query."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13BE_
"File '%0c' Line %1c: Premature end of query found.
Check the syntax of your query.  Correct syntax is:\n
.QUERY\n
        SELECT [ALL | DISTINCT] column {,column} FROM table {,table}\n
        [WHERE ...][GROUP BY ...][HAVING ...]\n
        {UNION SELECT ...}\n
        [ORDER BY ...]"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13BF_
"File '%0c' Line %1c: Error in SELECT 'where' clause in .query.
You probably have a problem with nesting of parentheses.
Rest of query skipped."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13C0_
"File '%0c' Line %1c: .%2c cannot be within an .IF statement."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13C1_
"File '%0c' Line %1c: .%2c must follow .IF  . . ."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13C2_
" File '%0c' Line %1c: .%2c cannot follow .ELSE."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13C3_
"File '%0c' Line %1c: .THEN expected."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13CA_
"File '%0c' Line %1c: Boolean expression expected."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13CB_
"File '%0c' Line %1c: Cannot use boolean expression in this command."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13CE_
"File '%0c' Line %1c: Expected closing parentheses."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13CF_
"File '%0c' Line %1c: Cannot have logical operator here."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D0_
"File '%0c' Line %1c: Expected comma."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D1_
"File '%0c' Line %1c: Expected open parenthesis."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D2_
"File '%0c' Line %1c: No such function or aggregate."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D3_
"File '%0c' Line %1c: Alphanumeric name must follow $."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D4_
"File '%0c' Line %1c: Logical operators must have boolean
expressions as operands."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D5_
"File '%0c' Line %1c: Operator is missing an operand."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D6_
"File '%0c' Line %1c: CUM must precede aggregate outside
of .FOOTER."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D7_
"File '%0c' Line %1c: The correct syntax for a .LET assignment
command is:\n
        .LET declared_variable_name [:]= expression"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D8_
"File '%0c' Line %1c: A boolean expression cannot be assigned to
a variable in a .LET command."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13D9_
"File '%0c' Line %1c: A comment was started and not ended before
the End Of File was reached."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13DA_
"File '%0c' Line %1c: The correct syntax for checking if an
expression is null (or is not null) is:\n
        expression IS [NOT] NULL"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13DE_
"File '%0c' Line %1c: '%2c' is not a valid query language at
this installation."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13DF_
"Report '%0c':  Query specified in '%1c'; '%1c' is not a
valid query language at this installation."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13E0_
"Report '%0c':  The 'order by' clause and 'distinct' keyword
cannot be used in the query specified with the .QUERY command.
To specify ordering use the .SORT command."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW13E7_
"More than %0c errors found.  Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/
E_RW13E8_
"Error connecting with database %0c. Processing stops."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/


E_RW13E9_Default_rpt_too_wide
"Report default is %0c characters wide.  Maximum value is %1c.
 Recreate the report in a different mode or
if you wish to reset the maximum value, use the -l flag."

/*%
** Explanation:
** A default report was being created in RBF, probably in column mode.
** The report is wider than the maximum width.  The maximum width is
** determined by either the terminal capabilities or the -ln command line
** option.
**
** System Status:
** Error message and continue..
**
**
** User Action:
** Either recreate the report in another mode, e.g. block, or exit RBF
** and re-enter with the -ln option where n is large enough for this report.
**
**
*/
E_RW13EA_Duplicate_attribute
"Duplicate column name '%0c' found. \n
Rename attribute to run report.  Processing stops."

E_RW13EB_Duplicate_setup
"File '%0c' Line %1c: Duplicate setup or cleanup section found.\n
Only one setup and one cleanup section per report is allowed."
/*
** Explanation: Not more than one setup or cleanup command is allowed
**
** System Status:
**
** User Action: Check for duplicate setup sections.
*/

E_RW13EC_Cannot_close_unit
"Cannot close report file. \n
Error closing report file '%0c'.  No report will be written."

/*
** Explanation:
** The report was specified to be directly sent to the printer.  An error 
** occurred trying to close the file before printing.  The report is not
** sent to the printer due to the error in its creation.
**
** The most common occurrence of an error closing a file is the file no longer
** exists.  Since the file was opened successfully, the file was deleted between
** the opening and the closing.
**
** System Status:
** Error message is printed.  ReportWriter exits.  No report is printed.
**
** User Action:
** Try again, or talk to the Site Technical Contact.
*/

E_RW13ED_Error_printing
"Error printing report occurred. \n
Error printing report file '%0c'. No report will be written.   \n
The error may be caused by an invalid printer name or a system \n
error sending the output to the printer. Check your printer and\n
all your parameters."

/*
** Explanation:
** An error occurred print the report.
**
** System Status:
** Error message is printed.  ReportWriter exits.  No report is printed.
**
** User Action:
** Check status of the printer/queue.
*/

E_RW13EE_Setup_Cleanup_Err
"Error in setup or cleanup section found\n
 Statement in error was:\n
 '%0c'"

/*
** Explanation: A DBMS error was found in a setup or cleanup section
**
** System Status:
**
** User Action: Check for SQL syntax and correctness
*/

E_RW13EF_Setup_Cleanup_Q_Err
"Mismatched quote in setup or cleanup section found\n
 Statement in error was:\n
 '%0c'"
/*
** Explanation: A mismatched quote was found in a setup or cleanup section
**
** System Status:
**
** User Action: Check for mismatched quotes in setup or cleanup sections
*/
E_RW13F0_
"Parameter '%0c' has not been initialized.\n
Initialize the parameter by using 'WITH VALUE' clause in the .DECLARE
command or you can have the Report-Writer prompt you for values by specifying
the 'WITH PROMPT' clause in the .DECLARE command.  You may also specify the
value of the parameter on the report command line.  If the variable is not
in a command evaluated when the report specifications are read, you could also
use the .LET command to inititalize the value.
Error check continues . . ."

/*
** Explanation: A variable has not been initialized and cannot be used.
**
** System Status: Report Writer continues with the error checking.
**
** User Action: The user may initialize the variable by:
**	1.  Using the .LET command
**	2.  Using the 'WITH PROMPT' clause in the .DECLARE command.  Report
**		Writer will then prompt the user for values.
**	3.  Specify the value of the parameter on the command line. 
**	4.  Specify the value in the WITH VALUE clause of the .DECLARE command
*/

S_RW13F1_RW_output_helpfile
"Report Writer Output"

/*
** Explanation: Report Writer output help file title.
**
** System Status: Help file title - displaying help file.
**
** User Action: Status message.  No action necessary.
*/

E_RW13F2_Invalid_Num_Expr_Type
"Invalid parameter type for command '%0c'.\n
 Unable to convert parameter to numeric type for this command."
/*
** Explanation: Could not convert the expression or variable to a type
**		appropriate to this command.
** System Status:
**
** User Action: Review command syntax and parameter used.  Commands
**		which take integers require have numeric expressions.
*/
E_RW13F3_Missing_Argument
"Missing argument in %0c text for '%1c'.
Line with error: '%2c %3c'.
Error check continues . . ."
/*
** Explanation: Missing argument for RIGHT or LEFT MARGIN
**		commands.
** System Status:
**
** User Action: Add numeric argument or expression to .RM or .LM
**		command indicated.
*/
E_RW13F4_Invalid_Col_Expr_Type
"Invalid parameter type for command '%0c'.\n
 Unable to convert parameter to numeric type or valid
 column name for this command."
/*
** Explanation: Could not convert the expression or variable to a type
**		appropriate to this command.
** System Status:
**
** User Action: Review command syntax and parameter used.  Commands
**		which take integers require have numeric expressions.
**		Commands which take column names should have parameters
**		which are character strings that evaluate to a valid
**		column name.
E_RW13F5_Invalid_Str_Expression_Type
"Invalid parameter type for command '%0c'.\n
 Unable to convert parameter to string type for this command."
/*
** Explanation: Could not convert the expression or variable to a type
**		appropriate to this command.
** System Status:
**
** User Action: Review command syntax and parameter used.  Commands
**		which take integers require have numeric expressions.
**		Commands which take column names should have parameters
**		which are character strings that evaluate to a valid
**		column name.
*/
E_RW13F6_RW_Runtime_errors
"Runtime errors encountered in report. Processing Stops.\n"
/*
** Explanation: Errors occurred when running the report.  Previous
**		messages should indicate the nature of those
**		problems.
** System Status: Stop
**
** User Action: Correct errors in report and rerun.
*/

E_RW13F7_
"Report '%0c' does not exist or is not owned by you."

/*
** Explanation:   The report specified by the -r flag is not a valid
**		  report name.
**
** System Status: Report Writer terminates.
**
** User Action:   Check the report name.  Invoke Report Writer with a valid
**		  report name.
*/
E_RW13F8_Bad_Expr
"Bad expression found in %0c text for '%1c'.\n
Line with error: '%2c %3c'."
/*
** Explanation: General Bad expression message for rp* functions.
** System Status: Stop
**
** User Action: Correct errors in expression.
*/

E_RW13F9_Bad_Break
"Bad break found.\n
Cannot create a break %0c for the non-existent column '%1c'."
/*
** Explanation: Report has a break on a column that does not exist.
**
** System Status: Report terminates.
**
** User Action: Correct column name in the report.
*/

E_RW13FA_
"The '-c' flag is no longer supported.  '-c' will be ignored.\n
Error check continues . . ."
/*
** Explanation: ReportWriter and COPYREP are no longer supporting the -c flag.
**
** System Status: Report continues.
**
** User Action: Do not use the -c flag.
*/
E_RW13FB_Bad_Sort_Direction
"Bad sort direction %0c found for column %1c.\n
Correct format is:\n
        .SORT { columnname [: ASCENDING|DESCENDING] [,]}"
/*
** Explanation: ReportWriter verifies sort direction
**
** System Status: Report continues, ascending is used.
**
** User Action: Check parameter/variable values to sort direction
*/

E_RW13FC_no_prompt
"No value for the variable '%0c' supplied.\n
Running report in non-interactive mode and cannot prompt for values.
Please rerun report with values for all variables before executing report.
Error check continues . . ."
/*
** Explanation: A needed value for a variable was not supplied.
**              ReportWriter is running in a separate process and is in
**              non-interactive mode.  Therefore it cannot prompt for
**              the value.
**
** System Status: Report terminates after processing all errors.
**
** User Action: Rerun the report, supplying values for all the variables in
**              the report.
*/

E_RW13FD_batchheader
"Report name: %0c\n
   Start time: %1c"

/*
** Explanation: This is the header for the error log of a report running
**              in non-interactive mode.  It states the report name and the
**              time the report started execution.
**
** System Status: Not applicable.
**
** User Action: Not applicable.
*/
    
E_RW13FE_batchfooter
"    End time:   %0c\n
   Total CPU:  %1d ms.\n
***************************************************************************\n
\n"
	
/*
** Explanation: This is the footer for the error log of a report running
**              in non-interactive mode.  It states the report completion time
**              and the total CPU time used by this process.
**
** System Status: Not applicable.
**
** User Action: Not applicable.
*/


E_RW13FF_
"No value for parameter '%0c' supplied.\nPlease rerun the report with the
value of the parameter on the command line.  Error check continues . . ."

/*
** Explanation:
**	The report has a parameter that not does have a value assigned to it in
**	either the report or on the command line.
**
** System Status:
**	The report continues to check for other errors.  The report then 
**	terminates.
**
** User Action:
**	Rerun the report with the value of the parameter on the command line.
*/

S_RW1400_report_title
"Report Name: %s"

/*
** Explanation:
**	Format template for the title of the output browser.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

E_RW1401_
"Please use the '-n' (number of copies) flag with the '-o' (output) flag.\n
Report terminates."
/*
** Explanation: 
**	The -n (number of copies) flag specifies the number of copies of the
**	report to be printed out.  This flag must be used with the -o (output) 
**	flag.
**
** System Status: 
**	The report terminates.
**
** User Action: 
**	Use the -n flag with the -o flag.
*/

E_RW1402_
"Values given for .POSITION command are too large.\n
The combination of the
the position value and the width value cause the column 
to go beyond the report boundary.
Error found in %0c text for '%1c'.
Maximum report boundary is %2c, minimum is 0.
Line with error: '%3c %4c'.
If you wish to reset the maximum value, use the -l flag.
Error check continues . . ."

/*
** Explanation:
**	A column must fit within the report boundary.  By default, the report 
**	boundary is 132, although the user may override the default using the
**	-l flag.  The sum of the position value and the width value in a 
**	.POSITION command must be less than the report boundary.  Otherwise 
**	the column goes beyond the right margin.
**
** System Status:
**	ReportWriter will terminate after continuing the error check.
**
** User Action:
**	Either execute the report with the -l flag and an appropriate value
**	or change the .POSITION values.
*/

E_RW1403_
"Bad numeric value specified for the variable '%0c' in the .PAGEWIDTH command.\n
Value set to default page width.  Error check continues . . ."

/*
** Explanation:
**	A variable was used for the .PAGEWIDTH command.  A bad value was 
**	entered.
**
** System Status:
**	The page width is set to the default value.  The report continues
**	executing.
**
** User Action:
**	None for this report run.  The user should enter a valid value the 
**	next time the report is executed.
*/


E_RW1404_
"File '%0c' Line %1c: .PAGEWIDTH already specified for report.\nYou can only
specify one page width command for a report."

/*
** Explanation:
**	Multiple .PAGEWIDTH commands have been encountered.
**
** System Status:
**	Program continues checking for errors.
**
** User Action:
**	Leave exactly one .PAGEWIDTH report.  Delete all others.
*/



E_RW1405_
"File '%0c' Line %1c: Invalid page width, '%2c' specified for report.\nPlease
correct error and re-run report.  Processing stops."

/*
** Explanation:
**	User entered an invalid value for the pagewidth.  The value must either
**	be a number or a variable name preceded by a dollar sign.
**
** System Status:
**	Processing stops.
**
** User Action:
**	Correct the .PAGEWIDTH command.
*/

E_RW1406_
"Invalid value for the .PAGEWIDTH command encountered.\nProcessing stops."

/*
** Explanation:
**	An internal error was encountered for the .PAGEWIDTH command.
**
** System Status:
**	Processing stops.
**
** User Action:
**	Contact the local INGRES representative. 
*/

E_RW1407_
"Multiple reports found in report file.\nYou may not have multiple .NAME
commands in a single file and its associated .INCLUDE files when specifying
the -i flag.  Report terminates."
 
/*
** Explanation:
**      User is executing:
**              report dbname -i<filename>.
**      The file has multiple reports, specified by having multiple .NAME
**      commands in the file.  Users may SREPORT multiple reports in a single
**      file, but must have exactly one report per file when using the -i
**      flag with the Report Writer.
**
** System Status:
**      ReportWriter terminates.
**
** User Action:
**      To run with the -i flag, change the file to contain exactly one report
**      or SREPORT the file first, then run report without using the -i flag.
**
*/

S_RW1408_print_report_title
"Report Writer - Sending a Report to a Printer"

/*
** Explanation: 
**	Report Writer print report title.
**
** System Status: 
**	Not applicable.
**
** User Action: 
**	Not applicable.
*/

S_RW1409_file_report_title
"Report Writer - Sending a Report to a File"

/*
** Explanation: 
**	Report Writer file report title.
**
** System Status: 
**	Not applicable.
**
** User Action: 
**	Not applicable.
*/

E_RW140A_Bad_break_col
"Bad column name %0c found in .BREAK statement. \n"

/*
** Explanation:
**	The value of a break column specified in a .BREAK command is not
**	a valid attribute name.
**
** System Status:
**	Report Writer Terminates
**
** User Action:
*/

E_RW140B_
"You may not specify the -m and -i flags together."

/*
** Explanation:
**	User tried to run ReportWriter with both the -i<filename> flag (execute
**	report from the given file) and the -m flags (force the report to be
**	based on a table or view).  These two flags are contradictory.
**
** System Status:
**	ReportWriter terminates.
**
** User Action:
**	Run ReportWriter with either the -m OR the -i flag.
*/


E_RW140C_
"The page width of %0d is too small.  A column cannot fit within the present
boundaries.  Please execute the report again using the -l flag with a larger
value."

/*
** Explanation:
**	User tried to run ReportWriter with a report:
**	    - that has very wide columns (either the column name or data 
**	      column or both are very wide)
**	    - with the -l flag with a low value.
**
** System Status:
**	ReportWriter terminates.
**
** User Action:
**	Run ReportWriter with the -l flag with a larger value.
*/

E_RW140D_
"Integer overflowed while in expression in\n %0c
text for '%1c'. Line with error:\n
'%2c %3c'.\n
Processing continues . . ."
/*
** Explanation:
**	Integer overflow from r_g_double--could have been from rgexpr() or 
**	rpagg()
**
** System Status:
**
** User Action:
**	Change integer constants in report to be within valid integer range
*/
E_RW140E_
"E_RW140E Incompatible value '%s' cannot be assigned to variable '%s'.\n
Previous value of variable will be retained.\n\n"
/*
** This IS a SIprintf message to stdout--
** Explanation:
**	Incompatible value assigned to a variable via command line or with value
**
** System Status:
**
** User Action:
*/
E_RW140F_
"Incompatible datatype in variable assignment in %0c text for\n
'%1c'.  An explicit conversion function may be required.\n
Previous value of variable will be retained.\n
Line with error: '%2c %3c'.\n
Processing continues . . ."
/*
** Explanation:
**	Incompatible value assigned to a variable via .let
**
** System Status:
**
** User Action:
*/
E_RW1410_
"E_RW1410 Divide by zero error.\n"

/*
** This IS a SIprintf message to stdout--
** Explanation: This should never happen...
**	Called by exception handler in rfixparl
**	which attempts to convert a string to an
** 	appropriate type by way possibly of adc_cvinto
**
** System Status:
**	Will loop back and prompt user for better result.
**
** User Action:
*/

E_RW1411_
"E_RW1411 Assignment overflow error\n"

/*
** This IS a SIprintf message to stdout--
** Explanation: This could happen...
**	Called by exception handler in rfixparl
**	which attempts to convert a string to an
** 	appropriate type by way possibly of adc_cvinto
**
** System Status:
**	Will loop back and prompt user for better result.
**
** User Action:
*/

E_RW1412_
"E_RW1412 Assignment underflow error.\n"

/*
** This IS a SIprintf message to stdout--
** Explanation: This could happen...
**	Called by exception handler in rfixparl
**	which attempts to convert a string to an
** 	appropriate type by way possibly of adc_cvinto
**
** System Status:
**	Will loop back and prompt user for better result.
**
** User Action:
*/

E_RW1413_do_not_modify
"*  DO NOT MODIFY.  DOING SO MAY CAUSE REPORT TO BE UNUSABLE."


/*
** Explanation:
**     Comments when copying or archiving a report out.
**
** System Status:
**     Not applicable.
**
** User Action:
**     Not applicable.
*/

E_RW1414_ignored_attrib
"Attribute '%0c' is an unsupported datatype, and cannot\n
be included in the report.  Processing continues."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_RW1415_
"Decimal overflowed while in expression in\n %0c
text for '%1c'. Line with error:\n
'%2c %3c'.\n
Processing continues . . ."
/*
** Explanation:
**	Decimal overflow from r_g_double--could have been from rgexpr() or 
**	rpagg()
**
** System Status:
**
** User Action:
**	Change integer constants in report to be within valid integer range
*/

E_RW1416_
"All attributes are of an unsupported datatype, and so no\n
report can be generated and/or run.  Processing stops."
/*
** Explanation:
**	r_m_dsort() could not find any participating columns in the report.
**	Note that RBF ChooseColumns ignores unsupported datatypes, and
**	prohibits pre-deletion of all columns.
**
** System Status:
**
** User Action:
**	The report cannot be run with the specified target list.  If the
**	target list is "SELECT *" (e.g., RBF), then a report cannot be
**	generated for the specified table(s).
*/

E_RW1417_BadDecimal	"A bad value was given in II_APP_DECIMAL.\n
The character `%0c' specified by the logical variable II_APP_DECIMAL is not an
acceptable decimal point character.   The value in II_DECIMAL or the \
character `.' will be used instead. Set the INGRES logical variable \
II_APP_DECIMAL to a valid decimal point character."

/*%
** Explanation:
**	The character given in II_APP_DECIMAL is not a valid decimal point
**	character.  This is the character you have used in RW source
**	statements to specify numeric literals that can have a fractional part,
**	such as money values or real numbers.  Valid decimal point characters
**	are `.' or `,'.  Important note: this logical symbol is provided
**	only for backward compatibility.  It is recommended that you use
**	a decimal point character of `.' in your RW source code,
**	and leave II_APP_DECIMAL unset (it will default to `.'),
**	or the (valid) value in II_DECIMAL.
**
** System Status:
**	The ReportWriter will assume that your statements will use
**	a decimal point character of `.' to specify the fractional parts
**	of numeric literals.  If your statements in fact use some other
**	character, a syntax error will be reported.
**
** User Action:
**	Set a valid single-character string for the II_APP_DECIMAL name.
*/


E_RW1418_	"An incorrect value was passed for flag 'numeric_overflow'.\n
Valid values are 'ignore', 'fail', 'warn'\n
Defaulting numeric_overflow to 'fail'\n"

/*%
** Explanation:
** An incorrect value was passed for flag 'numeric_overflow'. 
** Valid values are 'ignore', 'fail', 'warn'. 

** System Status:
** Defaulting numeric_overflow to 'fail'
**
** User Action:
** None
*/


