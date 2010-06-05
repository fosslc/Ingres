/*
**Copyright (c)  2004 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
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
** Name: DMATBLAC.C - Security Table access checking primitives.
**
** Description:
**      This file contains the routines needed to manage security
**	Table operations:
**	- Table-level security auditing.
**
** Routines defined in this file:
**	dma_table_access  - Do table security access
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
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Definition of static variables and forward static functions.
*/

/*{
** Name: dma_table_access - Process table-level access/auditing
**
** Description:
**      This routine does table-level access/auditing. 
**
**	The two main functions are:
**	- For a C2 server, write table audit records if necessary.
**
** Inputs:
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
**	 E_DM910F_TABLE_MAC_ERROR 
**
** History:
**	26-may-1993 (robf)
**          Created 
**	15-oct-93 (robf)
**          Add handling for system catalog queries.
**	30-dec-93 (robf)
**          Initialize sxf_scb/sxf_sess_id.
**	30-mar-94 (robf)
**          Initialize sxf_db_name, row auditing may AV after
**	    Steve's recent changes. Also MEfill entire structure to
**          help protect against any future changes.
**	23-may-94 (robf)
**          Added SXF_A_CONTROL as UPDATE operation. 
*/
DB_STATUS
dma_table_access(
i4	    access,	/* Type of access to table */
DMP_RCB	    *rcb,	/* RCB for table */
i4	    *cmp_res,	/* Result of arbitration */
bool	    sec_audit,	/* Do we do security audit success or not */
DB_ERROR    *dberr)
{
    DML_SCB	    *scb;
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
    ADF_CB	    adf_cb;
    i4	    local_error;
    static i4  	    debug=0;

    CLRDBERR(dberr);

    if(debug)
	return E_DB_OK;
    scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;

    for(;;)
    {
	/*
	** default no access
	*/
	*cmp_res=DMA_ACC_NOPRIV;
	/*
	** Check if C2 server
	*/
	if (dmf_svcb->svcb_status & SVCB_C2SECURE)
		c2server=TRUE;
	else
		c2server=FALSE;

	/*
	** If not C2  then we don't need to do any more,
	** so break;
	*/
	if (!c2server)
	{
		/* Allow access */
		*cmp_res=DMA_ACC_PRIV;
		break;
	}
	/*
	** Check kind of access to table. Each type of access must be 
	** mapped to one of the four values accepted by SXF
	*/
	switch(access)
	{
	case SXF_A_SELECT:
		sxf_access=SXF_A_SELECT;
		break;
	case SXF_A_UPDATE:
	case SXF_A_MODIFY:
	case SXF_A_ALTER:
	case SXF_A_INDEX:
	case SXF_A_RELOCATE:
	case SXF_A_CONTROL:
		sxf_access=SXF_A_UPDATE;
		break;
	case SXF_A_DELETE:
	case SXF_A_DROP:
		sxf_access=SXF_A_DELETE;
		break;
	case SXF_A_INSERT:
	case SXF_A_CREATE:
		sxf_access=SXF_A_INSERT;
		break;
	default:
		SETDBERR(dberr, 0, E_DM001A_BAD_FLAG );
		status=E_DB_ERROR;
		break;
	}
	if(status!=E_DB_OK)
		break;

        MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
	sxfrcb.sxf_cb_type=SXFRCB_CB;
	sxfrcb.sxf_length=sizeof(sxfrcb);
	sxfrcb.sxf_sess_id = (CS_SID)0;
	sxfrcb.sxf_scb=NULL;
	sxfrcb.sxf_accessmask=sxf_access;
	sxfrcb.sxf_objowner= (DB_OWN_NAME*)&tcb->tcb_rel.relowner;
	sxfrcb.sxf_objname=  (char*)&tcb->tcb_rel.relid;
	sxfrcb.sxf_objlength= DB_TAB_MAXNAME;
	sxfrcb.sxf_detail_len=0;
	sxfrcb.sxf_auditevent=SXF_E_TABLE; /* Table event */
	sxfrcb.sxf_force = 0;
	sxfrcb.sxf_detail_int=0;
	sxfrcb.sxf_db_name=NULL;
	/*
	** If a B1 server call SXF to arbitrate for us.
	** We do this BEFORE auditing so result of access can be audited.
	*/
	/*
	** Check access for selects on system catalogs
	*/

	*cmp_res=DMA_ACC_PRIV;

	/*
	** If a C2 server security audit the operation 
	**
	** We always audit failures, but only audit success if caller
	** requested, since the caller may want to do further processing
	** prior to determining real "success". 
	*/
	if(c2server)
	{
		if(*cmp_res==DMA_ACC_PRIV &&  sec_audit==FALSE)
			break;

		/*
		** Call SXF to write the audit record
		*/
		sxfrcb.sxf_msg_id = I_SX2026_TABLE_ACCESS;

		sxfrcb.sxf_accessmask=access;
		if(*cmp_res==DMA_ACC_NOPRIV)
			sxfrcb.sxf_accessmask|=SXF_A_FAIL;
		else
			sxfrcb.sxf_accessmask|=SXF_A_SUCCESS;

		status=sxf_call(SXR_WRITE, &sxfrcb);

		if(status!=E_DB_OK)
		{
			/*
			** SXF Error, log SXF error then break out. This is
			** a fatal error in a B1 server.
			*/
			(VOID) uleFormat(&sxfrcb.sxf_error, 0, NULL, ULE_LOG, NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
			SETDBERR(dberr, 0, E_DM0126_ERROR_WRITING_SECAUDIT);
			break;
		}
	}
	break;
    }
    return (status);
}
