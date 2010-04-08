/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<si.h>
# include	<lo.h>
# include	<nm.h>
# include	<equel.h>
# include	<eqscan.h>		/* needed for eqesql.h */
# include	<eqesql.h>
# include	<eqsym.h>		/* eqgen.h need SYM */
# include	<eqgen.h>
# include 	<eqc.h>
# include 	<eqrun.h>		/* for T_CHA and T_CHAR defines */
# include 	<eqlang.h>


/*
+* Filename:	csqgen.c
** Purpose:	Defines ESQL dependent routines and tables for C code 
**		generation.
** Defines:
**		gen_sqlca	- Generate an SQLCA if-then-else statement.
**		gen_sqarg	- Add "&sqlca" to arg list.
**		gen_sqlcode	- Add "&SQLCODE" to arg list.
**		gen_diaginit	- Generate Diagnostic Area init call.
**		gen__sqltab	- ESQL-specific gen table.
**
** Externals Used:
-*		esqlca_store[]	- Array of five WHENEVER states.
**
** Notes:
**	Interface layer on top of the EQUEL gen file.
** 
** History:
**		15-aug-1985	- Written (ncg, mrw)
**		13-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**		21-apr-1991	- Update for alerters to generate a
**				  call to IILQesEvStat when
**				  WHENEVER SQLEVENT is generated. (teresal)
**		20-jul-1991	- Change comments to indicate new DBEVENT
**				  syntax. (teresal)
**	14-oct-1992 (larrym)
**	    Added IIsqlcdInit to the appropirate place.  This is called 
**	    instead of IIsqInit if the user is using SQLCODE.  Added 
**	    functions gen_sqlcode and gen_diaginit.  
**	    Also updated Copyright notice and started using new history 
**	    format.
**	13-jan-1993 (larrym)
**	    fixed generation of SQLSTATE (used to gen &SQLSTATE).
**	11-feb-1993 (larrym)
**	    fixed bug with SQLSTATE (string descriptor work required 
**	    re-work of gen_diaginit)
**	08-mar-1993 (larrym)
**	    added new function IILQcnConName (IISQCNNM) for connection names.
**	13-mar-1996 (thoda04)
**	    added function parameter prototypes for local functions.
**	12-jun-1996 (sarjo01)
**	    Bug 76373: Use __declspec(dllimport) instead of extern for NT
**	    to assure correct linkage with DLL. __declspec(dllimport) is
**	    how GLOBALREF is defined for NT.
**	02-feb-1998 (somsa01)  X-integration of 433318 from oping12 (somsa01)
**	    In gen_sqlca(), the "__declspec(dllimport)" added from the
**	    previous change is not a C++ statement. Therefore, if we are
**	    precompiling to a C++ file, add an 'extern "C"' in front of the
**	    "__declspec(dllimport)" to tell the compiler that this is a C
**	    statement. (Bug #86893)
**	18-Dec-1997 (gordy)
**	    Added support for SQLCA host variables and multi-threaded
**	    applications.
**	6-Jun-2006 (kschendel)
**	    Added IIsqDescInput.
*/

static bool	gen_sqdeclared = FALSE;

/*
** Table of II calls - 
**     Correspond to II constants, the function to call to generate arguments, 
** the flags needed and the number of arguments.  For C there is very little
** argument information needed, as will probably be the case for regular 
** callable languages. For others (Cobol) this table will need to be more 
** complex and the code may be split up into partially table driven, and 
** partially special cased.
**
** This is the ESQL-specific part of the table
*/

/*
**  Some of these declarations are duplicated in eqesql.h.  Why?
*/
i4      gen__II(i4 func);
i4      gen_all_args(i4 flags, i4  numargs);
i4      gen_var_args(i4 flags, i4  numargs);
i4      gen_io(i4 ret_set, i4  args);
i4      gen_IIdebug(void);
void    gen__obj(i4 ind, char *obj);

GLOBALDEF GEN_IIFUNC	gen__sqltab[] = {
  /* Name */		/* Function */	/* Flags */	/* Args */
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
    { ERx("IIsqStop"),		gen_all_args,	0,		1 }, /* 10 */
    { ERx("IIsqUser"),		gen_all_args,	0,		1 },
    { ERx("IIsqPrint"),		gen_all_args,	0,		1 },
    { ERx("IIsqMods"),		gen_all_args,	0,		1 },
    { ERx("IIsqParms"),		gen_all_args,	0,		5 }, 
    { ERx("IIsqTPC"),		gen_all_args,	0,		2 }, /* 15 */
    { ERx("IILQsidSessID"),	gen_all_args,	0,		1,},
    { ERx("IIsqlcdInit"),	gen_all_args,	0,		2 },
    { ERx("IILQcnConName"),	gen_all_args,	0,		1 },
    { (char *)0,		0,		0,		0 },
    { (char *)0,		0,		0,		0 }, /* 20 */

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

  /* FRS Dynamic routines # 31 */
    { ERx("IIFRsqDescribe"),	gen_all_args,	0,		6 },
    { ERx("IIFRsqExecute"),	gen_all_args,	0,		4 },
    { (char *)0,		0,		0,		0 }
};

/*
+* Procedure:	gen_sqlca
** Purpose:	Code generation for SQL[C|D]A (include files and IF statements).
** Parameters:
**	flag    - i4  -    sqcaSQL	- include the SQLCA file
**			   sqcaSQLDA	- include the SQLDA file
**			   sqcaGEN	- generate the IF stmts, general case.
**			   sqcaSQPROC	- generate IFs inside exec-proc loop.
**			   sqcaGETEVT	- generation after GET DBEVENT stmt.
** Return Values:
-*	None
** Notes:
**   1. File inclusion is only done for SQLCA.  We will use the SQLCA include 
**	file for the SQLCA definition as well as for external definitions of 
**	II calls and II work space if the language requires it (PL/I, COBOL, 
**	FORTRAN).  Languages that bring in all procedures that are declared 
**	external, even if not used, can do something else.  
**	Example:
**	Currently only UNIX/F77 does this.  We have the include file have in it:
**		#ifdef IIFORMS
**			external integer IIformfuncs
**		#endif
**	Upon closure of file in EQF then rename file from file.f to file.F.
**	Document that the user must use:
**		f77 -DIIFORMS file.F
**	to bring in the forms system too.
**   2. IF statement generation is done based on the contents of the external
**      esqlca_store.  Non-COBOL programs can generate a sequence of 
**	IF-GOTO's, while COBOL will have to do something about IF-THEN-ELSE's
**	and should it print a period.
**   3. Code is generated differently for results from EXECUTE PROCEDURE
**	statements (sqcaSQPROC).  SQLMESSAGE events are handled only inside
**	the exec-proc loop.  NOTFOUND events are handled only outside this loop.
**	GOTO actions must also send a break.
**   4. The GET DBEVENT statement must not be followed by the WHENEVER 
**	DBEVENT condition (to avoid recursion of event handlers).
**
** History:
**	20-apr-1991 (kathryn)
**	    Generate SqlMessage code for all executable statements rather
**	    than just "execute procedure".  Procedures executed due to rule 
**	    firing will now have WHENEVER SQLMESSAGE handling for the last 
**	    message generated by the procedure.
**	02-feb-1998 (somsa01)  X-integration of 433318 from oping12 (somsa01)
**	    In gen_sqlca(), the "__declspec(dllimport)" added from the
**	    previous change is not a C++ statement. Therefore, if we are
**	    precompiling to a C++ file, add an 'extern "C"' in front of the
**	    "__declspec(dllimport)" to tell the compiler that this is a C
**	    statement. (Bug #86893)
**	18-Dec-1997 (gordy)
**	    Added support for SQLCA host variables and multi-threaded
**	    applications.
*/

void
gen_sqlca( flag )
i4	flag;
{
    static	char	*sqconds[] = {
		/* Not Found */		ERx("(%s%ssqlcode == 100) "),
		/* SqlError */		ERx("(%s%ssqlcode < 0) "),
		/* SqlWarning */	ERx("(%s%ssqlwarn.sqlwarn0 == 'W') "),
		/* SqlMessage */	ERx("(%s%ssqlcode == 700) "),
		/* Dbevent */		ERx("(%s%ssqlcode == 710) ")
				    };
    char	sqcond[ 128 ];
    register	SQLCA_ELM	*sq;
    register	i4		i;
    bool			need_else = FALSE;

    LOCATION	loc; 
    char	buf[MAX_LOC +1];
    char	*lname;

    switch (flag)
    {
      case sqcaSQL:
	/* Gen include file for SQLCA - C makes it external */
#ifdef IBM370
	gen_include( ERx("<eqsqlca.h>"), (char *)0, (char *)0 );
#else
	STcopy( ERx("eqsqlca.h"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	gen_include( lname, (char *)0, (char *)0 );
#endif /* IBM370 */

	if ( eq->eq_flags & EQ_MULTI )
	{
	    /*
	    ** Multi-threaded applications require a function
	    ** to referrence the sqlca for a given thread.
	    ** Declare the function and define a macro to
	    ** convert sqlca references into function calls.
	    */
	    gen__obj( FALSE, ERx("    IISQLCA *IIsqlca();\n") );
	    gen__obj( FALSE, ERx("# define sqlca (*(IIsqlca()))\n") );
	}
	else
	{
#ifdef	VMS
	    if (eq->eq_flags & EQ_COMPATLIB)
	    {
		gen__obj( FALSE,
		    ERx("# ifdef extern              /* defined in <compat.h> */\n") );
		gen__obj( FALSE, ERx("#   undef extern\n") );
		gen__obj( FALSE,
		    ERx("    extern IISQLCA sqlca;   /* SQL Communications Area */\n") );
		gen__obj( FALSE,
		    ERx("#   define extern globalref /* as in <compat.h> */\n") );
		gen__obj( FALSE, ERx("# else\n") );
		gen__obj( FALSE,
		    ERx("    extern IISQLCA sqlca;   /* SQL Communications Area */\n") );
		gen__obj( FALSE, ERx("# endif\n") );
	    } else
#endif /* VMS */
#ifdef NT_GENERIC
	if (eq_islang(EQ_CPLUSPLUS))
	    gen__obj( FALSE,
		ERx("    extern \"C\" __declspec(dllimport) IISQLCA sqlca;   /* SQL Communications Area */\n") );
	else
	    gen__obj( FALSE,
		ERx("    __declspec(dllimport) IISQLCA sqlca;   /* SQL Communications Area */\n") );
#else
	    gen__obj( FALSE,
		ERx("    extern IISQLCA sqlca;   /* SQL Communications Area */\n") );
#endif
	}
	gen_sqdeclared = TRUE;
	return;

      case sqcaSQLDA:
	/* Gen include file for SQLDA - C makes it external */
#ifdef IBM370
	gen_include( ERx("<eqsqlda.h>"), (char *)0, (char *)0 );
#else
	STcopy( ERx("eqsqlda.h"), buf );
	NMloc( FILES, FILENAME, buf, &loc );
	LOtos( &loc, &lname );
	gen_include( lname, (char *)0, (char *)0 );
#endif /* IBM370 */
	return;

      case sqcaGEN:
      case sqcaSQPROC:
      case sqcaGETEVT:
	if (!gen_sqdeclared)	/* Avoid sqcaPROC generation */
	    return;
	for (i = sqcaNOTFOUND, sq = esqlca_store; i < sqcaDEFAULT; i++, sq++)
	{
	    if (sq->sq_action == sqcaCONTINUE)
		continue;

	    /* never generate notfound code in proc-exec loop */
	    if (i == sqcaNOTFOUND && flag == sqcaSQPROC)
		continue;
	    /* Don't generate event handling after GET DBEVENT statement */
	    if (i == sqcaEVENT && flag == sqcaGETEVT)
		continue;

	    /* [else] if (sqlca cond) - else required for return from CALL */
	    if (need_else)
		gen__obj( TRUE, ERx("else ") );
	    else		/* First one */
		need_else = TRUE;
	    gen__obj( TRUE, ERx("if ") );

	    if ( sqlca_var )
		STprintf( sqcond, sqconds[i], sqlca_var, ERx("->") );
	    else
		STprintf( sqcond, sqconds[i], ERx("sqlca"), ERx(".") );
	    gen__obj( TRUE, sqcond );

	    /*
	    ** we'll be generating brackets for the following cases: 
	    **  1. GOTO in an exec-proc loop
	    **	2. Generating code for EVENT 
	    */
	    if (sq->sq_action == sqcaGOTO && flag == sqcaSQPROC)
	    {
	    	gen__obj( TRUE, ERx("{\n"));
	    	gen_do_indent( 1 );
		gen__II( IIBREAK );
		if (i == sqcaEVENT)
		{
		    gen__obj( TRUE, ERx(";\n") );
		    arg_int_add(1);
		    arg_int_add(0);
		    gen__II( IIEVTSTAT );
		}
		gen__obj( TRUE, ERx(";\n") );
	    }
	    else if (i == sqcaEVENT)
	    {
	    	gen__obj( TRUE, ERx("{\n"));
	    	gen_do_indent( 1 );
		arg_int_add(1);
		arg_int_add(0);
		gen__II( IIEVTSTAT );
		gen__obj( TRUE, ERx(";\n") );
	    }
	    else
	    {
		gen__obj( TRUE, ERx("\n") );
		gen_do_indent( 1 );
	    }

	    /* do action */
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
		    /* Ignore case in non-C */
		    if (STbcompare(sq->sq_label, 0, ERx("sqlprint"), 0, TRUE) 
			== 0)
		    {
			gen_sqarg();
			gen__II(IISQPRINT);
		    }
		    else
		    {
			gen__obj( TRUE, sq->sq_label );
			gen__obj( TRUE, ERx("()") );
		    }
		}
		else
			gen__obj( TRUE, ERx("II?call()") );
		break;
	    }
	    gen__obj( TRUE, ERx(";\n"));
	    gen_do_indent( -1 );
	    /* Close bracket if needed */
	    if ((sq->sq_action == sqcaGOTO && flag == sqcaSQPROC) ||
		 i == sqcaEVENT)
		gen__obj( TRUE, ERx("}\n"));
	}
	return;
    }
}

/*
+* Procedure:	gen_sqarg
** Purpose:	Add the address of the sqlca to the current argument list.
** Parameters:
**	None.
** Return Values:
-*	None.
** History:
**	18-Dec-1997 (gordy)
**	    Added support for SQLCA host variables and multi-threaded
**	    applications.
*/

i4
gen_sqarg()
{
    if (gen_sqdeclared)
	arg_str_add( ARG_RAW, sqlca_var ? sqlca_var : ERx("&sqlca") );
    else
	arg_str_add( ARG_CHAR, (char *)0 );
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
*/
void
gen_sqlcode()
{
    if (eq->eq_flags & EQ_SQLCODE)
        arg_str_add( ARG_RAW, ERx("&SQLCODE") );
    else
	arg_str_add( ARG_CHAR, (char *)0 );
}

void
gen_diaginit()
{
    gen__obj( TRUE, ERx("IIsqGdInit(32,SQLSTATE)"));
    out_emit( ERx(";\n") );
}
