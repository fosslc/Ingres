/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	errf.msg -	Message file for class RF
**
** Description:
**	This file defines the messages for class RF.
**
** History:
**	 5-aug-1987 (rdesmond)
**		Created.
**    	12-jul-89 (sylviap)
**            	Added F_RF0031, 32 and S_RF0051 and 52  for direct printing.
**		Also added CMR's changes F_RF0033 - 36 and Marty's changes 
**		  S_RF004E - 50.
**    	01-dec-89 (sylviap)
**		Added E_RF0054 - 59 so RBF can generate .DECLARE's for
**		runtime parameters (rfmdecl.c).
**	12/13/89 (elein)
**		Added lots of messages for listpick messages and
**		rfcat changes.  Changed "AutoReport" to "Preview"
**		per FRC decision.
**      15-dec-89 (sylviap)
**              Added F_RF0041 for the declared variables screen/batch reports.
**              Changed F_RF0032 to "Variables".
**              Added E_RF0064 for parsing declared variables.
**	12/18/89 (elein)
**		Fixed fast vs. error and slow message sequences for new
**		messages.  This is the score:
**			FAST: F_RF0053	SLOW/ERROR: S/E_RF0062
**	12/27/89 (elein)
**		rearranged some list pick data for styles, added S_RF0063
**	1/4/89  (martym) 
**		Added E_RF0071. 
**      05-jan-90 (sylviap)
**              Added E_RF0072_Need_errorlog for batch reports.
**	1/8/90 (elein)
**		Changed report style descriptions, hopefully improving them
**	1/11/90 (elein)
**		Changed the word title to titles in style descrip for block
**	1/12/90 (elein)
**		Added F_0005D, 5E & 5F (tabular, indented, label) for command 
**		  line modes;
**		changed loading	"old" report S_RF0029 to loading report
**      12-jan-90 (sylviap)
**		Added RF006A - 6D for help files.
**	1/23/90 (martym)
**		Added F_RF0062_Break_Columns.
**	2/2/90 (martym)
**		Added S_RF006E_Saving_JDrep___s__fo.
**      16-feb-90 (sylviap)
**		Added S_RF006F, S_RF0075 - S_RF007E.
**		Added F_RF0063 - F_RF006A.
**	3/1/90 (martym)
**		Added E_RF0080.
**      15-mar-90 (sylviap)
**		Added E_RF0082 - E_RF0084.
**      27-mar-90 (sylviap)
**		Added S_RF0085 and changed ellipses. . .
**	4/2/90 (martym)
**		Added E_RF0086.
**      02-apr-90 (sylviap)
**              Added F_RF006B and S_RF0087.
**	4/3/90 (martym)
**		Added a parameter to S_RF0087.
**	12-apr-90 (sylviap)
**		Added E_RF0088 for jupbug #20393.
**	16-apr-90 (sylviap)
**		Added S_RF000A to be consistent with r63.
**	19-apr-90 (sylviap)
**		Added S_RF0015.
**	5/29/90 (martym)
**		Added S_RF008B_ - S_RF008E_.
**	6/6/90 (martym)
**		Added F_RF0072.
**	6/22/90 (martym)
**		Added E_RF0090 - 91, and changed the wording of 
**		E_RF004A.
**	27-jul-90 (sylviap)
**		Added E_RF0092 - 98.
**	04-sep-90 (sylviap)
**		Added F_RF0073 - F_RF0088. 
**	12-oct-90 (sylviap)
**		Added F_RF0089 - F_RF008A. #32781 
**	23-oct-1990 (mgw)
**		Corrected some misspellings.
**	09-nov-90 (sylviap)
**		Added E_RF0099 - E_RF009E. #33894 
**	18-mar-91 (sylviap)
**		Added E_RF009F - E_RF00A1 and F_RF008B - F_RF008E. 
**	10-jan-92 (pearl)
**		bug 41791.  Change parameter name to rpt_object in E_RF0088, 
**		and also make command line look like the one in utexe.def, 
**		which is what is used when user gets error E_UG0011.  Leave
**		out -table flag for now, since it isn't to be publicized 
**		until 6.5.
**	4-nov-1992 (rdrane)
**		Add message E_RF00A2_Invalid_table_name.
**	12/05/92 (dkh) - Added a bunch of messages for Choose Column support.
**      05-jan-93 (sylviap)
**              Added E_RF00A9 - E_RF00AC and F_RF009C_EditExpl.
**	27-jan-1993 (rdrane)
**		Added E_RF00AD_XNS_QUEL.
**	12-mar-93 (essi)
**		Added message E_RF001B for bug 45261.
**	24-mar-1993 (lan)
**		Added message E_RF0007 for bug 49656.
**	01-jul-93 (sylviap)
**		Added S_RF00AE_first_ff - E_RF00B3_page_format for Report
**		Options to support first formfeed and editing the date, time
**		and page stamp.
**	7-jul-1993 (rdrane)
**		Added E_RF00B5_NoSuppCols.
**	23-jul-93 (sylviap)
**		For listchoices in report options screen, changed S_RF00AE &
**		S_RF00AF, added F_RF009D - F_RF00A4, S_RF00B6 - S_RF00B7. 
**	28-oct-1993 (rdrane)
**		Re-word date/time/pageno format error messages as part of
**		fix for bugs 55141 and 55163.
**	17-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'support representative'
**		with 'Site Technical Contact'.
**/

# define	_RF_CLASS	93

/*
**	rfafield.c
*/


E_RF0001_rFafield__Bad_att_ord	"rFafield: Bad att ordinal.\n
Please contact your Site Technical Contact for assistance."

/*%
 **Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0002_rFafield__Can_t_get_f	"rFafield: Can't get format string.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0003_rFafield__Can_t_get_f	"rFafield: Can't get format size.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

/*
**	rfaginfo.c
*/

E_RF0004_rFget_agnms__Can_t_al	"rFget_agnms: Can't alloc mem.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0005_rFget_agnms__Can_t_ge	"rFget_agnms: Can't get numags.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0006_rFget_agnms__Can_t_ge	"rFget_agnms: Can't get agnames.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0007_OSError	"Report failed with Operating System error message:\n
'%0c'"

/*
** Explanation:
**	The report failed and generated the Operating System level error
**	message shown.
**
** System Status:
**	The report failed.
**
** User Action:
**	Consult your System Administrator.
*/

S_RF000A_CreateWideReport
"Report is %0d characters wide.  Override the -l value of %1d? (yes/no): "
  
/*%
** Explanation:
**      User has selected a table and a style that goes beyond the -l setting.
**      RBF is prompting user to continue, overriding the -l value.
 **
** System Status:
**      Waits for user input.
**
** User Action:
**      Choose to either create the wide report or not.
*/

E_RF000B_rFinit_aginfo__dataty	"rFinit_aginfo: datatype not in AGG_TYPE.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

/*
**	rfcatalog.qc
*/

F_RF0001_RBF___Report_Catalog	"RBF - Report Catalog"
S_RF000C_Retrieving_report___s	"Retrieving report '%0c'. . ."
S_RF000D_Enter_table_name_to_u	"Enter table name and press <RETURN> : " 
F_RF0002_BlockMode	"BlockMode"
F_RF0003_ColumnMode	"ColumnMode"
F_RF0004_WrapMode	"WrapMode"
F_RF0005_DefaultMode	"DefaultMode"
S_RF000E_Create_Report_Submenu	"Create Report Submenu"

E_RF000F_Report_does_not_exist	"Report does not exist.\n
Try selecting a report that exists"

/*%
** Explanation:
**	In expert mode, the user has specified a report which does not exist.
**
** System Status:
**	Program continues.
**
** User Action:
**	User should select a report which exists.
*/

S_RF0010_Writing_report_defini	"Writing report definition to temporary file."

E_RF0011_Could_not_write_repor	"Could not write report to file.\n
Check available disk space and write permissions, then contact your
Site Technical Contact"

/*%
** Explanation:
**	During editing of a non-RBF report or archiving a report, the report
**	could not be written to a file.
**
** System Status:
**	Program continues.
**
** User Action:
**	User can check write permission and available disk space.
*/

S_RF0012_Now_in_system_editor	"Now in system editor."
S_RF0013_Do_you_wish_to_load_t	"Do you wish to load the report into the database ?"
S_RF0014_Retrieving_report___s	"Retrieving report '%0c'. . ."
S_RF0015_Archiving_Report	"Archiving a Report"
S_RF0017_Enter_file_name_	"Enter file name: "
S_RF0018_Writing___s__to_file_	"Writing '%0c' to file '%1c'. . ."
S_RF001A_Utilities_Submenu	"Utilities Submenu"
S_RF001C_Report_Catalogs	"Report Catalogs"
F_RF0006_RBF___Detail_on_a_rep	"RBF - MoreInfo about a Report"
F_RF0007_quel	"quel"
F_RF0008_sql	"sql"
F_RF0009_Query_Language_	"Query Language:"
/* B3751: FRC approved change.  Was "terminal is default."
   This is no longer true */
S_RF001D_Enter_file_name__term	"Enter file name or <RETURN> for default: "
S_RF001E_Running_report___s___	"Running report '%0c'. . ."
S_RF001F_Writing___s__to_file_	"Writing '%0c' to file '%1c'. . ."

E_RF0020_Deadlock_has_occurred	"Deadlock has occurred too many times.\n
Report NOT deleted from base catalogs."

/*%
** Explanation:
**	Backend deadlock during deletion of report.
**
** System Status:
**	Program continues.
**
** User Action:
**	Determine if database is working properly.
*/

E_RF0021_Error_occurred_in_del	"Error occurred in deleting report.\n
Report NOT deleted from base catalogs."

/*%
** Explanation:
**	Unspecified error while attempting to delete a report.
**
** System Status:
**	Program continues.
**
** User Action:
**	Not applicable.
*/

/*
**	rfcopt.qc
*/

F_RF000A_value	"value"
F_RF000B_range	"range"
F_RF000C_nosel	"none"
F_RF000D_When_to_print_values_	"When to print values:"
F_RF000E_break	"b"
F_RF000F_both	"t"
F_RF0010_none	"n"
F_RF0011__b_break__p_pages__t_	"(b=break, p=pages, t=both, a=always)"
F_RF0012_x	"x"
F_RF0013_Column_Options	"Column Options"

E_RF0022_Lines_to_skip_must_be	"Lines to skip must be numeric"

/*%
** Explanation:
**	On the column options frame, the user specified a non-numeric
**	value for the number of lines to skip after a break.
**
** System Status:
**	User may not leave field until a numeric value is entered.
**
** User Action:
**	User must enter a numeric value.
*/

E_RF0023_Lines_to_skip_must_be	"Lines to skip must be >= -1"

/*%
** Explanation:
**	On the column options frame, the user specified a large negative
**	value for the number of lines to skip after a break.
**
** System Status:
**	User may not leave field until a numeric value greater than or
**	equal to -1 is entered.
**
** User Action:
**	User must enter a numeric value greater than or equal to -1.
*/

F_RF0014_pages	"p"
F_RF0015_always	"a"
F_RF0016_neg_1_to_skip	"(-1 to skip to top of new page)"
F_RF0017_Lines_to_skip_on_brea	"Lines to skip on break:"

/*
**	rfcrack.c
*/

F_RF0018_default	"default"
F_RF0019_column		"column"
F_RF001A_wrap		"wrap"
F_RF001B_block		"block"
F_RF001C_Database	"Database"


/*
**	rfcsreset.c
*/

E_RF0025_rFcsreset___CS_struct	"rFcsreset.  CS structure not linked."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

/*
**	rfdisplay.qc
*/

E_RF0026_rFdisplay___Error_pro	"rFdisplay:  Error processing breaks."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0027_rFdisplay___Error_set	"rFdisplay:  Error setting up LAYOUT form."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0028_rFdisplay___Error_set	"rFdisplay:  Error setting up LAYOUT form."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

/*
**	rfropt.qsc
*/

F_RF001D_ul_last	"last"
F_RF001E_ul_none	"none"
F_RF001F_ul_all		"all"

/*
**	rfmpbot.c
*/

F_RF0020___Totals__PAGE__	"\"Totals: PAGE\""

/*
**	rfmptop.c
*/

F_RF0021_Report_	"Report: "


/*
**	rfmrfoot.c
*/

F_RF0022___Grand_Totals___REPO	"\"Grand Totals:  REPORT\""


/*
**	rfrbf.qc
*/

F_RF0023_RBF	"RBF"

/*
**	rfropt.qc
*/

F_RF0024_Report_Options	"Report Options"


/*
**	rfget.qc
*/

S_RF0029_Loading_old_report_sp	"Loading report specifications. . ."
S_RF002A_use_def_rep	"  Use default for table (y/n) ?"

E_RF002B_No_report__	"No report '%0c' found.%1c"

/*%
** Explanation:
**	The user specified the -r flag on the command line, but the report
**	doesn't exist.  If a table with the specified name exists, then
**	message S_RF002A is appended to the error message (in %1), and the user
**	may choose to create a default report based on that table.
**	The specified report was not found.
**
** System Status:
**	If no table by that name exists or if the user chooses not to create
**	a default report based on the table then control returns to the
**	catalog frame.  Otherwise the default report is created and the
**	layout form is entered.
**
** User Action:
**	If a table by that name exists the user may create a default report.
**	The user should specify existing reports when using the -r flag.
**	
*/

S_RF002C_Setting_up_default_re	"Setting up default report. . ."

/*
**	rfgetforms.qc
*/

E_RF002D_Cannot_locate_form	"Cannot locate form %0c"

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


/*
**	rfmaggs.c
*/

E_RF002E_rFm_aggs__Can_t_get_n	"rFm_aggs: Can't get numags."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF002F_rFm_aggs__Can_t_alloc	"rFm_aggs: Can't alloc mem."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF0031_rFm_aggs__Can_t_get_a	"rFm_aggs: Can't get agnames."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF0032_rFmaggs_c__Bad_index_	"rFmaggs.c: Bad index."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


/*
**	rfmdata.c
*/

E_RF0033_rFm_data__Can_t_deter	"rFm_data: Can't determine coercibility.\n
Please contact your Site Technical Contact for assistance."

E_RF0086_rFm_data__Failed_LIKE	"rFm_data: Failed in determining if the LIKE predicate could be used with field.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


/*
**	rfmfoot.c
*/

E_RF0034_rFm_foot__Bad_attribu	"rFm_foot: Bad attribute ordinal '%0d'."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


/*
**	rfmhead.c
*/

E_RF0035_rFm_head__Bad_attribu	"rFm_head: Bad attribute ordinal '%0d'."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF0036_rFm_head_Can_t_get_fo	"rFm_head:Can't get format string."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF0037_rFm_ptop_Can_t_get_fo	"rFm_ptop:Can't get format string."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

/*
**	rfmrhead.qc
*/

E_RF0038_rFm_rhead_Can_t_get_f	"rFm_rhead:Can't get format string."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


/*
**	rfmsec.c
*/

E_RF0039_rFafield__Can_t_get_f	"rFafield: Can't get format size."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


/*
**	rfqurset.c
*/

E_RF003A_rFqur_set__Bad_attrib	"rFqur_set: Bad attribute name '%0c'."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF003B_rFqur_set__Bad_where_	"rFqur_set: Bad where clause."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

/*
**	rfreset.c
*/

E_RF003C_rFreset__too_many_arg	"rFreset: too many arguments."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

/*
**	rfsave.qc
*/

/* This must use printf type formats !! */
S_RF003E_Saving_report___s__fo	"Saving report '%s' for table '%s'. . ."


/*
**	rfscan.c
*/

E_RF003F_rFscan__Bad_within_or	"rFscan: Bad within ordinal."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF0040_rFscan__Bad_end_of_co	"rFscan: Bad end of command %0d found."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


E_RF0041_rFscan__Bad_command_c	"rFscan: Bad command code %0d found."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/


/*
**	rfstruct.qc
*/

F_RF0025_sortup		"ascending"
F_RF0026_sortdown	"descending"
F_RF0027_ColumnOptions	"ColumnOptions"
F_RF0028_Report_Sort_Order	"Choosing a Sort Column"

E_RF0042_Duplicate_sort_sequen	"Duplicate sort sequences not allowed."

/*%
** Explanation:
**	On the sorting frame, no number may be assigned twice for a sort
**	sequence.
**
** System Status:
**	User may not leave frame until corrected.
**
** User Action:
**	Assign non-redundant sort sequences.
*/


E_RF0043_This_column_name_is_a	"This column name is a reserved word and cannot be a break."

/*%
** Explanation:
**	A column whose name is a keyword was selected as a break column, 
**	which is illegal.
**
** System Status:
**	This column may not be selected as a break column.  It is automatically
**	changed to "No" for the break column field on the form.
**
** User Action:
**	The user may create a view on the table or view which changes the name
**	of the desired break column to a non reserved word.
*/


/*
**	rfwrite.qc
*/

F_RF0029_RBF	"RBF"
F_RF002A_RBF___Report_Save_Fra	"RBF - Saving a Report"
F_RF002B_Query_language_	"Query language:"

E_RF0044_Table_names_only_cont	"Table names only contain [a-z,0-9,_], starting with [a-z]."

/*%
** Explanation:
**	On the save frame, the user specified an illegal table name.
**
** System Status:
**	The report may not be saved until a valid table name is entered.
**
** User Action:
**	Confirm and enter the correct name of the table or view on which the
**	report is based.
*/


E_RF0045_Query_language_must_b	"Query language must be sql or quel."

/*%
** Explanation:
**	The user entered an incorrect specification of the DML with which
**	the query statement of the report should be specified.
**
** System Status:
**	The report may not be saved until a valid DML specification is entered.
**	
** User Action:
**	Enter the correct name of the DML to use.
*/


E_RF0046_Table_does_not_exist_	"Table does not exist."

/*%
** Explanation:
**	On the save frame, the user specified a table which does not exist.
**
** System Status:
**	The report may not be saved until the name of an existing table 
**	is entered.
**
** User Action:
**	Confirm and enter the correct name of the table or view on which the
**	report is based.
*/

/*
**	rfattdflt.qc
*/

S_RF0047_Column_name_keyword	"WARNING: Column name '%0c' is also a Report Writer keyword."

/*
**	rfcat.qsc
*/

F_RF002C_AutoReport	"Preview"
F_RF002D_VisualQuery	"VisualQuery"
F_RF002E_JoinDef	"JoinDef"
F_RF002F_Table	"Table"
F_RF0030_View	"View"

/* menu item */

E_RF0048_Testing_can_not_begin	"Testing could not begin\n"

/*%
** Explanation:
**	This is the message to the syserr routine if an error occurred
**	from FEtstbegin. Something could be wrong with one of the input
**	or testing files: keystroke, dump or output.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Don't do it any more!
*/

S_RF0049_Auto_Sub	"Preview"

/* help file title */

E_RF004A_Bad_Table	
"Bad table name - '%0c'.\n
Table does not exist or cannot be accessed by you."

/*%
** Explanation:
**	Cannot find user specified table.
**
** System Status:
**	Return to main display loop
**
** User Action:
**	Check that you spelled the name right, try again with a different
**	name.
*/

S_RF004B_Running_auto	"Running default report for table '%0c'. . ."
S_RF004C_Writing_auto	"Writing default report for table '%0c' to file '%1c'. . ."

E_RF004D_No_Name	"No report or table name specified.\n
The -r flag requires a report name."

/*%
** Explanation:
**	No table or report name specified with the "-r" option.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Specify a report or table name if using the "-r" option.
*/

F_RF0031_RunRep       "RunReport"

/* menu item */

F_RF0032_Variables    "Variables"

/* menu item */

S_RF004E_Enter_view_name_to_u	"Enter view name and press <RETURN> : " 
S_RF004F_Enter_vq_name_to_u	"Enter VisualQuery name and press <RETURN> : "
S_RF0050_Enter_jd_name_to_u	"Enter JoinDef name and press <RETURN> : "

S_RF0051_Enter_file_name  "Enter file name and press <RETURN> : "

/*%
** Explanation:
**      RBF is prompting user for a filename to create for the report.
**
** System Status:
**      RBF is waiting for input.
**
** User Action:
**      Specify a filename.  If no filename is entered, user is returned to
**      the report catalog frame.
*/

S_RF0052_Printing_report      "Printing report '%0c'. . ."
/*%
** Explanation:
**      RBF is running the report to be sent to a printer.
**
** System Status:
**      Executing the report.
**
** User Action:
**      None.  This is a status message.
*/

F_RF0033_Report_Header	"Report Header"
F_RF0034_Report_Footer	"Report Footer"
F_RF0035_Page_Header	"Page Header"
F_RF0036_Page_Footer	"Page Footer"
F_RF0037_Sort_Break	"Sort/Break"

/*
**	rfscncol.c
*/

E_RF0053_rF_scn_col_No_Detail	"rFscn_col: No Detail Section."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/



F_RF0038_Report_Header	"Report-Header"
F_RF0039_Report_Footer	"Report-Footer"
F_RF003A_Page_Header	"Page-Header"
F_RF003B_Page_Footer	"Page-Footer"
F_RF003C_End_of_Report	"End-of-Report"
F_RF003D_Detail		"Detail"
F_RF003E_Break_Header	"-Break-Header"
F_RF003F_Break_Footer	"-Break-Footer"
F_RF0040_Edit_Layout    "Edit Report Layout"
F_RF0041_Full_Prompt    "FullPrompt"

/*	rfmdecl.c */

E_RF0054_invalid_dbtype
"An invalid datatype has been found.  Program terminates."

/*%
** Explanation:
**	This is a severe internal software error.  An invalid datatype
**	has been reached when trying to save the report to the database.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Try to save the report again.  If the same error is repeated, please 
**	call your Site Technical Contact.
*/

S_RF0055_prompt_string	
"%s with prompt 'Enter %s:'"


/*%
** Explanation:
**	The user must choose at least one field to break on.
**
** System Status:
**	The user will have to select at least one field or 
**  cancel the selection of the indented style.
**
** User Action:
**	Select at least one field.
*/

S_RF0056_min
"Min_"
/*%
** Explanation:
**	This is a string used to create a .DECLARE statement for
**	RBF runtime parameters.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

S_RF0057_max
"Max_"
/*%
** Explanation:
**	This is a string used to create a .DECLARE statement for
**	RBF runtime parameters.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

S_RF0058_min_prompt_string	
"%s with prompt 'Enter the minimum value for %s:'"

/*%
** Explanation:
**	This is a format string used to create a .DECLARE statement for
**	the minimum value of an RBF runtime parameter.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

S_RF0059_max_prompt_string	
"%s with prompt 'Enter the maximum value for %s:'"

/*%
** Explanation:
**	This is a format string used to create a .DECLARE statement for
**	the maximum value of an RBF runtime parameter.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/
/*
**  The following definitions are listpick data for source of a report,
**  Style of a report, or destination of a report.  The order of
**  the data is significant and corresponds to constants defined 
**  in rbfcons.h.  Do not change the order of one without changing the
**  order of the other.
**
**  These are fast messages because several of them are being passed
**  through to the listpick routines and we don't want to lose them
*/
F_RF0042_Source_Prompt
"RBF - Creating a Report\n \nChoose one of the selections below to create a report\ncontaining the specified information."
/*%
** Explanation:
**	This the prompt for the listpick form for Create Report
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/
F_RF0043_Source_Choices
"Duplicate;Create a report by copying an existing report\nTable;Create a report based on a Table\nJoinDef;Create a report based on a JoinDef\n"
/*%
** Explanation:
**	These are the choices for the listpick form for Create Report.
**	The order of the selections is defined in rbfcons.h
**	The wording of the choices should correspond to messages
**	in ervf.msg S_VF0145 thru S_VF014C, which pertain to forms.
**	These choices are also in slow messages S_RF0065--70.  This
**	set is for the usual create.  The slow messages are for the
**	special create from abf which requires its own form
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/
F_RF0044_Style_Prompt
"Choose a Report Style"
/*%
** Explanation:
**      This the prompt for the listpick form for Choosing a Style
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF005A_Style_Choices
"Tabular;Display in columns across the page\nWrap;Display as tabular but wrap to the next line if needed\nBlock;Display titles and data together\n"
/*%
** Explanation:
**      These are the choices for the listpick form for Choosing a Style.
**      The order of the selections is defined in rbfcons.h
**	F_RF005B_IndStyle_Choice and F_RF005C_MDStyle_Choice and
**	S_RF0063_Labels_choice may
**	be used in the same context if the style is not being selected
**	in the context of AutoReport/Preview and MD may be used only if the
**	Data Source is a MD data source.  These can be slow messages
**	because we are copying them into a buffer
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF005B_IndStyle_Choice
"Indented;Display indented at data breaks\n"
/*%
** Explanation:
**      These are the choices for the listpick form for Choosing a Style.
**      The order of the selections is defined in rbfcons.h
**      See Explanation for F_RF005A
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF005C_MDStyle_Choice
"Master/Detail;Display master data with detail data in tabular format\n"
/*%
** Explanation:
**      These are the choices for the listpick form for Choosing a Style.
**      The order of the selections is defined in rbfcons.h
**      See Explanation for F_RF005A
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0045_Dest_Prompt
"Select a Destination"
/*%
** Explanation:
**      This the prompt for the listpick form for Choosing a Destination
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0046_Dest_Choices
"Default;To the screen or file\nPrinter;To the printer\nFile;To a file\n"
/*%
** Explanation:
**      These are the choices for the listpick form for Choosing a Destination
**	Default means that if a .output is specified in the body of the
**	report, then the output is sent to that file.  If no .output statement
**	is specified, the output will be displayed on the screen.
**      The order of the selections is defined in rbfcons.h
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
/*
** The follow set of defines is used by create and autoreport in rfcat.qsc
*/
F_RF0047_Selecting_Jdef_names 
"Selecting JoinDef names. . ."
/*%
** Explanation:
**      Message used by IIUFopObjPik while waiting for list to come up
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0048_JoinDef_Name
"JoinDef Name"
/*%
** Explanation:
**      Title used by IIUFopObjPik List
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0049_JoinDefs
"JoinDefs"
/*%
** Explanation:
**      Object Type used by IIUFopObjPik List
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF005D_List_of_JD
"List Of JoinDefs"
/*%
** Explanation:
**      Help screen title used by IIUFopObjPik List
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF004A_Table_Name "Table Name"
/*%
** Explanation:
**      Prompt used in getname form
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF004B_Run_Table_Report "Run a Report Based on a Table"
/*%
** Explanation:
**      Title used in getname form
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF004C_Create_Table_Report "Create a Report Based on a Table"
/*%
** Explanation:
**      Title used in getname form
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF004D_Existing_Report_Name "Existing Report Name"
/*%
** Explanation:
**      Prompt used in getname form
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF004E_Create_by_Dup "Create a Report by Duplication"
/*%
** Explanation:
**      Title used in getname form
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF004F_Create_JD_Report "Create a Report based on a JoinDef"
/*%
** Explanation:
**      Title used in getname form
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0050_Existing_Reports "Existing Reports"
/*%
** Explanation:
**      List Pick Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0051_reports "reports"
/*%
** Explanation:
**      List Pick Error message
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0052_Selecting_Reports "Selecting Reports"
/*%
** Explanation:
**      List Pick message
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
F_RF0053_Report_Name "Report Name"
/*%
** Explanation:
**      List Pick Form Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF005E_Create_Report "Create Report"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF005F_Report_Styles "Report Styles"
/*%
** Explanation:
**      Help Title for listpick
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF0060_Report_Dest "Report Destinations"
/*%
** Explanation:
**      Help Title for listpick
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/
S_RF0061_Alt1_Source_Prompt
"RBF - Creating a Report\n \nRequested Report: "
/*%
** Explanation:
**	This the prompt for the listpick form for Create Report
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/
S_RF0062_Alt2_Source_Prompt
"\n \nThe report does not exist. You can create it by choosing\none of the selections below. If you do not want to create\na new report of this name then \"CANCEL\" this operation."
/*%
** Explanation:
**	This the prompt for the listpick form for Create Report
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/
S_RF0063_Labels_Choice "Labels;Display in mailing label format\n"
/*%
** Explanation:
**      These are the choices for the listpick form for Choosing a Style.
**      The order of the selections is defined in rbfcons.h
**      See Explanation for S_RF005A
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/


E_RF0064_Bad_decl_var
"Declared variable, '%0c', stored inconsistently in the database.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**      This is a severe internal software error
**
** System Status:
**      Program terminates.
**
** User Action:
**      Not applicable.
*/
F_RF0054_Create_Column	"RBF - Create a Column\n \nScroll through the columns and\nselect the one to be created."

F_RF0055_Create_Aggregate	"RBF - Create an Aggregate\n \nScroll through the columns and\nselect the one to be aggregated."

F_RF0056_Help_Column_Agg	"Creating a Column or Aggregate"

S_RF0065_Dup_Choice  	"Duplicate" 
S_RF0066_Dup_Expl	"Create a report by copying an existing report."
S_RF0067_Tbl_Choice	"Table"
S_RF0068_Tbl_Expl	"Create a report based on a Table."
S_RF0069_JD_Choice	"JoinDef"
S_RF006A_Decl_Vars	"Declared Variables"
S_RF006B_Get_Printer	"Sending a Report to a Printer"
S_RF006C_Send_File	"Sending a Report to a File"
S_RF006D_Full_Prompt	"Full Prompt"

/* This must use printf type formats !! */
S_RF006E_Saving_JDrep___s__fo  "Saving report '%s' for JoinDef '%s'. . ."

S_RF006F_sort_seq
"Enter the sort sequence (0 - 300) for this column.\nA sort value of 0 implies
the column will not be used in sorting the data.  Sorting is based on the sort
order.  The column with sequence 1 is sorted first, followed by the column with
sequence 2, and so forth."
/*%
** Explanation:
**	RBF is prompting for a sort sequence.  The data will be reported in
**	the order based on the sort sequence specified by the user.  Values
** 	range from 0 (do not use this column to sort) - 300.  If non-zero,
**	these columns will be used in the order of the sort list (i.e. the 
**	column with sequence 1 is sorted first, followed by the column with 
**	sequence 2 ...).
**
** System Status:
**	Program waits for user to press RETURN.
**
** User Action:
**	Press RETURN and enter the desired sort order.
*/

S_RF0070_JD_Expl	"Create a report based on a JoinDef."
/*%
** Explanation:
**	These are the choices for the listpick form for Create Report.
**	The order of the selections is defined in rbfcons.h
**	The wording of the choices should correspond to messages
**	in ervf.msg S_VF0145 thru S_VF014C, which pertain to forms.
**	These choices are also in slow messages S_RF0065--70.  This
**	set is for the usual create.  The slow messages are for the
**	special create from abf which requires its own form
**
** System Status:
**	Not applicable.
**
** User Action:
**      Not applicable.
*/

E_RF0071_Must_select_a_field  
"You must select at least one field to break on or cancel your initial \n
selection of the indented style."

/*%
** Explanation:
**	The user has chosen to create an Indented style report, and therefore
**	must select at least one column to be used as a break column.
**
** System Status:
**	The program will not allow the user to create an indented report without
**	selecting at least one break column.
**
** User Action:
**	Select a break column or cancel the command.
*/


E_RF0072_Need_errorlog
"Please enter the name of the file to be used as an error log.\nThe error log
contains the report statistics and any report errors encountered while running
the report.  The error log is used only when you choose not to wait for the
report to execute."

/*%
** Explanation:
**      The user is running the report in non-interactive mode and has not 
**	entered an error log name.
**
** System Status:
**      ReportWriter returns user to the error log name prompt.
**
** User Action:
**      The user may either enter in the error log name or run the report in
**	interactive mode (wait for the report to complete).
*/
F_RF0057_Break_Header	"Break Header"
F_RF0058_Break_Footer	"Break Footer"
F_RF0059_Create_Layout_Section	"Create"
F_RF005A_Delete_Layout_Section	"Delete"

F_RF005B_Create_Section_Title	"RBF - Creating a Layout Section\n \nScroll through the Report Sections\nand select the one to be created."

F_RF005C_Create_Section_Help	"Creating a Layout Section"

E_RF0073_Must_delete_hdr_ftr	"There is a break header or footer associated with this sort column.  Break headers and footers can only exist for sort columns.  Please delete the header or footer by using the Layout menu option."
/*%
** Explanation:
**	The user is trying to change the sort sequence of a column to 0 but
**	there exists a break header and/or footer associated with the sort col.
**
** System Status:
**	The sort sequence goes back to its previous value and the cursor is
**	resumed on that column.
**
** User Action:
**	The user must use the Layout menu option to delete the header and/or 
**	footer.
*/

E_RF0074_No_sort_cols	"A break header or footer must be associated with a sort column.  There are no sort columns available.  This may be because the report has no sort columns defined or because all of the sort columns already have an associated  header or footer.  Use the ColumnOptions menu option to adjust the sort order of the report."
/*%
** Explanation:
**	The user is trying to create a break header or column but there are no
**	sort columns.  Break headers/footers must be associated with a sort col.
**
** System Status:
**	Back up to the previous popup.
**
** User Action:
**	The user must use the ColumnOptions menu option to set up the sort 
**	cols for the report.
*/
E_RF001B_Higher_order_sort	"This field cannot be aggregated since it is use
d in a sort column of higher order."
/*%
** Explanation:
**	The user is trying to create an aggregate on a column in a 'column
**	break footer' area, but this column is used as a break in a higher
**	order.
**
** System Status:
**	Back up to the previous screen.
**
** User Action:
**	The user must use the ColumnOptions menu option to change the sort
**	order if s/he wishes to aggregate this field.
*/

F_RF005D_tabular	"tabular"
F_RF005E_indented	"indented"
F_RF005F_label		"labels"

F_RF0060_sel_sort_cols
"Select column to be used to create the break header/footer."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0061_Aggregate_Functions	"Aggregate Functions"

/*
** rfindopt.qsc:
*/

F_RF0062_Break_Columns	"Break Columns"

F_RF0063_sort_dir_title
"RBF - Column Options\n \nChoose the direction to sort the column."
/*%
** Explanation:
**	This the prompt for the listpick form for choosing the sort direction.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0064_asc_desc
"Ascending;Sort the column in ascending order\nDescending;Sort the column in descending order"
/*%
** Explanation:
**	These are the choices for the listpick form for choosing the sort 
**	direction.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0065_sel_crit_title
"RBF - Column Options\n \nChoose the selection criteria."
/*%
** Explanation:
**	This the prompt for the listpick form for choosing the selection
**	criteria.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0066_sel_crit_choices
"None;Do not prompt for any value\nValue;Prompt for a specific value for this column\nRange;Prompt for a range for this column"
/*%
** Explanation:
**	These are the choices for the listpick form for choosing the selection 
**	criteria.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0067_und_line_title
"RBF - Report Options\n \nSelect the underlining option."
/*%
** Explanation:
**	This the prompt for the listpick form for selecting the underlining
**	options.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0068_underline_choices
"None;Do not underline any text in this section\nAll;Underline all the text in this section\nLast;Only underline the last line in this section"
/*%
** Explanation:
**	These are the choices for the listpick form for choosing the selection 
**	criteria.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0069_ind_brk_title
"RBF - Break Columns\n \nIs this column a break column?"
/*%
** Explanation:
**	This the prompt for the listpick form for selecting break columns
**	when creating an indented report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF006A_yn_choices
"Yes;This column is a break column\nNo;This column is not a break column"
/*%
** Explanation:
**	These are the choices for the listpick form for choosing a break
**	column.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/



F_RF006B_wide_rep_choices
"Yes;Create the report beyond the -l flag.\nNo;Do not create the report."
/*%
** Explanation:
**      These are the choices for the listpick form for creating a wide report.
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

F_RF006C_Unique_Aggs "RBF - Unique Aggregates\n
\n Changing the sort sequence of this column to\n
0 will make it a non-break column.  There\n
are unique aggregates on this break column.\n
Please decide how to handle them."

F_RF006D_Unique_Aggs_Choices
"Delete;Delete unique aggregates on this column\nChange;Make unique aggregates non-unique"

F_RF006E_Help_Unique_Aggs "Handling Unique Aggregates"

S_RF0075_Sort_direction "Choosing the Sort Direction"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

S_RF0076_Selection_criteria "Choosing the Selection Criteria"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

S_RF0077_Underlining "Selecting the Underlining Options"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

S_RF0078_pg_len
"Enter the page length (0 - 99) for this report.\nA blank implies the report
will use the default value.  The default value is 20 when the report is sent to
the terminal and 61 when sent to a file or printer. A page length of 0 will
suppress all pagination."

/*%
** Explanation:
**	RBF is prompting for the report page length.  Valid values are
**	0 - 99 and blank.
**
** System Status:
**	Program waits for user to press RETURN.
**
** User Action:
**	Press RETURN and enter the desired page length.
*/


S_RF0079_underline_chr
"Enter the character to be used as the underlining character.  A blank implies
the report will use the default character.  The default character is dash '-'
when the report is sent to a terminal and underscore '_' when sent to a file
or printer."

/*%
** Explanation:
**      RBF is prompting for the character to be used as the underlining
**	character.
**
** System Status:
**	Program waits for user to press RETURN.
**
** User Action:
**	Press RETURN and enter the desired underlining character.
*/

S_RF007A_formfeed
"Enter 'yes' or 'no' to specify if the report should have formfeeds between
each page."

/*%
** Explanation:
**	RBF is prompting for "yes" or "no".  The user specifies if the report
**	should have formfeeds between each page.
**
** System Status:
**	Program waits for user to press RETURN.
**
** User Action:
**	Press RETURN and enter either 'yes' or 'no'.
*/

S_RF007B_pg_hdr
"Enter 'yes' or 'no' to specify if the page header should be printed on the
first page of the report."

/*%
** Explanation:
**      RBF is prompting for "yes" or "no".  The user specifies if the page
**	header should be printd on the first page of the report.
**
** System Status:
**	Program waits for user to press RETURN.
**
** User Action:
**	Press RETURN and enter either 'yes' or 'no'.
*/

S_RF007C_nullstring
"Enter how a null value should be represented in the report.  The character
string may either be 30 characters or less or a blank."

/*%
** Explanation:
**	RBF is prompting for a character string to be used in representing 
**	null values in the report.
**
** System Status:
**	Program waits for user to press RETURN.
**
** User Action:
**	Press RETURN and enter the desired character string.
*/

S_RF007D_indented_break  "Selecting Break Columns for an Indented Report"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

E_RF007E_input_not_found  
"Please enter a valid input."
/*%
** Explanation:
**      Generic error for user entering an invalid option.  Use the Help key
**	for specific information of the various options.
**
** System Status:
**      Returns back to the field where the invalid input was entered.
**	Allows user to re-enter input.
**
** User Action:
**      Enter valid input according to the field.
*/

E_RF007F_No_more_sections
"There are no more sections to delete."
/*%
** Explanation:
**      The Layout section catalog is empty and the user has selected
**	the Delete menu option.
**
** System Status:
**      Displays an error message and then resumes.
**
** User Action:
**      Don't try to delete non-existent section.
*/
E_RF0080_Cant_Del_Lab_RepFtr
"Not allowed to delete the Report Footer within a Labels Style Report."
/*%
** Explanation:
**      The user has selected to delete the Report Footer within a 
**	Labels Style Report. This is not allowed.
**
** System Status:
**      Displays an error message and then resumes.
**
** User Action:
**      Don't try to delete the Report Footer within a Labels Style Report.
*/

E_RF0081_No_columns_avail
"There are no available columns.  The report already contains all the columns."
/*%
** Explanation:
**      The user has selected Create Columns but there are no more
**	available columns.  
**
** System Status:
**      Displays an error message and then resume.
**
** User Action:
**      Don't try to create a new column.
*/

E_RF0082_bad_errorlog
"Enter the name of the file to be used as an error log.\nPlease check
your input for a valid file name."

/*%
** Explanation:
**      The user is prompted for a filename to be used as the error log.  An
**	invalid filename was encountered.
**
** System Status:
**      Displays an error message and returns to prompt for the error log name.
**
** User Action:
**      Enter a valid file name or choose to run the report interactively.
*/

E_RF0083_bad_filename
"Enter the name of the file to contain the report output.\nPlease check
your input for a valid file name."

/*%
** Explanation:
**      The user is prompted for a filename where to send the report output.
**	An invalid filename was encountered.
**
** System Status:
**      Displays an error message and returns to prompt for the filename.
**
** User Action:
**      Enter a valid file name or choose not to send the report output to a
**	file.
*/

E_RF0084_cannot_open_file
"Unable to write the file in the specified directory.\nPlease enter another
valid file name."

/*%
** Explanation:
**      The user entered a filename in a directory that the user does not have
**	write privileges.
**
** System Status:
**      Displays an error message and returns to prompt for another filename.
**
** User Action:
**      Enter a valid file name or choose to cancel this operation.
*/


S_RF0085_wide_report  "Creating a Wide Report"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

S_RF0087_wide_rep_title
"RBF - Creating a Wide Report\n \nThe report style that you have selected requires a width\nof %d. This goes beyond the specified maximum width of %d.\nDo you still want to create the wide report?"
/*%
** Explanation:
**	This the prompt for the listpick form for creating a wide report
**	beyond En_lmax, specified by the -l flag.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

E_RF0088_too_many
"Multiple table/report names are specified.\n
Correct usage of RBF command is:\n
	RBF dbname rpt_object [-uuser] [-r] [-s] [-e] \n
	[-m[tabular|wrap|block|indented|labels|column|default]] \n
	[-l{mxline}] [-GgroupID] [-P]\n
Please correct errors and rerun.  Processing stops."


/*%
** Explanation:
**      The user entered two or more table/report names, either on the command
**      line or through a 3GL/4GL application.
**
** System Status:
**      Displays the error message and terminates.
**
** User Action:
**      Correct the error by specifying exactly one table/report name.
*/
F_RF006F_Cum_Agg_Prompt	"RBF - Cumulative Aggregation\n \nSpecify if the aggregate should be cumulative."

F_RF0070_Cum_Agg_Choices	"No;Do not make this aggregate cumulative\nYes;Make this aggregate cumulative"

F_RF0071_Cum_Agg_Help	"Specifying Cumulative Aggregation"

F_RF0072_Break_Options  "Break Options"

E_RF0089_Can_t_delete_section
"This section: '%0c' is needed to support the suppression of repetition of break column values in the detail section.  To change this go to the Edit Field BreakOptions menu for every field affected."
/*%
** Explanation:
**	The user is trying to delete a section that is needed for the
**	internal .tformat generated when the user chooses to print the
**	break values on breaks and/or pages rather than always.
**
** System Status:
**	Gives an error message.  The section will not be deleted.
**
** User Action:
**	For every field effected, cursor to the field then select Edit and
**	BreakOptions.  Change the options for printing break values so
**	that the header or footer section you are trying to delete will
**	not be needed.
*/
E_RF008A_Can_t_delete_section
"This section: '%0c' is needed to support a page feed for this break column.  To change this go to the Edit Field BreakOptions menu for this field."

/*%
** Explanation:
**	The user is trying to delete a section that is needed for the
**	internal .newpage generated when the user chooses to skip to
**	a new page on breaks.
**
** System Status:
**	Gives an error message.  The section will not be deleted.
**
** User Action:
**	For the field effected, cursor to the field then select Edit and
**	BreakOptions.  Change the options for new page on break to 'no'.
*/

S_RF008B_PrintAlways 
"Enter 'yes' or 'no' to specify if the value of the break column should\n
always be printed."
/*%
** Explanation:
**	RBF prompting the user to specify if the value for the break 
**	column should always be printed.
**
** System Status:
**	Program waits for user to hit RETURN.
**
** User Action:
**	Hit RETURN and enter either 'yes' or 'no'.
*/

S_RF008C_PrintOnBreak
"Enter 'yes' or 'no' to specify if the value of the break column should\n
be printed whenever there is a break."
/*%
** Explanation:
**	RBF prompting the user to specify if the value of the break 
**	column should be printed when a break occurs.
**
** System Status:
**	Program waits for user to hit RETURN.
**
** User Action:
**	Hit RETURN and enter either 'yes' or 'no'.
*/

S_RF008D_PrintOnPageBreak
"Enter 'yes' or 'no' to specify if the value of the break column should\n
be printed whenever there is a page break."
/*%
** Explanation:
**	RBF prompting the user to specify if the value of the break 
**	column should be printed when a page break occurs.
**
** System Status:
**	Program waits for user to hit RETURN.
**
** User Action:
**	Hit RETURN and enter either 'yes' or 'no'.
*/

S_RF008E_SetNewPage
"Enter 'yes' or 'no' to specify if there should be a new page whenever the\n
value of the break column changes."
/*%
** Explanation:
**	RBF prompting the user to specify if a new page is desired 
**	whenever the value of break column changes.
**
** System Status:
**	Program waits for user to hit RETURN.
**
** User Action:
**	Hit RETURN and enter either 'yes' or 'no'.
*/

E_RF008F_DoOldAgg__Can_t_get_f	"DoOldAgg: Can't get format size.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF0090_Bad_JoinDef	
"Bad JoinDef name - '%0c'.\n
JoinDef does not exist or cannot be accessed by you."

/*%
** Explanation:
**	Cannot find user specified JoinDef. 
**
** System Status:
**	Return to main display loop
**
** User Action:
**	Check that you spelled the name right, try again with a different
**	name.
*/


E_RF0091_Bad_Report	
"Bad report name - '%0c'.\n
Report does not exist or cannot be accessed by you."

/*%
** Explanation:
**	Cannot find user specified report. 
**
** System Status:
**	Return to main display loop
**
** User Action:
**	Check that you spelled the name right, try again with a different
**	name.
*/

E_RF0092_no_range_var	
"Cannot find the correlation name, '%0c' in the list of tables.\n
Report is not saved."

/*%
** Explanation:
**	RBF is saving a report based on a joindef.  The range variable is not
**      in the list of tables.
**
** System Status:
**	Returns back to the last frame.
**
** User Action:
**	Try to archive the report out, and check the query for correct syntax
**	using correlation names.
*/

E_RF0093_no_jd_comment	
"Cannot find the joindef comment block.\n
The report has been modified in a manner making it impossible to edit in RBF.
To edit this report, you must archive the report and edit the command
specification file."

/*%
** Explanation:
**	RBF cannot edit the joindef report.  It cannot find the comment
**	block describing the joindef relationship.  The report has been
**	modified making it impossible for RBF to edit the report.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	To edit the report, archive the report out and edit the report
**	specification file.
*/

E_RF0094_no_jd_remainder	
"Cannot find the joindef comment block.\n
The report has been modified in a manner making it impossible to edit in RBF.
To edit this report, you must archive the report and edit the command
specification file."

/*%
** Explanation:
**	RBF cannot edit the joindef report.  It cannot find the 'remainder'
**	section describing the joindef relationship.  The report has been
**	modified making it impossible for RBF to edit the report.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	To edit the report, archive the report out and edit the report
**	specification file.
*/


E_RF0095_bad_range_var	
"Cannot find the correlation name in the query.\n
Archive the report and check the query for correct syntax specifying the
correlation name."


/*%
** Explanation:
**	RBF cannot edit the joindef report.  It cannot find the correlation
**	name for a table in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Check the query for correct syntax specifying
**	the correlation name.
*/


E_RF0096_bad_jd_comment	
"The joindef comment block has been modified making it impossible to edit the
report.  There are more tables in the query than are specified in the joindef
definition."

/*%
** Explanation:
**	RBF cannot edit the joindef report.  The comment block describing the 
**	joindef relationship does not match the query specified in the
**	report.  
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out and check the query against the joindef comment
**	block.
*/

E_RF0097_bad_jdcomment2	
"The joindef comment block has been modified making it impossible to edit the
report.  There are less tables in the query than are specified in the joindef
definition."

/*%
** Explanation:
**	RBF cannot edit the joindef report.  The comment block describing the 
**	joindef relationship does not match the query specified in the
**	report.  
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out and check the query against the joindef comment
**	block.
*/

E_RF0098_bad_from_clause
"The FROM clause in the query has a syntax error.\n
Archive the report and check the query for correct syntax." 


/*%
** Explanation:
**	RBF cannot edit the joindef report.  It cannot parse the FROM clause
**	in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Check the query for correct syntax. 
*/

F_RF0073_cr_layout_sec
"Create a report layout section that is not already listed."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0074_del_layout_sec
"Delete the currently highlighted report layout."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0075_indented_cols
"Create an Indented report with the specified break headers."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0076_return_layout
"Return to the main edit layout frame."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0077_brk_options
"Return to the main edit layout frame with the specified break options."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0078_create
"Create a new report based on an existing RBF report, on a table or on a JoinDef."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0079_delete
"Delete the currently highlighted report."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF007A_edit
"Edit the currently highlighted report.  You may edit RBF reports only."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF007B_rename
"Rename the currently highlighted report to another name."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF007C_preview
"Execute a default report based on a table."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF007D_utilities
"View a submenu of utility functions to perform on the report."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF007E_archive
"Write out the ReportWriter commands for the report out to a text file."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF007F_go
"Execute the currently highlighted report."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0080_quit
"Exit RBF."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0081_enter_object
"Create a report based on the specified object."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0082_decl_vars
"Execute the report with the specified values for the variables."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0083_full_prompt
"Display the whole prompt for the currently highlighted variable."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0084_ok_full_prompt
"Return to the Declared Variables frame."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0085_enter_printer
"Send the report to the specified printer."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0086_enter_filename
"Send the report to the specified file."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0087_archive_rep
"Write out the ReportWriter commands to the specified file."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0088_sel_agg
"Select the type of aggregate to be created."

/*%
** Explanation:
**	Help file explanation.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

F_RF0089_union_comment1
"*  The union clause is commented out because a selection"

F_RF008A_union_comment2
"*  criteria on a detail table is specified."


/*%
** Explanation:
**	Comments when archiving a report out.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

E_RF0099_union_modified
"This report cannot be edited by RBF.\n
The UNION SELECT comment block has been modified in a way that makes it 
impossible for RBF to edit.  To make further changes to this report, archive 
the report out, and edit the report definition."


/*%
** Explanation:
**	RBF cannot edit the report.  It cannot parse the comments in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Edit the report by modifying the report
**	definition.  Save the report in the database by using SREPORT.
**	This report can no longer be edited in RBF.
*/

E_RF009A_jd_modified
"This report cannot be edited by RBF.\n
The JoinDef definition comment block has been modified in a way that makes 
it impossible for RBF to edit.  To make further changes to this report, archive
the report out, and edit the report definition."


/*%
** Explanation:
**	RBF cannot edit the report.  It cannot parse the comments in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Edit the report by modifying the report
**	definition.  Save the report in the database by using SREPORT.
**	This report can no longer be edited in RBF.
*/

E_RF009B_union_asterisk
"This report cannot be edited by RBF.\n
The UNION SELECT clause has been modified in a way that makes it impossible
for RBF to edit.  RBF was expecting an asterisk '*'.\n
To make further changes to this report, archive the report out, and edit the 
report definition."


/*%
** Explanation:
**	RBF cannot edit the report.  It cannot parse the comments in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Edit the report by modifying the report
**	definition.  Save the report in the database by using SREPORT.
**	This report can no longer be edited in RBF.
*/


E_RF009C_union_paren
"This report cannot be edited by RBF.\n
The UNION SELECT clause has been modified in a way that makes it impossible
for RBF to edit.  RBF was expecting a left parenthesis ')'.\n
To make further changes to this report, archive the report out, and edit the 
report definition."


/*%
** Explanation:
**	RBF cannot edit the report.  It cannot parse the comments in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Edit the report by modifying the report
**	definition.  Save the report in the database by using SREPORT.
**	This report can no longer be edited in RBF.
*/

E_RF009D_union_where
"This report cannot be edited by RBF.\n
The UNION SELECT clause has been modified in a way that makes it impossible
for RBF to edit.  The UNION SELECT clause does not correctly include the WHERE 
clause.\n  
To make further changes to this report, archive the report out, and edit the 
report definition."


/*%
** Explanation:
**	RBF cannot edit the report.  It cannot parse the comments in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Edit the report by modifying the report
**	definition.  Save the report in the database by using SREPORT.
**	This report can no longer be edited in RBF.
*/

E_RF009E_union_and
"This report cannot be edited by RBF.\n
The UNION SELECT clause has been modified in a way that makes it impossible
for RBF to edit.  RBF was expecting the word 'and'.\n
To make further changes to this report, archive the report out, and edit the 
report definition."


/*%
** Explanation:
**	RBF cannot edit the report.  It cannot parse the comments in the .QUERY.
**
** System Status:
**	Returns to the main Catalog Frame.
**
** User Action:
**	Archive the report out.  Edit the report by modifying the report
**	definition.  Save the report in the database by using SREPORT.
**	This report can no longer be edited in RBF.
*/

F_RF008B_pr_yes_concurrent
"Enter the printer name, the number of copies desired and whether to wait for
completion."

F_RF008C_pr_no_concurrent
"Enter the printer name and the number of copies desired."

/*%
** Explanation:
**	F_RF008B & F_RF008C are the instructions when sending a report to the 
**	printer.  F_RF008C has been modified for platforms that do not have
**	batch reports and cannot create subprocesses
**
** System Status:
**	Not applicable.
**
** User Action:
**	None.
*/

F_RF008D_fi_yes_concurrent
"Enter the file name to contain the report output and whether to wait for
completion."

F_RF008E_fi_no_concurrent
"Enter the file name to contain the report output."

/*%
** Explanation:
**	F_RF008D & F_RF008E are the instructions when sending a report to a 
**	file.  F_RF008E has been modified for platforms that do not have
**	batch reports and cannot create subprocesses
**
** System Status:
**	Not applicable.
**
** User Action:
**	None.
*/

F_RF008F_ChooseCols	"ChooseColumns"

/*%
** Explanation:
**	Menuitem name to allow user to select the columns to base
**	default report on.
**
** System Status:
**	Not applicable.
**
** User Action:
**	None.
*/

F_RF0090_ChColExpl	"Choose the columns to be used in report being created."

/*%
** Explanation:
**	Help explanation for the ChooseColumns menuitem.
**
** System Status:
**	Not applicable.
**
** User Action:
**	None.
*/

E_RF009F_batch_report
"Please enter either 'yes' or 'no' to indicate if you want to wait for your
report to complete before continuing to work within RBF."

/*%
** Explanation:
**	The user did not enter anything for the "wait for report complete"
**	prompt when executing a report.
**
** System Status:
**	The cursor is returned to the prompt waiting for user input.
**
** User Action:
**	Enter either "yes" or "no".
*/

E_RF00A0_enter_printer
"Please enter a valid printer name."


/*%
** Explanation:
**	The user did not enter a printer (queue) name to send the report.
**
** System Status:
**	The cursor is returned to the prompt waiting for user input.
**
** User Action:
**	Enter a valid printer (queue) name.
*/

E_RF00A1_enter_copy
"Please enter the number of report copies to be printed.  You may select between
1 - 999 copies."

/*%
** Explanation:
**	The user did not enter the number of report copies to be printed.
**
** System Status:
**	The cursor is returned to the prompt waiting for user input.
**
** User Action:
**	Enter a number from 1 - 999.
*/

E_RF00A2_Invalid_table_name
"Table name is badly formed or contains invalid characters.\n
Ensure that owner and/or tablename components which are delimited\n
identifiers are properly enclosed in double quotes."

/*%
** Explanation:
**	On the save frame, the user specified an illegal table name
**  and the catalog level is at least 6.5.
**
** System Status:
**	The report may not be saved until a valid table name is entered.
**
** User Action:
**	Confirm and enter the correct name of the table or view on which the
**	report is based.
*/

E_RF00A3_No_Tbl_Name
"No table name has been entered.\n
You must enter a valid table name before the \"ChooseColumns\" menuitem
can be selected."

/*%
** Explanation:
**	The user selected the "ChooseColumns" menuitem but did not
**	enter a table name.  A table name must be entered before
**	"ChooseColumns" can be selected.
**
** System Status:
**	RBF will ignore the request and continue.
**
** User Action:
**	Enter a valid table name and then select "ChooseColumns".
*/

E_RF00A4_No_Del_Cols
"There are no columns to add.\n
You have selected the \"Add\" column menuitem.  However, there are no
deleted columns to add."

/*%
** Explanation:
**	The user selected the "Add" menuitem but there are presently
**	no deleted columns that can be used for this function.
**
** System Status:
**	RBF will ignore the request and continue.
**
** User Action:
**	The user can continue using other menuitems.
*/

F_RF0091_Add	"Add"

/*%
** Explanation:
**	F_RF0091_Add is the menuitem in the Choose Columns frame that allows
**	users to add back deleted columns.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0092_RemoveAll	"RemoveAll"

/*%
** Explanation:
**	F_RF0092_RemoveAll is the menuitem in the Choose Columns frame that
**	allows users to remove all remaining columns that are displayed
**	in the table field.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0093_Above	"Above"

/*%
** Explanation:
**	F_RF0093_Above is the menuitem in the Choose Columns frame that allows
**	the user to move the selected row to be above the current row in
**	the table field.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0094_Below	"Below"

/*%
** Explanation:
**	F_RF0094_Below is the menuitem in the Choose Columns frame that allows
**	the user to move selected row to be below the current row in
**	the table field.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0095_ExplAdd	"Add back a deleted column."

/*%
** Explanation:
**	Explanation for the Add menuitem in the Choose Columns frame.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0096_ExplRemoveAll	"Remove all columns currently displayed."

/*%
** Explanation:
**	Explanation for the RemoveAll menuitem in the Choose Columns frame.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0097_ExplAbove	"Move selected row to be above the current row."

/*%
** Explanation:
**	Explanation for the Above menuitem for the Move submenu
**	in the Choose Columns frame.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0098_ExplBelow	"Move selected row to be below the current row."

/*%
** Explanation:
**	Explanation for the Below menuitem for the Move submenu
**	in the Choose Columns frame.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF0099_ExplOK		"Have completed selecting columns for report."

/*%
** Explanation:
**	Explanation for the OK menuitem in the Choose Columns frame.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF009A_ExplDel	"Delete the current row."

/*%
** Explanation:
**	Explanation for the Delete menuitem in the Choose Columns frame.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

F_RF009B_ExplMove	"Select the current row so it can be moved."

/*%
** Explanation:
**	Explanation for the Move menuitem in the Choose Columns frame.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

S_RF00A5_ChooseColumns	"Choose Report Columns"

/*%
** Explanation:
**	Title for Choose Column help file.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

S_RF00A6_ChCol_Move	"Choose Report Columns Move Submenu"

/*%
** Explanation:
**	Title for Choose Column Move Submenu help file.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

S_RF00A7_ChCol_Add	"Choose Report Columns Add Command"

/*%
** Explanation:
**	Title for Choose Column Add Command help file.
**
** System Status:
**	n/a
**
** User Action:
**	n/a
*/

E_RF00A8_NoColsLeft
"Must have at least one column in the report.\n
RBF needs at least one column to create a default report.  However,
you have deleted all the columns that were shown.  Please use the
'Add' menuitem to add back one of the deleted columns before selecting
the 'OK' menuitem.  If you wish to discard your changes, you may use
the 'Cancel' menuitem."

/*%
** Explanation:
**	User removed all the columns and then selected OK.  This
**	is not allowed since RBF needs at least one column before
**	it can create a default report.
**
** System Status:
**	RBF will not exit out of the Choose Columns frame.
**
** User Action:
**	Either use the Cancel menuitem to exit or used the Add
**	menuitem to put back one of the columns and then select OK.
*/

E_RF00A9_NoJDName
"A JoinDef name must be entered.\n
A JoinDef name needs to be entered here in order to create a new
report.  You may also edit/create a JoinDef by entering in the name
and selecting 'Edit'.  If you do not wish to create a report based
upon a JoinDef, use the 'Cancel' menu item."
/*%
** Explanation:
**      The user has entered 'OK', thus requesting creation of a report
**      without entering the name of a joindef upon which
**      to base the new report.
**
** System Status:
**      Do nothing.
**
**  User Action:
**      Enter a JoinDef name and select either 'Edit' (to edit/create)
**      or 'OK' (to create a report based on the joindef.  The user
**      can select 'Cancel' to cancel the operation.
*/

E_RF00AA_NoTableName
"A table name must be entered.\n
A table name needs to be entered here in order to create a default
report.  If you do not wish to create a report based upon a table, use
the 'Cancel' menu item."
/*%
** Explanation:
**      The user has entered 'OK', thus requesting creation of a report
**      without entering the name of a table upon which
**      to base the new report.
**
** System Status:
**      Do nothing.
**
**  User Action:
**      Enter a table name and select 'OK' to create a report based on 
**	the table.  The user can select 'Cancel' to cancel the operation.
*/

E_RF00AB_NoRWName
"A report name must be entered.\n
A report name needs to be entered here in order to create a new
report.  If you do not wish to create a form based upon an existing 
report, use the 'Cancel' menu item."
/*%
** Explanation:
**      The user has entered 'OK', thus requesting creation of a report
**      without entering the name of a report upon which
**      to base the new report.
**
** System Status:
**      Do nothing.
**
**  User Action:
**      Enter a report name and select 'OK' to create a report based on 
**	the report.  The user can select 'Cancel' to cancel the operation.
*/

E_RF00AC_ExecutionError
"Error when executing %0c.\n
An error status of `%1x' was returned when the
program %0c was executed. Please check your configuration
to insure that the program is available."

/*%
** Explanation:
**      The program could not be executed, or returned an
**      error status when executed.
**
** System Status:
**      Operation resumes.
**
** User Action:
**      Look into configuration and insure that the
**      executable is available.
*/

E_RF00AD_XNS_QUEL
"The report contains table and/or identifier name constructs which\n
are not supported by the QUEL query langage.  Please specify the\n
query language as SQL and re-execute the Save operation."

/*%
** Explanation:
**      The report contains delimited identifiers and/or owner.tablename.
**	This cannot not always be determined prior to calling rFsave().
**
** System Status:
**      Operation resumes.
**
** User Action:
**      Bring up the Save screen and change the query language to SQL.
*/

F_RF009C_EditExpl	"Edit/create a JoinDef on which to create the report."

/*%
** Explanation:
**	Help explanation for the Edit menuitem.
**
** System Status:
**	Not applicable.
**
** User Action:
**	None.
*/

S_RF00AE_pg_hdr_help
"Printing the Page Header"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/

F_RF009D_pg_hdr_title
"RBF - Report Options\n
\nDo you want the Page Header to be printed on the first
\npage of the report?"
/*%
** Explanation:
**	This the prompt for the listpick form for selecting if the page header
**	should be printed on the first page of the report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/


F_RF009E_pg_hdr_choice
"Yes;Print the page header on the first report page\nNo;Do not print the page header on the first report page"
/*%
** Explanation:
**	These are the choices for the listpick form for selecting if the page
**	header should be printed on the first page of the report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

S_RF00AF_ff_help
"Inserting Formfeeds"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/


F_RF009F_ff_title
"RBF - Report Options\n
\nDo you want to insert formfeeds in your
\nreport?"
/*%
** Explanation:
**	This the prompt for the listpick form for selecting if the page header
**	should be printed on the first page of the report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/


F_RF00A0_ff_choice
"Yes;Insert formfeeds in the report\nNo;Do not insert formfeeds in the report"
/*%
** Explanation:
**	These are the choices for the listpick form for selecting if formfeeds 
**	should be inserted in the report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

S_RF00B0_format
"Enter the format to print the specified page component in the report.
For more information on display formats, select the 'Help' menuitem." 

/*%
** Explanation:
**      RBF is prompting for a format for the page component - date, time
**	or page.
**
** System Status:
**      Program waits for user to press RETURN.
**
** User Action:
**      Press RETURN and enter a valid format.
*/

E_RF00B1_date_format
"Please enter a valid date format.  The format cannot be a multi-row or
numeric format."

/*%
** Explanation:
**      RBF is prompting for a format for the date component. 
**
** System Status:
**      Program waits for user to press RETURN.
**
** User Action:
**      Press RETURN and enter a valid date format.  This must be either a
**	date template or a single line character format (e.g., c25).
*/

E_RF00B2_time_format
"Please enter a valid time format.  The format cannot be a multi-row or
numeric format."

/*%
** Explanation:
**      RBF is prompting for a format for the time component. 
**
** System Status:
**      Program waits for user to press RETURN.
**
** User Action:
**      Press RETURN and enter a valid time format.  This must be either a
**	date template or a single line character format (e.g., c25).
*/

E_RF00B3_page_format
"Please enter a valid page format.  The format must be a numeric format or
a numeric template."

/*%
** Explanation:
**      RBF is prompting for a format for the page component. 
**
** System Status:
**      Program waits for user to press RETURN.
**
** User Action:
**      Press RETURN and enter a valid page format.  This must be either a
**	numeric format (e,g,. f4), or a numeric template (e.g., '\-zzzn\-').
*/

E_RF00B4_roptions_cant_get_fmt
"rFroptions: Can't get format size.\n
Please contact your Site Technical Contact for assistance."

/*%
** Explanation:
**	This is a severe internal software error
**
** System Status:
**	Program terminates.
**
** User Action:
**	Not applicable.
*/

E_RF00B5_NoSuppCols
"The table '%0c' is composed entirely of columns of unsupported\n
datatype(s).  A report cannot be created on such a table.\n"

/*%
** Explanation:
**      The table consists of columns which are all of unsupported
**	datatype(s), e.g., BLOBs.
**
** System Status:
**      Return to table specification pop-up and continue.
**
** User Action:
**      Choose another table.
*/

S_RF00B6_first_ff_help
"Printing First Formfeeds"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/


F_RF00A1_first_ff_title
"RBF - Report Options\n
\nDo you want to print a formfeed before the first report page?"
/*%
** Explanation:
**	This the prompt for the listpick form for selecting if a formfeed
**	should be printed before the first page of the report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/


F_RF00A2_first_ff_choice
"Yes;Print formfeed before first report page\nNo;Do not print formfeed before first report page"
/*%
** Explanation:
**	These are the choices for the listpick form for selecting if formfeeds 
**	should be inserted in the report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

S_RF00B7_include_help
"Including Report Components"
/*%
** Explanation:
**      Help Title
**
** System Status:
**      Not applicable.
**
** User Action:
**      Not applicable.
*/


F_RF00A3_include_title
"RBF - Report Options\n
\nDo you want to include the associated report
\ncomponent in the report?"
/*%
** Explanation:
**	This the prompt for the listpick form for including a report component.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/


F_RF00A4_include_choice
"Yes;Include report component in report\nNo;Do not include report component in report"
/*%
** Explanation:
**	These are the choices for the listpick form for selecting if report 
**	component should be included in the report.
**
** System Status:
**	Not applicable.
**
** User Action:
**	Not applicable.
*/

