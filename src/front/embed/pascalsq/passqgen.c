/*
** Copyright 1985, 1992, 2001 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<nm.h>
# include	<equel.h>
# include	<eqesql.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<ereq.h>
# include	<ere2.h>
# include 	<eqpas.h>

/*
+* Filename:	passqgen.c
** Purpose:	Defines ESQL dependent routines and tables for PASCAL code 
**		generation.
** Defines:
**		gen_sqlca	- Generate an SQLCA if-then-else statement.
**		gen_sqarg	- Add "sqlca" to arg list.
**              gen_sqlcode     - Add "&SQLCODE" to arg list.
**              gen_diaginit    - Generate Diagnostic Area init call.
**		gen__sqltab	- ESQL-specific gen table.
**
** Externals Used:
-*		esqlca_store[]	- Array of three WHENEVER states.
**
** Notes:
**	Interface layer on top of the EQUEL gen file.
** 
** History:
**		15-aug-1985	- Written for C (ncg, mrw)
**		22-nov-1985	- Modified for PL/I (ncg)
**		01-apr-1986	- Modified for ADA (ncg)
**		01-apr-1986	- Modified for PASCAL (mrw)
**		11-may-1987	- Modified for 6.0 (ncg)
**              24-apr-1991     - Update for alerters to generate a
**                                call to IILQesEvStat when
**                                WHENEVER SQLEVENT is generated. 
**				  Also, update for alerters in
**			          general. (teresal)
**		29-apr-1991	- Fixed alerter enhancement.
**      16-dec-1992 (larrym)
**              Added IIsqlcdInit to gen__sqltab.  This is called
**              instead of IIsqInit if the user is using SQLCODE.  Added
**              function gen_sqlcode, which is called by esq_init, to
**              generate arguments for the IIsqlcdInit call.  Also
**              added function IIsqGdInit to gen__sqltab to initialize the
**              ANSI SQL Diagnositc Area.  Added gen_diaginit, which is called
**              by esq_init, to generate arguments to the IIsqGdInit call.
**	11-feb-1993 (larrym) 
**	    removed IIsqGdInit from gen__sqltab; not needed anymore.  Now
**	    calling gen_diaginit only (not gen_call) which generates 
**	    literal function call.
**      08-mar-1993 (larrym)
**          added new function IILQcnConName (IISQCNNM) for connection names.
**      08-30-93 (huffman)
**              add <st.h>
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	6-Jun-2006 (kschendel)
**	    Added describe input.
*/

static bool	gen_sqdeclared = FALSE;

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For PASCAL there is very
** little argument information needed, as will probably be the case for
** regular callable languages. 
**
** This is the ESQL-specific part of the table
*/

i4	gen_var_args(),
	gen_io(),
	gen_all_args(),
	gen_sqarg(),
	gen_IIdebug(),
	gen__II();
void	gen__obj(),
	gen_sqlcode(),
	gen_diaginit();

GLOBALDEF GEN_IIFUNC	gen__sqltab[] = {
  /* Name */			/* Function */	/* Flags */	/* Args */
    { ERx("IIf_"),		0,		0,		0 },

  /* Cursors # 1 */
    { ERx("IIcsDaGet"),		gen_all_args,	0,		2 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },

  /* SQL routines # 6 */
    { ERx("IIsqConnect"),	gen_var_args,	0,		15 },
    { ERx("IIsqDisconnect"),	0,		0,		0 },
    { ERx("IIsqFlush"),		gen_IIdebug,	0,		0 },
    { ERx("IIsqInit"),		gen_all_args,	0,		1 },
    { ERx("IIsqStop"),		gen_all_args,	0,		1 },
    { ERx("IIsqUser"),		gen_all_args,	0,		1 },
    { ERx("IIsqPrint"),		gen_all_args,	0,		1 },
    { ERx("IIsqMods"),		gen_all_args,	0,		1 },
    { ERx("IIsqParms"),		gen_all_args,	0,		5 },
    { ERx("IIsqTPC"),		gen_all_args,	0,		2 },
    { ERx("IILQsidSessID"),	gen_all_args,	0,		1 },
    { ERx("IIsqlcdInit"),	gen_all_args,	0,		2 },
    { ERx("IILQcnConName"),     gen_all_args,   0,              1 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },

  /* SQL Dynamic routines # 21 */
    { ERx("IIsqExImmed"),	gen_all_args,	0,		1 },
    { ERx("IIsqExStmt"),	gen_all_args,	0,		2 },
    { ERx("IIsqDaIn"),		gen_all_args,	0,		2 },
    { ERx("IIsqPrepare"),	gen_all_args,	0,		5 },
    { ERx("IIsqDescribe"),	gen_all_args,	0,		4 },
    { ERx("IIsqDescInput"),	gen_all_args,	0,		3 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },

  /* SQL Dynamic routines # 31 */
    { ERx("IIFRsqDescribe"),	gen_all_args,	0,		6 },
    { ERx("IIFRsqExecute"),	gen_all_args,	0,		4 },
    { (char *)0,		0,		0,		0 }
};

/*
+* Procedure:	gen_sqlca
** Purpose:	Code generation for SQL[C|D]A (include files and IF statements).
** Parameters:
**	flag	- i4	- sqcaSQL 	- include the file,
**			  sqcaSQLDA	- include the SQLDA file
**			  sqcaGEN 	- generate the IF statements.
**			  sqcaSQPROC 	- generate IFs inside exec-proc loop.
** Return Values:
-*	None
** Notes:
**   1. File inclusion is only done for SQLCA.  We will use the SQLCA include 
**	file for the SQLCA definition as well as for external definitions of 
**	II calls and II work space if the language requires it.
**   2. IF statement generation is done based on the contents of the external
**      esqlca_store.  PASCAL programs can generate a sequence of:
**		if (cond1) then
**			statement1
**		end else if (cond2) then
**			statement2;
**   3. Code is generated differently for results from EXECUTE PROCEDURE
**      statements (sqcaSQPROC).  SQLMESSAGE events are handled only inside
**      the exec-proc loop.  NOTFOUND events are handled only outside this
**      loop.  GOTO actions must also send a break.
**
** History:
**	20-apr-1991 (kathryn)
**	    Generate SqlMessage code for all executable statements rather
**	    than just "execute procedure".  Procedures executed due to rule 
**	    firing will now have WHENEVER SQLMESSAGE handling for the last 
**	    message generated by the procedure.
**	    
*/

void
gen_sqlca( flag )
i4	flag;
{
    static	char	*sqconds[] = {
	/* Not Found */		ERx("(sqlca.sqlcode = 100)"),
	/* SqlError */		ERx("(sqlca.sqlcode < 0)"),
	/* SqlWarning */	ERx("(sqlca.sqlwarn.sqlwarn0 = 'W')"),
	/* SqlMessage */	ERx("(sqlca.sqlcode = 700)"),
	/* SqlEvent */          ERx("(sqlca.sqlcode = 710)")
    };
    register	SQLCA_ELM	*sq;
    register	i4		i;
    bool			need_else = FALSE;
    LOCATION	loc; 
    char	buf[MAX_LOC +1];
    char	*lname;
    i4		len;

    switch (flag)
    {
      case sqcaSQL:
      /* Spit out real Include line */
	STcopy( ERx("eqdef.pas"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	gen_include( lname, (char *)0, ERx("/nolist") );

      /* Now the sql-specific stuff */
	STcopy( ERx("eqsqlca.pas"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	gen_include( lname, (char *)0, ERx("/nolist") );

      /* Now for some trickery ! */
	NMgtAt( ERx("II_PAS_WHO"), &lname );
	if (lname != (char *)0 && *lname != '\0')
	{
	    SIprintf( ERx("EXEC SQL PASCAL\n") );
	    SIprintf(
	      ERx("EXEC SQL\tWritten by Mark R. Wittenberg (RTI), June 1986.\n")
	      );
	    SIflush( stdout );
	}
	gen_sqdeclared = TRUE;
	return;

      case sqcaSQLDA:
	/* Gen include file for SQLDA - C makes it external */
	STcopy( ERx("eqsqlda.pas"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	gen_include( lname, (char *)0, ERx("/nolist") );
	return;

      case sqcaGEN:
      case sqcaSQPROC:
      case sqcaGETEVT:	
	for (i = sqcaNOTFOUND, sq = esqlca_store; i < sqcaDEFAULT; i++, sq++)
	{
	    if (sq->sq_action == sqcaCONTINUE)
		continue;

	    /* never generate notfound code in proc-exec loop */
	    if (i == sqcaNOTFOUND && flag == sqcaSQPROC)
		continue;
	    /* Don't generate event handling after GET EVENT statement */
            if (i == sqcaEVENT && flag == sqcaGETEVT)
                continue;

	    /* [else] if (sqlca cond) - else required for return from CALL */
	    if (need_else)
	    {
		out_emit( ERx("\n") );
		gen__obj( TRUE, ERx("else ") );
	    }
	    else		/* First one */
		need_else = TRUE;
	    gen__obj( TRUE, ERx("if ") );
	    gen__obj( TRUE, sqconds[i] );
	    /* do action */
	    gen__obj( TRUE, ERx(" then\n") );
	    
	    /*
            ** We must generate begin/end block for the following cases:
            **  1. GOTO in an exececute procedure loop
            **  2. Generating code for EVENT
            */
	    if (sq->sq_action == sqcaGOTO && flag == sqcaSQPROC)
	    {
		gen__obj( TRUE, ERx("begin\n") );
	    	gen_do_indent( 1 );
		gen__II(IIBREAK);
		gen__obj( TRUE, ERx(";\n") );
		if (i == sqcaEVENT)
                {
                    arg_int_add(1);
                    arg_int_add(0);
                    gen__II( IIEVTSTAT );
                    gen__obj( TRUE, ERx(";\n") );
                }
	    }
	    else if (i == sqcaEVENT)
	    {
		gen__obj( TRUE, ERx("begin\n") );
                gen_do_indent( 1 );
                arg_int_add(1);
                arg_int_add(0);
                gen__II( IIEVTSTAT );
                gen__obj( TRUE, ERx(";\n") );
	    }
	    else
	    	gen_do_indent( 1 );

	    switch (sq->sq_action)
	    {
	      case sqcaSTOP:
		gen_sqarg();
		gen__II(IISQSTOP);
		break;
	      case sqcaGOTO:
		gen__obj( TRUE, ERx("goto ") );
		gen__obj( TRUE, sq->sq_label ? sq->sq_label : ERx("II?lab") );
		break;
	      case sqcaCALL:
		if (sq->sq_label)
		{
		    if (!STbcompare(sq->sq_label, 0, ERx("sqlprint"), 0, TRUE))
		    {
			gen_sqarg();
			gen__II(IISQPRINT);
		    }
		    else
			gen__obj( TRUE, sq->sq_label );
		}
		else
			gen__obj( TRUE, ERx("II?call") );
		break;
	    }
	    gen_do_indent( -1 );

	    /* special case - must do a begin/end block */
	    if ((sq->sq_action == sqcaGOTO && flag == sqcaSQPROC) ||
		 i == sqcaEVENT)
	    {	
		gen__obj( TRUE, ERx("\n") );
		gen__obj( TRUE, ERx("end") );
	    }	

	}
	if (need_else)
	    gen__obj( TRUE, ERx(";\n") );
	return;
    }
}


/*
+* Procedure:	gen_sqarg
** Purpose:	Add the address of the sqlca to the currrent argument list.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
** 1. The entry pointers are defined as "sq: Address" for sqlca.
*/

i4
gen_sqarg()
{
    if (gen_sqdeclared)
	arg_str_add( ARG_RAW, ERx("%ref sqlca") );
    else
	arg_str_add( ARG_RAW, ERx("%immed 0") );
    return 0;
}	

/*
+* Procedure:   gen_sqlcode
** Purpose:     Add the address of the SQLCODE to the currrent argument list,
**              which should be for the IIsqlcdInit function.
** Parameters:
**      None.
** Return Values:
-*      None.
** Notes:
** 1. Called directly by esq_init.
**
** History:
**      14-oct-1992 (larrym)
**          written.
*/

void
gen_sqlcode()
{
    if (eq->eq_flags & EQ_SQLCODE)
	arg_str_add( ARG_RAW, ERx("%ref SQLCODE") );
    else
	arg_str_add( ARG_RAW, ERx("%immed 0") );
}	

/*
+* Procedure:   gen_diaginit
** Purpose:     Add the address of SQLSTATE to the currrent argument list
**              of the IIsqGdInit function call.
**
** Parameters:
**      None.
**
** Return Values:
-*      None.
**
** Notes:
** 1. Called directly by esq_init.
**
** History:
**      14-oct-1992 (larrym)
**          written.
*/
void
gen_diaginit()
{
	gen__obj( TRUE, ERx("IIxsqGdInit(32,%descr SQLSTATE)"));
	out_emit( ERx(";\n") );
}	
