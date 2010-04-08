/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erxf.msg -	Message file for class XF
**
** Description:
**	This file defines the messages for class XF.
**
** History:
**	12-aug-1987 (rdesmond)
**		Created.
**	18-may-89 (marian)
**		Added messages for RULE support.
**	27-jul-89 (marian)	bug 6687
**		Modified e_xf0019 and e_xf0020 so they now refer to
**		views as well as tables.
**	08-jan-90 (marian)	bug 9148
**		Added E_XF0029_Warn_udt_and_c_flag.
**	23-oct-1990 (mgw)
**		Corrected a misspelling. It was in a comment, not a message,
**		but this comment gets picked up for the messages.txt file via
**		erxtract and is thus in a deliverable.
**	06-mar-1991 (billc)
**		Added msgs for events.
**	17-nov-93 (robf)
**		Add XF0082_Alarms_comment for unloading security alarms
**	17-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'Ingres Technical Support'
**		with 'Site Technical Contact'.
**      05-Jun-2000 (hanal04) Bug 101667 INGDBA 65
**              Added E_XF0061 to report instances where we are unable to
**              determine an absolute directory path.
**	01-Nov-2001 (gupsh01) 
**		Added error messages E_XF0007_Cannot_open, 
**		E_XF0008_Cannot_read, E_XF00025_Cannot_close for 
**		parameter file handling.
**	09-May-02 (gupsh01) 
**		Added error messages, E_XF0055_No_locale_found,  
**		E_XF0056_Charset_not_set , E_XF0057_Unknown_LC_error 
**		and E_XF0058_LC_Format_incorrect.
**	28-May-02 (gupsh01)
**		Modified error message E_XF0055_No_locale_found to 
**		E_XF0055_No_encoding_found. 
**	30-May-20 (gupsh01)
**		Added new error message E_XF0059_Colinfo_incorrect.
**	28-jan-03 (inkdo01)
**	    Added SEQUENCES message (XF007A).
**	28-Apr-2004 (gupsh01)
**	    Modify message E_XF0019, to include repetition of any table.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	25-Jul-2007 (drivi01)
**	    Updated message E_XF0002_Cannot_create to provide a clearer 
**	    error message on Vista.
**      13-Feb-2008 (hanal04) Bug 119826
**          Added E_XF005A_Alt_Col_Warn. If reported we will need to
**          provide a mechanism by which we can convert a table that does
**          not allow tuples to span pages to one that does.
**      28-May-2008 (hanal04 & Chris Dawe) Bug 120429
**          Added E_XF0062_Invalid_Timestamp.
**	16-nov-2008 (dougi)
**	    Add IDENTITY message (XF007B).
**/

# define	_XF_CLASS	90

/*
**	xfcopy.c
*/

E_XF0001_Cannot_generate_file
"Cannot generate file name."

/*%
** Explanation:
**	The file name generator (for user scripts and temp data file names)
**	is not working properly.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Contact your Site Technical Contact.
*/

/*
**	xffileio.c
*/

E_XF0002_Cannot_create	
"Cannot create file '%0c'. \nMake sure you have sufficient privileges to write to this directory."

/*%
** Explanation:
**	The file whose name is given cannot be opened.
**
** System Status:
**	Program terminates.
**
** User Action:
**	Contact your Site Technical Contact.
*/

E_XF0003_Cannot_write_to_file
"Cannot write to file '%0c'."

/*%
** Explanation:
**	The file whose name is given cannot be written to.
**
** System Status:
**	Program continues.
**
** User Action:
**	Check disk space availability, then contact your Site Technical Contact.
*/

S_XF0004_Default_directory_is
"Default directory is '%0c'."

S_XF0005_Directory_is	
"Directory is '%0c'."

S_XF0064_Unload_Directory_is
"Unload directory is '%0c'."

S_XF0065_Reload_Directory_is
"Reload directory is '%0c'."

E_XF0006_Invalid_directory_pat
"Invalid directory path '%0c'."

E_XF0130_Path_Too_Long
"Directory name '%0c' \n
is too long, truncating to %1d characters."

E_XF0007_Cannot_open_param
"Cannot open parameter file '%0c'."

E_XF0008_Cannot_read_param
"Cannot read line from parameter file '%0c'."

E_XF0025_Cannot_close_param
"Cannot close parameter file '%0c'."

/*%
** Explanation:
**	The user specified an invalid path in which to place the files.
**
** System Status:
**	Program terminates.
**
** User Action:
**	The user should specify a valid directory.
*/

/*
**	xfwrscrp.c
*/

S_XF0021_comment_header
"\n/* %0c script created by %1c, version %2c. */\n\n"

S_XF0010_one_table_for_user
"There is one table owned by user '%0c'."

S_XF0011_n_tables_for_user
"There are %0d tables owned by user '%1c'."

S_XF0110_one_table
"There is one table in the database."

S_XF0111_n_tables
"There are %0d tables in the database."

S_XF0012_one_view_for_user
"There is one view owned by user '%0c'."

S_XF0013_n_views_for_user
"There are %0d views owned by user '%1c'."

S_XF0112_one_view
"There is one view in the database."

S_XF0113_n_views
"There are %0d views in the database."

E_XF0019_Cannot_find_table
"There was a table or view specified on the command line that does not 
exist or is not owned by you, or has been supplied more than once."

E_XF0020_Cannot_find_tables
"There were tables or views specified on the command line that do not 
exist or are not owned by you."

/*%
** Explanation:
**	There was a table or there were tables specified on the commandline 
**	that does not exist or is not owned by the person running copydb.
**
** System Status:
**	Program continues looking for other tables specified on the
**	commandline.
**
** User Action:
**	Re-run copydb specifying the correct table names.
*/

S_XF0090_include_file
"iiud.scr"

S_XF0091_64_include_file
"iiud64.scr"

S_XF0092_65_include_file
"iiud65.scr"

/*
** Comment headers.
*/
S_XF0070_Comments_comment	"COMMENTS"
# define COMMENTS_COMMENT	S_XF0070_Comments_comment

S_XF0071_Views_comment		"VIEWS"
# define VIEWS_COMMENT		S_XF0071_Views_comment

S_XF0072_Indexes_comment	 "INDEXES"
# define INDEXES_COMMENT	S_XF0072_Indexes_comment

S_XF0073_Procedures_comment	"PROCEDURES"
# define PROCEDURES_COMMENT	S_XF0073_Procedures_comment

S_XF0074_Rules_comment		"RULES"
# define RULES_COMMENT		S_XF0074_Rules_comment

S_XF0075_Events_comment		"EVENTS"
# define EVENTS_COMMENT		S_XF0075_Events_comment

S_XF0076_Tables_comment		"TABLES"
# define TABLES_COMMENT		S_XF0076_Tables_comment

S_XF0077_Integrities_comment	"INTEGRITIES"
# define INTEGRITIES_COMMENT	S_XF0077_Integrities_comment

S_XF0078_Registrations_comment	"REGISTRATIONS"
# define REGISTRATIONS_COMMENT	S_XF0078_Registrations_comment

S_XF0079_Synonyms_comment	"SYNONYMS"
# define SYNONYMS_COMMENT	S_XF0079_Synonyms_comment

S_XF007A_Sequences_comment	"SEQUENCES"
# define SEQUENCES_COMMENT	S_XF007A_Sequences_comment

S_XF007B_Identity_comment	"IDENTITY SEQUENCES"
# define IDENTITY_COMMENT	S_XF007B_Identity_comment

S_XF0080_Constraints_comment	"CONSTRAINTS"
# define CONSTRAINTS_COMMENT	S_XF0080_Constraints_comment

S_XF0081_Grants_comment		"PERMISSIONS"
# define GRANTS_COMMENT		S_XF0081_Grants_comment

S_XF0082_Alarms_comment		"SECURITY_ALARMS"
# define ALARMS_COMMENT		S_XF0082_Alarms_comment

/*
**	xfproc.qsc
*/
S_XF0017_one_proc_for_user
"There is one procedure owned by user '%0c'."

S_XF0018_n_procs_for_user
"There are %0d procedures owned by user '%1c'."

S_XF0117_one_proc
"There is one procedure in the database."

S_XF0118_n_procs
"There are %0d procedures in the database."

/*
** 	xfregstr.qsc
*/

S_XF001C_one_tbl_for_user
"There is one registered table owned by user '%0c'."

S_XF001D_n_tbls_for_user
"There are %0d registered tables owned by user '%1c'."

S_XF001E_one_index_for_user
"There is one registered index owned by user '%0c'."

S_XF001F_n_indexes_for_user
"There are %0d registered indexes owned by user '%1c'."

S_XF0022_one_view_for_user
"There is one registered view owned by user '%0c'."

S_XF0023_n_views_for_user
"There are %0d registered views owned by user '%1c'."

S_XF011C_one_tbl
"There is one registered table in the database."

S_XF011D_n_tbls
"There are %0d registered tables in the database."

S_XF011E_one_index
"There is one registered index in the database."

S_XF011F_n_indexes
"There are %0d registered indexes in the database."

S_XF0122_one_view
"There is one registered view in the database."

S_XF0123_n_views
"There are %0d registered views in the database."

/*
** xfcomm.qsc
*/

S_XF0040_one_comment_for_user
"There is one comment owned by user '%0c'."

S_XF0041_n_comments_for_user
"There are %0d comments owned by user '%1c'."

S_XF0140_one_comment
"There is one comment in the database."

S_XF0141_n_comments
"There are %0d comments in the database."

/*
**	xfrules.qsc
*/
S_XF0027_one_rule_for_user
"There is one rule owned by user '%0c'."

S_XF0028_n_rules_for_user
"There are %0d rules owned by user '%1c'."

S_XF002A_one_event_for_user
"There is one event owned by user '%0c'."

S_XF002B_n_events_for_user
"There are %0d events owned by user '%1c'."

S_XF002C_one_sequence_for_user
"There is one sequence owned by user '%0c'."

S_XF002D_n_sequences_for_user
"There are %0d sequences owned by user '%1c'."

S_XF002E_one_sequence
"There is one sequence in the database."

S_XF002F_n_sequences
"There are %0d sequences in the database."

S_XF0127_one_rule
"There is one rule in the database."

S_XF0128_n_rules
"There are %0d rules in the database."

S_XF012A_one_event
"There is one event in the database."

S_XF012B_n_events
"There are %0d events in the database."

S_XF012C_one_synonym_for_user
"There is one synonym owned by user '%0c'."

S_XF012D_n_synonyms_for_user
"There are %0d synonyms owned by user '%1c'."

S_XF012E_one_synonym
"There is one synonym in the database."

S_XF012F_n_synonyms
"There are %0d synonyms in the database."

/*
** 	xfgetatt.qsc
*/

E_XF0029_Warn_udt_and_c_flag
"Column '%0c' in table '%1c' is a 'user-defined' data type.  Ingres
Corporation strongly discourages the use of the '-c' flag with tables
containing 'user-defined' data types.  Copying 'user-defined' data types
into other than fixed or bulk format may produce unexpected results."

E_XF014B_Warn_udt_and_c_flag
"Column '%0c' in table '%1c.%2c' is a 'user-defined' data type.  Ingres
Corporation strongly discourages the use of the '-c' flag with tables
containing 'user-defined' data types.  Copying 'user-defined' data types
into other than fixed or bulk format may produce unexpected results."

/*%
** Explanation:
**	The -c flag should not be used to unload tables with 
**	user defined data type columns.  It is strongly recommended 
**	that tables with user defined data types use bulk copy for 
**	unloading/reloading data.
**
** System Status:
**	Program continues to produce unload and reload scripts.
**
** User Action:
**	Re-run copydb/unloaddb without the -c flag.
*/

E_XF0031_Log_key_warn
"Warning: column '%0c' of table '%1c' \n
is a user-maintained logical key.  If '%0c' contains a \n 
copy of a system-maintained logical key, it will no longer be \n
valid after reloading the database."

E_XF014A_Log_key_warn
"Warning: column '%0c' of table '%1c.%2c' \n
is a user-maintained logical key.  If '%0c' contains a \n 
copy of a system-maintained logical key, it will no longer be \n
valid after reloading the database."

/*%
** Explanation:
**	System-maintained logical keys will be re-assigned when the database 
**	is reloaded.  If the user has saved these keys in user-maintained 
**	logical keys, the user-maintained keys will no longer be valid after
**	the database is reloaded.
**
** System Status:
**	Program continues to produce unload and reload scripts.
**
** User Action:
**	The user must arrange to reassign all user-maintained logical keys
**	after the reload.
*/

/*
** 	xfwrap.c
*/

E_XF0050_Cannot_alloc_buf
"Cannot allocate buffer for text."

/*%
** Explanation:
**	This is a memory allocation error.
**
** System Status:
**	Current object definition is not written.
**
** User Action:
**	Rerun program, perhaps after checking memory availability.
*/

E_XF0051_Out_of_text_descs
"Cannot allocate text descriptors.  Internal error - contact your Site Technical Contact."

/*%
** Explanation:
**	This is an Ingres internal error.
**
** System Status:
**	Current object definition is not written.
**
** User Action:
**	Rerun program, then contact your Site Technical Contact.
*/

E_XF0052_Bad_text_descriptor
"Bad text descriptor.  Internal error - contact your Site Technical Contact."

/*%
** Explanation:
**	This is an Ingres internal error.
**
** System Status:
**	Current object definition is not written.
**
** User Action:
**	Rerun program, then contact your Site Technical Contact.
*/

E_XF0053_Cannot_init_buf
"Cannot initialize wrapping buffer for text."

/*%
** Explanation:
**	This is an Ingres internal error.
**
** System Status:
**	Current object definition is not written.
**
** User Action:
**	Rerun program, then contact your Site Technical Contact.
*/

E_XF0054_Cannot_get_line
"Cannot get next line of text."

/*
** 	xffilobj.qsc
*/

/*%
** Explanation:
**	This is an Ingres internal error.
**
** System Status:
**	Current object definition is not written.
**
** User Action:
**	Rerun program, then contact your Site Technical Contact.
*/

E_XF0060_Out_of_memory
"Out of memory -- attempt to allocate memory failed."

/*%
** Explanation:
**	UNLOADDB or COPYDB was unable to allocate enough memory to complete
**	its task.
**
** System Status:
**	Process is aborted.
**
** User Action:
**	You must obtain more memory, either by freeing memory used elsewhere,
**	by adding memory to your machine, or running UNLOADDB/COPYDB on
**	a larger machine.
*/

E_XF0061_Can_Not_Determine_Dir
"Can not determine directory, please check permissions."

/*%
** Explanation:
**	UNLOADDB or COPYDB was unable to  determine the directory path.
**
** System Status:
**	Process is aborted.
**
** User Action:
**	Check the permissions for each directory in the directory path.
*/

E_XF0055_No_encoding_found
"Warning!! No equivalent encoding found for charset. Encoding \n
value for the XML file will be set to Ingres charset name, \n
this may cause problems when processing data.  Check your \n
character set value or modify its equivalent mapping in \n
xmlnames.map to acceptable value."

/*%
** Explanation:
**      Genxml was unable to find an equivalent coded character set value for
**	the II_CHARSET values set in the installation. It will force  
**	that encoding value to be set to the current for the xml file generated.
**
** System Status:
**      Process will be aborted. 
**
** User Action:
**	User should set the II_CHARSET value to a valid character set
**	change the encoding value in the xmlnames.map file to an 
**	acceptable value.
*/

E_XF0056_Charset_not_set
"Warning!! II_CHARSET not set for installation, aborting."

/*%
** Explanation:
**      No value for II_CHARSET is set in the installation. 
**
** System Status:
**      Process will be aborted.
**
** User Action:
**      User should set the II_CHARSET value to a valid Ingres 
**	coded character set value.
*/

E_XF0057_Unknown_LC_error
"Warning!! Unknown error looking up charset name."

/*%
** Explanation:
**	An unknown error occured when looking up the encoding value in the 
**	LC facility. The process will be aborted. 
**
** System Status:
**      Process is aborted.
**
** User Action:
** 	An internal error occured, contact your Site Technical Contact.
*/

E_XF0058_LC_Format_incorrect
"Syntax error at line %0d, invalid file format for xmlnames.map."

/*%
** Explanation:
**      The format of the entry at the given line is invalid for xmlnames.map 
**	file. This file looks up equivalent IANA names for the Ingres character  
** 	names. The format of this file should conform to the following:
**
**	"Ingres Characterset"< TAB >"IANA standard encoding name"< TAB >#"Comments"	
**
** System Status:
**      Process is aborted.
**
** User Action:
**      This file has an illegal format at a given line, correct the error and rerun
**	application.  
*/

E_XF0059_Colinfo_incorrect
"An unknown error occured when obtaining information about a table column."
/*%
** Explanation:
**      No information about a table column could be obtained because of mismatch 
**      in the column information from iicolumns and values in sqlda. 
**
** System Status:
**      Process is aborted.
**
** User Action:
**      Check the table structure for the correct sql type.
*/

E_XF005A_Alt_Col_Warn
"Warning: ALTER TABLE ALTER COLUMN statements for table\n
'%0c.%1c' will exceed the current page size and fail."

E_XF0062_Invalid_Timestamp
"An invalid timestamp was found during the generation of a\n'%0c' statement."
/*%
** Explanation:
**      adu_hrtimetodate() or similar has reported an error on a value
**      held in the catalogs.
**
** System Status:
**      Process is aborted.
**
** User Action:
**      An internal error occured, contact your Site Technical Contact.
*/
