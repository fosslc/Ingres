/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<pv.h>
# include	<cm.h>
# include	<qr.h>
# include	<ug.h>
# include	<er.h>
# include	"erqr.h"
# include	<qrhelp.h>
# include       <libq.h>
# include       <adf.h>

/**
** Name:	qrrun.c - QR utility routines
**
** Description:
**
**	Contains QR module routine qrrun() and related routines
**	which "run" the query buffer according to the query language
**	set in the QRB control block structure.
**
**	Public (extern) routines:
**		qrrun(qrb)
**		qrstmt(qrb)
**		dostmt(stmt)
**		doprint(qrb)
**		doruntime(qrb)
**
** History:
**	19-may-1987 (daver)
**		Made Kanji changes. Took out RCS style history mechanism.
**		Provided minimal 'help' support in dohelp().
**	17-jul-1987 (daver)
**		Removed RCS comments, and extracted dohelp() from this
**		into its own file, ala qrretsel.qc and doretsel().
**	26-aug-1987 (daver)
**		took out references to IIdisperr and qrdisperr
**	16-jun-88 (bruceb)
**		Continue to use the QUEL range and retrieve statements
**		for doprint since this command is only viable when
**		run in the QUEL terminal monitor.
**	21-jul-88 (bruceb)	Fix for bug 2177.
**		Updated Kathryn's fix by use of IIUGlbo instead of CVlower.
**      30-sep-89 (teresal)
**              Added PC porting changes.
**	28-sep-89 (teresal)
**		Add support for Runtime INGRES.
**	02/10/90 (dkh) - Added second parameter to qrrun() to pass
**			 back the length of the returned buffer.  This
**			 is part of the tuning changes for TM.
**	26-apr-90 (teresal)
**		TM tuning fix - calculate length using start of output line.
**      05-jun-95 (harpa06) Bug 59265
**              Renumber the statement output based on STATEMENT number. If a 
**              statement exceeds more than one line, indent the statement
**              so that it lines up with the beginning of the statement.
**	18-dec-96 (chech02)
**		Fixed redefined symbols: QRqrb.
**	21-nov-2000 (devjo01) Sir 103291.
**		Allow accurate display of statement numbers greater than 999.
**	7-Nov-2007 (kibro01) b119432
**		Added separate handling of set statements so we can interpret
**		them ourselves before also sending them on to the back-end.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**	26-Aug-2009 (kschendel) 121804
**	    Need qrhelp.h to satisfy gcc 4.3.
**      15-Jan-2010 (maspa05) b123141
**          Changed call to FEset_date_format to FE_apply_date_format and so
**          on. Allows us to get the dependency on libq out of feadfcb.c
**/

GLOBALREF QRB	*QRqrb;

FUNC_EXTERN	i4	qrerror();
FUNC_EXTERN	STATUS	qrtblowner();
FUNC_EXTERN	char	*qrscanstmt();
static VOID	qrstmt();

/*
**	Following functions really return VOID.
**	They are declared i4  only to avoid a known pcc compiler bug
**	involving pointers to functions returning void.
**
**      9/30/89: It is assumed that the above fix is no longer needed.  The
**      functions below are now declared void due to a PC porting change
**      request.
*/
static void     doplain(), doprint(), doruntime();
FUNC_EXTERN	void	doretsel();
FUNC_EXTERN	void	dohelp();
FUNC_EXTERN	void	doset();

static VOID (*qrsd_func[])() = {
	doplain,	/* NOstmt	0	*/
	doplain,	/* PLAINstmt	1	*/
	doretsel,	/* RETSELstmt	2	*/
	doprint,	/* PRINTstmt	3	*/
	dohelp,		/* HELPstmt	4	*/
	doruntime,	/* RUNTIMEstmt	5	*/
	doset		/* SETstmt	6	*/
};

/*{
** Name:	qrrun() - run (or continue) go-block execution.
**
** Description:
**	qrrun() is called to obtain the next output string resulting
**	from execution of the current go-block.
**
**	Setting/saving/restoring LIBQ error/display handlers is done
**	here.
**
** Inputs:
**	QRB	*qrb;		// current QR control block.
**
** Outputs:
**	QRB	*qrb;		// updated QR control block.
**
** Returns:
**	(char *)		// output string
**	NULL			// on end-of-output
**
** Side Effects:
**	IIseterr() called.
**	(*qrb->contfunc)() may be called.
**
** History:
**	8/86 (peterk)	- created
**	9/86 (peterk)	- modification for initial implementation.
**	9/29/86 (peterk) - moved LIBQ err handler setting/restoring
**		here and other cosmetic mods for clarity.
**	12-feb-88 (neil) - Calls IItm_trace to trap trace output (BUG 648)
**	10-apr-89 (kathryn) - Bug #5382
**		Call IIbreak() when setting query language to prevent user
**		errors from being generated when changing languages.
**	27-apr-89 (kathryn) - Bug #5382
**		Changed fix to only call IIbreak when QUEL is the
**		query language.
**	10-may-89 (kathryn) - Bug #5382 - No longer call IIbreak for QUEL
**		A new cleanup routine should be added here, but for now
**		All embedded SELECT statments will be changed to REPEATED
**		SELECT.
*/

char *
qrrun(qrb, outlen)
register QRB	*qrb;
i4	*outlen;
{
	register char	*ret;
	i4		(*IIseterr())();
	void		qrtrace();

#ifdef DEBUG
	qrbdump(qrb);
#endif
	/* make qrb->errfunc the LIBQ error handler */
	qrb->saveerr = IIseterr(qrb->errfunc);

	/* Trap trace messages */
	IItm_trace(qrtrace);

	/* NOTE: LIBQ error functions will be restored before return */


	/* check if there is already output waiting */
	if (qrb->outlin == qrb->outp)
	{
	    /* none waiting -
	    ** reset output buffer ptrs
	    ** and try to generate some more
	    */
	    qrb->outlin = qrb->outp = qrb->outbuf;
	    *qrb->outp = '\0';

	    /* if no statment is in progress, scan input for one */
	    if (qrb->step == 0)
	    {
		/* check for end-of-statement processing */
		/* (indicated by nonnull != 0) */
		if (qrb->nonnull)
		{
		    qrb->nonnull = FALSE;

		    /* execute continue-function if any */
		    if (qrb->contfunc != NULL)
		    {
			/* check status of continue-function */
			if ((*qrb->contfunc)(qrb) != OK)
			{
			    ret = NULL;
			    goto out;
			}
			/* check for output from continue-function */
			if (qrb->outp != qrb->outlin)
			{
			    goto set_ret;
			}
		    }

		    /* otherwise default check for terminating error */
		    else if (qrb->error && qrb->severity)
		    {
			ret = NULL;
			goto out;
		    }
		}

		/* (re-)initialize statement echo buffer */
		qrb->s = qrb->stmt;
		qrb->sfirst = -1;

		/* if DML is SQL, do proper initialization */
		/* don't do this until later
		** if (qrb->lang == DB_SQL)
		**    IIsqInit(NULL);
		*/

		/* scan input til non-null statment is recognized */
		do
		{
		    /*
		    ** if we were already at end of input though,
		    ** just return NULL (qrb->eoi would have been
		    ** set last time time in this fcn.)
		    */
		    if (qrb->eoi)
		    {
			ret = NULL;
			goto out;
		    }
		    else if (qrb->lang == DB_QUEL)
		    {
			/* Bug 5382  - IIbreak used to be called here */
			qrquel(qrb);
		    }
		    else
		    {
			IIsqInit(NULL);
			qrsql(qrb);
		    }

		} while (!qrb->nonnull);

#ifdef DEBUG
		qrbdump(qrb);
#endif
	    }
	    /* end if (qrb->step == 0) */

	    /* execute or continue the statement */
	    qrstmt(qrb);

	    /* save line offset of next statement */
	    qrb->stmtoff = qrb->lines;
	}
	/* end check if there is already output waiting */

set_ret:
	*outlen = qrb->outp - qrb->outlin;
	ret = qrb->outlin;
	qrb->outlin = &ret[*outlen];
	if (qrb->outlin != qrb->outp)
	    qrb->outlin++;

out:
	/* restore LIBQ error handlers before return */
	IIseterr(qrb->saveerr);
	IItm_trace(NULL);

	return ret;
}

 /*{
** Name:	qrstmt - process a query language statement
**
** Description:
**	Performs initialization for statement execution if
**	we are at start of a statement (step == 0).
**	Then simply executes the statement execution function
**	in the statement descriptor for the current statement
**	in the active QRB.
**
** Inputs:
**	QRB *qrb	- Query Runner control block.
**
** Outputs:
**	QRB *qrb	- Query Runner control block (updated).
**
** Returns:
**	VOID
**
** Side effects:
**	A query statement has been initiated (sent to BE) or continued
**	and results from that query have been formatted and added
**	to the output buffer of the active QRB.
**
** History:
**	8/86 (peterk) - created
**	9/86 (peterk) - initialization and echo option code moved
**		here for clarity.
**	20-apr-89 (kathryn) - No longer initialize qrb->severity here.
**		This will be done in [monitor]go.qsc and [fstm]fsrun.qsc
**		User may change via the new [no]continue command.
**	20-apr-1991 (kathryn)  Bug 36909
**		Reset qrb->error to 0 when starting a new statement. 
**		If TM is set to "continue" on errors then previous stmt
**		may have had an error and we need to reset it for the next
**		statement in the go-block.
**	05-jun-1995 (harpa06) Bug 59265
**		Renumber the statement output based on STATEMENT number. If a 
**		statement exceeds more than one line, indent the statement
**		so that it lines up with the beginning of the statement.
**	21-nov-2000 (devjo01) Sir 103291.
**		Allow accurate display of statement numbers greater than 999.
**		
*/

static VOID
qrstmt(qrb)
register QRB	*qrb;
{
	/*
	** if starting a new statement:
	**    initialize error flags
	**    initialize global var for error handler
	**    increment statement no.
	**    handle echo option
	*/
	if (qrb->step == 0)
	{
	    QRqrb = qrb;

	    qrb->error = 0;
	    qrb->sno++;
	    if (qrb->echo)
	    {
		register char	*s;

		s = ERx("%3d> ");
		if ( qrb->sno >= 1000 )
		    s = ERx("%d> ");
                qrprintf(qrb, s, qrb->sno);

		/*
		** FSTM note: the FSTM always runs with echo turned
		** on. This controls printing the statement number,
		** and the database statement before executing that
		** statement. In the past, all QUEL commands were terminated
		** with a "\n". Now, only SQL commands are terminated with
		** a "\n".
		*/
		s = qrb->stmt;
		if (*s == '\n')
			++s;

		while (*s)
		{
		    qrputc(qrb, *s);
		    if (*s == '\n')
			qrprintf(qrb,ERx("     "));
		    *s++;
		}
		qrputc(qrb, (char)EOS);


		/*
		** If we're in the FSTM, we'll want to output the statement
		** before we execute it, thus it'll show up before the error
		** statement. afterwords, lets clear the output buffer so
		** it doesn't get printed again.
		*/
		if (qrb->flushfunc != NULL)
		{
			(*qrb->flushfunc)(qrb->flushparam, qrb->outbuf);
			qrb->outlin = qrb->outp = qrb->outbuf;
			*qrb->outp = '\0';
		}
	    }
	    /* end if (qrb->echo) */

	}
	/* end if (qrb->step == 0) */

	/* invoke statement execution function */
	(*qrsd_func[qrb->stmtdesc->qrsd_type])(qrb);
}

 /*{
** Name:	dostmt - send an ordinary statement to BE
**
** Description:
**	This routine simply sends a (textual) query language
**	statement to the BE if supplied as a string argument,
**	and handles the BE response using standard LIBQ routines.
**
** Inputs:
**	char	*stmt;	// statement string or NULL (if statement
**			   has already been sent to BE.)
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** Side effects:
**	A query statement (not a RETRIEVE/SELECT) may be sent to
**	the BE; pipeblocks are read to handle the response to this or
**	a previously sent query.
**
** History:
**	8-21-86 (peterk) - created
*/

static
VOID
dostmt(stmt)
register	char *stmt;
{
	if (stmt != NULL)
	    IIwritedb(stmt);
	IIsyncup((char *)0, 0);
}

 /*{
** Name:	doplain - send an ordinary statement to BE
**
** Description:
**	Process an ordinary (not a PRINT, HELP, RETRIEVE or SELECT)
**	statement.
**
**	Assumes that the text of the statement has already been
**	sent.  Just calls IIsyncup() directly to handle the response.
**
**	doplain() is called indirectly thru the qrsd_func[] array
**	by qrstmt().
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Outputs:
**	QRB *qrb	- QueryRunner control block (updated).
**
** Returns:
**	VOID
**
** Side effects:
**	Results of a query are read via calls to LIBQ routines.
**	They are formatted and added
**	to the output buffer of the active QRB.
**
** History:
**	8-21-86 (peterk) - created.
*/

static
VOID
doplain(qrb)
register QRB	*qrb;
{
	IIsyncup((char *)0, 0);
}


/*{
** Name:	doprint - send a PRINT statement to BE
**
** Description:
**	Process a PRINT (QUEL) statement.
**
**	Statement is interpreted by executing functionally equivalent
**	RANGE and RETRIEVE statement(s).
**	The RANGE stmt is executed by calling dostmt() and
**	the RETRIEVE by calling doretsel().
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Outputs:
**	QRB *qrb	- QueryRunner control block (updated).
**
** Returns:
**	VOID
**
** Side effects:
**	Queries are sent to the BE, and results are read via
**	calls to LIBQ routines.	 Formatted results are added
**	to the output buffere of the active QRB.
**
** History:
**	8-21-86 (peterk) - created.
**	3-10-89 (kathryn)- Bug #4816
**		Removed statment that disabled the rowcount message.
**		The number of rows retrieved (rowcount) should print for a
**		QUEL PRINT statement. 
**	11-6-89 (teresal) -Bug 7924: Turned off rowcount message if the PRINT
**		statement has a syntax error.
**	20-oct-92 (rogerl) - error message for correct syntax in print fail
**			     has a new number
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

static
VOID
doprint(qrb)
register QRB	*qrb;
{
	register char	*t, *s;
	register bool	incomment = FALSE;
	i4		token;
	char		towner[DB_MAXNAME+1];	/* for qrtblowner */
	char		tablename[DB_MAXNAME+1];
	static char	savec;
	bool		unprintable;
	bool		firstpass;

	/* qrb->norowmsg = TRUE;	Bug 4816 */
	if (qrb->step <= 0)
	{
	    if (qrb->step == 0)
	    {
		s = qrb->stmt;
		firstpass = TRUE;
	    }
	    else
	    {
		qrb->step = 0;
		s = qrb->s;
		firstpass = FALSE;
	    }

	    unprintable = FALSE;
	    do
	    {
		/*
		** if its the first time through, we're not looking
		** at a comma seperated list, but "print tablename".
		** otherwise, it IS a comma seperated list of tablenames.
		*/
		if (firstpass)
		{
			s = qrscanstmt(s, &token, FALSE);
			firstpass = FALSE;
		}
		else
		{
			s = qrscanstmt(s, &token, TRUE);
			if (token < 0)
			{
			    /*
			    ** passing TRUE, and getting token < 0 means
			    ** we expected a ',' as a seperator,
			    ** but didn't get it. null terminate the
			    ** string that we got instead, and
			    ** issue a syntax error.
			    */
			    t = s;
			    while (*s != EOS && !CMwhite(s) && *s != ',')
				CMnext(s);
			    *s = EOS;

			    qrprintf(qrb,
				ERget(F_QR011D_Correct_synt_is),
				t);
			    qrprintf(qrb, ERget(E_QR000B_PRINT_tblname_tblname));
			    qrb->norowmsg = TRUE;	/* Bug 7924 */

			    if (qrb->lang == DB_SQL)
			    {
				qrprintf(qrb,
				    ERget(E_QR000C_end_stmt_with_semicln));
			    }

			    qrb->step = 0;
			    return;
			}
		}

		if (*s == EOS)
		{
		    /* hit EOF on statement */
		    if (token != 0)
		    {
			/*
			** this is where print,\g or print table,\g is
			** entered. if so, issue a syntax error, set step
			** to 0, and return.
			*/
			qrprintf(qrb,
			    ERget(E_QR000D_Syntax_error_on_EOF));
		        qrprintf(qrb, ERget(E_QR000B_PRINT_tblname_tblname));
			qrb->norowmsg = TRUE;		/* Bug 7924 */
		    }
		    /* no more tokens were read,
		    ** i.e. we just skipped any extra white space
		    ** or comments at end of statement --
		    ** just set step to 0 and return.
		    */
		    qrb->step = 0;
		    return;
		}

		/*
		** we're finally out of error conditions!!
		** now we're pointing at the next token
		** which should be the table name;
		*/
		t = s;
		while (*s != EOS && !CMwhite(s) && *s != ',')
		    CMnext(s);
		savec = *s;
		*s = EOS;

		/* save ptr to remainder of PRINT statement */
		qrb->s = s;

		/* For bug 2177 */
		STcopy(t, tablename);
		IIUGlbo_lowerBEobject(tablename);
		if (qrtblowner(qrb, tablename, towner) != OK)
		{
		    unprintable = TRUE;
		    qrprintf(qrb, ERget(F_QR0008_Table_does_not_exist), t);
		    qrb->norowmsg = TRUE;		/* Bug 7924 */
		    *s = savec; /* save the last char!! */
		}
		else
		{
		    unprintable = FALSE;
		}

	    } while (unprintable);

	    qrprintf(qrb, ERget(F_QR0050_t_table_msg), t);

	    /* send the RANGE statement and table name */
	    IIwritedb(ERx("RANGE of iiqr9r1vn0rk="));
	    dostmt(t);
	    *s = savec;

	    /* check for error */
	    if (qrb->error && qrb->severity)
	    {
		/* translation to PRINT error messages is done
		** in qrerror().
		*/
		return;
	    }

	    /* write RETRIEVE query to BE */
	    IIwritedb(ERx("RETRIEVE(iiqr9r1vn0rk.all)"));
	}

	/* execute or continue the RETRIEVE */
	doretsel(qrb);

	/*
	** If this PRINT statement has a remaining comma-separated
	** list of table names, then qrb->s will point to the remainder
	** of that list.
	** Otherwise qrb->s will have been set to NULL.
	**
	** If there is such a list then
	** don't let the statement execution terminate yet.
	*/
	if (qrb->step == 0 && savec != '\0')
	{
	    /* force qrrun() to cycle thru here again */
	    qrb->step = -1;
	}
}

 /*{
** Name:	doruntime - Flag the statement as an error
**
** Description:
**	Process those statements that are not allowed in Runtime INGRES.
**	Flag these statements as errors - treat each disallowed statement as 
**	if it had caused a backend syntax error.
**
**	doruntime() is called indirectly thru the qrsd_func[] array
**	by qrstmt().
**
** Inputs:
**	QRB *qrb	- QueryRunner control block.
**
** Outputs:
**	QRB *qrb	- QueryRunner control block (updated).
**
** Returns:
**	VOID
**
** Side effects:
**	This statement will not be sent to the backend - it will be
**	flagged as an error.
**
** History:
**	9-28-89 (teresal) - created.
*/

static
VOID
doruntime(qrb)
register QRB	*qrb;
{
	qrb->error = 1;
	qrb->norowmsg = TRUE;
	qrprintf(qrb, qrb->errmsg); 
}

/*{
** Name:	doset - Run a SET command, but then send it to plain
**
** Description:
**	Apply a set statement before sending it on to back end
**
**
** Inputs:
**	QRB	*qrb		- static QRB data handle
**
** Side Effects:
**
** Returns:
**	VOID
**
** History:
**	7-Nov-2007 (kibro01) b119432
**		Written
**
*/

static VOID
drop_quotes(char *from, char *tmp)
{
	char *start = from;
	if (*start == '\'')
		start++;
	STcopy(start, tmp);
	start = tmp;
	while (*start && *start != '\'')
		start++;
	*start = '\0';
}

VOID
doset(
    QRB	* qrb
) {
	
	register char	*s = NULL;
	i4		token = 1;	/* Skip the SET */
	ADF_CB *thr_adfcb=IILQasGetThrAdfcb();
	char tmp_format[255];

	s = qrb->stmt;
	/* FALSE indicates this isn't a comma separated list */
	s = qrscanstmt(s, &token, FALSE);
	if (*s != EOS)
	{
	    if ( qrtokcompare( s, ERx( "date_format" ) ) )
	    {
		s = qrscanstmt(s, &token, FALSE);
		if (STlen(s) < sizeof(tmp_format))
		{
		    drop_quotes(s, tmp_format);
		    FEapply_date_format(thr_adfcb,tmp_format);
		}
	    }
	    if ( qrtokcompare( s, ERx( "money_format" ) ) )
	    {
		s = qrscanstmt(s, &token, FALSE);
		if (STlen(s) < sizeof(tmp_format))
		{
		    drop_quotes(s, tmp_format);
		    FEapply_money_format(thr_adfcb,tmp_format);
		}
	    }
	    if ( qrtokcompare( s, ERx( "money_prec" ) ) )
	    {
		s = qrscanstmt(s, &token, FALSE);
		if (STlen(s) < sizeof(tmp_format))
		{
		    drop_quotes(s, tmp_format);
		    FEapply_money_prec(thr_adfcb,tmp_format);
		}
	    }
	    if ( qrtokcompare( s, ERx( "decimal" ) ) )
	    {
		s = qrscanstmt(s, &token, FALSE);
		if (STlen(s) < sizeof(tmp_format))
		{
		    drop_quotes(s, tmp_format);
		    FEapply_decimal(thr_adfcb,tmp_format);
		}
	    }
	    if ( qrtokcompare( s, ERx( "null_string" ) ) )
	    {
		s = qrscanstmt(s, &token, FALSE);
		if (STlen(s) < sizeof(tmp_format))
		{
		    drop_quotes(s, tmp_format);
		    FEapply_null(thr_adfcb,tmp_format);
		}
	    }
	}
	/* And now send it to the back end anyway... */
	doplain(qrb);
}
