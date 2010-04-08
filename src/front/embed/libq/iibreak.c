/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<ex.h> 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>
# include	<fe.h>

/**
+*  Name: iibreak.c - Routines to interrupt the current query.
**
**  Defines:
**	IIbreak		- Break out of a RETRIEVE (or a query)
**	IIresync	- Try to handle interrupts
**
**  Notes:
**	1. IIresync still calls IIbreak.  Unfortunately no work has been
**	   done to make one the interrupt handler, and the other a RETRIEVE
**	   loop break out statement.
-*
**  History:
**	14-oct-1986	- Converted to 6.0 (mrw)
**	15-oct-1986	- Added code from old iiresync.c (ncg)
**	24-aug-1990 (barbara)
**	    When breaking out of a retrieve/select loop, set the II_L_XACT
**	    flag.  The loop statement will have started a transaction,
**	    but IIqry_end hasn't picked up this piece of information because
**	    a GCA_IACK doesn't send transaction information.
**	26-sep-1990 (kathryn) Bug 33512
**	    When breaking out of a retrieve/select loop, set the II_L_XACT
**	    only when Language is SQL.
**	05-dec-1990 (barbara)
**	    Refixed bug 21554 (INQUIRE_INGRES TRANSACTION).  Check for dml
**	    was never succeeding because IIqry_end had reset dml.
**	15-aug-1991 (kathryn) Bug 39274
**	    Integrate change from 3/90 which did not make it into r63p line.
**          03-apr-1990 - Pop session id stack if breaking out of retrieve or
**          dbproc execution loop (bjb).
**	21-nov-1996 (canor01)
**	    Suppress interrupt handling during calls to IIbreak().
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of fe.h to eliminate gcc 4.3 warnings.
**/

/*{
+*  Name: IIbreak - Break out of a RETRIEVE loop or interrupt query.
**
**  Description:
**	Terminate the current RETRIEVE immediately.  Accomplished by
**	interrupting INGRES and resetting global flags.  Don't reset
**	ii_er_num because errors causing the break out may be INQUIREd
**	about by the embedded program.  IIqry_end() is called to put 
**	end-of-query status information into SQLCA.
**
**	IIbreak is called to as the result of ENDRETRIEVE, or asynchronously
**	from a exception handler.  If we were only called from an ENDRETRIEVE
**	(ie, synchronously) then we should optimize here and test to see
**	if there is any more tuple data before sending an interrupt:
**		if (IIcgc_more_data(tuples))
**			send interrupt;
**		else 
**			IIqry_end();
**	However, we cannot guarantee that we are called from an ENDRETRIEVE,
**	and we may even be in the middle of a blocking read (deeper within GCA).
**	Consequently we cannot call IIcgc_more_data, because that call may
**	cause a read ahead, which will post a second blocking read - and two
**	simultaneous blocking reads are not allowed by GCA.
**	
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    None.
**	Errors:
**	    Whatever IIcgc_break returns.
-*
**  Side Effects:
**	Resets any local flags associated with current query.
**	
**  History:
**	17-aug-1985 - Added multi-channel networking parameter to IN calls (mmb)
**	14-oct-1986 - Modified for 6.0 (mrw)
**	10-aug-1987 - Modified to use GCA. (ncg)
**	03-may-1988 - Turns off procedure flags too. (ncg)
**	02-aug-1988 - Added debugging trace generation. (ncg)
**	19-may-1989 - Changed names of globals for multiple connects. (bjb)
**	24-aug-1990 (barbara)
**	    When breaking out of a retrieve/select loop, set the II_L_XACT
**	    flag.  The loop statement will have started a transaction,
**	    but IIqry_end hasn't picked up this piece of information because
**	    a GCA_IACK doesn't send transaction information.
**	05-dec-1990 (barbara)
**	    Refixed bug 21554 (INQUIRE_INGRES TRANSACTION).  Check for dml
**	    was never succeeding because IIqry_end had reset dml.
**	01-nov-1991 (kathryn)
**          When Printqry is set call IIdbg_response rather than IIdbg_dump.
**          The query text is now dumped when query is sent to DBMS.
**          IIdbg_response will dump the query response timing info, if it has
**          not already done so.
**      28-July-1994 (Timt) Bug 63636
**          When singleton select turn off II_L_SINGLE flag.
**	21-nov-1996 (canor01)
**	    Do not let IIbreak() be interrupted.  It can end up sending
**	    more than one GCA_ATTENTION, but only waiting for a single GCA_IACK.
**      27-oct-2004 (huazh01)
**          Do not send out a GCA_INTALL message if user interrupts 
**          "help table/view ..." within ISQL session. This prevents server 
**          to rollback TX if session enables "set sesson with on_error =
**          rollback TX". b64679, INGCBT545.
*/

void
IIbreak()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		rows;
    i4		sv_dml;

    /* Do not let break processing be interrupted */
    EXinterrupt(EX_OFF);

    /*
    ** If we are in the middle of a data retrieval/procedure loop this will
    ** force the trace text to be flushed.
    */
    IIdbg_response( IIlbqcb, TRUE );

    if ((IIlbqcb->ii_lq_flags & II_L_RETRIEVE) ||
        (IIlbqcb->ii_lq_flags & II_L_HELP))	/* In a retrieve ? */
    {
	rows = IIlbqcb->ii_lq_rowcount;			/* Get number looped */
	IIcgc_break(IIlbqcb->ii_lq_gca, GCA_ENDRET);	/* Interrupt */
	IIlbqcb->ii_lq_curqry = II_Q_DEFAULT;		/* No query active */
	sv_dml = IIlbqcb->ii_lq_dml;			/* IIqry_end will clr */
	IIqry_end();					/* To release SQLCA */
	IIlbqcb->ii_lq_rowcount = rows;			/* Real row count */
	IILQspSessionPop( thr_cb );
	if (sv_dml == DB_SQL)	
		IIlbqcb->ii_lq_flags |= II_L_XACT;	/* Xact started */
    }
    else					/* Not a retrieve */
    {
	if (IIlbqcb->ii_lq_flags & II_L_CONNECT)
	    IIcgc_break(IIlbqcb->ii_lq_gca, GCA_INTALL);/* Interrupt */
	IIlbqcb->ii_lq_curqry = II_Q_DEFAULT;		/* No query active */
	IIqry_end();					/* To release SQLCA */
	IIlbqcb->ii_lq_rowcount = GCA_NO_ROW_COUNT;	/* Not applicable */
	if (IIlbqcb->ii_lq_flags & II_L_DBPROC)
	    IILQspSessionPop( thr_cb );
	/* After a GCA_INTALL we don't know the xact state */
    } /* If retrieve or not */
    IIlbqcb->ii_lq_flags &=		/* No repeat, retrieve or procedure */
		~(II_L_RETRIEVE|II_L_REPEAT|II_L_DBPROC);
    /* If singleton SELECT, reset the flag.                   63636   */
    if (IIlbqcb->ii_lq_flags & II_L_SINGLE)
        IIlbqcb->ii_lq_flags &= ~II_L_SINGLE;

    /* resume normal interrupt processing */
    EXinterrupt(EX_RESET);
}

/*{
+*  Name: IIresync - Resynchronize communication channels after an interrupt.
**
**  Description:
**	If the caller passed the "-e" flag to INGRES then this routine is
**	called indirectly by the IN and EX routines to clean up on the current
**	query.  All it does is call IIbreak. In the future when RETRIEVE break
**	out and interrupts are separated then IIresync will provide a single
**	entry point into interrupt handling.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    None
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	dd-mmm-yyyy - text (init)
*/

void
IIresync()
{
    IIbreak();
}
