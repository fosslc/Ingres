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
# include	<qr.h>
# include	<er.h>
# include	"erqr.h"

/**
** Name:	qrquel.c -- QUEL recognizer routines.
**
** Description:
**	This file contains routines which specifically recognize
**	QUEL statements in a query go-block.
**
**	Public (extern) routines:
**		qrquel(qrb)
**
**	Private (static) routines:
**		quelstart(qrb)
**
** History:
**	13-sep-88 (bruceb)
**		Added 'define link', 'direct connect' and 'direct
**		disconnect' as keywords.
**	26-sep-88 (bruceb)
**		Yet another hack to allow 'index' to follow 'help'
**		without being perceived as start of the next statement.
**		(This means that index command CAN'T follow help
**		command in the same go-block.)
**	10-oct-88 (bruceb)
**		Added keywords 'register' and 'remove'.
**	31-oct-88 (bruceb
**		Allow 'register' to follow 'help' for 'help register'.
**		This means that register command can't follow help
**		command in the same go-block.
**	28-sep-89 (teresal)
**		Added Runtime INGRES.
**      28-nov-89 (teresal)
**              Referenced Runtime_INGRES global here instead of in qr.h
**              because of VMS build problems with Report Writer (which
**              includes qr.h).
**	20-apr-92 (seg)
**		Must declare qrgettok to be extern.
**	11-aug-2009 (maspa05) trac 397, SIR 122324
**	    in silent mode and \script don't output query text
**/

/* GLOBALDEF's */
GLOBALREF bool  Runtime_INGRES;
GLOBALREF bool  TrulySilent;

static char	*Begin_End_2[] = {
	ERx("transaction"),
	NULL
};

static char	*Define_2[] = {
	ERx("integrity"),
	ERx("link"),
	ERx("location"),
	ERx("permit"),
	ERx("view"),
	NULL
};

static char	*Direct_2[] = {
	ERx("connect"),
	ERx("disconnect"),
	NULL
};

static QRSD	Kw_Quel[] = {
	ERx("abort"),		PLAINstmt,	NULL,
	ERx("append"),		PLAINstmt,	NULL,
	ERx("begin"),		PLAINstmt,	Begin_End_2,
	ERx("copy"),		PLAINstmt,	NULL,
	ERx("create"),		PLAINstmt,	NULL,
	ERx("define"),		PLAINstmt,	Define_2,
	ERx("delete"),		PLAINstmt,	NULL,
	ERx("destroy"),		PLAINstmt,	NULL,
	ERx("direct"),		PLAINstmt,	Direct_2,
	ERx("end"),		PLAINstmt,	Begin_End_2,
	ERx("help"),		HELPstmt,	NULL,
	ERx("index"),		PLAINstmt,	NULL,
	ERx("modify"),		PLAINstmt,	NULL,
	ERx("print"),		PRINTstmt,	NULL,
	ERx("range"),		PLAINstmt,	NULL,
	ERx("register"),	PLAINstmt,	NULL,
	ERx("relocate"),	PLAINstmt,	NULL,
	ERx("remove"),		PLAINstmt,	NULL,
	ERx("replace"),		PLAINstmt,	NULL,
	ERx("retrieve"),	RETSELstmt,	NULL,
	ERx("save"),		PLAINstmt,	NULL,
	ERx("savepoint"),	PLAINstmt,	NULL,
	ERx("set"),		PLAINstmt,	NULL,
	0,	0,	NULL,
};
static QRSD	qrsd_RETINTO = {
	ERx("retrieve into"),	PLAINstmt,	NULL,
};
static QRSD	qrsd_RETRIEVE = {
	ERx("retrieve"),	RETSELstmt,	NULL,
};
static QRSD	runt_RETINTO = {
	ERx("retrieve into"),	RUNTIMEstmt,	NULL,
};
static QRSD	runt_CREATE = {
	ERx("create"),	RUNTIMEstmt,	NULL,
};

static bool	quelstart();

FUNC_EXTERN bool qrgettok();

/*{
** Name:	qrquel(qrb) - scan for next QUEL stmt in go-block
**
** Description:
**	This routine scans for the next QUEL statement in the
**	query go-block.  Upon return the buffer qrb->stmt will
**	contain the text of the statement.  Other flags indicate
** 	whether the statement text is non-null (.nonnull), the
**	statement descriptor (.stmtdesc), statement number (.sno) and
**	line offset of the start of the statement in go-block
**	(.stmtoff).
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
**	17-feb-1988 (neil) Fixed bug 1547 - script lookahead.
**	10-jan-1991 (kathryn) Bug 20623 21260 35098
**		Add calls to IIQRscript() - The new routine for handling the
**		the scripting of user input. Stmt buffer will now be flushed to
**		script file whenever end of statement or end of input is found.
*/

VOID
qrquel(qrb)
register QRB	*qrb;
{
	bool	help_stmt = FALSE;
	char	token2[QRTOKENSIZ];

	/* move token read previously into stmt buffer */
	qrputtok(qrb);

	/* check for RETRIEVE INTO case */
	if (qrb->nextdesc->qrsd_type == RETSELstmt)
	{
	    /* if next token is neither '(' nor UNIQUE this is RETRIEVE INTO */
	    if (qrgettok(qrb, TRUE) && *qrb->token != '(' &&
		STbcompare(qrb->token, 0, ERx("unique"), 0, TRUE) != 0)
		qrb->nextdesc = &qrsd_RETINTO;
	}

	/* check for Runtime INGRES RETRIEVE INTO case */
	else if (qrb->nextdesc->qrsd_type == RUNTIMEstmt &&
		 (STbcompare(qrb->token, 0, ERx("retrieve"), 0, TRUE) == 0))
	{
	    /* if next token is into then this is RETRIEVE INTO */
	    if (qrgettok(qrb, FALSE) &&
		STbcompare(qrb->token, 0, ERx("into"), 0, TRUE) == 0)
	    {
	        qrb->nextdesc = &runt_RETINTO;	/* it's RETRIEVE INTO */
	        qrb->errmsg = ERget(E_QR002C_no_creat_table);
	    }
	    else
	    {
	    /* It's Runtime INGRES, but it is not "RETRIEVE INTO" - put
	    ** "RETRIEVE" and the current token into the QRB so they will
	    ** get sent to the backend. 
	    */
		qrb->nextdesc = &qrsd_RETRIEVE; /* it's a 'regular' retrieve */
		STcopy(qrb->token, token2);
		STcopy(ERx("retrieve "), qrb->token);
		qrb->t = &qrb->token[STlength(qrb->token)];
		qrputtok(qrb);			/* add 'retrieve' to QRB */
		STcopy(token2, qrb->token);
		qrb->t = &qrb->token[STlength(qrb->token)];
		qrputtok(qrb);			/* add second token to QRB */
	    }		

	}

	/* check for special DEFINE PERMIT case */
	else if (STbcompare(ERx("define permit"), 0, qrb->token, 0, TRUE) == 0)
	{
		/* read past oplist to avoid mis-recognition as
		** statement starters
		*/
		while (qrgettok(qrb, TRUE) && qrgettok(qrb, TRUE) &&
			*qrb->token == ',')
		    ;
	}

	/* check for special MODIFY table TO RELOCATE case */
	else if (STbcompare(ERx("modify"), 0, qrb->token, 0, TRUE) == 0)
	{
		/* read past "table TO name" to avoid mis-recognition as
		** statement starters
		*/
		if (   qrgettok(qrb, TRUE)	/* table */
		    && qrgettok(qrb, TRUE)	/* TO 	 */
		    && qrgettok(qrb, TRUE))	/* name  */
		    ;
	}

	/* catch 'help index' and 'help register' */
	else if (STbcompare(ERx("help"), 0, qrb->token, 0, TRUE) == 0)
	{
		help_stmt = TRUE;
	}

	while (qrgettok(qrb, FALSE))
	{
	    if ((help_stmt)
		 && ((STbcompare(ERx("index"), 0, qrb->token, 0, TRUE) == 0)
		      || (STbcompare(ERx("register"), 0, qrb->token, 0, TRUE)
			      == 0)))
	    {
		/* overhead of STbcompare will only fall on help command */
	    }
	    else if (quelstart(qrb))
	    {	
		/*
		** The token just read will be start of next statement.
		** It will be moved into the stmt buffer on the next
		** call of qrquel(). 
		*/
		if (qrb->script && qrb->s && !TrulySilent)
			IIQRscript(qrb);
		return;
	    }

	    help_stmt = FALSE;

	    /* move last token read into stmt buffer */
	    qrputtok(qrb);
	}

	/* End of input - "\g" - Flush the statement to script file. */
	if (qrb->script && !TrulySilent)
		IIQRscript(qrb);

	qrb->stmtdesc = qrb->nextdesc;
}


/*
**	quelstart(qrb)
**
**	Test for keywords which start QUEL statements and
**	set appropriate member fields in QRB with statement
**	descriptor from QRSD array.
*/
static bool
quelstart(qrb)
register QRB	*qrb;
{
	register QRSD	*k;

	if (Runtime_INGRES)
	{
		if (STbcompare (ERx("create"), 0, qrb->token, 0, TRUE) == 0)
	        {
			k = &runt_CREATE;
			qrb->errmsg = ERget(E_QR002C_no_creat_table);
			goto ret_true;
		}
		else if (STbcompare (ERx("retrieve"), 0, qrb->token, 0, TRUE) == 0)
	        {
			k = &runt_RETINTO;  /* Assume RETRIEVE INTO for now */
			goto ret_true;
		}
	}
	for (k = Kw_Quel; k->qrsd_key != NULL; k++)
	{
		if (STbcompare(k->qrsd_key, 0, qrb->token, 0, TRUE) == 0)
		{
		    /*
		    ** Handle double keywords, e.g. BEGIN TRANSACTION, etc.
		    */
		    if (k->qrsd_follow != NULL)
		    {
			char	token2[QRTOKENSIZ];

			/* re-init token buffer */
			qrb->t = qrb->token;

			if (qrgettok(qrb, FALSE))
			{
			    char	**k2;
			    for (k2 = k->qrsd_follow; *k2; k2++)
			    {
				if (STbcompare(qrb->token, 0, *k2, 0, TRUE) == 0)
				{
				    /* yes, it's a real double keyword --
				    ** save the whole thing in the token buffer
				    */
				    STcopy(qrb->token, token2);
				    STprintf(qrb->token, ERx("%s %s"),
					     k->qrsd_key, token2);
				    qrb->t = &qrb->token[STlength(qrb->token)];

				    goto ret_true;
				}
			    }
			    /* 2nd token is not in the qrsd_follow list */
			}

			/* no, not a double keyword --
			** put out the 1st token now and call
			** quelstart recursively to check if the 
			** 2nd token was a statement starter
			*/
			STcopy(qrb->token, token2);
			STcopy(k->qrsd_key, qrb->token);
			STcat(qrb->token, ERx(" "));
			qrb->t = &qrb->token[STlength(qrb->token)];
			qrputtok(qrb);
			STcopy(token2, qrb->token);
			qrb->t = &qrb->token[STlength(qrb->token)];
			return quelstart(qrb);
		    }

ret_true:
		    qrb->stmtdesc = qrb->nextdesc;
		    qrb->nextdesc = k;
		    return TRUE;
		}
	}

	return FALSE;
}
