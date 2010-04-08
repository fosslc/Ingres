/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ermf.msg -	Interactive Terminal Monitor
**					Message Definitions File.
** Description:
**	Contains the definitions for the errors and messages of the
**	interactive terminal monitor, class MF.
**
** History:
**	Revision 6.0  87/12/12  daver
**	Created.
**	22-feb-1988 (neil)
**		Modified all dave's messages to make them actually work.
**		Obviously these were never tested.  I should have kicked
**		someone's butt!
**	24-apr-1989 (kathryn)
**		Added messages for the OnError option.
**	25-may-1989 (teresal)
**		Added messages for InsertLine and DeleteLine options.
**	20-jul-1989 (teresal)
**		Bug # 5807 Changed parameter to "%0" for E_MF000C. ERlookup
**		was crashing because number of parameters didn't match.
**	21-jul-1989 (teresal)
**		Bug # 6597 Added error message for query buffer full.
**	03-aug-89 (sylviap)
**		Added F_MF0021_Print_this_output, S_MF2103_Printing_output,
**		S_MF2104_RW_output_helptitle and S_MF2105_Overwrite_file
**		for the new output PRINT menu item and RW scrollable output.
**	16-oct-89 (sylviap)
**		Took out msgs for printing, and moved them to eruf.msg.
**	20-mar-1990 (teresal)
**		Added error message for dynamically allocated query buffer
**		unable to allocate more memory. Bug 8389.
**	01-may-90 (sylviap)
**		Added S_MF2103 - S_MF2106.
**	09-nov-90 (kathryn)
**		Added E_MF2005_P_FlagSyntax and E_MF2006_R_FlagSyntax.
**	31-may-1995 (harpa06) Bug #68876
**		Added E_MF0014 since QUEL is not currently supported by STAR.
**/

# define	_MF_CLASS	97
/*
**	fsaddrec.c
*/
E_MF0001_Unable_to_open_file	"Unable to open file '%0c' (rc=%1d)"
/*
**	The previous message refers to the 'spill' fill which it
**	can't open. 
*/

/*
**	fsdisp.c
*/

/*
**	These next few fast strings are loaded into a table of
**	menu items and their explanations. These strings are the
**	explanations for the menu items in the order of:
**		top, bottom, file, help, end.
*/
F_MF0002_Go_to_top_of_output	"Go to top of output"
F_MF0004_Go_to_bottom_of_outpu	"Go to bottom of output"
F_MF0005_Save_this_output	"Save this output"


/*
**	help menu item
*/
F_MF0006_Help_on_the_form	"Help on the form"
/*
**	end menu item
*/
F_MF0007_End_this_display	"End this display"
S_MF0002_FSTM_output_helptitle	"Interactive Terminal Monitor Output"
S_MF0003_Run_to_completion	"Run the request to completion"
E_MF0004_Unable_to_close_file	"Unable to close file '%0c' (rc=%1d)"
E_MF0005_Unable_open_file_read	"Unable to open file '%0c' for read (rc=%1d)"
E_MF0006_Unexpect_EOF_on_file	"Unexpected EOF on file '%0c' (rc=%1d)"
E_MF0007_Unable_open_file_apnd	"Unable to open file '%0c' for append (rc=%1d)"
/*
**	fsedit.qc
*/
E_MF0008_Unable_open_temp_edit	"Unable to open temporary edit file (rc=%d0)"
S_MF0009_File_empty_table_unch	"File '%s' is empty; input table is unchanged"
E_MF000A_cant_open_f_tbl_unchg	"Unable to open file '%0c' for input (rc=%1d); input table is unchanged"
E_MF000B_cant_read_temp_file	"Unable to read temporary file '%0c'"
/*
**	fsinit.c
*/
E_MF000C_cant_open_temp_browse	"Unable to open temporary browse file (rc=%0d)"
/*
**	fsload.qc
*/
S_MF000D_Enter_filenam_to_read	"Enter name of file to read: "
E_MF000E_File_does_not_exist	"File '%0c' does not exist"
E_MF000F_cant_open_file_input	"Unable to open file '%0c' for input (rc=%1d)"
E_MF0010_Unable_to_read_file	"Unable to read file '%0c'"
S_MF0011_Enter_filename_write	"Enter name of file to be written: "
E_MF0012_cant_open_file_output	"Unable to open file '%0c' for output (rc=%1d)"
E_MF0013_Unable_to_write_file	"Unable to write file '%0c'(rc=%1d)"
/*
**	fsmore.c
*/
/*
**	fsmore.c
*/
/*
**	fspanic.qc
*/
S_MF0014_FSTM_Terminating_msg	"FSTM Terminating - %s\n\n"
/*
**	fsprnscr.qc
*/
F_MF0008_printer	"printer"
S_MF0015_Enter_file_name	"Enter file name: "
E_MF0016_Error_opening_file	"Error opening file '%0c'"
S_MF0017_Writing_screen_to_fil	"Writing screen to '%s' file..."
S_MF0018_Appending_screen_to_f	"Appending screen to '%s' file..."
F_MF0009_Output	"Output"
F_MF000A_top_of_file_header	"___ %s ___________________________________________________\n"
/*
**	This previous 'wierd' string is the top of the output file to
**	be printed.
*/
E_MF0019_Unexpected_EOF_on_fil	"Unexpected EOF on file '%0c' (code=%1d)"
/*
**	The next string is the "footer" of the same output file.
*/
F_MF000B_output_file_footer	"________________________________________________________________________________\n"
E_MF001A_Error_closing_file	"Error closing file '%0c'"
E_MF001B_Error_printing_screen	"Error attempting to print screen."
/*
**	fsrecbe.c
*/
F_MF000C_End_of_Request	"End of Request"
F_MF000D_Terminated_by_Errors	" - Terminated by Errors"
/*
**	fsrun.qsc
*/
F_MF000E_Enter_SQL_Statements	"Enter SQL Statements"
F_MF000F_Enter_QUEL_Statements	"Enter QUEL Statements"
F_MF0010_Terminated_Interrupt	"*** Request Terminated by Interrupt ***"
S_MF001C_no_query_req_ignored	"There is no query - request ignored"
S_MF001D_Run_the_request	"Run the request"
S_MF001E_No_active_request	"No active request; cannot 'Resume'"
S_MF001F_Resume_the_request	"Resume the request"
/* 
**	OnError menu items
*/
F_MF0030_OnError	"OnError"
F_MF0031_Terminate	"Terminate"
F_MF0032_Continue	"Continue"
S_MF0031_TERMINATE	" TERMINATE"
S_MF0032_CONTINUE	" CONTINUE"
S_MF0033_Curr_Continue	"Statement processing will CONTINUE on error."
S_MF0034_Curr_Terminate "Statement processing will TERMINATE on error."
S_MF0035_Select_Continue	"Select CONTINUE and enter 'GO' if you wish to
continue"
S_MF0036_Select_Terminate	"Select TERMINATE and enter 'GO' if you wish to terminate"
S_MF0037_Choice_Terminate "Do not continue processing the remaining statements."
S_MF0038_Choice_Continue  "Continue processing the remaining statements."
F_MF0039_continue	"continue"
S_MF0040_ExplOnError	"Specify action to take when an error is encountered."
S_MF0042_ExplTerminate	"Invoke the TERMINATE option."
S_MF0043_ExplContinue	"Invoke the CONTINUE option."
/*
**	FSRUN.QSC EXPL messages
*/
S_MF0051_ExplBlank	"Erase the input screen."
S_MF0052_ExplEdit	"Escape to editor."
S_MF0053_ExplFile	"Read/Write specified File."
S_MF0054_ExplQuit	"End Session."
S_MF0055_ExplInsertLine	"Insert blank line in input screen."
S_MF0056_ExplDeleteLine	"Delete line in input screen."
/*
**	fsrun.qsc
*/
S_MF0057_query_buffer_full	"The query buffer is full - request ignored"
E_MF0027_unable_alloc_qb	"Unable to allocate enough memory (%0d bytes) for query buffer - request ignored"
/*
**	complete menu item
*/
F_MF0011_Complete	"Complete"
/*
**	read menu item
*/
F_MF0012_Read	"Read"
/*
**	write menu item
*/
F_MF0013_Write	"Write"

/*
**	InsertLine and DeleteLine menu items
*/
F_MF0018_InsertLine	"InsertLine"
F_MF0019_DeleteLine	"DeleteLine"

S_MF0020_FSTM_File_help_banner	"Interactive Terminal Monitor File"
S_MF0021_FSTM_input_helpbanner	"Interactive Terminal Monitor Input"
S_MF0022_in_a_trans_wanna_quit	"You are in a transaction.  Do you wish to abort the transaction and quit? "
S_MF0024_OnError_help_banner	"Interactive Terminal Monitor OnError"
S_MF0028_LineEdit_help_banner	"Interactive Terminal Monitor LineEdit"
/*
**	fssetup.qc
*/
/*
**	This is the message that only one row was processed. The next
**	message indicates how many rows were processed (second message is
**	plural).
*/
F_MF0014_1_row	"(1 row)\n"
F_MF0015_d_rows	"(%d rows)\n"
/*
**	fstm.c
*/
F_MF0016_ISQL	"ISQL"
F_MF0017_IQUEL	"IQUEL"
E_MF0023_Couldn_t_open_debug_f	"Couldn't open debug file\n"	/* DIAG */

E_MF0025_TempNoWrite	"Local or II_TEMPORARY directory NOT writable.\n
The current working directory or the temporary files directory (specified
by II_TEMPORARY) cannot have files created in it.  Please check that you
have access rights to the directory and that sufficient disk space exists
to create and write files there."

/*%
** Explanation:
**	The current working directory or the II_TEMPORARY directory cannot
**	have files created in it.  ISQL and/or a gateway may, at different
**	times, require that a temporary file be created.
**	
**
** System Status:
**
** User Action:
**	The user must change the access rights to the current working
**	directory or point II_TEMPORARY to a writable directory.
*/

/*
**	ftdraw.c
*/
F_MF001A_End_of_Output	"End of Output"
F_MF001B_Start_of_Output	"Start of Output"
F_MF001C_Scroll_down_for_more	"Scroll down for more"
F_MF001D_Start_of_in_memory_ou	"Start of in-memory output; use 'Top'"
F_MF001E_Column_header_info	"Column %d/%d  "
F_MF001F_Line_header_info	"Line %d/%d"
/*
**	The previous entry has the extra %d/%d to indicate that we're at
**	line N of M. This next entry doesn't know yet how many lines there
**	are, so only print Line N.
**	.
*/
F_MF0020_Line_info_inc	"Line %d"

/*
**	This next error was 'hand added', used in IIUGerr if
**	the status handed back by IIUFlocateform is bad
*/
E_MF0026_Bad_Get_Form	"Can't initialize forms"

E_MF2001_Syntax	"database [DBMS flags]"
E_MF2002_TooManyArgs	"Too many arguments specified for Ingres.  Last flag is '%0c'"
E_MF2003_TooManyDBs	"Only one database may be specified.  Last argument is '%0c'"
E_MF2004_DBrequired	"Database name must be specified."

E_MF2005_P_FlagSyntax   "dbname [-P] [ flags ]"
E_MF2006_R_FlagSyntax   "dbname [ -Rroleid ] [ flags ]"
/*%
** Explanation:
**      A password was included with the startup flags -P or -R .
**      User must wait for a prompt for these passwords. If password
**	is included with startup flags then a list of running processes
**	would show that password. 
**
**
** System Status:
**
** User Action:
**	The user must not include the password with these startup flags.
**	For the -R flag this includes -Rrole_id/ (ending with "/") indicating
**	that no password is required.
*/

S_MF2101_Pending_trans_aborted
"A pending transaction was aborted.\n"

S_MF2102_SQL_stmt_committed
"Your SQL statement(s) have been committed.\n\n"

S_MF2103_SQL_print_title
"ISQL - Sending the Query Results to a Printer"

S_MF2104_SQL_file_title
"ISQL - Sending the Query Results to a File"

S_MF2105_QUEL_print_title
"IQUEL - Sending the Query Results to a Printer"

S_MF2106_QUEL_file_title
"IQUEL - Sending the Query Results to a File"

E_MF2107_STAR_not_supported	"QUEL is not supported in this release of distributed INGRES."
/*
** Explanation:
**      QUEL is not compatible with the current STAR catalogs. STAR is driven
**      entirely by SQL semantics.  The STAR portion of OPC takes the best
**      CO-tree and converts it into a QP containing SQL queries that are
**      forwarded to the DBMSs (Ingres and others) that actually own the tables
**      involved in the query.  QUEL and SQL have some differences that are not
**      easily smoothed over.
**
**      This message will be in effect until the STAR catalogs are supported by
**      QUEL.
**
** System Status:
**
** User Action:
**      The user must use SQL to perform queries on a STAR database.
*/

