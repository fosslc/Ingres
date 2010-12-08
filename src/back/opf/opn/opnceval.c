/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
#include    <ex.h>
#include    <tm.h>
#define		    OPN_CEVAL		TRUE
#define             OPX_TMROUTINES      TRUE
#define             OPX_EXROUTINES      TRUE
#include    <me.h>
#include    <opxlint.h>


/**
**
**  Name: OPNCEVAL.C - evaluate cost of join operator tree
**
**  Description:
**      contains entry point to routines which evaluates cost of join
**      operator trees.
**
**  History:    
**      11-jun-86 (seputis)    
**          initial creation from costeval.c
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opn_recover(
	OPS_SUBQUERY *subquery);
static i4 opn_mhandler(
	EX_ARGS *args);
bool opn_timeout(
	OPS_SUBQUERY *subquery);
OPN_STATUS opn_ceval(
	OPS_SUBQUERY *subquery);

/*{
** Name: opn_recover	- recover from an out-of-memory error
**
** Description:
**      This routine will copy the best CO tree to non-enumeration memory
**      and reinitialize the enumeration memory stream, and related
**      variables.  The purpose of this is to allow enumeration to continue
**      from this point in the search space.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
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
**      31-oct-86 (seputis)
**          initial creation
**      14-aug-89 (seputis)
**          - fix b6538, initialize opn_sfree
**      16-jul-91 (seputis)
**          - fix access violation in star tests
**	    - check for array of query plans to copy out of enumeration
**	    memory for function aggregates.
**	18-sep-02 (inkdo01)
**	    Changes to copy out logic for new enumeration (only copy out
**	    bottom left CO fragment).
**	18-oct-02 (inkdo01)
**	    Externalized for accessibility from opn_newenum.
**	29-oct-04 (inkdo01)
**	    Added parm to opo_copyfragco().
**	5-oct-2007 (dougi)
**	    Count opn_recover() calls.
*/
VOID
opn_recover(
	OPS_SUBQUERY       *subquery)
{
    OPS_STATE		*global;	    /* ptr to global state variable */

    global = subquery->ops_global;
    global->ops_mstate.ops_recover++;
    /* Check for new enumeration & just copy fragment. */
    if (subquery->ops_mask & OPS_LAENUM &&
	subquery->ops_bestfragco &&
	!subquery->ops_fraginperm &&
	!subquery->ops_lastlaloop)
    {
	opo_copyfragco(subquery, &subquery->ops_bestfragco, TRUE);
	subquery->ops_fraginperm = TRUE;
    }
    /* Otherwise, treat old enumeration. We also need logic to
    ** copy out bestco when new enumeration is on the last loop.
    ** bestco is really the final plan at that point. */
    if (subquery->ops_bestco
	||
	(   (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	    &&
	    subquery->ops_dist.opd_bestco
	))
	opo_copyco(subquery, &subquery->ops_bestco, FALSE);/* copy the 
					    ** best CO prior to deallocating 
					    ** memory - FALSE indicates that
                                            ** this may not be the final CO
                                            ** tree */
    opu_deallocate(global, &global->ops_estate.opn_streamid);
    global->ops_estate.opn_streamid = opu_allocate(global);

    subquery->ops_msort.opo_base = NULL;    /* all this info was allocated
					    ** out of enumeration memory */
    /* initialize all memory management free lists */
    global->ops_estate.opn_sfree = NULL; /* init the OPN_SDESC free list */
    global->ops_estate.opn_rlsfree = NULL; /* init the RLS free list */
    global->ops_estate.opn_eqsfree = NULL; /* init the EQS free list */
    global->ops_estate.opn_stfree = NULL; /* init the SUBTREE free list */
    global->ops_estate.opn_cofree = NULL; /* init the CO free list */
    global->ops_estate.opn_hfree = NULL; /* init the HISTOGRAM free list */
    global->ops_estate.opn_ccfree = NULL; /* init the cell count free list */

    if (global->ops_cb->ops_smask & OPS_MDISTRIBUTED)
	opd_recover(global);		    /* reinitialize the distributed
					    ** memory structures which will be
					    ** lost when the memory stream is
					    ** recovered */
    global->ops_estate.opn_sbase = (OPN_ST *) opn_memory( global,
	(i4) sizeof( OPN_ST ));	    /* get memory for savework */
    MEfill( sizeof(OPN_ST), (u_char)0, 
	(PTR)global->ops_estate.opn_sbase->opn_savework); /*
                                            ** init all ptrs to NULL */
    global->ops_estate.opn_slowflg = TRUE;  /* have any intermediate results
                                            ** been deleted?
                                            */
    {	/* initialize joinop range table references to enumeration memory */
	OPV_RT		*vbase;		    /* ptr to local joinop range table
                                            */
	OPV_IVARS	varno;		    /* joinop range var number of 
                                            ** element being reset */
	vbase = subquery->ops_vars.opv_base;/* base of local joinop range table
                                            */
	for (varno = subquery->ops_vars.opv_rv; varno-- > 0;)
	{
	    vbase->opv_rt[varno]->opv_trl->opn_eqp = 0; /* reset ptrs into
                                            ** enumeration memory */
	}
    }
}

/*{
** Name: opn_mhandler	- exception handler for out of memory errors
**
** Description:
**	This exception handler used in out-of-memory error
**      situations, in which a recovery may be possible.  Any other
**      exception will be resignalled.
** Inputs:
**      args                            arguments which describe the 
**                                      exception which was generated
**
** Outputs:
**	Returns:
**	    EXRESIGNAL                 - if this was an unexpected exception
**          EXDECLARE                  - if an expected, internally generated
**                                      exception was found
**	Exceptions:
**	    none
**
** Side Effects:
**	    This routine will remove the calling frame to the point at which
**          the exception handler was declared.
**
** History:
**	27-oct-86 (seputis)
**          initial creation 
**	8-nov-88 (seputis)
**          exception handling interface changed
**	8-nov-88 (seputis)
**          look for EX_UNWIND
**	20-dec-90 (seputis)
**	    add support for floating point exceptions handling
**	4-feb-90 (seputis)
**	    add support for floating point exceptions handling
**	18-apr-91 (seputis)
**	    - fix for bug 36920, floating point handling problem
**	1-may-91 (seputis)
**	    - fix for 18-apr-91 integration problem
**	11-mar-92 (rog)
**	    Remove EX_DECLARE and change the EX_UNWIND case to return
**	    EXRESIGNAL instead of EX_OK.
[@history_line@]...
*/
static STATUS
opn_mhandler(
	EX_ARGS            *args)
{
    OPX_ERROR	    error;
    bool	    hard;

    switch (EXmatch(args->exarg_num, (i4)10, EX_JNP_LJMP, EX_UNWIND, EXFLTUND,
       EXFLTOVF, EXFLTDIV, EXINTOVF, EXINTDIV, EXHFLTDIV, EXHFLTOVF, EXHFLTUND))
    {
    case 1:
    {
	if ((args->exarg_count > 0)
	    && (args->exarg_array[0] == E_OP0400_MEMORY))
	    return (EXDECLARE);    /* return to invoking exception point
                                    ** - this is an out-of-memory error which
                                    ** perhaps can be handled
                                    */
	return(EXRESIGNAL);        /* resignal since this is normal
                                    ** enumeration completion exit - this
                                    ** will have the side effect of deleting
                                    ** the exception handler at this level */
    }
    case 2:
				    /* a lower level exception handler has
				    ** cleared the stack so return OK
				    ** to satisfy the protocol */
	return (EXRESIGNAL);
    case 10:
	hard = TRUE;
	error = E_OP049E_FLOAT_UNDERFLOW;
        break;
    case 3:
	hard = FALSE;
	error = E_OP049E_FLOAT_UNDERFLOW;
        break;
    case 9:
	hard = TRUE;
	error = E_OP04A0_FLOAT_OVERFLOW;
        break;
    case 4:
	hard = FALSE;
	error = E_OP04A0_FLOAT_OVERFLOW;
        break;
    case 8:
	hard = TRUE;
	error = E_OP049F_FLOAT_DIVIDE;
        break;
    case 5:
	hard = FALSE;
	error = E_OP049F_FLOAT_DIVIDE;
        break;
    case 6:
	hard = FALSE;
	error = E_OP04A1_INT_OVERFLOW;
        break;
    case 7:
	hard = FALSE;
	error = E_OP04A2_INT_DIVIDE;
        break;
    default:
        return (EXRESIGNAL);	    /* handle unexpected exception at lower
				    ** level exception handler, this exception
                                    ** handler only deals with out of memory
				    ** errors than can be recovered from */
    }
    return(opx_float(error, hard)); /* process float/integer problem by skipping
				    ** search space */
}

/*{
** Name: opn_timeout	- check for timeout condition
**
** Description:
**      Check the various indications that timeout has occurred. 
**
** Inputs:
**      subquery                        ptr to subquery state
**
** Outputs:
**
**	Returns:
**	    bool - TRUE is timeout condition has been detected 
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-may-90 (seputis)
**          - b21582, do not allow large CPU gaps before checking
**	    for a timeout condition
**	26-nov-90 (seputis)
**	    - added support for new server startup flags
**      31-jan-91 (seputis)
**          - use more time to optimize queries in DB procedures
**      20-mar-91 (seputis)
**          - b 36600 added calls to CSswitch to allow context 
**	switching if no query plan is found or notimeout is used 
**      3-dec-92 (ed)
**          OP255 was doing a notimeout on all subqueries if the second
**	    parameter was set
**	31-jan-97 (inkdo01)
**	    Simplify CSswitch interface so that all calls to timeout 
**	    result in a CSswitch call. This fixes a CPU lockout problem 
**	    in (at least) Solaris, because the switch depended largely
**	   on faulty quantum testing logic in CSstatistics.
**	15-sep-98 (sarjo01)
**	    Redo of function, adding support for ops_timeoutabort
**	14-jul-99 (inkdo01)
**	    Add code to do quick exit for where-less idiot queries.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better CScancelCheck.
**      21-sept-2010 (huazh01)
**          Flag 'OPS_IDIOT_NOWHERE' can also be set for a query having
**          only constant restrictions in its right branch of the query
**          tree. After we got the first qep, do not immediately timeout 
**          such type of query, otherwise, plans using secondary index 
**          won't be considered. (b124342)
[@history_template@]...
*/
bool
opn_timeout(
	OPS_SUBQUERY       *subquery)
{
    OPS_CB          *scb;               /* ptr to session control block for
                                        ** currently active user */
    OPS_STATE       *global;            /* ptr to global state variable */

    /* OPS_IDIOT_NOWHERE can also be set for a query having only constant
    ** restrictions in its right branch of the query tree. Do not immediately
    ** time out such type of query, otherwise, plans using secondary index
    ** won't be considered. (b124342)
    */
    if (subquery->ops_mask & OPS_IDIOT_NOWHERE && 
        (subquery->ops_root->pst_right == NULL || 
	subquery->ops_root->pst_right->pst_sym.pst_type == PST_QLEND ) &&
        subquery->ops_bestco) 
        return(TRUE);
					    /* if idiot query (join, but no where)
					    ** and we have valid plan, quit now */

    global = subquery->ops_global;	    /* get ptr to global state variable
                                            */
    scb = global->ops_cb;		    /* get ptr to active session control
                                            ** block */
    if (scb->ops_alter.ops_timeout)	    /* TRUE if optimizer should timeout
                                            */
    {
        i4		    joinop_timeout;
        i4		    joinop_abort;
        joinop_timeout = scb->ops_alter.ops_sestime;
        joinop_abort   = scb->ops_alter.ops_timeoutabort;

        if (joinop_abort == 0)
	{
	    if (subquery->ops_bestco)
	    {
	        TIMERSTAT	    timer_block;
	        STATUS		    cs_status;
	        i4		    nowcpu;    /* cpu time used by process */
	        OPO_COST	    miladjust; /* adjustment to cost to obtain
                                               ** millisec equivalent */
	        i4		    first;     /* plan to timeout */
	        i4		    second;    /* subquery plan to timeout */
	        bool		    planid_timeout; /* check if tracing flag is
					            ** set and override timeout
					            ** check with new one */

	        planid_timeout = FALSE;
	        if ((global->ops_qheader->pst_numparm > 0)
			||
		    global->ops_procedure->pst_isdbp)
	            miladjust = scb->ops_alter.ops_tout *
                                         scb->ops_alter.ops_repeat; 
					    /* use more time to optimize
					    ** a repeat query */
	        else
		    miladjust = scb->ops_alter.ops_tout;

	        if (scb->ops_check)
	        {
		    if (opt_svtrace( scb, OPT_F125_TIMEFACTOR, &first, &second)
		        &&
		        first>0)    /* see if there is a miladjust factor
				    ** from the user */
		        miladjust = (miladjust*first)/100.0;
				    /* this will increase/decrease
				    ** the amount of time required to timeout */
		    planid_timeout = 
		        opt_svtrace( scb, OPT_F127_TIMEOUT, &first, &second);
				    /* this will time out based on subquery
				    ** plan id and number of plans being
				    ** evaluated */
	        }


		if (cs_status = CSstatistics(&timer_block, (i4)0))
		{   /* get cpu time used by session from SCF to calculate
                    ** difference from when optimization began FIXME - this
                    ** is an expensive routine to call, perhaps some other
                    ** way of timing out could be used such as number of CO
                    ** nodes processed */
		    opx_verror(E_DB_ERROR, E_OP0490_CSSTATISTICS, 
			(OPX_FACILITY)cs_status);
		}
		nowcpu = timer_block.stat_cpu -global->ops_estate.opn_startime;

	        if (
		    (((nowcpu > (miladjust * subquery->ops_cost)) ||
					    /* if the amount of time spent so 
					    ** far on optimization is roughly 
                                            ** about how much time the best 
                                            ** solution found so far
					    ** will take to execute, then stop
                                            */
		      ( (joinop_timeout > 0) && (nowcpu > joinop_timeout)))
		    && 
		    (   !planid_timeout	    /* if the plan timeout tracing is
					    ** set then this should override
					    ** the normal timeout mechanism */
			||
			(   (second != 0)
			    &&
			    (second != subquery->ops_tsubquery) /* apply timeout
					    ** to all query plans except the
					    ** one specified */
			)
		    ))
		    ||
		    ( planid_timeout
		        &&
		      (first <= subquery->ops_tcurrent)
		        &&
		      (	(second == 0)	    /* check if all plans should be
					    ** timed out */
			||
			(second == subquery->ops_tsubquery) /* check if a
					    ** particular subquery is being
					    ** search for */
		      )
		    ))
	        {
		    opx_verror(E_DB_WARN, E_OP0006_TIMEOUT, (OPX_FACILITY)0);
		    return(TRUE);
	        }
	    }
	}
	else
	{
            TIMERSTAT           timer_block;
            STATUS              cs_status;
            i4             nowcpu;    /* cpu time used by process */
            OPO_COST            miladjust; /* adjustment to cost to obtain
                                           ** millisec equivalent */
            i4             first;     /* plan to timeout */
            i4             second;    /* subquery plan to timeout */
            bool                planid_timeout; /* check if tracing flag is
                                                ** set and override timeout
                                                ** check with new one */
	    if (joinop_timeout == 0 || joinop_timeout >= joinop_abort)
		joinop_timeout = joinop_abort; 
            planid_timeout = FALSE;
            if ((global->ops_qheader->pst_numparm > 0)
                    ||
                global->ops_procedure->pst_isdbp)
                miladjust = scb->ops_alter.ops_tout *
                                     scb->ops_alter.ops_repeat;
                                        /* use more time to optimize
                                        ** a repeat query */
            else
                miladjust = scb->ops_alter.ops_tout;

            if (scb->ops_check)
            {
                if (opt_svtrace( scb, OPT_F125_TIMEFACTOR, &first, &second)
                    &&
                    first>0)    /* see if there is a miladjust factor
                                ** from the user */
                    miladjust = (miladjust*first)/100.0;
                                /* this will increase/decrease
                                ** the amount of time required to timeout */
                planid_timeout =
                    opt_svtrace( scb, OPT_F127_TIMEOUT, &first, &second);
                                /* this will time out based on subquery
                                ** plan id and number of plans being
                                ** evaluated */
            }
            if (cs_status = CSstatistics(&timer_block, (i4)0))
            {
                opx_verror(E_DB_ERROR, E_OP0490_CSSTATISTICS,
                    (OPX_FACILITY)cs_status);
            }
            nowcpu = timer_block.stat_cpu -global->ops_estate.opn_startime;

            if ( subquery->ops_bestco
		 &&
                ( (
                 ((nowcpu > (miladjust * subquery->ops_cost)) ||
                  ((joinop_timeout > 0) && (nowcpu > joinop_timeout)))
                     &&
                 ( !planid_timeout ||
                    ( (second != 0)
                        &&
                        (second != subquery->ops_tsubquery)
                    )
                  )
                 )
                ||
                ( planid_timeout
                    &&
                  (first <= subquery->ops_tcurrent)
                    &&
                  ( (second == 0)
                    ||
                    (second == subquery->ops_tsubquery)
                  ))

                ) )
            {
                opx_verror(E_DB_WARN, E_OP0006_TIMEOUT, (OPX_FACILITY)0);
                return(TRUE);
            }
	    else if (nowcpu > joinop_abort)
	    {
		if (subquery->ops_bestco)
		{
    		    opx_verror(E_DB_WARN, E_OP0006_TIMEOUT, (OPX_FACILITY)0);
		    return(TRUE);
		}
		else
		{
		    scb->ops_interrupt = TRUE;
	            opx_rerror(subquery->ops_global->ops_caller_cb,
                               E_OP0018_TIMEOUTABORT);
		}
	    }
	} /* else */
    }
    /* Check for cancels if OS-threaded, time-slice if internal threaded */
    CScancelCheck(global->ops_cb->ops_sid);
    return(FALSE);
}

/*{
** Name: opn_ceval	- evaluate the cost of the join operator tree
**
** Description:
**      Entry point for routines which evaluate cost of a join operator tree.
**      Contains checks for timeouts within the optimizer.
**
** Inputs:
**      subquery                        ptr to subquery being analyzed
**          ->ops_estate.opn_sroot      ptr to root of join operator tree
**
** Outputs:
**      subquery->ops_bestco            ptr to best plan found
**      subquery->ops_cost              cost of best plan found
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-jun-86 (seputis)
**          initial creation from costeval
**	2-nov-88 (seputis)
**          changed CSstatistics interface
**	2-nov-88 (seputis)
**          add trace flag to timeout on number of plans being evaluated
**	15-aug-89 (seputis)
**	    add trace flag to adjust timeout factor which converts cost to time
**	    used to help isolate early timeout problems
**	16-may-90 (seputis)
**	    - b21582, move timeout checking to opn_timeout routine so it can be
**	    called from opn_arl
**	17-oct-90 (seputis)
**	    - b33386 - print error is no qep is found
**	22-apr-91 (seputis)
**	    - fix floating point exception handling problems
**	19-nov-99 (inkdo01)
**	    Changes to remove EXsignal from opn_exit processing.
**	18-oct-02 (inkdo01)
**	    Changes to enable new enumeration.
**	30-mar-2004 (hayke02)
**	    Call opn_exit() with a FALSE longjump.
**	30-Jun-2006 (kiria01) b116309
**	    Capture memory exhaustion error to errlog.log. 
**	11-Mar-2008 (kschendel) b122118
**	    Remove unused ops_tempco.  Fix up bfexception stuff so that we
**	    stop calling recover pointlessly at the start of every query.
[@history_line@]...
*/
OPN_STATUS
opn_ceval(
	OPS_SUBQUERY       *subquery)
{
    EX_CONTEXT	    excontext;		/* context block for exception 
                                        ** handler*/
    OPN_SUBTREE     *opn_ssubtp;        /* dummy var ignored at this level
                                        ** - list of subtrees with possible cost
                                        ** orderings (including reformats)
                                        ** - all element in the list have same
                                        ** relations but in different order or
                                        ** use a different tree structure.
                                        */
    OPN_RLS         *opn_srlmp;         /* dummy var ignored at this level
                                        ** - list of list of subtrees, with a
                                        ** different set of relations for each
                                        ** OPN_RLS element
                                        */
    OPN_EQS         *opn_seqp;          /* dummy var ignored at this level
                                        ** - used to create a list of different
                                        ** OPN_SUBTREE (using indexes) if
                                        ** an index exists.
                                        */
    OPN_STATUS	    sigstat = OPN_SIGOK;

    if ( EXdeclare(opn_mhandler, &excontext) == EX_DECLARE ) /* set
				    ** up exception handler to 
				    ** recover from out of memory
				    ** errors */
    {
	/* this point will only be reached if the enumeration processing has
	** run out of memory.  The optimizer will try to continue by copying 
	** the best CO tree found so far, out of enumeration memory, and  
	** reinitializing the enumeration stream, and continuing.  
	** The out-of-memory error will be reported if a complete pass of 
	** cost evaluation cannot be completed. */
	(VOID)EXdelete();		    /* cancel exception handler prior
					    ** to exiting routine */
	if (subquery->ops_global->ops_gmask & OPS_FLINT)
	{
	    subquery->ops_global->ops_gmask &= ~OPS_FLINT; /* reset exception
					    ** indicator */
	    return(OPN_SIGOK);		    /* skip this plan if a float
					    ** or integer exception has occurred
					    ** during processing */
	}
	if ( EXdeclare(opn_mhandler, &excontext) == EX_DECLARE ) /* set
				    ** exception handler to catch case in
				    ** which no progress is made */
	{
	    (VOID)EXdelete();		    /* cancel exception handler prior
					    ** to exiting routine */
	    if (subquery->ops_bestco)
		opx_verror(E_DB_WARN, E_OP0400_MEMORY, (OPX_FACILITY)0); /* report 
					    ** warning message to caller */
	    else
	    {
		opx_lerror(E_OP0400_MEMORY, 0,0,0,0,0); /* Output to errlog.log as well kiria01-b116309 */
		opx_error(E_OP0400_MEMORY); /* exit with a user error if the query
					    ** cannot find at least one acceptable
					    ** query plan */
	    }
	    opn_exit(subquery, FALSE);		    /* exit with current query plan */
	    return(OPN_SIGEXIT);
	}
	opn_recover(subquery);              /* routine will copy best CO tree 
					    ** and reinitialize memory stream
					    */
    }

    subquery->ops_global->ops_gmask &= (~OPS_FPEXCEPTION); /* reset fp exception
					    ** indicator before evaluating
					    ** plan */
    if (subquery->ops_global->ops_gmask & OPS_BFPEXCEPTION
      && subquery->ops_bestco != NULL)
	opn_recover(subquery);		    /* if the current best plan occurred with
					    ** exceptions then enumeration memory needs
					    ** to be flushed, so that subsequent plans
					    ** do not use subtrees which may have
					    ** been created with exceptions, this means
					    ** that the OPF cache of sub-trees is not
					    ** used if one exception has occurred and
					    ** the current best plan was created with
					    ** exceptions */
    

    (VOID) opn_nodecost (  subquery, 
		    subquery->ops_global->ops_estate.opn_sroot,
		    (subquery->ops_mask & OPS_LAENUM) ? subquery->ops_laeqcmap :
		    	&subquery->ops_eclass.ope_maps.opo_eqcmap, 
		    &opn_ssubtp,
		    &opn_seqp,
		    &opn_srlmp, &sigstat);

    if (sigstat != OPN_SIGEXIT && 
	!(subquery->ops_mask & OPS_LAENUM) &&
	opn_timeout(subquery))    	    /* check for timeout (but only for 
					    ** old style enumeration) */
    {
	(VOID)EXdelete();		    /* cancel exception handler prior
					    ** to exiting routine, call after
					    ** opn_timeout in case out of memory
					    ** errors occur */
	opn_exit(subquery, FALSE);	    /* at this point we 
					    ** return the subquery->opn_bestco
					    ** tree, this
					    ** could also happen in freeco
					    ** and enumerate */
	return(OPN_SIGEXIT);
    }
    (VOID)EXdelete();			    /* cancel exception handler prior
					    ** to exiting routine */
    return(sigstat);
}
