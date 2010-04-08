/* 
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <cv.h>          /* 6-x_PC_80x86 */
# include       <er.h>
# include       <me.h>
# include       <cm.h>
# include       <st.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <gca.h>
# include       <adf.h>
# include       <adp.h>
# include       <iicgca.h>
# include       <erlc.h>
# include       <erlq.h>
# include       <erug.h>
# include       <generr.h>
# include       <sqlstate.h>
#define CS_SID      SCALARP
# include       <raat.h>
# include       <raatint.h>
# include	<iisqlca.h>
# include       <iilibq.h>
/*
** Name: IIcraat_blob_get - get a long varchar/long byte tuple
**
** Description:
**	Formulate GCA message for RAAT record put command and send it to
**	the DBMS, then wait for reply and return status of operation.
**
** Inputs:
**	*raat_cb	RAAT_CB
**	raat_blob_cb	RAAT BLOB control block
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	14-sep-95 (shust01/thaju02)
**	    created.
**	08-jul-96 (toumi01)
**	    Modified to support axp.osf (64-bit Digital Unix).
**	    Most of these changes are to handle that fact that on this
**	    platform sizeof(PTR)=8, sizeof(long)=8, sizeof(i4)=4.
**	16-jul-1996 (sweeney)
**	    Add tracing.
**      11-feb-1997 (cohmi01)
**          Handle errors, give back RAAT return codes. (b80665)
**	18-Dec-97 (gordy)
**	    Libq session data now accessed via function call.
**	    Converted to GCA control block interface.
**      24-Mar-99 (hweho01)
**          Extended the changes dated 08-jul-96 by toumi01 for
**          axp_osf to ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Sep-2000 (hanje04)
**          Extended the changes dated 08-jul-96 by toumi01 for
**          axp_osf to axp_lnx (Alpha Linux).
**	13-oct-2001 (somsa01)
**	    Porting changes for NT_IA64.
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
*/
STATUS
IIcraat_blob_get (RAAT_CB *raat_cb, RAAT_BLOB_CB *raat_blob_cb)
{
    GCA_PARMLIST	gca_parm;
    GCA_SD_PARMS	*gca_snd;
    GCA_RV_PARMS	*gca_recv;
    GCA_IT_PARMS	*gca_int;
    GCA_Q_DATA		*query_data;
    i4			msg_size;
    STATUS		gca_stat;
    i4			pad, status;
    char		*buf_ptr;
    i4                  space_needed;
    IICGC_MSG           *dataptr;

    /* log any trace information */
    IIraat_trace(RAAT_BLOB_GET, raat_cb);

    /*
    ** Fill out gca data area, for record put the format is:
    **
    ** i4		op_code
    ** i4		table_handle->table_rcb
    ** char		*record
    **
    */
    pad = (raat_blob_cb->continuation) ? 6 : 18;

    /* enough space for sending and receiving data */
    space_needed = 6 * sizeof(i4) + sizeof(ADP_PERIPHERAL) + DB_MAXTUP +
	 sizeof(ADP_LO_WKSP) + raat_blob_cb->data_len + pad;

    /* make sure internal buffer is big enough to handle data */
    if  (raat_cb->internal_buf_size < space_needed)
    {
        status = allocate_big_buffer(raat_cb, space_needed);
        if (status != OK)
	{
	    raat_cb->err_code = S_UG0036_NoMemory;
	    return(FAIL);
	}
    }
	 
    dataptr = (IICGC_MSG *)raat_cb->internal_buf;
    query_data = (GCA_Q_DATA *)dataptr->cgc_data;
    query_data->gca_language_id = DB_NDMF;
    query_data->gca_query_modifier = 0;
    /*
    ** copy value to gca data buffer and incriment message size
    */
    buf_ptr = (char *)query_data->gca_qdata;
    *((i4 *)buf_ptr) = RAAT_BLOB_GET;
    msg_size = sizeof(i4);

    *((i4 *)(buf_ptr + msg_size)) = raat_blob_cb->continuation;
    msg_size += sizeof(i4);

    *((i4 *)(buf_ptr + msg_size)) = abs(raat_blob_cb->datatype);
    msg_size += sizeof(i4);
    
    /*
    ** copy data value (record + length) to gca buffer and 
    ** increment msg size
    */
    MEcopy(raat_blob_cb->coupon, sizeof(ADP_PERIPHERAL), buf_ptr + msg_size);
    msg_size += sizeof(ADP_PERIPHERAL);
    
    *((i4 *)(buf_ptr + msg_size)) = raat_blob_cb->data_len + pad;
    msg_size += sizeof(i4);

    MEcopy(raat_blob_cb->work_area, DB_MAXTUP + sizeof(ADP_LO_WKSP), buf_ptr + msg_size);
    msg_size += DB_MAXTUP + sizeof(ADP_LO_WKSP);

    /*
    ** send info to the DBMS
    */

    gca_snd = &gca_parm.gca_sd_parms;

    gca_snd->gca_association_id	= IILQaiAssocID();
    gca_snd->gca_message_type = GCA_QUERY;
    gca_snd->gca_buffer	= dataptr->cgc_buffer;
    gca_snd->gca_msg_length = msg_size + sizeof(query_data->gca_language_id) +
				  sizeof(query_data->gca_query_modifier);
    gca_snd->gca_end_of_data = TRUE;
    gca_snd->gca_descriptor = 0;
    gca_snd->gca_status	= E_GC0000_OK;

    IIGCa_cb_call( &IIgca_cb, GCA_SEND, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (CHECK_RAAT_GCA_RET(raat_cb, gca_stat, gca_snd))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */

    /*
    ** Wait for reply
    */
    gca_recv = &gca_parm.gca_rv_parms;

    gca_recv->gca_association_id = IILQaiAssocID();
    gca_recv->gca_flow_type_indicator = GCA_NORMAL;
    gca_recv->gca_buffer = dataptr->cgc_buffer;
    gca_recv->gca_b_length = dataptr->cgc_d_length;
    gca_recv->gca_descriptor = NULL;
    gca_recv->gca_status = E_GC0000_OK;

    IIGCa_cb_call( &IIgca_cb, GCA_RECEIVE, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (CHECK_RAAT_GCA_RET(raat_cb, gca_stat, gca_recv))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */

    /*
    ** Interpret results
    */
    gca_int = &gca_parm.gca_it_parms;

    gca_int->gca_buffer		= dataptr->cgc_buffer;
    gca_int->gca_message_type	= 0;
    gca_int->gca_data_area	= (char *)0;          /* Output */
    gca_int->gca_d_length	= 0;                  /* Output */
    gca_int->gca_end_of_data	= 0;                  /* Output */
    gca_int->gca_status		= E_GC0000_OK;        /* Output */

    IIGCa_cb_call( &IIgca_cb, GCA_INTERPRET, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (CHECK_RAAT_GCA_RET(raat_cb, gca_stat, gca_int))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */
	     
    if (CHECK_RAAT_GCA_RESPONSE(raat_cb, gca_int))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */

    buf_ptr = (char *)gca_int->gca_data_area;

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    raat_cb->err_code = *((i4 *)buf_ptr);
#else
    raat_cb->err_code = *((long *)buf_ptr);
#endif
    if (raat_cb->err_code)
        return (FAIL);
    buf_ptr += sizeof(i4);

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    status = *((i4 *)buf_ptr);
#else
    status = *((long *)buf_ptr);
#endif
    if (status == 0)  /* end of data.  Otherwise, would be E_DB_INFO (2) */
       raat_blob_cb->end_of_data = 1;
    buf_ptr += sizeof(i4);

    raat_blob_cb->actual_len = *((i4 *)buf_ptr);
    buf_ptr += sizeof(i4);

    MEcopy(buf_ptr, DB_MAXTUP + sizeof(ADP_LO_WKSP), raat_blob_cb->work_area);
    buf_ptr += DB_MAXTUP + sizeof(ADP_LO_WKSP);

    buf_ptr += pad;   /* skip the data header */
    MEcopy(buf_ptr, raat_blob_cb->actual_len, raat_blob_cb->data);

    return(OK);
}
/*
** Name: IIcraat_blob_put - put a long varchar/long byte tuple
**
** Description:
**	Formulate GCA message for RAAT record put command and send it to
**	the DBMS, then wait for reply and return status of operation.
**
** Inputs:
**	raat_blob_cb	RAAT BLOB control block
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	14-sep-95 (shust01/thaju02)
**	    First written.
**	16-jul-1996 (sweeney)
**	    Add tracing.
*/
STATUS
IIcraat_blob_put (RAAT_CB *raat_cb, RAAT_BLOB_CB *raat_blob_cb)
{
    GCA_PARMLIST	gca_parm;
    GCA_SD_PARMS	*gca_snd;
    GCA_RV_PARMS	*gca_recv;
    GCA_IT_PARMS	*gca_int;
    GCA_Q_DATA		*query_data;
    i4			msg_size;
    STATUS		gca_stat;
    i4			status;
    char		*buf_ptr;
    i4                  space_needed;
    IICGC_MSG           *dataptr;

    /* log any trace information */
    IIraat_trace(RAAT_BLOB_PUT, raat_cb);

    /*
    ** Fill out gca data area, for record put the format is:
    **
    ** i4		op_code
    ** i4		table_handle->table_rcb
    ** char		*record
    **
    */
    /*
    ** we have to send an extra 4 bytes (i4) along to fool the code
    ** so the back end will make sure that there is enough space for the
    ** end-of-data indicator (if we need one)
    */
    space_needed = 8 * sizeof(i4) + sizeof(ADP_PERIPHERAL) + DB_MAXTUP +
	 sizeof(ADP_LO_WKSP) + raat_blob_cb->data_len + sizeof(short);

    /* make sure internal buffer is big enough to handle data */
    if  (raat_cb->internal_buf_size < space_needed)
    {
        status = allocate_big_buffer(raat_cb, space_needed);
        if (status != OK)
	{
	    raat_cb->err_code = S_UG0036_NoMemory;		
	    return(FAIL);
	}
    }
	 
    dataptr = (IICGC_MSG *)raat_cb->internal_buf;
    query_data = (GCA_Q_DATA *)dataptr->cgc_data;
    query_data->gca_language_id = DB_NDMF;
    query_data->gca_query_modifier = 0;
    /*
    ** copy value to gca data buffer and incriment message size
    */
    buf_ptr = (char *)query_data->gca_qdata;
    *((i4 *)buf_ptr) = RAAT_BLOB_PUT;
    msg_size = sizeof(i4);

    *((i4 *)(buf_ptr + msg_size)) = raat_blob_cb->continuation;
    msg_size += sizeof(i4);

    *((i4 *)(buf_ptr + msg_size)) = raat_blob_cb->end_of_data;
    msg_size += sizeof(i4);

    *((i4 *)(buf_ptr + msg_size)) = raat_blob_cb->data_len;
    msg_size += sizeof(i4);

    *((i4 *)(buf_ptr + msg_size)) = abs(raat_blob_cb->datatype);
    msg_size += sizeof(i4);

    MEcopy(raat_blob_cb->coupon, sizeof(ADP_PERIPHERAL), buf_ptr + msg_size);
    msg_size += sizeof(ADP_PERIPHERAL);
    
    MEcopy(raat_blob_cb->work_area, DB_MAXTUP + sizeof(ADP_LO_WKSP), 
	buf_ptr + msg_size);
    msg_size += DB_MAXTUP + sizeof(ADP_LO_WKSP);

    /* 
       Even though we sent the length up above, send it again. This time
       as a short, since we are going to use the buffer on the back end,
       and adu_couponify() expects the size to precede the data.
    */
    *((short *)(buf_ptr + msg_size)) = raat_blob_cb->data_len;
    msg_size += sizeof(short);
    MEcopy(raat_blob_cb->data, raat_blob_cb->data_len, buf_ptr + msg_size);
    /*
    ** we have to send an extra 4 bytes (i4) along to fool the code
    ** so the back end will make sure that there is enough space for the
    ** end-of-data indicator (if we need one)
    */
    msg_size += raat_blob_cb->data_len + sizeof(i4);

    /*
    ** send info to the DBMS
    */

    gca_snd = &gca_parm.gca_sd_parms;

    gca_snd->gca_association_id	= IILQaiAssocID();
    gca_snd->gca_message_type = GCA_QUERY;
    gca_snd->gca_buffer	= dataptr->cgc_buffer;
    gca_snd->gca_msg_length = msg_size + sizeof(query_data->gca_language_id) +
				  sizeof(query_data->gca_query_modifier);
    gca_snd->gca_end_of_data = TRUE;
    gca_snd->gca_descriptor = 0;
    gca_snd->gca_status	= E_GC0000_OK;

    IIGCa_cb_call( &IIgca_cb, GCA_SEND, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (CHECK_RAAT_GCA_RET(raat_cb, gca_stat, gca_snd))
	return (FAIL);

    /*
    ** Wait for reply
    */
    gca_recv = &gca_parm.gca_rv_parms;

    gca_recv->gca_association_id = IILQaiAssocID();
    gca_recv->gca_flow_type_indicator = GCA_NORMAL;
    gca_recv->gca_buffer = dataptr->cgc_buffer;
    gca_recv->gca_b_length = dataptr->cgc_d_length;
    gca_recv->gca_descriptor = NULL;
    gca_recv->gca_status = E_GC0000_OK;

    IIGCa_cb_call( &IIgca_cb, GCA_RECEIVE, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (CHECK_RAAT_GCA_RET(raat_cb, gca_stat, gca_recv))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */

    /*
    ** Interpret results
    */
    gca_int = &gca_parm.gca_it_parms;

    gca_int->gca_buffer		= dataptr->cgc_buffer;
    gca_int->gca_message_type	= 0;
    gca_int->gca_data_area	= (char *)0;          /* Output */
    gca_int->gca_d_length	= 0;                  /* Output */
    gca_int->gca_end_of_data	= 0;                  /* Output */
    gca_int->gca_status		= E_GC0000_OK;        /* Output */

    IIGCa_cb_call( &IIgca_cb, GCA_INTERPRET, &gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (CHECK_RAAT_GCA_RET(raat_cb, gca_stat, gca_int))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */
	     
    if (CHECK_RAAT_GCA_RESPONSE(raat_cb, gca_int))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */

    buf_ptr = (char *)gca_int->gca_data_area;

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    raat_cb->err_code = *((i4 *)buf_ptr);
#else
    raat_cb->err_code = *((long *)buf_ptr);
#endif
    if (raat_cb->err_code)
        return (FAIL);
    buf_ptr += sizeof(i4);

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    status = *((i4 *)buf_ptr);
#else
    status = *((long *)buf_ptr);
#endif
    buf_ptr += sizeof(i4);

    MEcopy(buf_ptr, sizeof(ADP_PERIPHERAL), raat_blob_cb->coupon);
    buf_ptr += sizeof(ADP_PERIPHERAL);

    MEcopy(buf_ptr, DB_MAXTUP + sizeof(ADP_LO_WKSP), raat_blob_cb->work_area);
    buf_ptr += DB_MAXTUP + sizeof(ADP_LO_WKSP);

    return (OK);
}
