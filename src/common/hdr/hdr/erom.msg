/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erom.msg	- Error file for OMCORE.
**
** Description:
**	This is the error definition file for OMCORE.
**
** History:
**	23-may-1989 (Joe)
**	     First Written
**	27-aug-1990 (Joe)
**	    Added E_OM0036_NODBVRET and E_OM0037_PRMNOTBYREF.
**	23-jan-1991 (Joe)
**	    Added errors for DbSessionObjects.
**	11/20/91 (dkh) - ER changes for internationalization.
**	21-oct-1992 (emerson)
**	    Added S_OM0091_USER1 through S_OM009A_USER10.
**	30-oct-1992 (pete)
**	    Added messages for preprocessor related to #INCLUDE stmt.
**	11/92 (bobm)
**	    Added E_OM009E through E_OM00AC.
**	8-jan-1993 (pete)
**	    Added E_OM00AD.
**	1/93 (bobm)
**	    Added E_OM00AE
**	9-feb-1993 (pete)
**	    Removed OM0086 (new message for Amethyst).
**	3/93 (bobm)
**	    Add 00B1 - B6.
**	28-feb-94 (davel)
**	    Added E_OM003B_CS_ISOPEN and E_OM003C_CS_IS_CACHED.
**	7/93 (bobm)
**	    Add BF
**	29-jul-93 (mevans)
**	    Add E_OM00C0_NOSUPERSEDE and E_OM00C1_NOTSAMETYPE
**	30-jul-93 (milovan)
**	    Added E_OM00C2_* through E_OM00D3_*
**	7/93 (bobm)
**	    Add D4, D5
**	9/93 (bobm)
**	    Add D6
**	10/93 (bobm)
**	    Add D7
**	28-feb-94 (davel)
**	    Added E_OM003B_CS_ISOPEN and E_OM003C_CS_IS_CACHED.
**	13-mar-1994 (emerson)
**	    Added E_OM00DA.
**	17-mar-1994 (blaise)
**	    Added 0071-0079. Each of these replaces an existing preprocessor
**	    error message, but doesn't have line number, script type or
**	    script name parameters.
**	06-dec-95 (sandyd)
**	    Reserved S_OM00DB slot for kitch01 Level 2 Support fix.
**	18-dec-95 (pab)
**	    Added E_OM00DC.
**	26-mar-97 (chan)
**		Added E_OM00DD_UNKNOWN_EXTCLASS
**	01-may-2000 (rucmi01)
**	    Added E_OM00DE_DUPLICATE_STKOVFLOW, E_OM00DF_USEROBJFREE_STKOVFLOW.
**	08-jul-2001 (dansa02)
**	    Added E_OM00E0_DUPLICATE_STKOVFLOW, E_OM00E1_USEROBJFREE_STKOVFLOW.
**	03-mar-2005 (dupbr01)
**		Fix a typo in message E_OM006A
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	01-may-2008 (zhayu01) SIR 120626
**	    Added E_OM00E2_SOBADFILE_INVALIDUTF8.
**	    Added E_OM00E3_OPENERR_UTF8.
**      11-jun-2010 (berbo01) SIR 44794
**	    Added E_OM00E2_KEY_NOT_FOUND, E_OM00E3_DUPLICATE_KEY,
**	    E_OM00E4_INVALID_PARAM_VALUE, E_OM00E5_PRECOND_VIOLATION, E_OM00E6_INTERNAL_ERROR
**	12-jul-2010 (zhayu01) SIR 44794
**		Resolve message ID conflict between previous two changes.
**/

# define	_OM_CLASS	189

E_OM0001_BUFFER_TOO_SMALL	"Error buffer too small.\n
The error buffer is too small to hold the error given.  This is an
internal error and should be reported to your INGRES technical support
representative."

/*%
** Explanation:
**	A  buffer passed to an error processing routine was too small for
**	the error being returned.  Normally, this error is not shown to
**	the user.  If it is, then there is an error in the INGRES code
**	that needs to be reported.
**
** System Status:
**	The continuation of the application is uncertain at this point.
**	Different applications will deal with it differently.
**
** User Action:
**	Report this to you INGRES technical representative.
*/

E_OM0002_EMPTY			"Error stack is empty for pop.\n
This is an internal error and should be reported to your INGRES technical
support representative."

/*%
** Explanation:
**	An operation was applied to an internal structure known as the error
**	stack.  The structure is not in the correct state for the operation
**	being applied.  This represents a coding error in the INGRES code.
**
** System Status:
**	The continuation of the application is uncertain at this point.
**	Different applications will deal with it differently.
**
** User Action:
**	Report this to you INGRES technical representative.
*/

E_OM0003_OUTOFRANGE	"Row out of range for an InsertRow.\n
An InsertRow was done on a row which was outside the legal range of
the array.  The value for row for an InsertRow must be between 1 and
the LastRow + 1."

/*%
** Explanation:
**	An InsertRow method was applied to an array giving a value for
**	row that was outside the legal range for the array.
**
** System Status:
**	No row will be inserted.
**
** User Action:
**	Change the invocation of insertrow to use a legal value for row.
*/

E_OM0004_ROWNOTFOUND	"Row not found for a RemoveRow or SetRowDeleted.\n
A RemoveRow or SetRowDeleted method was applied to an array, and the value
given for the row to remove was not within the array."

/*%
** Explanation:
**	A bad row number was given to a RemoveRow or SetRowDeleted
**	method.  The row number must be within the range of the table
**	field.
**
** System Status:
**	No row will be removed or set to deleted.
**
** User Action:
**	Change the invocation of RemoveRow or SetRowDeleted so that it
**	uses a legal value for row.
*/

E_OM0005_NOPARAMS   "Parameters passed to object that doesn't take parameters.\n
Some parameters were passed to an object that does not have any parameters."

/*%
** Explanation:
**	You passed parameters to an object like a procedure or a method
**	that does not take any parameters.
**
** System Status:
**	The exact action depends on the object that was called.  Most
**	will simply continue executing after reporting the error.
**
** User Action:
**	Remove the parameter list from the code that calls this object.
*/

E_OM0006_TOOMANY	"Too many parameters passed.\n
A call was made on an object and too many positional parameters were passed
to the object.  The object only takes %0d parameters yet you passed at least
%1d parameters."

/*%
**
** Arguments:
**	0	This is the number of parameters that the called object
**		accepts.
**
**	1	This is the number of the positional parameter that
**		was passed that exceeds the number.
**
** Explanation:
**      The parameter list for the call of an object included too many
**	positional parameters.
**
** System Status:
**	The exact action depends on the object that was called.  Most
**	will simply continue executing by ignoring this parameter.
**
** User Action:
**	Remove the extra parameters from the parameter list and
**	check the documentation to find the correct number of parameters.
*/

E_OM0007_PRMNOTFOUND	"Unknown parameter '%0c'.\n
The name '%0c' was passed as a parameter to an object.  The called object does
not have a parameter with that name."

/*%
**
** Arguments:
**	0	This is the name that was passed to the routine and
**		that is not a parameter name.
**
** Explanation:
**	A named parameter was passed to an object, but the object does
**	not have a parameter with that name.
**
** System Status:
**	The exact action depends on the object that was called.  Most
**	will simply continue executing by ignoring this parameter.
**
** User Action:
**	Remove the named parameter from the parameter list of this
**	call, and check the documentation to find the correct parameter
**	name.
*/

E_OM0008_PASSTWICE	"More than one value passed to parameter '%0c'.\n
The parameter '%0c' was passed more than one value.  This can happen if the
parameter is passed both positionally and by name, or if it is listed more
than once in the named parameter list."

/*%
**
** Arguments:
**	0	This is the name that of the parameter that was given
**		more than one value.
**
** Explanation:
**	A parameter was passed more than one value: it appeared more
**	than once in the parameter list.  This can happen if the parameter
**	was passed positionally and by name, or it its name appears more
**	than once in the parameter list.
**
** System Status:
**	The exact action depends on the object that was called.  Most
**	will simply continue executing by ignoring this extra value for
**	the parameter.
**
** User Action:
**	Remove the extra value passed to this parameter.
*/

E_OM0009_MANDPRM	"Mandatory parameter '%0c' was not passed a value.\n
The mandatory parameter '%0c' was not passed a value in a call.  This parameter
must be given a value for the call to succeed."

/*%
**
** Arguments:
**	0	This is the name that of the mandatory parameter that was 
**		not passed a value.
**
** Explanation:
**	Some parameters are mandatory which means that they must be
**	passed a value whenever the object is called.  In this call,
**	a particular mandatory parameter was not given a value.
**
** System Status:
**	Since the parameter is required for correct operation of the
**	object, execution of the object will stop.
**
** User Action:
**	Pass a value for this mandatory parameter.
*/

E_OM000A_PRMNOTCHAR	"Non-character value passed to character parameter.\n
A non-character value was passed to the parameter '%0c' which expects a
character typed value."

/*%
**
** Arguments:
**	0	The name of the parameter that was passed a bad value.
**
** Explanation:
**	A parameter whose type is a character type was passed a non
**	character value.
**
** System Status:
**	The exact action will depend on whether the parameter is
**	needed.  Normally a default value will be used.
**
** User Action:
**	Pass a character typed value to this parameter.
*/

E_OM000B_BADOBJTYPE	"A non-object typed value passed to an object.\n
A value whose type is not a class was passed to a parameter that is
an object."

/*%
** Explanation:
**	A parameter whose type is some object type was passed a non object
**	typed value.
**
** System Status:
**	The exact action will depend on whether the parameter is
**	needed.  Normally a default value will be used.
**
** User Action:
**	Pass an object to this parameter.
*/

E_OM000C_CS_NOT_OPENED	"Attempt to perform operation on an unopened cursor\n.
An attempt was made to perform the %0c operation on an unopened cursor."

/*%
** Parameters:
**	0	This is the name of the operation that was being
**		performed.  It will be one of:
**			FETCH
**			CLOSE
**			UPDATE
**			DELETE
**
** Explanation:
**	A operation executed on a cursor that has not been opened.
**
** System Status:
**	The system will continue running, but the operation will
**	not be executed.
**
** User Action:
**	Open the cursor before attempting this operation.
*/

E_OM000D_CS_NOCURRENT	"The cursor is not currently positioned on a row.\n
An attempt was made to perform a %0c operation on a cursor that is
not currently positioned on a row.  A fetch must be done before the
%0c operation can be done."

/*%
** Parameters:
**	0	This is the name of the operation that was being
**		performed.  It will be one of:
**			UPDATE
**			DELETE
**
** Explanation:
**	An UPDATE or DELETE operation was performed on a cursor is not
**	currently positioned on a row.  These operations are only
**	legal when the cursor is positioned on a row after a fetch
**	operation.
**
** System Status:
**	The system will continue running, but the operation will
**	not be executed.
**
** User Action:
**	Fetch a row from the cursor before attempting this operation,
**	or check the State of the cursor and make sure it is
**	CS_CURRENT before attempting this operation.
*/

E_OM000E_CS_NODESC	"Couldn't get the descriptor for a cursor.\n
There was an error reading the descriptor for the cursor opened in
%0c at line %1d.  This is an internal error and should be reported to
you INGRES representative.  The program will continue executing, but
values cannot be fetched by name from this cursor."

/*%
** Parameters:
**	0	This is the name of the component that issued the
**		open statement for this cursor.
**
**	1	This is the line number of the open statement within
**		that component.
**
** Explanation:
**	When a value is fetched by name from a cursor, the system
**	must get the names of the columns generated by the select.
**	The system tried to get the names but got an internal error
**	that prevents it from getting the names.
**
** System Status:
**	The system will continue running, but fetch by name will not
**	be allowed on this cursor.
**
** User Action:
**	Report this problem to your INGRES representative.
*/

E_OM000F_CS_GETNAME	"Couldn't get column name for a cursor.\n
There was an error reading the column name for column %2d for the cursor
opened in %0c at line %1d.  This is an internal error and should be reported to
you INGRES representative.  The program will continue executing, but
values cannot be fetched by name from this cursor."

/*%
** Parameters:
**	0	This is the name of the component that issued the
**		open statement for this cursor.
**
**	1	This is the line number of the open statement within
**		that component.
**
**	2	This is the number of the column whose name was being
**		gotten.
**
** Explanation:
**	When a value is fetched by name from a cursor, the system
**	must get the names of the columns generated by the select.
**	The system tried to get the name of a particular column,
**	but got an internal error that prevents it from getting the name.
**
** System Status:
**	The system will continue running, but fetch by name will not
**	be allowed on this cursor.
**
** User Action:
**	Report this problem to your INGRES representative.
*/

E_OM0010_CS_LONGNAME	"A column named used in a fetch is too long.\n
The column name '%0c' was used in a fetch statement.  This name is too
long and a shorter name must be used.  The limit on names is %1d characters."

/*%
** Parameters:
**	0	This is the column name that was too long.
**
**	1	This is maximum number of characters a name can be.
**
** Explanation:
**	A column name used in a fetch statement in which values are being
**	fetched by name, was too long.
**
** System Status:
**	The system will continue, but this column can not be fetched
**	by name.
**
** User Action:
**	Shorten the name of the column.  One way to do this is by
**	using the 'value AS name' syntax in the target list for the
**	select statement given in the cursor open.
*/

E_OM0011_CS_BADNAME	"Column %2c not found for fetch.\n
The column name %2c was used in a fetch statement on the cursor
opened in %0c at line %1d.  This column name is not one of the columns
selected by this cursor.  Check the spelling of the column name in
the fetch statement."

/*%
** Parameters:
**	0	This is the name of the component that issued the
**		open statement for this cursor.
**
**	1	This is the line number of the open statement within
**		that component.
**
**	2	This is the column name that was being fetched.
**
** Explanation:
**	A fetch statement included a column name that is not known
**	for the cursor on which the fetch was executed.
**
** System Status:
**	The system will continue execution, but this column will be
**	ignored.
**
** User Action:
**	Correct the column name.  Check the name of the column against
**	the names given in the target list in the OPEN statement for
**	this cursor.
*/

E_OM0012_CS_FETCHTWICE	"The column %0c was fetched twice.\n
The column name %0c has been fetched twice within the same cursor
statement.  The column can only be listed once."

/*%
** Parameters:
**	0	This is the name of the column that was listed twice
**		in the fetch statement.
**
** Explanation:
**	A fetch statement listed a column twice.  A particular column
**	can only be listed once.
**
** System Status:
**	The system will continue ignoring this second fetch of the column.
**	The variable being fetched into will not be given a value.
**
** User Action:
**	Remove the duplicate column name.
*/

E_OM0013_SO_OWNERNAME	"FIPS naming is not supported for this version.\n
The table name '%0c' was given as the name of a table in which to store
a StringObject.  This name uses the FIPS naming syntax since it includes an
owner. clause. This version of StringObjects does not support FIPS naming.
Remove the owner. syntax from the table name."

/*%
** Parameters:
**	0	The name the user passed to one of the stringobject methods
**		as the name of a table in which to store a string.
**
** Explanation:
**	This version of StringObject does not support the FIPS naming
**	syntax for tables.
**
** System Status:
**	The operation will not complete, and the string will not be
**	added to the database.
**
** User Action:
**	Remove the owner. syntax from the table name given.
*/

E_OM0014_SO_BADTABLE	"Table given for StringObject does not exist.\n
The table '%0c' was given as the name of a table to use to store a StringObject.
However, this table does not exist.  Please create the table, or correct the
code to refer to an existing table that is configured appropriately for
storing StringObjects."

/*%
** Parameters:
**	0	The name the user passed to one of the stringobject methods
**		as the name of a table in which to store a string.
**
** Explanation:
**	A table given to a StringObject routine doesn't exist.
**
** System Status:
**	The operation will not complete and the string will not be added
**	to the database.
**
** User Action:
**	Create the table or correct the code.
*/

E_OM0015_SO_UPDATE  "Error while updating maximum id for StringObject table.\n
There was a DBMS error while updating the maximum id value for the
StringObject table '%0c'.  A subsequent error should give the DBMS error
message.  Please correct this problem."

/*%
** Explanation:
**	The system stores the maximum id for a StringObject in a special
**	row within the StringObject table.   When adding a new StringObject
**	it updates this row.  In doing this, a database error was encountered
**	so the entry could not be updated.
**
** System Status:
**	The operation will not complete.
**
** User Action:
**	Correct the problem causing the database error, and if this
**	can't be rectified, report the problem to your INGRES representative.
*/

E_OM0016_SO_ID_FETCH  "Error while fetching maximum id for StringObject table.\n
A database error occurred while fetching the maximum id for the StringObject
table '%0c.  A subsequent error message should give the database error.  Please
correct this problem"

/*%
** Parameters:
**	0	The name of the table being used to store a StringObject.
**
** Explanation:
**	The system stores the maximum id for a StringObject in a special
**	row within the StringObject table.   When adding a new StringObject
**	it fetches this row.  In doing this, a database error was encountered
**	so the value could not be fetched.
**
** System Status:
**	The operation will not complete.
**
** User Action:
**	The subsequent error message should help you resolve the problem.
**	If not, report this to you INGRES representative.
*/

E_OM0017_SO_ID_INSERT "Error while adding maximum id for StringObject table.\n
A database error occurred while inserting the maximum id into the StringObject
table '%0c.  A subsequent error message should give the database error.  Please
correct this problem"

/*%
** Parameters:
**	0	This is the name of the StringObject table.
**
** Explanation:
**	The system stores the maximum id for a StringObject in a special
**	row within the StringObject table.   If this special row is
**	not present, then one is inserted.  In doing this, a database error
**	was encountered so the special row could not be added.
**
** System Status:
**	The operation will not complete.
**
** User Action:
**	Subsequent errors may help you resolve this, but if this can't be
**	rectified, report the problem to your INGRES representative.
*/

E_OM0018_SO_SETHANDLE	"Error while setting DBHandle for StringObject.\n
An error occurred during the setting of a StringObject's DBHandle to
'%0c'.  A subsequent error should give more information about the error."

/*
** Parameters:
**	0	The value for DBHandle that to which the stringobject
**		was being set.
**
** Explanation:
**	An error occurred during the setting of a dbhandle for
**	a StringObject.
**
** System Status:
**	The DBHandle for the string object will be set, but the StringObject's
**	value will be the empty string.
**
** User Action:
**	Try to resolve the problem using the subsequent errors.
*/

E_OM0019_SO_BADHANDLE_COLON
"An invalid dbhandle was given to a StringObject.\n
The invalid dbhandle '%0c' was given to a StringObject.  The
handle does not contain a colon(:)."

/*%
** Parameters:
**	0	This is the dbhandle.
**
** Explanation:
**	A dbhandle has to have a certain syntax.  Part of this includes
**	a colon.  An invalid dbhandle was used in a method of StringObject.
**	The dbhandle does not contain a colon (:).
**
** System Status:
**	The operation will be unsuccessful.
**
** User Action:
**	Provide a correct dbhandle to the method.
*/

E_OM001A_SO_BADHANDLE_ID
"An invalid dbhandle was given to a StringObject.\n
The invalid dbhandle '%0c' was given to a StringObject.  The
handle's id can not be converted to an integer."

/*%
** Parameters:
**	0	This is the dbhandle.
**
** Explanation:
**	An integer that is part of the dbhandle could not be converted.
**
** System Status:
**	The operation will be unsuccessful.
**
** User Action:
**	Provide a correct dbhandle to the method.
*/

E_OM001B_SO_OP_ERROR	"Error while performing '%0c' on a StringObject.\n
An error occurred while performing the method '%0c' on a StringObject.
Subsequent error messages should explain the cause of this error."

/*%
** Parameters:
**	0	The name of the class method that was being invoked.
**		It will be one of:
**			InsertIntoDb
**			UpdateInDb
**			DeleteFromDb
**
** Explanation:
**	Some kind of error occurred during the execution of a StringObject
**	method.  Subsequent error messages should describe the error.
**	This message gives the context in which the error occurred.
**
** System Status:
**	The operation will not complete.
**
** User Action:
**	Correct the cause of the subsequent errors.
*/

E_OM001C_SO_NOHANDLE	"A StringObject does not contain a dbhandle.\n
An operation that requires a dbhandle was applied to a StringObject that
does not have a dbhandle.  You must set the dbhandle before this
operation can be applied."

/*%
** Explanation:
**	See error message.
** 
** System Status:
**	The operation will not complete.
**
** User Action:
**	Give the stringobject a valid dbhandle before applying this operation.
*/

E_OM001D_BADROWCOUNT	"Unexpected row count during operation.\n
A query was executed successfully, but the row count was not what
was expected. A row count of %0d was expected, but the actual row
count was %1d."

/*
** Parameters:
**	0	The expected number of rows that the query should
**		have affected.
**
**	1	The actual number of rows that the query affected.
**
** Explanation:
**	A operation performed a query, and expected the query to
**	affect a certain number of rows.  However, the query affected
**	a different number of rows than expected.
**	A previous error should have given a context in which this
**	occurred.
*
** System Status:
**	The exact behavior depends on the context in which this
**	occurred.
**
** User Action:
**	Sometimes, the previous errors will give you clues as to
**	how to proceed, but normally this must be reported to your
**	INGRES representatives.
*/

E_OM001E_SO_BADINSERT	"An error occurred while storing a StringObject.\n
An error occurred while a StringObject was being stored in the database.
The StringObject was being stored in the table '%0c' with the id '%1d'.
The error occurred during the store of row '%2d'.  Subsequent errors
may help you resolve this problem, but if not, report this to
your INGRES representatives, giving them the values reported above."

/*%
** Parameters
**	0	The name of the table the string object was
**		being stored in.
**
**      1	The id of the StringObject within that table.
**
**	2	The row of the StringObject that was being stored.
**
** Explanation:
**	While a StringObject was being stored in the database, some
**	kind of error occurred.
**
** System Status:
**	The StringObject will not be stored in the database.  Furthermore
**	the old value of the StringObject that may have been stored in
**	the database will be lost.
**
** User Action:
**	Subsequent errors may help in resolving this problem.  If not
**	report this to your INGRES  representative.  In doing this, make
**	sure to give them the exact values reported in the error message.
*/

E_OM001F_SO_NEGSIZE	"Bad size for a row in a string table.\n
A row in the table '%0c' that stores StringObjects has a bad value
for its text_total column.  The value in text_total is '%3d'.
The bad row has row_sequence '%2d' and string_id '%1d'."

/*
** Parameters:
**	0	This is the name of the table that stores the strings.
**
**	1	This is the value of string_id for the bad row.
**
**	2	This is the value of row_sequence for the bad row.
**
**	3	This is the bad size in that row.
**
** Explanation:
**	StringObjects are stored in several rows of a table.  Part
**	of the data in the row is size information about the string.
** 	This is repeated across each row.  For this row the size is
**	bad.
**
** System Status:
**	This string can will not be gotten from the database.
**	
** User Action:
**	Somehow bad data got put into the table.  Make sure no
**	other programs or users are putting data into this table.
**	If not, then report this to your INGRES representatives.
*/

E_OM0020_SO_SMALLSIZE	"Bad size for a row in a string table.\n
A row in the table '%0c' that stores StringObjects has too small a value
for its text_total column.  The value in text_total is '%3d'.,
yet '%4d' characters have already been read for this string.
The bad row has row_sequence '%2d' and string_id '%1d'."

/*
** Parameters:
**	0	This is the name of the table that stores the strings.
**
**	1	This is the value of string_id for the bad row.
**
**	2	This is the value of row_sequence for the bad row.
**
**	3	This is the bad size in that row.
**
**	4	This is the total number of characters that have
**		already been read for the string.
**
** Explanation:
**	StringObjects are stored in several rows of a table.  Part
**	of the data in the row is size information about the string.
** 	This is repeated across each row.  For this row the size is
**	bad.  It is less than the total number of characters
**	read.
**
** System Status:
**	This string can will not be gotten from the database.
**	
** User Action:
**	Somehow bad data got put into the table.  Make sure no
**	other programs or users are putting data into this table.
**	If not, then report this to your INGRES representatives.
*/

E_OM0021_SO_TOTALSIZE	"Bad size for a StringObject in a string table.\n
A row in the table '%0c' that stores StringObjects has an incorrect
value.  The total number of characters read for the StringObject is
not the same as the value in the text_total column.  The value in
text_total is '%2d', yet '%3d' characters were read for this string.
The bad row has string_id '%1d'."

/*
** Parameters:
**	0	This is the name of the table that stores the strings.
**
**	1	This is the value of string_id for the bad row.
**
**	2	This is the bad size in that row.
**
**	3	This is the total number of characters that have
**		been read for the string.
**
** Explanation:
**	StringObjects are stored in several rows of a table.  Part
**	of the data in the row is size information about the string.
** 	This is repeated across each row.  For this row the size is
**	bad.  It is less than the total number of characters
**	read.
**
** System Status:
**	This string can will not be gotten from the database.
**	
** User Action:
**	Somehow bad data got put into the table.  Make sure no
**	other programs or users are putting data into this table.
**	If not, then report this to your INGRES representatives.
*/

E_OM0022_SO_BADFILE	"Error while setting FileHandle for a StringObject.\n
An error occurred while setting the FileHandle of a StringObject to the value
'%0c'.  Check that the file '%0c' exists and can be read by you."

/*%
** Parameters:
**	0	This is the name of the file that the FileHandle was set
**		to.
**
** Explanation:
**	While setting the FileHandle for a StringObject, there was some error
**	in reading the file that was given as the value of the FileHandle.
**
** System Status:
**	The StringObject will be cleared of its old value and its FileHandle
**      will be set to the file name given.
**
** User Action:
**	Make sure the value given to the FileHandle was correctly spelled,
**	that the file exists, and that the file is readable by you.
*/

E_OM0023_SO_WRITEFILE	"Error while executing method WriteToFile.\n
An error occurred while writing a StringObject to the file
'%0c'.  Check that the file '%0c' can be created and written by you."

/*%
** Parameters:
**	0	This is the name of the file being written to.
**
** Explanation:
**	While executing the method WriteToFile for a StringObject an error
**	occurred.
**
** System Status:
**	The StringObject will not be written to a file.
**
** User Action:
**	Make sure the file given can be written by you.
*/

E_OM0024_SO_APPENDFILE	"Error while executing method AppendToFile.\n
An error occurred while appending a StringObject to the file
'%0c'.  Check that the file '%0c' exists and can be written by you."

/*%
** Parameters:
**	0	This is the name of the file being appended to.
**
** Explanation:
**	While executing the method AppendToFile for a StringObject an error
**	occurred.
**
** System Status:
**	The StringObject will not be appended to the file.
**
** User Action:
**	Make sure the file given can be written by you.
*/

E_OM0025_SO_EMPTYFILE	"Unexpected empty file name.\n
An empty file name was given as an argument to a routine that needs a
file name.  If you are relying on a default value, make sure the StringObject's
FileHandle is set."

/*%
** Explanation:
**	An operation that requires a file name was given an empty string for
**	the file name.
**
** System Status:
**	The operation will not execute.
**
** User Action:
**	Provide a valid file name for this operation.
*/

E_OM0026_SO_BADDBHANDLE	"Non-existent StringObject for a DBHandle.\n
A StringObject's DBHandle was set to a DBHandle for a string that does not
exist in the data base.   Either the DBHandle is bad, or the corresponding
StringObject was deleted from the data base.  The DBHandle is '%0c'."

/*%
** Parameters:
**	0	This is a string which is the DBHandle.
**
** Explanation:
**	The DBHandle for a StringObject was set to a DBHandle that does
**	not exist in the database.
**
** System Status:
**	The StringObject will not be given a value, and the DBHandle attribute
**	will be set to the empty string.
**
** User Action:
**	Only set the DBHandle to existing StringObjects.
*/

E_OM0027_PRMCONVERT	"Conversion error while passing parameter %0c.\n
A data conversion error occurred while passing a value to the parameter
%0c.  The type of the actual parameter value must be compatible with the
type of the parameter %0c."

/*%
** Parameters:
**	0	The name of the parameter being passed to.
**
** Explanation:
**	The value passed to a particular parameter was of an
**	incorrect type.
**
** System Status:
**	The parameter will not be given a value.
**
** User Action:
**	Correct the type of the parameter value to be compatible
**	with the type of the parameter.
*/

E_OM0028_REFPRM	   "Conversion error while returning parameter %0c.\n
A data conversion error occurred while returning a value by reference
through the parameter %0c.  The type of the parameter is not compatible
with the variable passed by reference."

/*%
** Parameters:
**	0	The name of the parameter being passed to.
**
** Explanation:
**	The value passed to a particular parameter was of an
**	incorrect type.
**
** System Status:
**	The parameter will not be given a value.
**
** User Action:
**	Correct the type of the parameter value to be compatible
**	with the type of the parameter.
*/

E_OM0029_BADRETURN	"Conversion error during return.\n
A data conversion error occurred while returning a value from a
procedure or method invocation.  The return value could not be
converted to the type expected by the calling object."

/*%
** Explanation:
**	A return value could not be converted to the type expected
**	by the caller.
**
** System Status:
**	A value will not be returned.
**
** User Action:
**	Convert the return value to the correct type.
*/

E_OM002A_SO_ID_DELETE  "Error trying to delete maximum id for StringObject table.\n
There was a DBMS error while deleting the maximum id value for the
StringObject table '%0c'.  A subsequent error should give the DBMS error
message.  Please correct this problem."

/*%
** Explanation:
**	The system stores the maximum id for a StringObject in a special
**	row within the StringObject table.   If it finds more than 1 of
**	these special rows in the table, it tries to correct the problem
**	by deleting them all and adding a new special row.  In doing
**	the delete, a database error was encountered.
**
** System Status:
**	The operation will not complete.
**
** User Action:
**	Correct the problem causing the database error, and if this
**	can't be rectified, report the problem to your INGRES representative.
*/

E_OM002B_SO_ID_MAX	"Error while creating new maximum id for StringObject table.\n
There was a DBMS error while finding the maximum id value for the
StringObject table '%0c'.  A subsequent error should give the DBMS error
message.  Please correct this problem."

/*%
** Explanation:
**	The system stores the maximum id for a StringObject in a special
**	row within the StringObject table.   If the system finds more
**	than 1 special row, or finds that non exists, it tries to correct
**	the problem by deleting any extra ones and adding a new one.
**	In doing this, a database error was encountered so the special
**	row could not be created.
**
** System Status:
**	The operation will not complete.
**
** User Action:
**	Correct the problem causing the database error, and if this
**	can't be rectified, report the problem to your INGRES representative.
*/

E_OM002C_CLASS_NOTINIT "The user class has not been initialized.\n
An attempt was made to add a definition to a user class that has
not yet been initialized.  The appropriate initialization must be invoked."

/*%
** Explanation:
**	A User Class is either being built, or having its definition
**	enhanced, and the class has not yet been initialized.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**	Invoke the proper Initialization method.
*/

E_OM002D_CLASS_NOTEXIST "The CLASS %0c does not exist.\n
An attempt has been made to resolve a reference to a class that does not exist."

/*%
** Explanation:
**	A reference has been made to either a System or User Class that does
**	not exist.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**	Check the name of the class being referenced.  In the case of a User
**	Class, check to make sure it is defined in the User application.
*/

E_OM002E_NOTA_CLASS "The specified object %0c is not a CLASS object.\n
An attempt has been made to reference an object as either a System or User
Class which is not truly a CLASS."

/*%
** Explanation:
**	An object which is expected to be a CLASS object is some other
**	type of object.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**	Check the name and type of the object being referenced.
*/

E_OM002F_ATTR_EXIST "The attribute \"%0c\" is already included in class %1c.\n
An attempt has been made to define an attribute for a user class which is
already included either in the class, or its parent.  Any reference 
to the attribute will resolve to the one belonging to class %1c, not its 
parent's.  The attribute should be renamed to insure uniqueness."

/*%
** Explanation:
**	Our CLASS model does not support the supercedence of a parents'
**	attributes, nor multiple attributes of the same name.  An attempt
**	has been made to violate one of these two conditions.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**      Check the name of the attribute being added.
*/

E_OM0030_METH_EXIST "The method %0c already belongs to class %1c.\n
An attempt has been made to add a method to a class which already has a method
definition of that name."

/*%
** Explanation:
**	Our CLASS model does not allow a class to have more than one method
**	of the same name.  An attempt has been made to violate this condition.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**      Check the name of the method being added.
*/

E_OM0031_NOCLASSNAME "No class name has been provided.\n
A mandatory class name has not been provided by the caller."

/*%
** Explanation:
**	A mandatory class name argument has not been provided to a procedure
**	that requires it.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**      Check the name and type of the object being referenced.
*/

E_OM0032_NO_ATTR "The attribute %0c of class %1c cannot be resolved.\n
An attempt was made to resolve an attribute of a type that is not recognized."

/*%
** Explanation:
**	An attempt was made to create an attribute whose type is not
**	recognizable.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**      Check the type of the attribute being referenced.
*/

E_OM0033_NO_METH "The method %0c of class %1c cannot be resolved.\n
An attempt was made to resolve an method of a type that is not recognized."

/*%
** Explanation:
**      An attempt was made to create an method whose type is not
**      recognizable.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**      Check the type of the method being referenced.
*/

E_OM0034_ARRMEM	"Ran out of memory during an array operation.\n
The system ran out of memory while doing an array operation.  The array
will be cleared and the system will attempt to continue."

/*%
** Explanation:
**	The system ran out of virtual memory during an array operation.
**
** System Status:
**	The array will be cleared and the system will continue.
**
** User Action:
**	The only recourse is to restructure the program so that less
**	memory is used by the program.
*/

E_OM0035_CLASSLOOP	"Recursive Loop found in class definition.\n
A recursive loop was found in a class definition while a UserClass object was
being allocated.  Subsequent lines will list the names of the classes that 
participate in this loop.  The first line shows the name of the class 
for which an object was being allocated when the loop was detected.
The last line shows the name of the class that contains an attribute whose 
type is the first class (in the case of a directly recursive class, there is 
only one line).  To avoid this recursive loop, you must set the DefaultValue 
for one of the attributes in the chain to NULL."
/*%
** Explanation:
**
** System Status:
**	The object for the last class in the chain will not be allocated,
**	and the attribute will be set to NULL.
**
** User Action:
**	Set one of the DefaultValue of one of the attributes in the chain
**	to NULL.
*/

E_OM0036_NODBVRET	"Bad return type used.\n
This is an internal coding error.  The return type found in a structure
was an incorrect type.  Please contact your Ingres Representative."

/*%
** Explanation:
**	This is an Ingres error.
**
** System Status:
**	The exact action depends on the object that was called.
**
** User Action:
**	Please contact you Ingres representative.
*/

E_OM0037_PRMNOTBYREF	"The parameter '%0c' must be passed BYREF.\n
The actual argument passed to the parameter '%0c' must be passed BYREF.
Change the call so that a BYREF is used on the actual argument for
'%0c'."

/*%
** Parameters:
**	0	The name of the parameter being passed a value.
**
** Explanation:
**	The given parameter must be passed any value by reference.
**
** System Status:
**	The exact action depends on the object that was called.  Most
**	will simply continue executing by ignoring this parameter.
**
** User Action:
**	Add a BYREF clause to the actual argument being passed to
**	this parameter.
*/

E_OM0038_SYSCLASS "'%0c' is a Windows/4GL System Class.\n"

/*%
** Parameters:
**      1       The name of the system class.
**
** Explanation:
**	The user tried to create a new datatype with the same name
**	as an alread-existing system class.
**
** System Status:
**	The new datatype won't be created.
**
** User Action:
**	Think of a new name.
*/

E_OM0039_OPENCNT	"Too many arguments given to a method.\n
The method '%0c' on a DbSessionObject was called with too many arguments.
The maximum number of arguments is %1d."

/*%
** Parameters
**	1	The name of the method that was called.
**
**	2	The maximum number of arguments permitted.
**
** Explanation:
**	A method was invoked on a DbSessionObject, and too many arguments
**	were passed to the method.
**
** System Status:
**	The method will not execute.
**
** User Action:
**	Reduce the number of arguments to the legal limit.
*/

E_OM003A_BADATTR "The attribute %0c could not be loaded.\n
An attempt has been made to load an attribute that could not be resolved."

/*%
** Explanation:
**	An attempt was made to load an attribute description that could
**  not be resolved.
**
** System Status:
**      The operation will not complete.
**
** User Action:
**	Check the type of the attribute.  Either its type is unrecognized,
**	or if another user class, deleted from the data dictionary.
*/

E_OM003B_CS_ISOPEN	"Cursor is already open.\n
There was an attempt to re-open a cursor, which was already opened
in %0c at line %1d.  You must first close the cursor before re-opening it."

/*%
** Parameters:
**	0	This is the name of the component that issued the
**		open statement for this cursor.
**
**	1	This is the line number of the open statement within
**		that component.
**
** Explanation:
**	It is not legal to issue an OPEN cursor statement specifying
**	a cursorobject which is already open.
**
** System Status:
**	The open cursor will be left unchanged.
**
** User Action:
**	Correct logic to ensure that a cursor is closed before attempting
**	to re-open it.
*/

E_OM003C_CS_IS_CACHED	"The cursor is currently opened in Cached mode.\n
An attempt was made to perform a %0c operation on a cursor opened in Cached
mode.  This is not permitted."

/*%
** Parameters:
**	0	This is the name of the operation that was being
**		performed.  It will be one of:
**			UPDATE
**			DELETE
**
** Explanation:
**	An UPDATE or DELETE operation was performed on a cursor opened in
**	cache mode. These operations are only legal when the cursor is 
**	opened in the non-cached mode.
**
** System Status:
**	The system will continue running, but the operation will
**	not be executed.
**
** User Action:
**	Did not attempt to update or delete from cursors opened in Cached mode.
*/

E_OM0040_OPENERR	"Couldn't open database '%0c'.\n
The database '%0c' could not be opened.  A subsequent error should give
more information about why the database could not be opened.  If no
subsequent error is given, check the spelling of the database name and
make sure that you can access it."

/*%
** Parameters
**	1	The name of the database that couldn't be opened.
**
** Explanation:
**	The given database could not be opened.  All attempts are made
**	to get the specific DBMS error that is causing the problem
**	and if that error can be gotten, it is printed too.
**
** System Status:
**	This depends on why the database is being opened.  If the database
**	is being opened to start running an application, the application
**	will not be run.
**
** User Action:
**	Correct the problem identified by the DBMS error.  If not DBMS
**	error is given, try opening the database using the terminal
**	monitor to see if a better error can be generated.
*/

E_OM0041_OPENSYS	"Attempt to open a second system session.\n
This is an internal error, and should be reported to your Ingres representative.
An attempt was made to open a second system session.  Only one system
session is allowed."

/*%
** Explanation:
**	This is an internal error.
**
** User Action:
**	Report this to your Ingres representative.
*/

E_OM0042_CLOSEERR	"Attempt to Close a DbSessionObject that isn't open.\n
The Close method was applied to a DbSessionObject that isn't open. The
Close method should only be applied to open DbSessionObjects."

/*%
** Explanation:
**	The Close method was applied to an unopen DbSessionObject.  This
**	is an error.
**
** System Status:
**	Processing continues.
**
** User Action:
**	Correct the invocation of Close so that it is not applied to
**	an unopened DbSessionObject.
*/

E_OM0043_CURRENT    "Attempt to make an unopened session the current session.\n
The MakeCurrent method was applied to an unopened session.  It can only
be applied to open DbSessionObjects."

/*%
** Explanation:
**	The MakeCurrent method was applied to an unopen DbSessionObject.  This
**	is an error.
**
** System Status:
**	Processing continues, and the current DbSessionObject remains unchanged.
**
** User Action:
**	Correct the invocation of MakeCurrent so that it is not applied to
**	an unopened DbSessionObject.
*/

E_OM0044_NOTARRAY "Attempt to assign a non-array value to an array variable.\n
A non-array value of class '%1c' was assigned to an array variable
of class '%0c'.  You can only assign an array value of class '%0c' to
this array variable."

/*%
** Parameters
**	0	The name of the class of the array. For example, if
**		the array variable is defined as ARRAY OF StringObject
**		this will be StringObject.
**
**	1	The name of the class of the value.
**
** Explanation:
**	A reference value was assigned to an array variable.  This
**	is not legal.  Only an array can be assigned to an array variable.
**
** System Status:
**	The value will not be moved into the array.
**
** User Action:
**	Fix this statement so that an array value is assigned to
**	the array variable.
*/

E_OM0045_ARRAY	"The class of an array value does not match the array variable.\n
An array value of class 'Array of %1c' is being assigned to an array
variable of class '%0c'.  The class 'Array of %1c' is not a subclass of 
the class '%0c', so the assignment will fail."

/*%
** Parameters
**	0	The name of the class of the array. For example, if
**		the array variable is defined as ARRAY OF StringObject
**		this will be 'Array of StringObject'.
**
**	1	The name of the class of the values rows.
**
** Explanation:
**	For two arrays to be assignment compatible, the classes of the
**	two arrays must be the same.  This is not the case for
**	this statement.
**
** System Status:
**	The value will not be moved into the array.
**
** User Action:
**	Fix this statement so the arrays have the same class.
*/

E_OM0046_BADCLASS	"'%1c' is not a subclass of '%0c'.\n
A value of class '%1c' is being assigned to a reference variable of
class '%0c'.  The class '%1c' is not a subclass of '%0c', so
this assignment is illegal."

/*%
** Parameters:
**	0	The name of the class of the variable.
**
**	1	The name of the class of the value.
**
** Explanation:
**	A value is being assigned to a reference variable.  For this
**	to be legal, the value must be the same class or a subclass of
**	the variable.  In this case, the class of the value is a subclass
**	of the variable.
**
** System Status:
**	The value will not be assigned to the reference variable.
**
** User Action:
**	Make the value a subclass of the reference variable.
*/

E_OM0047_BADCLASSBYREF	"Cannot pass '%1c' to '%0c' BYREF.\n
An argument defined as being of class '%1c' is being passed BYREF to a 
parameter of class '%0c'.  The class '%0c' is not a subclass of
'%1c', so the update of the argument would fail."

/*%
** Parameters:
**	0	The name of the type of object being duplicated.
**
**	1	The name of the attribute that couldn't be duplicated.
**
**	2	The name of the type of the attribute.
**
** Explanation:
**	An attempt was made to duplicate an object.  Each of its attributes
**	of type OBJECT (if any) were also to be duplicated, but one of them
**	failed.
**
** System Status:
**	No specific action is taken.
**
** User Action:
**	The duplicated object can be used within the limitations imposed
**	by having one of its attributes NULL.
*/

E_OM0048_NOTDUPLICATE "Cannot completely duplicate object.\n
An attempt was made to duplicate an object of type %0c.  Although the object
itself was duplicated, its attribute %1c of type %2c was not, and is now NULL."

/*%
**
** Arguments:
**	0	This is the number of parameters that the called object
**		accepts.
**
**	1	This is the number of the positional parameter that
**		was passed that exceeds the number.
**
** Explanation:
**      The parameter list for the call of an object included too many
**	positional parameters.
**
** System Status:
**	The exact action depends on the object that was called.  Most
**	will simply continue executing by ignoring this parameter.
**
** User Action:
**	Remove the extra parameters from the parameter list and
**	check the documentation to find the correct number of parameters.
*/

F_OM0000_CSRPRINTF	"(CursorObject opened at line %d of %s)"
/*%
** Explanation:
**	This string is used in a STprintf statement to construct a
**	name for a CursorObject.  The %d and %s are arguments for
**	the STprintf call.  The %d will be the line number that
**	the cursor object was opened on, and the %s is the name
**	of the frame or procedure that the cursor object was
**	opened in.
**
**	The string resulting from the STprintf will be used in
**	place of the DBMS cursor name in an error like E_US0899.
**
**	This allows 4GL to issue an error like:
**          E_US0899 line 1, Cursor '(Cursorobject opened at line 10 of cursors)**          already open.
**	Instead of:
**          E_US0899 line 1, Cursor 'sph_15' already open.
*/
F_OM0001_MISSING	"(Class is missing)"
/*%
** Explanation:
**	This string is used in the errors E_OM0044 to E_OM00XX in place
**	of a class name if the class could not be found at runimte.
*/

F_OM0002_VALDESC	"%0c, value = %1c"

/*
**	bag.c
*/

E_OM0049_BAGADD_ALLOC_ERR	"Windows4gl could not allocate %d bytes needed by routine BadAdd.  This is a fatal error.\n"

/*%
** Explanation:
**	Routine IIOMxxxBagAdd() could not allocate memory that it needed.
**	This is usually due to various process or system limits be too low.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None at the moment.  User should increase process or system limits
**	before restarting.
*/

E_OM004A_BAGCLONE_ALLOC_ERR	"Windows4gl could not allocate %d bytes needed by routine BagClone.  This is a fatal error.\n"

/*%
** Explanation:
**	Routine IIOMxxxBagClone() could not allocate memory that it needed.
**	This is usually due to various process or system limits be too low.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None at the moment.  User should increase process or system limits
**	before restarting.
*/

E_OM004B_BAGDECODE_ALLOC_ERR	"Windows4gl could not allocate %d bytes needed by routine BagDecode.  This is a fatal error.\n"

/*%
** Explanation:
**	Routine IIOMxxxBagDecode() could not allocate memory that it needed.
**	This is usually due to various process or system limits be too low.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None at the moment.  User should increase process or system limits
**	before restarting.
*/

E_OM004C_BAGINIT_ALLOC_ERR	"Windows4gl could not allocate %d bytes needed by routine BagInit.  This is a fatal error.\n"

/*%
** Explanation:
**	Routine IIOMxxxBagInit() could not allocate memory that it needed.
**	This is usually due to various process or system limits be too low.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None at the moment.  User should increase process or system limits
**	before restarting.
*/

E_OM004D_BAGINSERT_ALLOC_ERR	"Windows4gl could not allocate %d bytes needed by routine BagInsert.  This is a fatal error.\n"

/*%
** Explanation:
**	Routine IIOMxxxBagInsert() could not allocate memory that it needed.
**	This is usually due to various process or system limits be too low.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None at the moment.  User should increase process or system limits
**	before restarting.
*/

/*
**	buffer.c
*/

E_OM004E_NEWQB_ALLOC_ERR	"Windows4gl could not allocate %d bytes needed for a new queue buffer.  This is a fatal error.\n"

/*%
** Explanation:
**	Routine newqb() could not allocate memory that it needed.
**	This is usually due to various process or system limits be too low.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None at the moment.  User should increase process or system limits
**	before restarting.
*/

/*
**	mdbsess.sc
*/

S_OM004F_CANT_CACHE_ARGS	"Can't cache dbsession information due to too many arguments.\n"

E_OM0067_ALREADY_CONNECTED	
"The DBSESSIONOBJECT is already connected\n
An attempt was made to use the Connect method on a DBSESSIONOBJECT whose state 
is DS_CONNECTED.  This is not allowed.  The Connect will not be performed."

/*%
** Explanation:
**	An attempt was made to Connect a DBSESSIONOBJECT which is already 
**	connected to a database session.  This is not allowed.
**
** System Status:
**	The Connect method will not be performed; the DBSESSIONOBJECT will
**	keep the connection it already has.
**
** User Action:
**	Either disconnect the DBSESSIONOBJECT before attempting to connect it,
**	or use another DBSESSIONOBJECT for the Connect.
*/

E_OM0068_NOT_CONNECTED
"The DBSESSIONOBJECT is not currently connected.
An attempt was made to use the OpenNewConnection method specifying a 
DBSESSIONOBJECT which is not currently connected.  This is not allowed, since
the purpose of OpenNewConnection is to create a copy of an existing connection.
No new connection will be opened."

/*%
** Explanation:
**	An attempt was made to perform OpenNewConnection for a DBSESSIONOBJECT 
**	which is not connected to a database session.  This is not allowed.
**
** System Status:
**	No connection will be created.
**
** User Action:
**	Either use OpenNewConnection on a DBSESSIONOBJECT which is connected
**	or use another the Connect method to create a new connection.s
*/

E_OM006A_NO_SESSION
"A query was issued without a valid database session.\n
A frame or procedure that does not have a valid database session as its
DBsession attribute has issued a query.  The query cannot be executed."

/*%
** Explanation:
**      A frame or procedure which does not have a valid database session
**      has issued a query.  The frame's session may have been
**      closed previously or may be NULL.  The query can not be
**      executed, since there is no database session in which to execute it.
**
** System Status:
**      The query will not be executed.
**
** User Action:
**      Ensure that a valid database session is always current when a
**      query is issued.
*/


/*
**	mstrmraf.c
*/

S_OM0050_PRINTF_NOT_FOR_RA	"The 'printf' method is not allowed for Random Access Files.\n"

/*
**	mstrobj.sc
*/

E_OM0051_STRINGOBJECT_VERSION	"Stringobject(%x) has version# %d, a non-supported version.  This is a fatal error.\n"

/*%
** Explanation:
**	Windows4gl has detected a situation where a string object has the
**	wrong version.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None possible for the moment.  User should check to make sure that
**	the image file has not been altered and that the version of the image
**	file and sphmerge are compatible.
*/

E_OM0069_STRINGOBJECT_TOO_LONG
"An attempt has been made to send a quoted stringobject of length %0c to the
DBMS.  This can only be done for stringobjects of length at most %1c."

/*%
** Explanation:
**	The stringobject is too long to be quoted and sent to the DBMS.
**	A quoted string is used, for instance, as the associated text in
**	a RAISE DBEVENT statement.
**
** System Status:
**	The current query will fail.
**
** User Action:
**	Insure that the stringobject has a proper length.
*/

/*
**	musclobj.c
*/

S_OM0052_POINTS_TO	"\t'%s' points to '%s' \n"

/*
**	object.c
*/

E_OM0053_OBJECT_NULL_ASSIGN	"Attempt to do an object assignment to a NULL object pointer.  This is a fatal error.\n"

/*%
** Explanation:
**	Windows4gl detected an attempt to assign an object handle to a NULL
**	pointer in routine IIOMxxxObjectAssign().
**
** System Status:
**	Windows4gl will exit since this is a fatal error.
**
** User Action:
**	None.
*/

E_OM0054_ASSIGN_NULL_OBJECT	"Attempt to assign an object to a NULL pointer.  This is a fatal error.\n"

/*%
** Explanation:
**	Windows4gl detected an attempt to assign an object to a NULL
**	pointer in routine IIOMassign().
**
** System Status:
**	Windwos4gl will exit since this is a fatal error.
**
** User Action:
**	None.
*/

/*
**	omsetct.c
*/

E_OM0055_NO_SETCLASSTABLE	"The System Class table for Windows4gl has not been setup.\n Windows4gl will not be able to continue.\n"

/*%
** Explanation:
**	Windows4gl detect the fact that the system class table has not been
**	setup before trying to use it.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None.
*/

/*
**	stream.c
*/
S_OM0056_FILE_HAS_NO_OBJECTS	"File contains no objects;"

S_OM0057_FILE_MAY_BE_BAD	" file may be corrupt, truncated, or newer than executable.\n"

S_OM0058_PREMATURE_EOF	"Premature EOF searching for first section;"

E_OM0059_OBJECTS_ALLOC_ERR	"Unable to allocate object %c(%p).  %d bytes was needed.\n"

/*%
** Explanation:
**	Windows4gl could not allocate memory for objects coming from
**	a stream.  This is non-fatal at this stage.
**
** System Status:
**	Windows4gl will try to continue.
**
** User Action:
**	None.
*/

S_OM005A_CONVERTING_OLD_CLASS	"Converting old class name '%c' to '%c'.\n"

S_OM005B_WARNING	"** Warning **"

S_OM005C_ERROR	"** Error **"

S_OM005D_CLASS_IS_UNKNOWN	"  Class '%c' is unknown;"

S_OM005E_NULL_CLASSNAME	"  NULL Classname found;"

S_OM005F_OBJECT_IGNORED	"               The object will be ignored, but results are unpredictable.\n"

E_OM0060_INSTANCE_ALLOC_ERR	"Problems occurred while allocating an instance of class '%c' (%p %d).\n"

/*%
** Explanation:
**	Windows4gl had problems allocating an instance of the specified
**	class.  Message preceding this one should have provide more details
**	on the cause of the problem.
**
** System Status:
**	Windows4gl will continue processing.
**
** User Action:
**	None.
*/

S_OM0061_EOF_NEXT_OBJECT	"Premature EOF searching for next object;"

S_OM0062_EOF_END_CLUSTER	"Premature EOF searching for end of cluster;"

S_OM0063_CONVERTING_CLASS	"Converting old class name '%s' to '%s'.\n"

/*
**	tfarrdes.c
*/

E_OM0064_TFARRDESC_VERSION	"Table field array descriptor (%x) has version # %d, a non-supported version.  This is a fatal error.\n"

/*%
** Explanation:
**	Windows4gl has detected a situation where a table field array
**	descriptor object has the wrong version.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None possible for the moment.  User should check to make sure that
**	the image file has not been altered and that the versions of the image
**	file and sphmerge are compatible.
*/

/*
**	type.c
*/
E_OM0065_TYPEGETATTR_ILLEGAL	"An illegal attempt to use the 'getattr' method for class TYPE has been detected.\n"

/*%
** Explanation:
**	Windows4gl detected a situation where someone attempted to use the
**	getattr method on class TYPE.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None.
*/

E_OM0066_TYPEGETMETH_ILLEGAL	"An illegal attempt to use the 'getmethod' method for class TYPE has been detected.\n"

/*%
** Explanation:
**	Windows4gl detected a situation where someone attempted to use the
**	getmethod method on class TYPE.  This is a fatal error.
**
** System Status:
**	Windows4gl will exit.
**
** User Action:
**	None.
*/

/*
**	New messages for "Voyager" project for W4gl III.
**	Create gap in slow message numbers from 006A (highest one in Lapis)
**	to 0080 so no overlap.
*/

/*
**	files:  dbobj!srcobj!macrovar.c, preproc.c, ...
*/
E_OM0080_Bad_Macrovar_Name "Illegal macro variable name...\n
The name '%0c' is not a valid macro variable name.
Macro variable names must begin with a '$' and
following that must be alpha-numeric and underscore characters;
they may not be over %1d characters long (also, it is recommended that
user-defined macro variables not begin with '$_').\n
This error in your source script must be corrected before you can
successfully compile."

/*%
** Explanation:
**	$macro_variable name is not legal. Probably one of following problems:
**	- doesn't begin with '$'.
**	- is > 32 characters long.
**	- contains character that is not alphabetic, numeric or '_'.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change name of $macro_variable to legal name.
*/

E_OM0081_AV_SegViol     "Internal Error occurred while preprocessing 4gl code\n
An internal error (known as an Access Violation, Bus Error, or
Segmentation Violation on many systems)
occurred while preprocessing 4GL for this frame. Please
narrow down the conditions required to make this error occur,
by changing sections of frame definition and then re-Compiling this
frame until the error goes away. Then report this problem to Ingres Technical
Support."

/*%
** Explanation:
**	Segmentation or Access violation occurred during preprocessing. This
**	internal error should not happen and indicates a product bug.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Try to narrow down what conditions cause the problem to occur
**	and report to technical support.
*/

E_OM0082_Open_Error     "Error occurred while preprocessing 4gl code\n
Unable to open %1c file while preprocessing 4gl code.
An error occurred trying to preprocess 4gl code for this frame: the %1c
file '%0c' could not be opened. Check that you have sufficient disk space
and quota on the directory where this file lives, and that you have
authority to read and write files there."

/*%
** Explanation:
**	File open failed.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Check permissions in directory mentioned in message.
*/

E_OM0083_Token_TooLong		"Error occurred while preprocessing 4gl code\n
Found too long a token in source script.
An error occurred preprocessing 4gl code for this frame. A token
was found in a source script whose length exceeds the maximum allowable
token size (max size = %0d). If the token is a string constant, try
breaking it into several pieces and combine them with the string
concatenate operator ('+').\n
This error in your source script must be corrected before you can
successfully compile."

/*%
** Explanation:
**	Word (token) in script is too long.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change script to shorten token.
*/

E_OM0084_OutOfMemory		"Error occurred while preprocessing 4gl code\n
Not enough memory available.
The Windows4gl preprocessor was unable to allocate enough dynamic memory
to preprocess the 4gl script for this frame. This usually indicates
that your process or system ran out of memory, or tried to exceed a
memory limit quota. Take whatever steps you can to free up more memory
for your use and then try this again. Contact your Site Technical
Contact for assistance."

/*%
** Explanation:
**	Compiler was unable to allocate memory during preprocess stage.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Free up memory (for example, by closing windows, or exiting
**	other processes) and try again.
*/

E_OM0085_Write_Error		"Error occurred while preprocessing 4gl code\n
An error occurred while trying to write preprocessed 4gl code to a
temporary source file.
The temporary source file will be deleted. Make sure you have
enough disk space and sufficient privilege to create temporary
files."

/*%
** Explanation:
**	Compiler was unable to write to a temporary file.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Make sure you have enough disk space and sufficient privilege
**	to create temporary files.
*/

E_OM0087_Bad_TL_Statement       "Error occurred while preprocessing 4gl code\n
Line %1d of %2c%3c contains the statement '# %0c',
which is not a valid Macro Language statement.\n
This error in your source script must be corrected
before you can successfully compile."

E_OM0071_Bad_TL_Statement      "Error occurred while preprocessing 4gl code\n
The statement '# %0c' is not a valid Macro Language statement.\n\
This error in your source script must be corrected
before you can successfully compile."

/*%
** Explanation:
**	(E_OM0087 and E_OM0071)
**	An unknown #statement was found.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change the 4gl source script to only contain # statements that
**	are understood by the preprocessor.
*/

E_OM0088_Bad_Expression         "Error evaluating an expression...\n
An error occurred during 4gl preprocessing while trying to evaluate the
expression on line '%0d' of %1c%2c. A message in the trace
window will give more detail about the error.\n
This error in your source script
must be corrected before you can successfully compile."

E_OM0072_Bad_Expression         "Error evaluating an expression...\n
An error occurred during 4gl preprocessing while trying to evaluate this
expression. A message in the trace
window will give more detail about the error.\n\
This error in your source script
must be corrected before you can successfully compile."

/*%
** Explanation:
**	(E_OM0088 and E_OM0072)
**	Error occurred while trying to evaluate an expression.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change 4gl source script to contain only valid expressions in
**	#IF and #DEFINE statements.
*/

E_OM0089_No_Change_Builtin  "Cannot UNDEF a built-in $variable...\n
Line %1d of %2c%3c contains the statement: '#UNDEF %0c'. '%0c'
is a built-in $variable and cannot be UNDEFined.\n
This error in your source script
must be corrected before you can successfully compile."

E_OM0073_No_Change_Builtin  "Cannot UNDEF a built-in $variable...\n
'%0c' is a built-in $variable and cannot be UNDEFined.\n\
This error in your source script
must be corrected before you can successfully compile."

/*%
** Explanation:
**	(E_OM0089 and E_OM0073)
**	Source code tried to UNDEF a built-in $variable.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change 4gl source script to not try to UNDEF a built-in $variable.
*/

E_OM008A_IfStmt_Nesting  "Error occurred while preprocessing 4gl code\n
An error occurred trying to preprocess 4gl code for this frame or procedure.
#IF/#IFDEF/#IFNDEF blocks may only be nested up to %0d levels deep.
Line %1d of %2c%3c contains a #IF, #IFDEF or #IFNDEF
statement which, if executed, would have exceeded this limit.\n
This error in your source script must be corrected before you can
successfully compile."

E_OM0074_IfStmt_Nesting  "Error occurred while preprocessing 4gl code\n
An error occurred trying to preprocess 4gl code for this frame or procedure.
#IF/#IFDEF/#IFNDEF blocks may only be nested up to %0d levels deep;
this limit has been exceeded.\n\
This error in your source script must be corrected before you can
successfully compile."

/*%
** Explanation:
**	(E_OM008A and E_OM0074)
**	Source code tried to nest IF/IFDEF/IFNDEF statements more than
**	the maximum allowed depth.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change 4gl source script to not nest such statements so deeply.
*/

E_OM008B_Mismatch_ElseEndif     "Error occurred while preprocessing 4gl code\n
Error in source script: mismatched #ELSE or #ENDIF.
An error occurred trying to preprocess the 4gl code for this frame.
On line %0d of %1c%2c, a #ELSE or #ENDIF statement was
encountered that does not match any previous #IF, #IFDEF or #IFNDEF
statement.\n
This error in your source script must be corrected before you can
successfully compile."

E_OM0075_Mismatch_ElseEndif     "Error occurred while preprocessing 4gl code\n
Error in source script: mismatched #ELSE or #ENDIF.
An error occurred trying to preprocess the 4gl code for this frame:
a #ELSE or #ENDIF statement was
encountered that does not match any previous #IF, #IFDEF or #IFNDEF
statement.\n\
This error in your source script must be corrected before you can
successfully compile."

/*%
** Explanation:
**	(E_OM008B and E_OM0075)
**	Source script contains a #ELSE or #ENDIF statement that does
**	not match any previous IF/IFDEF/IFNDEF statement.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change 4gl source script to remove extra ELSE/ENDIF statement.
*/

E_OM008C_Too_Many_Elses "Error occurred while preprocessing 4gl code\n
Error in source script: too many #ELSE stmts found.
Line %0d of %1c%2c contains a duplicate #ELSE statement. A
#ELSE statement has already been seen for the previous #IF/#IFDEF/#IFNDEF
statement, so this duplicate #ELSE is an error in the source script.\n
This error in your source script must be corrected before you can
successfully compile."

E_OM0076_Too_Many_Elses "Error occurred while preprocessing 4gl code\n
Error in source script: too many #ELSE stmts found.
A #ELSE statement has already been seen for the previous #IF/#IFDEF/#IFNDEF
statement, so this duplicate #ELSE is an error in the source script.\n\
This error in your source script must be corrected before you can
successfully compile."

/*%
** Explanation:
**	(E_OM008C and E_OM0076)
**	Source script contains a #ELSE statement that does
**	not match any previous IF/IFDEF/IFNDEF statement.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change 4gl source script to remove extra ELSE statement.
*/

E_OM009B_Not_Scriptsource "Error occurred while preprocessing 4gl code\n
#INCLUDE statement references object of wrong type.
The statement: '#INCLUDE %0c' was found on line %1d of %2c%3c.
'%0c' is type '%4c', which is not an Include Script; the #INCLUDE
statement can only reference an application component that is an
Include Script.\n"

E_OM0077_Not_Scriptsource "Error occurred while preprocessing 4gl code\n
#INCLUDE statement references object of wrong type.
'%0c' is of type '%1c', which is not an Include Script; the #INCLUDE
statement can only reference an application component that is an
Include Script.\n"

/*%
** Explanation:
**	(E_OM009B and E_OM0077)
**	The #INCLUDE statement references an object
**	that is Not of type Include Script.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change 4gl source script to reference correct type of
**	object in #INCLUDE statement (Include Script).
*/

E_OM009C_NoSuch_Scriptsource "Error occurred while preprocessing 4gl code\n
The statement: '#INCLUDE %0c' was found on line %1d of %2c%3c.
The include script named '%0c' does not exist (or could not be accessed).
Check that this include script exists and can be accessed by you.\n"

E_OM0078_NoSuch_Scriptsource "Error occurred while preprocessing 4gl code\n
The include script named '%0c' does not exist (or could not be accessed).
Check that this include script exists and can be accessed by you.\n"

/*%
** Explanation:
**	(E_OM009C and E_OM0078)
**	#INCLUDE statement referenced a script that does not exist
**	(or could not be accessed).
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Check that include script referenced in #INCLUDE statement
**	exists.
*/

E_OM009D_MaxIncludeNest    "Error occurred while preprocessing 4gl code\n
#INCLUDE statements nested too deep.  The statement
'#INCLUDE %0c' was encountered on line %1d of Include Script %2c,
and cannot be processed, because that would exceed the limit of
%3d levels deep in #INCLUDE script processing.
Often an infinite loop in your #INCLUDE scripts will cause this
error (for example if script1 does '##INCLUDE script2', and script2 does
'#INCLUDE script1', then an infinite loop will occur). Check your include
scripts for an infinite loop in #INCLUDE statements, or #INCLUDE statements
nested more than %3d levels deep."

/*%
** Explanation:
**	A limit exists on how deeply #INCLUDE statements can reference
**	each other (in order to catch infinite loops). This frame exceeded
**	that limit.
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Check #include scripts for circular references.
*/

E_OM00AD_BadVarName "Error occurred while preprocessing 4gl code\n
A statement was found on line %1d of %2c%3c that references the
name '%0c'. This is not valid, because a $variable name should
appear at this position in this statement (including the leading '$').
Please replace '%0c' with a valid $variable name.\n"

E_OM0079_BadVarName "Error occurred while preprocessing 4gl code\n
Invalid statement: a $variable name should
appear at this position in this statement (including the leading '$').
Please replace '%0c' with a valid $variable name.\n"

/*%
** Explanation:
**	(E_OM00AD and E_OM0079)
**	An IFDEF/IFNDEF or UNDEF statement references a name that does
**	not begin with "$". (probably, you forgot to type the "$"
**	at the beginning of the $variable name in this statement).
**
** System Status:
**	Compilation fails in preprocess phase.
**
** User Action:
**	Change this statement to reference a $variable.
*/

/* Following fast messages are used in dbobj!srcobj!preproc.c as parameters to
** errors: S_OM009B_Not_Scriptsource & E_OM009C_NoSuch_Scriptsource.
*/

F_OM0006_CompFrameScript	"the composite Frame Script"
F_OM0007_IncludeScript		"Include Script "

/*
** The following messages provide users with 10 generic status codes
** (ER_USER1 through ER_USER10, defined in fleas!symtab!i4built.roc)
** which can be used to communicate between frames in a user application,
** and which are guaranteed not to conflict with any error codes
** returned by any W4GL system methods.
**
** The messages are "dummy" messages: they will never actually be printed.
** Introducing them appeared to be the most bullet-proof way of ensuring
** that they (ER_USER1 through ER_USER10) will never conflict with any real
** error codes.
*/
S_OM0091_USER1	"User routine returned ER_USER1\n"
S_OM0092_USER2	"User routine returned ER_USER2\n"
S_OM0093_USER3	"User routine returned ER_USER3\n"
S_OM0094_USER4	"User routine returned ER_USER4\n"
S_OM0095_USER5	"User routine returned ER_USER5\n"
S_OM0096_USER6	"User routine returned ER_USER6\n"
S_OM0097_USER7	"User routine returned ER_USER7\n"
S_OM0098_USER8	"User routine returned ER_USER8\n"
S_OM0099_USER9	"User routine returned ER_USER9\n"
S_OM009A_USER10	"User routine returned ER_USER10\n"

/*
** The following describe the type of file that could not be opened
** in message E_OM0082_Open_Error.
*/
F_OM0003_Input	"input"
F_OM0004_Output	"output"
F_OM0005_Error	"error"

E_OM009E_BADSTATE	"Improper state for requested operation on query object."
E_OM009F_TFILE		"Cannot open temporary file for query object cache."
E_OM00A0_TFWRITE	"Write failed - query object cache."
E_OM00A1_MEM		"Cannot allocate memory for query object."
E_OM00A2_BLAH		"Inconsistency in query object cols array."
E_OM00A3_TFREAD		"Read failed - query object cache."
E_OM00A4_NOPREV		"No previous record."
E_OM00A5_DCON		"Data conversion error when filling in target."
E_OM00A6_COL		"Bad column information."
E_OM00A7_BRK		"Bad break level."
E_OM00A8_NARR		"Null array pointer."
E_OM00A9_DYNCOMP	"Expressions failed to compile."
E_OM00AA_NOSCOPE	"No scope provided."
E_OM00AB_PARMS		"Parameter substitution failed."
E_OM00AC_LOADMAP	"Could not load row using provided expressions."
E_OM00AE_NONAME		"Query name has not been provided."
E_OM00AF_MISMATCH_COL_CNT	"Error occurred while checking query object.\n
Inconsistency in query object cols array.
The number of entries in the QueryObject.Columns[] array differs from
the number of target-list columns produced by the DESCRIBE statement."
E_OM00B0_MISMATCH_COL_NAME	"Error occurred while checking query object.\n
Inconsistency in query object cols array.
One of the QueryObject.Columns[].AsName entries differs from
the corresponding target-list column name produced by the DESCRIBE statement."
E_OM00B1_ALREADY_OPEN	"Query object is already open."
E_OM00B2_NOROW		"No row to load."
E_OM00B3_ERR0		"Error in %0c method on %1c."
E_OM00B4_ERR1		"Error in %0c method on %1c, %2c."
E_OM00B5_ERR2		"Error in %0c method on %1c, %2c, %3c."
E_OM00B6_ERR3		"Error in %0c method on %1c, %2c, %3c, %4c."

E_OM00B7_NOOP		"No operation waiting to commit."

E_OM00B8_LESS2		"Less than 2 columns in crosstab query."
E_OM00B9_DCONH		"Data conversion error in converting a handle."
E_OM00BA_BSIDX		"Bad level or index in BREAKSPEC."
E_OM00BB_FMTSTR		"Bad format string in BREAKSPEC."
E_OM00BC_NODBSESS	"No current DB session."
E_OM00BD_QTEXT		"Cannot format query text."

E_OM00BE_BADSYSEXEC	
"Cannot instantiate system package.  Windows4GL will exit due to this 
internal error.  Please report the problem to your Site Technical Contact."

E_OM00BF_SEQNOTAB     "Sequenced value column doesn't point to a table."

E_OM00C0_NOSUPERSEDE	"An attempt was made by user class %0c to supersede the method %1c of system class %2c."

/*%
** Explanation:
**      The user has defined a method that has the same name as a method
**  defined for a system class that is somewhere in the superclass chain.
**
** User Action:
**      Rename or delete the offending method.
*/

E_OM00C1_NOTSAMETYPE    "An attempt was made by user class %0c to supersede the method %1c of class %2c with a method of different type."

/*%
** Explanation:
**      The user has defined a method that is superseding the one that it
**  is inheriting from a parent user class, but which has a different return
**  type or privacy attribute.
**
** User Action:
**      Change the offending method so that it has the same characteristics
**  as the superseded one.
*/

E_OM00C2_INCORRARGS_S	 "Sort: incorrect arguments."

E_OM00C3_NONEXISTATT_S	 "Sort: attribute name %0c  doesn't exist."

E_OM00C4_NOTACCEPTATT_S  "Sort: Object type %0c  not acceptable."

E_OM00C5_ILLEGALATT_S    "Sort: Illegal attribute type %0c."

E_OM00C6_ILLEGALATT_F    "Find: Illegal attribute type %0c."

E_OM00C7_EMPTYARGLIST_S  "Sort: empty argument list."

E_OM00C8_ILLEGALORDER_S  "Sort: illegal sort order."

E_OM00C9_INCORRARGS_F    "Find: incorrect arguments."

E_OM00CA_NOATTPASSED_F   "Find: No attribute name passed to find."

E_OM00CB_VALSTRBOTH_F    "Find: Both VALUE and SEARCHSTRING passed."

E_OM00CC_VALSTRNONE_F    "Find: Neither VALUE nor SEARCHSTRING passed."

E_OM00CD_NONEXISTATT_F   "Find: attribute name %0c  doesn't exist."

E_OM00CE_VALEXPECTED_F   "Find: VALUE argument expected for this attribute."

E_OM00CF_INCMPTTYPES_F   "Find: VALUE arg. and attribute have incompatible types"

E_OM00D0_ILLEGALFMT_F    "Find: incorrect format."

E_OM00D1_INCMPTTYPES_F   "Find: (Formatted) search string and attribute \n
have incompatible types."

E_OM00D2_STARTROW_F	 "Find: STARTROW must be positive when USEDELETED \n
is either FALSE or is not specified."

E_OM00D3_ENDROW_F	 "Find: ENDROW must be positive when USEDELETED \n
is either FALSE or is not specified."

E_OM00D4_ZERO	"Zero rows affected by query."

E_OM00D5_TOOMANY	"Too many rows affected by query."

E_OM00D6_ATTR_CRASH "An exception occurred while trying to evaluate \n
attribute %0c of class %1c.  This error should be reported to your Ingres representative."

/*%
** Explanation:
**	The method used to compute an attribute encountered an exception,
**	probably while during examination by the debugger.
**
** System Status:
**      The attribute is unavailable.  The debugger simply will not display
**	this attribute.
**
** User Action:
**	Report this to you INGRES technical representative.
*/

E_OM00D7_EXPR_CRASH "An exception occurred while trying to process \n
expression '%0c'.  This error should be reported to your Ingres representative."

/*%
** Explanation:
**	An exception occurred while performing an examine or assign
**	from the debugger.
**
** System Status:
**      The will treat the situation as a runtime error and continue.
**
** User Action:
**	Report this to your INGRES technical representative.
*/

E_OM00D8_NULLPARAM	"Null value passed to method\n
A NULL value was passed to the parameter '%0c' which expects a
non-NULL value."

/*%
**
** Arguments:
**	0	The name of the parameter that was passed a NULL value.
**
** Explanation:
**	A parameter whose type is simple (that is, not an object)
**	was passed a NULL value.
**
** System Status:
**	The exact action will depend on whether the parameter is
**	needed.  Normally a default value will be used.
**
** User Action:
**	Pass a non-NULL value to this parameter.
*/


E_OM00D9_NOSCOPE	"Attempt to examine or assign with no valid scope available."

/*%
** Explanation:
**	Debugger attempted to use examine or assign while not focused on
**	a script which provides a usable scope (script not executing in
**	debugger's thread).  4GL should actually prevent this, anyway.
**
** System Status:
**	Simply an error in examining expression.
**
** User Action:
**	Focus on an executing script before using examine / assign.
*/

E_OM00DA_BAD_BLK_SEG_OFF "Large block has unexpected offset within segment\n
The INIT method for SESBLK allocated a large block and found that its offset
within its first segment (%0d) was not in the range 0-64.
Windows/4GL will continue, but attributes within large objects and
variables within large procedures may not behave correctly.
This error should be reported to your Ingres representative."

/*%
**
** Arguments:
**	0	The unexpected offset that was found.
**
** Explanation:
**	A block larger than 64K was allocated, but its starting location
**	is not within the first 64 bytes of a segment.
**
** System Status:
**	Windows/4GL will continue, but attributes within large objects and
**	variables within large procedures may not behave correctly.
**
** User Action:
**	Try restarting Windows/4GL and see if the error recurs.
**	In any case, contact your Ingres representative.
*/

S_OM00DB_PREMATURE_EOF_REMOVED 
"Removed premature end-of-file characters found in script."

/*%
**
** Arguments:
**	none
**
** Explanation:
**
** System Status:
**
** User Action:
**	
*/

E_OM00DC_NOTARGETS
"A DBInsert() or DBUpdate() failed because no appropriate targets\n
have been specified for this QUERYOBJECT."

/*%
**
** Arguments:
**      none
**
** Explanation:
**
** System Status:
**
** User Action:
**
*/

E_OM00DD_UNKNOWN_EXTCLASS
"Unable to resolve the class '%0c.'  Please add the external library \
that contains the class."

/*%
**
** Arguments:
**      class name
**
** Explanation:
**		The external class cannot be resolved.
**
** System Status:
**
** User Action:
**		Add the external library that implements the class.
**
*/

E_OM00DE_DUPLICATE_STKOVFLOW 
"UserClassObject Duplicate fails due to stack overflow.\n
An attempt was made to duplicate an object of type %s. \n
This failed after %d levels of recursion."

/*%
** Arguments:
**	0	The name of the type of object being duplicated.
**
**	1	The number of levels of recursion (clonelevel)
**
** Explanation:
**	A Duplicate of a Userclassobject failed because of stack overflow due
**	to too many levels of recursion.
**
** System Status:
**	No specific action is taken.
**
** User Action:
**	Need to either not do a duplicate or reorganize userclassobject so it
**	does not have so many levels of recursion.
*/

E_OM00DF_USEROBJFREE_STKOVFLOW 
"UserClassObject Free fails due to stack overflow.\n
An attempt was made to free an object of type %s. \n
This failed after %d levels of recursion."

/*%
** Arguments:
**	0	The name of the type of object being freed.
**
**	1	The number of levels of recursion (freelevel)
**
** Explanation:
**	A Free of a Userclassobject failed because of stack overflow due
**	to too many levels of recursion.
**
** System Status:
**	No specific action is taken.
**
** User Action:
**	Need to reorganize userclassobject so it does not have so many levels of
**	recursion.
*/

E_OM00E0_DUPLICATE_STKOVFLOW 
"UserClassObject Duplicate failed due to stack overflow.\n
The application may crash if the stack overflow is repeated.\n
Please save work and close the application."

/*%
** Arguments:
**	none
**
** Explanation:
**	A Duplicate of a Userclassobject failed because of stack overflow due
**	to too many levels of recursion.
**
** System Status:
**	A second occurrence of this error may cause a crash.
**
** User Action:
**	Save current work and close the application.
*/

E_OM00E1_USEROBJFREE_STKOVFLOW 
"UserClassObject Free failed due to stack overflow.\n
The application may crash if the stack overflow is repeated.\n
Please save work and close the application."

/*%
** Arguments:
**	none
**
** Explanation:
**	A Free of a Userclassobject failed because of stack overflow due
**	to too many levels of recursion.
**
** System Status:
**	A second occurrence of this error may cause a crash.
**
** User Action:
**	Save current work and close the application.
*/

E_OM00E2_SOBADFILE_INVALIDUTF8	"Error while setting FileHandle for a StringObject.\n
An error occurred while setting the FileHandle of a StringObject to the value
'%0c'.  Make sure that the file '%0c' dose not contain any invalid UTF8 characters."

/*%
** Parameters:
**	0	This is the name of the file that the FileHandle was set
**		to.
**
** Explanation:
**	While setting the FileHandle for a StringObject, there was some error
**	in reading the file that was given as the value of the FileHandle.
**
** System Status:
**	The StringObject will be cleared of its old value and its FileHandle
**      will be set to the file name given.
**
** User Action:
**	Make sure the file does not contain any invalid UTF8 characters.
*/

E_OM00E3_OPENERR_UTF8	"Couldn't open database '%0c'.\n
The database '%0c' could not be opened because it is not UTF8 enabled."

/*%
** Parameters
**	1	The name of the database that couldn't be opened.
**
** Explanation:
**	The given database could not be opened because it is not
**	UTF8 enabled.
**
** System Status:
**	This depends on why the database is being opened.  If the database
**	is being opened to start running an application, the application
**	will not be run.
**
** User Action:
**	Specify a UTF8 database or make the databse UTF8 enabled.
*/

/*
** The messages E_OM00E4_KEY_NOT_FOUND and E_OM00E5_DUPLICATE_KEY are status codes.
** used to return from W4GL system class methods.
** The messages are "dummy" messages: they will never actually be printed.
*/
E_OM00E4_KEY_NOT_FOUND	"The key was not found."
E_OM00E5_DUPLICATE_KEY	"The key already exists."

E_OM00E6_INVALID_PARAM_VALUE	"Invalid parameter value: %0c"

/*%
**
** Arguments:
**	0	The detail description about the invalid parameter.
**
** Explanation:
**	A parameter was passed an invalid value.
**
** System Status:
**	An error status indicating failure will be returned.
**
** User Action:
**	Pass a correct value to this parameter.
*/

E_OM00E7_PRECOND_VIOLATION	"Precondition violation: %0c"

/*%
**
** Arguments:
**	0	The detail description of the violated precondition.
**
** Explanation:
**	A precondition required to run the method was not fulfilled.
**
** System Status:
**	An error status indicating failure will be returned.
**
** User Action:
**	Make sure all preconditions are fulfilled before invoking the method.
*/

E_OM00E8_INTERNAL_ERROR	"Internal error: %0c"

/*%
**
** Arguments:
**	0	The detail description of the internal error.
**
** Explanation:
**	An internal error occured during execution of the method.
**
** System Status:
**	An error status indicating failure will be returned,
**	or a "default" action will be performed.
**
** User Action:
**	If possible, prevent conditions that can cause the error.
*/

