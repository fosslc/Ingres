/*
** Copyright (c) 2003, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.Collections.Specialized;
using System.Text;
using Ingres.Utility;


namespace Ingres.ProviderInternals
{
	/*
	** Name: sqlparse.cs
	**
	** Description:
	**	Defines a class which provides services for parsing SQL queries.
	**
	**  Classes:
	**
	**	SqlParse
	**
	** History:
	**	26-Oct-99 (gordy)
	**	    Created.
	**	12-Nov-99 (gordy)
	**	    Allow date formatter to be specified.
	**	11-Nov-99 (rajus01)
	**	    DatabaseMetaData Implementation.
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
	**	    Transform 'SELECT FOR UPDATE' syntax.  Check for
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
	**	    rather than gateway time.  Since clients appear as GMT,
	**	    CURDATE and NOW functions must be built as date literals of 
	**	    the current client time.
	**	31-Oct-01 (gordy)
	**	    Some gateways don't currently accept the date format used by
	**	    the driver/server for literals.  It has been found that
	**	    all supported DBMS accept the date format YYYY_MM_DD HH:MM:SS.
	**	    Local date formatters are now declared and the timezone for
	**	    the associated connection is configured.
	**      27-Nov-01 (loera01) 
	**          Added support for standard syntax for global temp table
	**          parameters.
	**	 7-Aug-02 (gordy)
	**	    Allow empty strings in date/time escape sequences.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	24-Feb-03 (gordy)
	**	    Parameter names allowed in call syntax.
	**	 3-Jul-03 (gordy)
	**	    Added support for DATABSE(), POWER(), and SOUNDEX() functions.
	**	21-Jul-03 (gordy)
	**	    Use local timezone with date literals since timezone independent.
	**	12-Sep-03 (gordy)
	**	    Date formatters moved to SqlDates utility.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	12-Sep-03 (gordy)
	**	    Date formatters moved to SqlDates utility.
	**	 1-Nov-03 (gordy)
	**	    Save table name from SELECT statement.
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	16-feb-10 (rajus01, ported by thoda04) Bug 123298
	**	    Added SendIngresDates support to send ingresdate functions
	**	    and literals rather than ANSI functions and literals
	**	    for escaped functions.
	**	12-apr-10 (thoda04)  SIR 123585
	**	    Add named parameter marker support.
	**	 4-jun-10 (thoda04)  Bug 123873
	**	    CREATE PROCEDURE should not have named parameter markers.
	**	    Don't try to replace ":myvar" host variables with "?".
	**	    Added 'CREATE PROCEDURE' to getQueryType().
	**	    Added keyword 'KW_CREATE'.  Added queryType 'QT_CREATE_PROC'.
	**	13-Jul-10 (gordy, ported by thoda04) B124115
	**	    Treat comments in SQL text as white space.
	*/



	/*
	** Name: SqlParse
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
	**	QT_CREATE_PROC	    Create procedure.
	**
	**  Public Methods:
	**
	**	getConvertTypes()   Get supported SQL types.
	**	getNumFuncs	    Get Ingres Numeric Functions.
	**	getStrFuncs	    Get Ingres String Functions.
	**	getTdtFuncs	    Get Ingres TimeDate Functions.
	**	getSysFuncs	    Get Ingres System Functions.
	**	getQueryType	    Determine type of SQL query/statement.
	**	getCursorName	    Get cursor for positioned UPDATE or DELETE.
	**	getTableName	    Get table name for SELECT FROM.
	**	getConcurrency	    Returns the statement concurrency type.
	**	parseSQL	    Convert SQL text to Server format.
	**	parseCall	    Convert CALL text to ProcInfo object.
	**
	**  Private Data:
	**	
	**	keywords	    SQL keywords.
	**	esc_seq		    ODBC escape sequences.
	**	esc_func	    ODBC functions.
	**	func_xlat	    Ingres functions (translated)
	**	types		    ODBC and Ingres conversion functions.
	**
	**	FUNC_IDATE_CONST    Ingres Date constant text.
	**	FUNC_ITIME_CONST    Ingres Time constant text.
	**	FUNC_ISTAMP_CONST   Ingres Timestamp constant text.
	**	FUNC_ADATE_CONST    ANSI Date constant text.
	**	FUNC_ATIME_CONST    ANSI Time constant text.
	**	FUNC_ASTAMP_CONST   ANSI Timestamp constant text.
	**	FUNC_IDATE_PREFIX   Ingres Date function prefix text.
	**	FUNC_IDATE_SUFFIX   Ingres Date function suffix text.
	**	FUNC_ADATE_PREFIX   ANSI Date literal prefix text.
	**	FUNC_ADATE_SUFFIX   ANSI Date literal suffix text.
	**	FUNC_TIME_PREFIX    ANSI Time literal prefix text.
	**	FUNC_TIME_SUFFIX    ANSI Time literal suffix text.
	**	FUNC_STAMP_PREFIX   ANSI Timestamp literal prefix text.
	**	FUNC_STAMP_SUFFIX   ANSI Timestamp literal suffix text.
	**
	**	conn		    Current connection.
	**	use_gmt		    Use GMT for date/time literals.
	**	sql		    Original SQL query.
	**	text		    Query text.
	**	txt_len		    Length of query in text.
	**	transformed	    True if query text has been modified.
	**	token_beg	    First character of current token in text.
	**	token_end	    Character following current token in text.
	**	nested		    True if nested character pairs found by match().	    
	**	query_type	    Query type.
	**	cursor		    Cursor name for positioned Delete/Update.
	**	for_readonly	    Does query contain 'FOR READONLY'.
	**	for_update	    Does query contain 'FOR UPDATE'.
	**
	**  Private Methods:
	**
	**	parseNumeric	    Parse a full numeric value.
	**	parseESC	    Convert ODBC escape sequence.
	**	parseConvert	    Convert ODBC escape function CONVERT.
	**	append_escape	    Copy escape text and process nested sequences.
	**	append_text	    Copy text array to string buffer.
	**	save_text	    Copy string buffer to text array.
	**	hex2bin		    Convert hex string to byte array.
	**	nextToken	    Scan for next token.
	**	prevToken	    Scan for previous token.
	**	comment  	    Scan a comment.
	**	match		    Find match for character pair.
	**	keyword		    Determine keyword ID of a token.
	**	isIdentChar	    Is character permitted in an identifier.
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
	**	31-Oct-02 (gordy)
	**	    Moved concurrancy and date literal format constants to 
	**	    DrvConst interface.  Renamed type to query_type.  Removed 
	**	    getNativeDBProcReturn() and isGTTParam() and moved 
	**	    functionality to parseCall().
	**	 3-Jul-02 (gordy)
	**	    Added FUNC_DB, FUNC_POWER, and FUNC_SNDEX.
	**	12-Sep-03 (gordy)
	**	    Replaced date formatters with utility SqlDates.  Timezone
	**	    replaced by GMT indicator use_gmt.
	**	 1-Nov-03 (gordy)
	**	    Added keyword FROM, table_name and getTableName().
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.  Added conn so all connection 
	**	    info is available. Added ANSI Date/Time literal prefix/suffix,
	**	    constants and escape functions.
	**	13-Jul-10 (gordy, ported by thoda04) B124115
	**	    Added comment() to scan comments in SQL text.
	*/

	class
		SqlParse : MsgConst
	{
		/*
		** Query types.
		*/
		public const int	QT_UNKNOWN  = 0;    // can't be UNKNOWN
		public const int	QT_SELECT   = 1;
		public const int	QT_DELETE   = 2;
		public const int	QT_UPDATE   = 3;
		public const int	QT_PROCEDURE= 4;
		public const int	QT_NATIVE_PROC = 5;
		public const int	QT_CREATE_PROC = 6;

		/*
		** General classification for tokens in SQL statements.
		*/
		private const int	UNKNOWN	    = -1;   // Unclassified.
		private const int	EOF	    = -2;   // End of query.
		private const int	STRING	    = 1;    // Quoted string
		private const int	IDENT	    = 2;    // Identifier, possible keyword.
		private const int	NUMBER	    = 3;    // Number.
		private const int	PUNCT	    = 4;    // General punctuation.
		private const int	ESCAPE	    = 5;    // ODBC escape sequence.

		private const int	P_LPAREN    = 10;   // Specific punctuation.
		private const int	P_RPAREN    = 11;
		private const int	P_LBRACE    = 12;
		private const int	P_RBRACE    = 13;
		private const int	P_COMMA	    = 14;
		private const int	P_QMARK	    = 15;
		private const int	P_EQUAL	    = 16;
		private const int	P_PLUS	    = 17;
		private const int	P_MINUS	    = 18;
		private const int	P_PERIOD    = 19;

		/*
		** Numeric types
		*/
		private const int	NUM_INT	    = 0;    // Integer
		private const int	NUM_DEC	    = 1;    // Decimal
		private const int	NUM_FLT	    = 2;    // Float

		/*
		** SQL keywords.
		**
		** The keyword ID must match position in keywords array.
		*/
		private const int	KW_CALLPROC = 0;
		private const int	KW_CURRENT  = 1;
		private const int	KW_DELETE   = 2;
		private const int	KW_EXECUTE  = 3;
		private const int	KW_FOR	    = 4;
		private const int	KW_FROM	    = 5;
		private const int	KW_INTO     = 6;
		private const int	KW_OF	    = 7;
		private const int	KW_PROCEDURE = 8;
		private const int	KW_READONLY = 9;
		private const int	KW_SELECT   = 10;
		private const int	KW_SESSION  = 11;
		private const int	KW_UPDATE   = 12;
		private const int	KW_WHERE    = 13;
		private const int	KW_CREATE   = 14;

		private static String[]	keywords =
		{
		"CALLPROC",	"CURRENT",	"DELETE",	"EXECUTE",	
		"FOR",		"FROM",		"INTO",		"OF", 		
		"PROCEDURE", 	"READONLY",	"SELECT",	"SESSION",	
		"UPDATE",	"WHERE",	"CREATE"
		};

		/*
		** ODBC escape sequences
		**
		** The sequence ID must match position in esc_seq array.
		*/
		private const int	ESC_CALL    = 0;
		private const int	ESC_DATE    = 1;
		private const int	ESC_ESC	    = 2;
		private const int	ESC_FUNC    = 3;
		private const int	ESC_OJ	    = 4;
		private const int	ESC_TIME    = 5;
		private const int	ESC_STAMP   = 6;

		private static String[] esc_seq =
	{ "CALL", "D", "ESCAPE", "FN", "OJ", "T", "TS" };

		/*
		** ODBC scalar functions
		**
		** The function ID must match position in esc_func array.
		** The corresponding Ingres function must appear in the
		** same position in the func_xlat array.
		*/
		private const int	FUNC_ABS    = 0;
		private const int	FUNC_ATAN   = 1;
		private const int	FUNC_CONCAT = 2;
		private const int	FUNC_CNVRT  = 3;
		private const int	FUNC_COS    = 4;
		private const int	FUNC_CDATE  = 5;
		private const int	FUNC_ANSICD = 6;
		private const int	FUNC_ANSICT = 7;
		private const int    FUNC_ANSICS = 8;
		private const int	FUNC_CTIME  = 9;
		private const int	FUNC_DB	    = 10;
		private const int	FUNC_DOW    = 11;
		private const int	FUNC_EXP    = 12;
		private const int	FUNC_IFNULL = 13;
		private const int	FUNC_LCASE  = 14;
		private const int	FUNC_LEFT   = 15;
		private const int	FUNC_LENGTH = 16;
		private const int	FUNC_LOG    = 17;
		private const int	FUNC_MOD    = 18;
		private const int	FUNC_NOW    = 19;
		private const int	FUNC_POWER  = 20;
		private const int	FUNC_RIGHT  = 21;
		private const int	FUNC_RTRIM  = 22;
		private const int	FUNC_SIN    = 23;
		private const int	FUNC_SNDEX  = 24;
		private const int	FUNC_SQRT   = 25;
		private const int	FUNC_UCASE  = 26;
		private const int	FUNC_USER   = 27;

		private static String[] esc_func = 
		{"ABS",      "ATAN",    "CONCAT",  "CONVERT", "COS",   "CURDATE",
		"CURRENT_DATE", "CURRENT_TIME",   "CURRENT_TIMESTAMP","CURTIME",  
		"DATABASE", "DAYNAME", "EXP",     "IFNULL",  "LCASE", "LEFT", 
		"LENGTH",   "LOG",     "MOD",     "NOW",     "POWER", "RIGHT", 
		"RTRIM",    "SIN",     "SOUNDEX", "SQRT",	   "UCASE", "USER" };

		private static String[] func_xlat = 
		{ "ABS",      "ATAN",    "CONCAT",  "CONVERT", "COS",       "?", 
		  "?",        "?",       "?",       "?",       "DBMSINFO('DATABASE')", 
		  "DOW",     "EXP",     "IFNULL",  "LOWERCASE", "LEFT", 
		  "LENGTH",   "LOG",     "MOD",     "?",       "POWER",     "RIGHT", 
		  "TRIM",     "SIN",     "SOUNDEX", "SQRT",    "UPPERCASE", "USER" };

		private static Hashtable  types = new Hashtable();
    
		private const String	FUNC_IDATE_CONST  = "date('today')";
		private const String	FUNC_ITIME_CONST  = "date('now')";
		private const String	FUNC_ISTAMP_CONST = "date('now')";
		private const String	FUNC_ADATE_CONST  = "CURRENT_DATE";
		private const String	FUNC_ATIME_CONST  = "CURRENT_TIME";
		private const String	FUNC_ASTAMP_CONST = "CURRENT_TIMESTAMP";
		private const String	FUNC_IDATE_PREFIX = "date('";
		private const String	FUNC_IDATE_SUFFIX = "')";
		private const String	FUNC_ADATE_PREFIX = "DATE'";
		private const String	FUNC_ADATE_SUFFIX = "'";
		private const String	FUNC_TIME_PREFIX  = "TIME'";
		private const String	FUNC_TIME_SUFFIX  = "'";
		private const String	FUNC_STAMP_PREFIX = "TIMESTAMP'";
		private const String	FUNC_STAMP_SUFFIX = "'";

		private DrvConn		conn = null;
		private bool		use_gmt = false;
		private bool		ansi_date_syntax = false;
		private String sql = null;
		private char[]		text = null;
		private int			txt_len;
		private bool		transformed = false;
		private int			token_beg = 0;	    // nextToken() locations
		private int			token_end = 0;
		private bool		nested = false;	    // match() scanned nested pairs
		private int			query_type = UNKNOWN;
		private bool		procedure_return = false;
		private String		cursor = null;
		private String		table_name = null;
		private bool		for_update = false;
		private bool		for_readonly = false;


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

		static SqlParse() // Class Initializer
		{
			/*
			** Supported CONVERT() function SQL types and
			** their equivalent Ingres conversion function.
			*/
			types.Add( "BINARY", "BYTE" );
			types.Add( "CHAR", "CHAR" );
			types.Add( "DECIMAL", "DECIMAL" );
			types.Add( "DOUBLE", "FLOAT8" );
			types.Add( "FLOAT", "FLOAT8" );
			types.Add( "INTEGER", "INT4" );
			types.Add( "LONGVARBINARY", "LONG_BYTE" );
			types.Add( "LONGVARCHAR", "LONG_VARCHAR" );
			types.Add( "NUMERIC", "DECIMAL" );
			types.Add( "REAL", "FLOAT4" );
			types.Add( "SMALLINT", "INT2" );
			types.Add( "TIMESTAMP", "DATE" );
			types.Add( "TINYINT", "INT1" );
			types.Add( "VARBINARY", "VARBYTE" );
			types.Add( "VARCHAR", "VARCHAR" );
		} // static


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

//		public static Enumeration
//			getConvertTypes()
//		{
//			return( types.keys() );
//		}


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
		**	31-Oct-02 (gordy)
		**	    Use StringBuilder for efficiency.  Remove dependency on constants.
		**	 3-Jul-03 (gordy)
		**	    Add POWER() function.
		*/

		public static String 
			getNumFuncs()
		{
			StringBuilder funcs = new StringBuilder();

			for( int i = 0; i < esc_func.Length; i++ )
				switch ( i )
				{
					case FUNC_ABS: 
					case FUNC_ATAN: 
					case FUNC_COS: 
					case FUNC_EXP: 
					case FUNC_LOG: 
					case FUNC_MOD: 
					case FUNC_POWER:
					case FUNC_SIN: 
					case FUNC_SQRT: 
						if ( funcs.Length > 0 )  funcs.Append( "," );
						funcs.Append( esc_func[ i ] ); 
						break;
				}

			return( funcs.ToString() );
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
		**	31-Oct-02 (gordy)
		**	    Use StringBuilder for efficiency.  Remove dependency on constants.
		**	 3-Jul-03 (gordy)
		**	    Add SOUNDEX() function.
		*/

		public static String 
			getStrFuncs()
		{
			StringBuilder funcs = new StringBuilder();

			for( int i = 0; i < esc_func.Length; i++ )
				switch ( i )
				{
					case FUNC_CONCAT: 
					case FUNC_LCASE: 
					case FUNC_LEFT: 
					case FUNC_LENGTH: 
					case FUNC_RIGHT: 
					case FUNC_RTRIM:
					case FUNC_SNDEX:
					case FUNC_UCASE: 
						if ( funcs.Length > 0 )  funcs.Append( "," );
						funcs.Append( esc_func[ i ] ); 
						break;
				}

			return( funcs.ToString() );
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
		**	31-Oct-02 (gordy)
		**	    Use StringBuilder for efficiency.  Remove dependency on constants.
		*/

		public static String 
			getTdtFuncs()
		{
			StringBuilder funcs = new StringBuilder();

			for( int i = 0; i < esc_func.Length; i++ )
				switch ( i )
				{
					case FUNC_CDATE:
					case FUNC_ANSICD:
					case FUNC_ANSICT:
					case FUNC_ANSICS:
					case FUNC_CTIME:
					case FUNC_DOW:
					case FUNC_NOW:
						if (funcs.Length > 0) funcs.Append(",");
						funcs.Append( esc_func[ i ] ); 
						break;
				}

			return( funcs.ToString() );
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
		**	31-Oct-02 (gordy)
		**	    Use StringBuilder for efficiency.  Remove dependency on constants.
		**	 3-Jul-03 (gordy)
		**	    Add DATABASE() function.
		*/

		public static String 
			getSysFuncs()
		{
			StringBuilder funcs = new StringBuilder();

			for( int i = 0; i < esc_func.Length; i++ )
				switch ( i )
				{
					case FUNC_DB:
					case FUNC_IFNULL: 
					case FUNC_USER: 
						if ( funcs.Length > 0 )  funcs.Append( "," );
						funcs.Append( esc_func[ i ] ); 
						break;
				}

			return( funcs.ToString() );
		}


		/*
		** Name: SqlParse
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	sql	    SQL query text.
		**	conn	Current connection.
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
		**	12-Sep-03 (gordy)
		**	    Replaced timezone with GMT indicator.
		**	19-Jun-06 (gordy)
		**	    I give up... just pass connection so all info is available.
		**	16-feb-10 (rajus01, ported by thoda04) Bug 123298
		**	    Added SendIngresDates support to send ingresdate functions
		**	    and literals rather than ANSI functions and literals
		**	    for escaped functions.
		*/

		public
			SqlParse(String sql, DrvConn conn)
		{
			this.sql = sql;
			this.conn = conn;
			ansi_date_syntax = (conn.sqlLevel >= DBMS_SQL_LEVEL_ANSI_DT  &&
			                    !conn.sendIngresDates);
			use_gmt = conn.timeLiteralsInGMT();
			text = sql.ToCharArray();
			txt_len = text.Length;
			return;
		} // Sqlparse


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
		**	31-Oct-02 (gordy)
		**	    Procedure return processing moved to parseCall().
		*/

		public int
			getQueryType()
		{
			int token;

			if ( query_type == UNKNOWN )
			{
				token_beg = token_end = 0;
				query_type = QT_UNKNOWN;

				switch( nextToken( false ) )
				{
					case P_LPAREN :		query_type = QT_SELECT;		break;

					case IDENT :
					switch( keyword( text, token_beg, token_end, keywords ) )
					{
						case KW_SELECT :	query_type = QT_SELECT;		break;
						case KW_DELETE :	query_type = QT_DELETE;		break;
						case KW_UPDATE :	query_type = QT_UPDATE;		break;
						case KW_CALLPROC :	query_type = QT_NATIVE_PROC;	break;

						case KW_EXECUTE :
							if ( nextToken( false ) == IDENT  &&
								keyword( text, token_beg, 
								token_end, keywords ) == KW_PROCEDURE )
								query_type = QT_NATIVE_PROC;
							break;

						case KW_CREATE :    // CREATE PROCEDURE
							if ( nextToken( false ) == IDENT  &&
								keyword( text, token_beg, 
								token_end, keywords ) == KW_PROCEDURE )
								query_type = QT_CREATE_PROC;
							break;

						case KW_PROCEDURE : // [CREATE] PROCEDURE w/o noise word
								query_type = QT_CREATE_PROC;
							break;
					}
						break;

					case P_LBRACE :	
						if ( (token = nextToken( false )) == P_QMARK )
							if ( nextToken( false ) != P_EQUAL )  
								break;  // Invalid syntax?
							else
							{
								procedure_return = true;
								token = nextToken( false );
							}

						if ( token == IDENT  &&  
							keyword( text, token_beg, token_end, esc_seq ) == ESC_CALL )
							query_type = QT_PROCEDURE;
						else
							procedure_return = false;   // Invalid syntax?
						break;
				}
			}

			return( query_type );
		} // getQueryType


		/*
		** Name: getCursorName
		**
		** Description:
		**	If the SQL text is a cursor positioned DELETE or UPDATE,
		**	the name of the cursor specified is returned.  The text
		**	is also modified by removing the WHERE clause as is
		**	required by the Ingres server.  This method should
		**	only be called prior to parseSQL().
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
		**	31-Oct-02 (gordy)
		**	    Don't allocate cursor name until syntax verified.
		*/

		public String
			getCursorName()
		{
			if ( cursor != null )  return( cursor );
			if ( query_type == UNKNOWN )  getQueryType();

			if ( query_type == QT_DELETE  ||  query_type == QT_UPDATE )
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
							int	crsr_beg = token_beg;
							int crsr_end = token_end;

							if ( nextToken( false ) == EOF )
							{
								cursor = new String(text,crsr_beg,crsr_end - crsr_beg);
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
				if ( cursor == null )  query_type = QT_UNKNOWN;
			}

			return( cursor );
		} // getCursorName


		/*
		** Name: getTableName
		**
		** Description:
		**	Returns the table name referenced in a SELECT statement.  The 
		**	first table named in a FROM clause is returned exactly as it 
		**	exists in the query text including schema and delimiters.  NULL
		**	is returned if the statement is not a SELECT or FROM <table>
		**	could not be parsed.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Table name or NULL.
		**
		** History:
		**	 1-Nov-03 (gordy)
		**	    Created.
		*/

		public String
		getTableName()
		{
			return (table_name);
		} // getTableName


		/*
		** Name: getConcurrency
		**
		** Description:
		**	Returns the query concurrency based on presence of 'FOR UPDATE' 
		**	or 'FOR READONLY' clause in SQL text.  Only valid after calling
		**	parseSQL().
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Concurrency: DRV_CRSR_{DBMS,READONLY,UPDATE}.
		**
		** History:
		**	20-Aug-01 (gordy)
		**	    Created.
		**	31-Oct-02 (gordy)
		**	    Concurrency constants moved to DrvConst interface.
		*/

		public int
			getConcurrency()
		{
			int concurrency;

			if ( for_readonly )
				concurrency = DrvConst.DRV_CRSR_READONLY;
			else  if ( for_update )
				concurrency = DrvConst.DRV_CRSR_UPDATE;
			else
				concurrency = DrvConst.DRV_CRSR_DBMS;

			return( concurrency );
		} // getConcurrency


		/*
		** Name: parseSQL
		**
		** Description:
		**	Return the SQL text transformed into Ingres format.  
		**	This method should not be called before getCursorName()
		**	(getCursorName() is not required).
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
		**	    Transform 'SELECT FOR UPDATE' syntax.  Check for
		**	    'FOR UPDATE' and 'FOR READONLY' clauses.
		**	20-Aug-01 (gordy)
		**	    Remove 'FOR READONLY' clause as it is not a part of Ingres
		**	    SELECT syntax.
		**	 1-Nov-03 (gordy)
		**	    Save table name for 'SELECT ... FROM <table>'.
		*/

		public String
			parseSQL(bool escapes)
		{
			return parseSQL(escapes, null);
		}


		public String
			parseSQL(bool escapes, StringCollection parameterMarkers)
		{
			StringBuilder    buff = null;
			int    token, tkn_beg, tkn_end, start;
			String parameterMarker;

			if ( query_type == UNKNOWN )  getQueryType();
			start = token_beg = token_end = 0;

			while( (token = nextToken( escapes )) != EOF )
				switch( token )
				{
					case IDENT :
					switch( keyword( text, token_beg, token_end, keywords ) )
					{
						case KW_SELECT :
							/*
							** The spec requires support for the syntax
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
									token_beg = token_end = buff.Length;
									buff = append_text( text,txt_len,start,txt_len,buff );
									buff.Append( " for update" );
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
							if ( query_type != QT_SELECT )  break;
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
						case KW_FROM:
							/*
							** Get table name for 'SELECT ... FROM <table>'
							*/
							if (query_type != QT_SELECT || table_name != null)
								break;

							tkn_beg = token_beg;	// Save in case no table name.
							tkn_end = token_end;

							switch (nextToken(false))
							{
								case IDENT:	// Identifier
								case STRING:	// Delimited identifier
									/*
									** The table reference could include a schema.
									** Save the current token in case it is a simple
									** table reference.
									*/
									tkn_beg = token_beg;
									tkn_end = token_end;

									/*
									** Check for schema.table construct
									*/
									if (nextToken(false) == P_PERIOD)
									{
										switch (nextToken(false))
										{
											case IDENT:	// Identifier
											case STRING:	// Delimited identifier
												tkn_end = token_end;    // Include with schema
												break;
										}
									}

									/*
									** Now save the table reference as it appears.
									*/
									table_name = new String(text, tkn_beg, tkn_end - tkn_beg);
									break;
							}

							token_beg = tkn_beg;	// Reset to last valid token
							token_end = tkn_end;
							break;
					}  // end switch( keyword( text, token_beg, token_end, keywords )  in IDENT
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

					case P_QMARK:
						if (query_type == QT_CREATE_PROC)  // don't touch host vars
							break;                         // if CREATE PROCEDURE
						parameterMarker =
							new String(text, token_beg, token_end - token_beg);
						ValidateParameterMarker(parameterMarker, parameterMarkers);
						if (parameterMarkers != null)
							parameterMarkers.Add(parameterMarker);

						buff = append_text( text, txt_len, start, token_beg, buff );
						buff.Append('?');
						start = token_end;
						break;
				}  // end switch( token )

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
		**	Populates a procedure information object with the info 
		**	parsed from the request text.
		**
		**	The following Database Procedure call syntax is accepted:
		**
		**	{[? =] CALL [schema.]name[( params )]}
		**	EXECUTE PROCEDURE [schema.]name[( params )] [INTO ?]
		**	CALLPROC [schema.]name[( params )] [INTO ?]
		**      
		**	params: param_spec | param_spec, params
		**	param_spec: [name =] [value]
		**	value: ? | numeric_literal | char_literal | hex_string |
		**		SESSION.table_name
		**
		**	Note that parameter values are optional and will be flagged
		**	as default parameters.  An empty parenthesis set implies a 
		**	single default parameter.
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
		**          Added support for standard syntax for global temp table
		**          parameters.
		**	31-Oct-02 (gordy)
		**	    Moved procedure return value and global temp table parameter
		**	    parsing into this method.
		**	24-Feb-03 (gordy)
		**	    ProcInfo parameter index is now 0 based.  Permit named
		**	    parameters in escape syntax.
		*/

		public void
			parseCall( ProcInfo procInfo )
		{
			int		token, index;
			String	name;

			/*
			** By calling getQueryType() we verify that this is
			** a procedure call request, determine if procedure
			** return value is expected, and position the token
			** scanner to read the procedure name (need to force
			** type to UNKNOWN otherwise getQueryType() may not
			** do the proper scanning).
			*/
			query_type = UNKNOWN;
			getQueryType();
    
			if ( (query_type != QT_PROCEDURE  &&  query_type != QT_NATIVE_PROC)  ||  
				nextToken( false ) != IDENT )
				throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
    
			/*
			** Save the procedure name, but check for schema qualification.
			*/
			name = new String( text, token_beg, token_end - token_beg );
			token = nextToken( false );

			if ( token == P_PERIOD )
			{
				if ( nextToken( false ) != IDENT )
					throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
	
				procInfo.setSchema( name );
				name = new String( text, token_beg, token_end - token_beg );
				token = nextToken( false );
			}

			procInfo.setName( name );

			/*
			** Process the optional parameter list.
			*/
			if ( token == P_LPAREN )
			{
				for(
					index = 0, token = nextToken( false );
					token != EOF;
					index++, token = nextToken( false )
					)
				{
					/*
					** Process next parameter value.
					*/
					switch( token )
					{
						case P_RPAREN :	// End of parameter list.
							procInfo.setParam( index, ProcInfo.PARAM_DEFAULT, null );
							goto break_for_loop;		// We be done.

						case P_COMMA :	// Missing parameter.
							procInfo.setParam( index, ProcInfo.PARAM_DEFAULT, null );
							goto continue_for_loop;	// Ready for next parameter.

						case P_QMARK :	// Dynamic parameter marker
							procInfo.setParam( index, ProcInfo.PARAM_DYNAMIC, null );
							break;

						case STRING :	// String parameter value.
							procInfo.setParam( index, ProcInfo.PARAM_CHAR,
								new String( text, token_beg + 1,
								token_end - token_beg - 2 ) );	// skip quotes
							break;

						case P_PLUS :	// Numeric parameter value.
						case P_MINUS :
						case P_PERIOD :
						case NUMBER :
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
										value = Int32.Parse( str );
										break;

									case NUM_DEC :
										type = ProcInfo.PARAM_DEC;
										value = Decimal.Parse( str );
										break;

									case NUM_FLT :
										type = ProcInfo.PARAM_FLOAT;
										value = Double.Parse( str );
										break;

									default :
										throw new FormatException();
								}

								procInfo.setParam( index, type, value );
							}
							catch( Exception )
							{
								throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
							}
							break;

						case IDENT:
							/*
							** This may be a Global Temp Table parameter, a hex 
							** literal or parameter name.  The following token 
							** differentiates between the three possibilities.
							*/
							int prev_end = token_end;
							int prev_beg = token_beg;

						switch( nextToken( false ) )
						{
							case P_PERIOD :	// Global Temp Table?
								if ( 
									keyword( text, prev_beg, 
									prev_end, keywords ) == KW_SESSION  && 
									nextToken( false ) == IDENT 
									)
									procInfo.setParam( index, ProcInfo.PARAM_SESSION, 
										new String(text,token_beg,token_end - token_beg) );
								else
									throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
								break;

							case STRING :	// Hex literal?
								if ( 
									(prev_end - prev_beg) == 1  &&
									(text[ prev_beg ] == 'X' || text[ prev_beg ] == 'x')
									)
									procInfo.setParam( index, ProcInfo.PARAM_BYTE,
										hex2bin( text, token_beg + 1, 
										token_end - token_beg - 2 ) );
								else
									throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
								break;

							case P_EQUAL :	// Parameter name
								procInfo.setParamName( index, 
									new String( text, prev_beg, prev_end - prev_beg ) );

								/*
								** The parameter value can be processed by
								** restarting at the top of the loop (but
								** need to avoid incrementing the index).
								*/
								index--;
								goto continue_for_loop;

							default :
								throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
						}
							break;

						default:
							throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
					}

					/*
					** A parameter has just been processed.  Either another
					** parameter follows, indicated by a separating comma,
					** or the end of the parameter list is expected (the
					** closing parenthesis).
					*/
					if ( (token = nextToken( false )) == P_RPAREN )
						break;
					else  if ( token != P_COMMA )
						throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
				continue_for_loop:
					continue;
				}  // end for loop
			break_for_loop:

				/*
				** We should be at the end of the parameter list.
				*/
				if ( token == P_RPAREN )
					token = nextToken( false );
				else
					throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
			}

			/*
			** Check syntax termination.  For the escape sequence this
			** is just the closing brace: '}'.  For native Ingres, only the
			** optional return value is expected: 'INTO ?'.
			*/
			if ( query_type == QT_PROCEDURE )
			{
				if ( token != P_RBRACE  ||  nextToken( false ) != EOF )  
					throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
			}
			else  if ( token != EOF )
			{
				if ( 
					token == IDENT  &&
					keyword( text, token_beg, token_end, keywords ) == KW_INTO  &&
					nextToken( false ) == P_QMARK  &&
					nextToken( false ) == EOF 
					)
					procedure_return = true;
				else
					throw SqlEx.get( ERR_GC4014_CALL_SYNTAX );
			}

			procInfo.setReturn( procedure_return );
			return;
		} // parseCall


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
		**	    when supporting date('now').  Since all clients appear to
		**	    be GMT, we must produce a timestamp literal of the current time
		**	    rather than use date('now').  This is done for all DBMS servers
		**	    since we can't tell here if this is a gateway connection.
		**	31-Oct-01 (gordy)
		**	    Some gateways don't currently accept the date format used by
		**	    the driver/server for literals.  It has been found that
		**	    all supported DBMS accept the date format YYYY_MM_DD HH:MM:SS.
		**	    Local date formatters are now declared and the timezone for
		**	    the associated connection is configured.
		**	 7-Aug-02 (gordy)
		**	    Allow empty strings in date/time escape sequences which will
		**	    be translated as an Ingres empty date by DBMS/gateway.
		**	 3-Jul-03 (gordy)
		**	    Translate DATABASE() function.
		**	21-Jul-03 (gordy)
		**	    Use local timezone with date literals since timezone independent.
		**	12-Sep-03 (gordy)
		**	    Replaced date formatters with SqlDates utility.
		**	19-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.
		**	15-Sep-06 (gordy)
		**	    Support empty date/time/timestamp literals with Ingres dates.
		**	    Use .NET DateTime class to parse escape
		**	    literals since they should be in .NET format.  WITH TZ now
		**	    formats with local rather than UTC time.  Fractional
		**	    seconds supported for ANSI timestamps. 
		*/

		private void
			parseESC( int beg, int end, bool nested, StringBuilder buff )
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
				buff.Append( '{' );
				append_escape( beg + 1, end - 1, nested, buff );
				buff.Append( '}' );
				return;
			}

			switch( type )
			{
				case ESC_DATE :
				case ESC_TIME :
				case ESC_STAMP :
					{
						if ((token = nextToken(false)) != STRING)
						{
							/*
							** Pass through argument without conversion.
							*/
							append_escape(token_beg, end - 1, nested, buff);
							break;
						}

						String val = new String(text, token_beg + 1, token_end - token_beg - 2);

						if (val.Length == 0)
						{
							/*
							** Empty dates are only supported by Ingres dates.
							*/
							buff.Append(FUNC_IDATE_PREFIX);
							buff.Append(FUNC_IDATE_SUFFIX);
							break;
						}

						switch (type)
						{
							case ESC_DATE:
								{
									/*
									** Validate date literal format.
									*/
									DateTime date = DateTimeParseInvariant(val);

									if (ansi_date_syntax)
									{
										/*
										** ANSI date values are independent of 
										** timezones.  Format into ANSI date 
										** literal using local TZ.
										*/
										buff.Append(FUNC_ADATE_PREFIX);
										buff.Append(SqlDates.formatDate(date, false));
										buff.Append(FUNC_ADATE_SUFFIX);
									}
									else
									{
										/*
										** Ingres date only values are independent
										** of timezones.  Format into Ingres date 
										** literal using local TZ.
										*/
										buff.Append(FUNC_IDATE_PREFIX);
										buff.Append(SqlDates.formatDateLit(date, false));
										buff.Append(FUNC_IDATE_SUFFIX);
									}
									break;
								} // case ESC_DATE

							case ESC_TIME:
								{
									/*
									** Validate time literal format.
									*/
									DateTime time = DateTimeParseInvariant(val);

									if (ansi_date_syntax)
									{
										/*
										** Format into ANSI time literal.  For OpenSQL 
										** use WITHOUT TZ format, WITH TZ otherwise.
										** Both formats use local time values.  Java 
										** applies TZ/DST of 'epoch' date: 1970-01-01.
										** Ingres applies TZ/DST of todays date, so 
										** use current date/time to determine explicit 
										** TZ offset.
										*/
										buff.Append(FUNC_TIME_PREFIX);
										buff.Append(SqlDates.formatTime(time, false));
										if (!conn.osql_dates)
											buff.Append(SqlDates.formatTZ(DateTime.Now));
										buff.Append(FUNC_TIME_SUFFIX);
									}
									else
									{
										/*
										** Ingres forces current date for time only
										** values while JDBC requires date EPOCH.
										** Validate JDBC time literal format and
										** format into Ingres timestamp (to enforce
										** JDBC date component) using connection
										** timezone.
										*/
										buff.Append(FUNC_IDATE_PREFIX);
										buff.Append(SqlDates.formatTimestampLit(time, use_gmt));
										buff.Append(FUNC_IDATE_SUFFIX);
									}
									break;
								} // case ESC_TIME

							case ESC_STAMP:
								{
									/*
									** Validate timestamp literal format.
									*/
									DateTime ts = DateTimeParseInvariant(val);
									int nanos = SqlDates.getNanos(ts);

									if (ansi_date_syntax)
									{
										/*
										** Format into ANSI timestamp literal.  For OpenSQL 
										** use WITHOUT TZ format, WITH TZ otherwise.
										** Both formats use local time values.
										*/
										buff.Append(FUNC_STAMP_PREFIX);
										buff.Append(SqlDates.formatTimestamp(ts, false));
										if (nanos != 0) buff.Append(SqlDates.formatFrac(nanos));
										if (!conn.osql_dates) buff.Append(SqlDates.formatTZ(ts));
										buff.Append(FUNC_STAMP_SUFFIX);
									}
									else
									{
										/*
										** Format into Ingres timestamp using 
										** connection timezone.
										*/
										buff.Append(FUNC_IDATE_PREFIX);
										buff.Append(SqlDates.formatTimestampLit(ts, use_gmt));
										buff.Append(FUNC_IDATE_SUFFIX);
									}
									break;
								} // case ESC_STAMP
						} // switch()
						break;
					}

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
							case FUNC_DB:
								buff.Append( func_xlat[ type ] );   // Function replacement.
								break;
		
							case FUNC_CNVRT :
								parseConvert( token_beg, end - 1, nested, buff );
								break;

							case FUNC_CDATE:		// TODO: enforce ()?
							case FUNC_ANSICD:
								/*
								** Date constants only work when DBMS knows client TZ.
								*/
								if (!conn.is_gmt)
									buff.Append(ansi_date_syntax ? FUNC_ADATE_CONST
												  : FUNC_IDATE_CONST);
								else if (ansi_date_syntax)
								{
									/*
									** Format current day as ANSI literal using 
									** local TZ since date literals are not 
									** dependent on timezone settings.
									*/
									buff.Append(FUNC_ADATE_PREFIX);
									buff.Append(SqlDates.formatDate(
										DateTime.Now, false));
									buff.Append(FUNC_ADATE_SUFFIX);
								}
								else
								{
									/*
									** Format current day as Ingres literal using 
									** local TZ since date only literals are not 
									** dependent on timezone settings.
									*/
									buff.Append(FUNC_IDATE_PREFIX);
									buff.Append(SqlDates.formatDateLit(
										DateTime.Now, false));
									buff.Append(FUNC_IDATE_SUFFIX);
								}
								break;

							case FUNC_CTIME:		// TODO: enforce ()?
							case FUNC_ANSICT:
								/*
								** Time constants only work when DBMS knows client TZ.
								*/
								if (!conn.is_gmt)
									buff.Append(ansi_date_syntax ? FUNC_ATIME_CONST
												  : FUNC_ITIME_CONST);
								else if (ansi_date_syntax)
								{
									/*
									** Format current time as ANSI literal.  For OpenSQL
									** use WITHOUT TZ format, WITH TZ otherwise.  Both
									** formats use local time values.  Java applies TZ &
									** DST of 'epoch' date: 1970-01-01.  Ingres applies 
									** TZ & DST of todays date, so use current date/time 
									** to determine explicit TZ offset.
									*/
									DateTime date = DateTime.Now;
									buff.Append(FUNC_TIME_PREFIX);
									buff.Append(SqlDates.formatTime(date, false));
									if (!conn.osql_dates)
										buff.Append(SqlDates.formatTZ(DateTime.Now));
									buff.Append(FUNC_TIME_SUFFIX);
								}
								else
								{
									/*
									** Format current time as Ingres literal using
									** appropriate TZ for connection.  Note that
									** Ingres only partially supports time-only
									** values, so use timestamp format.
									*/
									buff.Append(FUNC_IDATE_PREFIX);
									buff.Append(SqlDates.formatTimestampLit(
										new DateTime(), use_gmt));
									buff.Append(FUNC_IDATE_SUFFIX);
								}
								break;

							case FUNC_NOW:	// TODO: enforce ()?
							case FUNC_ANSICS:
								/*
								** Timestamp constants only work when DBMS knows client TZ.
								*/
								if (!conn.is_gmt)
									buff.Append(ansi_date_syntax ? FUNC_ASTAMP_CONST
												  : FUNC_ISTAMP_CONST);
								else if (ansi_date_syntax)
								{
									/*
									** Format current timestamp as ANSI literal.
									** For OpenSQL use WITHOUT TZ format, WITH TZ
									** otherwise.  Both use local time values.
									*/
									DateTime date = DateTime.Now;
									buff.Append(FUNC_STAMP_PREFIX);
									buff.Append(SqlDates.formatTimestamp(date, false));
									if (!conn.osql_dates)
										buff.Append(SqlDates.formatTZ(date));
									buff.Append(FUNC_STAMP_SUFFIX);
								}
								else
								{
									/*
									** Format current timestamp as Ingres literal 
								** using appropriate TZ for connection.
									*/
									buff.Append(FUNC_IDATE_PREFIX);
									buff.Append(SqlDates.formatTimestampLit(
										DateTime.Now, use_gmt));
									buff.Append(FUNC_IDATE_SUFFIX);
								}
								break;

							case FUNC_USER:		// TODO: enforce ()?
								buff.Append(func_xlat[type]);   // Function replacement.
								break;

							default:
								buff.Append( func_xlat[ type ] );   // Translate function name.
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
					buff.Append( '{' );
					append_escape( beg + 1, end - 1, nested, buff );
					buff.Append( '}' );
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
			parseConvert( int beg, int end, bool nested, StringBuilder buff )
		{
			int	    token;
			bool success = false;

			token_beg = token_end = beg;
    
			if ( (token = nextToken( false )) == IDENT  &&
				(token = nextToken( false )) == P_LPAREN )
			{
				int start = token_end;
				token_beg = token_end = end;
	
				if ( (token = prevToken()) == P_RPAREN  &&  (token = prevToken()) == IDENT )
				{
					String sqlType = new String( text, token_beg, token_end - token_beg );
		
					if ( (sqlType =
							(String)types[ ToInvariantUpper(sqlType) ]) != null  &&
						(token = prevToken()) == P_COMMA )
					{
						buff.Append( sqlType );
						buff.Append( '(' );
						append_escape( start, token_beg, nested, buff );
						buff.Append( ')' );
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
			append_escape( int beg, int end, bool nested, StringBuilder buff )
		{
			if ( ! nested )	// Easy case: no further processing needed.
				buff.Append( text, beg, end - beg );
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
							buff.Append( text, beg, token_beg - beg );
						parseESC( token_beg, (beg = token_end), this.nested, buff );
						token_beg = token_end = beg;
					}

				if ( beg < end )	// append trailing text.
					buff.Append( text, beg, end - beg );
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
		**	StringBuilder	New string buffer.
		**
		** History:
		**	20-Aug-01 (gordy)
		**	    Created.
		*/

		private StringBuilder
		append_text( char[] text, int txt_len, int start, int end, StringBuilder buff )
		{
			if ( buff == null )  buff = new StringBuilder( txt_len );
			if ( end > start )  buff.Append( text, start, end - start );
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
			save_text( StringBuilder buff )
		{
			/*
			** Expand text array if needed.
			*/
			if ( buff.Length > text.Length )
				text = new char[ buff.Length ];
			txt_len = buff.Length;

//			buff.getChars( 0, txt_len, text, 0 );
			buff.CopyTo(0, text, 0, txt_len);

			buff.Length = 0;
			return;
		} // save_text


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

		private byte[]
		hex2bin( char[] hex, int offset, int len )
		{
			byte[] ba = new byte[ len / 2 ];
			try
			{
				for( int i = 0; i < ba.Length; i++, offset += 2 )
					ba[ i ] = (byte)System.Convert.ToInt16(
						new String( hex, offset, 2 ), 16 );
			}
			catch( FormatException ex )
			{
				throw SqlEx.get( ERR_GC4014_CALL_SYNTAX, ex );
			}

			return( ba );
		} // hex2bin


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
		**	13-Jul-10 (gordy, ported by thoda04) B124115
		**	    Skip comments in SQL text (treat like white space).
		*/

		private int
			nextToken( bool esc )
		{
			int	    type;
			char    ch;
			bool    scanning = true;

			/*
			** Scan for start of next token skipping white space and comments.
			*/
			while( scanning )
			{
				/*
				** Scanning begins at end of current token.
				*/
				while( token_end < txt_len  &&  
					Char.IsWhiteSpace( text[ token_end ] ) )  token_end++;

				token_beg = token_end;
				if ( token_end >= txt_len )  return( EOF );

				if ((token_end = comment(text, token_beg, txt_len)) == token_beg)
				{
					/*
					** Found start of next token.  Initially,
					** the token is just a single character.
					*/
					token_end = token_beg + 1;
					scanning = false;
				}
			}  // end while( scanning )

			switch( (ch = text[ token_beg ]) )
			{
				case '(' :	type = P_LPAREN;    break;
				case ')' :	type = P_RPAREN;    break;
				case ',' :	type = P_COMMA;	    break;
				case '=' :	type = P_EQUAL;	    break;
				case '+' :	type = P_PLUS;	    break;
				case '-' :	type = P_MINUS;	    break;
				case '.' :	type = P_PERIOD;    break;
				case '}' :	type = P_RBRACE;    break;

				case '@':  // if "@myparm"  or ":1"
				case ':':  //    process as named pararmeter
					if (token_end < txt_len &&
						isIdentAlphaNumOrSpec(text[token_end]))
						goto case '?';
					else goto default;  // standalone "@" or ":" is not a parm

				case '?': type = P_QMARK;
					for (; token_end < txt_len; token_end++)
					{
						if (!isIdentAlphaNumOrSpec(text[token_end]))
							break;
					}
					break;

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
							if ( ! isIdentChar( ch )  &&  ! Char.IsDigit( ch ) )
								break;
						}
					}
					else  if ( Char.IsDigit( ch ) )
					{
						type = NUMBER;

						while( token_end < txt_len  &&  
							Char.IsDigit( text[ token_end ] ) )
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
		{
			int	    type;
			char    ch;

			while( --token_beg >= 0  &&  Char.IsWhiteSpace( text[ token_beg ] ) );

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
							if ( ! isIdentChar( ch )  &&  ! Char.IsDigit( ch ) )
								break;
						}
					}
					else  if ( Char.IsDigit( ch ) )
					{
						type = NUMBER;

						while( token_beg > 0  &&
							Char.IsDigit( text[ token_beg - 1 ] ) )
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
		** Name: comment
		**
		** Description:
		**	Scan a comment.  Returns position just following the 
		**	terminating delimiter.  If starting delimiter is not 
		**	at the position provided or terminating delimiter is 
		**	not found, the provided starting position is returned.
		**
		** Input:
		**	txt	    Text.
		**	beg	    Starting position of comment.
		**	end	    End of text.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Position following comment.
		**
		** History:
		**	13-Jul-10 (gordy, ported by thoda04)
		**	    Created.
		*/

		private int
		comment(char[] txt, int beg, int end)
		{
			int pos = beg;

			/*
			** Check for start-of-comment.
			*/
			if (pos >= end || txt[pos++] != '/') return (beg);
			if (pos >= end || txt[pos++] != '*') return (beg);

			/*
			** Scan looking for end-of-comment.
			** Return end position if found.
			*/
			while (pos < end)
				if (txt[pos++] == '*')
				{
					if (pos >= end) break;
					if (txt[pos] == '/') return (pos + 1);
				}

			/*
			** Failed to find end-of-comment.
			*/
			return (beg);
		}  // end comment


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
		match( char[] txt, int beg, int end, char end_ch )
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

			throw SqlEx.get( ERR_GC4013_UNMATCHED );
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
		keyword( char[] txt, int beg, int end, String[] table )
		{
			int i, ix, length = end - beg;

			for( ix = 0; ix < table.Length; ix++ )
				if ( length == table[ ix ].Length )
				{
					for( i = 0; i < length; i++ )
						if ( table[ ix ][ i ] !=
							ToInvariantUpper( txt[ beg + i ] ) )
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

		private static bool
			isIdentChar( char ch )
		{
			return( Char.IsLetter( ch )  ||  ch == '_'  ||  
				ch == '$'  ||  ch == '@'  ||  ch == '#' );
		} // isIdentChar


		/// <summary>
		/// Return true if character is alphabetic, numeric, $, @, _, or #.
		/// </summary>
		/// <param name="ch"></param>
		/// <returns></returns>
		private static bool
			isIdentAlphaNumOrSpec( char ch )
		{
			return( Char.IsLetterOrDigit( ch )  || isIdentChar( ch ) );
		} // isIdentChar


		/// <summary>
		/// Validate the parameter marker for consistency
		/// against the other parameter markers in the list.
		/// </summary>
		/// <param name="parameterMarker"></param>
		/// <param name="parameterMarkers"></param>
		private void ValidateParameterMarker(
			String            parameterMarker,
			StringCollection  parameterMarkers)
		{
			String parameterMarker0;
			Char   parameterMarker0Char;
			int    parameterMarker0Length;

			// only allow the singleton '?' character
			//for an unnamed parameter marker
			if (parameterMarker.Length == 1 &&
				parameterMarker[0] != '?')
				throw new SqlEx(
					"Invalid initial parameter marker " +
					"character for \"" +
					parameterMarker[0] + "\".");

			if (parameterMarkers == null  ||
				parameterMarkers.Count <= 1)
				return;

			// use the first parameter marker in the list as the touchstone
			parameterMarker0      = parameterMarkers[0];  // 1st marker
			parameterMarker0Char  = parameterMarker0[0];  // 1st char
			parameterMarker0Length= parameterMarker0.Length;

			if (parameterMarker0Char != parameterMarker[0])
				throw new SqlEx(
					"Invalid parameter marker format for \"" +
					parameterMarker + "\".  " +
					"Expecting parameter marker to begin with " +
					"'" + parameterMarker0Char + "' character.");

			if ((parameterMarker0Length  > 1  && // if named, all must be
				 parameterMarker.Length == 1)    ||
				(parameterMarker0Length == 1  && // if unnamed, all must be
				 parameterMarker.Length  > 1))
				throw new SqlEx(
					"Invalid parameter marker format for \"" +
					parameterMarker + "\".  " +
					"If one parameter is named then all " +
					"parameter markers must be named.");
		}  // end ValidateParameterMarker()

	} // class SqlParse

}  // namespace
