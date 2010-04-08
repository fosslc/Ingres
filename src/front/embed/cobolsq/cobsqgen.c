/*
** Copyright (c) 2004 Ingres Corporation
*/

# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<nm.h>
# include 	<cm.h>
# include	<equel.h>
# include	<eqesql.h>
# include	<eqsym.h>		/* eqgen.h needs SYM */
# include	<eqgen.h>
# include 	<eqcob.h>

/*
+* Filename:	cobsqgen.c
** Purpose:	Defines ESQL dependent routines and tables for COBOL code 
**		generation.
** Defines:
**		gen_sqlca	- Generate an SQLCA if-then-else statement.
**		gen_sqarg	- Add "sqlca" to arg list.
**              gen_sqlcode     - Add "&SQLCODE" to arg list.
**		gen__sqltab	- ESQL-specific gen table.
**              gen_diaginit    - Generate call that sets up Diagnostic area,
**                                i.e., SQLSTATE.
**
** Externals Used:
**		eq		- Pointer to global state vector
-*		esqlca_store[]	- Array of three WHENEVER states.
**
** Notes:
**	Interface layer on top of the EQUEL gen file.
** 
** History:
**		15-aug-1985	- Written (bjb)
**		12-aug-1987	- Converted to 6.0 (mrw)
**		18-apr-1989	- Fixed bug 5100 (bjb)
**		19-apr-1989	- Changed for DG to COPY EQDDEF.COB and
**				  EQDSQLDA.COB.  DG has its own 
**				  versions. (sylviap)
**		21-nov-1989	- Support for MF:
**				  1. Case sensitive routine names.
**				  2. Include file names.
**		19-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**              07-feb-1990     - Modified to support MF COBOL but without use
**                                of 'structured COBOL' facilities END-IF,
**                                END-PERFORM, or CONTINUE, uses a dynamic
**                                switch in eq->eq_config to choose not to
**                                use structured COBOL. (kwatts, ken)
**                                1. Introduced G_RTIF attributes on several
**                                   routines in order to make generated calls
**                                   go via a runtime glue layer when -u.
** 				  2. Amended gen_sqlca to terminate ifs without
**				     using END-IF, by using period or ELSE
**				     depending on whether the error action 
**				     returns (eg CALL) or doesn't (eg GO TO).
**              24-apr-1991     - Update for alerters to generate a
**                                call to IILQesEvStat when
**                                WHENEVER SQLEVENT is generated. (teresal)
**      14-oct-1992 (larrym)
**              Added IIsqlcdInit to gen__sqltab.  This is called
**              instead of IIsqInit if the user is using SQLCODE.  Added
**              function gen_sqlcode, which is called by esq_init, to
**              generate arguments for the IIsqlcdInit call.  Also
**              added function IIsqGdInit to gen__sqltab to initialize the
**              ANSI SQL Diagnositc Area.  Added gen_diaginit, which is called
**              by esq_init, to generate arguments to the IIsqGdInit call.
**	10-feb-1993 (larrym)
**	    If user is using ANSI's SQLCODE, then we now include a different
**	    sqlca file (EQSQLCD.xxx).  Also fixed descriptor handling.
**      08-mar-1993 (larrym)
**          added new function IILQcnConName (IISQCNNM) for connection names.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**      20-jan-1994 (teresal)
**          Fix gen_diaginit() so that SQLSTATE is not getting a EOS added
**          to it at runtime - this was causing memory to get corrupted!
**          SQLSTATE host variables for all non-C languages are DB_CHA_TYPE
**          types NOT DB_CHR_TYPE types. Bug 58912.
**	21-Apr-1994 (larrym)
**	    Fixed bug 59599, generated code that always ended in a period,
**	    even when in a perform loop.
**	24-apr-1996 (muhpa01)
**	    Added support for Digital Cobol (DEC_COBOL).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-Jun-2006 (kschendel)
**	    Added describe input.
*/

static bool	gen_sqdeclared = FALSE;

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  
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
/* 001 */    {ERx("IIcsDaGet"),		gen_all_args,	G_RTIF,		2},
/* 002 */    {(char *)0,		0,		0,		0},
/* 003 */    {(char *)0,		0,		0,		0},
/* 004 */    {(char *)0,		0,		0,		0},
/* 005 */    {(char *)0,		0,		0,		0},

			/* SQL routines # 6 */
/* 006 */    {ERx("IIsqConnect"),	gen_var_args,	0,		15},
/* 007 */    {ERx("IIsqDisconnect"),	0,		0,		0},
/* 008 */    {ERx("IIsqFlush"),		gen_IIdebug,	G_RTIF,		2},
/* 009 */    {ERx("IIsqInit"),		gen_all_args,	G_RTIF,		1},
/* 010 */    {ERx("IIsqStop"),		gen_all_args,	0,		1},
/* 011 */    {ERx("IIsqUser"),		gen_all_args,	0,		1},
/* 012 */    {ERx("IIsqPrint"),		gen_all_args,	0,		1},
/* 013 */    {ERx("IIsqMods"),		gen_all_args,	0,		1},
/* 014 */    {ERx("IIsqParms"),		gen_all_args,	G_LONG,		5},
/* 015 */    {ERx("IIsqTPC"),		gen_all_args,	0,		2},
/* 016 */    {ERx("IILQsidSessID"),	gen_all_args,	0,		1},
/* 017 */    {ERx("IIsqlcdInit"),	gen_all_args,	G_RTIF,		2},
/* 018 */    {ERx("IILQcnConName"),     gen_all_args,   0,              1},
/* 019 */    {(char *)0,		0,		0,		0},
/* 020 */    {(char *)0,		0,		0,		0},

			/* SQL Dynamic routines # 21 */
/* 021 */    {ERx("IIsqExImmed"),	gen_all_args,	G_DYNSTR,	1},
/* 022 */    {ERx("IIsqExStmt"),	gen_all_args,	0,		2},
/* 023 */    {ERx("IIsqDaIn"),		gen_all_args,	0,		2},
/* 024 */    {ERx("IIsqPrepare"),	gen_all_args,	G_DYNSTR,	5},
/* 025 */    {ERx("IIsqDescribe"),	gen_all_args,	0,		4},
/* 026 */    {ERx("IIsqDescInput"),	gen_all_args,	0,		3},
/* 027 */    {(char *)0,		0,		0,		0},
/* 028 */    {(char *)0,		0,		0,		0},
/* 029 */    {(char *)0,		0,		0,		0},
/* 030 */    {(char *)0,		0,		0,		0},

			/* FRS Dynamic routines # 31 */
/* 031 */    {ERx("IIFRsqDescribe"),	gen_all_args,	0,		6},
/* 032 */    {ERx("IIFRsqExecute"),	gen_all_args,	0,		4},
/* 033 */    {(char *)0,		0,		0,		0}
};

/*
+* Procedure:	gen_sqlca
** Purpose:	Code generation for SQLCA (include files and IF statements).
** Parameters:
**	flag	- i4	- sqcaSQL - include the file,
**			  sqcaGEN - generate the IF statements.
** Return Values:
-*	None
** Notes:
**   1. In COBOL, the "include sqlca" statement generates code that includes
**	the SQLCA definition file as well as the file of "external" definitions 
**	of II calls and II work space.  If the preprocessor was invoked
**	with the `-d` flag, generate a data declaration containing input file
**	name for output of debugging information.
**   2. IF statement generation is done based on the contents of the external
**      esqlca_store.  We do not want to print a period after the IF or
**	IF-THEN-ELSE statement in case the whole SQL statement was included
**	in a user IF-statement; therefore we just terminate the statement
**	we generate with an END-IF (with no period after it).
**   3. The GET EVENT statement must now be followed by the WHENEVER
**	SQLEVENT condition (to avoid recursion of event handlers).
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
		/* Not Found */		ERx("(SQLCODE = 100) "),
		/* SqlError */		ERx("(SQLCODE < 0) "),
		/* SqlWarning */	ERx("(SQLWARN0 = \"W\") "),
		/* SqlMessage */	ERx("(SQLCODE = 700) "),
		/* SqlEvent */		ERx("(SQLCODE = 710) ")
				    };
    register	SQLCA_ELM	*sq;
    register	i4		i;
    register	i4		num_ifs = 0;
    register	bool		is_call = FALSE;   /* Only tested in the
						   /* unstructured option */

    LOCATION	loc; 
    char	*lname;
    char	buf[MAX_LOC +1];
    i4		len;
    bool	old_gcdot = gc_dot;

    switch (flag)
    {
      case sqcaSQL:
	/* Gen include file for SQLCA */
	if (eq->eq_flags & EQ_SQLCODE)
	{
	    /* include a special SQLCA without an SQLCODE */
#ifdef MF_COBOL
	    NMloc( FILES, FILENAME, ERx("eqmsqlcd.cbl"), &loc );
	    LOtos( &loc, &lname );
#else
# if defined(DEC_COBOL)
	    NMloc( FILES, FILENAME, ERx("eqsqlcd.cob"), &loc );
	    LOtos( &loc, &lname );
#else
	    NMloc( FILES, FILENAME, ERx("EQSQLCD.COB"), &loc );
	    LOtos( &loc, &lname );
	    CVupper( lname );		/* In a global buffer */
#endif /* DEC_COBOL */
#endif /* MF_COBOL */
        }
	else
	{
	    /* use normal SQLCA */
#ifdef MF_COBOL
	    NMloc( FILES, FILENAME, ERx("eqmsqlca.cbl"), &loc );
	    LOtos( &loc, &lname );
#else
# if defined(DEC_COBOL)
	    NMloc( FILES, FILENAME, ERx("eqsqlca.cob"), &loc );
	    LOtos( &loc, &lname );
# else
	    NMloc( FILES, FILENAME, ERx("EQSQLCA.COB"), &loc );
	    LOtos( &loc, &lname );
	    CVupper( lname );		/* In a global buffer */
# endif /* DEC_COBOL */
#endif /* MF_COBOL */
	}
	gen_include( lname, (char *)0, (char *)0 );
# if defined(MF_COBOL) || defined(DEC_COBOL)
	NMloc( FILES, FILENAME, ERx("eqmdef.cbl"), &loc );
	LOtos( &loc, &lname );
#else
# ifdef DGC_AOS
	NMloc( FILES, FILENAME, ERx("EQDDEF.COB"), &loc );
# else
	NMloc( FILES, FILENAME, ERx("EQDEF.COB"), &loc );
# endif /* DGC_AOS */
	LOtos( &loc, &lname );
	CVupper( lname );		/* In a global buffer */
#endif /* MF_COBOL */
	gen_include( lname, (char *)0, (char *)0 );
	gen_sqdeclared = TRUE;

	/* Put out IIDBG structure (fix for bug #5100) */
	if (eq->eq_flags & EQ_IIDEBUG)
	{
	    gen_comment( TRUE, ERx("ESQLCBL -d run-time flag.") );
	    STprintf( buf, ERx("%s%s%s"), eq->eq_filename, 
		      eq->eq_infile == stdin ? ERx("") : ERx("."), eq->eq_ext );
	    len = (i4)STlength( buf );
	    gen__obj( TRUE, ERx("01 IIDBG.\n") );
	    gen_do_indent( 1 );
	    gen__obj( TRUE, ERx("02 FILLER PIC X(") );
	    gen__int( len );
	    gen__obj( TRUE, ERx(") VALUE IS \"") );
	    out_emit( buf );
	    gen__obj( TRUE, ERx("\".\n") );
	    gen__obj( TRUE, ERx("02 FILLER PIC X VALUE LOW-VALUE.\n") );
	    gen_do_indent( (-1) );
	}
	return;

      case sqcaSQLDA:
	/* Gen include file for SQLDA */
#ifdef MF_COBOL
	NMloc( FILES, FILENAME, ERx("eqmsqlda.cbl"), &loc );
	LOtos( &loc, &lname );
#else
# if defined(DEC_COBOL)
	NMloc( FILES, FILENAME, ERx("eqsqlda.cob"), &loc );
	LOtos( &loc, &lname );
# else
# ifdef DGC_AOS
	NMloc( FILES, FILENAME, ERx("EQDSQLDA.COB"), &loc );
# else
	NMloc( FILES, FILENAME, ERx("EQSQLDA.COB"), &loc );
# endif /* DGC_AOS */
	LOtos( &loc, &lname );
	CVupper( lname );		/* In a global buffer */
# endif /* DEC_COBOL */
#endif /* MF_COBOL */
	gen_include( lname, (char *)0, (char *)0 );
	return;

      case sqcaSQPROC:
      case sqcaGEN:
      case sqcaGETEVT:
	gc_dot = FALSE;		/* No dots until we're done. */

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

	    /* [else] if (sqlca cond) - else required for return from CALL */
	    if (num_ifs > 0)
		gen__obj( TRUE, ERx("ELSE ") );
	    num_ifs++;
	    gen__obj( TRUE, ERx("IF ") );
	    gen__obj( TRUE, sqconds[i] );
	    gen_do_indent( 1 );
	    gc_newline();
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
            }	
	    
	    /* do action */
	    switch (sq->sq_action)
	    {
	      case sqcaSTOP:
		gen_sqarg();
		gen__II( IISQSTOP );
		break;
	      case sqcaGOTO:
		if (flag == sqcaSQPROC)
		{
		    gen__II( IIBREAK );
		}
		gen__obj( TRUE, ERx("GO TO ") );
		gen__obj( TRUE, sq->sq_label ? sq->sq_label : ERx("II?LAB") );
		gc_newline();
		break;
	      case sqcaCALL:
		if (sq->sq_label && 
		    STbcompare(sq->sq_label, 8, ERx("sqlprint"), 8, TRUE) == 0)
		{
		    gen_sqarg();
		    gen__II( IISQPRINT );
		}
		else
		{
		    gen__obj( TRUE, ERx("PERFORM ") );
		    gen__obj( TRUE,
				sq->sq_label ? sq->sq_label : ERx("II?CALL") );
		    gc_newline();
		}
		is_call = TRUE;
		break;
	    }
	    gen_do_indent( -1 );
	}
	/* If forcing unstructured COBOL must output a period or an ELSE,
	** depending on whether the error action ever returns. Otherwise
	** simply output the right number of END-IFs to close open ifs
	*/
	if ((eq->eq_config & EQ_COBUNS) && (num_ifs != 0))
	{
	    if (is_call)
		/* Only force a dot now if not going to have one anyway */
		gen__obj( TRUE, (old_gcdot ? ERx(" ") : ERx(".") ) );
            else
		gen__obj( TRUE, ERx("ELSE MOVE 0 TO II0") );
	}
	else
	{
	    for (i = 0; i < num_ifs; i++ )	/* emit right num of end-ifs */
	    {
		if (i > 0)
		    gen__obj( TRUE, ERx(" ") );
		gen__obj( TRUE, ERx("END-IF") );
	    }
	}
	gc_dot = old_gcdot;			/* Reset dot state */
	if (num_ifs > 0)		/* newline only if 'if' stmt emitted */
	    gc_newline();
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
**	Called indirectly via dml_caarg.
*/

i4
gen_sqarg()
{
    if (gen_sqdeclared)
	arg_str_add( ARG_RAW, ERx("SQLCA") );
    else
	arg_str_add( ARG_CHAR, (char *)0 );	/* Just need null pointer */
    return 0;
}

/*
+* Procedure:	gen_sqlcode
** Purpose:	Add the address of the SQLCODE to the currrent argument list.
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
gen_sqlcode()
{
    if (eq->eq_flags & EQ_SQLCODE)
	arg_str_add( ARG_RAW, ERx("SQLCODE") );
    else
	arg_str_add( ARG_CHAR, (char *)0 );	/* Just need null pointer */
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
**	21-jan-1994 (teresal)
**	    The type of the user's SQLSTATE variable is a CHA type not
**	    a CHR (C string) type. Constants shouldn't be used here, but
**	    I have no choice given the way this code was written. This
**	    should be rewritten when someone has the time to do so.
**	21-Apr-1994 (larrym)
**	    Fixed bug 59599, generated code that always ended in a period,
**	    even when in a perform loop.
*/
void
gen_diaginit()
{

# if defined(MF_COBOL) || defined(DEC_COBOL)
	gen__obj( TRUE, 
	    ERx("CALL \"IIsqGdInit\" USING VALUE 20 REFERENCE SQLSTATE"));
#else
	gen__obj( TRUE, 
	    ERx("CALL \"IIXSQGDINIT\" USING VALUE 20 DESCRIPTOR SQLSTATE"));
#endif
	gc_newline();
}
