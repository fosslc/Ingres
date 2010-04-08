/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <st.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<fedml.h>
# include	<cm.h>
# include	<er.h>
# include	<ug.h>
# include	<errw.h>

/*{
** Name:	r_qur_set() -	Set Up Query Variables from Query Structure.
**
** Description:
**	This processes the QUR lines from the RCOMMANDS table, and sets up the
**	range statements, target list and qualification to the query.  The type
**	of the clause is specified in the "rcosection" field in the table.  For
**	range statements, the "rcoattid" field contains the range_variable and
**	the "rcotext" field contains the table name.  For the "targetlist" and
**	"where" clause, the "rcotext" field contains the clause.
**
** Parameters:
**	none.
**
** Returns:
**	TRUE	if query successfully processed (not run).
**	FALSE	if no query read, or if errors occurred.
**
** Called by:
**	r_chk_dat().
**
** Side Effects:
**	Sets up the following:
**		En_tar_list, En_q_qual, Q_buf.
**
** Trace Flags:
**	1.0, 1.13.
**
** Error Messages:
**	20, 21, 1008, 23.
**
** History:	
 * 
 * Revision 60.10  87/09/14  10:26:00  grant
 * moved errw.h to report facility hdr.
 * 
 * Revision 60.9  87/08/21  08:42:57  peterk
 * update for ER changes.
 * 
 * Revision 60.8  87/08/16  10:52:47  peter
 * Remove ifdefs for xRTR1 xRTR2 xRTR3 xRTM
 *
 * Revision 60.7  87/05/21  18:05:05  grant
 * now allows unique/distinct and sort by/order by clauses in .QUERY
 *
 * Revision 60.6  87/04/22  11:25:29  grant
 * changed to handle native-SQL queries for 6.0
 *
 * Revision 60.5  87/04/13  19:12:46  grant
 * integrated Kanji changes.
 *
 * Revision 60.4  87/04/08  01:26:53  joe
 * Added compat, dbms and fe.h
 *
 * Revision 60.3  87/03/26  18:53:48  grant
 * Initial 6.0 changes calling FMT, AFE, and ADF libraries
 *
 * Revision 60.2  86/11/19  19:42:43  dave
 * Initial60 edit of files
 *
** Revision 50.0  86/04/14	 10:49:10  wong
** Save range parameters in permanent storage.
**
** Revision 40.2  85/08/01	 14:09:08  wong
** Added support for SQL queries through interface to the
** Report Writer version of the SQL Translator.
**
**	12/29/81 (ps)	modified for two table version.
**	7/1/81 (ps)	written.
**	19-dec-88 (sylviap)
**		Changed memory allocation to use FEreqmem.
**	26-jul-89 (sylviap)
**		Bug 7011 - Changed error E_RW1016(22) to 
**		E_RW13F0 (1008).
**	18-aug-89 (sylviap)
**		Changed IIUGlbo_lowerBEobject(qur->qur_section) to
**		CVlower.  Cause a bug on IBM when it uppercased, then
**		failed the STcompare.
**	2/28/90 (elein) 20208
**		Changing backslash behaviour for queries...  
**		In the query, backslashes were passed literally
**		always.  So if they had a '\$' then the '\$'
**		would be passed along. But the $ would be escaped
**		for report writer purposes.  The $ is the only
**		significant character to the report writer at this
**		point. 
**		This is being changed so that if the backslash is
**		escaping a $, we will now only pass along the $,
**		not \$.  This should now allow the case where you
**		want to use a $ as a value in a query, as: where
**		table_owner != '$ingres'.  If you want a query which
**		contains literally '\$xxx', use '\\$xxx'.
**	3/20/90 (elein) performance
**		Change STcompare to STequal
**	03/16/92 (rdrane)
**		Added code to ignore comments (bug 40467).
**		Added code to treat "$" as variable token ID only if
**		1st character of token (bug 40757).
**		Restructured switch statement for efficiencies.  Specifically
**		added "qry_putchar" flag to avoid unnecessary duplication of
**		code, and removed EOS case (the while checks EOS as a condition
**		for continuation).  Also, CMnmchar() considers underscore valid.
**		Editorial changes to module to conform to coding conventions.
**	04-mar-92 (leighb) Desktop porting change:
**		Moved function declaration outside of calling function.
**	05/15/92 (rdrane)
**		Correct in_quote detection (bug 44270).  Fell into the
**		"C" confusion between relation equals and assignment,
**		and so SQUOTE ==> DBL_QUOTE for QUEL, and vice versa
**		for SQL.  This was introduced with the fix to bug 40467.
**	6-oct-1992 (rdrane)
**		Disable expanded namespace if the query language is
**		QUEL.  Converted r_error() err_num values to hex to
**		facilitate lookup in errw.msg file.  Remove declarations of
**		FEtsalloc() since it's already declared in hdr files.
**	14-dec-1992 (rdrane)
**		If the query language is QUEL, then only disable expanded
**		namespace for the duration of the query processing.
**	21-dec-1992 (rdrane)
**		Watch out for QUEL and escaped double quotes (bug 48537)!
**		Fix for bug 40757 caused an escaped double quote to throw
**		off the in_quote detection and subsequent token_start
**		setting alogorithm.
**	8-feb-1993 (rdrane)
**		Correct handling of nested comments.  Fell into the
**		"C" confusion between relation equals and assignment (again!),
**		and so any comment termination terminated all nesting levels.
**	14-dec-93 (rudyw)
**		Make a pass over this file to realign code which has gotten
**		out of alignment. This is in preparation for a real change
**		which will be more easily reviewed without misaligned code.
**	14-dec-93 (rudyw)
**		Fix bug 56264. Instead of proliferating more bsseen code,
**		handle certain chars within the backslash case to make sure
**		escaped " and \ are output. Retain special handling for $.
**		Allows for the elimination of the bsseen flag.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


bool
r_qur_set()
{
       register QUR	*qur;			/* fast ptr to QUR array */
       register char	*qc;			/* ptr to Q_buf loc */
       register char	*tc;			/* ptr to qur_text */
       register i4	qlen;			/* count of what's left in Q_buf */
		i4	sequence;		/* current seq in QUR array */
		bool	error = FALSE;		/* surrogate for return value */
		char	param[MAXPNAME+1];	/* hold a parameter name */
		char	*cp;			/* return from r_par_get() */
		char	oldsection[MAXPNAME+1];	/* old value of section. Used
						** in checking for new sect
						*/
		bool	is_sql  = (En_qlang == FEDMLSQL);
		bool	is_quel = (En_qlang == FEDMLQUEL);
		char	backslash = '\\';
		i4	comment_level = 0;	/* Comment nesting level */
		bool	in_quote = 0;
		bool	token_start;		/* Next non-white char may
						** begin token string. If see $
						** when TRUE, consider "$" to be
						** variable introduction char
						*/
		bool	qry_putchar = FALSE;	/* Add current char to buffer */
		bool	save_xns_given;		/* Save value of St_xns_given */


#define addtoqbuf(x)	((CMbytedec(qlen,x) > 0) ? (CMcpychar(x,qc),CMnext(qc)) : (x))

/* start of routine */

	qlen = En_qmax + 1;
	if ((Q_buf = (char *)FEreqmem ((u_i4)Rst4_tag,
			(u_i4)(qlen), TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("r_qur_set - Q_buf"));
	}
							 
	qc = Q_buf - 1;			/* will be set right presently */
	oldsection[0] = EOS;
	save_xns_given = St_xns_given;

	qur = Ptr_qur_arr;
	for (sequence = 1 ; sequence <= En_qcount ; ++qur, ++sequence)
	{	/* next QUR array element */

		CVlower(qur->qur_section);
		if (!is_sql && STequal(qur->qur_section, NAM_RANGE) )
		{ /* RANGE statement */
			St_xns_given = FALSE; /* Ensure expanded namespace disabled */
			if (!r_rng_add(FEtsalloc(Rst4_tag, qur->qur_attid),
					FEtsalloc(Rst4_tag, qur->qur_text)))
			{
				r_error(0x15, NONFATAL, qur->qur_attid,
							qur->qur_text, NULL);
			}
			STcopy(qur->qur_section, oldsection);
		}
		else
		{ /* Not a RANGE statement */

			/* Must be other section.  If same as last, add on */

			if (!STequal(oldsection, qur->qur_section) )
			{ /* new section found */
				STcopy(qur->qur_section, oldsection);

				CMbytedec(qlen, qc);
				if (STequal(qur->qur_section, NAM_TLIST) )
				{ /* target list */
					CMnext(qc);
					En_tar_list = qc;	/* start here */
					if (!STequal(qur->qur_attid, ERx(" ")) )
					{
						En_pre_tar = qur->qur_attid;
					}
				}
				else if (STequal(qur->qur_section, NAM_WHERE) )
				{
					CMnext(qc);
					En_q_qual = qc;		/* start here */
				}
				else if (is_sql &&
					 STequal(qur->qur_section, NAM_FROM) )
				{
					CMnext(qc);
					En_sql_from = qc;	/* start here */
				}
				else if (STequal(qur->qur_section, NAM_REMAINDER) )
				{
					CMnext(qc);
					En_remainder = qc;
				}
				else
				{	/* huh ?? */
					r_syserr(E_RW0040_r_qur_set_Bad_section,
							 qur->qur_section);
				}
			}

			/*
			** Get the command text
			**
			**	Dropped through on start of new section or
			**	directly on continuation of previous section.
			*/

			tc = qur->qur_text;   /* fast ptr */
			token_start = TRUE;
			while (*tc != EOS)    /* repeat until end of qur_text */
			{	
				if  ((comment_level > 0) &&
						 (*tc != '*') && (*tc != '/'))
				{
					CMnext(tc);
					continue;
				}
				qry_putchar = FALSE;
				switch(*tc)
				{
				case('\''):
				case('\"'):
					if  ((is_sql && (*tc == '\'')) ||
						 (is_quel && (*tc == '\"')))
					{
						in_quote = !in_quote;
					}
					if  (in_quote)
					{
						token_start = TRUE;
					}
					else
					{
						token_start = FALSE;
					}
					qry_putchar = TRUE;
					break;

				case('\\'):	/* backslash */

					CMnext(tc);
					token_start = FALSE;

					/*
					** Special handling so that backslash
					** is filtered out when char is $.
					*/
					if (*tc != '$')
					{
						addtoqbuf(&backslash);
					}

					/*
					** If the escaped char is $ or if quel
					** and char is \ or ", add the char
					** to the buffer this pass instead of
					** next as is usually done.
					*/
					if ( *tc == '$' ||
					  (is_quel && (*tc=='\"' || *tc=='\\')))
					{
						qry_putchar = TRUE;
					}
					break;

				case('$'):	/* possible parameter found */
                                        if (STscompare(tc,
                                               STlength(UI_FE_CAT_ID_65),
                                               UI_FE_CAT_ID_65,
                                               STlength(UI_FE_CAT_ID_65)) == 0)
                                        {
                                                qry_putchar = TRUE;
                                                break;
                                        }
					if (!token_start)
					{
						qry_putchar = TRUE;
						break;
					}
					token_start = FALSE;

					CMnext(tc);		/* skip $ */
					cp = param;
					while (CMnmchar(tc))
					{
						if (cp < &param[MAXPNAME])
						{
							CMcpychar(tc, cp);
							CMnext(cp);
						}
						CMnext(tc);
					}
					*cp = EOS;

					if (cp == param)
					{	/* bad param name */
						r_error(0x17, NONFATAL, tc, NULL);
						error = TRUE;
						break;
					}

					if ((cp = r_par_get(param)) == NULL)
					{	/* couldn't find param */
						/* Bug 7011 */
						r_error(0x3F0, NONFATAL, param, NULL);
						error = TRUE;
						break;
					}

					/* add param value to query */

					while (*cp != EOS)
					{
						addtoqbuf(cp);
						CMnext(cp);
					}
					break;

				case('+'): /* bona fide token separators */
				case('-'): /* Don't worry about "!" and	 */
				case('='): /* "^" since they must be	 */
				case('<'): /* followed by "=".		 */
				case('>'):
				case(','):
				case('('):
				case(')'):
				case('.'):
				case(':'):
					token_start = TRUE;
					qry_putchar = TRUE;
					break;

				case('/'): /* slash - check start of comment */
					if ((!in_quote) && (*(tc+1) == '*'))
					{
						CMnext(tc);
						CMnext(tc);
						comment_level++;
						token_start = FALSE;
						break;
					}
					if (comment_level > 0)
					{
						CMnext(tc);
						break;
					}
					token_start = TRUE; /* Division operator */
					qry_putchar = TRUE;
					break;

				case('*'): /* asterisk - check end of comment */
					if ((!in_quote) && (*(tc+1) == '/'))
					{
						CMnext(tc);
						CMnext(tc);
						comment_level--;
						if (comment_level == 0)
						{
							token_start = TRUE;
						}
						break;
					}
					if (comment_level > 0)
					{
						CMnext(tc);
						break;
					}
					token_start = TRUE; /* Multiplication /
							    **   exponentiation
							    */
					qry_putchar = TRUE;
					break;

				default:	/* add character directly */
					if (CMwhite(tc))
					{
						token_start = TRUE;
					}
					else
					{
						token_start = FALSE;
					}
					qry_putchar = TRUE;
					break;
				}	/* end switch */

        			if (qry_putchar)
            			{
                			addtoqbuf(tc);
                			CMnext(tc);
            			}
			}

			/* add a blank at end of each line */

			addtoqbuf(ERx(" "));
		}
	}		/* end loop */

	/* if query too long */

	if (qlen < 0)
	{
		CVna(En_qmax, param);
		r_error(0x14, NONFATAL, param, NULL);
		error = TRUE;
	}
	else if  ((En_tar_list == (char *)NULL) ||
			  ((is_sql) && (En_sql_from == (char *)NULL)))
	{
		error = TRUE;
	}

	St_xns_given = save_xns_given;

	return (bool)!error;
}
