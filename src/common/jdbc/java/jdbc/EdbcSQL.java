/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbc;

/*
** Name: EdbcSQL.java
**
** Description:
**	Implements the SQL text class EdbcSQL.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	12-Nov-99 (gordy)
**	    Allow date formatter to be specified.
**	11-Nov-99 (rajus01)
**	    DatabaseMetaData Implementation: Add java.util.Enumeration.
**	13-Jun-00 (gordy)
**	    Made the function methods static.  Added support for 
**	    procedure call requests.
**	20-Oct-00 (gordy)
**	    Added support for hex literals as parameters in procedure calls.
**	28-Mar-01 (gordy)
**	    ProcInfo now a parameter to parseCall() rather than return value.
**	11-Apr-01 (gordy)
**	    Do not include time with DATE datatypes.
**	21-Jun-01 (gordy)
**	    Transform JDBC 'SELECT FOR UPDATE' syntax.  Check for
**	    'FOR UPDATE' and 'FOR READONLY' clauses.
**      11-Jul-01 (loera01) SIR 105309
**          Added support for Ingres syntax on DB procedures.
**      13-Aug-01 (loera01) SIR 105521
**          Added support for global temp table parameters in DB procedures.
**	20-Aug-01 (gordy)
**	    Extend the FOR UPDATE/READONLY code with default concurrency.
**	    Add methods for common text manipulation functions.
**	26-Oct-01 (gordy)
**	    Relational gateways now translate date('now') into client time
**	    rather than gateway time.  Since JDBC clients appear as GMT,
**	    CURDATE and NOW functions must be built as date literals of 
**	    the current client time.
**	31-Oct-01 (gordy)
**	    Some gateways don't currently accept the date format used by
**	    the JDBC driver/server for literals.  It has been found that
**	    all supported DBMS accept the date format YYYY_MM_DD HH:MM:SS.
**	    Local date formatters are now declared and the timezone for
**	    the associated connection is configured.
**      27-Nov-01 (loera01) 
**          Added support for standard JDBC syntax for global temp table
**          parameters.
**	 7-Aug-02 (gordy)
**	    Allow empty strings in date/time escape sequences.
**      19-Nov-03 (weife01)
**          Bug 110950: using local default timezone when format input 
**          date using escape sequence and current date. 
*/

import	java.math.BigDecimal;
import	java.text.DateFormat;
import	java.text.SimpleDateFormat;
import	java.util.TimeZone;
import	java.util.Enumeration;
import	java.util.Hashtable;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.SQLException;
import	ca.edbc.util.EdbcEx;

/*
** Name: EdbcSQL
**
** Description:
**	Class providing SQL query text parsing and transformation.
**
**  Constants:
**
**	QT_UNKNOWN	    Unknown query type.
**	QT_SELECT	    Select query.
**	QT_DELETE	    Delete (cursor) query.
**	QT_UPDATE	    Update (cursor) query.
**	QT_PRODECURE	    Execute procedure (ODBC syntax).
**	QT_NATIVE_PROC	    Execute procedure (Ingres syntax).
**
**	CONCUR_DEFAULT	    Default concurrancy.
**	CONCUR_READONLY	    Readonly concurrency.
**	CONCUR_UPDATE	    Update concurrency.
**
**  Public Methods:
**
**	getQueryType	    Determine type of SQL query/statement.
**	getCursorName	    Get cursor for positioned UPDATE or DELETE.
**	getConcurrency	    Returns the statement concurrency type.
**	parseSQL	    Convert SQL text to JDBC Server format.
**	parseCall	    Convert CALL text to ProcInfo object.
**	getNumFuncs	    Get Ingres Numeric Functions.
**	getStrFuncs	    Get Ingres String Functions.
**	getTdtFuncs	    Get Ingres TimeDate Functions.
**	getSysFuncs	    Get Ingres System Functions.
**	getConvertTypes()   Get supported SQL types.
**
**  Private Methods:
**
**	getNativeDBProcReturn	Determine if Ingres DBProc call has return.
**	parseNumeric	    Parse a full numeric value.
**	hex2bin		    Convert hex string to byte array.
**	parseESC	    Convert ODBC escape sequence.
**	parseConvert	    Convert ODBC escape function CONVERT.
**	append_escape	    Copy escape text and process nested sequences.
**	append_text	    Copy text array to string buffer.
**	save_text	    Copy string buffer to text array.
**	nextToken	    Scan for next token.
**	prevToken	    Scan for previous token.
**	match		    Find match for character pair.
**	keyword		    Determine keyword ID of a token.
**	isIdentChar	    Is character permitted in an identifier.
**
**  Private Data:
**	
**	sql		    Original SQL query.
**	text		    Query text.
**	txt_len		    Length of query in text.
**	type		    Query type.
**	cursor		    Cursor name for positioned Delete/Update.
**	for_readonly	    Does query contain 'FOR READONLY'.
**	for_update	    Does query contain 'FOR UPDATE'.
**	timeZone	    Timezone for connection.
**	transformed	    True if query text has been modified.
**	token_beg	    First character of current token in text.
**	token_end	    Character following current token in text.
**	nested		    True if nested character pairs found by match().	    
**
**	keywords	    SQL keywords.
**	esc_seq		    ODBC escape sequences.
**	esc_func	    ODBC functions.
**	func_xlat	    Ingres functions (translated)
**	types		    ODBC and Ingres conversion functions.
**
**	FUNC_DATE_PREFIX    Date function prefix text.
**	FUNC_DATE_SUFFIX    Date function suffix text.
**	D_FMT		    Date format.
**	T_FMT		    Time format.
**	TS_FMT		    Timestamp format.
**	df_ts		    Timestamp formatter.
**	df_date		    Date formatter.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	12-Nov-99 (gordy)
**	    Allow date formatter to be specified.
**	11-Nov-99 (rajus01)
**	    DatabaseMetaData Implementation: Added getNumFuncs, getStrFuncs,
**	    getTdtFuncs, gettSysFuncs, getSQLtypes.
**	13-Jun-00 (gordy)
**	    Added support for procedure call requests.  Enhanced scanning
**	    of numerics.
**	20-Oct-00 (gordy)
**	    Added hex2bin().
**	21-Jun-01 (gordy)
**	    Added keywords 'FOR' and 'READONLY', flags for_readonly and 
**	    for_update, and methods isForReadonly() and isForUpdate().
**	20-Aug-01 (gordy)
**	    Replaced isForReadonly() and isForUpdate() with getConcurrency()
**	    to support default concurrency.  Added concurrency constants
**	    CONCUR_DEFAULT, CONCUR_READONLY, CONCUR_UPDATE.  Added private
**	    methods append_text() and save_text().
**	26-Oct-01 (gordy)
**	    Added FUNC_DATE_PREFIX, FUNC_DATE_SUFFIX to support date function.
**	31-Oct-01 (gordy)
**	    Added D_FMT, TS_FMT, df_ts, dt_date and changed dateFormat to
**	    timeZone for local date formatting.
*/

class
EdbcSQL
    implements EdbcErr
{
    /*
    ** Query types.
    */
    public static final int	QT_UNKNOWN  = 0;    // must be diffent than UNKNOWN
    public static final int	QT_DELETE   = 1;
    public static final int	QT_PROCEDURE= 2;
    public static final int	QT_SELECT   = 3;
    public static final int	QT_UPDATE   = 4;
    public static final int	QT_NATIVE_PROC = 5;

    /*
    ** Concurrency
    */
    public static final int	CONCUR_DEFAULT	= -1;
    public static final int	CONCUR_READONLY	= 0;
    public static final int	CONCUR_UPDATE	= 1;

    /*
    ** General classification for tokens in SQL statements.
    */
    private static final int	UNKNOWN	    = -1;   // Unclassified.
    private static final int	EOF	    = -2;   // End of query.
    private static final int	STRING	    = 1;    // Quoted string
    private static final int	IDENT	    = 2;    // Identifier, possible keyword.
    private static final int	NUMBER	    = 3;    // Number.
    private static final int	PUNCT	    = 4;    // General punctuation.
    private static final int	ESCAPE	    = 5;    // ODBC escape sequence.

    private static final int	P_LPAREN    = 10;   // Specific punctuation.
    private static final int	P_RPAREN    = 11;
    private static final int	P_LBRACE    = 12;
    private static final int	P_RBRACE    = 13;
    private static final int	P_COMMA	    = 14;
    private static final int	P_QMARK	    = 15;
    private static final int	P_EQUAL	    = 16;
    private static final int	P_PLUS	    = 17;
    private static final int	P_MINUS	    = 18;
    private static final int	P_PERIOD    = 19;

    /*
    ** Numeric types
    */
    private static final int	NUM_INT	    = 0;    // Integer
    private static final int	NUM_DEC	    = 1;    // Decimal
    private static final int	NUM_FLT	    = 2;    // Float

    /*
    ** SQL keywords.
    **
    ** The keyword ID must match position in keywords array.
    */
    private static final int	KW_CALLPROC = 0;
    private static final int	KW_CURRENT  = 1;
    private static final int	KW_DELETE   = 2;
    private static final int	KW_EXECUTE  = 3;
    private static final int	KW_FOR	    = 4;
    private static final int	KW_INTO     = 5;
    private static final int	KW_OF	    = 6;
    private static final int	KW_PROCEDURE = 7;
    private static final int	KW_READONLY = 8;
    private static final int	KW_SELECT   = 9;
    private static final int	KW_SESSION  = 10;
    private static final int	KW_UPDATE   = 11;
    private static final int	KW_WHERE    = 12;

    private final static String	keywords[] = 
    {
	"CALLPROC",	"CURRENT",	"DELETE",	"EXECUTE",	
	"FOR",		"INTO",		"OF", 		"PROCEDURE", 	
	"READONLY",	"SELECT",	"SESSION",	"UPDATE",	
	"WHERE"
    };

    /*
    ** ODBC escape sequences
    **
    ** The sequence ID must match position in esc_seq array.
    */
    private static final int	ESC_CALL    = 0;
    private static final int	ESC_DATE    = 1;
    private static final int	ESC_ESC	    = 2;
    private static final int	ESC_FUNC    = 3;
    private static final int	ESC_OJ	    = 4;
    private static final int	ESC_TIME    = 5;
    private static final int	ESC_STAMP   = 6;

    private final static String esc_seq[] =
    { "CALL", "D", "ESCAPE", "FN", "OJ", "T", "TS" };

    /*
    ** ODBC scalar functions
    **
    ** The function ID must match position in esc_func array.
    ** The corresponding Ingres function must appear in the
    ** same position in the func_xlat array.
    */
    private static final int	FUNC_ABS    = 0;
    private static final int	FUNC_ATAN   = 1;
    private static final int	FUNC_CONCAT = 2;
    private static final int	FUNC_CNVRT  = 3;
    private static final int	FUNC_COS    = 4;
    private static final int	FUNC_CDATE  = 5;
    private static final int	FUNC_DOW    = 6;
    private static final int	FUNC_EXP    = 7;
    private static final int	FUNC_IFNULL = 8;
    private static final int	FUNC_LCASE  = 9;
    private static final int	FUNC_LEFT   = 10;
    private static final int	FUNC_LENGTH = 11;
    private static final int	FUNC_LOG    = 12;
    private static final int	FUNC_MOD    = 13;
    private static final int	FUNC_NOW    = 14;
    private static final int	FUNC_RIGHT  = 15;
    private static final int	FUNC_RTRIM  = 16;
    private static final int	FUNC_SIN    = 17;
    private static final int	FUNC_SQRT   = 18;
    private static final int	FUNC_UCASE  = 19;
    private static final int	FUNC_USER   = 20;

    private static final String esc_func[] = 
    { "ABS", "ATAN", "CONCAT", "CONVERT", "COS", "CURDATE", "DAYNAME", 
      "EXP", "IFNULL", "LCASE", "LEFT", "LENGTH", "LOG", "MOD", "NOW", 
      "RIGHT", "RTRIM", "SIN", "SQRT", "UCASE", "USER" };

    private static final String func_xlat[] = 
    { "ABS", "ATAN", "CONCAT", "CONVERT", "COS", "DATE('TODAY')", "DOW", 
      "EXP", "IFNULL", "LOWERCASE", "LEFT", "LENGTH", "LOG", "MOD", "DATE('NOW')", 
      "RIGHT", "TRIM", "SIN", "SQRT", "UPPERCASE", "USER" };

    private static final Hashtable  types = new Hashtable();
    
    private static final String	    FUNC_DATE_PREFIX = "date('";
    private static final String	    FUNC_DATE_SUFFIX = "')";
    private static final String	    D_FMT = "yyyy_MM_dd";
    private static final String	    T_FMT = "HH:mm:ss";
    private static final String	    TS_FMT = D_FMT + " " + T_FMT;
    private static final DateFormat df_ts = new SimpleDateFormat( TS_FMT );
    private static final DateFormat df_date = new SimpleDateFormat( D_FMT );

    private String		sql = null;
    private char[]		text = null;
    private int			txt_len;
    private TimeZone		timeZone = null;
    private boolean		transformed = false;
    private boolean		procedure_return = false;
    private int			type = UNKNOWN;
    private String		cursor = null;
    private boolean		for_update = false;
    private boolean		for_readonly = false;

    private int			token_beg = 0;	    // nextToken() locations
    private int			token_end = 0;
    private boolean		nested = false;	    // match() scanned nested pairs


/*
** Name: <class initializer>
**
** Description:
**	Load the conversion function translation hashtable.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

static // Class Initializer
{
    /*
    ** Supported CONVERT() function SQL types and
    ** their equivilent Ingres conversion function.
    */
    types.put( "BINARY", "BYTE" );
    types.put( "CHAR", "CHAR" );
    types.put( "DECIMAL", "DECIMAL" );
    types.put( "DOUBLE", "FLOAT8" );
    types.put( "FLOAT", "FLOAT8" );
    types.put( "INTEGER", "INT4" );
    types.put( "LONGVARBINARY", "LONG_BYTE" );
    types.put( "LONGVARCHAR", "LONG_VARCHAR" );
    types.put( "NUMERIC", "DECIMAL" );
    types.put( "REAL", "FLOAT4" );
    types.put( "SMALLINT", "INT2" );
    types.put( "TIMESTAMP", "DATE" );
    types.put( "TINYINT", "INT1" );
    types.put( "VARBINARY", "VARBYTE" );
    types.put( "VARCHAR", "VARCHAR" );
} // static

/*
** Name: EdbcSQL
**
** Description:
**	Class constructor.
**
** Input:
**	sql	    SQL query text.
**	timeZone    Timezone for connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	12-Nov-99 (gordy)
**	    Allow date formatter to be specified.
**	31-Oct-01 (gordy)
**	    Parameter changed to timezone to use with local date formatters.
*/

public
EdbcSQL( String sql, TimeZone timeZone )
{
    this.sql = sql;
    text = sql.toCharArray();
    txt_len = text.length;
    this.timeZone = timeZone;
} // EdbcSQL

/*
** Name: getConvertTypes
**
** Description:
**	Returns an enumeration of keys in the Hashtable.
**
** Input:
**	None.
**
** Output:
** 	None	
**
** Returns:
**	Enumeration of keys. (SQL Types).
**
** History:
**	11-Nov-99 (rajus01)
**	    DatabaseMetaData Implementation: Created.
**	13-Jun-00 (gordy)
**	    Made static.
*/

public static Enumeration
getConvertTypes()
{
    return( types.keys() );
}

/*
** Name: getNumFuncs()
**
** Description:
**	Returns a comma separated list of math functions.
**
** Input:
**	None.
**
** Output:
** 	None	
**
** Returns:
** 	String	Math functions. 
**
** History:
**	11-Nov-99 (rajus01)
**	    DatabaseMetaData Implementation: Created.
**	13-Jun-00 (gordy)
**	    Made static.
*/

public static String 
getNumFuncs()
{
   String funcs = "";

   for( int i = FUNC_ABS; i <= FUNC_USER; i++ )
   {
	switch ( i )
	{
	case FUNC_ABS: 
	case FUNC_ATAN: 
	case FUNC_COS: 
	case FUNC_EXP: 
	case FUNC_LOG: 
	case FUNC_MOD: 
	case FUNC_SIN: 
	case FUNC_SQRT: 
	    funcs = funcs + esc_func[ i ]; 
	    if( i != FUNC_SQRT )
	        funcs = funcs + ",";
	    break;
	default:
	    break;
	}
   }

   return( funcs );
}

/*
** Name: getStrFuncs()
**
** Description:
**	Returns a comma separated list of string functions.
**
** Input:
**	None.
**
** Output:
** 	None	
**
** Returns:
** 	String	string functions. 
**
** History:
**	11-Nov-99 (rajus01)
**	    DatabaseMetaData Implementation: Created.
**	13-Jun-00 (gordy)
**	    Made static.
*/

public static String 
getStrFuncs()
{
   String funcs = "";

   for( int i = FUNC_ABS; i <= FUNC_USER; i++ )
   {
	switch ( i )
	{
	case FUNC_CONCAT: 
	case FUNC_LCASE: 
	case FUNC_LEFT: 
	case FUNC_LENGTH: 
	case FUNC_RIGHT: 
	case FUNC_RTRIM: 
	case FUNC_UCASE: 
	    funcs = funcs + esc_func[ i ]; 
	    if( i != FUNC_UCASE )
	        funcs = funcs + ",";
	    break;
	default:
	    break;
	}
   }

   return( funcs );
}


/*
** Name: getTdtFuncs()
**
** Description:
**	Returns a comma separated list of time and date functions.
**
** Input:
**	None.
**
** Output:
** 	None	
**
** Returns:
** 	String	time and date functions. 
**
** History:
**	11-Nov-99 (rajus01)
**	    DatabaseMetaData Implementation: Created.
**	13-Jun-00 (gordy)
**	    Made static.
*/

public static String 
getTdtFuncs()
{
   String funcs = "";

   for( int i = FUNC_ABS; i <= FUNC_USER; i++ )
   {
	switch ( i )
	{
	case FUNC_CDATE: 
	case FUNC_DOW: 
	case FUNC_NOW: 
	    funcs = funcs + esc_func[ i ]; 
	    if( i != FUNC_NOW )
	        funcs = funcs + ",";
	    break;
	default:
	    break;
	}
    }

   return( funcs );

}


/*
** Name: getSysFuncs()
**
** Description:
**	Returns a comma separated list of system functions.
**
** Input:
**	None.
**
** Output:
** 	None	
**
** Returns:
** 	String	system functions. 
**
** History:
**	11-Nov-99 (rajus01)
**	    DatabaseMetaData Implementation: Created.
**	13-Jun-00 (gordy)
**	    Made static.
*/

public static String 
getSysFuncs()
{
   String funcs = "";

   for( int i = FUNC_ABS; i <= FUNC_USER; i++ )
   {
	switch ( i )
	{
	case FUNC_IFNULL: 
	case FUNC_USER: 
	    funcs = funcs + esc_func[ i ]; 
	    if( i != FUNC_USER )
	        funcs = funcs + ",";
	    break;
	default:
	    break;
	}
    }

   return( funcs );
}


/*
** Name: getQueryType
**
** Description:
**	Returns the type of SQL query/statement contained
**	in the SQL text (currently only a limited number
**	of query types are actually supported, QT_UNKNOWN
**	is returned for all other types).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	SQL query type.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Support call procedure and determine if
**	    procedure result is expected.
**      11-Jul-01 (loera01) SIR 105309
**          Added EXECUTE, PROCEDURE, and CALLPROC keywords to support
**          Ingres syntax on DB procedures.
*/

public int
getQueryType()
    throws EdbcEx
{
    int token;

    if ( type == UNKNOWN )
    {
	token_beg = token_end = 0;

	switch( nextToken( false ) )
	{
	case IDENT :
	    switch( keyword( text, token_beg, token_end, keywords ) )
	    {
	    case KW_DELETE :    type = QT_DELETE;   break;
	    case KW_SELECT :    type = QT_SELECT;   break;
	    case KW_UPDATE :    type = QT_UPDATE;   break;
	    
	    case KW_CALLPROC :  
		type = QT_NATIVE_PROC; 
	        procedure_return = getNativeDBProcReturn();
		break;

	    case KW_EXECUTE :
		token = nextToken(false);
		if ( keyword( text, token_beg, 
			      token_end, keywords ) != KW_PROCEDURE )
		    type = QT_UNKNOWN;
		else
		{
		    type = QT_NATIVE_PROC;
	            procedure_return = getNativeDBProcReturn();
		}
		break;

	    default :		type = QT_UNKNOWN;   break;
	    }
	    break;

	case P_LBRACE :	
	    if ( (token = nextToken( false )) == P_QMARK )
		if ( nextToken( false ) == P_EQUAL )  
		{
		    procedure_return = true;
		    token = nextToken( false );
		}
		else
		{
		    type = QT_UNKNOWN;
		    break;
		}

	    if ( token == IDENT  &&  
		 keyword( text, token_beg, token_end, esc_seq ) == ESC_CALL )
		type = QT_PROCEDURE;
	    else
		type = QT_UNKNOWN;
	    break;

	case P_LPAREN :	type = QT_SELECT;	break;
	default :	type = QT_UNKNOWN;	break;
	}
    }

    return( type );
} // getQueryType

/*
** Name: getCursorName
**
** Description:
**	If the SQL text is a cursor positioned DELETE or UPDATE,
**	the name of the cursor specified is returned.  The text
**	is also modified by removing the WHERE clause as is
**	required by the Ingres JDBC server.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    The cursor name, NULL if not a positioned DELETE or UPDATE.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

public String
getCursorName()
    throws EdbcEx
{
    if ( cursor != null )  return( cursor );
    if ( type == UNKNOWN )  getQueryType();

    if ( type == QT_DELETE  ||  type == QT_UPDATE )
    {
	int token, prev_end;
	token_beg = token_end = prev_end = 0;

	while( (token = nextToken( false )) != EOF )
	{
	    /*
	    ** Check for WHERE CURRENT OF <ident><eof>
	    **
	    ** If found, save cursor name and remove the
	    ** WHERE clause from the query.  Otherwise,
	    ** continue processing (no other parsing
	    ** cares about these particular keywords).
	    */
	    if ( token == IDENT  &&  
		 keyword( text, token_beg, token_end, keywords ) == KW_WHERE )
	    {
		int where_clause = prev_end;
		
		if ( 
		     nextToken( false ) == IDENT  &&
		     keyword(text,token_beg,token_end,keywords) == KW_CURRENT &&
		     nextToken( false ) == IDENT  &&
		     keyword(text, token_beg, token_end, keywords) == KW_OF  &&
		     nextToken( false ) == IDENT
		   )
		{
		    cursor = new String( text, token_beg, 
					 token_end - token_beg );
		    /*
		    ** If we now find EOF, remove the WHERE
		    ** clause and exit.  Otherwise, this isn't
		    ** what we were looking for so drop the
		    ** cursor name.
		    */
		    if ( nextToken( false ) != EOF )
			cursor = null;
		    else
		    {
			txt_len = where_clause;
			transformed = true;	// see parseSQL().
			break;
		    }
		}
	    }

	    prev_end = token_end;
	}

	/*
	** We are only interested in update and delete
	** statements if they are cursor positioned.  If
	** no cursor, reset the query type.
	*/
	if ( cursor == null )  type = QT_UNKNOWN;
    }

    return( cursor );
} // getCursorName


/*
** Name: getConcurrency
**
** Description:
**	Returns the concurrency requested in SQL text if
**	'FOR UPDATE' or 'FOR READONLY' clause is present.
**	Otherwise, returns default concurrency.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Concurrency: JDBC_CONCUR_{DEFAULT,READONLY,UPDATE}.
**
** History:
**	20-Aug-01 (gordy)
**	    Created.
*/

public int
getConcurrency()
{
    int concurrency;

    if ( for_readonly )
	concurrency = CONCUR_READONLY;
    else  if ( for_update )
	concurrency = CONCUR_UPDATE;
    else 
	concurrency = CONCUR_DEFAULT;

    return( concurrency );
} // getConcurrency


/*
** Name: parseSQL
**
** Description:
**	Return the SQL text transformed into Ingres format.
**
**	'SELECT FOR UPDATE ...' and 'SELECT FOR READONLY ...'
**	are allowed.  The concurrency type is saved when a
**	'FOR UPDATE' or 'FOR READONLY' clause is present.
**	The 'FOR READONLY' clause is removed from the text.
**	If the escapes parameter is true, escape sequences
**	are transformed.
**
** Input:
**	escapes	    Transform ODBC escape sequences?
**
** Output:
**	None.
**
** Returns:
**	String	    Transformed text.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	21-Jun-01 (gordy)
**	    Transform JDBC 'SELECT FOR UPDATE' syntax.  Check for
**	    'FOR UPDATE' and 'FOR READONLY' clauses.
**	20-Aug-01 (gordy)
**	    Remove 'FOR READONLY' clause as it is not a part of Ingres
**	    SELECT syntax.
*/

public String
parseSQL( boolean escapes )
    throws EdbcEx
{
    StringBuffer    buff = null;
    int		    token, tkn_beg, tkn_end, start;
    
    if ( type == UNKNOWN )  getQueryType();
    start = token_beg = token_end = 0;

    while( (token = nextToken( escapes )) != EOF )
	switch( token )
	{
	case IDENT :
	    switch( keyword( text, token_beg, token_end, keywords ) )
	    {
	    case KW_SELECT :
		/*
		** The JDBC spec requires support for the syntax
		** 'SELECT FOR UPDATE ...' (we also support READONLY).
		** Check for the syntax and transform into valid
		** Ingres syntax.
		*/
		tkn_end = token_end;

		if ( 
		     nextToken( false ) != IDENT  ||
		     keyword(text, token_beg, token_end, keywords) != KW_FOR ||
		     nextToken( false ) != IDENT
		   )
		{
		    // reset to original scan position.
		    token_beg = token_end = tkn_end;
		    break;
		}

		switch( keyword( text, token_beg, token_end, keywords ) )
		{
		case KW_READONLY :
		    /*
		    ** Remove 'FOR READONLY' as it isn't Ingres syntax.
		    */
		    buff = append_text( text, txt_len, start, tkn_end, buff );
		    start = token_end;
		    for_readonly = true;
		    break;

		case KW_UPDATE :
		    /*
		    ** Remove 'FOR UPDATE' as it isn't Ingres syntax.
		    */
		    buff = append_text( text, txt_len, start, tkn_end, buff );
		    start = token_end;

		    /*
		    ** If this is the first instance, form Ingres syntax
		    ** by appending the 'FOR UPDATE' clause to query.
		    */
		    if ( ! for_update )
		    {
			/*
			** Copy remaining text to buffer and append the
			** 'FOR UPDATE' clause.  The scan will resume at
			** the point already processed in the buffer.
			*/
			token_beg = token_end = buff.length();
			buff = append_text( text,txt_len,start,txt_len,buff );
			buff.append( " for update" );
			save_text( buff );
			start = 0;
			for_update = transformed = true;
		    }
		    break;

		default :	// reset to original scan position.
		    token_beg = token_end = tkn_end;
		    break;
		}
		break;

	    case KW_FOR :
		/*
		** Check 'FOR UPDATE' and 'FOR READONLY'
		*/
		if ( type != QT_SELECT )  break;
		tkn_beg = token_beg;
		tkn_end = token_end;

		if ( (token = nextToken( false )) == IDENT )
		    switch( keyword( text, token_beg, token_end, keywords ) )
		    {
		    case KW_READONLY :	// Remove 'FOR READONLY'
			buff = append_text( text,txt_len,start,tkn_beg,buff );
			start = token_end;
			for_readonly = true;
			break;

		    case KW_UPDATE :
			for_update = true;
			break;

		    default :	// reset to original scan position
			token_beg = token_end = tkn_end;
			break;
		    }
		break;
	    }
	    break;

	case ESCAPE :
	    if ( escapes )
	    {
		/*
		** Append leading text to buffer (allocate if needed),
		** process escape sequence and continue processing
		** following the token.
		*/
		buff = append_text( text, txt_len, start, token_beg, buff );
		parseESC( token_beg, (start = token_end), nested, buff );
		token_beg = token_end = start;
	    }
	    break;
	}

    /*
    ** Update the original query text only if an
    ** escape sequence was found and processed.
    */
    if ( buff != null  &&  start > 0 )
    {
	buff = append_text( text, txt_len, start, txt_len, buff );
	save_text( buff );
	transformed = true;
    }

    /*
    ** If a transformation has occured, update
    ** the SQL string.  Otherwise, the original
    ** string is simply returned.  Note that 
    ** this also handles the transformation in 
    ** getCursorName().
    */
    if ( transformed )
    {
	sql = new String( text, 0, txt_len );
	transformed = false;
    }

    return( sql );
} // parseSQL


/*
** Name: parseCall
**
** Description:
**	Parses a Database Procedure call request of the form:
**
**	{ [?=] call [schema.]procname[(param_spec[,param_spec])] }
**
**                   - OR -
**
**      [ execute procedure | callproc ] [schema.]procname
**	    [ ( param_name = param_spec{,param_name = param_spec}) ]
**              [ into ? ]
**      
**	Populates a procedure information object with the info 
**	parsed from the request text.
**
** Input:
**	None.
**
** Output:
**	procInfo    Procedure information.
**
** Returns:
**	void.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    ProcInfo now parameter rather than return value as
**	    info needed for constructing ProcInfo is not available.
**      11-Jul-01 (loera01) SIR 105309
**          Added support for Ingres syntax.
**      27-Nov-01 (loera01) 
**          Added support for standard JDBC syntax for global temp table
**          parameters.
*/

public void
parseCall( ProcInfo procInfo )
    throws EdbcEx
{
    int		token, index;
    String	name;

    /*
    ** By calling getQueryType() we verify that this is
    ** a procedure call request, determine if procedure
    ** return value is expected, and position the token
    ** scanner to read the procedure name.
    */
    type = UNKNOWN;
    int qType = getQueryType();
    token = nextToken( false );
    if (qType == QT_PROCEDURE && token != IDENT)
        throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
    else if (qType == QT_NATIVE_PROC && token != IDENT && token != EOF)
        throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
    name = new String( text, token_beg, token_end - token_beg );
    token = nextToken( false );

    if ( token == P_PERIOD )
    {
	if ( nextToken( false ) != IDENT )
	    throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
	procInfo.setSchema( name );
	name = new String( text, token_beg, token_end - token_beg );
	token = nextToken( false );
    }

    procInfo.setName( name );
    procInfo.setReturn( procedure_return );

    if ( token == P_LPAREN )
    {
      for_loop:	
	for(
	     index = 1, token = nextToken( false );
	     token != EOF;
	     index++, token = nextToken( false )
	   )
	{
	    switch( token )
	    {
	    case P_RPAREN :
		procInfo.setParam( index, ProcInfo.PARAM_DEFAULT, null );
		break for_loop;

	    case P_COMMA :
		procInfo.setParam( index, ProcInfo.PARAM_DEFAULT, null );
		break;

	    default :
		switch( token )
		{
		case P_QMARK :
		    /*
		    ** Dynamic parameter marker
		    */
		    procInfo.setParam( index, ProcInfo.PARAM_DYNAMIC, null );
		    break;

		case P_PLUS :
		case P_MINUS :
		case P_PERIOD :
		case NUMBER :
		    /*
		    ** Start of a numeric (int, dec, float) literal.
		    */
		    try
		    {
			int	type;
			Object	value;
			String	str;

			type = parseNumeric( token_beg );
			str = new String( text, token_beg, 
					  token_end - token_beg );
			switch( type )
			{
			case NUM_INT :
			    type = ProcInfo.PARAM_INT;
			    value = new Integer( str );
			    break;

			case NUM_DEC :
			    type = ProcInfo.PARAM_DEC;
			    value = new BigDecimal( str );
			    break;

			case NUM_FLT :
			    type = ProcInfo.PARAM_FLOAT;
			    value = new Double( str );
			    break;

			default :
			    throw new NumberFormatException();
			}

			procInfo.setParam( index, type, value );
		    }
		    catch( Exception ex )
		    {
			throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
		    }
		    break;

		case STRING :
		    /*
		    ** String literal (strip quotes when saving).
		    */
		    procInfo.setParam( index, ProcInfo.PARAM_CHAR,
			new String( text, token_beg + 1,
				    token_end - token_beg - 2 ) );
		    break;

                case IDENT:
		    /*
		    ** Check for Ingres "param=" syntax.
		    */
		    int prev_end = token_end;
		    int prev_beg = token_beg;

		    if ( qType == QT_PROCEDURE && isGTTParam() )
		    {
	                procInfo.setParam( index, ProcInfo.PARAM_SESSION, 
		            new String( text, token_beg, 
			        token_end - token_beg ) );
		    	break;
		    }
		    else if ( qType == QT_NATIVE_PROC && 
			( token = nextToken( false ) ) == P_EQUAL )
		    {
		        procInfo.setParamName( index, 
			    new String( text, prev_beg, prev_end - prev_beg ) );

			if ( ! isGTTParam() )
			{
			    /*
			    ** Start from the top of the loop, since the 
		            ** next token should be P_QMARK or a literal.
			    */
			    index--;  // don't double-increment the index
			    continue for_loop;
			}
			else
			{
	                    procInfo.setParam( index, ProcInfo.PARAM_SESSION, 
				new String( text, token_beg, 
					    token_end - token_beg ) );
			    break;
			}
	            }
	            else
		    {
		        token_end = prev_end;
			token_beg = prev_beg;
		        /*
		        ** Check for a HEX literal: X'...'
		        */
		        if ( 
			     (token_end - token_beg) == 1  &&
			     Character.isLetter( text[ token_beg ] )  &&
			     String.valueOf( text,token_beg, 
					     1 ).equalsIgnoreCase("X")  &&
			     (token = nextToken( false )) == STRING
		           )
		        {
			    procInfo.setParam( index, ProcInfo.PARAM_BYTE,
				hex2bin( text, token_beg + 1, 
					 token_end - token_beg - 2 ) );
			    break;
			}
		    }
		    throw EdbcEx.get( E_JD0016_CALL_SYNTAX );

		default:
		    throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
		}
		
		if ( (token = nextToken( false )) == P_RPAREN )
		    break for_loop;
		else  if ( token != P_COMMA )
		    throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
		break;
	    }
	}

	if ( token != P_RPAREN )
	    throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
	else
	    token = nextToken( false );
    }

    if ( qType != QT_NATIVE_PROC  &&
	 (token != P_RBRACE  ||  (token = nextToken( false )) != EOF) )  
	throw EdbcEx.get( E_JD0016_CALL_SYNTAX );

    return;
} // parseCall


/*
** Name: getNativeDBProcReturn
**
** Description:
**	Determine if Ingres procedure call returns a value.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	True if the "execute procedure" command includes "into" keyword.
**
** History:
**	11-Jul-01 (loera01) SIR 105309
**	    Created.
*/
private boolean
getNativeDBProcReturn()
    throws EdbcEx
{
    char[] ntext = text;
    int token, ntoken_beg = token_beg, ntoken_end = token_end;

    while( (token = nextToken( false )) != EOF )
    {
        if ( token == IDENT  &&
           keyword( text, token_beg, token_end, keywords ) == KW_INTO )
        {
	    token_beg = ntoken_beg; token_end = ntoken_end; text = ntext;
            return( true );
        }
    }

    token_beg = ntoken_beg; token_end = ntoken_end; text = ntext;
    return( false );
} // getNativeDBProcReturn


/*
** Name: isGTTParam
**
** Description:
**	Determine if the procedure call includes a global temp table
**      parameter.  If a global temp table parameter is found, the token
**      end marker is left pointing at the end of the token; otherwise,
**      the end token marker is restored to its previous position.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if the DB proc includes "session.aaa" syntax 
**			as the parameter value.
**
** History:
**	13-Aug-01 (loera01) SIR 105521
**	    Created.
**      27-Nov-01 (loera01)
**          Added support for standard JDBC syntax.
*/

private boolean 
isGTTParam()
    throws EdbcEx
{
    int	prev_end = token_end;

    if 
       ( 
	 getQueryType() == QT_PROCEDURE &&
         keyword( text, token_beg, token_end, keywords ) == KW_SESSION  && 
         nextToken( false ) == P_PERIOD  && 
	 nextToken( false ) == IDENT 
       )
	return( true );
    else if ( 
         nextToken( false ) == IDENT  &&
         keyword( text, token_beg, token_end, keywords ) == KW_SESSION  && 
         nextToken( false ) == P_PERIOD  && 
	 nextToken( false ) == IDENT 
       )
	return( true );

    token_end = prev_end;
    return( false );
}


/*
** Name: parseNumeric
**
** Description:
**	Parses a full numeric value of type integer, decimal, or float.
**	Sets token_beg and token_end to delimit the full numeric value.
**	Returns the type of the numeric.
**
**	The supported syntax is as follows:
**
**	[+-][digits][.[digits]][eE[+-]digits]
**
**	
** Input:
**	beg	Beginning parse position.
**
** Ouput:
**	None.
**
** Returns:
**	int	Numeric type.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
*/

private int
parseNumeric( int beg )
    throws EdbcEx
{
    int	type = UNKNOWN;
    int	token, end;

    /*
    ** Load the first token saving the start so
    ** that all tokens can be included when done.
    */
    token_end = token_beg = beg;
    token = nextToken( false );
    beg = token_beg;
    end = token_end;
    
    /*
    ** Leading sign is optional.
    */
    if ( token == P_PLUS  ||  token == P_MINUS )
	token = nextToken( false );

    /*
    ** An integer numeric requires a string of digits
    ** following an optional sign.  This may also be
    ** a subset of a decimal or float numeric.
    */
    if ( token == NUMBER )
    {
	end = token_end;
	type = NUM_INT;
	token = nextToken( false );
    }

    /*
    ** A period is permitted following or proceding
    ** a string of digits and identifies a decimal
    ** or possibly a float.
    */
    if ( token == P_PERIOD )
    {
	end = token_end;
	
	if ( (token = nextToken( false )) == NUMBER )
	{
	    end = token_end;
	    type = NUM_DEC;
	    token = nextToken( false );
	}
	else  if ( type == NUM_INT )
	    type = NUM_DEC;
    }

    /*
    ** Float numerics require a leading integer or decimal.
    */
    if ( type == NUM_INT  ||  type == NUM_DEC )
    {
	/*
	** This may or may not be a float value.
	** We don't change the current type until
	** a valid float has been scanned.
	**
	** Floats include an exponent.  The tokenizer 
	** does not distinguish between an exponent 
	** and an identifier containing digits, so we 
	** must pick the current token apart.
	*/
	if ( token == IDENT  &&
	     (text[ token_beg ] == 'e'  ||  text[ token_beg ] == 'E') )
	{
	    /*
	    ** Begin scanning following the exponent character.
	    */
	    token_end = token_beg + 1;
	    token = nextToken( false );

	    if ( token == P_PLUS  ||  token == P_MINUS )
		token = nextToken( false );

	    if ( token == NUMBER )
	    {
		end = token_end;
		type = NUM_FLT;
	    }
	}
    }

    token_beg = beg;
    token_end = end;
    return( type );
}


/*
** Name: hex2bin
**
** Description:
**	Converts a hex string (subset of character array)
**	into a byte array.
**
** Input:
**	hex	    Character array containing hex string.
**	offset	    Start of hex string in character array.
**	length	    Number of characters in hex string.
**
** Output:
**	None.
**
** Returns:
**	byte []	    Byte array contraining binary value of hex string.
**
** History:
**	20-Oct-00 (gordy)
**	    Created.
**      14-Aug-01 (loera01)
**          Use parseShort to parse the byte.  Otherwise, an exception is
**          thrown when Byte.MAX_VALUE (127) is exceeded.
*/

private byte []
hex2bin( char hex[], int offset, int len )
    throws EdbcEx
{
    byte ba[] = new byte[ len / 2 ];
    try
    {
	for( int i = 0; i < ba.length; i++, offset += 2 )
	    ba[ i ] = (byte)Short.parseShort( new String( hex, offset, 2 ), 
		16 );
    }
    catch( NumberFormatException ex )
    {
	throw EdbcEx.get( E_JD0016_CALL_SYNTAX );
    }

    return( ba );
} // hex2bin


/*
** Name: parseESC
**
** Description:
**	Parses ODBC escape sequence text in the query buffer, 
**	transforms sequence and appends to output buffer,
**	Nested escape sequences are also handled if present.
**	The instance variables token_beg, token_end and 
**	nested are invalid when this method completes.
**
** Input:
**	beg	Start of escape sequence.
**	end	End of escape sequence.
**	nested	Nested escape sequences?
**
** Output:
**	buff	Output buffer.
**
** Returns:
**	void.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	12-Nov-99 (gordy)
**	    Allow date formatter to be specified.
**	11-Apr-01 (gordy)
**	    Ingres supports (to some extent) dates without time,
**	    so do not include time with DATE datatypes.
**	26-Oct-01 (gordy)
**	    It has now been decided that gateways will use the client time
**	    when supporting date('now').  Since all JDBC clients appear to
**	    be GMT, we must produce a timestamp literal of the current time
**	    rather than use date('now').  This is done for all DBMS servers
**	    since we can't tell here if this is a gateway connection.
**	31-Oct-01 (gordy)
**	    Some gateways don't currently accept the date format used by
**	    the JDBC driver/server for literals.  It has been found that
**	    all supported DBMS accept the date format YYYY_MM_DD HH:MM:SS.
**	    Local date formatters are now declared and the timezone for
**	    the associated connection is configured.
**	 7-Aug-02 (gordy)
**	    Allow empty strings in date/time escape sequences which will
**	    be translated as an Ingres empty date by DBMS/gateway.
**      19-Nov-03 (weife01)
**          Bug 110950: using local default timezone when format input
**          date using escape sequence and current date.
*/

private void
parseESC( int beg, int end, boolean nested, StringBuffer buff )
    throws EdbcEx
{
    int type, token;

    token_beg = token_end = beg + 1;

    if ( (token = nextToken( false )) != IDENT  ||
	 (type = keyword( text, token_beg, token_end, esc_seq )) == UNKNOWN )
    {
	/*
	** This is not a recognized escape sequence.
	** Append the entire sequence to the buffer
	** and let the DBMS worry about it.  Must
	** handle the surrounding braces here other-
	** wise append_escape would recurse as if for
	** nested escape sequence.
	*/
	buff.append( '{' );
	append_escape( beg + 1, end - 1, nested, buff );
	buff.append( '}' );
	return;
    }

    switch( type )
    {
    case ESC_DATE :
    case ESC_TIME :
    case ESC_STAMP :
	if ( (token = nextToken( false )) != STRING )
	{
	    /*
	    ** Pass through argument without conversion.
	    */
	    append_escape( token_beg, end - 1, nested, buff );
	}
	else
	{
	    String val = new String(text, token_beg + 1, token_end - token_beg - 2);

	    if ( val.length() > 0 )
		switch( type )
		{
		case ESC_DATE :
		    synchronized( df_date )
		    {
			df_date.setTimeZone( timeZone.getDefault());
			val = df_date.format((java.util.Date)Date.valueOf(val));
		    }
		    break;
		
		case ESC_TIME :
		    synchronized( df_ts )
		    {
			df_ts.setTimeZone( timeZone );
			val = df_ts.format((java.util.Date)Time.valueOf(val));
		    }
		    break;
		
		case ESC_STAMP :
		    synchronized( df_ts )
		    {
			df_ts.setTimeZone( timeZone );
			val = df_ts.format((java.util.Date)Timestamp.valueOf(val));
		    }
		    break;

		default :
		    synchronized( df_ts )
		    {
			df_ts.setTimeZone( timeZone );
			val = df_ts.format((java.util.Date)(new Timestamp(0)));
		    }
		    break;
		}

	    buff.append( FUNC_DATE_PREFIX );
	    buff.append( val );
	    buff.append( FUNC_DATE_SUFFIX );
	}
	break;

    case ESC_FUNC :
	if ( (token = nextToken( false )) != IDENT  ||
	     (type = keyword( text, token_beg, token_end, esc_func )) == UNKNOWN )
	{
	    /*
	    ** Unrecognized function. pass through as is.
	    */
	    append_escape( token_beg, end - 1, nested, buff );
	}
	else
	{
	    switch( type )
	    {
	    case FUNC_CNVRT :
		parseConvert( token_beg, end - 1, nested, buff );
		break;

	    case FUNC_CDATE :
		// TODO: enforce ()?
		synchronized( df_date )
		{
		    df_date.setTimeZone( timeZone.getDefault());
		    buff.append( FUNC_DATE_PREFIX );
		    buff.append( df_date.format( new java.util.Date() ) );
		    buff.append( FUNC_DATE_SUFFIX );
		}
		break;

	    case FUNC_NOW :
		// TODO: enforce ()?
		synchronized( df_ts )
		{
		    df_ts.setTimeZone( timeZone );
		    buff.append( FUNC_DATE_PREFIX );
		    buff.append( df_ts.format( new java.util.Date() ) );
		    buff.append( FUNC_DATE_SUFFIX );
		}
		break;

	    case FUNC_USER :
		// TODO: enforce ()?
		buff.append( func_xlat[ type ] );   // Function replacement.
		break;

	    default :
		buff.append( func_xlat[ type ] );   // Translate function name.
		append_escape( token_end, end - 1, nested, buff );
		break;
	    }
	}
	break;

    case ESC_ESC :
	/*
	** No transformation required, simply remove the surrounding braces.
	*/
	append_escape( beg + 1, end - 1, nested, buff );
	break;

    case ESC_OJ :
	/*
	** No transformation required, simply remove escape syntax.
	*/
	nextToken( false );
	append_escape( token_beg, end - 1, nested, buff );
	break;

    case ESC_CALL :
	/*
	** The escape sequence is supported through parseCall(),
	** which does use this routine.  Therefore, the call
	** escape sequence is not supported here.
	*/

    default :
	// Should not appear in SQL text, hence should not happen!
	buff.append( '{' );
	append_escape( beg + 1, end - 1, nested, buff );
	buff.append( '}' );
	break;
    }

    return;
} // parseESC

/*
** Name: parseConvert
**
** Description:
**	Parse the ODBC escape sequence for the CONVERT function,
**	transform into the corresponding Ingres conversion
**	function, and append to output buffer.
**
** Input:
**	beg	Beginning position for convert function.
**	end	Ending position of convert function.
**	nested	Is there a nested ODBC escape sequence.
**
** Output:
**	buff	Output buffer.
**
** Returns:
**	void
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

private void
parseConvert( int beg, int end, boolean nested, StringBuffer buff )
    throws EdbcEx
{
    int	    token;
    boolean success = false;

    token_beg = token_end = beg;
    
    if ( (token = nextToken( false )) == IDENT  &&
	 (token = nextToken( false )) == P_LPAREN )
    {
	int start = token_end;
	token_beg = token_end = end;
	
	if ( (token = prevToken()) == P_RPAREN  &&  (token = prevToken()) == IDENT )
	{
	    String sqlType = new String( text, token_beg, token_end - token_beg );
		
	    if ( (sqlType = (String)types.get( sqlType.toUpperCase() )) != null  &&
		 (token = prevToken()) == P_COMMA )
	    {
		buff.append( sqlType );
		buff.append( '(' );
		append_escape( start, token_beg, nested, buff );
		buff.append( ')' );
		success = true;
	    }
	}
    }

    if ( ! success )  append_escape( beg, end, nested, buff );
    return;
} // parseConvert

/*
** Name: append_escape
**
** Description:
**	Appends ODBC escape sequence text in query buffer
**	to output buffer provided.  The escape sequence
**	text should not include the surrounding braces.
**	Nested escape sequences are transformed as part 
**	of the text transfer.  The instance variables 
**	token_beg, token_end and nested are not valid 
**	when this method completes.
**
** Input:
**	beg	Start of escape sequence.
**	end	End of escape sequence.
**	nested	Nested escape sequence.
**
** Output:
**	buff	Output buffer.
**
** Returns:
**	void.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

private void
append_escape( int beg, int end, boolean nested, StringBuffer buff )
    throws EdbcEx
{
    if ( ! nested )	// Easy case: no further processing needed.
	buff.append( text, beg, end - beg );
    else
    {
	int token, old_len = txt_len;

	/*
	** Trick nextToken() into only scanning
	** the current escape sequence.
	*/
	txt_len = end;
	token_beg = token_end = beg;

	while( (token = nextToken( true )) != EOF )
	    if ( token == ESCAPE )
	    {
		if ( token_beg > beg )		// append leading text.
		    buff.append( text, beg, token_beg - beg );
		parseESC( token_beg, (beg = token_end), this.nested, buff );
		token_beg = token_end = beg;
	    }

	if ( beg < end )	// append trailing text.
	    buff.append( text, beg, end - beg );
	txt_len = old_len;
    }

    return;
} // append_escape


/*
** Name: append_text
**
** Description:
**	Copies text from a character array and appends to string buffer.
**	Allocates string buffer if necessary.  The character marking the
**	end of the text to be copied is not itself copied.
**
** Input:
**	text		Source character array.
**	txt_len		Length of text in array.
**	start		Starting character to copy.
**	end		End of text to copy (won't get copied)
**
** Output:
**	buff		String buffer which receives text.
**
** Returns:
**	StringBuffer	New string buffer.
**
** History:
**	20-Aug-01 (gordy)
**	    Created.
*/

private StringBuffer
append_text( char text[], int txt_len, int start, int end, StringBuffer buff )
{
    if ( buff == null )  buff = new StringBuffer( txt_len );
    if ( end > start )  buff.append( text, start, end - start );
    return( buff );
} // append


/*
** Name: save_text
**
** Description:
**	Copies text from a string buffer into the SQL text character array.
**	Array is expanded if necessary.  String buffer is emptied.
**
** Input:
**	buff	String buffer.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Aug-01 (gordy)
**	    Created.
*/
  
private void
save_text( StringBuffer buff )
{
    /*
    ** Expand text array if needed.
    */
    if ( buff.length() > text.length )  text = new char[ buff.length() ];
    txt_len = buff.length();
    buff.getChars( 0, txt_len, text, 0 );
    buff.setLength(0);
    return;
} // save_text


/*
** Name: nextToken
**
** Description:
**	Scan for the next token (starting at the end of 
**	the current token, token_end) and return a general
**	classification of the type of token found.  The 
**	position of the token is saved in the instance 
**	variables token_beg and token_end.
**
**	ODBC escape sequences may optionally be treated as
**	a single token.  If this option is selected, the
**	presence of nested escape sequences is indicated
**	by the instance variable nested.
**	
**
** Input:
**	esc	Treat ODBC escape sequences as single token.
**
** Output:
**	None.
**
** Returns:
**	int	Token ID.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Support all specific punctuation.  Scanning of numerics now
**	    done by helper method, so only scan integers not decimals.
*/

private int
nextToken( boolean esc )
    throws EdbcEx
{
    int	    type;
    char    ch;

    while( token_end < txt_len  &&  
	   Character.isWhitespace( text[ token_end ] ) )  token_end++;

    token_beg = token_end;
    if ( token_end >= txt_len )  return( EOF );
    token_end = token_beg + 1;

    switch( (ch = text[ token_beg ]) )
    {
    case '(' :	type = P_LPAREN;    break;
    case ')' :	type = P_RPAREN;    break;
    case ',' :	type = P_COMMA;	    break;
    case '?' :	type = P_QMARK;	    break;
    case '=' :	type = P_EQUAL;	    break;
    case '+' :	type = P_PLUS;	    break;
    case '-' :	type = P_MINUS;	    break;
    case '.' :	type = P_PERIOD;    break;
    case '}' :	type = P_RBRACE;    break;

    case '{' :
	if ( ! esc )
	    type = P_LBRACE;
	else
	{
	    token_end = match( text, token_beg, txt_len, '}' ) + 1;
	    type = ESCAPE;
	}
	break;

    case '\'' :	
    case '"' :
	token_end = match( text, token_beg, txt_len, ch ) + 1;
	type = STRING;
	break;

    default :
	if ( isIdentChar( ch ) )
	{
	    type = IDENT;

	    for( ; token_end < txt_len; token_end++ )
	    {
		ch = text[ token_end ];
		if ( ! isIdentChar( ch )  &&  ! Character.isDigit( ch ) )
		    break;
	    }
	}
	else  if ( Character.isDigit( ch ) )
	{
	    type = NUMBER;

	    while( token_end < txt_len  &&  
		   Character.isDigit( text[ token_end ] ) )
		 token_end++;
	}
	else
	{
	    type = PUNCT;
	}
	break;
    }

    return( type );
} // nextToken

/*
** Name: prevToken
**
** Description:
**	Scan backwards for the previous token (starting 
**	at the beginning of the current token, token_beg) 
**	and return a general classification of the type 
**	of token found.  The position of the token is saved 
**	in the instance variables token_beg and token_end.
**
**	This method does not currently support returning
**	compound tokens which require matching of begin
**	and end characters such as strings and ODBC escape
**	sequences.  The last character of the compound
**	token will be returned with a classification of
**	punctuation.
**
**	Also, tokens ending in a decimal digit will be
**	treated as a numeric literal while they may have
**	identified as an identifier when scanned by
**	nextToken().	
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Token ID.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
**	13-Jun-00 (gordy)
**	    Support all specific punctuation.  Scanning of numerics now
**	    done by helper method, so only scan integers not decimals.
*/

private int
prevToken()
    throws EdbcEx
{
    int	    type;
    char    ch;

    while( --token_beg >= 0  &&  Character.isWhitespace( text[ token_beg ] ) );

    if ( token_beg < 0 )  
    {
	token_end = token_beg = 0;
	return( EOF );
    }

    token_end = token_beg + 1;

    switch( (ch = text[ token_beg ]) )
    {
    case '(' :	type = P_LPAREN;    break;
    case ')' :	type = P_RPAREN;    break;
    case '{' :	type = P_LBRACE;    break;
    case '}' :	type = P_RBRACE;    break;
    case ',' :	type = P_COMMA;	    break;
    case '?' :	type = P_QMARK;	    break;
    case '=' :	type = P_EQUAL;	    break;
    case '+' :	type = P_PLUS;	    break;
    case '-' :	type = P_MINUS;	    break;
    case '.' :	type = P_PERIOD;    break;

    default :
	if ( isIdentChar( ch ) )
	{
	    type = IDENT;

	    for( ; token_beg > 0; token_beg-- )
	    {
		ch = text[ token_beg - 1 ];
		if ( ! isIdentChar( ch )  &&  ! Character.isDigit( ch ) )
		    break;
	    }
	}
	else  if ( Character.isDigit( ch ) )
	{
	    type = NUMBER;

	    while( token_beg > 0  &&
		   Character.isDigit( text[ token_beg - 1 ] ) )
		token_beg--;
	}
	else
	{
	    type = PUNCT;
	}
	break;
    }

    return( type );
} // prevToken

/*
** Name: match
**
** Description:
**	Find the position of the matching character for a pair 
**	of characters which delimit text.  The first character 
**	is the determined by the beginning position in the text.
**	The second character is provided by the caller.  The two 
**	characters may be the same, as for quoted strings, or 
**	different, as for the pairs '(' and ')', '{' and '}', etc.
**
**	Nesting of the match characters is permitted.  An instance 
**	variable, nested, indicates if nesting was detected.
**
** Input:
**	txt	    Text.
**	beg	    Position of starting character.
**	end	    End of text.
**	end_ch	    Matching character.
**
** Output:
**	None.
**
** Returns:
**	int	    Position of matching character.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

private int
match( char txt[], int beg, int end, char end_ch )
    throws EdbcEx
{
    char    beg_ch = text[ beg ];
    int	    nesting = 0;
    
    nested = false;

    while( ++beg < end )  
    {
	char ch = text[ beg ];
	
	/*
	** The begin and end characters may be the same,
	** in which case nesting is not meaningful and
	** the end character test must be done first.
	*/
	if ( ch == end_ch )
	{
	    if ( nesting > 0 )
		nesting--;
	    else
		return( beg );
	}
	else  if ( ch == beg_ch )
	{
	    nesting++;
	    nested = true;
	}
    }

    throw EdbcEx.get( E_JD000E_UNMATCHED );
} // match

/*
** Name: keyword
**
** Description:
**	Searches a string table for an entry matching
**	a text token.  The string table must be upper-
**	case and sorted alphabetically.  UNKNOWN is
**	returned if token is not found in the table.
**
** Input:
**	txt	Text containing keyword.
**	beg	Beginning position of keyword.
**	end	Ending position of keyword.
**	table	Keyword table.
**
** Output:
**	None.
**
** Returns:
**	int	Index in table of match, or UNKNOWN.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

private static int
keyword( char txt[], int beg, int end, String table[] )
{
    int i, ix, length = end - beg;

    for( ix = 0; ix < table.length; ix++ )
	if ( length == table[ ix ].length() )
	{
	    for( i = 0; i < length; i++ )
	        if ( table[ ix ].charAt( i ) !=
		     Character.toUpperCase( txt[ beg + i ] ) )
		    break;

	    if ( i == length )  return( ix );
	}

    return( UNKNOWN );
} // keyword

/*
** Name: isIdentChar
**
** Description:
**	Determine if character is permitted in an identifier.
**
** Input:
**	ch	Character to be tested.
**
** Output:
**	None.
**
** Returns:
**	boolean	True if character is permitted in an identifier.
**
** History:
**	26-Oct-99 (gordy)
**	    Created.
*/

private static boolean
isIdentChar( char ch )
{
    return( Character.isLetter( ch )  ||  ch == '_'  ||  
	    ch == '$'  ||  ch == '@'  ||  ch == '#' );
} // isIdentChar

} // class EdbcSQL

