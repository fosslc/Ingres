/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<cs.h>
#include	<me.h>
#include	<scf.h>
#include	<ulm.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include        <qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include        <ex.h>
#include	<rdfint.h>

/*
** prototypes
*/

/* rdf_handler - exception handler for RDF */
static STATUS rdf_handler(EX_ARGS *args);

/* rdf_cleanup	- clean up the status for this operation */
static STATUS rdf_cleanup(RDF_GLOBAL 	*global);

/**
**
**  Name: RDFCALL.C - Entry points to RDF.
**
**  Description:
**	This file contains the entry points to RDF for requesting 
**	information of a table.
**
**	rdf_call - Call a relation description operation.
**
**
**  History:
**      28-mar-86 (ac)    
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**      19-oct-87 (seputis)
**          added exception handler, and resource handler
**	22-mar-89 (neil)
**	    added support for rules.
**	30-aug-90 (teresa)
**	    retro-fitted fixes for bugs 21489 and 30694 from 6.3 line.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.  Also added include of me.h
**	22-apr-96 (pchang)
**	    Fix bug 76130 - In rdf_release(), a private info block that had 
**	    been deallocated after RDF had encountered an error was incorrectly
**	    returned to the caller of RDF and later emerged in rdf_unfix() 
**	    causing the same private stream to be closed twice and resulting 
**	    in a SEGV.
**	23-sep-1996 (canor01)
**	    Move global data definitions to rdfdata.c.
**      5-dec-96 (stephenb)
**          Add performance profiling code
**	21-may-1997 (shero03)
**	    Added facility tracing diagnostic
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**	16-mar-1998 (rigka01)
**          Cross integrate change 434645 for bug 89105
**	    Bug 89105 - clear up various access violations which occur when
**	    rdf memory is low.
**	07-sep-1998 (nanpr01)
**	    Take out the retry logic.
**	07-nov-1998 (nanpr01)
**	    Fixup Indentation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions.
**	    Added "call_info" static structure to decypher
**	    call-specific needs instead of relying on mutiple 
**	    switch statements.
**	9-Jan-2004 (schka24)
**	    Added partition-copy and -update dispatches
**	2-feb-2004 (schka24)
**	    Added finish-partition-def dispatch
**	25-Mar-2004 (schka24)
**	    Remove call-info table to rdfdata.
**	5-Aug-2004 (schka24)
**	    Expose the partition struct-compat checker.
**/
GLOBALREF   i4 RDF_privctr;	    /* used with RDU_BLD_PRIVATE trace*/

GLOBALREF const RDF_CALL_INFO Rdf_call_info[];	/* RDF operation table */

/*
** Name  rdf_handler	- exception handler for RDF
**
** Description:
**	This handler will release any resources held by RDF in the case of
**      and unexpected exception,... such as semaphores held, or temporary
**      memory streams created.  If an exception occurs when resources
**      are released then a fatal error will be reported, and no further
**      attempt will be made to release resources.
**
** Inputs:
**      none
**
** Outputs:
**      none
**	Returns:
**	    EX_DECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	22-jan-90 (teg)
**	    added support for EX_UNWIND
**	16-jul-92 (teresa)
**	    implemented prototypes
**	02-jul-1993 (rog)
**	    Removed the EX_DECLARE case.
[@history_line@]...
*/
static STATUS
rdf_handler(EX_ARGS *args)
{
    switch (EXmatch(args->exarg_num, 1, EX_UNWIND))
    {
    case 1:
	/* we are in an UNWIND preceeding a server shutdown */
	return (EXRESIGNAL);
    default:
	/* everything else is unexpected. */
        return (rdf_exception(args)); /* handle unexpected exception */
    }
}

/*{
** Name: rdf_release	- release any resources that RDF has on table
**
** Description:
**      Routine to release resources held on table, or in RDF.
**
** Inputs:
**      global                          ptr to RDF global state variable
**      status                          ptr to current status
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
**      20-oct-87 (seputis)
**          initial creation
**	22-mar-89 (neil)
**	    added support for rules.
**      01-may-89 (mings)
**          release ldb descriptor semaphore when error 
**      14-aug-90 (teresa)
**          test to assure we really have integrity/protect/rule tuple
**          open before calling rdu_qmclose. (bug 30694)
**	07-feb-92 (teresa)
**	    Merged STAR and Local for rdf_release.  Picked up some of the
**	    STAR enhancements, such as saving original error code and cleaning
**	    up retry, but implemented slightly different.
**	19-may-92 (teresa)
**	    changes for SYBIL - release resources held as part of making Star
**	    use ULH to manage the LDBcache objects.
**	16-jul-92 (teresa)
**	    implemented prototypes
**	29-sep-92 (teresa)
**	    Fixed AV that sometimes occurs on error paths where the infoblk 
**	    pointer was not initialized in the RDF_RB and rdf_release 
**	    unconditionally cleared the RDF_IULH bit. The fix is to clear the
**	    RDF_IULH status only for the RDF_INVALIDATE operation.
**	29-jan-93 (teresa, jhahn)
**	    Release iiprocedure_parameter and iikey catalogs if held.
**	17-feb-93 (teresa)
**	    add sem_type (RDF_BLD_DEFDESC) to rdd_setsem/rdd_resetsem interface.
**	    Also add support to release defaults cache resources.
**	20-may-93 (teresa)
**	    rename rdf_de_ulhcb to rdf_def_ulhcb so it remains unique to 
**	    8 characters from rdf_de_ulhobject.
**	11-nov-93 (teresa)
**	    Add support to handle RDF_INVAL_FIXED for bug 56471.
**	    Also look for ulhobject in RDF's ULH_CB if rdfcb->rdf_info_blk does
**	    not contain it, which fixes bug 56677.
**	08-dec-93 (teresa)
**	    Fix bug 57677 -- when we clear the RDF_RQTUPDATE resource bit, we
**	    must also clear the RDF_RULH bit or we will try to unfix the resource
**	    a second time, which can lead to memory corruption in concurrent
**	    conditions.  Also, if we are in a retry iteration of the loop
**	    (try_again is true) then do not decrement the rdf_shared_sessions
**	    counter -- it should only be decremented once per call of 
**	    rdf_release.
**	12-jan-94 (teresa)
**	    Change wrong typecase for rdf_info_blk assignment.
**	06-apr-94 (teresa)
**	    fix bug 60730 by forcing a reinit of a control block before calling
**	    the routine to release the LDBdesc cache object.
**	22-apr-96 (pchang)
**	    Fix bug 76130 - A private info block that had been deallocated
**	    after RDF had encountered an error was incorrectly returned to the
**	    caller of RDF and later emerged in rdf_unfix() causing the same
**	    private stream to be closed twice and resulting in a SEGV.
**	6-Oct-2005 (schka24)
**	    I had changed the shared-session counter to be manipulated
**	    with CSadjust_counter elsewhere, missed the usage here.
**	19-oct-2009 (wanfr01) Bug 122755
**	    Need to pass infoblk to the deulh routines
**	    Need to mutex protect rdf_shared_sessions as the object may be 
**	    about to be destroyed.  Added sanity check on rdf_shared_sessions
**      11-Nov-2010 (hanal04) Bug 124718
**          Mimic the RDF_RSEMAPHORE test when trying to acquire
**          RDF_BLD_LDBDESC (RDD_SEMAPHORE) and RDF_DEF_DESC (RDF_DSEMAPHORE)
**          to avoid requesting a semaphore we already own.
**      26-Nov-2010 (hanal04) Bug 124759 and 124718
**          Remove previous fix for Bug 124718. This was not a root cause
**          solution. Further investigation showed that rdd_defsearch()
**          was not a legitimate double acquisition in the way that
**          the GSEMAPHORE is treated. rdd_defsearch() was failing to
**          release the RDF_DEF_DESC mutex because it was checking the wrong
**          flag.
[@history_template@]...
*/
VOID
rdf_release(	RDF_GLOBAL         *global,
		DB_STATUS          *status)
{
    DB_STATUS	op_status;	    /* status for individual operations */
    DB_STATUS	run_status= *status; /* running (most severe) status for this 
				    ** routine */
    DB_ERROR	error;		    /* copy of any error data we were called
				    ** with
				    */

    /* save input error data (if any) */
    STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);

    /* LOOP TO RELEASE RESOURCES */
    if (global->rdf_resources & RDF_RUPDATE)
    {   
	/* 
	** master copy of RDF descriptor has been reserved for update by
	** this thread so release it 
	*/
        if (!(global->rdf_resources & RDF_RSEMAPHORE))
	{
	    op_status = rdu_gsemaphore(global); 
					/* 
					** get semaphore prior to updating 
					** ULH object descriptor 
					*/
	    if (DB_FAILURE_MACRO(op_status))
	    {
	        /* report error if one does not exist */
	        if (op_status > run_status)
	        {
		    /* this error is more serrious than any previously 
		    ** recorded, so save it 
		    */
		    run_status = op_status;
		    STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	        }
	    }
	}
	else
	    op_status = E_DB_OK;

	if (DB_SUCCESS_MACRO(op_status))
	{
	    /* 
	    ** only release object if we can get the semaphore.  
	    */
	    if (global->rdf_ulhobject && global->rdf_ulhobject->rdf_sysinfoblk) 
		((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk
			->rdr_object)->rdf_state = RDF_SSHARED;
	    else if (global->rdf_init & RDF_IULH)
	    {
		if (global->rdf_ulhcb.ulh_object->ulh_uptr)
		    ((RDF_ULHOBJECT *) global->rdf_ulhcb.ulh_object->ulh_uptr)
			    ->rdf_state = RDF_SSHARED;
	    }
	    global->rdf_resources &= (~RDF_RUPDATE);	
	}
    }

    if (global->rdf_resources & RDF_D_RUPDATE)
    {   
	/* 
	** master copy of LDBcache object has been reserved for update by
	** this thread so release it 
	*/
	op_status = rdd_setsem(global,RDF_BLD_LDBDESC); /* get semaphore
				** prior to updating ULH object descriptor */
	if (op_status > run_status)
	{
	    /* this error is more serrious than any previously 
	    ** recorded, so save it 
	    */
	    run_status = op_status;
	    STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	}
	else
	{
	    /* 
	    ** only release object if we can get the semaphore.  If we
	    ** fall out of retry loop, then we will unconditionally release
	    ** this object before we exit. 
	    */
	    if (global->rdf_d_ulhobject)
		global->rdf_d_ulhobject->rdf_state = RDF_SSHARED;
	    else if (global->rdf_init & RDF_D_IULH)
	    {
		if (global->rdf_dist_ulhcb.ulh_object->ulh_uptr)
		    ((RDF_DULHOBJECT *) global->rdf_dist_ulhcb.ulh_object->
				ulh_uptr)->rdf_state = RDF_SSHARED;
	    }
	    /* FIXME - report error when object cannot be found */
	    global->rdf_resources &= (~RDF_D_RUPDATE);	
	}
    }

    if (global->rdf_resources & RDF_DE_RUPDATE)
    {   
	/* 
	** master copy of Defaults object has been reserved for update by
	** this thread so release it 
	*/
	op_status = rdd_setsem(global, RDF_DEF_DESC); /* get semaphore
				** prior to updating ULH object descriptor */
	if (op_status > run_status)
	{
	    /* 
	    ** this error is more serrious than any previously 
	    ** recorded, so save it 
	    */
	    run_status = op_status;
	    STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	}
	else
	{
	    /* 
	    ** only release object if we can get the semaphore.  
	    */
	    if (global->rdf_de_ulhobject)
	        global->rdf_de_ulhobject->rdf_state = RDF_SSHARED;
	    else if (global->rdf_init & RDF_DE_IULH)
	    {
	        if (global->rdf_def_ulhcb.ulh_object->ulh_uptr)
		    ((RDF_DULHOBJECT *) global->rdf_def_ulhcb.ulh_object->
				ulh_uptr)->rdf_state = RDF_SSHARED;
	    }
	    /* FIXME - report error when object cannot be found */
	    global->rdf_resources &= (~RDF_DE_RUPDATE);	
	}
    }

    if ((DB_FAILURE_MACRO(*status)) && 
        (global->rdf_resources & RDF_RINTEGRITY) &&
        (global->rdf_resources & RDF_RQEU))
    {   
	/* 
	** master copy of RDF integrity tuple buffer has been reserved for 
	** update by this thread so release it 
	*/
	op_status = rdu_qmclose(global,
		&global->rdf_ulhobject->rdf_sintegrity,
		&((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk
		    ->rdr_object)->rdf_sintegrity);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* this error is more serrious than any previously 
	        ** recorded, so save it 
	        */
	        run_status = op_status;
	        STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= (~RDF_RINTEGRITY);	
    }

    if ((DB_FAILURE_MACRO(*status)) && 
        (global->rdf_resources & RDF_RPROTECTION) &&
        (global->rdf_resources & RDF_RQEU))
    {   
	/* 
	** master copy of RDF protection tuple buffer has been reserved for 
	** update by this thread so release it 
	*/
	op_status = rdu_qmclose(global, &global->rdf_ulhobject->rdf_sprotection,
		&((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk->
		    rdr_object)->rdf_sprotection);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* 
		** this error is more serrious than any previously 
	        ** recorded, so save it 
	        */
	        run_status = op_status;
	        STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= (~RDF_RPROTECTION);	
    }

    if ((DB_FAILURE_MACRO(*status)) && (global->rdf_resources & RDF_RRULE) &&
        (global->rdf_resources & RDF_RQEU))
    {
	/*
	** Master copy of RDF rule tuple buffer has been reserved for 
	** update by this thread so release it.
	*/
	op_status = rdu_qmclose(global,
		    &global->rdf_ulhobject->rdf_srule,
		    &((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk->
							rdr_object)->rdf_srule);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* this error is more serrious than any previously 
	        ** recorded, so save it 
	        */
	        run_status = op_status;
	        STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= (~RDF_RRULE);
    }

    if ((DB_FAILURE_MACRO(*status)) && 
        (global->rdf_resources & RDF_RPROCEDURE_PARAMETER) &&
        (global->rdf_resources & RDF_RQEU))
    {
	/*
	** Master copy of RDF rule tuple buffer has been reserved for 
	** update by this thread so release it.
	*/
	op_status = rdu_qmclose(global,
		    &global->rdf_ulhobject->rdf_sprocedure_parameter,
		    &((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk->
					rdr_object)->rdf_sprocedure_parameter);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* 
		** this error is more serrious than any previously 
	        ** recorded, so save it 
	        */
	        run_status = op_status;
	        STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= (~RDF_RPROCEDURE_PARAMETER);
    }

    if ((DB_FAILURE_MACRO(*status)) && (global->rdf_resources & RDF_RKEYS) &&
        (global->rdf_resources & RDF_RQEU))
    {
	/*
	** Master copy of RDF rule tuple buffer has been reserved for 
	** update by this thread so release it.
	*/
	op_status = rdu_qmclose(global,
		    &global->rdf_ulhobject->rdf_skeys,
		    &((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk->
							rdr_object)->rdf_skeys);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
		/* 
		** this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= (~RDF_RKEYS);
    }

    if (global->rdf_resources & RDF_RQTUPDATE)
    {   
	/* 
	** master copy of RDF query tree buffer has been reserved for 
	** update by this thread.  The tree was not built successfully,
	**  so destroy it (fixes bug 57577)
	*/
	op_status = ulh_destroy(&global->rdf_ulhcb,(unsigned char *) NULL,
			           (i4)0 );
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    /* 
	    ** fix bug 57677 -- assure that RDF_RULH is cleared when 
	    ** dealing with RDF_RQTUPDATE.  
	    */
	    global->rdf_resources &= (~ (RDF_RULH | RDF_RQTUPDATE) );
    }

    if (global->rdf_resources & RDF_RSEMAPHORE)
    {	
	/* release semaphore if it is held */
	op_status = rdu_rsemaphore(global);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= (~RDF_RSEMAPHORE);
    }

    if ((global->rdf_resources & RDF_RQEU) &&
	(!(global->rdf_resources & RDF_RMULTIPLE_CALLS) ||
				    /* 
				    ** if the file is to be left open
				    ** over several calls then
				    ** do not close the resource 
				    */
	  DB_FAILURE_MACRO(*status))) /* close in case of an error */
    {   
	/* close file if it is still open */
	op_status = rdu_qclose(global);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= (~RDF_RQEU);
    }

    if ((DB_FAILURE_MACRO(*status)) && (global->rdf_resources & RDF_DULH))
    {  
        /* force re-initialization of ulh control block -- bug 60730 */
	global->rdf_init &= ~RDF_D_IULH;

	if (global->rdf_resources & RDF_BAD_LDBDESC)
		op_status = rdu_i_dulh(global);   /* destroy ulh object */
	else
		op_status = rdu_r_dulh(global);   /* unfix the ulh object */
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
		/* 
		** this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= ~(RDF_DULH | RDF_BAD_LDBDESC);
    }

    if ((DB_FAILURE_MACRO(*status)) && (global->rdf_resources & RDF_DEULH))
    {  
	if (global->rdf_resources & RDF_BAD_DEFDESC)
	    op_status = rdu_i_deulh(global, global->rdfcb->rdf_info_blk,0);   /* destroy ulh object */
	else
	    op_status = rdu_r_deulh(global, global->rdfcb->rdf_info_blk, 0);   /* unfix the ulh object */
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
		/* 
		** this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= ~(RDF_DEULH | RDF_BAD_DEFDESC);
    }

    /* Note from MING:
    ** Allow exception is a bad habit.  Anyway, let me explain why.  
    ** Normally, if error occurs at the time we hold a ulh object, we simply
    ** release the object and return status.  Caller should not use the 
    ** object info at all.  However, OPF, wants to continue processing the 
    ** query even if index info is not consistent. So we cannot release the 
    ** object whenever error code is E_RD026A_OBJ_INDEXCOUNT.  We may want 
    ** to fix this later. 
    */
    if ((DB_FAILURE_MACRO(*status)) && (global->rdf_resources & RDF_RULH)
	    && (error.err_code != E_RD026A_OBJ_INDEXCOUNT))
    {  
	/* 
	** Assure that the ULHCB is reinitialized for the RDF_INVALIDATE
	** case, since it plays with the rdf_ulhcb after it has been 
	** initialized.
	*/
	if (global->rdf_operation == RDF_INVALIDATE)
		global->rdf_init &= ~RDF_IULH;

	/* 
	** if RDF_CLEANUP was set, this means cached object is no longer 
	** trustable, so destroy it.  The trick here is that once RDF set
	** return status to error, the caller should never try to
	** use the object again.  Be careful about some other tricks using 
	** ulh_destroy.  If we have the access to the object, we can go 
	** ahead to destroy the object without releasing it first. 
	** ulh_release will null the object ptr which means a ulh_destroy 
	** following ulh_release will remove the first element from the lru 
	** and this is not what we want 
	*/
	if (global->rdf_resources & RDF_CLEANUP)
		op_status = rdu_iulh(global);   /* destroy ulh object */
	else
	{
	    /* 
	    ** decrement fix counter and unfix the ulh object --
	    ** the rdf_ulhobject will usually be found in 
	    ** global->rdfcb->rdf_info_blk->rdr_object, but under certain
	    ** error paths this may not be set yet.  If it is not set, then
	    ** look for a newly fixed/created ulh object in RDF's ULH_CB.
	    ** (This fixes bug 56677, which always expected the ulhobject
	    ** in global->rdfcb->rdf_info_blk and Av'ed if it was not there.
	    */
	    RDF_ULHOBJECT	*ulhobj = 0;

	    if (global->rdfcb->rdf_info_blk)
	        ulhobj = 
		     (RDF_ULHOBJECT *)global->rdfcb->rdf_info_blk->rdr_object;
	    else if (global->rdf_ulhcb.ulh_object)
	    {
	        ulhobj = (RDF_ULHOBJECT *)
			         global->rdf_ulhcb.ulh_object->ulh_uptr;
	        /* 
		** fix bug 60730 -- was storing ptr to (RDF_ULHOBJECT
	        ** instead of ptr to RDR_INFO object in rdf_info_blk 
		*/
	        if (ulhobj)
	            global->rdfcb->rdf_info_blk =  ulhobj->rdf_sysinfoblk;
	    }

	    if (! global->rdf_ulhobject)
	    {
	        /* 
		** routine rdu_rulh requires that this be initialized,
	        ** and some paths through the code may error out before
	        ** it is set (another aspect of bug 56677).  If this
	        ** has not been initialized, then go ahead and 
	        ** initialize it.
	        */
	        global->rdf_ulhobject = ulhobj; 
	    }

	    if (ulhobj)
	    {
	        /* 
		** fix bug 57677 -- don't decrement rdf session counter
	        ** flag if we're in error retry -- it messes up the counter
	        */

		op_status = rdu_gsemaphore(global);
		if (DB_FAILURE_MACRO(op_status))
		{
		    *status = op_status;
		    return;
		}
		
	        CSadjust_counter(&ulhobj->rdf_shared_sessions, -1);

/*
		if (ulhobj->rdf_shared_sessions < 0)
		{
		    TRdisplay ("%@   Error:  rdf_shared_sessions = %d.  Resetting to 0\n",ulhobj->rdf_shared_sessions); 
		    ulhobj->rdf_shared_sessions = 0;
		}
*/
		op_status = rdu_rsemaphore(global);
		if (DB_FAILURE_MACRO(op_status))
		{
		    *status = op_status;
		    return;
		}

	        op_status = rdu_rulh(global);
	        /* 
		** we still have semaphore protection, so clear the 
	        ** RDR2_DEFAULT resource bit from this object 
	        */
	        global->rdfcb->rdf_info_blk->rdr_2_types &= ~RDR2_DEFAULT;
	    }
	    else
	    {
	        /* 
		** if we canot find the rdf_ulhobject for this resource,
	        ** (which theoritically should never happen), then we cannot
	        ** release it.  So, go ahead and set the status as though
	        ** we had infact released it and unset the RDF_RULH and
	        ** RDF_CLEANUP resources 
	        */
	        op_status = E_DB_OK;

	    }
        }
	    
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
	        /* 
		** this error is more serrious than any previously 
	        ** recorded, so save it 
	        */
	        run_status = op_status;
	        STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	    global->rdf_resources &= ( ~(RDF_RULH | RDF_CLEANUP) );
    }

    if ((DB_FAILURE_MACRO(*status))
            &&
            (global->rdf_resources & RDF_INVAL_FIXED)
	   )
    {
	/* 
	** Assure that the ULHCB is reinitialized for the RDF_INVALIDATE
	** case, since it plays with the rdf_ulhcb after it has been 
	** initialized.
	*/
	if (global->rdf_operation == RDF_INVALIDATE)
		global->rdf_init &= ~RDF_IULH;
    
	global->rdfcb->rdf_info_blk = global->rdf_fix_toinvalidate;
	/* we still have semaphore protection, so clear the RDR2_DEFAULT
	** resource bit from this object 
	*/
	global->rdfcb->rdf_info_blk->rdr_2_types &= ~RDR2_DEFAULT;
	op_status = rdu_rulh(global);
	if (DB_FAILURE_MACRO(op_status))
	{
		/* report error if one does not exist */
		if (op_status > run_status)
		{
		    /* this error is more serrious than any previously 
		    ** recorded, so save it 
		    */
		    run_status = op_status;
		    STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
		}
	}
	else
		global->rdf_resources &= ~RDF_INVAL_FIXED;
    }
    if ((DB_FAILURE_MACRO(*status)) && 
	    (global->rdf_resources & RDF_RPRIVATE)
	   )
    {   
	/* 
	** a private memory stream was created which should be deleted
	** if an error occurs, otherwise it should remain since it will
	** contain the relation descriptor information 
	*/

	RDR_INFO *sys_infoblk;

	/*
	** save the system info block pointer before closing the private
	** memory stream to avoid any race condition
	*/
	sys_infoblk = ((RDF_ULHOBJECT *)global->rdfcb->rdf_info_blk->rdr_object)
			->rdf_sysinfoblk;
 
	op_status = rdu_dstream(global, (PTR)NULL, (RDF_STATE *)NULL);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
		/* 
		** this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
	else
	{
	    global->rdf_resources &= (~RDF_RPRIVATE);

	    /*
	    ** the private info block was deallocated when the private 
	    ** memory stream was closed - need to reset the rdf_info_blk 
	    ** (which gets returned to the caller of RDF through rdf_cb) 
	    ** to point to the system info block.  (B76130)
	    */
	    global->rdfcb->rdf_info_blk = sys_infoblk;
	} 
    }

    if (global->rdf_ddrequests.rdd_ddstatus & RDD_SEMAPHORE)
    {  
	/* 
	** semaphore is hold for ldb descriptor cache when error occur.
	** release the semaphore and continue 
	*/
	op_status = rdd_resetsem(global,RDF_BLD_LDBDESC);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
		/* this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
    }
    if (global->rdf_resources & RDF_DSEMAPHORE)
    {  
	/* 
	** semaphore is held for defaults cache when the error occurred.
	** release the semaphore and continue 
	*/
	op_status = rdd_resetsem(global, RDF_DEF_DESC);
	if (DB_FAILURE_MACRO(op_status))
	{
	    /* report error if one does not exist */
	    if (op_status > run_status)
	    {
		/* 
		** this error is more serrious than any previously 
		** recorded, so save it 
		*/
		run_status = op_status;
		STRUCT_ASSIGN_MACRO(global->rdfcb->rdf_error, error);
	    }
	}
    }

    if (global->rdf_resources & RDF_RUPDATE)
    {
	/* 
	** We had update permission on this object.  Therefore, we tried to get
	** a semaphore to release this object, but were unable to do so.  This
	** indicates a fairly serrious system failure.
	** Try to clean up by releasing this object from update by this thread.
	** In this obscure error case, we do not worry about semaphore
	** protection.
	*/
	if (global->rdf_ulhobject &&
	    global->rdf_ulhobject->rdf_sysinfoblk)
	    ((RDF_ULHOBJECT *)global->rdf_ulhobject->rdf_sysinfoblk
		->rdr_object)->rdf_state = RDF_SSHARED;
	else if (global->rdf_init & RDF_IULH)
	{
	    if (global->rdf_ulhcb.ulh_object->ulh_uptr)
		((RDF_ULHOBJECT *) global->rdf_ulhcb.ulh_object->ulh_uptr)
		    ->rdf_state = RDF_SSHARED;
	}
    }

    /* when we get here, the most severe error encountered will be in 
    ** run_status and the data (err_code, err_data) assocaited with it will
    ** be in local variable error.  If run_status is not E_DB_OK, then update
    ** *status and copy the error data (err_code, err_data) from error back
    ** to global->rdfcb->rdf_error.err_code
    */
    if (run_status > *status)
    {
	STRUCT_ASSIGN_MACRO(error, global->rdfcb->rdf_error);
	*status = run_status;
    }
}

/*
** Name  rdf_cleanup	- clean up the status for this operation
**
** Description:
**	This routine cleans up the distributed status caused by current 
**	operation.
**
** Inputs:
**      global			ptr to RDF state variable
**	    .rdf_ddrequests	distributed info request block
**		.rdd_ddstatus	distributed status indicator
**
** Outputs:
**      none
**	Returns:
**	    EX_DECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	07-feb-82 (teresa)
**	    Drop in for SYBIL merge from STAR line
**	16-jul-92 (teresa)
**	    implemented prototypes
*/
static STATUS
rdf_cleanup(RDF_GLOBAL 	*global)
{
    switch (global->rdf_ddrequests.rdd_ddstatus)
    {
    case RDD_FETCH: 
	return(rdd_flush(global)); /* this is the case when error occurs during
				   ** tuple fetching operation */ 
    default:
        return (E_DB_OK); /* return ok */
    }
}

/*{
** Name: rdf_call - Call a relation description operation.
**
**	External call format:	status = rdf_call(op_code, &rdf_cb);
**
** Description:
**      Rdf_call() is the entry point to RDF for requesting 
**	information of a table. The interface to RDF is through this
**	function call. The function is called with a RDF operation code and
**	a call control block which varies depending on the type of operation.
**
**      One constraint is that a semaphore cannot be held over a DMF call
**      since then undetectable deadlocks will occur and the system hangs.
**      Unfortunately, this complicates the RDF management of a relation
**      descriptor in a multi-threaded environment.  In particular, a
**      descriptor can be built by one thread and concurrently built
**      by another since "windows" exist during a call to DMF when the
**      semaphore is released.  The solution is to detect that
**      a descriptor is being updated by one thread, and not to update
**      that descriptor, but to create a totally new descriptor which will
**      be destroyed when the query processing is complete.  The hope
**      is that the cache will quickly reach a steady state.
**	
** Inputs:
**      opcode                          Code representing the operation to be
**					performed.
**      rdf_cb                          Pointer to the control block 
**					containing the input parameters. 
**
** Outputs:
**      rdf_cb                          Pointer to the control block 
**					containing the output information
**					and error statuses.
**
**          	.rdf_error              Filled with error if encountered.
**
**			.err_code	The error number, if any.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**	    Depends on operation.
**
** Side Effects:
**	    Depends on operation.
**
** History:
**	28-mar-86 (ac)
**          written
**	12-jan-87 (puree)
**	    add rdf_invalidate()
**	24-mar-87 (puree)
**	    add utility calls.
**      02-May-90 (teg)
**          fix bug 21489 - RDF can get into endless loop if it AVs during
**          rdu_ierror() call for E_RD0126_EXEXCEPTION message.
**	31-jan-91 (teresa)
**	    add logic to keep a running count of RDF exceptions if we have
**	    access to SVCB when exception handler is evoked.
**	18-jun-91 (teresa)
**	    fixed a star bug where rdf was not initializing some fields in global
**	    before using them.  Replaced init of several fields to zero or ""
**	    with an MEfill of the whole structure with zero.
**	28-feb-92 (teresa)
**	    added a call to ulm_xclean() when processing an exception because I
**	    was poking around ULM code and found that this should be called.
**	16-jul-92 (teresa)
**	    implemented prototypes
**	29-jul-92 (teresa)
**	    removed logic to set timestamp and keep track of autocommit for
**	    the following OP_CODES since RDF no longer uses the timestamp:
**		RDF_AUTOCOMMIT, RDF_COMMIT, RDF_NO_AUTOCOMMIT, RDF_TRANSACTION.
**	14-aug-92 (teresa)
**	    remove work-around to force in local mode.
**	22-sep-92 (teresa)
**	    put ptr to session control block in global if needed.  Also change
**	    interface to rdu_hist_dump.
**	02-oct-92 (teresa)
**	    add handling for RDF_END_SESSION, which was somehow missed during
**	    original sybil merge.  (I think this is not currently called, but
**	    I am not sure.)
**	07-jan-93 (andre)
**	    added support for RDF_PACK_QTEXT and RDF_PACK_QTREE opcodes
**	07-jan-93 (teresa)
**	    Add support for caller supplied ULM_CB.
**	29-jan-93 (teresa)
**	    Add suport for new op codes: RDF_READTUPLES
*/
DB_STATUS
rdf_call(   i4		op_code,
	    void *	rdf_cb)
{
    DB_STATUS           status= E_DB_OK;
    RDF_GLOBAL          global;		/* state of rdf processing, contains
                                        ** control blocks, and resource usage
                                        */
    EX_CONTEXT		excontext;	/* context block for exception handler*/
    i2                  ex_cnt;         /* counter to detect for endless loops
                                        ** in exception handler and break out
                                        ** of them.
                                        */
    RDF_CB		*cb;
    RDF_CCB		*ccb;
#ifdef PERF_TEST
    CSfac_stats(TRUE, CS_RDF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(TRUE, CS_RDF_ID, op_code, rdf_cb);
#endif


    MEfill( sizeof(global),'\0', (PTR) &global);    /* init global */

    global.rdf_operation = op_code;	/* save operation for error reporting*/

    /* Set default ULM stream allocation to thread-shareable streams */
    global.rdf_ulmcb.ulm_flags = ULM_SHARED_STREAM;

    if ( EXdeclare(rdf_handler, &excontext) != OK)
    {
	EXdelete();			/* set up new exception to avoid
                                        ** recursive exceptions */
	ex_cnt = 0;
	if ( EXdeclare(rdf_handler, &excontext) != OK)
	{
	   /* if exception handler called again then do not do anything
	    ** except exit as quickly as possible*/
	    status = E_DB_FATAL;

           /* its possible to get into an endless loop if an exception occurs
            ** in rdu_ierror while trying to report message E_RD0126_EXEXCEPTION
            ** SO, add a counter to prevent that from occurring.
            */
            if (ex_cnt)
            {
                EXdelete();
                return(status);
            }
            else
                ex_cnt++;

	    rdu_ierror(&global, status, E_RD0126_EXEXCEPTION); /* exception
					** while processing exception */

	    /* increment exception counter if we have the address of the 
	    ** svcb */
	    Rdi_svcb->rdvstat.rds_n7_exceptions++;

	    /* do not try to deallocate resources since an exception
	    ** occurred when this was attempted below */
	}
	else
	{   /* attempt to release RDF resources */

	    status = E_DB_SEVERE;
	    rdu_ierror(&global, status, E_RD0125_EXCEPTION);

	    /* increment exception counter if we have the address of the 
	    ** svcb */
	    Rdi_svcb->rdvstat.rds_n7_exceptions++;

	    /* release any ulm semaphore this thread may hold */
            if (global.rdf_ulmcb.ulm_poolid)
                (VOID) ulm_xcleanup(&global.rdf_ulmcb);

	    /* release any resources held by this thread */
	    if (global.rdf_resources)
		rdf_release(&global, &status); 
	}
    }
    else if ( op_code <= RDF_MAX_OP )
    {
	if ( Rdf_call_info[op_code].cb_type == RDF_CB_TYPE )
	{
	    cb = global.rdfcb = (RDF_CB*)rdf_cb;

	    global.rdf_sess_id = cb->rdf_rb.rdr_session_id;

	    if ( Rdf_call_info[op_code].caller_ulm_cb &&
		 cb->rdf_rb.rdr_instr & RDF_USE_CALLER_ULM_RCB )
	    {
		global.rdf_fulmcb = (ULM_RCB *) cb->rdf_rb.rdf_fac_ulmrcb;
	    }
	}
	else if ( Rdf_call_info[op_code].cb_type == RDF_CCB_TYPE )
	{
	    ccb = global.rdfccb = (RDF_CCB*)rdf_cb;

	    global.rdf_sess_id = ccb->rdf_sess_id;
	}
	else 
	{
	    rdu_ierror(&global, E_DB_ERROR, E_RD0003_BAD_PARAMETER);
	    return(E_DB_ERROR);
	}

	if ( Rdf_call_info[op_code].need_sess_cb )
	{
	    global.rdf_sess_cb = GET_RDF_SESSCB(
			    global.rdf_sess_id);
	    global.rdf_qeucb.qeu_d_id = global.rdf_sess_id;
	    global.rdf_qefrcb.qef_sess_id = global.rdf_sess_id;
	}

	/* now process the requested operation */
	switch (op_code)
	{
	case RDF_GETDESC:
	    status = rdf_gdesc(&global, cb);
	    break;

	case RDF_UNFIX:
	    status = rdf_unfix(&global, cb);
	    break;

	case RDF_GETINFO:
	    status = rdf_getinfo(&global, cb);
	    break;

	case RDF_READTUPLES:
	    status = rdf_readtuples(&global, cb);
	    break;

	case RDF_UPDATE:
	    status = rdf_update(&global, cb);
	    break;

	case RDF_INVALIDATE:
	    status = rdf_invalidate(&global, cb);
	    break;

	case RDF_INITIALIZE:
	    status = rdf_initialize(&global, ccb);
	    break;

	case RDF_TERMINATE:
	    status = rdf_terminate(&global, ccb);
	    break;

	case RDF_STARTUP:
	    status = rdf_startup(&global, ccb);
	    break;

	case RDF_BGN_SESSION:
	    status = rdf_bgn_session(&global, ccb);
	    break;

	case RDF_END_SESSION:
	    status = rdf_end_session(&global, ccb);
	    break;

	case RDF_SHUTDOWN:
	    status = rdf_shutdown(&global, ccb);
	    break;

	case RDF_ULM_TRACE:
	    {
	        ULM_RCB         ulm_rcb;
	        RDI_FCB         *rdi_fcb;

	        rdi_fcb = (RDI_FCB *)ccb->rdf_fcb;

	        ulm_rcb.ulm_poolid = Rdi_svcb->rdv_poolid;
	        ulm_rcb.ulm_memleft = &Rdi_svcb->rdv_memleft;
	        ulm_rcb.ulm_facility = DB_RDF_ID;
	        ulm_rcb.ulm_streamid_p = &rdi_fcb->rdi_streamid;
	        status = ulm_trace(&ulm_rcb);
	        break;
	    }

        case RDF_DDB: 
            status = rdf_ddb(&global, cb);
            break;

	case RDF_AUTOCOMMIT:
	case RDF_COMMIT:
	case RDF_NO_AUTOCOMMIT:
	case RDF_TRANSACTION:
	    /* there nothing to do for these calls, so just break and return */
	    break;

	case RDF_PACK_QTEXT:
	    status = rdf_qtext_to_tuple(&global, cb);
	    break;

	case RDF_PACK_QTREE:
	    status = rdf_qtree_to_tuple(&global, cb);
	    break;

	case RDF_PART_COPY:
	    status = rdp_copy_partdef(&global, cb);
	    break;

	case RDF_PART_UPDATE:
	    status = rdp_update_partdef(&global, cb);
	    break;

	case RDF_PART_FINISH:
	    status = rdp_finish_partdef(&global, cb);
	    break;

	case RDF_PART_COMPAT:
	    status = rdp_part_compat(&global, cb);
	    break;


#ifdef xDEV_TEST
	
	/*
	**  The following cases are for RDF test suite.
	*/
	case RDU_INFO_DUMP:
	    rdu_info_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;

	case RDU_REL_DUMP:
	    rdu_rel_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;

	case RDU_ATTR_DUMP:
	    rdu_attr_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;

	case RDU_INDX_DUMP:
	    rdu_indx_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;

	case RDU_ATTHSH_DUMP:
	    rdu_atthsh_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;

	case RDU_KEYS_DUMP:
	    rdu_keys_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;

	case RDU_HIST_DUMP:
	{
	    RDR_INFO	*info=(RDR_INFO *) (cb->rdf_info_blk);

	    rdu_hist_dump(info, 1, info->rdr_no_attr);
	    break;
	}
	case RDU_PTUPLE_DUMP:
	    rdu_ptuple_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;

	case RDU_ITUPLE_DUMP:
	    rdu_ituple_dump((RDR_INFO *) (cb->rdf_info_blk));
	    break;
#endif
	default:
	    status = E_DB_ERROR;
	    rdu_ierror(&global, status, E_RD0003_BAD_PARAMETER);
	    break;
	}
	if ((global.rdf_resources)
	    ||
	    (global.rdf_ddrequests.rdd_ddstatus & RDD_SEMAPHORE)
	   )
	    rdf_release(&global, &status); /* release any resources held by
					    ** this thread */

	/* see if its necesary to clean up distributed processing */
	if (global.rdf_ddrequests.rdd_ddstatus)
	{
	    DB_STATUS	    c_status;

	    c_status = rdf_cleanup(&global);	   
	    if (c_status > status)
		status = c_status;  /* use highest priority error code */
	}

    }
    EXdelete();
#ifdef PERF_TEST
    CSfac_stats(FALSE, CS_RDF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(FALSE, CS_RDF_ID, op_code, rdf_cb);
#endif

    return(status);
}
