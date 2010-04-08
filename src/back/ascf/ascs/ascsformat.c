/*
**Copyright (c) 2004 Ingres Corporation
*/

/* 
** Name: ASCSFORMAT.C -- format message for sending
** 
** Description:
**    Routines in this module format messages in the session's send 
**    buffer.
**
** 	ascs_format_results -- format results of a method execute
** 	ascs_format_tdesc -- format a tuple descriptor
** 	ascs_format_tuples -- format tuples for sending
** 	ascs_format_response -- format response message
**
** History:
** 	03-Mar-1998 (wonst02)
** 	    Assimilate module for Frontier's use.
**	03-Aug-1998 (fanra01)
**	    Removed // comments to build on unix.
**      15-Jan-1999 (fanra01)
**          Rename scs_format_response to ascs specific.  Call
**          ascs_gca_flush specific to ascs.
**      18-May-2000 (fanra01)
**          Bug 101345
**          Add functions to handle the sending of an ICE response using
**          tuples and removed the GCA_C_FROM message response.
**          Add function ascs_save_tdesc to store tuple descriptor required
**          for each message sent via net.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Dec-2001 (gordy)
**	    Removed PTR from GCA_COL_ATT and updated access code.
**	11-Feb-2005 (fanra01)
**          Sir 112482.
**          Removed urs facility reference.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
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
#include    <adp.h>
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
#include    <urs.h>

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

#include    <cui.h>

# include <ascf.h>
# include <ascs.h>

# define RESP_DESC_COUNT    3

/*
** Forward/Extern references 
*/
DB_STATUS ascs_format_tuples( SCD_SCB *scb, URSB *ursb );
STATUS ascs_get_data( SCD_SCB *scb, i4 row, i4 col,
    DB_DATA_VALUE *dbdv);

static DB_STATUS ascs_format_tdesc(SCD_SCB *scb, URSB *ursb);
PTR ascs_save_tdesc( SCD_SCB *scb, PTR desc );

static i4  
ascs_cnvrt_var2blob(i4 type);

static STATUS
ascs_format_blob(
    SCD_SCB *scb, 
    bool nullable, 
    i4 length, 
    bool null_indicator,
    char *dataptr);


/*
** Global variables owned by this file
*/


/*
** Name: ascs_format_results -- format results of a method execute
** Description:
** Input:
** Output:
** Return:
** History:
** 	03-Mar-1998 (wonst02)
** 	    Assimilate routine for Frontier's use.
*/
DB_STATUS
ascs_format_results(SCD_SCB 	*scb,
		   URSB		*ursb)
{
    DB_STATUS  		status = E_DB_OK;

    /*
    ** Format and queue the tuple descriptor
    */
    status = ascs_format_tdesc(scb, ursb);
    if (DB_FAILURE_MACRO(status))
    {
	i4 line = __LINE__;
	sc0e_uput(E_SC0501_INTERNAL_ERROR, 0, 2,
		 sizeof(__FILE__), __FILE__,
		 sizeof(line), (PTR)&line,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	return (status);
    }

    /*
    ** Format and queue the result tuples
    */
    status = ascs_format_tuples(scb, ursb);
    if (DB_FAILURE_MACRO(status))
    {
	i4 line = __LINE__;
	sc0e_uput(E_SC0501_INTERNAL_ERROR, 0, 2,
		 sizeof(__FILE__), __FILE__,
		 sizeof(line), (PTR)&line,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	return (status);
    }

    return (status);
}



#if 0
/*
** Name: ascs_get_type -- get column type
** Description:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS
ascs_get_type(ODB_DATA *odata, i4  index, DB_DATA_VALUE *db_data )
{
    STATUS	r_stat = E_DB_OK;

    if (odata->odb_type == ODB_SET)
    {
        odata = odata->odb.odb_tpl_data;
    }

    if (odata->odb_type == ODB_TPL)
    {
        odata = odata->odb.odb_tpl_data;
        odata = &odata[index];
    }

    db_data->db_datatype = DB_NODT;
    db_data->db_length = 0;
    db_data->db_prec = 0;

    switch( odata->odb_type )
    {
	case ODB_INT :
	    db_data->db_datatype = -DB_INT_TYPE;
	    db_data->db_length = 5;
	    break;
	case ODB_STR :
	    db_data->db_datatype = -DB_VCH_TYPE;
	    db_data->db_length = odata->odb_length+2+1;
	    break;

	case ODB_DEC :
	    db_data->db_datatype = -DB_DEC_TYPE;
	    db_data->db_length = DB_PREC_TO_LEN_MACRO(odata->odb_precision)+1;
	    db_data->db_prec = DB_PS_ENCODE_MACRO( odata->odb_precision,
                                                   odata->odb_scale );
	    break;

	case ODB_REAL :
	    db_data->db_datatype = -DB_FLT_TYPE;
	    db_data->db_length = 8+1;
	    break;
	
	case ODB_BOOL :
	    db_data->db_datatype = -DB_INT_TYPE;
	    db_data->db_length = 1+1;
	    break;

	case ODB_BSEQ :
            db_data->db_datatype = -DB_VBYTE_TYPE;
	    db_data->db_length = odata->odb_length+2+1;
            break;
	default :
	    /*
            ** Unknown Data Type, include ODB_TPL
            ** no nesting of ODB_TPL is allowed.
            */
	    {
		i4 line = __LINE__;
		sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
			 sizeof(__FILE__), __FILE__,
			 sizeof(line), (PTR)&line,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		r_stat = E_DB_ERROR;
	    }

	    db_data->db_datatype = DB_NODT;
	    db_data->db_length = 0;
	    break;
    }

    return( r_stat );
}
#endif /* 0 */



/*
** Name: ascs_format_tdesc -- format a tuple descriptor
** Description:
** Input:
** Output:
** Return:
** History:
**	21-Dec-01 (gordy)
**	    Removed PTR from GCA_COL_ATT (no longer contains DB_DATA_VALUE).
**	    Updating structure dependent code.
**  11-Feb-2005 (fanra01)
**      Removed urs_call reference.
*/
DB_STATUS 
ascs_format_tdesc(SCD_SCB 	*scb,
		 URSB		*ursb)
{
    GCA_TD_DATA *gca_tdesc;
    ULM_RCB	*ulm = &ursb->ursb_ulm;
    URSB_PARM	*ursb_parm = ursb->ursb_parm;
    PTR		darea;
    i4          desc_cnt, td_len = 0;
    GCA_COL_ATT	*data_tdesc;
    SCC_GCMSG   *message;
    DB_ERROR	err;
    STATUS	stat;
    DB_STATUS   status;

    desc_cnt =  ursb->ursb_num_ele;
    ursb->ursb_tsize = 0;

    /*
    ** Calculate the size of the complete GCA Tuple Descriptor 
    ** buffer.  This consists of a header(GCA_TD_DATA) and an 
    ** array of descriptors. Calculate the size of the column 
    ** descriptor array by multiplying one element's size by 
    ** the number of columns (desc_cnt).  One descriptor is 
    ** included in the header size.
    */
    td_len = sizeof(GCA_TD_DATA) - sizeof(GCA_COL_ATT) +
		      ((sizeof( data_tdesc->gca_attdbv ) +
			sizeof( data_tdesc->gca_l_attname )) * desc_cnt);

    /*
    ** Prevent further execution of this function since urs has been removed
    */
    ursb->ursb_error.err_code = E_SC0025_NOT_YET_AVAILABLE;
    sc0e_0_put( ursb->ursb_error.err_code, 0 );
	return E_DB_ERROR;
    
    /*
    ** Allocate area to store the tuple descriptor data 
    */
    ulm->ulm_psize = td_len;
    if ((status = ulm_palloc(ulm)) != E_DB_OK)
    {
	sc0e_0_put(ulm->ulm_error.err_code, 0);
        if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
            ursb->ursb_error.err_code = E_SC0004_NO_MORE_MEMORY;
        else
            ursb->ursb_error.err_code = E_SC0204_MEMORY_ALLOC;
	return E_DB_ERROR;
    }

    ursb->ursb_td_data = (PTR)ulm->ulm_pptr;
    MEfill(td_len, '\0', (PTR)ursb->ursb_td_data);

    /*
    ** Format the Tuple Descriptor header.
    */
    gca_tdesc = (GCA_TD_DATA *)ursb->ursb_td_data;
    gca_tdesc->gca_id_tdscr = 0;
    gca_tdesc->gca_l_col_desc = desc_cnt;
    gca_tdesc->gca_result_modifier = GCA_COMP_MASK;

    /*
    ** Point to the first column descriptor.
    */
    data_tdesc = &gca_tdesc->gca_col_desc[0];

    for(ursb_parm->sNumDescs = 1;
    	ursb_parm->sNumDescs <= desc_cnt;
	ursb_parm->sNumDescs++ )
    {
#if 0
	/*
	** Get the next output parm type and convert to GCA type.
	*/
	status = urs_call(URS_GET_TYPE, ursb, &err);
	if (status)
	{
	    sc0e_0_put(err.err_code, 0);
	    break;
	}

        /*
        ** Check for extended varchar datatype.  These data values
        ** will be returned to the client in GCA segmented BLOB
        ** format, so change the tuple descriptor info accordingly.
        */
        if ( SCS_IS_VARTYPE( data_tdesc->gca_attdbv.db_datatype )  &&
             data_tdesc->gca_attdbv.db_length > DB_CHAR_MAX )
        {
            data_tdesc->gca_attdbv.db_datatype =
                            ascs_cnvrt_var2blob(data_tdesc->gca_attdbv.db_datatype);
        }

        if (SCS_IS_BLOB(data_tdesc->gca_attdbv.db_datatype))
        {
            /*
            ** BLOBs do not have a fixed length.  We set their
            ** descriptor length to the size of the shortest
            ** possible BLOB: header, end-of-segments indicator
            ** and optional null indicator.
            */
            data_tdesc->gca_attdbv.db_length = ADP_HDR_SIZE + sizeof( i4 );
            if ( data_tdesc->gca_attdbv.db_datatype < 0 )
                data_tdesc->gca_attdbv.db_length++;
        }


        if ( omqs->omqs_flags & OMQS_COMP_VARCHAR  &&
             SCS_IS_VARTYPE( data_tdesc->gca_attdbv.db_datatype ) )
        {
            gca_tdesc->gca_result_modifier |= GCA_COMP_MASK;
        }
#endif /* 0 */

	/*
	** Keep a running total of the tuple size by adding up the
	** length of each column.
	*/
	ursb->ursb_tsize += ursb->ursb_parm->data_value.db_length;


	/*
	** Now copy the "working" column descriptor to the 
	** "real" column descriptor that is part of the column
	** descriptor array after the Tuple Descriptor header.
	** Increment the "real" column descriptor pointer after the move.
	*/
	DB_COPY_DV_TO_ATT( &ursb->ursb_parm->data_value, data_tdesc );
	data_tdesc->gca_l_attname = 0;
	data_tdesc = (GCA_COL_ATT *)((char *)data_tdesc + 
				    sizeof( data_tdesc->gca_attdbv ) + 
				    sizeof( data_tdesc->gca_l_attname ));
    }

    if ( status )
    {
	td_len = 0;
	return E_DB_ERROR;
    }
    gca_tdesc->gca_tsize = ursb->ursb_tsize;

    /*
    ** Put tdesc on the message queue 
    */
    if (td_len <= scb->scb_cscb.cscb_dsize)
    {
	/*
	** Preallocated message is big enough to hold the 
	** descriptor
	*/
	message = &scb->scb_cscb.cscb_dscmsg;
	message->scg_mask = SCG_NODEALLOC_MASK;
	darea = (PTR) scb->scb_cscb.cscb_darea;
    }
    else
    {
	stat = sc0m_allocate(0,
		    sizeof(SCC_GCMSG) 
			+ td_len,
		    DB_SCF_ID,
		    (PTR)SCS_MEM,
		    SCG_TAG,
		    (PTR *)&message);
	if (stat)
	{
	    sc0e_0_put(stat, 0);
	    sc0e_0_uput(stat, 0);
	    return (stat);
	}

	message->scg_mask = 0;
	message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
	darea = (PTR) message->scg_marea;
    }

    message->scg_msize = td_len;
    message->scg_mtype = GCA_TDESCR;
    MEcopy((PTR)ursb->ursb_td_data, td_len, darea);
    message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
    scb->scb_cscb.cscb_mnext.scg_prev = message;

    return E_DB_OK;
}


/*
** Name: ascs_format_tuples -- format tuples for sending
** 
** Description:
**    This routine loops through the results of the query and 
**    copies :( the tuples into the session's output buffer. As the 
**    buffer gets full, it is sent to the front end before the
**    next tuple of the result is written.
** Input:
**    SCD_SCB                 session's control block
** Output: 
**    none
** Returns:
**    E_DB_OK
**    E_DB_ERROR
** History:
**    08-sep-1996 (tchdi01)
**        created
**     29-jan-1997 (reijo01)
**         fixed bug that crashed the jasgcc process when a client tried to
**         retrieve tuples from the server.
*/
DB_STATUS
ascs_format_tuples(SCD_SCB *scb, URSB *ursb)
{
    DB_STATUS      status = E_DB_OK;
    ULM_RCB	  *ulm = &ursb->ursb_ulm;
    char          *tuple_data;
    i4   	   row;
    SCC_GCMSG     *message;


    /*
    ** Clear the output buffer 
    */
    status = ascs_gca_clear_outbuf(scb);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /*
    ** Format the GCA_TUPLES message
    */
    message = scb->scb_cscb.cscb_outbuffer;
    message->scg_mask  = SCG_NODEALLOC_MASK;
    message->scg_mtype = GCA_TUPLES;

    message->scg_mdescriptor = ursb->ursb_td_data;


    message->scg_next =
	(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    message->scg_prev =
	scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next =
	message;
    scb->scb_cscb.cscb_mnext.scg_prev =
	message;

    for (row=0; row < ursb->ursb_num_recs; row++)
    {
	status = ascs_format_tuple(scb, row, ursb);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	tuple_data += ursb->ursb_tsize;
    }

    return (status);
}


/*
** Name: ascs_format_tuple -- format tuple message
** 
** Description:
**     This formats and queues the GCA_TUPLE message with the
**     results of a query 
** Input:
**     scb                   session control block
**      .omf_cb               OMF control block
**        .omf_omqs            query data
**          .omqs_qresult        result of the query 
**     row                   the row number
** Output:
**     buf                   buffer to where the tuple is written
** Returns:
**     E_DB_OK
**     E_DB_ERROR
** History:
** 	03-Mar-1998 (wonst02)
** 	    Assimilate routine for Frontier's use.
**	21-Dec-01 (gordy)
**	    Removed PTR from GCA_COL_ATT (no longer contains DB_DATA_VALUE).
**	    Updating structure dependent code.
*/
DB_STATUS
ascs_format_tuple(SCD_SCB *scb, i4  row, URSB *ursb)
{
    DB_STATUS      	status = E_DB_OK;
    DB_DATA_VALUE       db_data;
    i4			col, name_length;
    GCA_TD_DATA 	*gca_tdesc;
    GCA_COL_ATT		*column_desc;

    if (scb->scb_sscb.sscb_interrupt) /* front-end does not want a response */
        return (E_DB_OK);

    /*
    ** Point to the tuple descriptor header.
    */
    gca_tdesc = (GCA_TD_DATA *)ursb->ursb_td_data;

    /*
    ** Point to first column descriptor of this tuple.
    */
    column_desc = &gca_tdesc->gca_col_desc[ 0 ];

    /*
    ** Loop for each column in the tuple.
    */
    for (col = 0 ; status == OK  &&  col < ursb->ursb_num_ele; col++ )
    {
	/*
	** Use the type that was determined already.
	*/
	DB_COPY_ATT_TO_DV( column_desc, &db_data );

	/*
	** Call URSM to point to the value of the column
	*/
	db_data.db_data = ursb->ursb_parm->
		pOutValues[row * ursb->ursb_num_ele + col].dv_value;

	/*
	** Call scs_get_data to get the value of the column
	*/
        status = ascs_get_data(scb, row, col, &db_data);
        if ( status != OK )
	    break;

#if 0
	/*
	** The length could have changed for variable size 
	** fields. We should take this into account and change
	** the buffer position accordingly after the call 
	** to scs_get_data(). The db_length field of the db_data
	** structure will be updated to have the actual length
	** of the field
	*/

        if (SCS_IS_BLOB( db_data.db_datatype ))
	{
	    /*
            status = fmt_blob( msg_ptr, db_data.db_datatype < 0 ,
			       db_data.db_length, null_indicator);
	    */
	}
        else
        {
            if ( omqs->omqs_flags & OMQS_COMP_VARCHAR  &&
                 SCS_IS_VARTYPE( db_data.db_datatype ) )
            {
		/*
		** Fix a problem, changed db_data.db_length to length
		** because we are suppose to pass the max length so
		** that fmt_compressed can find the null indicator.
		*/
		/*
                msg_len = fmt_compressed( msg_ptr, msg_len,
                                          db_data.db_datatype < 0 );
		*/
            }
        }
#endif
#ifdef BYTE_ALIGN
	MECOPY_CONST_MACRO( &column_desc->gca_l_attname, 
			    sizeof(i4), &name_length);
#else
	name_length = column_desc->gca_l_attname;
#endif
        column_desc = (GCA_COL_ATT *)
		((char *)column_desc + name_length +
		 sizeof(column_desc->gca_attdbv) + 
		 sizeof(column_desc->gca_l_attname));
    }

    return status;
}



/*
** Name:	ascs_get_data	Get next data value of the tuple.
** Description:
** Input:
** Output:
** Return:
** History:
**	21-Dec-01 (gordy)
**	    Removed unneeded GCA_COL_ATT parameter: info is available in dbdv.
**	11-Sep-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/
STATUS
ascs_get_data(
    SCD_SCB  	  *scb,
    i4       	  row,
    i4       	  col,
    DB_DATA_VALUE *dbdv)
{
    STATUS		r_stat = OK;
    static char		logkeybuf[16];
    DB_DT_ID		dt_id = dbdv->db_datatype;
    bool		nullable;
    bool		null_data;

    nullable = (dt_id < 0) ? TRUE : FALSE;
    if (nullable)
    {
	dt_id = -1 * dt_id;
    }
    null_data = (dbdv->db_data == NULL) ? TRUE : FALSE;

    switch( dt_id )
    {
	case DB_INT_TYPE:
	case DB_FLT_TYPE:
	case DB_CHR_TYPE:
	case DB_DTE_TYPE:
	case DB_ADTE_TYPE:
	case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
	case DB_TME_TYPE:
	case DB_TSWO_TYPE:
	case DB_TSW_TYPE:
	case DB_TSTMP_TYPE:
	case DB_INDS_TYPE:
	case DB_INYM_TYPE:
	case DB_MNY_TYPE:
	case DB_LOGKEY_TYPE:
	case DB_TABKEY_TYPE:
	case DB_BIT_TYPE:
	case DB_VBIT_TYPE:
	case DB_LBIT_TYPE:
	case DB_CPN_TYPE:
	case DB_CHA_TYPE:
	case DB_BYTE_TYPE:
	case DB_VBYTE_TYPE:
	case DB_LBYTE_TYPE:
	case DB_BOO_TYPE:
	case DB_DIF_TYPE:
	case DB_ALL_TYPE:
	case DB_DYC_TYPE:
	case DB_LTXT_TYPE:
	case DB_QUE_TYPE:
	case DB_DMY_TYPE:
	case DB_DBV_TYPE:
	case DB_OBJ_TYPE:
	case DB_HDLR_TYPE:
	case DB_NULBASE_TYPE:
	case DB_QTXT_TYPE:
	case DB_TFLD_TYPE:
	case DB_DEC_CHR_TYPE:
	    {
		char bufnull[2048];

		if (null_data)
		    bufnull[dbdv->db_length] = 1;     /* null data */
		else
		{
		    MEcopy(dbdv->db_data, dbdv->db_length, bufnull);
		    bufnull[dbdv->db_length] = 0;
		}
		if (nullable)
		    dbdv->db_length++;

		r_stat = ascs_gca_outbuf(scb, dbdv->db_length, bufnull);
		if (DB_FAILURE_MACRO(r_stat))
		    return (r_stat);
	    }
	    break;

	case DB_DEC_TYPE :
	    {
		char decnull[17];

		if (null_data)
		{
		    decnull[dbdv->db_length] = 1;
		}
		else
		{
		    MEfill(17, 0, decnull);
		    MEcopy(dbdv->db_data,
			   DB_PREC_TO_LEN_MACRO(dbdv->db_prec),
			   decnull);
		}
		if (nullable)
		    dbdv->db_length++;

		r_stat = ascs_gca_outbuf(scb, dbdv->db_length, decnull);
		if (DB_FAILURE_MACRO(r_stat))
		    return (r_stat);
		break;

	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
        {
	    char nulchar;

	    {
		u_i2		len;

		I2ASSIGN_MACRO(*(u_i2 *)dbdv->db_data, len);
		r_stat = ascs_gca_outbuf(scb, len+2, (PTR)dbdv->db_data);
		if (DB_FAILURE_MACRO(r_stat))
		    break;

		if (nullable)
		{
		    if (null_data)
		    nulchar = 1;
		else
		    nulchar = 0;

		r_stat = ascs_gca_outbuf(scb, 1, (PTR)&nulchar);
	    }
	    }

	    break;
	}
	case DB_LVCH_TYPE:
	default :
	    {
		i4 line = __LINE__;
		sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
			 sizeof(__FILE__), __FILE__,
			 sizeof(line), (PTR)&line,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		r_stat = E_DB_ERROR;
	    }
	}
    }

    return( r_stat );
}



/*
** Name: ascs_format_response -- format response message
** Description:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS 
ascs_format_response(SCD_SCB *scb, i4  resptype, i4  qrystatus, i4  rowcount)
{
    DB_STATUS     status = E_DB_OK;
    SCC_GCMSG     *message;

    if (scb->scb_sscb.sscb_interrupt) /* front-end does not want a response */
        return (E_DB_OK);

    message = &scb->scb_cscb.cscb_rspmsg;
    message->scg_mtype = scb->scb_sscb.sscb_rsptype;
    message->scg_next = 
	(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    message->scg_prev = 
	scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = 
	message;
    scb->scb_cscb.cscb_mnext.scg_prev = 
	message;
    message->scg_mask = SCG_NODEALLOC_MASK;

    message->scg_mtype = resptype;
    message->scg_msize = sizeof(GCA_RE_DATA);

    scb->scb_cscb.cscb_rdata->gca_rqstatus = qrystatus;
    scb->scb_cscb.cscb_rdata->gca_rowcount = 
	(qrystatus & GCA_FAIL_MASK) ? GCA_NO_ROW_COUNT : rowcount;

    return (status);
}



/*
** Name: ascs_format_scanid -- format OpenScan results
** Description:
** Input:
** Output:
** Return:
** History:
**    11-sep-1996 (tchdi01)
**        created for Jasmine
*/
DB_STATUS 
ascs_format_scanid(SCD_SCB *scb)
{
    DB_STATUS     status  = E_DB_OK;
#if 0
    OMF_CB        *omf_cb = (OMF_CB *)scb->scb_sscb.sscb_omscb;
    OMQS_ENTRY    *omqs   = omf_cb->omf_omqs;
    GCA_ID        id_desc;
    CS_SID        sid;

    CSget_sid(&sid);

    /*
    ** Fill in the GCA identifier for the scan
    */
    MEfill(sizeof(id_desc), '\0', &id_desc);
    id_desc.gca_index[0] = (longnat)sid;
    id_desc.gca_index[1] = omf_cb->omf_scan_cnt;
    omf_cb->omf_scan_cnt ++;

    /*
    ** Assign the identifier to the QS entry 
    */
    status = omqs_assign_id(omqs, 
	sizeof(id_desc.gca_index[1]), (PTR)&id_desc.gca_index[1]);
    if (DB_FAILURE_MACRO(status))
	goto func_exit;

    /*
    ** Put the ID message on the send queue 
    */
    status = ascs_gca_put(scb, GCA_QC_ID, sizeof(id_desc), (PTR)&id_desc);
    if (DB_FAILURE_MACRO(status))
	goto func_exit;

    /*
    ** Format the tuple descriptor for the scan and put it 
    ** on the send queue 
    */
    status = ascs_format_tdesc(scb);
    if (DB_FAILURE_MACRO(status))
	goto func_exit;


func_exit:
#endif /* 0 */
    return (status);
}


/*
** Name ascs_cnvrt_var2blob -- convert data type of blob
** Description:
** Input:
** Output:
** Return:
** History:
*/
static i4  
ascs_cnvrt_var2blob(i4 type)
{
    switch (type)
    {
    case DB_VCH_TYPE:      type = DB_LVCH_TYPE;    break;
    case -DB_VCH_TYPE:     type = -DB_LVCH_TYPE;   break;
    case DB_VBYTE_TYPE:    type = DB_LBYTE_TYPE;   break;
    case -DB_VBYTE_TYPE:   type = -DB_LBYTE_TYPE;  break;
    case DB_TXT_TYPE:      type = DB_LVCH_TYPE;    break;
    case -DB_TXT_TYPE:     type = -DB_LVCH_TYPE;   break;
    case DB_LTXT_TYPE:     type = DB_LVCH_TYPE;    break;
    case -DB_LTXT_TYPE:    type = -DB_LVCH_TYPE;   break;
    default:
	break;
    }
    return (type);
}



/*
** Name ascs_format_blob -- format blob
**
** Description:
** Input:
** Output:
** Return:
** History:
**    17-sep-1996 (tchdi01)
**        adapted from SDK2 code
**      18-May-2000 (fanra01)
**          Segment length is an i2 in gca message.
*/
static STATUS
ascs_format_blob(
    SCD_SCB *scb, 
    bool nullable, 
    i4 length, 
    bool null_indicator,
    char *dataptr)
{
    STATUS              r_stat;
    ADP_PERIPHERAL      hdr;
    i4                  ind;
    i4                  total;
    i2   		len;
    SCC_CSCB            *sccb  = &scb->scb_cscb;
    SCC_GCMSG *message = (SCC_GCMSG *)scb->scb_cscb.cscb_outbuffer;

    total = length;
    ind = 1;

    /*
    ** Send the header.
    */
    hdr.per_tag = ADP_P_GCA_L_UNK;
    hdr.per_length0 = 0;
    hdr.per_length1 = 0;
    r_stat = ascs_gca_outbuf(scb, ADP_HDR_SIZE, (PTR)&hdr);
    if (DB_FAILURE_MACRO(r_stat))  
	return( r_stat );

    while(total)
    {
        len  = (i2)(sccb->cscb_tuples_max - sccb->cscb_tuples_len);
        len -= sizeof(ind) + sizeof(len);
        len  = (len > total) ? total : len;

        /*
        ** Send a data segment.
        */
        r_stat = ascs_gca_outbuf(scb, sizeof(ind), (PTR)&ind);
	if (DB_FAILURE_MACRO(r_stat))  
	    return( r_stat );

        r_stat = ascs_gca_outbuf(scb, sizeof(len), (PTR)&len);
	if (DB_FAILURE_MACRO(r_stat))  
	    return( r_stat );

        r_stat = ascs_gca_outbuf(scb, len, (PTR)dataptr);
	if (DB_FAILURE_MACRO(r_stat))  
	    return( r_stat );

        dataptr += len;
        total -= len;

        /*
        ** Flush the buffer after each segment except the last.
        */
        if (total)
        {
	    message->scg_mask |= SCG_NOT_EOD_MASK; 
	    r_stat = ascs_gca_flush_outbuf(scb);
	    if (DB_FAILURE_MACRO(r_stat))
		return (r_stat);
        }
    }

    message->scg_mask &= ~SCG_NOT_EOD_MASK;

    /*
    ** Send the end-of-segments indicator
    ** and optional null indicator.
    */
    ind = 0;
    r_stat = ascs_gca_outbuf(scb, sizeof(ind), (PTR)&ind);
    if (DB_FAILURE_MACRO(r_stat))  
	return( r_stat );

    if (nullable)
    {
	char nullchar = (null_indicator) ? 1 : 0;

        r_stat = ascs_gca_outbuf(scb, 1, &nullchar);
	if (DB_FAILURE_MACRO(r_stat))  
	    return( r_stat );
    }

    return (E_DB_OK);
}


/*
** Name: ascs_format_ice_header
**
** Description:
**      Function sets up a GCA tuple descriptor message describing three
**      tuples.  The first tuple contains an integer message type of either
**      WPS_HTML_BLOCK, WPS_URL_BLOCK, WPS_DOWNLOAD_BLOCK.
**      The second tuple contains a long byte HTTP header.  The third tuple
**      contains a long byte data block of an HTML page or a list of variables
**      and values returned from a function call.
**
**      This function sends the response type tuple and the HTTP header tuple.
**      The repsonse data tuple is sent from ascs_format_ice_response.
**
** Input:
**      scb         Server session control block
**      resp_type   Type of message to set in tuple 0.
**      hlength     Length of the HTTP header to send.
**      header      Pointer to the HTTP header.
**
** Output:
**      None.
**
** Return:
**      E_DB_OK     Successfully completed.
**      !0          Failed status.
**
** History:
**      18-May-2000 (fanra01)
**          Created.
**	21-Dec-2001 (gordy)
**	    Use macro to copy GCA_COL_ATT info.
*/
DB_STATUS
ascs_format_ice_header(
    SCD_SCB *scb,
    u_i4    resp_type,
    i4      hlength,
    char    *header )
{
    GCA_TD_DATA     *gca_tdesc = NULL;
    GCA_TD_DATA     *tdesc = NULL;
    DB_DATA_VALUE   db_data;
    PTR             darea;
    PTR             descriptor;
    i4              desc_cnt = RESP_DESC_COUNT; /* columns to return */
    i4              td_len = 0;
    i4              tlen = 0;
    GCA_COL_ATT*    data_tdesc;

    SCC_GCMSG       *message;
    STATUS          stat = OK;
    DB_STATUS       status = E_DB_OK;
    CS_SID          sesssid;

    /*
    ** Calculate the size of buffer required for the tuple descriptor
    */
    td_len = sizeof(GCA_TD_DATA) - sizeof(GCA_COL_ATT) +
        ( sizeof( *data_tdesc ) * desc_cnt );

    gca_tdesc = (GCA_TD_DATA*)MEreqmem(0, td_len, TRUE, &stat);
    if (gca_tdesc != NULL)
    {
        if (td_len <= scb->scb_cscb.cscb_dsize)
        {
            /*
            ** Preallocated message is big enough to hold the
            ** descriptor
            */
            message = &scb->scb_cscb.cscb_dscmsg;
            message->scg_mask = SCG_NODEALLOC_MASK;
            darea = (PTR) scb->scb_cscb.cscb_darea;
        }
        else
        {
            stat = sc0m_allocate(0,
                    sizeof(SCC_GCMSG)
                        + td_len,
                    DB_SCF_ID,
                    (PTR)SCS_MEM,
                    SCG_TAG,
                    (PTR *)&message);
            if (stat)
            {
                sc0e_0_put(stat, 0);
                sc0e_0_uput(stat, 0);
                return(stat);
            }

            message->scg_mask = 0;
            message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
            darea = (PTR)message->scg_marea;
        }

        /*
        ** set the tuple descriptor parameters
        */
        descriptor = darea;
        td_len = 0;

        tlen = 0;
        CSget_sid( &sesssid );
        gca_tdesc->gca_tsize = sizeof(i4) + (2 * (ADP_HDR_SIZE + sizeof(i4)));
        tlen += sizeof(gca_tdesc->gca_tsize);
        gca_tdesc->gca_id_tdscr = sesssid;
        tlen += sizeof(gca_tdesc->gca_id_tdscr);
        gca_tdesc->gca_l_col_desc = desc_cnt;
        tlen += sizeof(gca_tdesc->gca_l_col_desc);
        gca_tdesc->gca_result_modifier = GCA_LO_MASK;
        tlen += sizeof(gca_tdesc->gca_result_modifier);

        MEcopy((PTR)gca_tdesc, tlen, darea);
        darea += tlen;
        td_len += tlen;

        /*
        ** tuple 0 - Response type
        */
        tlen = 0;
        data_tdesc = &gca_tdesc->gca_col_desc[0];
        data_tdesc->gca_attdbv.db_datatype = DB_INT_TYPE;
        tlen += sizeof(data_tdesc->gca_attdbv.db_datatype);
        data_tdesc->gca_attdbv.db_prec = 0;
        tlen += sizeof(data_tdesc->gca_attdbv.db_prec);
        data_tdesc->gca_attdbv.db_length = sizeof( i4 );
        tlen += sizeof(data_tdesc->gca_attdbv.db_length);
        data_tdesc->gca_attdbv.db_data = 0;
        tlen += sizeof(data_tdesc->gca_attdbv.db_data);
        data_tdesc->gca_l_attname = 0;
        tlen += sizeof(data_tdesc->gca_l_attname);
        MEcopy((PTR)data_tdesc, tlen, darea);
        darea += tlen;
        td_len += tlen;

        /*
        ** tuple 1 - HTTP header
        */
        tlen = 0;
        data_tdesc = &gca_tdesc->gca_col_desc[1];
        data_tdesc->gca_attdbv.db_datatype = DB_LBYTE_TYPE;
        tlen += sizeof(data_tdesc->gca_attdbv.db_datatype);
        data_tdesc->gca_attdbv.db_prec = 0;
        tlen += sizeof(data_tdesc->gca_attdbv.db_prec);
        data_tdesc->gca_attdbv.db_length = ADP_HDR_SIZE + sizeof( i4 );
        tlen += sizeof(data_tdesc->gca_attdbv.db_length);
        if ( data_tdesc->gca_attdbv.db_datatype < 0 )
            data_tdesc->gca_attdbv.db_length += 1;
        data_tdesc->gca_attdbv.db_data = 0;
        tlen += sizeof(data_tdesc->gca_attdbv.db_data);
        data_tdesc->gca_l_attname = 0;
        tlen += sizeof(data_tdesc->gca_l_attname);
        MEcopy((PTR)data_tdesc, tlen, darea);
        darea += tlen;
        td_len += tlen;

        /*
        ** tuple 2 - Response data
        */
        tlen = 0;
        data_tdesc = &gca_tdesc->gca_col_desc[2];
        data_tdesc->gca_attdbv.db_datatype = DB_LBYTE_TYPE;
        tlen += sizeof(data_tdesc->gca_attdbv.db_datatype);
        data_tdesc->gca_attdbv.db_prec = 0;
        tlen += sizeof(data_tdesc->gca_attdbv.db_prec);
        data_tdesc->gca_attdbv.db_length = ADP_HDR_SIZE + sizeof( i4 );
        tlen += sizeof(data_tdesc->gca_attdbv.db_length);
        if ( data_tdesc->gca_attdbv.db_datatype < 0 )
            data_tdesc->gca_attdbv.db_length += 1;
        data_tdesc->gca_attdbv.db_data = 0;
        tlen += sizeof(data_tdesc->gca_attdbv.db_data);
        data_tdesc->gca_l_attname = 0;
        tlen += sizeof(data_tdesc->gca_l_attname);
        MEcopy((PTR)data_tdesc, tlen, darea);
        darea += tlen;
        td_len += tlen;

        message->scg_msize = td_len;
        message->scg_mask  = SCG_NODEALLOC_MASK;
        message->scg_mtype = GCA_TDESCR;
        message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
        message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
        scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
        scb->scb_cscb.cscb_mnext.scg_prev = message;
        if ((tdesc = (GCA_TD_DATA*)ascs_save_tdesc( scb, (PTR)descriptor ))
            == NULL)
        {
            i4 line = __LINE__;
            sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
                sizeof(__FILE__), __FILE__,
                sizeof(line), (PTR)&line,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0);
            status = E_DB_ERROR;
        }
    }

    status = ascs_gca_clear_outbuf( scb );
    if (DB_FAILURE_MACRO(status))
    {
        MEfree( (PTR)gca_tdesc );
        return(status);
    }
    /*
    ** Format the GCA_TUPLES message
    */
    message = scb->scb_cscb.cscb_outbuffer;
    message->scg_mask  = SCG_NODEALLOC_MASK;
    message->scg_mask  |= SCG_NOT_EOD_MASK;
    message->scg_mtype = GCA_TUPLES;
    message->scg_mdescriptor = (PTR)tdesc;

    DB_COPY_ATT_TO_DV( &gca_tdesc->gca_col_desc[0], &db_data );
    db_data.db_data     = (PTR)&resp_type;
    if ((status = ascs_get_data(scb, 0, 0, &db_data))
        == E_DB_OK)
    {
        status = ascs_format_blob( scb, FALSE, hlength, FALSE, header );
    }

    MEfree( (PTR)gca_tdesc );
    return(status);
}

/*
** Name: ascs_format_ice_response
**
** Description:
**      Function formats and sends the ICE response as a long byte.
**      The function is passed a buffer to the data which may be incomplete.
**      The function uses the msgind flag to determine whether the segment
**      header should be sent or whether terminating indicator should be sent.
**
** Input:
**      scb         Server session control block.
**      pglen       Length of data to send.
**      response    Buffer containing the response.
**      msgind      Message indicator, ICE_RESP_HEADER or ICE_RESP_FINAL.
**
** Output:
**      None.
**
** Return:
**      E_DB_OK     Completed successfully.
**      !0          Failed status
**
** History:
**      18-May-2000 (fanra01)
**          Created.
*/
DB_STATUS
ascs_format_ice_response(
    SCD_SCB *scb,
    i4      pglen,
    char    *response,
    i4      msgind,
    i4*     sent )
{
    STATUS          r_stat;
    ADP_PERIPHERAL  hdr;
    i4              ind;
    i4              total;
    i2              len;
    SCC_CSCB        *sccb  = &scb->scb_cscb;
    SCC_GCMSG       *message = (SCC_GCMSG *)scb->scb_cscb.cscb_outbuffer;

    message->scg_mask  = SCG_NODEALLOC_MASK;
    message->scg_mtype = GCA_TUPLES;
    message->scg_mdescriptor =
        (PTR)scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc;

    total = pglen;
    ind = 1;

    if (msgind & ICE_RESP_HEADER)
    {
        /*
        ** Send the blob header.
        */
        hdr.per_tag = ADP_P_GCA_L_UNK;
        hdr.per_length0 = 0;
        hdr.per_length1 = 0;
        r_stat = ascs_gca_outbuf(scb, ADP_HDR_SIZE, (PTR)&hdr);
        if (DB_FAILURE_MACRO(r_stat))
            return( r_stat );
    }
    len  = (i2)(sccb->cscb_tuples_max - sccb->cscb_tuples_len);
    len -= sizeof(ind) + sizeof(len);
    /*
    ** if what is being sent fits into a gca message
    */
    if (total <= len)
    {
        /*
        ** if the message doesn't have room for the terminating indicator
        ** split the message length.
        */
        if ((len - total) < sizeof(ind))
        {
            len = total - sizeof(ind);
        }
        else
        {
            len = total;
        }
    }

    /*
    ** Send a data segment.
    */
    r_stat = ascs_gca_outbuf(scb, sizeof(ind), (PTR)&ind);
    if (DB_FAILURE_MACRO(r_stat))
        return( r_stat );

        r_stat = ascs_gca_outbuf(scb, sizeof(len), (PTR)&len);
    if (DB_FAILURE_MACRO(r_stat))
        return( r_stat );

        r_stat = ascs_gca_outbuf(scb, len, (PTR)response);
    if (DB_FAILURE_MACRO(r_stat))
        return( r_stat );

    response += len;
    *sent = len;
    /*
    ** Flush the buffer after each segment except the last.
    */
    if ((msgind & ICE_RESP_FINAL) == 0)
    {
        message->scg_mask |= SCG_NOT_EOD_MASK;
        r_stat = ascs_gca_flush_outbuf(scb);
        if (DB_FAILURE_MACRO(r_stat))
            return (r_stat);
    }
    if (msgind & ICE_RESP_FINAL)
    {
        /*
        ** Send the end-of-segments indicator
        */
        ind = 0;
        r_stat = ascs_gca_outbuf(scb, sizeof(ind), (PTR)&ind);
        if (DB_FAILURE_MACRO(r_stat))
            return( r_stat );
    }
    return(E_DB_OK);
}

/*{
** Name: ascs_save_tdesc     - Save this tdesc in SCF storage.
**
** Description:
**      This routine allocates a tuple descriptor for later use in
**	SCF memory.  If the existing tuple descriptor is large enough
**	then it is reused, otherwise the existing descriptor is freed
**	and a new tuple descriptor is allocated.
**
** Inputs:
**      scb                             Session control block
**	descmsg				Ptr to tuple descriptor message
**
** Outputs:
**      scb				sscb_cquery.cur_rdesc.rd_tdesc is updated
**
**	Returns:
**	    GCA status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-apr-1997 (shero03)
**          Created.
**	14-may-1997 (nanpr01)
**	    Will cause memory leak.
**	14-nov-1997 (wonst02)
** 	    Bug 86490 - Varchar compression and cursors
** 	    Save tdesc in sscb_cquery only when scb ptr is supplied.
**      18-May-2000 (fanra01)
**          Taken in entirety from scsqncr.c
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer contains PTR (in DB_DATA_VALUE).
[@history_template@]...
*/
PTR
ascs_save_tdesc(
	SCD_SCB	    *scb,
	PTR 	    desc)
{
    i4			new_desc_size = 0;
    i4			name_length;
    i4			i;
    GCA_TD_DATA		*old_desc = NULL;
    GCA_TD_DATA		*new_desc;
    DB_STATUS		status;
    GCA_COL_ATT		*next_desc;
    PTR			block;
    PTR			tdesc;
    
    if (scb)
    	old_desc = (GCA_TD_DATA*)scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc;
    new_desc = (GCA_TD_DATA *)desc;

    if (desc == NULL)	/* no new tuple descriptor */
    {			/* free any existing tuple descriptor */
    	if (old_desc)
	{
	    block = (char *)old_desc - sizeof(SC0M_OBJECT);
	    status = sc0m_deallocate(0, &block);
	    if (scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc == (PTR)old_desc)
	    	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc = NULL;
	    scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = NULL;
	    scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = 0;
	}
    	return (PTR)NULL;
    }

    /* calculate size of new tuple descriptor and object header */
    new_desc_size = sizeof (GCA_TD_DATA) - sizeof(GCA_COL_ATT)
      			 + sizeof(SC0M_OBJECT);
      
    next_desc = new_desc->gca_col_desc;
    for (i = 0; i < new_desc->gca_l_col_desc; i++)
    {
    	 new_desc_size += sizeof(next_desc->gca_attdbv) + 
			  sizeof(next_desc->gca_l_attname);
	 if (new_desc->gca_result_modifier & GCA_NAMES_MASK)
	 { 
#ifdef BYTE_ALIGN
	     MECOPY_CONST_MACRO(&next_desc->gca_l_attname, sizeof(i4),
	     			&name_length);
#else
	     name_length = next_desc->gca_l_attname;
#endif
	     new_desc_size += name_length;
	 }
	 else
	    name_length = 0;
	 next_desc = (GCA_COL_ATT *)
		   ((char *)next_desc + name_length +
		    sizeof(next_desc->gca_attdbv) + 
		    sizeof(next_desc->gca_l_attname));
    } 	

    if (old_desc)
    {
	if (scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size >= new_desc_size)
	{
	    /* 
	    ** We just need to copy the new descriptor in the buffer
	    */ 
	    MEcopy(desc, new_desc_size - sizeof(SC0M_OBJECT), old_desc);
	    return (PTR)old_desc;
	}
	/* Not enough ... We need more */
	block = (char *)old_desc - sizeof(SC0M_OBJECT);
	status = sc0m_deallocate(0, &block);
        scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = NULL;
        scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = 0;
    }

    /* allocate and format tuple descriptor and tuple data messages */

    status = sc0m_allocate(0,
    		   (i4)(new_desc_size),
    		   DB_SCF_ID,
    		   (PTR)SCS_MEM,
    		   SCG_TAG,
    		   &block);
    if (status)
    {
        sc0e_0_put(status, 0);
        sc0e_0_uput(status, 0);
        ascd_note(E_DB_SEVERE, DB_SCF_ID);
        return (PTR)NULL;
    }
    tdesc = (PTR)((char *)block + sizeof(SC0M_OBJECT));

    if (scb)
    {
    	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = new_desc_size;
    	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = tdesc;
    }

    /* copy in tuple descriptor */

    MEcopy(desc, new_desc_size - sizeof(SC0M_OBJECT), tdesc);

    return tdesc;
}

/*
** Name: ascs_tuple_len
**
** Description:
**      Function returns the value of the current tuple message count.
**
** Input:
**      scb         Server session control block.
**
** Output:
**      None.
**
** Return:
**      The length of the current tuple message.
**
** History:
**      06-Jun-2000 (fanra01)
**          Created.
*/
i4
ascs_tuple_len( SCD_SCB *scb )
{
    SCC_CSCB    *sccb  = &scb->scb_cscb;
    return(sccb->cscb_tuples_len);
}
