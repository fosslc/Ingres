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
** Name: IIraat_record_get - get a record using the RAAT API
**
** Description:
**	Formulate GCA message for RAAT record get command and send it to
**	the DBMS, then wait for reply and return status of operation.
**
** Inputs:
**	raat_cb		RAAT control block
**
** Outputs:
**	raat_cb.record	Pointer to the record retrieved
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	12-apr-95 (stephenb)
**	    First written.
**      8-may-95 (lewda02/thaju02)
**          Extract from api.qc.
**          Streamline GCA usage.
**          Change naming convention.
**	22-may-95 (lewda02/shust01)
**	    Fixed bus error by using MEcopy of tidp.
**      7-jun-95 (shust01)
**          added logic to allow prefetching data from the database.  This
**          will eliminate GCA traffic, since we are only calling server once
**          and retrieving many rows.  This will only be done for tables that
**          has a read-lock on it.
**     	12-jun-95 (shust01)
**          fixed bug in setting recnum . Also fixed bug with prefetch when 
**	    only finding one record.
**	14-jul-95 (emmag)
**	    gca_parm structure shouldn't be passed to IIGCa_call()
**      14-sep-95 (shust01/thaju02)
**          Use RAAT internal_buf instead of GCA buffer(IIlbqcb ...), since
**          we could be passing more than 1 GCA buffers worth.
**      10-nov-95 (thaju02)
**          Added check to test for table open.
**	08-jul-96 (toumi01)
**	    Modified to support axp.osf (64-bit Digital Unix).
**	    Most of these changes are to handle that fact that on this
**	    platform sizeof(PTR)=8, sizeof(long)=8, sizeof(i4)=4.
**	    MEcopy is used to avoid bus errors moving unaligned data.
**	16-jul-1996 (sweeney)
**	    Add tracing.
**	16-oct-1996 (cohmi01)
**	    Resolve secondary index scan performance problem (#78981)
**	    with addition of RAAT_BAS_RECDATA flag to request that base
**	    table data be returned upon a get next/prev on secondary.
**	    Also clean up and comment, update GCA msg layout description. 
**      11-feb-1997 (cohmi01)
**          Handle errors, give back RAAT return codes. (b80665)
**	08-sep-1997 (somsa01)
**	    In case of "rollback" conditions, mark that the the table is closed.
**      01-dec-1997 (stial01)
**          Send btree key for RAAT_INTERNAL_REPOS
**      18-jun-1999 (shust01)
**          Added special case to send_bkey() to properly build record when
**          dealing with secondary index and RAAT_BAS_RECDATA flag.
**          Bug #97496.
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
IIraat_record_get (RAAT_CB	*raat_cb)
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
    i4                  dir;
    i4                  repos;
    i4             rcb_lowtid = 0;
    i4             rcb_currenttid = 0;
    i4             fetch_size = 0;
    char                tried_mult;
    i4                  space_needed;
    IICGC_MSG           *dataptr;
    i4		outrec_width;
    char                *tup_ptr = (char *)0;
    bool                reposition = FALSE;
    i4             row_request;

    /* log any tracing information */
    IIraat_trace(RAAT_RECORD_GET, raat_cb);

    /*
    ** Test for valid open table before we proceed.
    */
    if (raat_cb->table_handle == NULL ||
        !(raat_cb->table_handle->table_status & TABLE_OPEN))
    {
        raat_cb->err_code = E_UG00D0_RaatTblNotOpen;
        return (FAIL);
    }
 
    /* if this table is not a B-TREE, and user requesting PREV, return error */
    if ((raat_cb->flag & RAAT_REC_PREV) &&
	(raat_cb->table_handle->table_info.tbl_storage_type != RAAT_BTREE_TYPE))
    {
       raat_cb->err_code = 196718; /* E_DM006E_NON_BTREE_GETPREV */
       return (FAIL);
    }

    /*
    ** If caller is requesting that base table data be returned upon
    ** access via secondary index, validate options and determine data width.
    */
    if (raat_cb->flag & RAAT_BAS_RECDATA)
    {
	if (! (raat_cb->table_handle->table_info.tbl_status_mask & RAAT_IDX))
	{
	    raat_cb->err_code = E_UG00D3_RaatCantGetBaseofBase;
	    return (FAIL);
	}

	if (raat_cb->flag & (RAAT_REC_BYNUM | RAAT_BAS_RECNUM))
	{
	    raat_cb->err_code = E_UG00D4_RaatInvRecDataFlag;
	    return (FAIL);
	}

	if ((raat_cb->bas_table_handle == NULL) ||
	    !(raat_cb->bas_table_handle->table_status & TABLE_OPEN))
	{
	    raat_cb->err_code = E_UG00D5_RaatNoBaseHandle;
	    return (FAIL);
	}
	/* Output data will come from base table of this secondary index */
	outrec_width = raat_cb->bas_table_handle->table_info.tbl_width;
    }
    else
    {
	/* Output data comes from this table */
	outrec_width = raat_cb->table_handle->table_info.tbl_width;
    }

    /* if prefetch buffer valid and not doing a get by tid  */
    if ((raat_cb->table_handle->table_status & TABLE_CURRFETCH)  && 
      	(raat_cb->table_handle->fetch_buffer) &&
        (!(raat_cb->flag & RAAT_REC_BYNUM)))
    {
       i4  wasprev;
       /* 
	  if the current buffer was fetched by a PREVious call, we have to 
	  change the direction that we are scanning the buffer since 
	  instead of going forward on NEXT and bacwards on PREV, we have
	  to do the opposite.
       */
       dir = (wasprev = raat_cb->table_handle->table_status & TABLE_PREV_BUFF)
	      ? -1 : 1;
       /* repos is set to the displacement from the current position */
       if (raat_cb->flag & RAAT_REC_NEXT)
	  repos = dir;
       else if (raat_cb->flag & RAAT_REC_PREV)
	  repos = -dir;
       else if (raat_cb->flag & RAAT_REC_CURR)
	  repos = 0;
       /* if going to go before the beginning or after the end, we either
	  have to send back no more rows (if we got the error code when we
	  first retrieved this buffer) or fall through to get another buffer */
       if ((raat_cb->table_handle->fetch_pos + repos < 0) ||
           (raat_cb->table_handle->fetch_pos + repos >=
            raat_cb->table_handle->fetch_actual))
       {
	  /* only be 'no more rows' at this point */
          if (raat_cb->table_handle->fetch_error)
	  {
	     /* it is possible that we got the error code, but now we are
		goijng in the opposite direction */
	     if ( (wasprev && (raat_cb->flag & RAAT_REC_PREV)) ||
	          (!wasprev && (raat_cb->flag & RAAT_REC_NEXT)))
             {
	        raat_cb->err_code = raat_cb->table_handle->fetch_error;
	        return (FAIL);
             }
	  }
	  /* 
	  ** the following  checks have to do with falling off the
	  ** beginning of the fetch buffer either by doing a PREV when
	  ** the buffer was fetched with a NEXT, or doing  a NEXT when
	  ** the buffer was fetched with a PREV.  In either case, the
	  ** current record of the RCB must be changed to reflect the
	  ** record that the user thinks is the current one, so we 
	  ** correctly fetch the next set of data.
          */
	  if ( (wasprev && (raat_cb->flag & RAAT_REC_NEXT)) ||
               (!wasprev && (raat_cb->flag & RAAT_REC_PREV)) )
	  {
	     /* position at first record in buffer */
	     buf_ptr = raat_cb->table_handle->fetch_buffer;
	     /* get the rcb for the 'current' record so it can be
		passed to the back end for repositioning */
	     MEcopy(buf_ptr, sizeof(i4), &rcb_lowtid);
	     MEcopy(buf_ptr + sizeof(i4), sizeof(i4), &rcb_currenttid);
	     tup_ptr = buf_ptr + (2 * sizeof(i4)); 
	     reposition = TRUE;
	  }
       } 
       else
       {
	  /* 
	  ** Point buf_ptr to current record based on prefetch position.
	  ** 1st sizeof(i4) is for rcb lowtid
	  ** 2nd sizeof(i4) is for tid  
	  ** Size of record for RAAT_BAS_RECDATA request is that of base table.
	  */
	  buf_ptr = raat_cb->table_handle->fetch_buffer + (sizeof(i4) +
              sizeof(i4) + outrec_width) *
	     (raat_cb->table_handle->fetch_pos + repos);

	  /* 
	  ** Pass back 'recnum' - tid of the record being returned,
	  ** OR tid of base table if requested, and reading secondary.
	  */
	  buf_ptr += sizeof(i4);   /* skip over rcb lowtid */
	  if (raat_cb->flag & RAAT_BAS_RECNUM)
	  {
	     MEcopy(
		buf_ptr + raat_cb->table_handle->table_info.tbl_width,
		sizeof(i4), (char *)&raat_cb->recnum);
	  }
	  else
	     MEcopy(buf_ptr, sizeof(i4), &raat_cb->recnum);
       
          buf_ptr += sizeof(i4);

	  /* copy record in buffer and return */
          MEcopy(buf_ptr, outrec_width, raat_cb->record);
          /* update 'current record pointer */
	  raat_cb->table_handle->fetch_pos += repos;
	  return (OK);
       }
    }
    /* 
    ** if we are here, either we don't have a prefetch buffer or 
    ** the prefetch buffer was exhausted, so request data from server.
    */
    /*
    ** Fill out gca data area, for record get the request format is:
    **
    ** i4		op_code
    ** i4		flag
    ** i4		rcb 'lowtid' (only if requesting re-position)
    ** PTR		table_handle->table_rcb
    ** i4		table_width  (width of base tbl if RAAT_BAS_RECDATA)
    ** i4		recnum (optional for get by record number)
    ** i4		Number of rows requested (1 if no prefetch)
    ** PTR		bas_table_handle->table_rcb (only if RAAT_BAS_RECDATA)
    **
    ** The request will be returned in the following format:
    **
    ** i4		err_code
    ** i4		row_actual  (# of recs returned)
    ** One or more sets of the following:
    ** 	   i4 	rcb 'lowtid' that can be used for future re-position
    **	   i4	tid of record
    **     'outrec_width' bytes of data - the record returned 
    **
    ** Allocate space once based on worse case.
    */
    space_needed = (6 * sizeof(i4)) + (2 * sizeof(PTR)) + 4088;

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
    *((i4 *)buf_ptr) = RAAT_RECORD_GET;
    msg_size = sizeof(i4);

    /*
    ** copy data value (flag) to gca buffer and 
    ** increment msg size
    */
    *((i4 *)(buf_ptr + msg_size)) = raat_cb->flag;
    /* if lowtid set, we are doing a reposition.  set flag and pass lowtid */
    if (reposition)
    {
       *((i4 *)(buf_ptr + msg_size)) |= RAAT_INTERNAL_REPOS;

       /* Send btree key if server is compatible */
       if ((raat_cb->table_handle->table_status & TABLE_SEND_BKEY)
	   && (raat_cb->table_handle->table_info.tbl_storage_type 
							== RAAT_BTREE_TYPE))
       {
	    *((i4 *)(buf_ptr + msg_size)) |= RAAT_INTERNAL_BKEY;
       }

       msg_size += sizeof(i4);

       *((i4 *)(buf_ptr + msg_size)) = rcb_lowtid;
       msg_size += sizeof(i4);

       /*
       ** Send currenttid and btree key if server is compatible
       */
       if ((raat_cb->table_handle->table_status & TABLE_SEND_BKEY) &&
	   (raat_cb->table_handle->table_info.tbl_storage_type 
							== RAAT_BTREE_TYPE))
       {
	   *((i4 *)(buf_ptr + msg_size)) = rcb_currenttid;
	   msg_size += sizeof(i4);

	   (VOID)send_bkey(raat_cb, tup_ptr, buf_ptr, rcb_currenttid,
								&msg_size);
       }
    }
    else
    {
	/* increment msg size for flag */
	msg_size += sizeof(i4);
    }

    /*
    ** NOTE bkey may have made (buf_ptr + msg_size) unaligned
    **
    ** Copy data value (table_rcb) to gca buffer and increment msg size
    */
    MEcopy(&raat_cb->table_handle->table_rcb, sizeof(PTR), buf_ptr + msg_size);
    msg_size += sizeof(PTR);

    /*
    ** Copy data value (table_size) to gca buffer and increment msg size
    */
    MEcopy(&outrec_width, sizeof(i4), buf_ptr + msg_size);
    msg_size += sizeof(i4);

    if (raat_cb->flag & RAAT_REC_BYNUM)
    {
    	/*
    	** Copy data value (recnum) to gca buffer and increment msg size 
    	*/
	MEcopy(&raat_cb->recnum, sizeof(i4), buf_ptr + msg_size);
    	msg_size += sizeof(i4);
    }

    /* Copy the # of rows requested, 1 if table cannot have prefetching  */
    if (raat_cb->table_handle->table_status & TABLE_PREFETCH)
	row_request = raat_cb->row_request;
    else
	row_request = 1;
    MEcopy(&row_request, sizeof(i4), buf_ptr + msg_size);
    msg_size += sizeof(i4);

    /*
    ** if requesting data from secondary index's base table, copy the
    ** rcb of the base table, provided by caller only in this case.
    */
    if (raat_cb->flag & RAAT_BAS_RECDATA)
    {
    	MEcopy(&raat_cb->bas_table_handle->table_rcb, sizeof(PTR), 
	    buf_ptr + msg_size);
    	msg_size += sizeof(PTR);
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

    /*
    ** collect data
    */

    buf_ptr = (char *)gca_int->gca_data_area;

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    raat_cb->err_code = *((i4 *)buf_ptr);
#else
    raat_cb->err_code = *((long *)buf_ptr);
#endif
    buf_ptr += sizeof(i4);

#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
    raat_cb->row_actual  = *((i4 *)buf_ptr);
#else
    raat_cb->row_actual  = *((long *)buf_ptr);
#endif

    /*
    ** if we tried to fetch more than one row.
    ** we have to do this in case we tried to fetch more than one row, 
    ** but we only got back one, since error code will be set 
    */
    if (raat_cb->row_actual & RAAT_INTERNAL_REPOS)
    {
       tried_mult = TRUE;
       raat_cb->row_actual &= ~RAAT_INTERNAL_REPOS;
    }
    else
       tried_mult = FALSE;

    buf_ptr += sizeof(i4);

    /* if table can be prefetched and tried to get back more than 1 record,
    ** then allocated buffer (if needed) and save off buffer 
    ** Even if we only got back one record, we still have to go through this
    ** since we already got end-of-table code.
    */
    if (tried_mult)
    {
       fetch_size = (sizeof(i4) + sizeof(i4) + 
	       outrec_width) * raat_cb->row_actual;
       /*
	  check to see if buffer is big enough.  would only fail if user
	  changed the size of # rows to prefetch.  We free storage and then
	  allocate, rather than reallocate since we don't care about preserving
	  the data. (besides the fact that we weren't sure if there was any
	  ME realloc routine)
       */
       if (fetch_size > raat_cb->table_handle->fetch_size)
       {
	  MEfree(raat_cb->table_handle->fetch_buffer);
	  raat_cb->table_handle->fetch_buffer = NULL;
       }

       if (!raat_cb->table_handle->fetch_buffer)    
       {
      	  raat_cb->table_handle->fetch_buffer = MEreqmem(0, fetch_size, TRUE, 
                                                         &status);
      	  raat_cb->table_handle->fetch_size   = fetch_size;
       }
       
       raat_cb->table_handle->fetch_error  = raat_cb->err_code; /* save code */
       /* save number of rows fetched */
       raat_cb->table_handle->fetch_actual = raat_cb->row_actual; 
       raat_cb->err_code = 0;
       /* mark this table as having a current prefetch buffer */
       raat_cb->table_handle->table_status |= TABLE_CURRFETCH;

       /* save the direction we were going in when fetched this data */
       if (raat_cb->flag & RAAT_REC_PREV)
          raat_cb->table_handle->table_status |= TABLE_PREV_BUFF;
       else
          raat_cb->table_handle->table_status &= ~TABLE_PREV_BUFF;
    }
    else
       raat_cb->table_handle->table_status &= ~TABLE_CURRFETCH;
    if (raat_cb->err_code)
    {
	/* Mark the table closed based upon "rollback" conditions */
	if ( (raat_cb->err_code == 196674)	/* deadlock */
	  || (raat_cb->err_code == 4705)	/* maxlocks reached */
	  || (raat_cb->err_code == 4706) )	/* force abort */
	    raat_cb->table_handle->table_status &= ~(TABLE_OPEN);
	return (FAIL);
    }

    /* 
    ** At this point buf_ptr points to the 1st of a series of
    ** {rcb_lowtid, tid, record} sets. If requested to give back the tidp
    ** of a sec index record, since there are two 2 i4s before the
    ** record, adding size of 1 i4 plus length of sec index record 
    ** leaves us pointing 4 bytes before end, ie. at the tidp, otherwise
    ** use the records own tid (after 1 i4).
    */
    if (raat_cb->flag & RAAT_BAS_RECNUM)
    {
	MEcopy(buf_ptr + sizeof(i4) + 
	    raat_cb->table_handle->table_info.tbl_width,
	    sizeof(i4), (char *)&raat_cb->recnum);
    }
    else
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx) || defined(NT_IA64) || defined(NT_AMD64)
	raat_cb->recnum = *(i4 *)(buf_ptr + sizeof(i4));
#else
	raat_cb->recnum = *(long *)(buf_ptr + sizeof(i4));
#endif
    /*
       We are saving the 1st record, even though we are sending it back,
       since we may be reading backwards at a later time, or for current record
    */
    if (tried_mult)
    {
       MEcopy(buf_ptr, fetch_size, raat_cb->table_handle->fetch_buffer);
       raat_cb->table_handle->fetch_pos = 0; /* the current record position */
    }
    buf_ptr += (2 * sizeof(i4));

    MEcopy(buf_ptr, outrec_width, raat_cb->record);

    return (OK);
}

VOID send_bkey(
RAAT_CB *raat_cb,
char *tup_ptr,
char *buf_ptr,
i4 tid,
i4  *msg_size)
{
    i4	    i, j;
    RAAT_ATT_ENTRY  *attptr;
    i4         key_count = 0;
    i4         key_size = 0;
    i1		    need_tid = FALSE;

    for (i = 1, attptr = raat_cb->table_handle->table_att;
		i <= raat_cb->table_handle->table_info.tbl_attr_count;
			i++, attptr++)
    {
	if (attptr->att_key_seq_number)
	{
	    key_count++;
	    key_size += attptr->att_width;
	    /*	if this is the last field of the secondary index, it must 
		be the tidp.
	    */
	    if ( (raat_cb->table_handle->table_info.tbl_status_mask & RAAT_IDX) 
		&& (i == raat_cb->table_handle->table_info.tbl_attr_count) )
		  need_tid = TRUE;
	}
    }

    /* 
    ** Copy key count to the gca buffer and increment msg size
    */
    *((i4 *)(buf_ptr + *msg_size)) = key_count;
    *msg_size += sizeof(i4);

    /* 
    ** Copy key size to the gca buffer and increment msg size
    */
    *((i4 *)(buf_ptr + *msg_size)) = key_size;
    *msg_size += sizeof(i4);

    /*
    ** Copy key attributes from prefetch buffer to gca buffer
    ** and increment msg size
    ** This is needed for internal reposition when row locking.
    ** (A btree key is 440 bytes maximum)
    **
    ** If this is a secondary index and the RAAT_BAS_RECDATA flag is set,
    ** we are not looking at a secondary index record.  We have the record
    ** from the base table.  So, we have to pull out the appropriate fields
    ** from the base table to recreate the secondary index record.
    */
    if ( (raat_cb->table_handle->table_info.tbl_status_mask & RAAT_IDX) &&
	(raat_cb->flag & RAAT_BAS_RECDATA))
    {
	RAAT_IDX_ENTRY *tidx;
	tidx = raat_cb->bas_table_handle->table_idx;
	/* find the correct secondary index */
	for (j = 0; j < raat_cb->bas_table_handle->table_info.tbl_index_count;
	     j++, tidx++)
	{
	   if (tidx->idx_id.db_tab_index ==
	       raat_cb->table_handle->table_info.tbl_id.db_tab_index)  
		break;
	}
	for (j = 0; j < tidx->idx_key_count; j++)
	{
	   for (i = 1, attptr = raat_cb->bas_table_handle->table_att;
		i <= raat_cb->bas_table_handle->table_info.tbl_attr_count;
			i++, attptr++)
	   {
	      if (tidx->idx_attr_id[j] == i) 
	      {
		 /* pass key value */
		 MEcopy(tup_ptr + attptr->att_offset, 
		 attptr->att_width, buf_ptr + *msg_size);
		 *msg_size += attptr->att_width;
		 break;
	      }
	   }
	}
	/* if secondary index key includes the tidp, include it now */
	if (need_tid)
	{
	   MEcopy(&tid, sizeof(i4), buf_ptr + *msg_size);
	   *msg_size += sizeof(i4);
	}
    }
    else
    {
       for (i = 0, attptr = raat_cb->table_handle->table_att;
	   i < raat_cb->table_handle->table_info.tbl_attr_count; i++, attptr++)
       {
	  if (attptr->att_key_seq_number)
	  {
	     /* pass key value */
	     MEcopy(tup_ptr + attptr->att_offset, 
 		attptr->att_width, buf_ptr + *msg_size);
  	     *msg_size += attptr->att_width;
 	  }
      }
   }
}
