/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm1c.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm0p.h>

/**
**
**  Name: DMVESM2CONFIG.C - The recovery of a sysmod config file operation.
**
**  Description:
**	This file contains the routine for the recovery of a sysmod config
**	file operation.
**
**          dmve_sm2 _config - The recovery of a sysmod config file operation.
**
**
**  History:
**      11-nov-86 (jennifer)    
**          Created new for Jupiter.
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	22-jan-90 (rogerk)
**	    Changed to handle sysmod of iirelation and iirel_idx together
**	    since sysmod of iirelation always does iirel_idx also - and
**	    both operations are logged with a single dmve_sm2 log record.
**	17-may-90 (bryanp)
**	    After updating config file, make backup copy of file. Note that
**	    we do not need to make a backup in the DO case, since rollforward
**	    will make a backup of the config file at the successful end of
**	    rolling forward the DB. 
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Added rollforward support for sysmod.
**	25-sep-1991 (mikem) integrated following change: 22-jul-1991 (bryanp)
**	    B38740: Add UNDO support for sysmod of iidevices.
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	07-july-1992 (jnash)
**	    DMF function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	01-dec-1992 (jnash)
**	    Reduced logging project.  Add CLR handling.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	    Fix to Sysmod rollforward.  When rollforward modify of iiindex,
**	    update the dcb_idx_tcb_ptr rather than dcb_att_tcb_ptr.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      22-May-1997 (stial01)
**          Invalidate tbio_lpageno, tbio_lalloc, so they get re-initialized
**          correctly (B78889, B52131, B57674) after SYSMOD do/undo.
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**      15-feb-1999 (nanpr01)
**          Change the hard coded constants.
**      28-sep-99 (stial01)
**          Reset tbio_cache_ix whenever tbio_page_size is re-initialized.
**      10-may-2000 (stial01)
**          Reset tbio_cache_ix for iirel_idx when tbio_page_size is re-init
**      15-jun-2000 (stial01 for jenjo02)
**          When changing page size, update hcb_cache_tcbcount's B101850
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**          Also fixed calls to dm1c_getaccessors (related 3.0 changes 440298
**          and 436445)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-jul-2001 (stial01)
**          Added dmv_upd_config, to do/undo config updates (b105376)
**	13-Dec-2002 (jenjo02)
**	    Maintain hcb_cache_tcbcount for Index TCBs as well as
**	    for base TCBs (BUG 107628).
**	20-Feb-2008 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**/

static DB_STATUS dmv_upd_config(DMVE_CB *dmve_cb,
	DMP_RELATION *oldrel,
	DMP_RELATION *newrel,
	DMP_RELATION *oldidx,
	DMP_RELATION *newidx);


/*{
** Name: dmve_sm2_config- The recovery of a sysmod config file operation.
**
** Description:
**	This function performs the recovery of the sysmod config file 
**      update operation.  This is to make the config file consistent
**      with the old system relation since we are going back from a sysmod.
**	In the case of UNDO, reads the config file, if update has been
**      made, then it undoes it otherwise it just closes and continues.
**
**	For UNDO operation on 'iidevices', no special config file operations
**	are needed, since iidevices is not specially replicated in the config
**	file. Instead, we merely need to purge the iidevices TCB if it exists,
**	in order to cause it to be rebuilt the next time iidevices is
**	referenced.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The rename file log record.
**	    .dmve_action	    Should be DMVE_UNDO, DMVE_REDO or DMVE_DO.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**
** Outputs:
**	dmve_cb
**	    .dmve_error.err_code    The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-nov-86 (jennifer)
**          Created for Jupiter.
**	22-jan-90 (rogerk)
**	    Changed to handle sysmod of iirelation and iirel_idx together
**	    since sysmod of iirelation always does iirel_idx also - and
**	    both operations are logged with a single dmve_sm2 log record.
**	17-may-90 (bryanp)
**	    After updating config file, make backup copy of file. Note that
**	    we do not need to make a backup in the DO case, since rollforward
**	    will make a backup of the config file at the successful end of
**	    rolling forward the DB. 
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    B38527: Added rollforward support for sysmod.
**	25-sep-1991 (mikem) integrated following change: 22-jul-1991 (bryanp)
**	    B38740: Add UNDO support for sysmod of iidevices.
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Only update config file if undoing modify of iirelation.
**	01-dec-1992 (jnash)
**	    Reduced logging project.  Add CLR handling.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      22-May-1997 (stial01)
**          Invalidate tbio_lpageno, tbio_lalloc, so they get re-initialized
**          correctly (B78889, B52131, B57674) after SYSMOD do/undo.
**      28-may-1998 (stial01)
**          dmve_sm2_config() Support VPS system catalogs.
**          Added dm0p_close_page call to the undo processing of DM0LSM2CONFIG
**          The page size can change and the page flushes must be done
**          using the correct page size in the tcb and tbio.
*/
DB_STATUS
dmve_sm2_config(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    i4		error = E_DB_OK;
    DM0L_SM2_CONFIG	*log_rec = (DM0L_SM2_CONFIG*)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->sm2_header.lsn; 
    LG_LSN		lsn;
    DI_IO		di_file;
    i4		filelength;
    DB_TAB_ID           *tbl_id;
    i4             phys_length;
    DM_PATH             *phys_location;
    DMP_RELATION        cnf_relation;
    DMP_DCB             *dcb;
    DM0C_CNF		*config = 0;
    i4             compare;
    i4             lock_list;
    DMP_TCB             *t;
    i4		recovery_action;
    i4		dm0l_flags;

    CLRDBERR(&dmve->dmve_error);
	
    for (;;)
    {
	if (log_rec->sm2_header.length != sizeof(DM0L_SM2_CONFIG) || 
	    log_rec->sm2_header.type != DM0LSM2CONFIG ||
	    dmve->dmve_action == DMVE_REDO)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	tbl_id = &log_rec->sm2_tbl_id;
	dcb = dmve->dmve_dcb_ptr;
	lock_list = dmve->dmve_lk_id; 

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.  
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->sm2_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
	{
	case DMVE_UNDO:

	    /*
	    ** Write CLR if necessary
	    */
	    if ( (dmve->dmve_logging) &&
	         ((log_rec->sm2_header.flags & DM0L_CLR) == 0) )
	    {
		dm0l_flags = (log_rec->sm2_header.flags | DM0L_CLR);

		status = dm0l_sm2_config(dmve->dmve_log_id, dm0l_flags, 
		    &log_rec->sm2_tbl_id, &log_rec->sm2_oldrel, 
		    &log_rec->sm2_newrel, &log_rec->sm2_oldidx,
		    &log_rec->sm2_newidx, log_lsn, &lsn, &dmve->dmve_error);
		if (status != E_DB_OK)
		{
		    /* XXXX Better error message and continue after logging. */
		    TRdisplay(
			"dmve_sm2_config: dm0l_sm2_config error, status: %d, error: %d\n", 
			status, dmve->dmve_error.err_code);
		    /*
		     * Bug56702: return logfull indication.
		     */
		    dmve->dmve_logfull = dmve->dmve_error.err_code;
		    break;
		}
	    }

	    /* undo: replace newrel with oldrel */
	    status = dmv_upd_config(dmve,
		&log_rec->sm2_newrel, &log_rec->sm2_oldrel,
		&log_rec->sm2_newidx, &log_rec->sm2_oldidx);
	    break;
	    
	case DMVE_DO:
	    /* redo: replace oldrel with newrel */
	    status = dmv_upd_config(dmve,
		&log_rec->sm2_oldrel, &log_rec->sm2_newrel,
		&log_rec->sm2_oldidx, &log_rec->sm2_newidx);
	    break;    

	} /* end switch. */
	if (status != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    break;
	}
	return(E_DB_OK);

    } /* end for. */
    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9612_DMVE_SM2CONFIG);
    }
    return(status);
}


/*
** Name: dmv_upd_config	- redo/undo a SM2_CONFIG operation
**
** Description:
**	This routine is used as part of redo/undo a sysmod operation.
**
**	The database config file (aaaaaaaa.cnf) contains a copy of the
**	iirelation and iiattribute tuples for the 4 core catalogs (iirelation,
**	iirel_idx, iiattribute, and iiindex). Therefore, whenever one of
**	these catalogs is modified, the corresponding iirelation tuple in the
**	config file must be modified. In addition, since these 4 catalogs
**	are never closed and rebuilt as long as the database remains open,
**	the in-memory TCB copy of the iirelation tuple must also be modified.
**
**	A DM0L_SM2_CONFIG log record is written to describe these actions;
**	this routine is called to perform redo or undo actions.
**
**	One SM2_CONFIG log record is written for each core catalog modify,
**	with the exception of iirelation/iirel_idx: both core catalog modifies
**	result in only one DM0L_SM2_CONFIG log record.
**
**	To process this operation, we do the following:
**
**		1) open the config file using the DM0C_PARTIAL flag.
**		2) locate the appropriate TCB and update the in-memory copy
**		   of the iirelation tuple in the TCB using the information
**		   from the log record.
**		3) locate the appropriate section of the config file and
**		   update the config file copy of the iirelation tuple using
**		   the information from the log record.
**		4) close the config file, updating it and making a backup copy.
**
**	If this routine is called for 'iidevices', the 5th core catalog, then
**	the config file update is a no-op, since iidevices is not specially
**	replicated in the config file. However, we still refresh the iidevices
**	in-memory TCB information, just in case it exists, by opening the
**	iidevices table and then close-purging it, thus causing the TCB to
**	be rebuilt the next time iidevices is opened.
**
**	Note that the close-purge technique cannot be used for the 4 true core
**	catalogs since DMF is incapable of rebuilding their TCBs on demand.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The rename file log record.
**           oldrel
**           newrel
**           oldidx
**           newidx
**
** Outputs:
**	dmve_cb
**	    .dmve_error.err_code    The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** History:
**	25-sep-1991 (mikem) integrated following change: 16-jul-1991 (bryanp)
**	    Created as part of fixing B38527.
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Only update config file if redoing modify of iirelation.
**	23-aug-1993 (rogerk)
**	    Fix to Sysmod rollforward.  When rollforward modify of iiindex,
**	    update the dcb_idx_tcb_ptr rather than dcb_att_tcb_ptr.
**      22-May-1997 (stial01)
**          Invalidate tbio_lpageno, tbio_lalloc, so they get re-initialized
**          correctly (B78889, B52131, B57674) after SYSMOD do/undo.
**      28-may-1998 (stial01)
**          dmv_do_sm2_config() Support VPS system catalogs.
**      30-jul-2001 (stial01)
**          Cloned from dmv_do_sm2_config, Generalized for do/undo operations
*/
static DB_STATUS
dmv_upd_config(
DMVE_CB		*dmve_cb,
DMP_RELATION    *oldrel,
DMP_RELATION    *newrel,
DMP_RELATION    *oldidx,
DMP_RELATION    *newidx
)
{
    DMVE_CB		*dmve = dmve_cb;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status = E_DB_OK;
    i4			error = E_DB_OK;
    i4			local_error;
    DM0L_SM2_CONFIG	*log_rec = (DM0L_SM2_CONFIG*)dmve_cb->dmve_log_rec;
    DB_TAB_ID           *tbl_id;
    DMP_DCB             *dcb;
    DM0C_CNF		*config = 0;
    DMP_TCB             *t;
    i4			lock_list;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);

	
    for (;;)
    {
	tbl_id = &log_rec->sm2_tbl_id;
	dcb = dmve->dmve_dcb_ptr;
	lock_list = dmve->dmve_lk_id; 

	/*
	** Handle iidevices specially, since it's considerably different
	** than the other 4 core catalogs.
	*/
	if (tbl_id->db_tab_base > 4)
	{
	    /*
	    ** Forcibly flush the in-memory TCB information, forcing
	    ** it to be rebuilt the next time the iidevices table is referenced.
	    */
	    status = dm2t_purge_tcb(dcb, tbl_id, DM2T_PURGE,
				dmve->dmve_log_id, dmve->dmve_lk_id,
				&dmve->dmve_tran_id, dmve->dmve_db_lockmode,
				&dmve->dmve_error);
	    if (status)
		break;

	    return (E_DB_OK);
	}

	/* open the config file using the DM0C_PARTIAL flag */
	status = dm0c_open( dcb, DM0C_PARTIAL, lock_list, &config, &dmve->dmve_error);
	if (status)
	    break;

	/*
	** Get the appropriate tcb pointer in the dcb
	**
	** The iirelation and iirel_idx records are both handled by a single
	** DM0L_SM2_CONFIG log record.
	*/
	if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	    t = dcb->dcb_rel_tcb_ptr;
	else if (tbl_id->db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
	    t = dcb->dcb_att_tcb_ptr;
	else if (tbl_id->db_tab_base == DM_B_INDEX_TAB_ID)
	    t = dcb->dcb_idx_tcb_ptr;

	/* Update in-memory TCB */
	STRUCT_ASSIGN_MACRO(*newrel, t->tcb_rel);

	/* Invalidate stale tbio fields (B78889, B52131, B57674) */
	t->tcb_table_io.tbio_lpageno = -1;
	t->tcb_table_io.tbio_lalloc = -1;
	t->tcb_table_io.tbio_checkeof = TRUE;

	if (oldrel->relpgsize != newrel->relpgsize ||
		oldrel->relpgtype != newrel->relpgtype)
	{
	    /*
	    ** We MUST flush any pages left in buffer cache for
	    ** this table before we change tbio_page_size
	    */
	    status = dm0p_close_page(t, dmve->dmve_lk_id, dmve->dmve_log_id,
			    DM0P_CLOSE, &dmve->dmve_error);

	    if (oldrel->relpgsize != newrel->relpgsize)
	    {
		/* Update hcb_cache_tcbcount for base and index TCBs */
		dm0s_mlock(&dmf_svcb->svcb_hcb_ptr->hcb_tq_mutex);
		dmf_svcb->svcb_hcb_ptr->hcb_cache_tcbcount[DM_CACHE_IX(oldrel->relpgsize)]--;
		dmf_svcb->svcb_hcb_ptr->hcb_cache_tcbcount[DM_CACHE_IX(newrel->relpgsize)]++;
		dm0s_munlock(&dmf_svcb->svcb_hcb_ptr->hcb_tq_mutex);
	    }

	    t->tcb_table_io.tbio_page_size = newrel->relpgsize;
	    t->tcb_table_io.tbio_cache_ix = DM_CACHE_IX(newrel->relpgsize);
	    t->tcb_table_io.tbio_page_type = newrel->relpgtype;

	    /* Update the set of accessors (may have changed).
	    ** Don't need to fool with rowaccessors because core catalogs
	    ** are never compressed or row-versioned.
	    */
	    dm1c_get_plv(newrel->relpgtype, &t->tcb_acc_plv);
	}

	if (tbl_id->db_tab_base == DM_B_RELATION_TAB_ID)
	{
	    /* 
	    ** Iirelation and iirel_idx are always modified together.
	    ** Hence update the TCB's for iirel_idx as well.
	    */
	    STRUCT_ASSIGN_MACRO(*newidx, t->tcb_iq_next->tcb_rel);

	    /* Invalidate stale tbio fields (B78889, B52131, B57674) */
	    t->tcb_iq_next->tcb_table_io.tbio_lpageno = -1;
	    t->tcb_iq_next->tcb_table_io.tbio_lalloc = -1;
	    t->tcb_iq_next->tcb_table_io.tbio_checkeof = TRUE;

	    if (oldidx->relpgsize != newidx->relpgsize ||
		oldidx->relpgtype != newidx->relpgtype)
	    {
		/*
		** We MUST flush any pages left in buffer cache for
		** this table before we change tbio_page_size
		*/
		status = dm0p_close_page(t->tcb_iq_next,
			    dmve->dmve_lk_id, dmve->dmve_log_id,
			    DM0P_CLOSE, &dmve->dmve_error);

		t->tcb_iq_next->tcb_table_io.tbio_page_size 
			= newidx->relpgsize;
		t->tcb_iq_next->tcb_table_io.tbio_cache_ix = 
			DM_CACHE_IX(newidx->relpgsize);
		t->tcb_iq_next->tcb_table_io.tbio_page_type = newidx->relpgtype;

		/* Update the set of accessors (may have changed).
		** Don't need to fool with rowaccessors because core catalogs
		** are never compressed or row-versioned.
		*/
		dm1c_get_plv(newidx->relpgtype, &t->tcb_iq_next->tcb_acc_plv);
	    }

	    /* Update relprim in config file for iirelation. */
	    config->cnf_dsc->dsc_iirel_relprim = newrel->relprim;
	    config->cnf_dsc->dsc_iirel_relpgsize = newrel->relpgsize;
	    config->cnf_dsc->dsc_iirel_relpgtype = newrel->relpgtype;
	}
	else if (tbl_id->db_tab_base == 3)
	{
	    config->cnf_dsc->dsc_iiatt_relpgsize = newrel->relpgsize;
	    config->cnf_dsc->dsc_iiatt_relpgtype = newrel->relpgtype;
	}
	else if (tbl_id->db_tab_base == 4)
	{
	    config->cnf_dsc->dsc_iiind_relpgsize = newrel->relpgsize;
	    config->cnf_dsc->dsc_iiind_relpgtype = newrel->relpgtype;
	}

	/*  close the config file, updating it and making a backup copy */
	status = dm0c_close(config, DM0C_UPDATE | DM0C_COPY, &dmve->dmve_error);
	if (status)
	    break;
	config = 0;

	return (E_DB_OK);
    }

    /*
    ** Error recovery. If we opened the config file, close it, but don't
    ** bother making a backup copy.
    */

    if (config)
	_VOID_ dm0c_close(config, DM0C_UPDATE, &local_dberr);

    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9612_DMVE_SM2CONFIG);
    }

    return (status);
}
