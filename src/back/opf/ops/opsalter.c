/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <uld.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPS_RALTER      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPSALTER.C - alter session or server state for optimizer
**
**  Description:
**      This routine will alter the session or server state for the optimizer
**
**  History:    
**      30-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	15-sep-98 (sarjo01)
**	    Added support for OPF_TIMEOUTABORT
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	11-Nov-2009 (kiria01) SIR 121883
**	    Pass on cardinality_check setting.
**	03-Dec-2009 (kiria01) b122952
**	    Add .opf_inlist_thresh for control of eq keys on large inlists.
**      17-Aug-2010 (horda03) b124274
**          For SET [NO]QEP [SEGMENTED] ops_qep_flag. Needed uld.h for flag
**          values.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
static i4 ops_change(
	OPF_CB *opf_cb,
	OPS_ALTER *altercb);
i4 ops_exlock(
	OPF_CB *opf_cb,
	SCF_SEMAPHORE *semaphore);
i4 ops_unlock(
	OPF_CB *opf_cb,
	SCF_SEMAPHORE *semaphore);
i4 ops_alter(
	OPF_CB *opf_cb);

/*{
** Name: ops_change	- change characteristics for SET command
**
** Description:
**      This routine will change the characteristics of an OPS_ALTER 
**      control block
**
** Inputs:
**      opf_cb                          ptr to caller's control block
**      altercb                         ptr to alter control block to update
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jul-86 (seputis)
**          initial creation
**	8-aug-89 (seputis)
**          added OPF_SUBSELECT 
**	12-jul-90 (seputis)
**	    added OPS_ARESOURCE and OPS_ANORESOURCE for b30809
**	22-jul-02 (inkdo01)
**	    Added OPF_[NO]OPTIMIZEONLY to obviate need for tp auth.
**	4-june-03 (inkdo01)
**	    Added "set [no]ojflatten" as synonym for tp op134.
**	11-june-03 (inkdo01)
**	    Added support for "set joinop [no]newenum", "set [no]hash".
**	4-mar-04 (inkdo01)
**	    Added support for "set [no]parallel [n]".
**      17-Aug-2010 (horda03) b124274
**          IF SET QEP and opf_value set then enable Segmented QEP
**          displays. disable otherwise.
**	14-Oct-2010 (kschendel) SIR 124544
**	    Delete ret-into function, not needed any more.
*/
static DB_STATUS
ops_change(
	OPF_CB             *opf_cb,
	OPS_ALTER          *altercb)
{
    i4			opcode;			/* characteristic to be changed
                                                */
    DB_DEBUG_CB		debugcb;
    opcode = opf_cb->opf_alter;
    {
	switch (opcode)
	{
	case OPF_MEMORY:
	{
	    altercb->ops_maxmemory = opf_cb->opf_value; /* new default memory */
	    break;
	}
	case OPF_INLIST_THRESH:
	{
	    altercb->ops_inlist_thresh = opf_cb->opf_value; /* new default inlist threshold */
	    break;
	}
	case OPF_TIMEOUT:
	{
	    altercb->ops_timeout = TRUE;
	    altercb->ops_sestime = opf_cb->opf_value; /* new default timeout */
	    break;
	}
	case OPF_TIMEOUTABORT:
	{
	    altercb->ops_timeout = TRUE;
	    altercb->ops_timeoutabort = opf_cb->opf_value;
	    break;
	}
	case OPF_NOTIMEOUT:
	{
	    altercb->ops_timeout = FALSE;
	    break;
	}
	case OPF_CPUFACTOR:
	{
	    altercb->ops_cpufactor = opf_cb->opf_value; /* new default weight
						** between CPU and I/O */
	    break;
	}
	case OPF_QEP:
	{
	    altercb->ops_qep = TRUE;	        /* print QEP */
            altercb->ops_qep_flag = opf_cb->opf_value ? ULD_FLAG_SEGMENTS : ULD_FLAG_NONE;
	    break;
	}
	case OPF_NOQEP:
	{
	    altercb->ops_qep = FALSE;	        /* print qep */
            altercb->ops_qep_flag = ULD_FLAG_NONE;
	    break;
	}
	case OPF_SUBSELECT:
	{
	    /* FIXME - probably should make this a session control variable
	    ** rather than using a trace flag */
	    debugcb.db_trswitch = DB_TR_ON;
	    debugcb.db_value_count = 0;     
	    debugcb.db_trace_point = OPT_GMAX + OPT_F004_FLATTEN;
	    opt_call(&debugcb);	    /* turn OFF flattening */
	    break;
	}
	case OPF_NOSUBSELECT:
	{
	    debugcb.db_trswitch = DB_TR_OFF;
	    debugcb.db_value_count = 0;     
	    debugcb.db_trace_point = OPT_GMAX + OPT_F004_FLATTEN;
	    opt_call(&debugcb);	    /* turn ON flattening */
	    break;
	}
	case OPF_OJSUBSELECT:
	{
	    debugcb.db_trswitch = DB_TR_OFF;
	    debugcb.db_value_count = 0;     
	    debugcb.db_trace_point = OPT_GMAX + OPT_F006_NONOTEX;
	    opt_call(&debugcb);	    /* turn ON OJ flattening */
	    break;
	}
	case OPF_NOOJSUBSELECT:
	{
	    debugcb.db_trswitch = DB_TR_ON;
	    debugcb.db_value_count = 0;     
	    debugcb.db_trace_point = OPT_GMAX + OPT_F006_NONOTEX;
	    opt_call(&debugcb);	    /* turn OFF OJ flattening */
	    break;
	}
	case OPF_OPTIMIZEONLY:
	{
	    debugcb.db_trswitch = DB_TR_ON;
	    debugcb.db_value_count = 0;     
	    debugcb.db_trace_point = OPT_GMAX + OPT_F032_NOEXECUTE;
	    opt_call(&debugcb);	    /* turn OFF execution */
	    break;
	}
	case OPF_NOOPTIMIZEONLY:
	{
	    debugcb.db_trswitch = DB_TR_OFF;
	    debugcb.db_value_count = 0;     
	    debugcb.db_trace_point = OPT_GMAX + OPT_F032_NOEXECUTE;
	    opt_call(&debugcb);	    /* turn ON execution */
	    break;
	}
	case OPF_HASH:
	{
	    altercb->ops_hash = TRUE;		/* enable hash join/agg */
	    break;
	}
	case OPF_NOHASH:
	{
	    altercb->ops_hash = FALSE;		/* disable hash join/agg */
	    break;
	}
	case OPF_NEWENUM:
	{
	    altercb->ops_newenum = TRUE;	/* enable new enumeration */
	    break;
	}
	case OPF_NONEWENUM:
	{
	    altercb->ops_newenum = FALSE;	/* disable new enumeration */
	    break;
	}
	case OPF_PARALLEL:
	{
	    altercb->ops_parallel = TRUE;	/* enable || query plans */
	    if (opf_cb->opf_value >= 0)
		altercb->ops_pq_dop = opf_cb->opf_value;
						/* update degree of ||ism */
	    break;
	}
	case OPF_NOPARALLEL:
	{
	    altercb->ops_parallel = FALSE;	/* disable || query plans */
	    break;
	}
	case OPF_ARESOURCE:
	{
	    altercb->ops_amask |= OPS_ARESOURCE; /* causes enumeration
				    ** to be used in all cases so that
				    ** resource control gets accurate
				    ** estimates */
	    break;
	}
	case OPF_ANORESOURCE:
	{
	    altercb->ops_amask &= (~OPS_ARESOURCE); /* turns off enumeration
				    ** for single table queries so that
				    ** resource control will not get accurate
				    ** estimates if it is used */
	    break;
	}
	case OPF_CARDCHK:
	{
	    altercb->ops_nocardchk = FALSE; /* Enable cardinality checks */
	    break;
	}
	case OPF_NOCARDCHK:
	{
	    altercb->ops_nocardchk = TRUE; /* Ignore card checks (legacy mode) */
	    break;
	}
# ifdef E_OP0089_ALTER
	default:
	    {
		opx_rerror( opf_cb, E_OP0089_ALTER);
		return( E_DB_ERROR );
	    }
# endif
	}
    }
    return (E_DB_OK);
}

/*{
** Name: ops_exlock	- exclusive access to OPF server control block
**
** Description:
**      Get semaphore to have single thread access to update OPF 
**      structures including OPF server control block 
**
** Inputs:
**      opf_cb                          opf control block
**      semaphore                       ptr to OPF semaphore
**
** Outputs:
**	Returns:
**	    
**	Exceptions:
**
** Side Effects:
**	    gets lock on OPF semaphore
**
** History:
**      20-jul-93 (ed)
**          changed name ops_lock for solaris, due to OS conflict
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
[@history_line@]...
[@history_template@]...
*/
DB_STATUS
ops_exlock(
	OPF_CB             *opf_cb,
	SCF_SEMAPHORE      *semaphore)
{
    STATUS		status;

    /* First, wait to get exclusive access to the server control block */
    status = CSp_semaphore(1, semaphore); 	/* exclusive */

    if (status != OK)
    {
# ifdef E_OP0092_SEMWAIT
	opx_rverror( opf_cb, E_DB_ERROR, E_OP0092_SEMWAIT, status);
# endif
	return(E_DB_ERROR);
    }
    return(E_DB_OK);
}

/*{
** Name: ops_unlock	- release exclusive access to server control block
**
** Description:
{@comment_line@}...
**
** Inputs:
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
[@history_template@]...
*/
DB_STATUS
ops_unlock(
	OPF_CB             *opf_cb,
	SCF_SEMAPHORE      *semaphore)
{
    STATUS		status;

    status = CSv_semaphore(semaphore);

    if (status != OK)
    {
# ifdef E_OP0092_SEMWAIT
	opx_rverror( opf_cb, E_DB_ERROR, E_OP0092_SEMWAIT, status);
# endif
	return(E_DB_ERROR);
    }
    return(E_DB_OK);
}

/*{
** Name: ops_alter	- alter session or server state for OPF
**
** Description:
**      This routine contains all processing of the SET commands for the 
**      optimizer.  These commands can be at the server or session level.
**      There is currently no protection mechanism available for which
**      users are able to access these SET commands.  There probably should
**      be a system catalog created to decide privileges and min and max
**      values allowable for a session.  Currently, all users can do anything
**      at all.  This routine will not do anything in terms of checking
**      privileges.
**
**      SET [[SESSION] | SERVER] CPUFACTOR <value>
**      SET [[SESSION] | SERVER] TIMEOUT   <value>
**      SET [[SESSION] | SERVER] QEP
**      SET [[SESSION] | SERVER] NOQEP
**
**      FIXME need to use semaphores to access global structures.
**
** Inputs:
**      opf_cb                          ptr to caller's control block
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
DB_STATUS
ops_alter(
	OPF_CB             *opf_cb)
{
    DB_STATUS           status;			/* user return status */

    switch (opf_cb->opf_level)
    {
    case OPF_SERVER:
    {	/* change operating characteristics of the server */
	DB_STATUS	temp_status;
	/* get exclusive access before updating */
	status = ops_exlock(opf_cb,
	    &((OPS_CB *)(opf_cb->opf_scb))->ops_server->opg_semaphore);
	if (DB_FAILURE_MACRO(status))
	    break;
	temp_status = ops_change( opf_cb, 
	    &((OPG_CB*)opf_cb->opf_server)->opg_alter);
	/* release exclusive access */
	status = ops_unlock(opf_cb,
	    &((OPS_CB *)(opf_cb->opf_scb))->ops_server->opg_semaphore);
	if (DB_SUCCESS_MACRO(status))
	    status = temp_status;
	break;
    }
    case OPF_SESSION:
    {   /* change operating characteristics of this session */
	status = ops_change( opf_cb, &((OPS_CB *)(opf_cb->opf_scb))->ops_alter);
	break;
    }
    default:
# ifdef E_OP0089_ALTER
	    {
		opx_rerror( opf_cb, E_OP0089_ALTER);
		return( E_DB_ERROR );
	    }
#endif
    }
    return (status);
}

