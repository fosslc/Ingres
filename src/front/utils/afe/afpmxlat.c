/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>
# include	<nm.h>
# include	<cm.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	afpmxlat.c -	AFE Pattern Match Translation Module.
**
** Description:
**	Contains a routine used to translate a string possibly containing
**	pattern match characters for one DML into another containing pattern
**	match characters for the other DML.  Defines:
**
**	IIAFcvWildCard()	convert pattern match characters.
**
** History:
**	Revision 8.0  89/08/02  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#define QUEL_PM_CHAR(str) (*(str) == '*' || *(str) == '?')

#define BRACKET_PM_CHAR(str) (*(str) == '[' || *(str) == ']')

#define ALL_QUEL_PM_CHAR(str) (QUEL_PM_CHAR(str) || BRACKET_PM_CHAR(str))

#define SQL_PM_CHAR(str) (*(str) == '%' || *(str) == '_')

static char	quel_sql();
static char	sql_quel();

static DB_LANG	_PatternDML = DB_NOLANG;	/* Input DML */

/*{
** Name:	IIAFcvWildCard() -	Converts Pattern-Match Characters in
**						a String for a DML.
** Description:
**	Converts pattern-match (wildcard) characters in a string for one DML
**	into pattern-match characters in the other DML.  The input DML is set by
**	users using the II_PATTERM_MATCH logical. (Alternatively, the caller can
**	force conversion from SQL by passing in the AFE_PM_SQL_CHECK command.)
**	For SQL output, if escapes are not significant they will be removed.
**
**	This walks the input string translating characters, but not in place
**	because the resultant string may be larger than the input.  Afterwards,
**	the resultant string is copied back over the input string.
**
** Inputs:
**	string	{DB_DATA_VALUE *}  Value to convert.
**	qry_lang{DB_LANG}  DML to which to convert pattern-match characters.
**	pm_cmd	{nat}  The type of pattern-match checks to perform.
**				AFE_PM_NO_CHECK		no translation
**				AFE_PM_CHECK		translate
**				AFE_PM_GTW_CHECK	translate for Gateway
**				AFE_PM_SQL_CHECK	translate from SQL

**
** Outputs:
**	string	{DB_DATA_VALUE *}  Converted string value.
**	sqlpat	{nat *}	set to one of the following values:
**				AFE_PM_NOT_FOUND
**				AFE_PM_FOUND
**				AFE_PM_USE_ESC
**				AFE_PM_QUEL_STRING
**
** History:
**	27-mar-87 (grant)	implemented.
**	19-nov-87 (kenl)
**		Added sqlpat parameter (i4 *).  sqlpat will be set to TRUE
**		if a QUEL pattern matching character was changed to its
**		SQL equivalent.
**	06-aug-89 (jhw)  Rewrote as AFE routine.  Now gets string as data value.
**		Also, added force SQL translation command, AFE_PM_SQL_CHECK.
*/

DB_STATUS
IIAFcvWildCard ( string, qry_lang, pm_cmd, sqlpat )
DB_DATA_VALUE	*string;
i4		qry_lang;
i4		pm_cmd;
i4		*sqlpat;
{
    u_char	*input;
    u_char	*stripped;
    u_char	*unstripped;
    u_char	*unstrippedend;
    i4	ilen;

    u_char	buffer[DB_MAXSTRING+1];

    *sqlpat = AFE_PM_NOT_FOUND;

    if ( pm_cmd == AFE_PM_NO_CHECK || AFE_ISNULL(string) )
    { /* do nothing */
	return E_DB_OK;
    }

    if ( _PatternDML == DB_NOLANG
		&& ( qry_lang != DB_QUEL || pm_cmd != AFE_PM_SQL_CHECK ) )
    {
	char	*opt;

	NMgtAt(ERx("II_PATTERN_MATCH"), &opt);
	if ( opt != NULL && *opt != EOS )
	{
	    _PatternDML = IIAFstrDml(FEadfcb(), opt);
	}
	if ( _PatternDML == DB_NOLANG )
	     _PatternDML = DB_QUEL;
    }

    switch ( AFE_DATATYPE(string) )
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	input = (u_char *)string->db_data;
	ilen = string->db_length;
	if ( AFE_NULLABLE(string->db_datatype) )
		--ilen;
	break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
	input = ((DB_TEXT_STRING *)string->db_data)->db_t_text;
	ilen = ((DB_TEXT_STRING *)string->db_data)->db_t_count;
	break;

      default:
	return E_DB_OK;
    }

    unstrippedend = (unstripped = input) + ilen;

    stripped = buffer;

    if ( qry_lang == DB_QUEL )
    { /* output QUEL */
	*sqlpat = AFE_PM_QUEL_STRING;
	if ( _PatternDML != DB_SQL && pm_cmd != AFE_PM_SQL_CHECK )
	    return E_DB_OK;
	else
	{ /* ... from SQL */
	    while ( unstripped < unstrippedend &&
		stripped < buffer + DB_MAXSTRING )
	    {
		if ( *unstripped == '\\' )
		{
		    ++unstripped;
		    if ( *unstripped == '\\' || QUEL_PM_CHAR(unstripped) )
		    { /* escape '\' and QUEL pattern match characters */
			*stripped++ = '\\';
		    }
		    /* Note:  SQL escaped brackets, which are pattern match
		    ** characters, translate to unescaped brackets, which are
		    ** QUEL pattern match characters, as they should.
		    */
		    CMcpyinc(unstripped, stripped);
		}
		else
		{ /* not an escape, translate pattern matching chars. */
		    if ( ALL_QUEL_PM_CHAR(unstripped) )
		    { /* escape QUEL pattern match characters */
			*stripped++ = '\\';
			*stripped++ = *unstripped++;
		    }
		    else if ( SQL_PM_CHAR(unstripped) )
		    { /* translate SQL to QUEL */
			*stripped++ = sql_quel(*unstripped++);
		    }
		    else
		    {
			CMcpyinc(unstripped, stripped);
		    }
		}   /* end of 'not an escape character, translate' */
	    }	/* end of while loop */
	}   /* end of SQL -> QUEL translation */
    }
    else
    { /* output SQL */
	bool	esc_found = FALSE;
	bool	esc_esc_used = FALSE;
	bool	esc_pat_used = FALSE;
	bool	like_used = FALSE;

	while ( unstripped < unstrippedend && stripped < buffer + DB_MAXSTRING )
	{
	    if ( *unstripped == '\\' )
	    {
		esc_found = TRUE;
		/*
		**  Assume, initially, that pattern matching characters
		**  will follow.  We must escape the '%', '_' and '\'.
		*/
		if ( *++unstripped == '\\' )
		{
		    *stripped++ = '\\';
		    esc_esc_used = TRUE;
		}
		else if ( SQL_PM_CHAR(unstripped) )
		{
		    *stripped++ = '\\';
		    esc_pat_used = TRUE;
    		}
		CMcpyinc(unstripped, stripped);
	    }
	    else
	    { /* no escape character found */
		if ( BRACKET_PM_CHAR(unstripped) && pm_cmd != AFE_PM_GTW_CHECK )
		{ /* bracket pattern matching found, escape them */
		    if ( _PatternDML == DB_QUEL )
		    {
			*stripped++ = '\\';
			esc_pat_used = TRUE;
			like_used = TRUE;
		    }
		    CMcpyinc(unstripped, stripped);
		}
		else if ( SQL_PM_CHAR(unstripped) )
		{
		    if ( _PatternDML == DB_QUEL )
		    { /* if PM chars are QUEL, user must want literal %, _ */
			*stripped++ = '\\';
			esc_pat_used = TRUE;
		    }
		    else
		    { /* SQL are the PM chars, just copy it */
			like_used = TRUE;
    		    }
		    CMcpyinc(unstripped, stripped);
		}
		else if ( _PatternDML == DB_QUEL && QUEL_PM_CHAR(unstripped) )
		{ /* PM chars are QUEL, translate them to SQL */
		    *stripped++ = quel_sql(*unstripped++);
		    like_used = TRUE;
		}
		else
		{ /* PM chars are SQL, non-PM char found, just copy it */
		    CMcpyinc(unstripped, stripped);
		}
	    }	/* end of 'no escape found' */
	}   /* end of while loop */

	if ( ( ( esc_pat_used || esc_esc_used || esc_found ) && !like_used ) ||
		( like_used && esc_esc_used && !esc_pat_used ) )
	{ /* translate again without escapes */
	    if (like_used)
		*sqlpat = AFE_PM_FOUND;

	    unstripped = input;
	    stripped = buffer;

	    while ( unstripped < unstrippedend
			&& stripped < buffer + DB_MAXSTRING )
	    {
		if ( *unstripped == '\\' )
		{
		    ++unstripped;
		    CMcpyinc(unstripped, stripped);
		}
		else
		{
		    if ( _PatternDML == DB_QUEL && QUEL_PM_CHAR(unstripped) )
		    {
			*stripped++ = quel_sql(*unstripped++);
		    }
		    else
		    {
			CMcpyinc(unstripped, stripped);
		    }
		}
	    }
	}
	else if (esc_esc_used || esc_pat_used)
	{
	    *sqlpat = AFE_PM_USE_ESC;
	}
	else if (like_used)
	{
	    *sqlpat = AFE_PM_FOUND;
	}
    }	/* end of 'emitting SQL' */

    switch ( AFE_DATATYPE(string) )
    {
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
	MEcopy((PTR)buffer, (u_i2)min(ilen, (stripped - buffer)), (PTR)input);
	if ( (stripped - buffer) < ilen )
	    MEfill( (u_i2)(ilen - (stripped - buffer)), ' ',
			(PTR)(input + (stripped - buffer))
	    );
	break;
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_LTXT_TYPE:
	ilen = string->db_length;
	if ( AFE_NULLABLE(string->db_datatype) )
	    --ilen;
	MEcopy((PTR)buffer, (u_i2)min(ilen, (stripped - buffer)), (PTR)input);
	((DB_TEXT_STRING *)string->db_data)->db_t_count =
						min(ilen, (stripped - buffer));
	break;
    }
    return E_DB_OK;
}

/*
** Name:	quel_sql() -	Map QUEL Pattern Match Char to SQL.
**
** Description:
**	Returns the SQL pattern-match character for the given QUEL pattern-match
**	character; otherwise it just returns the given character.
**
** Inputs:
**	character	{char}  Wildcard character from which to get
**					the pattern-match control character.
**
** Returns:
**	Pattern-match control character corresponding to wildcard character;
**	if not a wildcard character, the character itself
**		is returned.
**
** History:
**	27-mar-87 (grant)	implemented.
**	19-nov-87 (kenl)	added parameter sqlpat
*/

static char
quel_sql ( character )
char	character;
{
	switch (character)
	{
	case '*':
	    return '%';
	    break;

	case '?':
	    return '_';
	    break;

	default:
	    return character;
	    break;
	}
}

static char
sql_quel ( character )
char	character;
{
	switch (character)
	{
	case '%':
	    return '*';
	    break;

	case '_':
	    return '?';
	    break;

	default:
	    return character;
	    break;
	}
}
