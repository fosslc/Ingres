/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <sl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0c.h>
#include    <dm0s.h>
#include    <dm2d.h>
#include    <dm0llctx.h>

/**
**
**  Name: DMVEJNLEOF.C - The recovery of a journal eof operation.
**
**  Description:
**	This file contains the routine for the recovery of a journal eof 
**	operation.
**
**          dmve_jnleof - The recovery of a journal eof operation.
**
**
**  History:
**      22-may-87 (Derek)    
**          Created for Jupiter.
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	26-June-89 (ac)
**	    Fixed error handling problem to make sure this routine work.
**	17-may-90 (bryanp)
**	    After updating config file, make a backup copy.
**	4-jun-1990 (bryanp)
**	    Update to previous fix -- must do complete read of config file to
**	    find dump location, since dcb information is incomplete.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	30-feb-1993 (rmuth)
**	    Include di.h for dm0c.h.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	18-oct-1993 (rmuth)
**	    CL prototype, add <me.h>.
**	26-Oct-1996 (jenjo02)
**	    Added semaphore name to dm0s_minit().
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**      14-jul-2003
**          Added include adf.h
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*{
** Name: dmve_jnleof - The recovery of a journal eof operation.
**
** Description:
**	This function undoes the effects of a aborted journal eof
**	operation.  This consists of resetting the oldest the journal
**	eof pointer in the conifguration file for the database.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The set begin of log file log record.
**	    .dmve_action	    Should only be DMVE_UNDO.
**	    .dmve_lg_addr	    The log address of the log record.
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
**	22-may-87 (Derek)
**          Created for Jupiter.
**	14-Feb-89 (ac)
**          Initialize dcb_status to make sure the config is locked while
**	    update the config file.
**	17-may-90 (bryanp)
**	    After updating config file, make a backup copy.
**	4-jun-1990 (bryanp)
**	    In order for dm0c_close() to make a backup copy of the config file,
**	    it must be able to find the dump location. In order to find the
**	    dump location, it must read in the entire config file, not just
**	    the header information. Changed "DM0C_PARTIAL" to "0" in the
**	    dm0c_open() call.
**	19-Nov-2008 (jonj)
**	    Fixed fat-fingered "uleformat" to "uleFormat".
*/
DB_STATUS
dmve_jnleof(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_JNLEOF		*log_rec = (DM0L_JNLEOF*)dmve_cb->dmve_log_rec;
    LG_PURGE		purge;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    DB_STATUS		close_status;
    DB_STATUS		error;
    DM0C_CNF		*cnf;
    DM0C_CNF		*config;
    DMP_DCB		dcb;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
    if (log_rec->je_header.length != sizeof(DM0L_JNLEOF) || 
	log_rec->je_header.type != DM0LJNLEOF||
	dmve->dmve_action != DMVE_UNDO)
    {
	uleFormat(NULL, E_DM9601_DMVE_BAD_PARAMETER, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9619_DMVE_JNLEOF);
	return (E_DB_ERROR);
    }

    /*	Fill in a temporary DCB. */

    MEfill(sizeof(dcb), 0, &dcb);
    dcb.dcb_location.physical = log_rec->je_root;
    dcb.dcb_location.phys_length = log_rec->je_l_root;
    dcb.dcb_name = log_rec->je_dbname;
    dcb.dcb_db_owner = log_rec->je_dbowner;
    dcb.dcb_access_mode = DCB_A_READ;
    dcb.dcb_served = DCB_MULTIPLE;
    dcb.dcb_db_type = DCB_PUBLIC;
    dcb.dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
    dcb.dcb_status = 0;

    dm0s_minit(&dcb.dcb_mutex, "DCB mutex");
    
    /*	Open and update the configuration file. */

    status = dm0c_open(&dcb, 0, dmve->dmve_lk_id, &cnf, &dmve->dmve_error);

    if (status == E_DB_OK)
    {
	config = cnf;

	if (log_rec->je_node < 0)
	{
	    /*	Update journal eof for this database. */

	    config->cnf_jnl->jnl_blk_seq = log_rec->je_newseq;
	    config->cnf_jnl->jnl_la = log_rec->je_new_f_cp;
	}
	else
	{
	    config->cnf_jnl->jnl_node_info[log_rec->je_node].cnode_blk_seq 
                                  = log_rec->je_newseq;
	    config->cnf_jnl->jnl_node_info[log_rec->je_node].cnode_la 
				  = log_rec->je_new_f_cp;
	}

	/*  Close the configuration file. */

	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_COPY, &dmve->dmve_error);

	if (status == E_DB_OK)
	    return(E_DB_OK);
    }
    if (dmve->dmve_error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmve->dmve_error, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	SETDBERR(&dmve->dmve_error, 0, E_DM9619_DMVE_JNLEOF);
    }
    return (E_DB_ERROR);
}
