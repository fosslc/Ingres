/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ere4.msg -	Message file for class E4
**
** Description:
**	This file defines the messages for class E4.
**
** History:
**	27-aug-1987 (mark)
**		Created.
**	06-jul-1989 (teresa)
**		Added error message E_E40017 for duplicate data declaration
**		clauses.
**	23-oct-1990 (mgw)
**		Corrected a misspelling in a message.
**	09-feb-1992 (larrym)
**	    Added E_E40018_hcbNOSQLCODE.
**/

# define	_E4_CLASS	145



E_E40001_hcbAMBIG	"Ambiguous qualification of COBOL data item '%0c'."

/*%
** Explanation:
**	This data item is not sufficiently qualified in order to distinguish
**	it from another data item.  It is likely that that the data item
**	is an elementary member of a COBOL record or group.  To avoid 
**	reference ambiguity qualify the data item further by using IN or OF.
**	When using COBOL table subscripts (via parenthesis), the subscripted
**	item must be unambiguous when the left parenthesis is processed.
**
**	The preprocessor will generate code using the most recently declared
**	instance of the ambiguous data item.
**
** System Status:
**
** User Action:
*/

E_E40002_hcbBAD	"Unsupported COBOL numeric PICTURE string '%0c'."

/*%
** Explanation:
**	An invalid picture character was encountered while processing a
**	numeric picture string.  A numeric picture string may include:
**
**		'S', '9', '(', ')' and 'V'
**
**	The preprocessor will treat the data item as though it was declared:
**
**		PICTURE S9(8) USAGE COMP.
**		
** System Status:
**
** User Action:
*/

E_E40003_hcbCOMPLEN	"COMP picture '%0c' requires too many storage bytes.\n
Try USAGE COMP-3."

/*%
** Explanation:
**	The COMPUTATIONAL data type must fit into a maximum of 4 bytes.
**	Numeric integers of more than 9 digits require VAX quad-word integer
**	storage (8 bytes), which is incompatible with the INGRES internal
**	run-time data types.  Try reducing the picture string or declaring
**	the data item as COMP-3 or COMP-2 which is compatible with INGRES
**	floating point data.
**
**	An exception is made to allow non-scaled 10-digit numeric picture
**	strings (PICTURE S(10) USAGE COMP), which is representable by a
**	4-byte integer.
**
** System Status:
**
** User Action:
*/

E_E40004_hcbDECLARE	"No ## DECLARE before first EQUEL statement '%0c'."

/*%
** Explanation:
**	You must issue the ## DECLARE statement before the first embedded
**	statement.  The preprocessor generates code that references data
**	items declared within a file copied by the ## DECLARE statement.
**	Without issuing the ## DECLARE statement, the COBOL compiler will
**	not accept those references.
**	
** System Status:
**
** User Action:
*/

E_E40005_hcbELEM	"'%0c' is not an elementary data item. Records cannot be used."

/*%
** Explanation:
**	Within this usage, COBOL records or tables cannot be used. In order
**	to use this data item you must refer to an elementary data item that
**	is a member of the record, or an element of the COBOL table.
**
** System Status:
**
** User Action:
*/

E_E40006_hcbLEVEL	"COBOL declaration level %0c is out of bounds."

/*%
** Explanation:
**	Only levels 01 through 49 and 77 are accepted for COBOL data item
**	declarations.  Level numbers outside of this range will be treated
**	as though they are level 01.
**
**	Syntax errors caused within leading clauses of a COBOL declaration
**	may cascade and generate this error message for the OCCURS and VALUE
**	clauses of the erroneous declaration.
**
** System Status:
**
** User Action:
*/

E_E40007_hcbNEEDPIC	"Data item requires a PICTURE string in this USAGE."

/*%
** Explanation:
**	The specified USAGE clause requires a COBOL PICTURE string in order
**	to determine preprocessor data item type information.  Not all USAGE
**	clauses require a PICTURE string.  Data items with USAGE COMP, COMP-3
**	and DISPLAY do require a PICTURE string.  If no PICTURE string is
**	specified the preprocessor will treat the data item as though it
**	was declared:
**
**		PICTURE X(10) USAGE DISPLAY.
**
** System Status:
**
** User Action:
*/

E_E40008_hcbNOPAR	"Data item on level %0c has no parent of lesser level."

/*%
** Explanation:
**	A data item declared on a level that is greater than the level of the
**	most recently declared data item is considered to be a subordinate
**	member of that group.  The previous level, therefore, must be the level
**	number of a COBOL record or group declaration.  This is typical with
**	a COBOL record containing a few elementary data items.
**
**	A data item declared on a level that is less than the level of the
**	most recently declared data item is considered to be on the same level
**	as the "parent" of that data item.
**
**	Level numbers violating this rule will be treated as though they 
**	are level 01.
**
** System Status:
**
** User Action:
*/

E_E40009_hcbNOPIC	"Keyword PICTURE and the describing string must be on the same line."

/*%
** Explanation:
**	When the preprocessor scans the COBOL PICTURE string, it must find
**	the PICTURE key word and the corresponding string description
**	on the same line in the source file.  The PICTURE word and the
**	string may be separated by the IS key word.  The preprocessor will treat
**	the declaration as though there was no PICTURE clause.
**
** System Status:
**
** User Action:
*/

E_E4000A_hcbNOVAR	"'%0c' is not a legally declared data item."

/*%
** Explanation:
**	The specified data item must has not been declared but has been
**	used in place of a COBOL variable within an embedded statement.
**
** System Status:
**
** User Action:
*/

E_E4000B_hcbNUMDISPLAY	"Unsupported PICTURE '%0c' is numeric-display.\n
USAGE COMP assumed."

/*%
** Explanation:
**	Some versions of the COBOL preprocessor do not support numeric
**	display data items.  For example:
**
**		PICTURE S9(8) USAGE DISPLAY.
**
**	If this is the case, you should use COMPUTATIONAL data items and
**	assign to and from DISPLAY items before using the data item within
**	embedded statements.
**
** System Status:
**
** User Action:
*/

E_E4000C_hcbOCCURS	"COBOL OCCURS clause is not allowed on level 01."

/*%
** Explanation:
**	The OCCURS clause must be used with a data item that is declared
**	on a level greater than 01.  This error is only a warning, and 
**	treats the data item correctly (as though declared as a COBOL table).
**	A warning may also be generated by the COBOL compiler.
**
** System Status:
**
** User Action:
*/

E_E4000D_hcbPARAM	"EQUEL/COBOL does not support PARAM target lists."

/*%
** Explanation:
**	This feature is not documented and should not be used with EQUEL
**	COBOL.
**
** System Status:
**
** User Action:
*/

E_E4000E_hcbPICLONG	"PICTURE '%0c' is too long.  The maximum length is %1c."

/*%
** Explanation:
**	COBOL PICTURE strings must not exceed the maximum length specified
**	in the error message.  Try to collapse consecutive occurrences of
**	the same PICTURE symbol into a "repeat count".  For example:
**		
**		PICTURE S99999999 becomes PICTURE S9(8)
**
** System Status:
**
** User Action:
*/

E_E4000F_hcbPICNUM	"PICTURE '%0c' contains non-integer repeat count, %1c."

/*%
** Explanation:
**	A COBOL "repeat count" within a PICTURE string was either too long 
**	or was not an integer.  The preprocessor treats the data item as
**	though declared with a PICTURE with a repeat count of 1.  For example:
**
**		S9(1) or X(1).
**
**
** System Status:
**
** User Action:
*/

E_E40010_hcbREDECLARE	"Reissue of ## DECLARE. Second time ignored."

/*%
** Explanation:
**	Unused Error!
**
**
** System Status:
**
** User Action:
*/

E_E40011_hcbUSAGE	"USAGE type '%0c' is not supported."

/*%
** Explanation:
**	This usage type is currently not supported.
**
** System Status:
**
** User Action:
*/

E_E40012_hcbS2PIC	"PICTURE '%0c' has two sign symbols (S)."

/*%
** Explanation:
**	The specified numeric PICTURE string has two sign symbols.
**	The preprocessor will treat the data item as though it was declared:
**
**		PICTURE S9(8) USAGE COMP.
**
** System Status:
**
** User Action:
*/

E_E40013_hcbV2PIC	"PICTURE '%0c' has two decimal point symbols (V)."

/*%
** Explanation:
**	The specified numeric PICTURE string has two decimal point symbols.
**	The preprocessor will treat the data item as though it was declared:
**
**		PICTURE S9(8) USAGE COMP.
**
** System Status:
**
** User Action:
*/

E_E40014_hcbCONTIN	"Missing quotation mark on continued string literal."

/*%
** Explanation:
**	The first non-blank character of a continued string literal must
**	be a quotation mark in the indicator area.  A missing quotation mark
**	in the continued string literal or the wrong quotation mark will
**	generate this error.
**
** System Status:
**
** User Action:
*/

E_E40015_hcbMUSTINDIR	"COBOL data item '%0c' is a table and must be subscripted."

/*%
** Explanation:
**	The data item is a COBOL table and must be subscripted in order to 
**	yield an elementary data item to retrieve or set INGRES data.
**
** System Status:
**
** User Action:
*/

E_E40016_hcbCANTINDIR	"COBOL data item '%0c' is not a table and must not be subscripted."

/*%
** Explanation:
** 	You have included subscripts when referring to a data item that was
**	not declared as a COBOL table.
**
** System Status:
**
** User Action:
*/

E_E40017_hcbDUPL	"Duplicate COBOL data declaration '%0c' clause found."

/*%
** Explanation:
** 	You have included either a duplicate USAGE, PICTURE, or OCCURS data
**	declaration clause when declaring a data item.
**
** System Status:
**
** User Action:
*/

E_E40018_hcbNOSQLCODE	"SQLCODE has been declared, but the -sqlcode flag
was not specified on the command line."
/*%
** Explanation:
**      Because the standard SQLCA include file declares an SQLCODE,
**      which would duplicate the one you've just declared, you must
**      use the -sqlcode flag.  This flag tells the preprocessor that
**      the application has already declared an SQLCODE before it tries
**      to include the SQLCA.
**
** System Status:
**      You will likely get a multiply defined symbol error, either from
**      the fortran compiler or from the linker.
**
** User Action:
**      recompile your application using the -sqlcode flag.  See the COBOL
**      companion guide (and possibly the release notes) for information on
**      command line options.
*/
