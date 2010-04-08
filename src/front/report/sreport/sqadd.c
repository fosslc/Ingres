/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include	<er.h>
# include	<st.h>
# include	<stype.h> 
# include	<sglob.h> 

/**
**   S_Q_ADD - add rows to the RCOMMAND file for an SQL FROM clause, an
**	SQL or QUEL target list, or the remainder of the query (which may
**	include a where clause, sort by or order by clause, or group by,
**	having, or union clause (for SQL).  
**	
**	For a FROM clause, (SQL queries only), items are added until
**	either a 'where', 'group by', 'having', 'union', 'order by' or the
**	next formatter command is found.
**
**	For the target list, if this is a QUEL query, items are added
**	until the parentheses are completed (the opening and closing
**	parentheses are skipped).  If this is an SQL query, items are
**	added until a 'from' token is found.
**
**	For a where clause, items are added until the next formatter 
**	command is found.
**
**	In any case, it will return on end of file,
**	or if the parenthesized nesting gets messed up.
**
**	(6.3/03) s_q_add saves comments in the .QUERY.  RBF saves joindef
**	info inside the comments.  If in a comment, noted by BG_COMMENT, it
**	must continue adding until it reaches the END_COMMENT.  Comment level
**	is tracked by in_comment (0 = not in a comment block).
**
**	Parameters:
**		type		the type of the next token in the line,
**				for the first time through.
**		clname		name of the clause (either NAM_TLIST or 
**				NAM_FROM or NAM_REMAINDER)
**		qtype		SQL or QUEL query
**
**	Returns:
**		type		the type of the token which ended it.
**				Returns TK_ENDSTRING on error.
**
**	Called by:
**		s_quer_set.
**
**	History:
**		6/29/81 (ps)	written.
**		12/22/81 (ps)	modified for two table version.
**		05/28/85 (drh)  modified for SQL support.
**		06/19/86 (jhw)	Conditional compilation for SQL code.
**		08/06/86 (drh)	Allow single quoted strings only in an SQL
**				query.
**		27-aug-86 (marian)	bug # 10064
**			When checking for sql from clause use STbcompare instead
**			of STcompare so case will not make a difference.
**		24-nov-86 (mgw) Bug # 10405
**			Flag error if multiple retrieves.
**		7/20/89 (elein) Bug 6364:
**			If we see a .command in a target list, they forgot
**			the from.  Return. squerset will display the error.
**		8/22/89 (elein) garnet
**			ENDFILE may be legit after from for .includes.
**			Dont forget to write row
**              9/14/89 (elein) JB6866
**                      Add token handling for = and !=.  They have been
**                      added to the list of possible tokens to so that
**                      .LET (JB6866) won't require spaces arount the =.
**		12/3/89 (elein) 8978
**			Check length correctly before adding a RELOP
**		2/8/90 (elein)
**			Ensure even number of parens when EOF reached
**		2/20/90 (martym)
**			Casting all call(s) to s_w_row() to VOID. Since it 
**			returns a STATUS now and we're not interested here.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**              4/9/90 (martym)
**			Modified to save the WHERE part of the query in the
**			WHERE section and not in the REMAINDER section.
**			(JUPBUG #9511).
**		11-oct-90 (sylviap) 
**			Checks if token is a comment (BG_COMMENT/END_COMMENT) 
**			in a WHERE,FROM or REMAINDER clause.  Needed because 
**			sreport was breaking up comments between the WHERE and 
**			REMAINDER.  RBF needs to have the joindef info (stored
**			in the comments) in one section.
**		05-nov-90 (sylviap) 
**			Changed the ERx to CONSTANTS to save space.
**			Added functionality to take the width saved in comments
**			in the .QUERY, and save it in the Ccommands column.
**			(#33894)
**		11/21/90 (elein) 34584
**			Check for in comment when hitting PERIOD.
**              3/7/91 (elein) 35943
**                      Add parameter rtn_char so that s_q_add can know if
**                      it is in a comment or not.  The context was lost
**			when a comment was encountered from sgskip called
**			from squerset.
**		3/7/91 (elein) 35728
**			Add conditions for parentheses and quotes for
**			when in a comment.   Don't validate them.
**		3/11/91 (elein) 36074
**			Don't set delimiter to nothing in the comma case.
**			If II_DECIMAL is set to a COMMA this causes syntax
**			errors from the DBMS.  If it has a space after it
**			it is OK.
**		8/20/91 (elein) 39295
**			Use delimeter before open paren and make it a blank
**			after a close paren.  RBF was not interpreting  words
**			delimited by parens
**		3/27/92 (rdrane) 35943/40467
**			Rework WIDTH comment detection and skipping logic.
**			Comments containg fewer than two tokens (e.g.,
**			"%*********%") would set skip_comment and so skip
**			ALL text until the next command or end of a subsequent
**			comment containing multiple tokens.  Additionally, the
**			WIDTH comment was NOT being skipped and was indeed
**			stored in the database.
**			When detecting BG_COMMENT, look for another comment
**			beginning so that we handle nested comments without
**			becoming confused or recognizing it as a WIDTH comment.
**		26-aug-1992 (rdrane)
**			Converted s_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Fix-up external declarations -
**			they've been moved to hdr files.
**			Support owner.tablename and column qualifications when
**			tablename and column are delimited identifiers, e.g.,
**			a."bc".  Additionally support owner and correlation
**			names as delimited identifiers, e.g., "a"."b".
**			For the a."b" case, s_g_token()	will consider it two
**			tokens, and s_q_add() will nominally separate them with
**			a space.  So, effect a look-ahead and if we see a '."',
**			then save the current token and concatenate it with the
**			next token before proceeding.  The "a".b and "a"."b"
**			cases go through s_g_ident(), not s_g_token, and so are
**			always returned intact.
**		23-dec-1992 (rdrane)
**			Re-architect significant portions of this routine to
**			correct comment recognition anomalies once and for all
**			(Bugs 44677 & 47949 ).  This also removes the need for
**			the s_g_token()/s_g_ident() kludge described above.
**		4-mar-1993 (rdrane)
**			Remove redundant code (probably copied instead of
**			moved ...).  Clarify comments regarding default case.
**		17-may-1993 (rdrane)
**			Added support for hex literals.
**		1-jun-1993 (rdrane)
**			Give error indication if s_g_ident() finds a delimited
**			identifier when they are not enabled.
**		21-jun-1993 (rdrane)
**			Correct improper rtn_char reference which resulted
**			from 1-jun-1993 fix.
**		20-jan-1994 (rdrane)
**			Correct improper parameterization of some error msgs
**			which probably resulted from 1-jun-1993 fix. b58958.
**		18-mar-1994 (rdrane)
**			Re-instate checks for in_comment when processing
**			quotes.  This was causing EOF failures for unmatched
**			quotes, and occasional "no/bad .delimid" failures for 
**			matched double quotes (b59963).
**		04-dec-1995 (kch)
**			In the function s_q_add(), the processing of any
**			characters within comments has been removed.
**			Matched double quotes within comments were not being
**			dealt with correctly. This change fixes bug 73003.
**		24-feb-1998 (i4jo01)
**			Allow for rather quirky method of dealing with closing
**			parens in stmt. We can get interrputed if processing
**			where clause with subselect and have parens and
**			GROUP BY. Now process as we did in 6.4 (b81774)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static	i4	count = MAXRTEXT;	/* Bytes left in forming line buffer*/
static	char	*delim = ERx("");	/* Add as separation between tokens */
static	char	*qt;			/* Ptr to accumulated tokens        */
static	bool	skip_comment = FALSE;	/*
					** TRUE if started/in a WIDTH
					** comment, so don't save in
					** database
					*/

static bool	sq_lgth_chk(i4);



i4
s_q_add(type, clname, qtype, rtn_char)
i4	type;
char	*clname;
i4	qtype;
i4	*rtn_char;
{
	/* internal declarations */

	i4	i, j;
	i4	inc = TRUE;
	i4	level = 0;		/* level of nesting of parens */
	i4	lgth = 0;		/* length of token */
	i4	in_comment = 0;		/*
					** Counts the levels of comments
					** but ONLY in WHERE, FROM
					** and REMAINDER.  Need to allow
					** comments in query because RBF
					** stores info this way
					*/
	char	*save_Tokchar;
	char	*tmp_tok;		/* temp ptr */
	char	tmp_buf[MAXRTEXT+1];
	char	compound_buf[((2 * (FE_MAXNAME + 2)) + 1)];


	/* start of routine */

	MEfill((MAXRTEXT + 1),EOS,Ctext);
	STcopy(clname,Csection);

	for(;;)
	{
		inc = TRUE;
		switch(type)
		{
		case(TK_OPAREN):
			if  (in_comment >  0)
			{
				break;
			}

			else
			{
				level++;
			}
			if  ((qtype == Q_SQL) || (level != 1) ||
			     (!STequal(clname,NAM_TLIST)))
			{	/*
				** Not open paren for target list.  "1"
				** is length of parens, and more efficient
				** then ERx("(") in this case.
				*/
				qt = ERx("(");
				lgth = STlength(delim) + 1;
				if  (!sq_lgth_chk(lgth))
				{
					return((i4)TK_ENDSTRING);
				}
			}
			delim = ERx("");
			break;

		case(TK_CPAREN):
			if  (in_comment >  0)
			{
				break;
			}

			else
			{
				level--;
			}
			if  ((level < 0) && (qtype != Q_SQL))
			{	/* gone too far.  return error */
				return((i4)TK_ENDSTRING);
			}

			if  (level == 0)
			{	/* no nesting.  If target list, return */
				if  ((qtype == Q_QUEL) &&
				     (STequal(clname,NAM_TLIST)))
				{	/*
					** Write out buffer and remove
					** any preceeding UNIQUE
					*/
					_VOID_ s_w_row();
					STcopy(WRD_BLANK,Cattid);
					return(type);
				}
			}

			/*
			** Just another closing paren.  Add it
			*/
			qt = ERx(")");
			delim = ERx("");
			lgth = 1;
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;

		case(TK_QUOTE):
			if  (in_comment > 0)
			{
				break;
			}
			else if  (qtype == Q_SQL)
			{
				qt = s_g_ident(FALSE,rtn_char,TRUE);
				if ((*rtn_char != UI_DELIM_ID) ||
				    (!St_xns_given))
				{
					s_error(0x0A7,NONFATAL,qt,NULL);
				}
			}
			else
			{
				qt = s_g_string(FALSE,(i4)TK_QUOTE);
			}
			lgth = STlength(delim) + STlength(qt);
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;

		case(TK_SQUOTE):
			if  (in_comment > 0)
			{
				break;
			}
			else if  (qtype == Q_QUEL)
			{
				/*
				** Single quotes not allowed in QUEL query
				*/
				s_error(0x3AC,NONFATAL,0);
				return((i4)TK_ENDSTRING);
			}
			else
			{
				qt = s_g_string(FALSE,(i4)TK_SQUOTE);
			}
			lgth = STlength(delim) + STlength(qt);
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;

		case(TK_COMMA):
			if  (in_comment > 0)
			{
				break;
			}
			qt = ERx(",");
			lgth = 1;
			delim = ERx("");
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;

		case TK_EQUALS:
		case TK_RELOP:
			if  (in_comment > 0)
			{
				break;
			}
			/*
			** For relation operators "=" or "!="
			** sgop automatically increments
			** so back up one
			*/
			save_Tokchar = Tokchar;
			qt = s_g_op();
			CMprev(Tokchar,save_Tokchar);

			lgth = STlength(delim) + STlength(qt);
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;

		case(TK_HEXLIT):
			if  (in_comment > 0)
			{
				break;
			}
			if  (qtype == Q_QUEL)
			{
				/*
				** Single quotes not allowed in QUEL query,
				** so hex literals are taboo too.
				*/
				s_error(0x3AC,NONFATAL,0);
				return((i4)TK_ENDSTRING);
			}
			tmp_buf[0] = 'X';
			tmp_buf[1] = EOS;
			CMnext(Tokchar);
			qt = s_g_string(FALSE,(i4)TK_SQUOTE);
			STcat(&tmp_buf[0],qt);
			qt = &tmp_buf[0];
			lgth = STlength(delim) + STlength(qt);
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;

		case TK_ALPHA:
			if  (in_comment > 0)
			{
				break;
			}
			save_Tokchar = Tokchar;
			qt = s_g_ident(FALSE,rtn_char,TRUE);
			if  (in_comment > 0)
			{
				lgth = STlength(delim) + STlength(qt);
				if  (!sq_lgth_chk(lgth))
				{
					return((i4)TK_ENDSTRING);
				}
				break;
			}
			/*
			** b10064
			** Change STcompare() to STbcompare() so case
			** will not be significant when testing for
			** FROM or WHERE.
			*/

			/*
			** b10405 - error on multiple retrieves
			*/
			if  ((qtype == Q_QUEL) &&
			     (STbcompare(qt,0,NAM_RETR,0,TRUE) == 0))
			{
				s_error(0x3AD,NONFATAL,NULL);
				break;
			}

			if  (qtype == Q_SQL)
			{
				if  ((STequal(clname,NAM_TLIST)) && 
				     (STbcompare(qt,0,NAM_FROM,0,TRUE) == 0))
				{
					/*
					** End of target list!  Don't back up
					** to the FROM - we never store that
					** word!
					*/
					_VOID_ s_w_row();
					/*
					** Remove any preceding 'distinct'|'all'
					*/
					STcopy(WRD_BLANK,Cattid);
					return((i4)TK_ALPHA);
				}

				if  (((STequal(clname,NAM_FROM)) &&
				      ((STbcompare(qt,0,WRD_WHERE,
						   0,TRUE) == 0) ||
				       (STbcompare(qt,0,WRD_ORDER,
						   0,TRUE) == 0) ||
				       (STbcompare(qt,0,WRD_GROUP,
						   0,TRUE) == 0) ||
				       (STbcompare(qt,0,WRD_HAVING,
						   0,TRUE) == 0) ||
				       (STbcompare(qt,0,WRD_UNION,
						   0,TRUE) == 0))) ||
				     ((STequal(clname,NAM_WHERE)) &&
				      ((STbcompare(qt,0,WRD_ORDER,
						   0,TRUE) == 0) ||
				       (STbcompare(qt,0,WRD_GROUP,
						   0,TRUE) == 0) ||
				       (STbcompare(qt,0,WRD_HAVING,
						   0,TRUE) == 0) ||
				       (STbcompare(qt,0,WRD_UNION,
						   0,TRUE) == 0))))
				{
					/*
					** End of SQL FROM or WHERE clause
					*/
					_VOID_ s_w_row();
					/*
					** Back up to the ending word -
					** we do store it!
					*/
					Tokchar = save_Tokchar;
					return((i4)TK_ALPHA);
				}
			}

			if  ((STbcompare(qt,0,WRD_SORT,0,TRUE) == 0) ||
			     (STbcompare(qt,0,WRD_ORDER,0,TRUE) == 0))
			{
				if  (St_s_given)
				{
					/*
					** .SORT previously specified
					*/
					s_error(0x39F,NONFATAL,NULL);
				}
				St_s_given = TRUE;
			}

			if  (qtype == Q_SQL)
			{
				/*
				** We know it's SQL, so screen out
				** identifier errors here.  BUT ...
				** we need to let UI_BOGUS_ID through,
				** or we'll choke on functions, AS
				** target specifications, etc.
				*/
				if ((*rtn_char == UI_DELIM_ID) &&
				    (!St_xns_given))
				{
					s_error(0x0A7,NONFATAL,qt,NULL);
				}
			}

			lgth = STlength(delim) + STlength(qt);
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;


		case(TK_ENDFILE):
			if  (level != 0)
			{
				/*
				** Confused parenthesis at EOF
				*/
				return((i4)TK_ENDSTRING);
			}
			/*
			** EOF while processing the query.  Fall into
			** the TK_PERIOD case which handles end-of-query
			** when followed by a new RW statement, so
			**	==> NO BREAK <==
			*/
		case(TK_PERIOD):
			/*
			** If in WHERE or FROM clause, this should end
			** the query
			*/
			if  (((level <= 0) && (in_comment == 0)) &&
			     ((STequal(clname,NAM_REMAINDER)) ||
			      (STequal(clname,NAM_FROM)) ||
			      (STequal(clname,NAM_WHERE))))
			{
				_VOID_ s_w_row();
				return(type);
			}
			/*
			** b6364
			** If this is an SQL target list and we 
			** run into a .<command> they forgot the FROM.
			** FYI a .<number> will not come to this case.
			*/
			if  ((qtype == Q_SQL) && (in_comment == 0) &&
			     (STequal(clname,NAM_TLIST)))
			{
				return(type);
			}
			/*b6364*/
			/*
			** If not, pass through to token routine
			** Not sure how/what can fall through here,
			** especially since we handle 'a.b' in other
			** places.  Anyway,
			**	==> NO BREAK <==
			*/
		default:
			/*
			** Probably a token, but might be start/end of comment.
			** Look for RBF generated WIDTH comment.  Note that we
			** look for a start/end of comment before looking
			** for tokens, since the s_g_token() routine will
			** confuse densely nested comments (bug 47949 et al.)
			*/

			/*
			** Allow comments in the query, i.e. WHERE, FROM,
			** and REMAINDER because RBF stores JoinDef and
			** WIDTH info this way.
			**
			** If in a comment, then keep adding until reach
			** end of comment (in_comment == 0)
			*/
			if  ((type == TK_OTHER) && (*rtn_char == END_COMMENT))
			{
				in_comment--;
				if  (in_comment < 0)
				{
					/* Whoops! */
					return((i4)TK_ENDSTRING);
				}
				/*
				** Reached the end of a comment, which may 
				** have been nested.  Write it out as
				** appropriate.  Note that Tokchar will be
				** pointing to the "*".  So, since the
				** end-of-loop s_g_skip() is usually called
				** with inc = TRUE, only skip the "*", not the
				** "/".
				*/
				CMnext(Tokchar);
				qt = ERx("*/");
				lgth = STlength(delim) + STlength(qt);
				if  (!sq_lgth_chk(lgth))
				{
					return( (i4)TK_ENDSTRING);
				}
				if  ((in_comment == 0) && (skip_comment))
				{
					skip_comment = FALSE;
				}
				break;
			}

			if  ((type == TK_OTHER) && (*rtn_char == BG_COMMENT))
			{
				in_comment++;
				/*
				** Note that Tokchar will be pointing
				** to the "/".  We need to keep in mind that
				** the remaining s_g_skip() are usually called
				** with inc = TRUE, so only unconditionally skip
				** over the "/".  Ditto for s_g_token().
				*/
				CMnext(Tokchar);
				if  (in_comment > 1)
				{
					/*
					** Legitimate WIDTH comment should
					** should NEVER be nested!
					*/
					qt = ERx("/*");
					lgth = STlength(delim) + STlength(qt);
					if  (!sq_lgth_chk(lgth))
					{
						return((i4)TK_ENDSTRING);
					}
					break;
				}

				i = s_g_skip(TRUE,&j);
				if  (j == BG_COMMENT) 
				{
					in_comment++;
					/* 
					** Let immediately nested comment come
					** around again ...
					*/
					qt = ERx("/*/*");
					lgth = STlength(delim) + STlength(qt);
					if  (!sq_lgth_chk(lgth))
					{
						return((i4)TK_ENDSTRING);
					}
					/*
					** Skip over the "/"
					*/
					CMnext(Tokchar);
					break;
				}
				/* 
				** Check if next word after opening
				** comment delimiter is "WIDTH".
				*/
				STcopy(ERx("/*"),tmp_buf);
				save_Tokchar = Tokchar;
				tmp_tok = s_g_token(FALSE);
				STpolycat(3,tmp_buf,WRD_BLANK,tmp_tok,tmp_buf);
				if  (STbcompare(tmp_tok,0,WRD_WIDTH,
						0,TRUE) == 0)
				{
					/*
					** Next "word" should be the 
					** report width
					*/
					i = s_g_skip(TRUE,&j);
					tmp_tok = s_g_token(FALSE);
					STpolycat(3,tmp_buf,WRD_BLANK, 
						  tmp_tok,tmp_buf);
					/*
					** save width in the Ccommand
					** column. (#33894)
					*/
					if  ((CVan(tmp_tok,&i) == OK) &&
					     (i > 0))
					{
						STcopy(tmp_tok,Ccommand);
						/*
						** Force out any pending line
						** since we'll be skipping
						** writes for a while ...
						*/
						if  (count < MAXRTEXT)
						{ 
							_VOID_ s_w_row();
							MEfill((MAXRTEXT + 1),
								EOS,Ctext);
							count = MAXRTEXT;
						}
						CMnext(Tokchar);
						skip_comment = TRUE;
						break;
				      	}
					/*
					** Case of found "bogus" WIDTH.
					** We never want to skip this type
					** of comment, so keep going and let
					** it be written out.  First, though,
					** handle the pathological case of
					** the single word WIDTH immediately!
					*/
					if  (STcompare(tmp_tok,ERx("*/")) == 0)
					{
						in_comment--;
					}
					qt = tmp_buf;
				}
				else
				{
					/*
					** Case of found non-WIDTH.
					** We never want to skip this type
					** of comment, so let it go around
					** again in case it's a nested comment
					** and was confused by s_g_token().
					** Don't forget to keep going, though,
					** so that the opening delimiter is
					** written out.  This is also the one
					** case where we don't want s_g_skip()
					** to pre-increment!
					*/
					Tokchar = save_Tokchar;
					qt = ERx("/*");
					inc = FALSE;
				}
			}
			else
			{
				save_Tokchar = Tokchar;
				qt = s_g_token(FALSE);
			}

			lgth = STlength(delim) + STlength(qt);
			if  (!sq_lgth_chk(lgth))
			{
				return((i4)TK_ENDSTRING);
			}
			break;
		}
		type = s_g_skip(inc,rtn_char);
	}

	/*
	** Should never get this far, and if we put a return here, the
	** ANSI compiler flags it as a warning!
	*/
}





/*
** SQ_LGTH_CHK - Check the lengths of the current token and forming
**		 line.  If it won't fit, write the line to the database
**		 and restart the forming line.  Otherwise, concatenate
**		 the current token to the forming line.
**
** Parameters:
**	lgth		the type of the next token in the line,
**
** Outputs:
**	Updates the static variables
**		count
**		delim (always set to WRD_BLANK)
**	Updates the global variable
**		Ctext
**
** Returns:
**	TRUE	the token has been placed into the forming line.
**	FALSE	the token length exceeds that of the longest possible
**		forming line.
**
** History:
**	23-dec-1992 (rdrane)
**		Extracted from numerous places in in-line code in s_q_add()
**		as part of a re-architect to finally correct comment
**		detection and handling in FROM, WHERE, and REMAINDER sections.
*/

static
bool
sq_lgth_chk(i4 lgth)
{


	/*
	** If we're in the middle of skiping a comment,
	** then just erase the forming line - we don't want
	** to write anything!
	*/
	if  (skip_comment)
	{
		MEfill((MAXRTEXT + 1),EOS,Ctext);
		count = MAXRTEXT;
		delim = WRD_BLANK;
		return(TRUE);
	}

	/*
	** Check if token will fit within current forming line.
	** If not, write the current forming line and start a new
	** forming line.
	*/
	if  (count < lgth)
	{
		_VOID_ s_w_row();
		MEfill((MAXRTEXT + 1),EOS,Ctext);
		count = MAXRTEXT;
		/*
		** Check if token exceeds max length forming line.
		** If so, return failure indication.  Note that this
		** situation can only occur in conjunction with having
		** written out the current forming line!
		*/
		if  (lgth > count)
		{
			return(FALSE);
		}
	}

	STpolycat(3,Ctext,delim,qt,Ctext);
	count -= (STlength(delim) + STlength(qt));

	delim = WRD_BLANK;

	return(TRUE);
}
