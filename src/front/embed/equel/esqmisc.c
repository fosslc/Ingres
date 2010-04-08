/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<si.h>		/* needed for equel.h */
# include	<equel.h>	/* needed for *eq structure */
# include	<eqsym.h>
# include	<eqgen.h>
# include	<eqrun.h>
# include       <eqscan.h>	/* needed for eqesql.h */
# include       <ereq.h>
# include       <ercl.h>

/* Special ESQL overlay */
# include       <eqesql.h>
/**
+*  Name: esqmisc.c - Miscellaneous ESQL-only routines.
**
**  Defines:
**	esq_init	- Generate error trap routine.
**	esq_repeat	- Indirect interface for ESQL REPEAT queries.
**
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	31-mar-1987	- Written (mrw)
**	03-apr-1989	- Added routine to generate IIsqMods (teresa)
**	14-oct-1992 (larrym)
**	    Added new include equel.h to get access to EQ_STATE global.
**	    Needed to see which type of IISQINIT to generate.  If user
**	    is using FIPS SQLSTATE or SQLCODE, then we need to gen 
**	    IISQSTINIT instead.
**	16-dec-1992 (larrym)
**	    Added check for INCLUDE SQLCA before generating code for SQLCODE.
**	    Had to put esq_struct in eqrun.h (it was already global, just not
**	    defined anywhere outside of the grammers).  Added declaration 
**	    of esq.
**	07-jun-1993 (kathryn)
**	    Changed esq_sqmods() to take one paramter.  This routine
**	    previously generated IIsqMods(1).  It will now take an integer
**	    arg and generate IIsqMods(arg).
**	22-jun-1993 (kathryn)
**	    Moved esq_sqmods from this file to eqmisc.c.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	15-mar-1996 (thoda04)
**	    Cleaned up warning that esq_repeat() returned nothing.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* structure definition is in front.hdr.hdr.eqesql.h */
GLOBALREF struct esq_struct *esq;

/*
+* Procedure:	esq_init
** Purpose:	Send run-time initializer to trap errors.
** Parameters:
**	None
** Return Values:
-*	None
** Generates:	IIsqGdInit( (int)32, &SQLSTATE );	- if using SQLSTATE
**		IIsqInit( &sqlca );			- to trap errors
**	or	IIsqlcdInit( &sqlca, &SQLCODE ); 	- If using SQLCODE.
*/

void
esq_init()
{

    /* 
    ** We want to do this everywhere we do esq_init, so I put it here 
    ** but we also want to do it for a SET_INGRES ( SESSION = x ); which
    ** doesn't do an esq_init.  So the gen_diaginit and gen_call( IISQGDINIT ) 
    ** are also done in the main grammer (esqlgram.my)
    */
    if (eq->eq_flags & EQ_SQLSTATE) 
    {
	gen_diaginit();	/* gen the DIAGNOSTIC AREA Init routine */
    }

    gen_sqarg();	/* gen's the sqlca arg, if needed */
    if (eq->eq_flags & EQ_SQLCODE)
    {
	if (esq->inc & sqcaSQL)
	{
	    gen_sqlcode(); /* gen the (deprecated) SQLCODE arg, if needed */
    	    gen_call( IISQLCDINIT );
	}
	else
	{
	    /* no INCLUDE SQLCA seen yet, shut off SQLCODE support */
	    er_write( E_EQ050E_NOSQLCA, EQ_ERROR, 0);
	    eq->eq_flags &= ~EQ_SQLCODE;
    	    gen_call( IISQINIT );
	}
    }
    else
    {
    	gen_call( IISQINIT );
    }
}

/*
+* Procedure:	esq_repeat
** Purpose:	Interface for ESQL REPEATED queries to use the EQUEL REPEAT
**		query routines.
** Parameters:
**	sqflag	- i4	- TRUE generates a call to IIsqMods (may be NULL).
** Return Values:
-*	None
** Notes:
**	This routine is called indirectly from repeat.c via the global
**	(*dml->dm_repeat)().  It just generates the IIsqInit call and
**	optionally the IIsqMods call.  This is indirect so that EQUEL
**	won't need to link it in.
*/

i4
esq_repeat( sqflag )
i4	sqflag;
{
    esq_init();
    if (sqflag) esq_sqmods(IImodSINGLE);
    return(0);
}
