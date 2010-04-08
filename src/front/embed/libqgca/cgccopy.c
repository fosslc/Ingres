/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<erlc.h>

/**
+*  Name: iicgccopy.c - This file contains routines to support COPY.
**
**  Description:
**	The routines in this file support the GCA COPY protocol.  They act
**	as a layer between the COPY handler (iicopy) and the GCA module.
**
**  Defines:
**
**	IIcgcp1_write_ack - 	 Send acknowledgement to server (GCA_ACK).
**	IIcgcp2_write_response - Send response message to server (GCA_RESPONSE).
**	IIcgcp3_write_data - 	 Set up buffers for sending COPY data.
**	IIcgcp4_write_read_poll- Poll GCA for pending reads.
**	IIcgcp5_read_data - 	 Read COPY data from GCA till response.
-*
**  History:
**	24-sep-1987	- Written (ncg)
**	15-jun-1990	(barbara)
**	    With bug fix 21832, which allows sessions with no connection
**	    to continue, we must check for null IICGC_DESC pointers.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
+*  Name: IIcgcp1_write_ack - Send acknowledge message to server.
**
**  Description:
**	This routine is used to send an ack message to the server.  The
**	COPY handler uses to inform that the COPY map has been successfully
**	read.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	ack		- Pointer to ack structure.
**	  .gca_ack_data	- Value is currently ignored.
**
**  Outputs:
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	24-sep-1987	- Written for GCA (ncg)
**	15-jun-1990	(barbara)
**	    With bug fix 21832, which allows sessions with no connection
**	    to continue, we must check for null IICGC_DESC pointers.
*/

VOID
IIcgcp1_write_ack(cgc, ack)
IICGC_DESC	*cgc;
GCA_AK_DATA	*ack;
{
    if (cgc == (IICGC_DESC *)0)
	return;
    /* Initialize the ack message, add ack data and send message */
    IIcgc_init_msg(cgc, GCA_ACK, DB_NOLANG, 0);
    IIcc3_put_bytes(cgc, TRUE, sizeof(*ack), (char *)ack);
    IIcgc_end_msg(cgc);
} /* IIcgcp1_write_ack */

/*{
+*  Name: IIcgcp2_write_response - Send response message to server.
**
**  Description:
**	This routine is used to send a response to the server.  The COPY
**	handler uses it so that the server knows when the COPY FROM (data
**	from file to database) is over.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	resp		- Pointer to response structure.
**
**  Outputs:
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	24-sep-1987	- Written for GCA (ncg)
**	15-jun-1990	(barbara)
**	    With bug fix 21832, which allows sessions with no connection
**	    to continue, we must check for null IICGC_DESC pointers.
*/

VOID
IIcgcp2_write_response(cgc, resp)
IICGC_DESC	*cgc;
GCA_RE_DATA	*resp;
{
    if (cgc == (IICGC_DESC *)0)
	return;
    /* Initialize the response message, add response data and send message */
    IIcgc_init_msg(cgc, GCA_RESPONSE, DB_NOLANG, 0);
    IIcc3_put_bytes(cgc, TRUE, sizeof(*resp), (char *)resp);
    IIcgc_end_msg(cgc);
} /* IIcgcp2_write_response */

/*{
+*  Name: IIcgcp3_write_data - Set up for writing COPY data.
**
**  Description:
**	This routine is used by COPY to set up the buffers for writing.  It is
**	called in a loop during COPY FROM (from a file to the database).
**	In order for COPY to avoid looking inside the cgc message buffers this
**	routine returns the data pointers and counters of the message buffers.
**
**	This is called in a loop to send COPY tuples, for example:
**
**	    ... client reads COPY map ...
**	    IIcgcp1_write_ack(cgc, &ack);
**
**	    IIcgc_init_msg(cgc, GCA_CDATA, DB_NOLANG, 0);
**	    ... set up COPY buffers to fill ...
**	    stat = IIcgcp3_write_data(cgc, FALSE, 0, &copy_dbv);
**	
**	    terminate = FALSE;
**	    do
**	    {
**		process COPY data from file into copy_dbv.db_data and
**		keep track of the number of bytes used (<= copy_dbv.db_length);
**
**		... send COPY buffers and reset pointers and counters ...
**	        stat = IIcgcp3_write_data(cgc, end_of_tuples,
**					  bytes_used, &copy_dbv);
**
**		... every few tuples check state ...
**		if (IIcgcp4_write_read_poll(cgc, GCA_CDATA) != GCA_CONTINUE)
**			terminate = TRUE;
**
**	    } while (there's more COPY data in file and !terminate);
**
**	    if (!terminate)
**		IIcgcp2_write_response(cgc, &resp);
**	
**	Note that the above logic allows the COPY module to send more than
**	one copy data message (much like tuple data returning from the DBMS).
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	end_of_data	- End of data indicator.
**	bytes_used	- Number of bytes used in the data area.  If this is
**			  the first time (when buffer pointers are being set
**		 	  up) then this should be 0.  Otherwise, this should
**			  indicate how many bytes were actually put into the
**			  data area, copy_dbv.
**	copy_dbv	- Pointer to DBV with copy data and length information.
**			  If bytes_used is 0 then this argument is ONLY used
**			  for output.
**	    .db_data	- Pointer to start of data area as set by this call on
**			  output.  The caller should not modify this value.
**			  The number of bytes pointed at is in 'bytes_used'.
**	
**  Outputs:
**	copy_dbv	- Pointer to DBV with message buffer data and length
**			  information.
**	    .db_data	- Pointer to start of data area.  Will be set to the
**			  write message data area.
**	    .db_length	- The number of bytes available within the data area.
**			  On subsequent calls, the caller must make sure that:
**			    bytes_used <= copy_dbv.db_length.
**
**	Returns:
**	    	STATUS 	- OK if sent successfully via GCA_SEND, FAIL otherwise.
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	24-sep-1987	- Written for GCA (ncg)
**	15-jun-1990	(barbara)
**	    With bug fix 21832, which allows sessions with no connection
**	    to continue, we must check for null IICGC_DESC pointers.
*/

STATUS
IIcgcp3_write_data(cgc, end_of_data, bytes_used, copy_dbv)
IICGC_DESC	*cgc;
bool		end_of_data;
i4		bytes_used;
DB_DATA_VALUE	*copy_dbv;
{
    IICGC_MSG	*cgc_msg;	/* Local message buffer */
    STATUS	retval;		/* Return value */

    if (cgc == (IICGC_DESC *)0)
	return FAIL;
    cgc_msg = &cgc->cgc_write_msg;
    if (bytes_used > 0)
    {
	cgc_msg->cgc_d_used = bytes_used;
	retval = IIcc1_send(cgc, end_of_data);
    }
    else	/* Just set up the buffers */
    {
	retval = OK;
    } /* If any bytes were used */

    copy_dbv->db_data   = (PTR)cgc_msg->cgc_data;
    copy_dbv->db_length = cgc_msg->cgc_d_length;
    return retval;
} /* IIcgcp3_write_data */

/*{
+*  Name: IIcgcp4_write_read_poll - During COPY writing poll for reads.
**
**  Description:
**	This routine is used by COPY during COPY FROM (from a file to the
**	database).  Because there may be many rows in the file and we are
**	not reading responses every few rows, there must be a way for the server
**	to interrupt the COPY process.  This is done via "polling" GCA every
**	few COPY tuples sent, to see if there is any pending data to read.
**
**	(This could have been done by GCA interrupting us via a handler, and
**	we could just mark that status.  Then, during the COPY FROM loop we
**	could check the status, and if set we would call this routine - which
**	would not poll anymore but know to read.)
**
**	If there is no data to read from the server we return "continue".
**
**	If there is anything we expect it to be an "interrupt" message. We
**	will reply with an "acknowledge" message and then read zero or
**	more "error" messages from the server (note that we have switched roles
**	and are now reading messages).  Following the last error we expect a
**	"response" message.  The status field is set to "continue" if this
**	error is not COPY-fatal.  If the return status is not "continue"
**	that means the server wants the COPY to be terminated, and in this
**	case NOTHING should be sent to the server following this call.
**
**	The message type of the surrounding control is passed as well, because
**	if we do have to acknowledge an interrupt we must re-initialize the
**	COPY message buffer for which we need the message type. 
**
**	See the example of IIcgcp3_write_data for usage of this routine.
**	
**  Inputs:
**	cgc		- Client general communications descriptor.
**	message_type	- Message type of surrounding control.  This is used in
**			  case we switch roles, and must implicitly reset the
**			  write message buffers.
**	
**  Outputs:
**	Returns:
**	    	The status value of the GCA_RESPONSE gca_rqstatus field:
**		GCA_CONTINUE_MASK - Continue sending COPY data.
**		Other		  - Stop now.
**		
**	Errors:
**		E_LC0020_READ_SETUP_FAIL  - Failed to read correct type.
**
**  Side Effects:
-*	
**  History:
**	24-sep-1987	- Written for GCA (ncg)
**	15-jun-1990	(barbara)
**	    With bug fix 21832, which allows sessions with no connection
**	    to continue, we must check for null IICGC_DESC pointers.
*/

i4
IIcgcp4_write_read_poll(cgc, message_type)
IICGC_DESC	*cgc;
i4		message_type;
{
    IICGC_MSG		*cgc_msg;	/* Local message buffer */
    GCA_AK_DATA		ack;		/* For acknowledge response */
    char		len_buf[20];	/* Length buffer for error */

    if (cgc == (IICGC_DESC *)0)
	return (i4)GCA_FAIL_MASK;

    /* Poll for any pending reads */
    IIcc3_read_buffer(cgc, (i4)0, GCA_EXPEDITED);

    /* If nothing was read return to continue */
    cgc_msg = &cgc->cgc_result_msg;
    if (cgc_msg->cgc_message_type == 0)
	return (i4)GCA_CONTINUE_MASK;

    /* Make sure we read the correct messages */
    if (   cgc_msg->cgc_message_type != GCA_NP_INTERRUPT
        && cgc_msg->cgc_message_type != GCA_ATTENTION)
    {
	char stat_buf[30];
	CVla(cgc_msg->cgc_d_length, len_buf);
	IIcc1_util_error(cgc, FALSE, E_LC0020_READ_SETUP_FAIL, 3,
			 ERx("GCA_NP_INTERRUPT"),
			 IIcc2_util_type_name(cgc_msg->cgc_message_type,
					      stat_buf),
			 len_buf, (char *)0);
	return (i4)GCA_FAIL_MASK;
    } /* if wrong type read */

    /* Send the IACK message */
    IIcgc_init_msg(cgc, GCA_IACK, DB_NOLANG, 0);
    ack.gca_ak_data = 0;
    IIcc3_put_bytes(cgc, TRUE, sizeof(ack), (char *)&ack);
    IIcgc_end_msg(cgc);

    /*
    ** Loop skipping error message buffers till response.  Errors will
    ** be implicitly handled.
    */
    do
    {
	IIcc3_read_buffer(cgc, IICGC_NOTIME, GCA_NORMAL);
    }
    while (cgc_msg->cgc_message_type != GCA_RESPONSE);

    /*
    ** The response has been read and cgc_resp has been filled.  Check the
    ** status.  If it's "continue" then we must reinitialize the messages
    ** for our caller to start writing.
    */
    if (cgc->cgc_resp.gca_rqstatus == GCA_CONTINUE_MASK)
	IIcgc_init_msg(cgc, message_type, DB_NOLANG, 0);

    return cgc->cgc_resp.gca_rqstatus;
} /* IIcgcp4_write_read_poll */

/*{
+*  Name: IIcgcp5_read_data - Read COPY data and handle errors and responses.
**
**  Description:
**	This routine is used by COPY to set up the buffers for reading.  It is
**	called in a loop during COPY INTO (from the database to a file).  This
**	routine reads and processes:
**
**	1. GCA_ERROR messages sent from the server.  Passes them to the
**	   standard error handler.  The COPY handler is not informed that an
**	   error was read.  The GCA_ERROR messages (and possibly GCA_TRACE
**	   messages) are implicitly skipped in this routine.
**	2. GCA_RESPONSE messages sent from the server, and that inform the
**	   COPY handler that there is no more COPY data to process.  In this
**	   case the COPY handler should clean up (close the file, etc) and
**	   return control to it's original caller.  The response structure
**	   will have been copied to the 'cgc_resp' field.
**	3. GCA_CDATA messages sent from the server.  In this case the COPY
**	   handler can read from the message buffers into the output area.
**
**	The COPY hander will regain control when a GCA_RESPONSE message or
**	GCA_CDATA message has been processed.
**
**	This is called in a loop to read COPY tuples, for example:
**
**	    ... client reads COPY map ...
**	    IIcgcp1_write_ack(cgc, &ack);
**
**	    while (IIcgcp5_read_data(cgc, &copy_dbv) == GCA_CDATA)
**	    {
**		process copy_dbv.db_length bytes of copy_dbv.db_data
**		into COPY output area;
**	    }
**	    ... client has read response so clean up ...
**	
**  Inputs:
**	cgc		- Client general communications descriptor.
**	
**  Outputs:
**	copy_dbv	- Pointer to DBV with copy data and length information.
**	    .db_data	- Pointer to start of data area.  Will be set to the
**			  result message data area.  If the message type
**			  returned is GCA_RESPONSE this is set to null.
**	    .db_length	- The number of bytes within the data area.  If the
**			  message type returned is GCA_RESPONSE this is 0.
**			  This number of bytes MUST be processed by the caller
**			  before this routine is called again, as the next
**			  call will force GCA to read a new message buffer.
**
**	Returns:
**	    	GCA_RESPONSE or GCA_CDATA to indicate the current message type.
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	24-sep-1987	- Written for GCA (ncg)
**	15-jun-1990	(barbara)
**	    With bug fix 21832, which allows sessions with no connection
**	    to continue, we must check for null IICGC_DESC pointers.
*/

i4
IIcgcp5_read_data(cgc, copy_dbv)
IICGC_DESC	*cgc;
DB_DATA_VALUE	*copy_dbv;
{
    IICGC_MSG		*cgc_msg;	/* Local message buffer */

    if (cgc == (IICGC_DESC *)0)
	return GCA_RESPONSE; 	/* Fake a response to cause caller to return */

    cgc_msg = &cgc->cgc_result_msg;
    do					/* Skip error and trace messages */
    {
	IIcc3_read_buffer(cgc, IICGC_NOTIME, GCA_NORMAL);
    }
    while (   (cgc_msg->cgc_message_type != GCA_CDATA)
	   && (cgc_msg->cgc_message_type != GCA_RESPONSE)
	   && (cgc->cgc_m_state & IICGC_LAST_MASK) == 0);

    if (cgc->cgc_m_state & IICGC_LAST_MASK)
	return GCA_RESPONSE; 	/* Fake a response to cause caller to return */

    if (cgc_msg->cgc_message_type == GCA_CDATA)	/* Point at COPY data */
    {
	copy_dbv->db_data   = (PTR)cgc_msg->cgc_data;
	copy_dbv->db_length = cgc_msg->cgc_d_length;
    }
    else					/* Response structure */
    {
	copy_dbv->db_data   = (PTR)0;
	copy_dbv->db_length = 0;
    } /* If message is COPY */

    return cgc_msg->cgc_message_type;
} /* IIcgcp5_read_data */
