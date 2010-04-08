/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<qr.h>
# include	<er.h>
# include	"erqr.h"

/**
** Name:	qrsql.c -- SQL recognizer routines.
**
** Description:
**	This file contains routines which specifically recognize
**	SQL statements in a query go-block.
**
**	Public (extern) routines:
**		qrsql(qrb)
**
**	Private (static) routines:
**		sqlstart(qrb)
**		sqlspecial(qrb)
**		sqlcreateproc	- Check start and end of CREATE PROCEDURE
**
** History:
**	12-feb-1988	SQL ALWAYS requires a semicolon.  If I had the time
**			this qr scanner should be rewritten.  All checks for
**			qrb->nosemi are assumed to be "SQL has semicolons" 
**			so act as though nosemi = FALSE always.
**			Removed sqlspecial as is unused now. (ncg)
**	18-may-1988	Added sqlcreateproc to allow the scanning of the
**			CREATE PROCEDURE statement without terminating on
**			nested semicolons. (ncg)
**	12-oct-88 (bruceb)
**		Use STbcompare to check against 'help' rather than STcompare.
**	19-oct-88 (bruceb)
**		Maintain static 'begin_stmt' to determine when I should
**		check for 'help' and set nobesend.  Otherwise, always
**		sets nobesend when not a ';' and would always trigger off
**		of the word 'help' regardless of context.
**	4-oct-89 (teresal)
**		Added Runtime INGRES.
**	7-nov-89 (teresal)
**		Fix for bug 7890.  Modified sqlstart() to look for 
**		closer match for "HELP" keyword.  Before, it would match
**		token "HELPjunkhjhjhj" to "HELP" keyword.
**	28-nov-89 (teresal)
**		Referenced/defined Runtime_INGRES global here instead of in qr.h
**		because of VMS build problems with Report Writer (which 
**		includes qr.h). 
**	10-aug-90 (teresal)
**		Modified sqlstart() to accept a non-white space after a
**		SQL keyword like ("SELECT*"). Bug 32455.
**	20-apr-92 (seg)
**		Must declare qrgettok to be extern.
**	18-dec-96 (chech02)
**		Fixed redefined symbols: Runtime_INGRES.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	1-feb-06 (dougi)
**	    Add "with" as potential starting keyword for select statement
**	    (to support query with clauses).
**	7-nov-07 (kibro01) b119432
**	    Add separate "set" option to handle set statements prior to
**	    sending them to the back-end.
**	11-aug-2009 (maspa05) trac 397, SIR 122324
**	    in silent mode and \script don't output query text
**/
/* GLOBALDEF's */
GLOBALREF bool  Runtime_INGRES;	/* TRUE - running Runtime INGRES Installation */
GLOBALREF bool  TrulySilent;	

/*
** This table was modified to only look for the special words that
** do not go through PLAINstmt or NOstmt (which are equivalent).
*/
static QRSD	Kw_SQL[] = {
	ERx("help"),		HELPstmt,	NULL,		/* 0 */
	ERx("select"),		RETSELstmt,	NULL,		/* 1 */
	ERx("with"),		RETSELstmt,	NULL,		/* 2 */
	ERx("set"),		SETstmt,	NULL,		/* 3 */
# define	Kw_RETSEL					   1
	0,	0,	NULL,
};

/*
** Statement description for commands (CREATE and DROP) that are not available
** when running Runtime INGRES.
*/
static QRSD	Kw_Runtime = {
	ERx(" "), 		RUNTIMEstmt,	NULL,
};
/*
** Used in sqlstart and sqlruntime to determine if the last statement
** is a command not available in Runtime INGRES.
*/
static bool	runtime_stmt = FALSE;	
/*
** Used in sqlstart to determine if this is an appropriate time to check
** whether in a help statement (and thus, whether nobesend).
*/
static	bool	begin_stmt = TRUE;

bool sqlstart();
bool sqlcreateproc();
bool sqlruntime();

FUNC_EXTERN bool qrgettok();

/*{
** Name:	qrsql(qrb) - scan for next SQL stmt in go-block
**
** Description:
**	This routine scans for the next SQL statement in the
**	query go-block.  Upon return the buffer qrb->stmt will
**	contain the text of the statement.  Other flags indicate
** 	whwther the statement text is non-null (.nonnull), the
**	statement type (.stmtdesc), statement number (.sno) and
**	line offset of the start of the statement in go-block
**	(.stmtOff).
**
** Inputs:
**	QRB	*qrb;	-- Query Runner control Block.
**
** Outputs:
**	QRB	*qrb;	-- Query Runner control Block (updated).
**
** Returns:
**	VOID
**
** Side Effects:
**	qrb argument updated to reflect current state of go-block.
**
** History:
**	7/86 (peterk) created
**	11-feb-88 (embarrassed to say who) Fixed bug 1660.
**	10-jan-1991 (kathryn) Bug 20623 21260 35098
**              Add calls to IIQRscript() - The new routine for handling the
**              the scripting of user input. Stmt buffer will now be flushed to
**              script file whenever end of statement or end of input is found.
**	19-may-1995 (harpa06)
**		Cross integration fix for bug 68269 by kch: Under ISQL when you
**		insert into a table containing a varchar column, if the 98th 
**		character of the string insterted is a semicolon (;), you
**		will get error message E_US0A8C.
**	20-aug-01 (inkdo01)
**	    Init qrb->casecnt to allow case funcs (and corr "end"s) to be 
**	    contained in CREATE PROCEDURE.
*/

VOID
qrsql(qrb)
register QRB	*qrb;
{
	bool	first_token = TRUE;
	bool	create_proc = FALSE;

	/* move token read previously into stmt buffer */

	while (qrgettok(qrb, FALSE))
	{
	    if (!create_proc && sqlstart(qrb)) 
	    {
		/* We have a semicolon so we are at end of statement
		** - flush statement to the script file.
		*/

		if (qrb->script && !TrulySilent)
			IIQRscript(qrb);

		/* as semicolon separators are used, clear the semicolon from
		** the token buffer so it won't be sent to the DBMS
		*/

		qrb->t = qrb->token;
		*qrb->token = '\0';

		/*
		** HOWEVER, the echo code in qrstmt relies on the fact
		** that quel statements were terminated by newlines.
		** So, terminate this statement with a newline.
		**
		** BUG 1660: If just semicolons were given then qrgettok
		**	     and sqlstart return TRUE but qrb->s is still null.
		** Make sure we do not copy into null buffer.
		*/
		if (qrb->s)
		    STcopy(ERx("\n"), qrb->s);

		/*
		** Otherwise if there are NOT semicolon separators
		** the token just read will be start of next statement.
		** It will be moved into the stmt buffer on the next
		** call of qrsql().
		**
		** NOTE: as of 9/87, semicolons ARE required in SQL.
		** so !qrb->nosemi will be TRUE in SQL, FALSE in QUEL.
		*/
		return;
	    }

	    /* move last token read into stmt buffer */
	    qrputtok(qrb);

	    /*
	    ** if semicolons check for special cases here
	    ** This code can be removed? 12-feb-1988 (ncg)
	    sqlspecial(qrb);
	    */

	    /*
	    ** Check if we need to enter a procedure, or if already
	    ** entered, then check if we need to exit.
	    */
	    if (first_token)
	    {
		qrb->casecnt = 0;
		create_proc = sqlcreateproc(TRUE, qrb);
	    }
	    else if (create_proc)
		create_proc = sqlcreateproc(FALSE, qrb);
	    first_token = FALSE;
	}
	if (qrb->script&&!TrulySilent)
		IIQRscript(qrb);

	/* here at EOI in query buffer */
	/* call sqlstart to determine statement type */
	STcopy(ERx(";"), qrb->token);
	sqlstart(qrb);
}


/*
**	sqlstart(qrb)
**
**	Test for keywords which start SQL statements and
**	set appropriate flags in QRB.
**
**	Disclaimer: This is how I think this works (neil)
**	When we reach a semicolon we go back to look at the first real
**	word of the current statement - this is marked by
**		qrb->stmt + qrb->sfirst
**	This word should identify the type of statement.
*/
bool
sqlstart(qrb)
register QRB	*qrb;
{
	register QRSD	*k = Kw_SQL;
	extern QRSD	noStmtDesc;
	i4		k_len;
	char		*s;

	if (*qrb->token == ';' && STlength(qrb->token) == 1)
	{
	    begin_stmt = TRUE;
	    qrb->nobesend = FALSE;
	    qrb->stmtdesc = &noStmtDesc;

	    /*
	    ** don't do this if there isn't a statement to be
	    ** processed: this happends if we hit \g immediately.
	    ** return true to signify that we do have a statement,
	    ** which is nothing, and send the nothing to the backend.
	    */
	    if (qrb->stmt == NULL)
		return TRUE;

	    /*
	    ** Check to see if the previous statement has been flaged as
	    ** not available in a Runtime INGRES environment.
	    */
	    if (runtime_stmt)
	    {
		qrb->stmtdesc = &Kw_Runtime;
		runtime_stmt = FALSE;
		return TRUE;
	    }

	    for (;k->qrsd_key != NULL; k++)
	    {
		k_len = STlength(k->qrsd_key);
		if (STbcompare(k->qrsd_key, k_len, qrb->stmt+qrb->sfirst,
			0, TRUE) == 0)
		{
		/* Bug 7890 - Allow "HELP", "HELP ", "HELP\n", but not
		** "HELPjunk". Make sure the next character after the
		** keyword is a white space or end of string.  Only check
		** this for the HELP command since "SELECT*" is ok.
		*/
		    s = qrb->stmt+qrb->sfirst+k_len;
		    if (k->qrsd_type != HELPstmt || CMwhite(s) || *s == '\0')
		    {
		        qrb->stmtdesc = k;
		        break;
		    }
		}
	    }
	    /*
	    ** BUG 872 - Allow '(... select...)' as well. (ncg)
	    **		 Skip left parens and following blanks.
	    ** Then compare the word to "select" - if we have a
	    ** match then point at the correct descriptor.
	    */
	    if (*(qrb->stmt + qrb->sfirst) == '(')
	    {
		char	*cp;

		for (cp = qrb->stmt + qrb->sfirst; 
		     *cp == '(' || CMwhite(cp); CMnext(cp))
			;
		if (STbcompare(ERx("select"), 0, cp, 6, TRUE) == 0)
		    qrb->stmtdesc = &Kw_SQL[Kw_RETSEL];
	    }
	    return TRUE;
	}
	else
	{
	    /*
	    ** Check for help. Set the flag if it is help.
	    */
	    if (begin_stmt)
	    {
		begin_stmt = FALSE;
		qrb->nobesend
		    = (STbcompare(qrb->token, 0, ERx("help"), 0, TRUE) == 0);

		if (Runtime_INGRES)
		    return sqlruntime(qrb);	
	    }
	}
	return FALSE;
}


/*{
** Name: sqlcreateproc - scan for start or end of CREATE PROCEDURE
**
** Description:
**	This routine scans the current tokens for the start and end of
**	CREATE PROCEDURE.  The template for such a block is:
**
**		[CREATE] PROCEDURE name ....
**			[declare]
**		BEGIN | '{'
**			[ statement {; statement } [;]]
**		END | '}'
**
**	When searching for 'start' this routine will check for:
**
**		[CREATE] PROCEDURE 
**
**	and return TRUE if it can find this clause, else it will return
**	FALSE.  If returning TRUE this will let the caller know to
**	suppress the searching for semicolons AND that it is now in a
**	procedure.
**
**	If the caller was able to get into the procedure, then it will
**	find out when it has ended.  When not searching for 'start' this
**	routine will check for:
**
**		'}' | END
**
**	and return TRUE, if it does NOT find the end (leaving the caller
**	still within the procedure) or FALSE if it finds the end, leaving
**	the caller in regular statement mode.
**
** Inputs:
**	start	bool	-- Search for start or end of procedure.
**	QRB	*qrb;	-- Query Runner control Block.
**	
** Outputs:
**	QRB	*qrb;	-- Query Runner control Block (updated).
**
** Returns:
**	bool		-- if 'start' then TRUE if found, else FALSE
**			       if not found CREATE PROCEDURE.
**			   if not 'start' (then was earlier found) and
**				will return FALSE if found end, else
**				TRUE to remain in procedure.
**
** Side Effects:
**	qrb argument updated to reflect current state of go-block.
**
** History:
**	18-may-1988 (neil) created for road runner.
**	20-aug-01 (inkdo01)
**	    Track nested case funcs so "end"s don't screw up syntax buff. 
*/

bool
sqlcreateproc(start, qrb)
bool	start;
QRB	*qrb;
{
    if (start)		/* Search for [CREATE] PROCEDURE */
    {
	if (   STbcompare(ERx("procedure"), 0, qrb->token, 0, TRUE) == 0
	    || (   STbcompare(ERx("create"), 0, qrb->token, 0, TRUE) == 0
		&& qrgettok(qrb, TRUE)
		&& STbcompare(ERx("procedure"), 0, qrb->token, 0, TRUE) == 0
	       )
	   )
	{
	    return TRUE; 	/* Got into procedure */
	}
	else
	{
	    return FALSE;	/* Did not get in */
	}
    }
    else		/* Search for } or END */
    {
	if (   *qrb->token == '}'
	    || (STbcompare(ERx("end"), 0, qrb->token, 0, TRUE) == 0 &&
		qrb->casecnt == 0))
	{
	    return FALSE;	/* Got out of procedure */
	}
	else
	{
	    if (STbcompare(ERx("end"), 0, qrb->token, 0, TRUE) == 0)
		qrb->casecnt--;
	    else if (STbcompare(ERx("case"), 0, qrb->token, 0, TRUE) == 0)
		qrb->casecnt++;
	    return TRUE;	/* Still in procedure */
	}
    } /* If start or end */
} /* sqlcreateproc */

/*{
** Name: sqlruntime - scan for all commands not available in Runtime INGRES
**
** Description:
**	This routine scans the tokens at the beginning of a statement
**	looking for commands that are not available in Runtime INGRES.  Runtime
**	INGRES does not allow tables to be created or destroyed; therefore,
**	the following statements are illegal under Runtime INGRES:
**	
**		CREATE TABLE tablename ...
**		DROP TABLE tablename ...
**		DROP objectname ...
**
**	If any of the above statements are found, an error message is set
**	in the QRB, and that statement is not sent to the backend (nobesend is
**	set to TRUE).
**
**	This routine sometimes has to 'look ahead' a few tokens to determine
**	what type of statement we have.  This routine will return TRUE is it
**	has stumbled upon a ';' otherwise, it will return FALSE.
**		
** Inputs:
**	QRB	*qrb;	-- Query Runner control Block.
**	
** Outputs:
**	QRB	*qrb;	-- Query Runner control Block (possibly updated).
**
** Returns:
**	bool	TRUE	- If ';' is found.  Note ';' is processed by calling
**			  sqlstart() recursively.
**		FALSE 	- If ';' is not found.
**
** Side Effects:
**	Errmsg and nobesend in qrb are modified if the statement is illegal in
**	Runtime INGRES.
**
** History:
**	04-oct-1989 (teresal) Written.
*/

bool
sqlruntime(qrb)
QRB	*qrb;
{
    char	token1[QRTOKENSIZ];
    char	token2[QRTOKENSIZ];
    char	tokensave[QRTOKENSIZ];
    bool	semi_found = FALSE;

    if (STbcompare(ERx("create"), 0, qrb->token, 0, TRUE) == 0 ||
    	STbcompare(ERx("drop"), 0, qrb->token, 0, TRUE) == 0)
    {
	STcopy(qrb->token, token1);	/* save the first token */
	qrb->t = qrb->token;		/* re-init token pointer */
	if (qrgettok(qrb, FALSE))	/* look ahead at the next token */
	{
	    if (STbcompare(ERx("table"), 0, qrb->token, 0, TRUE) == 0)	
	    {
		runtime_stmt = TRUE;	/* Found CREATE TABLE or DROP TABLE */
		qrb->errmsg = ERget(E_QR002B_no_creat_drop);
		qrb->nobesend = TRUE;
	    }		
	    else if (STbcompare(ERx(";"), 0, qrb->token, 0, TRUE) == 0)
	    {
		semi_found = TRUE;
	    }

	    /*
	    ** Determine if we have a "DROP objectname {,objectname};"
	    ** statement.  Look at the third token - if it is ';' ',' or
	    ** EOF, then it's DROP objectname.
	    */
	    else if (STbcompare(ERx("drop"), 0, token1, 0, TRUE) == 0)
	    {
		STcopy(qrb->token, token2);
		qrb->t = qrb->token;		/* re-init token pointer */
		if (qrgettok(qrb, FALSE))	/* look ahead another token */
		{
	            if (STbcompare(ERx(";"), 0, qrb->token, 0, TRUE) == 0)
		    {
			runtime_stmt = TRUE;	/* Found "DROP objectname;" */
		    	qrb->errmsg = ERget(E_QR002D_no_drop_object);
	            	qrb->nobesend = TRUE;
			semi_found = TRUE;
		    }
	            else if (STbcompare(ERx(","), 0, qrb->token, 0, TRUE) == 0)
		    {
			runtime_stmt = TRUE;	/* Found "DROP objectname," */
		    	qrb->errmsg = ERget(E_QR002D_no_drop_object);
	            	qrb->nobesend = TRUE;
		    }
		}
		else
		{
		    runtime_stmt = TRUE;	/* Found "DROP objectname" */
		    qrb->errmsg = ERget(E_QR002D_no_drop_object);
	            qrb->nobesend = TRUE;
		    STcopy(ERx(";"), qrb->token); /* Dummy a terminator */
		    semi_found = TRUE;
		}
		/*
		** Put back the first token into QRB so legal DROP statements
		** get sent to the backend.
		*/
		STcopy(qrb->token, tokensave);
		STcopy(token1, qrb->token);
		STcat(qrb->token, ERx(" "));
		qrb->t = &qrb->token[STlength(qrb->token)];
		qrputtok(qrb);
		STcopy(tokensave, qrb->token);
		STcopy(token2, token1);
	    } /* end if DROP statement */
	} /* end if qrgettok */
	else	/* Found end of input */
	{
		STcopy(ERx(";"), qrb->token); /* Dummy a terminator */
		semi_found = TRUE;
	}
	/*
	**  Put back the previous token into QRB so legal CREATE or
	**  DROP statements get sent to the backend.
	*/
	STcopy(qrb->token, tokensave);
	STcopy(token1, qrb->token);
	STcat(qrb->token, ERx(" "));
	qrb->t = &qrb->token[STlength(qrb->token)];
	qrputtok(qrb);
	STcopy(tokensave, qrb->token);
	qrb->t = &qrb->token[STlength(qrb->token)];
	if (semi_found)
	    return sqlstart(qrb);
    } /* A CREATE or DROP statement was not found */
    return FALSE;
} /* sqlruntime */
/*
**	sqlspecial(qrb)
**
**	Check for special cases where SQL statement keywords
**	can appear inside other statements.
**
** History:
**	05-feb-1988	BUG 179 - Always check for early semicolons. (ncg)
**	12-feb-1988	This routine is not used anymore (ncg)
*/
#ifdef SCANNER_HACK

sqlspecial(qrb)
QRB	*qrb;
{
	if (STbcompare(ERx("create"), 0, qrb->token, 0, TRUE) == 0)
	{
	    if (!qrgettok(qrb, TRUE)) return;
	    if (STbcompare(ERx("permit"), 0, qrb->token, 0, TRUE) == 0)
	    {
		while (   qrgettok(qrb, TRUE)
		       && CMnmstart(qrb->token)	/* First token is word */
		       && qrgettok(qrb, TRUE)
		       && *qrb->token == ',')	/* Second is a comma */
		    ;
	    }
	}
	else if (STbcompare(ERx("insert"), 0, qrb->token, 0, TRUE) == 0)
	{
	    register int i = 3;
	    while (i--)
	    {
		if (!qrgettok(qrb, TRUE) || *qrb->token == ';')
		    break;
	    }
	    if (*qrb->token == '(')
	    {
		while (   qrgettok(qrb, TRUE)
		       && *qrb->token != ')'
		       && *qrb->token != ';')
		    ;
		if (*qrb->token != ';')
		    qrgettok(qrb, TRUE);
	    }
	}
}
#endif /* SCANNER_HACK */
