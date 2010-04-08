/*
** Copyright 1985, 1992, 2000 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<nm.h>
# include	<equel.h>
# include	<eqesql.h>
# include	<eqsym.h>		/* eqgen.h need SYM */
# include	<eqgen.h>
# include	<ereq.h>
# include	<ere3.h>
# include 	<eqbas.h>
# include	<eqrun.h>


/*
+* Filename:	bassqgen.c
** Purpose:	Defines ESQL dependent routines and tables for BASIC code 
**		generation.
** Defines:
**		gen_sqlca	- Generate an SQLCA if-then-else statement.
**		gen_sqarg	- Add "sqlca" to arg list.
**              gen_sqlcode     - Add "&SQLCODE" to arg list.
**		gen__sqltab	- ESQL-specific gen table.
**
** Externals Used:
-*		esqlca_store[]	- Array of four WHENEVER states.
**
** Notes:
**	Interface layer on top of the EQUEL gen file.
** 
** History:
**		15-aug-1985	- Written for C (ncg, mrw)
**		04-dec-1986	- Modified for BASIC (bjb)
**		15-jan-1989 	- fixed bug #4436 (bjb)
**					Bring code generated for sqlwarning
**					condition into synch with updated
**					declaration of SQLCA data structure.
**		19-dec-1989	- Updated for Phoenix/Alerters (bjb)
**              24-apr-1991     - Update for alerters to generate a
**                                call to IILQesEvStat when
**                                WHENEVER SQLEVENT is generated. (teresal)
**      14-oct-1992 (larrym)
**              Added IIsqlcdInit to gen__sqltab.  This is called
**              instead of IIsqInit if the user is using SQLCODE.  Added
**              function gen_sqlcode, which is called by esq_init, to
**		generate arguments for the IIsqlcdInit call.  Also 
**		added function IIsqGdInit to gen__sqltab to initialize the 
**		ANSI SQL Diagnositc Area.  Added gen_diaginit, which is called
**		by esq_init, to generate arguments to the IIsqGdInit call.
**	11-feb-1993 (larrym)
**	    removed IIsqGdInit from gen__sqltab; don't need it anymore.
**          If user is using ANSI's SQLCODE, then we now include a different
**          sqlca file (EQSQLCD.BAS).
**      08-mar-1993 (larrym)
**          added new function IILQcnConName (IISQCNNM) for connection names.
**      08-30-93 (huffman)
**              add <st.h>
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	6-Jun-2006 (kschendel)
**	    Added IIsqDescInput.
*/

static bool	gen_sqdeclared = FALSE;

i4	gen_var_args(),
	gen_io(),
	gen_all_args(),
	gen_sqarg(),
	gen_IIdebug(),
	gen_II();
void	gen__obj(),
	gen_sqlcode(),
	gen_diaginit();

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  There is very little
** argument information needed, as will probably be the case for regular 
** callable languages. 
**
** This is the ESQL-specific part of the table
*/

GLOBALDEF GEN_IIFUNC	gen__sqltab[] = {
  /* Name */			/* Function */	/* Flags */	/* Args */
    {ERx("IIc_"),		0,		0,		0},

  /* Cursors # 1 */
    {ERx("IIcsDaGet"),		gen_all_args,	0,		2},
    {(char *)0,			0,		0,		0},
    {(char *)0,			0,		0,		0},
    {(char *)0,			0,		0,		0},
    {(char *)0,			0,		0,		0},

  /* SQL routines # 6 */
    {ERx("IIsqConnect"),	gen_var_args,	0,		15},
    {ERx("IIsqDisconnect"),	0,		0,		0},
    {ERx("IIsqFlush"),		gen_IIdebug,	0,		0},
    {ERx("IIsqInit"),		gen_all_args,	0,		1},
    {ERx("IIsqStop"),		gen_all_args,	0,		1},
    {ERx("IIsqUser"),		gen_all_args,	0,		1},
    {ERx("IIsqPrint"),		gen_all_args,	0,		1},
    {ERx("IIsqMods"),		gen_all_args,	0,		1},
    {ERx("IIsqParms"),		gen_all_args,	0,		5},
    {ERx("IIsqTPC"),		gen_all_args,	0,		2},
    {ERx("IILQsidSessID"),	gen_all_args,	0,		1},
    {ERx("IIsqlcdInit"),	gen_all_args,	0,		2},
    {ERx("IILQcnConName"),	gen_all_args,	0,		1},
    {(char *)0,			0,		0,		0},
    {(char *)0,			0,		0,		0},
 
  /* SQL Dynamic routines # 21 */
    {ERx("IIsqExImmed"),	gen_all_args,	0,		1},
    {ERx("IIsqExStmt"),		gen_all_args,	0,		2},
    {ERx("IIsqDaIn"),		gen_all_args,	0,		2},
    {ERx("IIsqPrepare"),	gen_all_args,	0,		5},
    {ERx("IIsqDescribe"),	gen_all_args,	0,		4},
    {ERx("IIsqDescInput"),	gen_all_args,	0,		3},
    {(char *)0,			0,		0,		0},
    {(char *)0,			0,		0,		0},
    {(char *)0,			0,		0,		0},
    {(char *)0,			0,		0,		0},

  /* FRS Dynamic routines # 31 */
    {ERx("IIFRsqDescribe"),	gen_all_args,	0,		6},
    {ERx("IIFRsqExecute"),	gen_all_args,	0,		4},
    {(char *)0,			0,		0,		0}
};

/*
+* Procedure:	gen_sqlca
** Purpose:	Code generation for SQL[C|D]A (include files and IF statements).
** Parameters:
**	flag	- i4	- sqcaSQL - include the SQLCA file
**			  sqcaSQLDA - include the SQLDA file
**			  sqcaGEN - generate the IF statements.
**			  sqcaSQPROC - generate IFs inside exec-proc loop.
**			  sqcaGETEVT - generation after GET EVENT stmt.
** Return Values:
-*	None
** Notes:
**   1. File inclusion is done for SQLCA and SQLDA.  At the same time as we
**	include the file containing the definition of the SQLCA, we include 
**	the file of external function declarations.
**   2. IF statement generation is done based on the contents of the external
**      esqlca_store.  BASIC programs can generate a sequence of 
**	IF-THEN-GOTO-ELSE-ENDIF statements.
**   3. Code is generated differently for results from EXECUTE PROCEDURE
**      statements (sqcaSQPROC).  SQLMESSAGE events are handled only inside
**      the exec-proc loop.  NOTFOUND events are never handled inside this
**      loop.  GOTO actions must send a break before leaving the loop.
**   4. The GET EVENT statement must not be followed by the WHENEVER
**	SQLEVENT condition (to avoid recursion of event handlers).
**
** History:
**	15-jan-1989 - fixed bug #4436 (bjb)
**	19-dec-1989 - Modified WHENEVER handling for EVENTS (bjb)
**	20-apr-1991 - (kathryn)
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
	/* Not Found */		ERx("(sqlcode = 100%)"),
	/* SqlError */		ERx("(sqlcode < 0%)"),
	/* SqlWarning */	ERx("(sqlwarn0 = 'W')"),
	/* SqlMessage */	ERx("(sqlcode = 700%)"),
	/* SqlEvent */		ERx("(sqlcode = 710%)")
    };
    register	SQLCA_ELM	*sq;
    register	i4		i;
    register	i4	    	num_ifs = 0;

    LOCATION	loc; 
    char	*lname;

    switch (flag)
    {
      case sqcaSQL:
	/* Gen include file for SQLCA */
        if (eq->eq_flags & EQ_SQLCODE)
        {
            /* include a special SQLCA without an SQLCODE */
	    NMloc( FILES, FILENAME, ERx("eqsqlcd.bas"), &loc );
	}
	else
	{
            /* include a special SQLCA without an SQLCODE */
	    NMloc( FILES, FILENAME, ERx("eqsqlca.bas"), &loc );
	}
        LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	Bput_lnum();			/* Emit optional line number */
	gen_include( lname, (char *)0, (char *)0 );
	/* Include the EQUEL file too */
	NMloc( FILES, FILENAME, ERx("eqdef.bas"), &loc );
	LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	gen_include( lname, (char *)0, (char *)0 );
	gen_sqdeclared = TRUE;
	return;

      case sqcaSQLDA:
	/* Gen include file for SQLDA */
	NMloc( FILES, FILENAME, ERx("eqsqlda.bas"), &loc );
	LOtos( &loc, &lname );
	CVlower( lname );		/* In a global buffer */
	Bput_lnum();			/* Emit optional line number */
	gen_include( lname, (char *)0, (char *)0 );
	return;

      case sqcaGEN:
      case sqcaSQPROC:
      case sqcaGETEVT:
	for (i = sqcaNOTFOUND, sq = esqlca_store; i < sqcaDEFAULT; i++, sq++)
	{
	    if (sq->sq_action == sqcaCONTINUE)
		continue;

	    /* never generate notfound code in exec proc loop */
	    if (i == sqcaNOTFOUND && flag == sqcaSQPROC)
		continue;
	    /* Don't generate event handling after GET EVENT statement */
	    if (i == sqcaEVENT && flag == sqcaGETEVT)
		continue;

	    /* [else] if (sqlca cond) - else required for return from CALL */
	    if (num_ifs > 0)
	    {
		gen__obj( TRUE, ERx("else ") );
		num_ifs++;
	    }
	    else		/* First one */
		num_ifs = 1;
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
		gen__obj( TRUE, ERx("call ") );
                arg_int_add(1);
                arg_int_add(0);
                gen__II( IIEVTSTAT );
                gen__obj( TRUE, ERx("\n") );
            }

	    switch (sq->sq_action)
	    {
	      case sqcaSTOP:
		gen_sqarg();
		gen__obj( TRUE, ERx("call ") );
		gen__II( IISQSTOP );
		break;
	      case sqcaGOTO:
		if (flag == sqcaSQPROC)
		{
		    gen__obj( TRUE, ERx("call ") );
		    gen__II( IIBREAK );
		    gen__obj( TRUE, ERx("\n") );
		}
		gen__obj( TRUE, ERx("goto ") );
		gen__obj( TRUE, sq->sq_label ? sq->sq_label : ERx("II?lab") );
		break;
	      case sqcaCALL:
		gen__obj( TRUE, ERx("call ") );
		if (sq->sq_label)
		{
		    if (!STbcompare(sq->sq_label, 0, ERx("sqlprint"), 0, TRUE))
		    {
			gen_sqarg();
			gen__II( IISQPRINT );
		    }
		    else
		    {
			gen__obj( TRUE, sq->sq_label );
		    }
		}
		else
		{
		    gen__obj( TRUE, ERx("II?call") );
		}
		break;
	    }
	    gen__obj( TRUE, ERx("\n") );
	}
	while (num_ifs-- > 0)
	{
	    gen_do_indent( -1 );
	    gen__obj( TRUE, ERx("end if\n") );
	}
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
*/

i4
gen_sqarg()
{
    if (gen_sqdeclared)
	arg_str_add( ARG_RAW, ERx("sqlcax by value") );
    else
	arg_int_add( 0 );
    return 0;
}

/*
+* Procedure:	gen_sqlcode
** Purpose:	Add the address of the SQLCODE to the currrent argument list,
**		which should be for the IIsqlcdInit function.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
** 1. Called directly by esq_init.
**
** History:
**	14-oct-1992 (larrym)
**	    written.
*/

void
gen_sqlcode()
{
    if (eq->eq_flags & EQ_SQLCODE)
	arg_str_add( ARG_RAW, ERx("SQLCODE") );
    else
	arg_int_add( 0 );
} 

/*
+* Procedure:   gen_diaginit
** Purpose:     Add the address of SQLSTATE to the currrent argument list
**		of the IIsqGdInit function call.
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
	gen__obj( TRUE, ERx("call IIxsqGdInit(32% by value,SQLSTATE)"));
	out_emit( ERx("\n"));
} 
