/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <adf.h>
#include    <scf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <lg.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm0m.h>
#include    <dm0s.h>
#include    <dm2t.h>
#include	<dmfgw.h>
/**
** Name: DMCBEGIN.C - Functions used to begin a session.
**
** Description:
**      This file contains the functions necessary to begin a session.
**      This file defines:
**    
**      dmc_begin_session() -  Routine to perform the begin 
**                             session operation.
**
** History:
**      01-sep-1985 (jennifer)
**          Created for Jupiter.      
**	18-nov-1985 (derek)
**	    Completed code for dmc_begin_session().
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	19-nov-1990 (rogerk)
**	    Obtain Front End session's timezone from the session's adf cb.
**	    DMF will use this timezone for all DML operations for this session.
**	    This was done as part of the DBMS Timezone changes.
**	6-jun-1991 (rachael,bryanp)
**	    Add error message e_dm9c07_out_of_locklists as part of
**	    error message project.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**      24-sep-1992 (stevet)
**          Replaced _timezone with _tzcb to support new timezone 
**          adjustment method.
**	21-nov-1992 (bryanp)
**	    Log LG/LK Error codes.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	17-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Initialize dml_dcb.scb_dbxlate from dmc_cb.dmc_dbxlate.
**	    Initialize dml_dcb.scb_cat_owner from dmc_cb.dmc_cat_owner.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**       5-Nov-1993 (fred)
**          Initialized new scb_lo_{next,prev} queue header.
**	09-oct-93 (swm)
**	    Bug 56437
**	    Get scf_session_id from new dmc_session_id rather than dmc_id.
**	31-jan-1994 (bryanp) B57826, B57827
**	    Log scf_error.err_code if scf_call fails.
**	    Log dm0m_allocate return code if dm0m_allocate fails.
**      04-apr-1995 (dilma04) 
**          Initialize scb_isolation_level.
**	17-Jan-1997 (jenjo02)
**	    Replaced svcb_mutex with svcb_dq_mutex (DCB queue) and
**	    svcb_sq_mutex (SCB queue).
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for SCB, manually
**	    initialize remaining fields instead.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Initialize locking characteristics: scb_timeout, scb_maxlocks,
**          scb_lock_level, scb_sess_iso and scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added SET SESSION <access mode>.
**	    Initialize scb_tran_am.
**      14-oct-97 (stial01)
**          dmc_begin_session() Added session readlock: scb_sess_readlock,
**      12-nov-97 (stial01)
**          dmc_begin_session() Init scb_tran_iso = DMC_C_SYSTEM;
**	31-Mar-1997 (jenjo02)
**	    One should not assume that ADF has been started for this session,
**	    hence test the value returned for adf_cb.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LKcreate_list() prototype.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-mar-2001 (stial01)
**          Init scb_global_lo (b104317)
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      08-nov-2004 (stial01)
**          Init session dop from server svcb_dop
**      09-feb-2007 (stial01)
**          Moved locator context from DML_XCB to DML_SCB
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**/

/*{
** Name: dmc_begin_session - Begins a session.
**
**  INTERNAL DMF call format:    status = dmc_begin_session(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_BEGIN_SESSION,&dmc_cb);
**
** Description:
**    The dmc_begin_session function handles the initialization of a session.
**    A session is a connection of one user to the database system.  This is a
**    user thread to the database server.  This command causes DMF to create
**    an execution environment for a user.  The environment is used to keep 
**    track of resources and objects currently associated with a user.  This
**    includes such things as transaction progress, tables open, position 
**    within table for retrieves, etc.  A session identifer is returned to
**    the caller and must be used in all calls DMF which alter the 
**    session environment such as open database, begin transaction, etc.
**
**    As part of establishing the session's environment, DMF will obtain
**    the session's ADF control block in order to find the session's timezone.
**    This ADF control block must have already been allocated and initialized.
**
**    When DMF performs internal actions (such as system catalog lookups,
**    sorts, recovery ...) it will act as though it were running in the
**    timezone in which the DBMS server runs.  However, when DMF searches 
**    tables as part of a user requested DML statement, it will act as though 
**    it were running in the timezone of the Front End session.  This is
**    done by changing the timezone in the ADF control block used by
**    the session (in the RCB) at dmt_open time.
**
**    NOTE:
**    Theoretically, the database structures that DMF builds and searches 
**    should be uneffected by session specific variables such as timezones.
**    Failure to adhere to this rule means that structures built by one 
**    session may not be properly searched by another with different session 
**    settings.  This rule is currently broken by Timezone semantics: ADF 
**    comparisons may differ depending on the timezone value in the session's
**    ADF control block.  We think that in the future, DMF should not make use
**    of session level values but should instead only use parameters tied to 
**    the database itself (as is done with collation sequences).  To do this 
**    would have database conversion and backward compatability implications,
**    however.  See timezone notes in dmf doc directory for more information.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_SERVER_OP.
**          .dmc_session_id		    Must be the SCF session id.
**	    .dmc_server			    Value return by DMC_START_SERVER.
**          .dmc_name                       Must be zero or a unique
**                                          session name.
**          .dmc_s_type                     Must be DMC_S_NORMAL or
**                                          DMC_S_SYSUPDATE, DMC_S_AUDIT
**                                          and or
**                                          DMC_S_SECURITY. Used to 
**                                          indicate type of session.
**          .dmc_flag_mask                  Must be zero, DMC_DEBUG, or
**                                          DMC_NODEBUG.
**	    .dmc_dbxlate		    Case translation flags.
**	    .dmc_cat_owner		    Catalog owner name.
**          .dmc_char_array                 Pointer to an area used to input
**                                          an array of characteristics 
**                                          entries of type DMC_CHAR_ENTRY.
**                                          See below for description of
**                                          <dmc_char_array> entries.
**
**          <dmc_char_array> entries are of type DMC_CHAR_ENTRY and
**          must have the following values:
**          char_id                         Must be one of following:
**          char_value                      Must be an integer value or
**                                          one of the following:
**                                          DMC_C_ON
**                                          DMC_C_OFF
**                                          DMC_C_READ_ONLY
**
** Output:
**      dmc_cb 
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000D_BAD_CHAR_ID
**                                          E_DM000E_BAD_CHAR_VALUE
**                                          E_DM000F_BAD_DB_ACCESS_MODE
**                                          E_DM0011_BAD_DB_NAME
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002D_BAD_SERVER_ID
**                                          E_DM002F_BAD_SESSION_ID
**                                          E_DM0041_DB_QUOTA_EXCEEDED
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0053_NONEXISTENT_DB
**                                          E_DM005C_SESSION_QUOTA_EXCEEDED
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM0084_ERROR_ADDING_DB
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**          .error.err_data                 Set to characteristic in error 
**                                          by returning index into 
**                                          characteristic array.
**	dml_scb
**	    .scb_dbxlate		    Copied from dmc_cb.dmc_dbxlate.
**	    .scb_cat_owner		    Copied from dmc_cb.dmc_cat_owner.
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	18-nov-85 (derek)
**	    Completed code for dmc_begin_session().
**	 8-dec-88 (rogerk)
**	    Make sure we return a legal error status when we encounter a lock
**	    error.
**	 1-Feb-89 (ac)
**	    Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Call to gateway to establish
**	    connection.
**	19-nov-1990 (rogerk)
**	    Obtain Front End session's timezone from the session's adf cb.
**	    DMF will use this timezone for all DML operations for this session.
**	    This was done as part of the DBMS Timezone changes.
**	6-jun-1991 (rachael,bryanp)
**	    Add error message e_dm9c07_out_of_locklists as part of
**	    error message project.
**      24-sep-1992 (stevet)
**          Replaced _timezone with _tzcb to support new timezone 
**          adjustment method.
**	04-nov-92 (jrb)
**	    Initialize scb_loc_masks to NULL.
**	21-nov-1992 (bryanp)
**	    Log LG/LK Error codes.
**	17-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Initialize dml_dcb.scb_dbxlate from dmc_cb.dmc_dbxlate.
**	    Initialize dml_dcb.scb_cat_owner from dmc_cb.dmc_cat_owner.
**       5-Nov-1993 (fred)
**          Initialized new scb_lo_{next,prev} queue header.
**	31-jan-1994 (bryanp) B57826, B57827
**	    Log scf_error.err_code if scf_call fails.
**	    Log dm0m_allocate return code if dm0m_allocate fails.
**      04-apr-1995 (dilma04)
**          Initialize scb_isolation_level.
**	21-Jan-1997 (jenjo02)
**	    Remove DM0M_ZERO from dm0m_allocate() for SCB, manually
**	    initialize remaining fields instead.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Initialize locking characteristics: scb_timeout, scb_maxlocks,
**          scb_lock_level, scb_sess_iso and scb_tran_iso.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added SET SESSION <access mode>.
**	    Initialize scb_tran_am.
**      14-oct-97 (stial01)
**          dmc_begin_session() Added session readlock: scb_sess_readlock,
**      12-nov-97 (stial01)
**          dmc_begin_session() Init scb_tran_iso = DMC_C_SYSTEM;
**	08-Jan-2001 (jenjo02)
**	    Get *DML_SCB with macro. DML_SCB now allocated by SCF
**	    as part of large SCF SCB, no longer separately by
**	    dm0m_allocate() in here. *ADF_CB, *GWF_SESSION now
**	    passed in DMC_CB so DMF need not call back to SCF
**	    to get them.
**	12-apr-2004 (jenjo02)
**	    Init (new) SCB scb_dop.
**	05-May-2004 (jenjo02)
**	    If Factotum thread, don't allocate scb_lock_list.
**	10-May-2004 (schka24)
**	    Init blob-cleanup list mutex.
**	21-Jun-2004 (schka24)
**	    Reverse direction and get rid of blob-cleanup list entirely.
**	22-Jul-2004 (schka24)
**	    Record factotum-ness of session in scb type.
**	10-Aug-2004 (jenjo02)
**	    Init scb_length, scb_type, so that dm0m_check()
**	    won't poop.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	14-Apr-2008 (kschendel)
**	    Init new qualification-function members.
**	9-sep-2009 (stephenb)
**	    Initialize scb_last_id.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Init BQCB list, mutex.
*/

DB_STATUS
dmc_begin_session(
DMC_CB    *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    DM_SVCB		*svcb = (DM_SVCB *)dmc->dmc_server;
    DML_SCB		*scb;
    DMC_CHAR_ENTRY	*chr;
    i4		chr_count;
    i4		i;
    DB_STATUS		status;
    i4		local_error;
    CL_ERR_DESC		sys_err;
    SCF_CB              scf_cb;
    SCF_SCI             sci_list[3]; 
    ADF_CB		*adf_cb;
    char		sem_name[7+16+10+1];	/* +10 is slop */

    svcb->svcb_stat.ses_begin++;

    CLRDBERR(&dmc->error);

    for (status = E_DB_ERROR;;)
    {
	/*  Check control block parameters. */

	if (dmc->dmc_op_type != DMC_SESSION_OP)
	{
	    SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
	    break;
	}
	if ((dmc->dmc_flags_mask & ~(DMC_DEBUG | DMC_NODEBUG)) ||
	    (dmc->dmc_flags_mask & (DMC_DEBUG | DMC_NODEBUG)) ==
		(DMC_DEBUG | DMC_NODEBUG))
	{
	    SETDBERR(&dmc->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}
	if (dmc->dmc_name.data_address && dmc->dmc_name.data_in_size == 0)
	{
	    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}
	if (dmf_svcb == NULL)
	    dmf_svcb = svcb;
	if (dmc->dmc_char_array.data_address)
	{
	    chr = (DMC_CHAR_ENTRY *)dmc->dmc_char_array.data_address;
	    chr_count = dmc->dmc_char_array.data_in_size / 
                          sizeof(DMC_CHAR_ENTRY);
	    for (i = 0; i < chr_count; i++)
		switch (chr[i].char_id)
		{
		default:
		    break;
		}
	    if (i == 0 || i != chr_count)
	    {
		SETDBERR(&dmc->error, i, E_DM000D_BAD_CHAR_ID);
		break;
	    }
	}

	/* Get pointer to DML_SCB embedded in SCF's SCB */
	scb = GET_DML_SCB(dmc->dmc_session_id);
	scb->scb_sid = (CS_SID)dmc->dmc_session_id;

	/* Get the SCF session information. */

        scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_session = (SCF_SESSION)scb->scb_sid;
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
        scf_cb.scf_len_union.scf_ilength = 2;

	sci_list[0].sci_length = sizeof(scb->scb_real_user);
	sci_list[0].sci_code = SCI_RUSERNAME;
	sci_list[0].sci_aresult = (char *)&scb->scb_real_user;
	sci_list[0].sci_rlength = 0;

	sci_list[1].sci_length = sizeof(scb->scb_dis_tranid);
	sci_list[1].sci_code = SCI_DISTRANID;
	sci_list[1].sci_aresult = (char *)&scb->scb_dis_tranid;
	sci_list[1].sci_rlength = 0;

	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    uleFormat(&scf_cb.scf_error, 0, NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, 0);
	    SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
	    break;
	}

	/*	Initialize the SCB. */
	scb->scb_last_id.intval = 0;
	scb->scb_length = sizeof(DML_SCB);
	scb->scb_type = SCB_CB;
	scb->scb_ascii_id = SCB_ASCII_ID;

	if (dmc->dmc_name.data_address)
	    MEmove(dmc->dmc_name.data_in_size, 
                dmc->dmc_name.data_address, ' ', sizeof(scb->scb_user), 
                (char *)&scb->scb_user);
	else
	    MEfill(sizeof(scb->scb_user), ' ', (char *)&scb->scb_user);
	    
	scb->scb_oq_next = (DML_ODCB*) &scb->scb_oq_next;
	scb->scb_oq_prev = (DML_ODCB*) &scb->scb_oq_next;
	scb->scb_x_next = (DML_XCB*) &scb->scb_x_next;
	scb->scb_x_prev = (DML_XCB*) &scb->scb_x_next;
	scb->scb_kq_next = (DML_SLCB*) &scb->scb_kq_next;
	scb->scb_kq_prev = (DML_SLCB*) &scb->scb_kq_next;
	scb->scb_s_type = SCB_S_NORMAL;
	scb->scb_ui_state = 0;
	scb->scb_state = 0;
	scb->scb_loc_masks = NULL;
	scb->scb_audit_state = SCB_A_FALSE;
	if (dmc->dmc_s_type & DMC_S_AUDIT && svcb->svcb_status & SVCB_C2SECURE)
	    scb->scb_audit_state = SCB_A_TRUE;
	if (dmc->dmc_s_type & DMC_S_SYSUPDATE)
	    scb->scb_s_type |= SCB_S_SYSUPDATE;
	if (dmc->dmc_s_type & DMC_S_SECURITY)
	    scb->scb_s_type |= SCB_S_SECURITY;
	if (dmc->dmc_s_type & DMC_S_FACTOTUM)
	    scb->scb_s_type |= SCB_S_FACTOTUM;
	scb->scb_dbxlate = dmc->dmc_dbxlate;
	scb->scb_cat_owner = dmc->dmc_cat_owner;
        scb->scb_timeout = DMC_C_SYSTEM;
        scb->scb_maxlocks = DMC_C_SYSTEM;
        scb->scb_lock_level = DMC_C_SYSTEM;
        scb->scb_sess_iso = DMC_C_SYSTEM;
	scb->scb_readlock = DMC_C_SYSTEM;
        scb->scb_tran_iso = DMC_C_SYSTEM;
	scb->scb_sess_am = scb->scb_tran_am = 0;
	scb->scb_bqcb_next = NULL;
	/* Really only need the mutex for "normal" scb's, but the session
	** type can be altered.  Just init the mutex.
	*/
	dm0s_minit(&scb->scb_bqcb_mutex, "BQCB");

	/* Init default degree of parallelism */
	scb->scb_dop = svcb->svcb_dop;

	/* Initialize remaining SCB fields */
	scb->scb_x_ref_count = 0;
	scb->scb_o_ref_count = 0;
	scb->scb_x_max_tran = 0;
	scb->scb_o_max_opendb = 0;
	scb->scb_sess_mask = 0;
	scb->scb_gw_session_id = (PTR)NULL;
	MEfill(sizeof(scb->scb_trace), 0, &scb->scb_trace);
	scb->scb_lloc_cxt = NULL;
	scb->scb_qfun_adfcb = NULL;
	scb->scb_qfun_errptr = NULL;

	/*
	** Just behind our DML_SCB may be a SCF-preallocated
	** chunk of memory of size svcb_st_ialloc
	** which we'll use as the initial ShortTerm
	** memory pool for this session.
	*/
	dm0m_init_scb(scb);

	/*
	**  Create a session lock list, but not if Factotum.
	**
	**  If Factotum, we'll connect to the SHARED session
	**  lock list if and when a transaction is begun.
	*/

	if ( dmc->dmc_s_type & DMC_S_FACTOTUM )
	{
	    scb->scb_lock_list = 0;
	}
	else
	{
	    status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
		(i4)0, (LK_UNIQUE *)0, (LK_LLID *)&scb->scb_lock_list, 0,
		&sys_err);
	    if (status != OK)
	    {
		uleFormat( NULL, status, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, 
			&local_error, 0);
		uleFormat( NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG , NULL,
			(char * )NULL, 0L, (i4 *)NULL,
			&local_error, 0);
		if (status == LK_NOLOCKS)
		{
		    uleFormat(NULL, E_DM9C07_OUT_OF_LOCKLISTS, &sys_err, ULE_LOG, NULL,
			(char * )NULL, 0L, (i4 *)NULL, &local_error, 0);
		    SETDBERR(&dmc->error, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		}
		else
		    SETDBERR(&dmc->error, 0, E_DM0105_ERROR_BEGIN_SESSION);
		status = E_DB_ERROR;
		break;
	    }
	    /* Non-factotum thread needs the blob cleanup hint */
	    scb->scb_global_lo = FALSE;  /* session temp etab(s) created? */
	}

	/*
	** Get pointer to the session's ADF control block,if any, and extract
	** the session's timezone value.  Note that a cleaner interface
	** would probably be for SCF to pass down the session's timezone
	** as a parameter in the dmc_char_array to this routine - however,
	** this change is meant to be somewhat temporary and it is not
	** felt that DMF should have require knowledge about the timezone
	** in which the user session runs.
	*/
	/* Most of scf_cb is initialized from above. */

	/* Save pointer to ADF_CB in session's SCB */
	scb->scb_adf_cb = (ADF_CB *) dmc->dmc_adf_cb;

	/*
	** If ADF has been started for this session,
	** store user session's timezone in the dmf session control block.
	** User DML operations will use this value to enable them to
	** behave as though they were running in that session's timezone.
	*/
	if ( (adf_cb = (ADF_CB*)scb->scb_adf_cb) )
	    scb->scb_tzcb = adf_cb->adf_tzcb;

	/*
	** Establish Gateway connection and get gateway session id.
	*/
	if (svcb->svcb_gateway)
	{
	    scb->scb_gw_session_id = dmc->dmc_gwf_cb;
	    scb->scb_sess_mask |= SCB_GATEWAY_CONNECT;
	}

	/*  Insert SCB onto SVCB queue of active sessions. */
	
	dm0s_mlock(&svcb->svcb_sq_mutex);
	scb->scb_q_next = svcb->svcb_s_next;
	scb->scb_q_prev = (DML_SCB*) &svcb->svcb_s_next;
	svcb->svcb_s_next->scb_q_prev = scb;
	svcb->svcb_s_next = scb;
        svcb->svcb_cnt.cur_session++;
	dm0s_munlock(&svcb->svcb_sq_mutex);
	return (E_DB_OK);
    }

    if (dmc->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmc->error, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &local_error, 0);
	SETDBERR(&dmc->error, 0, E_DM0105_ERROR_BEGIN_SESSION);
    }
    return (status);
}
