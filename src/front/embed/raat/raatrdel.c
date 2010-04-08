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
# include	<erug.h>
/*
** Name: IIraat_record_delete - delete a record using the RAAT API
**
** Description:
**	Formulate GCA message for RAAT record del command and send it to
**	the DBMS, then wait for reply and return status of operation.
**
** Inputs:
**	raat_cb		RAAT control block
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	16-apr-95 (stephenb)
**	    First written.
**	8-may-95 (lewda02/thaju02)
**	    Extract from api.qc.
**	    Streamline GCA usage.
**	    Change naming convention.
**	14-jul-95 (emmag)
**	    gca_parm structure shouldn't be passed to IIGCa_call.
**	14-sep-95 (shust01/thaju02)
**	    Use RAAT internal_buf instead of GCA buffer(IIlbqcb ...), since
**	    we could be passing more than 1 GCA buffers worth.
**	10-nov-95 (thaju02)
**	    Added check to test for table open and check of secondary
**	    index table.
**	08-jul-96 (toumi01)
**	    Modified to support axp.osf (64-bit Digital Unix).
**	    Most of these changes are to handle that fact that on this
**	    platform sizeof(PTR)=8, sizeof(long)=8, sizeof(i4)=4.
**	    MEcopy is used to avoid bus errors moving unaligned data.
**	16-jul-1996 (sweeney)
**	    Add tracing.
**      11-feb-1997 (cohmi01)
**          Handle errors, give back RAAT return codes. (b80665)
**	08-sep-1997 (somsa01)
**	    In case of "rollback" conditions, mark that the the table is closed.
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
IIraat_record_delete (RAAT_CB	*raat_cb)
{
    GCA_PARMLIST	gca_parm;
    GCA_SD_PARMS	*gca_snd;
    GCA_RV_PARMS	*gca_recv;
    GCA_IT_PARMS	*gca_int;
    GCA_Q_DATA		*query_data;
    i4			msg_size;
    STATUS		gca_stat;
    char		*buf_ptr;
    i4                  space_needed;
    IICGC_MSG           *dataptr;
    STATUS		status = OK;

    /* log any tracing information */
    IIraat_trace(RAAT_RECORD_DEL, raat_cb);

    /*
    ** Test for valid open table before we proceed.
    */
    if (raat_cb->table_handle == NULL ||
        !(raat_cb->table_handle->table_status & TABLE_OPEN))
    {
        raat_cb->err_code = E_UG00D0_RaatTblNotOpen;
        return (FAIL);
    }
 
    /*
    ** Test if table is a secondary index.
    */
    if (raat_cb->table_handle->table_info.tbl_id.db_tab_index)
    {
        raat_cb->err_code = E_UG00D1_RaatInvOperSecIdx;
        return(FAIL);
    }

    /*
    ** Fill out gca data area, for record put the format is:
    **
    ** i4		op_code
    ** i4		table_handle->table_rcb
    ** i4		flag
    ** i4		recnum (optional)
    **
    */
    space_needed = 6 * sizeof(i4);
	      
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
    *((i4 *)query_data->gca_qdata) = RAAT_RECORD_DEL;
    msg_size = sizeof(i4);

    /*
    ** copy data value (table rcb) to gca buffer and increment 
    ** message size
    */
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    MEcopy(&raat_cb->table_handle->table_rcb, sizeof(PTR), buf_ptr + msg_size);
    msg_size += sizeof(PTR);
#else
    *((i4 *)(buf_ptr + msg_size)) = raat_cb->table_handle->table_rcb;
    msg_size += sizeof(i4);
#endif

    /*
    ** copy data value (flag) to gca buffer and increment 
    ** message size
    */
    *((i4 *)(buf_ptr + msg_size)) = raat_cb->flag;
    msg_size += sizeof(i4);

    if (raat_cb->flag & RAAT_REC_BYNUM)
    {
    	/*
    	** copy data value (flag) to gca buffer and increment 
    	** message size
    	*/
    	*((i4 *)(buf_ptr + msg_size)) = raat_cb->recnum;
    	msg_size += sizeof(i4);
    }

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
    {
	/* Mark the table closed based upon "rollback" conditions */
	if ( (raat_cb->err_code == 196674)	/* deadlock */
	  || (raat_cb->err_code == 4705)	/* maxlocks reached */
	  || (raat_cb->err_code == 4706) )	/* force abort */
	    raat_cb->table_handle->table_status &= ~(TABLE_OPEN);
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */
    }

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
    {
	/* Mark the table closed based upon "rollback" conditions */
	if ( (raat_cb->err_code == 196674)	/* deadlock */
	  || (raat_cb->err_code == 4705)	/* maxlocks reached */
	  || (raat_cb->err_code == 4706) )	/* force abort */
	    raat_cb->table_handle->table_status &= ~(TABLE_OPEN);
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */
    }

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
    {
	/* Mark the table closed based upon "rollback" conditions */
	if ( (raat_cb->err_code == 196674)	/* deadlock */
	  || (raat_cb->err_code == 4705)	/* maxlocks reached */
	  || (raat_cb->err_code == 4706) )	/* force abort */
	    raat_cb->table_handle->table_status &= ~(TABLE_OPEN);
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */
    }
	     
    if (CHECK_RAAT_GCA_RESPONSE(raat_cb, gca_int))
	return (FAIL);	/* err_code in RAAT_CB has been set by macro */

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    raat_cb->err_code = *((i4 *)gca_int->gca_data_area);
#else
    raat_cb->err_code = *((long *)gca_int->gca_data_area);
#endif
    if (raat_cb->err_code)
    {
	/* Mark the table closed based upon "rollback" conditions */
	if ( (raat_cb->err_code == 196674)	/* deadlock */
	  || (raat_cb->err_code == 4705)	/* maxlocks reached */
	  || (raat_cb->err_code == 4706) )	/* force abort */
	    raat_cb->table_handle->table_status &= ~(TABLE_OPEN);
        return (FAIL);
    }

    return (OK);
}
