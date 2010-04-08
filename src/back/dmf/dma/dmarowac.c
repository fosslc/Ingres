/*
**Copyright (c)  2004 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <me.h>
#include    <tm.h>
#include    <pc.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <adf.h>
#include    <scf.h>
#include    <sxf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dma.h>
 
/*
** Name: DMAROWAC.C - Security Row access checking primitives.
**
** Description:
**      This file contains the routines needed to manage security
**	Row operations:
**	- Row-level security auditing.
**
** Routines defined in this file:
**	dma_row_access  - Do row security access
**
** History:
**      26-may-1993 (robf)
**          Created 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *,
**	    use new form uleFormat.
**/

/*
**  Definition of static variables and forward static functions.
*/
 

/*{
** Name: dma_row_access - Process row-level access/auditing
**
** Description:
**      This routine does row-level access/auditing. 
**
**	The two main functions are:
**	- For a C2 server, write row audit records if necessary.
**
**	The result of the MAC arbitration is returned in the 
**	parameter 'cmp_res':
**	DMA_ACC_PRIV == access allowed
**	DMA_ACC_NOPRIV == access denied
**	DMA_ACC_POLY == access denied, but polyinstantiation allowed.
**
** Inputs:
**      event                           Type of event being logged.
**      access                          Access masks indicating type
**                                      of access.
**      rcb                             Pointer to rcb of table/row.
**      record                          Pointer to record being arbitrated
**
** Outputs:
**      cmp_res                         Pointer to return results of 
**                                      comparison.  See description.
**      err_code                        Pointer of area to return 
**                                      error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    May write audit records.
**
** Error codes:
**	 E_DM001A_BAD_FLAG 
**	 E_DM0126_ERROR_WRITING_SECAUDIT 
**	 E_DM910D_ROW_MAC_ERROR 
**	 E_DM910E_ROW_SEC_AUDIT_ERROR 
**
** History:
**	26-may-1993 (robf)
**          Created 
**	26-oct-93 (robf)
**          Reset regrade flag prior to writing audit record
**	30-dec-93 (robf)
**          Initialize sxf_scb/sxf_sess_id.
**	30-mar-94 (robf)
**          Initialize sxf_db_name, row auditing may AV after
**	    Steve's recent changes. Also MEfill entire structure to
**          help protect against any future changes.
**	    Special case auditing of table keys (default audit attribute)
**          and format into HEX to readibility.
**	10-may-94 (robf)
**          Check for requested polyinstantiation and return new value
**	    caller in this case.
**	27-july-1995 (thaju02)
**	    Removed the initialization of local ptr 'scb' to
**	    rcb->rcb_xcb_ptr->xcb_scb_ptr, since scb is not referenced
**	    anywhere in this routine.  This was causing a segv(fatal
**	    exception) during 'relocatedb -new_database=...' on the 
**	    Sun Solaris platform.  The relocatedb code directly calls 
**	    dm2r_put which in turn calls dma_row_access.  The
**	    rcb->rcb_xcb_ptr was not initialized, thus causing the segv.
**	08-Jan-2001 (jenjo02)
**	    Get *ADF_CB from session's DML_SCB instead of calling SCF.
**	16-dec-04 (inkdo01)
**	    Added a collID.
*/
DB_STATUS
dma_row_access(
i4	    access,	/* Type of access to row */
DMP_RCB	    *rcb,	/* RCB for table */
char	    *record,	/* Record being accessed */
char	    *oldrecord,/* Oldrecord for updates */
i4	    *cmp_res,	/* Result of arbitration */
DB_ERROR    *dberr)
{
    i4		    sl_access;
    bool	    c2server;
    DB_STATUS	    status=E_DB_OK;
    SXF_ACCESS	    sxf_access;
    SXF_RCB	    sxfrcb;
    DMP_TCB	    *tcb = rcb->rcb_tcb_ptr; 
    DB_ATTS	    *att;
    i4		    op;
    DB_DATA_VALUE   fromdv;
    DB_DATA_VALUE   todv;
    i4	    local_error;
    char	    detail_text[SXF_DETAIL_TXT_LEN+2];
    DB_TEXT_STRING  *text_ptr;
    static  		i4 debug=0;
    /*
    ** Temporary alignment buffer for auditing. 
    */
    ALIGN_RESTRICT	align_buffer[SXF_DETAIL_TXT_LEN/sizeof(ALIGN_RESTRICT)];

    CLRDBERR(dberr);

    if(debug)
	return E_DB_OK;

    for(;;)
    {
	/*
	** default no access
	*/
	*cmp_res=DMA_ACC_NOPRIV;
	/*
	** Check if got C2 server
	*/

	if (dmf_svcb->svcb_status & SVCB_C2SECURE)
		c2server=TRUE;
	else
		c2server=FALSE;

	/*
	** If not C2 then we don't need to do any more,
	** so break;
	*/
	if (!c2server)
	{
		/* Allow access */
		*cmp_res=DMA_ACC_PRIV;
		break;
	}

	/*
	** Check kind of access to row
	*/
	if (access & DMA_ACC_SELECT)
	{
		sxf_access = SXF_A_SELECT;
	}
	else if (access & DMA_ACC_INSERT)
	{
		sxf_access = SXF_A_INSERT;
	}
	else if (access & DMA_ACC_DELETE)
	{
		sxf_access = SXF_A_DELETE;
	}
	else if (access & DMA_ACC_UPDATE)
	{
		sxf_access = SXF_A_UPDATE;
		if(!oldrecord)
		{
			/*
			** On  update need the old record
			*/
			SETDBERR(dberr, 0, E_DM001A_BAD_FLAG);
			status=E_DB_ERROR;
			break;
		}
	}
	else
	{
		/*
		** Invalid input, must specify the kind of access we are
		** attempting. Deny access and return an error.
		*/
		SETDBERR(dberr, 0, E_DM001A_BAD_FLAG);
		status=E_DB_ERROR;
		break;
	}
        MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);

	sxfrcb.sxf_cb_type=SXFRCB_CB;
	sxfrcb.sxf_length=sizeof(sxfrcb);
	sxfrcb.sxf_accessmask=sxf_access;
	sxfrcb.sxf_sess_id = (CS_SID)0;
	sxfrcb.sxf_scb=NULL;
	sxfrcb.sxf_objowner= (DB_OWN_NAME*)&tcb->tcb_rel.relowner;
	sxfrcb.sxf_objname=  (char*)&tcb->tcb_rel.relid;
	sxfrcb.sxf_objlength= DB_MAXNAME;
	sxfrcb.sxf_detail_len=0;
	sxfrcb.sxf_auditevent=SXF_E_RECORD; /* Record event */
	sxfrcb.sxf_force = 0;
	sxfrcb.sxf_detail_int=0;
	sxfrcb.sxf_db_name=NULL;
	/*
	** Set up security audit key if table row auditing.
	** We do this prior to the MAC check since SXM may write audit
	** records due to things like regrades.
	*/
	if(tcb->tcb_rel.relstat2&TCB_ROW_AUDIT)
	{
		att=tcb->tcb_sec_key_att;
		if(!att)
		{
			status=E_DB_ERROR;
			SETDBERR(dberr, 0, E_DM910E_ROW_SEC_AUDIT_ERROR);
			break;
		}
		fromdv.db_datatype= att->type;
		fromdv.db_length=att->length;
		fromdv.db_prec=att->precision;
		fromdv.db_collID=att->collID;
		/*
		** When updating we audit the OLD value, not the new 
		*/
		if(sxf_access == SXF_A_UPDATE)
			fromdv.db_data= &oldrecord[att->offset];
		else
			fromdv.db_data= &record[att->offset];
		/*
		** Check for alignment
		*/
		if(ME_ALIGN_MACRO((PTR)fromdv.db_data, sizeof(ALIGN_RESTRICT))
			!=fromdv.db_data)
		{
			i4 len;
			/*
			** Data is not aligned, so copy to temp buffer
			*/

			if(fromdv.db_length>sizeof(align_buffer))
				len=sizeof(align_buffer);
			else
				len=fromdv.db_length;
			MEcopy((PTR)fromdv.db_data, len,
					(PTR)&align_buffer);
			fromdv.db_data=(PTR)align_buffer;
		}
		/*
		** Special case handling for table_key audit key 
		** This is the default so we try to handle nicely
		*/
		if (fromdv.db_datatype==DB_TABKEY_TYPE)
		{
		    DB_TAB_LOGKEY_INTERNAL tlk;

		    MECOPY_CONST_MACRO((PTR)fromdv.db_data,
				DB_LEN_TAB_LOGKEY,
				(PTR)&tlk);

		    /* Format high,low as 16-byte hex */
		    CVlx(tlk.tlk_high_id, detail_text);
		    CVlx(tlk.tlk_low_id, detail_text+8);
		    sxfrcb.sxf_detail_txt= detail_text;
		    sxfrcb.sxf_detail_len=16;
		}
		else
		{
		    /*
		    ** We are logging a variable size detail, so convert to
		    ** longtext. Note that there should always be a coercion
		    ** from any datatype to longtext.
		    */
		    DML_SCB	*scb;
		    ADF_CB	*adf_cb;
		    PTR		ad_errmsgp;

		    if ( rcb->rcb_xcb_ptr )
			scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;
		    else
		    {
			CS_SID	sid;

			CSget_sid(&sid);
			scb = GET_DML_SCB(sid);
		    }
		    /* Get session's ADF_CB, preserve ad_errmsgp */
		    adf_cb = (ADF_CB*)scb->scb_adf_cb;
		    ad_errmsgp = adf_cb->adf_errcb.ad_errmsgp;
		    adf_cb->adf_errcb.ad_errmsgp = NULL;

		    todv.db_datatype=DB_LTXT_TYPE;
		    todv.db_length=SXF_DETAIL_TXT_LEN;
		    todv.db_data= detail_text;
		    status=adc_cvinto(adf_cb, &fromdv, &todv);
		    adf_cb->adf_errcb.ad_errmsgp = ad_errmsgp;
		    if(status!=E_DB_OK)
		    {
			/*
			** Log ADF error.
			*/
			(VOID) uleFormat(&adf_cb->adf_errcb.ad_dberror, 0,
			    NULL, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
			SETDBERR(dberr, 0, E_DM910E_ROW_SEC_AUDIT_ERROR);
			break;
		    }
		    text_ptr=(DB_TEXT_STRING*)detail_text;
		    sxfrcb.sxf_detail_txt= (char*)&text_ptr->db_t_text[0];
		    sxfrcb.sxf_detail_len=text_ptr->db_t_count;
		}

	}
	else
	{
		sxfrcb.sxf_detail_len=0;
		sxfrcb.sxf_auditevent=0;  /* No audit event */
	}

	*cmp_res=DMA_ACC_PRIV;

	/*
	** If a C2 server security audit the operation
	*/
	if(c2server)
	{
		/*
		** Check if we are writing an audit record. 
		** Log all key/tid lookups, and also all successful accesses
		** of tables which are audited, and all insert/update/deletes
		*/
		if((*cmp_res== DMA_ACC_PRIV && 
		  (access & DMA_ACC_SELECT) !=0 &&
		  (access &(DMA_ACC_BYTID | DMA_ACC_BYKEY))==0) ||
		  !(tcb->tcb_rel.relstat2&TCB_ROW_AUDIT))
			break;
		/*
		** Call SXF to write the audit record
		*/
		if(access&DMA_ACC_BYTID)
			sxfrcb.sxf_msg_id = I_SX2723_RECBYTID_ACCESS;
		else if (access&DMA_ACC_BYKEY)
			sxfrcb.sxf_msg_id = I_SX2724_RECBYKEY_ACCESS;
		else
			sxfrcb.sxf_msg_id = I_SX2722_RECORD_ACCESS;

		if(*cmp_res==DMA_ACC_PRIV)
			sxfrcb.sxf_accessmask|=SXF_A_SUCCESS;
		else
			sxfrcb.sxf_accessmask|=SXF_A_FAIL;

		sxfrcb.sxf_accessmask &= ~SXF_A_REGRADE;

		status=sxf_call(SXR_WRITE, &sxfrcb);

		if(status!=E_DB_OK)
		{
			/*
			** SXF Error, log SXF error then break out. This is
			** a fatal error in a B1 server.
			*/
			(VOID) uleFormat(&sxfrcb.sxf_error, 0, NULL, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
			(VOID) uleFormat(NULL, E_DM910E_ROW_SEC_AUDIT_ERROR, NULL, 
				ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, 
				&local_error, 0);
			SETDBERR(dberr, 0, E_DM0126_ERROR_WRITING_SECAUDIT);
			break;
		}
	}
	break;
    }
    return (status);
}
