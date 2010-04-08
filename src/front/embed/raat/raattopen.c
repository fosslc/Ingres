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
# include       <iicgca.h>
# include       <erlc.h>
# include       <erlq.h>
# include       <generr.h>
# include       <sqlstate.h>
#define CS_SID      SCALARP
# include       <raat.h>
# include       <raatint.h>
# include	<iisqlca.h>
# include       <iilibq.h>
# include       <erug.h>
/*
** Name: IIraat_table_open -  Open a table using the RATT API
**
** Description:
**	Formulate GCA message for RAAT table open command and send it to
**	the DBMS, then wait for reply and return status of operation.
**
** Inputs:
**	raat_cb		RAAT control block
**
** Outputs:
**	raat_cb.table_handle	pointer for future table reference
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	3-apr-95 (stephenb)
**	    First written.
**	13-apr-95 (lewda02)
**	    More work to return results via GCA.  So far only return RCB ptr.
**	19-apr-95 (lewda02)
**	    Get full table info from GCA buffers.  Allocate the table_handle
**	    structure and store the results.
**      8-may-95 (lewda02/thaju02)
**          Extract from api.qc.
**          Streamline GCA usage.
**          Change naming convention.
**	14-may-95 (lewda02)
**	    Add support for table open (lock) flag values.
**	01-jun-95 (lewda02)
**	    Add code to handle multi-buffer GCA responses.
**	 7-jun-95 (shust01)
**	    set flag if table can have prefetching done on it.
**	14-jul-95 (emmag)
**	    Cleaned up parameters passed to IIGCa_call.
**      14-sep-95 (shust01/thaju02)
**          Use RAAT internal_buf instead of GCA buffer(IIlbqcb ...), since
**          we could be passing more than 1 GCA buffers worth.
**	08-jul-96 (toumi01)
**	    Modified to support axp.osf (64-bit Digital Unix).
**	    Most of these changes are to handle that fact that on this
**	    platform sizeof(PTR)=8, sizeof(long)=8, sizeof(i4)=4.
**	    MEcopy is used to avoid bus errors moving unaligned data.
**	16-jul-1996 (sweeney)
**	    Add tracing.
**	16-oct-1996 (cohmi01)
**	    Turn on bit to indicate availability of RAAT_BAS_RECDATA support,
**	    if server says its ok, so newer clients dont try to use it
**	    against an old server, as the message formats are different.
**      11-feb-1997 (cohmi01)
**          Handle errors, give back RAAT return codes. (b80665)
**      01-dec-1997 (stial01)
**          Turn on bit to indicate if btree key should be sent to server
**          for RAAT_INTERNAL_REPOS requests.
**	18-Dec-97 (gordy)
**	    Libq session data now accessed via function call.
**	    Converted to GCA control block interface.
**      24-Mar-99 (hweho01)
**          Extended the changes dated 08-jul-96 by toumi01 for axp_osf
**          to ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Sep-2000 (hanje04)
**          Extended the changes dated 08-jul-96 by toumi01 for axp_osf
**          to axp_lnx (Alpha Linux).
**	13-oct-2001 (somsa01)
**	    Porting changes for NT_IA64.
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
*/
STATUS
IIraat_table_open (RAAT_CB	*raat_cb)
{
    GCA_PARMLIST	gca_parm;
    GCA_SD_PARMS	*gca_snd;
    GCA_RV_PARMS	*gca_recv;
    GCA_IT_PARMS	*gca_int;
    GCA_Q_DATA		*query_data;
    i4			msg_size;
    STATUS		gca_stat;
    STATUS		status;
    char		*buf_ptr;
    char		*block;
    RAAT_TABLE_HANDLE	*th;
    i4                  access_mode;
    i4			bytes;
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    OFFSET_TYPE         rcb;
#else
    i4		rcb;
#endif
    i4			attr_count;
    i4			index_count;
    i4			space_needed;
    IICGC_MSG		*dataptr;

    /* log any trace information */
    IIraat_trace(RAAT_TABLE_OPEN, raat_cb);

    /*
    ** Fill out gca data area, for table open the format is:
    **
    ** i4		op_code
    ** i4		flag
    ** char		*tablename
    */
    space_needed = 4 * sizeof(i4) + (i4)STlength(raat_cb->tabname) + 1; 

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
    ** copy value to gca data buffer and increment message size
    */
    buf_ptr = (char *)query_data->gca_qdata;
    *((i4 *)buf_ptr) = RAAT_TABLE_OPEN;
    msg_size = sizeof(i4);

    /*
    ** copy data value (flag) to gca buffer and increment msg size
    */
    *((i4 *)(buf_ptr + msg_size)) = raat_cb->flag;
    msg_size += sizeof(i4);

    /*
    ** copy data value (table name) to gca buffer and increment msg size
    */
    MEcopy(raat_cb->tabname, STlength(raat_cb->tabname) + 1, 
	(char *)query_data->gca_qdata + msg_size);
    msg_size += (i4)STlength(raat_cb->tabname) + 1;

    /*
    ** send info to the DBMS
    */

    gca_snd = &gca_parm.gca_sd_parms;

    gca_snd->gca_association_id	= IILQaiAssocID();
    gca_snd->gca_message_type	= GCA_QUERY;
    gca_snd->gca_buffer		= dataptr->cgc_buffer;
    gca_snd->gca_msg_length	= msg_size + 
				  sizeof(query_data->gca_language_id) +
				  sizeof(query_data->gca_query_modifier);
    gca_snd->gca_end_of_data	= TRUE;
    gca_snd->gca_descriptor	= 0;
    gca_snd->gca_status		= E_GC0000_OK;

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
    raat_cb->err_code = *((i4*)buf_ptr);
#else
    raat_cb->err_code = *((long *)buf_ptr);
#endif

    if (raat_cb->err_code)
	return (FAIL);
    buf_ptr += sizeof(i4);

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    MEcopy(buf_ptr, sizeof(PTR), &rcb);
#else
    rcb = *((long *)buf_ptr);
#endif
    buf_ptr += sizeof(PTR);

    /* get table access mode */
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    access_mode = *((i4 *)buf_ptr);
#else
    access_mode = *((long *)buf_ptr);
#endif
    buf_ptr += sizeof(i4);

    /*
    ** Get full table info.  Memory is allocated for the "table_handle"
    ** structure and for any attribute and index information returned.
    ** This memory will be freed when the table is closed.
    */
    attr_count = ((RAAT_TBL_ENTRY *)buf_ptr)->tbl_attr_count;
    index_count = ((RAAT_TBL_ENTRY *)buf_ptr)->tbl_index_count;

    block = MEreqmem(0, sizeof(RAAT_TABLE_HANDLE) +
	sizeof(RAAT_ATT_ENTRY) * attr_count +
	sizeof(RAAT_IDX_ENTRY) * index_count, TRUE, &status);
    if (status)
    {
	raat_cb->err_code = E_UG00D2_RaatTblHndlAlloc;
	return (FAIL);
    }

    th = raat_cb->table_handle = (RAAT_TABLE_HANDLE *)block;

    MEcopy(buf_ptr, sizeof(RAAT_TBL_ENTRY), &th->table_info);
    buf_ptr += sizeof(RAAT_TBL_ENTRY);
    block += sizeof(RAAT_TABLE_HANDLE);

    th->table_att = (RAAT_ATT_ENTRY *)block;

    if (th->table_info.tbl_index_count)
	th->table_idx = (RAAT_IDX_ENTRY *)
	    (block + attr_count * sizeof(RAAT_ATT_ENTRY));

    /* else field is already null from allocation */

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    bytes = gca_int->gca_d_length - (2 * sizeof(i4) + sizeof(PTR) +
#else
    bytes = gca_int->gca_d_length - (3 * sizeof(i4) +
#endif
	sizeof(RAAT_TBL_ENTRY));

    MEcopy(buf_ptr, bytes, block);
    block += bytes; 

    /*
    ** Now we test for multi-buffer response.  If the table info does not
    ** fit in one GCA buffer, then the end of data flag will not be
    ** set.  We will keep receiving the data until it is complete.
    */

    while (!gca_int->gca_end_of_data)
    {
	/*
	** Receive next buffer.
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
 
	gca_int->gca_buffer     = dataptr->cgc_buffer;
	gca_int->gca_message_type   = 0;
	gca_int->gca_data_area      = (char *)0;          /* Output */
	gca_int->gca_d_length       = 0;                  /* Output */
	gca_int->gca_end_of_data    = 0;                  /* Output */
	gca_int->gca_status         = E_GC0000_OK;        /* Output */
 
	IIGCa_cb_call( &IIgca_cb, GCA_INTERPRET, &gca_parm, 
		       GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    	if (CHECK_RAAT_GCA_RET(raat_cb, gca_stat, gca_int))
	    return (FAIL);	/* err_code in RAAT_CB has been set by macro */

	if (CHECK_RAAT_GCA_RESPONSE(raat_cb, gca_int))
	    return (FAIL);	/* err_code in RAAT_CB has been set by macro */

	MEcopy(gca_int->gca_data_area, gca_int->gca_d_length, block);
	block += gca_int->gca_d_length;
    }
    th->table_rcb = rcb;
	
    /*
    ** Move table name into structure for identification purposes.
    */
    MEcopy(raat_cb->tabname, STlength(raat_cb->tabname) + 1, &th->table_name);

    /*
    ** Set status to indicate open table.
    */
    /* if access_mode is TRUE, then it has a read_lock on it, which means
       prefetches can be done */
    th->table_status = TABLE_OPEN | ((access_mode) ? TABLE_PREFETCH : 0); 
    /* initialize prefetch flags */
    th->fetch_buffer = NULL;
    th->fetch_size   = 0;

    /* If server tells us it supports BAS_RECDATA, pass along to caller */
    if (th->table_info.tbl_qryid.db_qry_low_time >= RAAT_VERSION_2)
    {
	/* Inform caller of additional server capabilities available */
	th->table_status |= TABLE_BAS_RECDATA; /* available in version 2 */
	if (th->table_info.tbl_qryid.db_qry_low_time >= RAAT_VERSION_3)
	    th->table_status |= TABLE_SEND_BKEY; 
	/* reset overloaded qryid fld - always 0 for base tables */
	th->table_info.tbl_qryid.db_qry_low_time = 0; 
    }
    return (OK);
}
