/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ertu.msg -	Message file for class TU
**
** Description:
**	This file defines the messages for class TU.
**
** History:
**	10-aug-1987 (daver)
**		Created.
**	22-oct-1987 (peter)
**		Take out many of the FEmsg calls.
**	23-oct-1990 (mgw)
**		Corrected some misspellings. Both are in comments, not messages,
**		but these comments get picked up for the messages.txt file via
**		erxtract and are thus in a deliverable.
**	11/07/90 (emerson)
**		Added messages E_TU002F and E_TU0030 (bug 33846).
**	18-jun-1992 (mgw)
**		Added new strings F_TU006E_Synonym, F_TU000A_Base_Table,
**		F_TU0016_Base_Table_Owner and F_TU0046_Destroying_synonym and
**		changed F_TU006D_HelpSubject to support synonyms in 6.5
**	17-sep-1992 (mgw)
**		Added TU0009_Decimal.
**      12-jan-1993 (rogerl)
**              Correct E_TU0027_Illegal_column_name control so argument is
**              printed.
**	22-jan-1993 (mgw)
**		Added E_TU000E_Own_dot_Tab_notexist for clarity. Same as
**		E_TU0016_Table_doesnt_exist, but says table not owned by '%0c'
**		instead of table not owned by you.
**	16-apr-1993 (rogerl)
**		Add E_TU0010_Must_be_on_table_fiel (50751).
**	21-apr-1993 (rogerl)
**		Add E_TU000E_NotNull_NullDefault, to enable inclusion of row
**		number in error message (there may be multiple, consecutive
**		issuances of this message)
**	23-apr-1993 (rogerl)
**		Change wording & number of E_TU000A_QUEL_Defaults.  It was
**		never released and it is has been superseded, so use it for
**		a similar (but separate) purpose. (now E_TU000A_NA_Defaults)
**	21-may-1993 (rogerl)
**		Add E_TU0012_NADataType to display at BLOB column creat attempt
**	2-aug-1993 (rogerl)
**		Lower case datatype names (53015)
**	27-aug-1993 (mgw) Bug #54557
**		Added error message E_TU000F_Bad_Defaultability for when
**		user enters an illegal defaultability in the Defaults column.
**	20-sep-1993 (mgw)
**		Added new datatypes BYTE, BYTE VARYING, LONG BYTE, and
**		LONG VARCHAR.
**	17-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'INGRES Technical Support'
**		with 'Site Technical Contact'.
**	2-Dec-07 (kibro01) b119562
**		Added extra messages to handle scaling of values in "tables"
**/

# define	_TU_CLASS	88

/*
**	the n/a string stands for "not applicable".
**      (pgm variables that hold defaultability and nullability
**      strings are declared as 5 byte chars)
*/
F_TU0001_n_a	"n/a"

E_TU0001_Bad_Key_Number	
"'%0c' is an invalid key number.\n
You must enter a valid number for the key, numbered from 
1 for the most significant column in the key."
/*%
** Explanation:
**	Occurs if error found in converting the entry for a
**	key number into a number.
**
** System Status:
**	Key number is rejected.
**
** User Action:
**	User should enter a valid key number.
*/

E_TU0002_Must_be_on_table_fiel	
"Must be on list of columns to use \"Insert.\"\n
The cursor must be on the table field containing the list of columns to use the
\"Insert\" command.  The \"Insert\" command inserts a blank row in the list of
columns at the current position so that a new column can be entered.  Use the
\"Save\" command to create the table in the database."

/*%
** Explanation:
**	Occurs if the "Insert" menu selection is selected while the
**	cursor is not on the table field.
**
** System Status:
**	"Insert" command is ignored.
**
** User Action:
**	Put cursor on table field and retry.
*/

E_TU0003_Must_be_on_table_fiel	
"Must be on list of columns to use \"Delete.\"\n
The cursor must be on the table field containing the list of columns to use the
\"Delete\" command.  The \"Delete\" command removes the current row from the
list of columns.  Use the \"Destroy\" command in the catalog list to remove
the table from the database."

/*%
** Explanation:
**	Occurs if the "Delete" menu selection is selected while the
**	cursor is not on the table field.
**
** System Status:
**	"Delete" command is ignored.
**
** User Action:
**	Put cursor on table field and retry.
*/

E_TU0004_Cannot_Move_Empty_Col	"Cannot move an empty column.\n
The cursor must be on a row containing a column name to use the \"Move\"
command."

/*%
** System Status:
**	"Move" command is ignored.
**
** User Action:
**	Put cursor on row containing a valid column name.
*/

E_TU0005_Must_be_on_table_fiel	"Must be on list of columns to use \"Move.\"\n 
The cursor must be on the table field containing the list of columns to use
the \"Move\" command.  The \"Move\" command reorders the rows in the list of
columns." 

/*%
** Explanation:
**	Occurs if the "Move" menu selection is selected while the
**	cursor is not on the table field.
**
** System Status:
**	"Move" command is ignored.
**
** User Action:
**	Put cursor on table field and retry.
*/

E_TU0006_Dest_Must_Be_Column	"Must be on list of columns to use \"Place.\"\n 
The cursor must be on the table field containing the list of columns to use
the \"Place\" command.  The \"Place\" positions the column selected in a
previous \"Move\" command."

/*%
** Explanation:
**	Occurs if the "EditDefaults" menu selection is selected while the
**	cursor is not on the table field.
**
** System Status:
**	"EditDefaults" command is ignored.
**
** User Action:
**	Put cursor on row in table field and retry.
*/

E_TU0010_Must_be_on_table_fiel	"Must be on list of columns to use \"EditDefaults.\"\n The cursor must be on the table field containing the list of columns to
use the \"EditDefaults\" command.  The \"EditDefaults\" choice allows the user
to edit the user defined defaults value associated with a particular column."

/*%
** Explanation:
**	Occurs if the "Place" menu selection is selected while the
**	cursor is not on the table field.
**
** System Status:
**	"Place" command is ignored.
**
** User Action:
**	Put cursor on table field and retry.
*/

S_TU0007_Enter_Table_Name	"Enter a table name: "
F_TU0032_Working_dot_dot_dot	"Selecting table definition . . ."
F_TU0049_Selecting_Names	"Selecting table names . . ."

/* following appears in table field with popup prompt to select unique keys */
F_TU0069_Title		"Should table have unique keys?"
F_TU0034_Row1_Col1	"Unique"
F_TU0035_Row1_Col2	"Create table as btree with Unique Keys"
F_TU0036_Row2_Col1	"Not Unique"
F_TU0037_Row2_Col2	"Create table as btree with Non-unique Keys"

/* following are used in the confirmation popup for Destroy a table */
F_TU006A_Table	"table"
F_TU006B_View	"view"
F_TU006C_Index	"index"
F_TU006D_HelpSubject "Confirm a Destroy of Table, View, Index, or Synonym"
F_TU006E_Synonym "synonym"

/* following appears as help file title for popup form that gets info
on unique keys */
F_TU0033_Unique_Keys		"Unique Keys"

E_TU000D_Table_already_exists	"Table '%0c' already exists.\n
The table cannot be created because a table with the same name as '%0c'
already exists in the database.  Specify a different name for the table,
and select \"Save\" again."

/*%
** Explanation:
**	A table of the same name as the one that is being created
**	already exists.
**
** System Status:
**	Table is not created.
**
** User Action:
**	Specify a different name, and select Save again.
*/

F_TU003B_Creating_table	"Creating table '%0c' . . ."

F_TU003C_Table_created	"Table '%0c' created"

E_TU0011_Error_modifying_table	
"Error(s) occurred while modifying table '%0c'.\n
A database error occurred while trying to MODIFY the table to have a
keyed structure.  The previous error message should have given details
about what went wrong. Because of the error, the table has been dropped;
fix the problem and then select Save again to re-Create the table.
If you are not sure how to fix the problem, call your Site Technical Contact."

/*%
** Explanation:
**	A database error occurred.
**
** System Status:
**	Same status shown in the previous error.
**
** User Action:
**	Take action from previous error.
*/

F_TU0040_Table_modified	"Table '%0c' modified on '%1c'"

F_TU0041_Starting_QBF	"Starting QBF . . ."

S_TU0013_End_Anyway	"Changes were not saved.  Would you like to End anyway?"
					/* Prompt from CREATE table */
/*
**	examine.qc
*/
S_TU0014_Enter_new_table_name	"Enter new table name: "

E_TU0015_No_attributes_found	"No columns found for table '%0c'.\n
No columns were found for table '%0c'.  Check to see that the table exists."

/*%
** Explanation:
**	This should not happen very often.  The most likely explanation
**	is that a concurrent user destroyed the table.
**
** System Status:
**	Examine is ignored.
**
** User Action:
**	Check if table exists.
*/
/*
** following are displayed in display only fields, as field titles in
** form tuexaminefr.
*/
F_TU0002_Columns	"Columns:"
F_TU0003_Rows	"Rows:"
F_TU0004_Storage_Struct	"Storage Structure:"
F_TU0005_Pages_Overflow	"   Pages/Overflow:"
F_TU0006_Journaling	"Journaling:"
F_TU0091_PagesK_Overflow	"Pages(K)/Overflow:"
F_TU0092_Pages_OverflowK	"Pages/Overflow(K):"
F_TU0093_PagesK_OverflowK	"Pages(K)/O'flw(K):"
F_TU000A_Base_Table	"Base Table:"
F_TU0016_Base_Table_Owner	" Base Table Owner:"

E_TU0016_Table_doesnt_exist
"Table '%0c' does not exist or is not owned by you.\n
Table '%0c' does not exist or is owned by another user."
/*%
** Explanation:
**	Table lookup failed.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Give table name that exists, or check ownership of table.
*/

E_TU000E_Own_dot_Tab_notexist
"Table '%0c' does not exist or is not owned by '%1c'.\n
Table '%0c' does not exist or is owned by another user."
/*%
** Explanation:
**	Table lookup failed.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Give table name that exists, or check ownership of table.
*/

/*
**	fespawn.qc
*/
/*
**	fespawn.qc
*/
F_TU0007_UTexe_flags1	"database = %%S, equel = %%S, flags = %%S, %s"
F_TU0008_UText_flags2	"database = %%S, equel = %%S, %s"
S_TU0017_Press_RETURN_to_clear	"Press RETURN to clear screen"

E_TU0018_Subsys_not_available
"Subsystem '%0c' is not available.\n
Subsystem '%0c' is not available on your machine."
/*%
** Explanation:
**	Occurs if the subsystem startup system gets a UTNOPROG return.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Check installation.
*/

E_TU0019_Cannot_start_subsys
"Could not start subsystem '%0c'.\n
The subsystem '%0c' could not be started.  The return status
is '%1x'.  This is usually an installation problem, a permission
problem, or because you have a quota that is too low to run 
another subprocess.  See the previous error for details."
/*%
** Explanation:
**	A return value from the subsystem startup system was in error.  This
**	message follows another error that should have given more information.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Correct previous error.
*/

/*
**	formutil.qc
*/
/*
**	helptbl.qc
*/
E_TU001B_Error_obtaining_table	"Error obtaining table information (tufindowner,FErel_open)"
E_TU001C_Cant_allocate_memory	"Can't allocate memory for seq keys"
E_TU001D_Error_in_FEatt_open	"Error accessing catalogs in starting FEatt_open"
E_TU001E_Error_freeing_up_memo	"Error freeing up memory in tugetattr()"
/*
**	main.qc
*/
/* 2/90: following 3 slow messages no longer used (pete) */
S_TU001F_wanna_destroy_table	"Do you wish to destroy table '%0c'? "
S_TU0047_wanna_destroy_view	"Do you wish to destroy view '%0c'? "
S_TU0048_wanna_destroy_index	"Do you wish to destroy index '%0c'? "

F_TU003D_Destroying_table	"Destroying table '%0c' . . ."
F_TU003E_Destroying_view	"Destroying view '%0c' . . ."
F_TU003F_Destroying_index	"Destroying index '%0c' . . ."
F_TU0046_Destroying_synonym	"Destroying synonym '%0c' . . ."

E_TU004B_Unknown_tbltype	"Object is not a Table, View or Index.\n
The object type for object '%0c' has a value
of '%1d', which does not correspond to a Table, View or Index. The Tables Utility
can only Destroy Tables, Views and Indexes."

E_TU004C_Err_destroy_exprt	"Expert mode: error Destroying '%0c'.\n
An error occurred while trying to destroy '%0c' while running in expert mode.
Note that only tables, and not views or indexes, can be destroyed while
running in expert mode." 

E_TU0021_Err_destroying
"Error(s) while trying to destroy '%0c'.\n
A database error occurred while trying to destroy '%0c.'
The previous error should have more information."
/*%
** Explanation:
**	If any database error occurs in a destroy of a table, this
**	error will be printed.
**
** System Status:
**	Table is not destroyed.
**
** User Action:
**	Examine previous error message and retry.
*/

F_TU000B_SimpleFields	"SimpleFields"
F_TU000C_SimpleField_expl	"Use simple field layout to display data in the form"
F_TU000D_TableField_expl	"Use a table field to display data in the form"
F_TU000E_Help_expl	"Get help on Query-By-Forms"
F_TU000F_End_expl	"Return to table information"
F_TU0010_BlockMode	"Block"
F_TU0011_BlockMode_expl	"Display titles and data together"
F_TU0012_ColumnMode	"Tabular"
F_TU0013_ColumnMode_expl	"Display in columns across the page"
F_TU0014_WrapMode	"Wrap"
F_TU0015_WrapMode_expl	"Tabular report; wrap to next line if needed"
F_TU0018_Help_expl	"Get help on running reports"
F_TU0019_End_expl	"Return to table information"

/*
**	prscreate.qc
*/

E_TU0023_Out_of_space	
"Not enough memory space to create table.\n
An internal buffer used to hold the create for the table has
overflowed on column '%0c'.  Reduce the number of columns, or 
the lengths of the column names.  Please report this problem to your
Site Technical Contact."
/*%
** Explanation:
**	The size of an internal buffer overflowed.
**
** System Status:
**	Table will not be created.
**
** User Action:
**	Reduce the number of columns, or size of the names of
**	the columns.  Report this error to your Site Technical Contact.
*/

E_TU0024_Ignoring_blank_column
"Ignoring blank column descriptor in row '%0d'.\n
A blank row was found in the list of columns.  It will be ignored."
/*%
** System Status:
**	Create will proceed.
**
** User Action:
**	None.
*/

E_TU0025_Blank_name_on_row
"Blank name found on row '%0d'.\n
A blank name for a column was found in the list of columns.
You must enter a valid name."
/*%
** System Status:
**	Create will not proceed.
**
** User Action:
**	Fill in the name of the blank column and try the Save again.
*/

E_TU0026_Duplicate_column_name
"Duplicate column name '%0c'.\n
A duplicate column name of '%0c' was found on row '%1d'.
You must have unique column names within a table."
/*%
** System Status:
**	Create will not proceed.
**
** User Action:
**	Change one of the column names.
*/

E_TU0027_Illegal_column_name
"Illegal name for column, '%0c'."
/*%
** System Status:
**	Create will not proceed.
**
** User Action:
**	Correct column name and try Save again.
*/

E_TU0028_Bad_format_for_row
"Bad datatype '%0c' specified for column '%1c'.\n
A bad datatype name of '%0c' was found for
column '%1c' on row '%2d'.  Specify a valid INGRES
datatype name and length."
/*%
** System Status:
**	Create will not proceed.
**
** User Action:
**	Correct offending datatype format and try Save again.
*/

E_TU0029_Bad_key_number_on_row
"Bad key number '%0c' specified for column '%1c'.\n
A bad number for the key ('%0c') was specified for
column '%1c' on row '%2d'.  Specify a valid number to
represent the position of the column within the key,
numbered from 1."
/*%
** Explanation:
**	A conversion of the key number to a digit failed.  
**
** System Status:
**	Create will not proceed.
**
** User Action:
**	Correct offending key number and retry.
*/

E_TU002A_Duplicate_key_for_row
"Duplicate key '%0c' specified for column '%1c'.\n
A duplicate value for the key number was specified for column '%1c' on row
'%2d'.  Only one column may be assigned a specific key number, which should
be ordered beginning from 1."

/*%
** Explanation:
**	Duplicate key value found.
**
** System Status:
**	Create will not proceed.
**
** User Action:
**	Correct offending key number and retry.
*/

E_TU002B_Too_many_keyd_columns
"Can't use this many keyed columns.\n
You have specified too many keyed columns for the create command.
Reduce the number of columns that have keys.
Report this problem to your Site Technical Contact."
/*%
** Explanation:
**	An internal buffer for the number of keys has overflowed.
**
** System Status:
**	Create will not proceed.
**
** User Action:
**	Reduce number of keyed columns and report to your
**	Site Technical Contact.
*/

E_TU002C_Table_is_too_wide
"Table is too wide.\n
This table is too wide to create.  Maximum width for an INGRES
table is 2000 bytes.  Reduce the sizes of some of the columns
to create the table."
/*%
** Explanation:
**	The sum of the number of bytes for the columns in this
**	table would be bigger than the INGRES maximum of
**	2000.
**
** System Status:
**	Create will not proceed.
**
** User Action:
**	Reduce the sizes of some of the columns until the width
**	is less than 2000 bytes and try again.
*/

E_TU002D_Too_many_attributes
"Too many columns specified for a table.\n
This table has too many columns to create.  The maximum number
of columns for an INGRES table is 127.  Reduce the number of
columns to create the table."
/*%
** Explanation:
**	The number of columns in an INGRES table is limited to 127.
**
** System Status:
**	Create will not proceed.
**
** User Action:
**	Reduce the number of columns and try again.
*/

E_TU002E_No_attributes_for_tbl
"No columns specified - cannot create table.\n
You must specify some columns in the table field to create a table."

/*%
** System Status:
**	Create will not proceed.
**
** User Action:
**	Fill in at least one row in the table field and then retry.
*/

E_TU002F_ColIsSQLKeyWord
"Column '%0c' is an SQL keyword.\nYou must rename this column.\n"
/*
** Explanation:
**	The user has specified a column name which is an SQL keyword.
**
** System Status:
**	Inform the user and remain on the "Create a Table" screen.
**
** User Action:
**	The user has to rename the column.
*/


/*
**	creattbl.qsc
*/
E_TU0030_TabIsSQLKeyWord
"Table '%0c' is an SQL keyword.\nYou must rename this table.\n"
/*
** Explanation:
**	The user has specified a table name which is an SQL keyword.
**
** System Status:
**	Inform the user and remain on the "Create a Table" screen.
**
** User Action:
**	The user has to rename the table.
*/


/*
**	report.qc
*/
F_TU0038_Enter_file_name	"Enter file name (terminal is default): "
F_TU0039_Start_REPORT_WRITER	"Starting REPORT WRITER . . ."
/*
**	tuunique.c
*/

E_TU0033_tunmadd_bad_mem_alloc	"Cannot allocate memory.\n
Cannot allocate memory for the tables utility."

/*%
** Explanation:
**	Run out of memory in tables utility.
**
** System Status:
**	Program aborts.
**
** User Action:
**	Restart program.
*/

/*
**	util.qc
*/
F_TU001A_DBA	"DBA"

E_TU0034_Error_in_FErelopen	"Error accessing table information.\n
An error occurred in getting table information from 'FErel_open().'  The return
status is '%0x'.  Please report this problem to your Site Technical Contact."

/*%
** Explanation:
**	A call to FErel_open has returned an error status of some
**	sort.  This is an internal error.
**
** System Status:
**	Action will not be performed.
**
** User Action:
**	Report to your Site Technical Contact.
*/

E_TU0035_Blankname_not_allowed	"Blank name not allowed.\n
You must enter a valid name for a table or column."

/*%
** Explanation:
**	A blank name is not a valid name.
**
** System Status:
**	Action will not be performed until valid name is entered.
**
** User Action:
**	User should enter a valid name.
*/

E_TU0036_must_begin_with_alpha	
"'%0c' is not a valid name.\n
Valid INGRES names always begin with a letter (a-z or A-Z) or
an underscore character.  Valid Open SQL names must always begin
with a letter (a-z or A-Z)."
/*%
** Explanation:
**	NO LONGER USED.
**
** System Status:
**	Action will not be performed until valid name is entered.
**
** User Action:
**	User should enter a valid name.
*/

E_TU0037_Name_contain_bad_char	
"Name '%0c' is not a legal table name.\n
The entered name '%0c' is either too long, not properly delimited
with enclosing double quotes, or contains illegal characters."
/*%
** Explanation:
**	Valid table name is required.
**
** System Status:
**	Action will not be performed until valid table name is entered.
**
** User Action:
**	User should enter a valid table name.
*/

E_TU0009_Owner_name_illegal
"Name '%0c' is not a legal owner name.\n
The entered name '%0c' is either too long or not properly delimited
with enclosing double quotes."
/*%
** Explanation:
**	Valid owner name is required.
**
** System Status:
**	Action will not be performed until valid owner name is entered.
**
** User Action:
**	User should enter a valid owner name.
*/

F_TU001B_SYSTEM_CATALOG	"system catalog"
F_TU001C_VIEW	"view"
F_TU001D_INDEX	"index"
F_TU001E_USER_RELATION	"user table"
F_TU002E_TABLE		"table"

F_TU001F_Unknown	"unknown"
F_TU0020_unique	" unique"
/*
**	This prints out "cbtree" or "chash" if its compressed.  Use "c%s"
**	or whatever character is intended to indicate "compressed."
*/
F_TU0021_compchar_storage_str	"c%s"
F_TU0022_Enabled	"enabled"
F_TU0023_Disabled	"disabled"
F_TU0024_At_Next_Chkpt	"at next chkpt"
F_TU0025_Not_Applicable	"not applicable"

E_TU003B_Name_too_long	
"Name '%0c' is too long for a table name.\n
The maximum table name size is 24 characters for INGRES."
/*%
** Explanation:
**	This error is NO LONGER USED and 24 char limit is now wrong anyway
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Re-enter table name and retry.
*/

E_TU003D_No_tables_in_catalog	
"No tables in catalog.\n
Positional menu operations are not allowed unless the cursor
is pointing to a valid name.  Since there are no tables in the
catalog, the operation is not allowed."
/*%
** Explanation:
**	This error is given if the catalog is empty and someone
**	tries a positional operation.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Use Create option.
*/

E_TU003E_Name_not_specified
"Cursor on empty row.\n
The cursor must be positioned on a row in the table
list that contains an entry.  If you are using the -e
option, you must give a string to match.  If you want
all tables, use the '*' pattern match character."
/*%
** Explanation:
**	Occurs when an operation is attempted when the cursor
**	is positioned on an empty entry.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Try again with cursor positioned correctly.
*/

E_TU003F_No_tables_found	"No matching tables found.\n
No tables were found that matched the pattern you specified."

/*%
** Explanation:
**	This occurs when the -e flag is used and a pattern is given that does
**	not match any of the table names in the database.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Try again with cursor positioned correctly.
*/

E_TU0040_Table_not_owned_by_yo	"This table is not owned by you.\n
You cannot destroy a table that you do not own."

/*%
** Explanation:
**	Cannot destroy or change a table not owned by you.
**
** System Status:
**	Request is ignored.
**
** User Action:
**	Destroy table when run as owner of table.
*/

/*
**	examine.qc
*/
/*
**	New Table menu item.
*/
F_TU0026_NewTable	"NewTable"
/*
**	Edit and View Default information menu items.
*/
F_TU0017_EditDefaults	"EditDefaults"
F_TU006F_ViewDefaults	"ViewDefaults"
/*
**	menuitem explanations.
*/
F_TU004B_ExplNewTable	"Examine another table"
F_TU004C_ExplSave	"Create the table and return to previous screen"
F_TU004D_ExplGetTblDef	"Include the definition of another table"
F_TU004E_ExplInsert 	"Open a new table field row"
F_TU004F_ExplDelete 	"Delete the current table field row"
F_TU0050_ExplBlank 	"Clear every field on the form"
F_TU0051_ExplMove 	"Move the current table field row"
F_TU0052_ExplPlace 	"Place the Moved row above the current row"
F_TU0053_ExplCreate 	"Create a new table"
F_TU0054_ExplDestroy 	"Drop the current table"
F_TU0055_ExplExamine	"Examine the definition of the current table"
F_TU0056_ExplQuery	"Query data in current table using QBF (Query-By-Forms)"
F_TU0057_ExplReport	"Run Report on data in current table"
F_TU0058_ExplQuit	"Exit from the Tables Catalog"
F_TU0059_ExplGetOk	"Examine/Get this table definition"
F_TU0070_ExplEdtDflt	"Edit default value for current column"
F_TU0071_ExplViewDflt	"View default value for current column"

/*
** The datatypes that appear in the ListChoices popup in Create-a-Table screen.
** The "\n" indicates to ListPick, the end of each entry.
*/
F_TU005A_Varchar	"varchar(length)"
F_TU005B_Char		"char(length)"
F_TU005C_Integer	"integer"
F_TU005D_Smallint	"smallint"
F_TU0067_Integer1	"integer1"
F_TU005E_Float		"float"
F_TU0068_Float4		"float4"
F_TU005F_Date		"date"
F_TU0009_Decimal	"decimal(precision,scale)"
/* Ingres-only data types */
F_TU0060_Money		"money"
F_TU0061_TblKeySysMnt	"table_key with system_maintained"
F_TU0062_TblKeyNoSysMnt	"table_key not system_maintained"
F_TU0065_ObjKeySysMnt	"object_key with system_maintained"
F_TU0066_ObjKeyNoSysMnt	"object_key not system_maintained"
F_TU0086_Byte		"byte(length)"
F_TU0087_ByteVarying	"byte varying(length)"
F_TU0088_LongByte	"long byte"
F_TU0089_LongVarchar	"long varchar"

/* Title for popup frame */
F_TU0063_DtypeTitle	"Select one of the items below.\n\
Then replace 'length' with a number."

/* Title for Help screen */
F_TU0064_HlpTitle	"Data type Selection"

/*
** Error message if IIUFGetform fails
*/
E_TU0041_Get_Form_Failed	"Unable to read form from formindex file.\n
Tables Utility got an error trying to read form '%0c' from your formindex file.
Check the setting of the environment variables II_FORMFILE, II_CONFIG and
II_LANGUAGE. Make sure you have read access to all files in the II_LANGUAGE
subdirectory of II_CONFIG, which have a file extension of 'fnx' (formindex
files have an extension 'fnx'). If a 'fnx' file exists, and you have read
access to it, then report this problem to your Site Technical Contact."

E_TU0042_print_errbuf	"%0c"

/*
**	Help File Title Strings.
*/
F_TU0027_Move_Column_Submenu	"Move Column Submenu"
F_TU0028_New_Table		"Creating a New Table"
F_TU0029_Table_Information	"Examine Table Information"
F_TU002A_QBF			"Table Utility - QBF"
F_TU002B_Report			"Table Utility - Report"
F_TU002C_Table_Utility		"Table Utility"

F_TU003A_Loading_Tables		"Loading table list . . ."
	/* Status message when loading the table form */
E_TU0044_xConn_X_ERR	"Cannot create or destroy table at this time.\n
Could not claim exclusive database connection.  Try again when
other users are not updating the database."

E_TU0045_xConn_FTL_ERR    "Database connection lost."

E_TU0046_keys_req    "Table not created:  Key must be specified.\n
At least one key must be specified to create this table."

/*
** strings used to build screen titles
*/
F_TU002F_Tables			"TABLES"
F_TU002D_Table_Catalog		" - Tables Catalog"
F_TU0030_Examine		" - Examine a Table"
F_TU0031_Create			" - Create a Table"

E_TU004D_Error_in_IIUIrel_help	"Error accessing table information.\n
An error occurred selecting table information in routine 'IIUIrel_help().'
The Query return status is '%0x'.  Please report this problem to your
Site Technical Contact."
S_TU004E_Whitespace_removed	"Found & removed white space in column.\n
Found and removed white space in column '%0c'. The table will be created
with the name of this column changed to '%1c' (white space removed)."

/*
**	tutblpik.qsc
*/
/*	List Pick menuitem name & explanations. */
F_TU0042_Details	"Details"
F_TU0043_Details_Expl	"Show detailed information about a table"
F_TU0044_Create		"Create"
F_TU0045_Create_Expl	"Create a new table"

/*	Title for List Pick Help Screen */
F_TU0047_LP_Help_Title	"Table Name Lookup"

/*	List Pick popup form, table field title */
F_TU0048_LP_Tblfld_Title	"Table Name"

S_TU004F_No_Tables_Found "No tables matching '%0c' were found.
All table names will be loaded."

/*%
** Explanation:
**      Informative message. No tables were found with a name that
**	matches that entered by the user (name entered by user may 
**	contain pattern match characters).
**
** System Status:
**      Same as if ListChoices was never selected.
**
** User Action:
**      Enter a different table name and select ListChoices again.
*/

/* following are used in confirmation popup for tables listpick when
** attempting to look at available tables when none exist. user is
** asked if they want to create a new table.
*/
S_TU0050_EmptyDB_Prompt
"Empty database. There are no tables\n
available to you in this database.\n
Would you like to create a table?"

S_TU0054_HelpSubject "Confirm if want to Create a New Table"
S_TU0055_YesText "Create a new table"
S_TU0056_NoText "Do not create a table"

S_TU0059_ResetLogKey "Logical key has wrong NULL/Default settings\n
Warning: Column '%0c' is a Logical Key of type '%1c'.
When '%1c' is specified for a Logical Key, you must also specify
Nulls/Defaults of 'Nulls: no, Defaults: yes' (which is equivalent to the
SQL: '%3c'), however this
column specified '%2c'.\n
Table creation will continue, but the Nulls/Defaults information for
this column will be changed to '%3c'."

S_TU0052_EmptyDB_Msg "Empty database. There are no tables available to you
in this database."

E_TU0053_No_Lookup	"No '%0c' operation on the current field\n
The '%0c' operation is not supported on the current field. You must
be on the table field column 'Data Type' before the '%0c' operation
is active. When invoked on the 'Data Type' column, this operation
will bring up a list of data types for you to choose from."

/*%
** Explanation:
**
** System Status:
**      Disallow the operation.
**
** User Action:
**      Continue entering the table definition.
*/

/*	Title for "Enter Table Name:" Help Screen */
F_TU004A_Help_Table_Prompt	"Enter a Table Name"

E_TU0051_No_Table_Entered	"You must enter a Table Name\n
You must enter a Table Name before selecting menuitem '%0c'. If you wish to
leave this screen without entering a table name, then select menuitem '%1c'."

/*%
** Explanation:
**	User selected menuitem "OK" but there was no table name entered.
**	A table name must be entered before selecting "OK".
**
** System Status:
**	Same as it was before "OK" was selected.
**
** User Action:
**	Enter a table name before selecting "OK". To leave without having
**	to enter a table name, select "Cancel".
*/

E_TU0057_TooManyDeadlockTries	"Giving up after multiple deadlock retries\n
A deadlock occurred %0d times while trying to select information about
table '%1c'. Information on table '%1c' not selected."

S_TU0058_DoingDeadlockRetry	"Deadlock occurred. Retrying . . ."

E_TU0008_No_Default_Entered	"You must enter a Default Value\n
You must enter a Default Value before selecting menuitem '%0c'. If you
wish to leave this screen without entering a Default Value, then select
menuitem '%1c'."

/*%
** Explanation:
**	User selected menuitem "OK" but there was no table name entered.
**	A Default Value must be entered before selecting "OK".
**
** System Status:
**	Same as it was before "OK" was selected.
**
** User Action:
**	Enter a Default Value before selecting "OK". To leave without having
**	to enter a Default Value, select "Cancel".
*/


/*
**	Here are some fast strings for new (6.5) default handling.
**	Tried to order the "value", "user", and "null" synonyms for
**	optimal search speed.
*/

F_TU0072_Help_Default_Prompt	"Enter a Default Value"
F_TU0073_Value1		"v"
F_TU0074_Value2		"value"
F_TU0075_Value3		"va"
F_TU0076_Value4		"val"
F_TU0077_Value5		"valu"
F_TU0078_User1		"u"
F_TU0079_User2		"user"
F_TU007A_User3		"us"
F_TU007B_User4		"use"
F_TU007C_Null1		"nu"
F_TU007D_Null2		"null"
F_TU007E_Null3		"nul"

F_TU007F_Select_Defaultability	"Select this column's Defaultability from the list below"

F_TU0080_No_Desc	"No Default"
F_TU0081_Yes_Desc	"Ingres System Defined Default"
F_TU0082_User_Desc	"Runtime User's User ID is the Default"
F_TU0083_Null_Desc	"Default value is NULL"
F_TU0084_Value_Desc	"You will be prompted for the Default Value"

F_TU0085_Default_Val	"Default Values"

E_TU000A_NA_Defaults	"User defined defaults are not available\n
User defined defaults are not available with pre-6.5 releases or Gateways."

/*%
** Explanation:
**	Only values of 'yes' or 'no' are allowed in QUEL
**
** System Status:
**	Defaultability setting reverts back to what it was before.
**
** User Action:
**	User should select either 'yes' or 'no'
*/

E_TU000B_Null_Default	"Can't have null default if column isn't Nullable\n
Column must be nullable if the default value is to be null. You must put
a value of 'yes' in the 'Nulls' field for this column if you want the default
value to be null."
/*%
** Explanation:
**	Can't have default value of NULL for this column if column isn't
**	nullable.
**
** System Status:
**	Defaultability setting reverts back to what it was before.
**
** User Action:
**	User should go back to the 'Nulls' field and change it's value to
**	'yes' before changing 'Defaults' field to 'null'.
*/

E_TU000C_Not_User_Def	"Column '%0c' has no User Defined Default value\n
Column '%0c' has defaultability '%1c'. If it had a User Defined Default,
the defaultability would be listed as 'value'."
/*%
** Explanation:
**	User selected 'ViewDefaults' while on a column that doesn't have
**	User Defined Defaults. 'ViewDefaults' is only a valid choice for
**	a column when the defaultability for that column is listed as 'value'.
**
** System Status:
**	No change.
**
** User Action:
**	Only select 'ViewDefaults' for columns whose defaultability is listed
**	as 'value', not 'null', 'user', 'yes', or 'no'.
*/

E_TU001A_NotNull_NullDefault
"Row '%0d' has illegal 'Nulls no', 'Defaults null'\n
Column must be nullable if the default value is to be null. You must put
a value of 'yes' in the 'Nulls' field for this column if you want the default
value to be null."

/*%
** Explanation:
**	Can't have default value of NULL for this column if column isn't
**	nullable.  Include column number, since this might occur repeatedly
**	(several violations on the table field)
*/

E_TU0012_NADataType
"Column '%0c' has illegal datatype, '%1c'\n
This datatype is illegal in the Tables Utility.  You must use another tool
(for example 'INGRES Terminal Monitor') to create tables containing columns
with this datatype."

/*%
** Explanation:
**	User attempted to create an unbounded datatype column in tables
**	utility, which, according to the specifications, should not be allowed.
*/

E_TU000F_Bad_Defaultability
"Illegal defaultability indicator, '%0c'\n
Legal defaultabilities are 'user' for current user name, 'null' for
a default of NULL, 'yes' for INGRES default, 'no' for no default, or
'value' for any value you care to assign. If you want the default to
be '%0c' then you should enter 'value' here and you will be prompted
to enter the value '%0c'."
/*%
** Explanation:
**	User has entered a defaultability that is neither 'value', 'null',
**	'user', 'yes', or 'no' and thus is illegal.
**
** System Status:
**	No change.
**
** User Action:
**	User apparently meant to enter 'value' for the defaultability
**	and should do so. User will be prompted for the desired value.
*/
