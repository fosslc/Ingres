# include 	<compat.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<ex.h>
# include	<eqsym.h>
# include	<eqgen.h>
# include	<equel.h>
# include	<me.h>
# include	<ereq.h>

/*
+* Filename:	OUT.C
** Purpose:	Final code emitter. Does nothing but write out characters,
**	 	and keep track of new lines.
**
** Defines: 	out_init_state( state, func )	- Initialize second out state.
**		out_use_state( state )		- Use other state.
**		out_emit( s )			- Emit string.
** 		out_put_def( ch )		- Default output routine.
**		out_cur				- Current OUT_STATE.
**		out_erline( l )			- Increment error line count.
-*		out_suppress( 0/1 )		- Suppress output.
** History:
**		19-nov-1984 - 	Written (ncg)
**		13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**      12-Dec-95 (fanra01)
**              Added definitions for extracting data for DLLs on windows NT
**      18-mar-96 (thoda04)
**              Corrected cast of MEreqmem()'s tag_id (1st parm).
**              Cleaned up warnings on out_init_state()'s out_func parm.
**	08-oct-96 (mcgem01)
**		Global data moved to eqdata.c
**		
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	8-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets
**	    which can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
*/

/* 
** Local Output State Manager -
**    The default output state to be used in most cases contains the default
** output function (to the preprocessed file), and the current state pointer
** to be used.  In most cases the current state will use the default state.
*/
GLOBALREF		OUT_STATE	out_default;

/* May be turned off by out_suppress */
static		i4		out_nowrite = FALSE;

/* Current output state - parts of which are known outside */
GLOBALREF	OUT_STATE	*out_cur;

/*
+* Procedure:	out_init_state 
** Purpose:	Give the caller a pointer to a new output state vector,
**		and reset the fields.
**
** Parameters: 	out_s	 - OUT_STATE * - Pointer to an Out state,
**		out_func - i4  (*)()   - Function to use when using the state.
-* Returns: 	OUT_STATE *	       - Pointer to an Out state.
**
** This routine calls MEreqmem() if null argument, and assumes the caller 
** will keep the state vector around for furthur use.  
*/

OUT_STATE	*
out_init_state( OUT_STATE *out_s, i4  (*out_func)(char) )
{
    OUT_STATE	*out;

    if ((out = out_s) == OUTNULL)
    {
	if ((out = (OUT_STATE *)MEreqmem((u_i2)0, (u_i4)sizeof(OUT_STATE),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
	    er_write(E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(sizeof(OUT_STATE)), 
		    ERx("out_init_state()"));
	}
    }

    /* Reset contents of new state */
    if (!out_func)
    {
	er_write( E_EQ0336_outFUNC, EQ_ERROR, 0 );
	out->o_func = out_put_def;
    }
    else
	out->o_func = out_func;
    out->o_charcnt = 0;
    out->o_lastch = '\0';
    return out;
}


/*
+* Procedure:	out_use_state 
** Purpose:	Use a caller's output state.  
**
** Parameters: 	out_s - OUT_STATE * - Pointer to an Out state.
-* Returns: 	None
**
** This passed state should have gone through out_init_state() already.  If the
** passed state is null then use the default state - out_default.
*/

OUT_STATE *  
out_use_state( out_s )
OUT_STATE	*out_s;
{
    OUT_STATE	*old;

    old = out_cur;
    if ((out_cur = out_s) == OUTNULL)
	out_cur = &out_default;
    return old;
}

/*
+* Procedure:	out_emit 
** Purpose:	Emit string and keep track of things.
**
** Parameters: 	str - char * - String to emit to output file.
-* Returns: 	None
*/

void
out_emit( str )
char	*str;
{
    register OUT_STATE 	*out;
    register char	*cp;
    i4			iter, byte_cnt;

    out = out_cur;
    for (cp = str; *cp != '\0'; cp++)
    {
	if (*cp == '\n')
	    out->o_charcnt = -1;
	(*out->o_func)( *cp );
	out->o_lastch = *cp;
	out->o_charcnt++;
	if (CMdbl1st(cp))	/* Put out the second byte */
	{
	    byte_cnt = CMbytecnt(cp);
	    for (iter = byte_cnt; iter > 1; iter--)
	    {
	      cp++;
	      (*out->o_func)(*cp);
	      out->o_charcnt++;
	    }
	}
    }
}

static i4	out_line = 0;		/* If output is to stdout */

/*
+* Procedure:	out_erline 
** Purpose:	Error writer calls this to count lines to stdout.
**
** Parameters:	l - i4  - Number of error lines.
-* Returns:	None
*/

void
out_erline( l )
i4	l;
{
    out_line += l;
}

/*
+* Procedure:	out_put_def 
** Purpose:	What, if not output ?  Called indirectly via an Out state.
**
** After a newline has been printed, check for screenful of lines if the
** standard output flag is on.
**
** Parameters: 	ch - char - Character to put out.
-* Returns: 	i4 	  - Zero (unused).
*/

i4
out_put_def( char ch )
{
    if (out_nowrite)		/* Suppress writing */
	return 0;

    if (ch != '\0')
	SIputc( ch, eq->eq_outfile );
    /* If listing file also has output then dump character */
    if (eq->eq_flags & EQ_LSTOUT)
    {
	static	FILE	*orig_o = NULL;
	static	FILE	*orig_l = NULL;

	/* For each user's input file reset list and output files */
	if (orig_l != eq->eq_listfile)
	{
	    orig_o = eq->eq_outfile;
	    orig_l = eq->eq_listfile;
	}
	/* 
	** As long as we're writing to the original output file we can only
	** have moved down the main file, and direct 'inline' inclusions.
	*/
	if (eq->eq_outfile == orig_o && ch != '\0')
	    SIputc( ch, eq->eq_listfile );
    }

    /* 
    ** If the last character was a newline, then check for screenful.
    ** Make sure that there is no user input (otherwise lines will be off),
    ** and that the current file is to output (not the output of an include 
    ** file - either all include files are closed, or direct inline 
    ** inheritance).
    */
    if (ch == '\n' && (eq->eq_flags & EQ_STDOUT) && 
	 !(eq->eq_flags & EQ_STDIN) && eq->eq_outfile == stdout)
    {
	char		retbuf[256];

	if (++out_line >= 20)
	{
	    SIprintf( ERget(F_EQ0033_eqPRMPT) );
	    SIflush( stdout );		
	    if (SIgetrec(retbuf, 255, stdin) != OK || 
		    !CMcmpnocase(retbuf, ERget(F_EQ0034_eqQUIT)))
		EXsignal( EXEXIT, 0 );
	    out_line = 0;
	}
    }
    return 0;
}

/*
+* Procedure:	out_suppress 
** Purpose:	Supress or release output.
**
** Parameters:	flag - i4  - 0/1 : release/suppress.
-* Returns:	None
*/

void
out_suppress( flg )
i4	flg;
{
    out_nowrite = flg;
}
