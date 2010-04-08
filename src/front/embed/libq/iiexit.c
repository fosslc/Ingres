/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<si.h>
# include	<gca.h>
# include	<generr.h>
# include	<erlq.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<lqgdata.h>

/**
+*  Name: iiexit.c - Exit INGRES, voluntarily or forcefully.
**
**  Defines:
**	IIexit
**	IIabort
**	IIlqExit
-*
**  History:
**	14-oct-1986 - Modified for 6.0 (mrw)
**	26-aug-1987 - Modified to use GCA. (ncg)
**	18-feb-1988 - To implement ATEX recovery feature in 6.0:
**		      Added new cleanup function IIlqExit.
**		      Added call to IIcsAllfree() in IIexit.
**		      Removed PCexit call from IIabort.  (marge)
**	22-may-1989 - Updated for multiple connections. (bjb)
**	14-dec-1989 - Updated for Phoenix/Alerters. (bjb)
**	30-apr-1990 - Clear ii_lq_gca once disconnected. (ncg)
**	07-may-1990	(barbara)
**	    Set II_Q_DISCON flag so that any pending events are ignored.
**	14-jun-1990 	(barbara)
**	    In IIexit, don't send IIbreak if association has already failed.
**	04-nov-90 (bruceb)
**		Added ref of IILIBQgdata so file would compile.
**	03-dec-1990 (barbara)
**	    Fixed bug 9160.  See IIexit history.
**	13-mar-1991 (barbara)
**	    In IIexit, close PRINTTRACE file, if any.
**	04/02/91 (dkh) - Added included of si.h for pc folks.
**      09-Jul-96 (fanra01)
**          Access violation during disconnection in Windows NT.  Memory is
**          accessed after it has been freed.
**	16-sep-96 (nick)
**	    Make aware of database procedures.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	24-mar-1999 (somsa01)
**	    Corrected declaration of IIexit().
**/

GLOBALREF LIBQGDATA	*IILIBQgdata;

/*{
+*  Name: IIexit - Exit from INGRES.
**
**  Description:
**	Terminate the INGRES backend.  The LIBQ flags are reset to a
**	default state.  We make all statements "close" via IIqry_end(),
**	although the only effect of this call from IIexit() is to reset the
**	query language to QUEL.
**
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	The connection with the server is ended.
**	The II_LQB_CB for the disconnected session is freed.  The
**	global IIlbqcb pointer is set to point to the static II_LBQ_CB.
**	Fields in the II_GLB_CB are reset.
**
**  History:
**	17-aug-1986 - Added multi-channel networking parameter to IN calls (mmb)
**	14-oct-1986 - Modified for 6.0 (mrw)
**	18-feb-1988 - Added IIcsAllfree() call for cursor memory cleanup (marge)
**	19-may-1989 - Updated for multiple connections. (bjb)
**	14-dec-1989 - Updated for Phoenix/Alerters. (bjb)
**	14-jun-1990 	(barbara)
**	    Don't send IIbreak if association has already failed.  This
**	    is part of the fix of bug 21832 whereby we don't exit the
**	    program on association failures.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	    When testing this bug fix uncovered problems with situations when
**	    SESSNONCUR error occurs.  Because the code wasn't going through
**	    IIqry_end, certain state was not getting initialized.
**	13-mar-1991 (barbara)
**	    Close PRINTTRACE file, if any.
**	19-apr-1991 (teresal)
**	    When freeing event queue, make sure event has been consumed. 
**	01-nov-1991 (kathryn)
**	    Trace file pointers now stored in new IILQ_TRACE structures
**	    of IIglbcb rather than directly in IIglbcb.
**	29-jul-92 (szeng)
**	    Move freeing session IILQsdSessionDrop(ses_id) after freeing
**	    event elements from list since IIlbqcb was freed in 
**	    IILQsdSessionDrop.
**	09-mar-1993 (larrym)
**	    Now all applications are multisession; modified to not check for
**	    single or multi connects.
**	15-jun-1993 (larrym)
**	    Changed initialize of ses_id below to IIlbqcb->ii_lq_sid from
**	    ii_gl_sstate.ss_idsave, to faciliate disconnection from any
**	    session, not just your current one (see changes in iisqcrun and
**	    iilqsess).
**	21-jun-1993 (larrym)
**	    Fixed bug 47881.  Removed check for in-between session 
**	    disconnect.  It's ok now to be in a non-connected state and
**	    issue a disconnect for a different session.
**	23-Aug-1993 (larrym)
**	    Fixed bug 54430.  We now reset any error condition in the 
**	    static session control block if all else goes ok so that
**	    an inquire_sql(errorno) will return ok.
**	10-nov-93 (robf)
**          Preserve savepassword state across connections, since thats
**	    when its used.
**      09-Jul-96 (fanra01)
**          Changed order of processing to clear the IIlbqcb->ii_lq_gca before
**          calling IIqry_end.  Ensures that the memory pointed to by
**          IIlbqcb->ii_lq_gca is not accessed by IIsqEnd.
**	16-sep-96 (nick)
**	    Ensure IIbreak() is called when in a db proc ; although this is
**	    QUEL, you can call procedures via 'exec sql' ( as some of our
**	    tools do ) and hence '## exit' needs to account for this.
*/

void
IIexit()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		ses_id = IIlbqcb->ii_lq_sid;

    thr_cb->ii_th_sessid = 0; /* Reset user-specified session id */
    /* Reset error information */
    IIlbqcb->ii_lq_error.ii_er_num = IIerOK;
    IIlbqcb->ii_lq_error.ii_er_other = 0;
    if (IIlbqcb->ii_lq_error.ii_er_smsg != (char *)0)
	IIlbqcb->ii_lq_error.ii_er_smsg[0] = '\0';
    IIlbqcb->ii_lq_curqry |= II_Q_DISCON;

    if ( IIlbqcb->ii_lq_flags & II_L_CONNECT  &&
    	 ! (IIlbqcb->ii_lq_flags & II_L_EXITING) )
    {
	IIlbqcb->ii_lq_flags |= II_L_EXITING;

	/*
	** Interrupt dbms if query in process, but not if
	** association has already failed.
	*/
	if ( IIlbqcb->ii_lq_flags & (II_L_RETRIEVE | II_L_DBPROC)  ||
	     IIlbqcb->ii_lq_curqry & II_Q_INQRY )
	{
	    IIbreak();
	}
	IIcgc_disconnect( IIlbqcb->ii_lq_gca, FALSE ); 
	IIlbqcb->ii_lq_flags &= ~II_L_CONNECT;
    }

    /*
    ** Clear up query stuff
    */
    IIqry_end();				/* Reset DML */

    /* Free event elements from list */
    while (IIlbqcb->ii_lq_event.ii_ev_list != (II_EVENT *)0)
    {
	IIlbqcb->ii_lq_event.ii_ev_flags |= II_EV_CONSUMED;
	IILQefEvFree( IIlbqcb );
    }

    /* Switch to default session a drop old session */
    IILQssSwitchSession( thr_cb, NULL );
    IIlbqcb = thr_cb->ii_th_session;
    IILQsdSessionDrop( thr_cb, ses_id);

    /* reset static error information */
    IIlbqcb->ii_lq_error.ii_er_num = IIerOK;
    IIlbqcb->ii_lq_error.ii_er_other = 0;
    if (IIlbqcb->ii_lq_error.ii_er_smsg != (char *)0)
	IIlbqcb->ii_lq_error.ii_er_smsg[0] = '\0';
}

/*{
+*  Name: IIabort - Force an INGRES exit.
**
**  Description:
**	IIabort is called when a non-recoverable error occurs.  There
**	may not be any current database sessions if the error occurred
**	during the initial stages of connection or if a query has been
**	issued before connection.  In this case the code returns.
**	Otherwise, all existing sessions are aborted.  
**
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    Nothing (Exits).
**	Errors:
**	    None.
**  Notes:
**  1.	In future we might allow aborts to affect only the
**	current session, leaving the non-current sessions connected.
**  2.  To saveguard against recursive calls to IIabort (which could
**	happen if the disconnect fails and IIabort is called through
**	the gca handler) we save the id of the last disconnected session.
-*
**  Side Effects:
**	1. The backend is killed and the front end exits.
**  History:
**	14-oct-1986 - Modified for 6.0 (mrw)
**	18-feb-1988 - Removed PCexit call from IIabort.  (marge)
**	14-jun-1989 - For multiple connections, abort all sessions. (barbara)
**	04-mar-1991 - Take out check for last session id - not needed (teresa)
*/

void
IIabort()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if ( IILIBQgdata->form_on  &&  IILIBQgdata->f_end )
    {
	(*IILIBQgdata->f_end)( TRUE );
	IILIBQgdata->form_on = FALSE;
    }

    if ( IISQ_SQLCA_MACRO(IIlbqcb) )
	IIsqAbort( IIlbqcb );		/* Print ESQL errors, if any */

    /*
    ** Release current session.
    */
    IILQssSwitchSession( thr_cb, NULL );

    /* 
    ** Disconnect all database sessions.  We can't
    ** switch normally to each session since they
    ** may be active, so we must steal each session
    ** from the global list and force it active
    ** here.  We keep doing this until the global
    ** list is empty.
    */
    MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
    while( (thr_cb->ii_th_session = IIglbcb->ii_gl_sstate.ss_first) )
    {
	MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
	IIexit();
	MUp_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
    }

    MUv_semaphore( &IIglbcb->ii_gl_sstate.ss_sem );
    thr_cb->ii_th_session = &thr_cb->ii_th_defsess;	/* restore default */

    return;
}

/*{
**  Name:  IIlqExit --  cleanup libq to allow another database connection.
**
**  Description:
**	This function is called in PCexit, after having been set up by
**	a "PCatexit( IIlqExit )" call when at the point when a database
**	connection would be attempted.
** 	It must do everything necessary to allow the user to start
** 	another session, this one having failed with some unrecoverable error.
**
**  Inputs:
**	None
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  History:
**	10-feb-1988 - Written for 6.0.  (marge)
*/

VOID
IIlqExit( VOID )
{
    MUp_semaphore( &IIglbcb->ii_gl_sem );

    if ( IIglbcb->ii_gl_flags & II_G_ABORT )
	MUv_semaphore( &IIglbcb->ii_gl_sem );
    else
    {
	IIglbcb->ii_gl_flags |= II_G_ABORT;	/* Do only once */
	MUv_semaphore( &IIglbcb->ii_gl_sem );
	IIabort();	
    }
    
    return;
}
