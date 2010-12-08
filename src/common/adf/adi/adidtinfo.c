/*
** Copyright (c) 1989, 1993 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adfint.h>

/**
**
**  Name: ADIDTINFO.C - Obtain datatype information
**
**  Description:
**      This file contains the adi_dtinfo() procedure, which is used to obtain
**	datatype information. 
**
**          adi_dtinfo() - Obtain datatype information
**	    adi_per_under() - find underlying datatype for peripheral type
**
**
**  History:
**      7-Apr-1989 (fred)
**          Created for user defined abstract datatype support.
**      27-apr-1989 (fred)
**	    Renamed variable for debugging and portability reasons...
**      28-apr-89 (fred)
**          Altered to use ADI_DT_MAP_MACRO().
**	05-jun-89 (fred)
**	    Completed above fix.
**      06-feb-1990 (fred)
**	    Added adi_per_under() routine.
**      21-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	06-apr-1998 (hanch04)
**	    Do abs once, don't pass it to macro.
**      29-may-2001 (stial01)
**          adi_dtinfo: DB_SECID_TYPE is AD_NOSORT if not ADI_B1SECURE (b104804)
[@history_line@]...
[@history_template@]...
**/

/*{
** Name: adi_dtinfo	- Obtain datatype information
**
** Description:
**      This routine returns the datatype status bits based upon the datatype id
**	passed in.  These status bits contain information about whether the
**	datatype should be use outside of the DBMS or converted, whether the
**	datatype is user defined or not, whether the datatype can be used inside
**	tables.   
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dtid				The datatype ID to get info about.
**	dtinfo				Ptr to a i4  to fetch the bits.
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
**      dtinfo				Bit mask is returned here.
**
**    Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded
**	    E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
** Exceptions:
**	    none.
**
** Side Effects:
**	    none
**
** History:
**      07-apr-1989 (fred)
**          Created.
**      27-apr-1989 (fred)
**	    Change names of variables to enhance readability and debugability.
**	06-apr-1998 (hanch04)
**	    Do abs once, don't pass it to macro.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
[@history_template@]...
*/

DB_STATUS
adi_dtinfo(
ADF_CB		    *adf_scb,
DB_DT_ID            dtid,
i4		    *dtinfo)
{
    dtid = abs(dtid);
    dtid = ADI_DT_MAP_MACRO(dtid);
    if (    dtid <= 0 || dtid  > ADI_MXDTS
	 || Adf_globs->Adi_dtptrs[dtid] == NULL
       )
	return(adu_error(adf_scb, E_AD2004_BAD_DTID, 0));

    *dtinfo = Adf_globs->Adi_dtptrs[dtid]->adi_dtstat_bits;
    return(E_DB_OK);
}

/*
** {
** Name: adi_per_under - Obtain information about an underlying datatype
**
** Description:
**      This routine returns information about the segments of an
**      underlying datatype for a peripheral datatype.  This
**      information is returned via a DB_DATA_VALUE.  The information
**      returned includes the underlying datatype and length for a
**      segment of the peripheral datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dtid				The datatype ID to get info about.
**	under_dv			Ptr to a DBDV int which to place the datatype.
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
**      under_dv->db_datatype		Underlying datatype is returned here.
**
**    Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded
**	    E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
** Exceptions:
**	    none.
**
** Side Effects:
**	    none
**
** History:
**      06-feb-1990 (fred)
**          Created.
**      18-Oct-1993 (fred)
**          Rebuilt to include all underlying information -- type and length.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
[@history_template@]...
*/

DB_STATUS
adi_per_under(
ADF_CB		    *adf_scb,
DB_DT_ID	    dtid,
DB_DATA_VALUE	    *under_dv)
{
    DB_STATUS       	status;
    DB_DT_ID        	mapped_id;
    ADP_POP_CB          pop_cb;

    for (;;)
    {
	mapped_id = ADI_DT_MAP_MACRO(abs(dtid));
	if (    mapped_id <= 0 || mapped_id  > ADI_MXDTS
	    || Adf_globs->Adi_dtptrs[mapped_id] == NULL
	    || !(Adf_globs->Adi_dtptrs[mapped_id]->adi_dtstat_bits
		             & AD_PERIPHERAL)
	    || Adf_globs->Adi_dtptrs[mapped_id]->adi_under_dt == 0)
	{
	    status = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	    break;
	}

	under_dv->db_prec = 0;
	under_dv->db_length = 0;
	under_dv->db_datatype =
	    Adf_globs->Adi_dtptrs[mapped_id]->adi_under_dt;
	
	if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi == NULL)
	{
	    status = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	    break;
	}
	
	pop_cb.pop_type = (ADP_POP_TYPE);
	pop_cb.pop_length = sizeof(ADP_POP_CB);
	pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
	pop_cb.pop_underdv = under_dv;
	status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
	    (ADP_INFORMATION, &pop_cb);
	if (status)
	{
	    status = adu_error(adf_scb, E_AD7000_ADP_BAD_INFO, 0);
	    break;
	}
	
	status = adc_seglen(adf_scb, dtid, under_dv);
	break;
    }
    return(status);
}
