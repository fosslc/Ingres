/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<me.h>
# include	<cm.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<erlc.h>
# include	<generr.h>
# include	<sqlstate.h>

/**
+*  Name: cgcread.c - This file contains routines to read messages from GCA.
**
**  Description:
**	The routines in this file do the reading of the messages from GCA.
**	They act as a layer between user applications (as seen by LIBQ) and the
**	GCA module.  Mostly these routines just act as buffer managers
**	and police the protocol.  Nearly every routine confirms that the data
**	being copied to a user result buffer is correct (at least in size).
**
**  Defines:
**
**	IIcgc_read_results	- Set up for reading data.
**	IIcgc_event_read	- Set up for reading event (GCA_EVENT)
**	IIcgc_more_data		- Test if more result data (ie, GCA_TUPLES).
**	IIcgc_get_value		- Get a value from message buffer.
**      IIcgc_find_cvar_len     - Find length of compressed varchar.
**	IIcc1_read_bytes	- Low level byte copier to user buffers.
**	IIcc2_read_error	- Process an error message (GCA_ERROR message).
**	IIcc3_read_buffer	- Read message (GCA_RECEIVE, GCA_INTERPRET).
-*
**  History:
**	28-jul-1987	- Written (ncg)
**	26-feb-1988	- Modified error message handling to lookahead (ncg)
**	09-may-1988	- Added support for DB procedures (ncg)
**	09-dec-1988	- Removed all GCANEW ifdef's (ncg)
**	16-may-1989	- Replaced MEcopy with a MECOPY macro call (bjb)
**	21-nov-1989	- Updated for Phoenix/Alerters (bjb)
**	30-apr-1990	- Put in checks for "alternate read" for cursor
**			  performance project. (neil)
**	14-jun-1990	(barbara)
**	    Don't automatically quit on association failures. (bug 21832)
**	03-nov-1992	(larrym)
**	    Added SQLSTATE support to IIcc2_read_error.
**      05-apr-1996     (loera01)
**          Modified IIcgc_get_value() and IIcc1_read_bytes() to support
**          compressed variable-length datatypes.  Added IIcgc_find_cvar_len() 
**          to support compressed variable-length datatypes.
**	30-Jul-96 (gordy)
**	    Allow response type messages to be split into multiple segments.
**	16-dec-1996	(loera01)
**	    For IIcc1_read_bytes(), added a case for variable-length datatype 
**    	    decompression, in which a block boundary is encountered after the 
**	    first byte in the embedded length.
**      04-Aug-97 (fanra01)
**          When reading a long varchar which spans two buffers it is possible
**          for the bytes_ptr to become incremented changing it from a null.
**          This causes the next iteration of the loop to try and copy from
**          an invalid pointer.  Check pointer before increment.
**	25-aug-1997 (walro03)
**	    Data lost in vchar or varchar columns in tables created in the SQL
**	    terminal monitor.  Change compressed_type from bool to i2.
**	28-Oct-1997 (jenjo02)
**	    Compressed code was not handling the case where the compressed
**	    data was the first thing in the buffer, instead treating it
**	    as an uncompressed type and pretty much jumbling things up
**	    after that. (IIcc1_read_bytes())
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	12-May-98 (gordy)
**	    Timeout value for IIGCa_cb_call() is in milliseconds.
**	26-oct-1999 (thaju02)
**	    Modified IIcc3_read_buffer(). (b76625)
**	19-nov-1999 (thaju02)
**	    Changed E_GC0057_NOTMOUT_DLOCK to E_GC0058_NOTMOUT_DLOCK.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      26-jan-2001 (loera01) Bug 103786
**          For IIcc1_read_bytes(), saved bytes_ptr setting in case a new
**          GCA buffer fetch is required.  Otherwise, positioning of the
**          null indicator may fail during varchar decompression because
**          bytes_ptr gets incremented after the GCA read, and the null	
**          indicator positioning depends on an absolute reference. 
**     11-may-2001 (loera01)
**          Added support for Unicode varchars.  
**/

/*{
**  Name: IIcgc_read_results - Generic result processor.
**
**  Description:
**	After any successful request results will be returning from the server.
**	These results will be setup for reading specific data (ie, results of
**	a FETCH), status (regular query), and/or errors.  This routine just
**	makes sure the data has been setup for the more specific interfaces.
**	It can be called when flushing data (to a certain message type) or
**	when trying to set up for a detailed data read.
**
**	On a simple query with an optional error, followed by a response
**	structure, this routine is sufficient.  It should be called in a 
**	loop so that everything is flushed until done:
**
**		while (IIcgc_read_results(cgc, flushing, end_msg_type))
**			;
**
**	On a structured query that is broken into pieces (such as prereading a
**	descriptor, followed by data), this routine will be called initially
**	with to read, and then with to flush for cleanup. Read setup is:
**
**		if (IIcgc_read_results(cgc, reading, tup_desc_type))
**			IIcgc_get_value(cgc, tup_desc_type, &dbv);
**		else
**			return;
**
**	Cleanup (flushing) is like the end of the simple query (above).
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	flushing_on	- Mode of the caller:
**				 TRUE  - Flushing all data,
**				 FALSE - Setting up for specific read.
**	message_type	- In the case of flushing, this type indicates
**			  what buffer type we want to read to.  When not
**			  flushing this type is the expected type to read from.
**
**  Outputs:
**	Returns:
**	   i4
**		1  - Call me again; more data to come.
**		0  - Completed (and read final status).
**
**	Errors:
**	    E_LC0020_READ_SETUP_FAIL - Failed on initial buffer read.
**
**  Side Effects:
**	
**  History:
**	26-aug-1986	- Written (ncg)
**	21-jul-1987	- Rewritten for GCA (ncg)
**	17-feb-1990	- Added check for GCA_ERROR in flushing state to
**			  handle cases where the DBMS has not detected
**			  an error by the time the FE pools the expedited
**			  channel.  This causes the next query to hang
**			  since the poll finds an error condition but no
**			  subsequent error blocks.  bug 7927 (sandyh)
**	26-feb-1990	- Add check for COPY_MASK to Sandy's fix so that
**			  we check the expedited channel ONLY if we're
**			  reading results from the COPY statement.  Turn
**			  off COPY_MASK when flushing done.
**	16-apr-1990	- Add channel argument on call to IIcc3_read_buffer.
**			  (barbara)
**	03-may-1990	- Allow GCA_EVENT as one of the "ignore-on-read"
**			  types. (neil)
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if there
**	    is no connection.  Added checkthat cgc is non-null.
**	11-feb-1993 (larrym)
**	    Fixed bug.  SQLSTATE is not null terminated; replaced STlcopy with
**	    MEcopy.
**      18-jan-99 (loera01) Bug 94914
**          For IIcc3_read_buffer(), fill gca_protocol field of the trace 
**	    control block if printgca tracing is turned on, and the caller is 
**  	    a child process.
**	24-apr-00 (inkdo01)
**	    Make unexpected TDESCR's permissable, too, for row producing 
**	    procedures.
*/

i4
IIcgc_read_results(cgc, flushing_on, message_type)
IICGC_DESC	*cgc;
bool		flushing_on;
i4		message_type;
{

    IICGC_MSG		*cgc_msg;	/* Local message buffer */
    char		stat_buf[20];	/* Status buffer */

    if (cgc == (IICGC_DESC *)0)
	return 0;

    /*
    ** Everything including the status has already been read.  This call may
    ** happen based on the "static" way that client queries are called
    ** (the calls are dependent on what the preprocessor generates; in
    ** particular a retrieval loop).  When quitting the "quit" mask is set.
    */
    if (   (cgc->cgc_m_state & IICGC_LAST_MASK)
	|| (cgc->cgc_g_state & IICGC_QUIT_MASK))
    {
	cgc->cgc_g_state &= ~IICGC_COPY_MASK;
	return 0;	/* Done */
    }

    /* Pre-read the next buffer */

    IIcc3_read_buffer(cgc, IICGC_NOTIME, GCA_NORMAL);

    /*
    ** If flushing and the type is "message_type" or we are on a "last type"
    ** (as we always allow to drop out with a response) then return "done". 
    ** This does allow some obscure cases through, such as flushing for
    ** an "s_btran" message but received a "response" message.
    ** If setup for reading and the type of the message is the same as 
    ** "message_type" (or response) then return "done", otherwise return "more".
    */
    cgc_msg = &cgc->cgc_result_msg;
    if (flushing_on)
    {

	/* If in flushing mode and an error is returned while reading
	** COPY results, check the expedited channel in case the query
	** completed before the DBMS could write the message to it.
	** This happens in the case of lock timeout and copies.
	** (sandy/barbara) bug 7927.
	*/
	if ( cgc_msg->cgc_message_type == GCA_ERROR
	    && cgc->cgc_g_state & IICGC_COPY_MASK)
	{
	    IIcc3_read_buffer( cgc, (i4)0, GCA_EXPEDITED );
	}
	if (   (cgc->cgc_m_state & IICGC_LAST_MASK)
	    || (cgc_msg->cgc_message_type == message_type))
	{
	    cgc->cgc_g_state &= ~IICGC_COPY_MASK;
	    return 0;	/* Done */
	}
	else
	{
	    return 1;	/* More */
	}
    }
    else /* Reading */
    {
	/*
	** If the type is not what is expected, then signal an error.  The
	** caller will probably return and "flush".  Note that if the type
	** is an error message then we do not signal an error as the server's
	** error has already been flagged to the caller.
	**
	** If the type is a response type, then we let the caller figure it out.
	** This may occur when an invalid REPEAT SELECT query id is used.  The
	** status in the response message indicates that the query is unknown,
	** and the caller must figure out how to redefine the query.
	**
	** If the type is an EVENT and the caller wants a RETPROC this means
	** an EVENT arrived while an EXECUTE PROCEDURE is going on.  This
	** message type can be folded with the error and response type, too.
	*/
	if (   (cgc_msg->cgc_message_type != message_type)
	    && (cgc_msg->cgc_message_type != GCA_ERROR)
	    && (cgc_msg->cgc_message_type != GCA_EVENT)
	    && (cgc_msg->cgc_message_type != GCA_TDESCR)
	    && (cgc_msg->cgc_message_type != GCA_RESPONSE))
	{
	    char stat_buf2[30], stat_buf3[30];
	    CVla(cgc_msg->cgc_d_length, stat_buf);
	    IIcc1_util_error(cgc, FALSE, E_LC0020_READ_SETUP_FAIL, 3,
			     IIcc2_util_type_name(message_type, stat_buf2),
			     IIcc2_util_type_name(cgc_msg->cgc_message_type,
						  stat_buf3),
			     stat_buf, (char *)0);
	}
	if (cgc->cgc_m_state & IICGC_LAST_MASK)
        {
	    return 0;	/* Done */
        }
	else
        {
	    return 1;	/* More */
        }
    } /* If flushing or reading */
} /* IIcgc_read_results */

/*{
** Name: IIcgc_event_read - Read an event mesage from GCA
**
** Description:
**	This routine is part of the processing of a GET DBEVENT statement.
**	It is called from LIBQ if there are no events on LIBQ's II_EVENT
**	list.  It is a specialized version of IIcgc_read_results.
**
**	Unlike most results or data returning from the server, events are
**	sent asynchronously by the server and not as a result of the
**	client sending a query or request.  This means that they may
**	arrive with other results and data or they may arrive alone.  In
**	the former case, the lower level LIBQGCA routines read all events
**	and call into LIBQ to add each event to a list.  This routine is
**	called to read one of any events that may have arrived alone.
**	(In future, more than one event arriving alone may be read at a
**	time.)
**
**	Reading an event will block depending on the timeout value passed
**	in.
**
**	A check is made to see which message was read.  This routine
**	returns success only if a GCA_EVENT has been read.  An error is
**	raised if anything other than a timeout occurs or GCA_EVENT or
**	GCA_ERROR is read.
**
** Inputs:
**	cgc	- Client general communications descriptor.
**	timeout - Number of seconds to wait on READ.  IICGC_NOTIME
**		  means to wait forever.
**
** Outputs:
**	None.
**
** 	Returns:	OK	Event was read or timed out.
**			FAIL	Event was not read because of error.
**	Errors:
**	    E_LC0020_READ_SETUP_FAIL  - Failed to read correct type.
**
** Side Effects:
**	- If this routine returns OK, an event will have been added to
**	  LIBQ's II_EVENT list.  This happens when IIcc3_read_buffer
**	  reads a GCA_EVENT and calls cgc->cgc_handle()/IILQeaEvAdd() to
**	  save it.
**	- IICGC_EVENTGET_MASK is used set and used by IIcc3_read_buffer
**	  and IILQeaEvAdd.
**
** History:
**	01-nov-89 (barbara)
**	    Written for Phoenix/Alerters
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if there
**	    is no connection.  Added checkthat cgc is non-null.
**	25-jul-1991	(teresal)
**	    Update comments to reflect new DBEVENT syntax.
*/
STATUS
IIcgc_event_read(cgc, timeout)
IICGC_DESC	*cgc;
i4		timeout;
{
    IICGC_MSG		*cgc_msg;	/* Local message buffer */
    char		stat_buf[20];	/* Status buffer */

    if (cgc == (IICGC_DESC *)0)
	return FAIL;
    /*
    ** Setting the following flag informs IIcc3_read_buffer to
    ** read at most one GCA_EVENT message.  It is also used by
    ** IILQeaEvAdd to know NOT to call any DBEVENTHANDLER while already
    ** processing a GET DBEVENT statement.
    */
    cgc->cgc_m_state |= IICGC_EVENTGET_MASK;
    IIcc3_read_buffer(cgc, (i4)timeout, GCA_NORMAL);
    cgc->cgc_m_state &= ~IICGC_EVENTGET_MASK;

    cgc_msg = &cgc->cgc_result_msg;
    if (cgc_msg->cgc_message_type == GCA_ERROR)
    {
	return FAIL;
    }
    if (   cgc_msg->cgc_message_type != 0
	&& cgc_msg->cgc_message_type != GCA_EVENT)
    {
	char stat_buf3[30];
	CVla(cgc_msg->cgc_d_length, stat_buf);
	IIcc1_util_error(cgc, FALSE, E_LC0020_READ_SETUP_FAIL, 3,
			 ERx("GCA_EVENT"),
			 IIcc2_util_type_name(cgc_msg->cgc_message_type,
					      stat_buf3),
			 stat_buf, (char *)0);
	return FAIL;
    }
    return OK;
}

/*{
**  Name: IIcgc_more_data - Is there more data returning from server.
**
**  Description:
**	During data retrieval-type requests some data will be returning.
** 	This function returns the number of bytes that are left to read in
**	this buffer with the correct message type.  The actual number
**	returned cannot be used but does flag whether or not there is any
**	data left and whether an error was read during this routine.  This
**	routine should be called on a data boundary, such as:
**
**		if (IIcgc_read_results(cgc, reading_on, tuple_type)) {
**		    while (IIcgc_more_data(cgc, tuple_type) > 0) {
**			    IIcgc_get_value(cgc, tuple_type, &dbv1);
**			    IIcgc_get_value(cgc, tuple_type, &dbv2); ...
**	  	    }
**		}
**
**	If this routine encounters an error message it looks ahead.
**	If it finds the right message type, then the error message is
**	treated as a warning and the caller is notified.  If it does not
**	find the right message type, then the error message is treated
**	as an error and an "error indicator" is returned.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	message_type	- Message type for which there is more data.
**
**  Outputs:
**	Returns:
**	    i4
**		IICGC_GET_ERR - <0  - Error block came in and not followed
**				      by the correct message type.
**		0	            - No more data to read.
**		other 	      - >0  - Number of bytes left in data buffer.
**			
**	Errors:
**	    None
**
**  Side Effects:
**
**	
**  History:
**	17-sep-1986	- Written (ncg)
**	21-jul-1987	- Rewritten for GCA (ncg)
**	26-feb-1988	- Modified the way errors return. (ncg)
**	21-nov-1989	- New interface to IIcc3_read_buffer for Alerters. (bjb)
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if there
**	    is no connection.  Added checkthat cgc is non-null.
*/

i4
IIcgc_more_data(cgc, message_type)
IICGC_DESC	*cgc;
i4		message_type;
{
    IICGC_MSG		*cgc_msg;	/* Local message buffer */
    bool		err;		/* Was there a GCA_ERROR message */

    if (cgc == (IICGC_DESC *)0)
	return 0;

    err = FALSE;
    cgc_msg = &cgc->cgc_result_msg;

    /* Don't allow caller in here */
    if (   (cgc->cgc_m_state & IICGC_LAST_MASK)
	|| (cgc->cgc_g_state & IICGC_QUIT_MASK))
    {
	return 0;	/* Done */
    }

    /*
    ** While there is no data to read AND we are not at the very end of
    ** the complete reply.  Note that this is a while loop to handle
    ** cases of there being zero length result data buffers and error
    ** message skipping.
    */
    while (   (cgc_msg->cgc_d_used >= cgc_msg->cgc_d_length)
	   && (cgc->cgc_m_state & IICGC_LAST_MASK) == 0)
    {
	/* Need to read a new buffer */
	IIcc3_read_buffer(cgc, IICGC_NOTIME, GCA_NORMAL);

	if (cgc_msg->cgc_message_type == GCA_ERROR)
	{
	    err = TRUE;		/* Note this but continue */
	}
	else if (cgc_msg->cgc_message_type != message_type)
	{
	    if (err)		/* There was an error here */
		return IICGC_GET_ERR;
	    else
		return 0;		/* Dropped out of last data buffer */
	} /* If type is error or tuples */
    } /* While no tuple data message left for reading */

    /* If previous error, unset error for caller */
    if (err)
	_VOID_ (*cgc->cgc_handle)(IICGC_HDBERROR, (IICGC_HARG *)0);

    return cgc_msg->cgc_d_length - cgc_msg->cgc_d_used;
} /* IIcgc_more_data */

/*{
**  Name: IIcgc_get_value - Get specific data results returning from server.
**
**  Description:
**	After successful requests data will be returning from the server.
**	This data is copied into the caller's supplied address space.  This
**	routine may also be used for any "valued" information returning
**	from the server, such as tuple results, query ids, row counts and
**	error numbers.
**
**	This routine provides a tag, 'get_tag', which is a mask passed
**	from LIBQ.  The source mask, defined in iirowdesc.h, indicates
**      key factors about the characteristics of the tuple.  In this
**      routine, the only relevant factor is whether or not the tuple
**      contains compressed varchars.
**
**  Inputs:
**	cgc		- Client general communications descriptor:
**			  If cgc_alternate is set and the alternate state is
**			  CGC_AREADING, the we detour into IIcga5_get_data.
**			  We expect the caller to have set up all data
**			  structures required to support this detour.
**	message_type	- The type of the message from which caller expects
**			  valid results.
**	get_tag		- Indicates characteristics of the tuple.
**			  IICGC_VVCH - the tuple contains compressed 
**                        varchars.
**	get_arg		- DB data value for results:
**	    .db_datatype - The type of the object we want to read.  This may not
**			   correspond to an atomic type.  It may be a structured
**			   type, such as DB_DBV_TYPE - as when getting th DB
**			   data value from a tuple descriptor.
**	    .db_length	 - The number of bytes we want to read.  This length
**			   must be set even if we are read a complex object
**			   such as a query id.
**
**  Outputs:
**	get_arg		- DB data value for results:
**	    .db_data	 - Pointer to a result buffer.  If this pointer is
**			   not null then the result buffer is filled with data.
**	    		   If this pointer is null then the data is skipped in
**			   line.  For example, if some data was to be skipped
**			   at the end of the results of a SELECT loop row,
**			   this could be called for the remaining data of the
**			   current tuple and the caller could recover before
**			   the start of the next row.
**
**	Returns:
**	    	i4 - Return values from IIcc1_read_bytes
**			IICGC_GET_ERR - <  0  - Error message came in.
**			other 	      - >= 0  - Number of bytes read.
**			
**	Errors:
**		None
**
**  Examples:
**	
**	Stmt:	SELECT * INTO :i4var FROM emp;
**	Call:	Send query (as described in cgcwrite);
**		IIcgc_read_results(cgc, FALSE, GCA_TDESCR);
**		IIcgc_get_value(cgc,GCA_TDESCR,GCA_VVAL,dbv_of(tup_desc_head));
**		for (i in number of columns in tupdesc_head)
**		    IIcgc_get_value(cgc, GCA_TDESCR, GCA_VVAL, dbv_of(dbv[i]));
**		for (each tuple that returns)
**		    IIcgc_get_value(cgc, GCA_TUPLES, GCA_VVAL, dbv_of(resvar1));
**		    IIcgc_get_value(cgc, GCA_TUPLES, GCA_VVAL, dbv_of(resvar2));
**		    ....
**
**	Stmt:	OPEN cursor1;
**	Call:	Send query (as described in cgcwrite);
**		IIcgc_read_results(cgc, FALSE, GCA_QC_ID);
**		IIcgc_get_value(cgc, GCA_QC_ID, GCA_VID, dbv_of(cursor_id));
**		IIcgc_get_value(cgc,GCA_TDESCR,GCA_VVAL,dbv_of(tup_desc_head));
**		for (i in number of columns in tupdesc_head)
**		    IIcgc_get_value(cgc, GCA_TDESCR, GCA_VVAL, dbv_of(dbv[i]));
**
**  Side Effects:
**	
**  History:
**	18-aug-1986	- Written (ncg)
**	20-jul-1987	- Rewritten for GCA (ncg)
**	30-apr-1990	- Check "alternate read" for cursor performance. (ncg)
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if there
**	    is no connection.  Added checkthat cgc is non-null.
**      05-apr-1996     (loera01)
**          Utilized previously unused get_tag argument to support compressed 
**          variable-length datatypes.  
*/

i4
IIcgc_get_value(cgc, message_type, get_tag, get_arg)
IICGC_DESC	*cgc;
i4		message_type;
i4		get_tag;
DB_DATA_VALUE	*get_arg;
{

    i2 compressed_type = IICGC_NON_VCH;

    if (cgc == (IICGC_DESC *)0)
	return 0;
    if (message_type == GCA_TUPLES)
    {
        if (get_tag == IICGC_VVCH)
        {
           switch (get_arg->db_datatype)
           {
	       case -DB_NVCHR_TYPE:
	       {
		  compressed_type = IICGC_NVCHR;
		  break;
               }
               case -DB_VCH_TYPE:
               case -DB_TXT_TYPE:
               case -DB_VBYTE_TYPE:
               case -DB_LTXT_TYPE: 
               {
                   compressed_type = IICGC_VCH;
                   break;
               }
	       case DB_NVCHR_TYPE:
	       {
		   compressed_type = IICGC_NVCHRN;
		   break;
	       }
               case DB_VCH_TYPE:
               case DB_TXT_TYPE:
               case DB_VBYTE_TYPE:
               case DB_LTXT_TYPE: 
               {
                   compressed_type = IICGC_VCHN;
                   break;
               }
               default:
               {
                   compressed_type = IICGC_NON_VCH;
                   break;
               }
           }
        }
        else
        {
           compressed_type = IICGC_NON_VCH;
        }
    }
    if (   cgc->cgc_alternate == NULL
	|| (cgc->cgc_alternate->cgc_aflags & IICGC_AREADING) == 0)
    {
 	return IIcc1_read_bytes(cgc, message_type, compressed_type,
			       (i4)get_arg->db_length,
			       (char *)get_arg->db_data);
    }
    else
    {
	/* Detour through alternate read stream */
	return IIcga5_get_data(cgc, message_type,
			       (i4)get_arg->db_length,
			       (char *)get_arg->db_data,
		               compressed_type);
    }
} /* IIcgc_get_value */

/*{
**  Name: IIcc1_read_bytes - Get a specific number of bytes from server.
**
**  Description:
**	This routine is internal to the iicgc routines.
**
**	This is the lowest level byte transfer routine in this layer.  The
** 	routine does not know about context of the callers - it is just a buffer
**	copier.  The number of bytes indicated are copied into a user supplied
**	buffer - if the buffer is null then the same number of bytes are just
**	skipped.
**
**	If an error message appears then the routine looks ahead.
**	If it finds the right message type, then the error message is treated
**	like a warning and the caller is notified.  If it does not find
**	the right message type, then the error is treated as an error and
**	an "error indicator" is returned.  (If an internal caller is
**	currently processing an error (error mask) then the "error indicator"
**	is not returned.)
**
**	If a message continues over a message buffer boundary this routine
**	will silently read the next buffer and continue copying the data.
**	Before reading the next buffer, it checks to see if we have already
**	read the end_of_data flag.  There are a few types of messages that
**	should not be continued to be read after end_of_data (EOD).  These
**	are:
**		GCA_TDESCR, GCA_C_INTO, GCA_C_FROM, GCA1_C_INTO, GCA1_C_FROM.
**
**	These messages are self describing (may vary in what they contain and
**	how much), and if trashed may mislead the caller that there is a lot
**	more data in the message.  Note that the check for EOD in these cases
**	will fail if we have the situation where two messages of the same type
**	follow one another in the protocol (ie, a cursor descriptor followed
**	by a paramater descriptor - both may be GCA_TDESCR).
**
**	Many other messages may still continue AFTER end_of_data.  For example,
**	tuple data, GCA_TUPLES, comes as many messages (and many message
**	buffers), each message with one or more tuples, and each message ending
**	with an end_of_data indicator.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	message_type	- The message type from which caller wants the
**			  data.  For example, a query id should come from
**			  a GCA_QC_ID type, while the result value of tuple
**			  should come from a GCA_TUPLES type.
**	compressed_type - Indicates information about compressed varchars in
**                        the tuple.
**                        -2 Compressed nullable Unicode varchar
**                        -1 Compressed nullable
**                        0  No compressed varchars
**                        1  Compressed non-nullable
**                        2 Compressed non-nullable Unicode varchar
**	bytes_len	- Number of bytes to copy (i4).
**
**  Outputs:
**	bytes_ptr	- Ptr to byte sequence (char * - must be 1 byte sizes).
**			  If this pointer is not null then the area pointed at
**			  is filled.  If this pointer is null then the 
**			  specified number of bytes is skipped in line.
**
**	Returns:
**	    	i4
**			IICGC_GET_ERR - <  0 - Error block came in and not
**					       followed by correct msg type.
**			other 	      - >= 0 - Number of bytes read.  If not
**					       the same as bytes_len then caller
**					       should report an error.
**			
**	Errors:
**		E_LC0021_READ_SHORT_DATA - Too few bytes of this message.
**		E_LC0022_READ_WRONG_MESSAGE - Read into wrong message type.
**		E_LC0023_READ_AFTER_EOD - Read message after end_of_data.
**
**  Side Effects:
**	
**  History:
**	22-oct-1986	- Written (ncg)
**	25-mar-1987	- Modified to allow data skipping (ncg)
**	20-jul-1987	- Rewritten for GCA (ncg)
**	26-feb-1988	- Modified the way errors return. (ncg)
**	21-nov-1989	- New interface to IIcc3_read_buffer for alerters. (bjb)
**	06-aug-1991	- New copy map for bug fix 37564. (teresal)
**	16-dec-1996	- Added a case for variable-length datatype
**			  decompression, in which a block boundary is 
**			  encountered after the first byte in the embedded 
**			  length. (loera01)
**      04-Aug-97 (fanra01)
**          When reading a long varchar which spans two buffers it is possible
**          for the bytes_ptr to become incremented changing it from a null.
**          This causes the next iteration of the loop to try and copy from
**          an invalid pointer.  Check pointer before increment.
**	28-Oct-1997 (jenjo02)
**	    Compressed code was not handling the case where the compressed
**	    data was the first thing in the buffer, instead treating it
**	    as an uncompressed type and pretty much jumbling things up
**	    after that.
**      26-jan-2001 (loera01) Bug 103786
**          Saved bytes_ptr setting in case a new GCA buffer fetch is 
**          required.  Otherwise, positioning of the null indicator may fail 
**          during varchar decompression because bytes_ptr gets incremented 
**          after the GCA read, and the null indicator positioning depends 
**          on an absolute reference. 
*/

i4
IIcc1_read_bytes(cgc, message_type, compressed_type, bytes_len, bytes_ptr)
IICGC_DESC	*cgc;
i4		message_type;
i2		compressed_type;
i4		bytes_len;
char		*bytes_ptr;
{
    i4			count;		/* Count value to return */
    i4			user_len;	/* User length requested */
    u_i2		cplen;		/* Copy length */
    IICGC_MSG		*cgc_msg;	/* Local message buffer */
    bool		err;		/* Was there a GCA_ERROR message */
    char		stat_buf1[20];	/* Status buffer */
    char		stat_buf2[20];
    char		stat_buf3[30];
    u_i2                vchar_len = 0;  /* Vchar length */
    bool 		frag_embed_length=FALSE; /* Was embedded length */
						 /* fragmented? */
    char *bytes_ptr_save=bytes_ptr;     /* In case IIcc3_read_buffer gets  */
					/* called and we have a null ind */
    /* 
    ** Keep copying data into callers result buffer, until there is no
    ** more data or an error.  Allow crossing of block boundaries.
    */

    err = FALSE;
    count = 0;
    user_len = bytes_len;
    cgc_msg = &cgc->cgc_result_msg;
    while (user_len > 0)
    {
        
	/*  If there is no more data then read another buffer */
	while (cgc_msg->cgc_d_used >= cgc_msg->cgc_d_length)
	{
	    /* This was the last buffer so we'd better return */
	    if (cgc->cgc_m_state & IICGC_LAST_MASK)
	    {
		/* If earlier error return error status */
		if (err)
                {
		    return IICGC_GET_ERR;
                }
		CVna(bytes_len, stat_buf1);
		CVna(count, stat_buf2);
		IIcc1_util_error(cgc, FALSE, E_LC0021_READ_SHORT_DATA, 3,
				 IIcc2_util_type_name(message_type, stat_buf3),
				 stat_buf1, stat_buf2, (char *)0);
		return count;	/* Do not try to copy more */
	    }

	    /* 
	    ** Before we read a new buffer check the state of the message.
	    ** Some messages are not continued if the end_of_data has already
	    ** been sent.
	    */
	    if (cgc->cgc_m_state & IICGC_EOD_MASK)
	    {
		/* End of data detected - check out message types */
		if (   cgc_msg->cgc_message_type == message_type
		    && (   cgc_msg->cgc_message_type == GCA_TDESCR
		        || cgc_msg->cgc_message_type == GCA_C_INTO
		        || cgc_msg->cgc_message_type == GCA_C_FROM
		        || cgc_msg->cgc_message_type == GCA1_C_INTO
		        || cgc_msg->cgc_message_type == GCA1_C_FROM
		       )
		   )
		{
		    /* If earlier error return error status */
		    if (err)
			return IICGC_GET_ERR;
		    IIcc1_util_error(cgc, FALSE, E_LC0023_READ_AFTER_EOD, 1,
				IIcc2_util_type_name(cgc_msg->cgc_message_type,
						     stat_buf3),
				(char *)0, (char *)0, (char *)0);
		    return count;
		}
		else	/* Turn off EOD as is unused for other message types */
		{
		    cgc->cgc_m_state &= ~IICGC_EOD_MASK;
		} /* if type is not continued */
	    } /* if end_of_data was read */

	    /* Need to read a new buffer */
	    IIcc3_read_buffer(cgc, IICGC_NOTIME, GCA_NORMAL);

	    /*
	    ** If found an error and NOT in the middle of processing errors
	    ** then this is NOT the data we want to copy, so flag error.
	    */
	    if (   (cgc_msg->cgc_message_type == GCA_ERROR)
		&& (cgc->cgc_m_state & IICGC_ERR_MASK) == 0)
	    {
		err = TRUE;
	    }
	} /* While no more data left in message */

	if (cgc_msg->cgc_message_type != message_type)
	{
	    char stat_buf4[30];
	    /* If earlier error return error status */
	    if (err)
		return IICGC_GET_ERR;
	    CVna(count, stat_buf1);
	    IIcc1_util_error(cgc, FALSE, E_LC0022_READ_WRONG_MESSAGE, 3,
			     IIcc2_util_type_name(cgc_msg->cgc_message_type,
						  stat_buf3),
			     IIcc2_util_type_name(message_type, stat_buf4),
			     stat_buf1, (char *)0);
	    return count;		/* Do not try to copy wrong data */
	}

	/*
	** The length we want to copy from the result block into the
	** caller's db value is the minimum of the requested length 
	** (user_len), and the number of bytes left in the result block.
	*/

	/* If compressed, have we extracted the length yet? */
	if (compressed_type && vchar_len == 0)
	{
	    if (frag_embed_length && bytes_ptr)
	    {
		/* Second byte of vchar len from previous buffer */
		MEcopy((PTR)(cgc_msg->cgc_data + cgc_msg->cgc_d_used),
			    sizeof(char), (PTR)bytes_ptr);
		bytes_ptr--;
		user_len = (IIcgc_find_cvar_len(bytes_ptr, compressed_type) - 
		    		sizeof(char));
		vchar_len = user_len; 
		bytes_ptr++;
	    }
	    else
	    {
		if (min(user_len, cgc_msg->cgc_d_length - cgc_msg->cgc_d_used) 
				== sizeof(char))
		{
		    /* Second byte of vchar length is in next buffer */
		    frag_embed_length = TRUE;
		}
		else
		{
		    user_len = IIcgc_find_cvar_len((cgc_msg->cgc_data +
			cgc_msg->cgc_d_used), compressed_type);
		    vchar_len = user_len;
		}
	    }
        }

        cplen = min(user_len, cgc_msg->cgc_d_length - cgc_msg->cgc_d_used);
	if (bytes_ptr)				/* Don't copy if null */
	{
	    MECOPY_VAR_MACRO((PTR)(cgc_msg->cgc_data + cgc_msg->cgc_d_used),
		   cplen, (PTR)bytes_ptr);
        }

	/* 
	**  If this is a compressed, nullable varchar, copy the null
	**  flag to the end of compressed string, to the end of the column,
	**  but wait til we have all of the varchar.  Use the original 
        **  bytes_ptr, in case bytes_ptr got incremented after a call to
        **  IIcc3_read_buffer() to complete a fragmented varchar buffer.
	**  This, in effect, decompresses the varchar.
	*/
	if (compressed_type < 0 && bytes_ptr && user_len == cplen)
	    {
	       *(PTR)(bytes_ptr_save + bytes_len - sizeof(char)) = 
                   *(PTR)(bytes_ptr + user_len - sizeof(char));
	    }

        if (bytes_ptr)
            bytes_ptr += cplen;
	user_len             -= cplen;
	cgc_msg->cgc_d_used  += cplen;    
	count                += cplen;
    } /* While more bytes to copy */

    /* If previous error, unset error for caller */
    if (err)
	_VOID_ (*cgc->cgc_handle)(IICGC_HDBERROR, (IICGC_HARG *)0);
    if (compressed_type)
    {
       return bytes_len;
    }
    else
    {
       return count;	/* And count should be correct */
    }
} /* IIcc1_read_bytes */

/*{
**  Name: IIcc2_read_error - Pick off an error returning from the server.
**
**  Description:
**	This routine is internal to the IIcgc routines.
**
**	The block type is GCA_ERROR or GCA_REJECT, and as long 
**	as there are errors keep on processing them.  There can be more than 
**	one error structure within a single error message.
**
**  Block Description:
**
**	Note for INGRES Version 6.0:
**
**	Note that in version 6.0 the server sends back an error id, and a
**	single parameter that has the whole error text inside (the errors
**	are not currently parameterized).  The rest of the error is skipped.
**
**	In GCA_PROTOCOL_LEVELs >= 60, an SQLSTATE is returned instead of
**	a Generic Error.   SQLSTATEs are char[5]'s, but because of GCC issues
**	it is encoded into an i4, and passed in the same "element" as generic
**	error used to be, gca_id_error.  When they get here, they are decoded
**	with a common!cuf routine back into a char[5].
**	
**  Inputs:
**	cgc		- Client general communications descriptor.
**			  If cgc_alternate is set and the alternate state is
**			  CGC_ALOADING, the we detour into IIcga4_load_error.
**			  We expect that all alternate data structures have
**			  already been set up.
**
**  Outputs:
**	Returns:
**		None
**	Errors:
**		None
**
**  Side Effects:
**	1. Because an error message may span more than one buffer, the call
**	   to read data may implicitly read a new block.
**	2. Error lengths and values may be updated in this routine if an error
**	   occurred while reading the error length or value.
**	
**  History:
**	26-aug-1986	- Written (ncg)
**	21-jul-1987	- Rewritten for GCA (ncg)
**	10-may-1988	- Modified to read user messages from DB
**			  procedures, which are returned as errors (ncg)
**	22-jun-1988	- Modified to read gateway local errors too (ncg)
**	12-jul-1988	- Consistency check on # of errors and parameters (ncg)
**	09-dec-1988	- Field h_use is assigned GCA severity (ncg)
**	30-apr-1990	- Check "alternate read" for cursor performance. (ncg)
**	01-nov-1992 (larrym)
**	    added SQLSTATE support.  See note above.
**	18-nov-1992 (larrym)
**	    Fixed setting of generic error when receiving an invalid sqlstate.
**	    We were just setting it to 0, which caused other routines to 
**	    believe there was no error.
**	18-jan-1993 (larrym)
**	    fixed bug introduced with above fix:  when calling the sqlstate-
**	    to-generic_error routine, a return value of 0 means it couldn't
**	    find a mapping and we just return GE_OTHER_ERROR.  But a user
**	    message could legitimately return error number 0.  So if the
**	    error is a message, we just leave the error number alone.  
*/

VOID
IIcc2_read_error(cgc)
IICGC_DESC	*cgc;
{
    i4			count;			/* Byte-read counter */
    IICGC_MSG		*cgc_msg;		/* Local message buffer */
    IICGC_HARG		handle_arg;		/* To call caller's handler */
    i4		num_of_errors;		/* Number of error messages */
    i4		num_of_parms;		/* Number of parameters */
    GCA_E_ELEMENT	err_element;		/* Error element for 1st parm */
    i4			err_head_size;		/* Error elem size w/o data */
    GCA_DATA_VALUE	*err_gcv;		/* Data area */
    char		err_string[ER_MAX_LEN];	/* Buffer for string */
    DB_SQLSTATE		db_sqlst;
/*
** Maximum number of parameters and errors.  If this number is exceeded then
** it is likely that the error element is trashed.
*/
#   define		MAXERPARM	10


    cgc_msg = &cgc->cgc_result_msg;
    count  = IIcc1_read_bytes(cgc, cgc_msg->cgc_message_type, FALSE,
				(i4)sizeof(num_of_errors),
				(char *)&num_of_errors);
    if (count != sizeof(num_of_errors))
	return;

    /*
    ** For each error process its error element.  The element starts with
    ** a header (consisting of error #, ids, severity and the number of
    ** parameters - GCA data values).  Following the header is a sequence of
    ** GCA data values.  If we have a very large number of errors then
    ** assume the error is trashed and read just one error.
    */
    if (num_of_errors > MAXERPARM)
	num_of_errors = 1;
    while (num_of_errors--)
    {
	cgc->cgc_m_state |= IICGC_ERR_MASK;

	/* Get the error header information */
        err_head_size = sizeof(err_element.gca_id_error) +
                        sizeof(err_element.gca_id_server) +
                        sizeof(err_element.gca_server_type) +
                        sizeof(err_element.gca_severity) +
                        sizeof(err_element.gca_local_error) +
                        sizeof(err_element.gca_l_error_parm);
        count = IIcc1_read_bytes(cgc, cgc_msg->cgc_message_type, FALSE,
                        err_head_size, (char *)&err_element);

	if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_60)
	{
	    i4		derived_generic_error;	/* derived from sqlstate */

	    /* 
	    ** Protocol level > 60.  No generic error, SQLSTATE instead.
	    ** The 5 character sqlstate is encoded to fit into a i4.
	    ** It is passed in the gca_id_error, as this is now not used
	    ** (no more generic errors over the wire).  Here we decode 
	    ** the value into a proper sqlstate, and then generate a
	    ** Generic error from it, placing the generic error back
	    ** into gca_id_error.
	    */
	    ss_decode(db_sqlst.db_sqlstate, err_element.gca_id_error);

	    /*
	    ** if ss_decode returns 0, and it's not from a message, then 
	    ** we got an unknown SQLSTATE.  In that case we set it to
	    ** GE_OTHER_ERROR.  BTW, ss_decode takes care of copying a
	    ** message number from the local_error to the gen_error field
	    ** so we don't have to.
	    */
	    if ( ((derived_generic_error = ss_2_ge(
					db_sqlst.db_sqlstate, 
					err_element.gca_local_error
					)) == 0) 
		&& (STbcompare(db_sqlst.db_sqlstate, DB_SQLSTATE_STRING_LEN,
			       SS5000G_DBPROC_MESSAGE, DB_SQLSTATE_STRING_LEN,
			       FALSE)))
	    {
		/* bogus SQLSTATE */
		err_element.gca_id_error = GE_OTHER_ERROR;
		MEcopy(SS5000K_SQLSTATE_UNAVAILABLE, DB_SQLSTATE_STRING_LEN,
			db_sqlst.db_sqlstate);
	    }
	    else
	    {
		err_element.gca_id_error = derived_generic_error;
	    }
        }
	else
	{
	    MEcopy (SS5000K_SQLSTATE_UNAVAILABLE, DB_SQLSTATE_STRING_LEN,
		     db_sqlst.db_sqlstate);
	}

	if (count != err_head_size)
	{
	    cgc->cgc_m_state &= ~IICGC_ERR_MASK;
	    return;
	}


	/* Check error message usage */
	handle_arg.h_use = err_element.gca_severity;

	/* Assign local error number */
	handle_arg.h_local = err_element.gca_local_error;

	/* Set the SQLSTATE */
	handle_arg.h_sqlstate = &db_sqlst;

	/*
	** For each parameter of this error message copy the data into the
	** same buffer.  There better be just ONE parameter in INGRES errors.
	** If we have a very large number of parameters then assume the
	** error is trashed and read just one parameter.
	*/
	num_of_parms = err_element.gca_l_error_parm;
	if (num_of_parms > MAXERPARM)
	    num_of_parms = 1;
	while (num_of_parms--)
	{
	    /* Get the error parm information */
	    err_gcv = &err_element.gca_error_parm[0];
	    err_head_size = sizeof(err_gcv->gca_type) +
			    sizeof(err_gcv->gca_precision) +
			    sizeof(err_gcv->gca_l_value);
	    count = IIcc1_read_bytes(cgc, cgc_msg->cgc_message_type, FALSE,
				     err_head_size, (char *)err_gcv);
	    if (count != err_head_size)
	    {
		cgc->cgc_m_state &= ~IICGC_ERR_MASK;
		return;
	    }
	    /*
	    ** Read the string data - all read into the same buffer
	    ** as INGRES error return just a string error message.
	    ** Fred says GCA will have to change to accept this!
	    */
	    err_gcv->gca_l_value =
		min(err_gcv->gca_l_value, sizeof(err_string));
	    count = IIcc1_read_bytes(cgc, cgc_msg->cgc_message_type, FALSE,
				    (i4)err_gcv->gca_l_value,
				    (char *)&err_string[0]);
	    if (count != err_gcv->gca_l_value)
	    {
		/* Recover by just truncating the error string */
		err_gcv->gca_l_value = count;
	    } /* If count less than the size read */
	} /* While element has more parameters - should be just one */

	/* Call the caller error handler to process the actual message */
	handle_arg.h_errorno = err_element.gca_id_error;
	if (err_element.gca_l_error_parm)
	{
	    handle_arg.h_numargs = 1;
	    handle_arg.h_length  = (i2)err_gcv->gca_l_value;
	    handle_arg.h_data[0] = err_string;
	}
	else	/* This is an error in error protocol */
	{
	    handle_arg.h_numargs = 0;
	    handle_arg.h_length  = 0;
	    handle_arg.h_data[0] = ERx("");
	}
	if (   cgc->cgc_alternate == NULL
	    || (cgc->cgc_alternate->cgc_aflags & IICGC_ALOADING) == 0)
	{
	    _VOID_ (*cgc->cgc_handle)(IICGC_HDBERROR, &handle_arg);
	}
	else
	{
	    /* Try to detour through alternate stream error loader */
	    if (IIcga4_load_error(cgc, &handle_arg) != OK)
		_VOID_ (*cgc->cgc_handle)(IICGC_HDBERROR, &handle_arg);
	}

	cgc->cgc_m_state &= ~IICGC_ERR_MASK;
    } /* While message holds more than one error message */
} /* IIcc2_read_error */

/*{
**  Name: IIcc3_read_buffer - Read a buffer into the client processing area.
**
**  Description:
**	This routine is internal to the IIcgc routines.
**
**	Read a message buffer of returning data into the client processing 
**	area.
**
** 	Buffers that are currently processed here (in terms of their data or
**	global side effects) are:
**
**	1. GCA_ACCEPT, GCA_IACK -
**	   Turns on the "last message" mask.
**	2. GCA_REJECT - 
**	   The content of this is exactly the same as a GCA_ERROR, except
**	   that this message also turns on the "last message" mask.
**	3. GCA_RELEASE - 
**	   The content of this message MAY be an error message, which will
**	   be processed.  Disconnect the session (internally) and call the 
**	   caller supplied handler to clean up.
**	4. GCA_S_BTRAN -
**	   Contains a transaction id, that is currently ignored. Turns on
**	   "last message" mask.
**	5. GCA_RESPONSE, GCA_DONE, GCA_REFUSE, GCA_S_ETRAN -
**	   Contains a response structure which we copy into the local one
**	   and turn on the "last message" mask.
**	6. GCA_RETPROC -
**	   Contains a DB procedure return structure which we copy into the
**	   local one and turn on the "last message" mask.
**	7. GCA_C_INTO, GCA_C_FROM, GCA1_C_INTO, GCA1_C_FROM -
**	   These messages are sent when processing a batch COPY request.  Only
**	   the first buffer is processed here when the "in copy" mask is not on.
**	   It is up to the caller to turn the mask on and off.
**	   GCA1 is the newer version of the copy map.  As of protocol level 50,
**	   the copy map was changed to fix COPY bug 37564.
**	8. GCA_ERROR -
**	   An error message is processed from within here.  When the routine
**	   leaves it is left pointing at the GCA_ERROR buffer (used as an
**	   error indication for other routines).  If the protocol lever is 
**	   60 or greater, then an encoded SQLSTATE is passed in the gca_id_error
**	   field.
**	9. GCA_TRACE -
**	   These buffers are sent with characters data, cgc_d_length marking
**	   how many bytes are used.  Each trace buffer is passed to the caller
**	   supplied trace handler, and on the last (non-trace) buffer the trace
**	   data is flushed.
**     10. GCA_EVENT -	
**	   LIBQ is called to copy event data into a list of events.  Several
**	   GCA_EVENTS may arrive along with query results.
**     11. All other message types are just verified to be legal result message
**	   types.
**
**	Some of the above handlers (error handler, copy handler, etc) are not
**	called if we are in the middle of an interrupt call (the "Write" message
** 	type is GCA_ATTENTION).
**
**	If the "end of data" is detected then the "EOD" mask is set.  This
**	is used for some messages, that just do not continue over message
**	buffer boundaries (after the end_of_data).  For example, tuple data
**	may continue, but a tuple descriptor will not.
**
**	After correctly translating a message the internal GCA trace handler
**	may be called to generate message tracing.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	time_out	- Time out value for GCA_RECEIVE service:
**			  0 - Try to read the next message buffer.  If there
**			      is nothing there and the status returned is
**			      "timed out", then we set the result message type
**			      to zero and return to the caller.  If there is
**			      something there then the caller will handle the
**			      data.  The caller can check the result message
**			      type to see if anything was read.
**			  > 0 - As above but rather than just poll we wait for
**			      the indicated number of seconds before timing
**			      out.  This and the above value expect the 
**			      E_GC0020_TIME_OUT error status.
**			  IICGC_NOTIME - Sit here and read till we are done.
**			      This is the default way for callers to read in
**			      message buffers.  In this case the result
**			      message type will be set to the new type.
**	channel		- GCA_NORMAL - Read from the normal channel.
**			- GCA_EXPEDITED - Read from the expedited channel.
**			      Currently we only read from expedited when
**			      executing a COPY FROM wherein every few rows we
**			      check for interrupts.
**
**  Outputs:
**	Returns:
**		None
**	Errors:
**		E_LC0001_SERVICE_FAIL - GCA RECEIVE/INTERPRET service failed.
**		E_LC0024_READ_BAD_TYPE - Ignoring bad message type.
**		E_LC0025_READ_ASSOC_FAIL - Database partner association failed.
**
**  Side Effects:
**	1. This is the only place where "cgc_result_block" is actually filled
**	   with data and its counters set.
**    	2. If the communication channels closed down then this implicitly
**	   quits.
**	3. If an interpret fails then we turn on the "last" mask to flag our
**	   caller to stop reading.
**	4. The trace generator may indirectly call outside to its emitting
**	   function.
**	
**  History:
**	26-aug-1986	- Written (ncg)
**	21-jul-1987	- Rewritten for GCA (ncg)
**	29-sep-1987	- Modified to allow 0-time reading (ncg)
**	09-may-1988	- Handles "DB procedure return" message (ncg)
**	14-jul-1988	- Updated to copy as much as needed of status (ncg)
**	07-sep-1988	- Validated error messages on GCA_REJECT/RELEASE (ncg)
**	07-sep-1988	- Use expedited channel if time_out is 0 (sandyh)
**	19-sep-1988	- Added calls to trace generator (ncg)
**	28-nov-1988	- Allows purged receives and ignores them (ncg)
**	06-dec-1988	- Rolled back purged received fix due to gc fix (dave)
**	07-dec-1988	- Use the "interrupt" parm if waiting for an IACK (ncg)
**	21-nov-1989	- Updated for Phoenix/Alerters. (bjb)
**	30-mar-1990	- Time out check for alerters (ncg)
**	14-jun-1990	(barbara)
**	    Don't automatically quit on association failures.  Removed call
**	    to IIcgc_disconnect. (bug 21832)
**	31-jan-1991	(barbara)
**	    Allow purged messages on RECEIVE and ignore them.
**	06-feb-1991	(barbara)
**	    After receiving GCA_RELEASE, set FASSOC mask (failed association)
**	    BEFORE calling error handler.  This flag will prevent a disconnect
**	    statement in a user handler from sending a GCA_RELEASE to the dbms.
**	06-aug-1991 	(teresal)
**	    Add new GCA message types GCA1_C_FROM and GCA2_C_INTO for the
**	    new copy map.
**	01-nov-1992	(larrym)
**	    Added SQLSTATE support.
**	14-dec-1992	(larrym)
**	    Modified protocol for GCA_RETPROC to support BYREF parameters. 
**	    RETPROC is not the last message if the PROTOCOL >= 60, so 
**	    if that's the case, we don't set IICGC_LAST_MASK in cgc_m_state.
**	4-nov-93 (robf)
**          Add GCA_PROMPT message support
**	30-Jul-96 (gordy)
**	    Allow response type messages to be split into multiple segments.
**	18-jan-99 (loera01) Bug 94914
**	    Fill gca_protocol field of the trace control block if printgca
**	    tracing is turned on, and the caller is a child process.
**	12-May-98 (gordy)
**	    Timeout value for IIGCa_cb_call() is in milliseconds.
**	26-oct-1999 (thaju02)
**	    If read blocking normal channel and data is detected on 
**	    expedited channel, force blocking read to complete via timeout
**	    so we can process expedited message. (b76625)
**	19-nov-1999 (thaju02)
**	    Changed E_GC0057_NOTMOUT_DLOCK to E_GC0058_NOTMOUT_DLOCK.
*/

VOID
IIcc3_read_buffer(cgc, time_out, channel)
IICGC_DESC	*cgc;
i4		time_out;
i4		channel;
{
    GCA_PARMLIST	*gca_parm;	/* General GCA parameter */
    GCA_RV_PARMS	*gca_rcv;	/* GCA_RECEIVE parameter */
    GCA_IT_PARMS	*gca_int;	/* GCA_INTERPRET parameter */
    GCA_TR_BUF		gca_trb;	/* GCA message trace parameter */
    STATUS		gca_stat;	/* Status for IIGCa_call - unused */
    i4			end_of_data;	/* End of data for this message */
    IICGC_MSG		*cgc_msg;	/* Local message buffer */
    bool		was_trace;	/* Was there any trace data */
    IICGC_HARG		handle_arg;	/* Argument to handler */
    bool		canceling;	/* Are we in the middle of canceling */
    char		stat_buf[IICGC_ELEN + 1];	/* Status buffer */
    u_i2		len, cur_len = 0;	/* Message lengths */

    cgc_msg = &cgc->cgc_result_msg;
    was_trace = FALSE;
    canceling = cgc->cgc_write_msg.cgc_message_type == GCA_ATTENTION;

    /* Just to be safe */
    handle_arg.h_sqlstate = (DB_SQLSTATE *)0;	/* no sqlstate yet */

    /* If the message is an interrupt then use the interrupt parameter */
    if (canceling)
	gca_parm = &cgc_msg->cgc_intr_parm;
    else
	gca_parm = &cgc_msg->cgc_parm;


    /* Loop through till we receive a non-trace message */
    for (;;)
    {
	/* Set up GCA_RECEIVE information */
	gca_rcv = &gca_parm->gca_rv_parms;
	gca_rcv->gca_association_id = cgc->cgc_assoc_id;	/* Input */
	gca_rcv->gca_flow_type_indicator = channel;		/* Input */
	gca_rcv->gca_buffer    = cgc_msg->cgc_buffer;		/* Input */
	gca_rcv->gca_b_length  = cgc_msg->cgc_b_length;		/* Input */
	gca_rcv->gca_descriptor= NULL;				/* Input */
        
	gca_rcv->gca_status    = E_GC0000_OK;			/* Output */

	IIGCa_cb_call( &IIgca_cb, GCA_RECEIVE, gca_parm, GCA_SYNC_FLAG, 0, 
		       (time_out > 0) ? time_out * 1000 : time_out, 
		       &gca_stat );

	if (gca_rcv->gca_status != E_GC0000_OK)	/* Check for success */
	{
	    if (gca_stat == E_GC0000_OK)
		gca_stat = gca_rcv->gca_status;
	}
	if (gca_stat != E_GC0000_OK)
	{
	    /* If we were just polling the system or timed out then return */
	    if (time_out >= 0 && gca_rcv->gca_status == E_GC0020_TIME_OUT)
	    {
		cgc_msg->cgc_message_type = 0;
		return;
	    } /* timed out */

	    if (time_out == -1 && gca_rcv->gca_status == E_GC0058_NOTMOUT_DLOCK)
	    {
		cgc_msg->cgc_message_type = GCA_ERROR;
		return;
	    }

	    /* If request was purged and we are mid-cancel then ignore */
		if (canceling && gca_rcv->gca_status == E_GC0027_RQST_PURGED)
		    continue;

	    IIcc3_util_status(gca_stat, &gca_rcv->gca_os_status, stat_buf);
	    if (gca_stat == E_GC0001_ASSOC_FAIL) 	/* Partner went away */
	    {
		IIcc1_util_error(cgc, FALSE, E_LC0025_READ_ASSOC_FAIL, 1,
				 stat_buf, (char *)0, (char *)0, (char *)0);
	    }
	    else
	    {
		IIcc1_util_error(cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
				 ERx("GCA_RECEIVE"), stat_buf,
				 (char *)0, (char *)0);
	    } /* if check error status */
	    cgc->cgc_g_state |= IICGC_FASSOC_MASK;
	    if ((cgc->cgc_m_state & IICGC_START_MASK) == 0)
		_VOID_ (*cgc->cgc_handle)(IICGC_HFASSOC, &handle_arg);
	    cgc->cgc_m_state |= IICGC_LAST_MASK;	/* Fake end-of-read */
	    return;
	} /* status not ok */

	/* Set up GCA_INTERPRET information */
	cgc_msg->cgc_message_type = 0;
	cgc_msg->cgc_d_length     = 0;

	gca_int = &gca_parm->gca_it_parms;
	gca_int->gca_buffer       = cgc_msg->cgc_buffer;/* Input */
	gca_int->gca_message_type = 0;			/* Output */
	gca_int->gca_data_area    = (char *)0;		/* Output */
	gca_int->gca_d_length     = 0;			/* Output */
	gca_int->gca_end_of_data  = 0;			/* Output */
	gca_int->gca_status       = E_GC0000_OK;	/* Output */

	IIGCa_cb_call( &IIgca_cb, GCA_INTERPRET, gca_parm, 
		       GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

	if (gca_int->gca_status != E_GC0000_OK)	/* Check for success */
	{
	    if (gca_stat == E_GC0000_OK)
		gca_stat = gca_int->gca_status;
	}
	if (gca_stat != E_GC0000_OK)
	{
	    IIcc3_util_status(gca_stat, &gca_int->gca_os_status, stat_buf);
	    IIcc1_util_error(cgc, FALSE, E_LC0001_SERVICE_FAIL, 2,
			     ERx("GCA_INTERPRET"),stat_buf,(char *)0,(char *)0);
	    cgc->cgc_m_state |= IICGC_LAST_MASK;	/* Fake end-of-read */
	    return;
	} /* status not ok */

	cgc_msg->cgc_message_type = gca_int->gca_message_type;
	cgc_msg->cgc_data     = gca_int->gca_data_area;
	cgc_msg->cgc_d_length = gca_int->gca_d_length;
	cgc_msg->cgc_d_used   = 0;
	end_of_data 	      = gca_int->gca_end_of_data;

	if (end_of_data)		/* Turn it on, otherwise turn it off */
	    cgc->cgc_m_state |= IICGC_EOD_MASK;
	else
	    cgc->cgc_m_state &= ~IICGC_EOD_MASK;

	/* If tracing then generate internal trace data */
	if (cgc->cgc_trace.gca_emit_exit)
	{
            if (cgc->cgc_g_state & IICGC_CHILD_MASK)
	        cgc->cgc_trace.gca_protocol = cgc->cgc_version;
	    gca_trb.gca_service 	= GCA_RECEIVE;
	    gca_trb.gca_association_id 	= cgc->cgc_assoc_id;
	    gca_trb.gca_message_type 	= cgc_msg->cgc_message_type;
	    gca_trb.gca_d_length 	= cgc_msg->cgc_d_length;
	    gca_trb.gca_data_area 	= cgc_msg->cgc_data;
	    gca_trb.gca_descriptor 	= NULL;
	    gca_trb.gca_end_of_data 	= end_of_data;
	    IIcgct2_gen_trace(&cgc->cgc_trace, &gca_trb);
	}

	/* If there was any trace information, flush it and continue */
	if (cgc_msg->cgc_message_type != GCA_TRACE && was_trace)
	{
	    was_trace = FALSE;
	    handle_arg.h_errorno = -1;		/* Flag to flush */
	    _VOID_ (*cgc->cgc_handle)(IICGC_HTRACE, &handle_arg);
	}

	switch (cgc_msg->cgc_message_type)
	{
	  case GCA_ACCEPT:
	  case GCA_IACK:
	    cgc->cgc_m_state |= IICGC_LAST_MASK;
	    break;

	  case GCA_REJECT:
	    if (   (cgc_msg->cgc_d_length > 0)
	        && (cgc->cgc_m_state & IICGC_ERR_MASK) == 0
		&& (!canceling))
		IIcc2_read_error(cgc);
	    cgc->cgc_m_state |= IICGC_LAST_MASK;
	    break;

	  case GCA_RELEASE:
	    cgc->cgc_g_state |= IICGC_FASSOC_MASK;
	    /* If there is an error message then display it - always do this */
	    if (   (cgc_msg->cgc_d_length > 0)
	        && (cgc->cgc_m_state & IICGC_ERR_MASK) == 0)
		IIcc2_read_error(cgc);
	    if ((cgc->cgc_m_state & IICGC_START_MASK) == 0)
		_VOID_ (*cgc->cgc_handle)(IICGC_HFASSOC, &handle_arg);
	    cgc->cgc_m_state |= IICGC_LAST_MASK;
	    break;

	  case GCA_S_BTRAN:
	    /*
	    ** Copy transaction id - currently UNUSED.
	    **
	    if ( 
		 cgc_msg->cgc_d_length > 0        &&  
		 cur_len < sizeof( GCA_TX_DATA )  &&
		 ! canceling 
	       )
	    {
		len = min( cgc_msg->cgc_d_length, 
			   sizeof( GCA_TX_DATA ) - cur_len );

		MECOPY_VAR_MACRO( (PTR)cgc_msg->cgc_data, len,
				  (PTR)((char *)&cgc->cgc_xact_id + cur_len) );

		cur_len += len;
		cgc_msg->cgc_d_used += len;
	    }
	    if ( ! end_of_data )  continue;
	    */
	    cgc->cgc_m_state |= IICGC_LAST_MASK;
	    break;

	  case GCA_DONE:
	  case GCA_REFUSE:
	  case GCA_S_ETRAN:
	  case GCA_RESPONSE:
	    /* 
	    ** Copy response structure 
	    */
	    if ( 
		 cgc_msg->cgc_d_length > 0        &&  
		 cur_len < sizeof( GCA_RE_DATA )  &&
		 ! canceling 
	       )
	    {
		len = min( cgc_msg->cgc_d_length, 
			   sizeof( GCA_RE_DATA ) - cur_len );

		MECOPY_VAR_MACRO( (PTR)cgc_msg->cgc_data, len,
				  (PTR)((char *)&cgc->cgc_resp + cur_len) );

		cur_len += len;
		cgc_msg->cgc_d_used += len;
	    }

	    if ( ! end_of_data )  continue;	/* read remainder of message */
	    cgc->cgc_m_state |= IICGC_LAST_MASK;
	    break;

	  case GCA_RETPROC:
	    /* 
	    ** Copy "procedure return" data
	    */
	    if ( 
	         cgc_msg->cgc_d_length > 0        &&
		 cur_len < sizeof( GCA_RP_DATA )  &&
		 ! canceling
	       )
	    {
		len = min( cgc_msg->cgc_d_length, 
			   sizeof( GCA_RP_DATA ) - cur_len );

		MECOPY_VAR_MACRO( (PTR)cgc_msg->cgc_data, len,
				  (PTR)((char *)&cgc->cgc_retproc + cur_len) );

		cur_len += len;
		cgc_msg->cgc_d_used += len;
	    }

	    if ( ! end_of_data )  continue;	/* read remainder of message */
	    if(cgc->cgc_version < GCA_PROTOCOL_LEVEL_60)
	    {
	        cgc->cgc_m_state |= IICGC_LAST_MASK;
	    }
	    break;

	  case GCA_C_INTO:
	  case GCA_C_FROM:
	  case GCA1_C_INTO:
	  case GCA1_C_FROM:
	    /*
	    ** Don't initiate COPY if a copy is already executing.  If the COPY 
	    ** information does not fit in one communication block, then the
	    ** server will send multiple blocks.
	    */
	    if (   (cgc->cgc_g_state & IICGC_COPY_MASK) == 0
		&& !canceling)
	    {
		handle_arg.h_errorno = cgc_msg->cgc_message_type;
		handle_arg.h_numargs = 0;
		handle_arg.h_data[0] = (char *)0;
		_VOID_ (*cgc->cgc_handle)(IICGC_HCOPY, &handle_arg);
	    }
	    break;

	  case GCA_ERROR:
	    if (   (cgc->cgc_m_state & IICGC_ERR_MASK) == 0
		&& !canceling)
		IIcc2_read_error(cgc);
	    break;

	  case GCA_TRACE:
	    if (!canceling)
	    {
		/* Call user supplied trace data handler */
		if (was_trace)
		    handle_arg.h_errorno = 0;
		else
		{
		    handle_arg.h_errorno = 1;	/* Flag first time */
		    was_trace = TRUE;
		}
		handle_arg.h_length = (i2)cgc_msg->cgc_d_length;
		handle_arg.h_numargs = 1;
		handle_arg.h_data[0] = (char *)cgc_msg->cgc_data;
		_VOID_ (*cgc->cgc_handle)(IICGC_HTRACE, &handle_arg);
		continue; /* And get next buffer */
	    }
	    break;

	  case GCA_PROMPT:
	    {
		handle_arg.h_length = (i2)cgc_msg->cgc_d_length;
		handle_arg.h_numargs = 1;
		handle_arg.h_data[0] = (char *)cgc_msg->cgc_data;
		_VOID_ (*cgc->cgc_handle)(IICGC_HPROMPT, &handle_arg);
		continue;
	    }
	    break;

	  case GCA_EVENT:
	    /* Don't recursively call handler if already processing event. */
	    if ((cgc->cgc_m_state & IICGC_PROCEVENT_MASK) == 0)
	    {
		handle_arg.h_numargs = 0;
		handle_arg.h_data[0] = (char *)0;
		_VOID_ (*cgc->cgc_handle)(IICGC_HVENT, &handle_arg);
	    }
	    /* 
	    ** In general, we want to loop back and get other data/messages.
	    ** The exception to this if when processing GET DBEVENT statement,
	    ** and when processing an EXECUTE PROCEDURE.  In those cases,

	    ** the program loop must iterate on events as well as errors
	    ** in order for WHENEVER handling to work properly.
	    */
	    /* One mask covers EXEC PROC and GET DBEVENT */
	    if ((cgc->cgc_m_state & IICGC_EVENTGET_MASK) == 0)
		continue;
	    break;

	  /* Make sure the type is a valid read type */
	  case GCA_MD_ASSOC:
	  case GCA_ATTENTION:
	  case GCA_QC_ID:
	  case GCA_TDESCR:
	  case GCA_TUPLES:
	  case GCA_CDATA:
	  case GCA_ACK:
	  case GCA_NP_INTERRUPT:
	    break;
	  default:				/* Invalid read type */
	    if (!canceling)
	    {
		char stat_buf3[30];
		IIcc1_util_error(cgc, FALSE, E_LC0024_READ_BAD_TYPE, 1,
				IIcc2_util_type_name(cgc_msg->cgc_message_type,
						     stat_buf3),
				(char *)0, (char *)0, (char *)0);
		cgc->cgc_m_state |= IICGC_LAST_MASK;
	    }
	    break;
	} /* Switch message type */

	return;		/* Only trace messages will not return here */
    } /* Looping for trace */
} /* IIcc3_read_buffer */

/*{
**  Name: IIcgc_find_cvar_length - Find length of compressed varchar
**
**  Description:
**	This routine returns the string length of a varchar embedded in its
**      first two bytes.
**
**  Inputs:
**	column		- Pointer to the varchar
**	compressed_type    - A flag indicating nullability status of the varchar.
**				 -2 - nullable Unicode varchar.
**				 2  - non-nullable Unicode varchar.
**				 -1 - nullable varchar.
**				 1  - non-nullable varchar.
**  Outputs:
**	Returns:
**     	    i4
**		Length of the varchar.
**
**	Errors:
**	    None.
**
**  Side Effects:
**  	None.
**	
**  History:
**	05-apr-1996 (loera01)
**	    Created.
*/

i4  
IIcgc_find_cvar_len(column, compressed_type)
char *column;   		/* Pointer to data */
i2 compressed_type;              /* Nullability status */

{
    u_i2	       length;

    MECOPY_CONST_MACRO((PTR)column,sizeof(u_i2),(PTR)&length);
    switch (compressed_type)
    {
        case (IICGC_NVCHRN):
	    length *= DB_ELMSZ;
        case (IICGC_VCHN):
	    return (length+sizeof(u_i2));  
        case (IICGC_NVCHR):
	    length *= DB_ELMSZ;
        case (IICGC_VCH):
	    return (length+sizeof(u_i2) + sizeof(char)); 
        default:
            return length;
    }
}
