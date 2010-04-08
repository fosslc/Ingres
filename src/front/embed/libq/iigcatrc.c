/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<tm.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<fe.h>
# include	<ug.h>
# include	<eqrun.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iigcatrc.c - Trace support for GCA message tracing.
**
**  Description:
**	This file contains functions for the support of GCA message tracing.
**	
**	Gca message tracing may be specified by the user via the II_EMBED_SET
**	logical or toggled on and off via the SET_SQL/SET_INGRES statement.
**	Ingres products (W4GL debugger) may toggle gca tracing on and off
**	via registration/de-registration of a gca trace output handler through
**	IILQgthGcaTraceHandler().
**
**	IILQgstGcaSetTrace() is called to turn Gca message tracing on 
**	and register IILQgwtGcaWriteTrace() to libqgca as the internal 
**	trace write function.
**      
**	When PRINTGCA has been turned on via II_EMBED_SET or SET_SQL,
**	IILQgtfGcaTraceFile() will be called to open the trace log file and
**	write session and timing information to the log. If a file name has 
**	not been specified via II_EMBED_SET "gcafile <filename>" or 
**	SET_SQL(gcafile = 'filename') then the default file "iiprtgca.log" 
**	will be used.
**	
**	IILQgwtGcaWriteTrace() will be indirectly called from libqgca
** 	(via the emit function) to handle the GCA message trace output.
** 	If tracing was turned on by the user via II_EMBED_SET or SET_SQL, 
**	then it will print the trace data to the gca trace log file.  
**	In addition, if a GCA message ouput handler is registered then it 
**	will invoke the registered handler.  If GCA message tracing was turned
**	on by both the user and the registration of an trace data handler, then
**	data will go to both the log file and the handler.
**
**  Defines:
**	IILQgstGcaSetTrace      - Resume/suspend GCA message tracing.
**	IILQgtfGcaTraceFile	- Write session information to log file.
**	IILQgwtGcaWriteTrace	- Handle preformatted trace data.
**	IILQgthGcaTraceHdlr     - Register GCA trace output handler.
**
**  Notes:
**	The flags in the IILQ_TRACE structure of the global control block 
**	(IIglbcb->msgtrc.ii_tr_flags) specify how trace output should be 
**	handled.
**	    II_TR_FILE  -  Set via II_EMBED_SET toggled via SET_SQL.
**	                   Write trace information to specified log file.	
**	    II_TR_HDLR  -  Toggled via [de]registration of trace data handler.
**			   Invoke trace output handler with trace data.
-*
**  History:
**	09-sep-1988	- Written for tracing 6.1 GCA messages (ncg)
**	19-may-1989	- Updated for multiple connects. (bjb)
**	22-jan-1990	- Fixed LOfroms call to pass buffer, not literal. (bjb)
**	14-mar-1991	- Modify IILQgtfGcaTraceFile behavior. (teresal)
**	01-nov-1991	(kathryn)
**	    Added function IILQgthGcaTraceHdlr() for registration of GCA trace
**	    data handler.  Changed IILQgstGcaSetTrace() -  Moved code which 
**	    opens trace log file and writes session and timing info to log file
**	    from IILQgstGcaSetTrace into new function IILQgtfGcaTraceFile().
**	    Made changes for trace file information now stored in new 
**	    IILQ_TRACE structure of IIglbcb rather than directly in IIglbcb.
**	15-Jun-92 (seng/fredb)
**	    Integrated fredb's change of 22-Apr-91 from 6202p:
**      	22-apr-1991 (fredb) hp9_mpe
**                 Added MPE/XL specific code because we do not have the
**		   concept of filename extensions and we need to set specific
**		   file attributes using the extra parameters of SIfopen.
**	10-oct-92 (leighb) DeskTop Porting Change:
**	    Added extra space for iiprtgca.log name, because MS-WIN requires
**	    full pathnames (it's too stupid to have a workable default dir).
**      07-oct-93 (sandyd)
**          Added <fe.h>, which is now required by <ug.h>.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Data
*/
# ifdef hp9_mpe
# define	GCTRACEFILE	ERx("iiprtgca")		/* Trace file name */
# else
# ifdef	PMFEWIN3					 
# define	GCTRACEFILE	ERx("iiprtgca.log                                                  ")	/* Trace file name */
# else
# define	GCTRACEFILE	ERx("iiprtgca.log")	/* Trace file name */
# endif
# endif
# define	GCLINELEN	80			/* Max line length */


/*
** Forward references.
*/
VOID	IILQgwtGcaWriteTrace();

/*{
+*  Name: IILQgtfGcaTraceFile
**
**  Description:
**	This procedure is called to open or close the GCA trace log file,
**	or to record a switch in the session being traced.
**
**      It opens the trace log file if not already open and prints a 
**	comment line to the log file.  The comment line contains the 
**	session number and a timestamp as well as the change in tracing 
**	specified by the incoming "action" flag (ON, OFF, SWITCH).
**
**	If the name of the gca trace log file has not been specified via
**	II_EMBED_SET or SET_SQL, then the default file "iiprtgca.log" will
**	be used.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	action		IITRC_OFF:	Close log file.
**			IITRC_ON:	Open log file.
**			IITRC_SWITCH: Switch session.
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    E_LQ0007_PRINTQRY	- Cannot open trace file.
-*
**  Side Effects:
**	Sets the trace file pointer.  The tracing semaphore must
**	be set by the caller of this routine.
**
**  History:
**	09-sep-1988	- Written (ncg)
**	23-may-1989	- Added flags and session id for multiple connects.(bjb)
**	22-jan-1990	- LOfroms call must pass a buffer, not a liter. (bjb)
**	14-mar-1991	- Allow PRINTGCA file to be opened if not already
**			  when switching sessions.  This allows PRINTGCA
**			  to work even if it has been set on (via SET_INGRES) 
**			  in between sessions.
**	10-apr-1991 (kathryn)
**	    Added support for user specified file name for "printgca" output.
**	    Check ii_gl_msgfile for user specified file name set via:
**	    II_EMBED_SET "gcafile <filename>" or SET_SQL (gcafile = 'filename').
**	    If gcafile was not specified, then set ii_gl_msgfile to default 
**	    file name "iiprtgca.log".
**	15-apr-1991 (kathryn)
**	    If II_G_APPTRC flag is set then open printgca file in append mode.
**	    A child process has been called and we want its tracing to be
**	    appended to the file of its parent.
**	01-nov-1991  (kathryn)  WRITTEN.
**	    Move all log file handling code and applicable history from 
**	    IIgstGcaSetTrace to this function.  IIgstGcaSetTrace will now
**	    only be used to to turn message tracing on and off.  
**	    Trace file information now stored in new IILQ_TRACE structure of
**	    IIglbcb rather than directly in IIglbcb.
*/

static VOID
IILQgtfGcaTraceFile( II_LBQ_CB *IIlbqcb, i4  action )
{
    IILQ_TRACE		*msgtrc = &IIglbcb->iigl_msgtrc;
    FILE		*trace_file = (FILE *)msgtrc->ii_tr_file;
    char		*title = ERx("off");
    SYSTIME		now;
    char		nowbuf[100];
    i4			nowend;

    /*
    ** Check to see that we actually need to open
    ** or close the trace file.
    */
    if ( (action == IITRC_ON  &&  trace_file)    ||
	 (action == IITRC_OFF  &&  ! trace_file)  ||
	 (action == IITRC_SWITCH  &&  ! trace_file) )
	return;

    if ( action == IITRC_SWITCH )  title = ERx("switched");

    if ( action == IITRC_ON )
    {
	LOCATION	trace_loc;
	STATUS		stat;

	/*
	** If trace file name not provided, use default.
	*/
	if ( ! msgtrc->ii_tr_fname )
	    msgtrc->ii_tr_fname = STalloc( GCTRACEFILE );

	LOfroms(FILENAME, msgtrc->ii_tr_fname, &trace_loc );
# ifdef hp9_mpe
	if ( msgtrc->ii_tr_flags & II_TR_APPEND )
	    stat = SIfopen(&trace_loc, ERx("a"), SI_TXT, 252, &trace_file);
	else
	    stat = SIfopen(&trace_loc, ERx("w"), SI_TXT, 252, &trace_file);
# else
	if ( msgtrc->ii_tr_flags & II_TR_APPEND )
	    stat = SIopen(&trace_loc, ERx("a"), &trace_file);
	else
	    stat = SIopen(&trace_loc, ERx("w"), &trace_file);
# endif
	if (stat != OK)
	{
	    /* Simplest error path; don't want IIlocerr functionality */
	    IIUGerr(E_LQ0007_PRINTQRY, 0, 1, msgtrc->ii_tr_fname);

	    /*
	    ** We can't call IILQgstGcaSetTrace() because
	    ** of possible conflicts with the tracing
	    ** semaphore, so just turn off tracing here.
	    */
	    msgtrc->ii_tr_flags &= ~II_TR_FILE;
	    if ( ! (msgtrc->ii_tr_flags & II_TR_HDLR) )
		IIcgct1_set_trace( IIlbqcb->ii_lq_gca, 0, NULL, NULL );
	    return;
	}

	msgtrc->ii_tr_file = (PTR)trace_file;
	title = ERx("on ");
    }

    /* Get time stamp */
    TMnow(&now);
    TMstr(&now, nowbuf);
    nowend = STlength( nowbuf );
    if ( nowbuf[ nowend - 1 ] == '\n' )  nowbuf[ nowend - 1 ] = EOS;

    SIfprintf( trace_file, ERx("---- printgca = %s session %d (%s) ---\n\n"),
	       title, IIlbqcb->ii_lq_sid, nowbuf );
    SIflush( trace_file );

    if ( action != IITRC_OFF )
	msgtrc->ii_tr_sid = IIlbqcb->ii_lq_sid;
    else
    {
	SIclose( trace_file );
	msgtrc->ii_tr_file = NULL;
	msgtrc->ii_tr_sid = 0;
	msgtrc->ii_tr_flags |= II_TR_APPEND;  /* Don't overwrite if reopened */
    }

    return;
} /* IILQgtfGcaTraceFile */

/*{
+*  Name: IILQgstGcaSetTrace
**
**  Description:
**	Set the GCA tracing state globally and for the current session.
**	Enables/disables tracing to the log file or a tracing handler.
**	May also be used to set just the tracing state of the current
**	session (when starting/switching sessions).  May also be used
**	to temporarily close the trace file without changing the trace
**	state (when spawning a child process, for example).
**		
**  Inputs:
**	IIlbqcb		Current session control block.
**      action          IITRC_OFF:	Suspend tracing to file.
**                      IITRC_ON:	Enable tracing to file.
**			IITRC_HDLON :	Enable tracing to handler.
**			IITRC_HDLOFF:	Suspend tracing to handler.
**                      IITRC_SWITCH:	Set trace state for new session.
**			IITRC_RESET:	Close file without changing state.
**
**  Outputs:
**	None.
-*
**  Side Effects:
**	The trace log may be closed.
**
**  History:
**	01-nov-1991 (kathryn) 
**	    Moved all gca message log file handling from this routine to 
**	    IILQgtfGcaTraceFile.  
*/

VOID
IILQgstGcaSetTrace( II_LBQ_CB *IIlbqcb, i4  action )
{
    IILQ_TRACE *msgtrc = &IIglbcb->iigl_msgtrc;

    /*
    ** This should work, so just ignore request if fails.
    */
    if ( MUp_semaphore( &msgtrc->ii_tr_sem ) != OK )  return;

    switch( action )
    {
	case IITRC_ON :
	    msgtrc->ii_tr_flags |= II_TR_FILE;
	    IIcgct1_set_trace( IIlbqcb->ii_lq_gca, GCLINELEN, 
			       (PTR)&msgtrc->ii_tr_sem, IILQgwtGcaWriteTrace );
	    break;

	case IITRC_OFF :
	    msgtrc->ii_tr_flags &= ~II_TR_FILE;
	    IILQgtfGcaTraceFile( IIlbqcb, IITRC_OFF );
	    if ( ! (msgtrc->ii_tr_flags & II_TR_HDLR) )
		IIcgct1_set_trace( IIlbqcb->ii_lq_gca, 0, NULL, NULL );
	    break;

	case IITRC_HDLON :
	    if ( msgtrc->ii_num_hdlrs )
	    {
		msgtrc->ii_tr_flags |= II_TR_HDLR;
		IIcgct1_set_trace( IIlbqcb->ii_lq_gca, GCLINELEN, 
				   (PTR)&msgtrc->ii_tr_sem, 
				   IILQgwtGcaWriteTrace );
	    }
	    break;

	case IITRC_HDLOFF :
	    if ( ! msgtrc->ii_num_hdlrs )
	    {
		msgtrc->ii_tr_flags &= ~II_TR_HDLR;
		if ( ! (msgtrc->ii_tr_flags & II_TR_FILE) )
		    IIcgct1_set_trace( IIlbqcb->ii_lq_gca, 0, NULL, NULL );
	    }
	    break;

	case IITRC_SWITCH :
	    if ( msgtrc->ii_tr_flags & (II_TR_FILE | II_TR_HDLR) )
		IIcgct1_set_trace( IIlbqcb->ii_lq_gca, GCLINELEN, 
				   (PTR)&msgtrc->ii_tr_sem, 
				   IILQgwtGcaWriteTrace );
	    else
		IIcgct1_set_trace( IIlbqcb->ii_lq_gca, 0, NULL, NULL );
	    break;

	case IITRC_RESET :
	    IILQgtfGcaTraceFile( IIlbqcb, IITRC_OFF );
	    break;

    }

    MUv_semaphore( &msgtrc->ii_tr_sem );

    return;
}

/*{
+*  Name: IILQgwtGcaWriteTrace - Write trace line to a file.
**
**  Description:
**	While GCA is tracing messages this routine is indirectly called
**	(via the emit function exit).  It invokes the gca message output
**	handler if one is registered, and if printgca was set by either
**	II_EMBED_SET or SET_SQL then it dumps its arguments to the trace
**	log.
**
**	Since it may require several calls to this routine to fully trace
**	a message, the caller of this routine is required to handle the
**	tracing semaphore so that a series of calls may be protected by
**	a single lock.
**
**  Inputs:
**	action		- GCA_TR_WRITE - Write data
**			- GCA_TR_FLUSH - [Write and] flush data
**	len		- Length of following buffer.  If the length is zero
**			  and the buffer is non-null then it is null terminated.
**	buf		- Buffer with trace data (may be null).
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None - If write/flush fails this is ignored.
-*
**  Side Effects:
**	The trace log may be opened or closed.
**
**  History:
**	14-sep-1988	- Written (ncg)
**	15-apr-1991  (kathryn)
**	    When file for "printgca" trace is not open, but tracing has been
**	    turned on, Call IILQgtfGcaTraceFile to open the file.
**	01-nov-1991  (kathryn)
**	    If a trace output handler is registered then invoke it here.
**	    Made changes for trace file information now stored in new 
**	    IILQ_TRACE structure of IIglbcb rather than directly in IIglbcb.
*/

VOID
IILQgwtGcaWriteTrace(action, len, buf)
i4	action;
i4	len;
char	*buf;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILQ_TRACE	*msgtrc = &IIglbcb->iigl_msgtrc;
    FILE	*trace_file;		/* Current trace file */
    i4	dummy;

    /* 
    ** Invoke GCA trace output handlers.
    */
    if ( msgtrc->ii_tr_flags & II_TR_HDLR )
	IILQcthCallTraceHdlrs( msgtrc->ii_tr_hdl_l, buf, FALSE );

    /* 
    ** If PRINTGCA not enabled, we be done.
    */
    if ( ! (msgtrc->ii_tr_flags & II_TR_FILE) )  return;

    /* 
    ** Open the output trace file if necessary.
    */
    if ( ! msgtrc->ii_tr_file )
	IILQgtfGcaTraceFile( IIlbqcb, IITRC_ON );

    /* Trace file could not be opened */
    if ( ! (trace_file = (FILE *)msgtrc->ii_tr_file) )
	return;

    /*
    ** Check to see if the session has been switched
    ** since the last trace was written.
    */
    if ( msgtrc->ii_tr_sid != IIlbqcb->ii_lq_sid )
	IILQgtfGcaTraceFile( IIlbqcb, IITRC_SWITCH );

    /*
    ** Write/flush output trace.
    */
    if ( buf )
    {
	if ( ! len )
	    SIfprintf( trace_file, ERx("%s"), buf );
	else if ( len > 0 )
	    SIwrite( (i4)len, buf, &dummy, trace_file );
    }

    if ( action == GCA_TR_FLUSH )  SIflush( trace_file );

    return;
} /* IILQgwtGcaWriteTrace */


/*{
+*  Name:  IILQgthGcaTraceHdlr - Register GCA message trace output handler.
**
**  Description:
**      Register callback routine to handle GCA trace output.  This does NOT
**      affect the logging which may be active due to II_EMBED_SET or SET_SQL.
**      Registration of a callback routine will turn "printgca"
**      tracing on if it is not already on, and de-registration will turn
**      "printgca" tracing off if it has been turned on only via the handler
**      registration and not via II_EMBED_SET or SET_SQL.
**      This routine is called by Ingres products (W4GL Debugger) to register
**      a printgca trace output handler.  The routine registered via this 
**	function is stored in the global control block (IIglbcb) and will be
**	invoked by IILQgwtGcaWriteTrace() when it is called to handle gca 
**	trace output (via the emit function) from libqgca.
**
**  Inputs:
**
**      rtn - callback routine to recieve future output (if active = TRUE).
**
**      cb - context block pointer to be passed to (*rtn)().  Opaque to libq.
**
**      active - if TRUE, we are registering a new callback.  If false,
**              we are unregistering an existing callback.  Done this way
**              rather than by a NULL rtn argument to allow future support
**              for multiple callbacks, although we only need one at present.
**              (currently, a new active callback may simply replace an
**              existing one).
**  Outputs:
**	None.
**	Returns:
**	    FAIL - Attempt to register a NULL callback routine.
**	    OK   
**	Errors:
**	    None.
**
**  Side Effects:
**	printgca tracing may be turned on/off.
**
**  History:
**	01-nov-1991 (kathryn) Written.
**	15-dec-1991 (kathryn)
**	    If not yet connected to database then just set II_G_MSGTRC flag.
**	    Only call IILQgstGcaSetTrace if already connected to DBMS.
**	31-dec-1992 (larrym)
**	    Modified to work with a list of callback routines.  Moved 
**	    some common (among QRY,GCA,and server tracing) functionality 
**	    into IILQathAdminTraceHdlr which this routine now calls.
*/
STATUS
IILQgthGcaTraceHdlr(rtn,cb,active)
VOID (*rtn)();
PTR cb;
bool active;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILQ_TRACE	*msgtrc = &IIglbcb->iigl_msgtrc;
    STATUS	status;

    if ( MUp_semaphore( &msgtrc->ii_tr_sem ) != OK )
	return( FAIL );

    status = IILQathAdminTraceHdlr( msgtrc, rtn, cb, active );

    MUv_semaphore( &msgtrc->ii_tr_sem );

    if ( status != FAIL )
	IILQgstGcaSetTrace( IIlbqcb, active ? IITRC_HDLON : IITRC_HDLOFF );

    return( status );
}
