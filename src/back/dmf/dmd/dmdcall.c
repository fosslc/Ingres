/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1b.h>
#include    <dmccb.h>
#include    <dmdcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <dm0m.h>
#include    <dmftrace.h>
#include    <dm0l.h>
#include    <dmd.h>
#include    <dmmcb.h>


/**
**
**  Name: DMDCALL.C - Call interface debugging routines.
**
**  Description:
**      This module contains routines to perform the call interface tracing.
**
**          dmd_call	- Call parameter tracing
**	    dmd_return	- Return parameter tracing.
**	    dmd_timer	- Start function timing.
**
**  History:
**      30-jan-1987 (Derek)
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added tracing for DMC_WRITE_BEHIND calls.
**	12-feb-1990 (rogerk)
**	    Added DMR_ALTER entry point.
**	20-feb-1991 (bryanp)
**	    Change the trace for dm1202 error code tracing to reflect the
**	    correct error code values.
**      23-oct-1991 (jnash)
**          In dmd_return, remove LINT detected error parameter on 
**	    dm0m_verify call.  Elsewhere, fill out parameter list to
**          dmd_timer also to make LINT smile.
**	07-july-1992 (jnash)
**	    Add DMF function prototypes
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	18-oct-1993 (jnash)
**	    Add DMC_RESET_EFF_USER_ID dmd_call() trace.
**	08-oct-93 (johnst/tad)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**	31-jan-1994 (jnash)
**	    Fix lint detected problem: uninitialized err_code passed to 
**	    dmd_timer().
**	15-jul-1996 (ramra01/bryanp)
**	    Trace statement support for DMU_ALTER_TABLE.
**	14-May-1998 (jenjo02)
**	    Replaced DMC_WRITE_BEHIND with DMC_START_WB_THREADS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      13-mar-2002 (stial01)
**          Added more TRdisplays 
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**/


/*{
** Name: dmd_call	- Display call parameters.
**
** Description:
**      This routine displays call parameters for each call.
**
** Inputs:
**      operation                       Operation code.
**      cb                              Control block.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jan-1987 (Derek)
**          Created for Jupiter.
**	12-feb-1990 (rogerk)
**	    Added DMR_ALTER entry point.
**      23-oct-1991 (jnash)
**          Pass zeroes as parameters to dmd_timer to satisfy LINT,
**	    and remove err_code parameter in call to dm0m_verify.
**      18-oct-1993 (jnash)
**          Add DMC_RESET_EFF_USER_ID trace.
**	31-jan-1994 (mikem)
**	    Sir #58499
**	    Adding logging of interesting arguments (table name, id, owner)
**	    to call logging.  This goes along with a change to dmf_call() to
**	    make this tracing available in non-xDEBUG servers.
**	09-oct-93 (swm)
**	    Bug #56437
**	    Changed dmc_id call parameter to new dmc_session_id for
**	    DMC_BEGIN_SESSION and DMC_END_SESSION.
**      15-jul-1996 (ramra01/bryanp)
**          Trace statement support for DMU_ALTER_TABLE.
**	14-May-1998 (jenjo02)
**	    Replaced DMC_WRITE_BEHIND with DMC_START_WB_THREADS.
**      22-jan-2004 (horda03) Bug 111657/INGSRV 2677
**          Use the char array field in calls to TRdisplay. not the
**          name of the structure.
*/
VOID
dmd_call(
i4		operation,
PTR		control_block,
i4		error)
{
    DB_STATUS	    err_code;
    DMC_CB	    *dmc = (DMC_CB *)control_block;
    DMR_CB  	    *dmr_cb = (DMR_CB *) control_block;
    DMT_CB  	    *dmt_cb = (DMT_CB *) control_block;
    DMU_CB	    *dmu_cb = (DMU_CB *) control_block;
    DMX_CB	    *dmx_cb = (DMX_CB *) control_block;
    DMT_SHW_CB      *dmt_show_cb = (DMT_SHW_CB *) control_block;
    DMP_RCB         *rcb;
    i4		    i;

    if (dmf_svcb == 0)
	return;

    if (DMZ_CLL_MACRO(20))
	(VOID) dm0m_verify(DM0M_WRITEABLE);

    if (error || DMZ_CLL_MACRO(1) || DMZ_CLL_MACRO(3))
    {
	TRdisplay("DMF CALL TRACE: DM%16w\n",
",,,,,,,C_RESET_USERID,C_START_WB_THREADS,C_FAST_COMMIT,C_ALTER,C_SHOW,\
C_START,C_STOP,C_BEGIN,C_END,C_OPEN,\
C_CLOSE,C_ADD,C_DEL,,,,,,,,,,,R_DELETE,R_GET,R_POSITION,R_PUT,\
R_REPLACE,R_LOAD,R_DUMP,R_ALTER,,,,,,,,,,,,,T_ALTER,T_CLOSE,T_CREATE,\
T_DELETE,T_OPEN,T_SHOW,T_COST,,,,U_CREATE,U_DESTROY,U_INDEX,\
U_MODIFY,U_ALTER,U_SHOW,,,,,X_ABORT,X_BEGIN,X_COMMIT,X_PHASE1,\
X_SAVEPOINT,X_RESUME,M_CREATE,M_DESTROY,M_ALTER,A_WRITE", operation);
    }

    if (error)
    {
	/* Hex dump control block */
	TRdisplay("ERROR %d\n", error);
	for (i = 0; i < dmc->length; i+= 32)
	    TRdisplay("\t 0x%p:%< %8.4{%x %}%2< >%32.1{%c%}<\n",
		(char *)dmc + i, 0);
    }

    if (error || DMZ_CLL_MACRO(3))
    {
	switch (operation)
	{
	case DMC_START_SERVER:
	    TRdisplay("\tOp_type = %DMC_%w_OP Server_id = %x Server_name = %1.#{%c%}\n",
		",SERVER,SESSION,DATABASE", dmc->dmc_op_type, dmc->dmc_id,
		dmc->dmc_name.data_in_size, dmc->dmc_name.data_address);
	    if (dmc->dmc_char_array.data_address)
		TRdisplay("\tCharacteristics:%#.#{\t\tDMC_C_%w = %d\n%}",
		    sizeof(DMC_CHAR_ENTRY), 
                    dmc->dmc_char_array.data_in_size / sizeof(DMC_CHAR_ENTRY),
		    ",SESSION,DATABASE,MEMORY,TRAN,BUFFERS,GBUFFERS,GCOUNT,TCB_HASH", &((DMT_CHAR_ENTRY*)0)->char_id, &((DMT_CHAR_ENTRY*)0)->char_value);
	    break;

	case DMC_STOP_SERVER:
	    TRdisplay("\tOp_type = %DMC_%w_OP Server_id = %x\n",
		",SERVER,SESSION,DATABASE", dmc->dmc_op_type, dmc->dmc_id);
	    break;

	case DMC_FAST_COMMIT:
	    TRdisplay("\tOp_type = %DMC_%w_OP Server_id = %x\n",
		",SERVER,SESSION,DATABASE", dmc->dmc_op_type, dmc->dmc_id);
	    break;

	case DMC_START_WB_THREADS:
	    TRdisplay("\tOp_type = %DMC_%w_OP Server_id = %x\n",
		",SERVER,SESSION,DATABASE", dmc->dmc_op_type, dmc->dmc_id);
	    break;

	case DMC_BEGIN_SESSION:
	    TRdisplay("\tOp_type = DMC_%w_OP Server_id = %p Session_id = %p\n",
		",SERVER,SESSION,DATABASE", 
                dmc->dmc_op_type, dmc->dmc_server, dmc->dmc_session_id);
	    break;

	case DMC_END_SESSION:
	    TRdisplay("\tOp_type = DMC_%w_OP Session_id = %p\n",
		",SERVER,SESSION,DATABASE", dmc->dmc_op_type,
		dmc->dmc_session_id);
	    break;

	case DMC_ADD_DB:
	case DMC_DEL_DB:
	case DMC_OPEN_DB:
	case DMC_CLOSE_DB:
	case DMC_RESET_EFF_USER_ID:
	    break;

	case DMX_BEGIN:
	case DMX_COMMIT:
	case DMX_SAVEPOINT:
	case DMX_ABORT:
	    break;

	case DMT_OPEN:
	    TRdisplay("\t table id (%d, %d)\n", 
		dmt_cb->dmt_id.db_tab_base,
		dmt_cb->dmt_id.db_tab_index);
	    break;

	case DMT_CLOSE:
	    rcb = (DMP_RCB *)dmt_cb->dmt_record_access_id;
	    TRdisplay("\t table name (%~t), id (%d, %d), owner (%~t)\n", 
		sizeof(rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name), 
		rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name, 
		rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base,
		rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_index,
		sizeof(rcb->rcb_tcb_ptr->tcb_rel.relowner.db_own_name), 
		rcb->rcb_tcb_ptr->tcb_rel.relowner.db_own_name);
	    break;

	case DMT_SHOW:
	    if (dmt_show_cb->dmt_flags_mask & DMT_M_NAME)
	    {
		TRdisplay("\t table name (%~t), owner (%~t)\n", 
		    sizeof(dmt_show_cb->dmt_name.db_tab_name),
		    dmt_show_cb->dmt_name.db_tab_name,
		    sizeof(dmt_show_cb->dmt_owner.db_own_name),
		    dmt_show_cb->dmt_owner.db_own_name);
	    }
	    else if (dmt_show_cb->dmt_flags_mask & DMT_M_ACCESS_ID)
	    {
		rcb = (DMP_RCB *)dmt_show_cb->dmt_record_access_id;
		TRdisplay("\t table name (%~t), id (%d, %d), owner (%~t)\n", 
		    sizeof(rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name), 
		    rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name, 
		    rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base,
		    rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_index,
		    sizeof(rcb->rcb_tcb_ptr->tcb_rel.relowner.db_own_name), 
		    rcb->rcb_tcb_ptr->tcb_rel.relowner.db_own_name);
	    }
	    break;

	case DMR_GET:
	case DMR_PUT:
	case DMR_REPLACE:
	case DMR_DELETE:
	case DMR_POSITION:
	case DMR_ALTER:
	    rcb = (DMP_RCB *)dmr_cb->dmr_access_id;
	    TRdisplay("\t table name (%~t), id (%d, %d), owner (%~t)\n", 
		sizeof(rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name), 
		rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name, 
		rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base,
		rcb->rcb_tcb_ptr->tcb_rel.reltid.db_tab_index,
		sizeof(rcb->rcb_tcb_ptr->tcb_rel.relowner.db_own_name), 
		rcb->rcb_tcb_ptr->tcb_rel.relowner.db_own_name);
	    break;

	case DMU_CREATE_TABLE:
	case DMU_DESTROY_TABLE:
	case DMU_INDEX_TABLE:
	case DMU_MODIFY_TABLE:
	case DMU_RELOCATE_TABLE:
	case DMU_ALTER_TABLE:		
	    break;
	}
    }

    if (DMZ_CLL_MACRO(10))
	dmd_timer(0,0,0);
}

/*{
** Name: dmd_return 	- Display return parameters.
**
** Description:
**      This routine displays return parameters for each call.
**
** Inputs:
**      operation                       Operation code.
**      cb                              Control block.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jan-1987 (Derek)
**          Created for Jupiter.
**	12-feb-1990 (rogerk)
**	    Added DMR_ALTER entry point.
**	20-feb-1991 (bryanp)
**	    Change the trace for dm1202 error code tracing to reflect the
**	    correct error code values.
**      23-oct-1991 (jnash)
**          Remote LINT detected error param on dm0m_verify call, add
**          LINT detected err_code parameter to dmd_timer call.
**	18-oct-1993 (jnash)
**	    Add DMC_RESET_EFF_USER_ID.
**	31-jan-1994 (jnash)
**	    Fix lint detected problem: uninitialized err_code passed to 
**	    dmd_timer().
*/
VOID
dmd_return(
i4             operation,
PTR		    control_block,
i4		    status)
{
    i4	    err_code;
    DMC_CB	    *dmc = (DMC_CB *)control_block;

    if (dmf_svcb == 0)
	return;

    if (DMZ_CLL_MACRO(10))
	dmd_timer(1, operation, (i4)0);

    if (DMZ_CLL_MACRO(20))
    {
	/*	Check integrity and write protect the DMF memory pool. */

	dm0m_verify((DM0M_READONLY | DM0M_POOL_CHECK));
    }

    if (DMZ_CLL_MACRO(4))
    {
	switch (operation)
	{
	case DMC_START_SERVER:
	    TRdisplay("\tServer_id = %p\n", dmc->dmc_server);
	    break;

	case DMC_STOP_SERVER:
	case DMC_BEGIN_SESSION:
	case DMC_END_SESSION:
	case DMC_DEL_DB:
	case DMC_CLOSE_DB:
	case DMC_FAST_COMMIT:
	case DMC_START_WB_THREADS:
	case DMC_RESET_EFF_USER_ID:
	    break;

	case DMC_ADD_DB:
	case DMC_OPEN_DB:
	    break;

	case DMX_BEGIN:
	case DMX_COMMIT:
	case DMX_SAVEPOINT:
	case DMX_ABORT:
	    break;

	case DMT_OPEN:
	case DMT_CLOSE:
	case DMT_SHOW:
	    break;


	case DMR_GET:
	case DMR_PUT:
	case DMR_REPLACE:
	case DMR_DELETE:
	case DMR_POSITION:
	case DMR_ALTER:
	    break;

	case DMU_CREATE_TABLE:
	case DMU_DESTROY_TABLE:
	case DMU_INDEX_TABLE:
	case DMU_MODIFY_TABLE:
	case DMU_RELOCATE_TABLE:
	    break;
	}
    }


    if (DMZ_CLL_MACRO(2))
    {
	TRdisplay("DMF RETURN TRACE: DM%16w status = %6w err_code = %x\n",
",,,,,,,C_RESET_USERID,C_WRITE_BEHIND,C_FAST_COMMIT,C_ALTER,C_SHOW,C_START,\
C_STOP,C_BEGIN,C_END,C_OPEN,\
C_CLOSE,C_ADD,C_DEL,,,,,,,,,,,R_DELETE,R_GET,R_POSITION,R_PUT,\
R_REPLACE,R_LOAD,R_DUMP,R_ALTER,,,,,,,,,,,,,T_ALTER,T_CLOSE,T_CREATE,\
T_DELETE,T_OPEN,T_SHOW,T_COST,,,,U_CREATE,U_DESTROY,U_INDEX,\
U_MODIFY,U_ALTER,U_SHOW,,,,,X_ABORT,X_BEGIN,X_COMMIT,X_PHASE1,\
X_SAVEPOINT,X_RESUME,M_CREATE,M_DESTROY,M_ALTER,A_WRITE", operation,
"OK,1,INFO,3,WARN,ERROR,6,SEVERE,8,FATAL", status,
	((DMC_CB *)control_block)->error.err_code);
    }

    if (DMZ_CLL_MACRO(11))
	dmd_timer(2, operation, ((DMC_CB *)control_block)->error.err_code);
}

/*{
** Name: dmd_timer	- Call timer control.
**
** Description:
**      Start a timed section of code.
**
** Inputs:
**	type				The type of tiemr operation:
**					0 : Start timer,
**					1 : Stop timer,
**					2 : Display timer.
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jan-1987 (Derek)
**          Created for Jupiter.
**	18-oct-1993 (jnash)
**	    Add DMC_RESET_EFF_USER_ID.
*/
VOID
dmd_timer(
i4		    type,
i4		    operation,
i4		    error)
{
    i4		    i;
    static i4	    cpu, dio, bio;
    static i4	    tot_cpu, tot_dio, tot_bio, tot_cnt;
    static struct
    {
	i4	    cpu;
	i4	    dio;
	i4	    bio;
	i4	    cnt;
    }			    call_hist[DM_NEXT_OP];

    switch (type)
    {
    case 0:
#ifdef VMS
	lib$init_timer();
#endif
	return;

    case 1:
#ifdef VMS
	i = 2; 	lib$stat_timer(&i, &cpu);
	i++; lib$stat_timer(&i, &bio);
	i++; lib$stat_timer(&i, &dio);
	call_hist[operation].cpu += cpu;
	call_hist[operation].bio += bio;
	call_hist[operation].dio += dio;
	call_hist[operation].cnt++;
	tot_cnt++;
	tot_cpu += cpu;
	tot_bio += bio;
	tot_dio += dio;
#endif
	return;

    case 2:
	TRdisplay("DMF CALL TIMING: DM%12w cpu=%6d dio=%5d bio=%5d error=%x\n",
",,,,,,,C_RESET_USERID,C_WRITE_BEHIND,C_FAST_COMMIT,C_ALTER,C_SHOW,C_START,\
C_STOP,C_BEGIN,C_END,C_OPEN,\
C_CLOSE,C_ADD,C_DEL,,,,,,,,,,,R_DELETE,R_GET,R_POSITION,R_PUT,\
R_REPLACE,R_LOAD,R_DUMP,,,,,,,,,,,,,,T_ALTER,T_CLOSE,T_CREATE,\
T_DELETE,T_OPEN,T_SHOW,T_COST,,,,U_CREATE,U_DESTROY,U_INDEX,\
U_MODIFY,U_ALTER,U_SHOW,U_TABID,,,,X_ABORT,X_BEGIN,X_COMMIT,X_PHASE1,\
X_SAVEPOINT,X_RESUME,M_CREATE,M_DESTROY,M_ALTER,A_WRITE", operation,
cpu * 10, dio, bio, error);
	return;

    case 3:
	TRdisplay("DMF CALL HISTOGRAM    COUNT%6* CPU%15* DIO%15* BIO\n%90*-\n");
	for (i = 0; i < DM_NEXT_OP; i++)
	{
	    if (call_hist[i].cnt)
		TRdisplay("%4* DM%12w %8d %8d:%8.3f %8d:%8.3f %8d:%8.3f\n",
",,,,,,,C_RESET_USERID,C_WRITE_BEHIND,C_FAST_COMMIT,C_ALTER,C_SHOW,C_START,\
C_STOP,C_BEGIN,C_END,C_OPEN,\
C_CLOSE,C_ADD,C_DEL,,,,,,,,,,,R_DELETE,R_GET,R_POSITION,R_PUT,\
R_REPLACE,R_LOAD,R_DUMP,,,,,,,,,,,,,,T_ALTER,T_CLOSE,T_CREATE,\
T_DELETE,T_OPEN,T_SHOW,T_COST,,,,U_CREATE,U_DESTROY,U_INDEX,\
U_MODIFY,U_ALTER,U_SHOW,,,,,X_ABORT,X_BEGIN,X_COMMIT,X_PHASE1,\
X_SAVEPOINT,X_RESUME,M_CREATE,M_DESTROY,M_ALTER,A_WRITE",
	    i, 
	    call_hist[i].cnt,
	    call_hist[i].cpu * 10, 
                  (call_hist[i].cpu * 10) / (double)call_hist[i].cnt,
	    call_hist[i].dio, call_hist[i].dio / (double)call_hist[i].cnt,
	    call_hist[i].bio, call_hist[i].bio / (double)call_hist[i].cnt);
	}
	TRdisplay("%90*-\n%4* TOTAL          %8d %8d%10* %8d%10* %8d\n",
		tot_cnt, tot_cpu * 10, tot_dio, tot_bio);
	return;
    }
}

