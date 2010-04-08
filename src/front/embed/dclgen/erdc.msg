/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erdc.msg -	Message file for class DC
**
** Description:
**	This file defines the messages for class DC.
**
** History:
**	10-sep-1987 (barbara)
**		Created.
**	17-nov-1992 (lan)
**		Added E_DC000A_BADCHRSTR.
**	17-sep-1993 (sandyd)
**		Added E_DC000B_BYTETYPE warning.
**	20-apr-1994 (geri)
**		Added E_DC0011_COLNAME warning.
**	25-Jul-2007 (drivi01)
**		Updated E_DC0003_FILE error to provide a 
**		clearer message on Vista.
**	
**/

# define	_DC_CLASS	162



E_DC0001_OPENDB	"Unable to open database '%0c'."

/*
** Explanation:
**	This error will occur when the database specified as a command line
** 	argument does not exist.  There may be other reasons as to why a 
**	connection to the named database cannot be made.
**
** System Status:
**	Program exits.
**
** User Action:
**	If the named database is known to exist, try connecting to it 
** 	using the terminal monitor.   This will result in  a more explicit 
**	error message.
*/

E_DC0002_NOTABLE	"Table '%0c' does not exist in database '%1c'."

/*
** Explanation:
**	The table that the user wishes to be described as a data structure
** 	does not exist.
**
** System Status:
**	Program exits.
**
** User Action:
**	Check the table name used for correctness.
*/

E_DC0003_FILE	"Cannot open output file '%0c'.\nVerify that you have write privilege to this location."

/*
** Explanation:
**	This error occurs if the user does not have write permission in the 
**	location specified or has given an directory path that does not exist.
**
** System Status:
**	Program exits.
**
** User Action:
**	Check that a file can be opened in the location specified.
*/

E_DC0004_OPNTABLE	"An error occurred while trying to open table '%0c'."

/*
** Explanation:
**	A DBMS error occurred during the opening of the specified table.
**
** System Status:
*	Program exits.
**
** User Action:
**	Try running the program again.  There may be a temporary condition
**	that is preventing the table from being opened.
*/

E_DC0005_SQLTYPE	"Internal conversion error with SQL data type %0d, data size %1d."

/*
** Explanation:
**	The internal data type of one or more of the columns in the table to be
**	described cannot be converted to an SQL datatype.   This is probably
**	caused by an internal error.
**
** System Status:
**	Program exits.
**
** User Action:
**	Contact your INGRES technical support representative.
*/

E_DC0006_FLAG	"Unknown flag '%0c'."

/*
** Explanation:
**	An unknown flag was used on the command line.
**
** System Status:
**	Program exits.
**
** User Action:
**	Check validity of command line arguments.
*/

E_DC0007_MANYARGS	"Too many command line arguments."
/*
** Explanation:
**	DCLGEN takes five arguments: language, database, table, file and
**	structure-name.  If more than 5 are given, it is an error.
**
** System Status:
**      Program exits.
**
** User Action:
**	Check validity of command line arguments.
*/

E_DC0008_BADNAME	"Illegal database/table name '%0c'."

/*
** Explanation:
**	INGRES database and table names may consist of between 1 and 24
**	characters.  They must start with an alpha character or an
**	underscore followed by alpha-numeric characters or underscores.
**	They may contain no special characters.
**
** System Status:
**	Program will prompt for valid database name.
**
** User Action:
*/

E_DC0009_BADLANG	"Unknown or unsupported language '%0c'."

/*
** Explanation:
**	The user has specified a host language that is not supported by
**	the DCLGEN utility.
**
** System Status:
**	Program will prompt for valid language name.
**
** User Action:
**	Check spelling of language.
*/

E_DC000A_BADCHRSTR	
"Warning: Table '%0c' contains column(s) of unlimited length.
Character string(s) of zero length have been generated and this will cause
a compile-time error.  You need to modify the output file to specify an
appropriate length."

/*
** Explanation:
**	For large object columns which have been declared with no maximum
**	length specifier, rather than attempt to determine the largest variable
**	allowed for each machine, DCLGEN will issue an error message and
**	generate a character string variable of zero length.  The error
**	message will be displayed to the Standard Output stream, but will not
**	be included in the generated file.
**
** System Status:
**	Program will continue.
**
** User Action:
**	The user will need to modify the output file and specify an appropriate
**	length since a character string of zero length will generate a compile-
**	time error.
*/

E_DC000B_BYTETYPE	
"Warning: Table '%0c' contains column(s) of 'byte', 'byte varying', or
'long byte' datatype(s).  These have been mapped to character string 
variables.  If your application does inserts or updates of byte datatypes 
across a heterogeneous INGRES/NET connection, you must use Dynamic SQL and 
specify an appropriate byte datatype in the sqlvar entries of your SQLDA to
ensure that the data does not go through character-set translation when
transmitted from client machine to server machine."
/*
** Explanation:
**	In non-C languages there are no embedded types that uniquely correspond
**	to 'byte', 'byte varying', or 'long byte' datatypes.  Those types are 
**	mapped to character string variables.  This works fine for retrieval
**	of byte data, and even works for inserts or updates when the client and
**	server machines use identical character sets.
**
**	However, when your application does inserts or updates of byte data
**	across heterogeneous INGRES/NET, you must use Dynamic SQL and specify
**	an appropriate byte datatype in the sqlvar entries of your SQLDA.
**	That is the only way to inform ESQL that you are sending "byte" data
**	and not character data.  By default, data from character string 
**	variables will be sent as character data, and may go through
**	character-set translation when transmitted from client machine to 
**	server machine.  To prevent that translation, you must explicitly
**	identify it as a "byte" datatype in the SQLDA.
**
**	This warning message will be displayed to the Standard Output stream, 
**	but will not be included in the generated file.
**
** System Status:
**	Program will continue.
**
** User Action:
**	For insert or update operations on "byte" data, the user may need 
**	to use Dynamic SQL, specifying the appropriate "byte" datatype in
**	the sqlvar entries of the SQLDA.
*/

E_DC0010_BADCOBPREC	"Precision '%0d' for column '%1c' exceeds COBOL limit
of 18.\n DCLGEN will generate a decimal with size (18,8) instead."

/*
** Explanation:
**	The table includes a column with a decimal data type that exceeds 18,
**	the maximum allowable precision for a COBOL variable.  To avoid
**	the user getting a COBOL compiler error, DCLGEN will generate a 
**	a decimal variable with a precision of 18 and a scale of 8.
**
** System Status:
**	Program will continue.
**
** User Action:
**	Check the precision and scale of the column in question and make sure 
**	that the default precision and scale generated is sufficient.  If
**	it is not appropriate, the user can modify the output.
*/

E_DC0011_COLNAME	"Warning: column name '%0c' has been changed to
a valid variable name for your compiler. Please check DCLGEN output for
uniqueness of variable names."

/*
** Explanation:
**	Delimited ids may contain characters (ie spaces) which are not
**	legal for the compiler. DCLGEN will translate the illegal characters
**	to legal characters for the compilers. This warns the user, however,
**	that the variable name has been changed, and should be checked
**	for uniqueness.
**
** System Status:
**	Program will continue.
**
** User Action:
**	Check the DCLGEN output and make sure that the generated variable 
**	name is unique, and if not, the user should modify it.
*/

/* Slow messages begin here */

/*
**	S_DC00A1_WORK: Informs user that the program is in progress.
*/

S_DC00A1_WORK	"DCLGEN: Working . . ."

/*
**	S_DC00A2_DESC: Informs user that program is writing out data 
**	declaration to file.
*/

S_DC00A2_DESC	"DCLGEN: Describing table '%0c' . . ."

/*
**	S_DC00A3_COMMENT: This text represents a host language comment that
**	is generated into the output file along with the data structure.
*/

S_DC00A3_COMMENT	"Description of table %0c from database %1c"

/*
**	S_DC00A4_USAGE:
**	S_DC00A5_ARGS: These two messages are issued if the user types
**	in the DCLGEN command line with an incorrect number of arguments.
*/

S_DC00A4_USAGE	"DCLGEN Usage is:"
S_DC00A5_ARGS	"dclgen language database table file structure/prefix"

/*
**	S_DC00A6_ARGLANG:
**	S_DC00A7_ARGDB:
**	S_DC00A8_ARGTABLE:
**	S_DC00A9_ARGFILE:
**	S_DC00AA_ARGSTRUCT: These are used as command line prompts to the user.
*/

S_DC00A6_ARGLANG	"Language "
S_DC00A7_ARGDB	"Database "
S_DC00A8_ARGTABLE	"Table    "
S_DC00A9_ARGFILE	"File     "
S_DC00AA_ARGSTRUCT	"Structure"
S_DC00AC_ARGPREFIX	"Prefix   "

/*
**	S_DC00AB: This message is printed if the user has interrupted the 
**	program.
*/

S_DC00AB_ABORT	"DCLGEN: Aborting . . ."
