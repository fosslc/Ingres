/*
**Copyright (c) 2004 Ingres Corporation
*/

/* 
** Name:
** 
** Description:
**
** History:
**      18-Jan-1999 (fanra01)
**          Renamed scs_get_next_token to ascs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-jul-2001 (toumi01)
**	    #include "ascstok.h" before system headers to avoid conflict
**	    with system definitions of MIN and MAX (i64_aix)
**	25-oct-2001 (somsa01)
**	    Re-worked previous change, as we want the MIN and MAX redefinitions
**	    from "ascstok.h".
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <tr.h>
#include    <er.h>
#include    <cs.h>
#include    <lk.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <usererror.h>

#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <ddb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sc0a.h>

#include    <scserver.h>

#include    <cui.h>

#ifdef i64_aix
#undef MIN
#undef MAX
#endif  /* i64_aix */
#include    "ascstok.h"

/*
** Forward/Extern references 
*/
static DB_STATUS ascs_token( char **stmt_pos, char *stmt_end, char *word );

/*
** Global variables owned by this file
*/

/*
** Secondary keyword array.  Contains the second token and keyword 
** id of a double keyword.  The first token of the double keyword 
** is in the primary keyword array.  The keyword id in the primary 
** keyword array is used to subscript into this array.  Note that
** the primary keyword must be unique to the double keyword; the
** CREATE statements cannot be made into double keywords using the
** representation used here.
*/
static struct _double_list
{
    char	*token;
    i4		id;
} double_list[] =
{
    { "PRECISION",	FLOAT_TYPE },
};

/*
** Subscripts into double_list array to be placed
** in keyword_list[].id to access double_list entries.
*/
#define	DKW_DOUBLE_PRECISION	0

/*
** Primary keyword array.  The token type is used to control
** processing of compound and double keywords.  Compound
** keywords are built by the tokenizer with additional elements
** following the initial character and keyword matching only
** occurs on the first character.  Double keywords consist of
** two tokens which are scanned independently.  The first token
** is in this array and the token id is used as a subscript
** into the double keyword array for the second token.  For
** non-double keywords, the token id identifies the keyword.
** This table is expected to be in ALPHANUMERIC order.
*/
#define	SKW_COMPOUND	0
#define	SKW_SINGLE	1
#define	SKW_DOUBLE	2

static struct _keyword_list
{
    char	*token;
    i4		type;
    i4		id;
} keyword_list[] = 
{
    { "\"",		SKW_COMPOUND,	DOUBLE_QUOTE },
    { "$",              SKW_COMPOUND,	DOLLAR_SIGN },
    { "\'",		SKW_COMPOUND,	SINGLE_QUOTE },
    { "(",              SKW_SINGLE,	L_PAREN },
    { ")",              SKW_SINGLE,	R_PAREN },
    { "*",		SKW_SINGLE,	ASTERISK },
    { "*/",		SKW_SINGLE,	END_COMMENT },
    { "+",		SKW_SINGLE,	PLUS_SIGN },
    { ",",              SKW_SINGLE,	COMMA },
    { "-",		SKW_SINGLE,	MINUS_SIGN },
    { ".",              SKW_COMPOUND,	PERIOD },
    { "/*",		SKW_SINGLE,	BEGIN_COMMENT },
    { "=",              SKW_SINGLE,	EQUAL_SIGN },
    { "?",              SKW_SINGLE,	QUESTION_MARK },
    { "ALL",            SKW_SINGLE,	ALL },
    { "AS",             SKW_SINGLE,	AS },
    { "ASC",            SKW_SINGLE,	ASC },
    { "AUTOCOMMIT",     SKW_SINGLE,	AUTOCOMMIT },
    { "AVG",		SKW_SINGLE,	AVG },
    { "BY",             SKW_SINGLE,	BY },
    { "BYREF",		SKW_SINGLE,	BYREF },
    { "CHAR",           SKW_SINGLE,	CHAR_TYPE },
    { "CHARACTER",	SKW_SINGLE,	CHAR_TYPE },
    { "CHECK",		SKW_SINGLE,	CHECK },
    { "COMMIT",         SKW_SINGLE,	COMMIT_STATEMENT },
    { "COPY",           SKW_SINGLE,	COPY_STATEMENT },
    { "COUNT",		SKW_SINGLE,	COUNT },
    { "CREATE",         SKW_SINGLE,	CREATE_STATEMENT },
    { "CURRENT",        SKW_SINGLE,	CURRENT },
    { "CURRENT_DATE",	SKW_SINGLE,	CURRENT_DATE },
    { "CURRENT_TIMESTAMP", SKW_SINGLE,	CURRENT_TIMESTAMP },
    { "CURSOR",		SKW_SINGLE,	CURSOR },
    { "DATE",           SKW_SINGLE,	DATE_TYPE },
    { "DBA",		SKW_SINGLE,	DBA },
    { "DEC",		SKW_SINGLE,	DECIMAL_TYPE },
    { "DECIMAL",	SKW_SINGLE,	DECIMAL_TYPE },
    { "DEFAULT",	SKW_SINGLE,	DEFAULT },
    { "DEFINE",         SKW_SINGLE,	DEFINE_STATEMENT },
    { "DELETE",         SKW_SINGLE,	DELETE_STATEMENT },
    { "DESC",           SKW_SINGLE,	DSC },
    { "DESCRIBE",       SKW_SINGLE,	DESCRIBE_STATEMENT },
    { "DIRECT",         SKW_SINGLE,	DIRECT_STATEMENT },
    { "DISTINCT",       SKW_SINGLE,	DISTINCT },
    { "DOUBLE",		SKW_DOUBLE,	DKW_DOUBLE_PRECISION },
    { "DROP",           SKW_SINGLE,	DROP_STATEMENT },
    { "EXECUTE",        SKW_SINGLE,	EXECUTE_STATEMENT },
    { "FLOAT",		SKW_SINGLE,	FLOAT_TYPE },
    { "FLOAT4",		SKW_SINGLE,	REAL_TYPE },
    { "FLOAT8",		SKW_SINGLE,	FLOAT_TYPE },
    { "FOR",            SKW_SINGLE,	FOR },
    { "FOREIGN",	SKW_SINGLE,	FOREIGN },
    { "FROM",           SKW_SINGLE,	FROM },
    { "GRANT",          SKW_SINGLE,	GRANT_STATEMENT },
    { "GROUP",		SKW_SINGLE,	GROUP },
    { "HAVING",		SKW_SINGLE,	HAVING },
    { "IMMEDIATE",      SKW_SINGLE,	IMMEDIATE },
    { "IMPORT",		SKW_SINGLE,	IMPORT },
    { "INDEX",          SKW_SINGLE,	INDEX },
    { "INSERT",         SKW_SINGLE,	INSERT_STATEMENT },
    { "INT",		SKW_SINGLE,	INTEGER_TYPE },
    { "INTEGER",	SKW_SINGLE,	INTEGER_TYPE },
    { "INTEGER2",	SKW_SINGLE,	SMALLINT_TYPE },
    { "INTEGER4",	SKW_SINGLE,	INTEGER_TYPE },
    { "INTO",           SKW_SINGLE,	INTO },
    { "IS",		SKW_SINGLE,	IS },
    { "JOINOP",		SKW_SINGLE,	JOINOP },
    { "JOURNALING",	SKW_SINGLE,	JOURNALING },
    { "LOCKMODE",	SKW_SINGLE,	LOCKMODE },
    { "MAX",		SKW_SINGLE,	MAX },
    { "MIN",		SKW_SINGLE,	MIN },
    { "NOJOURNALING",	SKW_SINGLE,	NOJOURNALING },
    { "NOPRINTQRY",	SKW_SINGLE,	NOPRINTQRY },
    { "NOQEP",		SKW_SINGLE,	NOQEP },
    { "NOT",            SKW_SINGLE,	NOT },
    { "NOTRACE",        SKW_SINGLE,	NOTRACE },
    { "NULL",           SKW_SINGLE,	NULL_SYMBOL },
    { "NUMERIC",	SKW_SINGLE,	DECIMAL_TYPE },
    { "ODBADDMMDATA",   SKW_SINGLE,     ODBADDMMDATA_STATEMENT },
    { "ODBGETMMDATA",   SKW_SINGLE,     ODBGETMMDATA_STATEMENT },
    { "ODBGETOBJSVERS", SKW_SINGLE,     ODBGETOBJSVERS_STATEMENT },
    { "ODBGETVALUES",   SKW_SINGLE,     ODBGETVALUES_STATEMENT },
    { "ODBREPMMDATA",   SKW_SINGLE,     ODBREPMMDATA_STATEMENT },
    { "ODBSETVALUES",   SKW_SINGLE,     ODBSETVALUES_STATEMENT },
    { "OFF",		SKW_SINGLE,	OFF },
    { "ON",             SKW_SINGLE,	ON },
    { "OPEN",           SKW_SINGLE,	OPEN_STATEMENT },
    { "OPTION",		SKW_SINGLE,	OPTION },
    { "ORDER",          SKW_SINGLE,	ORDER  },
    { "PREPARE",        SKW_SINGLE,	PREPARE_STATEMENT },
    { "PRIMARY",	SKW_SINGLE,	KW_PRIMARY },
    { "PRINTQRY",	SKW_SINGLE,	PRINTQRY },
    { "PRIVILEGES",	SKW_SINGLE,	PRIVILEGES },
    { "PROCEDURE",	SKW_SINGLE,	PROCEDURE },
    { "QEP",		SKW_SINGLE,	QEP },
    { "READONLY",	SKW_SINGLE,	READ_ONLY },
    { "REAL",		SKW_SINGLE,	REAL_TYPE },
    { "REGISTER",	SKW_SINGLE,	REGISTER_STATEMENT },
    { "REMOVE",		SKW_SINGLE,	REMOVE_STATEMENT },
    { "RESULT_STRUCTURE",  SKW_SINGLE,	RESULT_STRUCTURE },
    { "REVOKE",		SKW_SINGLE,	REVOKE_STATEMENT },
    { "ROLLBACK",       SKW_SINGLE,	ROLLBACK_STATEMENT },
    { "SAVEPOINT",	SKW_SINGLE,	SAVEPOINT_STATEMENT },
    { "SCHEMA",		SKW_SINGLE,	SCHEMA },
    { "SELECT",         SKW_SINGLE,	SELECT_STATEMENT },
    { "SET",            SKW_SINGLE,	SET_STATEMENT },
    { "SMALLINT",	SKW_SINGLE,	SMALLINT_TYPE },
    { "SUM",		SKW_SINGLE,	SUM },
    { "TABLE",          SKW_SINGLE,	TABLE },
    { "TO",		SKW_SINGLE,	TO },
    { "TRACE",          SKW_SINGLE,	TRACE },
    { "UNION",          SKW_SINGLE,	UNION },
    { "UNIQUE",         SKW_SINGLE,	UNIQUE },
    { "UPDATE",         SKW_SINGLE,	UPDATE_STATEMENT },
    { "USER",		SKW_SINGLE,	USER },
    { "USING",          SKW_SINGLE,	USING },
    { "VALUES",		SKW_SINGLE,	VALUES },
    { "VARCHAR",        SKW_SINGLE,	VARCHAR_TYPE },
    { "VIEW",           SKW_SINGLE,	VIEW },
    { "WHERE",          SKW_SINGLE,	WHERE },
    { "WITH",           SKW_SINGLE,	WITH },
    { "WORK",		SKW_SINGLE,	WORK },
};


/*
**  Name: ascs_token
**
**  Description:
**
**	This module, given a string of characters will return the next
**	legal Ingres token.  
**
**	The buffer for returning the token must be at least 
**	SCS_MAX_TOKEN_LENGTH + 1
**	bytes in length.  Some tokens are not returned in their entirety.
**	Significant puctuation returns just the character.  Quoted strings
**	return just the starting quote.  Names and Keywords must fit into
**	SCS_MAX_TOKEN_LENGTH characters otherwise an error is returned.  
**
**	There are GCA constructs which begin with '$' and '~'.  If the
**	GCA construct is not successfully scanned then only the '$' or
**	'~' will be returned as a puctuation character, otherwise the
**	construct is returned.  In the case of the '$' construct only
**	the '$' and following numeric digits are returned.  The rest
**	of the construct is only scanned.
**
**	All other tokens return at most SCS_MAX_TOKEN_LENGTH characters even 
**	if more is scanned.  If no token is found before end of string then a 0 
**	length buffer is returned.
**
**  Parameters:
**
**     Input:
**
**     Name             Type    Description
**     ----             ----    -----------
**    **stmt_pos        char    running pointer to position in
**                               SQL request
**     *stmt_end        char    pointer to end of request stmt
**
**     Output:
**
**     Name		Type    Description
**     ----		----    -----------
**     word		char    array containing next valid Ingres name
**
**  Returns:
**
**	STATUS		OK or error code.
**
**  History:
*/

DB_STATUS
ascs_token( char **stmt_pos, char *stmt_end, char *word )
{
    DB_STATUS	status = OK;
    char	*ptr = *stmt_pos;
    i4		word_len = 0;
    char	quote, *start;
    bool	decimal = FALSE;

    /* Skip leading white space */
    while( ptr < stmt_end  &&  *ptr != EOS  &&  CMwhite( ptr ) )  
	CMnext( ptr );

    if ( ptr < stmt_end )
	switch( *ptr )
	{
	    case EOS:
		ptr = stmt_end;
		break;

	    case '(':
	    case ')':
	    case '+':
	    case ',':
	    case '-':
	    case '=':
	    case '?':
		CMcpyinc( ptr, word );
		break;

	    case '/':
		CMcpyinc( ptr, word );
		if ( ptr < stmt_end  &&  *ptr == '*' )  
		    CMcpyinc( ptr, word );
		break;
	    
	    case '*':
		CMcpyinc( ptr, word );
		if ( ptr < stmt_end  &&  *ptr == '/' )  
		    CMcpyinc( ptr, word );
		break;

	    case '.':
		CMcpyinc( ptr, word );
		word_len++;

		/*
		** If followed by numeric then this is the
		** start of a numeric token, otherwise, we
		** are done.
		*/
		if ( ptr >= stmt_end  ||  ! CMdigit( ptr ) )  break;

		/*
		** Fall through to process rest of floating point literal.
		*/
		decimal = TRUE;

	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		CMcpyinc( ptr, word );
		word_len++;

		/*
		** Scan in the numeric literal, allowing an embeded
		** decimal point (it may have been a leading decimal
		** point which was processed above).
		*/
		while( ptr < stmt_end  &&  ( CMdigit( ptr ) ||  *ptr == '.' ) )
		    if ( ++word_len > SCS_MAX_TOKEN_LENGTH )
		    {
			/* 
			** FIXME
			status = E_GX0203_TOKEN_TOO_LONG;
			*/
			status = E_DB_ERROR;
			break;
		    }
		    else
		    {
			/*
			** Only allow one decimal point.  Stop
			** scanning on subsequent '.'.
			*/
			if ( ! CMdigit( ptr ) )
			    if ( ! decimal )
				decimal = TRUE;
			    else
				break;

			CMcpyinc( ptr, word );
		    }

		/*
		** Check for a possible exponent.
		*/
		if ( ptr < stmt_end  &&  ( *ptr == 'e'  ||  *ptr == 'E' ) )
		    if ( ++word_len > SCS_MAX_TOKEN_LENGTH )
		    {
			/* 
			** FIXME
			status = E_GX0203_TOKEN_TOO_LONG;
			*/
			status = E_DB_ERROR;
		    }
		    else
		    {
			CMcpyinc( ptr, word );

			/*
			** Check for sign on exponent.
			*/
			if ( ptr < stmt_end  &&  (*ptr == '+' || *ptr == '-' ))
			    if ( ++word_len > SCS_MAX_TOKEN_LENGTH )
				/* 
				** FIXME
				status = E_GX0203_TOKEN_TOO_LONG;
				*/
				status = E_DB_ERROR;
			    else
				CMcpyinc( ptr, word );

			/*
			** Read in the exponent.
			*/
			while( ptr < stmt_end  &&  CMdigit( ptr ) )
			    if ( ++word_len <= SCS_MAX_TOKEN_LENGTH )
				CMcpyinc( ptr, word );
			    else
			    {
				/* 
				** FIXME
				status = E_GX0203_TOKEN_TOO_LONG;
				*/
				status = E_DB_ERROR;
				break;
			    }
		    }
		break;


	    case '\'': 
	    case '\"':
		quote = *ptr;
		CMcpyinc( ptr, word );
                ++word_len;

		for(;;)
		{
		    while( ptr < stmt_end  &&  *ptr != quote )
			if ( ++word_len <= SCS_MAX_TOKEN_LENGTH )
			    CMcpyinc( ptr, word );
			else
			    CMnext( ptr );

		    if ( ptr >= stmt_end )
		    {
			/* 
			** FIXME
			status = E_GX0202_INVALID_STRING;
			*/
			status = E_DB_ERROR;
			break;
		    }
		    else
		    {
			if ( ++word_len <= SCS_MAX_TOKEN_LENGTH )
			    CMcpyinc( ptr, word );
			else
			    CMnext( ptr );

			if ( ptr >= stmt_end  ||  *ptr != quote )
			    break;

			if ( ++word_len <= SCS_MAX_TOKEN_LENGTH )
			    CMcpyinc( ptr, word );
			else
			    CMnext( ptr );
		    }
		}
		break;

	    case '$':
		CMcpyinc( ptr, word );
                ++word_len;

		if ( ptr >= stmt_end )
		    break;
		else  if ( ! CMdigit( ptr ) )
		{
		    /*
		    ** We permit $ as the first character
		    ** of identifiers to allow for the $ingres
		    ** user ID.  No other SQL construct permits
		    ** $ followed immediatly by an identifier.
		    ** Get the rest of the identifier.
		    */
		    while( ptr < stmt_end  &&  CMnmchar( ptr ) )
			if( ++word_len <= SCS_MAX_TOKEN_LENGTH )
			    CMcpyinc( ptr, word );
			else
			{
			    /* 
			    ** FIXME
			    status = E_GX0203_TOKEN_TOO_LONG;
			    */
			    status = E_DB_ERROR;
			    break;
			}
		    break;
		}
		
		/*
		** Scan the $<no> = ~V<no> construct.
		*/

		/* Assume invalid until successful scan completion. */
		status = FAIL;
		*stmt_pos = ptr;
		start = word;

		/* Return the numeric identifier */
		while( ptr < stmt_end  &&  CMdigit( ptr ) )
		    if ( ++word_len <= SCS_MAX_TOKEN_LENGTH )
			CMcpyinc( ptr, word );
                    else
			break;

		/* Scan the remainder of the construct */
		while( ptr < stmt_end  &&  CMwhite( ptr ) )
		    CMnext( ptr );

		if ( ptr < stmt_end  &&  *ptr == '=' )
		{
		    CMnext( ptr );

		    while( ptr < stmt_end  &&  CMwhite( ptr ) )
			CMnext( ptr );

		    if ( ptr < stmt_end &&  *ptr == '~' )
		    {
			CMnext( ptr );

			if ( ptr < stmt_end &&  *ptr == 'V' )
			{
			    CMnext( ptr );
			    status = OK;

			    while( ptr < stmt_end  &&  CMdigit( ptr ) )
				CMnext( ptr );
			}
		    }
		}

		if ( status != OK )
		{
		    /*
		    ** Did not find a valid construct.  Return
		    ** just the $ as a punctuation symbol.
		    */
		    status = OK;
		    ptr = *stmt_pos;
		    word = start;
		}
		break;

	    case '~':
		CMcpyinc( ptr, word );
                word_len++;

		if ( ptr < stmt_end  &&  ( *ptr == 'V'  ||  *ptr == 'Q' ) )
		{
		    CMcpyinc( ptr, word );
                    word_len++;

		    while( ptr < stmt_end  &&  CMdigit( ptr ) )
			if ( ++word_len <= SCS_MAX_TOKEN_LENGTH )
			    CMcpyinc( ptr, word );
			else
			    break;
		}
		break;

	    default:
		if ( CMnmstart( ptr ) )
		{
		    CMcpyinc( ptr, word );
		    word_len++;

		    while( ptr < stmt_end  &&  CMnmchar( ptr ) )
			if( ++word_len <= SCS_MAX_TOKEN_LENGTH )
			    CMcpyinc( ptr, word );
			else
			{
			    /* 
			    ** FIXME
			    status = E_GX0203_TOKEN_TOO_LONG;
			    */
			    status = E_DB_ERROR;
			    break;
			}
		}
		else
		{
		    bool done = FALSE;

		    while( ! done  &&  ptr < stmt_end )
		    {
			switch( *ptr )
			{
			    case EOS:
			    case '(':
			    case ')':
			    case '+':
			    case ',':
			    case '-':
			    case '.':
			    case '?':
			    case '=':
			    case '/':
			    case '*':
			    case '\'': 
			    case '\"':
			    case '$':
			    case '~':
				done = TRUE;
				break;

			    default:
				if ( CMnmstart( ptr )  ||  CMwhite( ptr ) )
				    done = TRUE;
				else  if ( ++word_len <= SCS_MAX_TOKEN_LENGTH )
				    CMcpyinc( ptr, word );
				else
				    CMnext( ptr );
				break;
			}
		    }
		}
		break;
	}

    *stmt_pos = ptr;
    *word = EOS;

    return( status );
}


/*
**  Name: ascs_get_next_token
**
**  Description:
**     This function is passed query text and should scan the text
**     looking for a token that matches one in its token table.
**     After it finds a match, it will return an integer that 
**     identifies the token.
**
**  Parameters:
**
**     Input:
**
**     Name              Type    Description
**     ----              ----    -----------
**     *stmt_pos         char    String containing SQL stmt
**     *stmt_end         char    Pointer to end of request
**
**     Output:
**
**     Name              Type    Description
**     ----              ----    -----------
**     token_type        u_i4   Enumeration value for token:
**                               Name(as in a cursor or statement name),
**                               Keyword(in list of SQL reserved words),
**                               Tilde(to indicate DB_DATA_VALUE marker)
**                               Punctuation(right now, ',', '(', ')')
**     keyword_type      u_i4   Value for keyword in reserved word array
**     *stmt_pos         char    Running position in SQL request
**     *word             char    Next legal Ingres wrod
**
**  Returns:
**
**	STATUS		OK
**			FAIL	End of input statement, no token.
**			E_GXnnn	A token syntax error.
**
**  History:
**	31-Jul-96 (reijo01)
**	    Adapted for the Jasmine server from the gateway code.
*/

DB_STATUS
ascs_get_next_token( char **stmt_pos, char *stmt_end, i4  *token_type, 
		i4 *keyword_type, char *word )
{
    struct _keyword_list	*keyword;
    i4                  	entry, lo, hi, cmp;
    char			first_char, *ptr;
    char			word1[ SCS_MAX_TOKEN_LENGTH + 1 ];
    char                	*local_pos = *stmt_pos;
    DB_STATUS              	status = OK;

    *token_type = SYMBOL;
    *keyword_type = NOT_A_KEYWORD;

    /*
    ** Get next legal Ingres token
    */
    status = ascs_token( &local_pos, stmt_end, word );

    if ( status != OK )  return( status );

    *stmt_pos = local_pos;

    if ( *word == EOS )
    {
	/*
	** No token indicates that the end of the string
	** has been reached.
	*/
	return( FAIL );
    }

    /*
    ** Do binary search of token list
    */
    lo = 0;
    hi = ( sizeof(keyword_list) / sizeof(struct _keyword_list) ) - 1;
    CMtoupper( word, &first_char );

    while( lo <= hi )
    {
	entry = ( lo + hi ) >> 1;
	keyword = &keyword_list[ entry ];
	cmp = first_char - keyword->token[ 0 ];

	/* If first characters match then do full comparison */
	if ( ! cmp  &&  keyword->type != SKW_COMPOUND )
	    cmp = STbcompare( word, 0, keyword->token, 0, TRUE );

	if ( ! cmp )
	    break;
	else if ( cmp < 0 )
	    hi = entry - 1;
	else
	    lo = entry + 1;
    }

    if ( lo > hi )
    {
	/*
	** Token was not in the keyword/puncuation table.
	** It can be a numeric literal or Ingres name,
	** otherwise it will be left a generic symbol.
	*/
	if ( CMdigit( word ) )  
	{
	    *token_type = NUMERIC_LITERAL;
	    *keyword_type = INTEGER_TYPE;

	    /*
	    ** Check to see if this is a floating point
	    ** literal (any non-digit characters present).
	    */
	    for( ptr = word; *ptr != EOS; CMnext( ptr ) )
		if ( ! CMdigit( ptr ) )
		{
		    *keyword_type = FLOAT_TYPE;
		    break;
		}
	}
	else  if ( CMnmstart( word ) )  
	    *token_type = NAME;
    }
    else
    {
	switch( keyword->type )
	{
	    case SKW_COMPOUND :
		/*
		** Check for special composite construct.
		*/
		switch( keyword->id )
		{
		    case PERIOD:
			/*
			** A floating point literal may start
			** with a leading decimal point.
			*/
			if ( word[ 1 ] == EOS )
			{
			    *token_type = PUNCTUATION;
			    *keyword_type = keyword->id;
			}
			else
			{
			    *token_type = NUMERIC_LITERAL;
			    *keyword_type = FLOAT_TYPE;
			}
			break;

		    case DOUBLE_QUOTE:
		    case SINGLE_QUOTE:
			*token_type = QUOTED_STRING;
			*keyword_type = keyword->id;
			break;

		    case DOLLAR_SIGN:
			/*
			** A '$' may start an identifier or a
			** GCA variable replacement token.
			*/
			if ( word[ 1 ] == EOS )
			{
			    *token_type = PUNCTUATION;
			    *keyword_type = keyword->id;
			}
			else  if ( ! CMdigit( word + 1 ) )
			{
			    /*
			    ** Identifiers permitted to
			    ** start with $ (such as $ingres).
			    */
			    *token_type = NAME;
			    *keyword_type = NOT_A_KEYWORD;
			}
			else
			{
			    *token_type = KEYWORD;
			    *keyword_type = DOLLAR_VAR;
			}
			break;

		    case TILDE:
			/*
			** A '~' may start a GCA variable replacement
			** token.
			*/
			if ( word[ 1 ] == EOS )
			{
			    *token_type = PUNCTUATION;
			    *keyword_type = keyword->id;
			}
			else
			{
			    *token_type = KEYWORD;
			    *keyword_type = ( word[ 1 ] == 'V' ) ? TILDE_V 
							         : TILDE_Q;
			}
			break;
	
		    default:
			/*
			** FIXME
			GWfatal( FAIL, "GWget_next_token: bad compound id" );
			*/
			break;
		}
		break;

	    case SKW_SINGLE :
		*keyword_type = keyword->id;

		/*
		** Determine if keyword is PUNCTUATION
		*/
		switch( *keyword_type )
		{
		    case L_PAREN:
		    case R_PAREN:
		    case ASTERISK:
		    case PLUS_SIGN:
		    case COMMA:
		    case MINUS_SIGN:
		    case EQUAL_SIGN:
		    case QUESTION_MARK:
		    case BEGIN_COMMENT:
		    case END_COMMENT:
			*token_type = PUNCTUATION;
			break;

		    default:
			*token_type = KEYWORD;
			break;
		}
		break;

	    case SKW_DOUBLE :
		/*
		** Get next legal Ingres token
		*/
		status = ascs_token( &local_pos, stmt_end, word1 );

		if ( 
		     status != OK  ||  word1[ 0 ] == EOS  ||
		     STbcompare( word1, 0, 
				 double_list[ keyword->id ].token, 0, TRUE ) 
		   )
		{
		    /*
		    ** If double keyword matching fails then
		    ** the first token is treated as an identifier.
		    */
		    status = OK;
		    *token_type = NAME;
		    *keyword_type = NOT_A_KEYWORD;
		}
		else
		{
		    *stmt_pos = local_pos;
		    STpolycat( 3, word, " ", word1, word );
		    *token_type = KEYWORD;
		    *keyword_type = double_list[ keyword->id ].id;
		}
		break;

	    default :
		/*
		** FIXME
		GWfatal( FAIL, "GWget_next_token: bad keyword type" );
		*/
		break;
	}
    }

    return( status );
}
