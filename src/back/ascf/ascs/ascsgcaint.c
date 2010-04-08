/*
**Copyright (c) 2004 Ingres Corporation
*/

/* 
** Name: SCSGCAINT.C -- sequencer GCA interface
** 
** Description:
**
** History:
**     09-sep-1996 (tchdi01)
**         created for Jasmine
**      15-Jan-1999 (fanra01)
**          Renamed all functions as these are specific to ascs.
**      12-Feb-99 (fanra01)
**          Renamed scs_disassoc and scd_note to ascs and ascd equivalents.
**      06-Jun-2000 (fanra01)
**          Bug 101345
**          Move setting of SCG_NOT_EOD_MASK post assignment.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <tr.h>
#include    <er.h>
#include    <cs.h>
#include    <lk.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <usererror.h>

#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <ddb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sc0a.h>

#include    <scserver.h>

#include    <urs.h>
#include    <ascd.h>
#include    <ascs.h>

#include    <cui.h>


/*
** Forward/Extern references 
*/
DB_STATUS ascs_gca_flush_outbuf(SCD_SCB *scb);


/*
** Global variables owned by this file


/*
** Forward/Extern references 
*/


/*
** Global variables owned by this file
*/


/*
** Name: ascs_gca_send -- send queued messages
**
** Descripition:
**    This routine sends messages queued on the session's send queue
** Input:
**    scb                session control block
** Output:
**    none
** Return:
** History:
**    09-sep-1996 (tchdi01)
**        created for Jasmine
*/
DB_STATUS 
ascs_gca_send(SCD_SCB *scb)
{
    STATUS status = E_DB_OK;

    do 
    {
        status = scc_send(scb, FALSE);
	if (status == OK)
	    status = CSsuspend(CS_INTERRUPT_MASK | CS_BIO_MASK, 0, 0);
    } while (status == OK || status == E_CS0008_INTERRUPTED);

    if (   status 
	&& status != E_CS0008_INTERRUPTED 
	&& status != E_CS001D_NO_MORE_WRITES)
	status = E_DB_SEVERE;
    else
	status = E_DB_OK;


    return (status);
}



/*
** Name: ascs_gca_recv -- receive a message from FE
**
** Descripition:
**    This routine recieves message from the FE. The message
**    is interpreted and the SCS_GCA_DESC block in the session 
**    control block is initialized
** Input:
**    scb                session control block
** Output:
**    none
** Return:
**    E_DB_SERVER        communication error; the caller 
**                       must terminate the thread
** History:
*/
DB_STATUS 
ascs_gca_recv(SCD_SCB *scb)
{
    STATUS         status;
    i4             done = FALSE;
    SCS_GCA_DESC   *gca_desc = &scb->scb_sscb.sscb_gca_desc;
    DB_STATUS      error;



    scb->scb_sscb.sscb_state = SCS_INPUT;
    
    while (!done)
    {
	status = scc_recv(scb, FALSE);
	if (status == OK)
	    status = CSsuspend(CS_INTERRUPT_MASK | CS_BIO_MASK, 0, 0);

	if (   status
	    && status != E_CS0008_INTERRUPTED
	    && status != E_CS001E_SYNC_COMPLETION)
	{
	    status = E_DB_SEVERE;
	    goto func_exit;
	}

	if (status == E_CS0008_INTERRUPTED &&
	    scb->scb_sscb.sscb_flags & SCS_DROP_PENDING)
	{
	    /* The session is being dropped by SCF */
	    scb->scb_sscb.sscb_flags &= ~SCS_DROP_PENDING;
	    scb->scb_sscb.sscb_flags |= SCS_DROPPED_GCA;
	    ascs_disassoc(scb->scb_cscb.cscb_assoc_id);
	    if (scb->scb_sscb.sscb_interrupt < 0)
		scb->scb_sscb.sscb_interrupt = -scb->scb_sscb.sscb_interrupt;
	    status = E_DB_SEVERE;
	    goto func_exit;
	}

	if (scb->scb_cscb.cscb_gci.gca_status != E_GC0000_OK)
	{
	    if (   scb->scb_cscb.cscb_gci.gca_status == E_GCFFFF_IN_PROCESS
		|| scb->scb_cscb.cscb_gci.gca_status == E_SC1000_PROCESSED
		|| scb->scb_cscb.cscb_gci.gca_status == E_GC0027_RQST_PURGED)
	    {
		/*
		 * This is normal after an interrupt
		 */
		if (scb->scb_sscb.sscb_interrupt == 0)
		{
		    scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
		    continue;
		}
	    }
	    else
	    {
		/* the client session must have disconnected
		   terminate the session */
		status = E_DB_SEVERE;
		goto func_exit;
	    }
	}
	done = TRUE;
    }

    scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
    if (scb->scb_sscb.sscb_interrupt == 0)
    {
	gca_desc->buf_ptr  = 
	gca_desc->data_ptr = 
	gca_desc->msg_ptr  =
		scb->scb_cscb.cscb_gci.gca_data_area;

	gca_desc->buf_len  =
	gca_desc->data_len =
	gca_desc->msg_len  =
		scb->scb_cscb.cscb_gci.gca_d_length;

	gca_desc->msg_continued =
		!scb->scb_cscb.cscb_gci.gca_end_of_data;
    }
    else
    {
	gca_desc->buf_ptr  = 
	gca_desc->data_ptr = 
	gca_desc->msg_ptr  =
		scb->scb_cscb.cscb_gce.gca_data_area;

	gca_desc->buf_len  =
	gca_desc->data_len =
	gca_desc->msg_len  =
		scb->scb_cscb.cscb_gce.gca_d_length;

	gca_desc->msg_continued =
		!scb->scb_cscb.cscb_gce.gca_end_of_data;
    }
    status = E_DB_OK;

func_exit:

    scb->scb_sscb.sscb_state   = SCS_PROCESS;
    scb->scb_sscb.sscb_is_idle = FALSE;

    
    return (status);
}


/*{
** Name: ascs_gca_get	- read a piece from buffer
**
** Description:
**     This routine reads a chunk from the sessions receive
**     buffer
**
** Inputs:
**     scb                session control block
**     get_ptr            buffer allocated by caller
**     get_len            num of bytes to read
**
** Outputs:
**     get_ptr            buffer filled with data
**
** History:
**	26-june-1996 (reijo01)
**	    Written.
**      09-sep-1996 (tchdi01)
**          Added handling of the empty buffer condition. 
**          Now we read from the FE directly without invloving
**          the dispatcher
*/
DB_STATUS
ascs_gca_get(SCD_SCB *scb, PTR get_ptr, i4 get_len)
{
    SCS_GCA_DESC 	*buf = &scb->scb_sscb.sscb_gca_desc;	
    i4		copy_len;
    DB_STATUS           status;

    while (   get_len > 0 
	   && (buf->msg_len > 0 || buf->msg_continued))
    {
	if (buf->msg_len == 0)
	{
	    /*
	    ** Get more data from the front end
	    */
	    status = ascs_gca_recv(scb);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	}

	/*
	** Copy the data into supplied buffer
	*/
	copy_len = min(get_len, buf->msg_len);
	MEcopy(buf->msg_ptr, copy_len, get_ptr);

	/*
	** Update pointers and counters 
	*/
	buf->msg_ptr = (PTR)((char *)buf->msg_ptr + copy_len);
	buf->msg_len -= copy_len;
	get_ptr = (PTR)((char *)get_ptr + copy_len);
	get_len -= copy_len;
    }

    return ((get_len == 0) ? E_DB_OK : E_DB_ERROR);
}



/*
** Name: ascs_gca_data_available -- check the buffer
**
** Descripition:
**    This routine returns a value greate than zero if there is 
**    something in the session's receive buffer or if more can 
**    be read from the front end
** Input:
** Output:
** Return:
** History:
*/
i4
ascs_gca_data_available(SCD_SCB *scb)
{
    SCS_GCA_DESC *buf = &scb->scb_sscb.sscb_gca_desc;	

    return (buf->msg_len || buf->msg_continued);
}


/*{
** Name: ascs_gca_error	- Process error from GCA
**
** Description:
**      This routine processes errors from GSA.  If the error is the 
**      fault of the caller, then the error_blame parameter is used 
**      to identify the caller in the error log.
**
** Inputs:
**      status                          DB_STATUS returned by gca
**      err_code                        error code "	   " 
**      error_blame                     Error to log if the error is not
**                                      gca's fault
**
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none	    
**
** Side Effects:
**	    none
**
** History:
**	12-june-1996 (lewda02)
**	    Stripped from Ingres for Jasmine.
*/
VOID
ascs_gca_error(DB_STATUS status,
	      i4 err_code,
	      i4 error_blame )
{
	sc0e_0_put(err_code, 0);
	sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
	ascd_note(E_DB_SEVERE, DB_GCF_ID);
}



/*
** Name: ascs_gca_flush -- flush unread message part
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS 
ascs_gca_flush(SCD_SCB *scb)
{
    DB_STATUS status = E_DB_OK;
    SCS_GCA_DESC *buf = &scb->scb_sscb.sscb_gca_desc;	

    while (status == E_DB_OK && buf->msg_continued)
    {
	status = ascs_gca_recv(scb);
    }

    return (status);
}



/*
** Name: ascs_gca_put -- format and queue a new message
**
** Descripition:
**    This routine is used to put a message on the GCA 
**    output queue of the current session. 
** Input:
**    scb               session control block
**    msgtype           type of the message
**    len               length of the message
**    data              contents of the message
** Output:
**    none
** Return:
**    E_DB_OK           successful
**    E_DB_ERROR        error occurred
** History:
**    07-sep-1996 (tchdi01)
**        created for Jasmine
*/
DB_STATUS 
ascs_gca_put(SCD_SCB *scb, i4  msgtype, i4 len, PTR data)
{
    DB_STATUS status = E_DB_OK;
    SCC_GCMSG *message;
    DB_STATUS error;

    /*
    ** Allocate the fresh message buffer 
    */
    status = sc0m_allocate(0, 
	(i4)(sizeof(SCC_GCMSG) + len), 
	DB_SCF_ID,
	(PTR)SCS_MEM,
	SCG_TAG,
	(PTR *)&message);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /*
    ** Put the message on the send queue 
    */
    message->scg_mask = 0;
    message->scg_marea =
	((char *) message + sizeof(SCC_GCMSG));
    message->scg_next = 
	(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    message->scg_prev = 
	scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = 
	message;
    scb->scb_cscb.cscb_mnext.scg_prev = 
	message;

    /*
    ** Format the message and fill it with data 
    */
    message->scg_mtype = msgtype;
    message->scg_msize = len;

    MEcopy(data, len, message->scg_marea);

    return (status);
}


/*
** Name: ascs_gca_clear_outbuf -- clear session's outbuf
**
** Descripition:
**    This routine clears the session's output buffer. Usually 
**    this is done before the results of a new query are written
** Input:
** Output:
** Return:
** History:
**    10-sep-1996 (tchdi01)
**        created for Jasmine
*/
DB_STATUS
ascs_gca_clear_outbuf(SCD_SCB *scb)
{
    DB_STATUS status = E_DB_OK;
    SCC_CSCB  *sccb  = &scb->scb_cscb;
    SCC_GCMSG *message = (SCC_GCMSG *)scb->scb_cscb.cscb_outbuffer;

    sccb->cscb_tuples_len = 0;
    message->scg_msize    = 0;

    return (status);
}


/*
** Name: ascs_gca_outbuf -- write to message to outbuf
**
** Descripition:
**    The following routine adds message data to the body of 
**    the preformatted message in the session's output buffer.
**    If the buffer is full, a partial message is sent to the 
**    user before the new data is put in.
** Input:
** Output:
** Return:
** History:
**    09-sep-1996 (tchdi01)
**        created for Jasmine
**      06-Jun-2000 (fanra01)
**          Move setting of SCG_NOT_EOD_MASK post assignment.
*/
DB_STATUS 
ascs_gca_outbuf(SCD_SCB *scb, i4 msg_len, char *msg_ptr)
{
    DB_STATUS status = E_DB_OK;
    SCC_CSCB  *sccb  = &scb->scb_cscb;
    i4   len;
    SCC_GCMSG *message = (SCC_GCMSG *)scb->scb_cscb.cscb_outbuffer;

    /*
    ** If the message is to big to fit in the outbuffer, copy
    ** as much of it as possible into the buffer, send out
    ** the buffer, and than copy the rest 
    */
    while (sccb->cscb_tuples_len + msg_len > sccb->cscb_tuples_max)
    {
	if (sccb->cscb_tuples_len < sccb->cscb_tuples_max)
	{
	    len = sccb->cscb_tuples_max - sccb->cscb_tuples_len;

	    MEcopy(msg_ptr, len, &sccb->cscb_tuples[sccb->cscb_tuples_len]);
	    sccb->cscb_tuples_len += len;
	    msg_len -= len;
	    msg_ptr += len;
	}

	/*
	** Turn the end_of_data flag OFF. This will tell the peer
	** that the message will be continued
	*/
	message->scg_mask  = SCG_NODEALLOC_MASK;
	message->scg_mask |= SCG_NOT_EOD_MASK;
	message->scg_mtype = GCA_TUPLES;
	message->scg_msize = sccb->cscb_tuples_len;

	/*
	** Send out the queued messages
	*/
	status = ascs_gca_send(scb);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/*
	** Re-queue the stdout message back on the send queue 
	*/
	message->scg_mask &= ~SCG_NOT_EOD_MASK;
	message->scg_next = 
	    (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev = 
	    scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = 
	    message;
	scb->scb_cscb.cscb_mnext.scg_prev = 
	    message;

	sccb->cscb_tuples_len = 0;
    }

    MEcopy(msg_ptr, msg_len, &sccb->cscb_tuples[sccb->cscb_tuples_len]);
    sccb->cscb_tuples_len += msg_len;

    /*
    ** Update the message fields
    */
    message->scg_msize = sccb->cscb_tuples_len;

    return (status);
}




/*
** Name: ascs_gca_flush_outbuf -- flush contents of the outbuf
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS 
ascs_gca_flush_outbuf(SCD_SCB *scb)
{
    DB_STATUS status = E_DB_OK;
    SCC_CSCB  *sccb  = &scb->scb_cscb;
    SCC_GCMSG *message = (SCC_GCMSG *)scb->scb_cscb.cscb_outbuffer;

    /*
    ** Send out the queued messages
    */
    status = ascs_gca_send(scb);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /*
    ** Re-queue the stdout message back on the send queue 
    */
    message->scg_next = 
	(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    message->scg_prev = 
	scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = 
	message;
    scb->scb_cscb.cscb_mnext.scg_prev = 
	message;

    sccb->cscb_tuples_len = 0;

    return (status);
}


/*
** Name:
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
