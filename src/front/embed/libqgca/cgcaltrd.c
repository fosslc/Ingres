/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<er.h>
# include	<me.h>
# include	<cm.h>
# include	<st.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<erlc.h>

/**
**  Name: cgcaltrd.c - This file manages "alternate buffer read" processing.
**
**  Description:
**	The routines in this file provide an alternate source of data that
**	can be read by the caller (LIBQ).  The "alternate read streams" can
**	be attached to a query, loaded, and then used as a data source.
**	Typically there will be one such stream per cursor query.  Multiple
**	cursors may be open at the same time, and some of them may have
**	alternate buffer streams associated with them.
**
**	Operations in this file may be traced through the setting of the
**	IICGC_ATRACE flag.  This flag may be set through the II_EMBED_SET
**	and SET_INGRES value PRINTCSR.
**
**  Defines:
**	IIcga1_open_stream	- Allocate empty stream.
**	IIcga2_close_stream	- Free stream.
**	IIcga3_load_stream	- Load a stream with data.
**	IIcga4_load_error	- Load an out-of-band error into stream.
**	IIcga5_get_data		- Alternate read operation.
**	IIcga6_reset_stream	- Reset stream on interrupt.
**
**  Examples:
**    The following examples shows the call flow into this module for
**    cursor processing:
**
**    OPEN:
**	detect that cursor is pre-fetchable;
**	IIcga1_open_stream(cgc, open-flag, qry-id, cursor-row-size,
**			   user-pre-fetch-rows, new-stream);
**
**    Initial FETCH:
**	issue GCA_FETCH query with #-rows;
**	IIcga3_load_stream(cgc, stream-to-load);
**
**    FETCH from stream:
**	set cgc_alternate to pre-fetched-stream;
**	allow user to get to IIcgc_get_value which will then call:
**	IIcga5_get_data(cgc, GCA_TUPLES, number-bytes, destination);
**
**    CLOSE:
**	IIcga2_close_stream(cgc, old-stream);
**
**  History:
**	23-apr-1990 (neil)
**	    Written for cursor performance project.
**	15-jun-1990 (barbara)
**	    Check parameter cgc for null because query processing
**	    now continues even if there is no connection. (Bug 21832)
**	27-jul-1990 (barbara)
**	    Changed formula upon which row count for prefetch is based.
**	    Originally it was based upon a fixed number of buffers; now
**	    it is based upon a fixed number of bytes.
**	27-feb-1992 (davel)
**	    Fixed bug 42265 - corrected calculation of number of pre-fetch rows
**	    when user specifies more than IICGC_AMAX_BYTES worth of rows.
**	01-nov-1992 (larrym)
**	    Review for SQLSTATE issues.  Code should handle extra element of
**	    IICGC_HARG with no mods.  Note that although the DBMS sends 
**	    SQLSTATE info in the gca_id_error field, when it gets passed to 
**	    the cgc handler, it will recognize the presence of SQLSTATE 
**	    from the protocol level and set it accordingly.
**	23-feb-1993 (mohan) 
**		Cast argument to cgc_handle.
**      05-apr-1996 (loera01)
**          Modified IIcga5_get_data() to support compressed variable-length
**          datatypes.
**	25-aug-1997 (walro03)
**	    Data lost in vchar or varchar columns in tables created in the SQL
**	    terminal monitor.  Change compressed_type from bool to i2.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**      23-Dec-1999 (hanal04)
**          Compressed code was not handling the case where the embedded
**          length charaters in a varchar spanned two buffers. Apply the
**          same logic as jenjo01's 28-Oct-1997 fix in IIcc1_read_bytes()
**          to IIcga5_get_data().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-may-2001 (loera01)
**          Add support for Unicode varchar compression.
**      06-Aug-2002 (horda03) Bug 108464.
**          Memory overrun occurs if reading compressed nullable varchars.
**          Trying to add the NULL character to the end of the buffer was
**          writing beyond the length of the buffer, if the compressed
**          nullable varchar spanned two buffers. Really this is the same
**          bug as 103786.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**/

/*{
**  Name: IIcga1_open_stream	- Allocate/open an alternate read stream.
**
**  Description:
**	This routine is used to initialize an alternate stream from which 
**	later data retrieval requests will be executed.  This routine allocates
**	the base structure of the alternate read stream and assigns some
**	fields initial values.  The row count for each read operation is
**	bound when the stream is opened.  This internal count (cgc_arows) is
**	set to a function of the tuple size (tup_size), GCA message buffer size
**	(those have to be allocated) and a user row count (usr_pfrows).
**
**	if (usr_rows = 0) then
**	    calculate a default based on IICGC_ADEF_BYTES / tupsize;
**	else
**	    bytes = usr_rows * tupsize;
**	    clip bytes to IICGC_AMAX_BYTES;
**	    cgc_arows = bytes / tupsize;
**	endif;
**
**  Inputs:
**	cgc			Client general communications descriptor:
**	    .cgc_d_length 	To determine how many default rows to request
**				together with tup_size.
**	flags			Open stream state flag modifiers:
**				    IICGC_ATRACE - Trace this stream
**	qid			Query id for tracing & errors in certain cases.
**	tup_size		Maximum row size for the current object.
**	usr_pfrows		User value for the number of rows to pre-fetch
**				for all GCA reads against this object.  See
**				above for request row calculation.
**  Outputs:
**	astream			Set to point at newly allocated & cleared
**				stream or NULL if could not allocate.  Caller
**				is expected to handle allocation errors.
**	    .cgc_atupsize	Set to tup_size passed in.
**	    .cgc_arows		Set to "best" fit (see above).
**	    .cgc_aflags		Set to input flags.
**	    .cgc_aname		Set to [truncated] input name.
**	    .cgc_aresp		Alternate response block:
**		.gca_rowcount	Set to GCA_ROWCOUNT_INIT to avoid assuming = 0.
**      Returns:
**	    STATUS		Success or Failure
**      Errors:
**	    Memory allocation handled by caller.  If cannot allocate then
**	    caller should go through regular non-prefetch processing.
**
**  Side Effects:
**	Memory allocation.
**
**  History:
**	23-apr-1990 (neil)
**	    Written for cursor performance.
**	15-jun-1990 (barbara)
**	    Check parameter cgc for null because query processing
**	    now continues even if there is no connection. (Bug 21832)
**      27-jul-1990 (barbara)
**	    Changed formula upon which row count for prefetch is based.
**	    Originally it was based upon a fixed number of buffers; now
**	    it is based upon a fixed number of bytes.
**	24-Feb-2010 (kschendel) SIR 123275
**	    Old algorithm assumed that IICGC_ADEF_BYTES was larger than
**	    the GCA buffer, which isn't necessarily true any more.  Revise
**	    the calculations to not assume that GCA buffers are small.
*/

STATUS
IIcga1_open_stream(cgc, flags, qid, tup_size, usr_pfrows, astream)
IICGC_DESC	*cgc;
i4		flags;
char		*qid;
i4		tup_size;
i4		usr_pfrows;
IICGC_ALTRD	**astream;
{
    IICGC_ALTRD	*cga;		/* New alternate stream to allocate */
    i4		gc_buf_size;
    i4		bytes;
    char	*row_why;	/* For tracing */

    if (cgc == (IICGC_DESC *)0)
	return FAIL;

    *astream = NULL;
    cga = (IICGC_ALTRD *)MEreqmem(0, sizeof(*cga), TRUE, (STATUS *)NULL);
    if (cga == NULL)	/* Caller can handle the error or not use stream */
	return FAIL;

    /* Figure out the number of rows */
    cga->cgc_aflags = flags;
    cga->cgc_atupsize = tup_size;
    gc_buf_size = cgc->cgc_write_msg.cgc_d_length;
    if (usr_pfrows <= 0)
    {
	/* round up to next multiple of GCA buffer size -- no deep dark
	** reason, but eliminates extra round-trips at the end of the
	** prefetch.
	*/
	bytes = ((IICGC_ADEF_BYTES + gc_buf_size - 1) / gc_buf_size) * gc_buf_size;
	row_why = "default";
    }
    else if (usr_pfrows > IICGC_AMAX_BYTES / tup_size)
    {
	bytes = ((IICGC_AMAX_BYTES + gc_buf_size - 1) / gc_buf_size) * gc_buf_size;
	row_why = "user count (clipped)";
    }
    else
    {
	/* Don't align specific user row count, just use it */
	bytes = usr_pfrows * tup_size;
	row_why = "user count";
    }
    cga->cgc_arows = bytes / tup_size;
    if (cga->cgc_arows < 1)
	cga->cgc_arows = 1;

    /* Copy query/cursor/caller id (truncate if required - just for display) */
    STlcopy(qid, cga->cgc_aname, DB_MAXNAME-1);
    /* Set response rowcount to non-0 value */
    cga->cgc_aresp.gca_rowcount = GCA_ROWCOUNT_INIT;
    if (cga->cgc_aflags & IICGC_ATRACE)
    {
	SIprintf(ERx("PRTCSR* - Open %s: row-length = %d, #-rows = %d (%s)\n"),
		 cga->cgc_aname, cga->cgc_atupsize, cga->cgc_arows, row_why);
    }
    *astream = cga;
    return OK;
} /* IIcga1_open_stream */

/*{
**  Name: IIcga2_close_stream	- Close/deallocate an alternate read stream.
**
**  Description:
**	This routine is used to deallocate an alternate stream which may have
**	been filled with previous read data.  Local buffers are deallocated,
**	followed by the descriptor itself.
**
**  Inputs:
**	cgc			Client general communications descriptor.  This
**				may be NULL if the client is deallocating the
**				stream after they have disconnected from GCA.
**	astream			Stream to deallocate:
**	    .cgc_amsgq		Any buffers on queue are deallocated prior to
**				deallocating astream. 
**  Outputs:
**	None
**      Returns:
**	    VOID
**      Errors:
**	    None
**
**  Side Effects:
**	Memory deallocation.
**
**  History:
**	23-apr-1990 (neil)
**	    Written for cursor performance.
*/

VOID
IIcga2_close_stream(cgc, astream)
IICGC_DESC      *cgc;
IICGC_ALTRD     *astream;
{
    IICGC_ALTRD         *cga;           /* Alternate read descriptor */
    IICGC_ALTMSG	*cgamsg;	/* To walk list of buffer */
    IICGC_ALTMSG	*cgamsg_next;
    i4			fc = 0;		/* Trace free count */

    cga = astream;
    for (cgamsg = cga->cgc_amsgq; cgamsg != NULL; cgamsg = cgamsg_next)
    {
	cgamsg_next = cgamsg->cgc_anext;
	_VOID_ MEfree((PTR)cgamsg);
	fc++;
    }
    if (cga->cgc_aflags & IICGC_ATRACE)
    {
	SIprintf(ERx("PRTCSR* - Close %s: free %d message buffer(s)\n"), 
		 cga->cgc_aname, fc);
	SIflush(stdout);
    }
    /* Check if cgc is non-NULL as we may already be disconnected */
    if (cgc && cgc->cgc_alternate == cga)
	cgc->cgc_alternate = NULL;
    _VOID_ MEfree((PTR)cga);
} /* IIcga2_close_stream */

/*{
**  Name: IIcga3_load_stream	- Load/pre-fetch an alternate read stream.
**
**  Description:
**	This routine is used to pre-fetch 0 or more GCA_TUPLES messages so
**	that subsequent read operations may execute against a loaded buffer
**	queue rather than against GCA and the DBMS server.
**
**	When encountering errors while reading ahead the errors are inserted
**	into the buffer chain.  These errors normally are returned to the
**	user when the corresponding read request encounters the error buffer.
**	This may be complicated if the error terminates the tuple data or
**	if the error is a session-failure error.
**
**	Note that this routine may end up allocating nothing.  For example,
**	if the caller requested N rows but there was only 1, the the DBMS
**	will return end-of-data after the first row.  The caller, however,
**	may decide to return the end-of-data indicator to the user on the
**	*next* request rather than with this row (as would be done for a 
**	cursor).
**
**  Algorithm:
**	chain = NULL;
**	set stream IICGC_ALOADING flag;
**	set cgc_alternate for internal loading;
**	while (IIcgc_more_data(cgc, GCA_TUPLES) > 0) loop
**	    if (data length = 0) then skip;
**	    allocate data length + alternate message header (w/o clear);
**	    if (cannot allocate) then
**		allocation error;
**		free all message buffers & reset state;
**		return to caller and allow them to flush data;
**	    endif;
**	    copy all data & reset fields;
**	    total length += data length;
**	    if (total length / tuple size) > current_row_request then
**		protocol error;
**		free all message buffers & reset state;
**		return to caller and allow them to flush data;
**	    endif;
**	    if (error loaded by IIcga4_load_error - IICGC_AERRLOAD) then
**		recalculate chain;
**		reset IICGC_AERRLOAD flag;
**	    endif;
**	    if (chain = NULL) then
**		first buffer in queue;
**	    else
**		chain->cgc_anext = buffer;
**	    endif;
**	    chain = buffer;
**	    used data length = data length;	-- Allowed to read next
**	endloop;
**	copy response message to local one;
**	if (response rowcount > current_row request) then
**	    protocol error;
**	    free all message buffers & reset state;
**	endif;
**	clear stream IICGC_ALOADING flag (and on all returns);
**	unset cgc_alternate (and on all returns);
**
**  Inputs:
**	cgc			Client general communications descriptor to
**				use to access normal CGC read routines.
**	    .cgc_alternate	During processing this field is set internally
**				to allow called routines to know that we are
**				are in the process of loading up a stream.  In
**				particular error handling.
**	astream			Stream to load with new data.
**
**  Outputs:
**	astream			Stream to load with new data:
**	    .cgc_amsgq		Set to point at list of unread buffers.
**	    .cgc_aresp		Final response after loading stream.
**      Returns:
**	    STATUS		Success or Failure
**      Errors:
**	    Memory allocation errors - if encountered then queue is flushed.
**	    E_LC0028_AREAD_LOAD_ALLOC - Could not allocate buffer.
**	    E_LC0029_AREAD_ROW_COUNT - Requested row count exceeded.
**	    E_LC002A_AREAD_WRONG_MESSAGE - Encountered wrong message type.
**
**  Side Effects:
**	Memory is allocated for all the message buffers as they are read in.
**
**  History:
**	23-apr-1990 (neil)
**	    Written for cursor performance.
**	15-jun-1990 (barbara)
**	    Check parameter cgc for null because query processing
**	    now continues even if there is no connection. (Bug 21832)
*/

STATUS
IIcga3_load_stream(cgc, astream)
IICGC_DESC	*cgc;
IICGC_ALTRD	*astream;
{
    IICGC_ALTRD		*cga;		/* Alternate read descriptor */
    IICGC_ALTMSG	*cgamsg;	/* New message to queue */
    IICGC_ALTMSG	*cgamsg_last;	/* Last in queue */
    IICGC_MSG		*cgrmsg;	/* GCA read message buffer */
    i4			msg_size;	/* Allocation size required */
    i4			cur_dlen;	/* Current data length */
    i4		tot_size;	/* Total data size */
    i4		tot_errs;	/* Total error messages */
    char		stat_buf1[20];	/* Status buffer */
    char		stat_buf2[20];
    char		stat_buf3[30];
    i4			mpos;		/* Trace - message position */

    if (cgc == (IICGC_DESC *)0)
	return FAIL;
    cga = astream;
    cga->cgc_amsgq = NULL;
    cgamsg_last = NULL;
    cga->cgc_aflags |= IICGC_ALOADING;	/* Mark that we're now loading */
    cgc->cgc_alternate = cga;
    cgrmsg = &cgc->cgc_result_msg;
    tot_size = 0;
    tot_errs = 0;
    mpos = 0;

    if (cga->cgc_aflags & IICGC_ATRACE)
    {
	SIprintf(ERx("PRTCSR* - Load %s: asked-rows = %d, row-length = %d\n"),
		 cga->cgc_aname, cga->cgc_arows, cga->cgc_atupsize);
    }

    while (IIcgc_more_data(cgc, GCA_TUPLES) > 0)
    {
	/*
	** If this is the first then part of the message may already be used,
	** and therefore subtract the used part.  Otherwise it's zero.
	*/
	cur_dlen = cgrmsg->cgc_d_length - cgrmsg->cgc_d_used;
	if (cur_dlen == 0)	  		/* Skip empty messages */
	{
	    if (cga->cgc_aflags & IICGC_ATRACE)
		SIprintf(ERx("PRTCSR* - Load: skip empty message\n"));
	    continue;
	}

	/* Allocate the message buffer for amount of data not yet read */
	msg_size = sizeof(*cgamsg) + cur_dlen;
	cgamsg = (IICGC_ALTMSG *)MEreqmem(0, msg_size, FALSE, (STATUS *)NULL);
	if (cgamsg == NULL)
	{
	    /* Could not allocate message buffer */
	    CVna(msg_size, stat_buf1);
	    IIcc1_util_error(cgc, FALSE, E_LC0028_AREAD_LOAD_ALLOC, 2,
			     IIcc2_util_type_name(cgrmsg->cgc_message_type,
						  stat_buf3),
			     stat_buf1, (char *)0, (char *)0);
	    IIcga6_reset_stream(cgc);
	    return FAIL;
	}
	/* Initialize fields and copy data into our new message */
	cgamsg->cgc_anext = NULL;
	cgamsg->cgc_amsg_type = GCA_TUPLES;
	cgamsg->cgc_aused = 0;
	cgamsg->cgc_adlen = cur_dlen;
	MEcopy(cgrmsg->cgc_data + cgrmsg->cgc_d_used, cgamsg->cgc_adlen,
	       cgamsg->cgc_adata);
	tot_size += cgamsg->cgc_adlen;
	if ((tot_size / cga->cgc_atupsize) > cga->cgc_arows)
	{
	    /* Wrong # of rows encountered */
	    CVna(cga->cgc_arows, stat_buf1);
	    CVna(tot_size/cga->cgc_atupsize, stat_buf2);
	    IIcc1_util_error(cgc, FALSE, E_LC0029_AREAD_ROW_COUNT, 3,
			     IIcc2_util_type_name(cgrmsg->cgc_message_type,
						  stat_buf3),
			     stat_buf1, stat_buf2, (char *)0);
	    IIcga6_reset_stream(cgc);
	    return FAIL;
	}
	/* Add new message onto queue - check if out-of-band error occurred */
	if (cga->cgc_aflags & IICGC_AERRLOAD)
	{
	    /* Recalculate trailing message and turn off error flag */
	    if (   (cgamsg_last != NULL)
		|| (cgamsg_last = cga->cgc_amsgq) != NULL)
	    {
		while (cgamsg_last->cgc_anext != NULL)
		{
		    mpos++;
		    tot_errs++;
		    cgamsg_last = cgamsg_last->cgc_anext;
		}
	    }
	    cga->cgc_aflags &= ~IICGC_AERRLOAD;
	}
	if (cgamsg_last == NULL)		/* First in queue */
	    cga->cgc_amsgq = cgamsg;
	else					/* Sometime later */
	    cgamsg_last->cgc_anext = cgamsg;
	cgamsg_last = cgamsg;
	mpos++;
	if (cga->cgc_aflags & IICGC_ATRACE)
	{
	    SIprintf(ERx("PRTCSR* - Load: GCA_TUPLES, data-length = %d, "),
		     cgamsg->cgc_adlen);
	    SIprintf(ERx("message-# = %d\n"), mpos);
	}
	cgrmsg->cgc_d_used = cgrmsg->cgc_d_length;    /* Force next GCA read */
    }
    if (cgrmsg->cgc_message_type != GCA_RESPONSE)
    {
	/* Wrong message type encountered */
	IIcc1_util_error(cgc, FALSE, E_LC002A_AREAD_WRONG_MESSAGE, 2,
			 ERx("GCA_TUPLES, GCA_ERROR, GCA_RESPONSE"),
			 IIcc2_util_type_name(cgrmsg->cgc_message_type,
					      stat_buf3),
			 (char *)0, (char *)0);
	IIcga6_reset_stream(cgc);
	return FAIL;
    }
    STRUCT_ASSIGN_MACRO(cgc->cgc_resp, cga->cgc_aresp);
    if (cga->cgc_aresp.gca_rowcount > cga->cgc_arows)
    {
	/* Wrong # of rows encountered */
	CVna(cga->cgc_arows, stat_buf1);
	CVna(cga->cgc_aresp.gca_rowcount, stat_buf2);
	IIcc1_util_error(cgc, FALSE, E_LC0029_AREAD_ROW_COUNT, 3,
			 IIcc2_util_type_name(cgrmsg->cgc_message_type,
					      stat_buf3),
			 stat_buf1, stat_buf2, (char *)0);
	IIcga6_reset_stream(cgc);
	return FAIL;
    }

    cga->cgc_aflags &= ~IICGC_ALOADING;
    cgc->cgc_alternate = NULL;
    if (cga->cgc_aflags & IICGC_ATRACE)
    {
	SIprintf(ERx("PRTCSR* - Load: total-data-length = %d, "), tot_size);
	SIprintf(ERx("#-messages = %d, #-errors = %d\n"), mpos, tot_errs);
	SIprintf(ERx("PRTCSR* - Load: load-rows = %d, resp-rows = %d, "),
		 cga->cgc_aresp.gca_rowcount - 1, cga->cgc_aresp.gca_rowcount);
	SIprintf(ERx("status = 0x%x, eoq = %c\n"),
		 cga->cgc_aresp.gca_rqstatus,
		 cga->cgc_aresp.gca_rqstatus & GCA_END_QUERY_MASK ? 'Y' : 'N');
	SIflush(stdout);
    }
    return OK;
} /* IIcga3_load_stream */

/*{
**  Name: IIcga4_load_error	- Load error encountered while pre-fetching.
**
**  Description:
**	This routine loads an error (out of band) while IIcga3_load_stream
**	is processing data.  For example, IIcga3_load_stream will be reading
**	buffers when an error is encountered.  The error is processed normally
**	through IIcc2_read_error, where it is sent to this routine rather
**	than the caller's CGC handler.  The error will eventually be given
**	to the caller when they encounter it while reading.
**
**	IIcc2_read_error checks if cgc_alternate is set and if it's state
**	is IICGC_ALOADING.  If so, then it calls this routine with the full
**	error.  This routine will load the error in line with the other 
**	pre-fetched data and mark IICGC_AERRLOAD.  This flag indicates to
**	the loader that something may have been entered out of band and that
**	it may have to readjust the chain pointers.
**
**	If the error cannot be allocated and chained then the caller should
**	issue the error through normal error processing (cgc->cgc_handle).
**
**  Inputs:
**	cgc			Client general communications descriptor:
**	    .cgc_alternate	Loader has already set this field.
**	earg			Error argument (saved in line):
**	    .h_errorno		Error numbers
**	    .h_local
**	    .h_use		Severity/usage
**	    .h_sqlstate		sqlstate returned with error
**	    .h_length		Length of data field
**	    .h_data[0]		Pointer to data
**  Outputs:
**	cgc			Client general communications descriptor:
**	  .cgc_alternate	Alternate stream:
**	     .cgc_aflags	Set to indicate error loaded (IICGC_AERRLOAD).
**	     .cgc_amsgq		Error message queued in line.  The data saved,
**				cgc_adata, points at sizeof(earg) + h_length
**				bytes.  These are reconstructed when gotten
**				during a read.
**      Returns:
**	    STATUS		OK - was able to queue error
**				FAIL - caller should take care of it
**      Errors:
**	    Memory allocation errors - if encountered then default err handling.
**
**  Side Effects:
**	Memory is allocated for the error in the stream message queue.
**
**  History:
**	23-apr-1990 (neil)
**	    Written for cursor performance.
**	15-jun-1990 (barbara)
**	    Check parameter cgc for null because query processing
**	    now continues even if there is no connection. (Bug 21832)
*/

STATUS
IIcga4_load_error(cgc, earg)
IICGC_DESC	*cgc;
IICGC_HARG	*earg;
{
    IICGC_ALTRD		*cga;		/* Alternate read descriptor */
    IICGC_ALTMSG	*cgaerr;	/* New error message to queue */
    IICGC_ALTMSG	*cgamsg;	/* To walk message queue */
    i4			msg_size;	/* Allocation size required */
    i4			mpos;		/* Trace - message position */

    if (cgc == (IICGC_DESC *)0)
	return FAIL;

    /* Make sure we're in the right context */
    if (   (cga = cgc->cgc_alternate) == NULL
	|| (cga->cgc_aflags & IICGC_ALOADING) == 0
       )
    {
	return FAIL;
    }

    /* Allocate the error message */
    msg_size = sizeof(*cgaerr) + sizeof(*earg) + earg->h_length;
    cgaerr = (IICGC_ALTMSG *)MEreqmem(0, msg_size, FALSE, (STATUS *)NULL);
    if (cgaerr == NULL)		/* Caller can issue the error right now */
	return FAIL;
    /* Initialize fields */
    cgaerr->cgc_anext = NULL;
    cgaerr->cgc_amsg_type = GCA_ERROR;
    cgaerr->cgc_aused = 0;
    cgaerr->cgc_adlen = sizeof(*earg) + earg->h_length;
    MEcopy((PTR)earg, sizeof(*earg), cgaerr->cgc_adata);
    if (earg->h_length > 0)
    {
	MEcopy(earg->h_data[0], earg->h_length,
	       &cgaerr->cgc_adata[sizeof(*earg)]);
    }
    /* Insert onto end of queue and mark as "loaded error" */
    if ((cgamsg = cga->cgc_amsgq) == NULL)	/* First in queue */
    {
	mpos = 1;
	cga->cgc_amsgq = cgaerr;
    }
    else					/* Sometime later */
    {
	mpos = 2;
	while (cgamsg->cgc_anext != NULL)
	{
	    cgamsg = cgamsg->cgc_anext;
	    mpos++;
	}
	cgamsg->cgc_anext = cgaerr;
    }
    cga->cgc_aflags |= IICGC_AERRLOAD;		/* Let loader know */
    if (cga->cgc_aflags & IICGC_ATRACE)
    {
	SIprintf(ERx("PRTCSR* - Load: GCA_ERROR, data-length = %d, "),
		 cgaerr->cgc_adlen);
	SIprintf(ERx("message-# = %d\n"), mpos);
    }
    return OK;
} /* IIcga4_load_error */

/*{
**  Name: IIcga5_get_data	- Read from an alternate read stream.
**
**  Description:
**	Once an alternate-read operation has been detected, this routine is
**	called to read data from a pre-fetched data stream rather than from
**	GCA.  The data is extracted from the current alternate stream message
**	buffer.  This was set up in IIcga3_load_stream.
**
**	This routine is very similar to IIcc1_read_bytes except that the
**	data source is already buffered.
**
**  Algorithm:
**	if (no buffers in queue) then
**	    error - short alternate read;
**	    return 0;
**	endif;
**	buffer = first buffer;
**	while (caller request length) loop
**	    if (bytes used >= buffer data length) then
**		free current and goto next buffer;
**		if (no buffers left in queue) then
**		    error - short alternate read;
**		    return total copied;
**		endif;
**	    endif;
**	    if (message = GCA_ERROR) then
**		process error;
**		bytes used = buffer data length;
**		continue loop with next message;
**	    endif;
**	    copy min (caller length, length left in buffer);
**	    caller request length -= copy length;
**	    bytes used 		  += copy length;
**	    total copied	  += copy length;
**	endloop;
**	if at end of buffer then free current buffer;
**	return total copied;
**
**  Inputs:
**	cgc             	Client general communications descriptor:
**	    .cgc_alternate	Alternate stream where to read from.
**	message_type		The message type from which caller wants the
**                        	data.  Currently should always be GCA_TUPLES.
**      bytes_len       	Number of bytes to copy (i4).
**      compressed_type            Whether or not column is a compressed varchar.
**                              -2 compressed nullable Unicode varchar.
**                              -1 compressed nullable varchar.
**                              0 uncompressed data type.
**                              1 compressed unnullable varchar.
**                              2 compressed unnullable Unicode varchar.
**
**  Outputs:
**      bytes_ptr       	Ptr to byte sequence (char * - must be 1 byte
**				sizes).  If this pointer is not null then the
**				area pointed at is filled.  If this pointer is
**				null then the specified number of bytes is
**				skipped in line.
**      Returns:
**	    i4  	IICGC_GET_ERR - <  0 - Error block encountered.
**			other         - >= 0 - Number of bytes read.  If not
**					       same as bytes_len then caller
**                                             should report an error.
**
**      Errors:
**	    E_LC002B_AREAD_SHORT_DATA - Short read on pre-fetched data.
**
**  Side Effects:
**	1. Each message buffer whose data is exhausted is deallocated.
**	2. Internal data counters are reset as data is read. 
**
**  History:
**	23-apr-1990 (neil)
**	    Written for cursor performance.
**	15-jun-1990 (barbara)
**	    Check parameter cgc for null because query processing
**	    now continues even if there is no connection. (Bug 21832)
**      05-apr-1996 (loera01)
**          Modified to support compressed variable-length datatypes.
**      23-Dec-1999 (hanal04)
**          Compressed code was not handling the case where the embedded
**          length charaters in a varchar spanned two buffers. Apply the
**          same logic as jenjo01's 28-Oct-1997 fix in IIcc1_read_bytes().
**      19-Mar-2002 (hweho01)
**          Use base_ptr to save the beginning address of the buffer,     
**          then later use it in the null flag copying if it is a 
**          compressed type. Otherwise, the memory overlay would occur  
**          if running in the loop for more than one time. (Bug 107364)   
**      06-Aug-2002 (horda03) Bug 108464.
**          Memory overrun occurs if reading compressed nullable varchars.
**          Trying to add the NULL character to the end of the buffer was
**          writing beyond the length of the buffer, if the compressed
**          nullable varchar spanned two buffers. Really this is the same
**          bug as 103786.
*/

i4
IIcga5_get_data(cgc, message_type, bytes_len, bytes_ptr,compressed_type)
IICGC_DESC	*cgc;
i4		message_type;
i4		bytes_len;
char		*bytes_ptr;
i2	        compressed_type;
{
    IICGC_ALTRD		*cga;		/* Alternate stream */
    IICGC_ALTMSG	*cgamsg;	/* Alternate message buffer */
    i4			count;		/* Count value to return */
    i4			user_len;	/* User length requested (runs down) */
    i4			cplen;		/* Copy length */
    bool		err;		/* Was there a GCA_ERROR message */
    char		stat_buf1[20];	/* Status buffer */
    char		stat_buf2[20];
    char		stat_buf3[30];
    IICGC_HARG		harg;		/* To call caller's error handler */

    u_i2                vchar_len = 0;  /* Vchar length */
    i4                  frag_embed_length=FALSE; /* Was embedded length */
                                                 /* fragmented? */
    char		*bytes_ptr_save = bytes_ptr;

    err = FALSE;
    count = 0;
    user_len = bytes_len;

    if (cgc == (IICGC_DESC *)0)
	return 0;

    /* Check if there's any data buffered */
    if (   (cga = cgc->cgc_alternate) == NULL
	|| (cgamsg = cga->cgc_amsgq) == NULL
       )
    {
	/* Short read on pre-fetched data */
	CVna(bytes_len, stat_buf1);
	CVna(count, stat_buf2);
	IIcc1_util_error(cgc, FALSE, E_LC002B_AREAD_SHORT_DATA, 3,
			 stat_buf1, stat_buf2,
			 IIcc2_util_type_name(message_type, stat_buf3), 
			 (char *)0);
	return count;
    }

    /* 
    ** Keep copying data into callers result buffer, until there is no
    ** more data or an error.  Allow crossing of buffer boundaries.
    */
    if (cga->cgc_aflags & IICGC_ATRACE)
    {
	SIprintf(ERx("PRTCSR* - Get from %s: %s %d bytes\n"), cga->cgc_aname,
		 bytes_ptr ? ERx("requesting") : ERx("skipping"), bytes_len);
    }
    while (user_len > 0)		/* While caller wants more */
    {
	/*  If there is no more data then read another buffer */
	if (cgamsg->cgc_aused >= cgamsg->cgc_adlen)
	{
	    if (cga->cgc_aflags & IICGC_ATRACE)
	    {
		SIprintf(ERx("PRTCSR* - Get: free %s message, %d bytes used\n"),
			 IIcc2_util_type_name(cgamsg->cgc_amsg_type, stat_buf3),
			 cgamsg->cgc_aused);
	    }
	    /* Step into next buffer and free current */
	    cga->cgc_amsgq = cgamsg->cgc_anext;
	    cgamsg->cgc_anext = NULL;
	    _VOID_ MEfree((PTR)cgamsg);
	    if ((cgamsg = cga->cgc_amsgq) == NULL)	/* No more data! */
	    {
		if (err)
		    return IICGC_GET_ERR;
		/* Short read on pre-fetched data */
		CVna(bytes_len, stat_buf1);
		CVna(count, stat_buf2);
		IIcc1_util_error(cgc, FALSE, E_LC002B_AREAD_SHORT_DATA, 3,
				 stat_buf1, stat_buf2,
				 IIcc2_util_type_name(message_type, stat_buf3), 
				 (char *)0);
		return count;
	    }
	    cgamsg->cgc_aused = 0;		/* New message used count */
	} /* If no more data left in message */

	/*
	** If we reach a queued GCA_ERROR message then process it here and 
	** free it, by setting used bytes to the data length and looping.
	*/
	if (cgamsg->cgc_amsg_type == GCA_ERROR)
	{
	    if (cga->cgc_aflags & IICGC_ATRACE)
		SIprintf(ERx("PRTCSR* - Get: processing an error\n"));

	    /* Extract caller error handler argument */
	    MEcopy((PTR)cgamsg->cgc_adata, sizeof(harg), (PTR)&harg);
	    if (harg.h_length > 0)
		harg.h_data[0] = &cgamsg->cgc_adata[sizeof(harg)];
	    else
		harg.h_data[0] = ERx("");
	    _VOID_ (*cgc->cgc_handle)((i4)IICGC_HDBERROR, &harg);
	    err = TRUE;
	    cgamsg->cgc_aused = cgamsg->cgc_adlen;  /* Born to be freed */
	    continue;				    /* With outer loop */
	}

	/*
	** The length we want to copy into the caller's result buffer is the
	** minimum of the current requested length (user_len), and the number
	** of bytes left in the message buffer.  
        **
        ** If the column is compressed, reset user_len to a value dependent 
        ** on the string length embedded in the first two bytes of the varchar.
	*/
        if (compressed_type && vchar_len == 0)
        {
            if(frag_embed_length && bytes_ptr)
            {
                MEcopy((PTR)&cgamsg->cgc_adata[cgamsg->cgc_aused],
                            sizeof(char), (PTR)bytes_ptr);
                bytes_ptr--;
                user_len = (IIcgc_find_cvar_len(bytes_ptr, compressed_type) -
                                sizeof(char));
                vchar_len = user_len;
                bytes_ptr++;
            }
            else
            {
                if (min(user_len, cgamsg->cgc_adlen - cgamsg->cgc_aused)
				== sizeof(char))
                {
                    /* Second byte of vchar length is in next buffer */
                    frag_embed_length = TRUE;
                }
                else
                {
                    user_len = IIcgc_find_cvar_len(
                        &cgamsg->cgc_adata[cgamsg->cgc_aused], compressed_type);                    vchar_len = user_len;
                }
            }
        }
	cplen = min(user_len, cgamsg->cgc_adlen - cgamsg->cgc_aused);
	if (bytes_ptr)				/* Don't copy if NULL */
	{
	    MECOPY_VAR_MACRO((PTR)&cgamsg->cgc_adata[cgamsg->cgc_aused],
			     cplen, (PTR)bytes_ptr);
	}

        /* 
        **  If this is a compressed, nullable varchar, copy the null
        **  flag to the end of the column.  This, in effect, decom-
        **  presses the varchar. Use the original byte_ptr value
	**  in case it was changed due to the varchar spanning
	**  two buffers.
        */
        if (compressed_type < 0 && bytes_ptr && user_len == cplen)
        {
            *(PTR)(bytes_ptr_save + bytes_len - 1) = 
              *(PTR)(bytes_ptr + user_len - 1);
        }
        if (bytes_ptr)
            bytes_ptr += cplen;

	user_len          -= cplen;
	count             += cplen;
	cgamsg->cgc_aused += cplen;    
    } /* While more bytes to copy */

    /*  All data has been copied - can we free the current buffer? */
    if (cgamsg->cgc_aused >= cgamsg->cgc_adlen)
    {
	if (cga->cgc_aflags & IICGC_ATRACE)
	{
	    SIprintf(ERx("PRTCSR* - Get: free GCA_TUPLES message on exit, "));
	    SIprintf(ERx("%d bytes used\n"), cgamsg->cgc_aused);
	}
	cga->cgc_amsgq = cgamsg->cgc_anext;
	cgamsg->cgc_anext = NULL;
	_VOID_ MEfree((PTR)cgamsg);
	/* Note that the next entry into this routine may encounter an error */
    }

    /* If previous error, unset error for caller */
    if (err)
	_VOID_ (*cgc->cgc_handle)((i4)IICGC_HDBERROR, (IICGC_HARG *)0);
    if (compressed_type)
    {
       return bytes_len;
    }
    else
    {
       return count;	/* And count should be correct */
    }
} /* IIcga5_get_data */

/*{
**  Name: IIcga6_reset_stream	- Reset alternate stream processing.
**
**  Description:
**	Because pre-fetching will provide a greater time interval during which
**	the load operation can be interrupted we will make sure to clean
**	up any data associated with the current stream (if it is in use).
**	This can be called from IIcgc_break (for user interrupts) and from
**	internal error found during loading.
**
**	Note that we reset whether we are currently loading the stream (time
**	interval) or if we are just reading the stream (less of a chance
**	but can happen too).  If we did not do this for "reading" then the
**	caller may end up with an undefined data boundary within the stream.
**  
**  Inputs:
**	cgc             	Client general communications descriptor.
**	    .cgc_alternate	If not NULL then we make sure to clear flags
**				and free all queued buffers (if any) and reset
**				counters.  Otherwise we do nothing here.
**  Outputs:
**      Returns:
**	    VOID
**	Errors:
**	    None
**
**  Side Effects:
**	None
**
**  History:
**	23-apr-1990 (neil)
**	    Written for cursor performance.
**	15-jun-1990 (barbara)
**	    Check parameter cgc for null because query processing
**	    now continues even if there is no connection. (Bug 21832)
*/

VOID
IIcga6_reset_stream(cgc)
IICGC_DESC	*cgc;
{
    IICGC_ALTRD		*cga;			/* Stream to clear */
    IICGC_ALTMSG	*cgamsg;		/* To walk list of buffer */
    IICGC_ALTMSG	*cgamsg_next;
    i4			fc = 0;			/* Trace free count */

    if (cgc == (IICGC_DESC *)0)
	return;

    if ((cga = cgc->cgc_alternate) == NULL)
	return;

    /* Free queue of message buffers */
    for (cgamsg = cga->cgc_amsgq; cgamsg != NULL; cgamsg = cgamsg_next)
    {
	cgamsg_next = cgamsg->cgc_anext;
	_VOID_ MEfree((PTR)cgamsg);
	fc++;
    }
    cga->cgc_amsgq = NULL;
    /* Reset response data (if already gotten) */
    MEfill(sizeof(GCA_RE_DATA), 0, (PTR)&cga->cgc_aresp);
    /* Reset state flags (only leave on trace) */
    if (cga->cgc_aflags & IICGC_ATRACE)
    {
	SIprintf(ERx("PRTCSR* - Reset %s: free %d message buffer(s)\n"),
		 cga->cgc_aname, fc);
	SIflush(stdout);
	cga->cgc_aflags = IICGC_ATRACE; 
    }
    else
    {
	cga->cgc_aflags = 0; 
    }
    cgc->cgc_alternate = NULL;
} /* IIcga6_reset_stream */
