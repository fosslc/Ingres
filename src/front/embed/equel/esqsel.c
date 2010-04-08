/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<si.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<eqstmt.h>
# include	<equtils.h>
# include	<eqgen.h>
# include	<eqlang.h>

/**
+*  Name: esqselect.c - Bulk/Singleton SELECT/RETRIEVE utility buffer routines.
**
**  Description:
**	The Data General Gateway needs to know if a SELECT is Bulk or
**	Singleton; it distinguishes between the two by inspecting a bit
**	in the initial control block, which we must set.
**	As the two kinds of statements are differentiated only by the
**	presence or absence of a BEGIN/END block following the statement,
**	we must buffer the output until we know which kind it is (ie, until
**	after the statement proper has been parsed).  Note that the flag bit
**	is set in IIqry_start, which will be called by the first IIwritio.
**	We force the SINGLETON bit to be set by calling IIsqMods(IImodSINGLE);
**	the default is that this is a bulk select.
**
**	There are four cases:
**	    1. RETRIEVE
**	    2. REPEAT RETRIEVE
**	    3. SELECT
**	    4. REPEAT SELECT
**
**	1, 2. In EQUEL, retrieves are ALWAYS Bulk retrieves, as the semantics
**	    of RETRIEVE are that we always generate a loop and retrieve all
**	    the rows, even if the user didn't explicitly specify a loop.
**	    (In which case s/he will end up with the last row -- this will be
**	    documented soon).  If the user wants only the first row then s/he
**	    must explicitly code a loop and include an ENDRETRIEVE statement.
**	    As BULK is the default, EQUEL does not need to buffer its output
**	    and thus never calls this code.  Whether or not the query is
**	    repeated is immaterial.
**
**	4.  No current gateways support REPEAT queries, DG has no
**	    corresponding notion, and it's not at all clear what meaning
**	    a repeat query has for a foreign backend, nor what a gateway
**	    should do with such a creature.  Therefore we are NOT supporting
**	    repeat queries for gateways at this time.
**
**	    The INGRES backend doesn't need to know the difference between
**	    Singleton and Bulk SELECTs, whether or not they are repeated.
**	    The semantics of the two are different, so currently the
**	    preprocessor generates code differently for the two cases.
**	    As REPEATED queries already buffer their output we don't want
**	    to introduce more buffering in here unnecessarily (although it
**	    should work; see the cursor code -- eqcursor.c), so REPEAT SELECTs
**	    will continue to generate the current code and avoid this module.
**
**	    If REPEATED SELECT ever needs to be supported for gateways then
**	    we will deal the problem then.  If there's any justice in the
**	    world, by then we'll be building parse trees or something, and
**	    won't have to worry about buffering output.
**
**	3.  Thus the only case we need to deal with is non-repeated SELECTs.
**	    As the default is BULK SELECT we will call IIsqMods only for the
**	    singleton case (although we'll have to go through this module
**	    to buffer the query until we find out which kind of select it is).
**
**  Defines:
**	esl_query( flag )		- Start/stop/end buffering a query
**	esl_clean()			- Clean up after syntax error
**  (Local)
**	esl_buffer( ch )		- Buffer a character
**
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	20-aug-1987	- Written (mrw)
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      06-Feb-96 (fanra01)
**          Made data extraction generic.
**      15-mar-96 (thoda04)
**          Added function parameter prototypes for esl_buffer(), esl_clean()
**	18-dec-96 (chech02)
**	    Fixed redefined symbol errors: esl_strtable.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define ESL_BUFSIZE	400
# define STATIC

GLOBALREF STR_TABLE	*esl_strtable;	/* the text of the associated query */
static	OUT_STATE
		*esl_outstate ZERO_FILL;
static	OUT_STATE
		*esl_oldstate ZERO_FILL;

STATIC i4	esl_buffer(char ch);
STATIC i4	esl_clean(void);

/*{
** Name: esl_query - Set the query of a cursor to the passed query.
** Inputs:
**	init	- i4		- Start/Stop saving the query
** Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		None.
** Description:
**	- If "init == ESL_START_QUERY", then allocate a new string table,
**	  and switch emit-output state to go to it.
**	- If "init == ESL_SNGL/BULK_STOP_QUERY", then switch back to normal
**	  output and dump the saved query.
**	- When this is called the grammar is about to parse the user's
**	  SELECT query.  The actual query is not sent till ESL_..._STOP_QUERY
**	  time, therefore the query must be buffered.  By switching the
**	  code-emitter state to use the output state we indirectly buffer
**	  the complete query (within its host language calls) into the current
**	  cursor's esl_strtable string table (see esl_buffer).
** Side Effects:
**	Redirects output and associates it with this query.
** Generates:
**	Nothing.
*/

i4
esl_query( init )
i4	init;
{
    switch (init)
    {
      case ESL_START_QUERY:				/* Start saving query */
        /*
	** Allocate new (or clear old) string table for query,
	** and force output into i
	*/
	if (esl_strtable == NULL)
	    esl_strtable = str_new( ESL_BUFSIZE );
	esl_outstate = out_init_state( esl_outstate, esl_buffer );
	esl_oldstate = out_use_state( esl_outstate );
	break;

      case ESL_SNGL_STOP_QUERY:				/* End current query */
       /* Go back to using default output state */
	_VOID_ out_use_state( esl_oldstate );
	arg_int_add( IImodSINGLE );
	gen_call( IISQMODS );
	esl_oldstate = OUTNULL;
	str_chget( esl_strtable, NULL, out_put_def );	/* Dump the query */
	str_free( esl_strtable, (char *)0 );
	break;

      case ESL_BULK_STOP_QUERY:				/* End current query */
       /* Go back to using default output state */
	_VOID_ out_use_state( esl_oldstate );
	esl_oldstate = OUTNULL;
	str_chget( esl_strtable, NULL, out_put_def );	/* Dump the query */
	str_free( esl_strtable, (char *)0 );
	break;
    }
    return TRUE;
} /* esl_query */

/*{
+*  Name: esl_clean - Reset SELECT buffer state on syntax error
**
**  Description:
**    Resets output state and flushes the buffer.
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    0 -	No current meaning.
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	21-aug-1987 - Written (mrw)
*/

i4
esl_clean()
{
   /* Go back to using default output state */
    _VOID_ out_use_state( esl_oldstate );
    esl_oldstate = OUTNULL;
    str_chget( esl_strtable, NULL, out_put_def );	/* Dump the query */
    str_free( esl_strtable, (char *)0 );
    return 0;
}

/*{
** Name: esl_buffer - (local) Buffer characters that are written by the code
**				emitter to a select string table buffer.
** Inputs:
**	ch - char 	- Character to buffer.
** Outputs:
**	Returns:
**		i4 	- Zero (unused).
**	Errors:
**		None.
** Description:
**	Indirectly called via the code emitter state.
** Side Effects:
**	- Non-nul chars are just added to the query buffer.
*/

STATIC i4
esl_buffer( char ch )
{
    if (ch != '\0')
	str_chadd( esl_strtable, ch );
    return 0;
} /* esl_buffer */
