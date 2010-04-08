/*
**Copyright (c) 2004 Ingres Corporation
*/

/* 
** Name: SCSBUILD.C -- build requests from GCA msgs
** 
** Description:
**
** History:
**	03-Aug-1998 (fanra01)
**	    Removed // comments for building on unix.
**      15-Jan-1999 (fanra01)
**          Renamed scs_gca_get, scs_gca_data_availaible, scs_bld_query,
**          scs_add_vardata and scs_read_blob to make them ascs specific.
**          Also add includes for prototypes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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

#include    <cui.h>

#include    <urs.h>
#include    <ascf.h>
#include    <ascs.h>


/*
** Macros
*/

/*
** Forward/Extern references 
*/


/*
** Global variables owned by this file
*/


/*
** Name: ascs_bld_query -- build query representation 
**
** Descripition:
**    This builds the OMF query description from the contents of
**    the GCA message
** Input:
**    scb                 session control block
** Output:
**    next_state          next state of the session 
** Return:
**    E_DB_OK             success
**    E_DB_ERROR          error occurred
** History:
**    06-sep-1996 (tchdi01)
**        created for Jasmine
*/
DB_STATUS 
ascs_bld_query(SCD_SCB *scb, i4  *next_state)
{
    DB_STATUS      status = E_DB_OK;
    GCA_Q_DATA     query_hdr;
    i4             done;
    GCA_DATA_VALUE value_hdr;
    i4        val_hdr_size;

    /*
    ** Get pointer to the query data buffer
    */
    status = ascs_gca_get(scb, (PTR)&query_hdr, 
	sizeof(GCA_Q_DATA) - sizeof(GCA_DATA_VALUE));
    if (DB_FAILURE_MACRO(status))
    {
	if (done == FALSE)
	{
	    /*
	    ** It's OK. We just don't have the query 
	    ** descriptor in here yet. Read it now.
	    */
	    *next_state = SCS_INPUT;
	    return (E_DB_OK);
	}
	else
	{
	    return (status);
	}
    }

    /*
    ** Verify the query language type. 
    */
    if (query_hdr.gca_language_id != DB_ODQL)
    {
	sc0e_0_put(E_SC0500_INVALID_LANGCODE, 0);
	return (E_DB_ERROR);
    }

#if 0
    /*
    ** Get the query descriptor pointer
    */
    omf_cb = (OMF_CB *)scb->scb_sscb.sscb_omscb;
    status = omqs_alloc_entry(omf_cb, &omqs);
    if (DB_FAILURE_MACRO(status))
	return (status);
    omf_cb->omf_omqs = omqs;

    /*
    ** Get ODQL query modifier
    */
    if (query_hdr.gca_query_modifier & GCA_ODQL_MASK)
       omqs->omqs_flags |= 
	   ((query_hdr.gca_query_modifier & GCA_ODQL_MASK) << 8);

    /*
    ** Set the compression flag
    */
    if (query_hdr.gca_query_modifier & GCA_COMP_MASK)
	omqs->omqs_flags |= OMQS_COMP_VARCHAR;
#endif /* 0 */

    val_hdr_size = sizeof(value_hdr.gca_type) +
		   sizeof(value_hdr.gca_precision) +
		   sizeof(value_hdr.gca_l_value); 


    while (status == E_DB_OK && ascs_gca_data_available(scb))
    {
	/*
	** read the gca data value descriptor
	*/
	status = ascs_gca_get(scb, (PTR)&value_hdr, val_hdr_size);
	if (DB_FAILURE_MACRO(status))
	    /*
	    ** We have only been able to read a part of the 
	    ** message. This is an error.
	    */
	    break;

	if (value_hdr.gca_type == DB_QTXT_TYPE )
	{
	    /*
	    ** The GCA_DATA_VALUE contains query text.
	    ** Concatenate the text to the preceding text.
	    */
#if 0
	    QBUFF_CHECK(omf_cb, &omqs->omqs_stmt_text, value_hdr.gca_l_value);
	    status = ascs_gca_get(scb, 
		omqs->omqs_stmt_text.text + omqs->omqs_stmt_text.len,
		value_hdr.gca_l_value);
	    if (DB_FAILURE_MACRO(status))
		break;
	    omqs->omqs_stmt_text.len += value_hdr.gca_l_value;
	    omqs->omqs_stmt_text.end += value_hdr.gca_l_value;
#endif
	}
	else
	{
	    /*
	    ** The GCA_DATA_VALUE contains an input variable value.
	    ** Add it to the gca data value array.
	    */
	    status = ascs_add_vardata(scb, &value_hdr);
	    if (DB_FAILURE_MACRO(status))
		return (status);
	}
    }
	
    return (status);
}


/*
** Name: ascs_add_vardata
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS 
ascs_add_vardata(SCD_SCB *scb, GCA_DATA_VALUE *gdv)
{
    DB_STATUS      status = E_DB_OK;
#if 0
    OMF_CB         *omf_cb;
    OMQS_ENTRY     *omqs;
    u_i2	   len,length;
    ODB_DATA       *odb_data;

    omf_cb = (OMF_CB *)scb->scb_sscb.sscb_omscb;
    omqs   = omf_cb->omf_omqs;


    if ( SCS_IS_BLOB( gdv->gca_type ) )
    {
	i4 blob_len = 0;
	PTR     segment;

	status = omqs_data_alloc(omf_cb, &odb_data);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_0_put(E_SC0204_MEMORY_ALLOC, 0);
	    return (status);
	}
	odb_data->odb_null = ODB_DATA_VALID;

        status = ascs_read_blob(scb, gdv->gca_type, odb_data);
    }
    else
    {
	/*
	** Allocate the ODB_DATA structure to hold the data value 
	*/
	status = omqs_data_alloc(omf_cb, &odb_data);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_0_put(E_SC0204_MEMORY_ALLOC, 0);
	    return (status);
	}
	odb_data->odb_null = ODB_DATA_VALID;


	/*
	** Convert the value and the type from GCA_DATA_VALUE 
	** to ODB_DATA. 
	*/
	switch(gdv->gca_type)
	{
	case DB_INT_TYPE:
	    if (gdv->gca_l_value == sizeof(i1))
	    {
		/* 
		** Allocate a boolean 
		*/
		status = omqs_tmp_alloc(omqs, 
		    sizeof(*odb_data->odb.odb_bool_data), 
		    (PTR *)&odb_data->odb.odb_bool_data);
		if (DB_FAILURE_MACRO(status))
		    break;

		/* 
		** Set the boolean value in ODB_DATA
		*/
		odb_data->odb_type = ODB_BOOL;
		status = ascs_gca_get(scb, 
		    (PTR)odb_data->odb.odb_bool_data, gdv->gca_l_value);
	    }
	    else
	    {
		/* 
		** Allocate an integer 
		*/
		status = omqs_tmp_alloc(omqs, 
		    sizeof(*odb_data->odb.odb_int_data), 
		    (PTR *)&odb_data->odb.odb_int_data);
		if (DB_FAILURE_MACRO(status))
		    break;

		/*
		** Store an integer in ODB_DATA 
		*/
		odb_data->odb_type = ODB_INT;
		status = ascs_gca_get(scb, 
		    (PTR)odb_data->odb.odb_int_data, gdv->gca_l_value);
	    }
	    break;

	case DB_FLT_TYPE:

	    /* 
	    ** Allocate a float 
	    */
	    status = omqs_tmp_alloc(omqs, 
		sizeof(*odb_data->odb.odb_real_data), 
		(PTR *)&odb_data->odb.odb_real_data);
	    if (DB_FAILURE_MACRO(status))
		break;

	    /*
	    ** Store a float value in ODB_DATA 
	    */
	    odb_data->odb_type = ODB_REAL;
	    status = ascs_gca_get(scb, 
		(PTR)odb_data->odb.odb_real_data, gdv->gca_l_value);
	    break;

	case DB_VCH_TYPE:
	case DB_LVCH_TYPE :
	    /*
	    ** We will have to copy the string in order to be able to 
	    ** add the EOS character at the end of the string 
	    */
	    odb_data->odb_type = ODB_STR;
	    odb_data->odb_length = gdv->gca_l_value - sizeof(i2) + 1;

	    status = omqs_tmp_alloc(omqs, odb_data->odb_length, 
		&odb_data->odb.odb_str_data);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    status = ascs_gca_get(scb, (PTR)&len, sizeof(len));
	    if (DB_FAILURE_MACRO(status))
		break;

	    status = ascs_gca_get(scb, 
		(PTR)odb_data->odb.odb_str_data, len);
	    if (DB_FAILURE_MACRO(status))
		break;

	    odb_data->odb.odb_str_data[len] = EOS;
	    break;

	case DB_BYTE_TYPE :
	case DB_VBYTE_TYPE :
	    odb_data->odb_type = ODB_BSEQ;

	    /* get the length of the bytesequence */
	    status = ascs_gca_get(scb, (PTR)&len, sizeof(len));
	    if (DB_FAILURE_MACRO(status))
		break;

	    odb_data->odb_length = len;

	    /*
	    ** Allocate memory to hold the data 
	    */
	    status = omqs_tmp_alloc(omqs, len, 
		(PTR *)&odb_data->odb.odb_str_data);
	    if (DB_FAILURE_MACRO(status))
		break;

	    /* retrieve the pointer to the bytesequence */
	    status = ascs_gca_get(scb, odb_data->odb.odb_str_data, len);

	    break;

	case DB_DEC_TYPE:
	    odb_data->odb_type = ODB_DEC;
	    odb_data->odb_length = gdv->gca_l_value;
	    odb_data->odb_precision = DB_P_DECODE_MACRO(gdv->gca_precision);
	    odb_data->odb_scale = DB_S_DECODE_MACRO(gdv->gca_precision);

	    /*
	    ** Allocate memory to hold the decimal value
	    */
	    status = omqs_tmp_alloc(omqs, odb_data->odb_length, 
		(PTR *)&odb_data->odb.odb_dec_data);
	    if (DB_FAILURE_MACRO(status))
		break;

	    status = ascs_gca_get(scb, (PTR)odb_data->odb.odb_dec_data,
		gdv->gca_l_value);
	    break;

	case DB_LBYTE_TYPE :
	default:
	    {
		i4 line = __LINE__;
		sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
			 sizeof(__FILE__), __FILE__,
			 sizeof(line), (PTR)&line,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		return (status);
	    }
	}

	if (DB_FAILURE_MACRO(status))
	{
	    i4  line = __LINE__;
	    sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
		     sizeof(__FILE__), __FILE__,
		     sizeof(line), (PTR)&line,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    return (status);
	}
    }

#endif /* 0 */

    return(status);
}


/*
** Name: ascs_read_blob -- read a blob segment
** Descripition:
** Input:
** Output:
** Return:
** History:
**     10-sep-1996 (tchdi01)
**         adapted from SDK2 code
*/
#if 0
DB_STATUS 
ascs_read_blob(SCD_SCB *scb, i4  type, ODB_DATA *odata)
{
    DB_STATUS      r_stat = E_DB_OK;
    DB_STATUS      mem_stat;
    ADP_PERIPHERAL hdr;
    i4             ind;
    u_i2           len;
    OMF_CB         *omf_cb   = (OMF_CB *)scb->scb_sscb.sscb_omscb;
    char           *segment  = NULL;
    i4        seglength = 0;

    /*
    ** Initialize our in-memory segment buffer.
    */
    r_stat = omqs_tmp_alloc(omf_cb->omf_omqs, OMQS_BLOB_MAX_SEG + 1, (PTR *)&segment);
    if (DB_FAILURE_MACRO(r_stat))
	return (r_stat);


    /*
    ** Read the BLOB header and first segemnt indicator.
    */
    r_stat = ascs_gca_get(scb, (PTR)&hdr, ADP_HDR_SIZE );
    if (DB_FAILURE_MACRO(r_stat)) 
	goto func_exit;

    r_stat = ascs_gca_get(scb, (PTR)&ind, sizeof(ind));
    if (DB_FAILURE_MACRO(r_stat))
	goto func_exit;

    while (ind)
    {
	r_stat = ascs_gca_get(scb, (PTR)&len, sizeof(len));
	if (DB_FAILURE_MACRO(r_stat))
	    goto func_exit;

	if (seglength + (i4)len > (i4)OMQS_BLOB_MAX_SEG)
	{
	    /*
	    ** At this point we could write the current segment
	    ** to disk and continue filling the memory buffer,
	    ** but there is no current requirement to support
	    ** BLOBs larger than the provided memory space, so
	    ** we just produce an error instead.
	    */
	    i4  line = __LINE__;
	    sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
		     sizeof(__FILE__), __FILE__,
		     sizeof(line), (PTR)&line,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    goto func_exit;
	}

	/*
	** Get the segment data.
	*/
	r_stat = ascs_gca_get(scb, &segment[seglength], len);
	if (DB_FAILURE_MACRO(r_stat))
	    goto func_exit;

	seglength += len;

	/*
	** Get the next segment indicator.
	*/
	r_stat = ascs_gca_get(scb, (char *)&ind, sizeof(ind));
	if (DB_FAILURE_MACRO(r_stat))
	    goto func_exit;
    }

    /*
    ** If the BLOB is nullable, read the null indicator
    ** byte into the segment buffer immediately following
    ** the segment data.
    */
    if (type < 0)
    {
	r_stat = ascs_gca_get(scb, &segment[seglength], 1);
	if (DB_FAILURE_MACRO(r_stat))
	    goto func_exit;
	seglength++;
    }

    odata->odb_length = seglength;

    if (abs(type) == DB_LVCH_TYPE)
    {
	odata->odb_type = ODB_STR;
	odata->odb.odb_str_data = segment;
    }
    else
    {
	odata->odb_type = ODB_BSEQ;
	odata->odb.odb_bseq_data = segment;
    }

func_exit:

    if (DB_FAILURE_MACRO(r_stat) && *segment) 
    {
	mem_stat = omqs_tmp_free(omf_cb->omf_omqs, (PTR)*segment);
	if (mem_stat > r_stat) 
	    r_stat = mem_stat;
	*segment = NULL;
    }


    return (r_stat);
}
#endif /* 0 */


/*
** Name:
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
