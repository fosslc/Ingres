/*
** Copyright (c) 1986, 2006 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>
#include    <cv.h>
#include    <cs.h>
#include    <lk.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <gca.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <sc0e.h>
#include    <scc.h>
#include    <scfscf.h>
#include    <scs.h>
#include    <scd.h>
#include    <sce.h>
#include    <scu.h>
#include    <scfcontrol.h>
#include    "scfint.h"
#ifdef VMS
#include <lib$routines.h>
#include <ssdef.h>
#endif

/**
**
**  Name: scfcall.c - Provide standard entry point to SCF
**
**  Description:
**      This file contains the entry point level services of SCF.
**
**          scf_call - Main entry point to the System Control Facility
**
**
**  History: 
**      26-Mar-86 (fred)    
**          Created on Jupiter
**	23-Mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added support for SCU_XENCODE opcode
**	04-may-89 (neil)
**	    Added new entry SCC_RSERROR for RAISE ERROR statement.
**	11-apr-1990 (fred)
**	    Added support for SCS_{SET,CLR}_ACTIVITY
**	10-dec-1990 (neil)
**	    Alerters: new SCE entry points to manage events.
**	    New trace points 921-930 for maintaining events.
**	02-jul-91 (ralph)
**	    remove scu_sfree support.
**	    remove unneeded references to scu_swait() and scu_srelease().
**	    replace call to scu_sinit() with inline call to CSi_semaphore().
**	11-jul-1991 (ralph)
**	    Fix Intergraph bug -- assignment used where conditional s/b used
**	    Fix ICL bug -- use EX_MAX_SYS_REP as extent of array a_vio_msg[].
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**      13-jul-92 (rog)
**          Included er.h because of a change to scs.h.
**	23-oct-92 (daveb)
**	    Name the semaphores.
**	17-nov-1992 (bryanp)
**	    Check return code from scfinitdbg().
**	14-dec-92 (jhahn)
**	    Added tracepoint SC912
**	    (to turn off byref protocol).
**	12-Mar-1993 (daveb)
**	    Add include <st.h> <me.h>
**	29-Jun-1993 (daveb)
**	    include <tr.h>
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototype scf_call().  Remove internal FUNC_EXTERNs,
**	    pull them from exported headers.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**      31-jan-1994 (mikem)
**          Reordered #include of <cs.h> to before <lk.h>.  <lk.h> now
**          references a CS_SID and needs <cs.h> to be be included before
**          it.
**      07-apr-1995 (kch)
**          In the function scf_call(), cb->scf_error.errr_code is now set
**          to status if status = E_CS0017_SMPR_DEADLOCK. Previously,
**          status was set to E_SC0028_SEMAPHORE_DUPLICATE if status =
**          E_CS0017_SMPR_DEADLOCK.
**          This change fixes bug 65445.
**	25-Apr-1995 (jenjo02)
**	    Combined CSi_semaphore and CSn_semaphore into
**	    single CSw_semaphore function call.
**      14-jun-1995 (chech02)
**          Added SCF_misc_sem semaphore to protect critical sections.(MCT)
**      5-dec-96 (stephenb)
**          Add performance profiling code
**	5-jun-96 (stephenb)
**	    Add case SCU_DBADDUSER and SCU_DBDELUSER for calls from DMF
**	    replicator queue management thread.
**	30-jan-97 (stephenb)
**	    Add case SCD_INITSNGL to init single user servers (called from DMF).
**	21-may-1997 (shero03)
**	    Added facility tracing diagnostic
**	05-aug-1997 (canor01)
**	    Add SCU_SDESTROY to remove semaphore.
**	12-jan-98 (stephenb)
**	    Add trace point SC914 to re-start replicator queue management 
**	    threads.
**	09-Mar-1998 (jenjo02)
**	    Added support for Factotum threads.
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSadd_thread() prototype.
**	24-Sep-1999 (hanch04)
**	    Call ERmsg_hdr to format error message header.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    If caller has passed scf_session and it's not DB_NOSESSION,
**	    trust that it's a valid SCB pointer rather than doing yet
**	    another CSget_scb() to double check.
**	29-may-2002 (devjo01)
**	    Add support for alerters in clusters. (SCE_RAISE).
**      10-jun-2003 (stial01)
**          Added trace point sc924 (ule_doformat will TRdisplay query)
**	30-nov-2005 (abbjo03)
**	    Remove globavalue SS$_DEBUG declaration which causes unexplained
**	    compilation errors.
**	18-jan-2006 (abbjo03)
**	    Fix 30-nov-2005 change.  Need to include VMS headers (indirectly
**	    included by pthread.h).
**	30-Nov-2006 (kschendel) b122041
**	    Fix ult_check_macro usage.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new forms of sc0ePut(), uleFormat().
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**      02-sep-2010 (maspa05) SIRs 124345, 124346
**          extra SC930 options including QEP SEGMENTED and QEP FULL (original
**          format). Added SC930 to SC1000 output (SC trace point help)
**          set notrace point sc930 now turns off sc930
**          ult_set_always_trace uses bitmask now
**	06-sep-2010 (maspa05) SIR 124363
**	    Add trace point sc925 for logging long-running queries
**      08-sep-2010 (maspa05) SIR 124345
**          SC930 2 should switch on tracing when run by itself
**/


/*
**  Forward references.
*/
# ifdef xDEBUG
static STATUS scf_handler( EX_ARGS *ex_args );
# endif


/*
** Definition of all global variables owned by this file.
*/

/*
** The server CB should be zero filled on entry;  in all status fields,
** zero (0) means uninitialized.
*/
GLOBALREF SC_MAIN_CB          *Sc_main_cb; /* The server's server cb */

/*{
** Name: scf_call	- client entry point for the system control facility
**
** Description:
**      This routine is the entry point for those clients wishing to 
**      invoke the service of scf.  It is through this call that information, 
**      semaphore, and memory services are requested. 
**        
**      This routine validates the "global" parameters (such as facility, session, 
**      cb type, cb size, etc).  It then invokes the appropriate sub function
**      to perform the service for the client.
**
** Inputs:
**      operation                       the operation requested by the client
**      cb                              the SCF_CB which fully describes the
**					    requested action
**
** Outputs:
**      scf_error                       filled in as appropriate
**      --- for details, see the individual operations.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-Mar-86 (fred)
**          Created on Jupiter.
**	23-Mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added support for SCU_XENCODE opcode.
**	04-may-89 (neil)
**	    Added new entry SCC_RSERROR for RAISE ERROR statement.
**	11-apr-1990 (fred)
**	    Added support for SCS_{SET,CLR}_ACTIVITY
**	06-jul-1990 (sandyh)
**	    Added sc911 trace point for memory usage tracing
**	02-jul-91 (ralph)
**	    remove scu_sfree support.
**	    remove unneeded references to scu_swait() and scu_srelease().
**	    replace call to scu_sinit() with inline call to CSi_semaphore().
**	11-jul-1991 (ralph)
**	    Fix Intergraph bug -- assignment used where conditional s/b used
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              16-Jan-89 (alexh)
**                  added SCC_RELAY case
**          End of STAR comments.
**	23-oct-92 (daveb)
**	    Name the semaphores.
**	17-nov-1992 (bryanp)
**	    Check return code from scfinitdbg().
**	2-Jul-1993 (daveb)
**	    prototype scf_call()
**	6-jul-93 (shailaja)
**	    Fixed prototype incompatibility.
**	16-Jul-1993 (daveb)
**	    Fix some sc0e_put's inside xDEBUG, revealed on VMS.
**	08-sep-93 (swm)
**	    Changed type of sid from i4 and CS_SID to reflect recent
**	    CL interface specification and cast relevant values in
**	    comparisons to CS_SID.
**      07-apr-1995 (kch)
**          The variable cb->scf_error.errr_code is now set to status if
**          status = E_CS0017_SMPR_DEADLOCK. Previously, status was set to
**          E_SC0028_SEMAPHORE_DUPLICATE if status = E_CS0017_SMPR_DEADLOCK.
**	5-jun-96 (stephenb)
**	    Add case SCU_DBADDUSER and SCU_DBDELUSER for calls from DMF
**	    replicator queue management thread.
**	30-jan-97 (stephenb)
**	    Add case SCD_INITSNGL to init single user servers (called from DMF).
**	05-aug-1997 (canor01)
**	    Add SCU_SDESTROY to remove semaphore.
**	09-Mar-1998 (jenjo02)
**	    Added support for Factotum threads.
**	    Fixed test and cast of sc_proxy_scb, which is a pointer to SCD_SCB,
**	    not a CS_SID!
**	04-Jan-2001 (jenjo02)
**	    If caller has passed scf_session and it's not DB_NOSESSION,
**	    trust that it's a valid SCB pointer rather than doing yet
**	    another CSget_scb() to double check. Pass this session's
**	    SCD_SCB* to all called functions.
**	    Push and pop session's current facility (sscb_cfac).
*/
DB_STATUS
scf_call(operation, cb)
i4            operation;
SCF_CB             *cb;
{
    DB_STATUS           status;
    SCD_SCB		*scb;
    i4			fac;
#ifdef	xDEBUG
    i4			ex_declared = 0;
    EX_CONTEXT		ex_context;
    DB_STATUS		internal_status;
#endif /* xDEBUG */
#ifdef PERF_TEST
    CSfac_stats(TRUE, CS_SCF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(TRUE, CS_SCF_ID, operation, cb);
#endif


    status = E_DB_OK;

    CLRDBERR(&cb->scf_error);

    if (!Sc_main_cb)
    {
	/* for standalone processing */
	status = scf_init_dbg();
	if (status)
	    return (status);
    }

#ifdef xDEBUG
    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_EXDECL, NULL, NULL))
    {
	ex_declared = 1;
	if (EXdeclare(scf_handler, &ex_context) != OK)
	{
	    EXdelete();
	    SETDBERR(&cb->scf_error, 0, E_SC0024_INTERNAL_ERROR);
	    return(E_DB_FATAL);
	}
    }

    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMCHECK, NULL, NULL))
    {
	/* if this is debug time, make sure we can read our memory */
	internal_status = sc0m_check(SC0M_READONLY_MASK | SC0M_WRITEABLE_MASK,
					 "scf_call entry");
	if (internal_status != E_SC_OK)
	{
	    sc0ePut(&cb->scf_error, E_SC0023_INTERNAL_MEMORY_ERROR, NULL, 0);
	    sc0ePut(NULL, internal_status, NULL, 0);
	    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_EXDECL, NULL, NULL))
	    {
		if (ex_declared)
		    EXdelete();
	    }
	    return(E_DB_FATAL);
	}
    }
#endif /* xDEBUG */

    /*
    ** first, the generic system checks    
    ** Make sure that the cb is of correct type & size,
    ** and that operation code is valid.  Make sure
    ** user has provided valid facility and session.
    ** If not, return appropriate error.
    */

    if (cb->scf_type != SCF_CB_TYPE ||
	cb->scf_length != SCF_CB_SIZE)
    {
	SETDBERR(&cb->scf_error, 0, E_SC0022_BAD_REQUEST_BLOCK);
    }
    else if (cb->scf_facility < DB_MIN_FACILITY ||
	     cb->scf_facility > DB_MAX_FACILITY)
    {
	SETDBERR(&cb->scf_error, 0, E_SC0018_BAD_FACILITY);
    }
    else if (operation < SCF_MIN_OPERATION || 
	     operation > SCF_MAX_OPERATION)
    {
	SETDBERR(&cb->scf_error, 0, E_SC0001_OP_CODE);
    }
    else
    {
	/* Trust that caller has passed a valid scf_session (CS_SCB*) */
	if ( (scb = (SCD_SCB*)cb->scf_session) == (SCD_SCB*) DB_NOSESSION )
	    CSget_scb((CS_SCB**)&scb);

	/* Admin thread's CS_SCB has no SCF component (scb_sscb) */
	if ( scb->cs_scb.cs_self != CS_ADDER_ID )
	{
	    /* Push current facility */
	    fac = scb->scb_sscb.sscb_cfac;
	    scb->scb_sscb.sscb_cfac = DB_SCF_ID;
	}

	switch (operation)
	{
	case SCU_INFORMATION:
	    status = scu_information(cb, scb);
	    break;
	
	case SCU_IDEFINE:
	    status = scu_idefine(cb, scb);
	    break;
	
	case SCU_MALLOC:
	    status = scu_malloc(cb, scb);
	    break;

	case SCU_MFREE:
	    status = scu_mfree(cb, scb);
	    break;

	case SCU_SINIT:
	    {
		char sem_name[CS_SEM_NAME_LEN];

                if (!cb->scf_ptr_union.scf_semaphore)
                {
		    SETDBERR(&cb->scf_error, 0, E_SC0002_BAD_SEMAPHORE);
                    status = E_DB_ERROR;
                    break;
                }
		STprintf( sem_name, "SCU_SINIT sem %p",
			  cb->scf_ptr_union.scf_semaphore );
		if (CSw_semaphore( cb->scf_ptr_union.scf_semaphore,
		                   CS_SEM_SINGLE, sem_name ))
		{
		    SETDBERR(&cb->scf_error, 0, E_SC0024_INTERNAL_ERROR);
		    status = E_DB_ERROR;
		}
#ifdef SCF_SINGLE_USER
		else
		{
		    cb->scf_ptr_union.scf_sngl_sem->scse_checksum
			= FREE_PHASE_1;
		}
#endif
	    }
    /*	    status = scu_sinit(cb); */
	    break;

	case SCU_SWAIT:
	    {
		i4		    term;
		i4		    type;

		if (!cb->scf_ptr_union.scf_semaphore)
		{
		    SETDBERR(&cb->scf_error, 0, E_SC0002_BAD_SEMAPHORE);
		    status = E_DB_ERROR;
		    break;
		}

		term = cb->scf_nbr_union.scf_stype & SCU_LT_MASK;
		type = cb->scf_nbr_union.scf_stype & ~SCU_LT_MASK;

		status = CSp_semaphore(type, cb->scf_ptr_union.scf_semaphore);
		if (status)
		{
		    if (status == E_CS0017_SMPR_DEADLOCK)
		    {
			cb->scf_error.err_code = status;
			status = E_DB_ERROR;
		    }
		    else
		    {
			cb->scf_error.err_code = E_SC0024_INTERNAL_ERROR;
			status = E_DB_ERROR;
		    }
		}
	    }
    /*	status = scu_swait(cb); */
	    break;

	case SCU_SRELEASE:
	    {
		if (!cb->scf_ptr_union.scf_semaphore)
		{
		    SETDBERR(&cb->scf_error, 0, E_SC0002_BAD_SEMAPHORE);
		    status = E_DB_ERROR;
		    break;
		}

		status = CSv_semaphore(cb->scf_ptr_union.scf_semaphore);
		if (status)
		{
		    SETDBERR(&cb->scf_error, 0, E_SC0012_NOT_SET);
		    status = E_DB_ERROR;
		}
	    }
    /*	status = scu_srelease(cb);  */
	    break;

	case SCU_SDESTROY:
            {
                if (!cb->scf_ptr_union.scf_semaphore)
                {
		    SETDBERR(&cb->scf_error, 0, E_SC0002_BAD_SEMAPHORE);
                    status = E_DB_ERROR;
                    break;
                }

                status = CSr_semaphore(cb->scf_ptr_union.scf_semaphore);
                if (status)
                {
		    SETDBERR(&cb->scf_error, 0, E_SC0024_INTERNAL_ERROR);
                    status = E_DB_ERROR;
                }
            }
	    break;

	case SCU_DBADDUSER:
	    status = scs_dbadduser(cb, scb);
	    break;

	case SCU_DBDELUSER:
	    status = scs_dbdeluser(cb, scb);
	    break;

	case SCS_DECLARE:
	    status = scs_declare(cb, scb);
	    break;

	case SCS_CLEAR:
	    status = scs_clear(cb, scb);
	    break;

	case SCS_DISABLE:
	    status = scs_disable(cb, scb);
	    break;

	case SCS_ENABLE:
	    status = scs_enable(cb, scb);
	    break;

	case SCC_ERROR:
	case SCC_MESSAGE:
	case SCC_WARNING:
	case SCC_RSERROR:
	    status = scc_error(operation, cb, scb);
	    break;

	case SCC_TRACE:
	    status = scc_trace(cb, scb);
	    break;

	case SCC_RCONNECT:
	case SCC_RACCEPT:
	case SCC_RDISCONNECT:
	case SCC_RWRITE:
	case SCC_RREAD:
	    SETDBERR(&cb->scf_error, 0, E_SC0025_NOT_YET_AVAILABLE);
	    status = E_DB_ERROR;
	    break;

	case SCU_XENCODE:
	    status = scu_xencode(cb, scb);
	    break;

	case SCS_SET_ACTIVITY:
	case SCS_CLR_ACTIVITY:
	    {
		i4	length = min(SCS_ACT_SIZE,
				cb->scf_len_union.scf_blength);

		if (Sc_main_cb->sc_state == SC_OPERATIONAL)
		{
		    if (operation == SCS_CLR_ACTIVITY)
			length = 0;

		    if (cb->scf_nbr_union.scf_atype == SCS_A_MAJOR)
		    {
			if (length)
			    MEcopy((PTR) cb->scf_ptr_union.scf_buffer,
				length,
				(PTR) scb->scb_sscb.sscb_ics.ics_act1);
			scb->scb_sscb.sscb_ics.ics_l_act1 = length;
		    }
		    else if (cb->scf_nbr_union.scf_atype == SCS_A_MINOR)
		    {
			if (length)
			    MEcopy((PTR) cb->scf_ptr_union.scf_buffer,
				length,
				(PTR) scb->scb_sscb.sscb_ics.ics_act2);
			scb->scb_sscb.sscb_ics.ics_l_act2 = length;
		    }
		}
		break;
	    }

	case SCE_REGISTER:
	    status = sce_register(cb, scb);
	    break;

	case SCE_REMOVE:
	    status = sce_remove(cb, scb);
	    break;
	    
	case SCE_RAISE:
	    status = sce_raise(cb, scb);
	    break;
	    
	case SCE_RESIGNAL:
	    status = sce_resignal(cb, scb);
	    break;

	case SCC_RELAY:
	    status = scc_relay(cb, scb);
	    break;

	case SCD_INITSNGL:
	    status = scd_init_sngluser(cb, scb);
	    break;

	case SCS_ATFACTOT:
	    status = scs_atfactot(cb, scb);
	    break;

	default:
	    SETDBERR(&cb->scf_error, 0, E_SC0024_INTERNAL_ERROR);
	    status = E_DB_FATAL;
	    break;
	}  /* EO switch */
		
#ifdef xDEBUG
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMCHECK, NULL, NULL))
	    {
	    /* if this is debug time, make sure no one else can write our memory */
	    if (cb->scf_facility == DB_SCF_ID)
	    {
		/*
		** Then it is just us calling us.  Don't map out our memory
		*/
		internal_status = sc0m_check(SC0M_READONLY_MASK | SC0M_WRITEABLE_MASK,
					"scf_call exit");
	    }
	    else
	    {
		internal_status = sc0m_check(SC0M_READONLY_MASK | SC0M_WRITEABLE_MASK,
						"scf_call exit");
	    }
	    if (internal_status != E_SC_OK)
	    {
		sc0ePut(&cb->scf_error, E_SC0023_INTERNAL_MEMORY_ERROR, NULL, 0);
		sc0ePut(NULL, internal_status, NULL, 0);
		status = E_DB_FATAL;
	    }
	}

	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_EXDECL, NULL, NULL))
	{
	    if (ex_declared)
		EXdelete();
	}
#endif /* xDEBUG */
#ifdef PERF_TEST
	CSfac_stats(FALSE, CS_SCF_ID);
#endif
#ifdef xDEV_TRACE
    	CSfac_trace(FALSE, CS_SCF_ID, operation, cb);
#endif
	if ( scb->cs_scb.cs_self != CS_ADDER_ID )
	{
	    /* Pop current facility */
	    scb->scb_sscb.sscb_cfac = fac;
	}

	return(status);
    }

    /* error */
#ifdef	xDEBUG
    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_EXDECL, NULL, NULL))
    {
	if (ex_declared)
	    EXdelete();
    }
#endif
    return(E_DB_ERROR);

}

/*{
** Name: scf_handler	- exception handler to trap errors
**
** Description:
**      This routine traps any unexpected exceptions which occur 
**      within the boundaries of an scf_call(), logs the error, 
**      and returns.  Scf_call() will interpret this as a fatal error.
**
** Inputs:
**      ex_args                         Arguments provided by the exception mechanism
**
** Outputs:
**      (logs an error)
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    (in response)
**
** Side Effects:
**
**
** History:
**	3-Apr-86 (fred)
**          Created on Jupiter.
**	21-may-89 (jrb)
**	    Change to ule_format interface.
**	11-jul-1991 (ralph)
**	    Fix ICL bug -- use EX_MAX_SYS_REP as extent of array a_vio_msg[].
**	2-Jul-1993 (daveb)
**	    prototype.
*/
# ifdef xDEBUG
static STATUS
scf_handler( EX_ARGS *ex_args )
{
    i4             error;
    i4		a_vio_length, junk;
    char		a_vio_msg[EX_MAX_SYS_REP];

    /* got an unexpected exception -- log it and die */

    if (EXmatch(EXSEGVIO, 1, ex_args->exarg_num))
    {
	char	    server_id[64];
	CS_SCB      *scb;

	CSget_svrid( server_id );
	CSget_scb( &scb );
	ERmsg_hdr( server_id, scb->cs_self, a_vio_msg);

	EXsys_report(ex_args, a_vio_msg);
	a_vio_length = STlength(a_vio_msg);
	uleFormat(NULL, 0, NULL, ULE_MESSAGE, NULL,
	    a_vio_msg, a_vio_length, &junk, &error, 0);
	uleFormat(NULL, E_SC0108_UNEXPECTED_EXCEPTION, NULL, ULE_LOG, NULL,
	    0, 0, 0, &error, 1, 0, ex_args->exarg_num);
    }
    else
    {
	uleFormat(NULL, E_SC0108_UNEXPECTED_EXCEPTION, &ex_args->exarg_num, ULE_LOG,
	    NULL, 0, 0, 0, &error, 5, 
	    sizeof(i4), &ex_args->exarg_array[0],
	    sizeof(i4), &ex_args->exarg_array[1],
	    sizeof(i4), &ex_args->exarg_array[2],
	    sizeof(i4), &ex_args->exarg_array[3],
	    sizeof(i4), &ex_args->exarg_array[4]);
    }
    return(EX_DECLARE);
}
# endif /* xDEBUG */

/*{
** Name: scf_trace	- Entry point for setting and clearing of trace flags
**
** Description:
**      This routine is called with the standard DB_TRACE_CB to set 
**      and/or clear trace flags for SCF.  The precise definition of these flags 
**      can be found below. 
**
**
** Inputs:
**      trace_cb                        the trace cb
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-Apr-96 (fred)
**          Created on Jupiter.
**     14-Jul-88 (rogerk)
**	    Added trace point to crash the server (sc908)
**     20-sep-88 (rogerk)
**	    Added trace point to dump server statistics (sc909)
**     31-oct-88 (rogerk)
**	    Added new argument to CSdump_statistics.
**     05-may-89 (jrb)
**	    Added new tracepoint to send scc_trace output to log.  (sc910)
**     10-dec-90 (neil)
**	    Alerters: field new trace points.
**	14-dec-92 (jhahn)
**	    Added tracepoint SC912
**	    (to turn off byref protocol).
**	2-Jul-1993 (daveb)
**	    prototyped.
**	5-dec-96 (stephenb)
**	    Add case SC913 for performance profiling
**	12-jan-98 (stephenb)
**	    Add trace point SC914 to re-start replicator queue management 
**	    threads.
**	30-jun-98 (stephenb)
**	    Add trace SC915 to start CUT tests (IIDEV_TEST only).
**	25-Jun-2008 (kibro01)
**	    Add trace point SC930 for session tracing.
**	19-Aug-2009 (kibro01) b122509
**	    Add trace point sc930 value 2 for QEP tracing as well.
**	06-sep-2010 (maspa05) SIR 124363
**	    Add trace point sc925 for logging long-running queries
**	07-sep-2010 (maspa05) SIR 124363
**	    Allow a ceiling as well as a threshold value for sc925
*/
DB_STATUS
scf_trace(DB_DEBUG_CB  *trace_cb)
{
    SCD_SCB		*scb;
    switch (trace_cb->db_trace_point)
    {
	case	0:
	/* Trace points 4-7 are reserved for alerters */
	case	SCS_TEVPRINT:
	case	SCS_TEVLOG:
	case	SCS_TALERT:
	case	SCS_TNOTIFY:
	    CSget_scb((CS_SCB **)&scb);
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		if (ult_set_macro(&scb->scb_sscb.sscb_trace,
		    (trace_cb->db_trace_point),
		    trace_cb->db_vals[0],
		    trace_cb->db_vals[1]) != E_DB_OK)
		{
		    sc0e_uput(E_SC0228_INVALID_TRACE, 0, 0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0);
		}
	    }
	    else if (trace_cb->db_trswitch == DB_TR_OFF)
	    {
		if (ult_clear_macro(&scb->scb_sscb.sscb_trace,
		    (trace_cb->db_trace_point)) != E_DB_OK)
		{
		    sc0e_uput(E_SC0228_INVALID_TRACE, 0, 0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0 );
		}
	    }
	    break;

	case	SCS_TASTATE:
	    CSget_scb((CS_SCB **)&scb);
	    if (trace_cb->db_trswitch == DB_TR_ON)
		scb->scb_sscb.sscb_aflags |= SCS_AFSTATE;
	    else if (trace_cb->db_trswitch == DB_TR_OFF)
		scb->scb_sscb.sscb_aflags &= ~SCS_AFSTATE;
    	    break;

	case	900:
	case	901:
	case	902:
	case	903:
	case	904:
	case	906:
	case	907:
	case	910:
	case	912:/* temporary */
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		if (ult_set_macro(&Sc_main_cb->sc_trace_vector,
		    (trace_cb->db_trace_point - 900),
		    trace_cb->db_vals[0],
		    trace_cb->db_vals[1]) != E_DB_OK)
		{
		    sc0e_uput(E_SC0228_INVALID_TRACE, 0, 0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0 );
		}
	    }
	    else if (trace_cb->db_trswitch == DB_TR_OFF)
	    {
		if (ult_clear_macro(&Sc_main_cb->sc_trace_vector,
		    (trace_cb->db_trace_point - 900)) != E_DB_OK)
		{
		    sc0e_uput(E_SC0228_INVALID_TRACE, 0, 0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0 );
		}
	    }
	    break;
#ifdef PERF_TEST
        case 913: /* collect cpu stats for each facility */
            if (trace_cb->db_trswitch == DB_TR_ON)
            {
                CScollect_stats(TRUE);
            }
            else
            {
                CScollect_stats(FALSE);
            }
            break;
#endif
	case 914: /* start a replicator queue management thread */
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		GCA_LS_PARMS	local_crb;
		STATUS		stat;
		CL_ERR_DESC	sys_err;

            	local_crb.gca_status = 0;
            	local_crb.gca_assoc_id = 0;
            	local_crb.gca_size_advise = 0;
            	local_crb.gca_user_name = DB_REP_QMAN_THREAD;
            	local_crb.gca_account_name = 0;
            	local_crb.gca_access_point_identifier = "NONE";
            	local_crb.gca_application_id = 0;
 
                stat = CSadd_thread(Sc_main_cb->sc_max_usr_priority, 
		    (PTR)&local_crb, SCS_REP_QMAN, (CS_SID*)NULL, &sys_err);
 
            	/* Check for error adding thread */
            	if (stat != OK)
            	{
                    sc0ePut(NULL, stat, &sys_err, 0);
                    sc0ePut(NULL, E_SC037D_REP_QMAN_ADD, NULL, 0);
	        }
	    }
	    /* 
	    ** trace_cb->db_trswitch == DB_TR_OFF...deleteing threads not
	    ** yet supported
	    */
	    break;
#ifdef IIDEV_TEST
        case 915: /* test cut */
	    if (trace_cb->db_trswitch == DB_TR_ON)
		scf_cut_test();
	    break;
#endif
#ifdef	    VMS
	case	905:
		{
		    lib$signal(SS$_DEBUG);
		    break;
		}
#endif

	case	908:
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		/* Trace Point to crash the server */
		CSterminate(CS_CRASH, 0);
	    }
	    break;

	case	909:
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		/* Trace point to dump server statistics */
		CSdump_statistics(TRdisplay);
	    }
	    break;

	case	911:	/* Trace point to dump server memory info */
	    {
		scu_mdump(Sc_main_cb->sc_mcb_list, (i4) 0);
	    }
	    break;

	/* Trace points 921-930 are reserved for events */
	case	921:
	    sce_dump(TRUE);	/* Dump server alert data structures */
	    break;
	case	922:
	    sce_dump(FALSE);	/* Dump event subsystem data structures */
	    break;
	case	923:		/* Alter event processing */
	    sce_alter(trace_cb->db_vals[0], trace_cb->db_vals[1]);
	    break;
	/* ... till 930 reserved for events */

	case	924:
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		if (trace_cb->db_value_count == 0)
		    Sc_main_cb->sc_trace_errno  = -1;
		else
		    Sc_main_cb->sc_trace_errno = trace_cb->db_vals[0];
	    }
	    else
	    {
		Sc_main_cb->sc_trace_errno = 0;
	    } 
	    break;
	    
  	case	925:
  	    if (trace_cb->db_trswitch == DB_TR_ON)
  	    {
		switch (trace_cb->db_value_count)
		{
                  case 0:
	            sc0e_trace("sc925 requires at least one parameter - threshold value");
		    break;
		  case 1:
  		    ult_set_trace_longqry(trace_cb->db_vals[0],0);
		    break;
		  default:
		    if (trace_cb->db_vals[1] <= trace_cb->db_vals[0])
	              sc0e_trace("ceiling value must exceed threshold value");
		    else
  		      ult_set_trace_longqry(trace_cb->db_vals[0],
		                            trace_cb->db_vals[1]);
		}
  	    }
  	    else
  	    {
  		ult_set_trace_longqry(0,0);
  	    } 
  	    break;
  	    
	case	930:
	    if (trace_cb->db_trswitch == DB_TR_ON)
	    {
		i4 trace_val;
		i2 v1,v2;

		v1=(trace_cb->db_value_count==0? 1:trace_cb->db_vals[0]);
		v2=(trace_cb->db_value_count==1? 0:trace_cb->db_vals[1]);

		switch (v1)
		{
		  case 0:
	            ult_set_always_trace(0,Sc_main_cb->sc_pid);
		    break;
		  case 1:
		    trace_val=ult_always_trace();
		    trace_val |= SC930_TRACE;
		    trace_val &= ~(SC930_QEP_FULL|SC930_QEP_SEG);
                    ult_set_always_trace(trace_val,Sc_main_cb->sc_pid);
		    break;
		  case 2:
		    trace_val=ult_always_trace();
		    switch (v2)
		    {
		      case 0:
		      case 1:
		        trace_val &= ~SC930_QEP_FULL;
		        trace_val |= (SC930_TRACE|SC930_QEP_SEG);
                        ult_set_always_trace(trace_val,Sc_main_cb->sc_pid);
		        break;
		      case 2:
		        trace_val |= (SC930_TRACE|SC930_QEP_FULL);
		        trace_val &= ~SC930_QEP_SEG;
                        ult_set_always_trace(trace_val,Sc_main_cb->sc_pid);
		        break;
		      case 3:
		        trace_val |= SC930_TRACE;
		        trace_val &= ~(SC930_QEP_FULL|SC930_QEP_SEG);
                        ult_set_always_trace(trace_val,Sc_main_cb->sc_pid);
		        break;
		      default:
	                sc0e_trace("invalid value for QEP style (1=Seg,2=Full,3=Off)");
		        break;
		    }
		    break;
		  default:
	            sc0e_trace("invalid sc930 parameter");
		    break;
		}
	    }
	    else
	        ult_set_always_trace(0,Sc_main_cb->sc_pid);
	    break;

	case	1000:
	    sc0e_trace("Valid SCF trace flags are:\n");
	    sc0e_trace(" 0\tPrint query text - use SET PRINTQRY\n");
	    sc0e_trace(" 1\tTrace extraordinary events\n");
	    sc0e_trace(" 2\tPrint user alert events - use SET PRINTEVENTS\n");
	    sc0e_trace(" 3\tLog user alert events - use SET LOGEVENTS\n");
	    sc0e_trace(" 4\tLog SCE requests (from QEF)\n");
	    sc0e_trace(" 5\tTrace alert notifications to terminal\n");
	    sc0e_trace(" 6\tTrace alert state transitions to terminal\n");
	    sc0e_trace(" 900\tDeclare exception handler for each utility service\n");
	    sc0e_trace(" 901\tCheck SCF memory pool on each memory op\n");
	    sc0e_trace(" 902\tWrite error messages to trace log\n");
	    sc0e_trace(" 903\tSend all logged messages to the client\n");
	    sc0e_trace(" 904\tTrace memory allocations to trace file\n");
	    sc0e_trace(" 906\tLog session startup and shutdown\n");
	    sc0e_trace(" 909\tDump server statistics\n");
	    sc0e_trace(" 911\tDump server memory info\n");
	    sc0e_trace(" 921\tDump local server event memory\n");
	    sc0e_trace(" 922\tDump cross server event memory\n");
	    sc0e_trace(" 923\tAlter SCE event processing - (help = 1000)\n");
	    sc0e_trace(" 924\tDump query before error \n");
	    sc0e_trace(" 925 x [y]\tLog long-running queries (> x, < y secs)\n");
	    sc0e_trace(" 930\tServer-based Query Tracing ");
	    sc0e_trace("\t   0\t\t- turn off");
	    sc0e_trace("\t  [1]   \t- turn on");
	    sc0e_trace("\t   2 [1]\t- include QEPs (segmented style)");
	    sc0e_trace("\t   2  2 \t- include QEPs (full style)");
	    sc0e_trace("\t   2  3 \t- turn off QEPs");
	    break;
	    
	default:
	    sc0e_uput(E_SC0228_INVALID_TRACE, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0 );
	    break;
    }
    return(E_DB_OK);
}
