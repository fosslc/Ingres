/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erqf.msg -	QBF Message Definition File.
**
** Description:
**	This file defines the messages for class QF.
**
** History:
**	Revision 6.5
**	15-sep-1993 (rdrane)
**		Reword msgs E_QF014B_BadTblName, E_QF0147_BadAbbrv, and
**		E_QF0187_BadOwnName to cover the case of user input which
**		is not in the proper unnormalized form for delimited
**		identifiers.  This addresses bug 54945.
**	30-jul-1993 (rdrane)
**		Add msgs E_QF0177_UnDataType, E_QF0178_AppUnDataType and
**		E_QF0179_AllUnDataType, and E_QF0180_NoAppAllowed as part
**		of fix for b51411.
**	16-aug-1993 (rdrane)
**		Add msgs E_QF0181_InvalidName, E_QF0182_TLit, E_QF0183_JLit,
**		E_QF0184_QLit, E_QF0185_OLit, S_QF0186_TableOnly, and
**		E_QF0187_BadOwnName.  Part of
**		extensive corrections to 6.5 owner.tablename support (b54048,
**		b51773, b51756, and b52570).
**
**
**	Revision 6.4
**	08/12/91 (emerson)
**		Added E_QF0175_BadTable (for bug 37365).
**
**	Revision 6.3/03
**	04/18/90 (martym)
**		Added E_QF015D, E_QF015E, E_QF015F.
**	06/21/90 (martym)
**		Added F_QF0160, F_QF0161, S_QF0162, S_QF0163.
**	07/02/90 (martym)
**		Added S_QF0164, S_QF0165.
**	10/04/90 (emerson)
**		Added E_QF0166, E_QF0167, E_QF0168, E_QF0169,
**		E_QF0170, E_QF0171 (for logical key support).
**	11/03/90 (emerson)
**		Added E_QF0172 to reflect the decision to disallow
**		joins on logical keys for this release.
**		Errors E_QF0168 thru E_QF0171 will never appear.
**
**	Revision 6.1  88/09  wong
**		Deleted unused and duplicate entries.  S_QF0007, S_QF0013-17,
**		S_QF0047, S_QF004C-4F, E_QF0073-4, E_QF0084, E_QF0089-8A,
**		E_QF0140-41, S_QF0037-39, S_QF00CC, S_QF00A0, S_QF00A5-8,
**		S_QF00AD-E, S_QF00B9, S_QF00D5, E_QF012F, E_QF0101-2,E_QF0104-7.
**	13-mar-89 (kenl) bug #4114, #4115, #4122
**		Removed references to range variables.  Added more info to
**		several messages.
**
**	Revision 6.0
**	30-jul-1987 (danielt)
**		Created.
**	24-nov-87 (kenl)
**		Deleted duplicate entries for S_QF00D3 and S_QF00D4.
**	6-aug-91 (blaise)
**		Added E_QF0173_BadArgument and E_QF0174_R_FlagSyntax.
**	09-mar-93 (connie) Bug #46785
**		Added E_QF008C_No_valid_type
**	23-mar-93 (connie) Bug #46785
**		Wording change for E_QF008C_No_valid_type
**	17-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'technical support representative'
**		with 'Site Technical Contact'.
**/

# define	_QF_CLASS	91
/*
**	append.qc
*/
S_QF0001_NoRows	"WARNING:  No row(s) appended by INGRES, failed on row %d."

E_QF0002_RangeError	"%0c(): couldn't turn off range for frame." /*DIAG */

E_QF0003_NoFields	"No fields on form match tables(s) in JoinDef.
Cannot perform APPEND.\n"

S_QF0004_Append	"Append #%d ..."
S_QF0005_NoRow	"WARNING:  Row %d was not appended by INGRES."

E_QF0007_NoMaster	"WARNING:  No row(s) appended by INGRES,
failed on master row.\n"

S_QF0008_InsertRow	"Inserting new row ..."

E_QF0009_CannotInsert	"Cannot insert a row into an empty table field.\n "

S_QF000A_QBF_Append_Function	"QBF Append Function"
S_QF000B_DoNotAppendQ
	"Do you wish to leave APPEND without appending data on the screen? "
/* PROMPT */

S_QF000C_TotalMDAppend	"Appended %d master and %d detail row(s)"
S_QF000D_TotalAppend	"Appended %d row(s)"
S_QF000E_Appending	"Appending data ..."

E_QF000F_InsertOnTableFld	"Must be on table field to insert a new row."
/*
**	bldquery.qc
*/
S_QF0010_InitForm		"Initializing form ..."
S_QF0011_UseDefaultFormQ	"Do you want to use the default form? "
/*Prompt*/
S_QF0012_InitDefaultForm	"Initializing default form ..."

/*
**	bldwhere.qc
*/
E_QF0018_bldwhere__couldn_t_in	"bldwhere: couldn't initialize range"
/*DIAG */
E_QF0019_bldwhere__couldn_t_ge	"Could not get value of field '%0c' in form %1c.\n
The WHERE clause for this query could not be built.  This is an internal coding
error:  Please contact your Site Technical Contact."
/*%
** Explanation:
**	This error occurs if a WHERE clause for a given query cannot be built.
**
** System Status: 
**	Program continues.
**	
** User Action: 
**	Contact support. 
*/

E_QF001A_bldwhere__couldn_t_ge	"Could not get value of column '%0c' in table field '%1c'.\n
The WHERE clause for this query could not be built.  This is an internal coding
error:  Please contact your Site Technical Contact."
/*%
** Explanation:
**	This error occurs if a WHERE clause for a given query cannot be built.
**
** System Status:
**	
** User Action:
*/

/*
**	browse.qc
*/
S_QF001B_QBF_Retrieve_Function	"QBF Retrieve Function"
S_QF001C_NoRowsForQuery	"No rows found for this query."
F_QF0001_NextMaster	"NextMaster"
F_QF0002_NxtDetail	"NxtDetail"

E_QF001D_FormInitError	"Cannot initialize form/data for browsing.\n
Cannot display this form."
/*%
** Explanation:
**	This error occurs if a browse form for a QBF query definition
**	cannot be initialized or built.
**
** System Status: 
**	The current form display is terminated
**	
** User Action: 
*/

S_QF001E_Retrieving	"Retrieving records ..."
S_QF001F_BackToQuery	"Returning to query mode . . ."
S_QF0020_EndBrowse	"Ending browse mode . . ."

E_QF0021_OutOfData	"Out of data below"
/* actually not an error, just a message */

S_QF0022_RetrieveMore	"Retrieving more rows ..."
/*
**	catalog.qc
*/
F_QF001F_QBF__JoinDefs_Catalog	"QBF - JoinDefs Catalog"
S_QF0023_Retrieving_JoinDefs_c	"Retrieving JoinDefs catalog . . ."
S_QF0024_MoreInfo	"MoreInfo"
S_QF0025_QBF___JoinDef_Details	"QBF - MoreInfo about a JoinDef"

E_QF0026_No_JoinDefs_found	"No JoinDefs found."
/* This is a status message rather than a true error */

/*
**	defform.qc
*/
E_QF0027_Can_t_initialize_QBF_	"Cannot initialize QBF default form.\n
The default form could not be created in the defform module of
QBF.  This is an internal coding error, and should be reported
to your Site Technical Contact."
/*%
** Explanation:
**	The default form for a table or a JoinDef could not be 
**	initialized.  This should never happen, so your
**	Site Technical Contact should be notified.
**
** System Status: 
**	Program terminates.
**	
** User Action: 
**	Contact support. 
*/
/*
**	doqexec.qc
*/
S_QF0028_Initializing_form____	"Initializing form ..."
/*
**	emisc.c
*/
E_QF0029_mqsaveStr__out_of_spa	"mqsaveStr: out of space."
/*DIAG */

/*
**	getchange.qc
*/
F_QF0004_NextTable	"NextTable"
E_QF002A_There_is_only_one_tab	"There is only one table in this JoinDef."
/* This is more of a message than a true error */

E_QF002B_Place_cursor_over_nam	"Place cursor over name of column you wish to delete\n
You need to move the cursor into the tablefield containing the name of the
column you wish to delete"
/*%
** Explanation:
**	This is a warning message telling the user to move to the
**	tablefield containing the column names for the JoinDef.
**
** System Status: 
**	Program continues.
**	
** User Action: 
*/
E_QF002C_No_fields_to_delete_f	"No fields to delete for this table"
/* This is a warning message: it hasn't been converted to use IIUGerr() */

E_QF002D_Can_t_delete_the_only	"Can't delete the only field remaining in the JoinDef"
/* This is a warning message: it hasn't been converted to use IIUGerr() */

E_QF002E_cant_delete_join_fld	"'%0c' is a join field - can't delete it from the display"
/* This is a warning message: it hasn't been converted to use IIUGerr() */

E_QF002F_No_previous_delete_to	"No previous delete to undo"
/* This is a warning message: it hasn't been converted to use IIUGerr() */

S_QF0030_QBF_Change_Display_Fo	"QBF Change Display Form"
E_QF0031_Bad_datatype_format	"Bad datatype format"
/* This is a warning message: it hasn't been converted to use IIUGerr() */


/*
**	getinfo.qc
*/
S_QF0032_RetrieveTblEntryInfo	"Retrieving Table Entry Form information ..."

E_QF0033_BadJoinDefName	"Invalid JoinDef name specified.\n
Please enter a valid name.  Use the JoinDefs Catalog frame to view valid
JoinDef names."
/*%
** Explanation:
**	This error occurs when a JoinDef name is given that
**	is not in the system catalogs.
**
** System Status: 
**	Returns to calling module
**	
** User Action: 
*/

E_QF0034_JoinDefProcErr	"Cannot process JoinDef with no tables specified.\n
Please specify tables in JoinDef specification frame."
/*%
** Explanation:
**	This error occurs when no tables are given in the 
**	JoinDef specification.
**
** System Status: 
**	Returns to calling module
**	
** User Action: 
*/

E_QF0035_BadStatus	"Unknown return code from 'mqgetinfo()'.\n
This is an internal coding error.  Please contact your Site Technical Contact
representative"
/*%
** Explanation:
**	This error occurs when mqgetinfo() returns an unknown return
**	code, which should never happen.
**
** System Status: 
**	Returns to calling module
**	
** User Action: 
**	Contact your Site Technical Contact.
*/

S_QF0036_RetrieveTableInfo	"Retrieving Table information ..."
/*
**	getnames.qc
*/
F_QF0005_Table	"Table: "

E_QF003A_ExceedMaxTables	"Exceeded maximum number of tables allowed.\n
Only %0d tables are allowed."
/*%
** Explanation:
**
** System Status: 
**	Returns to calling module
**	
** User Action: 
*/
E_QF003B_NonUniqueAbbrv	"Non-unique table abbreviation '%0c' found.\n
The abbreviation used for a table must be unique.  The default
abbreviation for a table, when one is not supplied by the user, is
the same as the table name."

/* Status message, not true error message */
/*
**	getnewnam.qc
*/
S_QF003C_Enter_a_QBFName__a_Jo	"Enter a QBFName, a JoinDef or a table name: "
E_QF003D_mqqexecute__couldn_t_	"mqqexecute: couldn't end range for frame."
/*DIAG*/

/*
**	getqdef.qc
*/
E_QF003E_Can_t_retrieve_named_	"Cannot retrieve named JoinDef from catalogs."
E_QF003F_Problem_building_join	"Problem building joins for named JoinDef."
/* 
**This appears to be a diagnostic message of some sort.  It should be
** investigated further (danielt).
*/
E_QF0040_Range_variable___s__a	"Abbreviation '%0c' and column '%1c' in join not found."
/*
**	getqname.qc
*/
S_QF0042_JoinDef_Name_	"JoinDef Name: "
S_QF0043_QBFName_	"QBFName: "
/*
**	getquery.qc
*/
F_QF0008_LastQuery	"LastQuery"
F_QF0009_Order		"Order"
S_QF0044_RetrievingSorted	"Retrieving sorted data . . ."
S_QF0045_RetrievingData		"Retrieving data . . ."
S_QF0046_QBF_Query_State	"QBF Query State"

E_QF0048_RngOnErr	"IIqfGetQuery: couldn't turn range on for frame." /*DIAG*/
E_QF0049_LastQryErr	"IIqfGetQuery: couldn't get last query." /*DIAG*/
E_QF004A_RngInitErr	"%0c():  could not init range for frame." /*DIAG*/

E_QF004B_NoLastQry	"No previous query to run again."

/*
**	initfrms.c
*/
E_QF00DF_InitFormError "Could not initialize form:  %0c."
/*
**	joins.qc
*/
F_QF000A_Rules	"Rules"
S_QF0050_Updating_joins______	"Updating joins . . ."
E_QF0051___s__is_an_invalid_ta	"'%0c' is an invalid table name."
S_QF0052_QBF_Join_Specificatio	"QBF Join Specification Frame"
S_QF0053_Updating_joins______	"Updating joins . . ."
E_QF0054_Improper_format__use_	"Improper format.\n
Use table.column or abbreviation.column."
E_QF0055_Table_range_variable_	"Table/abbreviation '%0c' not in current JoinDef.\n
The table name or abbreviation used for a join specification did not match
any of those used in the JoinDef Definition Form."

/*%
** Explanation:
**	The user must supply either the table name or a valid abbreviation
**	from the JoinDef Definition Form when entering a join specification.
**
** System Status:
**	An error is display.
**
** User Action:
**	The user should enter the correct table name or abbreviation or
**	return to the JoinDef Definition Form to find out what the correct
**	names are.
*/

E_QF0056_Column___s__not_in_ta	"Column '%0c' not in table '%1c'."
E_QF0057_Illegal_join_pair___s	"Illegal join pair.\n
Same table abbreviation '%0c' used on both sides."
E_QF0058_Illegal_join_pair___t	"Illegal join pair - type mismatch."
E_QF0059_Problem_adding_change	"Problem adding changed/new join information."
E_QF005A_Problem_updating_attr	"Problem updating attribute information."
E_QF005B_DEFAULT__bad_row_stat	"DEFAULT:  bad row state is %0d." /*DIAG*/
/*
**	makefrm.c
*/
E_QF005C_Unable_to_create_tabl	"Unable to create table fields with more than %d columns"
E_QF008C_No_valid_type	"Error in creating default form. Table contains no displayable columns. \n
To create a default form, INGRES requires the table to contain one or more
columns with displayable data types. Non-displayable data types include
table_key and object_key. A description of the supported types can be
found in the Using Ingres Through Forms and Menus manual."

/*%
** Explanation:
**	TableFields can only be created with less than MQ_MAXCOLS
**
** System Status: 
**	
**	
** User Action: 
*/

S_QF005D_notgiven	"notgiven"
S_QF005E_notgiven	"notgiven"
S_QF005F_notgiven	"notgiven"
S_QF0060_Query_Target_Name_is	"Query Target Name is "
S_QF0061_TABLE_IS	"TABLE IS "

E_QF0062_Could_not_get_default	"Could not get default format"
/*%
** Explanation:
**
** System Status: 
**	
**	
** User Action: 
*/
E_QF0063_Error_checking_dataty	"Error checking datatype compatibility"
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF0064_Bad_default_format	"Bad default format"
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF0065_No_more_memory	"No more memory" /*DIAG*/
E_QF0066_Can_t_set_format	"Can't set format"
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF0067_Could_not_get_display	"Could not get display size"
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF0068_Could_not_get_wrap_fo	"Could not get wrap format"
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
S_QF0069_TABLE_S__	"TABLE(S): "
/*
**	maketbl.qc
*/
E_QF006A_Could_not_get_format_	"Could not get format for column"
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF006B_Error_checking_column	"Error checking column datatype compatibility."
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF006C_Error_getting_format_	"Error getting format size information."
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF006D_No_more_memory_for_co	"No more memory for column format."
/*DIAG*/
E_QF006E_Error_setting_column_	"Error setting column format."
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF006F_Not_able_to_get_colum	"Not able to get column format size."
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF0156_Could_not_create_defa  "Could not create default form."
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
/*
**	mqbfhelp.qc
*/
F_QF000B_SimpleFields	"SimpleFields"
S_QF0070_Run_Table_Submenu	"Run Table Submenu"
/*
**	mqbfmain.qc
*/
S_QF00D3_JoinDefs	"JoinDefs"
S_QF00D4_Tables	"Tables"
E_QF0071_Can_t_initialize_stru	"Cannot initialize structure for JoinDef.\n
This is an internal coding error and should be reported to your
Site Technical Contact."
/*%
** Explanation:  
**	Couldn't initialize internal data structure for JoinDef.
**
** System Status: 
**	Program terminates or returns to caller with error code of (-1).
**
** User Action:
**	Contacts your Site Technical Contact.
*/

E_QF0072_ERROR__Incorrect_comm	"Correct syntax is:\n
qbf dbname [-uuser] [-e] [-l name]\n
	[ [ name ] [ [-j] [-F form] ] | [-t|-f] ]\n
	[ -mretrieve | -mupdate | -mappend | -mall ]\n"

/*%
** Explanation:  
**	User used incorrect command line syntax.
**
** System Status: 
**	Program terminates or returns to caller with error code of (-1).
**
** User Action:
*/
E_QF0075_Could_not_start_forms	"Could not start forms system - QBF exiting ..."
/*%
** Explanation:  
**	Couldn't initialize forms system.
**
** System Status: 
**	Program terminates or returns to caller with error code of (-1).
**
** User Action:
*/
S_QF0076_Do_you_want_to_use_th	"Do you want to use the default form? "
S_QF0077_QBF_Start_Up_Frame	"QBF Start-Up Frame"
/*
**	mqgetbrws.qc
*/
S_QF0078_No_more_masters	"No more masters"
S_QF0079_No_more_rows_in_query	"No more rows in query"
S_QF007A_No_more_records	"No more records"
S_QF007B_No_more_details_for_t	"No more details for this master ..."
/*
**	mqtblinfo.qc
*/
S_QF007C_Retrieving_column_inf	"Retrieving column information ..."

/*
**	qbfnames.qc
*/
F_QF0020_MoreInfo	"MoreInfo"
S_QF007D_QBF___QBFNames_Detail	"QBF - MoreInfo about a QBFName"
F_QF000C_QBF___QBFNames_Catalo	"QBF - QBFNames Catalog"
S_QF007E_Retrieving_QBFNames_c	"Retrieving QBFNames catalog . . ."
E_QF007F_There_are_no_QBFNames	"No QBFNames are in the catalog."
S_QF0080_QBFNames_Catalog	"QBFNames Catalog"
E_QF0081_QBFName___d__not_foun	"QBFName not found in %0c system catalog."
/*%
** Explanation:  
**
** System Status: 
**
** User Action:
*/
E_QF0082_JoinDef___s__not_foun	"JoinDef '%0c' not found in system catalogs."
/*%
** Explanation:  JoinDef name not found in the catalogs
**
** System Status: Returns from Catalog module
**
** User Action:
*/
E_QF0083_QBFName___d__not_foun	"QBFName '%0d' not found in %1c system catalog"
/*%
** Explanation:  QBF name not found in the catalogs
**
** System Status: Returns from Catalog module
**
** User Action:
*/
F_QF000D_Table_name_	"Table name: "
F_QF000E_JoinDef_name_	"JoinDef name: "
F_QF000F_Form_name_	"Form name: "
/*
**	qdescfrm.qc
*/
F_QF0010_YES	"YES"
F_QF0011_NO	"NO"
/*
**	qexecute.qc
*/
F_QF0021_Start	"Start"
F_QF0012_NewQueryTarget	"NewQueryTarget"

F_QF0024_APPEND	"APPEND"
F_QF0025_UPDATE	"UPDATE"

E_QF0085_NotOnSysCats	"Cannot use %0c Function on system catalogs."
/*%
** Explanation:  Appends and Updates are not allowed on system catalogs.
**
** System Status: Returns from query execution module
**
** User Action:
*/
E_QF0086_NotOnViews	"Cannot use %0c Function on views."
/*%
** Explanation:  Appends and Updates are not allowed on views.
**
** System Status: Returns from query execution module
**
** User Action:
*/
E_QF0087_NotForMD	"Cannot use %0c Function for Master/Detail with no tablefield."
/*%
** Explanation:
**	Appends and Updates are not allowed on Master/Detail JoinDefs with
**	no tablefield.
**
** System Status: Returns from query execution module
**
** User Action:
*/
E_QF0088_NotWithoutKey	"Cannot use %0c Function on tables without keys.\n
A table must contain one or more columns that define a unique key for that
table.  Otherwise, the %0c function will not work."

/*%
** Explanation:  Appends or Updates are not allowed on tables without keys.
**
** System Status: Returns from query execution module
**
** User Action:  Exit QBF, define a unique key for the table, return to QBF
*/

S_QF008B_QBF_Execution_Phase	"QBF Execution Phase"
E_QF008E_Maximum_number_of_fie	"Number of fields in target list exceeded %0d."
/*%
** Explanation: The target list for a query cannot exceed MQ_MAXCOLS
**
** System Status:
**
** User Action:
*/
/*
**	qfgtargs.c
*/
S_QF0090_Mode	"Mode: "
S_QF00D6_Name	"Name: "
/*
**	rules.qc
*/
F_QF0022_Joins	"Joins"
F_QF0015_MASTER	"MASTER"
F_QF0016_detail	"detail"
F_QF0017_QBF_UpdateDelete	"QBF Update & Delete Rules"
/*
**	sort.qc
*/
F_QF0018_Sort_Form	"Sort Form"
E_QF0091_DuplicateSortSeq	"Duplicate sort sequences are not allowed."
/*%
** Explanation: 
**	Duplicate sort sequences not allowed.
**
** System Status:
**	Program continues.
**
** User Action:
*/
E_QF0092_DetailSortSeq	"Detail sort sequences must be greater than master sort
sequences."
/*%
** Explanation: 
**	Detail sort sequences must be greater than the sort sequence associated
**	with the related master.
**
** System Status:
**	Program continues.
**
** User Action:
*/
E_QF0093_MaxSortSeq	"Sort sequence number must be between 1 and %0d."
/*%
** Explanation: 
**	Sort sequence must be between 1 and MQ_MAXCOLS.
**
** System Status:
**	Program continues.
**
** User Action:
*/
E_QF0094_ValidSortDir	"Only 'd' or 'a' may follow sort sequence number:\n
'd' = descending\n
'a' = ascending"
/*%
** Explanation: 
**	a sequence number can only be followed by a 'd' or an 'a'.
**
** System Status:
**	Program continues.
**
** User Action:
*/
/*
**	tables.qc
*/
S_QF0095_RetrievingJoinDef "Retrieving QBF Join Definition from catalogs . . ."
E_QF0096_JoinDefFetchErr "Cannot retrieve previously saved JoinDef."
E_QF0097_Enter1Table	"You must enter at least one MASTER table."
S_QF0098_Executing	"Executing . . ."

E_QF0099_NoJoinFlds	"No join fields found.\n
None of the tables within your JoinDef are joined to any other tables.
Use the JoinDef Join Specification Screen, 'Joins', to establish the
correct joins between the tables in your JoinDef."

/*%
** Explanation:
**	An AddDetail has been done on a Master/Detail Joindef (both in
**	SimpleField format).
**
** System Status:
**
** User Action:
*/

E_QF009A_NotAllJoined	"Not all tables joined.\n
At least one table specified in your JoinDef is not joined to any of
the other tables.  Use the JoinDef Join Specification
Screen, 'Joins', to establish
the correct joins between the tables in your JoinDef."

/*%
** Explanation:
**	There is at least one table in the JoinDef that is not joined to
**	any others.
**
** System Status:
**	Do not allow the user to execute the JoinDef until the table
**	is joined to another table.
**
** User Action:
**	Determine which table(s) is not joined to any other tables and
**	update the join information via the 'Joins' menu item.
*/

S_QF009B_NotSavedBlankQ "Changes were not saved.  Would you like to Blank anyway?"
/*Prompt*/

F_QF0019_ChangeDisplay	"ChangeDisplay"

E_QF009C_JoinDefNotRun	"You must run the JoinDef before changing the display."
E_QF009D_NoChangeForm	"You cannot change the display of a VIFRED form."
E_QF009E_ChangeDispErr	"Problem changing display."
E_QF009F_ReqMoreThan1	"Join Definition requires multiple tables.\n
More than one table is required in a Join Definition to specify joins for it."
S_QF00A1_QBF_Table_Entry_Form	"QBF Table Entry Form"
S_QF00A2_NotSavedJoinDefQ "JoinDef changes were not saved.  Would you like to %0c anyway?"
S_QF00A3_JoinDefSaveTitle	"QBF - Saving a JoinDef"

S_QF00A4_name	"name"
/*
**	tblwidth.qc
*/
E_QF00A9_The_number_of_columns	"The number of columns in the table exceeds %0d."
/*%
** Explanation: 
**	Number of columns in the given table exceeds the maximum 
**	allowed in the database.
**
** System Status:
**
** User Action:
*/
E_QF00AA_Error_found_in_format	"Error found in format"
/*
**	update.qc
*/
F_QF0003_Row		"Row"
F_QF0013_Master		"Master"
F_QF0014_Detail		"Detail"
F_QF001A_AllDetailRows	"AllDetailRows"
F_QF001B_DetailRow	"DetailRow"
F_QF001C_AllRows	"AllRows"
F_QF001D_NxtDetail	"NxtDetail"
F_QF001E_AddDetail	"AddDetail"

E_QF00AB_InitFormErr	"Cannot initialize form/data\nExiting Update."
/*%
** Explanation: 
**	Cannot initialize the form for this update session.
**
** System Status:
**	Exit Update module.
**
** User Action:
*/

S_QF00AC_DoNotUpdateQ	"Do you wish to leave UPDATE without saving changes? "
/* Prompt */

S_QF00AF_ReturnQryMode	"Returning to query mode . . ."
S_QF00B0_DeletesNotPermit	"Deletes are not permitted."

S_QF00B1_DeleteAllAssoc	"Delete all associated details?
(no for just the displayed one) "
/* Prompt */

S_QF00B2_Deletes_of_details_ar	"Deletes of details are not permitted."
S_QF00B3_No_more_detail_rows	"No more detail rows"
S_QF00B4_No_detail_rows_to_del	"No detail rows to delete"
S_QF00B5_Must_be_on_a_row_to_d	"Must be on a row to delete"
S_QF00B6_DeleteMenu	"Delete Submenu"
S_QF00B7_SavingChanges	"Saving changes . . ."
S_QF00B8_NoUpdates	"No updates to process ..."
S_QF00BA_EndingUpdate	"Ending update mode . . ."
E_QF00BB_IgnoreUpdateWarn	"WARNING:  Update ignored for row %0d."
/* Warning message */
E_QF00BC_NoColUpdWarn	"WARNING:  Column '%0c' in row %1d of table field
will NOT be updated."
/* Warning message */

S_QF00BD_SaveThusFarQ	"Do you wish to save the changes made thus far? "
/* Prompt */

S_QF00BE_QBF_Update_Function	"QBF Update Function"

/*
**	vfbldqry.qc
*/
E_QF00BF_Table_field_width_is_	"Table field width is 0.\n
Cannot build query."
/*%
** Explanation: 
** 	The width of the table field is 0.
**
** System Status:
**	The query will not be built.
**
** User Action:
*/
/*
**	vfmqbf.qc
*/
E_QF00C0_Can_t_initialize_stru	"Cannot initialize structure for QBF JoinDef"
S_QF00C1_Retrieving_QBF_Join_D	"Retrieving QBF Join Definition from catalogs ..."
E_QF00C2_Can_t_retrieve_QBF_Jo	"Can't retrieve QBF JoinDef from catalog"
S_QF00C3_Creating_default_form	"Creating default form from JoinDef '%s' . . ."
/*
**	writetodb.qc
*/
E_QF00C4__rQBF__ERROR__Unable_	"QBF: ERROR: Unable to create temp file needed to write out JoinDef."
/*DIAG*/
E_QF00C5_Could_not_get_current	"Could not get current node for writing form."
/*DIAG*/
/*
**	wrtupd.qc
*/
S_QF00C6_WritingData	"Writing data to INGRES ..."
E_QF00C7_ReadDataError	"Error reading data record."
E_QF00C8_UnmatchedRngVar	"Cannot match range variable."
E_QF00C9_SomeUpdated	"WARNING:  %0d out of %1d records were
updated/deleted."
S_QF00CA_ChangesSaved	"Changes saved."
E_QF00CB_NotAllUpdated	"WARNING:  Errors occurred - not all
updates/deletes saved."
E_QF00CD_DeadLockAborts	"Your changes were not made because of a deadlock."

F_QF0006_Skip		"Skip"
F_QF0007_Continue	"Continue"
S_QF00CE_SkipCurrentMasterQ	"Do you wish skip current master(S) or quit
update processing(Q)? "
S_QF00CF_ContinueProcessQ	"Do you wish to continue(C) or quit update
processing(Q)? "

E_QF00D0_InvalidAction	"Invalid action code, please enter legal response."

/*
**  Following are ER msgs for former QBF error.11 file error messages.
**  In each case the number following the last underscore gives the
**  corresponding former error number.
*/

E_QF0100_11000
"No database name given."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0103_11003
"No form specified for the '-F' flag."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0108_11008
"No compiled form specified with '-C' flag."	/* DIAG */

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0109_11009
"Cannot specify '-mMODE' flag without a QBFName, JoinDef or Table."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF010A_11010
"Error allocating space."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF010B_11011
"Illegal record type found in QBF catalog."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF010C_11012
"Error initializing internal QBF forms."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF010D_11013
"Bad format type found."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF010E_11014
"(Table: '%0c', Range: '%1c') Bad range variable."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF010F_11015
"BUILD TABLE: Can't create default table field."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0110_11016
"TABLE-FRAME MATCH: Join column '%0c' doesn't have a matching field on the form."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0111_11017
"FORM INIT: Illegal Vifred form, more than one table field found."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0112_11018
"FORM INIT: Illegal Vifred form, no table field found."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0113_11019
"TABLE-FRAME MATCH: field '%0c' and attribute '%1c' have incompatible types."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0114_11020
"QUERY BUILD: Bad operator for query in form '%0c', field '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0115_11021
"QUERY BUILD: Bad type for query in form '%0c', field '%1c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0116_11022
"WRTREC: Bad write to temporary file."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0117_11023
"Table '%0c' does not exist or is not owned by you."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0118_11024
"Can't open temporary file to save updates."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0119_11025
"Error breaking out of retrieve loop."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF011A_11026
"Bad table name, '%0c', retrieved."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF011B_11027
"Bad JoinDef name, '%0c', retrieved."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF011C_11028
"NAME not specified with '-l' flag."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF011D_11029
"JoinDef '%0c' does not exist or is not owned by you."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF011E_11030
"QBFName '%0c' does not exist or is not owned by you."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF011F_11031
"Can't initialize form '%0c'.  The form will not be used by QBF."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0120_11032
"No QBFName, JoinDef or Table named '%0c' exists or is owned by you."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0121_11033
"Can't update the QBF system catalog to save the Join Definition."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0122_11034
"Can't copy the Join Definition information into the catalog."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0123_11035
"Can't restart the transaction."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0124_11036
"Error starting up ingres."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0125_11037
"Mode '%0c' is an invalid query execution mode for -m flag."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0126_11038
"Cannot initialize forms for QBF Help and Catalog Functions."


/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0127_11039
"Cannot initialize forms for QBF Join Definitions Functions."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0129_11041
"Master/Detail join field using column '%0c' found in table field."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF012A_11042
"User codes could not be initialized."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF012B_11043
"Master/Detail joins specified are illegal.\n
Joins are only allowed between one master and one detail table."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF012C_11044
"Too many relations in JoinDef.  Maximum allowed is '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF012D_11045
"Exceeded total number of attributes in all relations.  Maximum allowed is '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF012E_11046
"Too many joins in JoinDef.  Maximum allowed is '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0130_11048
"Cannot specify both the '-F' and either the '-f' or '-t' flags."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0131_11049
"FECONNECT: Unable to access table '%0c'."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0132_11050
"Failure return from ade_pad."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0133_11051
"Out of space in retrieve buffer."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0134_11052
"Failure in afe_cancoerce."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0135_unrecognized_datatype
"Unrecognized datatype for column '%0c'"

/*
** Explanation:
**
** System Status:
**
** User Action:
*/


E_QF0136_COLUMN_SPEC_TOO_WIDE
"Join column specification is too wide for display.\n
    '%0c'
is too wide to fit on the display.
Choose a shorter abbreviation for the table name on the
JoinDef Definition Frame."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0137_JOINDEF_INCONSISTENT
"The JoinDef definition is internally inconsistent or malformed.\n
This is a system error.  Please contact your system administrator
and/or your Site Technical Contact."

/*
** Explanation:
**
** System Status:
**
** User Action:
*/

E_QF0138_NO_TABLES_IN_JOINDEF
"The JoinDef definition has no tables specified."

/*
** Explanation:
**	Joindef has no tables specified in the definition --
**	this would be a system error, the JoinDef is malformed.
**
** System Status:
**
** User Action:
*/

E_QF0139_NO_TABLE_RECORD
"The Table record for '%0c' cannot be located."

/*
** Explanation:
**	Can't find the table record corresponding to an attribute
**	range variable at the time the form is being built.  If this
**	happens the JoinDef is internally inconsistent and malformed.
**	It is a system error.
**
** System Status:
**
** User Action:
*/

E_QF0142_Bad_Joindef_Column	"Column in JoinDef not in table.\n
The column '%1c' in table '%0c' referenced in the JoinDef does not
appear in the underlying table.  Check to see if the table definition
has changed since the JoinDef was created."

/*%
** Explanation:
**	The columns in the JoinDef definition, as stored in the database,
**	do not match the columns in the table at the time the JoinDef
**	is retrieved.
**
** System Status:
**	No operations will be allowed on the JoinDef.
**
** User Action:
**	Check table definition underlying JoinDef.
*/

E_QF0143_Conflicting_Flags	"Conflicting command line flags.\n
More than one of the flags -f, -j, or -t appeared on
the command line.  These flags are mutually exclusive."

/*%
** Explanation:
**	Only one of the flags -f, -j or -t may appear on the QBF
**	command line at one time.
**
** System Status:
**	Program terminates or returns to caller with error code of (-1).
**
** User Action:
**	Re-enter command line with correct flags.
*/

E_QF0144_Mixed_Datatype_Join	"WARNING: Different datatypes used in join.\n
You have joined two columns that have different datatypes.  A join
between columns '%0c.%1c' and '%2c.%3c' may cause anomalies when accessing
data from tables within your database.  Please refer to the
Using INGRES Through Forms and Menus manual for more information on the problems
that you may encounter when joining two columns that are not of the same
datatype."

E_QF0145_Mixed_Nullable_Join	"WARNING: Nullable and non-nullable datatype used in join.\n
You have joined a nullable column with a non-nullable column.  A join
between columns '%0c.%1c' and '%2c.%3c' may cause anomalies when accessing
data from tables within your database.  Please refer to the
Using INGRES Through Forms and Menus manual for more information on the problems
that you may encounter when joining two columns with different
nullability characteristics."

E_QF0146_Mixed_Length_Join	"WARNING:  Datatypes of different lengths used in join.\n
You have joined two columns that have different lengths.  A join
between columns '%0c.%1c' and '%2c.%3c' may cause anomalies when accessing
data from tables within your database.  Please refer to the
Using INGRES Through Forms and Menus manual for more information on the problems
that you may encounter when joining two columns with different
lengths."


E_QF0147_BadAbbrv	"Invalid table abbreviation.\n
Table abbreviation '%0c' contains illegal characters or is
not properly delimited."

E_QF0148_Illegal_join_pair	"Illegal join pair - type mismatch.\n
QBF cannot create a default join if it is not possible to coerce the
datatype of one column to that of the other.  A default join
between columns '%0c.%1c' and '%2c.%3c' is not allowed in QBF.  Please
go to the JoinDef Join Specification Screen, 'Joins',  to remove
this join before selecting 'GO', 'ChangeDisplay', or 'Rules'."

E_QF0149_ChangesNotSaved	"WARNING:  Errors occurred - updates/deletes
not saved.\nAn error occurred while performing updates/deletes.  To prevent 
inconsistent data from being written to the database, the current set
of updates/deletes have been rolled back."

E_QF014A_max_hidecol_hit	"QBF key limit reached.\n
There is a limit of %0d keys that QBF allows within all tables it is
working with.  This can be either a single table or the sum of all of
the keys within a JoinDef."

E_QF014B_BadTblName	"Invalid table name.\n
Table name '%0c' contains illegal characters or is not properly delimited."

E_QF014C_BadJoinDefObj	"Invalid name found in JoinDef.\n
The current JoinDef has an object named '%0c' which contains illegal
characters. This JoinDef may have been created via the copyform command.
Please ensure that the file used with this command is correct." 

E_QF0150_duplicate_formfield	"Duplicate column name encountered.\n
QBF does not allow column names within the same table to differ from one another
only by the case of the letters within the columns.  The column '%0c' in
table '%1c' is identical, if you ignore case, to another column in the
same table."

F_QF0023_tblutil	"QBF"

S_QF0151_no_details_found	"No detail rows found for next master."

/*%
** Explanation:
**	A retrieve has been done on a Master/Detail Joindef (both in
**	SimpleField format) but no detail row exists for the retrieved
**
** System Status:
**	Detail row is left blank.
**
** User Action:
**	User must chose AddDetail if details are desired for this master.
*/

S_QF0152_DetailAdded	"Detail row added."

/*%
** Explanation:
**	An AddDetail has been done on a Master/Detail Joindef (both in
**	SimpleField format).
**
** System Status:
**
** User Action:
*/

E_QF0153_NoFldUpdWarn	"WARNING:  Field '%0c' in table %1c will NOT be
updated."

/*%
** Explanation:
**	An attempt has been made to modify the join field of a JoinDef.
**
** System Status:
**
** User Action:
**	If the user wants the ability to modify the join field of a JoinDef
**	the user should change the definition of the JoinDef to allow this
**	capability.
*/

E_QF0154_TempNoWrite	"Local or II_TEMPORARY directory NOT writable.\n
The current working directory or the temporary files directory (specified
by II_TEMPORARY) cannot have files created in it.  Please check that you
have access rights to the directory and that sufficient disk space exists
to create and write files there."

/*%
** Explanation:
**	The current working directory or the II_TEMPORARY directory cannot
**	have files created in it.  QBF and/or a gateway may, at different
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
**	makefrm.qsc
*/
S_QF0155_TABLES_	"TABLES: "
S_QF0157_TABLE_		"TABLE: "

E_QF0158_no_joindef_name
"A JoinDef name must be entered.\n
A JoinDef name needs to be entered here in order to create a new
JoinDef.  If you do not wish to create a JoinDef based upon a JoinDef, use
the 'Cancel' menuitem."
/*
** Explanation:
**      The user has entered 'OK', thus requesting creation of a JoinDef
**      without entering the name of a JoinDef upon which
**      to base the new JoinDef.
**
** System Status:
**      Do nothing.
**
** User Action:
**      Either enter a name or type Cancel.
*/

S_QF0159_JoinDefs       "List of JoinDefs"
/*
** Explanation:
**      Help file title for QBF JoinDef creation:
**              list of joindef names.
**
** System Status:
**
** User Action:
*/

S_QF015A_JoinDef_from_JoinDef      "Duplicating a JoinDef"

F_QF0026_Selecting_JDef_names   "Selecting JoinDef names. . ."
F_QF0027_JoinDef_Name   "JoinDef Name"
F_QF0028_JoinDefs	"JoinDefs"

F_QF0029_qfrole_prmpt
"Select a Role"

F_QF0030_qfrole_opt
"MASTER\n\
detail"

S_QF015B_Missing_role_name	"Missing role name.\n"
S_QF015C_Role		"Role: "
/*
** Explanation:
**	If the -R (role) flag is specified with neither a role nor a
**	password, then prompt for the role.  If prompting is turned off,
**	then state that the role name is missing (and then exit.)
**
** System Status:
**	Obtain role name, or exit.
**
** User Action:
**	Enter role name.
*/

/*
**	mqqdesc.qsc
*/
E_QF015D_ColIsSQLKeyWord
"Unable to execute SQL query on table '%0c'.\nColumn '%1c' is an SQL keyword.\nYou must rename this column.\n"
/*
** Explanation:
**	The user has a table containing a field name which is an SQL keyword.
**	A legal SQL statement can not be generated to operate on this table.
**
** System Status:
**	Inform the user and retrun.
**
** User Action:
**	The user has to rename the field.
*/
E_QF015E_RvarIsSQLKeyWord
"Unable to execute SQL query on table '%0c'.\nRange variable '%1c' is an SQL keyword.\nYou must rename this range variable.\n"
/*
** Explanation:
**	The user has a range variable which is an SQL keyword.
**	A legal SQL statement can not be generated to operate on this table.
**
** System Status:
**	Inform the user and return.
**
** User Action:
**	The user has to rename the range variable.
*/
E_QF015F_TabIsSQLKeyWord
"Unable to execute SQL query on table '%0c'.\nTable '%1c' is an SQL keyword.\nYou must raname this table.\n"
/*
** Explanation:
**	The user has a table which is an SQL keyword.
**	A legal SQL statement can not be generated to operate on this table.
**
** System Status:
**	Inform the user and return.
**
** User Action:
**	The user has to rename the table.
*/
F_QF0160_EnableModOfJF
"Yes;Enable modification of this field\nNo;Disable modification of this field"
/*
** Explanation:
**	These are choices for the ListPick form for enabling the modification 
**	of Join Fields.
**
** System Status:
**	Not applicable.
**
** User Action:
**	The user has to choose either Yes -or- No.
*/
F_QF0161_DisableDelOfRows
"Yes;Enable deletion of rows in this table\nNo;Disable deletion of rows in this table"
/*
** Explanation:
**	These are choices for the ListPick form for disabling the deletion
**	of rows.
**
** System Status:
**	Not applicable.
**
** User Action:
**	The user has to choose either Yes -or- No.
*/

S_QF0162_EnableModOfJF  "Enabling/Disabling Modification Of Join Fields"
S_QF0163_DisableDelOfRows  "Enabling/Disabling Deletion Of Rows"
S_QF0164_SaveJDEnd  "Save JoinDef Before Ending?"
S_QF0165_SaveJDQuit  "Save JoinDef Before Quitting?"

/*
**	joins.qsc
*/
E_QF0166_bad_object_key_join
"An object key can't be joined to something else.\n
Column '%0c' in table '%1c' is an object key but
column '%2c' in table '%3c' is not, so they cannot be joined."

/*%
** Explanation:
**	An object key may not be joined to a column that's not an object key.
**
** System Status:
**	An error is displayed, and the cursor is then positioned
**	on the offending pair of join columns.
**
** User Action:
**	The user should clear or correct the pair of join columns.
*/

E_QF0167_bad_table_key_join
"A table key can't be joined to something else.\n
Column '%0c' in table '%1c' is a table key but
column '%2c' in table '%3c' is not, so they cannot be joined."

/*%
** Explanation:
**	A table key may not be joined to a column that's not a table key.
**
** System Status:
**	An error is displayed, and the cursor is then positioned
**	on the offending pair of join columns.
**
** User Action:
**	The user should clear or correct the pair of join columns.
*/

E_QF0168_system_system_join
"Can't join two system-maintained logical keys.\n
Column '%0c' in table '%1c' and
column '%2c' in table '%3c' are both system-maintained logical keys
and cannot be joined, either directly or indirectly."

/*%
** Explanation:
**	A system-maintained logical key may not be joined to another
**	system-maintained logical key, either directly or indirectly.
**	(An indirect join is a chain of explicit joins).
**	A system-maintained logical key may be joined only
**	to a user-maintained logical key.
**	Note that a default joindef will never directly join
**	two system-maintained logical keys, even if they have the same
**	column name, but it may try to join two system-maintained
**	logical keys indirectly if they have the same name
**	and there's also a user-maintained logical key with same name.
**
** System Status:
**	An error is displayed, and, if the join is direct,
**	the cursor is then positioned on the offending pair of join columns.
**
** User Action:
**	The user should clear or correct the pair of join columns.
**	If the error occurs on the Tables screen (in a default joindef),
**	you must select the Joins menuitem; once there, you should remove
**	one of the explicit joins that's part of the offending indirect join.
*/

E_QF0169_user_system_MD_join
"Invalid join on a system-maintained logical key.\n
Column '%0c' in table '%1c' is a system-maintained logical key
and cannot be the detail in a master-detail join.
(This includes the case where the join is indirect, that is,
where there is a chain of explicit joins from the master to the detail)."

/*%
** Explanation:
**	A system-maintained logical key in a detail table may not be joined
**	to anything in a master table, either directly or indirectly.
**	Note that a default joindef will never directly join
**	a detail system-maintained logical key to a master
**	user-maintained logical key, even if they have the same
**	column name, but it may try to join two such
**	logical keys indirectly if they have the same name
**	and there's also a detail user-maintained logical key with same name.
**
** System Status:
**	An error is displayed, and, if the join is direct (explicit),
**	the cursor is then positioned on the offending pair of join columns.
**
** User Action:
**	The user should clear or correct the pair of join columns.
**	If the error occurs on the Tables screen (in a default joindef),
**	you must select the Joins menuitem; once there, you should remove
**	one of the explicit joins that's part of the offending indirect join.
*/

/*
**	update.qsc
*/
E_QF0170_DD_system_user_append
"This joindef does not support detail appends.\n
Your joindef includes a DD join on a system-maintained logical key
and a user-maintained logical key.
Appends of a detail are not supported in this case."

/*%
** Explanation:
**	If the join specification includes a detail-detail join
**	on a system-maintained logical key and a user-maintained logical key,
**	appends of a detail are not supported.
**	This restriction may be removed in a future release.
**
** System Status:
**	Control remains on the Update screen.
**
** User Action:
**	If the join is changed to be a master-detail join
**	(on the system-maintained logical key
**	and the user-maintained logical key),
**	masters can be added by doing an append and
**	details can be added by doing an update
**	(selecting 'Update' from the Execution Phase screen).
*/

/*
**	qexecute.qsc
*/
E_QF0171_MM_system_user_append
"This joindef does not support master appends.\n
Your joindef includes a MM join on a system-maintained logical key
and a user-maintained logical key.
Appends of a master are not supported in this case."

/*%
** Explanation:
**	If the join specification includes a master-master join
**	on a system-maintained logical key and a user-maintained logical key,
**	appends of a master are not supported.
**	This restriction may be removed in a future release.
**
** System Status:
**	Control remains on the Execution Phase screen.
**
** User Action:
**	If the join is changed to be a master-detail join
**	(on the system-maintained logical key
**	and the user-maintained logical key),
**	masters can be added by doing an append and
**	details can be added by doing an update
**	(selecting 'Update' from the Execution Phase screen).
*/

/*
**	joins.qsc
*/
E_QF0172_lkey_lkey_join
"Can't join logical keys.\n
Column '%0c' in table '%1c' and
column '%2c' in table '%3c' are logical keys and cannot be joined."

/*%
** Explanation:
**	A logical key may not be joined to another logical key.
**	This restriction may be removed in a future release.
**
** System Status:
**	An error message is displayed, and, if the error was detected
**	on the JoinDef Joins screen, the cursor is then positioned
**	on the offending pair of join columns.
**
** User Action:
**	The user should clear or correct the pair of join columns.
**	If the error occurs on the Tables screen (in a default joindef),
**	you should select the Joins menuitem and remove the offending join.
**	If the error occurs when you attempt to run an old joindef with a join
**	on a logical key (created with a release of QBF prior to 6.3/03),
**	you should destroy the JoinDef and create a new one without
**	the offending join.
*/

E_QF0173_BadArgument
"%0c: Bad argument."

E_QF0174_R_FlagSyntax
"Correct Syntax:\nQBF dbname [ -Rroleid ] [ flags ]"
/*%
** Explanation:
**      The user tried to specify a role and password when running QBF, by
**      using the illegal syntax "-R<role_id>/<passwd>".
**
** System Status:
**      QBF will exit after issuing this error message.
**
** User Action:
**      Specify the role, using "-R<role_id>", then wait to be prompted for a
**      password.
*/

E_QF0175_BadTable
"Error looking for table.\n
An error occurred in the DBMS when it tried to determine if table '%0c' exists.
It may be that the name '%0c' is somehow invalid for the DBMS (e.g. too long).
The previous message may give more details about the error."

/*%
** Explanation:
**	An error occurred in the target DBMS when it tried to determine
**	if a table exists.  The target DBMS may be a gateway; there may be
**	something about the table name which is invalid for that gateway.
**	For example, DB2 requires that table names be no more than 19
**	characters long.  A previous popup message may give more details
**	about the error.
**
** System Status:
**	The table name is not accepted.
**
** User Action:
**	Make sure you are specifying a table name that's valid for the DBMS
**	you are running QBF against.  If the error persists, you may need
**	to contact your system administrator.
*/
E_QF0176_Not_supported_type  "Unsupported Datatype Detected\n
Column '%0c.%1c' is defined with a datatype which is not
supported by the INGRES forms-based tools"
/*%
** Explanation:
**
** System Status:
**
** User Action:
*/
E_QF0177_UnDataType
"Cannot use %0c Function on tables containing columns of unsupported
datatype(s) unless all such columns are nullable and/or have a defined
default.  Otherwise, the %0c function will not work."
/*%
** Explanation:  Appends are not allowed on tables with unsupported
**		 datatypes (BLOBs) if a value must be supplied (NOT
**		 NULL and no defined DEFAULT).  The APPEND function
**		 was specified.
**
** System Status: Returns from query execution module
**
** User Action:  Exit QBF, define a default for the column(s), return to QBF
*/
E_QF0178_AppUnDataType
"The query returned zero rows.  Cannot APPEND to tables containing columns
of unsupported datatype(s) unless all such columns are nullable and/or have
a defined default."
/*%
** Explanation:  Appends are not allowed on tables with unsupported
**		 datatypes (BLOBs) if a value must be supplied (NOT
**		 NULL and no defined DEFAULT).  The UPDATE function was
**		 specified, and there are currently no rows to work with.
**		 Thus, APPEND is the only valid option, and it is disallowed.
**
** System Status: Returns from query execution module
**
** User Action:  Exit QBF, define a default for the column(s), return to QBF
**		 -OR- redefine the query such that rows are returned.
*/
E_QF0179_AllUnDataType
"All columns are of unsupported datatype(s).  The query cannot
be processed."
/*%
** Explanation:  No eligible columns exist in the query, and so no
**		 processing can occur.
**
** System Status: Returns from query execution module
**
** User Action:  Redefine the query to contain at least one eligible
**		 column (if possible).
*/
E_QF0180_NoAppAllowed
"The query includes at least one column of an unsupported datatype which
is NOT NULL and has no defined default.\n\n
UPDATE will not change the value of the unsupported datatype
column(s).\n\n
APPEND and INSERT operations are not allowed.\n\n
DELETE'd rows cannot be re-added by QBF."
/*%
** Explanation:  Appends are not allowed on tables with unsupported
**		 datatypes (BLOBs) if a value must be supplied (NOT
**		 NULL and no defined DEFAULT).  The UPDATE function was
**		 specified.
**
** System Status: Execution continues.
**
** User Action:  Understand the limitation of what you're doing!
*/
E_QF0181_InvalidName
"'%0c' is not a valid INGRES name for a %1c."
/*%
** Explanation:  A table specification is invalid (owner and/or name portion),
**		 a QBFname or Joindef specification is not a valid FE name,
**		 or a QBFname or Joindef was specified as an owner.object
**		 construct (determined by specification of -j/-f flag).
**		 The second field is retrieved from E_QF0182_TLit,
**		 E_QF0183_JLit, or E_QF0184_QLit.
**
** System Status: Execution stops.
**
** User Action:  Ensure that the object type is correct if an owner is
**		 specified.  Otherwise correct the probable typo.
*/
E_QF0182_TLit
"table"
E_QF0183_JLit
"JoinDef"
E_QF0184_QLit
"QBFName"
E_QF0185_OLit
"QBF object"

S_QF0186_TableOnly
"WARNING:  '%0c' will be treated as a table name, since owner specifications
and delimited identifiers are not recognized for JoinDefs and QBFNames."
/*%
** Explanation:  A name specification on lookup had an owner component or was
**		 determined to be a delimited identifier.  These constructs
**		 are only valid for table names, and so the name search will
**		 be limited to that object type.
**
** System Status: Nothing. This is just a warning ...
**
** User Action:  Ensure that you only want a table.  Otherwise correct the
**		 probable typo.
*/

E_QF0187_BadOwnName	"Invalid table name.\n
Owner specification '%0c' contains illegal characters or is not properly
delimited."

/*%
** Explanation:  A table name specification's owner component was determined
**		 to be a bogus identifier.
**
** System Status: Force respecification of the offending table name.
**
** User Action:  Correct the probable typo.
*/
