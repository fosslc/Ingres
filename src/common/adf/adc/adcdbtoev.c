/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <ulf.h>
#include    <adf.h>
#include    <adfint.h>
#include    <aduint.h>

/**
**
**  Name: ADCDBTOEV.C - Determines to which type to export
**
**  Description:
**      This file contains the routine DB to EV determination routine.
**	This routine determines how to export a database type. 
**
**          adc_dbtoev() - Make the determination
**
**
**  History:
**      07-apr-89 (fred)
**          Created.
**      28-apr-89 (fred)
**          Altered to use ADI_DT_MAP_MACRO().
**      27-apr-89 (fred)
**          Improved (to the point of correctness!) handling of nullable types.
**      29-oct-1992 (stevet)
**          Added check for protocol level to see if decimal data can
**          be supported.
**      22-dec-1992 (stevet)
**          Added function prototype.
**      12-Apr-1993 (fred)
**          Added byte datatype support
**      12-Jul-1993 (fred)
**          Fixed bug in incorrect length calculations for bit datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	26-feb-96 (thoda04)
**	    added aduint.h to include function prototype for adu_bitsize()
**	09-oct-96 (mcgem01)
**	    Error message E_AD101A modified to make it generic for 
**	    Jasmine & Ingres.
**	21-May-2009 (kiria01) b122051
**	    Reduce uninit'ed collID
**  16-Jun-2009 (thich01)
**      Treat GEOM type the same as LBYTE.
**  20-Aug-2009 (thich01)
**      Treat all spatial types the same as LBYTE.
**      09-mar-2010 (thich01)
**          Add DB_NBR_TYPE like DB_BYTE_TYPE for rtree indexing.
[@history_template@]...
**/

/*{
** Name: adc_dbtoev	- Determine which type to export
**
** Description:
**      This routine, given an input datatype, determines which type to send to
**	output.  It is assumed that a coercion exists for this datatype
**	transformation.
**
**	If the datatype is exportable (AD_NOEXPORT is not set), then the result
**	datatype is identical with the input.  Otherwise, a call is made to the
**	dbtoev routine specified in the datatype com vector. 
**
**	If the input is nullable, then the output is nullable as well. 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      db_value			Ptr to db_data_value for database
**					type/prec/length
**      ev_value                        Pointer to DB_DATA_VALUE for exported type
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      *ev_value                       Filled appropriately.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-apr-89 (fred)
**          Created.
**	11-jul-89 (jrb)
**	    Removed unused variables.
**      29-oct-1992 (stevet)
**          Added check for protocol level support to see if decimal data
**          can be exported.
[@history_template@]...
*/
DB_STATUS
adc_dbtoev(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *db_value,
DB_DATA_VALUE	   *ev_value)
{
    DB_DT_ID            bdt = abs(db_value->db_datatype);
    DB_DT_ID		bdtv;
    DB_STATUS		status = E_DB_OK;

    bdtv = ADI_DT_MAP_MACRO(bdt);
    if (    bdtv <= 0 || bdtv > ADI_MXDTS
	 || Adf_globs->Adi_dtptrs[bdtv] == NULL
       )
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    if (Adf_globs->Adi_dtptrs[bdtv]->adi_dtstat_bits &
		(AD_NOEXPORT | AD_CONDEXPORT))
    {
	if( bdt == DB_DEC_TYPE)
	{
	    if(adf_scb->adf_proto_level & AD_DECIMAL_PROTO)
	    {
		ev_value->db_datatype = db_value->db_datatype;
		ev_value->db_length = db_value->db_length;
		ev_value->db_prec = db_value->db_prec;		
		ev_value->db_collID = db_value->db_collID;		
	    }
	    else
		return(adu_error(adf_scb, E_AD101A_DT_NOT_SUPPORTED, 2,
				(i4) sizeof(SystemProductName),
				(i4 *) &SystemProductName));
	}
	else
	{
	    if (Adf_globs->Adi_dtptrs[bdtv]->adi_dt_com_vect.adp_dbtoev_addr)
	    {
		if (db_value->db_datatype >= 0)
		{
		    status = (*Adf_globs->Adi_dtptrs[bdtv]->
			      adi_dt_com_vect.adp_dbtoev_addr)
			      (adf_scb, db_value, ev_value);
		}
		else
		{
		    DB_DATA_VALUE	    tmp_db;
		    DB_DATA_VALUE	    tmp_ev;
		    
		    /* Datatype is nullable -- adjust length & datatype for */
		    /* underlying code	                                    */

		    tmp_db.db_length = db_value->db_length - 1;
		    tmp_db.db_prec = db_value->db_prec;
		    tmp_db.db_collID = db_value->db_collID;
		    tmp_db.db_datatype = bdt;
		    status = (*Adf_globs->Adi_dtptrs[bdtv]->
			      adi_dt_com_vect.adp_dbtoev_addr)
			       (adf_scb, &tmp_db, &tmp_ev);
		    ev_value->db_datatype = -(abs(tmp_ev.db_datatype));
		    ev_value->db_length = tmp_ev.db_length + 1;
		    ev_value->db_prec = tmp_ev.db_prec;
		    ev_value->db_prec = tmp_ev.db_collID;
		}
	    }
	    else
	    {
		return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	}
    }
    else
    {
	status = E_DB_WARN;
	STRUCT_ASSIGN_MACRO(*db_value, *ev_value);
    }
    return(status);
}

/*{
** Name: adc_1dbtoev_ingres	- Dbtoev for Ingres datatypes
**
** Description:
**      This routine, given an input datatype, determines which type to send to
**	output.  It is assumed that a coercion exists for this datatype
**	transformation.
**
**	If the datatype is exportable (AD_NOEXPORT is not set), then the result
**	datatype is identical with the input.  Otherwise, a call is made to the
**	dbtoev routine specified in the datatype com vector. 
**
**	If the input is nullable, then the output is nullable as well. 
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      db_value			Ptr to db_data_value for database
**					type/prec/length
**      ev_value                        Pointer to DB_DATA_VALUE for exported type
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      *ev_value                       Filled appropriately.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      09-apr-93 (fred)
**          Created.
**      12-Jul-1993 (fred)
**          For the bit datatypes, since they are being output as
**          varchar, we need to increase the size of the
**          aforementioned varchar element by DB_CNTSIZE.  This is
**          necessary since the number of characters needed is given
**          by adu_bitsize, but we need space for the count.
[@history_template@]...
*/
DB_STATUS
adc_1dbtoev_ingres(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *db_value,
DB_DATA_VALUE	   *ev_value)
{
    DB_DT_ID            bdt = abs(db_value->db_datatype);
    DB_DT_ID            bdtv;
    DB_STATUS		status = E_DB_OK;
    DB_DATA_VALUE       local_dv;
    i4                  must_coerce;

    must_coerce =
	Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(bdt)]->adi_dtstat_bits
	    & AD_NOEXPORT;

    bdtv = ADI_DT_MAP_MACRO(bdt);

    switch (bdt)
    {
    case DB_BYTE_TYPE:
    case DB_NBR_TYPE:
	ev_value->db_length = db_value->db_length;
	ev_value->db_datatype = db_value->db_datatype;
	ev_value->db_prec = db_value->db_prec;
	ev_value->db_collID = db_value->db_collID;
	if (  (must_coerce)
	    ||(   ((adf_scb->adf_proto_level & AD_BYTE_PROTO) == 0)
	       && (Adf_globs->Adi_dtptrs[bdtv]->adi_dtstat_bits &
		                                    AD_CONDEXPORT)
	       )
	   )
	{
	    ev_value->db_datatype = DB_CHA_TYPE;
	}
	break;

    case DB_VBYTE_TYPE:
	STRUCT_ASSIGN_MACRO(*db_value, *ev_value);
	ev_value->db_length = db_value->db_length;
	ev_value->db_datatype = db_value->db_datatype;
	ev_value->db_prec = db_value->db_prec;
	ev_value->db_collID = db_value->db_collID;
	if (  (must_coerce)
	    ||(   ((adf_scb->adf_proto_level & AD_BYTE_PROTO) == 0)
	       && (Adf_globs->Adi_dtptrs[bdtv]->adi_dtstat_bits &
		                                    AD_CONDEXPORT)
	       )
	   )
	{
	    ev_value->db_datatype = DB_VCH_TYPE;
	}
	break;

    case DB_LBYTE_TYPE:
    case DB_GEOM_TYPE:
    case DB_POINT_TYPE:
    case DB_MPOINT_TYPE:
    case DB_LINE_TYPE:
    case DB_MLINE_TYPE:
    case DB_POLY_TYPE:
    case DB_MPOLY_TYPE:
	ev_value->db_length = db_value->db_length;
	ev_value->db_datatype = db_value->db_datatype;
	ev_value->db_prec = db_value->db_prec;
	ev_value->db_collID = db_value->db_collID;
	if (  (must_coerce)
	    ||(   ((adf_scb->adf_proto_level & AD_BYTE_PROTO) == 0)
	       && (Adf_globs->Adi_dtptrs[bdtv]->adi_dtstat_bits &
		                                    AD_CONDEXPORT)
	       )
	   )
	{
	    ev_value->db_datatype = DB_LVCH_TYPE;
	}
	break;

    case DB_BIT_TYPE:
    case DB_VBIT_TYPE:
	ev_value->db_length = db_value->db_length;
	ev_value->db_datatype = db_value->db_datatype;
	ev_value->db_prec = db_value->db_prec;
	ev_value->db_collID = db_value->db_collID;
	if (  (must_coerce)
	    ||(   ((adf_scb->adf_proto_level & AD_BIT_PROTO) == 0)
	       && (Adf_globs->Adi_dtptrs[bdtv]->adi_dtstat_bits &
		                                    AD_CONDEXPORT)
	       )
	   )
	{
	    ev_value->db_datatype = DB_VCH_TYPE;
	    /* send them back as varchar, size appropriate to datatype */

	    local_dv.db_datatype = DB_INT_TYPE;
	    local_dv.db_length = sizeof(ev_value->db_length);
	    local_dv.db_prec = 0;
	    local_dv.db_data = (char *) &ev_value->db_length;
	    status = adu_bitsize(adf_scb, db_value, &local_dv);
	    ev_value->db_length += DB_CNTSIZE;
	}
	break;

    default:
	status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	break;
    }
    return(status);
}
