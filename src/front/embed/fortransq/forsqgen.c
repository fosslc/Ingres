/*
** Copyright (c) 2004, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<ereq.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<nm.h>
# include	<equel.h>
# include	<eqesql.h>
# include	<eqsym.h>		/* eqgen.h need SYM */
# include	<eqgen.h>
# include	<ere1.h>
# include	<ereq.h>
# include 	<eqf.h>
# include       <eqrun.h>               /* for T_CHA and T_CHAR defines */

/*
+* Filename:	forsqgen.c
** Purpose:	Defines ESQL dependent routines and tables for FORTRAN code 
**		generation.
** Defines:
**		gen_sqlca	- Generate an SQLCA if-then-else statement.
**		gen_sqarg	- Add "sqlca" to arg list.
**		gen_sqlcode	- Add "SQLCOD" to arg list.
**		gen__sqltab	- ESQL-specific gen table.
**		gen_diaginit	- Generate call that sets up Diagnostic area,
**				  i.e., SQLSTATE.
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
**		25-jun-1987	- Updated for 6.0 (bjb)
**		04-may-1988	- support for stored procedures (whc)
**		09-may-1989	- Added DG include files (sylviap)
**				  Added error message if user tries to include 
**				     SQLDA.  DG ESQL/FORTRAN does not support
**				     Dynamic SQL.
**		26-sep-1989	- Changed E_EQ0505_NO_SQLDA to E_EQ0125_NO_SQLDA
**				     so it won't collide with the FIPS errors.
**				     (sylviap)
**		06-oct-1989	- NO_SQLDA error applies to UNIX, too. (bjb)
**	 	18-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**		21-aug-1990 (barbara)
**		    Fixed bug 21083.  Call to IIbreak was being made without
**		    the word 'call'.
**		04-sep-1990 (kathryn)
**		    Add Support for UNIX Dynamic SQL SQLDA.  (SUN F77 now 
**		    supports structures.)  Error for ("NO_SQLDA") no longer 
**		    generated on UNIX.
**		05-sep-1990 (kathryn)
**		    Integrate Porting changes from 6202p code line:
**                  10-jul-1990 - Support for HP3000 MPE/Xl (hp9_mpe). (fredb)
**              24-apr-1991     - Update for alerters to generate a
**                                call to IILQesEvStat when
**                                WHENEVER SQLEVENT is generated. (teresal)
**		03-jun-1991	- Fixed call to "IILQesEvStat" to 
**				  generate correct fortran syntax to
**				  avoid fortran compiler error.
**				  Bug #37869. (teresal)
**      14-oct-1992 (larrym)
**          Added IIsqlcdInit to the appropirate place.  This is called
**          instead of IIsqInit if the user is using SQLCOD.  Added
**          functions gen_sqlcode and gen_diaginit.
**	11-feb-1993 (larrym)
**	    fixed handling of sqlstate descriptors.  Modified gen_diaginit.
**      08-mar-1993 (larrym)
**          added new function IILQcnConName (IISQCNNM) for connection names.
**	26-Aug-1993 (fredv)
**	    Included <st.h>.
**	20-jan-1994 (teresal)
**	    Fix gen_diaginit() so that SQLSTATE is not getting a EOS added
**	    to it at runtime - this was causing memory to get corrupted!
**	    SQLSTATE host variables for all non-C languages are DB_CHA_TYPE
**	    types NOT DB_CHR_TYPE types. Bug 58912. Also, brought code up
**          to Ingres coding standards by including comments.
**	28-mar-1994 (larrym)
**	    Finished adding comments, and added a fix to bug 60322.  Long
**	    term solution to this bug is to gen SQLSTATE (or SQLSTA) from 
**	    the symbol table; putting in a short term fix so we can pass
**	    SQL 92 tests.  Short term fix is yet another flag value for
**	    eq->eq_flags.  
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-apr-2001 (mcgem01)
**          Implement Embedded Fortran Support on Windows.
**	20-may-2003 (mcgem01)
**	    Fortran file extension on Windows is .for rather than .f
**	6-Jun-2006 (kschendel)
**	    Added describe input.
**	22-Jun-2008 (hweho01)
**	    Included eqdef64.f and eqsqlda64.f for 64-bit Fortran
**	    support on hybrid ports.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

/* 
** Allow testing of VMS preprocessor to be done on Unix.
*/
# ifdef VMS_TEST_ON_UNIX
# undef UNIX
# define VMS
# endif /* VMS_TEST_ON_UNIX */

static bool	gen_sqdeclared = FALSE;

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For PL/I there is very little
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
	gen_II();
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
# if defined(UNIX) || defined(hp9_mpe)
    { ERx("IIsqEImmed"),	gen_all_args,	0,		1 },
# else
    { ERx("IIsqExImmed"),	gen_all_args,	0,		1 },
# endif
    { ERx("IIsqExStmt"),	gen_all_args,	0,		2 },
# if defined(UNIX) || defined(hp9_mpe)
    { ERx("IIsqDtIn"),		gen_all_args,	0,		2 },
# else
    { ERx("IIsqDaIn"),		gen_all_args,	0,		2 },
# endif
# if defined(UNIX) || defined(hp9_mpe)
    { ERx("IIsqPepare"),	gen_all_args,	0,		5 },
# else
    { ERx("IIsqPrepare"),	gen_all_args,	0,		5 },
# endif
    { ERx("IIsqDescribe"),	gen_all_args,	0,		4 },
    { ERx("IIsqDescInput"),	gen_all_args,	0,		3 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 },

  /* FRS Dynamic routines # 31 */
# if defined(UNIX) || defined(hp9_mpe)
    { ERx("IIFsqDescribe"),	gen_all_args,	0,		6 },
# else
    { ERx("IIFRsqDescribe"),	gen_all_args,	0,		6 },
# endif
# if defined(UNIX) || defined(hp9_mpe)
    { ERx("IIFsqExecute"),	gen_all_args,	0,		4 },
# else
    { ERx("IIFRsqExecute"),	gen_all_args,	0,		4 },
# endif
    { (char *)0,		0,		0,		0 }
};

/*
+* Procedure:	gen_sqlca
** Purpose:	Code generation for SQL[C|D]A (include files and IF statements).
** Parameters:
**	flag	- i4	- sqcaSQL - include the file,
**			  sqcaSQLDA - include the SQLDA file
**			  sqcaGEN - generate the IF statements.
**			  sqcaSQPROC - generate proc-loop code.
**			  sqcaGETEVT - generation after GET EVENT stmt.
** Return Values:
-*	None
** Notes:
**   1. File inclusion is only done for SQLCA.  We will use the SQLCA include 
**	file for the SQLCA definition as well as for external definitions of 
**	II calls and II work space if the language requires it (PL/I, COBOL, 
**	FORTRAN).  
**   2. IF statement generation is done based on the contents of the external
**      esqlca_store.  FORTRAN programs can generate a sequence of 
**	IF-THEN-GOTO-ELSE-ENDIF
**   3. File inclusion may be done for SQLDA at a future date.
**   4. Code is generated differently for results from EXECUTE PROCEDURE
**	statements (sqcaSQPROC).  SQLMESSAGE events are handled only inside
**	the exec-proc loop.  NOTFOUND events are handled only outside this loop.
**	GOTO actions must also send a break.
**   5. The GET EVENT statement must not be followed by the WHENEVER 
**	SQLEVENT condition.
**
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
		/* Not Found */		ERx("(sqlcod .eq. 100)"),
		/* SqlError */		ERx("(sqlcod .lt. 0)"),
		/* SqlWarning */	ERx("(sqlwrn(0) .eq. 'W')"),
		/* SqlMessage */	ERx("(sqlcod .eq. 700)"),
		/* SqlEvent */		ERx("(sqlcod .eq. 710)")
				    };
    register	SQLCA_ELM	*sq;
    register	i4		i;
    char                        buf[MAX_LOC + 1];
    bool			need_else = FALSE;
    LOCATION	loc; 
    char	*lname;

    switch (flag)
    {
      case sqcaSQL:
	/* Gen include file for SQLCA */
	if (eq->eq_flags & EQ_SQLCODE)
	{
# if defined(UNIX) || defined(hp9_mpe)
            STcopy(ERx("eqsqlcd.f"), buf);
# else
# 	ifdef DGC_AOS
        	STcopy(ERx("eqsqlcd.in"), buf);
# 	else
        	STcopy(ERx("eqsqlcd.for"), buf);
# 	endif /* DGC_AOS */
# endif /* ifdef UNIX */
        }
	else
	{
# if defined(UNIX) || defined(hp9_mpe) 
            STcopy(ERx("eqsqlca.f"), buf);
# else
# 	ifdef DGC_AOS
        	STcopy(ERx("eqsqlca.in"), buf);
# 	else
        	STcopy(ERx("eqsqlca.for"), buf);
# 	endif /* DGC_AOS */
# endif /* ifdef UNIX */
	}
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	gen_include( lname, (char *)0, (char *)0 );
	gen_sqdeclared = TRUE;
	/* Include the EQUEL file too */
# if defined(UNIX) 
#	if defined(conf_BUILD_ARCH_32_64)
	        if (eq->eq_flags & EQ_FOR_DEF64)
        	    STcopy(ERx("eqdef64.f"), buf);
                else
	            STcopy(ERx("eqdef.f"), buf);
#	else
	        STcopy(ERx("eqdef.f"), buf);
#	endif /* hybrid */
# else
# 	ifdef DGC_AOS
        	STcopy(ERx("eqdef.in"), buf);
# 	else
        	STcopy(ERx("eqdef.for"), buf);
# 	endif /* DGC_AOS */
# endif /* ifdef UNIX */
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	gen_include( lname, (char *)0, (char *)0 );
# if defined (UNIX) /* MPE/XL looks for eqdeffor */
	STcopy( ERx("eqfdef.f"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	gen_include( lname, (char *)0, (char *)0 );
# endif
	return;

      case sqcaSQLDA:
	/* Gen include file for SQLDA */
	/* DG and UNIX ESQL/FORTRAN do not support the SQLDA, so print error. */
# ifdef DGC_AOS
	er_write ( E_EQ0125_NO_SQLDA, EQ_ERROR, 0);
	return;
# endif /* DGC_AOS */
# if defined(UNIX) || defined(hp9_mpe)
#	if defined(conf_BUILD_ARCH_32_64)
                if (eq->eq_flags & EQ_FOR_DEF64)
                    STcopy(ERx("eqsqlda64.f"), buf);
                else
                    STcopy(ERx("eqsqlda.f"), buf);
#       else
                STcopy(ERx("eqsqlda.f"), buf);
#       endif /* hybrid */
# else
        STcopy(ERx("eqsqlda.for"), buf);
# endif /* UNIX - sqlda supported */

	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	gen_include( lname, (char *)0, (char *)0 );
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
		gen__obj( TRUE, ERx("else ") );
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
		gen__II(IISQSTOP);
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
		if (sq->sq_label)
		{
		    if (!STbcompare(sq->sq_label, 0, ERx("sqlprint"), 0, TRUE))
		    {
			gen_sqarg();
			gen__obj( TRUE, ERx("call ") );
			gen__II(IISQPRINT);
		    }
		    else
		    {
			gen__obj( TRUE, ERx("call ") );
			gen__obj( TRUE, sq->sq_label );
		    }
		}
		else
			gen__obj( TRUE, ERx("call II?call") );
		break;
	    }
	    gen__obj( TRUE, ERx("\n") );
	    gen_do_indent( -1 );
	}
	if (need_else)
	    gen__obj( TRUE, ERx("end if\n") );
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
*/

i4
gen_sqarg()
{
    if (gen_sqdeclared)
	arg_str_add( ARG_RAW, ERx("sqlcax") );
    else
	arg_int_add( 0 );
    return 0;
}

/*
+* Procedure:	gen_sqlcode
** Purpose:	Add the address of the SQLCOD to the currrent argument list.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
*/

void
gen_sqlcode()
{
    if (eq->eq_flags & EQ_SQLCODE)
	arg_str_add( ARG_RAW, ERx("SQLCOD") );
    else
	arg_int_add( 0 );
}

/*
+* Procedure:	gen_diaginit
** Purpose:	Generate call that initializes the SQL Diagnostic area 
**		at runtime.
** Parameters:
**	None.
** Return Values:
-*	None.
** Notes:
**	o This function generates a call to IIsqGdInit() before each
**	  SQL query so that the SQL Diagnostic area (i.e., SQLSTATE)
**	  gets initialized for every query. IIsqGdInit() is passed the address
**	  of the user's SQLSTATE host variable and it's type
**	  to LIBQ at runtime. For non-C languages, the type is always 
**	  a DB_CHA_TYPE (20) because the SQLSTATE variable doesn't need to 
**	  be null terminated.
**	o Constants shouldn't be used here, but given how this function is 
**	  written, we have no choice. This function should be rewritten
**	  to generate this call using the same method we use for generating
**	  all the other calls, i.e, use gen__sqltab[] table etc.
**
**  History
**      14-oct-1992 (larrym)
**	    written.
**	20-jan-1994 (teresal)
**	    Fix gen_diaginit() so that SQLSTATE is not getting a EOS added
**	    to it at runtime - this was causing memory to get corrupted!
**	    SQLSTATE host variables for all non-C languages are DB_CHA_TYPE
**	    types NOT DB_CHR_TYPE types. Bug 58912. Also, brought code up
**          to Ingres coding standards by including comments.
**	28-mar-1994 (larrym)
**	    fix to bug 60322.  Long term solution to this bug is to gen 
**	    SQLSTATE (or SQLSTA) from the symbol table; putting in a short 
**	    term fix so we can pass SQL 92 tests.  Short term fix is yet 
**	    another flag value for eq->eq_flags.  
*/

void
gen_diaginit()
{
    if (eq->eq_flags & EQ_SQLSTA)
    {
#ifdef VMS
	gen__obj( TRUE, ERx("call IIxsqGdInit(%val(20),SQLSTA)"));
#else
	gen__obj( TRUE, ERx("call IIsqGd(20,SQLSTA)"));
#endif
    }
    else
    {
#ifdef VMS
	gen__obj( TRUE, ERx("call IIxsqGdInit(%val(20),SQLSTATE)"));
#else
	gen__obj( TRUE, ERx("call IIsqGd(20,SQLSTATE)"));
#endif
    }
    out_emit( ERx("\n") );
}
