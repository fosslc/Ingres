/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ermo.msg -	Message file for class MO
**
** Description:
**	This file defines the messages for class MO.
**
** History:
**	12-aug-1987 (daver)
**		Created.
**	10-mar-1989 (kathryn)	- Print blank line before printing rowcount.
**	03-may-1989 (marian)
**		Changed the copyright banner so it now says 1989 instead of
**		1986.  F_MO0033_TM_login_banner.	
**	02-nov-1989 (sandyd)
**		Removed the redundant copyright message from F_MO0033.
**	07-nov-1989 (teresal)
**		Rewrote 'Query buffer full' F_MO000B message to be more
**		informative.  Bug #8049.
**	14-mar-1990 (teresal)
**		Added warning message for input query missing an ending quote.
**	29-aug-1990 (kathryn)
**		Changed E_MO003D Not to print CMS file_spec syntax. 
**		Changed writeout.c to check for this error and print this 
**		message on all platforms.
**	31-aug-1990 (kathryn)
**		Fixed above change to Add new error message rather change old
**		one.
**	09-nov-1990 (kathryn)
**		Added E_MO006F_P_FlagSyntax and E_MO0070_R_FlagSyntax
**	08-feb-1993 (rogerl)
**		Added E_MO0071_R_NoCmdLnPass
**	10-feb-1993 (rogerl)
**		Back out '08-feb-1993 (rogerl)'.  Not needed & message never
**		shipped.
**	13-jan-1993 (mgw)
**		Change INGRES to OpenINGRES in F_MO0033_TM_login_banner and
**		F_MO0034_INGRES_exit_message.
**      21-Mar-1995 (liibi01)
**              Bug 67115, modify the login message from terminal monitor.
**	31-may-1995 (harpa06)
**		Bug #68876 - Added an error message that will appear if the 
**		user is trying to invoke QUEL with a STAR database since STAR
**		is currently not supported by QUEL. This error message will no
**		longer be used once the STAR catalogs are supported.
**	23-aug-95 (emmag)
**		Changed OpenINGRES to OpenIngres.
**	23-apr-1998 (hanch04)
**		Changed OpenIngres to Ingres. 
**	27-apr-1999 (mcgem01)
**		The login and logout banners shouldn't have the product
**		name hardcoded within them.
**	07-jun-1999 (somsa01)
**		Modified the last change to take out the first "%s". The
**		files which use these banners will have to append the
**		product name to them before usage.
**      10-Apr-2003 (hanal04) Bug 109389 INGSRV 2041
**		Modified F_MO003C to E_MO003C so that it is included
**              in the slow mnx file and can be retrieved by IIUGerr().
**      31-aug-2004 (sheco02)
**          	X-integrate change 467391 to main.
**      12-aug-2009 (maspa05) trac 397, SIR 122324
**          	messages associated with setting silent mode and related
**              options
**/

# define	_MO_CLASS	98
/*
**	mac.c
*/
/*
**	Macro's will execute the same code (loop) an infinite time due to
**	a coding error.
*/
F_MO0001_Infiniteloop_in_macro	"Infinite loop in macro\n"
/*
**	A note should be made about these fast strings which seem like error
**	messages. They are error messages, but they are "putprintf'd" to
**	the screen, rather than using standard forms output mechanisms.
**	The terminal monitor is SPECIAL in this way in that it is an
**	interactive NON-FORMS program. Hence the error messages going in fast
**	strings to be printed in the TM's own way.
*/
/*
**	The parameter used in the macro was undefined.
*/
F_MO0002_undef_param_reference	"define: parameter %c referenced but not defined\n"
/*
**	No delimiter was supplied for this parameter.
*/
F_MO0003_no_delim_for_param	"define: every parameter needs a delimiter\n"
F_MO0004_null_param_name	"define: null parameter name\n"
/*
**	The given parameter has already been defined.
*/
F_MO0005_parameter_redeclared	"define: parameter %c redeclared\n"
/*
**	A primative was found in the macro processor.
*/
F_MO0006_Primitive_xxx	"Primitive xxx"
/*
**	branch.c
*/
F_MO0007_Cannot_branch_n	"Cannot branch\n"
F_MO0008_Cannot_branch_on_term	"Cannot branch on a terminal\n"
F_MO0009_no_branch_at_toplevel	"Unable to branch at top nesting level\n"
/*
**	clear.c
*/
/*
**	The string before the "go" is the bell character or nothing,
**	depending on whether dear user wants us to beep after every 
**	statement or not. "go" is printed after \r is hit to "reset"
**	the query buffer.
*/
F_MO000A_beep_go	"%sgo\n"
/*
**	clrline.c
*/
F_MO000B_Query_buffer_is_full	"\nThe query input is too large to handle.\nCheck your input for mismatched quotes."
/*
**	The input has filled the query buffer.  The query input is just
**	too large for the TM to process.  The user is advised to look for
**	an odd number of quotes as this can cause the input to be treated as
**	one long string and thus, inadvertently fill up the query buffer.
*/
/*
**	edit.c
*/
F_MO000C_Cant_create_qry_file	"Can't create query file '%s' for editor.\n\t%s\n"
/*
**	The above message occurs if there was an error invoking the editor.
**	The following message is what is printed when the editor is 
**	invoked, that is, this is printed while waiting for the editor
**	to come up.
*/
F_MO000D_editor_prompt	">>editor\n"
F_MO000E_Can_t_start_up_editor	"Can't start up editor: %s\n"
F_MO000F_Cant_delete_file	"Can't delete file %s: %s\n"
/*
**	eval.c
*/
/*
**	expr.c
*/
F_MO0010_Extra_paren_found	"Extra parenthesis found: assumed typo.\n"
F_MO0011_Insufficient_parens	"Insufficient parenthesis found: Assumed zero.\n"
F_MO0012_Expression_expected	"Expression expected: end of expression found.\n"
F_MO0013_Expression_expected__	"Expression expected; '%c' found: Assumed zero.\n"
F_MO0014_Operator_expected	"Operator expected: '!%c' found.\n"
F_MO0015_Oper_expected_found_c	"Operator expected: '%c' found.\n"
F_MO0016_Divide_by_zero	"Divide by zero: zero assumed.\n"
/*
**	getfname.c
*/
F_MO0017_filename	"filename \"%s\"\n"
/*
**	go.c
*/
F_MO0018_Executing_dot_dot	"Executing . . .\n\n"
F_MO0019_continue	"continue"
/*
**	The above two strings need commenting. "Executing . . ."
**	is printed after every retrieve or anything which requires
**	a "\g" to run it. This is paired with "continue". If there is 
**	no output from the query entered, then just those two strings,
**	separated by a couple of blank lines, will be printed.
**	--
**	These next two strings indicate how many rows were affected
**	by the command. 1 row is singular, not 1 is plural.
*/
F_MO001A_1_row	"(1 row)\n"
F_MO001B_N_rows	"(%d rows)\n"
/*
**	include.c
*/
/*
**	include.c
*/
F_MO001C_Error_in_include_file	"Error in include file name '%s'%s:\n%s\n"
F_MO001D_Error_opening_include	"Error opening include file %s%s:\n\t%s\n"
F_MO001E_Include_nest_too_deep	"Include nested too deep\n"
F_MO001F_input	"<<input>>"
/*
**	monitor.c
*/
F_MO0020_Transcript_log_is_off	"Transcript logging is off.\n"
F_MO0021_script_ing	"script.ing"

F_MO0022_Transcript_log_into_f	"Transcript logging into `%s'.\n"
F_MO0023_Cant_open_script_f	"Can't open %s: %s\n"
F_MO0024_Nonprint_char_to_blan	"Non-printing character 0%o converted to blank\n"
F_MO0025_reset__does_nothing	"reset: does nothing"
F_MO0026_trace__does_nothing	"trace: does nothing until I can figure it out."
/*
**	newdirec.c
*/
F_MO0027_Error_in_path_name	"Error in path name: %s\n"
F_MO0028_Cannot_change_dir	"Cannot change to directory %s: %s\n"
/*
**	prompt.c
*/
/*
**	Although this message "go" is essentially the same as the
**	"go" message above (i think its called go_beep, above), these
**	two translations for "go" should be kept consistent.
*/
F_MO0029_go	"go"
/*
**	qbuf.c
*/
F_MO002A_Warning_qry_buf_full	"\007Warning: query buffer full, characters dropped.\n"
F_MO002B_Warning__query_buffer	"Warning: query buffer full, characters dropped.\n"
/*
**	quit.c
*/
F_MO002C_In_a_trans_Wanna_quit	"You are in a transaction.  Do you wish to abort your transaction and quit? "
F_MO002D_Pending_trans_aborted	"A pending transaction was aborted.\n"
F_MO002E__STAR	"/STAR"
/*
**	tmmain.qc
*/
E_MO0001_Cant_init_ADF_ctrlblk	"Terminal Monitor: could not initialize ADF control block"
E_MO002F_Cant_set_exc_handler	"Terminal Monitor: could not set exception handler"
F_MO0030_TERMINAL_MONITOR	"TERMINAL MONITOR"
F_MO0031_1981	"1981"
F_MO0032_No_database_name	"No database name specified\n"
/*
**	Below is the standard Terminal Monitor Banner.
*/
F_MO0033_TM_login_banner	"%s%s Version %s login\n%s\n"
F_MO0034_INGRES_exit_message	"%s Version %s logout\n%s\n"
F_MO0035_Error_finding_dayfile	"Error in finding dayfile--%s\n"
F_MO0036_while_run_sys_init_fi	"\nwhile running the system initialization file"
F_MO0037_Error_in_User_Startup	"Error in User Startup File Name %s: %s\n"
F_MO0038_while_run_INIT_INGRES	"\nwhile running the Terminal Monitor startup file"
F_MO0039_Error_Startup_FileNam	"Error in Terminal Monitor Startup File Name %s: %s\n"
F_MO003A_while_run_db_startupf	"\nwhile running the database start-up file"
F_MO003B_Interrupt	"\nInterrupt\n"
E_MO003C_Unknown_exception	"Unknown exception %0x: current query program aborted\n"
/*
**	writeout.c
*/
F_MO003D_w_requires_a_filename	"\\w requires a file name: '\\w fm:fn.ft'\n"
F_MO003E_Bad_file_name	"Bad file name \"%s\": %s\n"
F_MO003F_Cannot_open_file	"Cannot open file \"%s\": %s\n"

/*%
/*
**	quit.qc
*/
F_MO0040_SQL_stmt_committed	"Your SQL statement(s) have been committed.\n\n"
/*
**	tmmain.qc
*/
F_MO0041_More_than_one_db	"More than one database name specified\n"
/*
**	quit.qc
*/
F_MO0042_Warn_missing_quote	"Warning: The input query is missing an ending quote.  Part of the input\nmight have been ignored.  Check your input for mismatched quotes.\n\n"
F_MO0043_W_filename_required	"Write: File Name Required.\n"
/*%
** Explanation:
**	No filename was given with the \write command.
**	User has issued write command without indicating the name of a
**	file to write to.
**
** System Status:
**	No Change.
**
** User Action:
**	Re-enter \w command with filename: "\w <filename>."
**	
*/
F_MO0044_Titles_on	"Column titles set to on\n"
F_MO0045_Titles_off	"Column titles set to off\n"
F_MO0046_Silent_on	"Entering silent mode\n"
F_MO0047_Silent_off	"Exiting silent mode\n"
F_MO0048_vdelim_set	"Vertical column separator set to '%s'\n"
F_MO0049_vdelim_reset	"Vertical column separator reset to default\n"
F_MO0050_Padding_on     "Output will be space-padded to column width\n" 
F_MO0051_Padding_off    "Output will be trimmed (no space-padding)\n"
/*
**	former ipanic messages
*/
E_MO0041_1500100	"In branch: %0c"
E_MO0042_1500200	"In macro processor: %0c"
E_MO0043_1500300	"clear: q_ropen"
E_MO0044_1500400	"edit: q_ropen 1"
E_MO0045_1500401	"edit: q_ropen 2"
E_MO0046_1500402	"edit: q_ropen 3"
E_MO0047_1500403	"can't reopen editfile %0c: %1c"
E_MO0048_1500404	"Error reading edit file: %0c"
E_MO0049_1500500	"eval: q_open"
E_MO004A_1500501	"eval: q_ropen 1"
E_MO004C_1500503	"eval: q_ropen 2"
E_MO004D_1500600	"exp_op: bad op %0d"
E_MO004E_1500601	"popop: underflow"
E_MO004F_1500602	"popnum: underflow"
E_MO0050_1500700	"go: ropen 1"
E_MO0051_1500701	"go: ropen 2"
E_MO0052_1500800	"macro: bad primitive %0d"
E_MO0053_1500900	"monitor: bad code %0d"
E_MO0054_1500901	"monitor: bad $ code %0d"
E_MO0062_1501301	"ttymon: qbuf fail"
E_MO0063_1501302	"tm_intr: typ %0d"
E_MO0068_1501400	"print: q_ropen 1"
E_MO0069_1501401	"print: q_ropen 2"
E_MO006D_1501700	"writeout: q_ropen"
E_MO006E_1501701	"writeout: q_ropen 2"

E_MO0002_Cant_use_macros	"Macro processing will not be performed when running the SQL terminal monitor,\nso \\macro is being ignored.\n"
/*%
** Explanation:
**	The user has typed \macro.  Since macro processing is not supposed to
**	exist in SQL, this command is ignored and the user notified.
**
** System Status:
**	No change.
**
** User Action:
**	None.
*/
E_MO0003_Unable_alloc_init	"Unable to allocate initial %0d bytes of memory for the\ninput query buffer.\n\n"
/*%
** Explanation:
**	The Terminal Monitor cannot allocate the initial memory needed for the
**	input query buffer.
**
** System Status:
**	The Terminal Monitor exits. 
**
** User Action:
**	
*/
E_MO0004_Unable_alloc_mem	"Unable to allocate enough memory (%0d bytes) for input query buffer.\nThe current go block will be ignored.\n\n"
/*%
** Explanation:
**	The user has typed in a long query which requires the Terminal 
**	Monitor to allocate more memory for the input query buffer; however,  
**	the memory allocation has failed.  The go block or query which ran out 
**	of memory will be ignored - it will not be sent to the server because 
**	the last query has probably been truncated.
**
** System Status:
**	The Terminal Monitor will ignore all characters till the next '\'
**	TM command. A '\g' will cause the query buffer to be reset.
**	The current go block will not be sent to the server.
**
** User Action:
**	Enter a '\g' or '\r' to reset the query buffer and continue processing
**	queries.
*/
E_MO006F_P_FlagSyntax "dbname [-P] [ flags ]"
E_MO0070_R_FlagSyntax "dbname [ -Rroleid ] [ flags ]"
/*%
** Explanation:
**      A password was included with the startup flags -P or -R .
**      User must wait for a prompt for these passwords. If password
**      is included with startup flags then a list of running processes
**      would show that password.
**
**
** System Status:
**
** User Action:
**      The user must not include the password with these startup flags.
**      For the -R flag this includes -Rrole_id/ (ending with "/") indicating
**      that no password is required.
*/
E_MO0071_STAR_not_supported "QUEL is not supported in this release of distributed INGRES."
/* 
** Explanation:
**	QUEL is not compatible with the current STAR catalogs. STAR is driven
**	entirely by SQL semantics.  The STAR portion of OPC takes the best
**	CO-tree and converts it into a QP containing SQL queries that are
**	forwarded to the DBMSs (Ingres and others) that actually own the tables
**	involved in the query.  QUEL and SQL have some differences that are not
**	easily smoothed over. 
**
**	This message will be in effect until the STAR catalogs are supported by
**	QUEL.
**
** System Status:
**
** User Action:
**	The user must use SQL to perform queries on a STAR database.
*/
