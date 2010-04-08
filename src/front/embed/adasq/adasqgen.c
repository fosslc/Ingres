/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<lo.h>
# include	<nm.h>
# include	<si.h>
# include	<st.h>
# include	<equel.h>
# include	<eqesql.h>
# include	<eqsym.h>		/* eqgen.h need SYM */
# include	<eqgen.h>
# include 	<eqada.h>
# include	<eqrun.h>


/*
+* Filename:	adasqgen.c
** Purpose:	Defines ESQL dependent routines and tables for ADA code 
**		generation.
** Defines:
**		gen_sqlca	- Generate an SQLCA if-then-else statement.
**		gen_sqarg	- Add "sqlca" to arg list.
**              gen_sqlcode     - Add "SQLCODE" to arg list.
**              gen_diaginit    - Generate Diagnostic Area init call.
**		gen__sqltab	- ESQL-specific gen table.
**		gen_diaginit    - Generate call that sets up Diagnostic area,
**				  i.e., SQLSTATE.
**
** Externals Used:
-*		esqlca_store[]	- Array of four WHENEVER states.
**
** Notes:
**	Interface layer on top of the EQUEL gen file.
** 
** History:
**		15-aug-1985	- Written for C (ncg, mrw)
**		22-nov-1985	- Modified for PL/I (ncg)
**		01-apr-1986	- Modified for ADA (ncg)
**		03-oct-1989	- Modified for dynamic Ada configuration (ncg)
**		19-dec-1989	- Updated for Phoenix/Alerters (bjb)
**		24-apr-1991	- Update for alerters to generate a 
**				  call to IILQesEvStat when
**				  WHENEVER SQLEVENT is generated. (teresal)
**      16-dec-1992 (larrym)
**          Added IIsqlcdInit to the appropirate place.  This is called
**          instead of IIsqInit if the user is using SQLCODE.  Added
**          functions gen_sqlcode and gen_diaginit.  
**	11-feb-1993 (larrym)(
**	    Added gen_diaginit for SQLSTATE support.
**      08-mar-1993 (larrym)
**          added new function IILQcnConName (IISQCNNM) for connection names.
**	26-aug-1993 (dianeh)
**	    Added #include of <st.h>.
**      20-jan-1994 (teresal)
**          Fix gen_diaginit() so that SQLSTATE is not getting a EOS added
**          to it at runtime - this was causing memory to get corrupted!
**          SQLSTATE host variables for all non-C languages are DB_CHA_TYPE
**          types NOT DB_CHR_TYPE types. Bug 58912. Also, brought code up
**          to Ingres coding standards by including comments.
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Jun-2006 (kschendel)
**	    Added IIsqDescInput.
**	04-May-2007 (toumi01)
**	    Add ESQLGBL Ada package for non-multi programs.
**	10-Oct-2007 (toumi01)
**	    Ditched ESQLGBL split-out of sqlca instantiation in favor of
**	    ESQL package for single-thread and ESQLM for multi-threaded.
*/

static bool	gen_sqdeclared = FALSE;

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For ADA there is very little
** argument information needed, as will probably be the case for regular 
** callable languages. 
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
/* Index     Name			Function	Flags		Args */
/* -----     ----------			--------	-----		---- */

/* 000 */    {ERx("IIf_"),		0,		0,		0},

			/* Cursors # 1 */
/* 001 */    {ERx("IIcsDaGet"),		gen_all_args,	0,		2},
/* 002 */    {(char *)0,		0,		0,		0},
/* 003 */    {(char *)0,		0,		0,		0},
/* 004 */    {(char *)0,		0,		0,		0},
/* 005 */    {(char *)0,		0,		0,		0},

			/* SQL routines # 6 */
/* 006 */    {ERx("IIsqConnect"),	gen_var_args,	0,		15},
/* 007 */    {ERx("IIsqDisconnect"),	0,		0,		0},
/* 008 */    {ERx("IIsqFlush"),		gen_IIdebug,	0,		0},
/* 009 */    {ERx("IIsqInit"),		gen_all_args,	0,		1},
/* 010 */    {ERx("IIsqStop"),		gen_all_args,	0,		1},
/* 011 */    {ERx("IIsqUser"),		gen_all_args,	0,		1},
/* 012 */    {ERx("IIsqPrint"),		gen_all_args,	0,		1},
/* 013 */    {ERx("IIsqMods"),		gen_all_args,	0,		1},
/* 014 */    {ERx("IIsqParms"),		gen_all_args,	0,		5},
/* 015 */    {ERx("IIsqTPC"),		gen_all_args,	0,		2},
/* 016 */    {ERx("IILQsidSessID"),	gen_all_args,	0,		1},
/* 017 */    {ERx("IIsqlcdInit"),	gen_all_args,	0,		2},
/* 018 */    {ERx("IILQcnConName"),	gen_all_args,	0,		1},
/* 019 */    {(char *)0,		0,		0,		0},
/* 020 */    {(char *)0,		0,		0,		0},
	
			/* SQL Dynamic routines # 21 */
/* 021 */    {ERx("IIsqExImmed"),	gen_all_args,	0,		1},
/* 022 */    {ERx("IIsqExStmt"),	gen_all_args,	0,		2},
/* 023 */    {ERx("IIsqDaIn"),		gen_all_args,	0,		2},
/* 024 */    {ERx("IIsqPrepare"),	gen_all_args,	G_SQLDA,	5},
/* 025 */    {ERx("IIsqDescribe"),	gen_all_args,	0,		4},
/* 026 */    {ERx("IIsqDescInput"),	gen_all_args,	0,		3},
/* 027 */    {(char *)0,		0,		0,		0},
/* 028 */    {(char *)0,		0,		0,		0},
/* 029 */    {(char *)0,		0,		0,		0},
/* 030 */    {(char *)0,		0,		0,		0},

			/* Dynamic FRS routines # 31 */
/* 031 */    {ERx("IIFRsqDescribe"),	gen_all_args,	0,		6},
/* 032 */    {ERx("IIFRsqExecute"),	gen_all_args,	0,		4},
/* 026 */    {(char *)0,		0,		0,		0}
};

/*
+* Procedure:	gen_sqlca
** Purpose:	Code generation for SQLCA (include files and IF statements).
** Parameters:
**	flag	- i4	- sqcaSQL - include the SQLCA file,
**			- sqcaSQLDA - include the SQLDA file,
**			  sqcaGEN - generate the IF statements.
**			  sqcaSQPROC - generate IFs inside exec-proc loop.
** Return Values:
-*	None
** Notes:
**   1. File inclusion is only done for SQLCA.  We will use the SQLCA include 
**	file for the SQLCA definition as well as for external definitions of 
**	II calls and II work space if the language requires it.
**   2. IF statement generation is done based on the contents of the external
**      esqlca_store.  ADA programs can generate a sequence of:
**		if (cond1) then
**			statement1;
**		elsif (cond2) then
**			statement2;
**		endif;
**   3. Code is generated differently for results from EXECUTE PROCEDURE
**      statements (sqcaSQPROC).  SQLMESSAGE events are handled only inside
**      the exec-proc loop.  NOTFOUND loops are never handled inside this loop.
**      GOTO actions must send a break before leaving the loop.
**   4. The GET EVENT statement must not be followed by the WHENEVER
**	SQLEVENT condition (to avoid recursion of event handlers).
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
    static char	*sqconds[] = {
	/* Not Found */		ERx("(sqlca.sqlcode = 100)"),
	/* SqlError */		ERx("(sqlca.sqlcode < 0)"),
	/* SqlWarning */	ERx("(sqlca.sqlwarn.sqlwarn0 = 'W')"),
	/* SqlMessage */	ERx("(sqlca.sqlcode = 700)"),
	/* SqlEvent */		ERx("(sqlca.sqlcode = 710) ")
    };
    register	SQLCA_ELM	*sq;
    register	i4		i;
    bool			need_else = FALSE;
    LOCATION			loc; 
    char			buf[MAX_LOC +1];
    char			*lname;

    switch (flag)
    {
      case sqcaSQL:
	/* Spit out the With lines */
	out_emit( ERx("\n") );
	gen__obj(FALSE,
		ERx("with EQUEL;       use EQUEL;       -- INGRES/Run-time\n"));
	if (eq->eq_flags & EQ_MULTI)
		gen__obj( FALSE,
		ERx("with ESQLM;       use ESQLM;       -- INGRES/SQL\n") );
	else
		gen__obj( FALSE,
		ERx("with ESQL;        use ESQL;        -- INGRES/SQL\n") );
	gen__obj( FALSE,
		ERx("with EQUEL_FORMS; use EQUEL_FORMS; -- INGRES/FORMS\n") );
	out_emit( ERx("\n") );
	gen_sqdeclared = TRUE;
	return;

      case sqcaSQLDA:
	/* Gen include file for SQLDA - Ada makes it external */
	gen__obj(FALSE,
		ERx("with ESQLDA;      use ESQLDA;      -- INGRES/SQLDA\n"));
	return;

      case sqcaGEN:
      case sqcaSQPROC:
      case sqcaGETEVT:
	for (i = sqcaNOTFOUND, sq = esqlca_store; i < sqcaDEFAULT; i++, sq++)
	{
	    if (sq->sq_action == sqcaCONTINUE)
		continue;

	    /* never generate notfound code in proc-exec loop. */
	    if (i == sqcaNOTFOUND && flag == sqcaSQPROC)
		continue;
	    /* Don't generate event handling after GET EVENT statement */
	    if (i == sqcaEVENT && flag == sqcaGETEVT)
		continue;

	    /* [els]if (sqlca cond) - else required for return from CALL */
	    if (need_else)
		gen__obj( TRUE, ERx("els") );
	    else		/* First one */
		need_else = TRUE;
	    gen__obj( TRUE, ERx("if ") );
	    gen__obj( TRUE, sqconds[i] );
	    /* do action */
	    gen__obj( TRUE, ERx(" then\n") );
	    gen_do_indent( 1 );
	    /*
	    ** Generate an extra call for WHENEVER SQLEVENT 
	    ** to consume the event before the action
	    ** code is generated.
	    */
	    if (i == sqcaEVENT)
	    {
		arg_int_add(1);
		arg_int_add(0);
		gen__II( IIEVTSTAT );
		gen__obj( TRUE, ERx(";\n") );
	    }
	    switch (sq->sq_action)
	    {
	      case sqcaSTOP:
		gen_sqarg();
		gen__II( IISQSTOP );
		break;
	      case sqcaGOTO:
		if (flag == sqcaSQPROC)
		{
		    gen_sqarg();
		    gen__II( IIBREAK );
		    gen__obj( TRUE, ERx(";\n") );
		}
		if (sq->sq_label)
		{
		    /* Check for "!name" == raise name */
		    if (*sq->sq_label == '!')
		    {
			gen__obj( TRUE, ERx("raise ") );
			gen__obj( TRUE, sq->sq_label + 1 );
		    }
		    else
		    {
			gen__obj( TRUE, ERx("goto ") );
			gen__obj( TRUE, sq->sq_label );
		    }
		}
		else
		{
		    gen__obj( TRUE, ERx("goto II?lab") );
		}
		break;
	      case sqcaCALL:
		if (sq->sq_label)
		{
		    if (!STbcompare(sq->sq_label, 0, ERx("sqlprint"), 0, TRUE))
		    {
			gen_sqarg();
			gen__II( IISQPRINT );
		    }
		    else
			gen__obj( TRUE, sq->sq_label );
		}
		else
			gen__obj( TRUE, ERx("II?call") );
		break;
	    }
	    gen__obj( TRUE, ERx(";\n") );
	    gen_do_indent( -1 );
	}
	if (need_else)
	    gen__obj( TRUE, ERx("end if;\n") );
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
** 1. Called indirectly via dml_caarg.
** 2. The entry pointers are defined as "sq: Address" for sqlca.
*/

i4
gen_sqarg()
{
    if (gen_sqdeclared)
	arg_str_add( ARG_RAW, ERx("sqlca'Address") );
    else if (eq->eq_config & EQ_ADVAX)
	arg_str_add( ARG_RAW, ERx("sqlca'Null_parameter") );
    else 		/* EQ_ADVADS */
	arg_str_add( ARG_RAW, ERx("IInull") );
    return 0;
}

/*
+* Procedure:	gen_sqlcode
** Purpose:	Add the address of the SQLCODE to the current argument list.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
** 1. Called indirectly via dml_caarg.
** 2. The entry pointers are defined as "sq: Address" for sqlca.
*/

void
gen_sqlcode()
{
    if (eq->eq_flags & EQ_SQLCODE)
	arg_str_add( ARG_RAW, ERx("SQLCODE'Address") );
    else if (eq->eq_config & EQ_ADVAX)
	arg_str_add( ARG_RAW, ERx("SQLCODE'Null_parameter") );
    else 		/* EQ_ADVADS */
	arg_str_add( ARG_RAW, ERx("IInull") );
}

/*
+* Procedure:   gen_diaginit
** Purpose:     Generate call that initializes the SQL Diagnostic area
**              at runtime.
** Parameters:
**      None.
** Return Values:
-*      None.
** Notes:
**      o This function generates a call to IIsqGdInit() before each
**        SQL query so that the SQL Diagnostic area (i.e., SQLSTATE)
**        gets initialized for every query. IIsqGdInit() is passed the address
**        of the user's SQLSTATE host variable and it's type
**        to LIBQ at runtime. For non-c languages, the type is always
**        a T_CHA type because the SQLSTATE variable doesn't need to
**        be null terminated.
**      o Constants shouldn't be used here, but given how this function is
**        written, we have no choice. This function should be rewritten
**        to generate this call using the same method we use for generating
**        all the other calls, i.e, use gen__sqltab[] table etc.
*/

void
gen_diaginit()
{
    if (eq->eq_config & EQ_ADVAX)
	gen__obj( TRUE, ERx("IIsqGdInitS(20, String(SQLSTATE))") );
    else
	gen__obj( TRUE, ERx("IIsqGdInit(20, SQLSTATE'Address)") );

    out_emit( ERx(";\n") );
}

/*
+* Procedure:	gen_sqtext 
** Purpose:	Put out call for COPY SQLERROR INTO .... 
**		Special case because second argument is sent as ADA descriptor. 
** Parameters:	None
-* Returns:	0
*/

i4
gen_sqtext()
{
    gen_data( 0, 1 );			/* First argument (SQLCA) */
    out_emit( ERx(",") );
    gen__obj( TRUE, arg_name(2) );	/* Second argument (user's buffer) */
    out_emit( ERx(",") );
    gen_data( 0, 3 );			/* Third argument - length of buffer */
}
