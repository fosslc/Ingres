/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icegca.c
**
** Description:
**      Module provides client side access to the ice server.
**
** History:
**      08-Mar-99 (fanra01)
**          Add history.
**          Reformatted spacing.
**          Make server class dependent on define.
**      09-Mar-1999 (fanra01)
**          Put the node and server class in the right order for connect.
**      06-Mar-2000 (fanra01)
**          Bug 100745
**          When an initialise call is performed to begin reconnecting to
**          to the ice server after a terminate a duplicate initialise
**          error is returned causing the remaining initialise to fail and
**          a failed sempahore message to be returned.
**      18-May-2000 (fanra01)
**          Bug 101345
**          Corrected MD_ASSOC message parameters and length which allows net
**          to perform conversion.  Also modified the ICEGCAWait, ICEGCAHeader
**          and ICEGCAPage to handle the tuple message format.
**          Modified ICEGCASendData to send query data longer than a gca
**          message buffer.
**      18-Jul-2000 (fanra01)
**          Bug 102126
**          Modify request check to the status held in the gca control block,
**          as function status does not return communications failure.
**      24-Nov-2000 (fanra01)
**          Bug 103328
**          Modified the test for the terminating blob segment indicator
**          so that if we don't get a terminating one we don't assume that
**          the remaining message is a continuation.
**	30-Aug-2002 (hanje04)
**	    Bug 108477
**	    If the initial IIGCa_cb_call() fails in ICEGCAConnect then we 
**	    shouldn't call ICEGCADisconnect before we exit otherwise we SEGV
**	    trying to disconnect a non existent session.
**	04-Nov-2002 (hanje04)
**	    Bug 108477
**	    Initial fix for this bug disconnected all sessions (oops!). Redo
**	    fix, and set VALID_GC_SESS=TRUE after sucessful initial connection
**	    so that we know when it is O.K. to ICEGCADisconnect that session.
*/

#include <icegca.h>
#include <adf.h>
#include <adp.h>

#define BLOBHDR     1
#define BLOBIND     2
#define BLOBSEG     4
#define BLOBDATA    8
#define BLOBTIND   16

static	PTR		cb = NULL;
static	i4		timeout = -1;

/*
** Name: ICEGCAInitiate
**
** Description:
**      Initialise the GCA interface.
**
** Inputs:
**      time        number of seconds to attempt connection
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**      06-Mar-2000 (fanra01)
**          Bug 100745
**          Mutliple calls of GCA_INITIATE cause an error to be returned and
**          a failed open semaphore message.  Handle the duplicate initiate
**          case.
*/
GSTATUS
ICEGCAInitiate( i4 time, i4* gca_hdr_len )
{
    GSTATUS         err = GSTAT_OK;
    GCA_PARMLIST    gca_parms;
    STATUS          status = E_GC0000_OK;

    if (time > 0)
        timeout = time;

    MEfill(sizeof(GCA_PARMLIST), EOS, &gca_parms);
    IIGCa_cb_call(&cb, GCA_INITIATE, &gca_parms, GCA_SYNC_FLAG, 0, timeout,
        &status);

    switch(status)
    {
        case E_GC0000_OK:
        case E_GC0006_DUP_INIT:
            if (gca_hdr_len != NULL)
            {
                *gca_hdr_len = gca_parms.gca_in_parms.gca_header_length;
            }
            break;

        default:
            err = DDFStatusAlloc(status);
            break;
    }
    return(err);
}

/*
** Name: ICEGCAConnect
**
** Description:
**      Attempt connection with requested node.
**
** Inputs:
**      session     client session control block
**      node        node to connect to.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**      09-Mar-1999 (fanra01)
**          Put the node and server class in the right order.
**      04-May-2000 (fanra01)
**          Corrected MD_ASSOC message parameters and length.
**      18-Jul-2000 (fanra01)
**          Modify request check to the status held in the gca control block.
*/
GSTATUS
ICEGCAConnect( PICEGCASession session, char* node )
{
    GSTATUS err = GSTAT_OK;
    char    name[50];
    STATUS  status = E_GC0001_ASSOC_FAIL;
    i4    VALID_GC_SESS = FALSE;

    if (node != NULL && node[0] != EOS)
        STprintf(name, "%s::dbname/%s", node, GCA_ICE_CLASS);
    else
        STprintf(name, "dbname/%s", GCA_ICE_CLASS);
    MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
    session->gca_parms.gca_rq_parms.gca_partner_name = name;
    IIGCa_cb_call(&cb, GCA_REQUEST, &session->gca_parms, GCA_SYNC_FLAG, 0,
        timeout, &status);

    if ((status == E_GC0000_OK) &&
        ((status = session->gca_parms.gca_rq_parms.gca_status) == E_GC0000_OK))
    {
	VALID_GC_SESS = TRUE;
        session->aid = session->gca_parms.gca_rq_parms.gca_assoc_id;
        MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
        session->gca_parms.gca_rv_parms.gca_buffer = session->buf;
        session->gca_parms.gca_rv_parms.gca_b_length = sizeof(session->buf);
        session->gca_parms.gca_rv_parms.gca_association_id = session->aid;
        session->gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
        IIGCa_cb_call(&cb, GCA_RECEIVE, &session->gca_parms, GCA_SYNC_FLAG, 0,
            timeout, &status);
        if (status == E_GC0000_OK)
        {
            MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
            session->gca_parms.gca_it_parms.gca_buffer = session->buf;
            IIGCa_cb_call(&cb, GCA_INTERPRET, &session->gca_parms,
                GCA_SYNC_FLAG, 0, timeout, &status);
            if (status == E_GC0000_OK)
            {
                if (session->gca_parms.gca_it_parms.gca_message_type != GCA_ACCEPT)
                    status = E_GC0001_ASSOC_FAIL;
            }
        }
    }
    if (status == E_GC0000_OK)
    {
        MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
        session->gca_parms.gca_fo_parms.gca_buffer = session->buf;
        session->gca_parms.gca_fo_parms.gca_b_length = sizeof(session->buf);
        IIGCa_cb_call(&cb, GCA_FORMAT, &session->gca_parms, GCA_SYNC_FLAG, 0,
            timeout, &status);
        if (status == E_GC0000_OK)
        {
            i4 ascdata = GCA_SVNORMAL;  /* variable to send in MD_ASSOC */
            i4 msglen = 0;              /* message length */
            GCA_SESSION_PARMS *area =
                (GCA_SESSION_PARMS *) session->gca_parms.gca_fo_parms.gca_data_area;

            area->gca_l_user_data = 1;
            msglen += sizeof(i4);
            area->gca_user_data[0].gca_p_index = GCA_SVTYPE;
            msglen += sizeof(i4);
            area->gca_user_data[0].gca_p_value.gca_type = DB_INT_TYPE;
            msglen += sizeof(i4);
            area->gca_user_data[0].gca_p_value.gca_precision = 0;
            msglen += sizeof(i4);
            area->gca_user_data[0].gca_p_value.gca_l_value = sizeof(i4);
            msglen += sizeof(i4);
            MECOPY_CONST_MACRO(&ascdata, sizeof(i4),
                area->gca_user_data[0].gca_p_value.gca_value );
            msglen += sizeof(i4);

            MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
            session->gca_parms.gca_sd_parms.gca_buffer = session->buf;
            /*
            ** Modified from using sizeof to calculate message length as
            ** the aligned size of GCA_SESSION_PARMS was returned on NT.
            */
            session->gca_parms.gca_sd_parms.gca_msg_length = msglen;

            session->gca_parms.gca_sd_parms.gca_message_type = GCA_MD_ASSOC;
            session->gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
            session->gca_parms.gca_sd_parms.gca_association_id = session->aid;
            IIGCa_cb_call(&cb, GCA_SEND, &session->gca_parms, GCA_SYNC_FLAG,
                (PTR)0, timeout, &status);
            if (status == E_GC0000_OK)
            {
                MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
                session->gca_parms.gca_rv_parms.gca_buffer = session->buf;
                session->gca_parms.gca_rv_parms.gca_b_length = sizeof(session->buf);
                session->gca_parms.gca_rv_parms.gca_association_id = session->aid;
                session->gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
                IIGCa_cb_call(&cb, GCA_RECEIVE, &session->gca_parms,
                    GCA_SYNC_FLAG, 0, timeout, &status);
                if (status == E_GC0000_OK)
                {
                    MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
                    session->gca_parms.gca_it_parms.gca_buffer = session->buf;
                    IIGCa_cb_call(&cb, GCA_INTERPRET, &session->gca_parms,
                        GCA_SYNC_FLAG, 0, timeout, &status);
                    if (status == E_GC0000_OK)
                        if (session->gca_parms.gca_it_parms.gca_message_type != GCA_ACCEPT)
                            status = E_GC0001_ASSOC_FAIL;
                }
            }
        }
    }
    if (status != E_GC0000_OK)
    {
	    if (VALID_GC_SESS) /* only disconnect if
					initial connection suceeded */
        	CLEAR_STATUS(ICEGCADisconnect(session));
            err = DDFStatusAlloc(status);
    }
    return(err);
}

/*
** Name: ICEGCASendQuery
**
** Description:
**      Send a query request to the ice server.
**
** Inputs:
**      session     client session control block
**      query       requested url
**      size        length of query string
**      end         end of query indicator
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**      12-Jun-2000 (fanra01)
**          Set the language id to DB_ICE.
*/
GSTATUS
ICEGCASendQuery( PICEGCASession session, char* query, u_i4 size, bool end )
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = E_GC0000_OK;

    MEfill( sizeof(GCA_PARMLIST), EOS, &session->gca_parms );
    session->gca_parms.gca_fo_parms.gca_buffer = session->buf;
    session->gca_parms.gca_fo_parms.gca_b_length = sizeof(session->buf);
    status = IIGCa_cb_call( &cb, GCA_FORMAT, &session->gca_parms,
        GCA_SYNC_FLAG, 0, timeout, &status );

    if (status == E_GC0000_OK)
    {
        GCA_Q_DATA  *query_data;
        MEfill( session->gca_parms.gca_fo_parms.gca_d_length,
                0,
                session->gca_parms.gca_fo_parms.gca_data_area );

        query_data = (GCA_Q_DATA *)(char*)session->gca_parms.gca_fo_parms.gca_data_area;
        query_data->gca_language_id = DB_ICE;
        query_data->gca_query_modifier = GCA_NAMES_MASK | GCA_COMP_MASK;
        query_data->gca_qdata[0].gca_type = DB_QTXT_TYPE;
        query_data->gca_qdata[0].gca_l_value = size + 1;
        MECOPY_VAR_MACRO( query,
            query_data->gca_qdata[0].gca_l_value,
            query_data->gca_qdata[0].gca_value );

        MEfill(sizeof(GCA_PARMLIST), 0, &session->gca_parms);
        session->gca_parms.gca_sd_parms.gca_buffer = session->buf;
        session->gca_parms.gca_sd_parms.gca_msg_length =
            sizeof(query_data->gca_language_id) +
            sizeof(query_data->gca_query_modifier) +
            sizeof(query_data->gca_qdata[0].gca_type) +
            sizeof(query_data->gca_qdata[0].gca_precision) +
            sizeof(query_data->gca_qdata[0].gca_l_value) +
            query_data->gca_qdata[0].gca_l_value;

        session->gca_parms.gca_sd_parms.gca_message_type = GCA_QUERY;
        session->gca_parms.gca_sd_parms.gca_end_of_data = end;
        session->gca_parms.gca_sd_parms.gca_association_id = session->aid;

        status = IIGCa_cb_call(&cb, GCA_SEND, &session->gca_parms,
            GCA_SYNC_FLAG, 0, timeout, &status);
    }
    if (status != E_GC0000_OK)
        err = DDFStatusAlloc(status);
    return(err);
}

/*
** Name: ICEGCASendData
**
** Description:
**      Send a data buffer to the ice server.
**
** Inputs:
**      session     client session control block
**      data        data buffer
**      size        length of data buffer
**      end         end of message indicator
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**      06-Jun-2000 (fanra01)
**          Modified to send query data as blob.
*/

/*
** Name: ICEGCASendData
**
** Description:
**      Send a data buffer to the ice server.
**
** Inputs:
**      session     client session control block
**      data        data buffer
**      size        length of data buffer
**      end         end of message indicator
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**      06-Jun-2000 (fanra01)
**          Modified to send query data as blob.
*/
GSTATUS
ICEGCASendData( PICEGCASession session, char* data, i4 size, i4* flags,
    i4* sent )
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = E_GC0000_OK;
    GCA_FO_PARMS*   foparms = &session->gca_parms.gca_fo_parms;

    MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
    foparms->gca_buffer = session->buf;
    foparms->gca_b_length = sizeof(session->buf);
    status = IIGCa_cb_call(&cb, GCA_FORMAT, &session->gca_parms,
        GCA_SYNC_FLAG, 0, timeout, &status);

    if (status == E_GC0000_OK)
    {
        GCA_TD_DATA     gca_tdesc;
        GCA_SD_PARMS    *sdparms;
        GCA_DATA_VALUE  *params;
        i4              max_size;
        u_i4            length = 0;
        i2              seglen = 0;
        i4              ind = 1;
        ADP_PERIPHERAL  hdr;
        char*           buf;
        i4              msglen = 0;

        MEfill( foparms->gca_d_length, 0, foparms->gca_data_area );

        /*
        ** Calculate the maximum amount of data that can be sent less
        ** segment indicator and segment length
        */
        max_size = GCA_BUFFER_SIZE -
            (session->gca_hdr_len + sizeof(i4) + sizeof(i2));

        /*
        ** If this is first time through send the descriptor and blob header
        */
        if (*flags & ICE_QUERY_HEADER)
        {
            params = (GCA_DATA_VALUE*)(char*)foparms->gca_data_area;
            params->gca_type = DB_LBYTE_TYPE;
            params->gca_precision = 0;
            params->gca_l_value = ADP_HDR_SIZE + sizeof(i4);

            buf = params->gca_value;

            msglen += (sizeof(params->gca_type) + sizeof(params->gca_precision) +
                sizeof(params->gca_l_value));

            /*
            ** Set the blob header
            */
            MEfill( sizeof(ADP_PERIPHERAL) , 0, (PTR)&hdr );
            hdr.per_tag = ADP_P_GCA_L_UNK;
            hdr.per_length1 = 0;

            /*
            ** Write the blob header into message buffer
            */
            MECOPY_VAR_MACRO( &hdr, ADP_HDR_SIZE, buf);
            buf += ADP_HDR_SIZE;
            msglen += ADP_HDR_SIZE;
        }
        else
        {
            buf = (char*)foparms->gca_data_area;
        }

        MEfill(sizeof(GCA_PARMLIST), 0, &session->gca_parms);
        sdparms = &session->gca_parms.gca_sd_parms;
        sdparms->gca_buffer = session->buf;
        sdparms->gca_message_type = GCA_QUERY;
        sdparms->gca_association_id = session->aid;
        sdparms->gca_end_of_data = FALSE;
        {
            GCA_COL_ATT*    data_tdesc;

            gca_tdesc.gca_tsize = 0x10;
            gca_tdesc.gca_id_tdscr = session->aid;
            gca_tdesc.gca_l_col_desc = 1;
            gca_tdesc.gca_result_modifier = GCA_LO_MASK;
            data_tdesc = &gca_tdesc.gca_col_desc[0];
            data_tdesc->gca_attdbv.db_data = NULL;
            data_tdesc->gca_attdbv.db_length = sizeof( i4 ) + ADP_HDR_SIZE;
            data_tdesc->gca_attdbv.db_datatype = DB_LBYTE_TYPE;
            data_tdesc->gca_attdbv.db_prec = 0;
            data_tdesc->gca_l_attname = 0;
        }
        sdparms->gca_descriptor = (PTR)&gca_tdesc;

        if (size > 0)
        {
            /*
            ** segment indicator
            */
            MECOPY_VAR_MACRO( &ind, sizeof(i4), buf );
            buf += sizeof(i4);
            msglen += sizeof(i4);

            /*
            ** max segment length
            */
            seglen = (i2)(max_size - msglen - sizeof(i2));

            /*
            ** Set segment length to size since it fits in a message.
            */
            if (size < seglen)
            {
                seglen = size;
            }

            MECOPY_VAR_MACRO( &seglen, sizeof(i2), buf );
            buf += sizeof(i2);
            msglen += sizeof(i2);

            MECOPY_VAR_MACRO( data + length, seglen, buf );
            msglen += seglen;
            length += seglen;
            size -= seglen;
            buf += seglen;
        }
        if ((*flags & ICE_QUERY_FINAL) != 0)
        {
            /*
            ** If terminating indicator cannot be sent
            ** leave the ICE_QUERY_FINAL flag set
            */
            if ((max_size - msglen) > sizeof(ind))
            {
                ind = 0;
                MECOPY_VAR_MACRO( &ind, sizeof(i4), buf );
                buf += sizeof(i4);
                msglen += sizeof(i4);
                *flags &= ~ICE_QUERY_FINAL;
                sdparms->gca_end_of_data = TRUE;
            }
        }
        sdparms->gca_msg_length = msglen;
        status = IIGCa_cb_call(&cb, GCA_SEND, &session->gca_parms,
            GCA_SYNC_FLAG, 0, timeout, &status);

        *sent = seglen;
    }

    if (status != E_GC0000_OK)
    {
        err = DDFStatusAlloc(status);
    }
    return(err);
}

/*
** Name: ICEGCAReceive
**
** Description:
**      Function to a recieve a GCA message.
**
** Inputs:
**      session     client session control block.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**      18-May-2000 (fanra01)
**          Add initialization of session variables for handling tuple
**          processing.
*/
GSTATUS
ICEGCAReceive( PICEGCASession session )
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = E_GC0000_OK;

    MEfill(sizeof(session->gca_parms), 0, &session->gca_parms);
    session->gca_parms.gca_rv_parms.gca_buffer = session->buf;
    session->gca_parms.gca_rv_parms.gca_b_length = sizeof(session->buf);
    session->gca_parms.gca_rv_parms.gca_association_id = session->aid;
    session->gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
    status= IIGCa_cb_call( &cb, GCA_RECEIVE, &session->gca_parms,
        GCA_SYNC_FLAG, (PTR)0, timeout, &status );

    if (status == E_GC0000_OK)
    {
        GCA_IT_PARMS* itparms = &session->gca_parms.gca_it_parms;

        MEfill(sizeof(session->gca_parms), 0, &session->gca_parms);
        itparms->gca_buffer = session->buf;
        status= IIGCa_cb_call( &cb, GCA_INTERPRET, &session->gca_parms,
           GCA_SYNC_FLAG, (PTR)0, timeout, &status );
        if (status == E_GC0000_OK)
        {
            switch(itparms->gca_message_type)
            {
                case GCA_TDESCR:
                    break;
                case GCA_TUPLES:
                    session->mlen = itparms->gca_d_length;
                    MEcopy( itparms->gca_data_area, session->mlen,
                        session->rcv + session->flen );
                    session->mlen += session->flen;
                    session->flen = 0;
                    break;
                case GCA_RESPONSE:
                default:
                    session->complete = TRUE;
                    session->mlen = session->flen;
                    session->flen = 0;
                    break;
            }
        }
    }
    if (status != E_GC0000_OK)
        err = DDFStatusAlloc(status);
    return(err);
}

/*
** Name: ICEGCAWait
**
** Description:
**      Wait for a response to a sent gca message.
**
** Inputs:
**      session     client session control block.
**
** Outputs:
**      messageType received respose message type.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**      18-May-2000 (fanra01)
**          Changed to read the tuple descriptor message and the first tuple
**          to get response type.
*/
GSTATUS
ICEGCAWait( PICEGCASession session, u_i4 *messageType )
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = E_GC0000_OK;

    session->tdata = 0;
    session->blobsegs = 0;
    session->complete = FALSE;

    err = ICEGCAReceive( session );
    if (err == GSTAT_OK)
    {
        GCA_IT_PARMS* itparms = &session->gca_parms.gca_it_parms;
        if (itparms->gca_message_type == GCA_TDESCR)
        {
            if ((err = ICEGCAReceive( session )) == GSTAT_OK)
            {
                u_i4 msgtype;

                if (itparms->gca_message_type == GCA_TUPLES)
                {
                    MEcopy(itparms->gca_data_area, sizeof(u_i4), &msgtype);
                    *messageType = msgtype;
                    /*
                    ** Set offset to next tuple
                    */
                    session->tdata += sizeof(u_i4);
                }
            }
        }
        else
            err = DDFStatusAlloc(E_GC0001_ASSOC_FAIL);
    }
    return(err);
}

/*
** Name: ICEGCAHeader
**
** Description:
**      Receive a gca message as an HTTP header.
**
** Inputs:
**      session     client session control block.
**
** Outputs:
**      result      pointer to data
**      returned    number of bytes of data returned
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**          Add processing to handle result set from the server as 3 columns,
**          response type, HTTP header, HTTP page body.
**          Handle up to the HTTP header here.
*/
GSTATUS
ICEGCAHeader( PICEGCASession session, char** result, u_i4 *returned )
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = E_GC0000_OK;
    GCA_IT_PARMS* itparms = &session->gca_parms.gca_it_parms;
    i4 dummy = 0;
    i4* more = &dummy;

    *more = FALSE;

    if (itparms->gca_message_type == GCA_TUPLES)
    {
        i4 ind;

        /*
        **  point at start
        */
        PTR dp = session->rcv + session->tdata;

        /*
        **  set buflen
        */
        i4  buflen = session->mlen - session->tdata;

        *returned = 0;

        /*
        **  if this is the first part of blob skip the header
        */
        if ((session->segflgs & BLOBHDR) == 0)
        {
            if (buflen >= ADP_HDR_SIZE)
            {
                dp += ADP_HDR_SIZE;
                buflen -= ADP_HDR_SIZE;
                session->segflgs |= BLOBHDR;
                session->tdata += ADP_HDR_SIZE;
            }
            else
            {
                *more = TRUE;
            }
        }

        /*
        **  if we don't have the blob segment indicator get it
        */
        if ((session->segflgs & (BLOBHDR|BLOBIND)) == BLOBHDR)
        {
            if (buflen >= sizeof(ind))
            {
                MEcopy(dp, sizeof(ind), &ind);
                dp += sizeof(ind);
                session->tdata += sizeof(ind);
                session->blobsegs += ind;
                session->segflgs |= BLOBIND;
                buflen -= sizeof(ind);
            }
            else
            {
                *more = TRUE;
            }
        }

        /*
        ** if the segment flags show that the segment length is required
        ** get segment length from the next 2 bytes.
        */
        if ((session->segflgs & (BLOBHDR|BLOBIND|BLOBSEG)) ==
            (BLOBHDR | BLOBIND))
        {
            i2 seglen;

            if (buflen >= sizeof(seglen))
            {
                MEcopy(dp, sizeof(i2), &seglen);
                session->seglen = seglen;
                dp += sizeof(seglen);
                session->tdata += sizeof(seglen);
                session->segflgs |= BLOBSEG;
                buflen -= sizeof(seglen);
            }
            else
            {
                *more = TRUE;
                session->seglen = 0;
            }
        }

        if ((session->segflgs & (BLOBHDR|BLOBIND|BLOBSEG)) ==
            (BLOBHDR | BLOBIND | BLOBSEG))
        {
            if ((session->seglen > 0) && (buflen >= session->seglen))
            {
                *returned = session->seglen;
                err = GAlloc(result, *returned, FALSE);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(dp, *returned, *result);
                    session->tdata += session->seglen;
                    dp += session->seglen;
                    session->segflgs &= ~(BLOBIND|BLOBSEG);
                    session->segflgs |= BLOBDATA;
                    buflen -= session->seglen;
                    session->seglen = 0;
                }
            }
            else
            {
                *more = TRUE;
            }
        }

        /*
        ** if the segment flags show that we have processed the segment data
        ** get terminating indicator from the next 4 bytes.
        */
        if (session->segflgs & BLOBDATA)
        {
            if (buflen >= sizeof(ind))
            {
                MEcopy(dp, sizeof(ind), &ind);
                if (ind == 0)
                {
                    dp += sizeof(ind);
                    session->tdata += sizeof(ind);
                    session->blobsegs = 0;
                    session->segflgs = 0;
                    buflen -= sizeof(ind);
                }
            }
        }

        /*
        ** If we've processed the whole buffer reset the length and the
        ** offset.
        */
        if (session->mlen == session->tdata)
        {
            session->mlen = 0;
            session->tdata = 0;
        }

        if (((buflen == 0) || (session->segflgs != 0)) &&
            (session->complete == FALSE))
        {
            err = ICEGCAReceive(session);
        }
    }
    else
         err = DDFStatusAlloc(E_GC0001_ASSOC_FAIL);
    return(err);
}

/*
** Name: ICEGCAPage
**
** Description:
**      Receives a gca message as an HTTP page.
**
** Inputs:
**      session     client session control block.
**
** Outputs:
**      result      pointer to data
**      returned    number of bytes of data returned
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
**          Add processing to handle result set from the server as 3 columns,
**          response type, HTTP header, HTTP page body.
**          Handle up to the HTTP page body here.
**      24-Nov-2000 (fanra01)
**          If we've received the last blob segment and the terminating
**          indicator is in the next message test the indicator and modify
**          the flags to prevent continuation processing.
*/
GSTATUS
ICEGCAPage( PICEGCASession session, char** result, u_i4 *returned, i4* more )
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = E_GC0000_OK;
    GCA_IT_PARMS* itparms = &session->gca_parms.gca_it_parms;
    bool    dorcv = TRUE;

    *more = FALSE;
    if (session->mlen > 0)
    {
        i4 ind;

        /*
        **  point at start
        */
        PTR dp = session->rcv + session->tdata;

        /*
        **  set buflen
        */
        i4  buflen = session->mlen - session->tdata;

        *returned = 0;

        /*
        **  if this is the first part of blob skip the header
        */
        if ((session->segflgs & BLOBHDR) == 0)
        {
            if (buflen >= ADP_HDR_SIZE)
            {
                dp += ADP_HDR_SIZE;
                buflen -= ADP_HDR_SIZE;
                session->segflgs |= BLOBHDR;
                session->tdata += ADP_HDR_SIZE;
            }
            else
            {
                *more = TRUE;
            }
        }

        /*
        **  if we don't have the blob segment indicator get it
        */
        if ((session->segflgs & (BLOBHDR|BLOBIND)) == BLOBHDR)
        {
            if (buflen >= sizeof(ind))
            {
                MEcopy(dp, sizeof(ind), &ind);
                dp += sizeof(ind);
                session->tdata += sizeof(ind);
                session->blobsegs = ind;
                /*
                ** if this is a start blob segment indicator set the flag
                ** to say we got it.  Otherwise clear the flags, we're done.
                */
                if (ind != 0)
                {
                    session->segflgs |=  BLOBIND;
                }
                else
                {
                    session->segflgs = 0;
                }
                buflen -= sizeof(ind);
            }
            else
            {
                *more = TRUE;
            }
        }

        /*
        ** if the segment flags show that the segment length is required
        ** get segment length from the next 2 bytes.
        */
        if ((session->segflgs & (BLOBHDR|BLOBIND|BLOBSEG)) ==
            (BLOBHDR | BLOBIND))
        {
            i2 seglen;
            if (buflen >= sizeof(seglen))
            {
                MEcopy(dp, sizeof(i2), &seglen);
                session->seglen = seglen;
                dp += sizeof(seglen);
                session->tdata += sizeof(seglen);
                session->segflgs |= BLOBSEG;
                buflen -= sizeof(seglen);
            }
            else
            {
                *more = TRUE;
                session->seglen = 0;
            }
        }

        if ((session->segflgs & (BLOBHDR|BLOBIND|BLOBSEG)) ==
            (BLOBHDR | BLOBIND | BLOBSEG))
        {
            if ((session->seglen > 0) && (buflen >= session->seglen))
            {
                *returned = session->seglen;
                err = GAlloc(result, *returned, FALSE);
                if (err == GSTAT_OK)
                {
                    MECOPY_VAR_MACRO(dp, *returned, *result);
                    session->tdata += session->seglen;
                    dp += session->seglen;
                    session->segflgs &= ~(BLOBIND|BLOBSEG);
                    session->segflgs |= BLOBDATA;
                    buflen -= session->seglen;
                }
            }
            else
            {
                *more = TRUE;
            }
        }

        /*
        ** if the segment flags show that we have processed the segment data
        ** get terminating indicator from the next 4 bytes.
        */
        if (session->segflgs & BLOBDATA)
        {
            if (buflen >= sizeof(ind))
            {
                MEcopy(dp, sizeof(ind), &ind);
                if (ind == 0)
                {
                    dp += sizeof(ind);
                    session->tdata += sizeof(ind);
                    buflen -= sizeof(ind);
                    session->blobsegs = 0;
                    session->segflgs = 0;
                }
                else if (ind == 1)
                {
                    i2 seglen;

                    /*
                    ** see if the whole of the next message is in the buffer
                    */
                    if (buflen >= (sizeof(ind) + sizeof(seglen)))
                    {
                        MEcopy(dp + sizeof(ind), sizeof(i2), &seglen);
                        if (buflen >= seglen)
                        {
                            dorcv = FALSE;
                        }
                    }
                }
            }
        }

        /*
        ** If we've processed the whole buffer reset the length and the
        ** offset.
        */
        if (buflen == 0)
        {
            session->mlen = 0;
            session->tdata = 0;
        }

        /*
        **  if buflen > 0
        **      flen = buflen
        **      copy (flen bytes from start point to buffer head)
        **      tdata = 0
        */
        if (buflen > 0)
        {
            MEcopy(dp, buflen, session->rcv);
            /*
            ** If we don't do a receive the mlen will reflect remaining
            ** message length otherwise the new message will be appended
            ** to the fragement.
            */
            if (dorcv == FALSE)
            {
                session->mlen = buflen;
            }
            else
            {
                session->flen = buflen;
            }
            session->tdata = 0;
            buflen = 0;
        }
        if (((buflen == 0) || (session->segflgs != 0)) &&
            (session->complete == FALSE) &&
            (dorcv == TRUE))
        {
            err = ICEGCAReceive(session);
        }
    }
    else
        *returned = 0;

    return(err);
}

/*
** Name: ICEGCADisconnect
**
** Description:
**      Terminates a connection with the ice server.
**
** Inputs:
**      session     client session control block.
**
** Outputs:
**      None.
**
** Returns:
**      GSTAT_OK    success
**      !0          failure
**
** History:
**      08-Mar-99 (fanra01)
**          Add History.
*/
GSTATUS
ICEGCADisconnect( PICEGCASession session )
{
    GSTATUS err = GSTAT_OK;
    STATUS  status = E_GC0000_OK;

    MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
    session->gca_parms.gca_fo_parms.gca_buffer = session->buf;
    session->gca_parms.gca_fo_parms.gca_b_length = sizeof(session->buf);
    IIGCa_cb_call(&cb, GCA_FORMAT, &session->gca_parms, GCA_SYNC_FLAG, 0,
        timeout, &status);

    if (status == E_GC0000_OK)
    {
        GCA_ER_DATA *err_data = (GCA_ER_DATA *)
            session->gca_parms.gca_fo_parms.gca_data_area;

        err_data->gca_l_e_element = 0;
        MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
        session->gca_parms.gca_sd_parms.gca_buffer = session->buf;
        session->gca_parms.gca_sd_parms.gca_msg_length = sizeof(GCA_ER_DATA);
        session->gca_parms.gca_sd_parms.gca_message_type = GCA_RELEASE;
        session->gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
        session->gca_parms.gca_sd_parms.gca_association_id = session->aid;
        IIGCa_cb_call(&cb, GCA_SEND, &session->gca_parms, GCA_SYNC_FLAG, 0,
            timeout, &status);
        if (status == E_GC0000_OK)
        {
            GCA_DA_PARMS    *disassoc;
            MEfill(sizeof(GCA_PARMLIST), EOS, &session->gca_parms);
            disassoc = &session->gca_parms.gca_da_parms;
            disassoc->gca_association_id = session->aid;
            disassoc->gca_status = E_GC0000_OK;
            IIGCa_cb_call(&cb, GCA_DISASSOC, &session->gca_parms,
                GCA_SYNC_FLAG, 0, (i4) timeout, &status);
        }
    }
    if (status != E_GC0000_OK)
        err = DDFStatusAlloc(status);
    return(err);
}
