/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sgstring.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<ui.h>
# include	<cm.h>

/*
**   S_G_STRING - pull out a string from the internal line.  The string
**	will be all characters between the quotes.
**	If an end of line is encountered, it is ignored.
**	Note that the opening and closing quotes are included in the string.
**	Also, any backslashed characters are passed directly into the string,
**	with their corresponding backslash doubled except when this routine is 
**	called by report when the -i flag is specified.  If it is called
**	by report then we don't double the backslash (we only double
**	the backslash when the string is to be stored in the database).
**
**	Both single- and double-quote delimited strings are now supported.
**	In a single-quoted string, single quotes are escaped by being 
**	doubled, and are passed through transparently.  Any double quotes
**	encountered in the string are escaped so they pass through silently.
**
**	If the 6.5 expanded namespace (delimited identifiers) is enabled
**	(St_xns_given == TRUE), then double quoted strings are considered to
**	be delimited identifiers.  Note that no validation exists when extracted
**	by this routine!  Backslashes are not valid in this instance, and so are
**	simply doubled - destined to cause errors further down the line.  Embedded
**	double quotes are escaped by doubling, and so are passed through.
**
**	Note that while strings may span lines, they may not exceed
**		(MAXLINE - 2 - 1)
**
**	Parameters:
**		inc			TRUE if to point to next character after last
**					character in string.
**		tok_type	Type of string - TK_QUOTE or TK_SQUOTE
**
**	Returns:
**		address of static buffer containing string.
**
**	History:
**		5/29/81 (ps) - written.
**		11/22/83 (gac)	added inc.
**		5/10/84 (gac)
**			fixed bug 1804 -- double backslashes within quotes now works.
**		08/06/86 (drh)	Added TK_SQUOTE support
**		25-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		21-jul-89 (cmr)
**			fixed bug 6865 -- do NOT double backslashes if
**			called by report (when the -i flag is specified
**			report calls s_readin to load the report).
**		26-mar-90 (cmr) fix for bug 9362
**			Do not convert a tab to a single space; let FMT
**			deal with embedded tabs.  This is consistent with
**			the way tabs are handled for data columns.  This 
**			code was probably left over from the days when 
**			rcotext was a 'c' field (now it is a varchar).
**		17-sep-1992 (rdrane)
**			Add support for delimited identifiers.  Converted 
**			r_error() err_num value to hex to facilitate lookup
**			in errw.msg file.  Correct the fact that count was not being
**			decremented when extra backslashes were put into the static buffer.
**			Allow for two delimiters and a NULL terminater ALWAYS.
**			Rename type parameter to tok_type.  If the static buffer becomes
**			full, just give up.  Previously, it read through the file happily
**			throwing away characters until it hit the EOF trap.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



char *
s_g_string(inc, tok_type)
bool	inc;
i4		tok_type;
{
	/* internal declarations */

	register char	*t;						/* ptr to internal string */
			char	*tTokchar;
											/* Limit of characters returned */
	register i4	count = (MAXLINE - 2 - 1);
			i4		nchars;					/* # of chars returned from read */
			char	stdelim;

	static	char	tok_buffer[MAXLINE+1];	/* contains string */


	/* start of routine */

	t = tok_buffer;

	if ( tok_type == TK_SQUOTE )
		stdelim = '\'';
	else
		stdelim = '\"';

	/*
	** Account for the initial character, which had better be the
	** same as *Tokchar (so why aren't we using it?)
	*/
	*t++ = stdelim;
	CMbytedec(count, Tokchar);
	CMnext(Tokchar);

	for(;;)
	{
		if (*Tokchar == '\0')
		{	/* end of line, get another */
			nchars = s_next_line();
			if (nchars == 0)
			{	/* end of file found */
				r_error(0x3A2, FATAL, NULL);
			}
		}

		if (CMbytedec(count, Tokchar) <= 0)
		{
			r_error(0x3A2, FATAL, NULL);
		}

		if ( *Tokchar == '\'' && tok_type == TK_SQUOTE )
		{
			tTokchar = Tokchar;
			CMnext(Tokchar);
			if ( *Tokchar == '\'' ) /* check for escape */
			{
				/* it's escaped */

				*t++ = '\'';
				CMcpyinc(Tokchar, t);
				count--;
			}
			else
			{
				Tokchar = tTokchar;
				break;		/* end of singquote str */
			}
		}
		else if ( *Tokchar == '\"'  && tok_type == TK_QUOTE)
		{
			if  (!St_xns_given)
			{
				break;	/* No delim_ids, so end of string!	*/
			}
			tTokchar = Tokchar;
			CMnext(Tokchar);
			if ( *Tokchar == '\"' )
			{
				/* it's escaped */

				*t++ = '\"';
				CMcpyinc(Tokchar, t);
				count--;
			}
			else
			{
				Tokchar = tTokchar;
				break;		/* end of doublequote str */
			}
		}
		else if (*Tokchar == '\\')
		{
			CMcpyinc(Tokchar, t);
			/**
			** fix bug 6865 (cmr)-- do not double backslash 
			** if called because R/W was invoked with -i.
			**/
			if ( !St_ispec && (IIUIdml() != UI_DML_GTWSQL))
			{
				/*
				** add another which will be
				** stripped by COPY
				*/
				*t++ = '\\';
				count--;
			}
			if (*Tokchar != '\0' && tok_type == TK_QUOTE)
			{
				CMcpychar(Tokchar, t);
				CMnext(t);
				if (*Tokchar == '\\' && !St_ispec
					&& (IIUIdml() != UI_DML_GTWSQL))
				{
					*t++ = '\\';
					count--;
				}
				CMnext(Tokchar);
			}
		}
		else
		{
			CMcpyinc(Tokchar, t);
		}
	}

	if (count > 0)
	{
		*t++ = stdelim;
	}

	*t = '\0';

	if (inc)
	{
		CMnext(Tokchar);
	}


	return(tok_buffer);
}
