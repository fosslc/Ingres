/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <st.h>
# include	<stype.h> 
# include	<sglob.h> 
# include	<fedml.h>
# include	<cm.h>
# include	<er.h>

/*
**   S_QUER_SET - set up the query in response to the .quel or .sql command.
**	The .query command can be followed by any number of range
**	statements, and one retrieve statement (no into clause).
**	This routine breaks up the query into the component parts:
**		The range statements.
**		The target list for the retrieve.
**		The where clause on the retrieve.
**
**	It writes out the components to the query.rws file for insertion
**	into the rquery system table.  This checks some of  the basic
**	QUEL syntax on the way, and if any errors are encountered,
**	text is skipped up to the next formatter command.
**
**	Parameters:
**		none
**
**	Returns:
**		the type of the token that ended the query.  This 
**		should always be a TK_PERIOD.
**
**	Called by:
**		s_readin.
**
**	Side Effects:
**		Updates Tokchar, etc.
**
**	Error Messages:
**		906, 930, 941, 950, 951, 952, 953, 954, 955, 956, 957, 958, 959
**		Syserr: Improper return.
**
**	Trace Flags:
**		13.0, 13.11.
**
**	History:
**		6/24/81 (ps)	written.
**		12/22/81 (ps)	modified for two table version.
**		7/6/82 (ps)	allow range to have parameters
**		05/28/85 (drh)	modified to allow SQL SELECT statements
**		06/19/86 (jhw)  Conditional compilation for SQL code.
**		24-nov-86 (mgw) Bug # 10405
**			Set St_q_started to true on return to prevent more
**			than one of either .data or .query
**			Also, prevent multiple retrieves.
**		25-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		7/20/89 (elein) B6364
**			Return a syntax error if TK_PERIOD is returned
**			by s_q_add after a request for the TLIST. 
**			What we should have seen next is a FROM
**		8/22/89 (elein) garnet
**			Allow ENDFILE to signal end of query for .include
**		2/20/90 (martym)
**			Casting all call(s) to s_w_row() to VOID. Since it 
**			returns a STATUS now and we're not interested here.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**		4/9/90 (martym)
**			Modified to save the WHERE part of the query in the
**			WHERE section and not in the REMAINDER section.
**			(JUPBUG #9511).
**		5/9/90 (elein)
**			Fixed (again) problem where missing FROM caused
**			a syntax message.  THis change was lost during
**			garnet integrations.
**              11-oct-90 (sylviap)
**                      Added new paramter to s_g_skip.
**		11/22/90 (elein) 34634
**			Use global En_qlang instead of local qtype so that
**			it is available to sgskip.
**		2/4/91 (elein) 35943
**			pass along rtn_char so that s_q_add can know if
**			it is in a comment or not.
**		26-aug-1992 (rdrane)
**			Converted r/s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Fix-up external declarations -
**			they've been moved to hdr files.  Indicate that s_qlang_ok()
**			is static to this module.  Disable expanded namespace if the
**			main query language is QUEL.
**		14-dec-1992 (rdrane)
**			If the query language is QUEL, then only disable expanded
**			namespace for the duration of the query processing.
**		15-mar-1993 (rdrane)
**			For the W_WHERE case, invoke s_g_name() with FALSE (like
**			all the other instances.  The subsequent s_g_skip() would
**			lead to failure for the case
**				WHERE(a.col ...)
**			since the leading parens would be lost (bug 49813).

**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static	bool	s_qlang_ok();


/*
**	Stte definitions.
*/

# define	W_CMD	0		/* looking for "range" or "retrieve" */
# define	W_OF	1		/* looking for "of" in range */
# define	W_VAR	2		/* looking for range_var in range */
# define	W_IS	3		/* looking for "is" or "=" in range */
# define	W_NAME	4		/* looking for table name in range */

# define	W_TLST	5		/* want target list of retrieve */
# define	W_WHERE	6		/* want where clause on retrieve */
# define	W_SKIP	7		/* error occurred. Skip the rest of query */
# define    W_REMAINDER   12        /* want remainder of query */

# define	W_SQTLST 8		/* want SQL target list */
# define	W_SQFROM 9		/* want SQL from clause */
# define	W_SQWHERE 10		/* want SQL where clause */
# define   	W_SQREMAINDER 11       	/* want SQL remainder of query */

i4
s_quer_set()
{
	/* internal declarations */

	char	*nexttok;		/* ptr to found token */
	i4		type;			/* type of next char */
	i4		state;			/* state */
	i4		qtype;			/* Q_SQL or Q_QUEL */
	bool	get_char = TRUE; /* TRUE if next char needed */
	char	buf[256];
	char	*save_Tokchar;
	i4     rtn_char;		/* returns begin/end comment */
	bool	save_xns_given;	/* Save value of St_xns_given	*/

	/* start of routine */


	if (Cact_ren == NULL)
	{
		s_error(0x38A, FATAL, NULL);
	}

	if (St_q_started == TRUE)
	{	/* already have query or data for this report */
		s_error(0x3BC, NONFATAL, NULL);
		state = W_SKIP;
	}

	save_em();		/* temporarily save the Command fields */
	save_xns_given = St_xns_given;

	En_qlang = Q_QUEL;		/* assume it's quel to start with */
	STcopy(NAM_QUERY,Ctype);
	state = W_CMD;

	Csequence = 0;
	STcopy(ERx(" "),Csection);
	STcopy(ERx(" "),Cattid);
	STcopy(ERx(" "),Ccommand);
	STcopy(ERx(" "),Ctext);

	for(;;)
	{
		if (get_char)
		{
			type = s_g_skip(TRUE, &rtn_char);
		}
		get_char = TRUE;


		switch(state)
		{

			case(W_CMD):			/* want a range or retrieve */
				switch(type)
				{
					case(TK_ENDFILE):
						r_error(0x3A2, FATAL, NULL);
						goto allexit;
					case(TK_PERIOD):
						s_error(0x3BD, NONFATAL, NULL);
						goto allexit;

					case(TK_ALPHA):		/* check name */
						nexttok = s_g_name(FALSE);
						CVlower(nexttok);
						if (STequal(nexttok,NAM_RANGE) )
						{
							St_xns_given = FALSE;
							state=W_OF;
							STcopy(NAM_RANGE,Csection);
							if ((s_qlang_ok(En_qlang)) != TRUE )
								state=W_SKIP;
							break;
						}
						if (STequal(nexttok,NAM_RETR))
						{
							St_xns_given = FALSE;
							state=W_TLST;
							STcopy(NAM_TLIST,Csection);
							if ((s_qlang_ok(En_qlang)) != TRUE )
								state=W_SKIP;
							break;
						}

# ifdef SQL
						if (STequal(nexttok,NAM_SELECT))
						{
							En_qlang = Q_SQL;
							STcopy( NAM_SQL, Ctype );
							STcopy(NAM_TLIST, Csection);
							state = W_SQTLST;
							if ((s_qlang_ok(En_qlang)) != TRUE )
								state=W_SKIP;
							break;
						}
# endif /* SQL */

						s_error(0x3B7, NONFATAL, nexttok, NULL);
						state = W_SKIP;
						break;

					default:
						s_error(0x3B7, NONFATAL, Tokchar, NULL);
						state = W_SKIP;
						break;
				}
				break;

			case(W_OF):			/* want "of" in range */
				switch(type)
				{
					case(TK_ENDFILE):
						r_error(0x3A2, FATAL, NULL);
						goto allexit;
					case(TK_PERIOD):
						s_error(0x3BB, NONFATAL, NULL);
						goto allexit;

					case(TK_ALPHA):
						nexttok = s_g_name(FALSE);
						CVlower(nexttok);
						if (STequal(nexttok,NAM_OF) )
						{
							state = W_VAR;
							break;
						}
						/* fall through to error */

					default:
						s_error(0x3B6, NONFATAL, NULL);
						state = W_SKIP;
						break;
				}
				break;

			case(W_VAR):			/* want range_var */
			case(W_NAME):			/* want table name */
				switch(type)
				{
					case(TK_ENDFILE):
						r_error(0x3A2, FATAL, NULL);
						goto allexit;
					case(TK_PERIOD):
						s_error(0x3BB, NONFATAL, NULL);
						goto allexit;

					case(TK_ALPHA):
					case(TK_DOLLAR):
						if (type==TK_ALPHA)
						{
							nexttok = s_g_name(FALSE);
							CVlower(nexttok);
						}
						else
						{
							CMnext(Tokchar);
							STprintf(buf, ERx("$%s"), s_g_name(FALSE));
							nexttok = buf;
						}
						switch (state)
						{
							case(W_VAR):
								state = W_IS;
								STcopy(nexttok, Cattid);
								break;

							case(W_NAME):
								STcopy(nexttok,Ctext);	
								_VOID_ s_w_row();		/* good one */
								type = s_g_skip(TRUE, &rtn_char);
								STcopy(ERx(" "),Cattid);
								STcopy(ERx(" "),Ctext);

								if (type==TK_COMMA)
								{	/* expect more range statements */
									state = W_VAR;
								}
								else
								{	/* expect retrieve */
									get_char = FALSE;
									state = W_CMD;
								}
								break;
						}
						break;

					default:
						s_error(0x3B6, NONFATAL, NULL);
						state = W_SKIP;
						break;
				}
				break;

			case(W_IS):			/* want "is" in range */
				switch(type)
				{
					case(TK_ENDFILE):
						r_error(0x3A2, FATAL, NULL);
						goto allexit;
					case(TK_PERIOD):
						s_error(0x3BB, NONFATAL, NULL);
						goto allexit;

					case(TK_EQUALS):
						state = W_NAME;
						break;

					case(TK_ALPHA):
						nexttok = s_g_name(FALSE);
						CVlower(nexttok);
						if (STequal(nexttok,NAM_IS))
						{
							state = W_NAME;
							break;
						}
						/* fall through on error */

					default:
						s_error(0x3B6, NONFATAL, NULL);
						state = W_SKIP;
						break;

				}
				break;

			case(W_TLST):			/* want target list */
				switch(type)
				{
					case(TK_ENDFILE):
						r_error(0x3A2, FATAL, NULL);
						goto allexit;
					case(TK_PERIOD):
						s_error(0x3BB, NONFATAL, NULL);
						goto allexit;

					case(TK_OPAREN):	/* only ok token */
						type = s_q_add(type,NAM_TLIST,En_qlang, &rtn_char);
						if (type == TK_CPAREN)
						{	/* looks good */
							STcopy(NAM_WHERE, Csection);
							get_char = TRUE;
							state = W_WHERE;
							break;
						}
						if (type == TK_ENDFILE || type == TK_ENDSTRING)
						{
							r_error(0x3A2, FATAL, NULL);
						}
						/* fall through to error */
						s_error(0x3B9, NONFATAL, NULL);
						state = W_SKIP;
						break;

					case(TK_ALPHA):
						nexttok = s_g_name(FALSE);
						CVlower(nexttok);
						if (STequal(nexttok, ERx("unique") ))
						{
							STcopy(nexttok, Cattid);
						}
						else
						{
							/* must be "retrieve [into] <table>" - no good */
							s_error(0x3B8, NONFATAL, NULL);
							state = W_SKIP;
						}
						break;

					default:
						s_error(0x3B9, NONFATAL, NULL);
						state = W_SKIP;
						break;

				}
				break;

			case(W_WHERE):			/* want where clause */
				switch(type)
				{
					case(TK_ENDFILE):
						/* NO BREAK */
					case(TK_PERIOD):
						/* no where clause is ok */
						goto allexit;

					case(TK_ALPHA):
						save_Tokchar = Tokchar;
						nexttok = s_g_name(FALSE);
						if (STbcompare(nexttok, 0,
								NAM_WHERE, 0, TRUE) == 0)
						{
							type = s_g_skip(TRUE, &rtn_char);
							type = s_q_add(type, NAM_WHERE, En_qlang,&rtn_char);
						}
						else
						{
							Tokchar = save_Tokchar;
							get_char = FALSE;
							state = W_REMAINDER;
							STcopy(NAM_REMAINDER, Csection);
							break;
						}

						if (type == TK_ENDFILE ||
							type == TK_PERIOD)
						{	/* good */
							goto allexit;
						}
						if ( type == TK_ENDSTRING)
						{
							r_error(0x3A2, FATAL, NULL);
						}
						/*
						** If the language is QUEL and we have a WHERE
						** clause then there shouldn't be a REMAINDER,
						** so fall through.
						*/

					default:
						s_error(0x3BA, NONFATAL, NULL);
						state = W_SKIP;
						break;

				}
				break;


			case(W_REMAINDER):              /* want remainder */
					switch(type)
					{
					case(TK_ENDFILE):
						/* NO BREAK */
					case(TK_PERIOD):
						/* no where clause is ok */
						goto allexit;
					case(TK_ALPHA):
						type = s_q_add(type, NAM_REMAINDER, En_qlang,&rtn_char);
						if (type == TK_PERIOD || type == TK_ENDFILE)
						{       /* good */
							goto allexit;
						}
						if (type == TK_ENDSTRING)
						{
							r_error(0x3A2, FATAL, NULL);
						}
						/* fall through to error */
					default:
						s_error(0x3BA, NONFATAL, NULL);
						state = W_SKIP;
						break;
					}
					break;	
# ifdef SQL
			case(W_SQTLST):		/* want SELECT target */
				switch(type)
				{
					case(TK_ENDFILE):
						r_error(0x3A2, FATAL, NULL);
						goto allexit;
					case(TK_PERIOD):
						s_error(0x3BE, NONFATAL, NULL);
						goto allexit;
						break;

					case(TK_OPAREN):
						s_error(0x3BE, NONFATAL, NULL);
						goto allexit;
						break;

					default:
						if (type == TK_ALPHA)
						{
							save_Tokchar = Tokchar;
							nexttok = s_g_name(FALSE);
							CVlower(nexttok);
							if (STequal(nexttok, ERx("distinct"))  ||
							    STequal(nexttok, ERx("all")) )
							{
								STcopy(nexttok, Cattid);
								type = s_g_skip(TRUE, &rtn_char);
							}
							else
							{
								/* back up */
								Tokchar = save_Tokchar;
							}
						}

						type = s_q_add(type, NAM_TLIST, En_qlang,&rtn_char);
						if ( type == TK_ENDFILE || 
							type == TK_ENDSTRING )
						{
							r_error(0x3A2, FATAL, NULL);
							break;
						}
						/* B6364 */
						else if (type == TK_PERIOD)
						{
							s_error(0x3BE, NONFATAL, NULL);
							goto allexit;
							break;
						}
						/* B6364 */
						else
						{
							STcopy(NAM_FROM,Csection);
							state = W_SQFROM;
							break;
						}

				}
				break;

			case(W_SQFROM):		/* want SQL FROM clause */
				type = s_q_add(type, NAM_FROM, En_qlang,&rtn_char);

				if ( type == TK_PERIOD || type == TK_ENDFILE )
				{
					goto allexit;
					break;
				}
				else if( type == TK_ENDSTRING)
				{
					r_error(0x3A2, FATAL, NULL);
					break;
				}
				save_Tokchar = Tokchar;
				nexttok = s_g_name(FALSE);
				if (STbcompare(nexttok, 0,
						NAM_WHERE, 0, TRUE) == 0)
				{
					get_char = TRUE;
					state = W_SQWHERE;/* want where clause */
					STcopy(NAM_WHERE, Csection);
				}
				else
				{
					Tokchar = save_Tokchar;
					get_char = FALSE;
					state = W_SQREMAINDER;	/* want the raminder, not where */
					STcopy(NAM_REMAINDER, Csection);
				}
				break;

			case(W_SQWHERE):                /* want SQL WHERE clause */
				type = s_q_add(type, NAM_WHERE, En_qlang,&rtn_char);
				if ( type == TK_ENDSTRING )
				{
					r_error(0x3A2, FATAL, NULL);
					break;
				}
				if (type == TK_PERIOD || type == TK_ENDFILE)
				{
					goto allexit;
					break;
				}
				get_char = FALSE;
				state = W_SQREMAINDER;  /* want the remainder of the query */
				STcopy(NAM_REMAINDER, Csection);
				break;

			case(W_SQREMAINDER):	/* want the remainder of the query */
				switch(type)
				{
					case(TK_ENDFILE):
						/* NO BREAK */
					case(TK_PERIOD):
						goto allexit;
						break;

					default:
						type = s_q_add(type, NAM_REMAINDER, En_qlang,&rtn_char);
						if (type == TK_PERIOD || 
							type == TK_ENDFILE)
						{
							goto allexit;
						}
						r_error(0x3A2, FATAL, NULL);
						s_error(0x3BF, NONFATAL, NULL);
						state = W_SKIP;
						break;
				}
				break;
# endif /* SQL */

			case(W_SKIP):			/* error occurred. skip */
				switch(type)
				{
					case(TK_ENDFILE):
						/* NO BREAK */
					case(TK_PERIOD):
						goto allexit;

					default:
						nexttok = s_g_token(FALSE);
						break;

				}
				break;

		}

	}

	/* should never get here */


	/* all exits come here */

	allexit:	
		get_em();		/* restore Command fields */
		St_xns_given = save_xns_given;

		St_q_started = TRUE;	/* Bug 10405 - only one .query or */
					/* .data per report */
		return(type);
}

/*
**	S_QLANG_OK - confirm that the query language specified in the
**		.QUERY command is a valid one for the installation by
**		calling FEdml and seeing if it is one of the ones available.
**		If it is not a valid query language, print an error and return
**		FALSE, otherwise return TRUE.
**
**	Parameters:
**		qutype  The query langauge type, Q_QUEL or Q_SQL
**
**	Returns:
**		TRUE if the query language is allowed for the installation,
**		FALSE otherwise.
**
**	Called by:
**		s_quer_set
**
**	Side effects:
**		none
**
**	History:
**		7/9/85	(drh)	Written.
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Indicate that this routine is static.
*/

static
bool
s_qlang_ok( qutype )
i4	qutype;		/* query language from .QUERY stmt */
{
	bool	lang_ok;	/* assume language not valid */
# ifdef SQL
	i4	def;		/* default query language */
	i4	avail;		/* available query languages */

	FEdml( &def, &avail );	

	if ( avail == FEDMLBOTH )
		lang_ok = TRUE;
	else if ( avail == FEDMLSQL && qutype == Q_SQL )
		lang_ok = TRUE;
	else if ( avail == FEDMLQUEL && qutype == Q_QUEL )
		lang_ok = TRUE;
	else
		lang_ok = FALSE;
	
# else	/* not SQL */
	lang_ok = qutype == Q_QUEL;
# endif /* SQL */
	if (lang_ok == FALSE)
	{
		s_error(0x3DE, NONFATAL, (qutype == Q_SQL) ? ERx("sql") : ERx("quel"),
				NULL);
	}

	return lang_ok;
}
