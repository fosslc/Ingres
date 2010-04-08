/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<lo.h>
# include	<pc.h>		 
# include	<st.h>		 
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<generr.h>
# include	<eqrun.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<lqgdata.h>
# include	<erlq.h>
# include	<cm.h>

/**
+*  Name: iilqstrc.c - Handle trace output received from dbms server.
**
**  Description:
**	This module contains functions for handling server trace data.
**
**	The trace data is sent from the dbms in the GCA_TRACE message.  
**	When LIBQGCA reads a GCA_TRACE message it calls the LIBQ handler, 
**	IIhdl_gca, which in turn calls into this module to handle the trace.
**
**	Ingres products (W4GL Debugger) call into this module to register
**	trace output handlers (IILQsthSvrTraceHdlr). 
**
**	If the user has set PRINTTRACE via the II_EMBED_SET logical or via 
**	the SET_SQL statement then the trace data is dumped to a file.  
**	Otherwise it goes to the screen, with special provisions for turning 
**      the forms system off and on. In addition, if a server trace output 
**	handler has been registered then it will be invoked.  
**
**	Note that the trace output handler may suspend the printing of trace 
**	data to the screen (re-directing it to the handler only).  However, 
**  	it will not suspend tracing to a file.  The trace data will
**	be directed to both the file and the output handler.
**
**	The session switching code, the connection code and the SET_SQL
**	PRINTTRACE code also call into this module, opening the file if
**	necessary, and printing a comment line to the file. 
**
**  Defines:
**	IILQstpSvrTracePrint	- Print trace output to screen or file
**	IILQstfSvrTraceFile	- Open file and/or print session info to file.
**	IILQsthSvrTraceHdlr	- Register server trace output handler.
-*
**  History:
**	12-mar-1991 (barbara)
**	    Written.
**	17-aug-91 (leighb) DeskTop Porting Change: added pc.h & st.h
**	01-nov-91 (kathryn)
**	    Add function IILQsthSvrTraceHdlr for registration of server
**	    trace output handler.  Made changes for trace file information
**	    now stored in new IILQ_TRACE structure of IIglbcb rather than
**	    directly in IIglbcb.
**	10-oct-92 (leighb) DeskTop Porting Change:
**	    Added extra space for iiprttrc.log name, because MS-WIN requires
**	    full pathnames (it's too stupid to have a workable default dir).
**    16-Feb-93 (mrelac) hp9_mpe
**        Added MPE/iX specific code because we do not have the concept of
**        filename extensions.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* Global and Local data */

GLOBALREF LIBQGDATA	*IILIBQgdata;

# ifdef		PMFEWIN3				 
# define 	PRTRCFILE	ERx("iiprttrc.log                                                  ")
# else
#   ifdef hp9_mpe
#     define    PRTRCFILE       ERx("iiprttrc")
#   else
#     define    PRTRCFILE       ERx("iiprttrc.log")
#   endif
# endif
# define	IITRCENDLN    ERx("-------------------------------------------------------------------\n")


/*{
+*  Name: IILQstfSvrTraceFile
**
**  Description:
**	This procedure is called to open or close the server trace log file,
**	or to record a switch in the session being traced.
**
**	It opens the trace log file if not already open and prints a comment
**	line to the log file.  The comment line contains the session number
**	and the change in tracing specified by the incoming "action" flag
**	(ON, OFF, SWITCH).
**
**	If the name of the server trace log file has not been specified via
**	II_EMBED_SET or SET_SQL, then the default file "iiprttrc.log" will 
**	be used.
**
**  Inputs:
**	IIlbqcb		Current Session control block.
**	action		IITRC_OFF:	Close log file.
**			IITRC_ON:	Open log file.
**			IITRC_SWITCH:	Switch session.
**
**  Outputs:
**	None
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Side Effects:
**	The tracing semaphore must be set by the caller of this routine.
-*	
**  History:
**	12-mar-1991 (barbara)
**	    Written. 
**	10-apr-1991 (kathryn)
**	    Added support for user specified file name for "printtrace" output.
**	    Check ii_gl_svrfile for user specified file name set via:
**	    II_EMBED_SET "tracefile <filename>"  OR
**	    SET_SQL(tracefile = 'filename'). If tracefile was not specified, 
**	    then set ii_gl_svrfile to default file name "iiprttrc.log".
**	15-apr-1991 (kathryn)
**	    If II_G_APPTRC flag is set then open printtrace file in append mode.
**	    A child process has been called and we want its tracing to be
**	    appended to the file of its parent.
**	01-nov-91  (kathryn)
**	    Trace file information now stored in new IILQ_TRACE structure
**	    of IIglbcb rather than directly in IIglbcb.
*/

static VOID
IILQstfSvrTraceFile( II_LBQ_CB *IIlbqcb, i4  action )
{
    IILQ_TRACE  *svrtrc = &IIglbcb->iigl_svrtrc;
    FILE	*trace_file = (FILE *)svrtrc->ii_tr_file;
    char	*title = ERx("off -----");

    /*
    ** Check to see that we actually need 
    ** to open or close the trace file.
    */
    if ( (action == IITRC_ON  &&  trace_file)  ||
	 (action == IITRC_OFF  &&  ! trace_file)  ||
	 (action == IITRC_SWITCH  &&  ! trace_file) )
	return;

    if ( action == IITRC_SWITCH )  title = ERx("switched ");

    if ( action == IITRC_ON )
    {
	LOCATION	trace_loc;
	STATUS		stat;

	if ( ! svrtrc->ii_tr_fname )  
	    svrtrc->ii_tr_fname = STalloc( PRTRCFILE );

	LOfroms(FILENAME, svrtrc->ii_tr_fname, &trace_loc );

	if ( svrtrc->ii_tr_flags & II_TR_APPEND )
	    stat = SIopen( &trace_loc, ERx("a"), (FILE **)&trace_file );
	else
	    stat = SIopen( &trace_loc, ERx("w"), (FILE **)&trace_file );

	if (stat != OK)
	{
	    /* Simplest error path; don't want IIlocerr function */
	    IIUGerr( E_LQ0007_PRINTQRY, 0, 1, svrtrc->ii_tr_fname);

	    /*
	    ** We can't call IILQgstGcaSetTrace() because
	    ** of possible conflicts with the tracing
	    ** semaphore, so just turn off tracing here.
	    */
	    svrtrc->ii_tr_flags &= ~II_TR_FILE;
	    return;
	}

	svrtrc->ii_tr_file = (PTR)trace_file;
	title = ERx("on ------");
    }

    SIfprintf( trace_file,
	ERx("---------- printtrace = %s session %d -----------------------\n"),
	title, IIlbqcb->ii_lq_sid );
    SIflush( trace_file );

    if ( action != IITRC_OFF )
	svrtrc->ii_tr_sid = IIlbqcb->ii_lq_sid;
    else
    {
	SIclose( trace_file );
	svrtrc->ii_tr_file = NULL;
	svrtrc->ii_tr_sid = 0;
	svrtrc->ii_tr_flags |= II_TR_APPEND;  /* Don't overwrite if reopened */
    }

    return;
} /* IILQstfSvrTraceFile */

/*{
+*  Name: IILQsstSvrSetTrace
**
**  Description:
**	Set the server tracing state globally and for the current session.
**	Enables/disables tracing to the log file or a tracing handler.
**	May also be used to temporarily close the log file without changing
**	the tracing state (when spawning a child process, for example).
**		
**  Inputs:
**	IIlbqcb		Current session control block.
**      action          IITRC_OFF:	Suspend tracing to file.
**                      IITRC_ON:	Enable tracing to file.
**			IITRC_HDLON :	Enable tracing to handler.
**			IITRC_HDLOFF:	Suspend tracing to handler.
**			IITRC_REST:	Close file without changing state.
**
**  Outputs:
**	None.
-*
**  Side Effects:
**	The trace log may be closed.
**
**  History:
*/

VOID
IILQsstSvrSetTrace( II_LBQ_CB *IIlbqcb, i4  action )
{
    IILQ_TRACE *svrtrc = &IIglbcb->iigl_svrtrc;

    /*
    ** This should work, so just ignore request if fails.
    */
    if ( MUp_semaphore( &svrtrc->ii_tr_sem ) != OK )  return;

    switch( action )
    {
	case IITRC_ON :
	    svrtrc->ii_tr_flags |= II_TR_FILE;
	    break;

	case IITRC_OFF :
	    svrtrc->ii_tr_flags &= ~II_TR_FILE;
	    IILQstfSvrTraceFile( IIlbqcb, IITRC_OFF );
	    break;

	case IITRC_HDLON :
	    if ( svrtrc->ii_num_hdlrs )  svrtrc->ii_tr_flags |= II_TR_HDLR;
	    break;

	case IITRC_HDLOFF :
	    if ( ! svrtrc->ii_num_hdlrs )  svrtrc->ii_tr_flags &= ~II_TR_HDLR;
	    break;

	case IITRC_SWITCH :
	    /* nothing to do */
	    break;

	case IITRC_RESET :
	    IILQstfSvrTraceFile( IIlbqcb, IITRC_OFF );
	    break;
    }

    MUv_semaphore( &svrtrc->ii_tr_sem );

    return;
}

/*{
+*  Name: IILQstpSvrTracePrint - Handle server trace output.
**
**  Description:
**	When the LIBQ handler for GCA messages receives a GCA_TRACE
**	message, it calls this function to dump the trace to the screen
**	or to a file, and call the server trace output handler if one is 
**	registered.  Trace output is sent by the dbms in "batches" of 
**	GCA_TRACE messages.  
**
**	If the user has enabled PRINTTRACE, the output is written to the file 
**	specified by the user via II_EMBED_SET or SET_SQL.  If no file 
**	name was specified then output is written to the file 'iiprttrc.log' 
**	in the current directory.  If PRINTTRACE is not enabled, the output 
**	is written to the standard output channel.
**
**      Note that the trace output handler may suspend the printing of trace 
**	data to the standard output channel.  It may not suspend the
**      tracing to a file.  If both PRINTTRACE is set and a trace output
**      handler is registered, then the trace data will be directed to both 
**	the file and the handler.
**
**	To permit more flexibility in trace protection, the caller of this
**	routine is required to lock the tracing semaphore.
**
**  Inputs:
**	trace_done - TRUE if no more GCA_TRACE messages in this "batch".
**	trace_dbv  - Pointer to a DB_DATA_VALUE containing the trace data.
**
**  Outputs:
**	None
**	Returns:
**	    0 - Unused.
**	Errors:
**	    None
-*	
**  History:
**	12-mar-1991 (barbara)
**	    Written. 
**	15-apr-1991 (kathryn) Bug 36886
**	    Remove new-line character to alleviate double spacing.
**	15-apr-1991  (kathryn)
**	    When "printtrace" has been specified but the output file is not 
**	    open, Call IILQstfSvrTraceFile to open the file. 
**	01-nov-1991  (kathryn)
**	    If trace handler is registered then invoke it here.
*/

i4
IILQstpSvrTracePrint( bool trace_done, DB_DATA_VALUE *trace_dbv )
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILQ_TRACE	*svrtrc = &IIglbcb->iigl_svrtrc;
    FILE	*trace_file;
    char	tmpbuf[ER_MAX_LEN + 3];
    i4	dummy;
    int		len;

    /* 
    ** Static is okay here because these variables
    ** are only significant if the forms system is
    ** active which is only supported as single
    ** threaded and we never switch sessions in 
    ** between a series of trace messages.
    */
    static	bool	first = TRUE;
    static	bool	pause = FALSE;

    if ( svrtrc->ii_tr_flags & II_TR_HDLR )
	if ( trace_done )
	    IILQcthCallTraceHdlrs(svrtrc->ii_tr_hdl_l, IITRCENDLN, FALSE);
	else
	{
	    len = CMcopy(trace_dbv->db_data, trace_dbv->db_length, tmpbuf);
	    tmpbuf[len] = EOS;
	    IILQcthCallTraceHdlrs(svrtrc->ii_tr_hdl_l, tmpbuf, FALSE);
	}

    if ( svrtrc->ii_tr_flags & II_TR_FILE ) 	/* Printtrace is set */
    {
	/* 
	** "printtrace" is turned on but the file is closed. It may be that we
	** are returning from a child process and the child process closed the
	** file so we need to re-open the file for the parent. 
	*/
	if ( ! svrtrc->ii_tr_file )  IILQstfSvrTraceFile( IIlbqcb, IITRC_ON );

	/* Trace file could not be opened */
	if ( ! (trace_file = (FILE *)svrtrc->ii_tr_file) )
	    return;

	/*
	** Check to see if the session has been switched
	** since the last trace was written.
	*/
	if ( svrtrc->ii_tr_sid != IIlbqcb->ii_lq_sid )
	    IILQstfSvrTraceFile( IIlbqcb, IITRC_SWITCH );
    }
    else  if ( ! (svrtrc->ii_tr_flags & II_TR_NOTERM) )	/* Trace to stdout */
    {
	trace_file = stdout;

        if ( first ) 
        {
	    /* Check forms system for first time through */
	    if ( IILIBQgdata->form_on  &&  IILIBQgdata->f_end )
	    {
	        (*IILIBQgdata->f_end)( TRUE );	/* Restore to NORMAL */
		pause = TRUE;
	    }

	    first = FALSE;
        }
    }
    else
    {
	/* Nothing to do */
	return;
    }

    if ( ! trace_done )
	SIwrite( (i4)(trace_dbv->db_length), 
		 trace_dbv->db_data, &dummy, trace_file );
    else
    {
	SIfprintf( trace_file, IITRCENDLN );
	SIflush( trace_file );
	first = TRUE;

	if ( pause )
	{
	    PCsleep(2000);
	    (*IILIBQgdata->f_end)(FALSE);	/* Restore FRS */
	    pause = FALSE;
	}
    }

    return;
} /* IILQstpSvrTracePrint

/*
+*  Name:  IILQsthSvrTraceHdlr - Register server trace output handler.
**
**  Description:
**      Register callback routine to handle server trace output.  
**	This routine is called by Ingres products (W4GL Debugger) to
**	register a server trace output handler.  The routine registered via
**	this function is stored in the global control block (IIglbcb) and
**	will be invoked by IILQstpSvrTracePrint() when the LIBQ handler for
**	GCA messages receives a GCA_TRACE message.
**
**  Inputs:
**      rtn - callback routine to receive future output (if active = TRUE).
**      cb - context block pointer to be passed to (*rtn)().  Opaque to libq.
**      active - if TRUE, we are registering a new callback.  If false,
**              we are unregistering an existing callback.  Done this way
**              rather than by a NULL rtn argument to allow future support
**              for multiple callbacks, although we only need one at present.
**              (currently, a new active callback may simply replace an
**              existing one).
**      no_dbterm - if TRUE, shut off current server trace output IF it is to
**              a terminal.  If it is to a file, it is to continue to
**              be directed both places.  Irrelevent if active is FALSE.
**              The implication is that tty output will be restored when
**              callback unregisters.
**  Outputs:
**	None
**  	Returns:
**	    FAIL - Attempt to register NULL trace handler.
**	    OK	 - Success
**  	Errors:
**	    None
**
**  Side Effects:
**	When no_dbterm is TRUE the printing of trace data to the screen will
**	be suspended until this handler is un-registered.
**
**  History:
**	01-nov-1991 (kathryn) Written.
**      31-dec-1992 (larrym)
**          Modified to maintain a list of handlers instead of just one.
**          Stripped out common code amongst all three trace handler
**          routines (including this one) and placed it in a new function
**          IILQathAdminTraceHdlr, which this routine now calls.
*/

STATUS
IILQsthSvrTraceHdlr(rtn,cb,active,no_dbterm)
VOID (*rtn)();
PTR cb;
bool active;
bool no_dbterm;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILQ_TRACE	*svrtrc = &IIglbcb->iigl_svrtrc;
    STATUS	status;

    if ( MUp_semaphore( &svrtrc->ii_tr_sem ) != OK )
	return( FAIL );

    status = IILQathAdminTraceHdlr( svrtrc, rtn, cb, active );

    if ( ! svrtrc->ii_num_hdlrs )
	svrtrc->ii_tr_flags &= ~II_TR_NOTERM;
    else  if ( active  &&  no_dbterm )
	svrtrc->ii_tr_flags |= II_TR_NOTERM;

    MUv_semaphore( &svrtrc->ii_tr_sem );

    if ( status != FAIL )
	IILQsstSvrSetTrace( IIlbqcb, active ? IITRC_HDLON : IITRC_HDLOFF );

    return( status );
}
