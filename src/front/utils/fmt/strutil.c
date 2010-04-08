/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<nm.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<cm.h>
# include	<st.h>
# include	<fedml.h>

/**
** Name:	strutil.c	This file contains the utility routines
**				for formatting strings.
**
** Description:
**	This file defines:
**
**	f_strlen	Returns pointer to the beginning of a string and its
**			length given its DB_DATA_VALUE.
**
**	f_lentrmwhite	Returns the length of a string if its trailing blanks
**			were trimmed.
**
**	f_strT		Converts string to one containing \c or \ddd for
**			control characters.
**
**	f_Tstr		Converts string containing \c or \ddd for control
**			characters into one containing control characters.
**
**	f_strCformat	Converts string to one containing period for control
**			characters.
**
**	f_stcopy	Copies a null-terminated string to a buffer without
**			null-termination.
**
**	f_revrsbuf	Reverse the first 'width' bytes of the contents of
**			a buffer.  Used for right-to-left fields.
**
**	f_wildcard	Converts QUEL wildcard characters in a string
**			to SQL pattern-match characters and
**			strips backslash characters used for escaping.
**
**	f_patternmatch	Given a character, returns the SQL pattern-match
**			character if it is a wildcard character; otherwise,
**			returns the original character.
**
** History:
**	19-jan-87 (grant)	implemented.
**	27-mar-87 (bab)		added f_revrsbuf routine.
**	24-jun-87 (danielt)	added dummy (0) argument in calls to
**		f_patternmatch, which expects a qry_lang parameter (but
**		doesn't do anything with it).
**	19-nov-87 (kenl)
**		Put back qry_lang parameter in call to f_patternmatch.
**		Added parameter (sqlpat) which returns TRUE to call if a
**		QUEL pattern matching character was changed to an SQL pattern
**		matching character.  (NOTE: qry_lang is passed around but
**		is not used by f_patternmatch.  Until ALL pattern matching
**		issues are resolved, it should continue to be passed around.
**		We may need it!)
**	28-mar-88 (bruceb)
**		Check if datatype is nullable in f_strlen.  For certain
**		char types, the dbv's length specification includes an
**		extra byte for the null indicator; this is not part of the
**		string's length.
**	28-sep-88 (kenl)
**		Changed UI_DML_QUEL to DB_QUEL and changed UI_DML_SQL to
**		DB_QUEL.  This file CANNOT #include ui.h, therefore the
**		appropriate constants from DBMS.H will be used.  Routines
**		that call f_wildcard are now required to pass in either
**		DB_QUEL or DB_SQL.
**	29-sep-88 (kenl)
**		Changed f_wildcard (and the routines that call it) so that
**		it is now passed in a flag indicating whether to perform
**		pattern matching and returns in a different variable the
**		results of the pattern matching.
**       9-jan-90 (stevet) fix for jupbug 9466
**              Fixed kanji blank problem : preserve double-byte blank by 
**		not setting ' ' for double byte white space character in 
**		f_strCformat routine.
**       4-mar-93 (smc)
**              Cast parameter of STcopy to match prototype.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Jan-2009 (hweho01)
**	    In f_lentrmwhite() routine, only process the string if the  
**          length is not zero.   
**/

/* # define's */
/* extern's */

FUNC_EXTERN	char	quel_patternmatch();
FUNC_EXTERN	char	sql_patternmatch();

/* static's */
static	i4	pat_lang = 0;

/*{
** Name:	f_strlen	Returns pointer to the beginning of a string
**				and its length given its DB_DATA_VALUE.
**
** Description:
**	This routine returns a pointer to the beginning of a string and its
**	length given a DB_DATA_VALUE.
**
** Inputs:
**	value		A string DB_DATA_VALUE.
**
** Outputs:
**	string		Will point to the beginning of the value's string.
**
**	length		Will point to the length of the value's string.
**
**	Returns:
**		None.
**
** History:
**	19-jan-87 (grant)	implemented.
*/

f_strlen(value, string, length)
DB_DATA_VALUE	*value;
u_char		**string;
i4		*length;
{
    switch (abs(value->db_datatype))
    {
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
	*string = (u_char *)(value->db_data);
	*length = value->db_length;
	if (AFE_NULLABLE_MACRO(value->db_datatype))
		(*length)--;
	break;

    case DB_TXT_TYPE:
    case DB_VCH_TYPE:
    case DB_LTXT_TYPE:
	*string = ((DB_TEXT_STRING *)(value->db_data))->db_t_text;
	*length = ((DB_TEXT_STRING *)(value->db_data))->db_t_count;
	break;
    }
}

/*{
**	Name:  f_lentrmwhite	Returns the length of a string if its trailing
**				blanks were trimmed.
**
** Description:
**	This procedure returns the length of a string if its trailing blanks
**	were trimmed. It does NOT actually trim the blanks.
**
** Inputs:
**	string		Points to a string of any characters
**			(even non-printing and NULL)
**
**	length		The entire length of the string.
**
** Outputs:
**	Returns:
**		The length of the string if its trailing blanks were trimmed.
**
** History:
**	19-jan-87 (grant)	implemented.
*/

i4
f_lentrmwhite(string, length)
u_char	*string;
i4	length;
{
    u_char  *last_non_blank;
    u_char  *s;
    u_char  *strend;

    if( length == 0 )	  
	return(0);

    for (s = string, last_non_blank = s-CMbytecnt(s), strend = s+length;
	 s <= strend-CMbytecnt(s); CMnext(s))
    {
	if (!CMwhite(s))
	{
	    last_non_blank = s;
	}
    }

    return (last_non_blank-string+CMbytecnt(last_non_blank));
}

/*{
** Name:	f_strT		Converts a string to a T formatted string.
**
** Description:
**	This procedure converts a string to one containing \c or \ddd
**	for control characters.
**
** Inputs:
**	source		The given string to convert.
**
**	srclen		The length of the given string.
**
**	dest		Must point to where the converted string will be
**			placed.
**
**	dstlen		The length of the buffer.
**
** Outputs:
**	source		Will point to the character following the last
**			character processed (for multi-line format).
**
**	srclen		The number of characters left in the string
**			that are unprocessed.
**
**	dest		Will contain the converted string.
**
**	Returns:
**		The length of the converted string.
**
** History:
**	27-jan-87 (grant)	implemented.
*/

i4
f_strT(source, srclen, dest, dstlen)
u_char	**source;
i4	*srclen;
u_char	*dest;
i4	dstlen;
{
    u_char	*s;
    u_char	*srcend;
    u_char	*d;
    u_char	*destend;

    s = *source;
    srcend = s + *srclen;
    d = dest;
    destend = d + dstlen;

    for (; s < srcend && d <= destend-CMbytecnt(s); CMnext(s))
    {
	switch (*s)
	{
	case '\n':
	case '\t':
	case '\b':
	case '\r':
	case '\f':
	case '\\':
	    if (d+2 > destend)
	    {
		*d++ = ' ';
		s--;
	    }
	    else
	    {
		switch(*s)
		{
		case '\n':
		    f_stcopy(ERx("\\n"), d);
		    break;

		case '\t':
		    f_stcopy(ERx("\\t"), d);
		    break;

		case '\b':
		    f_stcopy(ERx("\\b"), d);
		    break;

		case '\r':
		    f_stcopy(ERx("\\r"), d);
		    break;

		case '\f':
		    f_stcopy(ERx("\\f"), d);
		    break;

		case '\\':
		    f_stcopy(ERx("\\\\"), d);
		    break;
		}
		d += 2;
	    }
	    break;

	default:
	    if (CMcntrl(s))
	    {
		u_char	chr;
		u_char	escout[5];
		i4	curpos;

		if (d+4 > destend)
		{
		    while (d < destend)
		    {
			*d++ = ' ';
		    }
		    s--;
		    continue;
		}

		/* convert the character to \ddd, where the d's are octal digits */
		chr = *s;
		STcopy(ERx("\\000"), (char *)escout);
		curpos = 3;

		/* while there are more octal digits */
		while (chr)
		{
		    /* put the digit in the output string */
		    escout[curpos] = '0' + chr % 8;
		    /* remove the digit from the current value */
		    chr /= 8;
		    curpos--;
		}

		f_stcopy(escout, d);
		d += 4;
	    }
	    else
	    {
		CMcpychar(s, d);
		CMnext(d);
	    }
	    break;
	}
    }

    *srclen -= s - *source;
    *source = s;
    return d-dest;
}

/*{
** Name:	f_Tstr		Converts string containing \c and \ddd for
**				control characters into one containing
**				control characters.
**
** Description:
**	This procedure converts a string containing \c and \ddd for control
**	characters into one containing control characters.
**
** Inputs:
**	source		The given string to convert.
**
**	srclen		The length of the given string.
**
**	dest		Must point to where the converted string will be
**			placed.
**
**	dstlen		The length of the buffer.
**
** Outputs:
**	dest		Will contain the converted string.
**
**	Returns:
**		The length of the converted string.
**
** History:
**	28-jan-87 (grant)	implemented.
**	19-nov-87 (kenl)
**		Removed call to f_patternmatch.  Routine fmt_cvt now
**		determines when to deal with pattern matching and will
**		call f_wildcard, when appropriate.
*/

i4
f_Tstr(source, srclen, dest, dstlen)
u_char	*source;
i4	srclen;
u_char	*dest;
i4	dstlen;
{
    u_char	*srcend;
    u_char	*d;
    u_char	*destend;

    srcend = source + srclen;
    d = dest;
    destend = d + dstlen;

    while (source < srcend && d <= destend-CMbytecnt(source))
    {
	if (*source == '\\')
	{
	    CMnext(source);
	    if (source < srcend)
	    {
		switch(*source)
		{
		case 'n':
		    *d = '\n';
		    break;

		case 't':
		    *d = '\t';
		    break;

		case 'b':
		    *d = '\b';
		    break;

		case 'r':
		    *d = '\r';
		    break;

		case 'f':
		    *d = '\f';
		    break;

		default:
		    if (*source >= '0' && *source <= '7')
		    {
			u_char	chr;
			i4	num;

			chr = 0;
			for (num = 3; source < srcend &&
				      *source >= '0' && *source <= '7' &&
				      num > 0; CMnext(source), num--)
			{
			    chr = 8*chr + (*source - '0');
			}

			*d++ = chr;
			continue;	/* don't need to increment ptrs */
		    }
		    else
		    {
			CMcpychar(source, d);
		    }
		}
	    }
	}
	else
	{
	    if (CMbytecnt(source) > 1)
	    {
		*(d+1) = *(source+1);
	    }
	}
	CMnext(d);
	CMnext(source);
    }

    return d-dest;
}

/*{
** Name:	f_strCformat	Converts a string to one containing period
**				for control characters.
**
** Description:
**
** Inputs:
**	source		The given string to convert.
**
**	srclen		The length of the given string.
**
**	dest		Must point to where the converted string will be
**			placed.
**
**	dstlen		The length of the buffer.
**
** Outputs:
**	source		Will point to the character following the last
**			character processed.
**
**	srclen		The number of characters left in the string
**			that are unprocessed.
**
**	dest		Will contain the converted string.
**
**	Returns:
**		The length of the converted string.
** Side Effects:
**
** History:
**	28-jan-87 (grant)	implemented.
**	2/22/90 (b7213)
**		It is possible that we are trying to format 
**		a value which has no value even though the length has been set.
**		This particular problem was shown when printing a column in
**		the footer of a report run with the -h flag.
**		The -h flag never retrieves data.  So break out of the loop,
**		don't increment anything and then f_string will MEfill the
**		output string with spaces.
*/

i4
f_strCformat(source, srclen, dest, dstlen, change_cntrl)
u_char	**source;
i4	*srclen;
u_char	*dest;
i4	dstlen;
bool	change_cntrl;
{
    u_char	*s;
    u_char	*srcend;
    u_char	*d;
    u_char	*destend;
    i4		num_blanks;

    s = *source;
    srcend = s + *srclen;
    d = dest;
    destend = d + dstlen;

    for (; s < srcend && d <= destend-CMbytecnt(s); CMnext(s), CMnext(d))
    {
	if (*s == '\t')
	{
	    /* convert tabs to an equivalent number of blanks */

	    for (num_blanks = 8 - ((d-dest) % 8); num_blanks > 0 && d < destend;
		 num_blanks--, d++)
	    {
		*d = ' ';
	    }
	    d--;
	}
	else if (*s == '\n' || *s == '\r')
	{
	    CMnext(s);
	    break;
	}
	/* set white space to blank for non Kanji characters only */
	else if (CMwhite(s) && !CMdbl1st(s))
	{
	    *d = ' ';
	}
	else if (change_cntrl && CMcntrl(s))
	{
	    *d = '.';
	}
	/* 7213 */
	else if (*s == '\0' )
	{
		break;
	}
	else
	{
	    CMcpychar(s, d);
	}
    }

    *srclen -= s - *source;
    *source = s;
    return d-dest;
}

/*{
** Name:	f_stcopy	Copies a null-terminated string to a buffer
**				without null-termination.
**
** Description:
**	This procedure copies a null-terminated string to a buffer without
**	null-termination.
**
** Inputs:
**	source		Null-terminated string to copy.
**
** Outputs:
**	dest		Will contain copied string without the null terminator.
**
** History:
**	27-jan-87 (grant)	implemented.
*/

f_stcopy(source, dest)
u_char	*source;
u_char	*dest;
{
    while (*source)
    {
	*dest++ = *source++;
    }
}


/*{
** Name:	f_revrsbuf	Reverse the contents of a buffer.
**
** Description:
**	This procedure reverses the contents of a buffer with no regard
**	for a null-terminator.	If so specified, also swaps (, {, [,
**	and < for their counterparts--), }, ], and >.
**
**	Reverses by swapping buf[0] with buf[width-1],
**	buf[1] with buf[width-2], and so forth,
**	up until the middle of the buffer.
**
**	This will NOT work with Kanji double-byte characters; assumption
**	is that Kanji users will not know about the reversed format.
**
** Inputs:
**	width		The number of bytes to reverse.
**			buf is assumed to be at least width+1 bytes wide.
**
**	swap		Specifies whether or not to swap brackets.
**
**	buf		The buffer whose contents are to be reversed.
**
** Outputs:
**	buf		The buffer with the first width bytes reversed.
**
** History:
**	27-mar-87 (bab) implemented.
**	13-may-87 (bab)
**		Added swap parameter to reverse the direction of various
**		kinds of brackets.
*/

f_revrsbuf(width, swap, buf)
i4		width;
bool		swap;
register char	*buf;
{
    char		temp;
    i4			numbytesToSwap = width / 2;
    register i4	i;

    for (i=0; i < numbytesToSwap; i++)
    {
	temp = buf[i];
	buf[i] = buf[width - 1 - i];
	buf[width - 1 -i] = temp;
    }
    if (swap)
    {
	for (i=0; i < width; i++)
	{
	    if (buf[i] == '(')
		buf[i] = ')';
	    else if (buf[i] == ')')
		buf[i] = '(';
	    else if (buf[i] == '[')
		buf[i] = ']';
	    else if (buf[i] == ']')
		buf[i] = '[';
	    else if (buf[i] == '{')
		buf[i] = '}';
	    else if (buf[i] == '}')
		buf[i] = '{';
	    else if (buf[i] == '<')
		buf[i] = '>';
	    else if (buf[i] == '>')
		buf[i] = '<';
	}
    }
}

/*{
** Name:	f_wildcard	Converts wildcard characters in a string
**				to SQL pattern-match characters and
**				strips backslash characters used for escaping.
**
** Description:
**	This routine converts wildcard characters in a string to
**	SQL pattern-matching characters and strips backslash characters
**	used for escaping special characters.
**
** Inputs:
**	string		string to convert.
**
**	qry_lang	specifies language being emitted by frontend
**
**	pm_cmd		indicates whether to perform pattern matching checks
**
** Outputs:
**	string		will contain the converted string.
**
**	sqlpat		set to one of the following values:
**				PM_NOT_FOUND
**				PM_FOUND
**				PM_USE_ESC
**				PM_QUEL_STRING
**
**	Returns:
**		none.
**
** History:
**	27-mar-87 (grant)	implemented.
**	19-nov-87 (kenl)
**		Added sqlpat parameter (i4 *).  sqlpat will be set to TRUE
**		if a QUEL pattern matching character was changed to its
**		SQL equivalent.
*/

f_wildcard(string, qry_lang, pm_cmd, sqlpat)
DB_TEXT_STRING	*string;
i4		qry_lang;
i4		pm_cmd;
i4		*sqlpat;
{
    u_char	*stripped;
    u_char	*unstripped;
    u_char	*unstrippedend;
    AFE_DCL_TXT_MACRO(DB_MAXSTRING) buffer;
    DB_TEXT_STRING *text;
    bool    esc_esc_used = FALSE;
    bool    esc_pat_used = FALSE;
    bool    esc_found = FALSE;
    bool    like_used = FALSE;
    char    *opt;

    *sqlpat = PM_NOT_FOUND;
    if (pm_cmd == PM_NO_CHECK)
    {
	return;
    }

    if (pat_lang == 0)
    {
	NMgtAt(ERx("II_PATTERN_MATCH"), &opt);
	pat_lang = DB_QUEL;
	if (opt != NULL && *opt != EOS)
	{
	    CVlower(opt);
	    if (STequal(opt, ERx("sql")))
	    {
		pat_lang = DB_SQL;
	    }
	}
    }

    unstripped = string->db_t_text;
    unstrippedend = unstripped + string->db_t_count;

    text = (DB_TEXT_STRING *) &buffer;
    stripped = text->db_t_text;

    if (qry_lang == DB_QUEL)
    {
	*sqlpat = PM_QUEL_STRING;
	*stripped++ = '"';
	if (pat_lang == DB_SQL)
	{
	    while ((unstripped < unstrippedend) &&
		(stripped < text->db_t_text + (DB_MAXSTRING - DB_CNTSIZE)))
	    {
		if (*unstripped == '\\')
		{
		    unstripped++;
		    if ((*unstripped == '\\') ||
		        (*unstripped == '*') || (*unstripped == '?'))
		    {
			*stripped++ = '\\';
		    }
		    else if (((*unstripped == '[') || (*unstripped == ']'))
			&& (pm_cmd != PM_GTW_CHECK))
		    {
			*stripped++ = '\\';
		    }
		    CMcpyinc(unstripped, stripped);
		}
		else	/* not an escape, translate pattern matching chars. */
		{
		    if ((*unstripped == '*') || (*unstripped == '?'))
		    {
			*stripped++ = '\\';
			*stripped++ = *unstripped;
		    }
		    else
		    {
			*stripped++ = sql_patternmatch(*unstripped);
		    }

		    if (CMbytecnt(unstripped) > 1)
		    {
			*stripped++ = *(++unstripped);
		    }
		    unstripped++;
		}   /* end of 'not an escape character, translate' */
	    }	/* end of while loop */


	}   /* end of SQL -> QUEL translation */
	else	/* start of QUEL -> QUEL */
	{
	    while ((unstripped < unstrippedend) &&
		(stripped < text->db_t_text + (DB_MAXSTRING - DB_CNTSIZE)))
	    {
		CMcpyinc(unstripped, stripped);
	    }
	}
	*stripped++ = '"';
	MEcopy((PTR) text->db_t_text, (u_i2) (stripped - text->db_t_text),
		(PTR) string->db_t_text);
	string->db_t_count = stripped - text->db_t_text;
    }	/* end of 'is query language QUEL?' */

    else    /* spitting out INGRES SQL or gateway SQL */
    {
	while ((unstripped < unstrippedend) &&
		(stripped < text->db_t_text + (DB_MAXSTRING - DB_CNTSIZE)))
	{
	    if (*unstripped == '\\')
	    {
		esc_found = TRUE;
		unstripped++;
		if (*unstripped == '\\')
		{
		    /*
		    **  Assume, initially, that pattern matching characters
		    **  will follow.  We must escape the '%', '_' and '\'.
		    */
		    *stripped++ = '\\';
		    esc_esc_used = TRUE;
		}
		else if ((*unstripped == '%') || (*unstripped == '_'))
		{
		    *stripped++ = '\\';
		    esc_pat_used = TRUE;
    		}
		CMcpyinc(unstripped, stripped);
	    }
	    else    /* no escape character found */
	    {
		if (((*unstripped == ']') || (*unstripped == '[')) &&
			(pm_cmd != PM_GTW_CHECK))
		{
		    /* patter matching found, escape them */
		    *stripped++ = '\\';
		    like_used = TRUE;
		    esc_pat_used = TRUE;
		    CMcpyinc(unstripped, stripped);
		}
		else if ((*unstripped == '%') || (*unstripped == '_'))
		{
		    if (pat_lang == DB_QUEL)
		    {
			/* if PM chars are QUEL, user must want literal %, _ */
			*stripped++ = '\\';
			esc_pat_used = TRUE;
		    }
		    else
		    {
			/* SQL are the PM chars, just copy it */
			like_used = TRUE;
    		    }
		    CMcpyinc(unstripped, stripped);
		}
		else
		{
		    if (pat_lang == DB_QUEL)
		    {
			/* PM chars are QUEL, translate them to SQL */
			*stripped++ = quel_patternmatch(*unstripped,
				&like_used);
			if (CMbytecnt(unstripped) > 1)
			{
			    *stripped++ = *(++unstripped);
			}
			unstripped++;
		    }
		    else
		    {
			/* PM chars are SQL, non-PM char found, just copy it */
			CMcpyinc(unstripped, stripped);
		    }
		}
	    }	/* end of 'no escape found' */
	}   /* end of while loop */

	if (((esc_pat_used || esc_esc_used || esc_found) && !like_used) ||
		((like_used && esc_esc_used) && !esc_pat_used))
	{
	    if (like_used)
	    {
		*sqlpat = PM_FOUND;
	    }

	    unstripped = string->db_t_text;
	    stripped = text->db_t_text;

	    while ((unstripped < unstrippedend) &&
		(stripped < text->db_t_text + (DB_MAXSTRING - DB_CNTSIZE)))
	    {
		if (*unstripped == '\\')
		{
		    unstripped++;
		    CMcpyinc(unstripped, stripped);
		}
		else
		{
		    if (pat_lang == DB_QUEL)
		    {
			*stripped++ = quel_patternmatch(*unstripped,
				&like_used);
			if (CMbytecnt(unstripped) > 1)
			{
			    *stripped++ = *(++unstripped);
			}
			unstripped++;
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
	    *sqlpat = PM_USE_ESC;
	}
	else if (like_used)
	{
	    *sqlpat = PM_FOUND;
	}

	MEcopy((PTR) text->db_t_text, (u_i2) (stripped - text->db_t_text),
		(PTR) string->db_t_text);
	string->db_t_count = stripped - text->db_t_text;
    }	/* end of 'emitting SQL' */
}

/*{
** Name:	quel_patternmatch	Given a character, returns the
**					SQL pattern-match character if it
**					is a wildcard character; otherwise,
**					returns the original character.
**
** Description:
**	This routine returns the SQL pattern-match character if the given
**	character is a wildcard character; otherwise it just returns the
**	given character.
**
** Inputs:
**	character	wildcard character from which to get the pattern-match
**			control character.
**
** Outputs:
**	like_used	set to TRUE if a QUEL pattern matching character was
**			changed to its SQL equivalent
**
**	Returns:
**		Pattern-match control character corresponding to wildcard
**		character; if not a wildcard character, the character itself
**		is returned.
**
** History:
**	27-mar-87 (grant)	implemented.
**	19-nov-87 (kenl)	added parameter sqlpat
*/

char
quel_patternmatch(character, like_used)
char	character;
bool	*like_used;
{
	switch(character)
	{
	case '*':
	    *like_used = TRUE;
	    return('%');
	    break;

	case '?':
	    *like_used = TRUE;
	    return('_');
	    break;

	default:
	    return character;
	    break;
	}
}


char
sql_patternmatch(character)
char	character;
{
	switch(character)
	{
	case '%':
	    return('*');
	    break;

	case '_':
	    return('?');
	    break;

	default:
	    return character;
	    break;
	}
}
