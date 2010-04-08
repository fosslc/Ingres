/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfqurset.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cm.h>
# include	<st.h>
# include	<fe.h>
# include	<ui.h>
# include	<ug.h>
# include	<fedml.h>
# include	"rbftype.h"
# include	"rbfglob.h"

/*
**   RFQUR_SET - process the query for the report.  This is very
**	stylized by RBF in the routine rFm_data.
**	The one range statement, which declares the
**	variable "RBF" to range over the relation name has already
**	been read, so the En_n_attribs, etc has already
**	been read in.  Since the variable names are known,
**	the query can be checked for names, etc, which is
**	not possible in the standard report writer.
**
**	Since all RBF reports are simply "retrieve (RBF.all)"
**	reports, no target list checks are made.  The "where"
**	clause is searched to determine the column names, and
**	whether a range or value is being checked (by simply
**	seeing if the query contains a ">" in it.  This is
**	very minimal parsing,
**
**	Parameters:
**		none.
**
**	Returns:
**		TRUE	if query successfully processed (not run).
**		FALSE	if no query read, or if errors occurred.
**
**	Called by:
**		rFdisplay.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		200.
**
**	Error Messages:
**		Syserr if anything wrong.
**
**	History:
**		9/9/82 (ps)	written (from r_qur_set).
**		10/9/84 (gac)	fixed bug #4156 -- RBF now can read where
**				clause in query from report COPYREP'd and
**				SREPORT'd.
**		26-nov-1986 (yamamoto)
**			Modified for double byte characters.
**		9/22/89 (elein) UG changes ingresug change #90045
**				changed <rbftype.h> & <rbfglob.h> to
**				"rbftype.h" & "rbfglob.h"
**		15-jan-89 (cmr) fix for bug 9612
**				Took out call to CVlower() of query text; 
**				check for "like" as well as "LIKE".
**		15-jan-89 (cmr)
**				Fix to above fix.  Still need IIUGlbo...
**				for attname.
**		3/5/90 (martym)
**				Changed to parse the entire qualification 
**				stmt at one time and compensate for 
**				JoinDef's qualifications.
**		3/7/90 (martym)
**				Changed to handle JoinDef reports that contain 
**				RBF generated unique column names.
**		20-nov-1992 (rdrane)
**			Esentially rewrote this routine to support 6.5
**			delimited identifiers.  The parsing loop now uses
**			the RW routines and Tokchar.
**		8-dec-1992 (rdrane)
**			Use RBF specific global to determine if delimited
**			identifiers are enabled.
**		17-dec-1992 (rdrane)
**			Correct case sensitivity of the STbcompare() on
**			'LIKE'.  Allow for ignore of QUEL string constant.
**		26-jan-1993 (rdrane)
**			Correct TK_RELOP processing so that TK_EQUALS
**			resets state to FIND_OPAREN.  Correct and streamline
**			initial scan for JoinDefs.  Stop parsing upon seeing
**			the UNION clause (or its comment delimiter).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**	scanning states for where clause for determining
**	whether an attribute has a range
*/
# define 	FIND_OPAREN	1	/* look for "(" opening comparison */
# define 	FIND_IDENT	2	/* look for identifier		   */
# define	FIND_GREATER	3	/* look for ">" before ")"	   */
# define	FIND_CPAREN	4	/* look for ")" ending comparison  */
# define	FIND_2CPAREN	5	/* look for "))" ending range	   */


FUNC_EXTERN VOID rfNextQueryLine();
FUNC_EXTERN bool r_JDMaintAttrList();


bool
rFqur_set()
{
	/* internal declarations */

	i4		sequence;		/* current seq in QUR array */
	register QUR	*qur;			/* fast ptr to QUR array */
	i4		rfq_parse_state;	/*
						** Scanning state for where
						** clause.
						*/
	i4		type;			/* Token type returned	*/
	bool		rfq_error;		/* TRUE if parse error occurs*/
	i4		ord;			/* ordinal of attribute */
	COPT		*copt;			/* Copt structure */
	char 		*qual;			/* Pointer to qualification */
	char 		*n_qual;		/*
						** Pointer to qualification
						** after skip to any parens
						** for JoinDef
						*/
	char		chk_char;		/*
						** Hold current char to check
						** for comments.
						*/
	char		rvar[FE_UNRML_MAXNAME+1];
						/* Range Var name in where */
	char		attname[FE_UNRML_MAXNAME+1];
						/* Attribute name in where */
	char		ColName[FE_MAXNAME+1];	/* RBF Internal column name */
	FE_RSLV_NAME	rfq_ferslv;		/*
						** Work struct to decompose
						** compound identifiers.
						*/


	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(200,0))
	{
		SIprintf(ERx("rFqur_set: entry.\r\n"));
	}
#	endif


	qual = NULL;
	for (sequence = 1,qur = Ptr_qur_arr; sequence <= En_qcount;
	     qur++,sequence++)
	{	
		CVlower(qur->qur_section);
		if (STcompare(qur->qur_section,NAM_WHERE) == 0)
		{	
			_VOID_ rfNextQueryLine(&qual, qur->qur_text);
		}
	}


	/*
	** If we are here when creating a new report, we don't have 
	** any selection criteria to look for.
	*/
	if  (qual == NULL)
	{
		return(TRUE);
	}
	if  (*qual == EOS)
	{
		_VOID_ MEfree((PTR)qual);
		return(TRUE);
	}

	/*
	** If the source of data is a JoinDef, we need to skip over 
	** the part of the qualification that defines the join.  So, find
	** any open parenthesis - if none, then no selection criteria exist.
	** Otherwise, reset the qual pointer to the open parens.
	*/
	n_qual = qual;
	if  ((En_SrcTyp == JDRepSrc) &&
	     ((n_qual = STindex(qual,ERx("("),0)) == NULL))
	{
		return(TRUE);
	}

	r_g_set(n_qual);
	rfq_error = FALSE;
	rfq_parse_state = FIND_OPAREN;
	/*
	** Note that the parse state won't change until we first see
	** an open parens.
	*/
	while ((!rfq_error) && ((type = r_g_skip()) != TK_ENDSTRING))
	{
		switch(type)
		{
		case(TK_ALPHA):
		case(TK_QUOTE):
			if  (type == TK_QUOTE)
			{
				/*
				** Check for QUEL string constant first,
				** then check for disallowed delimited
				** identifier.
				*/
				if  (En_qlang == FEDMLQUEL)
				{
					_VOID_ MEfree((PTR)r_g_string(
								TK_QUOTE));
					break;
				}
				else if (!Rbf_xns_given)
				{
					rfq_error = TRUE;
					break;
				}
			}
			rfq_ferslv.name = r_g_ident(TRUE);
			/*
			** Handle the relation operator 'LIKE' if
			** that's what we're looking for.  Note that
			** for it to be a valid identifier, it would have
			** to be in quotes!
			*/
			if ((rfq_parse_state == FIND_GREATER) &&
			    (STbcompare(rfq_ferslv.name,
				       STlength(rfq_ferslv.name),
				       ERx("like"),
				       STlength(ERx("like")),TRUE) == 0))
			{
				rfq_parse_state = FIND_OPAREN;
				_VOID_ MEfree((PTR)rfq_ferslv.name);
				break;
			}
			/*
			** Handle the start of the UNION SELECT clause - 
			** it means that we're all done.  However, unless the
			** state is FIND_OPAREN, then we have an error which
			** the subsequent identifier check will catch (failed
			** for the identifier being a reserved word).  Note
			** that for it to be a valid identifier, it would have
			** to be in quotes!
			*/
			if ((rfq_parse_state == FIND_OPAREN) &&
			    (STbcompare(rfq_ferslv.name,
				       STlength(rfq_ferslv.name),
				       ERx("union"),
				       STlength(ERx("union")),TRUE) == 0))
			{
				_VOID_ MEfree((PTR)rfq_ferslv.name);
				_VOID_ MEfree((PTR)qual);
				return(TRUE);
			}
			/*
			** Keep skipping unless we're looking for an
			** identifier.  This will handle LOGICALS like
			** AND, OR, etc., as well as right side expressions.
			*/
			if  (rfq_parse_state != FIND_IDENT)
			{
				_VOID_ MEfree((PTR)rfq_ferslv.name);
				break;
			}
			rfq_ferslv.name_dest = &attname[0];
			rfq_ferslv.owner_dest = &rvar[0];
			rfq_ferslv.is_nrml = FALSE;
			FE_decompose(&rfq_ferslv);
			if  ((IIUGdlm_ChkdlmBEobject(rfq_ferslv.name_dest,
						     rfq_ferslv.name_dest,
						     rfq_ferslv.is_nrml)
					 == UI_BOGUS_ID) ||
			     ((rfq_ferslv.owner_spec) &&
			      (IIUGdlm_ChkdlmBEobject(rfq_ferslv.owner_dest,
						      rfq_ferslv.owner_dest,
						      rfq_ferslv.is_nrml)
					 == UI_BOGUS_ID)))
			{
				rfq_error = TRUE;
				_VOID_ MEfree((PTR)rfq_ferslv.name);
				break;
			}
			if (En_SrcTyp == JDRepSrc)
			{
			    if (!r_JDMaintAttrList(JD_ATT_GET_ATTR_NAME,
						   &rvar[0],&attname[0],
						   &ColName[0]))
			    {
				/*
				** Why doesn't this result in the fatal error
				** that a parse failure would?  The 6.4 version
				** would actually fail here if a UNION SELECT
				** was present and return - rFdisplay() does
				** not check the return code.  Since we now
				** handle the end of the WHERE clause in a more
				** sane manner, maybe this should be a fatal
				** error ...
				*/
				_VOID_ MEfree((PTR)rfq_ferslv.name);
				_VOID_ MEfree((PTR)qual);
				return(FALSE);
			    }
			    STcopy(&ColName[0],&attname[0]);
			}
			ord = r_mtch_att(&attname[0]);
			if (ord < 0)
			{
			    /* Bad attribute name */
			    IIUGerr(E_RF003A_rFqur_set__Bad_attrib, 
				    UG_ERR_FATAL,1,&attname[0]);
			}
			/*
			** Set the column options of the attribute.  We assume
			** its a value until/unless we find a range indicator.
			*/
			copt = rFgt_copt(ord);
			copt->copt_select = 'v';
			rfq_parse_state = FIND_GREATER;
			_VOID_ MEfree((PTR)rfq_ferslv.name);
			break;

		case(TK_OPAREN):
			CMnext(Tokchar);        /* Skip the paren	*/
			if  (rfq_parse_state == FIND_OPAREN)
			{
				rfq_parse_state = FIND_IDENT;
				break;
			}
			/*
			** Ignore open parens unless we're specifically
			** looking for them.  This handles instances of
			** min(), max(), etc.
			*/
			break;

		case(TK_CPAREN):
			CMnext(Tokchar);        /* Skip the paren	*/
			if ((rfq_parse_state == FIND_CPAREN) &&
			    (copt != (COPT *)NULL) &&
			    (copt->copt_select == 'r'))
			{
				rfq_parse_state = FIND_2CPAREN;
			    	copt = (COPT *)NULL;
				break;
			}
			if  (rfq_parse_state == FIND_2CPAREN)
			{
				rfq_parse_state = FIND_OPAREN;
				break;
			}
			/*
			** Ignore closing parens unless we're specifically
			** looking for them.  This also handles instances of
			** min(), max(), etc.
			*/
			break;

		case(TK_EQUALS):
		case(TK_RELOP):
			CMnext(Tokchar);        /* Skip the relation operator */
			/*
			** Handle '!=', '>=', '<=' compound operators
			*/
			if  (*Tokchar == '=')
			{
				CMnext(Tokchar);
			}
			if  (rfq_parse_state == FIND_GREATER)
			{
				if  (type == TK_RELOP)
				{
					/* Must be a range of values */
					copt->copt_select = 'r';
					rfq_parse_state = FIND_CPAREN;
				}
				else
				{
					rfq_parse_state = FIND_OPAREN;
				}
			}
			break;

		case(TK_SQUOTE):
			CMnext(Tokchar);
			/*
			** Handle single quoted string values atomically so
			** we don't get confused by their containing parens,
			** etc.
			*/
			_VOID_ MEfree((PTR)r_g_string(TK_SQUOTE));
			break;

		case(TK_DOLLAR):
			CMnext(Tokchar);
			/*
			** Handle variables independently from identifiers so
			** we don't get confused by something like '$like'.
			** Additionally, allow compound constructs so we don't
			** get confused by something like "$abc.columnname".
			** Note that this compound construct should only result
			** from user modifications, and is detrimental only
			** when the state is FIND_IDENT - we won't assign any
			** ColumnOptions to columnname.
			*/
			_VOID_ MEfree((PTR)r_g_ident(TRUE));
			break;

		default:
			/*
			** We should only really be here if we see the start
			** of a comment ...
			*/
			chk_char = *Tokchar;
			CMnext(Tokchar);
			if  ((chk_char == '/') && (*Tokchar == '*'))
			{
				/*
				** Note the implication that a comment may not
				** "interrupt" the WHERE clause.  If we
				** allowed one to, then we'd have to add
				** comment recognition to the entire parse
				** loop to avoid being confused!
				*/
				_VOID_ MEfree((PTR)qual);
				return(TRUE);
			}
			break;
		}
	}


	if ((rfq_error) || (rfq_parse_state != FIND_OPAREN))
	{
	    IIUGerr(E_RF003B_rFqur_set__Bad_where_,UG_ERR_FATAL,0);
	}

	_VOID_ MEfree((PTR)qual);

	return(TRUE);
}

