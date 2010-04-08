/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)sgskip.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	 <cm.h> 
# include	 <st.h> 

/*
**   S_G_SKIP - skip white spaces in the line.  White spaces are considered
**	to be blanks and tabs and newlines and end-of-string. 
**	Note that strings can be single- or double-quote delimited.
**	If at an end of line, read a new line.
**	Also skip over comments, even if nested.
**
**	Parameters:
**		inc	TRUE to move to the next character as its
**			first action.
**
**	Returns:
**		type - code for type of token next on the line.
**			Values are TK_NUMBER,TK_ALPHA,TK_SIGN,
**			TK_QUOTE,TK_SQUOTE, TK_OPAREN,TK_CPAREN, TK_EQUALS,
**			TK_COMMA , TK_PERIOD,
**			TK_MULTOP, TK_POWER, TK_RELOP,
**			TK_HEXLIT, or TK_OTHER.
**			At end of file, return TK_ENDFILE.
**		rtn_char
**		    - returns BG_COMMENT (begin comment) or
**			      END_COMMENT (close comment)
**		    - s_g_skip usually skips over comments, but 6.3/03 RBF saves
**		      joindef info (type of joindef, union clause, etc. -- see
**		      rbf/rjoindef.c for more info) in comments in the .QUERY 
**		      section.  s_g_skip needs to pass back info to s_q_add
**		      when it encounters a comment.  Responsiblity is up to
**		      s_q_add to keep the comments together, that is, not to
**		      change states in a comment.  The result would be part
**		      of a comment saved as a 'where' type in ii_rcommands and
**		      the rest of the comment saved as a 'remainder'.
**
**	Side Effects:
**		none.
**
**	Error Messages:
**		985.
**
**	Trace Flags:
**		12.0, 12.1, 12.10.
**
**	History:
**		5/29/81 (ps) - written.
**		11/22/83 (gac)	added inc.
**		11/30/83 (gac)	TK_NUMBER includes numbers preceeded by decimal
**				point (period).
**		1/24/86 (mgw)	Bug 7660 - check comment level (comlevel) before
**				returning TK_ENDFILE and give error if still in
**				a comment.
**		08/06/86 (drh)	Added TK_SQUOTE for single-quote delimited
**				strings.
**		24-mar-1987 (yamamoto)
**			Changed CH macros to CM.
**		09-feb-90 (sylviap)
**			Does not check for end of comment if not currently in
**			a comment.
**		7/26/90 (elein) 31757
**			In order to keep commented information in queries
**			for rbf joindef reports which have been copyrep'd out
**			ONLY: If we see a comment in the query section pass
**			back "TK_OTHER" instead of skipping the comments.  The
**			only places comments appear in RBF copyrep'd reports
**			are at the beginning before the .name (and so the
**			Cact_ren is NULL and we skip those comments) or now in
**			the query.  But Section check is still necessary for
**			sreporting >1 report in one file.
**		11-oct-90 (sylviap)
**			Added recognizing comments in a FROM clause because of
**			   joindef based on one table.  There would be no WHERE 
**			   clause in this query.
**			Added new parameter, rtn_char, to pass back to s_q_add
**			   if reaches a begin/end comment.  sreport recognizes 
**			   comments in the .QUERY only because RBF stores 
**			   joindef info there.  It's up to s_q_add, to keep 
**			   the comment together (don't change state until it
**			   reaches the end of comment).
**		05-nov-90 (sylviap)
**			Save comments in the .QUERY to database for both
**			RBF (copyrep) and RW reports.  Need to save for RW
**			reports to handle the case of archiving an RBF report
**			out (so will have width in comments) and saving back
**			to the database.  Need to preserve the report width.
**			(#33894)
**            11/21/90 (elein) 34584
**			At END COMMENT, allow ren_type of 's' (for saved rpts)
**			to be symetrical with BEGIN COMMENT
**		11/27/90 (elein) 34634
**			Don't save comments in QUEL queries 
**		17-may-1993 (rdrane)
**			Add support for hex literals.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-feb-2009 (gupsh01)
**	    In a UTF8 installation skip the Byte Order mark character if found.
**	27-mar-2009 (gupsh01)
**	    Previous change did not take into account that CM_ischarsetUTF8 is now
**	    renamed to CMischarset_utf8.
*/


i4
s_g_skip(inc, rtn_char)
bool	inc;
i4	*rtn_char;
{
	/* internal declarations */

	i4	type;			/* temp storage for return value */
	i4	count;			/* # of characters read in */
	i4	comlevel = 0;		/* nesting level of comments */
	bool	utf8 = CMischarset_utf8();

	/* start of routine */


	*rtn_char = 0;
	for(;;)
	{
		/* first see if a new line is needed */

		if ((Tokchar == NULL) || (*Tokchar == '\0') || (*Tokchar == '\n'))
		{
			count = s_next_line();
			if (count == 0)
			{
				if (comlevel > 0)
					s_error(985, NONFATAL, NULL);
				return(TK_ENDFILE);
			}
		}
		else
		{
			if (inc)
			{
				CMnext(Tokchar);
			}
		}

		inc = TRUE;

		while (CMwhite(Tokchar))
		{
			CMnext(Tokchar);
		}

		if (utf8 && CM_ISUTF8_BOM(Tokchar))
		{
		    CMnext(Tokchar);
		}

		if (*Tokchar == '\0')
		{
			continue;		/* get a new line */
		}
		else if (CMdigit(Tokchar))
		{
			type = TK_NUMBER;
		}
		else if (CMnmstart(Tokchar))
		{
			type = TK_ALPHA;
			if  (((*Tokchar == 'X') || (*Tokchar == 'x')) &&
			     (*(Tokchar + 1) == '\''))
			{
				type = TK_HEXLIT;
			}
		}
		else
		{
			switch (*Tokchar)
			{
				case('+'):
				case('-'):
					type = TK_SIGN;
					break;

				case(','):
					type = TK_COMMA;
					break;

				case('.'):
					if (CMdigit(Tokchar+1))
					{
						type = TK_NUMBER;
					}
					else
					{
						type = TK_PERIOD;
					}
					break;

				case('('):
					type = TK_OPAREN;
					break;

				case(')'):
					type = TK_CPAREN;
					break;

				case('='):
					type = TK_EQUALS;
					break;

				case('\"'):
					type = TK_QUOTE;
					break;

				case( '\''):
					type = TK_SQUOTE;
					break;

				case(':'):
					type = TK_COLON;
					break;

				case('$'):
					type = TK_DOLLAR;
					break;

				case('/'):
					/* is it a comment */
					if (*(Tokchar+1) == '*')
					{
					   /*
					   ** If this is a comment in the
					   ** where or remainder section
					   ** of an rbf report--pass back
					   **
					   ** added FROM section for single
					   ** jd reports (reports based on
					   ** a single table, so no where
					   ** clause).
					   **
					   ** added 's'. see change history
					   ** for more info (#33894)
					   */
					   if( Cact_ren != NULL && En_qlang == Q_SQL &&
					     ((*(Cact_ren->ren_type)=='f' ) ||
					     (*(Cact_ren->ren_type)=='s' )))
					   {
						if ((STequal(NAM_REMAINDER,Csection)) ||
					           (STequal(NAM_FROM,Csection)) || 
						   (STequal(NAM_WHERE,Csection)))
					        {
						    type = TK_OTHER;
						    *rtn_char = BG_COMMENT;
						    break;
						}
					   }
					   Tokchar++;
					   comlevel++;
					   break;
					}
					type = TK_MULTOP;
					break;

				case('*'):
					/* 
					** is it end of comment? only check if
					** currently in a comment
					*/
					if ((*(Tokchar+1) == '/') 
						&& (comlevel > 0))
					{
						Tokchar++;
						comlevel--;
						continue;
					}
					/*
					** If this is a comment in the
					** where or remainder section
					** of a report--pass back
					*/
					else if ((*(Tokchar+1) == '/')  &&
					          Cact_ren != NULL &&
					          En_qlang == Q_SQL &&
					        (*(Cact_ren->ren_type)=='f' ||
					         *(Cact_ren->ren_type)=='s') &&
					     (STequal(NAM_REMAINDER,Csection) ||
					      STequal(NAM_FROM,Csection) ||
					      STequal(NAM_WHERE,Csection)) )
					{
							type = TK_OTHER;
						        *rtn_char = END_COMMENT;
							break;
					}
					else if (*(Tokchar+1) == '*')
					{
						type = TK_POWER;
					}
					else
					{
						type = TK_MULTOP;
					}
					break;

				case('>'):
				case('<'):
					type = TK_RELOP;
					break;

				case('!'):
					if (*(Tokchar+1) == '=')
					{
						type = TK_RELOP;
					}
					else
					{
						type = TK_OTHER;
					}
					break;

				default:
					type = TK_OTHER;

			}
		}



		if (comlevel <= 0)
		{
			return(type);
		}
	}
}
