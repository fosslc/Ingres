/*
** Copyright (c) 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<si.h>		/* needed for iicxfe.h */
# include	<er.h>
# include       <me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include	<iilibq.h>
# include	<iilibqxa.h>
# include	<erlq.h>

/*
NO_OPTIM = nc4_us5
*/

/**
+*  Name: iiqry.c - Starts and ends a generic query sent from somewhere
**		    in LIBQ.
**
**  Description:
**	This file contains the routines to start and end a query.  Normally
**	queries will come through IIwritedb and IIsync, but others may
**	come through IIxact, IIcs or IIex calls.  This routines are here
**	so that others do not have to duplicate the query start up checking.
**
**  Defines:
**	IIqry_start	- Check and if okay start a query.
**	IIqry_end	- End off a query.
**	IIqry_read	- End off a query to DBMS but prepare to read.
**
**  Notes:
**	The typical case that this is called may be:
**	    IIwritedb	- If nothing happenning yet call IIqry_start,
**			  else send my string.
**	    IIsyncup	- Call IIqry_end.
**	    IIretinit	- Call IIqry_read.
**
**	APPEND example:			     Where we get called:
**	    IIqry_start();			IIwrite
**	    IIwritedb(query text);		IIwrite
**	    if (Q_INQRY)			IIsync
**		IIcgc_end_msg();
**	    IIqry_end();
**
**	RETRIEVE [OPEN CURSOR] example:
**	    IIqry_start();			IIwrite   [IIcsOpen]
**	    IIwritedb(query text);		IIwrite   [IIwrite]
**	    if (INQRY)				IIretinit [IIcsQuery]
**		IIcgc_end_msg();
**	    if (!Q_INQRY || Q_QERR) {
**		IIqry_end();
**		return;
**	    }
**	    stat = IIqry_read();
**	    if (stat == FAIL || DB_ERR(ii_er_num))
**		IIqry_end();
**		return;
**	    }
**	    read_data(); ...
**	    IIqry_end();			IIflush  [IIcsQuery]
-*
**  History:
**	08-oct-1986	- Written when we introduced new query entry points into
**			  LIBQ. (ncg)
**	14-apr-1987	- Added ESQL/SQLCA support (bjb).
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	12-dec-1988	- Generic error processing. (ncg)
**	19-may-1989	- Changed names of globals for multiple connects. (bjb)
**	02-aug-1989	- Check that response block processed before changing
**			  transaction flag. (barbara)
**	17-apr-1990	(barbara)
**	    At query start, clear "subsidiary" error buffer.
**	30-apr-1990	(neil)
**	    Cursor performance: IIqry_start accepts 0 message (for checks).
**	    IIqry_end/read close all cursor on xact-end, & process II_Q_PFRESP.
**	14-jun-1990	(barbara)
**	    Don't PCexit if there is no database connection unless
**	    PGQUIT flag is set. (bug 21832)
**	23-aug-1990	(barbara)
**	    Initialize ii_er_msg and ii_er_mnum (saved user message number and
**	    text from dbprocs).  Bug 20883.
**      20-mar-1991     (teresal)
**          Add support for storing logical key values in IIqry_end.
**	15-jun-1993 (larrym)
**	    (XA stuff) Added call to IIXA_SET_ROLLBACK_VALUE when we get
**	    a GCA_ILLEGAL_XACT_STMT from the server.
**      23-apr-96 ( chech02)
**          fixed compiler errors for windows 3.1 port.
**	    IIlocerr() needs 5 arguments.
**	04-Apr-96 (loera01)
**	    Modified IIqry_start() to support compressed variable-length 
**	    datatypes: send GCA_COMP_MASK to back end if compression is
**          supported on the front end.
**	15-aug-96 (chech02)
**	    If a query session has been disconnected, IIlbqcb->ii_lq_gca 
**	    is set to NULL and then we will get a GPF in IIqry_end(). So, 
**	    don't need to read the query result in IIqry_end(), if a query 
**	    session has been disconnected.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	16-oct-98 (matbe01)
**	    Added NO_OPTIM for NCR (nc4_us5) to fix error "E_LQ003A Cannot
**	    start up 'select' query."
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jun-2008 (lunbr01)  bug 120432
**	    Read of II_VCH_COMPRESS_ON variable was being done here for
**	    every query, which is bad for performance.  Moved to
**	    initialization logic so it is only done once.
**/

void	IIqry_end();

/*{
+*  Name: IIqry_start - Check if we can start a query and if okay start it.
**
**  Description:
**	Makes sure that certain conditions are okay to start the query.
**	If they are not then print an error message (and abort if we are
**	not connected to the DBMS).  Otherwise call GCA and mark that we have 
**	started a query by setting the II_Q_INQRY flag in 'ii_lq_curqry'.
**	
** 	On a nested query, we "forget" the II_Q_INQRY flag because that 
**	flag really belongs to the parent retrieve.  Instead we mark the
**	current (nested) query as being in error.  IIqry_end() will end off 
**	this erroneous query, but will not reset the rowcount, which belongs 
**	to the parent.  The parent query (a retrieve loop, select loop, or
**	procedure invocation) is noted as as having a nested-error so it
**	won't try executing anymore (although we could let it carry on!).
**
**	A special GCA message type of 0 indicates that the caller just wants
**	this routine to run some checks and to reinitialize various fields
**	without entering a real query.  This is useful for cursor FETCH
**	operations where data may be read from an allocated data stream.
**
**  Inputs:
**	gcatype	- i4	- Type to send to IIcgc_init_msg.  If 0 then just check.
**	gcamask	- i4	- Mask to send to IIcgc_init_msg.
**	qrylen	- i4	- Length of qryerr string (if 0 then use STlength).
**	qryerr	- char *- Query string currently being processed (for errors).
**
**  Outputs:
**	Returns:
**	    STATUS - OK or FAIL on start.
**	Errors:
**	    NODB	- Not connected to a database.
**	    INQUERY	- Attempting to nest a query within retrieval.
**	    INPROC	- Attempting to nest a query within procedure.
-*
**  Side Effects:
**	1. Sets the 'ii_lq_curqry' bits to flag whether or not we are in
**	   a query.
**	2. Resets error numbers (ii_lq_err.ii_er_num & ii_lq_err.ii_er_other).
**	
**  History:
**	08-oct-1986	- Written. (ncg)
**	18-feb-1988	- Changed "return FAIL" to "PCexit(-1)".  (marge)
**	10-may-1988	- Collapsed some of the retrieve loop control to
**			  be used for checking procedures too. (ncg)
**	03-may-1989	- Fixed bug 5494 - initialize ii_er_smsg. (bjb)
**	19-may-1989	- Changed names of globals for multiple connects. (bjb)
**	17-apr-1990	(barbara)
**	    Clear buffer that saves "subsidiary" error messages.
**	30-apr-1990	(neil)
**	    Cursor performance: Accepts 0 message to just run checks.
**	14-jun-1990	(barbara)
**	    Don't PCexit if there is no database connection unless
**	    PGQUIT flag is set. (bug 21832)
**	15-jun-1990	(barbara)
**	    Allow GCA_EVENT set up to go through here.
**	23-aug-1990	(barbara)
**	    Initialize ii_er_msg and ii_er_mnum (saved user message number and
**	    text from dbprocs).  Bug 20883.
**	13-sep-1990	- Added check for II_L_NAMES to see if caller wants
**			  names in descriptor on queries.  (Joe)
**	21-mar-1991	(teresal)
**	    Initialize logical key flag value ii_lq_logkey.ii_lg_flags.
**	04-apr-1991	(kathryn)   Bug 36849
**	    Don't turn off II_L_SINGLE flag here.  It will be turned off
**	    by IIflush().
**	10-jan-1993 (kathryn)
**	    Always set GCA_NAMES MASK to get names in descriptor for 
**	    new INQUIRE_SQL(columnname).
**	09-mar-1993 (larrym)
**	    added XA support - if app is XA, set GCA_Q_NO_XACT_STMT so server
**	    won't allow commits/rollbacks/etc.
**	06-jul-1993 (larrym)
**	    fixed bug XA code mentioned above.
**	04-Apr-96 (loera01)
**	    Modified to support compressed variable-length datatypes:
**	    send GCA_COMP_MASK to back end if compression is
**          supported on the front end.
**	10-Jun-2008 (lunbr01)  bug xxxxx
**	    Read of II_VCH_COMPRESS_ON variable was being done here for
**	    every query, which is bad for performance.  Moved to
**	    initialization logic so it is only done once.
*/

STATUS
IIqry_start(gcatype, gcamask, qrylen, qryerr)
i4	gcatype;
i4	gcamask;
i4	qrylen;
char	*qryerr;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    char	ebuf[31];

    /* Initialize error information from previous queries. */
    IIlbqcb->ii_lq_error.ii_er_num = IIerOK;
    IIlbqcb->ii_lq_error.ii_er_other = 0;
    IIlbqcb->ii_lq_error.ii_er_mnum = 0;
    if (IIlbqcb->ii_lq_error.ii_er_smsg != (char *)0)
	IIlbqcb->ii_lq_error.ii_er_smsg[0] = '\0';
    if (IIlbqcb->ii_lq_error.ii_er_submsg != (char *)0)
	IIlbqcb->ii_lq_error.ii_er_submsg[0] = '\0';
    if (IIlbqcb->ii_lq_error.ii_er_msg != (char *)0)
	IIlbqcb->ii_lq_error.ii_er_msg[0] = '\0';

    /* Initialize logical key info from previous queries. */
    IIlbqcb->ii_lq_logkey.ii_lg_flags = II_LG_INIT;

    /* Check if connected */
    if ( ! (IIlbqcb->ii_lq_flags & II_L_CONNECT) )
    {
	if (qrylen == 0)
	    qrylen = STlength(qryerr);
	STlcopy(qryerr, ebuf, min(30, qrylen));
	IIlbqcb->ii_lq_curqry = II_Q_QERR;
	if (IIglbcb->ii_gl_flags & II_G_PRQUIT)
	{
	    IIlocerr(GE_COMM_ERROR, E_LQ002A_NODB, II_ERR, 1, ebuf);
	    IIabort();
	    PCexit(-1);
	}
	IIlocerr( GE_COMM_ERROR, E_LQ002E_NODB, II_ERR, 1, ebuf );
	return FAIL;
    }

    /* Check for association failure */
    if ( ! (IIlbqcb->ii_lq_flags & II_L_CONNECT) )
    {
	IIlocerr( GE_COMM_ERROR, E_LQ002D_ASSOCFAIL, II_ERR, 0, (char *)0 );
	return FAIL;
    }

    /* EVENTS have their own checking from now on */
    if (gcatype == GCA_EVENT)
	return OK;

    /*
    ** Check if query nested within a RETRIEVE/SELECT loop, or a procedure.
    ** See header comments for action taken.
    */
    if (IIlbqcb->ii_lq_flags & (II_L_RETRIEVE|II_L_DBPROC))
    {
	/* Only one such error message per error, please */
	if ((IIlbqcb->ii_lq_curqry & II_Q_QERR) == 0)
	{
	    if (qrylen == 0)
		qrylen = STlength(qryerr);
	    STlcopy(qryerr, ebuf, min(30, qrylen));
	    if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)	/* Retrieve */
		IIlocerr(GE_SYNTAX_ERROR, E_LQ002B_INQUERY, II_ERR, 1, ebuf);
	    else					/* Procedure */
		IIlocerr(GE_SYNTAX_ERROR, E_LQ002C_INPROC, II_ERR, 1, ebuf);
	}
	if (IIlbqcb->ii_lq_flags & II_L_RETRIEVE)
	    IIlbqcb->ii_lq_retdesc.ii_rd_flags |= II_R_NESTERR;
	else
	    IIlbqcb->ii_lq_prflags |= II_P_NESTERR;
	IIlbqcb->ii_lq_curqry &= ~II_Q_INQRY;
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	IIqry_end();
	return FAIL;
    }

    if (gcatype == GCA_QUERY)
    {
	gcamask |= GCA_NAMES_MASK;
	if (!(IIglbcb->ii_gl_flags & II_G_NO_COMP))
            gcamask |= GCA_COMP_MASK;
	/* If XA app, tell server to disallow commits in dynamic sql */
	if (IIXA_CHECK_IF_XA_MACRO)
	{
	    if (IIXA_CHECK_INGRES_STATE(IICX_XA_T_ING_INTERNAL))
	        gcamask |= GCA_Q_NO_XACT_STMT;
	}
    }

    /* Check if we need to flag the singleton SELECT - and turn off indicator */
    if (IIlbqcb->ii_lq_flags & II_L_SINGLE)
    {
	gcamask |= GCA_SINGLE_MASK;
    }

    /* Mark that we're in a query and initialize it */
    IIlbqcb->ii_lq_qrystat = GCA_OK_MASK;
    if (gcatype != 0)		/* Only if caller really wants to use GCA */
    {
	IIlbqcb->ii_lq_curqry = II_Q_INQRY;
	IIcgc_init_msg(IIlbqcb->ii_lq_gca, gcatype, IIlbqcb->ii_lq_dml,gcamask);
    }
    return OK;
}


/*{
+*  Name: IIqry_end - End of writing query (the complement of IIqry_start).
**
**  Description:
**	Depending on the value of 'ii_lq_curqry, do the following:
**
**	II_Q_INQRY: 	We're in a query.  Flush the last results and 
**			set the global row count and DB query status.
**
**	II_Q_DEFAULT:   This means the query never got started because of an 
**			error detected by LIBQ.   It can also mean that the
**			query got as far as reading results from the DBMS,
**			but none came, i.e., IIqry_read() called GCA which 
**			returned DONE.  Leave flags as they are.  They have 
**			either in their default state or have been set by 
**			IIqry_read().
**
**	II_Q_QERR:	A LIBQ error occurred somewhere during query execution.
**			Set the rowcount to "not applicable" (-1), unless this 
**			is a nested query, in which case the rowcount should be 
**			left around for the parent query); and set DB query 
**			status to FAIL.
**
**	II_Q_PFRESP	A previously-fetched response block has been loaded
**			by the caller.  Process just the response information
**			as though we we in a query.
**
**	In all cases, IIqry_end() does the following:
**	    -	sets 'ii_lq_curqry' to II_Q_DEFAULT because the bits in 
**		this field should never span more than one query.  
**	    -	sets 'ii_lq_dml' to QUEL, the default query language.
**	    -   if ESQL, sets status fields in the SQLCA regarding number of
**		rows processed by the DBMS.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	1. Resets 'ii_lq_curqry'.  (See above)
**	2. Sets 'ii_lq_qrystat' according to GCA status (or FAIL).
**	3. Because we are not guaranteed to always come through IIexExec and
**	   IIexDefine (see iiexec.c) we have to turn off REPEAT query execution
**	   here.  If the special status (qrystat flags that a REPEAT query was
**	   not defined) returns, then REPEAT is turned on and the query can be
** 	   defined.
**	4. Resets any query saved for debugging DBMS errors.
**	5. May set/reset the "soft" in_transaction flag - II_L_XACT.  This is
**	   reset only if the current qry resulted in a response block.
**	   If rowcount still holds LIBQGCA's initial value this means:
**	       - response block was not sent
**	       - rqstatus field holds initial value, so don't reset
**		 update qrystat because XACT info will be lost.
**	       - LIBQ's rowcount should be set to GCA_NO_ROW_COUNT because
**		 qry was non-row producing.
**	6. If a transaction just ended then close cursors.
**	7. Flushes query id's when GCA_FLUSH_QRY_IDS_MASK set in rqstatus.  It
**	   does this by calling IIqid_free.
**	8. NULL's ii_lq_cdata when GCA_NEW_EFF_USER_MASK set in rqstatus.  It 
**	   does this by freeing ii_lq_cdata and setting it to NULL
**	
**  History:
**	08-oct-1986	- Written. (ncg)
**	14-apr-1987	- Added SQLCA support (bjb)
**	27-sep-1988	- Turns on/off in_transaction state flag. (ncg)
**	19-may-1989	- Changed names of globals for multiple connects. (bjb)
**	07-sep-1989	- Turn off II_L_DYNSEL flag (set in dyn selects. (bjb).
**	30-apr-1990	- Close cursors on end-xact & handle II_Q_PFRESP. (ncg)
**	15-mar-1991	- Took out call to IIdbg_end for savequery
**			  feature.  Need to keep last query text buffer
**			  around till next query is sent down. (teresal)
**      20-mar-1991     - Look for logical key value in response block.
**                        (teresal)
**	01-nov-1991  (kathryn)
**		When Printqry is set call IIdbg_response rather than IIdbg_dump.
**		The query text is now dumped when query is sent to DBMS.
**		IIdbg_response will dump the query completion timing info.
**	02-jul-1992  (larrym)
**		Check for GCA_FLUSH_QRY_IDS_MASK and GCA_NEW_EFF_USER_MASK
**		in IIlbqcb->ii_lq_qrysta.  GCA_NEW_EFF... gets set after a SET
**		USER AUTHORIZATION or DIRECT [DIS]CONNECT statement - Free
**		mem pointed to by ii_lq_cdata, and set it to NULL.  The next
**		time this is accessed (via IIUIdbdata()), the structure will
**		get reallocated and filled in by ui.
**		GCA_FLUSH_QRY_IDS_MASK gets set after a SET USER AUTHORIZATION,
**		SET [NO]GROUP, or SET [NO]ROLE statement - Flush the query id's
**		by calling IIqid_free().
**	21-feb-1993 (larrym)
**	    Check for illegal transcation statement if this is an XA 
**	    application.  Also, check for rollback, and notify xa via
**	    IIXA_SET_ROLLBACK_VALUE callback macro
**	21-Apr-1994 (larrym)
**	    Fixed bug 59216.  We now call IIsqEnd unconditionally (as apposed
**	    to when the user declared an SQLCA) so that SQLSTATE 
**	    can get updated.
*/

void
IIqry_end()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    bool	got_resp_time = FALSE;	/* For "printqry" feature */

    /*
    ** Turn off REPEAT (see Side Effects for reason) whether we are in a query
    ** or not (someone may have nested a REPEAT inside a RETRIEVE!).  We check
    ** for the "repeat unknown" mask so that the current code generation will
    ** continue to work.  The "repeat unknown" mask is set by:
    ** 1. IIexExec when finding no query id.  This falls through to IIqry_end
    **    and will leave the REPEAT flag on.
    ** 2. IIqry_end when LIBQ thought it had an id but DBMS didn't and was not
    **    a retrieval.  In this case we will turn the REPEAT flag on when
    **    reading the response in this routine.
    ** 3. IIqry_read when LIBQ thought it had an id but DBMS didn't and WAS 
    **    a retrieval.  This mask was then gotten in IIqry_read (reading the
    **    response) and will come through here as though it was case 1.
    ** 4. See IIqry_read why the flag may be turned off earlier.
    */
    if (IIlbqcb->ii_lq_qrystat & GCA_RPQ_UNK_MASK)
	IIlbqcb->ii_lq_flags |= II_L_REPEAT;
    else
	IIlbqcb->ii_lq_flags &= ~II_L_REPEAT;

    /* 
    ** Whether an error occurred or not, if we're in a query we must read
    ** any results.
    */
    if ( IIlbqcb->ii_lq_curqry & II_Q_INQRY  &&  
	 IIlbqcb->ii_lq_flags & II_L_CONNECT )
    {
	/* Read results returning */
	while (IIcgc_read_results(IIlbqcb->ii_lq_gca, TRUE, GCA_RESPONSE))
	{
	    /* After first data read, record response time for "printqry" */
	    if ( ! got_resp_time )  IIdbg_response( IIlbqcb, TRUE );
	    got_resp_time = TRUE;
	}

	IIlbqcb->ii_lq_rowcount = IIlbqcb->ii_lq_gca->cgc_resp.gca_rowcount;
	IIlbqcb->ii_lq_qrystat  = IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus;
	IIlbqcb->ii_lq_stamp[0] = IIlbqcb->ii_lq_gca->cgc_resp.gca_rhistamp;
	IIlbqcb->ii_lq_stamp[1] = IIlbqcb->ii_lq_gca->cgc_resp.gca_rlostamp;

	/*
        ** Look for logical key value to be returned in the
        ** response block.  If it is there, store it away for later
        ** use (i.e., for INQUIRE_INGRES).  Need to check protocol
        ** level here because the previous DBMS version didn't return any
        ** logical key data.
        */
        if ((IIlbqcb->ii_lq_gca->cgc_version >= GCA_PROTOCOL_LEVEL_40) &&
            ((IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus & GCA_OBJKEY_MASK) ||
            (IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus & GCA_TABKEY_MASK)))
        {
	    MECOPY_CONST_MACRO((PTR)IIlbqcb->ii_lq_gca->cgc_resp.gca_logkey,
                (u_i2)sizeof(IIlbqcb->ii_lq_gca->cgc_resp.gca_logkey),
                (PTR)IIlbqcb->ii_lq_logkey.ii_lg_key);

            /* Set flags to tell us what logical key types we have */
            if (IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus & GCA_OBJKEY_MASK)
                IIlbqcb->ii_lq_logkey.ii_lg_flags |= II_LG_OBJKEY;
            if (IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus & GCA_TABKEY_MASK)
                IIlbqcb->ii_lq_logkey.ii_lg_flags |= II_LG_TABKEY;
        }
        else
            IIlbqcb->ii_lq_logkey.ii_lg_flags = II_LG_INIT;

	/*
	** See comment above - this will happen if a non-retrieval REPEAT
	** query was thought to be defined by LIBQ but not by the server.
	*/
	if (IIlbqcb->ii_lq_qrystat & GCA_RPQ_UNK_MASK)
	    IIlbqcb->ii_lq_flags |= II_L_REPEAT;	/* Turn it back on */

	/* check for illegally executed transaction statement. */
	if (IIXA_CHECK_IF_XA_MACRO)
	{
	    if (IIlbqcb->ii_lq_qrystat & GCA_ILLEGAL_XACT_STMT)
	    {
	        IIlocerr(GE_LOGICAL_ERROR, E_LQ00D3_XA_ILLEGAL_XACT_STMT, 
			 II_ERR, 0, (PTR)0);
	        /* 
	        ** this error doesn't abort the transaction, so call this here.
	        ** Normally, this is called when the transaction has been 
	        ** aborted.
	        */
	        IIXA_SET_ROLLBACK_VALUE;
	    }
	}

	if (IIlbqcb->ii_lq_rowcount == GCA_ROWCOUNT_INIT)
	    /* No response block returned */
	    IIlbqcb->ii_lq_rowcount = GCA_NO_ROW_COUNT;
	/* 
	** Only set/unset transaction state if response block received.
	** "terminate xact" then turns off LIBQ's transaction state
	*/
	else if (IIlbqcb->ii_lq_qrystat & GCA_LOG_TERM_MASK)
	{
	    /* If we were in a transaction then close all cursors */
	    if (IIlbqcb->ii_lq_flags & II_L_XACT)
	    {
		IIcsAllFree( IIlbqcb );
	        /* Check for rolled back statement (if XA application) */
		if (IIXA_CHECK_IF_XA_MACRO)
		{
		    if (II_DBERR_MACRO(IIlbqcb))
		    {
		        IIXA_SET_ROLLBACK_VALUE;
		    }
		}
	    }
	    IIlbqcb->ii_lq_flags &= ~II_L_XACT;	/* Out of transaction */
	}
	else
	    IIlbqcb->ii_lq_flags |= II_L_XACT;	/* In a transaction */
	/*
	**  Check for GCA_FLUSH_QRY_IDS_MASK and GCA_NEW_EFF_USER_MASK
	*/
	if (IIlbqcb->ii_lq_qrystat & GCA_FLUSH_QRY_IDS_MASK)
	{
	   IIqid_free( IIlbqcb );
	}
	if (IIlbqcb->ii_lq_qrystat & GCA_NEW_EFF_USER_MASK)
	  /* free ii_lq_cdata and set it to NULL */ 
	    if(IIlbqcb->ii_lq_cdata != NULL) 
	    {
	       MEfree(IIlbqcb->ii_lq_cdata);
	       IIlbqcb->ii_lq_cdata = NULL;
	    }
    }

    else if (IIlbqcb->ii_lq_curqry & II_Q_PFRESP)
    {
	/* Process a pre-fetched response w/o reading from GCA */
	IIlbqcb->ii_lq_rowcount = IIlbqcb->ii_lq_gca->cgc_resp.gca_rowcount;
	IIlbqcb->ii_lq_qrystat  = IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus;
	IIlbqcb->ii_lq_stamp[0] = IIlbqcb->ii_lq_gca->cgc_resp.gca_rhistamp;
	IIlbqcb->ii_lq_stamp[1] = IIlbqcb->ii_lq_gca->cgc_resp.gca_rlostamp;

	/* Did we just terminate a transaction */
	if (IIlbqcb->ii_lq_qrystat & GCA_LOG_TERM_MASK)
	{
	    if (IIlbqcb->ii_lq_flags & II_L_XACT)
		IIcsAllFree( IIlbqcb );
	    IIlbqcb->ii_lq_flags &= ~II_L_XACT;
	}
	else
	{
	    IIlbqcb->ii_lq_flags |= II_L_XACT;
	}
    }

    else if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
    {
	/* Don't reset rowcount if we're nested in a retrieve  */
	if ((IIlbqcb->ii_lq_retdesc.ii_rd_flags & II_R_NESTERR) == 0)
	    IIlbqcb->ii_lq_rowcount = GCA_NO_ROW_COUNT;
	IIlbqcb->ii_lq_qrystat = FAIL;
    }

    /* 
    ** ESQL statements set rowcount and warning information in SQLCA and
    ** SQLSTATE
    */
    IIsqEnd( thr_cb );

    /* Reset current query state, query language and dynamic select flag */
    IIlbqcb->ii_lq_curqry = II_Q_DEFAULT;
    IIlbqcb->ii_lq_dml = DB_QUEL;
    IIlbqcb->ii_lq_flags &= ~II_L_DYNSEL;

    /* Dump saved query for "printqry" if we haven't already */
    if ( ! got_resp_time )  IIdbg_response( IIlbqcb, TRUE );
}


/*{
+*  Name: IIqry_read - End of writing query but prepare to read results.
**
**  Description:
**	If we're in a query then set up for reading results returning.
**	If this executes successfully then we should be in the middle of the
**	control flow generated by the preprocessor.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	gcatype	- Type to send to IIcgc_read_results (for reading).
**
**  Outputs:
**	Returns:
**	    STATUS - Mapping of result of IIcgc_read_results
**		OK    - More data coming.
**		FAIL  - No more data. Note that if this value (which is probably
**		     in error to the caller) the caller should NOT set II_Q_QERR
**		     as this is the final call to GCA.  If they do set II_Q_QERR
**		     then the status flags may be reset in IIqry_end.
**	Errors:
**	    None
-*
**  Side Effects:
**	1. Sets the Q_QERR flag in 'ii_lq_curqry', if we are not in a 
**	   query.  Then IIqry_end will reset the IIlbqcb flags.  If
**	   IIcgc_read_results() fails to return any data, then the Q_DEFAULT
**	   flag is set in 'ii_lq_curqry'.  This insures that IIqry_end
**	   will not reset the IIlbqcb flags.  (The caller, however, will
**	   be aware from the return status that something failed.) 
**	2. See IIqry_end for other side effects.
**	
**  History:
**	08-oct-1986	- Written. (ncg)
**	27-sep-1988	- Turns off in_transaction state flag. (ncg)
**	19-apr-1989	- Changed names of globals for multiple connects. (bjb)
**	02-aug-1989	- Check that response block received before resetting
**			  xact info.  (barbara)
**	30-apr-1990	- Close all cursors on end-xact. (neil)
**	01-nov-1991  (kathryn)
**              When Printqry is set call IIdbg_response rather than IIdbg_dump.
**              The query text is now dumped when query is sent to DBMS.
**              IIdbg_response will dump the query response timing info.
*/

STATUS
IIqry_read( II_LBQ_CB *IIlbqcb, i4  gcatype )
{
    STATUS	retval = OK;			/* Return value */

    /*
    ** Turn off REPEAT flag always (see IIqry_end for 1-3).
    ** 4. If a repeat RETRIEVE successfully starts we arrive here before
    **    retrieving tuple data.  We must make sure the code does not fall
    **    through into the define phase of a repeat query so turn off the flag.
    */
    IIlbqcb->ii_lq_flags &= ~II_L_REPEAT;

    /* 
    ** Whether an error occurred or not, if we're in a query we must read
    ** first results.
    */
    if (IIlbqcb->ii_lq_curqry & II_Q_INQRY)
    {
	if (IIcgc_read_results(IIlbqcb->ii_lq_gca, FALSE, gcatype) == 0)
	{
	    retval = FAIL;
	    IIlbqcb->ii_lq_rowcount = IIlbqcb->ii_lq_gca->cgc_resp.gca_rowcount;
	    IIlbqcb->ii_lq_qrystat  = IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus;
	    IIlbqcb->ii_lq_stamp[0] = IIlbqcb->ii_lq_gca->cgc_resp.gca_rhistamp;
	    IIlbqcb->ii_lq_stamp[1] = IIlbqcb->ii_lq_gca->cgc_resp.gca_rlostamp;
	    IIlbqcb->ii_lq_curqry = II_Q_DEFAULT;	/* Reset query state */
	    /*
	    ** If we get an undefined repeat query error, then make sure we
	    ** pretend there was an error so callers do not try to issue
	    ** another error.  This would happen if we thought we had an id
	    ** and we were in a RETRIEVE, and the DBMS had no id.
	    */
	    if (IIlbqcb->ii_lq_qrystat & GCA_RPQ_UNK_MASK)
	    {
		IIlbqcb->ii_lq_error.ii_er_num = E_LQ004B_UNDEFREP;
		IIlbqcb->ii_lq_error.ii_er_other = E_LQ004B_UNDEFREP; /*safety*/
	    }

	    if (IIlbqcb->ii_lq_rowcount == GCA_ROWCOUNT_INIT)
	        /* No response block returned */
		IIlbqcb->ii_lq_rowcount = GCA_NO_ROW_COUNT;
	    /* 
	    ** Only set/unset transaction state if response block received.
	    ** "terminate xact" then turns off LIBQ's transaction state
	    */
	    else if (IIlbqcb->ii_lq_qrystat & GCA_LOG_TERM_MASK)
	    {
		/* If we were in a transaction then close all cursors */
		if (IIlbqcb->ii_lq_flags & II_L_XACT)
		    IIcsAllFree( IIlbqcb );
		IIlbqcb->ii_lq_flags &= ~II_L_XACT;	/* Out of transaction */
	    }
	    else
		IIlbqcb->ii_lq_flags |= II_L_XACT;	/* In a transaction */
	}
	/* 
	** Else we do not set row number, or reset query state till the
	** query has actually ended.
	*/

	/* Record response time for "printqry" */
	IIdbg_response( IIlbqcb, FALSE );
    }
    else
    {
	/* There's probably an error number hanging around */
	IIlbqcb->ii_lq_curqry = II_Q_QERR;
	retval = FAIL;
    }

    return retval;
}
