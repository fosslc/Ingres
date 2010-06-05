/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erqr.msg -	Message file for class QR
**
** Description:
**	This file defines the messages for class QR.
**
** History:
**	11-aug-1987 (daver)
**		Created.
**	07-jun-88 (bruceb)
**		Added a slew of new messages for gateway sql.
**	22-jun-88 (bruceb)
**		Changed F_QR000C_Table_not_a_view message to
**		indicate possibility that table IS a view, but
**		that definition is not available.
**	21-jul-88 (bruceb)
**		Added new messages for permission information when
**		running SQL.
**	17-aug-88 (bruceb)
**		Modified E_QR0005 to reference the new name of qrhlpsql.hlp.
**	06-sep-88 (bruceb)
**		Capitalized some of the 'title' text.
**	09-sep-88 (bruceb)
**		Added new messages for logical help.
**	12-sep-88 (bruceb)
**		New messages for new info in table help and view help and for
**		multiple locations.  (F_QR...71-77 are new, 14 is modified.)
**	16-sep-88 (bruceb)
**		Modified the header text for views, integrities and permits
**		so as to be in non-comment format.
**	23-sep-88 (bruceb)
**		New syntax for help table, and add messages for syntax of
**		help index and logical help.  New messages for help on
**		links and procedures.
**	07-apr-89 (teresal)
**		New messages for additional STAR info in help register.
**	29-sep-89 (teresal)
**		Added new messages for Runtime INGRES.
**	13-nov-89 (teresal)
**		Modified display for HELP PERMIT to show SQL 'grant' statement 
**		rather than the obsolete 'create permit' statement. (Bug 6927)
**	13-nov-89 (teresal)
**		Added new messages for HELP SECURITY_ALARM.
**	03-jan-89 (teresal)
**		Add new messages for HELP SYNONYM, HELP COMMENT TABLE and HELP
**		COMMENT COLUMN.
**	09-apr-90 (teresal)
**		Fix for bug 9423 - added invalid object name error message.
**	17-aug-90 (kathryn) 
**		Shortened Title of Message E_QR0008 to just
**		to E_QR0008_Type_help_help
**	05-sep-91 (pholman)
**		Added F_QR010A, message for Secure INGRES SQL monitor, which
**		will indicate the access label that the object was created at.
**	20-mar-91 (kathryn)
**		Added compression (data,key) messages for "HELP [TABLE,INDEX]" 
**		output.
**	22-mar-1991 (kathryn)
**		Added Messages for HELP PERMIT ON PROCEDURE and HELP PERMIT ON
**		EVENT.
**	10-jul-1991 (kathryn) 
**		Changed error messages to reflect new 6.4 FCS syntax of DBEVENT 
**		rather than EVENT. Added Error Message E_QR0046_HELP_perm_on.
**	18-jun-1992 (rogerl) 
**		6.5 additions:
**		1) change syntax error messages to 6.5 syntax.
**		(affects: F_QR0046, E_QR0008, user should use HELP SQL to get
**		correct 'combo word' syntax) 2) HELP object_type object_name
**		3) add strings 'user object' 'system object' 4) add stuff for
**		HELP_PERM_{VIEW,IDX} change text
**		E_QR000A_Syntax_err_correct_is "\nSyntax error on %s:\n" and on
**		E_QR000D_Syntax_error_on_EOF	"\nSyntax error on 'EOF'\n"
**		5) changed all 'No xxx were found that matched 'x' (quoted x)
**		6) F_QR011A_user_synonym 7) add F_QR011B_user_synonym
**		8) F_QR011C_synonym 9) F_QR011D_check_permits 10) add
**		object name argument to E_QR0025_err_ret_help_info
**		11) changed 'or is not owned by you' to 'or is not accessible
**		by yo
**	18-jun-1992 (rogerl) 
**		more 6.5 additions:
**	02-nov-1992 (rogerl) 
**		Add tokens corresponding to user-defined and unbounded data
**		for those who don't want blob or udt data
**	12-nov-1992 (rogerl) 
**		Add comment about size restriction for messages
**		F_QR011E_Unbounded_data, F_QR011F_User_defined_data
**		since these must be less than 32 characters.  Change
**		F_QR0090_Local_Table from 'Local Table' to 'Local Object'
**		since it's also used for indices, procedures etc.
**	11-dec-1992 (rogerl) 
**		Add F_QR012A_procedure, generalize F_QR0090_Local_Table
**	22-apr-1993 (rogerl) 
**		Add F_QR00A1_no_links for no registered objects found.
**	21-jul-1993 (rogerl) 
**		Add E_QR004F_NotAuthSL
**  	18-oct-93 (robf)
**               Updated F_QR010A_Access_Label to be Security Label, matching
**               ES 1.0 product.
** 	25-oct-93 (robf)
**               Add F_QR010B_Label_Gran, F_QR010C_rowc,F_QR010D_table,
**	10-dec-93 (robf)
**	        Add E_QR004B_HELP_sec_alarm for extended security alarm
**	        syntax, new messages for HELP SECURITY_ALARM:
**		  F_QR00A0_Database_has_no_alarm, 
**		  F_QR00A2_no_databases
**		  F_QR00A3_Installation_has_no_a
**		  F_QR00A4_Error_iidatabase_info
**	17-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'Technical Support Representative'
**		with 'Site Technical Contact'.
**	14-sep-94 (fraser)
**		Add E_QR0052.  This message was required for W4GL DBI.  It
**		was initially added to ingres63p: msw!front!tm!qr erqr.msg
**		and propagated here so that it will not be lost in the
**		6.5 code line.
**      08-mar-1995 (newca01) Bug 59235 
**              Add F_QR0039_n_a_default for "n/a" constant.
**	22-mar-1995 (markg)
**		Add new message E_QR004C (b67536).
**      06-mar-1996 (stial01)
**              Add new msg F_QR00A5_Page_size for variable page size project
**	21-may-1996 (shero03)
**		Added F_QR0123_hidata for RTree project
**	17-jul-1996 (ramra01)
**	        Add new msg F_QR00A6_rel_version for alter table project
**		Add new msg F_QR00A7_rel_totwid for alter table project
**	25-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added F_QR00A8_rel_cachepri
**	2-Jun-2004 (schka24)
**	    Messages for partitioned table help
**	25-Jun-2004 (schka24)
**	    More partitioning help messages
**	20-Dec-2004 (kodse01)
**	    BUG 113657
**	    Added E_QR00B1_HELP_perm_seq 
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	10-May-2006 (jenjo02)
**	    Add F_QR00B2_clustered
**	30-May-2006 (jenjo02)
**	    Removed F_QR00B2_clustered - no longer needed.
**	25-mar-2010 (toumi01) SIR 122403
**	    Add message constants for encryption.
*/

# define	_QR_CLASS	99
/*
**	qrerror.qc
*/
/*
**	Error occurs when a non existent table name is passed through QR
**	to the backend for the print command. This shouldn't occur, as this
**	should be caught up in the doprint routine. The message may
**	be removed in the future.
*/
E_QR0003_PRINT_table_not_exist	"PRINT: table does not exist or is not accessible by you.\n"
/*
**	Error occurs when a syntax error occurs from the PRINT statement;
**	the actual error is from a range statement syntax error. Like the
**	above error, it may also be removed.
*/
E_QR0004_PRINT_syntax_error	"PRINT: syntax error.\n"
/*
**	qrhelp.qc
*/
/*
**	This fast string is used to print the name...owner...type header
**	for the terminal monitor when just "help" is entered as a command.
**	The spacing here between name, owner, and type is critical, as
**	the data lines up under these headings.
**
**	NO LONGER USED.
*/
F_QR0001_name_own_type_header	"name                             owner                            type\n"
/*
**	These next 3 fast messages are the types shown under the "type"
**	heading in the "help" command. The possible types are table, 
**	view, or index.
*/
F_QR0002_table	"table"
F_QR0003_view	"view"
F_QR0004_index	"index"
/*
**	If an unknown type is stored in the catalogs, then "unknown"
**	is shown under the type heading.
*/
F_QR0005_unknown	"unknown"
/*
**	This is used when a syntax error occurs in the "help view" statement.
**	Following is the correct syntax for the 'help view' statement,
**	which really is in SQL, and shouldn't be translated. These are
**	fast strings because they're loaded into the qrb output buffer
**	rather than going to the normal FE error procedures.
**	
**	This 'legal syntax' string is also printed before the 'correct
**	syntax is' string for the help permit and help integrity commands.
*/
F_QR0006_Legal_Syntax_is	"Legal Syntax is:\n"
/*
**	HELP VIEW shouldn't be translated, but 'viewname' may be something
**	else in another natural language.
*/
F_QR0007_HELP_VIEW_viewname	"\tHELP VIEW [owner.]viewname[,[owner.]viewname]\n"
/*
**	Error occurs when the view requested to get help on doesn't
**	exist.
*/
/*
**	Text of error message should probably read 'view does not exist...'
*/
F_QR0008_Table_does_not_exist	"Table %s does not exist or is not accessible by you\n"
F_QR0009_View_does_not_exist	"View %s does not exist or is not accessible by you\n"
/*
**	An error has occurred retrieving info from the iiview system 
**	catalogs.
**	.
*/
F_QR000A_Err_ret_iiview	"\nSorry, Error(s) occurred obtaining Help from iiviews.\n"
/*
**	This is the " View <viewname> defined " heading that precedes
**	all view definitions.
*/
F_QR000B_View_def_heading	"View:                 %s\n"
/*
**	The view we're receiving help on isn't a view, its either a
**	table or an index, and we can't print the information requested
**	Alternatively, it's a view, but the definition isn't available.
*/
F_QR000C_Table_not_a_view	"Table %s is either not a view or its definition is unavailable\n"
/*
**	An error occurred issuing the OPEN CURSOR command while processing
**	the "help all" command.
**
**	NO LONGER USED.
*/
F_QR000D_Error_opening_CURSOR	"Error opening CURSOR for Help All\n"
/*
**	An error occurred during a CURSOR retrieve
**
**	NO LONGER USED.
*/
F_QR000E_Err_retrieving_CURSOR	"Error retrieving CURSOR for Help All\n"
/*
**	qrtblhlp.qc
*/
/*
**	An error occurred doing the retrieve on the tables mentioned
*/
F_QR000F_ret_err_ingtables	"\nSorry, Error(s) occurred obtaining Help from iiingres_tables.\n"
F_QR0010_ret_err_iiphysical	"\nSorry, Error(s) occurred obtaining Help from iiphysical_tables.\n"
F_QR0011_ret_err_iiindexes	"\nSorry, Error(s) occurred obtaining Help from iiindexes.\n"
/*
**	The following strings are titles used in the Help <table> 
**	display of output. All spacing is critical, that is the %s should
**	always be the same number of characters away from the beginning
**	of the string.
*/
F_QR0012_Name	"Name:                 %s\n"
F_QR0013_Owner	"Owner:                %s\n"
F_QR0014_Location	"Location:             %s"
F_QR0015_Type	"Type:                 "
F_QR0016_system_catalog	"system catalog\n"
/*
**	"system catalog", above, and the next few strings are the types
**	of objects that are valid for the "Type:" column, above. They
**	are "system catalog", "user table", secondary index on <table>", 
**	or "view", or "user synonym"
*/
F_QR0017_user_table	"user table\n"
F_QR0018_secondary_index_on	"secondary index on %s\n"
F_QR0019_view	"view\n"
F_QR001A_unknown	"unknown\n"
F_QR011A_user_synonym	"user synonym\n"
F_QR011B_user_synonym	"user synonym on table %s\n"
F_QR012A_procedure	"procedure\n"
/*
**	If a table has been created to expire at a certain date, then
**	it is said to be "Saved until" that date. Spacing is critical as
**	per the other titles for help.
*/
F_QR001B_Saved_until	"Saved until:          %s\n"
/*
**	How many bytes the row (tuple) takes up.
*/
F_QR001C_Row_width	"Row width:            %d\n"
F_QR001D_Number_of_rows	"Number of rows:       %d\n"
F_QR001E_Storage_structure	"Storage structure:    "
F_QR001F_compressed	"compressed "
/*
**	The compressed string, the storage structure (directly from the
**	database) and "with unique keys" is printed is these things are
**	part of the features of this table
*/
F_QR0020_with_unique_keys	"%s with unique keys\n"
F_QR0021_Number_of_pages	"Number of pages:      %d\n"
F_QR0022_Overflow_data_pages	"Overflow data pages:  %d\n"
F_QR0023_Journaling	"Journaling:           "
F_QR0024_index_enabled	"enabled if journaling on the base table is enabled\n"
F_QR0025_enabled	"enabled\n"
F_QR0026_at_next_checkpoint	"enabled after the next checkpoint\n"
F_QR0027_disabled	"disabled\n"
F_QR0028_ret_err_iikeycolumns	"\nSorry, Error(s) occurred obtaining Help from iikey_columns\n"
F_QR0029_Permissions	"Permissions:          "
F_QR002A_including_ALL_to_ALL	"yes, including ALL to ALL "
F_QR002B_and_RETRIEVE_to_ALL	"and RETRIEVE to ALL"
F_QR002C_yes__including_RETRIE	"yes, including RETRIEVE to ALL\n"
F_QR002D_yes_n	"yes\n"
F_QR002E_PROBE_n	"PROBE\n"
F_QR002F_none_n	"none\n"
F_QR0030_Integrities	"Integrities:          "
F_QR0031_ret_err_iistats	"\nSorry, Error(s) occurred obtaining Help from iistats\n"
F_QR0032_Optimizer_statistics	"Optimizer statistics: "
F_QR0033_view_stats	"will use any existing statistics on the base table(s)\n"
F_QR0034_index_stats	"will use any existing statistics on the base table\n"
F_QR0035_yes_see_stats_below	"yes; see avg count below, more info in the iistats catalog\n"
F_QR0036_Column_Information	"Column Information:\n"
/*
**	The heading for Column information is spacing critical, just
**	like the spacing for the "help" command. This is a two line
**	heading, with "key" and "avg count" (for statistics) on the top
**	line, and the rest of the junk on the second line.
**
**	F_QR0037 and F_QR0039 are NO LONGER USED.
*/
F_QR0037_column_info_topline	"                                                          key"
F_QR0038_avg_count	"  Avg Count\n"
/*
F_QR0039_column_info_mainline	" column name             type       length nulls defaults seq"
*/ 
F_QR003A_per_value	"  Per Value\n"
/*
**	These 'yes' and 'no' answers are space critical, depending on 
**	whether they are under the nulls or defaults column. They will
**	need a different amount of whitespace to properly center each
**	response under the appropriate heading.
*/
F_QR003B_yes_null	" yes "
F_QR003C_no_null	"  no "
F_QR003D_yes_default	"   yes  "
F_QR003E_no_default	"    no  "
F_QR0039_n_a_default    "   n/a  "
F_QR003F_unique_stats	"    unique"
F_QR0040_ret_err_iicolumns	"\nSorry, Error(s) occurred obtaining Help from iicolumns\n"
F_QR0041_Secondary_indices	"\nSecondary indexes:\n"
/*
**	The following string is the heading for the optional list
**	of secondary indices. Like all headings, spacing is critical
**
**	NO LONGER USED--now using F_QR0062, 63 and 64
*/
F_QR0042_indices_heading	" index name               structure  keyed on\n"
F_QR0043_Secondary_index_none	"\nSecondary indexes:    none\n"
/*
**	qrutil.qc
*/
F_QR0044_ret_err_iitables	"\nSorry, Error(s) occurred obtaining Help from iitables.\n"
/*
**	qrviphlp.qc
*/
F_QR0045_HELP_INTEGRITY_tblnam	"HELP INTEGRITY [owner].tablename [,[owner.]tablename]\n"
F_QR0046_HELP_PERMIT_tablename	"HELP PERMIT ON object_type [owner.]object [,[owner.]object]\n"
F_QR0047_Bad_value_to_qrvip	"Bad value 'which' passed to qrviphelp\n"
F_QR0048_Integrity_header	"Integrity Constraints on %s are:\n\n"
F_QR0049_Integ_Constraint_num	"Integrity Constraint %d:\n"
F_QR004A_Err_ret_iiintegrities	"Error retrieving data from iiintegrities\n"
F_QR004B_Table_has_no_integs	"Table %s has no integrities constraints on it\n"
F_QR004C_Permissions_header	"Permissions on %s are:\n\n"
F_QR004D_Error_retrieving_data	"Error retrieving data from iipermits\n"
F_QR004E_Permission_header	"Permission %d:\n"
F_QR004F_Table_has_no_permits	"Table %s has no permissions granted on it\n"
/*
**	qrrun.c
*/
F_QR0050_t_table_msg	"\n%s table\n"
/*
**	qrviphlp.qc
*/
F_QR0051_def_permit_alltoall	"define permit all on %s to all\n"
F_QR0052_def_permit_rettoall	"define permit retrieve on %s to all\n"
/*
**	qrsqhelp.c
*/
E_QR0005_Unable_to_open_sqhelp	" Unable to open SQL help file, qrhlpsql.hlp\n"
E_QR0007_invalid_help_syntax	"Sorry, HELP SQL %s is invalid. \n"
E_QR0008_Type_help_help         "Type \"help help\" for valid help syntax.\n"
/*
**	qrformat.c
*/
E_QR0009_adctmcvt_internal_err	"\n Internal Error from adc_tmcvt occurred, number %x\n"
/*
**	qrrun.c
*/
E_QR000A_Syntax_err_correct_is	"\nSyntax error on '%s'\n"
E_QR000B_PRINT_tblname_tblname	"\tPRINT tablename[, tablename]\n\n"
E_QR000C_end_stmt_with_semicln	"\tTerminate all statements with a semicolon.\n\n"
E_QR000D_Syntax_error_on_EOF	"\nSyntax error on 'EOF'\n"
/*
**	qrrun.c
*/
/*
**	qrtblhlp.qc
*/
E_QR000E_HELP_tblname_tblname	"\tHELP TABLE [owner.]tablename[,[owner.]tablename]\n\n"
/*
**	qrviphlp.qc
*/
/*
**	qrutil.qc
*/
E_QR000F_Error_allocing_mem	"\nSorry, an error occurred during memory allocation.\n"
E_QR0010_Error_freeing_memory	"\nWarning: an error occurred while freeing up memory.\n"
/*
**	qrhelp.qc
*/
F_QR0053_name	"Name"
F_QR0054_owner	"Owner"
F_QR0055_type	"Type"

/*
**	qrtblhlp.qsc
*/

F_QR00A5_Page_size      "Page size:            %d\n"
F_QR00A6_rel_version    "Alter table version:  %d\n"
F_QR00A7_rel_totwid     "Alter table totwidth: %d\n"
F_QR00A8_rel_cachepri   "Cache priority:       %d\n"
F_QR00A9_user_table_part	"user table (partitioned)\n"
F_QR00AA_phys_parts	"Physical partitions:  %d\n"
F_QR00AB_part_dims	"Partition dimensions: %d\n"

F_QR0056_Number_of_rows	"Number of rows:       %s\n"
/*%
** Explanation:
**	Indicate the number of rows for this table; in this case,
**	since on a gateway, the number of rows is not a number,
**	but is instead 'unknown'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0057_Number_of_pages	"Number of pages:      %s\n"
/*%
** Explanation:
**	Indicate the number of pages for this table; in this case,
**	since on a gateway, the number of pages is not a number,
**	but is instead 'not applicable'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0058_Overflow_data_pages	"Overflow data pages:  %s\n"
/*%
** Explanation:
**	Indicate the number of pages for this table; in this case,
**	since on a gateway, the number of pages is not a number,
**	but is instead 'not applicable'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0059_unknown	"unknown"
/*%
** Explanation:
**	Indicate that some item of information is unknown.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR005A_unknown_	"unknown   "
/*%
** Explanation:
**	Indicate that some item of information is unknown.
**	Minimum length (actually, exact length) is 10 due to
**	formatting.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR005B_topline_key	"Key"
F_QR005C_mainline_coltitle	"Column Name"
F_QR005D_mainline_dtype_name	"Type"
F_QR005E_mainline_dtype_len	"Length"
F_QR005F_mainline_nulls	"Nulls"
F_QR0060_mainline_defaults	"Defaults"
F_QR0061_mainline_seq	"Seq"
/*%
** Explanation:
**	These strings together comprise most of the per-column
**	title line for help output on a table.
**	Warning for translators:  The code as currently written,
**	not just the above seven strings, is very length specific.
**	That is, the translation for F_QR005D must be less than 9 chars,
**	that for F_QR005E be less than 7 chars, that for F_QR005F
**	be 5 chars (may include spaces), that for F_QR0060 be 8 chars
**	(may include spaces), and that for F_QR0061 be 3 chars.
**	These restrictions are because the 'help table' output needs
**	to line up, and there is absolutely no more space available
**	on an 80 column terminal (when including the additional space
**	required for F_QR0038/F_QR003A.)
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0062_index_name	"Index Name"
F_QR0063_structure	"Structure"
F_QR0064_keyed_on	"Keyed On"
/*%
** Explanation:
**	These strings together comprise most of the per-secondary index
**	title line for help output on a table.
**	Warning for translators:  The code as currently written,
**	not just the above three strings, is length specific.
**	That is, the translation for F_QR0063 must be less than or equal
**	to 10 chars.  This restriction is because the output needs
**	to line up.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0065_and_SELECT_to_ALL	"and SELECT to ALL"
F_QR0066_yes__including_SELECT	"yes, including SELECT to ALL\n"
F_QR0067_def_permit_alltoall	"grant all on %s to public;\n"
F_QR0068_def_permit_rettoall	"grant select on %s to public;\n"
/*%
** Explanation:
**	The above strings, used in qrviphlp and qrtblhlp, give information
**	on permissions when user is running SQL instead of QUEL.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

/*
**	qrhelp.qsc
*/

E_QR0011_Internal_help_error	"\nInternal error in the help module, code %d\n"
/*%
** Explanation:
**	The help command has been called with a code (qrb->step) that isn't
**	known about (shouldn't have been set).
**
** System Status:
**	The help command will not be processed further, but further queries
**	may be entered.
**
** User Action:
**	The user might conceivably wish to report the error.
*/

E_QR0012_no_help_permit	"Help on table permissions is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help permit' on a system for which such
**	help is unavailable--at time of writing, on gateways.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0013_no_help_integ	"Help on table integrities is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help integrity' on a system for which such
**	help is unavailable--at time of writing, on gateways.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0014_HELP_tblname_tblname	"\tHELP TABLE [owner.]tablename[,[owner.]tablename]\n\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

F_QR0069_ret_err_iimulti	"\n\nSorry, Error(s) occurred obtaining Help from iimulti_locations.\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iimulti_locations table.
**	The additional \n at the start of the message is to terminate some
**	text that precedes this message.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

F_QR006A_link	"link\n"
/*%
** Explanation:
**	This entry is a link to something (table, view or index) rather
**	than a native object.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR006B_link_to	"link to "
/*%
** Explanation:
**	This entry is a link to something of a known type, table, view or index,
**	rather than a native object of that type.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR006C_import	"import\n"
/*%
** Explanation:
**	This entry is an import of something (table, view or index) rather
**	than a native object.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR006D_import_of	"import of "
/*%
** Explanation:
**	This entry is an import of something of a known type, table, view or
**	index, rather than a native object of that type.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR006E_secondary_index	"secondary index\n"
/*%
** Explanation:
**	This entry is an index on an unknown object.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR006F_Version	"Version:              %s\n"
/*%
** Explanation:
**	Display the version of this object.  That is, what is its
**	origin.  i.e. INGRES 6.0 or ING6.0
**
**	Translator's note:  the spacing of this string is important
**	to lining up the output from 'help'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0070_Created	"Created:              %s\n"
/*%
** Explanation:
**	Indicate the date this object was created.  The date is displayed
**	in local time.
**
**	Translator's note:  the spacing of this string is important
**	to lining up the output from 'help'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0071_Location	",\n                      %s"
/*%
** Explanation:
**	Used in qrtblhlp.qsc.  If more than one location exists, print a
**	comma after the previous location name, go to the next line, and
**	print out this location properly lined up with previous output.
**
**	Translator's note:  the spacing of this string is important
**	to lining up the output from 'help'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0072_Duplicate_Rows	"Duplicate Rows:       %s\n"
F_QR0073_allowed	"allowed"
F_QR0074_not_allowed	"not allowed"
/*%
** Explanation:
**	Used in qrtblhlp.qsc.  Indicate if this table allows duplicate
**	rows, or whether instead rows must be unique.  Possible values are
**	"allowed" and "not allowed".
**
**	Translator's note:  the spacing of this string is important
**	to lining up the output from 'help'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0075_Check_Option	"Check option:         %s\n"
F_QR0076_on	"on"
F_QR0077_off	"off"
/*%
** Explanation:
**	Used in qrviphlp.qsc.  Indicate if this view has the Check Option
**	turned 'on' or 'off'.
**
**	Translator's note:  the spacing of this string is important
**	to lining up the output from 'help'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0078_View_Def	"\nView Definition:\n"
/*%
** Explanation:
**	Used in qrviphlp.qsc.  Heading for the view definition.  Used to
**	separate the definition from the tabular information.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

E_QR0015_HELP_indexname	"\tHELP INDEX [owner.]indexname[,[owner.]indexname]\n\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0006_Index_does_not_exist	"Index %s does not exist or is not accessible by you\n"
/*%
** Explanation:
**	The user has requested help on a non-existent index.  Note:  It is
**	possible that the DBA owns an index by this name but that the user
**	has a table of the same name that takes precedence.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Regroup and press on.
*/

E_QR0016_HELP_procedure	"\tHELP PROCEDURE [owner.]procedure-name[,[owner.]procedure-name]\n\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0017_HELP_link	"\tHELP REGISTER tablename[, tablename]\n\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0018_no_help_dbproc	"Help on database procedures is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help procedure' on a system for which such
**	help is unavailable.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0019_Procedure_does_not	"Procedure %s does not exist or is not accessible by you\n"
/*%
** Explanation:
**	The user has requested help on a non-existent procedure.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Regroup and press on.
*/

E_QR001A_Link_does_not_exist	"Registered object %s does not exist or is not accessible by you\n"
/*%
** Explanation:
**	The user has requested help on a non-existent registration.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Regroup and press on.
*/

E_QR001B_no_help_link	"Help on registered objects is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help register' on a system for which such
**	help is unavailable.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

F_QR0079_Proc_def_heading	"Procedure:            %s\n"
/*%
** Explanation:
**	Heading for the procedure definition.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR007A_Proc_Def	"\nProcedure Definition:\n"
/*%
** Explanation:
**	Heading for the procedure definition.  Used to
**	separate any permits from the procedure text.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

E_QR001C_err_ret_iiproc	"\nSorry, Error(s) occurred obtaining Help from iiprocedures\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iiprocedures table.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

F_QR007B_Link_def_heading	"Link:                 %s\n"
/*%
** NO LONGER USED
** Explanation:
**	Heading for the link definition.  Indicate name of the link.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR007C_Link_Def	"\nRegistration Text:\n"
/*%
** Explanation:
**	Heading for the registered object definition.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

E_QR001D_err_ret_iireg	"\nSorry, Error(s) occurred obtaining Help from iiregistrations\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iiregistrations table.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

E_QR001E_Table_not_a_link	"Table %s is either not registered or its\ndefinition is unavailable\n"
/*%
** Explanation:
**	The object specified has no registration definition stored in
**	iiregistrations.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR007D_Permissions_header	"Permissions on procedure %s are:\n\n"
/*%
** Explanation:
**	If permissions exist on the procedure for which help has been
**	requested, then this heading is printed before the permits
**	are printed.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR007E_Base_table	"Base table for view:  %s\n"
F_QR007F_yes	"yes"
F_QR0080_no	"no"
/*%
** Explanation:
**	Indicate if this table has a view defined on it.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0081_Proc_is_DBAs	"Procedure %s is owned by the DBA.\n"
/*%
** Explanation:
**	Indicate that the procedure on which help was requested is owned
**	by someone else (has to be the DBA) and that "that's it".
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0082_Base_table_name	"Base table name:      %s\n"
/*%
** Explanation:
**	Indicate the name of the base table for this index.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0083_Base_table_owner	"Base table owner:     %s\n"
/*%
** Explanation:
**	Indicate the owner of the base table for this index.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0084_Dup_index_keys	"Duplicate index keys: %s\n"
/*%
** Explanation:
**	Indicate if this index allows duplicate index keys.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0085_Index_Col_Info	"Index Column Information:\n"
/*%
** Explanation:
**	Header for the per column information for help index.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0086_Sort_Direction	"Sort Direction"
/*%
** Explanation:
**	Column header for sort direction information for help index.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0087_ascending	"ascending"
F_QR0088_descending	"descending"
/*%
** Explanation:
**	Indicate the sort direction of this key column.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

E_QR001F_err_ret_iiindex_col	"\nSorry, Error(s) occurred obtaining Help from iiindex_columns\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iiindex_columns table.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

E_QR0020_err_ret_ii_loghelp	"\nSorry, Error(s) occurred obtaining Help from iilog_help.\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iilog_help table.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

E_QR0021_not_a_table	"%s is of type %s and is not a table\n"
E_QR0022_not_a_view	"%s is of type %s and is not a view\n"
E_QR0023_not_an_index	"%s is of type %s and is not an index\n"
/*%
** Explanation:
**	User requested help TABLE on a view or index, or
**	user requested help VIEW on a table or index, or
**	user requested help INDEX on a table or view.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR0089_no_tables	"No tables were found that matched '%s'\n"
F_QR008A_no_views	"No views were found that matched '%s'\n"
F_QR008B_no_indices	"No indexes were found that matched '%s'\n"
F_QR008C_no_procedures	"No procedures were found that matched '%s'\n"
F_QR00A1_no_links	"No registered objects were found that matched '%s'\n"
F_QR00A2_no_databases	"No databases found that matched '%s'\n"
/*%
** Explanation:
**	User specified an object name to the help command using a pattern
**	match specification for which there were no tables (views, indices,
**	procedures, links (registered objects)) discovered.
**
** System Status:
**	Nothing special.
**
** User Action:
**	Re-type a different pattern match sequence.
*/

F_QR008D_name_too_long	"The name specified, %s,\nwas too long to match.  The limit is %d characters.\n"
/*%
** Explanation:
**	User specified an object name to the help command using a pattern
**	match specification which was longer than the allowable length.
**
**	The allowable length is FE_MAXNAME, even though a user could
**	conceivably type 'a**...***b' which is functionally equivalent to
**	the shorter 'a*b'.
**
** System Status:
**	Nothing special.
**
** User Action:
**	Re-type a different pattern match sequence.
*/

E_QR0024_not_a_link	"%s exists but is not a registered object\n"
/*%
** Explanation:
**	User requested help REGISTER on a non-registered object.
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

E_QR0025_err_ret_help_info	"\nSorry, Error(s) occurred obtaining Help \
from system catalogs\n on object '%s'.\nYou may not have permissions to \
access this object."
/*%
** Explanation:
**	An error occurred doing the retrieve on the system catalogs through
**	IIUIrel_help.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

/*
**	qrtxthlp.qsc
*/
F_QR008E_Node		"Node:                 %s\n"
F_QR008F_Database	"Database:             %s\n"
F_QR0090_Local_Table	"Local Object:          %s\n"
F_QR0091_Local_Owner	"Local Owner:          %s\n"
F_QR0092_DBMS		"DBMS:                 %s\n"

E_QR0026_err_ret_star_info	"\nSorry, Error(s) occurred obtaining Help from the STAR system catalogs.\n"
/*%
** Explanation:
**	An error occurred when retrieving information from the STAR system 
**	catalogs iiddb_tableinfo, iiddb_catalogs, and iiddb_ldbids for
**	the help register command.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

/*
**	The following strings are titles used in the Help rule
**	display of output. All spacing is critical, that is the %s should
**	always be the same number of characters away from the beginning
**	of the string.
*/
F_QR0093_Rule		"Rule:                 %s\n"
F_QR0094_Table		"Table:                %s\n"
F_QR0095_Rule_Def	"\nRule Definition:\n"

F_QR0096_no_rules	"No rules were found that matched '%s'\n"
/*%
** Explanation:
**	User specified an object name to the help rule command using a pattern
**	match specification for which there were no rules found.
**
** System Status:
**	Nothing special.
**
** User Action:
**	Re-type a different pattern match sequence.
*/

F_QR0097_Rule_is_DBAs	"Rule %s is owned by the DBA.\n"

/*%
** Explanation:
**	Indicate that the rule on which help was requested is owned
**	by someone else (has to be the DBA) and that "that's it".
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

E_QR0027_no_help_rule	"Help on database rules is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help rule' on a system for which such
**	help is unavailable.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0028_Rule_does_not_exist	"Rule %s does not exist or is not accessible by you\n"
/*%
** Explanation:
**	The user has requested help on a non-existent rule.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Regroup and press on.
*/

E_QR0029_HELP_rule	"\tHELP RULE [owner.]rulename[,[owner.]rulename]\n\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR002A_err_ret_iirul	"\nSorry, Error(s) occurred obtaining Help from iirules\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iirules table.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

/*
**	The following strings are titles used in the Help sequence
**	display of output. All spacing is critical, that is the %s should
**	always be the same number of characters away from the beginning
**	of the string.
*/
F_QR00AC_Sequence	"Sequence:             %s\n"
F_QR00AD_Seq_Def	"\nSequence Definition:\n"

F_QR00AE_no_sequences	"No sequences were found that matched '%s'\n"
/*%
** Explanation:
**	User specified an object name to the help sequence command using a pattern
**	match specification for which there were no sequences found.
**
** System Status:
**	Nothing special.
**
** User Action:
**	Re-type a different pattern match sequence.
*/

F_QR00AF_Seq_is_DBAs	"Sequence %s is owned by the DBA.\n"

/*%
** Explanation:
**	Indicate that the rule on which help was requested is owned
**	by someone else (has to be the DBA) and that "that's it".
**
** System Status:
**	Nothing special.
**
** User Action:
**	None.
*/

F_QR00B0_part_def	"Table partitioning definition:"

F_QR00C0_crypto_hdr	"Column encryption:    %s\n"
F_QR00C1_crypto_colname	"Encrypted Column Name"
F_QR00C2_crypto_width	"Width"
F_QR00C3_crypto_salt	"Salt"
F_QR00C4_yes_salt	" yes"
F_QR00C5_no_salt	"  no"
F_QR00C6_rel_datawid	"Encrypted width:      %d\n"
F_QR00C7_rel_totdatawid	"Alter table totwidth: %d\n"

E_QR0060_no_help_sequence  "Help on database sequences is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help sequence' on a system for which such
**	help is unavailable.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0061_Sequence_does_not_exist  "Sequence %s does not exist or is not accessible by you\n"
/*%
** Explanation:
**	The user has requested help on a non-existent sequence.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Regroup and press on.
*/

E_QR0062_HELP_sequence	"\tHELP SEQUENCE [owner.]sequencename[,[owner.]sequencename]\n\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0063_err_ret_iiseq	"\nSorry, Error(s) occurred obtaining Help from iisequences\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iisequences table.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

E_QR002B_no_creat_drop	"\nYour installation is not authorized to create or drop tables.\n"
/*%
** Explanation:
**	The user attempted to create or drop a table while in a Runtime 
**	INGRES installation.  "CREATE TABLE" and "DROP TABLE" SQL statements
**	are not allowed in Runtime INGRES.
**
** System Status:
**	Display an error message to the user.
**
** User Action:
**	None.
*/

E_QR002C_no_creat_table	"\nYour installation is not authorized to create tables.\n"
/*%
** Explanation:
**	The user attempted to create a table while in a Runtime INGRES
**	installation.  The "CREATE" and "RETRIEVE INTO" QUEL statements
**	are not allowed in Runtime INGRES.
**
** System Status:
**	Display an error message to the user.
**
** User Action:
**	None.
*/

E_QR002D_no_drop_object	"\nDrop operation must be explicit on your installation.\nUse 'drop index' or 'drop view'.\n"
/*%
** Explanation:
**	The user possibly attempted to drop an table while in a Runtime INGRES
**	installation.  The "DROP objectname" SQL statement is not allowed
**	in Runtime INGRES because objectname could be a tablename.  If
**	you wish to drop an index or view, you must use the "DROP INDEX"
**	or "DROP VIEW" statements.
**
** System Status:
**	Display an error message to the user.
**
** User Action:
**	None.
*/

/*
**	These are the data type descriptions used for logical keys.  Because
**	there is no room to display whether a column is system maintained or
**	not, this information is combined with the logical key data type.
**		
*/
F_QR0098_objkey_sm	"ObjKey,SM "
F_QR0099_objkeynosm	"ObjKeyNoSM"
F_QR009A_tblkey_sm	"TblKey,SM "
F_QR009B_tblkeynosm	"TblKeyNoSM"
/*
**	qrtxthlp.qsc  Security Alarm Messages
*/
F_QR009C_Sec_Alarm_header	"Security alarms on %s are:\n\n"
F_QR009D_Error_iisecurity	"Error retrieving data from iisecurity_alarms.\n"
F_QR009E_Sec_Alarm_number	"Security alarm %d:\n"
F_QR009F_Table_has_no_alarms	"Table %s has no security alarms.\n"
F_QR00A0_Database_has_no_alarm  "Database %s has no security alarms.\n"
F_QR00A3_Installation_has_no_a  "No security alarms defined for CURRENT INSTALLATION.\n"
F_QR00A4_Error_iidatabase_info	"Error retrieving data from iidatabase_info.\n"

E_QR002E_Syntax_err_invalid_ob	"\nSyntax error: %s is an invalid object name, correct syntax is:\n"
/*%
** Explanation:
**	The HELP command was expecting a valid INGRES object name. A syntax
**	error is issued.
**
** System Status:
**	Displays a syntax error message to the user.
**
** User Action:
**	None.
*/
E_QR002F_HELP_sec_alarm		"HELP SECURITY_ALARM  tablename[,tablename]\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0030_no_help_synonym	"Help on synonyms is not available on this system.\n"
/*%
** Explanation:
**	The user has typed a "help synonym" command on a system for which such
**	help is unavailable.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0031_HELP_synonym		"HELP SYNONYM [owner.]synonymname[,[owner.]synonymname]\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0032_HELP_comment		"HELP COMMENT TABLE [owner.]tablename[,[owner.]tablename]\n     or\nHELP COMMENT COLUMN [owner.]tablename columnname[,columnname]\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0033_HELP_comment_col	"HELP COMMENT COLUMN [owner.]tablename columnname[,columnname]\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR0034_no_help_comment	"Help on comments is not available on this system.\n"
/*%
** Explanation:
**	The user has typed a "help comment" command on a system for which such
**	help is unavailable.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0035_err_ret		"\nSorry, Error(s) occurred obtaining Help from %s\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on a standard catalog
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

E_QR0036_Synonym_does_not	"Synonym %s does not exist or is not accessible by you\n"
/*%
** Explanation:
**	The user has requested help on a non-existent synonym.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Keep going.
*/

E_QR0037_Column_does_not_exist	"Column %s does not exist\n"
/*%
** Explanation:
**	The user has requested help on a non-existent column.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Keep going.
*/

E_QR0038_no_help_sec_alarm	"Help on security alarms is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help security_alarm' on a system for which such
**	help is unavailable.  The 'help security_alarm' command is only 
**	available with a B1 DBMS server.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0039_Error_in_FErelopen     "Error accessing table information.\n
An error occurred in getting table information from 'FErel_open().' The return
status is '%0x'.  Please report this problem to your Site Technical
Contact."
/*%
** Explanation:
**	An error has occured when calling FErel_open() to find the list
**	of tables for help\g. User can't use help. This is the same
**	error which is displayed in the tables utility, where the routine was
**	originally taken from.
**
** System Status:
**	Display error to the user.
**
** User Action:
**	Keep going.
*/

E_QR003B_no_help_constraint	"Help on table constraints is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help constraint' on a system for which such
**	help is unavailable--at time of writing, on gateways.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR003C_no_help_default	"Help on table defaults is not available on this system.\n"
/*%
** Explanation:
**	The user has typed 'help default' on a system for which such
**	help is unavailable--at time of writing, on gateways.
**
** System Status:
**	Execution of the next query proceeds.
**
** User Action:
**	None.
*/

E_QR0040_no_help_event   "Help on dbevents is not available on this system.\n"
/*%
** Explanation:
**      The user has typed 'help permit on dbevent' on a system for which such
**      help is unavailable.  
**
** System Status:
**      Execution of the next query proceeds.
**
** User Action:
**      None.
*/
E_QR0041_HELP_perm_event      "HELP PERMIT ON DBEVENT [owner.]dbevent[,[owner.]dbevent]\n"
/*%
** Explanation:
**      The user has mis-typed the help command.
**
** System Status:
**      Display a syntax message to the user.
**
** User Action:
**      Re-type the command.
*/

E_QR0042_err_ret_iievt	"\nSorry, Error(s) occurred obtaining Help from iievents\n"
/*%
** Explanation:
**	An error occurred doing the retrieve on the iievents table.
**
** System Status:
**	Apologize to the user.
**
** User Action:
**	None.
*/

E_QR0043_Event_does_not   "dbevent %s does not exist or is not accessible by you\n"
/*%
** Explanation:
**      The user has requested help on a non-existent dbevent.
**
** System Status:
**      Display error to the user.
**
** User Action:
**      Regroup and press on.
*/


E_QR0045_HELP_perm_proc    "HELP PERMIT ON PROCEDURE [owner.]procedure[,[owner.]procedure]\n"
/*%
** Explanation:
**      The user has mis-typed the help command.
**
** System Status:
**      Display a syntax message to the user.
**
** User Action:
**      Re-type the command.
*/

E_QR0046_HELP_perm_on	    "HELP PERMIT ON TABLE|VIEW|PROCEDURE|DBEVENT|SEQUENCE [owner.]object_name[,[owner.]object_name]\n"
/*%
** Explanation:
**      The user has mis-typed the help command.
**
** System Status:
**      Display a syntax message to the user.
**
** User Action:
**      Re-type the command.
*/

E_QR0047_HELP_CONSTRAINT_obj    "HELP CONSTRAINT [owner.]object_name[,[owner.]object_name]\n"
/*%
** Explanation:
**      The user has mis-typed the help command.
**
** System Status:
**      Display a syntax message to the user.
**
** User Action:
**      Re-type the command.
*/

E_QR004A_HELP_DEFAULT_obj    "HELP DEFAULT [owner.]object_name[,[owner.]object_name]\n"
/*%
** Explanation:
**      The user has mis-typed the help command.
**
** System Status:
**      Display a syntax message to the user.
**
** User Action:
**      Re-type the command.
*/

E_QR004B_HELP_sec_alarm		"HELP SECURITY_ALARM  tablename{,tablename}\n
| HELP SECURITY_ALARM ON [TABLE] tablename{,tablename} \n
| HELP SECURITY_ALARM ON DATABASE database{,database}\n
| HELP SECURITY_ALARM ON CURRENT INSTALLATION\n"
/*%
** Explanation:
**	The user has mis-typed the help command.
**
** System Status:
**	Display a syntax message to the user.
**
** User Action:
**	Re-type the command.
*/

E_QR004C_help_sec_alarm_iidbdb	"You must be connected to the iidbdb to get help on database or installation\nsecurity alarms.\n"
/*%
** Explanation:
**	The user attempted to issue a HELP SECURITY_ALARM ON DATABASE, or
**	CURRENT INSTALLATION command while connected to a database other than
**	the iidbdb. These commands can only be executed when connected to
**	the iidbdb.	
**
** System Status:
**	Display an error message to the user.
**
** User Action:
**	Connect to the iidbdb, and issue the command again.
*/

F_QR0100_no_synonyms	"No synonyms were found that matched '%s'\n"
F_QR0101_no_columns	"No columns were found that matched '%s'\n"

/*
**	qrtxthlp.qsc  Synonym and Comments Messages
**
**	The following strings are titles used in the Help synonym
**	display of output. All spacing is critical, that is the %s should
**	always be the same number of characters away from the beginning
**	of the string.
*/
F_QR0102_has_no_remarks	"%s %s has no comments.\n"
F_QR0103_Synonym_Name	"Synonym Name:         %s\n"
F_QR0104_Synonym_Owner	"Synonym Owner:        %s\n"
F_QR0105_Table_Name	"Table Name:           %s\n"
F_QR0106_Table_Owner	"Table Owner:          %s\n"
F_QR0107_Column		"Column:               %s\n"
F_QR0108_Remark		"Remark:  %s\n\n"
F_QR0109_Long_Remark	"Long Remark:\n\n"
F_QR010A_Access_Label	"Security Label:       %s\n"
F_QR010B_Label_Gran     "Label Granularity:    %s\n"
F_QR010C_row            "row"
F_QR010D_table          "table"


/*	
**	qrtabhlp.qsc - Compression Information Strings
**
**	The following strings are titles displayed by the HELP INDEX and
**	HELP TABLE commands for Compression information. All spacing is 
**	critical, that is the %s should always be the same number of 
**	characters away from the beginning of the string.
*/
F_QR0110_Compression	"Compression:          "
F_QR0111_data		"data"
F_QR0112_key		"key"

F_QR0113_no_events       "No dbevents were found that matched '%s'\n"
/*%
** Explanation:
**      User specified an object name to the help permit dbevent command using 
**	a pattern match specification for which there were no dbevents found.
**
** System Status:
**      Nothing special.
**
** User Action:
**      Re-type a different pattern match sequence.
*/

F_QR0114_Object_has_no_permits   "Object %s has no permissions granted on it\n"

/*%
** Explanation:
**	'help sql stmt' was issued, where 'stmt' is not a valid sql stmt
**
** System Status:
**      Nothing special.
**
** User Action:
**      Re-type query with valid sql statement.
*/

/*
**	qrhelp.qsc
*/

E_QR0048_bad_sql_stmt          "%s is not a valid SQL statement\n"

/*%
** Explanation:
**	'help stmt' was issued, no noise word (object type)
**
** System Status:
**      Nothing special.
**
** User Action:
**      Re-type query using an object type
*/

/*
**	qrhelp.qsc
*/

E_QR0049_HELP_object         "\tHELP object_type [owner.]object_name[,[owner.]object_name]\n"

/*%
** 
*/
E_QR0050_HELP_perm_view	     "HELP PERMIT ON VIEW [owner.]view[,[owner.]view]\n"

/*%
**
*/

E_QR0051_HELP_perm_idx       "HELP PERMIT ON INDEX [owner.]index[,[owner.]index]\n"

/*%
** Explanation:
**	Heading used in HELP PROCEDURE [name]
*/

/*
** qrtxthlp.qsc
*/

F_QR0115_Proc_type          	"Procedure Type:       %s\n"
F_QR0122_Obj_type		"Object type:          %s\n"
F_QR0123_hidata			"hidata"

/*%
** Explanation:
**	tell user that an object was created by the system [ user ]
**
*/

/*
**	qrtxthlp.qsc
*/

F_QR0116_System_object		"system object"
F_QR0117_User_object		"user object"

/*%
** Explanation:
**	tell user what type of procedure it is
*/

/*
**	qrtxthlp.qsc
*/

F_QR0118_Native_proc  		"native"
F_QR0119_Imported_proc		"imported"
F_QR0120_Enterprise_proc	"enterprise access"
F_QR0121_Star_proc		"registered"

/*%
** Explanation
**	to print type of object [ if synonym ] at 'help'
*/

/*
**	qrhelp.qsc
*/

F_QR011C_synonym		"synonym"

/*%
** Explanation
**	Issue syntax error, inform of impending correct syntax message
*/

/*
** qrrun.c
*/
F_QR011D_Correct_synt_is	"\nSyntax error on '%s', correct syntax is:\n\n"

/*%
** Explanation
**	These tokens are issued in place of user-defined or unbounded data
**	in (for now) itm.
*/

/*
** qrretsel.c
*/

F_QR011E_Unbounded_data		"**Unbounded Data Type**"

F_QR011F_User_defined_data	"**User-Defined Data Type**"

/* TRANSLATORS NOTE:
** these two strings must be less than DB_MAXNAME (32 characters for now)
*/

E_QR004F_NotAuthSL	"You have insufficient authorization to
view security alarms\n\twhich may be in effect on '%s'.\n"

E_QR0052_all_unsupported_types	"The result contains only unsupported data types.\n"
/*%
** Explanation:
**	In the W4GL debugger, the user has executed an SQL statement where
**	the result contains only unsupported datatypes.  This is a DBI
**	message.
**
** System Status:
**	Display an informational message to the user.
**
** User Action:
**	Read the message.
*/

/*
** qrmklist.c - User didn't have auth to view alarms on object
*/

E_QR00B1_HELP_perm_seq  "HELP PERMIT ON SEQUENCE [owner.]sequencename[,[owner.]sequencename]\n"

/*%
**
*/
