/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
#ifndef AFE_H_INCLUDED
#define AFE_H_INCLUDED
#include	<eraf.h>

/**
** Name:	afe.h -	Front-End ADF Interface Definitions File.
**
** Description:
**	Contains all the definitions that make up the Front-End ADF interface
**	(AFE.)  These definitions are required to use any of the AFE routines.
**	Also defined in this file are the error number definitions and the
**	following types and macros:
**
** 	AFE_DCL_TXT_MACRO	declare a text string variable.
** 	DB_USER_TYPE		a user specified type string.
** 	AFE_OPERS		operands to a function.
**	AFE_DCMP_INIT_MACRO()	compiler initialize a DB_DATA_VALUE.
**	AFE_NULLABLE()		is the data type nullable?
**	AFE_MKNULL()		make the data value nullable.
**	AFE_UNNULL()		make the data value not nullable.
**	AFE_SET_NULLABLE()	set the data value nullable.
**	AFE_ISNULL()		determine if data dalue is null.
**	AFE_DATATYPE()		return base data type for data value.
**	AFE_TRIMBLANKS_MACRO()	return length of str data w/o trailing blanks.
**
** History:
**	Revision 6.4  89/08  wong
**	Added PM constants.  Added 'AFE_ISNULL()' and 'AFE_DATATYPE()' macros;
**	rename 'AFE_SET_NULL()' to 'AFE_SET_NULLABLE()'.
**	90/09  Added 'AFE_SETNULL()' and 'AFE_CLRNULL()'
**
**	Revision 6.2  89/02  wong
**	Added 'AFE_SET_NULL()'.
**
**	Revision 6.0  87/02  daver
**	Initial revision.
**	30-mar-1987 (Joe)
**		Added ADF_AGNAMES datatype.
**	25-nov-87 (bruceb)
**		Added datatype class definitions (CHAR_DTYPE,
**		NUM_DTYPE, DATE_DTYPE, ANY_DTYPE).
**	04-sep-90 (bruceb)
**		Added AFE_TRIMBLANKS_MACRO.  Moved from afe!afcvinto.c which
**		copied it from adf.h.  It was named ADF_TRIMBLANKS_MACRO.
**	09/01/92 (dkh) - Added declaration of IIAFfedatatype().
**	23-feb-93 (mohan)
**		added tags to structure for proper automatic protyping tool.
**	12-mar-93 (fraser)
**		Changed structure tag names: they shouldn't begin with
**		underscore.
**      22-feb-96 (lawst01)
**	   Windows 3.1 port changes - added some fcn prototypes.
**	09-apr-1996 (chech02)
**	   Windows 3.1 port changes - add more function prototypes.
**	28-jun-1996 (thoda04)
**	   Windows 3.1 port changes - add still more function prototypes.
**	18-dec-1996 (canor01)
**	   Move definition of AGNAMES structure from afe.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-mar-2001 (abbjo03)
**	    Add AFE_NTEXT_STRING to support Unicode.
**	05-dec-2006 (gupsh01)
**	    Increased the size of DB_TYPE_SIZE from 24 to 32 to support
**	    longer datatype names in ADF eg TIMESTAMP WITH LOCAL TIME ZONE 
**	    etc.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	    Drop the old WIN16 section, misleading and out of date.  Better
**	    to just reconstruct as needed.
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary to eliminate
**	    gcc 4.3 warnings.
**     13-Aug-2010 (hanal04) Bug 124250
**        Moved AFE_NTEXT_STRING from afe.h to iicommon.h
**/


/*: 
** AFE Error Codes.
**	These are all of the error codes that can be returned from
**	the AFE library. In addition, AFE routines can also return
**	an ADF error code. See adf.h for a list of all ADF error codes.
**	
**	You'll notice that the error messages were commented out. 
**	They can now be found in eraf.h
*/ 
    /*  Numbers 0 to 3FFF are user errors; 4000 to FFFF are internal errors */
#define		E_AF_MASK			(i4)(0x00200000)
#define		AFE_FMT_MASK			(i4)(0x0000FFFF)

    /*	Numbers 0 to 0FFF are reserved for future (?) less-than-warnings */
    /*	Numbers 1000 to 1FFF have severity level of WARNING */
#define		AFE_WARN_BOUNDARY		(i4)(0x1000)

    /*	Numbers 2000 to 2FFF have severity level of ERROR */
#define		AFE_ERROR_BOUNDARY		(i4)(0x2000)

    /*  Numbers 3000 to 3FFF have severity level of FATAL */
#define		AFE_FATAL_BOUNDARY		(i4)(0x3000)

    /*  Numbers 0 to 3FFF are user errors; 4000 to FFFF are internal errors */
    /*	Numbers 4000 to 4FFF are reserved for future (?) less-than-warnings */
#define		AFE_INTERNAL_BOUNDARY		(i4)(0x4000)

    /*	Numbers 5000 to 5FFF have severity level of WARNING */
/*	 	E_AF5000_VALUE_TRUNCATED	(i4)(E_AF_MASK + 0x5000) */

    /*	Numbers 6000 to 6FFF have severity level of ERROR */ 
/*	 	E_AF6001_WRONG_NUMBER		(i4)(E_AF_MASK + 0x6001) */
/*	 	E_AF6002_INVALID_TYPE		(i4)(E_AF_MASK + 0x6002) */
/*	 	E_AF6004_NOT_LONGTEXT		(i4)(E_AF_MASK + 0x6004) */
/*	 	E_AF6005_SMALL_BUFFER		(i4)(E_AF_MASK + 0x6005) */
/*	 	E_AF6006_OPTYPE_MISMATCH	(i4)(E_AF_MASK + 0x6006) */
/*	 	E_AF6007_INCOMPATABLE_OPID	(i4)(E_AF_MASK + 0x6007) */
/*	 	E_AF6008_AMBIGUOUS_OPID		(i4)(E_AF_MASK + 0x6008) */
/*	 	E_AF6009_BAD_MECOPY		(i4)(E_AF_MASK + 0x6009) */
/*	 	E_AF600A_NAME_TOO_LONG		(i4)(E_AF_MASK + 0x600A) */
/*	 	E_AF600B_BAD_LENGTH		(i4)(E_AF_MASK + 0x600B) */
/*	 	E_AF600C_NOT_TEXT_TYPE		(i4)(E_AF_MASK + 0x600C) */
/*	 	E_AF600D_BAD_ASCII		(i4)(E_AF_MASK + 0x600D) */
/*	 	E_AF600E_NO_DBDV_ROOM		(i4)(E_AF_MASK + 0x600E) */
/*	 	E_AF600F_NO_CLOSE_PAREN		(i4)(E_AF_MASK + 0x600F) */
/*	 	E_AF6010_XTRA_CH_PAREN		(i4)(E_AF_MASK + 0x6010) */
/*	 	E_AF6011_XTRA_CH_NUM		(i4)(E_AF_MASK + 0x6011) */
/*	 	E_AF6012_BAD_TRAIL_CH		(i4)(E_AF_MASK + 0x6012) */
/*	 	E_AF6013_BAD_NUMBER		(i4)(E_AF_MASK + 0x6013) */
/*	 	E_AF6014_BAD_ERROR_PARAMS	(i4)(E_AF_MASK + 0x6014) */
/*	 	E_AF6015_CANT_CONVERT_NUMERIC   (i4)(E_AF_MASK + 0x6015) */
/*	 	E_AF6016_BUFFER_TOO_SMALL	(i4)(E_AF_MASK + 0x6016) */
/*	 	E_AF6017_CANT_FIND_COERCE	(i4)(E_AF_MASK + 0x6017) */
/*	 	E_AF6018_ZERO_IN_TEXT		(i4)(E_AF_MASK + 0x6018) */
/*	 	E_AF6019_BAD_OPCOUNT		(i4)(E_AF_MASK + 0x6019) */
/*	 	E_AF601A_NOT_AGG_FUNC		(i4)(E_AF_MASK + 0x601A) */
/*	 	E_AF601B_MORE_AGG_NEED		(i4)(E_AF_MASK + 0x601B) */
/*	 	E_AF601C_BAD_AGG_INIT		(i4)(E_AF_MASK + 0x601C) */
/*	 	E_AF601D_NO_LEN_CHAR		(i4)(E_AF_MASK + 0x601D) */
/*	 	E_AF601E_NOT_VCHR_TYPE		(i4)(E_AF_MASK + 0x601E) */

    /*  Numbers 7000 to 7FFF have severity level of FATAL */

#define		AFE_USER_ERROR		1
#define		AFE_INTERNAL_ERROR	2
#define		MAX_ERR_PARAMS		6

#define AFE_DATESIZE	25	/* maximum string length for date string */


/*
** Name:	AFE_PM -	Pattern Match Options/Results.
**
** Descriptions:
**	Pattern match translation options and result definitions.
**
** History:
**	08/89 (jhw) -- Moved from <fmt.h>.
*/

/* The following are inputs to the pattern matching routines. */
#define		AFE_PM_NO_CHECK		0	/* no translation */
#define		AFE_PM_CHECK		1	/* translate */
#define		AFE_PM_GTW_CHECK	2	/* translate for Gateway */
#define		AFE_PM_SQL_CHECK	3	/* translate from SQL */

/* The following are ouputs from the pattern matching routines. */
#define		AFE_PM_NOT_FOUND	0
#define		AFE_PM_FOUND		1
#define		AFE_PM_USE_ESC		2
#define		AFE_PM_QUEL_STRING	3


/*{
** Name:	AFE_DCL_TXT_MACRO	- Declare a text string variable.
**
** Description:
**	This macro is used to declare a variable that has a structure
**	like an internal value of a TEXT type.  It is needed because the
**	typedef DB_TEXT_STRING only has room for one character.  This
**	macro can be used to declare a variable that can be type cast
**	into a DB_TEXT_STRING.  For example:
**
**		AFE_DCL_TXT_MACRO(256)	temptext;
**		DB_TEXT_STRING		*text;
**
**		text = (DB_TEXT_STRING *) temptext;
**		
**		 (text nows points to a text string that can hold
**		  256 bytes..)
**		
**
** Inputs:
**	size
**		The number of bytes the variable can hold.
**
**
**	AFE_DCL_TXT_MACRO(255)		tmptext;
**	DB_TEXT_STRING			*text;
**
**	...
**	text = (DB_TEXT_STRING *) &tmptext;
*/

# define	AFE_DCL_TXT_MACRO(size)	struct {u_i2 count; u_char text[size];}

/*{
** Name:	AFE_NULLABLE() -	Is a Data Type Nullable?
**
** Description:
**	Returns whether the input data type is Nullable.  (A type is Nullable
**	if a data value with that type can contain a Null value.)
**
**	A common example would be to pass the 'db_datatype' field of
**	a DB_DATA_VALUE:
**
**		if ( AFE_NULLABLE(dbv.db_datatype) )
**			...
**
** Inputs:
**	type	{DB_DT_ID}  The type to check.
**
** Returns:
**	{bool}	TRUE if the type is Nullable, FALSE otherwise.
**
** History:
**	13-feb-1987 (Joe)
**		First Written
**
*/ /* Usage:
bool
AFE_NULLABLE ( type )
DB_DT_ID	type;
{
}
*/

#define AFE_NULLABLE(type)	( (type) < 0 )
#define AFE_NULLABLE_MACRO(type) AFE_NULLABLE(type)

/*{
** Name:	AFE_MKNULL() -	Make a DB_DATA_VALUE Nullable.
**
** Description:
**	Turn the type and length information in a DB_DATA_VALUE
**	to its Nullable version.  (The Nullable version would be the
**	corresponding type that allows Null values.)
**
**	This call will change the type and length fields in the DB_DATA_VALUE,
**	if it was non-Nullable.  After the call, the length field can be used to
**	allocate storage for the Nullable data value.
**
**	If the type in the DB_DATA_VALUE is already Nullable (or has no data
**	type, DB_NODT) then the data value is not changed.
**
** Inputs:
**	dbv	{DB_DATA_VALUE *}  A data value to be made Nullable.
**			.db_datatype	{DB_DT_ID} The type of the data value.
**			.db_length	{i4}  The length of the value.
**
** Outputs:
**	dbv	{DB_DATA_VALUE *}  The modified data value.
**			.db_datatype	{DB_DT_ID}  The corresponding Nullable
**						data type for the data value.
**
**			.db_length	{i4}  The new length for the
**							Nullable data value.
**
** History:
**	13-feb-1987 (Joe)
**		First Written
*/ /* Usage:
VOID
AFE_MKNULL ( dbv )
DB_DATA_VALUE	*dbv;
{
}
*/
#define	AFE_MKNULL(dbv) ( \
	( (dbv)->db_datatype <= 0 ) \
		? 0 : ((dbv)->db_datatype *= -1, ((dbv)->db_length += 1)) \
)
#define AFE_MKNULL_MACRO(dbv) AFE_MKNULL(dbv)


/*{
** Name:	AFE_UNNULL() -	Make a DB_DATA_VALUE non-Nullable.
**
** Description:
**	Turn the type and length information in a DB_DATA_VALUE to its
**	non-Nullable version.  (The non-Nullable data value would be the
**	corresponding type that does not allow Null values.)
**
**	This call will change the type and length fields in the DB_DATA_VALUE,
**	if it was Nullable.  After the call, the length field can be used to
**	allocate storage for the non-Nullable data value.
**
**	If the type in the DB_DATA_VALUE is already non-Nullable (or has no
**	data type, DB_NODT) then the data value will not be changed.
**
** Inputs:
**	dbv	{DB_DATA_VALUE *}  A data value to be made non-Nullable.
**			.db_datatype	{DB_DT_ID} The type of the data value.
**			.db_length	{i4}  The length of the value.
**
** Outputs:
**	dbv	{DB_DATA_VALUE *}  The non-Nullable data value.
**			.db_datatype	{DB_DT_ID}  The corresponding
**						non-Nullable data type for the
**						data value.
**			.db_length	{i4}  The new length for the
**						non-Nullable data value.
**
** History:
**	13-feb-1987 (Joe)
**		First Written
*/ /* Usage:
VOID
AFE_UNNULL ( dbv )
DB_DATA_VALUE	*dbv;
{
}
*/
#define	AFE_UNNULL(dbv) ( \
	!AFE_NULLABLE((dbv)->db_datatype) ? 0 \
		: ((dbv)->db_datatype *= -1, ((dbv)->db_length -= 1)) \
)
#define AFE_UNNULL_MACRO(dbv) AFE_UNNULL(dbv)


/*{
** Name:	AFE_SET_NULLABLE() -	Set the Nullability of a Data Value.
**
** Description:
**	Sets whether a data value is Nullable as specified by the input
**	boolean parameter.
**
**	This call will change the type and length fields in the DB_DATA_VALUE,
**	if the Nullability of the data value changes.  After the call, the
**	length member can be used to allocate storage for the data value.
**
**	If the Nullability of the type in the DB_DATA_VALUE is already matches
**	the that of the data value (or has no data type, DB_NODT) then it will
**	not be changed.
**
** Inputs:
**	nullable	{bool}  Whether to set the data value Nullable or not.
**	dbv		{DB_DATA_VALUE *}  The data value to have its
**						Nullability set.
**				.db_datatype	{DB_DT_ID} The type of the data
**								value.
**				.db_length	{i4}  The length of the
**								value.
**
** Outputs:
**	dbv	{DB_DATA_VALUE *}  The modified data value.
**			.db_datatype	{DB_DT_ID}  The corresponding Nullable
**							or non-Nullable data
**							type for the data value.
**			.db_length	{i4}  The new length for the
**							data value.
**
** History:
**	02/89 (jhw) -- Written.
*/ /* Usage:
VOID
AFE_SET_NULLABLE ( nullable, dbv )
bool		nullable;
DB_DATA_VALUE	*dbv;
{
}
*/
#define AFE_SET_NULLABLE(nullable, dbv) ( \
	( (nullable) == AFE_NULLABLE((dbv)->db_datatype) || \
			(dbv)->db_datatype == DB_NODT ) \
		? 0 : ((dbv)->db_datatype *= -1, \
			((dbv)->db_length += (nullable) ? 1 : -1)) \
)

/*{
** Name:	AFE_ISNULL() -	Determine if Data Value is Null.
**
** Description:
**	Returns whether the data value is Null.
**
** Input:
**	v	{DB_DATA_VALUE *}  The data value.
**		    .db_datatype	{DB_DT_ID}  The data type.
**		    .db_data		{PTR}  The data.
**
** Returns:
**	{bool}  TRUE if the data value is Null; FALSE otherwise.
**
** History:
**	08/89 (jhw) -- Written.
*/ /* Usage:
bool
AFE_ISNULL ( v )
DB_DATA_VALUE	*v;
{
}
*/
#define  AFE_ISNULL(v)  ( AFE_NULLABLE((v)->db_datatype) && (*(((u_char *)(v)->db_data) + (v)->db_length - 1) & ADF_NVL_BIT) != 0 )

/*{
** Name:	AFE_SETNULL() -	Set Data Value Null.
**
** Description:
**	Sets the data value to be Null.
**
** Input:
**	v	{DB_DATA_VALUE *}  The data value.
**		    .db_datatype	{DB_DT_ID}  The data type.
**		    .db_data		{PTR}  The data.
**
** History:
**	09/90 (jhw) -- Written.
*/ /* Usage:
AFE_SETNULL ( v )
DB_DATA_VALUE	*v;
{
}
*/
#define  AFE_SETNULL(v)  if ( AFE_NULLABLE((v)->db_datatype) ) *(((u_char *)(v)->db_data) + (v)->db_length - 1) |= ADF_NVL_BIT;

/*{
** Name:	AFE_CLRNULL() -	Set Data Value Not Null.
**
** Description:
**	Sets the data value to be not Null.
**
** Input:
**	v	{DB_DATA_VALUE *}  The data value.
**		    .db_datatype	{DB_DT_ID}  The data type.
**		    .db_data		{PTR}  The data.
**
** History:
**	09/90 (jhw) -- Written.
*/ /* Usage:
AFE_CLRNULL ( v )
DB_DATA_VALUE	*v;
{
}
*/
#define  AFE_CLRNULL(v)  if ( AFE_NULLABLE((v)->db_datatype) ) *(((u_char *)(v)->db_data) + (v)->db_length - 1) &= ~ADF_NVL_BIT;

/*{
** Name:	AFE_DATATYPE() -	Return Base Data Type for Data Value.
**
** Description:
**	Returns the base data type for the data value.  That is, the
**	non-Nullable data type for the data value.
**
** Input:
**	v	{DB_DATA_VALUE *}  The data value.
**		    .db_datatype	{DB_DT_ID}  The data type.
**
** Returns:
**	{DB_DT_ID}  The data type.
**
** History:
**	08/89 (jhw) -- Written.
*/ /* Usage:
DB_DT_ID
AFE_DATATYPE ( v )
DB_DATA_VALUE	*v;
{
}
*/
#define AFE_DATATYPE(v) ( AFE_NULLABLE((v)->db_datatype) ? -(v)->db_datatype : (v)->db_datatype )


/*{
** Name:	AFE_DCMP_INIT_MACRO -	Compiler initialize a DB_DATA_VALUE
**
** Description:
**	This macro is used to Compiler initialize a DB_DATA_VALUE.  It's
**	main function is to hide dependancy on the positions of items in
**	the DB_DATA_VALUE structure, in case of changes.  This makes
**	compiler initializations as "change proof" as runtime code which
**	explicitly assigns items by name.
**
**	This macro substitutes for one instance of a DB_DATA_VALUE, ie.
**	what you would normally enclose in curly braces.  It does not
**	define a terminating character, so it may be used either in
**	initializing a single DB_DATA_VALUE or the elements of a compiler
**	initialized array.
**
** Inputs:
**	type	of DB_DATA_VALUE
**	length	of DB_DATA_VALUE
**	addr	of data area
**
** Example:
**
**	static f8 Myf8area;
**
**	static DB_DATA_VALUE Mydbd =
**			AFE_DCMP_INIT_MACRO(DB_FLT_TYPE,sizeof(f8),&myf8area);
**
** History:
**	3/87 (bobm) written
*/

# define AFE_DCMP_INIT_MACRO(type,length,addr)	{addr,length,type,0}

/*}
** Name:	DB_USER_TYPE	-	A user specified type string.
**
** Description:
**	This structure is used to contain all the information needed
**	from a user specified type (e.g. text(20) or varchar(20) with NULL).
**	The structure collects the information into a single structure
**	for convenience.
**
**	Different programs will use this structure.  How they fill it
**	in will depend on how they input the information from the user.
**	For example, in OSL, the parser will parse the OSL
**	declaration statements:
**		name [(size [,prec])] [WITH NULL | NOT NULL [WITH DEFAULT]]
**	and fill in the structure.
**	The table utilities will probably take the input in fields and
**	fill in the structure.
**
**	dbut_kind should be set to:
**		DB_UT_NULL	if the type allows NULLs.
**
**		DB_UT_NOTNULL	if the type does NOT allow NULLs or DEFAULTs.
**
**		DB_UT_DEF	if the type does NOT allow NULLs, but
**					has DEFAULTs
**
*/
typedef struct s_DB_USER_TYPE
{
	i4	dbut_kind;	/* The nullability and defaults for type */
# define	DB_UT_NULL	1
# define	DB_UT_NOTNULL	2
# define	DB_UT_DEF	3
# define	DB_TYPE_SIZE	32	/* Largest size of a type name? */
	char	dbut_name[DB_TYPE_SIZE];
} DB_USER_TYPE;


/*}
** Name:	AFE_OPERS	-	Operands to a function.
**
** Description:
**	This structure collects the operands for a function together
**	into a single structure.   Collecting them into a single structure
**	allows us to isolate all dependencies on number of operands to
**	an ADF function call in the AFE library.
**
**	afe_ops must point to an array of DB_DATA_VALUE pointers with
**	at least afe_opcnt elements.
*/
typedef struct s_AFE_OPERS
{
	i4		afe_opcnt;
	DB_DATA_VALUE	**afe_ops;
} AFE_OPERS;

/*}
** Name:   ADF_AGNAMES   -   Aggregate synonyms and primeness.
** 
** Description:
**      This is a structure which contains the function instance id, 
** 	operator id, function name, frame label, report label, 
** 	and the primeness for one aggregate function.
** 
** 	The function id is not the function instance id, but is only a numeric
** 	value which implies the function name (ag_funcname) and which is 
** 	functionally dependent on the function name (i.e. they are synonyms).
** 
**      An aggregate function is considered prime if it does not accept
**      duplicate values (i.e. it is a "unique" aggregate).
**
** History:
**	30-mar-1987 (Joe)
**		Added to this file from Richard's description.
*/
typedef struct s_ADF_AGNAMES
{
	ADI_FI_ID	ag_fid;
	ADI_OP_ID	ag_opid;
        char    	ag_funcname[FE_MAXNAME];
        char    	ag_framelabel[FE_MAXNAME];
        char    	ag_reportlabel[FE_MAXNAME];
        bool    	ag_prime;
} ADF_AGNAMES;

/*}
** Name:        AGNAMES         - Structure containing agg info.
**
** Description:
**      This structure holds a generic version of an ADF_AGNAME
**      plus a bitmask that shows which datatype the aggregate is
**      appropriate for.
**
** History:
**      27-mar-1987 (Joe)
**              Written
*/
typedef struct s_AGNAMES
{
        ADF_AGNAMES     ag_name;
        ADI_DT_BITMASK  ag_dtmask;
        ADI_FI_TAB      ag_fitab;
} AGNAMES;
 
/*
** Count of known aggregates
**    "any", "avg", "avgu", "count", "countu", "max", "min",
**    "sum", "sumu", ""
*/
# define	AGNAMES_COUNT	10

/*
** Datatype classes.
**
** Currently broken down into character, numeric, data, and 'other'
** classifications.
*/

# define	ANY_DTYPE	0
# define 	CHAR_DTYPE	1
# define 	NUM_DTYPE	2
# define 	DATE_DTYPE	3


/*}
**  Name:	AFE_TRIMBLANKS() - Strip trailing blanks from a string
**
**  Description:
**	This macro takes a string (pointer to char) variable called "str" and
**  an integer variable "len" which is the length of the string in bytes.  It
**  then adjusts the len variable to reflect the length of the string disre-
**  garding trailing spaces.  Note that it does NOT modify the string.
**
**  History:
**	08/89 (jhw) -- Stolen from "common!adf!hdr!adfint.h", which says this
**		will eventually end up in the CM module of the CL.
*/

# ifdef DOUBLEBYTE
# define AFE_TRIMBLANKS(str, len) \
    {\
	char	*last = NULL;\
	char	*p    = (char *)(str);\
	char	*endp = (char *)(str) + (len);\
	\
	while (p < endp)\
	{\
	    if (!CMspace(p))\
		last = p;\
	    CMnext(p);\
	}\
	(len) = (last == NULL ? 0 : last - (str) + CMbytecnt(last));\
    }
# else
# define AFE_TRIMBLANKS(str, len) \
    {\
	while ( --(len) >= 0 &&  *((str) + (len)) == ' ')\
	    ;\
	++(len);\
    }
# endif
#define AFE_TRIMBLANKS_MACRO(str,len) AFE_TRIMBLANKS(str,len)


FUNC_EXTERN bool afe_preftype (DB_DT_ID curtype, DB_DT_ID bestype);
FUNC_EXTERN bool afe_tycoerce(ADF_CB *, DB_DT_ID, DB_DT_ID);
FUNC_EXTERN bool IIAFckFuncName(char *);
FUNC_EXTERN bool IIAFfedatatype(DB_DATA_VALUE *dbv);
FUNC_EXTERN bool IIAFfpcFindPatternChars(DB_DATA_VALUE *);
FUNC_EXTERN DB_STATUS afe_cancoerce(ADF_CB *, DB_DATA_VALUE *, DB_DATA_VALUE *, bool *);
FUNC_EXTERN DB_STATUS afe_ctychk(ADF_CB *, DB_USER_TYPE *, char *, char *, DB_DATA_VALUE *);
FUNC_EXTERN STATUS afe_agnames(ADF_CB *, DB_DATA_VALUE *, i4, ADF_AGNAMES *, i4 *);


#endif
